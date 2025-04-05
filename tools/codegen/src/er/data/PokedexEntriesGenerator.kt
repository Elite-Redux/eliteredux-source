package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.proto.Species.SpeciesDexInfo
import er.proto.SpeciesEnum
import er.text.FontMapping
import er.text.FontMapping.Font.NORMAL
import er.text.FontMapping.Font.SMALL
import er.text.FontMapping.breakString
import java.io.OutputStreamWriter

object PokedexEntriesGenerator : Generator {
    private fun dexEntry(dexInfo: SpeciesDexInfo) =
        """
        |$IND[${dexInfo.nationalDexNum}] = { // ${dexInfo.name}
        |$IND$IND.categoryName = $("${dexInfo.category}"),
        |$IND$IND.height = ${dexInfo.height},
        |$IND$IND.weight = ${dexInfo.weight},
        |$IND$IND.description = $("${breakString(dexInfo.description, NORMAL, 224, 4)}"),
        |$IND$IND.pokemonScale = ${dexInfo.pokemonScale},
        |$IND$IND.pokemonOffset = ${dexInfo.pokemonOffset},
        |$IND$IND.trainerScale = ${dexInfo.trainerScale},
        |$IND$IND.trainerOffset = ${dexInfo.trainerOffset},
        |$IND},""".trimMargin()

    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const struct PokedexEntry gPokedexEntries[] = {
            |${
                SPECIES_LIST.filter { it.dex.nationalDexNum > 0 || it.id == SpeciesEnum.SPECIES_NONE }
                    .joinToString("\n") { dexEntry(it.dex) }
            }
            |};
            |""".trimMargin()
        )
    }

}