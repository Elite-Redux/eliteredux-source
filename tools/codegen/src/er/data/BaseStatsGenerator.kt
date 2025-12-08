package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.proto.AbilityEnum
import er.proto.EggGroup
import er.proto.SpeciesEnum
import er.proto.Type
import java.io.OutputStreamWriter

object BaseStatsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |#define PERCENT_FEMALE(percent) min(254, ((percent * 255) / 100))
            |
            |const BaseStats gBaseStats[$REAL_SPECIES_COUNT] = {""".trimMargin()
        )

        val multiheadedAbilities =
            setOf(AbilityEnum.ABILITY_MULTI_HEADED, AbilityEnum.ABILITY_HAND_BARNACLES, AbilityEnum.ABILITY_HYDRA)
        val badMultiheadedMons =
            SPECIES_LIST.filter { ((it.abilityList + it.innateList) intersect multiheadedAbilities).isNotEmpty() }
                .filter { it.heads <= 1 }
        check(badMultiheadedMons.isEmpty()) { "Mons with multiheaded and no head count defined: ${badMultiheadedMons.map { it.id }}" }

        for (species in SPECIES_LIST) {
            val lines = mutableListOf<String>()
            if (species.hp > 0) lines += ".baseHP = ${species.hp}"
            if (species.atk > 0) lines += ".baseAttack = ${species.atk}"
            if (species.def > 0) lines += ".baseDefense = ${species.def}"
            if (species.spatk > 0) lines += ".baseSpAttack = ${species.spatk}"
            if (species.spdef > 0) lines += ".baseSpDefense = ${species.spdef}"
            if (species.spe > 0) lines += ".baseSpeed = ${species.spe}"
            if (species.type != Type.TYPE_NONE) {
                lines += ".type1 = ${species.type}"
                lines += ".type2 = " + if (species.type2 != Type.TYPE_NONE) {
                    species.type2.name
                } else {
                    species.type.name
                }
            }
            if (species.abilityList.isNotEmpty()) {
                require(species.abilityList.size <= 3) { "Too many abilities for ${species.id}" }
                lines += ".abilities = {${(species.abilityList + List(3 - species.abilityList.size) { species.abilityList.last() }).joinToString()}}"
            }
            if (species.innateList.isNotEmpty()) {
                require(species.innateList.size <= 3) { "Too many innates for ${species.id}" }
                lines += ".innates = {${species.innateList.joinToString()}}"
            }
            val dex =
                if (species.formOf != SpeciesEnum.SPECIES_NONE) {
                    checkNotNull(SPECIES_MAP[species.id]) { "Expected ${species.formOf} (from ${species.id}) to have dex but did not have one." }.dex
                } else {
                    species.dex
                }
            if (dex.eggGroup != EggGroup.EGG_GROUP_NONE) {
                lines += ".eggGroup1 = EGG_GROUP_${dex.eggGroup}"
                lines += ".eggGroup2 = EGG_GROUP_" + if (dex.eggGroup2 != EggGroup.EGG_GROUP_NONE) {
                    dex.eggGroup2.name
                } else {
                    dex.eggGroup.name
                }
                if (dex.hasBodyColor()) lines += ".bodyColor = ${dex.bodyColor}"
            }
            if (species.noFlip) lines += ".noFlip = TRUE"
            if (species.tier > 0) lines += ".tier = ${species.tier}"

            val flags = buildList {
                if (species.heads == 2) add("F_TWO_HEADED")
                if (species.heads >= 3) add("F_THREE_HEADED")
                if (species.isTagTeam) add("F_TAG_TEAM")
            }.joinToString(" | ")

            if (flags.isNotEmpty()) lines += ".flags = $flags"

            if (lines.isNotEmpty()) {
                lines += ".genderRatio = " + when {
                    species.genderless -> "MON_GENDERLESS"
                    species.percentFemale >= 100 -> "MON_FEMALE"
                    species.percentFemale <= 0 -> "MON_MALE"
                    else -> "PERCENT_FEMALE(${species.percentFemale})"
                }
                writer.appendLine(
                    """
                    |$IND[${species.id}] = {
                    |$IND$IND${lines.joinToString(",\n$IND$IND")}
                    |$IND},
                    |
                    |""".trimMargin()
                )
            }
        }
        writer.appendLine("};")
    }
}
