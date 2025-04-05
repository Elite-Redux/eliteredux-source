package er.move

import com.google.protobuf.Descriptors
import com.google.protobuf.Descriptors.EnumValueDescriptor
import com.google.protobuf.ProtocolMessageEnum
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVES_LIST
import er.GeneratorUtils.getOption
import er.proto.*
import er.proto.MoveBehaviorOuterClass.*
import er.proto.MoveListOuterClass.*
import java.io.OutputStreamWriter

object BattleMovesGenerator : Generator {
    private val PROTECT_AFFECTED_TARGETS = setOf(
        MoveTarget.BOTH,
        MoveTarget.DEPENDS,
        MoveTarget.FOES_AND_ALLY,
        MoveTarget.RANDOM,
        MoveTarget.SELECTED,
        MoveTarget.USER_OR_SELECTED
    )

    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |const struct BattleMove gBattleMoves[MOVES_COUNT] = {
        """.trimMargin()
        )

        for (move in MOVES_LIST) {
            writer.appendLine(
                """
                |$IND[${move.id}] = {
                |$IND$IND.effect = ${move.effect},
                |$IND$IND.split = SPLIT_${move.split},""".trimMargin()
            )

            if (move.splitModifier != SplitFlag.USE_BASE_SPLIT) {
                writer.appendLine("$IND$IND.splitFlag = ${move.splitModifier},")
            }

            if (move.power > 0) writer.appendLine("$IND$IND.power = ${move.power},")

            writer.appendLine("$IND$IND.type = ${move.type},")

            if (move.type2 != Type.TYPE_NONE) writer.appendLine("$IND$IND.type2 = ${move.type2},")

            if (move.accuracy > 0) writer.appendLine("$IND$IND.accuracy = ${move.accuracy},")

            writer.appendLine("$IND$IND.pp = ${move.pp},")

            if (move.effectChance > 0) writer.appendLine("$IND$IND.secondaryEffectChance = ${move.effectChance},")

            writer.appendLine("$IND$IND.target = MOVE_TARGET_${move.target},")

            if (move.priority != 0) writer.appendLine("$IND$IND.priority = ${move.priority},")

            val sheerForce = when {
                move.split == MoveSplit.STATUS -> false
                move.effectChance == 0 -> false
                move.effect.getOption(noSheerForce) -> false
                move.argument.effect.effect.getOption(noSheerForce) -> false
                else -> true
            }

            val kingsRock = when {
                move.split == MoveSplit.STATUS -> false
                move.effectChance > 0 && move.effect.getOption(flinchEffect) -> false
                move.effectChance > 0 && move.argument.effect.effect.getOption(flinchEffect) -> false
                move.effect.getOption(noKingsRock) -> false
                else -> true
            }

            val protectAffected = !move.ignoresProtect && move.target in PROTECT_AFFECTED_TARGETS

            val noParentalBond = move.noParentalBond || move.effect.getOption(noParentalBond) || move.hitCount > 1

            val twoTurn = move.twoTurn || move.effect.getOption(twoTurn)

            val bitFlags = listOfNotNull(
                "FLAG_PROTECT_AFFECTED".takeIf { protectAffected },
                "FLAG_KINGS_ROCK_AFFECTED".takeIf { kingsRock },
                "FLAG_SHEER_FORCE_BOOST".takeIf { sheerForce }).toMutableList()
            val fieldFlags = listOfNotNull(
                "parentalBondBanned".takeIf { noParentalBond },
                "twoTurnMove".takeIf { twoTurn }).toMutableList()

            for ((descriptor, value) in move.allFields) {
                if (!descriptor.getOption(isFlag)) continue

                val flag = when (val type = descriptor.type) {
                    Descriptors.FieldDescriptor.Type.BOOL -> {
                        if (!(value as Boolean)) continue
                        descriptor.getOption(flagCodeValue)
                    }

                    Descriptors.FieldDescriptor.Type.ENUM -> (value as EnumValueDescriptor).getOption(enumFlagCodeValue)
                    else -> TODO("Field data type $type unsupported for flags")
                }

                if (flag.isBlank()) continue
                if (flag.startsWith("FLAG_")) bitFlags += flag else fieldFlags += flag
            }

            if (bitFlags.isNotEmpty()) writer.appendLine("$IND$IND.flags = ${bitFlags.joinToString(" | ")},")

            fieldFlags.forEach { writer.appendLine("$IND$IND.$it = TRUE,") }

            val argument = when (move.argument.argumentCase) {
                Argument.ArgumentCase.TYPE -> move.argument.type.toString()
                Argument.ArgumentCase.EFFECT -> {
                    var output = move.argument.effect.effect.toString()
                    if (move.argument.effect.certain) output += " | MOVE_EFFECT_CERTAIN"
                    if (move.argument.effect.affectsUser) output += " | MOVE_EFFECT_AFFECTS_USER"
                    output
                }

                Argument.ArgumentCase.INT -> move.argument.int.toString()
                Argument.ArgumentCase.OTHER -> move.argument.other
                Argument.ArgumentCase.STATUS -> move.argument.status.toString()
                Argument.ArgumentCase.MISC -> move.argument.misc.toString()
                Argument.ArgumentCase.ARGUMENT_NOT_SET, null -> ""
            }

            if (argument.isNotBlank()) writer.appendLine("$IND$IND.argument = $argument,")

            writer.appendLine("$IND},")
        }
        writer.appendLine("};")
    }
}