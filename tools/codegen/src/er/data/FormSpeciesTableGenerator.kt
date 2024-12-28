package er.data

import er.FileGenerator.header
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object FormSpeciesTableGenerator : Generator {
    val PREFIX = "__sFormSpeciesTable_"
    override fun generate(writer: OutputStreamWriter) {
        val forms = SPECIES_LIST.groupBy({ it.formOf }, { it.id }) - SpeciesEnum.SPECIES_NONE

        writer.appendLine(header)

        val (formIds, speciesIds) = NO_EGG_LIST.map {
            val id = if (it.hasFormOf()) it.formOf else it.id
            (listOf(id) + forms[id].orEmpty()) to it.id
        }.filter { it.first.size > 1 }.createDedupMaps()

        writer.appendLine(formIds.entries.joinToString("\n") {
            """
            |const SpeciesEnum[] $PREFIX${it.value} = {
            |$IND${it.key.joinToString("\n$IND") { id -> "$id," }}
            |${IND}0};
            |""".trimMargin()
        })

        speciesIds.printLookupTable("const u16 *const gFormSpeciesIdTables[$REAL_SPECIES_COUNT]", PREFIX, writer)
    }
}
