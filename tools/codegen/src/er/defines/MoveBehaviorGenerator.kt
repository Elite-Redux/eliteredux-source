package er.defines

import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIORS
import er.proto.MoveBehavior
import java.io.OutputStreamWriter

object MoveBehaviorGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val effects = MOVE_BEHAVIORS.keys.withIndex()
        writer.appendLine(
            """
            |#define NUM_BATTLE_MOVE_EFFECTS ${effects.count()}
            |
            |#ifdef __assembly__
            |
            |${effects.joinToString("\n") { "#define ${it.value} ${it.index}" }}
            |
            |#else
            |
            |typedef enum MoveBehaviorEnum {
            |${effects.joinToString("\n") { "    ${it.value} = ${it.index}," }}
            |} MoveBehaviorEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }
}