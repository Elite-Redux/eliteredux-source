package er.item

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ITEMS_LIST
import er.GeneratorUtils.createDedupMaps
import er.proto.Item
import er.proto.Pocket
import er.text.FontMapping.Font.SMALL_NARROW
import er.text.FontMapping.breakString
import java.io.OutputStreamWriter

object MegaHintGenerator : Generator {
    private fun fitEvoString(string: String) = breakString(string, SMALL_NARROW, 152, 2)

    private val BADGE_STRINGS = mapOf(
        5 to fitEvoString("Available after Leader Norman."),
        6 to fitEvoString("Available after Leader Winona."),
        7 to fitEvoString("Available after Leaders Tate & Liza."),
        8 to fitEvoString("Available after Leader Juan."),
        9 to fitEvoString("Available after becoming the Pokémon League champion."),
    )

    private const val NURSE_JOY = "Talk to Nurse Joy."
    private fun adoptionCenterHint(item: Item) = "Purchase ${item.name} from an Adoption Center."
    private const val LEGENDARY_SAGE = "Talk to the legendary sage in Littleroot Town."
    private const val UNKNOWN = "Unknown unlock method."

    private const val HINT_PREFIX = "__sMegaStoneHint_"
    private const val BADGE_PREFIX = "__sMegaStoneBadge_"

    override fun generate(writer: OutputStreamWriter) {
        val (stringsToIds, itemEnumToIds) = ITEMS_LIST.filter { it.grouping == Pocket.POCKET_MEGA_STONES }.map {
            when {
                it.talkToNurseJoy -> NURSE_JOY
                it.adoptionCenter -> adoptionCenterHint(it)
                it.talkToLegendarySage -> LEGENDARY_SAGE
                it.uniqueMegaLocation.isNotBlank() -> it.uniqueMegaLocation
                else -> UNKNOWN
            } to it.id
        }.createDedupMaps()

        writer.appendLine(
            """
            |${
                stringsToIds.entries.joinToString("\n") {
                    """static const u8 $HINT_PREFIX${it.value}[] = _("${
                        fitEvoString(
                            it.key
                        )
                    }");"""
                }
            }
            |
            |${BADGE_STRINGS.entries.joinToString("\n") { """static const u8 $BADGE_PREFIX${it.key}[] = _("${it.value}");""" }}
            |
            |int FromLegendarySage(ItemEnum megaStone) {
            |${IND}switch (megaStone) {
            |${
                ITEMS_LIST.filter { it.talkToLegendarySage }
                    .joinToString("\n") { "$IND${IND}case ${it.id}: return TRUE;" }
            }
            |$IND${IND}default: return FALSE;
            |$IND}
            |}
            |
            |int GetBadgeFlag(ItemEnum megaStone) {
            |${IND}switch (megaStone) {
            |${
                ITEMS_LIST.filter { it.megaBadgeRequirement > 0 }
                    .joinToString("\n") { "$IND${IND}case ${it.id}: return ${if (it.megaBadgeRequirement == 9) "FLAG_SYS_GAME_CLEAR" else "FLAG_BADGE0${it.megaBadgeRequirement}_GET"};" }
            }
            |$IND${IND}default: return 0;
            |$IND}
            |}
            |
            |const u8* GetBadgeString(ItemEnum megaStone) {
            |${IND}switch (megaStone) {
            |${
                ITEMS_LIST.filter { it.megaBadgeRequirement > 0 }
                    .joinToString("\n") { "$IND${IND}case ${it.id}: return $BADGE_PREFIX${it.megaBadgeRequirement};" }
            }
            |$IND${IND}default: return NULL;
            |$IND}
            |}
            |
            |const u8* GetMegaHintString(ItemEnum megaStone) {
            |${IND}switch (megaStone) {
            |${itemEnumToIds.entries.joinToString("\n") { "$IND${IND}case ${it.key}: return $HINT_PREFIX${it.value};" }}
            |$IND${IND}default: return NULL;
            |$IND}
            |}
            |""".trimMargin()
        )

    }
}