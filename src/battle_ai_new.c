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

int CheckCancelledAlways(int battlerAtk, int move, struct AiData* aiData)
{
    if (gBattleMons[battlerAtk].status1 & STATUS1_SLEEP)
    {
        // TODO: Handle not knowing the sleep timer
    }

    if (GetAbilityState(battlerAtk, ABILITY_TRUANT) && !IS_MOVE_STATUS(move))
        return AI_SCORE_LOSE_TURN(100);

    if (gVolatileStructs[battlerAtk].skyDropped)
    {
        // TODO: Handle Sky Drop
    }

    if (gBattleMons[battlerAtk].status2 & STATUS2_RECHARGE)
        return AI_SCORE_LOSE_TURN(100);
    
    if (gVolatileStructs[battlerAtk].throatChopTimer && gBattleMoves[move].flags & FLAG_SOUND)
        return AI_SCORE_LOSE_TURN(100);

    if (gBattleMons[battlerAtk].status1 & STATUS1_PARALYSIS)
        return AI_SCORE_LOSE_TURN(25);
    
    return 0;
}

int BelowHalfHp(int battler)
{
    return gBattleMons[battler].hp <= gBattleMons[battler].maxHP;
}

void PopulateAbilities(int battler, struct AiData* aiData)
{
    if (!aiData->abilities[battler][1])
    {
        GET_ALL_BATTLER_ABILITIES(aiData->abilities[battler], battler, aiData->activeBattler);
    }
}

int HasAbility(int battler, int ability, struct AiData* aiData)
{
    int i;
    PopulateAbilities(battler, aiData);
    for (i = 0; i < TOTAL_ABILITY_COUNT; i++)
    {
        if (aiData->abilities[battler][i] == ability) return TRUE;
    }
    return FALSE;
}

int AiIsUnaware(int battler, struct AiData* aiData)
{
    if (HasAbility(battler, ABILITY_UNAWARE, aiData)) return TRUE;
    if (HasAbility(battler, ABILITY_CONTEMPT, aiData)) return TRUE;
    if (HasAbility(battler, ABILITY_SWORD_OF_DAMNATION, aiData)) return TRUE;
    return FALSE;
}

void ReplaceDisguise(struct DisguiseSimulation* actual)
{

}

void RestoreDisguise(struct DisguiseSimulation* actual)
{

}

int AreSameSide(int battler1, int battler2)
{
    return GetBattlerSide(battler1) == GetBattlerSide(battler2);
}

int IsSleeping(int battler, struct AiData* aiData)
{
    return gBattleMons[battler].status1 & STATUS1_SLEEP || HasAbility(battler, ABILITY_COMATOSE, aiData);
}

int SeesSunlight(int battler, struct AiData* aiData)
{
    if (HasAbility(battler, ABILITY_CHLOROPLAST, aiData)) return TRUE;
    if (HasAbility(battler, ABILITY_BIG_LEAVES, aiData)) return TRUE;
    if (HasAbility(battler, ABILITY_SOLAR_FLARE, aiData)) return TRUE;
    return IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY);
}

int AdjustForChance(int chance, int score)
{
    if (chance > 100) chance = 100;
    if (chance < 0) chance = 0;
    return chance * score / 100;
}

int ScoreMove(int battlerAtk, int battlerDef, int move, int targets, struct AiData* aiData)
{

}

int GetAiDecision(int battler)
{
    struct DisguiseSimulation disguise0 = { 0 }, disguise2 = { 0 };
    int unusableMoves, i, j;
    int moveScores[4][2] = { 0 };
    int switchScore = 0;
    struct AiData aiData = { 0 };
    int flags;
    
    if (IsBattlerAlive(0)) ReplaceDisguise(&disguise0);
    if (IsBattlerAlive(2)) ReplaceDisguise(&disguise2);

    unusableMoves = CheckMoveLimitations(battler, 0, 0xFF);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        flags = GetBattlerBattleMoveTargetFlags(gBattleMons[battler].moves[i], battler);
        for (j = 0; j < 2; j++)
        {
            if (j == 2)
            {
                switch (flags)
                {
                case MOVE_TARGET_SELECTED:
                case MOVE_TARGET_USER_OR_SELECTED:
                    break;
                
                default:
                    moveScores[i][j] = AI_SCORE_UNUSABLE;
                    continue;
                }
            }
            if (unusableMoves & (1 << i))
            {
                moveScores[i][j] = AI_SCORE_UNUSABLE;
                continue;
            }
            moveScores[i][j] = ScoreMove(battler, j * 2, gBattleMons[battler].moves[i], flags, &aiData);
        }
    }
    
    if (IsBattlerAlive(0)) RestoreDisguise(&disguise0);
    if (IsBattlerAlive(2)) RestoreDisguise(&disguise2);
}
