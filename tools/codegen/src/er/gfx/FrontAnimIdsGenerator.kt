package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.resolveVisuals
import er.proto.SpeciesEnum.SPECIES_NONE
import java.io.OutputStreamWriter

object FrontAnimIdsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const u8 sMonFrontAnimIdsTable[NUM_SPECIES - 1] = {
            |$IND${NO_EGG_LIST.filter { it.id != SPECIES_NONE }.joinToString("\n$IND") { "[${it.id} - 1] = ${it.resolveVisuals().front.anim}," } }
            |};
            |""".trimMargin()
        )
    }
}