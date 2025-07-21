package er.trainer

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.TRAINERS_LIST
import er.GeneratorUtils.expandLearnset
import er.GeneratorUtils.findLearnsetForSpecies
import er.proto.ItemEnum
import er.proto.Species.Gender.FEMALE
import er.proto.TrainerParty.TrainerMon
import er.proto.TrainerParty.TrainerMon.Nature
import er.proto.Type
import er.trainer.TrainerPartyGenerator.NatureStat.*
import java.io.OutputStreamWriter

object TrainerPartyGenerator : Generator {
    enum class NatureStat {
        NEUTRAL,
        ATK,
        DEF,
        SPATK,
        SPDEF,
        SPE,
    }

    private fun getNegativeNatureStat(nature: Nature): NatureStat = when (nature) {
        Nature.NATURE_HARDY -> NEUTRAL
        Nature.NATURE_LONELY -> DEF
        Nature.NATURE_BRAVE -> SPE
        Nature.NATURE_ADAMANT -> SPATK
        Nature.NATURE_NAUGHTY -> SPDEF
        Nature.NATURE_BOLD -> ATK
        Nature.NATURE_DOCILE -> NEUTRAL
        Nature.NATURE_RELAXED -> SPE
        Nature.NATURE_IMPISH -> SPATK
        Nature.NATURE_LAX -> SPDEF
        Nature.NATURE_TIMID -> ATK
        Nature.NATURE_HASTY -> DEF
        Nature.NATURE_SERIOUS -> NEUTRAL
        Nature.NATURE_JOLLY -> SPATK
        Nature.NATURE_NAIVE -> SPDEF
        Nature.NATURE_MODEST -> ATK
        Nature.NATURE_MILD -> DEF
        Nature.NATURE_QUIET -> SPE
        Nature.NATURE_BASHFUL -> NEUTRAL
        Nature.NATURE_RASH -> SPDEF
        Nature.NATURE_CALM -> ATK
        Nature.NATURE_GENTLE -> DEF
        Nature.NATURE_SASSY -> SPE
        Nature.NATURE_CAREFUL -> SPATK
        Nature.NATURE_QUIRKY -> NEUTRAL
        Nature.UNRECOGNIZED -> error("Unrecognized nature")
    }

    private inline fun <T> MutableList<T>.addIfNot(condition: Boolean, output: () -> T) {
        if (!condition) add(output())
    }

    fun TrainerMon.validate() = buildList {
        if (nonstandard.isEmpty()) {
            val possibleMoves = findLearnsetForSpecies(SPECIES_MAP[species]!!)
            val invalidMoves =
                moveList subtract (expandLearnset(
                    possibleMoves,
                    SPECIES_MAP[species]!!
                ) + possibleMoves.levelList.flatMap { it.moveList }).toSet()
            addIfNot(invalidMoves.isEmpty()) { "Invalid moves: $invalidMoves\n$this" }

            addIfNot(((hpEv / 4) + (atkEv / 4) + (defEv / 4) + (spatkEv / 4) + (spdefEv / 4) + (speEv / 4)) == 508 / 4) {
                "Invalid EV total\n$this"
            }

            addIfNot(item != ItemEnum.ITEM_NONE) { "Missing item\n$this" }

            when (getNegativeNatureStat(nature)) {
                NEUTRAL -> add("Neutral nature\n$this")
                ATK -> addIfNot(atkEv < 4) { "Positive EV for negative nature\n$this" }
                DEF -> addIfNot(defEv < 4) { "Positive EV for negative nature\n$this" }
                SPATK -> addIfNot(spatkEv < 4) { "Positive EV for negative nature\n$this" }
                SPDEF -> addIfNot(spdefEv < 4) { "Positive EV for negative nature\n$this" }
                SPE -> addIfNot(speEv < 4) { "Positive EV for negative nature\n$this" }
            }

            if (ironPill) {
                addIfNot(getNegativeNatureStat(nature) == SPE) { "Iron pill should use negative speed nature\n$this" }
            }
        }
    }

    override fun generate(writer: OutputStreamWriter) {
        val allParties = TRAINERS_LIST.flatMap { listOf(it.ace.monList, it.elite.monList, it.hell.monList) }.distinct()

        val errors = buildList {
            for (party in allParties) {
                writer.appendLine(
                    "static const struct TrainerMonItemCustomMoves __sParty_${
                        party.hashCode().toUInt()
                    }[] = {"
                )
                for (mon in party) {
                    with(mon) {
                        validate()

                        writer.appendLine(
                            """
                            |$IND{
                            |$IND$IND.species = $species,
                            |$IND$IND.heldItem = $item,""".trimMargin()
                        )

                        val abilityIndex = SPECIES_MAP[species]!!.abilityList.indexOf(ability)
                            .also { addIfNot(it > -1) { "Mon $species does not have ability $ability, partyHash: ${party.hashCode().toUInt()}" } }

                        if (abilityIndex >= 0) {
                            writer.appendLine()
                            "$IND$IND.ability = $abilityIndex, // $ability".trimMargin()
                        }

                        writer.appendLine(
                            """
                            |$IND$IND.nature = $nature,
                            |$IND$IND.evs = {$hpEv, $atkEv, $defEv, $spatkEv, $spdefEv, $speEv},
                            |$IND$IND.moves = {${moveList.joinToString()}},
                            |$IND$IND.zeroSpeedIvs = ${if (ironPill) "TRUE" else "FALSE"},
                            |$IND$IND.hpType = ${hiddenPowerType.takeIf { it != Type.TYPE_NONE } ?: Type.TYPE_NORMAL},
                            |$IND},""".trimMargin()
                        )
                    }
                }
                if (party.isEmpty()) writer.append("0")
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
                        addAll(
                            listOf(
                                "AI_FLAG_CHECK_BAD_MOVE",
                                "AI_FLAG_TRY_TO_FAINT",
                                "AI_FLAG_CHECK_VIABILITY",
                                "AI_FLAG_CHECK_FOE",
                                "AI_FLAG_SMART_SWITCHING",
                                "AI_FLAG_HP_AWARE",
                                "AI_FLAG_WILL_SUICIDE"
                            )
                        )
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
                        |$IND$IND.party = __sParty_${ace.monList.hashCode().toUInt()},
                        |$IND$IND.partySizeInsane = ${actualElite.size},
                        |$IND$IND.partyInsane = __sParty_${actualElite.hashCode().toUInt()},
                        |$IND$IND.partySizeHell = ${actualHell.size},
                        |$IND$IND.partyHell = __sParty_${actualHell.hashCode().toUInt()},
                        |$IND$IND.aiFlags = $flags,
                        |$IND},
                    """.trimMargin()
                    )
                }
            }
            writer.appendLine("};")
        }

        if (errors.isNotEmpty()) {
            writer.appendLine("""
                |#ifdef VALIDATE_TRAINERS
                |${errors.joinToString("\n")}
                |#endif
            """.trimMargin())
        }
    }
}