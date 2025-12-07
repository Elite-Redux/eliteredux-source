package er.defines

import er.Generator
import er.GeneratorUtils.ABILITIES_LIST
import java.io.OutputStreamWriter

object AbilitiesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val abilities = ABILITIES_LIST.map { it.id }.sorted()
        val abilityCount = abilities.maxOf { it.number } + 1
        writer.appendLine(
            """
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
            |
            |#define FOR_EACH_ABILITY \
            |${abilities.joinToString(" \\\n") { "FOR_EACH_ABILITY_FUNCTION(${it.name})" }}
            |""".trimMargin()
        )
    }
}