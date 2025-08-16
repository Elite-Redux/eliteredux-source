#ifndef GUARD_BEHAVIOR_MOVE_MOVE_BEHAVIOR_HH
#define GUARD_BEHAVIOR_MOVE_MOVE_BEHAVIOR_HH

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "generated/constants/moves.h"

int AdjustMovePowerC(u8, u8, MoveEnum, int);
Type GetBattleMoveTypeC(u8 battlerAtk, u8 battlerDef, MoveEnum move);
Type GetOutOfBattleMoveTypeC(struct Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2, MoveEnum move);

#ifdef __cplusplus
}
#endif
#endif
