package er.defines

import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import java.io.OutputStreamWriter

object ItemEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val items = ITEMS_LIST.map { it.id }
        val itemEnum = items.maxOf { it.number } + 1
        writer.appendLine(
            """
            |#ifdef __assembly__
            |
            |${items.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |
            |#define ITEMS_COUNT $itemEnum
            |
            |#else
            |
            |typedef enum ItemEnum {
            |${items.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |    ITEMS_COUNT = $itemEnum,
            |} ItemEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }
}