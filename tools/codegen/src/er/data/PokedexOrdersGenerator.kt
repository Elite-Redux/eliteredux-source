package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import java.io.OutputStreamWriter

object PokedexOrdersGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val dexEntries = SPECIES_LIST.sortedBy { it.id }.map { it.dex }.filter { it.nationalDexNum > 0 }
        writer.appendLine(
            """
            |const u16 gPokedexOrder_Alphabetical[] = {
            |$IND${dexEntries.sortedBy { it.name }.joinToString("\n$IND") { "${it.nationalDexNum}, // ${it.name}"  }}
            |};
            |
            |const u16 gPokedexOrder_Weight[] = {
            |$IND${dexEntries.sortedBy { it.weight }.joinToString("\n$IND") { "${it.nationalDexNum}, // ${it.name}"  }}
            |};
            |
            |const u16 gPokedexOrder_Height[] = {
            |$IND${dexEntries.sortedBy { it.height }.joinToString("\n$IND") { "${it.nationalDexNum}, // ${it.name}"  }}
            |};
            |""".trimMargin()
        )
    }
}