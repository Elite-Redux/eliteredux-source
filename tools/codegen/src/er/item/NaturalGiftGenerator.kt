package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import java.io.OutputStreamWriter

object NaturalGiftGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val naturalGiftBerries = ITEMS_LIST.filter { it.hasNaturalGift() }
        writer.appendLine(
            """
            |const TypePower gNaturalGiftTable[LAST_BERRY_INDEX - FIRST_BERRY_INDEX + 1] = {
            |${
                naturalGiftBerries.joinToString("\n") {
                    """
                    |$IND[${it.id} - FIRST_BERRY_INDEX] = {
                    |$IND$IND.power = ${it.naturalGift.power},
                    |$IND$IND.type = ${it.naturalGift.type},
                    |$IND$IND.effect = ${it.naturalGift.effect}},""".trimMargin()
                }
            }
            |};
            |
            |int NaturalGiftPriority(ItemEnum item) {
            |${IND}switch (item) {
            |${naturalGiftBerries.filter { it.naturalGift.priority != 0 }.joinToString("\n") { "$IND${IND}case ${it.id}: return ${it.naturalGift.priority};" }}
            |$IND${IND}default: return TRUE;
            |$IND}
            |};
            """.trimMargin()
        )
    }
}