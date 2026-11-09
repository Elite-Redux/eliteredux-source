package er.data

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.BATTLE_FRONTIER_SETS
import er.GeneratorUtils.data
import er.proto.BattleFrontierSet
import er.proto.BattleFrontierSet.DoublesSets.DOUBLES_ONLY
import er.proto.SpeciesEnum
import java.io.OutputStreamWriter

object BattleFrontierMonGenerator : Generator {
  // TODO: fail codegen on sets with an ability their species doesn't have once the content team fixed them, instead of dropping them
  val SETS by lazy {
    BATTLE_FRONTIER_SETS.flatMap { mon ->
      mon.setList
        .filter { it.illegalList.isEmpty() && !it.restricted && it.allowedFormats != DOUBLES_ONLY && it.ability in mon.id.data.abilityList }
        .map { mon.id to it }
    }
  }
  val RESTRICTED_SETS by lazy {
    BATTLE_FRONTIER_SETS.flatMap { mon ->
      mon.setList
        .filter { it.illegalList.isEmpty() && it.restricted && it.allowedFormats != DOUBLES_ONLY && it.ability in mon.id.data.abilityList }
        .map { mon.id to it }
    }
  }

  fun writeSet(id: SpeciesEnum, set: BattleFrontierSet, writer: OutputStreamWriter) {
    val evs =
      listOfNotNull(
          "F_EV_SPREAD_HP".takeIf { set.hpUp },
          "F_EV_SPREAD_ATTACK".takeIf { set.atkUp },
          "F_EV_SPREAD_DEFENSE".takeIf { set.defUp },
          "F_EV_SPREAD_SP_ATTACK".takeIf { set.spatkUp },
          "F_EV_SPREAD_SP_DEFENSE".takeIf { set.spdefUp },
          "F_EV_SPREAD_SPEED".takeIf { set.speedUp },
        )
        .ifEmpty { listOf("0") }

    val abilityNumber = id.data.abilityList.indexOf(set.ability)
    check(abilityNumber >= 0) {
      "Mon $id does not learn ability ${set.ability} specified in frontier set $set"
    }

    writer.appendLine(
      """
        |$IND{
        |$IND$IND.species = $id,
        |$IND$IND.moves = {${set.moveList.joinToString()}},
        |$IND$IND.item = ${set.item},
        |$IND$IND.hpType = ${set.hpType},
        |$IND$IND.evSpread = ${evs.joinToString(" | ")},
        |$IND$IND.nature = ${set.nature},
        |$IND$IND.abilityNum = $abilityNumber,
        |$IND},"""
        .trimMargin()
    )
  }

  override fun generate(writer: OutputStreamWriter) {
    writer.appendLine("const struct FacilityMon gBattleFrontierMons[] = {")
    for ((id, set) in SETS) writeSet(id, set, writer)
    for ((id, set) in RESTRICTED_SETS) writeSet(id, set, writer)
    writer.appendLine("};")
  }
}
