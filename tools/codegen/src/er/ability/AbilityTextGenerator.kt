package er.ability

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.ABILITIES_LIST
import er.text.FontMapping.Font.SMALL_NARROW
import er.text.FontMapping.breakString
import java.io.OutputStreamWriter

object AbilityTextGenerator : Generator {
    private val PREFIX = "__sAbilityText_"
    override fun generate(writer: OutputStreamWriter) {
        val shortDescriptions = buildMap {
            val errors = ABILITIES_LIST.mapNotNull {
                try {
                    put(
                        it.id,
                        breakString(
                            it.description,
                            SMALL_NARROW,
                            150,
                            2
                        )
                    )
                    null
                } catch (e: Exception) {
                    e.message
                }
            }

            check(errors.isEmpty()) { errors.joinToString("\n") }
        }

        val expandedDescriptions = buildMap {
            val errors = ABILITIES_LIST.mapNotNull {
                try {
                    put(
                        it.id,
                        breakString(
                            if (it.expandedDescription.isNotEmpty()) {
                                it.expandedDescription
                            } else {
                                it.description
                            },
                            SMALL_NARROW,
                            150,
                            11
                        )
                    )
                    null
                } catch (e: Exception) {
                    e.message
                }
            }

            check(errors.isEmpty()) { errors.joinToString("\n") }
        }

        val allStrings = (shortDescriptions.values + expandedDescriptions.values).toSet()

        writer.appendLine(allStrings.joinToString("\n") {
            """constexpr u8 $PREFIX${
                it.hashCode().toUInt()
            }[] = _("$it");"""
        })

        writer.appendLine(
            """
            |template <AbilityEnum Id>
            |constexpr Ability AbilityStrings = {0};
            |""".trimMargin()
        )

        writer.appendLine(
            ABILITIES_LIST.joinToString("\n") {
                """
                |template <>
                |constexpr Ability AbilityStrings<${it.id}> = {
                |$IND.name = ${'$'}("${it.name}"),
                |$IND.description = $PREFIX${shortDescriptions[it.id]!!.hashCode().toUInt()},
                |$IND.expandedDescription = $PREFIX${expandedDescriptions[it.id]!!.hashCode().toUInt()},
                |};
                |""".trimMargin()
            }
        )
    }
}
