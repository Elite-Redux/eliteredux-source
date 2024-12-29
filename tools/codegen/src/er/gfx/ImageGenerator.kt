package er.gfx

import er.FileGenerator.header
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.proto.Species
import java.io.OutputStreamWriter

object ImageGenerator : Generator {
    private fun printImageTable(writer: OutputStreamWriter, prefix: String, tableName: String, transform: (Species) -> String) {
        val (imageIds, speciesIds) = SPECIES_LIST.map {
            val mon = if (it.hasReuseVisuals()) SPECIES_MAP[it.reuseVisuals]!! else it
            transform(mon) to it.id
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
        writer.appendLine(header)

        printImageTable(writer, "__sFrontPic_", "gMonFrontPicTable") { it.visuals.front.path }
        printImageTable(writer, "__sFrontPicFemale_", "gMonFrontPicTableFemale") { it.visuals.female.front }
        printImageTable(writer, "__sBackPic_", "gMonBackPicTable") { it.visuals.back.path }
        printImageTable(writer, "__sBackPicFemale_", "gMonBackPicTableFemale") { it.visuals.female.back }
    }
}