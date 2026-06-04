package er.skill

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.BATTLE_SKILLS
import java.io.OutputStreamWriter

object BattleSkillGenerator : Generator {
  private val PREFIX = "__sSkillText_"

  override fun generate(writer: OutputStreamWriter) {
    val allStrings = BATTLE_SKILLS.map { it.name }.toSet()

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
        |};
        |"""
          .trimMargin()
      }
    )
  }
}
