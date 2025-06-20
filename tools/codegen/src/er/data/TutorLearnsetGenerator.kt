package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.expandLearnset
import er.GeneratorUtils.findLearnsetForSpecies
import er.GeneratorUtils.printLookupTable
import er.proto.MoveEnum
import er.proto.Species
import er.proto.Species.Learnset
import er.proto.Species.Learnset.UniversalTutors
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object TutorLearnsetGenerator : Generator {

    private const val PREFIX = "__sTutorMoveset_"

    private fun learnsetString(id: Int, learnset: List<MoveEnum>): String = """
        |static const TutorUnion $PREFIX$id = { .fields = {
        |$IND${learnset.distinct().joinToString("\n$IND") { ".TUTOR_BIT_FIELD(${it}) = TRUE," }}
        |${IND}}};
        |""".trimMargin()

    override fun generate(writer: OutputStreamWriter) {
        val (tutorIds, speciesIds) = NO_EGG_LIST.map { expandLearnset(findLearnsetForSpecies(it), it) to it.id }
            .createDedupMaps()

        writer.appendLine(
            """
            |${tutorIds.entries.joinToString("\n") { learnsetString(it.value, it.key) }}
            |""".trimMargin())

        speciesIds.printLookupTable("const TutorUnion *const gTutorLearnsets[$REAL_SPECIES_COUNT]", "&$PREFIX", writer)
    }
}
