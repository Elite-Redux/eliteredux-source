package er.scripting.move

import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.Descriptors.FieldDescriptor
import er.proto.BattlerTarget
import er.proto.MoveRequirement
import kotlin.collections.Map.Entry

object Requirement {
    private val FILTERED_FIELDS = setOf("not", "target", "require")

    private val MESSAGE_HANDLERS = mapOf<String, (Descriptor, FieldDescriptor, Any, String) -> String>()

    val TEMPLATE_CONFIG = TemplateConfig(
        additionalEnums = setOf(BattlerTarget.getDescriptor()),
        descriptorsToPrint = mapOf(
            MoveRequirement.getDescriptor() to { it.name !in FILTERED_FIELDS }),
        parameters = "BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef"
    )

    fun getRequirementString(requirementEntry: List<MoveRequirement>, params: String? = null): String {
        return requirementEntry.joinToString(" && ", prefix = "(", postfix = ")") {
            getOrRequirement(
                it,
                params ?: "move, battlerAtk, battlerDef"
            )
        }
    }

    private fun getOrRequirement(requirement: MoveRequirement, params: String): String {
        val entries = requirement.allFields.filter { it.key.name !in FILTERED_FIELDS }
            .flatMap {
                getSingleRequirement(
                    it,
                    requirement.target,
                    params
                )
            } + if (requirement.requireList.isNotEmpty()) {
            listOf(getRequirementString(requirement.requireList, params))
        } else {
            emptyList()
        }

        return entries.joinToString(" || ", prefix = "(", postfix = ")") { if (requirement.not) "!$it" else it }
    }


    private fun getSingleRequirement(entry: Entry<FieldDescriptor, Any>, battler: BattlerTarget, params: String) =
        MoveBehaviorEnumConfigGenerator.printTemplateCall(
            "BattlerTarget::$battler, $params",
            entry.value,
            entry.key,
            MoveRequirement.getDescriptor(),
            MESSAGE_HANDLERS
        )
}