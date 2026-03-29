package er

import er.GeneratorUtils.ABILITIES_LIST
import er.GeneratorUtils.ITEMS_LIST
import er.GeneratorUtils.MOVES_LIST
import er.GeneratorUtils.SPECIES_LIST
import er.GeneratorUtils.data
import er.GeneratorUtils.expandLearnset
import er.GeneratorUtils.findLearnsetForSpecies
import er.proto.BattleFrontierListKt.battleFrontierMon
import er.proto.BattleFrontierSet
import er.proto.BattleFrontierSet.DoublesSets.DOUBLES_ONLY
import er.proto.BattleFrontierSet.DoublesSets.SINGLES_OR_DOUBLES
import er.proto.MoveEnum.MOVE_HIDDEN_POWER
import er.proto.MoveEnum.MOVE_SECRET_POWER
import er.proto.Nature
import er.proto.Species
import er.proto.SpeciesEnum
import er.proto.SpeciesEnum.SPECIES_NONE
import er.proto.Type
import er.proto.battleFrontierList
import er.proto.battleFrontierSet
import er.proto.copy
import java.io.File

/** Temp tool to parse battle frontier sets */
object SetReader {
  val speciesNamesToSpecies =
    SPECIES_LIST.associateWith { it.id }
      .mapKeys { (species, _) ->
        when {
          species.hasDex() -> species.dex.name
          species.longName.isNotEmpty() -> species.longName
          species.hasFormOf() ->
            when (species.regionPrefix) {
              Species.Region.REGION_NONE -> ""
              Species.Region.ALOLA -> species.formOf.data.dex.name + " Alolan"
              Species.Region.GALAR -> species.formOf.data.dex.name + " Galarian"
              Species.Region.HISUI -> species.formOf.data.dex.name + " Hisui"
              Species.Region.PALDEA -> species.formOf.data.dex.name + " Paldean"
              Species.Region.REDUX -> species.formOf.data.dex.name + " Redux"
              Species.Region.UNRECOGNIZED -> ""
            }
          else -> ""
        }
      } - ""

  val itemNameToItem = ITEMS_LIST.associate { it.name.lowercase() to it.id }
  val abilityNameToAbility = ABILITIES_LIST.associate { it.name.lowercase() to it.id }
  val moveNameToMove = MOVES_LIST.associate { it.name.lowercase() to it.id }

  @JvmStatic
  fun main(args: Array<String>) {
    val map = mutableMapOf<Pair<SpeciesEnum, Int>, BattleFrontierSet>()
    var indices = emptyList<Int>()
    var step = 0
    var curSpecies = SPECIES_NONE
    for (file in File("./New Folder").listFiles()) {
      for (line in file.reader().readLines()) {
        val pieces = line.split(",").map { it.trim() }
        when (step) {
          0 -> {
            val name = pieces[0].replace("-", " ")
            val species =
              speciesNamesToSpecies[name]
                ?: continue.also {
                  if (name != "Placeholder" && name.isNotEmpty()) println("Bad name: $name")
                }

            curSpecies = species

            indices = buildList {
              for ((idx, piece) in pieces.withIndex().drop(1)) {
                val speciesAndItem = piece.split("@").map { it.trim() }
                if (speciesAndItem.size != 2) continue
                val (lineName, item) = speciesAndItem
                if (lineName.replace("-", " ") != name) continue
                add(idx)
                map[species to idx] = battleFrontierSet {
                  val itemId = itemNameToItem[item.lowercase()]
                  if (itemId != null) {
                    this.item = itemId
                  } else {
                    illegal += "Unknown item $item"
                  }
                }
              }
            }
            step = 1
          }
          1 -> step++
          2 -> {
            step++
            for (idx in indices) {
              val piece = pieces[idx].removeSuffix("Nature").trim()
              val enum =
                try {
                  Nature.valueOf("NATURE_" + piece.uppercase())
                } catch (e: Exception) {
                  null
                }

              map[curSpecies to idx] =
                map[curSpecies to idx]!!.copy {
                  if (enum != null) {
                    nature = enum
                  } else {
                    illegal += "Unknown nature $piece"
                  }
                }
            }
          }
          3 -> {
            step++
            for (idx in indices) {
              val piece = pieces[idx].removePrefix("Ability: ").lowercase()
              val enum = abilityNameToAbility[piece]

              map[curSpecies to idx] =
                map[curSpecies to idx]!!.copy {
                  if (enum != null) {
                    ability = enum
                    if (enum !in curSpecies.data.abilityList)
                      illegal += "$curSpecies does not have $enum"
                  } else {
                    illegal += "Unknown ability $piece"
                  }
                }
            }
          }
          4 -> {
            step++
            for (idx in indices) {
              val piece = pieces[idx].removePrefix("EVs: ")
              val subpieces = piece.split("/").map { it.trim().takeLast(3) }

              map[curSpecies to idx] =
                map[curSpecies to idx]!!.copy {
                  for (subpiece in subpieces) {
                    when (subpiece.lowercase()) {
                      " hp" -> hpUp = true
                      "atk" -> atkUp = true
                      "def" -> defUp = true
                      "spa" -> spatkUp = true
                      "spd" -> spdefUp = true
                      "spe" -> speedUp = true
                      else -> illegal += "Unknown EV type $subpiece"
                    }
                  }
                }
            }
          }
          5,
          6,
          7,
          8 -> {
            step++
            for (idx in indices) {
              val piece = pieces[idx].removePrefix("- ").lowercase()
              val enum = moveNameToMove[piece]

              map[curSpecies to idx] =
                map[curSpecies to idx]!!.copy {
                  if (enum != null) {
                    move += enum
                    val learnset = findLearnsetForSpecies(curSpecies.data)

                    if (
                      enum !in
                        expandLearnset(learnset, curSpecies.data) +
                          learnset.levelList.flatMap { it.moveList }
                    )
                      illegal += "$curSpecies does not learn $enum"
                  } else if (
                    piece.startsWith("h.p.") ||
                      piece.startsWith("hidden power") ||
                      piece.startsWith("h. p.")
                  ) {
                    val type =
                      piece
                        .removePrefix("h.p. ")
                        .removePrefix("h. p. ")
                        .removePrefix("hidden power ")
                    val typeEnum =
                      try {
                        Type.valueOf("TYPE_${type.uppercase()}")
                      } catch (e: Exception) {
                        null
                      }
                    if (typeEnum != null) {
                      this.hpType = typeEnum
                      this.move += MOVE_HIDDEN_POWER
                    } else {
                      illegal += "Unknown Hidden Power type $type"
                    }
                  } else if (piece.startsWith("s.p.")) {
                    val type = piece.removePrefix("s.p. ")
                    val typeEnum =
                      try {
                        Type.valueOf("TYPE_${type.uppercase()}")
                      } catch (e: Exception) {
                        null
                      }
                    if (typeEnum != null) {
                      this.hpType = typeEnum
                      this.move += MOVE_SECRET_POWER
                    } else {
                      illegal += "Unknown Secret Power type $type"
                    }
                  } else {
                    illegal += "Unknown move $piece"
                  }
                }
            }
          }
          9 -> {
            step++
            for (idx in indices) {
              val piece = pieces[idx]

              map[curSpecies to idx] =
                map[curSpecies to idx]!!.copy {
                  when (piece) {
                    "Both Formats" -> this.allowedFormats = SINGLES_OR_DOUBLES
                    "Doubles Only" -> this.allowedFormats = DOUBLES_ONLY
                    else -> illegal += "Unknown doubles or singles type: $piece"
                  }
                }
            }
          }
          10 -> {
            step++
            for (idx in indices) {
              val piece = pieces[idx]

              map[curSpecies to idx] =
                map[curSpecies to idx]!!.copy {
                  when (piece) {
                    "Any format" -> this.restricted = false
                    "Restricted" -> this.restricted = true
                    else -> illegal += "Unknown restricted type: $piece"
                  }
                }
            }
          }
          11 -> step = 0
        }
      }
    }
    File("../../proto/BattleFrontierList.textproto").writeText(
        battleFrontierList {
            this.species +=
              map.entries
                .groupBy { it.key.first }
                .mapValues { (_, values) -> values.map { it.value } }
                .map { (species, sets) ->
                  battleFrontierMon {
                    id = species
                    set += sets
                  }
                }
          }
          .toString()
      )
  }
}
