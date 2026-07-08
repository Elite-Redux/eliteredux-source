#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "generated/constants/battle_skills.h"

typedef int (*BattleSkillOnEntryHandler)(BattleSkillEnum skill, u8 battler);
typedef int (*BattleSkillOnBattleStart)(BattleSkillEnum skill, u8 battler);
typedef int (*BattleSkillOnAttackerHandler)(AbilityEnum ability, u8 battler, u8 target, MoveEnum move, Type moveType);

typedef struct BattleSkill {
    const u8* name;
    BattleSkillOnBattleStart onBattleStart;
    BattleSkillOnEntryHandler onEntry;
    BattleSkillOnAttackerHandler onAttacker;
} BattleSkill;

extern const BattleSkill gBattleSkills[BATTLE_SKILL_COUNT];

#ifdef __cplusplus
}
#endif
