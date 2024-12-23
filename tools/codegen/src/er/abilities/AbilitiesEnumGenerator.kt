package er.abilities

import er.proto.AbilityEnum
import java.io.FileWriter

object AbilitiesEnumGenerator {
    private const val PATH = "/include/constants/abilities.h"
    fun generate(erPath: String) {
        FileWriter(erPath + PATH).use {writer ->
            val abilities = AbilityEnum.entries.filter { it != AbilityEnum.UNRECOGNIZED }
            val abilityCount = abilities.maxOf { it.number } + 1
            writer.appendLine("""
                |#pragma once
                |
                |#ifdef __assembly__
                |
                |${abilities.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#define ABILITY_COUNT $abilityCount
                |
                |#else
                |
                |typedef enum AbilityEnum {
                |${abilities.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |    ABILITY_COUNT = $abilityCount,
                |};
                |
                |#endif
                """.trimMargin())
        }
    }
}