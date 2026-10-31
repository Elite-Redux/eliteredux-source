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
import er.proto.InlineSkill.Effect.KindCase as EffectKind
import er.proto.InlineSkill.Trigger.KindCase as TriggerKind
import er.proto.MoveEnum
import er.proto.Stat
import er.proto.Type
import java.io.OutputStreamWriter

object BattleSkillGenerator : Generator {
  // Same as in src/battle_skills.cc
  private const val INLINE_MAX_EFFECTS = 4
  private const val INLINE_MAX_ONCE = 4

  // Side conditions with layers instead of turns
  private val HAZARDS = listOf("SPIKES", "TOXIC_SPIKES", "STEALTH_ROCK", "STICKY_WEB", "CALTROPS")

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
      // Each stat change, side condition, field status and status is one battle script on the stack
      // (20 slots), shared with the other skills
      for ((hook, ons) in skill.onList.groupBy { it.kindCase }) {
        val scripts = ons.sumOf { on -> on.effectList.sumOf { it.scripts() } }
        check(scripts <= 10) { "$id: too many effects at once on ${hook.name.lowercase()}" }
        // The extra move queue has 4 slots, shared with abilities
        val moves = ons.sumOf { on -> on.effectList.count { it.hasUseMove() } }
        check(moves <= 4) { "$id: $moves use_move on ${hook.name.lowercase()}, max is 4" }
      }
      val onces = skill.onList.count { it.once != InlineSkill.Once.ALWAYS }
      check(onces <= INLINE_MAX_ONCE) { "$id: $onces `once` triggers, max is $INLINE_MAX_ONCE" }
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
        |constexpr BattleSkill Impl<$id> = InlineSkill<${(listOf(skill.abilitiesToCpp(id)) + skill.triggersToCpp(id)).joinToString()}>;
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

  private fun InlineSkill.triggersToCpp(skillId: String): List<String> {
    var onceIndex = 0
    return onList.map { it.toCpp(skillId, if (it.once == InlineSkill.Once.ALWAYS) 0 else onceIndex++) }
  }

  // An `on` block as an InlineTrigger, see battle_skills.cc. All the fields, in their order there.
  private fun InlineSkill.Trigger.toCpp(skillId: String, onceIndex: Int): String {
    val kind =
      when (kindCase) {
        TriggerKind.ENTRY -> "ENTRY"
        TriggerKind.END_TURN -> "END_TURN"
        TriggerKind.BATTLE_START -> "BATTLE_START"
        TriggerKind.HIT -> "HIT"
        TriggerKind.FAINT -> "FAINT"
        TriggerKind.STAT_CHANGE -> "STAT_CHANGE"
        TriggerKind.SWITCH_OUT -> "SWITCH_OUT"
        else -> error("$skillId: `on` without a trigger")
      }
    val what = "$skillId: ${kind.lowercase()}"
    check(effectCount in 1..INLINE_MAX_EFFECTS) { "$skillId: `on` needs between 1 and $INLINE_MAX_EFFECTS effects" }
    check(side != InlineSkill.Side.UNRECOGNIZED) { "$what: unknown side" }
    check(once != InlineSkill.Once.UNRECOGNIZED) { "$what: unknown once" }
    check(chance in 0..99) { "$what: chance $chance isn't between 1 and 99" }
    check(hpBelow in 0..99) { "$what: hp_below $hpBelow isn't between 1 and 99" }
    if (kindCase == TriggerKind.BATTLE_START) {
      check(side == InlineSkill.Side.TRAINER) { "$what: battle_start has no side" }
      check(once == InlineSkill.Once.ALWAYS) { "$what: battle_start already fires once" }
    }
    if (kindCase == TriggerKind.SWITCH_OUT) {
      check(effectList.all { it.hasUseMove() }) { "$what: switch_out can only use_move" }
    }
    // The skill fires in the middle of a move or a faint, the hazard scripts keep the battlers but a whole party
    // status would be too much there
    if (kindCase in listOf(TriggerKind.HIT, TriggerKind.FAINT, TriggerKind.STAT_CHANGE)) {
      check(effectList.none { it.hasStatus() && it.status.target.number >= InlineSkill.Target.TRAINER_PARTY.number }) {
        "$what: party status only on battle_start, entry and end_turn"
      }
    }

    val slots = entry.slots
    check(slots.first in 0..6 && slots.last in 0..6) { "$what: slots between 0 and 6" }
    check(endTurn.every in 0..255) { "$what: every ${endTurn.every} isn't between 1 and 255" }
    check(hit.typeList.none { it == Type.TYPE_NONE || it == Type.UNRECOGNIZED }) { "$what: hit on ${hit.typeList} isn't a type" }
    check(hit.typeList.distinct().size == hit.typeCount) { "$what: hit lists a type twice" }
    val types = hit.typeList.joinToString(" | ") { "1u << ${it.name}" }.ifEmpty { "0" }
    check(hit.effectivenessList.none { it == InlineSkill.Hit.Effectiveness.EFFECTIVENESS_UNSET || it == InlineSkill.Hit.Effectiveness.UNRECOGNIZED }) {
      "$what: hit on ${hit.effectivenessList} isn't an effectiveness"
    }
    check(hit.effectivenessList.distinct().size == hit.effectivenessCount) { "$what: hit lists an effectiveness twice" }
    val effectiveness = hit.effectivenessList.joinToString(" | ") { "1 << InlineTrigger::${it.name}" }.ifEmpty { "0" }
    check(statChange.statList.all { it.number in 1..7 }) { "$what: stat_change on ${statChange.statList} isn't a stat" }
    check(statChange.statList.distinct().size == statChange.statCount) { "$what: stat_change lists a stat twice" }
    val stats = statChange.statList.joinToString(" | ") { "1 << ${it.name}" }.ifEmpty { "0" }
    val direction =
      when (statChange.direction) {
        InlineSkill.StatChanged.Direction.UP -> 1
        InlineSkill.StatChanged.Direction.DOWN -> -1
        InlineSkill.StatChanged.Direction.ALL -> 0
        else -> error("$what: unknown direction")
      }

    val effects = effectList.joinToString { "{${it.toCpp(skillId)}}" }
    return "InlineTrigger{.kind = InlineTrigger::$kind, .when = InlineTrigger::${entry.`when`}, " +
      ".playerSide = ${side == InlineSkill.Side.PLAYER}, .once = InlineTrigger::${once.name}, .onceIndex = $onceIndex, " +
      ".chance = $chance, .hpBelow = $hpBelow, .slotsFirst = ${slots.first}, .slotsLast = ${slots.last}, " +
      ".every = ${endTurn.every}, .onField = ${endTurn.onField}, .types = $types, .contact = ${hit.contact}, " +
      ".effectiveness = $effectiveness, .stats = $stats, .direction = $direction, " +
      ".effectCount = $effectCount, .effects = {$effects}}"
  }

  // Battle scripts the effect can push, see the budget above
  private fun InlineSkill.Effect.scripts(): Int =
    when (kindCase) {
      EffectKind.STAT_CHANGE -> statChange.statCount
      EffectKind.SIDE_CONDITION ->
        sideCondition.typeCount * if (sideCondition.side == InlineSkill.SetSideCondition.TargetSide.BOTH_SIDES) 2 else 1
      EffectKind.FIELD -> field.typeCount
      EffectKind.WEATHER, EffectKind.TERRAIN -> 2
      // Two Pokemon on the field per side
      EffectKind.STATUS ->
        when (status.target) {
          InlineSkill.Target.BOTH_PARTIES -> 4
          InlineSkill.Target.TRAINER_PARTY, InlineSkill.Target.PLAYER_PARTY -> 2
          else -> 1
        }
      else -> 0
    }

  private fun InlineSkill.Effect.toCpp(skillId: String): String =
    when (kindCase) {
      EffectKind.STAT_CHANGE -> {
        val change = statChange
        check(change.statCount in 1..7) { "$skillId: stat_change needs between 1 and 7 stats" }
        check(change.statList.none { it == Stat.STAT_HP || it == Stat.UNRECOGNIZED }) {
          "$skillId: stat_change on ${change.statList} isn't a stat"
        }
        check(change.statList.distinct().size == change.statCount) { "$skillId: stat_change lists a stat twice" }
        check(change.amount in -6..6 && change.amount != 0) {
          "$skillId: stat_change amount ${change.amount} isn't between -6 and 6, or is 0"
        }
        check(change.target.number in 0..2) { "$skillId: stat_change target is EVENT_MON, TRAINER_MON or PLAYER_MON" }
        ".kind = InlineEffect::STAT_CHANGE, .target = InlineEffect::${change.target.name}, .statCount = ${change.statCount}, " +
          ".stats = {${change.statList.joinToString { it.name }}}, .amount = ${change.amount}"
      }
      EffectKind.SIDE_CONDITION -> {
        val condition = sideCondition
        val names = condition.typeList.map { it.name }
        check(condition.typeCount > 0) { "$skillId: side_condition without a type" }
        check(names.none { it == "NONE" || it == "UNRECOGNIZED" }) { "$skillId: side_condition on $names isn't a condition" }
        check(names.distinct().size == names.size) { "$skillId: side_condition lists a type twice" }
        check(condition.side != InlineSkill.SetSideCondition.TargetSide.UNRECOGNIZED) { "$skillId: unknown side" }
        val hazards = names.count { it in HAZARDS }
        check(hazards == 0 || hazards == names.size) { "$skillId: hazards and timed side conditions need their own side_condition" }
        val types = names.joinToString(" | ") { "1 << SKILL_SIDE_$it" }
        val side = ".side = InlineEffect::${condition.side.name.replace("DEFAULT", "DEFAULT_SIDE")}"
        if (hazards > 0) {
          check(condition.layers in 0..3) { "$skillId: hazard layers ${condition.layers} isn't between 1 and 3" }
          check(!condition.hasTurns()) { "$skillId: hazards have layers, not turns" }
          ".kind = InlineEffect::SIDE_CONDITION, .types = $types, $side, .layers = ${maxOf(condition.layers, 1)}, .permanent = ${condition.permanent}"
        } else {
          check(condition.layers == 0) { "$skillId: only hazards have layers" }
          ".kind = InlineEffect::SIDE_CONDITION, .types = $types, $side, ${condition.durationCase.toCpp(skillId, condition.turns, condition.permanent)}"
        }
      }
      EffectKind.FIELD -> {
        val names = field.typeList.map { it.name }
        check(field.typeCount > 0) { "$skillId: field without a type" }
        check(names.none { it == "NONE" || it == "UNRECOGNIZED" }) { "$skillId: field on $names isn't a field status" }
        check(names.distinct().size == names.size) { "$skillId: field lists a type twice" }
        val types = names.joinToString(" | ") { "1 << SKILL_FIELD_$it" }
        ".kind = InlineEffect::FIELD, .types = $types, ${field.durationCase.toCpp(skillId, field.turns, field.permanent)}"
      }
      EffectKind.WEATHER -> {
        val name = weather.weather.name
        check(weather.weather != InlineSkill.SetWeather.Weather.WEATHER_NONE && weather.weather != InlineSkill.SetWeather.Weather.UNRECOGNIZED) {
          "$skillId: weather without a weather"
        }
        val primal = name in listOf("WEATHER_SUN_PRIMAL", "WEATHER_RAIN_PRIMAL", "WEATHER_STRONG_WINDS")
        check(!primal || weather.permanent) { "$skillId: $name can only be permanent" }
        ".kind = InlineEffect::WEATHER, .weather = ENUM_$name, ${weather.durationCase.toCpp(skillId, weather.turns, weather.permanent)}"
      }
      EffectKind.TERRAIN -> {
        check(terrain.terrain != InlineSkill.SetTerrain.Terrain.TERRAIN_NONE && terrain.terrain != InlineSkill.SetTerrain.Terrain.UNRECOGNIZED) {
          "$skillId: terrain without a terrain"
        }
        ".kind = InlineEffect::TERRAIN, .terrain = ENUM_${terrain.terrain.name}, ${terrain.durationCase.toCpp(skillId, terrain.turns, terrain.permanent)}"
      }
      EffectKind.USE_MOVE -> {
        check(useMove.move != MoveEnum.MOVE_NONE && useMove.move != MoveEnum.UNRECOGNIZED) { "$skillId: use_move without a move" }
        check(useMove.power in 0..255) { "$skillId: use_move power ${useMove.power} isn't between 0 and 255" }
        ".kind = InlineEffect::USE_MOVE, .move = ${useMove.move.name}, .power = ${useMove.power}, .falseSwipe = ${useMove.falseSwipe}"
      }
      EffectKind.STATUS -> {
        val name = status.status.name
        check(name != "NONE" && name != "UNRECOGNIZED") { "$skillId: status without a status" }
        check(status.target != InlineSkill.Target.UNRECOGNIZED) { "$skillId: unknown status target" }
        val party = status.target.number >= InlineSkill.Target.TRAINER_PARTY.number
        val major = status.status.number <= InlineSkill.ApplyStatus.Status.BLEED.number
        check(major || !party) { "$skillId: $name only on EVENT_MON, TRAINER_MON or PLAYER_MON" }
        check(party || !status.hasSlots()) { "$skillId: slots are for party targets" }
        check(status.slots.first in 0..6 && status.slots.last in 0..6) { "$skillId: slots between 0 and 6" }
        ".kind = InlineEffect::STATUS, .target = InlineEffect::${status.target.name}, .status = InlineEffect::$name, " +
          ".slotsFirst = ${status.slots.first}, .slotsLast = ${status.slots.last}"
      }
      else -> error("$skillId: effect without a kind")
    }

  // turns or permanent of a weather, terrain, side condition or field. The oneofs are generated per message, so this takes their name.
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
