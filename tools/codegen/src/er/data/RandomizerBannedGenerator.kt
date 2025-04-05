package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.proto.Species.RandomizeBanned.BANNED_NONE
import er.proto.Species.RandomizeBanned.LEGENDARY
import java.io.OutputStreamWriter

object RandomizerBannedGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |int isSpeciesRandomizerBanned(SpeciesEnum species, int allowLegendaries) {
            |${IND}switch (species) {
            |$IND$IND${
                SPECIES_LIST.filter { it.randomizerBanned != BANNED_NONE }
                    .joinToString("\n$IND$IND") { "case ${it.id}: return ${if (it.randomizerBanned == LEGENDARY) "allowLegendaries" else "TRUE"};" }
            }
            |$IND${IND}default: return FALSE;
            |$IND}
            |}
            |""".trimMargin()
        )
    }
}