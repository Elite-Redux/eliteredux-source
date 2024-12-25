package er.defines

import er.FileGenerator.HEADER
import er.Generator
import er.proto.MoveEnum
import java.io.OutputStreamWriter

object MovesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val moves = MoveEnum.entries.filter { it != MoveEnum.UNRECOGNIZED }
        val movesCount = moves.maxOf { it.number } + 1
        writer.appendLine(
            """
                |$HEADER
                |#pragma once
                |
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
