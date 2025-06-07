package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.proto.Item
import er.proto.ItemEnum
import er.proto.Pocket
import java.io.OutputStreamWriter

object PocketGenerator : Generator {
    private fun itemGroup(id: Pocket, items: List<Item>) =
        when {
            items.isEmpty() ->
                "$IND[$id - 1] = (const u16[]) {0},"

            id == Pocket.POCKET_MEGA_STONES -> {
                val specialMegas = items.filter { it.uniqueMegaLocation.isNotBlank() }
                    .sortedWith(compareByDescending<Item> { it.megaBadgeRequirement }.thenBy { it.name }).map { it.id }
                val adoptionCenterMegas = items.filter { it.adoptionCenter }.sortedBy { it.name }.map { it.id }
                val legendaryMegas = items.filter { it.talkToLegendarySage }.sortedBy { it.id }.map { it.id }
                val nurseJoyMegas = items.filter { it.talkToNurseJoy }.sortedBy { it.name }.map { it.id }
                val knownMegas = (specialMegas + adoptionCenterMegas + legendaryMegas + nurseJoyMegas).toSet()
                val unknownMegas = items.filter { it.id !in knownMegas }.sortedBy { it.name }.map { it.id }
                """
                |$IND[$id - 1] = (const u16[]) {
                |$IND$IND${specialMegas.joinToString("\n$IND$IND") { "$it," }}
                |$IND$IND${adoptionCenterMegas.joinToString("\n$IND$IND") { "$it," }}
                |$IND$IND${legendaryMegas.joinToString("\n$IND$IND") { "$it," }}
                |$IND$IND${nurseJoyMegas.joinToString("\n$IND$IND") { "$it," }}
                |$IND$IND${unknownMegas.joinToString("\n$IND$IND") { "$it," }}
                |$IND},
                |""".trimMargin()
            }

            else ->
                """
                |$IND[$id - 1] = (const u16[]) {
                |$IND$IND${items.joinToString("\n$IND$IND") { "${it.id}," }}
                |$IND},
                |""".trimMargin()
        }

    override fun generate(writer: OutputStreamWriter) {
        val items =
            ITEMS_LIST.groupBy { it.grouping } - Pocket.POCKET_NONE
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