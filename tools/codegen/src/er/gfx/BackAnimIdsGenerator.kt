package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.resolveVisuals
import er.proto.SpeciesEnum.SPECIES_NONE
import java.io.OutputStreamWriter

object BackAnimIdsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const u8 sSpeciesToBackAnimSet[NUM_SPECIES] = {
            |$IND${
                NO_EGG_LIST.filter { it.id != SPECIES_NONE }
                    .joinToString("\n$IND") { "[${it.id}] = ${it.resolveVisuals().back.anim}," }
            }
            |};
            |""".trimMargin()
        )
    }
}