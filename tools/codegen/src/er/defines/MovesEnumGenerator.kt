package er.defines

import er.proto.MoveEnum
import java.io.FileWriter

object MovesEnumGenerator {
    fun generate(file: String) {
        FileWriter(file).use { writer ->
            val moves = MoveEnum.entries.filter { it != MoveEnum.UNRECOGNIZED }
            val movesCount = moves.maxOf { it.number } + 1
            writer.appendLine(
                """
                |#pragma once
                |
                |#ifdef __assembly__
                |
                |${moves.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#define MOVES_COUNT $movesCount
                |
                |#else
                |
                |typedef enum MoveEnum {
                |${moves.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |    MOVES_COUNT = $movesCount,
                |} MoveEnum;
                |
                |#endif
                """.trimMargin()
            )
        }
    }
}