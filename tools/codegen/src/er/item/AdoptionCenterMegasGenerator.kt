package er.item

import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import java.io.OutputStreamWriter

object AdoptionCenterMegasGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val adoptionCenterMegas = ITEMS_LIST.filter { it.adoptionCenter }.sortedBy { it.name }.map { it.id }
        writer.appendLine(
            """
            |#define ALL_ADOPTION_CENTER_MEGAS \
            |${adoptionCenterMegas.joinToString("; \\\n.2byte ")}
            """.trimMargin()
        )
    }
}