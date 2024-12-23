package er.abilities

import er.proto.SpeciesEnum
import java.io.FileWriter

object SpeciesEnumGenerator {
    fun generate(file: String) {
        FileWriter(file).use { writer ->
            val species = SpeciesEnum.entries.filter { it != SpeciesEnum.UNRECOGNIZED }
            val speciesCount = species.maxOf { it.number } + 1
            writer.appendLine(
                """
                |#pragma once
                |
                |#ifdef __assembly__
                |
                |${species.joinToString("\n") { "#define ${it.name} ${it.number}" }}
                |
                |#define SPECIES_COUNT $speciesCount
                |
                |#else
                |
                |typedef enum SpeciesEnum {
                |${species.joinToString("\n") { "    ${it.name} = ${it.number}," }}
                |    SPECIES_COUNT = $speciesCount,
                |} SpeciesEnum;
                |
                |#endif
                """.trimMargin()
            )
        }
    }
}