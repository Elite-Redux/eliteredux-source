package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.GeneratorUtils.createDedupMaps
import er.proto.Pocket
import java.io.OutputStreamWriter

object ItemGfxGenerator : Generator {
    private const val ICON_PREFIX = "__sItemIcon_"
    private const val PALETTE_PREFIX = "__sItemPalette_"
    override fun generate(writer: OutputStreamWriter) {
        val (iconMap, idIconMap) = ITEMS_LIST.map { it.image.ifBlank { "question_mark" } to it.id }.createDedupMaps()
        val (paletteMap, idPaletteMap) = ITEMS_LIST.map { it.palette.ifBlank { "question_mark" } to it.id }.createDedupMaps()

        writer.appendLine(
            """
            |${iconMap.entries.joinToString("\n") { """static const u32 $ICON_PREFIX${it.value}[] = INCBIN_U32("graphics/items/icons/${it.key}.4bpp.lz");""" }}
            |${paletteMap.entries.joinToString("\n") { """static const u32 $PALETTE_PREFIX${it.value}[] = INCBIN_U32("graphics/items/icon_palettes/${it.key}.gbapal.lz");""" }}
            |const u32 gItemIcon_ReturnToFieldArrow[] = INCBIN_U32("graphics/items/icons/return_to_field_arrow.4bpp.lz");
            |const u32 gItemIconPalette_ReturnToFieldArrow[] = INCBIN_U32("graphics/items/icon_palettes/return_to_field_arrow.gbapal.lz");
            |
            |const u32 *const gItemIconTable[][2] = {
            |${ITEMS_LIST.joinToString("\n") { "$IND[${it.id}] = {$ICON_PREFIX${idIconMap[it.id]}, $PALETTE_PREFIX${idPaletteMap[it.id]}}," }}
            |$IND[ITEM_FIELD_ARROW] = {gItemIcon_ReturnToFieldArrow, gItemIconPalette_ReturnToFieldArrow},
            |};
            |
            |const u32 *const gBallIconTable[][2] = {
            |${
                ITEMS_LIST.filter { it.grouping == Pocket.POCKET_POKE_BALLS }.joinToString("\n") {
                    "$IND[BALL_${it.id.toString().removeSuffix("_BALL").removePrefix("ITEM_")}] = " +
                            """{(const u32[]) INCBIN_U32("graphics/summary_screen/ball_icons/${it.image.removeSuffix("_ball")}.4bpp.lz"),""" +
                            """ (const u32[]) INCBIN_U32("graphics/summary_screen/ball_icons/${it.image.removeSuffix("_ball")}.gbapal.lz")},"""
                }
            }
            |};
            |""".trimMargin()
        )
    }
}