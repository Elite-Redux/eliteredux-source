#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "generated/constants/battle_skills.h"

typedef int (*BattleSkillOnBattleStart)(BattleSkillEnum skill);
typedef int (*BattleSkillOnEntryHandler)(BattleSkillEnum skill, u8 battler);
typedef void (*BattleSkillOnDefensiveMultiplier)(u8 battler, u16* resistance, u16* modifier);

typedef struct BattleSkill {
    const u8* name;
    const u8* announceString;
    BattleSkillOnBattleStart onBattleStart;
    BattleSkillOnEntryHandler onEntry;
    BattleSkillOnDefensiveMultiplier onDefensiveMultiplier;
} BattleSkill;

extern const BattleSkill gBattleSkills[BATTLE_SKILL_COUNT];

#ifdef __cplusplus
}
#endif
