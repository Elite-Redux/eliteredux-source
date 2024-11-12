#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_scripts.h"
#include "battle_main.h"
#include "mgba_printf/mgba.h"
#include "constants/abilities.h"
#include "constants/species.h"
#include "constants/battle_move_effects.h"
#include "constants/items.h"
#include "item.h"
#include "constants/moves.h"
#include "battle_anim.h"
#include "constants/hold_effects.h"
#include "battle_ai_new.h"
#include "battle_ai_scoring.h"
#include "battle_main.h"
#include "battle_ai_new_util.h"

#define TABLE_MATH(value) TABLE_FUNCTION(value)
#define LOOKUP_PERCENTILE {TABLE_MATH(.01), TABLE_MATH(.02), TABLE_MATH(.03), TABLE_MATH(.04), TABLE_MATH(.05), TABLE_MATH(.06), TABLE_MATH(.07), TABLE_MATH(.08), TABLE_MATH(.09), \
TABLE_MATH(.10), TABLE_MATH(.11), TABLE_MATH(.12), TABLE_MATH(.13), TABLE_MATH(.14), TABLE_MATH(.15), TABLE_MATH(.16), TABLE_MATH(.17), TABLE_MATH(.18), TABLE_MATH(.19), \
TABLE_MATH(.20), TABLE_MATH(.21), TABLE_MATH(.22), TABLE_MATH(.23), TABLE_MATH(.24), TABLE_MATH(.25), TABLE_MATH(.26), TABLE_MATH(.27), TABLE_MATH(.28), TABLE_MATH(.29), \
TABLE_MATH(.30), TABLE_MATH(.31), TABLE_MATH(.32), TABLE_MATH(.33), TABLE_MATH(.34), TABLE_MATH(.35), TABLE_MATH(.36), TABLE_MATH(.37), TABLE_MATH(.38), TABLE_MATH(.39), \
TABLE_MATH(.40), TABLE_MATH(.41), TABLE_MATH(.42), TABLE_MATH(.43), TABLE_MATH(.44), TABLE_MATH(.45), TABLE_MATH(.46), TABLE_MATH(.47), TABLE_MATH(.48), TABLE_MATH(.49), \
TABLE_MATH(.50), TABLE_MATH(.51), TABLE_MATH(.52), TABLE_MATH(.53), TABLE_MATH(.54), TABLE_MATH(.55), TABLE_MATH(.56), TABLE_MATH(.57), TABLE_MATH(.58), TABLE_MATH(.59), \
TABLE_MATH(.60), TABLE_MATH(.61), TABLE_MATH(.62), TABLE_MATH(.63), TABLE_MATH(.64), TABLE_MATH(.65), TABLE_MATH(.66), TABLE_MATH(.67), TABLE_MATH(.68), TABLE_MATH(.69), \
TABLE_MATH(.70), TABLE_MATH(.71), TABLE_MATH(.72), TABLE_MATH(.73), TABLE_MATH(.74), TABLE_MATH(.75), TABLE_MATH(.76), TABLE_MATH(.77), TABLE_MATH(.78), TABLE_MATH(.79), \
TABLE_MATH(.80), TABLE_MATH(.81), TABLE_MATH(.82), TABLE_MATH(.83), TABLE_MATH(.84), TABLE_MATH(.85), TABLE_MATH(.86), TABLE_MATH(.87), TABLE_MATH(.88), TABLE_MATH(.89), \
TABLE_MATH(.90), TABLE_MATH(.91), TABLE_MATH(.92), TABLE_MATH(.93), TABLE_MATH(.94), TABLE_MATH(.95), TABLE_MATH(.96), TABLE_MATH(.97), TABLE_MATH(.98), TABLE_MATH(.99)}

#define LOOKUP_TENS {TABLE_MATH(.1), TABLE_MATH(.2), TABLE_MATH(.3), TABLE_MATH(.4), TABLE_MATH(.5), TABLE_MATH(.6), TABLE_MATH(.7), TABLE_MATH(.8), TABLE_MATH(.9)}

#define HIT_2(value) value * value
#define HIT_3(value) HIT_2(value) * value
#define HIT_4(value) HIT_3(value) * value
#define HIT_5(value) HIT_4(value) * value
#define HIT_6(value) HIT_5(value) * value
#define HIT_7(value) HIT_6(value) * value
#define HIT_8(value) HIT_7(value) * value
#define HIT_9(value) HIT_8(value) * value

// gHitOdds[n - 2][p - 1] gives the odds of n rolls at probably p% in a row
const u16 gHitOdds[8][99] = {
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_2(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_3(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_4(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_5(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_6(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_7(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_8(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(HIT_9(value))
    LOOKUP_PERCENTILE,
    #undef TABLE_FUNCTION
};

#define AVERAGE_2(value) 1 + value
#define AVERAGE_3(value) 1 + value * (AVERAGE_2(value))
#define AVERAGE_4(value) 1 + value * (AVERAGE_3(value))
#define AVERAGE_5(value) 1 + value * (AVERAGE_4(value))
#define AVERAGE_6(value) 1 + value * (AVERAGE_5(value))
#define AVERAGE_7(value) 1 + value * (AVERAGE_6(value))
#define AVERAGE_8(value) 1 + value * (AVERAGE_7(value))
#define AVERAGE_9(value) 1 + value * (AVERAGE_8(value))
#define AVERAGE_10(value) 1 + value * (AVERAGE_9(value))

// gHitOdds[n - 2][p - 1] gives the odds of n rolls at probably p% in a row
#define TABLE_FUNCTION(value) UQ_4_12(AVERAGE_10(value))
const u16 gTenHitsMultiplier[99] = LOOKUP_PERCENTILE;
#undef TABLE_FUNCTION

// gHitOdds[n - 2][p - 1] gives the odds of n rolls at probably p% in a row
#define TABLE_FUNCTION(value) UQ_CLAMP_EXPECT(UQ_4_12(AVERAGE_3(value)))
const u8 gTripleKickHitExpected[99] = LOOKUP_PERCENTILE;
#undef TABLE_FUNCTION

#define TABLE_FUNCTION(value) UQ_4_12(1.0 + 2.0 * (value) + 3.0 * (value * value))
// gTripleKickMultiplier[acc - 1] gives the average damage multiplier of triple kick at accuracy acc
const u16 gTripleKickMultiplier[99] = LOOKUP_PERCENTILE;
#undef TABLE_FUNCTION

#define COMPOUND_2(value) 1 - (1.0 - value) * (1.0 - value)
#define COMPOUND_3(value) COMPOUND_2(value) * (1.0 - value)
#define COMPOUND_4(value) COMPOUND_3(value) * (1.0 - value)
#define COMPOUND_5(value) COMPOUND_4(value) * (1.0 - value)
#define COMPOUND_6(value) COMPOUND_5(value) * (1.0 - value)
#define COMPOUND_7(value) COMPOUND_6(value) * (1.0 - value)
#define COMPOUND_8(value) COMPOUND_7(value) * (1.0 - value)
#define COMPOUND_9(value) COMPOUND_8(value) * (1.0 - value)
#define COMPOUND_10(value) COMPOUND_9(value) * (1.0 - value)

// Compiler complains incorrectly that this is variably modified at file scope, if you change the math please uncomment this to make sure your changes are correct
// STATIC_ASSERT(1000 * (COMPOUND_3(.3)) == 657, TestCompoundMath)

// gCompoundOdds[n - 2][p - 1] gives the odds of at least 1 success from n rolls at probability p * 10%
const u16 gCompoundOdds[8][9] = {
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_2(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_3(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_4(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_5(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_6(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_7(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_8(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
    #define TABLE_FUNCTION(value) UQ_4_12(COMPOUND_9(value))
    LOOKUP_TENS,
    #undef TABLE_FUNCTION
};

int AlwaysCancelled(int battlerAtk, int move, struct MoveContainer* moveContainer)
{
    if (gVolatileStructs[battlerAtk].throatChopTimer && gBattleMoves[move].flags & FLAG_SOUND)
        return TRUE;

    if (GetAbilityState(battlerAtk, ABILITY_TRUANT) && !IS_MOVE_STATUS(move))
        return TRUE;

    if (gBattleMons[battlerAtk].status1 & STATUS1_PARALYSIS)
        moveContainer->cancellationState = AI_CANCEL_25;
    else if (gBattleMons[battlerAtk].status1 & STATUS1_SLEEP)
    {
        // TODO: Handle not knowing the sleep timer
    }

    return FALSE;
}

union SpeedValue AiPerformMoveSpeedCalculation(int battlerAtk, int battlerDef, int move)
{
    gChosenMoveByBattler[battlerAtk] = move;
    gBattleStruct->moveTarget[battlerAtk] = battlerDef;
    return GetMoveSpeed(battlerAtk, move);
}

int CheckCancelled(int battlerAtk, int battlerDef, int move, struct MoveState* moveState, struct AiData* aiData)
{

    if (GetAbilityState(battlerAtk, ABILITY_TRUANT) && !IS_MOVE_STATUS(move))
        return TRUE;

    if (gVolatileStructs[battlerAtk].skyDropped)
    {
        // TODO: Calculate sky drop speed and cancel if selected move speed > sky drop speed
    }

    if (!AreSameSide(battlerAtk, battlerDef)
        && !gProcessingExtraAttacks
        && moveState->speedValue.speedStruct.priority > 6)
    {
        if (gSideTimers[GetBattlerSide(battlerDef)].quickGuardTimer) return TRUE;
        if (IsBattlerTerrainAffected(battlerDef, STATUS_FIELD_PSYCHIC_TERRAIN)) return TRUE;
    }
    
    if (gBattleMoves[move].flags & FLAG_POWDER && battlerAtk != battlerDef && !BattlerHasAbility(battlerAtk, ABILITY_MYCELIUM_MIGHT, FALSE))
    {
        if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS)) return TRUE;
        if (GetBattlerHoldEffect(battlerDef, TRUE) == HOLD_EFFECT_SAFETY_GOGGLES) return TRUE;
    }
    
    return FALSE;
}

int BelowHalfHp(int battler)
{
    return gBattleMons[battler].hp <= gBattleMons[battler].maxHP;
}

int AreSameSide(int battler1, int battler2)
{
    return GetBattlerSide(battler1) == GetBattlerSide(battler2);
}

int IsSleeping(int battler, struct AiData* aiData)
{
    return gBattleMons[battler].status1 & STATUS1_SLEEP || BattlerHasAbility(battler, ABILITY_COMATOSE, FALSE);
}

int SeesSunlight(int battler, struct AiData* aiData)
{
    return IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY);
}

int AdjustForChance(int chance, int score)
{
    if (chance > 100) chance = 100;
    if (chance < 0) chance = 0;
    return chance * score / 100;
}
