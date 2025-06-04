package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.GeneratorUtils.createDedupMaps
import er.proto.HoldEffect
import er.proto.Item
import er.proto.Pocket
import er.proto.Type
import er.text.FontMapping.Font.NORMAL
import er.text.FontMapping.breakString
import java.io.OutputStreamWriter

object ItemGenerator : Generator {
    private const val DESCRIPTION_PREFIX = "__sItemDescription_"

    private fun printItem(item: Item, descriptionId: Int) = buildString {
        append(
            """
            |$IND[${item.id}] = {
            |$IND$IND.name = $("${item.name}"),
            |$IND$IND.itemId = ${item.id},
            |$IND$IND.bpPrice = ${item.bpPrice},
            |$IND$IND.pocket = ${item.grouping},
            |$IND$IND.type = ${item.useType},
            |$IND$IND.description = $DESCRIPTION_PREFIX$descriptionId,
            |""".trimMargin()
        )

        if (item.grouping == Pocket.POCKET_KEY_ITEMS) appendLine("$IND$IND.importance = TRUE,")

        when (item.holdEffect) {
            HoldEffect.UNRECOGNIZED,
            HoldEffect.HOLD_EFFECT_NONE -> {
            }

            HoldEffect.HOLD_EFFECT_CUSTOM -> {
                if (item.holdEffectAlias.isNotBlank()) {
                    appendLine("$IND$IND.holdEffect = HOLD_EFFECT_${item.holdEffectAlias},")
                } else {
                    appendLine("$IND$IND.holdEffect = HOLD_EFFECT_${item.id.name.removePrefix("ITEM_")},")
                }
            }

            else -> {
                appendLine("$IND$IND.holdEffect = ${item.holdEffect},")
            }
        }

        if (item.holdEffectStrength != 0) appendLine("$IND$IND.holdEffectParam = ${item.holdEffectStrength},")
        if (item.holdEffectType != Type.TYPE_NONE) appendLine("$IND$IND.secondaryId = ${item.holdEffectType},")
        if (item.holdEffectMiscParam.isNotBlank()) appendLine("$IND$IND.secondaryId = ${item.holdEffectMiscParam},")

        if (item.fieldUseFunc.isNotBlank()) {
            appendLine("$IND$IND.fieldUseFunc = ItemUseOutOfBattle_${item.fieldUseFunc},")
        } else {
            appendLine("$IND$IND.fieldUseFunc = ItemUseOutOfBattle_CannotUse,")
        }

        if (item.battleUseFunc.isNotBlank()) {
            appendLine(
                "$IND$IND.battleUsage = ITEM_B_USE_${
                    if (item.grouping in setOf(
                            Pocket.POCKET_MEDICINE,
                            Pocket.POCKET_BERRIES
                        )
                    ) "MEDICINE" else "OTHER"
                },"
            )
            appendLine("$IND$IND.battleUseFunc = ItemUseInBattle_${item.battleUseFunc},")
        }
        append("$IND},")
    }

    override fun generate(writer: OutputStreamWriter) {
        val (descriptionToIds, itemToIds) = ITEMS_LIST.sortedBy { it.id }.map { it.description to it }.createDedupMaps()

        writer.appendLine(
            descriptionToIds.entries.joinToString("\n") {
                """const u8 $DESCRIPTION_PREFIX${it.value}[] = _("${
                    breakString(
                        it.key,
                        NORMAL,
                        103,
                        3
                    )
                }");"""
            }
        )
        writer.appendLine(
            """
            |
            |const struct Item gItems[] = {
            |${itemToIds.entries.joinToString("\n") { printItem(it.key, it.value) }}
            |};
            |""".trimMargin()
        )
    }
}