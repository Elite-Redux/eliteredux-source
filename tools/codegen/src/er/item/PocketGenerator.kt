package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.proto.ItemEnum
import er.proto.Pocket
import java.io.OutputStreamWriter

object PocketGenerator : Generator {
    private fun itemGroup(id: Pocket, items: List<ItemEnum>) =
        if (items.isEmpty()) {
            "$IND[$id - 1] = (const u16[]) {0},"
        } else {
            """
            |$IND[$id - 1] = (const u16[]) {
            |$IND$IND${items.joinToString("\n$IND$IND") { "$it," }}
            |$IND},
            |""".trimMargin()
        }

    override fun generate(writer: OutputStreamWriter) {
        val items = ITEMS_LIST.groupBy { it.grouping }.mapValues { (_, group) -> group.map { it.id } } - Pocket.POCKET_NONE
        writer.appendLine(
            """
            |const u16 *const gItemsForPocket[] = {
            |${items.entries.joinToString("\n") { itemGroup(it.key, it.value) }}
            |};
            |
            |const u16 gItemCountsForPocket[] = {
            |$IND${items.entries.joinToString("\n$IND") { "[${it.key} - 1] = ${it.value.size}," }}
            |};
            |""".trimMargin()
        )
    }
}