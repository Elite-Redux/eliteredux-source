package er.move

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVES_LIST
import java.io.OutputStreamWriter

object MoveNameGenerator : Generator {
    private const val MOVE_NAME_SHORT_LENGTH = 13
    private const val MOVE_NAME_LONG_LENGTH = 19

    override fun generate(writer: OutputStreamWriter) {
        MOVES_LIST.forEach { check(it.shortName.length <= MOVE_NAME_SHORT_LENGTH) { "Move short name ${it.name} exceeds character limit of 13." } }
        MOVES_LIST.forEach { check(it.shortName.length <= MOVE_NAME_LONG_LENGTH) { "Move name ${it.name} exceeds character limit of 19." } }

        writer.appendLine(
            """
                |const u8 gMoveNames[MOVES_COUNT][$MOVE_NAME_SHORT_LENGTH] = {
                |$IND${MOVES_LIST.joinToString("\n$IND") { """[${it.id}] = _("${it.shortName}"),""" }}
                |};
                |
                |const u8 gMoveNamesLong[MOVES_COUNT][$MOVE_NAME_LONG_LENGTH] = {
                |$IND${MOVES_LIST.joinToString("\n$IND") { """[${it.id}] = _("${it.name}"),""" }}
                |};
            """.trimMargin()
        )
    }
}