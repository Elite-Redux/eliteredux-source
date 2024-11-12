#ifndef GUARD_BATTLE_AI_NEW_UTIL_H
#define GUARD_BATTLE_AI_NEW_UTIL_H

#include "global.h"
#include "battle_main.h"

extern const u16 gHitOdds[8][99];
extern const u16 gTenHitsMultiplier[99];
extern const u8 gTripleKickHitExpected[99];
extern const u16 gTripleKickMultiplier[99];
extern const u16 gCompoundOdds[8][9];

int AdjustForChance(int chance, int score);
int BelowHalfHp(int battler);
int AreSameSide(int battler1, int battler2);
int IsSleeping(int battler, struct AiData* aiData);
int SeesSunlight(int battler, struct AiData* aiData);
int AdjustForChance(int chance, int score);
int AlwaysCancelled(int battlerAtk, int move, struct MoveContainer* moveContainer);
union SpeedValue AiPerformMoveSpeedCalculation(int battlerAtk, int battlerDef, int move);
int CheckCancelled(int battlerAtk, int battlerDef, int move, struct MoveState* moveState, struct AiData* aiData);

#endif