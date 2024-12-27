package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object SpeciesNameGenerator : Generator {
    private const val PREFIX = "__sName_"
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(HEADER)
        val (nameIds, speciesIds) = NO_EGG_LIST.map { it.id to if (it.hasFormOf()) it.formOf else it.id }
            .map { (mon, form) ->
                (SPECIES_MAP[form]?.dex?.name ?: "") to mon
            }.createDedupMaps()

        writer.appendLine(nameIds.entries.joinToString("\n") { """static const u8 $PREFIX${it.value}[] = _("${it.key}");""" })

        speciesIds.printLookupTable("const u8 *const gSpeciesNames[]", PREFIX, writer)

        writer.appendLine(
            """
            |const u8 *const gSpeciesLongNames[REAL_SPECIES_COUNT] = {
            |$IND${
                SPECIES_LIST.filter { it.longName.isNotBlank() }
                    .joinToString(",\n$IND") { """[${it.id}] = $("${it.longName}")""" }
            }
            |};
            |""".trimMargin()
        )
    }
}
