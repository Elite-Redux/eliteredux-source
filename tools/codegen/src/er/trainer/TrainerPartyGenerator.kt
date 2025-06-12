package er.trainer

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.TRAINERS_LIST
import er.proto.Species
import er.proto.Species.Gender.FEMALE
import er.proto.Type
import java.io.OutputStreamWriter

object TrainerPartyGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val allParties = TRAINERS_LIST.flatMap { listOf(it.ace.monList, it.elite.monList, it.hell.monList) }
            .filter { it.isNotEmpty() }.distinct()

        for (party in allParties) {
            writer.appendLine("static const struct TrainerMonItemCustomMoves __sParty_${party.hashCode().toUInt()}[] = {")
            for (mon in party) {
                with(mon) {
                    writer.appendLine(
                        """
                        |$IND{
                        |$IND$IND.species = $species,
                        |$IND$IND.heldItem = $item,
                        |$IND$IND.ability = ${
                            SPECIES_MAP[species]!!.abilityList.indexOf(ability)
                                .also { check(it > -1) { "Mon $species does not have ability $ability" } }
                        }, // $ability
                        |$IND$IND.nature = $nature,
                        |$IND$IND.evs = {$hpEv, $atkEv, $defEv, $spatkEv, $spdefEv, $speEv},
                        |$IND$IND.moves = {${moveList.joinToString()}},
                        |$IND$IND.ironPill = ${if (ironPill) "TRUE" else "FALSE"},
                        |$IND$IND.hpType = ${hiddenPowerType.takeIf { it != Type.TYPE_NONE } ?: Type.TYPE_NORMAL},
                        |$IND},""".trimMargin()
                    )
                }
            }
            writer.appendLine("};")
        }

        writer.appendLine("const struct Trainer gTrainers[] = {")
        for (trainer in TRAINERS_LIST) {
            with(trainer) {
                val actualElite = elite.monList.ifEmpty { ace.monList }
                val actualHell = hell.monList.ifEmpty { actualElite }
                val flags = buildList {
                    if (risky) add("AI_FLAG_RISKY")
                    if (preferStall) add("AI_FLAG_STALL")
                    if (preferStatus) add("AI_FLAG_PREFER_STATUS_MOVES")
                    if (noSwitching) add("AI_FLAG_DISABLE_SWITCHING")
                    addAll(listOf("AI_FLAG_CHECK_BAD_MOVE", "AI_FLAG_TRY_TO_FAINT", "AI_FLAG_CHECK_VIABILITY", "AI_FLAG_CHECK_FOE", "AI_FLAG_SMART_SWITCHING", "AI_FLAG_HP_AWARE", "AI_FLAG_WILL_SUICIDE"))
                }.joinToString(" | ")

                writer.appendLine(
                    """
                    |$IND[$id] = {
                    |$IND$IND.partyFlags = F_TRAINER_PARTY_HELD_ITEM | F_TRAINER_PARTY_CUSTOM_MOVESET,
                    |$IND$IND.trainerClass = $class_,
                    |$IND$IND.encounterMusic_gender = ${if (gender == FEMALE) "F_TRAINER_FEMALE | " else ""}$music,
                    |$IND$IND.trainerPic = $pic,
                    |$IND$IND.trainerName = $("$name"),
                    |$IND$IND.doubleBattle = ${if (forcedDouble) "TRUE" else "FALSE"},
                    |$IND$IND.partySize = ${ace.monList.size},
                    |$IND$IND.party = {.ItemCustomMoves = __sParty_${ace.monList.hashCode().toUInt()}},
                    |$IND$IND.partySizeInsane = ${actualElite.size},
                    |$IND$IND.partyInsane = {.ItemCustomMoves = __sParty_${actualElite.hashCode().toUInt()}},
                    |$IND$IND.partySizeHell = ${actualHell.size},
                    |$IND$IND.partyHell = {.ItemCustomMoves = __sParty_${actualHell.hashCode().toUInt()}},
                    |$IND$IND.aiFlags = $flags,
                    |$IND},
                """.trimMargin()
                )
            }
        }
        writer.appendLine("};")
    }
}