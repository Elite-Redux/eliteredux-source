package er.data

import er.FileGenerator.header
import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.SPECIES_MAP
import er.GeneratorUtils.createDedupMaps
import er.GeneratorUtils.printLookupTable
import er.proto.Species
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object EvolutionsGenerator : Generator {
    private const val EVO_PREFIX = "__sEvoList_"
    private const val FORM_PREFX = "__sFormList_"

    private data class Evo(val method: String, val condition: String, val to: SpeciesEnum) {
        override fun toString() = "{$method, $condition, $to}"
    }

    override fun generate(writer: OutputStreamWriter) {
        val megas = SPECIES_LIST.flatMap { it.megaList }.groupBy { it.from }
        val primals = SPECIES_LIST.flatMap { it.primalList }.groupBy { it.from }

        writer.appendLine(header)

        val (evoIds, speciesEvoIds) = NO_EGG_LIST.map { species ->
            species.evoList.map {
                Evo(
                    when {
                        it.gender == Species.Gender.MALE -> "EVO_LEVEL_MALE"
                        it.gender == Species.Gender.FEMALE -> "EVO_LEVEL_MALE"
                        species.id == SpeciesEnum.SPECIES_SHEDINJA -> "EVO_LEVEL_SHEDINJA"
                        species.id == SpeciesEnum.SPECIES_NINJASK -> "EVO_LEVEL_NINJASK"
                        else -> "EVO_LEVEL"
                    }, it.level.toString(), it.to
                )
            } + megas[species.id].orEmpty().map {
                if (it.hasMove()) {
                    Evo("EVO_MOVE_MEGA_EVOLUTION", it.move.toString(), it.from)
                } else {
                    Evo("EVO_MEGA_EVOLUTION", it.item.toString(), it.from)
                }
            } + primals[species.id].orEmpty().map {
                Evo("EVO_PRIMAL_REVERSION", it.item.toString(), it.from)
            }.toSet() to species.id
        }.createDedupMaps()

        writer.appendLine(evoIds.entries.joinToString("\n") {
            """
            |static const Evolution *const $EVO_PREFIX${it.value} = {
            |$IND${it.key.joinToString("\n$IND") { evo -> "$evo," }}
            |${IND}0};
            |""".trimMargin()
        })

        speciesEvoIds.printLookupTable("const Evolution *const gEvolutionTable[REAL_SPECIES_COUNT]", EVO_PREFIX, writer)

        val reverseForms =
            NO_EGG_LIST.map { it.formShiftOf to it.id }.groupBy({ it.first }, { it.second }) - SpeciesEnum.SPECIES_NONE

        val (formIds, speciesFormIds) = NO_EGG_LIST.map { species ->
            val id = if (species.hasFormShiftOf()) species.formShiftOf else species.id
            reverseForms[id]?.let { listOf(id) + it }.orEmpty() to species.id
        }.createDedupMaps()

        writer.appendLine(formIds.entries.joinToString("\n") {
            """
            |static const Evolution *const $FORM_PREFX${it.value} = {
            |$IND${
                it.key.joinToString("\n$IND") { id ->
                    when (SPECIES_MAP[id]!!.formShiftGender) {
                        Species.Gender.MALE -> "{EVO_FORM_SHIFT_GENDER, MON_MALE, $id},"
                        Species.Gender.FEMALE -> "{EVO_FORM_SHIFT_GENDER, FEMALE_MALE, $id},"
                        else -> "{EVO_FORM_SHIFT, 1, $id},"
                    }
                }
            }
            |${IND}0};
            |""".trimMargin()
        })

        speciesFormIds.printLookupTable(
            "const Evolution *const gFormChangeTable[REAL_SPECIES_COUNT]",
            FORM_PREFX,
            writer
        )
    }
}
