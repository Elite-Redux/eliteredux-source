package er.item

import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import java.io.OutputStreamWriter
import kotlin.math.max

object AdoptionCenterMegasGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val adoptionCenterMegas = ITEMS_LIST.filter { it.adoptionCenter }
        val adoptionCenterIds = adoptionCenterMegas.sortedBy { it.name }.map { it.id }
        
        writer.appendLine(
            """
            |#define ALL_ADOPTION_CENTER_MEGAS \
            |${adoptionCenterIds.joinToString("; \\\n.2byte ")}
            |
            |#define ADOPTION_CENTER_MEGA_MAX_LENGTH ${
                max(
                    16,
                    adoptionCenterMegas.maxOf { it.name.length } + 1)
            } // ${adoptionCenterMegas.maxBy { it.name.length }.name}
            """.trimMargin()
        )
    }
}