package er.scripting.move

import com.google.protobuf.Descriptors.*
import com.google.protobuf.Descriptors.FieldDescriptor.Type.*
import er.Generator
import java.io.OutputStreamWriter

data class TemplateConfig(
    val additionalEnums: Set<EnumDescriptor>,
    val descriptorsToPrint: Map<Descriptor, (FieldDescriptor) -> Boolean>,
    val parameters: String
)

object MoveBehaviorEnumConfigGenerator : Generator {
    private val GENERIC_ENUMS = setOf(
        "AbilityEnum",
        "HoldEffectEnum",
        "ItemEnum",
        "MoveBehaviorEnum",
        "MoveEffectEnum",
        "MoveEnum",
        "SpeciesEnum",
        "TrainerEnum",
        "Type",
        "Pocket",
    )

    override fun generate(writer: OutputStreamWriter) {
        val generatedEnums = GENERIC_ENUMS.toMutableSet()

        fun EnumDescriptor.printEnumIfNotPresent(): Boolean {
            if (name in generatedEnums) return false
            generatedEnums += name
            writer.appendLine(protoEnumToCppEnum(this))
            return true
        }

        fun TemplateConfig.print() {
            additionalEnums.forEach { it.printEnumIfNotPresent() }
            for ((descriptor, filter) in descriptorsToPrint) {
                for (field in descriptor.fields.filter { filter(it) }) {
                    if (field.type == ENUM) field.enumType.printEnumIfNotPresent()
                    writer.appendLine(printFieldMethods(parameters, descriptor, field))
                }
            }
        }

        Requirement.TEMPLATE_CONFIG.print()
        PowerAdjustmentGenerator.TEMPLATE_CONFIG.print()
    }

    fun printTemplateCall(
        parameters: String,
        value: Any,
        field: FieldDescriptor,
        descriptor: Descriptor,
        messageHandlers: Map<String, (Descriptor, FieldDescriptor, Any, String) -> String>
    ): List<String> = if (value is List<*>) {
        value.flatMap { printTemplateCall(parameters, it!!, field, descriptor, messageHandlers) }
    } else {
        listOf(
            when (field.type) {
                INT32, FLOAT -> "resolve${descriptor.name}${field.name.snakeToPascal()}($parameters, $value)"

                BOOL -> {
                    check(value == true) { "Bool values do not support false, descriptor ${descriptor.name}, field ${field.name}" }
                    "resolve${descriptor.name}${field.name.snakeToPascal()}($parameters)"
                }

                ENUM -> {
                    val enumPrefix = if (field.enumType.name in GENERIC_ENUMS) "" else field.enumType.name + "::"
                    "resolve${descriptor.name}${field.name.snakeToPascal()}<$enumPrefix$value>($parameters)"
                }

                MESSAGE -> checkNotNull(messageHandlers[field.messageType.name]) { "No handler for message type ${field.messageType} in descriptor ${descriptor.name} and field ${field.name}" }(
                    descriptor, field, value, parameters
                )

                else -> TODO("No handler for type ${field.type} of descriptor ${descriptor.name} and field ${field.name}")

            }
        )
    }

    private fun printFieldMethods(
        parameters: String,
        descriptor: Descriptor,
        fieldDescriptor: FieldDescriptor
    ): String =
        when (fieldDescriptor.type) {
            INT32 -> "int resolve${descriptor.name}${fieldDescriptor.name.snakeToPascal()}($parameters, int value);"
            FLOAT -> "int resolve${descriptor.name}${fieldDescriptor.name.snakeToPascal()}($parameters, float value);"
            BOOL -> "int resolve${descriptor.name}${fieldDescriptor.name.snakeToPascal()}($parameters);"
            ENUM -> """
                |
                |template<${fieldDescriptor.enumType.name} Value>
                |int resolve${descriptor.name}${fieldDescriptor.name.snakeToPascal()}($parameters);
                |""".trimMargin()

            else -> ""
        }

    private fun protoEnumToCppEnum(enumDescriptor: EnumDescriptor) = """
        |enum class ${enumDescriptor.name} {
        |  ${enumDescriptor.values.joinToString(",\n  ") { "${it.name} = ${it.number}" }}
        |};""".trimMargin()

    private fun String.snakeToPascal() = split("_").joinToString("") { it.replaceFirstChar { it.uppercase() } }
}