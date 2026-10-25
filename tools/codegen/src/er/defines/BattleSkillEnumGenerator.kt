package er.defines

import er.GeneratorUtils.INLINE_SKILLS
import er.proto.BattleSkillEnum

private val CATALOG_SKILLS =
  BattleSkillEnum.entries.filter { it != BattleSkillEnum.UNRECOGNIZED }.associate { it.name to it.number }

object BattleSkillEnumGenerator :
  EnumGenerator(
    name = "BattleSkillEnum",
    keyMap =
      CATALOG_SKILLS +
        INLINE_SKILLS.keys.mapIndexed { index, id -> id to CATALOG_SKILLS.values.max() + 1 + index },
    countName = "BATTLE_SKILL_COUNT",
    forEachName = "FOR_EACH_BATTLE_SKILL",
  )
