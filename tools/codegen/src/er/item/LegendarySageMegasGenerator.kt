package er.item

import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import java.io.OutputStreamWriter

object LegendarySageMegasGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val adoptionCenterMegas = ITEMS_LIST.filter { it.talkToLegendarySage }.map { it.id }
        writer.appendLine(
            """
            |#define ALL_LEGENDARY_SAGE_MEGAS \
            |${adoptionCenterMegas.joinToString("; \\\nadditem ")}
            """.trimMargin()
        )
    }
}