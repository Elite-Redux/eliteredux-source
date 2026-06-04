#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"

typedef struct BattleSkill {
    const u8* name;
} BattleSkill;

#define BATTLE_SKILL_COUNT 1

extern const BattleSkill gBattleSkills[BATTLE_SKILL_COUNT];

#ifdef __cplusplus
}
#endif
