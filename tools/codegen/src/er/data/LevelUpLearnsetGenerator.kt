package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.Generator
import er.TextprotoReader.NO_EGG_LIST
import er.TextprotoReader.createDedupMaps
import er.data.TutorLearnsetGenerator.findLearnsetForSpecies
import er.proto.MoveEnum
import er.proto.Species.Learnset
import java.io.OutputStreamWriter

object LevelUpLearnsetGenerator : Generator {

    const val PREFIX = "__sLevelUpMoveset_"

    private fun learnsetString(index: Int, learnset: Learnset): String =
        """
        |static const LevelUpMove $PREFIX$index[] = {
        |$IND${
            learnset.levelList.flatMap { it.moveList.map { move -> it.level to move } }
                .sortedWith(compareBy<Pair<Int, MoveEnum>> { it.first }.thenBy { it.second.name })
                .joinToString("\n$IND") { "{ .level = ${it.first}, .move = ${it.second} }," }
        }
        |$IND{0}
        |};
        |""".trimMargin()

    override fun generate(writer: OutputStreamWriter) {
        val (learnsetIds, speciesIds) = NO_EGG_LIST.map { findLearnsetForSpecies(it) to it.id }.createDedupMaps()

        writer.appendLine(HEADER)
        writer.appendLine()
        learnsetIds.forEach { writer.appendLine(learnsetString(it.value, it.key)) }

        writer.appendLine(
            """
            |const LevelUpMove *const gLevelUpLearnsets[REAL_SPECIES_COUNT] = {
            |$IND${
                NO_EGG_LIST.joinToString("\n$IND") { "[${it.id}] = $PREFIX${speciesIds[it.id]}," }
            }
            |};
            |""".trimMargin()
        )
    }
}
