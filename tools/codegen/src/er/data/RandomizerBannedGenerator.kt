package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.proto.Species
import er.proto.Species.RandomizeBanned.*
import java.io.OutputStreamWriter

object RandomizerBannedGenerator : Generator {
    private fun randomizerCondition(species: Species) = when {
        species.randomizerBanned == LEGENDARY -> "allowLegendaries"
        species.randomizerBanned == PARADOX -> "allowParadox"
        species.randomizerBanned != BANNED_NONE -> "FALSE"
        species.megaList.isNotEmpty() -> "FALSE"
        species.primalList.isNotEmpty() -> "FALSE"
        species.hasFormShiftOf() -> "FALSE"
        else -> "TRUE"
    }

    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |int isSpeciesRandomizerAllowed(SpeciesEnum species, int allowLegendaries, int allowParadox) {
            |${IND}switch (species) {
            |$IND$IND${
                SPECIES_LIST.joinToString("\n$IND$IND") { "case ${it.id}: return ${randomizerCondition(it)};" }
            }
            |$IND${IND}default: return FALSE;
            |$IND}
            |}
            |""".trimMargin()
        )
    }
}