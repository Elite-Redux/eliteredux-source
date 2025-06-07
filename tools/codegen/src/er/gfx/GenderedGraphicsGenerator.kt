package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.resolveVisuals
import java.io.OutputStreamWriter

object GenderedGraphicsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine("""
        |int SpeciesHasGenderDifference(SpeciesEnum species) {
        |${IND}switch (species) {
        |${SPECIES_LIST.filter { it.resolveVisuals().hasFemale() }.joinToString("\n") { "$IND${IND}case ${it.id}: return TRUE;" }}
        |$IND}
        |${IND}return FALSE;
        |}
        """.trimMargin())
    }
}