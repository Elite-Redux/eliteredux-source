package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import java.io.OutputStreamWriter

object CoordsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const struct MonCoords gMonFrontPicCoords[] = {
            |$IND${SPECIES_LIST.joinToString("\n$IND") { 
                val mon = if (it.hasReuseVisuals()) SPECIES_MAP[it.reuseVisuals]!! else it
                "[${it.id}] = {.size = ${mon.visuals.front.coords.size}, .y_offset = ${mon.visuals.front.coords.yOffset}},"
            }}
            |};
            |
            |const struct MonCoords gMonBackPicCoords[] = {
            |$IND${SPECIES_LIST.joinToString("\n$IND") {
                val mon = if (it.hasReuseVisuals()) SPECIES_MAP[it.reuseVisuals]!! else it
                "[${it.id}] = {.size = ${mon.visuals.back.coords.size}, .y_offset = ${mon.visuals.back.coords.yOffset}},"
            }}
            |};
            |""".trimMargin())
    }
}