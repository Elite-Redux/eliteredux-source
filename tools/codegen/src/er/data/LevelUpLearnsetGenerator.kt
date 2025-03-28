package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.data.TutorLearnsetGenerator.findLearnsetForSpecies
import er.proto.MoveEnum
import er.proto.Species.Learnset
import java.io.OutputStreamWriter

object LevelUpLearnsetGenerator : Generator {
    private const val PREFIX = "__sLevelUpMoveset_"

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

        learnsetIds.forEach { writer.appendLine(learnsetString(it.value, it.key)) }

        speciesIds.printLookupTable("const LevelUpMove *const gLevelUpLearnsets[$REAL_SPECIES_COUNT]", PREFIX, writer)
    }
}
