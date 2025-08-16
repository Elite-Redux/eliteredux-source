package er.defines

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.proto.HoldEffect
import er.proto.HoldEffect.HOLD_EFFECT_CUSTOM
import er.proto.HoldEffect.HOLD_EFFECT_NONE
import java.io.OutputStreamWriter

object HoldEffectGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val staticHoldEffects = HoldEffect.entries.filter { it != HoldEffect.UNRECOGNIZED && it != HOLD_EFFECT_CUSTOM }
            .sortedBy { it.number }

        val effects = ITEMS_LIST.map {
            when {
                it.holdEffect != HOLD_EFFECT_CUSTOM -> it.holdEffect.name
                it.holdEffectAlias.isNotBlank() -> "HOLD_EFFECT_" + it.holdEffectAlias
                else -> "HOLD_EFFECT_" + it.id.name.removePrefix("ITEM_")
            }
        } subtract staticHoldEffects.map { it.name }

        val staticEnd = staticHoldEffects.maxOf { it.number } + 1

        writer.appendLine(
            """
            |
            |#ifdef __assembly__
            |
            |${staticHoldEffects.joinToString("\n") { "#define $it ${it.number}" }}
            |${effects.withIndex().joinToString("\n") { "#define ${it.value} ${staticEnd + it.index}" }}
            |
            |#else
            |
            |typedef enum HoldEffectEnum {
            |${staticHoldEffects.joinToString("\n") { "$IND$it = ${it.number}," }}
            |${effects.withIndex().joinToString("\n") { "$IND${it.value} = ${staticEnd + it.index}," }}
            |} HoldEffectEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }
}