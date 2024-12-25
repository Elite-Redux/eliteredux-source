package er.data

import er.FileGenerator.HEADER
import er.FileGenerator.IND
import er.Generator
import er.TextprotoReader.SPECIES_LIST
import er.proto.Species
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object EvolutionsGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val megas = SPECIES_LIST.flatMap { it.megaList }.groupBy { it.from }
        val primals = SPECIES_LIST.flatMap { it.primalList }.groupBy { it.from }
        val formShifts = SPECIES_LIST.groupBy({ it.formShiftOf }, { it.id to it.formShiftGender })

        writer.appendLine(
            """
            |$HEADER
            |const struct Evolution *const gEvolutionTable[REAL_SPECIES_COUNT] = {
            |""".trimMargin()
        )
        for (species in SPECIES_LIST) {
            if (species.evoList.isEmpty() && species.id !in megas && species.id !in primals && species.id !in formShifts) continue
            writer.appendLine("$IND[${species.id}] = (Evolution[]) {")
            species.evoList.forEach {
                val evo = when {
                    it.gender == Species.Gender.MALE -> "EVO_LEVEL_MALE"
                    it.gender == Species.Gender.FEMALE -> "EVO_LEVEL_MALE"
                    species.id == SpeciesEnum.SPECIES_SHEDINJA -> "EVO_LEVEL_SHEDINJA"
                    species.id == SpeciesEnum.SPECIES_NINJASK -> "EVO_LEVEL_NINJASK"
                    else -> "EVO_LEVEL"
                }
                writer.appendLine("$IND$IND{$evo, ${it.level}, ${it.to}},")
            }
            megas[species.id].orEmpty().forEach {
                if (it.hasMove()) {
                    writer.appendLine("$IND$IND{EVO_MOVE_MEGA_EVOLUTION,${it.move},${it.from}},")
                } else {
                    writer.appendLine("$IND$IND{EVO_MEGA_EVOLUTION,${it.item},${it.from}},")
                }
            }
            primals[species.id].orEmpty().forEach {
                writer.appendLine("$IND$IND{EVO_PRIMAL_REVERSION,${it.item},${it.from}},")
            }
            writer.appendLine(
                """
                |$IND$IND{0}},
                |""".trimMargin()
            )
        }
        writer.appendLine(
            """
            |};
            |
            |const struct Evolution *const gFormChangeTable[REAL_SPECIES_COUNT] = {
            |""".trimMargin()
        )

        for (species in SPECIES_LIST) {
            val shiftBase = when {
                species.id == SpeciesEnum.SPECIES_NONE -> continue
                species.id in formShifts -> species.id
                species.formShiftOf != SpeciesEnum.SPECIES_NONE -> species.formShiftOf
                else -> continue
            }
            writer.appendLine(
                """
                |$IND[${species.id}] = (Evolution[]) {
                |$IND$IND{EVO_FORM_SHIFT, 1, $shiftBase},""".trimMargin()
            )
            formShifts[shiftBase]!!.forEach { (mon, gender) ->
                when (gender) {
                    Species.Gender.MALE -> writer.appendLine("$IND$IND{EVO_FORM_SHIFT_GENDER, MON_MALE, $mon},")
                    Species.Gender.FEMALE -> writer.appendLine("$IND$IND{EVO_FORM_SHIFT_GENDER, FEMALE_MALE, $mon},")
                    else -> writer.appendLine("$IND$IND{EVO_FORM_SHIFT, 1, $mon},")
                }
            }
            writer.appendLine("$IND$IND{0}},")
        }

        writer.appendLine("};")
    }
}
