package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.Generator
import er.TextprotoReader.SPECIES_LIST
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object FormSpeciesTableGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val forms = SPECIES_LIST.groupBy({ it.formOf }, { it.id }) - SpeciesEnum.SPECIES_NONE

        writer.appendLine(
            """
            |$HEADER
            |const SpeciesEnum *const gFormSpeciesIdTables[REAL_SPECIES_COUNT] = {
            |""".trimMargin()
        )

        for ((base, variants) in forms) {
            val monForms = listOf(base) + variants
            for (variant in monForms) {
                writer.appendLine(
                    """
                    |$IND[$variant] = (SpeciesEnum[]) {
                    |$IND$IND${monForms.joinToString(",\n$IND$IND")},
                    |$IND$IND{0}},
                    |""".trimMargin()
                )
            }
        }
        writer.appendLine("};")
    }
}
