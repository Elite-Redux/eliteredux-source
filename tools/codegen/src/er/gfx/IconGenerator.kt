package er.gfx

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_COUNT
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.proto.VisualsKt.icon
import java.io.OutputStreamWriter

object IconGenerator : Generator {
    private const val ICON_PREFIX = "__sIcon_"
    override fun generate(writer: OutputStreamWriter) {
        val iconInfo = SPECIES_LIST.map {
            val id = when {
                it.hasReuseVisuals() -> it.reuseVisuals
                it.visuals.hasReuseIcon() -> it.visuals.reuseIcon
                else -> it.id
            }
            checkNotNull(SPECIES_MAP[id]) { "$id $it" }.visuals.icon to it.id
        }.filter { it.first != icon {} }
        val (iconIds, speciesIconIds) = iconInfo.createDedupMaps()

        writer.appendLine(iconIds.entries.joinToString("\n") {
            "const u8 $ICON_PREFIX${it.value}[] = INCBIN_U8(\"graphics/pokemon/${it.key.path}.4bpp\");"
        })
        speciesIconIds.printLookupTable("const u8 *const gMonIconTable[$SPECIES_COUNT]", ICON_PREFIX, writer)

        writer.appendLine(
            """
            |const u8 gMonIconPaletteIndices[] = {
            |$IND${iconInfo.joinToString("\n$IND") { "[${it.second}] = ${it.first.palette}," }}
            |};
            |""".trimMargin()
        )

        val femaleIcons = SPECIES_LIST.map { it.id to it.visuals.female.icon }.filter { it.second.path.isNotBlank() }

        writer.appendLine(
            """
            |const u8 *const gMonIconTableFemale[$SPECIES_COUNT] = {
            |$IND${femaleIcons.joinToString("\n$IND") { "[${it.first}] = (const u8[]) INCBIN_U8(\"graphics/pokemon/${it.second.path}.4bpp\")," }}
            |};
            |
            |const u8 gMonIconPaletteIndicesFemale[] = {
            |$IND${femaleIcons.joinToString("\n$IND") { "[${it.first}] = ${it.second.palette}," }}
            |};
            |""".trimMargin()
        )
    }
}