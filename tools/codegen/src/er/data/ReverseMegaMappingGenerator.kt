package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import java.io.OutputStreamWriter

object ReverseMegaMappingGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine("""
            |SpeciesEnum GetBaseSpeciesFromMega(SpeciesEnum megaSpecies) {
            |${IND}switch (megaSpecies) {
            |$IND$IND${SPECIES_LIST.filter { it.megaList.isNotEmpty() }.joinToString("\n$IND$IND") { "case ${it.id}: return ${it.megaList.first().from};" }}
            |$IND$IND${SPECIES_LIST.filter { it.primalList.isNotEmpty() }.joinToString("\n$IND$IND") { "case ${it.id}: return ${it.primalList.first().from};" }}
            |$IND${IND}default: return SPECIES_NONE;
            |$IND}
            |}                
        """.trimMargin())
    }
}