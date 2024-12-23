package er.defines

import er.proto.AbilityEnum
import java.io.FileWriter

object AbilitiesEnumGenerator {
    fun generate(file: String) {
        FileWriter(file).use { writer ->
            val abilities = AbilityEnum.entries.filter { it != AbilityEnum.UNRECOGNIZED }
            val abilityCount = abilities.maxOf { it.number } + 1
            writer.appendLine(
                """
                |#pragma once
                |
                |#ifdef __assembly__
                |
                |${abilities.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#define ABILITIES_COUNT $abilityCount
                |
                |#else
                |
                |typedef enum AbilityEnum {
                |${abilities.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |    ABILITIES_COUNT = $abilityCount,
                |} AbilityEnum;
                |
                |#endif
                """.trimMargin()
            )
        }
    }
}