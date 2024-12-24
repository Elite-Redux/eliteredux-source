package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.TextprotoReader.SPECIES_LIST
import er.TextprotoReader.SPECIES_MAP
import er.proto.EggGroup
import er.proto.SpeciesEnum
import er.proto.Type
import java.io.FileWriter

object BaseStatsGenerator {
    fun generate(path: String) {
        FileWriter(path).use { writer ->
            writer.appendLine("""
                |$HEADER
                |#define PERCENT_FEMALE(percent) min(254, ((percent * 255) / 100))
                |
                |const BaseStats gBaseStats[] = {""".trimMargin())

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
                    lines += ".innates = {${species.abilityList.joinToString()}}"
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
                }
                if (species.hasBodyColor()) lines += ".bodyColor = BODY_COLOR_${species.bodyColor}"
                if (species.noFlip) lines += ".noFlip = TRUE"
                if (species.tier > 0) lines += ".tier = ${species.tier}"
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
                    """.trimMargin()
                    )
                }
            }
            writer.appendLine("};")
        }
    }
}