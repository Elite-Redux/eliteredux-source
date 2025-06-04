package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.proto.Species
import er.proto.Species.SpeciesDexInfo
import java.io.OutputStreamWriter

object PokedexNumbersMapGenerator : Generator {
    fun Species.baseSpeciesInfo(): SpeciesDexInfo = if (hasFormOf()) SPECIES_MAP[formOf]!!.baseSpeciesInfo() else dex
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const u8 gSpeciesToHoennPokedexNum[NUM_SPECIES] = {
            |$IND${SPECIES_LIST.filter { it.baseSpeciesInfo().hoennDexNum > 0 }.joinToString("\n$IND") { "[${it.id}] = ${it.baseSpeciesInfo().hoennDexNum}," }}
            |};
            |
            |const u16 gSpeciesToNationalPokedexNum[NUM_SPECIES] = {
            |$IND${SPECIES_LIST.filter { it.baseSpeciesInfo().nationalDexNum > 0 }.joinToString("\n$IND") { "[${it.id}] = ${it.baseSpeciesInfo().nationalDexNum}," }}
            |};
            |
            |const u16 gHoennToNationalOrder[HOENN_DEX_COUNT] = {
            |$IND${(SPECIES_LIST.associate { it.dex.hoennDexNum to it.dex.nationalDexNum } - 0).entries.joinToString("\n$IND") { "[${it.key}] = ${it.value}," } }
            |};
            |""".trimMargin()
        )
    }
}