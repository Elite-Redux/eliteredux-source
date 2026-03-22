package er.move

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIORS
import java.io.OutputStreamWriter

object MoveRecoilGenerator : Generator {
  override fun generate(writer: OutputStreamWriter) {
    val recoilEffects =
      MOVE_BEHAVIORS.mapValues { it.value.attack.recoilFraction }.filterValues { it > 0 }
    writer.appendLine(
      """
            |int GetRecoilFraction(MoveBehaviorEnum effect) {
            |${IND}switch (effect) {
            |$IND$IND${recoilEffects.entries.joinToString("\n$IND$IND") { "case ${it.key}: return ${it.value};" }}
            |$IND${IND}default: return 0;
            |$IND}
            |}
            |"""
        .trimMargin()
    )
  }
}
