package er.skill

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.BATTLE_SKILLS
import er.GeneratorUtils.BATTLE_SKILL_TEMPLATES
import er.GeneratorUtils.INLINE_SKILLS
import er.proto.AbilityEnum
import er.proto.BattleSkillTemplate
import er.proto.BattleSkillTemplate.BattleSkillTemplateParam
import er.proto.BattleSkillTemplateEnum
import er.proto.InlineSkill
import er.proto.InlineSkill.SetHazard.Hazard.HAZARD_NONE
import er.proto.MoveEnum
import er.proto.Stat
import java.io.OutputStreamWriter

object BattleSkillGenerator : Generator {
  // Same as in src/battle_skills.cc
  private const val INLINE_MAX_EFFECTS = 4

  private val PREFIX = "__sSkillText_"

  override fun generate(writer: OutputStreamWriter) {
    val allStrings =
      (BATTLE_SKILLS.map { it.name } + BATTLE_SKILLS.map { it.announceString }).toSet() +
        BATTLE_SKILL_TEMPLATES.map { it.name } +
        BATTLE_SKILL_TEMPLATES.flatMap {
          it.validParamsList.map { params -> params.announceString }
        } +
        INLINE_SKILLS.flatMap { (id, skill) -> skill.texts(id).toList() }

    writer.appendLine(
      allStrings.joinToString("\n") {
        "constexpr u8 $PREFIX${it.hashCode().toUInt()}[] = _(\"$it\");"
      }
    )

    writer.appendLine(
      """
      |template <BattleSkillEnum Id>
      |constexpr BattleSkill BattleSkillStrings = {0};
      |"""
        .trimMargin()
    )

    writer.appendLine(
      BATTLE_SKILLS.joinToString("\n") {
        """
        |template <>
        |constexpr BattleSkill BattleSkillStrings<${it.id}> = {
        |$IND.name = $PREFIX${it.name.hashCode().toUInt()},
        |$IND.announceString = $PREFIX${it.announceString.hashCode().toUInt()},
        |};
        |"""
          .trimMargin()
      }
    )

    for ((id, skill) in INLINE_SKILLS) {
      check(skill.onCount > 0 || skill.extraAbilityCount > 0) {
        "$id: inline skill with neither an `on` block nor an extra_ability"
      }
      // Each stat change and hazard is one battle script on the stack (20 slots), shared with the other skills
      for ((hook, ons) in skill.onList.groupBy { it.kindCase }) {
        val scripts = ons.sumOf { on -> on.effectList.sumOf { it.statChange.statCount + it.hazard.hazardCount + if (it.hasWeather() || it.hasTerrain()) 2 else 0 } }
        check(scripts <= 10) { "$id: too many effects at once on ${hook.name.lowercase()}" }
        // The extra move queue has 4 slots, shared with abilities
        val moves = ons.sumOf { on -> on.effectList.count { it.hasUseMove() } }
        check(moves <= 4) { "$id: $moves use_move on ${hook.name.lowercase()}, max is 4" }
      }
      val (name, announceString) = skill.texts(id)
      writer.appendLine(
        """
        |template <>
        |constexpr BattleSkill BattleSkillStrings<$id> = {
        |$IND.name = $PREFIX${name.hashCode().toUInt()},
        |$IND.announceString = $PREFIX${announceString.hashCode().toUInt()},
        |};
        |
        |template <>
        |constexpr BattleSkill Impl<$id> = InlineSkill<${(listOf(skill.abilitiesToCpp(id)) + skill.onList.map { it.toCpp(id) }).joinToString()}>;
        |"""
          .trimMargin()
      )
    }

    for (template in BATTLE_SKILL_TEMPLATES) {
      val validOptions =
        template.validParamsList.associate {
          it.paramList.map { param -> param.paramCase } to it.announceString
        }

      for (impl in template.implList) {
        val announceString =
          validOptions[impl.paramList.map { it.paramCase }]
            ?: error(
              "Invalid parameter list for ${impl.id} using template ${template.id}: ${impl.paramList}"
            )

        writer.appendLine(
          """
          |template <>
          |constexpr BattleSkill BattleSkillStrings<${impl.id}> = {
          |$IND.name = $PREFIX${template.name.hashCode().toUInt()},
          |$IND.announceString = $PREFIX${announceString.hashCode().toUInt()},
          |};
          |"""
            .trimMargin()
        )

        fun BattleSkillTemplateParam.string() =
          when (this.paramCase) {
            BattleSkillTemplateParam.ParamCase.ABILITY -> ability.toString()
            BattleSkillTemplateParam.ParamCase.PARAM_NOT_SET ->
              error("Param without value: $impl")
          }

        writer.appendLine(
          """
          |template <>
          |constexpr BattleSkill Impl<${impl.id}> = BattleSkillTemplate<${template.id}, ${impl.paramList.joinToString { it.string() }}>;
          |"""
            .trimMargin()
        )
      }
    }
  }

  // name and announce_string, from the "Extra Ability" template when a skill only grants abilities
  private fun InlineSkill.texts(skillId: String): Pair<String, String> {
    if (extraAbilityCount == 0 || (name.isNotEmpty() && announceString.isNotEmpty())) {
      return name to announceString
    }
    // The default announce is about the abilities, it would be wrong each time a trigger fires
    check(onCount == 0) { "$skillId: needs a name and an announce_string with both `on` and extra_ability" }
    val template = BATTLE_SKILL_TEMPLATES.first { it.id == BattleSkillTemplateEnum.SKILL_TEMPLATE_EXTRA_ABILITY }
    val announce =
      template.validParamsList.firstOrNull { it.paramCount == extraAbilityCount }?.announceString
        ?: error("$skillId: no \"Extra Ability\" announce for $extraAbilityCount abilities")
    return name.ifEmpty { template.name } to announceString.ifEmpty { announce }
  }

  // extra_ability as an InlineAbilities, see battle_skills.cc
  private fun InlineSkill.abilitiesToCpp(skillId: String): String {
    check(extraAbilityList.none { it == AbilityEnum.ABILITY_NONE || it == AbilityEnum.UNRECOGNIZED }) {
      "$skillId: extra_ability without an ability"
    }
    return "InlineAbilities{.count = $extraAbilityCount, .ability = {${extraAbilityList.joinToString { it.name }}}}"
  }

  // An `on` block as an InlineTrigger, see battle_skills.cc
  private fun InlineSkill.Trigger.toCpp(skillId: String): String {
    val kind =
      when (kindCase) {
        InlineSkill.Trigger.KindCase.ENTRY -> "ENTRY"
        InlineSkill.Trigger.KindCase.END_TURN -> "END_TURN"
        else -> error("$skillId: `on` without a trigger")
      }
    check(effectCount in 1..INLINE_MAX_EFFECTS) { "$skillId: `on` needs between 1 and $INLINE_MAX_EFFECTS effects" }
    val effects = effectList.joinToString { "{${it.toCpp(skillId)}}" }
    return "InlineTrigger{.kind = InlineTrigger::$kind, .when = InlineTrigger::${entry.`when`}, .effectCount = $effectCount, .effects = {$effects}}"
  }

  private fun InlineSkill.Effect.toCpp(skillId: String): String =
    when (kindCase) {
      InlineSkill.Effect.KindCase.STAT_CHANGE -> {
        val change = statChange
        check(change.statCount in 1..7) { "$skillId: stat_change needs between 1 and 7 stats" }
        check(change.statList.none { it == Stat.STAT_HP || it == Stat.UNRECOGNIZED }) {
          "$skillId: stat_change on ${change.statList} isn't a stat"
        }
        check(change.statList.distinct().size == change.statCount) { "$skillId: stat_change lists a stat twice" }
        check(change.amount in -6..6 && change.amount != 0) {
          "$skillId: stat_change amount ${change.amount} isn't between -6 and 6, or is 0"
        }
        ".kind = InlineEffect::STAT_CHANGE, .statCount = ${change.statCount}, .stats = {${change.statList.joinToString { it.name }}}, .amount = ${change.amount}"
      }
      InlineSkill.Effect.KindCase.HAZARD -> {
        check(hazard.hazardCount > 0) { "$skillId: hazard without a hazard" }
        check(hazard.hazardList.none { it == HAZARD_NONE || it == InlineSkill.SetHazard.Hazard.UNRECOGNIZED }) {
          "$skillId: hazard on ${hazard.hazardList} isn't a hazard"
        }
        check(hazard.hazardList.distinct().size == hazard.hazardCount) { "$skillId: hazard lists a hazard twice" }
        check(hazard.layers in 0..3) { "$skillId: hazard layers ${hazard.layers} isn't between 1 and 3" }
        val bits = hazard.hazardList.joinToString(" | ") { "1 << SKILL_${it.name}" }
        ".kind = InlineEffect::HAZARD, .hazards = $bits, .layers = ${maxOf(hazard.layers, 1)}, .permanent = ${hazard.permanent}"
      }
      InlineSkill.Effect.KindCase.WEATHER -> {
        val name = weather.weather.name
        check(weather.weather != InlineSkill.SetWeather.Weather.WEATHER_NONE && weather.weather != InlineSkill.SetWeather.Weather.UNRECOGNIZED) {
          "$skillId: weather without a weather"
        }
        val primal = name in listOf("WEATHER_SUN_PRIMAL", "WEATHER_RAIN_PRIMAL", "WEATHER_STRONG_WINDS")
        check(!primal || weather.permanent) { "$skillId: $name can only be permanent" }
        ".kind = InlineEffect::WEATHER, .weather = ENUM_$name, ${weather.durationCase.toCpp(skillId, weather.turns, weather.permanent)}"
      }
      InlineSkill.Effect.KindCase.TERRAIN -> {
        check(terrain.terrain != InlineSkill.SetTerrain.Terrain.TERRAIN_NONE && terrain.terrain != InlineSkill.SetTerrain.Terrain.UNRECOGNIZED) {
          "$skillId: terrain without a terrain"
        }
        ".kind = InlineEffect::TERRAIN, .terrain = ENUM_${terrain.terrain.name}, ${terrain.durationCase.toCpp(skillId, terrain.turns, terrain.permanent)}"
      }
      InlineSkill.Effect.KindCase.USE_MOVE -> {
        check(useMove.move != MoveEnum.MOVE_NONE && useMove.move != MoveEnum.UNRECOGNIZED) { "$skillId: use_move without a move" }
        check(useMove.power in 0..255) { "$skillId: use_move power ${useMove.power} isn't between 0 and 255" }
        ".kind = InlineEffect::USE_MOVE, .move = ${useMove.move.name}, .power = ${useMove.power}, .falseSwipe = ${useMove.falseSwipe}"
      }
      else -> error("$skillId: effect without a kind")
    }

  // turns or permanent of a weather or terrain. The oneofs are generated per message, so this takes their name.
  private fun Enum<*>.toCpp(skillId: String, turns: Int, permanent: Boolean): String =
    when (name) {
      "TURNS" -> {
        check(turns in 1..255) { "$skillId: turns $turns isn't between 1 and 255" }
        ".turns = $turns"
      }
      "PERMANENT" -> {
        check(permanent) { "$skillId: permanent: false, use turns" }
        ".permanent = TRUE"
      }
      else -> error("$skillId: needs turns or permanent")
    }
}
