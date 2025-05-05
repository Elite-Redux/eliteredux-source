package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.NO_EGG_LIST
import er.GeneratorUtils.REAL_SPECIES_COUNT
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

    private data class Evo(val method: EvoOrFormType, val condition: String, val to: SpeciesEnum) {
        override fun toString() = "{$method, $condition, $to}"
    }

    private enum class EvoOrFormType {
        EVO_FORM_SHIFT,
        EVO_FORM_SHIFT_GENDER,
        EVO_MEGA_EVOLUTION,
        EVO_PRIMAL_REVERSION,
        EVO_MOVE_MEGA_EVOLUTION,
        EVO_LEVEL_MALE,
        EVO_LEVEL_FEMALE,
        EVO_LEVEL_SHEDINJA,
        EVO_LEVEL_NINJASK,
        EVO_LEVEL,
        EVO_DEEVOLUTION,
        EVO_UNMEGA,
    }

    override fun generate(writer: OutputStreamWriter) {

        val deevos = SPECIES_LIST.filter { it.allowDeevolutionTo }
            .flatMap { species -> species.evoList.map { it.to to species.id } }.toMap()
            .mapValues { Evo(EvoOrFormType.EVO_DEEVOLUTION, "TRUE", it.value) }

        // De-evolutions are expected to come first
        val (evoIds, speciesEvoIds) = NO_EGG_LIST.map { species ->
            listOfNotNull(deevos[species.id]) + species.evoList.filter { it.to in SPECIES_MAP }.map {
                Evo(
                    when {
                        it.gender == Species.Gender.MALE -> EvoOrFormType.EVO_LEVEL_MALE
                        it.gender == Species.Gender.FEMALE -> EvoOrFormType.EVO_LEVEL_FEMALE
                        species.id == SpeciesEnum.SPECIES_SHEDINJA -> EvoOrFormType.EVO_LEVEL_SHEDINJA
                        species.id == SpeciesEnum.SPECIES_NINJASK -> EvoOrFormType.EVO_LEVEL_NINJASK
                        else -> EvoOrFormType.EVO_LEVEL
                    }, it.level.toString(), it.to
                )
            }.toSet() to species.id
        }.createDedupMaps()

        writer.appendLine(evoIds.entries.joinToString("\n") {
            """
            |static const Evolution *const $EVO_PREFIX${it.value} = (const Evolution[]) {
            |$IND${it.key.joinToString("\n$IND") { evo -> "$evo," }}
            |${IND}0};
            |""".trimMargin()
        })

        speciesEvoIds.printLookupTable(
            "const Evolution *const gEvolutionTable[$REAL_SPECIES_COUNT]",
            EVO_PREFIX,
            writer
        )

        val megas = SPECIES_LIST.flatMap { it.megaList.map { mega -> mega to it.id } }.groupBy { it.first.from }
            .mapValues { (_, values) ->
                values.map { (mega, to) ->
                    if (mega.hasMove()) {
                        Evo(EvoOrFormType.EVO_MOVE_MEGA_EVOLUTION, mega.move.toString(), to)
                    } else {
                        Evo(EvoOrFormType.EVO_MEGA_EVOLUTION, mega.item.toString(), to)
                    }
                }
            }
        val primals = SPECIES_LIST.flatMap { it.primalList.map { primal -> primal to it.id } }.groupBy { it.first.from }
            .mapValues { (_, values) ->
                values.map { (primal, to) ->
                    Evo(
                        EvoOrFormType.EVO_PRIMAL_REVERSION,
                        primal.item.toString(),
                        to
                    )
                }
            }

        val reverseForms =
            (NO_EGG_LIST.map { it.formShiftOf to it.id }
                .groupBy({ it.first }, { it.second }) - SpeciesEnum.SPECIES_NONE).mapValues { (base, forms) ->
                listOf(Evo(EvoOrFormType.EVO_FORM_SHIFT, "TRUE", base)) +
                        forms.map {
                            when (SPECIES_MAP[it]!!.formShiftGender) {
                                Species.Gender.MALE -> Evo(EvoOrFormType.EVO_FORM_SHIFT_GENDER, "MON_MALE", it)
                                Species.Gender.FEMALE -> Evo(EvoOrFormType.EVO_FORM_SHIFT_GENDER, "MON_FEMALE", it)
                                else -> Evo(EvoOrFormType.EVO_FORM_SHIFT, "TRUE", it)
                            }
                        }
            }

        val allForms = NO_EGG_LIST.map {
            val formId = if (it.hasFormShiftOf()) it.formShiftOf else it.id
            val unmega =
                listOfNotNull((it.megaList.map { m -> m.from } + it.primalList.map { p -> p.from }).firstOrNull()).map { from ->
                    Evo(
                        EvoOrFormType.EVO_UNMEGA,
                        "TRUE",
                        from
                    )
                }
            // Unmegas are expected to come first
            (unmega + reverseForms[formId].orEmpty() + megas[it.id].orEmpty() + primals[it.id].orEmpty()) to it.id
        }

        val (formIds, speciesFormIds) = allForms.createDedupMaps()

        writer.appendLine(formIds.entries.joinToString("\n") {
            """
            |static const Evolution *const $FORM_PREFX${it.value} = (const Evolution[]) {
            |$IND${it.key.joinToString("\n$IND") { evo -> "$evo," }}
            |${IND}0};
            |""".trimMargin()
        })

        speciesFormIds.printLookupTable(
            "const Evolution *const gFormChangeTable[$REAL_SPECIES_COUNT]",
            FORM_PREFX,
            writer
        )
    }
}
