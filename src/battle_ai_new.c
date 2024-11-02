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

int CheckCancelledAlways(int battlerAtk, int battlerDef, int move, struct AiData* aiData)
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

    if (!AreSameSide(battlerAtk, battlerDef)
        && gSideTimers[GetBattlerSide(battlerDef)].quickGuardTimer
        // TODO: Set gProcessingExtraAttacks when scoring extra moves
        && !gProcessingExtraAttacks
        && GetMovePriority(battlerAtk, move, battlerDef) > 0)
    {
        return AI_SCORE_LOSE_TURN(100);
    }
    
    if (gBattleMoves[move].flags & FLAG_POWDER && battlerAtk != battlerDef && !HasAbility(battlerAtk, ABILITY_MYCELIUM_MIGHT, aiData))
    {
        if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS) || GetBattlerHoldEffect(battlerDef, TRUE) == HOLD_EFFECT_SAFETY_GOGGLES)
            return AI_SCORE_LOSE_TURN(100);
    }

    if (!AreSameSide(battlerAtk, battlerDef)
        && !gProcessingExtraAttacks
        && IsBattlerTerrainAffected(battlerDef, STATUS_FIELD_PSYCHIC_TERRAIN)
        && GetMovePriority(battlerAtk, move, battlerDef) > 0)
        return AI_SCORE_LOSE_TURN(100);

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
}

int HasAbility(int battler, int ability, struct AiData* aiData)
{
    return BattlerHasAbility(battler, ability, TRUE);
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

int ShouldEvaluateSpecial(int player, int move, struct AiData* aiData)
{
    return FALSE;
}

int ComputeMoveScoring(struct MoveState* moveState,  struct MoveState* opponentModifiedState, struct BattlerState* resultAttackerData, struct BattlerState* resultTargetData, int* koMultiplier, struct AiData* aiData)
{

}

#define AI_BATTLER 1
#define PLAYER_BATTLER 0
int GetSinglesDecision(struct AiData* aiData)
{
    int i, j, maxScore, bestMove;
    int moveScores[MAX_MON_MOVES] = { 0 };

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (ShouldEvaluateSpecial(AI_BATTLER, i, aiData)) continue;

        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            int score, koScore, koMultiplier;
            int relativeSpeed = aiData->moveState[AI_BATTLER][i].speedValue.comparable - aiData->moveState[PLAYER_BATTLER][j].speedValue.comparable;
            struct MoveState modifiedState;
            struct BattlerState resultingAttackerState, resultingDefenderState;
            if (ShouldEvaluateSpecial(PLAYER_BATTLER, j, aiData)) continue;
            if (relativeSpeed > 0)
            {
                // score = ComputeMoveScoring(&aiData->moveState[AI_BATTLER][i], &modifiedState, &resultingAttackerState, &resultingDefenderState, AI_BATTLER, PLAYER_BATTLER, j, &koMultiplier, aiData);
                if (!koMultiplier)
                    goto AI_GET_SINGLES_DECISION_OPPONENT_MOVE_BREAK;
                // score -= ComputeMoveScoring()
            }
            AI_GET_SINGLES_DECISION_OPPONENT_MOVE_BREAK:
        }
    }
}
#undef AI_BATTLER
#undef PLAYER_BATTLER

/*
Plan:
Calc accuracy for each move on battler and opponent
If acc > 0 calc damage for each move

Calculate move scores
Add KO scores
Fuzz values
Check for KO chance
Best move follows this:
* Skip protect for now
* For each move find the move that the opponent has that matches up the best
** If move has a KO chance reduce values of moves by KO percents
** If opponent has move that goes first and disables move assume they use it

Switch math:
* Don't evaluate double-switching
* Use above process assuming opponent picks optimal move
* Then evaluate actual turn scores ignoring protect
* Optimal switch combines low score this turn and high score on following turns

* Evaluate protect:
** Choose move or switch with the highest score vs protect
** If moves differ pick a winning mixed strategy according to https://www3.nd.edu/~andyp/teaching/2023SpringMath10120/Schedule/Lecture29.pdf
* Idk what to do about encore

Calc move orders for each battler
Calc damage for each move on battler and opponent
If KO -> SCORE_KO
If not record damage

*/
