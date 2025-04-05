package er.defines

import er.Generator
import er.proto.MoveBehavior
import java.io.OutputStreamWriter

object MoveBehaviorGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val effects = MoveBehavior.entries.filter { it != MoveBehavior.UNRECOGNIZED }
        val effectsCount = effects.maxOf { it.number } + 1
        writer.appendLine(
            """
                |#define NUM_BATTLE_MOVE_EFFECTS $effectsCount
                |
                |#ifdef __assembly__
                |
                |${effects.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#else
                |
                |typedef enum MoveBehaviorEnum {
                |${effects.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |} MoveBehaviorEnum;
                |
                |#endif
                |""".trimMargin()
        )
    }
}