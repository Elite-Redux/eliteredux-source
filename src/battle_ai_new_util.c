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
    if (BattlerHasAbility(battler, ABILITY_CHLOROPLAST, FALSE)) return TRUE;
    if (BattlerHasAbility(battler, ABILITY_BIG_LEAVES, FALSE)) return TRUE;
    if (BattlerHasAbility(battler, ABILITY_SOLAR_FLARE, FALSE)) return TRUE;
    return IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY);
}

int AdjustForChance(int chance, int score)
{
    if (chance > 100) chance = 100;
    if (chance < 0) chance = 0;
    return chance * score / 100;
}
