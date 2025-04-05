package er.defines

import er.Generator
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.GeneratorUtils.SPECIES_COUNT
import er.GeneratorUtils.SPECIES_LIST
import er.proto.Species.RandomizeBanned.SPECIES_HIDDEN
import java.io.OutputStreamWriter

object SpeciesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val species = SPECIES_LIST.filter { it.randomizerBanned != SPECIES_HIDDEN }.map { it.id }
        writer.appendLine(
            """
            |// Max possible number of species
            |#define SPECIES_COUNT $SPECIES_COUNT
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
