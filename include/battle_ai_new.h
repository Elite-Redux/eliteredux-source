#ifndef GUARD_BATTLE_AI_MAIN_H
#define GUARD_BATTLE_AI_MAIN_H

#include "global.h"

#define AI_CHOICE_FLEE 4
#define AI_CHOICE_WATCH 5
#define AI_CHOICE_SWITCH 7

#define AI_SCORE(value) (value * 1024)
#define AI_SCORE_UNUSABLE AI_SCORE(-1000)
#define AI_SCORE_IMMUNE 0
#define AI_SCORE_ADJUST(percent, score) AdjustForChance(percent, score)

union SpeedValue {
    struct SpeedStruct {
        u16 afterYou:1;
        u16 dazedNegation:1;
        u16 goesFirst:2;
        u16 goesLastNegation:2;
        u16 effectiveSpeed;
    } speedStruct;
    u32 comparable;
};

struct MoveState {
    union SpeedValue speedValue;
    u16 koChance;
    u16 damage;
    u16 multiHitExpect;
    u8 multiplier;
    u8 critChance;
    u8 targetFlags;
    u8 accuracy;
    u8 falseSwipe:1;
    u8 contact:1;
    u8 seeKo:1;
    u8 superEffective:1;
    u8 breakSubstitute:1;
    u8 breakDisguise:1;
    u8 cancelled:1;
};

struct BattlerState
{
    u16 hp;
};


struct AiData {
    struct MoveState moveState[MAX_BATTLERS_COUNT][MAX_MON_MOVES];
    struct BattlerState battlerState[MAX_BATTLERS_COUNT];
};

struct DisguiseSimulation {
    int ability;
};

int GetAiDecision(int battler);
int AdjustForChance(int chance, int score);
int BelowHalfHp(int battler);
void PopulateAbilities(int battler, struct AiData* aiData);
int HasAbility(int battler, int ability, struct AiData* aiData);
int AiIsUnaware(int battler, struct AiData* aiData);
int AreSameSide(int battler1, int battler2);
int IsSleeping(int battler, struct AiData* aiData);
int SeesSunlight(int battler, struct AiData* aiData);
int AdjustForChance(int chance, int score);

#endif