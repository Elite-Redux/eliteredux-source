package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.resolveVisuals
import java.io.OutputStreamWriter

object CoordsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const struct MonCoords gMonFrontPicCoords[] = {
            |$IND${
                SPECIES_LIST.joinToString("\n$IND") {
                    "[${it.id}] = {.size = ${it.resolveVisuals().front.coords.size}, .y_offset = ${it.resolveVisuals().front.coords.yOffset}},"
                }
            }
            |};
            |
            |const struct MonCoords gMonBackPicCoords[] = {
            |$IND${
                SPECIES_LIST.joinToString("\n$IND") {
                    "[${it.id}] = {.size = ${it.resolveVisuals().back.coords.size}, .y_offset = ${it.resolveVisuals().back.coords.yOffset}},"
                }
            }
            |};
            |""".trimMargin()
        )
    }
}