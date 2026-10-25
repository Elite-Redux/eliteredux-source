package er.skill

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.BATTLE_SKILLS
import er.GeneratorUtils.BATTLE_SKILL_TEMPLATES
import er.GeneratorUtils.INLINE_SKILLS
import er.proto.BattleSkillTemplate
import er.proto.BattleSkillTemplate.BattleSkillTemplateParam
import er.proto.InlineSkill
import er.proto.Stat
import java.io.OutputStreamWriter

object BattleSkillGenerator : Generator {
  private val PREFIX = "__sSkillText_"

  override fun generate(writer: OutputStreamWriter) {
    val allStrings =
      (BATTLE_SKILLS.map { it.name } + BATTLE_SKILLS.map { it.announceString }).toSet() +
        BATTLE_SKILL_TEMPLATES.map { it.name } +
        BATTLE_SKILL_TEMPLATES.flatMap {
          it.validParamsList.map { params -> params.announceString }
        } +
        INLINE_SKILLS.values.flatMap { listOf(it.name, it.announceString) }

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
      check(skill.onCount > 0) { "$id: inline skill without an `on` block" }
      writer.appendLine(
        """
        |template <>
        |constexpr BattleSkill BattleSkillStrings<$id> = {
        |$IND.name = $PREFIX${skill.name.hashCode().toUInt()},
        |$IND.announceString = $PREFIX${skill.announceString.hashCode().toUInt()},
        |};
        |
        |template <>
        |constexpr BattleSkill Impl<$id> = InlineSkill<${skill.onList.joinToString { it.toCpp(id) }}>;
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

  // An `on` block as an InlineTrigger, see battle_skills.cc
  private fun InlineSkill.Trigger.toCpp(skillId: String): String {
    val kind =
      when (kindCase) {
        InlineSkill.Trigger.KindCase.ENTRY -> "ENTRY"
        InlineSkill.Trigger.KindCase.END_TURN -> "END_TURN"
        else -> error("$skillId: `on` without a trigger")
      }
    val effect =
      when (effect.kindCase) {
        InlineSkill.Effect.KindCase.STAT_CHANGE -> {
          val change = effect.statChange
          check(change.stat != Stat.STAT_HP) { "$skillId: stat_change without a stat" }
          check(change.amount in -6..6 && change.amount != 0) {
            "$skillId: stat_change amount ${change.amount} isn't between -6 and 6, or is 0"
          }
          ".kind = InlineEffect::STAT_CHANGE, .stat = ${change.stat}, .amount = ${change.amount}"
        }
        else -> error("$skillId: `on` without an effect")
      }
    return "InlineTrigger{.kind = InlineTrigger::$kind, .when = InlineTrigger::${entry.`when`}, .effect = {$effect}}"
  }
}
