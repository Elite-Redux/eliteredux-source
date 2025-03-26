package er.defines

import er.FileGenerator.header
import er.Generator
import er.proto.MoveEffect
import java.io.OutputStreamWriter

object MoveEffectGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val effects = MoveEffect.entries.filter { it != MoveEffect.UNRECOGNIZED }
        val effectsCount = effects.maxOf { it.number }
        writer.appendLine(
            """
                |$header
                |#pragma once
                |
                |#define NUM_MOVE_EFFECTS $effectsCount
                |
                |#ifdef __assembly__
                |
                |${effects.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#else
                |
                |typedef enum MoveEffectEnum {
                |${effects.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |} MoveEffectEnum;
                |
                |#endif
                |""".trimMargin()
        )
    }
}