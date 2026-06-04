#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "generated/constants/battle_skills.h"

typedef struct BattleSkill {
    const u8* name;
} BattleSkill;

extern const BattleSkill gBattleSkills[BATTLE_SKILL_COUNT];

#ifdef __cplusplus
}
#endif
