package er.gfx

import er.Generator
import er.GeneratorUtils
import er.GeneratorUtils.LookupTable
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.printLookupTables
import er.GeneratorUtils.resolveVisuals
import java.io.OutputStreamWriter

object PaletteGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        printLookupTables(
            writer,
            GeneratorUtils.PrintMode.FILE,
            "__sPalette_",
            { name, file -> """u32 $name[] = INCBIN_U32("graphics/pokemon/$file.gbapal.lz")""" },
            LookupTable(
                "struct CompressedSpritePalette gMonPaletteTable[]",
                SPECIES_LIST.map { it.id to it.resolveVisuals().palette }) { id, name -> "SPECIES_PAL($id, $name)" },
            LookupTable(
                "struct CompressedSpritePalette gMonShinyPaletteTable[]",
                SPECIES_LIST.map { it.id to it.resolveVisuals().shiny }) { id, name -> "SPECIES_SHINY_PAL($id, $name)" },
            LookupTable(
                "struct CompressedSpritePalette gMonRareShinyPaletteTable[]",
                SPECIES_LIST.map { it.id to it.resolveVisuals().rare }) { id, name -> "SPECIES_SHINY_PAL($id, $name)" },
            LookupTable(
                "struct CompressedSpritePalette gMonLegendaryShinyPaletteTable[]",
                SPECIES_LIST.map { it.id to it.resolveVisuals().legendary }) { id, name -> "SPECIES_SHINY_PAL($id, $name)" },
            LookupTable(
                "struct CompressedSpritePalette gMonPaletteTableFemale[]",
                SPECIES_LIST.map {
                    it.id to it.resolveVisuals().takeIf { v -> v.hasFemale() }
                        ?.let { v -> v.female.palette.ifEmpty { v.palette } }
                }) { id, name -> "SPECIES_PAL($id, $name)" },
            LookupTable(
                "struct CompressedSpritePalette gMonShinyPaletteTableFemale[]",
                SPECIES_LIST.map {
                    it.id to it.resolveVisuals().takeIf { v -> v.hasFemale() }
                        ?.let { v -> v.female.shiny.ifEmpty { v.shiny } }
                }) { id, name -> "SPECIES_SHINY_PAL($id, $name)" },
        )
    }

}