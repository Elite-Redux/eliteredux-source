package er.defines

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.proto.HoldEffect.HOLD_EFFECT_CUSTOM
import er.proto.HoldEffect.HOLD_EFFECT_NONE
import java.io.OutputStreamWriter

object HoldEffectGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val effects = ITEMS_LIST.map {
            when {
                it.holdEffect != HOLD_EFFECT_CUSTOM -> it.holdEffect.name
                it.holdEffectAlias.isNotBlank() -> "HOLD_EFFECT_" + it.holdEffectAlias
                else -> "HOLD_EFFECT_" + it.id.name.removePrefix("ITEM_")
            }
        }.toSet() - HOLD_EFFECT_NONE.name
        writer.appendLine(
            """
            |
            |#ifdef __assembly__
            |
            |#define $HOLD_EFFECT_NONE 0
            |${effects.withIndex().joinToString("\n") { "#define ${it.value} ${it.index}" }}
            |
            |#else
            |
            |typedef enum HoldEffectEnum {
            |$IND${HOLD_EFFECT_NONE} = 0,
            |${effects.withIndex().joinToString("\n") { "$IND${it.value} = ${it.index}," }}
            |} HoldEffectEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }
}