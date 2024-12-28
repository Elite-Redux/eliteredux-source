package er.gfx

import er.FileGenerator.header
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.proto.Species
import java.io.OutputStreamWriter

object PaletteGenerator : Generator {
    private fun printTable(
        writer: OutputStreamWriter,
        tableName: String,
        pal: String,
        prefix: String,
        transform: (Species) -> String
    ) {
        val (paletteIds, speciesIds) = SPECIES_LIST.map {
            val mon = if (it.hasReuseVisuals()) SPECIES_MAP[it.reuseVisuals]!! else it
            transform(mon) to it.id
        }.filter { it.first.isNotBlank() }.createDedupMaps()

        writer.appendLine(paletteIds.entries.joinToString("\n") {
            """const u32 $prefix${it.value}[] = INCBIN_U32("graphics/pokemon/${it.key}.gbapal.lz");"""
        })

        writer.appendLine(
            """
            |const struct CompressedSpritePalette $tableName[] = {
            |$IND${speciesIds.entries.joinToString("\n$IND") { "$pal(${it.key}, $prefix${it.value})," }}
            |};
            |""".trimMargin()
        )
    }

    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(header)
        printTable(writer, "gMonPaletteTable", "SPECIES_PAL", "__sPalette_") { it.visuals.palette }
        printTable(writer, "gMonPaletteTableFemale", "SPECIES_PAL", "__sPaletteFemale_") { it.visuals.female.palette }
        printTable(writer, "gMonShinyPaletteTable", "SPECIES_SHINY_PAL", "__sPaletteShiny_") { it.visuals.shiny }
        printTable(writer, "gMonRareShinyPaletteTable", "SPECIES_SHINY_PAL", "__sPaletteShinyRare_") { it.visuals.rare }
        printTable(writer, "gMonLegendaryShinyPaletteTable", "SPECIES_SHINY_PAL", "__sPaletteShinyLegendary_") { it.visuals.legendary }
        printTable(writer, "gMonShinyPaletteTableFemale", "SPECIES_SHINY_PAL", "__sPaletteShinyFemale_") { it.visuals.female.shiny }
    }

}