package er.defines

import er.Generator
import er.GeneratorUtils.MOVES_LIST
import java.io.OutputStreamWriter

object MovesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val moves = MOVES_LIST.map { it.id }
        val movesCount = moves.maxOf { it.number } + 1
        writer.appendLine(
            """
                |#define MOVES_COUNT $movesCount
                |
                |#ifdef __assembly__
                |
                |${moves.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#else
                |
                |typedef enum MoveEnum {
                |${moves.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |} MoveEnum;
                |
                |#endif
                |""".trimMargin()
        )
    }
}
