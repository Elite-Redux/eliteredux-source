package er.defines

import er.Generator
import er.GeneratorUtils.ABILITIES_LIST
import java.io.OutputStreamWriter

object AbilitiesEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val abilities = ABILITIES_LIST.map { it.id }
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
            |#define OVER_ALL_ABILITIES \
            |${abilities.joinToString(" \\\n") {"__ON_ABILITY($it)"}}
            |
            |""".trimMargin()
        )
    }
}