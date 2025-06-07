package er.defines

import er.FileGenerator.IND
import er.Generator
import er.proto.MoveEffect
import java.io.OutputStreamWriter

object MoveEffectGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val effects = MoveEffect.entries.filter { it != MoveEffect.UNRECOGNIZED }
        val effectsCount = effects.maxOf { it.number }
        writer.appendLine(
            """
                |#define NUM_MOVE_EFFECTS $effectsCount
                |
                |#ifdef __assembly__
                |
                |${effects.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#define MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES 0x2000
                |#define MOVE_EFFECT_AFFECTS_USER 0x4000
                |#define MOVE_EFFECT_CERTAIN 0x8000
                |
                |#else
                |
                |typedef enum MoveEffectEnum {
                |${effects.joinToString("\n") { "$IND${it.name} = ${it.number}," }}
                |${IND}MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES = 0x2000,
                |${IND}MOVE_EFFECT_AFFECTS_USER = 0x4000,
                |${IND}MOVE_EFFECT_CERTAIN = 0x8000,
                |} MoveEffectEnum;
                |
                |#endif
                |""".trimMargin()
        )
    }
}