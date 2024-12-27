package er.defines

import er.FileGenerator.HEADER
import er.Generator
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object SpeciesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val species = SpeciesEnum.entries.filter { it != SpeciesEnum.UNRECOGNIZED }
        val speciesCount = species.maxOf { it.number } + 1
        writer.appendLine(
            """
            |$HEADER
            |#pragma once
            |
            |// Max possible number of species
            |#define SPECIES_COUNT $speciesCount
            |// Max number of non-egg mons
            |#define REAL_SPECIES_COUNT $REAL_SPECIES_COUNT
            |
            |#ifdef __assembly__
            |
            |${species.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |
            |#else
            |
            |typedef enum SpeciesEnum {
            |${species.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |} SpeciesEnum;
            |
            |#endif
            |""".trimMargin()
        )
    }
}
