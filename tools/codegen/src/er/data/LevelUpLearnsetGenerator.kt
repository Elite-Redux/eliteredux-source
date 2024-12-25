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

    private fun learnsetString(species: Species, learnset: Learnset): String =
        if (learnset.levelList.isNotEmpty()) (
                """
                |$IND[${species.id}] = (LevelUpMove[]) {
                |$IND$IND${
                    learnset.levelList.flatMap { it.moveList.map { move -> it.level to move } }
                        .sortedWith(compareBy<Pair<Int, MoveEnum>> { it.first }.thenBy { it.second.name })
                        .joinToString("\n$IND$IND") { "{ .level = ${it.first}, .move = ${it.second} }," }
                }
                |$IND$IND{0}},
                |
                |""".trimMargin()) else {
            ""
        }

    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
                |$HEADER
                |
                |const LevelUpMove *const gLevelUpLearnsets[REAL_SPECIES_COUNT] = {
                |${
                SPECIES_LIST.filter { it.id != SpeciesEnum.SPECIES_EGG }.joinToString("\n") {
                    learnsetString(it, findLearnsetForSpecies(it))
                }
            }
                |};
            |""".trimMargin()
        )
    }
}
