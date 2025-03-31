#ifndef GUARD_BATTLE_AI_NEW_H
#define GUARD_BATTLE_AI_NEW_H

#include "global.h"
#include "battle_main.h"

#define AI_CHOICE_FLEE 4
#define AI_CHOICE_WATCH 5
#define AI_CHOICE_SWITCH 7

#define AI_SCORE(value) ((1 << UQ_4_12_PRECISION) * value)
#define AI_SCORE_UNUSABLE AI_SCORE(-1000)
#define AI_SCORE_IMMUNE 0
#define AI_SCORE_ADJUST(percent, score) AdjustForChance(percent, score)

enum {
    AI_MISSES_THIS_TURN = 1,
    AI_MISSES_THIS_TURN_IF_FIRST = 2,
};

enum {
    AI_EFFECTIVENESS_NEUTRAL,
    AI_EFFECTIVENESS_SE,
    AI_EFFECTIVENESS_NVE,
};

typedef enum {
    AI_PHASE_BASIC,
    AI_PHASE_ACCURACY,
    AI_PHASE_DAMAGE,
    // AI_PHASE_RETALIATION,
    // AI_PHASE_TURN_TWO,
    // AI_PHASE_SECONDARY,
    // AI_PHASE_DISABLE,
    // AI_PHASE_FLINCH,
    // AI_PHASE_PROTECT,
    // AI_PHASE_END_TURN,
    AI_PHASE_COUNT,
    AI_PHASE_DAMAGE_ROUGH,
} AiProcessingPhase;

#define UQ_CLAMP_EXPECT(value) ((value) >> ((4 + UQ_4_12_PRECISION) - 8))
#define UQ_UNCLAMP_EXPECT(value) ((value) << ((4 + UQ_4_12_PRECISION) - 8))

struct MoveState {
    union SpeedValue speedValue;
    int score;
    u16 koChance;
    u16 damage;
    u16 negatedDamage;
    u8 multiHitExpect;
    u8 accuracy;
    u8 overkillInHalves:2;
    u8 type:5;
    u8 effectiveness:2;
    u8 critChance:3;
    u8 target:2;
    u8 falseSwipe:1;
    u8 contact:1;
    u8 seeKo:1;
    u8 superEffective:1;
    u8 breakShield:1;
    u8 cancelled:1;
    u8 noVariance:1;
    u8 missesThisTurn:2;
    u8 critKo:1;
};

enum
{
    AI_CANCEL_DETERMINISTIC,
    AI_CANCEL_25,
    AI_CANCEL_33,
    AI_CANCEL_50,
};

struct MoveContainer
{
    struct MoveState targetData[3];
    u16 move;
    u8 targetFlags;
    u8 startingMoveType:5;
    u8 count:2;
    u8 cancellationState:2;
    u8 unusable:1;
    u8 fixedDamage:1;
    u8 multihitType:4;
    u8 isTwoTurn:1;
};


struct BattlerState
{
    u16 hp;
    u16 shield;
    u8 sash:1;
};


struct AiData {
    struct MoveContainer moveState[MAX_BATTLERS_COUNT][MAX_MON_MOVES + 1];
    struct BattlerState battlerState[MAX_BATTLERS_COUNT];
};

struct DisguiseSimulation {
    AbilityEnum ability;
};

int GetAiDecision(int battler);

#endif