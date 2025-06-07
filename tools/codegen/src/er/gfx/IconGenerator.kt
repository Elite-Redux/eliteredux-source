package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils
import er.GeneratorUtils.LookupTable
import er.GeneratorUtils.SPECIES_COUNT
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.printLookupTables
import er.GeneratorUtils.resolveVisuals
import er.proto.VisualsKt.icon
import java.io.OutputStreamWriter

object IconGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val iconInfo = SPECIES_LIST.map {
            val id = when {
                it.hasReuseVisuals() -> it.reuseVisuals
                it.visuals.hasReuseIcon() -> it.visuals.reuseIcon
                else -> it.id
            }
            it.id to checkNotNull(SPECIES_MAP[id]) { "$id $it" }.visuals.icon
        }.filter { it.second != icon {} }

        val femaleIcons = SPECIES_LIST.map { it.id to it.resolveVisuals() }.filter { it.second.hasFemale() }
            .map { it.first to if (it.second.female.hasIcon()) it.second.female.icon else it.second.icon }

        printLookupTables(writer, GeneratorUtils.PrintMode.FILE, "__sMonIcon_", {name, path -> "u8 $name[] = INCBIN_U8(\"graphics/pokemon/$path.4bpp\")"},
            LookupTable("u8 *const gMonIconTable[$SPECIES_COUNT]", iconInfo.map { it.first to it.second.path }),
            LookupTable("u8 *const gMonIconTableFemale[$SPECIES_COUNT]", femaleIcons.map { it.first to it.second.path })
        )

        writer.appendLine(
            """
            |const u8 gMonIconPaletteIndices[] = {
            |$IND${iconInfo.joinToString("\n$IND") { "[${it.first}] = ${it.second.palette}," }}
            |};
            |
            |const u8 gMonIconPaletteIndicesFemale[] = {
            |$IND${femaleIcons.joinToString("\n$IND") { "[${it.first}] = ${it.second.palette}," }}
            |};
            |""".trimMargin()
        )
    }
}