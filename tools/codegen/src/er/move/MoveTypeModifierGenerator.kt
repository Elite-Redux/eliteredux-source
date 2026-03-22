package er.move

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIORS
import er.proto.Type.TYPE_NONE
import java.io.OutputStreamWriter

object MoveTypeModifierGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val seVs = MOVE_BEHAVIORS.mapValues { it.value.attack.superEffectiveVs }.filterValues { it != TYPE_NONE }
        writer.appendLine(
            """
            |u16 UpdateTypeModifier(Type type, MoveBehaviorEnum effect, u16 modifier) {
            |${IND}switch (effect) {
            |$IND$IND${seVs.entries.joinToString("\n$IND$IND") { "case ${it.key}: return type == ${it.value} ? UQ_4_12(2.0) : modifier;" } }
            |$IND${IND}case EFFECT_IGNORE_TYPE_IMMUNITY: return !modifier ? UQ_4_12(1.0) : modifier;
            |$IND${IND}default: return modifier;
            |$IND}
            |}""".trimMargin()
        )
    }

}
