package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import java.io.OutputStreamWriter

object NurseJoyMegasGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const ItemEnum gNurseJoyItems[] = {
            |${ITEMS_LIST.filter { it.talkToNurseJoy }.joinToString("\n") { "$IND${it.id}," }}
            |};
            |""".trimMargin()
        )
    }
}