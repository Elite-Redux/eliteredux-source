package er.data

import er.FileGenerator.header
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.REAL_SPECIES_COUNT
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.proto.MoveEnum
import er.proto.Species
import er.proto.Species.Learnset
import er.proto.Species.Learnset.UniversalTutors
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object TutorLearnsetGenerator : Generator {
    fun findLearnsetForSpecies(species: Species): Learnset =
        when {
            species.id == SpeciesEnum.SPECIES_NONE -> species.learnset
            species.hasLearnset() -> species.learnset
            species.usesLearnset != SpeciesEnum.SPECIES_NONE -> findLearnsetForSpecies(SPECIES_MAP[species.usesLearnset]!!)
            species.formShiftOf != SpeciesEnum.SPECIES_NONE -> findLearnsetForSpecies(SPECIES_MAP[species.formShiftOf]!!)
            species.megaList.isNotEmpty() -> findLearnsetForSpecies(SPECIES_MAP[species.megaList.first().from]!!)
            species.primalList.isNotEmpty() -> findLearnsetForSpecies(SPECIES_MAP[species.primalList.first().from]!!)
            else -> findLearnsetForSpecies(SPECIES_MAP[species.formOf]!!)
        }

    private fun expandLearnset(learnset: Learnset, species: Species) =
        UNIVERSAL_TUTORS + UNIVERSAL_ATTACKS.takeIf { learnset.universalTutors != UniversalTutors.NO_ATTACKS }
            .orEmpty() + listOfNotNull(MoveEnum.MOVE_ATTRACT.takeIf { !species.genderless }) + learnset.tutorList

    private val UNIVERSAL_TUTORS = listOf(
        MoveEnum.MOVE_ENDURE,
        MoveEnum.MOVE_HELPING_HAND,
        MoveEnum.MOVE_PROTECT,
        MoveEnum.MOVE_REST,
        MoveEnum.MOVE_SLEEP_TALK,
        MoveEnum.MOVE_SUBSTITUTE,
    )
    private val UNIVERSAL_ATTACKS = listOf(
        MoveEnum.MOVE_HIDDEN_POWER,
        MoveEnum.MOVE_SECRET_POWER,
        MoveEnum.MOVE_RETURN,
    )

    private const val PREFIX = "__sTutorMoveset_"

    private fun learnsetString(id: Int, learnset: List<MoveEnum>): String = """
        |static const TutorUnion $PREFIX$id = { .fields = {
        |$IND${learnset.joinToString("\n$IND") { ".TUTOR_BIT_FIELD(${it.number}) = TRUE," }}
        |${IND}}};
        |""".trimMargin()

    override fun generate(writer: OutputStreamWriter) {
        val (tutorIds, speciesIds) = NO_EGG_LIST.map { expandLearnset(findLearnsetForSpecies(it), it) to it.id }
            .createDedupMaps()

        writer.appendLine(
            """
            |$header
            |${tutorIds.entries.joinToString("\n") { learnsetString(it.value, it.key) }}
            |""".trimMargin())

        speciesIds.printLookupTable("const TutorUnion *const gTutorLearnsets[$REAL_SPECIES_COUNT]", "&$PREFIX", writer)
    }
}
