package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.resolveVisuals
import er.proto.Visuals
import java.io.OutputStreamWriter

object ImageGenerator : Generator {
    private fun printImageTable(
        writer: OutputStreamWriter,
        prefix: String,
        tableName: String,
        transform: (Visuals) -> String
    ) {
        val (imageIds, speciesIds) = SPECIES_LIST.map {
            transform(it.resolveVisuals()) to it.id
        }.filter { it.first.isNotBlank() }.createDedupMaps()

        writer.appendLine(imageIds.entries.joinToString("\n") {
            """const u32 $prefix${it.value}[] = INCBIN_U32("graphics/pokemon/${it.key}.4bpp.lz");"""
        })

        writer.appendLine(
            """
            |const struct CompressedSpriteSheet $tableName[] = {
            |$IND${speciesIds.entries.joinToString("\n$IND") { "SPECIES_SPRITE(${it.key}, $prefix${it.value})," }}
            |};
            |""".trimMargin()
        )

    }

    override fun generate(writer: OutputStreamWriter) {
        printImageTable(writer, "__sFrontPic_", "gMonFrontPicTable") { it.front.path }
        printImageTable(writer, "__sFrontPicFemale_", "gMonFrontPicTableFemale") { it.female.front }
        printImageTable(writer, "__sBackPic_", "gMonBackPicTable") { it.back.path }
        printImageTable(writer, "__sBackPicFemale_", "gMonBackPicTableFemale") { it.female.back }
    }
}