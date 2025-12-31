package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.resolveVisuals
import java.io.OutputStreamWriter

object ElevationGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const u8 gEnemyMonElevation[NUM_SPECIES] = {
            |${SPECIES_LIST.joinToString("\n") {"$IND[${it.id}] = ${it.resolveVisuals().front.coords.elevation},"} }
            |};
            |""".trimMargin()
        )
    }
}