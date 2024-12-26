package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.Generator
import er.TextprotoReader.SPECIES_LIST
import er.data.TutorLearnsetGenerator.findLearnsetForSpecies
import er.proto.MoveEnum
import er.proto.Species
import er.proto.Species.Learnset
import er.proto.SpeciesEnum
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
        val learnsets =
            SPECIES_LIST.map { it.id to findLearnsetForSpecies(it) }.groupBy({ it.second }, { it.first }).toMap()

        val learnsetIds = learnsets.keys.withIndex().associate { it.value to it.index }
        val idForSpecies =
            learnsets.flatMap { (key, value) -> value.map { it to key } }.toMap().mapValues { learnsetIds[it.value] }

        writer.appendLine(HEADER)
        writer.appendLine()
        learnsetIds.forEach { writer.appendLine(learnsetString(it.value, it.key)) }

        writer.appendLine(
            """
            |const LevelUpMove *const gLevelUpLearnsets[REAL_SPECIES_COUNT] = {
            |$IND${
                SPECIES_LIST.filter { it.id != SpeciesEnum.SPECIES_EGG }
                    .joinToString("\n$IND") { "[${it.id}] = $PREFIX${idForSpecies[it.id]}," }
            }
            |};
            |""".trimMargin()
        )
    }
}
