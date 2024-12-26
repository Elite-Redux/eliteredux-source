package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.Generator
import er.TextprotoReader.SPECIES_LIST
import er.TextprotoReader.SPECIES_MAP
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

    private fun learnsetString(species: Species, learnset: Learnset): String {
        val tutors =
            UNIVERSAL_TUTORS + UNIVERSAL_ATTACKS.takeIf { learnset.universalTutors != UniversalTutors.NO_ATTACKS }
                .orEmpty() + listOfNotNull(MoveEnum.MOVE_ATTRACT.takeIf { !species.genderless }) + learnset.tutorList

        return """
        |$IND[${species.id}] = TUTOR_LEARNSET
        |$IND$IND${tutors.joinToString("\n$IND$IND") { "TUTOR($it)" }}
        |$IND${IND}TUTOR_LEARNSET_END
        |
        |""".trimMargin()
    }

    override fun generate(writer: OutputStreamWriter) {
        writer.appendLine(
            """
            |$HEADER
            |#define TUTOR_LEARNSET (TutorUnion[]) {{ .fields = {
            |#define TUTOR(tutor) .TUTOR_BIT_FIELD(tutor) = TRUE,
            |#define TUTOR_LEARNSET_END }}},
            |
            |const TutorUnion *const gTutorLearnsets[REAL_SPECIES_COUNT] = {
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
