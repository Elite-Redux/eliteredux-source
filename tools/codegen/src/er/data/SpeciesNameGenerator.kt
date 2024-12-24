package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.TextprotoReader.REAL_SPECIES_COUNT
import er.TextprotoReader.SPECIES_LIST
import er.TextprotoReader.SPECIES_MAP
import er.proto.SpeciesEnum
import java.io.FileWriter

object SpeciesNameGenerator {
    fun generate(file: String) {
        FileWriter(file).use { writer ->

            writer.appendLine(
                """
                |$HEADER
                |const u8 *const gSpeciesNames[] = {
                |$IND${
                    SPECIES_LIST.filter { it.id != SpeciesEnum.SPECIES_EGG }.joinToString(",\n$IND") {
                        """[${it.id}] = $("${it.dex.name.ifEmpty { SPECIES_MAP[it.formOf]!!.dex.name }}")"""
                    }
                }
                |};
                |
                |const u8 *const gSpeciesLongNames[$REAL_SPECIES_COUNT] = {
                |$IND${
                    SPECIES_LIST.filter { it.longName.isNotBlank() }
                        .joinToString(",\n$IND") { """[${it.id}] = $("${it.longName}")""" }
                }
                |};
            """.trimMargin()
            )
        }
    }
}