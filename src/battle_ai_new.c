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
#include "battle_ai_attack.h"

int ScoreMove(int battlerAtk, int battlerDef, int move, int targets, struct AiData* aiData)
{

}

void ReplaceDisguise(struct DisguiseSimulation* actual)
{

}

void RestoreDisguise(struct DisguiseSimulation* actual)
{

}

int CalculateAccuracy(int battlerAtk, int battlerDef, int move, AiProcessingPhase phase, struct MoveState* moveState, struct MoveContainer* moveContainer, struct AiData* aiData)
{
    // TODO: Anticipation
    int accuracy = GetTotalAccuracy(battlerAtk, battlerDef, move, moveState);
    if (!accuracy)
        moveState->cancelled = TRUE;
    else
        moveState->accuracy = accuracy;
    return 0;
}

int CalculateBasicMoveInfo(int battlerAtk, int battlerDef, int move, AiProcessingPhase phase, struct MoveState* moveState, struct MoveContainer* moveContainer, struct AiData* aiData)
{
    moveState->speedValue = AiPerformMoveSpeedCalculation(battlerAtk, battlerDef, move);
    if (CheckCancelled(battlerAtk, battlerDef, move, moveState, aiData))
    {
        moveState->cancelled = TRUE;
    }
    
    return 0;
}

static int (* const sPhaseHandlersTable[AI_PHASE_COUNT])(int battlerAtk, int battlerDef, int move, AiProcessingPhase phase, struct MoveState* moveState, struct MoveContainer* moveContainer, struct AiData* aiData) =
{
    [AI_PHASE_BASIC] = CalculateBasicMoveInfo,
    [AI_PHASE_ACCURACY] = CalculateAccuracy,
    [AI_PHASE_DAMAGE] = ScoreMoveDamage,
};

void SetDamage()
{

}

void IterateForPhase(AiProcessingPhase phase, struct AiData* aiData)
{
    int battlerAtk, targetNum, moveNum;
    for (battlerAtk = 0; battlerAtk < gBattlersCount; battlerAtk++)
    {
        for (moveNum = 0; moveNum < MAX_MON_MOVES; moveNum++)
        {
            struct MoveContainer* container = &aiData->moveState[battlerAtk][moveNum];
            FILTER_NOT(container->unusable)
            gHitMarker &= ~(HITMARKER_MOLD_BREAKER | HITMARKER_MYCELIUM_MIGHT);
            if (ShouldSetMoldBreaker(battlerAtk, container->move)) gHitMarker |= HITMARKER_MOLD_BREAKER;
            if (BattlerHasAbility(battlerAtk, ABILITY_MYCELIUM_MIGHT, FALSE)) gHitMarker |= HITMARKER_MYCELIUM_MIGHT;
            for (targetNum = 0; targetNum < container->count; targetNum++)
            {
                struct MoveState* state = &container->targetData[targetNum];
                FILTER_NOT(state->cancelled)
                state->score += sPhaseHandlersTable[phase](battlerAtk, state->target, container->move, phase, state, container, aiData);
            }
        }
    }
}

struct MoveState* SetMoveVs(int battlerAtk, int battlerDef, int moveNum, struct AiData* aiData)
{
    int targetCount;
    struct MoveState* moveData;
    if (!IsBattlerAlive(battlerDef)) return NULL;
    targetCount = aiData->moveState[battlerAtk][moveNum].count++;
    moveData = &aiData->moveState[battlerAtk][moveNum].targetData[targetCount];
    moveData->target = battlerDef;
    return moveData;
}

void ConfigureMoves(int battlerAtk, int unusableMoves, struct AiData* aiData)
{
    int moveNum;
    for (moveNum = 0; moveNum < MAX_MON_MOVES; moveNum++)
    {
        int move;
        struct MoveContainer* moveContainer = &aiData->moveState[battlerAtk][moveNum];
        if (move == MOVE_NONE || unusableMoves & (1 << moveNum))
        {
            moveContainer->unusable = TRUE;
            continue;
        }
        move = moveContainer->move = gBattleMons[battlerAtk].moves[moveNum];
        FILTER(!AlwaysCancelled(battlerAtk, move, moveContainer))
        moveContainer->targetFlags = GetBattlerBattleMoveTargetFlags(move, battlerAtk);
        switch (moveContainer->targetFlags)
        {
        case MOVE_TARGET_ALLY:
            SetMoveVs(battlerAtk, BATTLE_PARTNER(battlerAtk), moveNum, aiData);
            break;
        
        case MOVE_TARGET_BOTH:
        case MOVE_TARGET_RANDOM:
            SetMoveVs(battlerAtk, BATTLE_OPPOSITE(battlerAtk), moveNum, aiData);
            SetMoveVs(battlerAtk, BATTLE_PARTNER(BATTLE_OPPOSITE(battlerAtk)), moveNum, aiData);
            break;
            
        case MOVE_TARGET_OPPONENTS_FIELD:
            SetMoveVs(battlerAtk, GetBattlerSide(BATTLE_OPPOSITE(battlerAtk)), moveNum, aiData)
                || SetMoveVs(battlerAtk, GetBattlerSide(BATTLE_PARTNER(BATTLE_OPPOSITE(battlerAtk))), moveNum, aiData);
            break;
        
        case MOVE_TARGET_USER_OR_SELECTED:
        case MOVE_TARGET_DEPENDS:
            // These aren't real
            break;
        
        case MOVE_TARGET_USER:
            SetMoveVs(battlerAtk, battlerAtk, moveNum, aiData);
            break;
        
        case MOVE_TARGET_SELECTED:
        case MOVE_TARGET_FOES_AND_ALLY:
            SetMoveVs(battlerAtk, BATTLE_OPPOSITE(battlerAtk), moveNum, aiData);
            SetMoveVs(battlerAtk, BATTLE_PARTNER(BATTLE_OPPOSITE(battlerAtk)), moveNum, aiData);
            SetMoveVs(battlerAtk, BATTLE_PARTNER(battlerAtk), moveNum, aiData);
            break;
        }

        FILTER(moveContainer->count)

        moveContainer->startingMoveType = GetTypeBeforeUsingMove(move, battlerAtk);

        FILTER_NOT(IS_MOVE_STATUS(move))

        moveContainer->multihitType = GetMultihitType(battlerAtk, move);
        if (!moveContainer->multihitType) moveContainer->multihitType = GetParentalBondType(battlerAtk, move);
    }
}

int GetAiDecision(int battler)
{
    struct DisguiseSimulation disguise0 = { 0 }, disguise2 = { 0 };
    int battlerAtk;
    struct AiData aiData = { 0 };
    
    if (IsBattlerAlive(0)) ReplaceDisguise(&disguise0);
    if (IsBattlerAlive(2)) ReplaceDisguise(&disguise2);

    for (battlerAtk = 0; battlerAtk < gBattlersCount; battlerAtk++)
    {
        int unusableMoves;
        FILTER(IsBattlerAlive(battlerAtk))
        unusableMoves = CheckMoveLimitations(battler, 0, -1);
        ConfigureMoves(battlerAtk, unusableMoves, &aiData);
    }
    
    if (IsBattlerAlive(0)) RestoreDisguise(&disguise0);
    if (IsBattlerAlive(2)) RestoreDisguise(&disguise2);
}

int ShouldEvaluateSpecial(int player, int move, struct AiData* aiData)
{
    return FALSE;
}

// Effects which are applied regardless of defender's survival. Calculates KO percent.
int ComputeAttackPrimaryScoring(int battlerAtk, int battlerDef, int move, struct AiData* aiData)
{

}

int AdjustStateForMove(int battlerAtk, int battlerUpdated, int move, struct AiData* aiData)
{
    
}

int AdjustHpForMove(int battlerAtk, int battlerUpdated, int move, struct AiData* aiData)
{
    
}

// int SpeedDifference(int battler1, int move1, int battler2, int move2, struct AiData* aiData)
// {
//     return aiData->moveState[battler1][move1].speedValue.comparable - aiData->moveState[battler2][move2].speedValue.comparable;
// }

// int CalcEndTurnScore(int battler, struct AiData* aiData)
// {

// }

// typedef enum {
//     AI_UPDATE_BOTH = 0,
//     AI_UPDATE_B1_FAINTS_ON_ATTACK = 1 << 0,
//     AI_UPDATE_B2_FAINTS_ON_ATTACK = 1 << 1,
//     AI_UPDATE_BOTH_FAINT_ON_ATTACK = AI_UPDATE_B1_FAINTS_ON_ATTACK | AI_UPDATE_B2_FAINTS_ON_ATTACK,
//     AI_UPDATE_B1_FAINTS_AFTER = 1 << 2,
//     AI_UPDATE_B2_FAINTS_AFTER = 1 << 3,
//     AI_UPDATE_BOTH_FAINT_AFTER = AI_UPDATE_B1_FAINTS_AFTER | AI_UPDATE_B2_FAINTS_AFTER,
// } AiScoreEvaluationResult;

// int CalcMaxDamageScore(int battlerAtk, int battlerDef, struct AiData* aiData)
// {
//     int i, maxScore = 0;
//     for (i = 0; i < MAX_MON_MOVES; i++)
//     {
//         int score = ComputeAttackPrimaryScoring(battlerAtk, battlerDef, i, aiData);
//         if (score > maxScore) maxScore = score;
//     }
//     return maxScore;
// }

// int GetMaxSpeedOfFaintingMove(int battlerAtk, struct AiData* aiData)
// {
//     int i, maxSpeed = 0;
//     for (i = 0; i < MAX_MON_MOVES; i++)
//     {
//         if (aiData->moveState[battlerAtk][i].koChance)
//         {
//             int speed = aiData->moveState[battlerAtk][i].speedValue.comparable;
//             if (speed > maxSpeed) maxSpeed = speed;
//         }
//     }
// }

// int CalcRelativeMoveScore(int battlerFirst, int moveFirst, int battlerSecond, int moveSecond, int doUpdates, AiScoreEvaluationResult* updateState, struct AiData* aiData, struct AiData* aiDataKo)
// {
//     int scoreBefore;
//     int score = ComputeAttackPrimaryScoring(battlerFirst, battlerSecond, moveFirst, aiData);
//     if (doUpdates) AdjustStateForMove(battlerFirst, battlerFirst, moveFirst, aiData);
//     else AdjustHpForMove(battlerFirst, battlerFirst, moveFirst, aiData);

//     *updateState = AI_UPDATE_BOTH;

//     if (aiData->moveState[battlerFirst][moveFirst].koChance >= UQ_4_12(1.0))
//     {
//         *updateState |= AI_UPDATE_B2_FAINTS_ON_ATTACK;
//         score += AI_SCORE_KO;
//     }
//     else if (aiData->moveState[battlerFirst][moveFirst].koChance)
//     {
//         STRUCT_COPY(aiData, aiDataKo)
//     }
//     if (!aiData->battlerState[battlerFirst].hp)
//     {
//         *updateState |= AI_UPDATE_B1_FAINTS_ON_ATTACK;
//         score -= AI_SCORE_KO;
//     }
//     if (*updateState) return score;
//     scoreBefore = score;
    
//     if (doUpdates) AdjustStateForMove(battlerFirst, battlerSecond, moveFirst, aiData);
//     else AdjustHpForMove(battlerFirst, battlerFirst, moveFirst, aiData);
//     score -= ComputeAttackPrimaryScoring(battlerSecond, battlerFirst, moveSecond, aiData);
//     if (doUpdates) AdjustStateForMove(battlerSecond, battlerSecond, moveSecond, aiData);
//     else AdjustHpForMove(battlerFirst, battlerFirst, moveFirst, aiData);

//     if (aiData->moveState[battlerSecond][moveSecond].koChance >= UQ_4_12(1.0))
//     {
//         *updateState |= AI_UPDATE_B1_FAINTS_AFTER;
//         score -= AI_SCORE_KO;
//     }
//     else
//     {
//         if (doUpdates) AdjustStateForMove(battlerSecond, battlerFirst, moveSecond, aiData);
//         else AdjustHpForMove(battlerFirst, battlerFirst, moveFirst, aiData);
//         if (aiData->moveState[battlerSecond][moveSecond].koChance)
//             score -= ApplyModifier(aiData->moveState[battlerSecond][moveSecond].koChance, AI_SCORE_KO);
//     }

//     if (!aiData->battlerState[battlerSecond].hp)
//     {
//         *updateState |= AI_UPDATE_B2_FAINTS_AFTER;
//         score += AI_SCORE_KO;
//     }
    
//     if (aiData->moveState[battlerFirst][moveFirst].koChance)
//         return ApplyModifier(aiData->moveState[battlerFirst][moveFirst].koChance, scoreBefore) + ApplyModifier(UQ_4_12(1.0) - aiData->moveState[battlerFirst][moveFirst].koChance, score);
//     else
//         return score;
// }

// int CalcTurnSelectionScore(int battlerFirst, int moveFirst, int battlerSecond, int moveSecond, AiScoreEvaluationResult* noEval, struct AiData* aiData)
// {
//     struct AiData mutableAiData, mutableAiDataMaybeKo;
//     AiScoreEvaluationResult updateState;
//     int score, endTurnScore, i, maybeKoModifier;

//     STRUCT_COPY(mutableAiData, *aiData)
    
//     score = CalcRelativeMoveScore(battlerFirst, moveFirst, battlerSecond, moveSecond, TRUE, &updateState, &mutableAiData, &mutableAiDataMaybeKo);

//     switch (updateState)
//     {
//     case AI_UPDATE_B1_FAINTS_AFTER:
//     case AI_UPDATE_B1_FAINTS_ON_ATTACK:
//         endTurnScore = -CalcEndTurnScore(battlerSecond, &mutableAiData);
//         break;

//     case AI_UPDATE_B2_FAINTS_AFTER:
//     case AI_UPDATE_B2_FAINTS_ON_ATTACK:
//         endTurnScore = CalcEndTurnScore(battlerFirst, &mutableAiData);
//         break;
    
//     case AI_UPDATE_BOTH:
//         endTurnScore = CalcEndTurnScore(battlerFirst, &mutableAiData) - CalcEndTurnScore(battlerSecond, &mutableAiData);
//         break;
//     }

//     if (updateState & AI_UPDATE_BOTH_FAINT_ON_ATTACK)
//     {
//         *noEval = updateState;
//         score += endTurnScore;
//         if (updateState == AI_UPDATE_B1_FAINTS_ON_ATTACK)
//             return score - AI_SCORE_TURN_TWO_DAMAGE(CalcMaxDamageScore(battlerSecond, battlerFirst, aiData));
//         else if (updateState == AI_UPDATE_B2_FAINTS_AFTER)
//             return score + AI_SCORE_TURN_TWO_DAMAGE(CalcMaxDamageScore(battlerFirst, battlerSecond, aiData));
//         else return score;
//     }

//     maybeKoModifier = aiData->moveState[battlerFirst][moveFirst].koChance;

//     if (!aiData->battlerState[battlerFirst].hp)
//         endTurnScore -= AI_SCORE_TURN_TWO_DAMAGE(CalcMaxDamageScore(battlerSecond, battlerFirst, aiData));
//     else if (!aiData->battlerState[battlerSecond].hp)
//         endTurnScore += AI_SCORE_TURN_TWO_DAMAGE(CalcMaxDamageScore(battlerFirst, battlerSecond, aiData));
//     else
//     {
//         int damageScoreFirst = CalcMaxDamageScore(battlerFirst, battlerSecond, aiData);
//         int damageScoreSecond = CalcMaxDamageScore(battlerSecond, battlerFirst, aiData);
//         int speed = GetMaxSpeedOfFaintingMove(battlerFirst, aiData) - GetMaxSpeedOfFaintingMove(battlerSecond, aiData);
//         if (speed >= 0) endTurnScore += AI_SCORE_TURN_TWO_DAMAGE(damageScoreFirst);
//         if (speed <= 0) endTurnScore -= AI_SCORE_TURN_TWO_DAMAGE(damageScoreSecond);
//     }

//     if (maybeKoModifier)
//     {
//         int maybeKoScore = 0;
//         if (mutableAiDataMaybeKo.battlerState[battlerFirst].hp)
//         {
//             maybeKoScore += CalcEndTurnScore(battlerFirst, &mutableAiDataMaybeKo);
//         }
//         if (mutableAiDataMaybeKo.battlerState[battlerFirst].hp)
//         {
//             maybeKoScore += CalcMaxDamageScore(battlerFirst, battlerSecond, &mutableAiDataMaybeKo);
//         }
//         return score + ApplyModifier(maybeKoModifier, maybeKoScore) + ApplyModifier(UQ_4_12(1) - maybeKoModifier, endTurnScore);
//     }

//     return score + endTurnScore;
// }

// #define AI_BATTLER 1
// #define PLAYER_BATTLER 0
// int GetSinglesDecision(struct AiData* aiData)
// {
//     int i, j, maxScore, bestMove, evaluated = MAX_MON_MOVES;
//     int moveScores[MAX_MON_MOVES] = { 0 };
//     struct AiData mutableAiData, mutableAiDataKoUncertain;

//     for (i = 0; i < MAX_MON_MOVES; i++)
//     {
//         int playerMoved, minScore;
//         if (ShouldEvaluateSpecial(AI_BATTLER, i, aiData))
//         {
//             evaluated--;
//             continue;
//         }
        
//         playerMoved = MAX_MON_MOVES;
//         minScore = 2 * AI_SCORE_KO;

//         for (j = 0; j < MAX_MON_MOVES; j++)
//         {
//             int score;
//             int relativeSpeed = SpeedDifference(AI_BATTLER, i, PLAYER_BATTLER, j, aiData);
//             AiScoreEvaluationResult noEval;

//             if (ShouldEvaluateSpecial(PLAYER_BATTLER, j, aiData))
//             {
//                 playerMoved--;
//                 continue;
//             }

//             STRUCT_COPY(mutableAiData, *aiData)

//             if (relativeSpeed > 0)
//             {
//                 score = CalcTurnSelectionScore(AI_BATTLER, i, PLAYER_BATTLER, j, &noEval, aiData);
//                 if (noEval & AI_UPDATE_B2_FAINTS_ON_ATTACK) playerMoved--;
//             }
//             // For simplicity assume we lose speed ties, helps mitigate save scumming, if we are always dead we will pick a different option later
//             else
//             {
//                 score = -CalcTurnSelectionScore(PLAYER_BATTLER, j, AI_BATTLER, i, &noEval, aiData);
//                 if (noEval & AI_UPDATE_B2_FAINTS_ON_ATTACK) playerMoved--;
//             }

//             if (score < minScore) minScore = score;
//         }

//         moveScores[i] = AI_SCORE_FUZZ(minScore);
//         if (!playerMoved) evaluated--;
//     }

//     if (!evaluated)
//     {
//         int faintScore = AI_SCORE_KO + AI_SCORE_DAMAGE(100 * aiData->battlerState[AI_BATTLER].hp / gBattleMons[AI_BATTLER].maxHP);
//         for (i = 0; i < MAX_MON_MOVES; i++)
//         {
//             if (ShouldEvaluateSpecial(AI_BATTLER, i, aiData))
//                 continue;
            
//             moveScores[i] = AI_SCORE_FUZZ(ComputeAttackPrimaryScoring(AI_BATTLER, PLAYER_BATTLER, i, aiData)) - faintScore;            
//         }
//     }
// }
// #undef AI_BATTLER
// #undef PLAYER_BATTLER

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
