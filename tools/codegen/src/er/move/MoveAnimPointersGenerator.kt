package er.move

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVES_LIST
import er.proto.MoveEnum
import java.io.OutputStreamWriter

object MoveAnimPointersGenerator : Generator{
    override fun generate(writer: OutputStreamWriter) {
        val maxMoveEnum = MoveEnum.entries.filter { it != MoveEnum.UNRECOGNIZED }.maxOf { it.number }
        writer.appendLine(".macro animtable")

        val movesById = MOVES_LIST.associateBy { it.id.number }
        for (i in 0..maxMoveEnum) {
            val move = movesById[i]
            when {
                move == null -> writer.appendLine("$IND.4byte Move_NONE")
                move.hasUsesAnimation() -> writer.appendLine("$IND.4byte Move_${move.usesAnimation.toString().removePrefix("MOVE_")} @ ${move.id}")
                else -> writer.appendLine("$IND.4byte Move_${move.id.toString().removePrefix("MOVE_")}")
            }
        }
        writer.appendLine(".endm")
    }

}