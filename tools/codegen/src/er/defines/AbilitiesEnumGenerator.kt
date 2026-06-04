package er.defines

import er.GeneratorUtils.ABILITIES_LIST
import java.io.OutputStreamWriter

object AbilitiesEnumGenerator :
  EnumGenerator(
    name = "AbilityEnum",
    keyMap = ABILITIES_LIST.associate { it.id.name to it.id.number },
    countName = "ABILITIES_COUNT",
    forEachName = "FOR_EACH_ABILITY",
  )