package er.defines

import er.FileGenerator.HEADER
import er.Generator
import er.proto.AbilityEnum
import java.io.OutputStreamWriter

object AbilitiesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val abilities = AbilityEnum.entries.filter { it != AbilityEnum.UNRECOGNIZED }
        val abilityCount = abilities.maxOf { it.number } + 1
        writer.appendLine(
            """
            |$HEADER
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
            |""".trimMargin()
        )
    }
}