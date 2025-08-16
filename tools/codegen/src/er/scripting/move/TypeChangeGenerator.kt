package er.scripting.move

import er.FileGenerator.IND
import er.Generator
import er.proto.AttackBehaviorConfig.TypeChange
import er.proto.MoveBehavior
import er.scripting.move.MoveBehaviorEnumConfigGenerator.printTemplateCall
import er.scripting.move.MoveScriptingUtils.groupAttackData
import java.io.OutputStreamWriter

object TypeChangeGenerator : Generator {
    private val MODIFIERS = setOf("require")
    val TEMPLATE_CONFIG = TemplateConfig(
        additionalEnums = emptySet(),
        descriptorsToPrint = mapOf(TypeChange.getDescriptor() to { it.name !in MODIFIERS }),
        parameters = "MoveEnum move, u8 battler",
        returnType = "Type",
    )

    val DISPLAY_TEMPLATE_CONFIG = TemplateConfig(
        additionalEnums = emptySet(),
        descriptorsToPrint = mapOf(TypeChange.getDescriptor() to { it.name !in MODIFIERS }),
        parameters = "MoveEnum move, struct Pokemon *mon, ItemEnum item, Type type1, Type type2",
        returnType = "Type",
    )

    val REQUIRE_CONFIG =
        Requirement.TEMPLATE_CONFIG.copy(parameters = "BattlerTarget target, MoveEnum move, struct Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2")

    override fun generate(writer: OutputStreamWriter) {
        val typeChanges = groupAttackData { it.typeChangeList }

        writer.appendLine(
            """
            |Type GetBattleMoveType(u8 battlerAtk, u8 battlerDef, MoveEnum move, Type baseType) {
            |${IND}switch (gBattleMoves[move].effect) {
            |${
                typeChanges.entries.joinToString("\n") {
                    typeChangeCase(it, "move, battlerAtk")
                }
            }
            |$IND}
            |${IND}return baseType;
            |}
            |""".trimMargin()
        )

        writer.appendLine(
            """
            |Type GetOutOfBattleMoveType(struct Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2, MoveEnum move, Type baseType) {
            |${IND}switch (gBattleMoves[move].effect) {
            |${
                typeChanges.entries.joinToString("\n") {
                    typeChangeCase(it, "move, mon, item, type1, type2", "move, mon, species, item, type1, type2")
                }
            }
            |$IND}
            |${IND}return baseType;
            |}
            |""".trimMargin()
        )

        writer.appendLine()
    }

    private fun typeChangeCase(
        entry: Map.Entry<List<TypeChange>, List<MoveBehavior>>,
        params: String,
        requireParams: String? = null,
        ind1: String = "$IND$IND",
        ind2: String = ind1 + IND,
        ind3: String = ind2 + IND,
    ) =
        """
        |${ind1}${entry.value.joinToString("\n$ind1") { "case $it:" }}
        |${ind2}${
            entry.key.joinToString("\n$ind2") {
                if (it.requireList.isEmpty()) {
                    "return ${powerCall(it, params)};"
                } else {
                    "if (${Requirement.getRequirementString(it.requireList, requireParams)}) {\n${ind3}return ${
                        powerCall(
                            it,
                            params
                        )
                    };\n$ind2}"
                }
            }
        }
        |${ind2}break;
        |""".trimMargin()

    private fun powerCall(typeChange: TypeChange, params: String) =
        typeChange.allFields.entries.single { it.key.name !in MODIFIERS }.let {
            printTemplateCall(
                params,
                it.value,
                it.key,
                TypeChange.getDescriptor(),
                mapOf()
            ).single()
        }
}