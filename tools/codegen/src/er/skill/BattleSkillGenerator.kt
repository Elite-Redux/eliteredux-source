package er.skill

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.BATTLE_SKILLS
import er.GeneratorUtils.BATTLE_SKILL_TEMPLATES
import er.proto.BattleSkillTemplate
import er.proto.BattleSkillTemplateParam
import java.io.OutputStreamWriter

object BattleSkillGenerator : Generator {
  private val PREFIX = "__sSkillText_"

  override fun generate(writer: OutputStreamWriter) {
    val allStrings =
      (BATTLE_SKILLS.map { it.name } + BATTLE_SKILLS.map { it.announceString }).toSet() +
        BATTLE_SKILL_TEMPLATES.map { it.name } +
        BATTLE_SKILL_TEMPLATES.flatMap {
          it.validParamsList.map { params -> params.announceString }
        }

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

        fun BattleSkillTemplate.BattleSkillTemplateParam.string() =
          when (this.paramCase) {
            BattleSkillTemplate.BattleSkillTemplateParam.ParamCase.ABILITY -> ability.toString()
            BattleSkillTemplate.BattleSkillTemplateParam.ParamCase.PARAM_NOT_SET ->
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
}
