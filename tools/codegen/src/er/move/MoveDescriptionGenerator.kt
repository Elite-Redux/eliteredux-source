package er.move

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVES_LIST
import er.text.FontMapping.Font.SMALL_NARROW
import java.io.OutputStreamWriter
import er.text.FontMapping.breakString

object MoveDescriptionGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val moveDescriptionTwoLine =
            MOVES_LIST.map { it.id to breakString(it.shortDescription, SMALL_NARROW, 154, 2) }
                .filter { it.second.isNotBlank() }
        val moveDescriptionFourLine =
            MOVES_LIST.map { it.id to breakString(it.description, SMALL_NARROW, 108, 4) }
                .filter { it.second.isNotBlank() }

        writer.appendLine("""
            |const u8 *const gMoveDescriptionPointers[MOVES_COUNT - 1] = {
            |$IND${moveDescriptionTwoLine.joinToString("\n$IND") { (id, desc) -> """[$id - 1] = $("$desc"),""" } }
            |};
            |""".trimMargin())

        writer.appendLine("""
            |const u8 *const gMoveFourLineDescriptionPointers[MOVES_COUNT - 1] = {
            |$IND${moveDescriptionFourLine.joinToString("\n$IND") { (id, desc) -> """[$id - 1] = $("$desc"),""" } }
            |};
            |""".trimMargin())
    }
}