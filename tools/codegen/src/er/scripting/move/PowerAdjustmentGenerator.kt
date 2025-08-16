package er.scripting.move

import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.Descriptors.FieldDescriptor
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVES_BEHAVIOR_CONFIGS_LIST
import er.proto.AttackBehaviorConfig.AdjustPower
import er.proto.AttackBehaviorConfig.AdjustPower.CustomScaling
import er.proto.MoveBehavior
import er.proto.MoveBehaviorConfig
import er.scripting.move.MoveBehaviorEnumConfigGenerator.printTemplateCall
import er.scripting.move.MoveScriptingUtils.groupAttackData
import java.io.OutputStreamWriter

object PowerAdjustmentGenerator : Generator {
    val TEMPLATE_CONFIG = TemplateConfig(
        additionalEnums = emptySet(), descriptorsToPrint = mapOf(
            AdjustPower.getDescriptor() to { it.name != "require" },
            CustomScaling.getDescriptor() to { it.name == "measurement" },
        ),
        "int basePower, u8 battlerAtk, u8 battlerDef"
    )

    override fun generate(writer: OutputStreamWriter) {
        val adjustments = groupAttackData { it.powerModifierList }
        writer.appendLine(
            """
            |int AdjustMovePower(u8 battlerAtk, u8 battlerDef, MoveEnum move, int basePower) {
            |${IND}switch (gBattleMoves[move].effect) {
            |${
                adjustments.entries.joinToString("\n") {
                    adjustPowerCase(
                        it,
                        "$IND$IND",
                        "$IND$IND$IND",
                        "$IND$IND$IND$IND"
                    )
                }
            }
            |$IND}
            |${IND}return basePower;
            |}
            |""".trimMargin()
        )
    }

    private fun adjustPowerCase(
        entry: Map.Entry<List<AdjustPower>, List<MoveBehavior>>,
        ind1: String,
        ind2: String,
        ind3: String
    ) =
        """
        |${ind1}${entry.value.joinToString("\n$ind1") { "case $it:" }}
        |${ind2}${
            entry.key.joinToString("\n$ind2") {
                if (it.requireList.isEmpty()) {
                    "return ${powerCall(it)};"
                } else {
                    "if (${Requirement.getRequirementString(it.requireList)}) {\n${ind3}return ${powerCall(it)};\n$ind2}"
                }
            }
        }
        |${ind2}break;
        |""".trimMargin()

    private fun powerCall(adjustPower: AdjustPower) =
        adjustPower.allFields.entries.single { it.key.name != "require" }.let {
            printTemplateCall(
                "basePower, battlerAtk, battlerDef",
                it.value,
                it.key,
                AdjustPower.getDescriptor(),
                mapOf(CustomScaling.getDescriptor().name to ::formulaCalculation)
            ).single()
        }

    private fun formulaCalculation(
        descriptor: Descriptor,
        field: FieldDescriptor,
        value: Any,
        parameters: String
    ): String {
        val formula = value as CustomScaling
        val base = when (formula.baseTypeCase) {
            CustomScaling.BaseTypeCase.BASE -> value.base.toString()
            CustomScaling.BaseTypeCase.USE_MOVE_BASE -> "basePower"
            CustomScaling.BaseTypeCase.BASETYPE_NOT_SET -> "0"
        }
        val calculationString = printTemplateCall(
            parameters,
            formula.measurement,
            CustomScaling.getDescriptor().findFieldByName("measurement"),
            CustomScaling.getDescriptor(),
            emptyMap()
        ).single()
        return when (formula.maxTypeCase) {
            CustomScaling.MaxTypeCase.MAX -> "max(${formula.max}, ($base + $calculationString))"
            CustomScaling.MaxTypeCase.MAX_BONUS -> "$base + max(${formula.max}, ($calculationString))"
            CustomScaling.MaxTypeCase.MAXTYPE_NOT_SET -> "$base + $calculationString"
        }
    }
}