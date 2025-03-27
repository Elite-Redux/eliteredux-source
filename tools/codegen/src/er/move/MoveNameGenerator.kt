package er.move

import er.FileGenerator.IND
import er.FileGenerator.header
import er.Generator
import er.GeneratorUtils.MOVES_LIST
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.data.SpeciesNameGenerator
import java.io.OutputStreamWriter

object MoveNameGenerator : Generator {
    private const val MOVE_NAME_SHORT_LENGTH = 13
    private const val MOVE_NAME_LONG_LENGTH = 19

    private enum class Name { SHORT, LONG }

    private const val PREFIX = "__sMoveName_"

    override fun generate(writer: OutputStreamWriter) {
        MOVES_LIST.forEach { check(it.shortName.length <= MOVE_NAME_SHORT_LENGTH) { "Move short name ${it.name} exceeds character limit of 13." } }
        MOVES_LIST.forEach { check(it.shortName.length <= MOVE_NAME_LONG_LENGTH) { "Move name ${it.name} exceeds character limit of 19." } }

        writer.appendLine(header)

        val idNamePairs = MOVES_LIST.map { it.shortName to (it.id to Name.SHORT) } + MOVES_LIST.map { it.name to (it.id to Name.LONG) }
        val (nameIds, moveIds) = idNamePairs.createDedupMaps()

        writer.appendLine(nameIds.entries.joinToString("\n") { """static const u8 ${PREFIX}${it.value}[] = _("${it.key}");""" })

        val shortMoveIds = moveIds.filterKeys { it.second == Name.SHORT }.mapKeys { it.key.first }
        val longMoveIds = moveIds.filterKeys { it.second == Name.LONG }.mapKeys { it.key.first }

        shortMoveIds.printLookupTable("const u8 *const gMoveNames[]", PREFIX, writer)
        longMoveIds.printLookupTable("const u8 *const gMoveNamesLong[]", PREFIX, writer)
    }
}