package er.ability

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ABILITIES_LIST
import er.text.FontMapping.Font.SMALL_NARROW
import er.text.FontMapping.breakString
import java.io.OutputStreamWriter

object AbilityTextGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val errors = ABILITIES_LIST.mapNotNull {
            try {
                breakString(
                    it.description,
                    SMALL_NARROW,
                    150,
                    2
                )
                null
            } catch (e: Exception) {
                e.message
            }
        }

        check(errors.isEmpty()) { errors.joinToString("\n") }

        val errorsExpanded = ABILITIES_LIST.mapNotNull {
            try {
                breakString(
                    it.expandedDescription,
                    SMALL_NARROW,
                    150,
                    11
                )
                null
            } catch (e: Exception) {
                e.message
            }
        }

        check(errorsExpanded.isEmpty()) { errorsExpanded.joinToString("\n") }

        writer.appendLine(
            """
                |constexpr AbilityKVPair sAbilityText[] = {
                |$IND${
                ABILITIES_LIST.joinToString("\n$IND") {
                    """{${it.id}, { .name = $("${it.name}"), .description = $("${
                        breakString(
                            it.description,
                            SMALL_NARROW,
                            150,
                            2
                        )
                    }")${
                        if (it.hasExpandedDescription())
                            """, .expandedDescription = $("${
                                breakString(
                                    it.expandedDescription,
                                    SMALL_NARROW,
                                    150,
                                    11
                                )
                            }")"""
                        else
                        ""
                    }}},"""
                }
            }
                |};
            """.trimMargin()
        )
    }
}
