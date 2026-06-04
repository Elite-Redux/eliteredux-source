package er.defines

import er.proto.BattleSkillEnum

object BattleSkillEnumGenerator :
  EnumGenerator(
    name = "BattleSkillEnum",
    keyMap =
      BattleSkillEnum.entries
        .filter { it != BattleSkillEnum.UNRECOGNIZED }
        .associate { it.name to it.number },
    countName = "BATTLE_SKILL_COUNT",
    forEachName = "FOR_EACH_BATTLE_SKILL",
  )
