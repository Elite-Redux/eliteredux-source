#ifndef GUARD_SCRIPT_CONDITIONS_H
#define GUARD_SCRIPT_CONDITIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "generated/constants/battle_move_effects.h"
#include "global.h"

u32 UpdateBaseDamage(u32 baseDamage, u8 battlerAtk, u8 battlerDef, MoveEnum move, MoveBehaviorEnum effect);

#ifdef __cplusplus
}
#endif
#endif