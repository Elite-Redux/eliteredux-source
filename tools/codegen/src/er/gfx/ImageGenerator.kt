package er.gfx

import er.Generator
import er.GeneratorUtils
import er.GeneratorUtils.LookupTable
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.printLookupTables
import er.GeneratorUtils.resolveVisuals
import java.io.OutputStreamWriter

object ImageGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        printLookupTables(
            writer,
            GeneratorUtils.PrintMode.FILE,
            "__sMonImage_",
            { name, file -> """u32 $name[] = INCBIN_U32("graphics/pokemon/$file.4bpp.lz")""" },
            LookupTable(
                "struct CompressedSpriteSheet gMonFrontPicTable[]",
                SPECIES_LIST.map { it.id to it.resolveVisuals().front.path }) { id, name -> "SPECIES_SPRITE($id, $name)" },
            LookupTable(
                "struct CompressedSpriteSheet gMonFrontPicTableFemale[]",
                SPECIES_LIST.map {
                    it.id to it.resolveVisuals().takeIf { v -> v.hasFemale() }
                        ?.let { v -> v.female.front.ifEmpty { v.front.path } }
                }) { id, name -> "SPECIES_SPRITE($id, $name)" },
            LookupTable(
                "struct CompressedSpriteSheet gMonBackPicTable[]",
                SPECIES_LIST.map { it.id to it.resolveVisuals().back.path }) { id, name -> "SPECIES_SPRITE($id, $name)" },
            LookupTable(
                "struct CompressedSpriteSheet gMonBackPicTableFemale[]",
                SPECIES_LIST.map {
                    it.id to it.resolveVisuals().takeIf { v -> v.hasFemale() }
                        ?.let { v -> v.female.back.ifEmpty { v.back.path } }
                }) { id, name -> "SPECIES_SPRITE($id, $name)" },
        )
    }
}