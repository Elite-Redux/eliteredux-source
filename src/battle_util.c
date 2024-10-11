#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_arena.h"
#include "battle_pyramid.h"
#include "battle_util.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "battle_setup.h"
#include "data.h"
#include "party_menu.h"
#include "pokemon.h"
#include "international_string_util.h"
#include "item.h"
#include "rtc.h"
#include "util.h"
#include "battle_scripts.h"
#include "random.h"
#include "text.h"
#include "safari_zone.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "trig.h"
#include "window.h"
#include "battle_message.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "event_data.h"
#include "link.h"
#include "malloc.h"
#include "berry.h"
#include "pokedex.h"
#include "mail.h"
#include "field_weather.h"
#include "reshow_battle_screen.h"
#include "ui_battle_menu.h"
#include "constants/abilities.h"
#include "constants/battle_anim.h"
#include "constants/battle_config.h"
#include "constants/battle_frontier.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_string_ids.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/weather.h"
#include "constants/rgb.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"

/*
NOTE: The data and functions in this file up until (but not including) sSoundMovesTable
are actually part of battle_main.c. They needed to be moved to this file in order to
match the ROM; this is also why sSoundMovesTable's declaration is in the middle of
functions instead of at the top of the file with the other declarations.
*/

static bool32 TryRemoveScreens(u8 battler);
static bool32 IsUnnerveAbilityOnOpposingSide(u8 battlerId);
static bool8 DoesMoveBoostStats(u16 move);
static void UpdateMoveResultFlags(u16 modifier);

extern const u8 *const gBattleScriptsForMoveEffects[];
extern const u8 *const gBattlescriptsForBallThrow[];
extern const u8 *const gBattlescriptsForRunningByItem[];
extern const u8 *const gBattlescriptsForUsingItem[];
extern const u8 *const gBattlescriptsForSafariActions[];

static const u8 sPkblToEscapeFactor[][3] = {
    {
        [B_MSG_MON_CURIOUS]    = 0,
        [B_MSG_MON_ENTHRALLED] = 0,
        [B_MSG_MON_IGNORED]    = 0
    },{
        [B_MSG_MON_CURIOUS]    = 3,
        [B_MSG_MON_ENTHRALLED] = 5,
        [B_MSG_MON_IGNORED]    = 0
    },{
        [B_MSG_MON_CURIOUS]    = 2,
        [B_MSG_MON_ENTHRALLED] = 3,
        [B_MSG_MON_IGNORED]    = 0
    },{
        [B_MSG_MON_CURIOUS]    = 1,
        [B_MSG_MON_ENTHRALLED] = 2,
        [B_MSG_MON_IGNORED]    = 0
    },{
        [B_MSG_MON_CURIOUS]    = 1,
        [B_MSG_MON_ENTHRALLED] = 1,
        [B_MSG_MON_IGNORED]    = 0
    }
};
static const u8 sGoNearCounterToCatchFactor[] = {4, 3, 2, 1};
static const u8 sGoNearCounterToEscapeFactor[] = {4, 4, 4, 4};

static const u16 sSkillSwapBannedAbilities[] =
{
    ABILITY_WONDER_GUARD,
};

static const u16 sRolePlayBannedAbilities[] =
{
    ABILITY_TRACE,
    ABILITY_WONDER_GUARD,
    ABILITY_RECEIVER,
};

static const u16 sEntrainmentTargetSimpleBeamBannedAbilities[] =
{
    ABILITY_TRUANT,
};

static const u16 sTwoStrikeMoves[] =
{
    MOVE_DOUBLE_IRON_BASH,
    MOVE_TWINEEDLE,
    MOVE_CROSS_POISON,
};

u8 CalcBeatUpPower(void)
{
    struct Pokemon *party;
    u8 basePower;
    u16 species;

    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    // Party slot is set in the battle script for Beat Up
    species = GetMonData(&party[gBattleCommunication[0] - 1], MON_DATA_SPECIES);
    basePower = (gBaseStats[species].baseAttack / 10) + 5;

    return basePower;
}

bool32 IsAffectedByFollowMe(u32 battlerAtk, u32 defSide, u32 move)
{
    u32 ability = GetBattlerAbility(battlerAtk);

    if (gSideTimers[defSide].followmeTimer == 0
        || gBattleMons[gSideTimers[defSide].followmeTarget].hp == 0
        || gBattleMoves[move].effect == EFFECT_SNIPE_SHOT
        || BATTLER_HAS_ABILITY_FAST(battlerAtk, ABILITY_PROPELLER_TAIL, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerAtk, ABILITY_STALWART, ability))
        return FALSE;

    if (gSideTimers[defSide].followmePowder && !IsAffectedByPowder(battlerAtk, ability, GetBattlerHoldEffect(battlerAtk, TRUE)))
        return FALSE;

    return TRUE;
}

// Returns the target field of a move, modified by ability
u8 GetBattleMoveTargetFlags(u16 moveId, u16 ability)
{
    if ((BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_ARTILLERY, ability)) 
         && (gBattleMoves[moveId].flags & FLAG_MEGA_LAUNCHER_BOOST)
         && gBattleMoves[moveId].target == MOVE_TARGET_SELECTED)
        return MOVE_TARGET_BOTH;
    else if ((BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_SWEEPING_EDGE, ability) || BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_SWEEPING_EDGE_PLUS, ability)) 
         && (gBattleMoves[moveId].flags & FLAG_KEEN_EDGE_BOOST)
         && gBattleMoves[moveId].target == MOVE_TARGET_SELECTED)
        return MOVE_TARGET_BOTH;
    else if ((BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_AMPLIFIER, ability) || BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_BASS_BOOSTED, ability))
         && (gBattleMoves[moveId].flags & FLAG_SOUND)
         && gBattleMoves[moveId].target == MOVE_TARGET_SELECTED)
        return MOVE_TARGET_BOTH;
    else if (gBattleMoves[moveId].effect == EFFECT_EXPANDING_FORCE && GetCurrentTerrain() == STATUS_FIELD_PSYCHIC_TERRAIN)
        return MOVE_TARGET_BOTH;
    return gBattleMoves[moveId].target;
}

u8 GetBattlerBattleMoveTargetFlags(u16 moveId, u8 battler)
{
    u16 ability = GetBattlerAbility(battler);
    if ((BATTLER_HAS_ABILITY_FAST(battler, ABILITY_ARTILLERY, ability)) 
         && (gBattleMoves[moveId].flags & FLAG_MEGA_LAUNCHER_BOOST)
         && gBattleMoves[moveId].target == MOVE_TARGET_SELECTED)
        return MOVE_TARGET_BOTH;
    else if ((BATTLER_HAS_ABILITY_FAST(battler, ABILITY_SWEEPING_EDGE, ability) || BATTLER_HAS_ABILITY_FAST(battler, ABILITY_SWEEPING_EDGE_PLUS, ability)) 
         && (gBattleMoves[moveId].flags & FLAG_KEEN_EDGE_BOOST)
         && gBattleMoves[moveId].target == MOVE_TARGET_SELECTED)
        return MOVE_TARGET_BOTH;
    else if ((BATTLER_HAS_ABILITY_FAST(battler, ABILITY_AMPLIFIER, ability) || BATTLER_HAS_ABILITY_FAST(battler, ABILITY_BASS_BOOSTED, ability))
         && (gBattleMoves[moveId].flags & FLAG_SOUND)
         && gBattleMoves[moveId].target == MOVE_TARGET_SELECTED)
        return MOVE_TARGET_BOTH;
    else if (gBattleMoves[moveId].effect == EFFECT_EXPANDING_FORCE && GetCurrentTerrain() == STATUS_FIELD_PSYCHIC_TERRAIN)
        return MOVE_TARGET_BOTH;

        
    switch (moveId)
    {
        case MOVE_RAIN_DANCE:
        case MOVE_SUNNY_DAY:
        case MOVE_SANDSTORM:
        case MOVE_HAIL:
        case MOVE_EERIE_FOG:
            if (!BATTLER_HAS_ABILITY(battler, ABILITY_FORECAST))
            {
                return MOVE_TARGET_USER;
            }
            break;
    }

    return gBattleMoves[moveId].target;
}

// Functions
void HandleAction_UseMove(void)
{
    u32 i, side, moveType, var = 4;

    gBattlerAttacker = GetTurnBattler();
    if (gBattleStruct->field_91 & 1 << gBattlerAttacker || !IsBattlerAlive(gBattlerAttacker))
    {
        gCurrentActionFuncId = B_ACTION_FINISHED;
        return;
    }

    if (!gProcessingExtraAttacks)
    {
        SetAbilityState(gBattlerAttacker, ABILITY_RAMPAGE, FALSE);
        SetAbilityState(gBattlerAttacker, ABILITY_MASTER_HAND, FALSE);
        SetAbilityState(gBattlerAttacker, ABILITY_BERSERKER_RAGE, FALSE);
        SetAbilityState(gBattlerAttacker, ABILITY_RAGING_GODDESS, FALSE);
    }

    gIsCriticalHit = FALSE;
    gBattleStruct->atkCancellerTracker = 0;
    gMoveResultFlags = 0;
    gBattleCommunication[6] = 0;
    gBattleScripting.savedMoveEffect = 0;
    gCurrMovePos = gChosenMovePos = gBattleStruct->chosenMovePositions[gBattlerAttacker];

    // choose move
    if (gProcessingExtraAttacks)
    {
        gCurrentMove = gChosenMove = gQueuedExtraAttackData[0].move;
        gCurrMovePos = gQueuedExtraAttackData[0].movePos;
        if (gCurrMovePos == MAX_MON_MOVES) gHitMarker |= HITMARKER_NO_PPDEDUCT;
        gBattlerTarget = gQueuedExtraAttackData[0].target;
        gBattleScripting.usingExtraMove = TRUE;
    }
    else if (gRoundStructs[gBattlerAttacker].noValidMoves)
    {
        gRoundStructs[gBattlerAttacker].noValidMoves = FALSE;
        gCurrentMove = gChosenMove = MOVE_STRUGGLE;
        gHitMarker |= HITMARKER_NO_PPDEDUCT;
        gBattlerTarget = gBattleStruct->moveTarget[gBattlerAttacker] = GetMoveTarget(MOVE_STRUGGLE, 0);
    }
    else if (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS || gBattleMons[gBattlerAttacker].status2 & STATUS2_RECHARGE)
    {
        gCurrentMove = gChosenMove = gLockedMoves[gBattlerAttacker];
        gBattlerTarget = gBattleStruct->moveTarget[gBattlerAttacker];
    }
    // encore forces you to use the same move
    else if (gVolatileStructs[gBattlerAttacker].encoredMove != MOVE_NONE
          && gVolatileStructs[gBattlerAttacker].encoredMove == gBattleMons[gBattlerAttacker].moves[gVolatileStructs[gBattlerAttacker].encoredMovePos])
    {
        gCurrentMove = gChosenMove = gVolatileStructs[gBattlerAttacker].encoredMove;
        gCurrMovePos = gChosenMovePos = gVolatileStructs[gBattlerAttacker].encoredMovePos;
        gBattlerTarget = gBattleStruct->moveTarget[gBattlerAttacker] = GetMoveTarget(gCurrentMove, 0);
    }
    // check if the encored move wasn't overwritten
    else if (gVolatileStructs[gBattlerAttacker].encoredMove != MOVE_NONE
          && gVolatileStructs[gBattlerAttacker].encoredMove != gBattleMons[gBattlerAttacker].moves[gVolatileStructs[gBattlerAttacker].encoredMovePos])
    {
        gCurrMovePos = gChosenMovePos = gVolatileStructs[gBattlerAttacker].encoredMovePos;
        gCurrentMove = gChosenMove = gBattleMons[gBattlerAttacker].moves[gCurrMovePos];
        gVolatileStructs[gBattlerAttacker].encoredMove = MOVE_NONE;
        gVolatileStructs[gBattlerAttacker].encoredMovePos = 0;
        gVolatileStructs[gBattlerAttacker].encoreTimer = 0;
        gBattlerTarget = gBattleStruct->moveTarget[gBattlerAttacker] = GetMoveTarget(gCurrentMove, 0);
    }
    else if (gBattleMons[gBattlerAttacker].moves[gCurrMovePos] != gChosenMoveByBattler[gBattlerAttacker])
    {
        gCurrentMove = gChosenMove = gBattleMons[gBattlerAttacker].moves[gCurrMovePos];
        gBattlerTarget = gBattleStruct->moveTarget[gBattlerAttacker] = GetMoveTarget(gCurrentMove, 0);
    }
    else
    {
        gCurrentMove = gChosenMove = gBattleMons[gBattlerAttacker].moves[gCurrMovePos];
        gBattlerTarget = gBattleStruct->moveTarget[gBattlerAttacker];
    }

    if (gBattleMons[gBattlerAttacker].hp != 0)
    {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            gBattleResults.lastUsedMovePlayer = gCurrentMove;
        else
            gBattleResults.lastUsedMoveOpponent = gCurrentMove;
    }

    // Set dynamic move type.
    SetTypeBeforeUsingMove(gChosenMove, gBattlerAttacker);
    GET_MOVE_TYPE(gChosenMove, moveType);

    // choose target
    side = GetBattlerSide(gBattlerAttacker) ^ BIT_SIDE;
    if (IsAffectedByFollowMe(gBattlerAttacker, side, gCurrentMove)
        && GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) == MOVE_TARGET_SELECTED
        && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gSideTimers[side].followmeTarget))
    {
        gBattlerTarget = gSideTimers[side].followmeTarget;
    }
    else if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
           && gSideTimers[side].followmeTimer == 0
           && (gBattleMoves[gCurrentMove].power != 0 || GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) != MOVE_TARGET_USER)
           && ((GetBattlerAbility(gBattlerTarget) != ABILITY_LIGHTNING_ROD && !BattlerHasInnate(gBattlerTarget, ABILITY_LIGHTNING_ROD) && moveType == TYPE_ELECTRIC)
            || (GetBattlerAbility(gBattlerTarget) != ABILITY_STORM_DRAIN   && !BattlerHasInnate(gBattlerTarget, ABILITY_STORM_DRAIN) && moveType == TYPE_WATER)))
    {
        side = GetBattlerSide(gBattlerAttacker);
        for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
        {
            if (side != GetBattlerSide(gActiveBattler)
                && gBattlerTarget != gActiveBattler
                && (((GetBattlerAbility(gActiveBattler) == ABILITY_LIGHTNING_ROD || BattlerHasInnate(gActiveBattler, ABILITY_LIGHTNING_ROD)) && moveType == TYPE_ELECTRIC)
                 || ((GetBattlerAbility(gActiveBattler) == ABILITY_STORM_DRAIN   || BattlerHasInnate(gActiveBattler, ABILITY_STORM_DRAIN))   && moveType == TYPE_WATER))
                && GetBattlerTurnOrderNum(gActiveBattler) < var
                && gBattleMoves[gCurrentMove].effect != EFFECT_SNIPE_SHOT
                && !(BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_PROPELLER_TAIL)
                    || BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_STALWART)))
            {
                var = GetBattlerTurnOrderNum(gActiveBattler);
            }
        }
        if (var == 4)
        {
            if (GetBattlerBattleMoveTargetFlags(gChosenMove, gBattlerAttacker) & MOVE_TARGET_RANDOM)
            {
                if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
                {
                    if (Random() & 1)
                        gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
                    else
                        gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
                }
                else
                {
                    if (Random() & 1)
                        gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
                    else
                        gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
                }
            }
            else if (GetBattlerBattleMoveTargetFlags(gChosenMove, gBattlerAttacker) & MOVE_TARGET_FOES_AND_ALLY)
            {
                for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
                {
                    if (gBattlerTarget == gBattlerAttacker)
                        continue;
                    if (IsBattlerAlive(gBattlerTarget))
                        break;
                }
            }
        }
        else
        {
            gActiveBattler = gBattlerByTurnOrder[var];
            RecordAbilityBattle(gActiveBattler, gBattleMons[gActiveBattler].ability);
            if (GetBattlerAbility(gActiveBattler) == ABILITY_LIGHTNING_ROD    || BattlerHasInnate(gActiveBattler, ABILITY_LIGHTNING_ROD)) 
                gTurnStructs[gActiveBattler].lightningRodRedirected = TRUE;
            else if (GetBattlerAbility(gActiveBattler) == ABILITY_STORM_DRAIN || BattlerHasInnate(gActiveBattler, ABILITY_STORM_DRAIN))
                gTurnStructs[gActiveBattler].stormDrainRedirected = TRUE;
            gBattlerTarget = gActiveBattler;
        }
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
          && GetBattlerBattleMoveTargetFlags(gChosenMove, gBattlerAttacker) & MOVE_TARGET_RANDOM)
    {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        {
            if (Random() & 1)
                gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
            else
                gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        }
        else
        {
            if (Random() & 1)
                gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
            else
                gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
        }

        if ((gAbsentBattlerFlags & 1 << gBattlerTarget || GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE)
            && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget))
        {
            gBattlerTarget = GetBattlerAtPosition(GetBattlerPosition(gBattlerTarget) ^ BIT_FLANK);
        }
    }
    else if (GetBattlerBattleMoveTargetFlags(gChosenMove, gBattlerAttacker) == MOVE_TARGET_ALLY)
    {
        if (IsBattlerAlive(BATTLE_PARTNER(gBattlerAttacker)) && GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) < COMMANDER_ACTIVE)
            gBattlerTarget = BATTLE_PARTNER(gBattlerAttacker);
        else
            gBattlerTarget = gBattlerAttacker;
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
          && GetBattlerBattleMoveTargetFlags(gChosenMove, gBattlerAttacker) == MOVE_TARGET_FOES_AND_ALLY)
    {
        for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
        {
            if (gBattlerTarget == gBattlerAttacker)
                continue;
            if (GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE)
                continue;
            if (IsBattlerAlive(gBattlerTarget))
                break;
        }
    }
    else
    {
        if (!IsBattlerAlive(gBattlerTarget) || GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE)
        {
            if (GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget))
            {
                gBattlerTarget = GetBattlerAtPosition(GetBattlerPosition(gBattlerTarget) ^ BIT_FLANK);
            }
            else
            {
                gBattlerTarget = GetBattlerAtPosition(GetBattlerPosition(gBattlerAttacker) ^ BIT_SIDE);
                if (!IsBattlerAlive(gBattlerTarget) || GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE)
                    gBattlerTarget = GetBattlerAtPosition(GetBattlerPosition(gBattlerTarget) ^ BIT_FLANK);
            }
        }
    }

    if (gBattleTypeFlags & BATTLE_TYPE_PALACE && gRoundStructs[gBattlerAttacker].palaceUnableToUseMove)
    {
        // Battle Palace, select battle script for failure to use move
        if (gBattleMons[gBattlerAttacker].hp == 0)
        {
            gCurrentActionFuncId = B_ACTION_FINISHED;
            return;
        }
        else if (gPalaceSelectionBattleScripts[gBattlerAttacker] != NULL)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_INCAPABLE_OF_POWER;
            gBattlescriptCurrInstr = gPalaceSelectionBattleScripts[gBattlerAttacker];
            gPalaceSelectionBattleScripts[gBattlerAttacker] = NULL;
        }
        else
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_INCAPABLE_OF_POWER;
            gBattlescriptCurrInstr = BattleScript_MoveUsedLoafingAround;
        }
    }
    else
    {
        if (gProcessingExtraAttacks && !IsBattlerAlive(gBattlerTarget))
        {
            gCurrentActionFuncId = B_ACTION_TRY_FINISH;
        }
        else if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability)
        {
            gBattleScripting.abilityPopupOverwrite = gQueuedExtraAttackData[0].ability;
            gBattlescriptCurrInstr = BattleScript_AttackerUsedAnExtraMove;
        }
        else
        {
            gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
        }
    }

    if (gBattleTypeFlags & BATTLE_TYPE_ARENA)
        BattleArena_AddMindPoints(gBattlerAttacker);
    
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
            && IsBattlerAlive(gBattlerTarget)
            && GetBattlerSide(gBattlerTarget) != GetBattlerSide(gBattlerAttacker)
            && GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE)
    {
        gBattlerTarget = GetBattlerAtPosition(GetBattlerPosition(gBattlerTarget) ^ BIT_FLANK);
    }

    // Record HP of each battler
    for (i = 0; i < gBattlersCount; i++)
        gBattleStruct->hpBefore[i] = gBattleMons[i].hp;

    if (gProcessingExtraAttacks)
    {
        gTurnStructs[gBattlerAttacker].pranksterElevated = FALSE;
    }
    else
    {
        // Sets prankster elevated
        GetMovePriority(gBattlerAttacker, gCurrentMove, gBattlerTarget);
    }

    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_Switch(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gActionSelectionCursor[gBattlerAttacker] = 0;
    gMoveSelectionCursor[gBattlerAttacker] = 0;

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gBattlerAttacker, *(gBattleStruct->battlerPartyIndexes + gBattlerAttacker))
    
    gBattleScripting.battler = gBattlerAttacker;
    gBattlescriptCurrInstr = BattleScript_ActionSwitch;
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;

    if (gBattleResults.playerSwitchesCounter < 255)
        gBattleResults.playerSwitchesCounter++;

    UndoFormChange(gBattlerPartyIndexes[gBattlerAttacker], GetBattlerSide(gBattlerAttacker), TRUE);
}

void HandleAction_UseItem(void)
{
    gBattlerAttacker = gBattlerTarget = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    ClearFuryCutterDestinyBondGrudge(gBattlerAttacker);

    gLastUsedItem = gBattleResources->bufferB[gBattlerAttacker][1] | (gBattleResources->bufferB[gBattlerAttacker][2] << 8);

    if (gLastUsedItem <= LAST_BALL_INDEX) // is ball
    {
        gBattlescriptCurrInstr = gBattlescriptsForBallThrow[gLastUsedItem];
    }
    else if (gLastUsedItem == ITEM_POKE_DOLL || gLastUsedItem == ITEM_FLUFFY_TAIL)
    {
        gBattlescriptCurrInstr = gBattlescriptsForRunningByItem[0];
    }
    else if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
    {
        gBattlescriptCurrInstr = gBattlescriptsForUsingItem[0];
    }
    else
    {
        gBattleScripting.battler = gBattlerAttacker;

        switch (*(gBattleStruct->AI_itemType + (gBattlerAttacker >> 1)))
        {
        case AI_ITEM_FULL_RESTORE:
        case AI_ITEM_HEAL_HP:
            break;
        case AI_ITEM_CURE_CONDITION:
            gBattleCommunication[MULTISTRING_CHOOSER] = AI_HEAL_CONFUSION;
            if (*(gBattleStruct->AI_itemFlags + gBattlerAttacker / 2) & (1 << AI_HEAL_CONFUSION))
            {
                if (*(gBattleStruct->AI_itemFlags + gBattlerAttacker / 2) & 0x3E)
                    gBattleCommunication[MULTISTRING_CHOOSER] = AI_HEAL_SLEEP;
            }
            else
            {
                // Check for other statuses, stopping at first (shouldn't be more than one)
                while (!(*(gBattleStruct->AI_itemFlags + gBattlerAttacker / 2) & 1))
                {
                    *(gBattleStruct->AI_itemFlags + gBattlerAttacker / 2) >>= 1;
                    gBattleCommunication[MULTISTRING_CHOOSER]++;
                    // MULTISTRING_CHOOSER will be either AI_HEAL_PARALYSIS, AI_HEAL_FREEZE,
                    // AI_HEAL_BURN, AI_HEAL_POISON, or AI_HEAL_SLEEP
                }
            }
            break;
        case AI_ITEM_X_STAT:
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_ROSE_ITEM;
            if (*(gBattleStruct->AI_itemFlags + (gBattlerAttacker >> 1)) & (1 << AI_DIRE_HIT))
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_USED_DIRE_HIT;
            }
            else
            {
                PREPARE_STAT_BUFFER(gBattleTextBuff1, STAT_ATK)
                PREPARE_STRING_BUFFER(gBattleTextBuff2, CHAR_X)

                while (!((*(gBattleStruct->AI_itemFlags + (gBattlerAttacker >> 1))) & 1))
                {
                    *(gBattleStruct->AI_itemFlags + gBattlerAttacker / 2) >>= 1;
                    gBattleTextBuff1[2]++;
                }

                gBattleScripting.animArg1 = gBattleTextBuff1[2] + 14;
                gBattleScripting.animArg2 = 0;
            }
            break;
        case AI_ITEM_GUARD_SPEC:
            // It seems probable that at some point there was a special message for
            // an AI trainer using Guard Spec in a double battle.
            // There isn't now however, and the assignment to 2 below goes out of
            // bounds for gMistUsedStringIds and instead prints "{mon} is getting pumped"
            // from the next table, gFocusEnergyUsedStringIds.
            // In any case this isn't an issue in the retail version, as no trainers
            // are ever given any Guard Spec to use.
#ifndef UBFIX
            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MIST_FAILED + 1;
            else
#endif
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SET_MIST;
            break;
        }

        gBattlescriptCurrInstr = gBattlescriptsForUsingItem[*(gBattleStruct->AI_itemType + gBattlerAttacker / 2)];
    }
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

bool8 TryRunFromBattle(u8 battler)
{
    bool8 effect = FALSE;
    u8 holdEffect;
    u8 pyramidMultiplier;
    u8 speedVar;

    if (gBattleMons[battler].item == ITEM_ENIGMA_BERRY)
        holdEffect = gEnigmaBerries[battler].holdEffect;
    else
        holdEffect = ItemId_GetHoldEffect(gBattleMons[battler].item);

    gPotentialItemEffectBattler = battler;

    if (holdEffect == HOLD_EFFECT_SHED_SHELL)
    {
        gLastUsedItem = gBattleMons[battler].item;
        gRoundStructs[battler].fleeFlag = 1;
        effect++;
    }
    #if B_GHOSTS_ESCAPE >= GEN_6
    else if (IS_BATTLER_OF_TYPE(battler, TYPE_GHOST))
    {
        effect++;
    }
    #endif
    else if (GetBattlerAbility(battler) == ABILITY_RUN_AWAY)
    {
        if (InBattlePyramid())
        {
            gBattleStruct->runTries++;
            pyramidMultiplier = GetPyramidRunMultiplier();
            speedVar = (gBattleMons[battler].speed * pyramidMultiplier) / (gBattleMons[BATTLE_OPPOSITE(battler)].speed) + (gBattleStruct->runTries * 30);
            if (speedVar > (Random() & 0xFF))
            {
                gLastUsedAbility = ABILITY_RUN_AWAY;
                gRoundStructs[battler].fleeFlag = 2;
                effect++;
            }
        }
        else
        {
            gLastUsedAbility = ABILITY_RUN_AWAY;
            gRoundStructs[battler].fleeFlag = 2;
            effect++;
        }
    }
    else if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        effect++;
    }
    else
    {
        u8 runningFromBattler = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(runningFromBattler))
            runningFromBattler |= BIT_FLANK;

        if (InBattlePyramid())
        {
            pyramidMultiplier = GetPyramidRunMultiplier();
            speedVar = (gBattleMons[battler].speed * pyramidMultiplier) / (gBattleMons[runningFromBattler].speed) + (gBattleStruct->runTries * 30);
            if (speedVar > (Random() & 0xFF))
                effect++;
        }
        #ifdef DEBUG_BUILD
        else // Debug roms can always run
        {
            effect++;
        }
        #else
        else if (gBattleMons[battler].speed < gBattleMons[runningFromBattler].speed)
        {
            speedVar = (gBattleMons[battler].speed * 128) / (gBattleMons[runningFromBattler].speed) + (gBattleStruct->runTries * 30);
            if (speedVar > (Random() & 0xFF))
                effect++;
        }
        else // same speed or faster
        {
            effect++;
        }
        #endif

        gBattleStruct->runTries++;
    }

    if (effect)
    {
        gCurrentTurnActionNumber = gBattlersCount;
        gBattleOutcome = B_OUTCOME_RAN;
    }

    return effect;
}

void HandleAction_Run(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];

    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
    {
        gCurrentTurnActionNumber = gBattlersCount;

        for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
        {
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
            {
                if (gChosenActionByBattler[gActiveBattler] == B_ACTION_RUN)
                    gBattleOutcome |= B_OUTCOME_LOST;
            }
            else
            {
                if (gChosenActionByBattler[gActiveBattler] == B_ACTION_RUN)
                    gBattleOutcome |= B_OUTCOME_WON;
            }
        }

        gBattleOutcome |= B_OUTCOME_LINK_BATTLE_RAN;
        gSaveBlock2Ptr->frontier.disableRecordBattle = TRUE;
    }
    else
    {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        {
            if (!TryRunFromBattle(gBattlerAttacker)) // failed to run away
            {
                ClearFuryCutterDestinyBondGrudge(gBattlerAttacker);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_ESCAPE_2;
                gBattlescriptCurrInstr = BattleScript_PrintFailedToRunString;
                gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
            }
        }
        else
        {
            if (!CanBattlerEscape(gBattlerAttacker))
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ATTACKER_CANT_ESCAPE;
                gBattlescriptCurrInstr = BattleScript_PrintFailedToRunString;
                gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
            }
            else
            {
                gCurrentTurnActionNumber = gBattlersCount;
                gBattleOutcome = B_OUTCOME_MON_FLED;
            }
        }
    }
}

void HandleAction_WatchesCarefully(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattlescriptCurrInstr = gBattlescriptsForSafariActions[0];
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_SafariZoneBallThrow(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gNumSafariBalls--;
    gLastUsedItem = ITEM_SAFARI_BALL;
    gBattlescriptCurrInstr = gBattlescriptsForBallThrow[ITEM_SAFARI_BALL];
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_ThrowBall(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gLastUsedItem = gLastThrownBall;
    RemoveBagItem(gLastUsedItem, 1);
    gBattlescriptCurrInstr = BattleScript_BallThrow;
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_ShowBattleInfo(void)
{
    u8 value = 2;
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    VarSet(VAR_BATTLE_CONTROLLER_PLAYER_F, value);

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    VarSet(VAR_TEMP_SPECIAL_VAR, gActiveBattler);
    gBattlescriptCurrInstr = BattleScript_PrintCantRunFromTrainer;
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_ThrowPokeblock(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattleCommunication[MULTISTRING_CHOOSER] = gBattleResources->bufferB[gBattlerAttacker][1] - 1;
    gLastUsedItem = gBattleResources->bufferB[gBattlerAttacker][2];

    if (gBattleResults.pokeblockThrows < 0xFF)
        gBattleResults.pokeblockThrows++;
    if (gBattleStruct->safariPkblThrowCounter < 3)
        gBattleStruct->safariPkblThrowCounter++;
    if (gBattleStruct->safariEscapeFactor > 1)
    {
        // BUG: safariEscapeFactor can become 0 below. This causes the pokeblock throw glitch.
        #ifdef BUGFIX
        if (gBattleStruct->safariEscapeFactor <= sPkblToEscapeFactor[gBattleStruct->safariPkblThrowCounter][gBattleCommunication[MULTISTRING_CHOOSER]])
        #else
        if (gBattleStruct->safariEscapeFactor < sPkblToEscapeFactor[gBattleStruct->safariPkblThrowCounter][gBattleCommunication[MULTISTRING_CHOOSER]])
        #endif
            gBattleStruct->safariEscapeFactor = 1;
        else
            gBattleStruct->safariEscapeFactor -= sPkblToEscapeFactor[gBattleStruct->safariPkblThrowCounter][gBattleCommunication[MULTISTRING_CHOOSER]];
    }

    gBattlescriptCurrInstr = gBattlescriptsForSafariActions[2];
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_GoNear(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;

    gBattleStruct->safariCatchFactor += sGoNearCounterToCatchFactor[gBattleStruct->safariGoNearCounter];
    if (gBattleStruct->safariCatchFactor > 20)
        gBattleStruct->safariCatchFactor = 20;

    gBattleStruct->safariEscapeFactor += sGoNearCounterToEscapeFactor[gBattleStruct->safariGoNearCounter];
    if (gBattleStruct->safariEscapeFactor > 20)
        gBattleStruct->safariEscapeFactor = 20;

    if (gBattleStruct->safariGoNearCounter < 3)
    {
        gBattleStruct->safariGoNearCounter++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CREPT_CLOSER;
    }
    else
    {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_GET_CLOSER;
    }
    gBattlescriptCurrInstr = gBattlescriptsForSafariActions[1];
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
}

void HandleAction_SafariZoneRun(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    PlaySE(SE_FLEE);
    gCurrentTurnActionNumber = gBattlersCount;
    gBattleOutcome = B_OUTCOME_RAN;
}

void HandleAction_WallyBallThrow(void)
{
    gBattlerAttacker = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gBattlerAttacker, gBattlerPartyIndexes[gBattlerAttacker])

    gBattlescriptCurrInstr = gBattlescriptsForSafariActions[3];
    gCurrentActionFuncId = B_ACTION_EXEC_SCRIPT;
    gActionsByTurnOrder[1] = B_ACTION_FINISHED;
}

void ClearMiscTurnFlags() {
    gHitMarker &= ~(HITMARKER_DESTINYBOND | HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_ATTACKSTRING_PRINTED
                    | HITMARKER_NO_PPDEDUCT | HITMARKER_IGNORE_SAFEGUARD | HITMARKER_PASSIVE_DAMAGE
                    | HITMARKER_OBEYS | HITMARKER_x10 | HITMARKER_SYNCHRONISE_EFFECT
                    | HITMARKER_CHARGING | HITMARKER_x4000000 | HITMARKER_IGNORE_DISGUISE
                    | HITMARKER_NO_ATTACKSTRING);

    gCurrentMove = 0;
    gBattleMoveDamage = 0;
    gMoveResultFlags = 0;
    gBattleScripting.animTurn = 0;
    gBattleScripting.animTargetsHit = 0;
    gBattleStruct->dynamicMoveType = 0;
    gBattleScripting.moveendState = 0;
    gBattleScripting.abilityLoopCounter = 0;
    gBattleCommunication[3] = 0;
    gBattleCommunication[4] = 0;
    gBattleScripting.multihitMoveEffect = 0;
    gBattleResources->battleScriptsStack->size = 0;
    gBattleScripting.acceleratedTwoTurn = 0;
    gBattleScripting.usingExtraMove = FALSE;
    gChosenMove = MOVE_NONE;
    gSwapDamageCategory = FALSE;
}

void HandleAction_TryFinish(void)
{
    if (gQueuedAttackCount)
    {
        ClearMiscTurnFlags();
        gQueuedExtraAttackData[0] = gQueuedExtraAttackData[gQueuedAttackCount--];
        if (!IsBattlerAlive(gQueuedExtraAttackData[0].attacker)) return;
        gProcessingExtraAttacks = TRUE;
        gCurrentActionFuncId = B_ACTION_USE_MOVE;
        return;
    }
    
    gProcessingExtraAttacks = FALSE;

    if (gDelayedTurnActionId != B_ACTION_FINISHED)
    {
        ClearMiscTurnFlags();
        gCurrentActionFuncId = gDelayedTurnActionId;
        gDelayedTurnActionId = B_ACTION_FINISHED;
        return;
    }

    if (!HandleFaintedMonActions())
    {
        gBattleStruct->faintedActionsState = 0;
        gCurrentActionFuncId = B_ACTION_FINISHED;
    }
}

void TryPreemptiveActions()
{
    int battler = gBattlerByTurnOrder[gCurrentTurnActionNumber];
    if (gCurrentActionFuncId == B_ACTION_USE_MOVE)
    {
        int i;
        int move = GetChosenMove(battler);
        int target = GetMoveTarget(move, FALSE);
        int targetFlag = GetBattlerBattleMoveTargetFlags(move, battler);
        int surpriser = 0;
        if ((targetFlag == MOVE_TARGET_FOES_AND_ALLY || GetBattlerSide(target) != GetBattlerSide(battler))
            && GetMovePriority(battler, move, target) > 0
            && (surpriser = IsAbilityOnOpposingSide(battler, ABILITY_SURPRISE)))
        {
            gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct) {
                .ability = ABILITY_SURPRISE,
                .attacker = surpriser - 1,
                .move = MOVE_ASTONISH,
                .target = battler,
            };
        }
    }

    if (gCurrentActionFuncId == B_ACTION_SWITCH)
    {
        int i;
        for (i = BATTLE_OPPOSITE(GetBattlerSide(battler)); i < gBattlersCount; i += 2)
        {
            if (BATTLER_HAS_ABILITY(i, ABILITY_TAG))
            {
                gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct) {
                    .ability = ABILITY_TAG,
                    .attacker = i,
                    .move = MOVE_PURSUIT,
                    .target = battler,
                    .movePower = 20,
                };
            }
        }
    }

    if (gQueuedAttackCount)
    {
        gDelayedTurnActionId = gCurrentActionFuncId;
        gCurrentActionFuncId = B_ACTION_USE_MOVE;
        gQueuedExtraAttackData[0] = gQueuedExtraAttackData[gQueuedAttackCount--];
        gProcessingExtraAttacks = TRUE;
    }
}

void HandleAction_NothingIsFainted(void)
{
    RecalculateMoveOrder(++gCurrentTurnActionNumber, gBattlersCount);
    if (gCurrentTurnActionNumber < gBattlersCount)
        gCurrentActionFuncId = gActionsByTurnOrder[gCurrentTurnActionNumber];
    else
        gCurrentActionFuncId = B_ACTION_FINISHED;
    ClearMiscTurnFlags();
    TryPreemptiveActions();
}

void HandleAction_ActionFinished(void)
{
    gBattleStruct->monToSwitchIntoId[gBattlerByTurnOrder[gCurrentTurnActionNumber]] = 6;
    RecalculateMoveOrder(++gCurrentTurnActionNumber, gBattlersCount);
    if (gCurrentTurnActionNumber < gBattlersCount)
        gCurrentActionFuncId = gActionsByTurnOrder[gCurrentTurnActionNumber];
    else
        gCurrentActionFuncId = B_ACTION_FINISHED;
    TurnStructsClear();
    gRoundStructs[gBattlerAttacker].extraMoveUsed = 0;
    gLastLandedMoves[gBattlerAttacker] = 0;
    gLastHitByType[gBattlerAttacker] = 0;
    ClearMiscTurnFlags();
    TryPreemptiveActions();
}

// rom const data

static const u8 sAbilitiesAffectedByMoldBreaker[ABILITIES_COUNT] =
{
    [ABILITY_BATTLE_ARMOR] = 1,
    [ABILITY_CLEAR_BODY] = 1,
    // [ABILITY_DAMP] = 1,
    [ABILITY_DRY_SKIN] = 1,
    [ABILITY_FILTER] = 1,
    [ABILITY_FLASH_FIRE] = 1,
    [ABILITY_FLOWER_GIFT] = 1,
    [ABILITY_HEATPROOF] = 1,
    [ABILITY_HYPER_CUTTER] = 1,
    [ABILITY_IMMUNITY] = 1,
    [ABILITY_INNER_FOCUS] = 1,
    [ABILITY_INSOMNIA] = 1,
    [ABILITY_KEEN_EYE] = 1,
    [ABILITY_LEAF_GUARD] = 1,
    [ABILITY_LEVITATE] = 1,
    [ABILITY_LIGHTNING_ROD] = 1,
    [ABILITY_LIMBER] = 1,
    [ABILITY_MAGMA_ARMOR] = 1,
    [ABILITY_MARVEL_SCALE] = 1,
    [ABILITY_MOTOR_DRIVE] = 1,
    [ABILITY_OBLIVIOUS] = 1,
    [ABILITY_OWN_TEMPO] = 1,
    [ABILITY_SAND_VEIL] = 1,
    [ABILITY_SHELL_ARMOR] = 1,
    [ABILITY_SHIELD_DUST] = 1,
    [ABILITY_SNOW_CLOAK] = 1,
    [ABILITY_SOLID_ROCK] = 1,
    [ABILITY_SOUNDPROOF] = 1,
    [ABILITY_STICKY_HOLD] = 1,
    [ABILITY_STORM_DRAIN] = 1,
    [ABILITY_STURDY] = 1,
    [ABILITY_SUCTION_CUPS] = 1,
    [ABILITY_TANGLED_FEET] = 1,
    [ABILITY_THICK_FAT] = 1,
    [ABILITY_UNAWARE] = 1,
    [ABILITY_VITAL_SPIRIT] = 1,
    [ABILITY_VOLT_ABSORB] = 1,
    [ABILITY_WATER_ABSORB] = 1,
    [ABILITY_WATER_VEIL] = 1,
    [ABILITY_WHITE_SMOKE] = 1,
    [ABILITY_WONDER_GUARD] = 1,
    [ABILITY_BIG_PECKS] = 1,
    [ABILITY_CONTRARY] = 1,
    [ABILITY_FRIEND_GUARD] = 1,
    [ABILITY_HEAVY_METAL] = 1,
    [ABILITY_LIGHT_METAL] = 1,
    [ABILITY_MAGIC_BOUNCE] = 1,
    [ABILITY_MULTISCALE] = 1,
    [ABILITY_SAP_SIPPER] = 1,
    [ABILITY_TELEPATHY] = 1,
    [ABILITY_WONDER_SKIN] = 1,
    [ABILITY_AROMA_VEIL] = 1,
    [ABILITY_BULLETPROOF] = 1,
    [ABILITY_FLOWER_VEIL] = 1,
    [ABILITY_FUR_COAT] = 1,
    [ABILITY_OVERCOAT] = 1,
    [ABILITY_SWEET_VEIL] = 1,
    [ABILITY_DAZZLING] = 1,
    [ABILITY_DISGUISE] = 1,
    [ABILITY_FLUFFY] = 1,
    [ABILITY_QUEENLY_MAJESTY] = 1,
    [ABILITY_WATER_BUBBLE] = 1,
    [ABILITY_MIRROR_ARMOR] = 1,
    [ABILITY_PUNK_ROCK] = 1,
    [ABILITY_ICE_SCALES] = 1,
    [ABILITY_ICE_FACE] = 1,

    // New abilities
    [ABILITY_DUNE_TERROR] = 1,
    [ABILITY_GIFTED_MIND] = 1,
    [ABILITY_HYDRO_CIRCUIT] = 1,
    [ABILITY_DESERT_CLOAK] = 1,
    [ABILITY_ARCTIC_FUR] = 1,
    [ABILITY_BIG_LEAVES] = 1,
    [ABILITY_POISON_ABSORB] = 1,
    [ABILITY_SEAWEED] = 1,
    [ABILITY_RAW_WOOD] = 1,
    [ABILITY_BAD_LUCK] = 1,
    [ABILITY_JUGGERNAUT] = 1,
    [ABILITY_MOUNTAINEER] = 1,
    [ABILITY_DRAGONFLY] = 1,
    [ABILITY_LIQUIFIED] = 1,
    [ABILITY_FOSSILIZED] = 1,
    [ABILITY_LEAD_COAT] = 1,
    [ABILITY_CHRISTMAS_SPIRIT] = 1,
    [ABILITY_AERODYNAMICS] = 1,
    [ABILITY_WATER_COMPACTION] = 1,
    [ABILITY_GRASS_PELT] = 1,
    [ABILITY_PRIMAL_ARMOR] = 1,
    [ABILITY_WEATHER_CONTROL] = 1,
    [ABILITY_ICE_DEW] = 1,
    [ABILITY_WELL_BAKED_BODY] = 1,
    [ABILITY_EVAPORATE] = 1,
    [ABILITY_RADIANCE] = 1,
    [ABILITY_JUNGLES_GUARD] = 1,
    [ABILITY_EARTH_EATER] = 1,
    [ABILITY_SAND_GUARD] = 1,
    [ABILITY_WIND_RIDER] = 1,
    [ABILITY_ENLIGHTENED] = 1,
    [ABILITY_BASS_BOOSTED] = 1,
    [ABILITY_CHROME_COAT] = 1,
    [ABILITY_PURIFYING_SALT] = 1,
    [ABILITY_LEAF_GUARD_CLONE] = 1,
    [ABILITY_GOOD_AS_GOLD] = 1,
    [ABILITY_THERMAL_EXCHANGE] = 1,
    [ABILITY_NOISE_CANCEL] = 1,
    [ABILITY_PARROTING] = 1,
    [ABILITY_GALLANTRY] = 1,
    [ABILITY_ANTICIPATION] = 1,
    [ABILITY_AERIALIST] = 1,
    [ABILITY_FIRE_SCALES] = 1,
    [ABILITY_APPLE_ENLIGHTENMENT] = 1,
    [ABILITY_OLD_MARINER] = 1,
    [ABILITY_LAST_STAND] = 1,
    [ABILITY_BLOOD_BATH] = 1,
    [ABILITY_BLOODLUST] = 1,
    [ABILITY_SMOKEY_MANEUVERS] = 1,
    [ABILITY_BAD_OMEN] = 1,
    [ABILITY_FLUFFIEST] = 1,
    [ABILITY_IRON_GIANT] = 1,
    [ABILITY_ENLIGHTENED] = 1,
    [ABILITY_WAY_OF_PRECISION] = 1,
    [ABILITY_HUGE_WINGS] = 1,
    [ABILITY_PATCHWORK] = 1,
    [ABILITY_TERASTAL_TREASURE] = 1,
    [ABILITY_DREAM_STATE] = 1,
    [ABILITY_FLAME_SHIELD] = 1,
    [ABILITY_HOVER] = 1,
    [ABILITY_LUCKY_HALO] = 1,
    [ABILITY_DRAGONSLAYER] = 1,
    // Intentionally not included: 
    //   Color Change
    //   Prismatic Fur
    //   Cheating Death
    //   Delta Stream
    //   Unlocked Potential
};

static const u8 sHoldEffectToType[][2] =
{
    {HOLD_EFFECT_BUG_POWER, TYPE_BUG},
    {HOLD_EFFECT_STEEL_POWER, TYPE_STEEL},
    {HOLD_EFFECT_GROUND_POWER, TYPE_GROUND},
    {HOLD_EFFECT_ROCK_POWER, TYPE_ROCK},
    {HOLD_EFFECT_GRASS_POWER, TYPE_GRASS},
    {HOLD_EFFECT_DARK_POWER, TYPE_DARK},
    {HOLD_EFFECT_FIGHTING_POWER, TYPE_FIGHTING},
    {HOLD_EFFECT_ELECTRIC_POWER, TYPE_ELECTRIC},
    {HOLD_EFFECT_WATER_POWER, TYPE_WATER},
    {HOLD_EFFECT_FLYING_POWER, TYPE_FLYING},
    {HOLD_EFFECT_POISON_POWER, TYPE_POISON},
    {HOLD_EFFECT_ICE_POWER, TYPE_ICE},
    {HOLD_EFFECT_GHOST_POWER, TYPE_GHOST},
    {HOLD_EFFECT_PSYCHIC_POWER, TYPE_PSYCHIC},
    {HOLD_EFFECT_FIRE_POWER, TYPE_FIRE},
    {HOLD_EFFECT_DRAGON_POWER, TYPE_DRAGON},
    {HOLD_EFFECT_NORMAL_POWER, TYPE_NORMAL},
    {HOLD_EFFECT_FAIRY_POWER, TYPE_FAIRY},
};

// percent in UQ_4_12 format
static const u16 sPercentToModifier[] =
{
    UQ_4_12(0.00), // 0
    UQ_4_12(0.01), // 1
    UQ_4_12(0.02), // 2
    UQ_4_12(0.03), // 3
    UQ_4_12(0.04), // 4
    UQ_4_12(0.05), // 5
    UQ_4_12(0.06), // 6
    UQ_4_12(0.07), // 7
    UQ_4_12(0.08), // 8
    UQ_4_12(0.09), // 9
    UQ_4_12(0.10), // 10
    UQ_4_12(0.11), // 11
    UQ_4_12(0.12), // 12
    UQ_4_12(0.13), // 13
    UQ_4_12(0.14), // 14
    UQ_4_12(0.15), // 15
    UQ_4_12(0.16), // 16
    UQ_4_12(0.17), // 17
    UQ_4_12(0.18), // 18
    UQ_4_12(0.19), // 19
    UQ_4_12(0.20), // 20
    UQ_4_12(0.21), // 21
    UQ_4_12(0.22), // 22
    UQ_4_12(0.23), // 23
    UQ_4_12(0.24), // 24
    UQ_4_12(0.25), // 25
    UQ_4_12(0.26), // 26
    UQ_4_12(0.27), // 27
    UQ_4_12(0.28), // 28
    UQ_4_12(0.29), // 29
    UQ_4_12(0.30), // 30
    UQ_4_12(0.31), // 31
    UQ_4_12(0.32), // 32
    UQ_4_12(0.33), // 33
    UQ_4_12(0.34), // 34
    UQ_4_12(0.35), // 35
    UQ_4_12(0.36), // 36
    UQ_4_12(0.37), // 37
    UQ_4_12(0.38), // 38
    UQ_4_12(0.39), // 39
    UQ_4_12(0.40), // 40
    UQ_4_12(0.41), // 41
    UQ_4_12(0.42), // 42
    UQ_4_12(0.43), // 43
    UQ_4_12(0.44), // 44
    UQ_4_12(0.45), // 45
    UQ_4_12(0.46), // 46
    UQ_4_12(0.47), // 47
    UQ_4_12(0.48), // 48
    UQ_4_12(0.49), // 49
    UQ_4_12(0.50), // 50
    UQ_4_12(0.51), // 51
    UQ_4_12(0.52), // 52
    UQ_4_12(0.53), // 53
    UQ_4_12(0.54), // 54
    UQ_4_12(0.55), // 55
    UQ_4_12(0.56), // 56
    UQ_4_12(0.57), // 57
    UQ_4_12(0.58), // 58
    UQ_4_12(0.59), // 59
    UQ_4_12(0.60), // 60
    UQ_4_12(0.61), // 61
    UQ_4_12(0.62), // 62
    UQ_4_12(0.63), // 63
    UQ_4_12(0.64), // 64
    UQ_4_12(0.65), // 65
    UQ_4_12(0.66), // 66
    UQ_4_12(0.67), // 67
    UQ_4_12(0.68), // 68
    UQ_4_12(0.69), // 69
    UQ_4_12(0.70), // 70
    UQ_4_12(0.71), // 71
    UQ_4_12(0.72), // 72
    UQ_4_12(0.73), // 73
    UQ_4_12(0.74), // 74
    UQ_4_12(0.75), // 75
    UQ_4_12(0.76), // 76
    UQ_4_12(0.77), // 77
    UQ_4_12(0.78), // 78
    UQ_4_12(0.79), // 79
    UQ_4_12(0.80), // 80
    UQ_4_12(0.81), // 81
    UQ_4_12(0.82), // 82
    UQ_4_12(0.83), // 83
    UQ_4_12(0.84), // 84
    UQ_4_12(0.85), // 85
    UQ_4_12(0.86), // 86
    UQ_4_12(0.87), // 87
    UQ_4_12(0.88), // 88
    UQ_4_12(0.89), // 89
    UQ_4_12(0.90), // 90
    UQ_4_12(0.91), // 91
    UQ_4_12(0.92), // 92
    UQ_4_12(0.93), // 93
    UQ_4_12(0.94), // 94
    UQ_4_12(0.95), // 95
    UQ_4_12(0.96), // 96
    UQ_4_12(0.97), // 97
    UQ_4_12(0.98), // 98
    UQ_4_12(0.99), // 99
    UQ_4_12(1.00), // 100
};

#define X UQ_4_12

static const u16 sTypeEffectivenessTable[NUMBER_OF_MON_TYPES][NUMBER_OF_MON_TYPES] =
{
//   normal  fight   flying  poison  ground  rock    bug     ghost   steel   mystery fire    water   grass  electric psychic ice     dragon  dark    fairy
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(0.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // normal
    {X(2.0), X(1.0), X(0.5), X(0.5), X(1.0), X(2.0), X(0.5), X(0.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0), X(2.0), X(0.5), X(1.0)}, // fight
    {X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // flying
    {X(1.0), X(1.0), X(1.0), X(0.5), X(0.5), X(0.5), X(1.0), X(0.5), X(0.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0)}, // poison
    {X(1.0), X(1.0), X(0.0), X(2.0), X(1.0), X(2.0), X(0.5), X(1.0), X(2.0), X(1.0), X(2.0), X(1.0), X(0.5), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // ground
    {X(1.0), X(0.5), X(2.0), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // rock
    {X(1.0), X(0.5), X(0.5), X(0.5), X(1.0), X(1.0), X(1.0), X(0.5), X(0.5), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(2.0), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0)}, // bug
    {X(0.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0)}, // ghost
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(0.5), X(1.0), X(0.5), X(0.5), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(1.0), X(2.0), X(1.0)}, // steel
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // mystery
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0), X(2.0), X(1.0), X(0.5), X(0.5), X(2.0), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(1.0), X(1.0)}, // fire
    {X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(0.5), X(0.5), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0)}, // water
    {X(1.0), X(1.0), X(0.5), X(0.5), X(2.0), X(2.0), X(0.5), X(1.0), X(0.5), X(1.0), X(0.5), X(2.0), X(0.5), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0)}, // grass
    {X(1.0), X(1.0), X(2.0), X(1.0), X(0.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(0.5), X(0.5), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0)}, // electric
    {X(1.0), X(2.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(0.0), X(1.0), X(1.0)}, // psychic
    {X(1.0), X(1.0), X(2.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(0.5), X(0.5), X(2.0), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0), X(1.0), X(1.0)}, // ice
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(0.0), X(1.0)}, // dragon
    {X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(0.5), X(0.5), X(1.0)}, // dark
    {X(1.0), X(2.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(2.0), X(1.0), X(1.0)}, // fairy
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0)}, // stellar
};

static const u16 sInverseTypeEffectivenessTable[NUMBER_OF_MON_TYPES][NUMBER_OF_MON_TYPES] =
{
//   normal  fight   flying  poison  ground  rock    bug     ghost   steel   mystery fire    water   grass  electric psychic ice     dragon  dark    fairy
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(2.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // normal
    {X(0.5), X(1.0), X(2.0), X(2.0), X(1.0), X(0.5), X(2.0), X(2.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(0.5), X(2.0), X(1.0)}, // fight
    {X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // flying
    {X(1.0), X(1.0), X(1.0), X(2.0), X(2.0), X(2.0), X(1.0), X(2.0), X(2.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0)}, // poison
    {X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(0.5), X(2.0), X(1.0), X(0.5), X(1.0), X(0.5), X(1.0), X(2.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // ground
    {X(1.0), X(2.0), X(0.5), X(1.0), X(2.0), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0)}, // rock
    {X(1.0), X(2.0), X(2.0), X(2.0), X(1.0), X(1.0), X(1.0), X(2.0), X(2.0), X(1.0), X(2.0), X(1.0), X(0.5), X(1.0), X(0.5), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0)}, // bug
    {X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0)}, // ghost
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(2.0), X(1.0), X(2.0), X(2.0), X(1.0), X(2.0), X(1.0), X(0.5), X(1.0), X(1.0), X(0.5), X(1.0)}, // steel
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0)}, // mystery
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(0.5), X(1.0), X(2.0), X(2.0), X(0.5), X(1.0), X(1.0), X(0.5), X(2.0), X(1.0), X(1.0), X(1.0)}, // fire
    {X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(2.0), X(2.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0)}, // water
    {X(1.0), X(1.0), X(2.0), X(2.0), X(0.5), X(0.5), X(2.0), X(1.0), X(2.0), X(1.0), X(2.0), X(0.5), X(2.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0)}, // grass
    {X(1.0), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(2.0), X(2.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0)}, // electric
    {X(1.0), X(0.5), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0)}, // psychic
    {X(1.0), X(1.0), X(0.5), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(2.0), X(2.0), X(0.5), X(1.0), X(1.0), X(2.0), X(0.5), X(1.0), X(1.0), X(1.0)}, // ice
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(2.0), X(1.0)}, // dragon
    {X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(1.0), X(1.0), X(2.0), X(2.0), X(1.0)}, // dark
    {X(1.0), X(0.5), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(2.0), X(1.0), X(2.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5), X(0.5), X(1.0), X(1.0)}, // fairy
    {X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(1.0), X(0.5)}, // stellar
};

#undef X

// code
u8 GetBattlerForBattleScript(u8 caseId)
{
    u8 ret = 0;
    switch (caseId)
    {
    case BS_TARGET:
        ret = gBattlerTarget;
        break;
    case BS_CHOOSE_FAINTED_MON:
    case BS_ATTACKER:
        ret = gBattlerAttacker;
        break;
    case BS_EFFECT_BATTLER:
        ret = gEffectBattler;
        break;
    case BS_BATTLER_0:
        ret = 0;
        break;
    case BS_SCRIPTING:
        ret = gBattleScripting.battler;
        break;
    case BS_FAINTED:
        ret = gBattlerFainted;
        break;
    case 5:
        ret = gBattlerFainted;
        break;
    case 4:
    case 6:
    case 8:
    case 9:
    case BS_PLAYER1:
        ret = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        break;
    case BS_OPPONENT1:
        ret = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        break;
    case BS_PLAYER2:
        ret = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
        break;
    case BS_OPPONENT2:
        ret = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        break;
    case BS_ABILITY_BATTLER:
        ret = gBattlerAbility;
        break;
    case BS_ABILITY_PARTNER:
        ret = BATTLE_PARTNER(gBattlerAbility);
        break;
    case BS_TARGET_PARTNER:
        ret = BATTLE_PARTNER(gBattlerTarget);
        break;
    case BS_ATTACKER_PARTNER:
        ret = BATTLE_PARTNER(gBattlerAttacker);
        break;
    case BS_STACK_1:
        ret = gStackBattler1;
        break;
    case BS_STACK_2:
        ret = gStackBattler2;
        break;
    case BS_STACK_3:
        ret = gStackBattler3;
        break;
    case BS_STACK_4:
        ret = gStackBattler4;
        break;
    }
    return ret;
}

void MarkAllBattlersForControllerExec(void) // unused
{
    int i;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        for (i = 0; i < gBattlersCount; i++)
            gBattleControllerExecFlags |= (1 << i) << (32 - MAX_BATTLERS_COUNT);
    }
    else
    {
        for (i = 0; i < gBattlersCount; i++)
            gBattleControllerExecFlags |= 1 << i;
    }
}

bool32 IsBattlerMarkedForControllerExec(u8 battlerId)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        return (gBattleControllerExecFlags & ((1 << battlerId) << 0x1C)) != 0;
    else
        return (gBattleControllerExecFlags & (1 << battlerId)) != 0;
}

void MarkBattlerForControllerExec(u8 battlerId)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        gBattleControllerExecFlags |= (1 << battlerId) << (32 - MAX_BATTLERS_COUNT);
    else
        gBattleControllerExecFlags |= 1 << battlerId;
}

void MarkBattlerReceivedLinkData(u8 battlerId)
{
    s32 i;

    for (i = 0; i < GetLinkPlayerCount(); i++)
        gBattleControllerExecFlags |= (1 << battlerId) << (i << 2);

    gBattleControllerExecFlags &= ~(0x10000000 << battlerId);
}

void CancelMultiTurnMoves(u8 battler)
{
    gBattleMons[battler].status2 &= ~(STATUS2_MULTIPLETURNS);
    gBattleMons[battler].status2 &= ~(STATUS2_LOCK_CONFUSE);
    gBattleMons[battler].status2 &= ~(STATUS2_UPROAR);
    gBattleMons[battler].status2 &= ~(STATUS2_BIDE);

    gStatuses3[battler] &= ~(STATUS3_SEMI_INVULNERABLE);

    gVolatileStructs[battler].rolloutCounter = 0;
    gVolatileStructs[battler].furyCutterCounter = 0;
}

bool8 WasUnableToUseMove(u8 battler)
{
    if (gRoundStructs[battler].prlzImmobility
        || gRoundStructs[battler].usedImprisonedMove
        || gRoundStructs[battler].loveImmobility
        || gRoundStructs[battler].usedDisabledMove
        || gRoundStructs[battler].usedTauntedMove
        || gRoundStructs[battler].usedGravityPreventedMove
        || gRoundStructs[battler].usedHealBlockedMove
        || gRoundStructs[battler].flag2Unknown
        || gRoundStructs[battler].flinchImmobility
        || gRoundStructs[battler].confusionSelfDmg
        || gRoundStructs[battler].powderSelfDmg
        || gRoundStructs[battler].usedThroatChopPreventedMove)
        return TRUE;
    else
        return FALSE;
}

void PrepareStringBattle(u16 stringId, u8 battler)
{
    bool8 hasContrary = BATTLER_HAS_ABILITY(battler, ABILITY_CONTRARY);
    bool8 targetHasContrary = BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_CONTRARY);
    u8 abilityBattler;

    //Overwrite
    if(VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_1) != 0){
        stringId = VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_1);
        VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_1, 0);
    }
    else if(VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_2) != 0){
        stringId = VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_2);
        VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_2, 0);
    }
    else if(VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_3) != 0){
        stringId = VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_3);
        VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_3, 0);
    }
    else if(VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_4) != 0){
        stringId = VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_4);
        VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_4, 0);
    }

    // Support for Contrary ability.
    // If a move attempted to raise stat - print "won't increase".
    // If a move attempted to lower stat - print "won't decrease".
    if (stringId == STRINGID_STATSWONTDECREASE && !(gBattleScripting.statChanger & STAT_BUFF_NEGATIVE))
        stringId = STRINGID_STATSWONTINCREASE;
    else if (stringId == STRINGID_STATSWONTINCREASE && gBattleScripting.statChanger & STAT_BUFF_NEGATIVE)
        stringId = STRINGID_STATSWONTDECREASE;
    else if (stringId == STRINGID_STATSWONTDECREASE2 && hasContrary)
        stringId = STRINGID_STATSWONTINCREASE2;
    else if (stringId == STRINGID_STATSWONTINCREASE2 && hasContrary)
        stringId = STRINGID_STATSWONTDECREASE2;
    else if (stringId == STRINGID_PKMNCUTSSTATWITHINTIMIDATECLONE && (targetHasContrary || BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_GUARD_DOG)))
        stringId = STRINGID_PKMNRAISESSTATWITHINTIMIDATECLONE;
    else if (stringId == STRINGID_PKMNCUTSSTATWITHINTIMIDATECLONE2 && (targetHasContrary || BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_GUARD_DOG)))
        stringId = STRINGID_PKMNRAISESSTATWITHINTIMIDATECLONE2;
    else if (stringId == STRINGID_PKMNCUTSSTATWITHINTIMIDATECLONE3 && (targetHasContrary || BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_GUARD_DOG)))
        stringId = STRINGID_PKMNRAISESSTATWITHINTIMIDATECLONE3;
    else if((stringId == STRINGID_DEFENDERSSTATFELL) &&
            (abilityBattler = IsAbilityOnSide(gBattlerTarget, ABILITY_KINGS_WRATH)) &&
            gTurnStructs[gBattlerTarget].changedStatsBattlerId != BATTLE_PARTNER(gBattlerTarget) &&
            gTurnStructs[gBattlerTarget].changedStatsBattlerId != gBattlerTarget){
            
            //Overwrites the Popout
			gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_KINGS_WRATH;
            //Overwrites where it's written
            if(abilityBattler - 1 != gBattlerTarget){
                gBattleScripting.battlerPopupOverwrite = BATTLE_PARTNER(gBattlerTarget);
                gBattlerAbility = BATTLE_PARTNER(gBattlerTarget);
            }
            else
                gBattlerAbility = gBattlerTarget;
            
            BattleScriptCall(BattleScript_KingsWrathActivated);
    }
    else if((stringId == STRINGID_DEFENDERSSTATFELL) &&
            (abilityBattler = IsAbilityOnSide(gBattlerTarget, ABILITY_QUEENS_MOURNING)) &&
            gTurnStructs[gBattlerTarget].changedStatsBattlerId != BATTLE_PARTNER(gBattlerTarget) &&
            gTurnStructs[gBattlerTarget].changedStatsBattlerId != gBattlerTarget){
            
            //Overwrites the Popout
			gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_QUEENS_MOURNING;
            //Overwrites where it's written
            if(abilityBattler - 1 != gBattlerTarget){
                gBattleScripting.battlerPopupOverwrite = BATTLE_PARTNER(gBattlerTarget);
                gBattlerAbility = BATTLE_PARTNER(gBattlerTarget);
            }
            else
                gBattlerAbility = gBattlerTarget;
            
            BattleScriptCall(BattleScript_QueensMourningActivated);
    }
    else if (stringId == STRINGID_DEFENDERSSTATFELL && (BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_DEFIANT) || BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_CONTEMPT)))
    {
        gBattleScripting.abilityPopupOverwrite = BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_DEFIANT) ? ABILITY_DEFIANT : ABILITY_CONTEMPT;
        gBattlerAbility = gBattlerTarget;
        BattleScriptCall(BattleScript_DefiantActivates);
    }
    else if (stringId == STRINGID_DEFENDERSSTATFELL && BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_COMPETITIVE))
    {
        gBattleScripting.abilityPopupOverwrite = ABILITY_COMPETITIVE;
        gBattlerAbility = gBattlerTarget;
        BattleScriptCall(BattleScript_CompetitiveActivates);
    }
    else if (stringId == STRINGID_DEFENDERSSTATFELL && BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_FORT_KNOX))
    {
        gBattleScripting.abilityPopupOverwrite = ABILITY_FORT_KNOX;
        gBattlerAbility = gBattlerTarget;
        BattleScriptCall(BattleScript_FortKnoxActivates);
    }
    else if (stringId == STRINGID_DEFENDERSSTATFELL && BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_RUN_AWAY))
    {
        gBattleScripting.abilityPopupOverwrite = ABILITY_RUN_AWAY;
        gBattlerAbility = gBattlerTarget;
        BattleScriptCall(BattleScript_RunAwayActivates);
    }
    
    gActiveBattler = battler;
    BtlController_EmitPrintString(0, stringId);
    MarkBattlerForControllerExec(gActiveBattler);
}

void ResetSentPokesToOpponentValue(void)
{
    s32 i;
    u32 bits = 0;

    gSentPokesToOpponent[0] = 0;
    gSentPokesToOpponent[1] = 0;

    for (i = 0; i < gBattlersCount; i += 2)
        bits |= gBitTable[gBattlerPartyIndexes[i]];

    for (i = 1; i < gBattlersCount; i += 2)
        gSentPokesToOpponent[(i & BIT_FLANK) >> 1] = bits;
}

void OpponentSwitchInResetSentPokesToOpponentValue(u8 battler)
{
    s32 i = 0;
    u32 bits = 0;

    if (GetBattlerSide(battler) == B_SIDE_OPPONENT)
    {
        u8 flank = ((battler & BIT_FLANK) >> 1);
        gSentPokesToOpponent[flank] = 0;

        for (i = 0; i < gBattlersCount; i += 2)
        {
            if (!(gAbsentBattlerFlags & 1 << i))
                bits |= gBitTable[gBattlerPartyIndexes[i]];
        }

        gSentPokesToOpponent[flank] = bits;
    }
}

void UpdateSentPokesToOpponentValue(u8 battler)
{
    if (GetBattlerSide(battler) == B_SIDE_OPPONENT)
    {
        OpponentSwitchInResetSentPokesToOpponentValue(battler);
    }
    else
    {
        s32 i;
        for (i = 1; i < gBattlersCount; i++)
            gSentPokesToOpponent[(i & BIT_FLANK) >> 1] |= gBitTable[gBattlerPartyIndexes[battler]];
    }
}

void BattleScriptPush(const u8 *bsPtr)
{
    gBattleResources->battleScriptsStack->ptr[gBattleResources->battleScriptsStack->size++] = bsPtr;
    BattleScriptSaveCurrentStackData();
}

void BattleScriptSaveCurrentStackData()
{
    struct SavedStackData savedStackData = 
        {
            .abilityOverride = gBattleScripting.abilityPopupOverwrite,
            .multistringChooser = gBattleCommunication[MULTISTRING_CHOOSER],
            .stackBattler1 = gStackBattler1,
            .stackBattler2 = gStackBattler2,
            .stackBattler3 = gStackBattler3,
            .stackBattler4 = gStackBattler4,
        };
    gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size] = savedStackData;
}

void BattleScriptPushCursor()
{
    BattleScriptPush(gBattlescriptCurrInstr);
}

void BattleScriptCall(const u8* command)
{
    BattleScriptPushCursor();
    gBattlescriptCurrInstr = command;
}

void BattleScriptPop(void)
{
    gBattlescriptCurrInstr = gBattleResources->battleScriptsStack->ptr[--gBattleResources->battleScriptsStack->size];
    ReadActiveScriptInitialStackState();
}

void ReadActiveScriptInitialStackState()
{
    struct SavedStackData *data = &gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size];
    gBattleScripting.abilityPopupOverwrite = data->abilityOverride;
    gBattleCommunication[MULTISTRING_CHOOSER] = data->multistringChooser;
    gStackBattler1 = data->stackBattler1;
    gStackBattler2 = data->stackBattler2;
    gStackBattler3 = data->stackBattler3;
    gStackBattler4 = data->stackBattler4;
}

void SetActiveMultistringChooser(u8 messageId)
{
    gBattleCommunication[MULTISTRING_CHOOSER] = messageId;
    gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size].multistringChooser = messageId;
}

void SetActiveAbilityPopupOverride(u16 abilityPopupOverride)
{
    gBattleScripting.abilityPopupOverwrite = abilityPopupOverride;
    gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size].abilityOverride = abilityPopupOverride;
}

void SetActiveStackBattler(u8 battler, u8 number)
{
    switch (number)
    {
    case 1:
        gStackBattler1 = battler;
        gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size].stackBattler1 = battler;
        return;
    case 2:
        gStackBattler2 = battler;
        gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size].stackBattler2 = battler;
        return;
    case 3:
        gStackBattler3 = battler;
        gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size].stackBattler3 = battler;
        return;
    case 4:
        gStackBattler4 = battler;
        gBattleResources->battleScriptsStack->savedStackData[gBattleResources->battleScriptsStack->size].stackBattler4 = battler;
        return;
    }
}

bool32 IsGravityPreventingMove(u32 move)
{
    if (!IsGravityActive())
        return FALSE;

    switch (move)
    {
    case MOVE_BOUNCE:
    case MOVE_FLY:
    case MOVE_FLYING_PRESS:
    case MOVE_HIGH_JUMP_KICK:
    case MOVE_JUMP_KICK:
    case MOVE_MAGNET_RISE:
    case MOVE_SKY_DROP:
    case MOVE_SPLASH:
    case MOVE_TELEKINESIS:
    case MOVE_FLOATY_FALL:
        return TRUE;
    default:
        return FALSE;
    }
}

bool32 IsHealBlockPreventingMove(u8 battler, u32 move)
{
    if (!(gStatuses3[battler] & STATUS3_HEAL_BLOCK) && !IsAbilityOnOpposingSide(battler, ABILITY_PERMANENCE))
        return FALSE;
    
    return IsHealingMoveEffect(gBattleMoves[move].effect) && gBattleMoves[move].split == SPLIT_STATUS;
}

static bool32 IsBelchPreventingMove(u32 battler, u32 move)
{
    if (gBattleMoves[move].effect != EFFECT_BELCH)
        return FALSE;

    if (gBattleStruct->ateBerry[battler & BIT_SIDE] & gBitTable[gBattlerPartyIndexes[battler]])
        return FALSE;
    
    return ItemId_GetPocket(gBattleMons[battler].item) != POCKET_BERRIES || IsUnnerveAbilityOnOpposingSide(battler);
}

u8 TrySetCantSelectMoveBattleScript(void)
{
    u32 limitations = 0;
    u8 moveId = gBattleResources->bufferB[gActiveBattler][2] & ~(RET_MEGA_EVOLUTION);
    u32 move = gBattleMons[gActiveBattler].moves[moveId];
    u32 holdEffect = GetBattlerHoldEffect(gActiveBattler, TRUE);
    u16 *choicedMove = &gBattleStruct->choicedMove[gActiveBattler];

    if (gVolatileStructs[gActiveBattler].disabledMove == move && move != MOVE_NONE)
    {
        gBattleScripting.battler = gActiveBattler;
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingDisabledMoveInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingDisabledMove;
            limitations++;
        }
    }
    else if (gBattleMoves[move].split == SPLIT_STATUS && getMonotypeChampType() == TYPE_FIGHTING && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
    {
        gBattleScripting.battler = gActiveBattler;
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingDisabledMoveInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingDisabledMove;
            limitations++;
        }
    }

    if (move == gLastMoves[gActiveBattler] && move != MOVE_STRUGGLE && (gBattleMons[gActiveBattler].status2 & STATUS2_TORMENT))
    {
        CancelMultiTurnMoves(gActiveBattler);
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingTormentedMoveInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingTormentedMove;
            limitations++;
        }
    }

    if (gVolatileStructs[gActiveBattler].tauntTimer != 0 && gBattleMoves[move].power == 0)
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveTauntInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveTaunt;
            limitations++;
        }
    }

    if (gVolatileStructs[gActiveBattler].throatChopTimer != 0 && gBattleMoves[move].flags & FLAG_SOUND)
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveThroatChopInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveThroatChop;
            limitations++;
        }
    }

    if (GetImprisonedMovesCount(gActiveBattler, move))
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingImprisonedMoveInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingImprisonedMove;
            limitations++;
        }
    }

    if (IsGravityPreventingMove(move))
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveGravityInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveGravity;
            limitations++;
        }
    }

    if (IsHealBlockPreventingMove(gActiveBattler, move))
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveHealBlockInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveHealBlock;
            limitations++;
        }
    }

    if (IsBelchPreventingMove(gActiveBattler, move))
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedBelchInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedBelch;
            limitations++;
        }
    }
    
    if (move == MOVE_STUFF_CHEEKS && ItemId_GetPocket(gBattleMons[gActiveBattler].item) != POCKET_BERRIES)
    {
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gPalaceSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedBelchInPalace;
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = 1;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedStuffCheeks;
            limitations++;
        }
    }

    //Sleep Clause
    if(IsSleepClauseDisablingMove(gActiveBattler, move)){
        gCurrentMove = move;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = 1;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingSleepClauseNotAllowed;
            limitations++;
        }
    }

    gPotentialItemEffectBattler = gActiveBattler;
    if (HOLD_EFFECT_CHOICE(holdEffect) && *choicedMove != 0 && *choicedMove != 0xFFFF && *choicedMove != move)
    {
        gCurrentMove = *choicedMove;
        gLastUsedItem = gBattleMons[gActiveBattler].item;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveChoiceItem;
            limitations++;
        }
    }
    else if ((holdEffect == HOLD_EFFECT_ASSAULT_VEST || holdEffect == HOLD_EFFECT_TACTICAL_VEST) && IS_MOVE_STATUS(move) && move != MOVE_ME_FIRST)
    {
        gCurrentMove = move;
        gLastUsedItem = gBattleMons[gActiveBattler].item;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveAssaultVest;
            limitations++;
        }
    }
    //Sage Power and Gorilla Tactics
    if ((BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_SAGE_POWER)      ||
         BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_GORILLA_TACTICS) ||
         BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_DISCIPLINE))     &&
		*choicedMove != 0      &&
        *choicedMove != 0xFFFF && 
        *choicedMove != move)
    {
        gCurrentMove = *choicedMove;
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = 1;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveGorillaTactics;//To Change
            limitations++;
        }
    }

    if (gBattleMons[gActiveBattler].pp[moveId] == 0)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingMoveWithNoPP;
            limitations++;
        }
    }

    if (move == gLastChosenMove[gActiveBattler]
        && gBattleMoves[move].everyOtherTurn
        && !GetAbilityState(gActiveBattler, ABILITY_RAMPAGE)
        && !GetAbilityState(gActiveBattler, ABILITY_MASTER_HAND)
        && !GetAbilityState(gActiveBattler, ABILITY_RAGING_GODDESS)
        && !GetAbilityState(gActiveBattler, ABILITY_BERSERKER_RAGE))
    {
        if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
        {
            gRoundStructs[gActiveBattler].palaceUnableToUseMove = TRUE;
        }
        else
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANTSELECTTWICE;
            gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedGeneric;
            limitations++;
        }
    }

    return limitations;
}

u8 CheckMoveLimitations(u8 battlerId, u8 unusableMoves, u8 check)
{
    u8 holdEffect = GetBattlerHoldEffect(battlerId, TRUE);
    u16 *choicedMove = &gBattleStruct->choicedMove[battlerId];
    s32 i;

    gPotentialItemEffectBattler = battlerId;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gBattleMoves[gBattleMons[battlerId].moves[i]].split == SPLIT_STATUS && getMonotypeChampType() == TYPE_FIGHTING && GetBattlerSide(battlerId) == B_SIDE_PLAYER)
            unusableMoves |= 1 << i;
        else if (gBattleMons[battlerId].moves[i] == 0 && check & MOVE_LIMITATION_ZEROMOVE)
            unusableMoves |= 1 << i;
        else if (gBattleMons[battlerId].pp[i] == 0 && check & MOVE_LIMITATION_PP)
            unusableMoves |= 1 << i;
        else if (gBattleMons[battlerId].moves[i] == gVolatileStructs[battlerId].disabledMove && check & MOVE_LIMITATION_DISABLED)
            unusableMoves |= 1 << i;
        else if (gBattleMons[battlerId].moves[i] == gLastMoves[battlerId] && check & MOVE_LIMITATION_TORMENTED && gBattleMons[battlerId].status2 & STATUS2_TORMENT)
            unusableMoves |= 1 << i;
        else if (gVolatileStructs[battlerId].tauntTimer && check & MOVE_LIMITATION_TAUNT && gBattleMoves[gBattleMons[battlerId].moves[i]].power == 0)
            unusableMoves |= 1 << i;
        else if (GetImprisonedMovesCount(battlerId, gBattleMons[battlerId].moves[i]) && check & MOVE_LIMITATION_IMPRISON)
            unusableMoves |= 1 << i;
        else if (gVolatileStructs[battlerId].encoreTimer && gVolatileStructs[battlerId].encoredMove != gBattleMons[battlerId].moves[i])
            unusableMoves |= 1 << i;
        else if (HOLD_EFFECT_CHOICE(holdEffect) && *choicedMove != 0 && *choicedMove != 0xFFFF && *choicedMove != gBattleMons[battlerId].moves[i])
            unusableMoves |= 1 << i;
        else if ((holdEffect == HOLD_EFFECT_ASSAULT_VEST || holdEffect == HOLD_EFFECT_TACTICAL_VEST) && IS_MOVE_STATUS(gBattleMons[battlerId].moves[i]) && gBattleMons[battlerId].moves[i] != MOVE_ME_FIRST)
            unusableMoves |= 1 << i;
        else if (IsGravityPreventingMove(gBattleMons[battlerId].moves[i]))
            unusableMoves |= 1 << i;
        else if (IsHealBlockPreventingMove(battlerId, gBattleMons[battlerId].moves[i]))
            unusableMoves |= 1 << i;
        else if (IsBelchPreventingMove(battlerId, gBattleMons[battlerId].moves[i]))
            unusableMoves |= 1 << i;
        else if (gVolatileStructs[battlerId].throatChopTimer && gBattleMoves[gBattleMons[battlerId].moves[i]].flags & FLAG_SOUND)
            unusableMoves |= 1 << i;
        else if (gBattleMons[battlerId].moves[i] == MOVE_STUFF_CHEEKS && ItemId_GetPocket(gBattleMons[gActiveBattler].item) != POCKET_BERRIES)
            unusableMoves |= 1 << i;
        else if (BATTLER_HAS_ABILITY(battlerId, ABILITY_DISCIPLINE) && *choicedMove != 0 && *choicedMove != 0xFFFF && *choicedMove != gBattleMons[battlerId].moves[i])
            unusableMoves |= 1 << i;
        else if (gBattleMons[battlerId].moves[i] == gLastChosenMove[battlerId]
            && gBattleMoves[gBattleMons[battlerId].moves[i]].everyOtherTurn
            && !GetAbilityState(battlerId, ABILITY_RAMPAGE)
            && !GetAbilityState(battlerId, ABILITY_MASTER_HAND)
            && !GetAbilityState(battlerId, ABILITY_RAGING_GODDESS)
            && !GetAbilityState(battlerId, ABILITY_BERSERKER_RAGE))
            unusableMoves |= 1 << i;
        else if (IsSleepClauseDisablingMove(battlerId, gBattleMons[battlerId].moves[i])){
            unusableMoves |= 1 << i;
        }
        else if ((BATTLER_HAS_ABILITY(battlerId, ABILITY_GORILLA_TACTICS) ||
                  BATTLER_HAS_ABILITY(battlerId, ABILITY_SAGE_POWER))
			&& *choicedMove != 0 && *choicedMove != 0xFFFF && *choicedMove != gBattleMons[battlerId].moves[i])
            unusableMoves |= 1 << i;
    }
    return unusableMoves;
}

bool8 AreAllMovesUnusable(void)
{
    u8 unusable;
    unusable = CheckMoveLimitations(gActiveBattler, 0, 0xFF);

    if (unusable == 0xF) // All moves are unusable.
    {
        gRoundStructs[gActiveBattler].noValidMoves = TRUE;
        gSelectionBattleScripts[gActiveBattler] = BattleScript_NoMovesLeft;
    }
    else
    {
        gRoundStructs[gActiveBattler].noValidMoves = FALSE;
    }

    return (unusable == 0xF);
}

u8 GetImprisonedMovesCount(u8 battlerId, u16 move)
{
    s32 i;
    u8 imprisonedMoves = 0;
    u8 battlerSide = GetBattlerSide(battlerId);

    for (i = 0; i < gBattlersCount; i++)
    {
        if (battlerSide != GetBattlerSide(i) && gStatuses3[i] & STATUS3_IMPRISONED_OTHERS)
        {
            s32 j;
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                if (move == gBattleMons[i].moves[j])
                    break;
            }
            if (j < MAX_MON_MOVES)
                imprisonedMoves++;
        }
    }

    return imprisonedMoves;
}

void RestoreBattlerOriginalTypes(u8 battlerId)
{
    gBattleMons[battlerId].type1 = gBaseStats[gBattleMons[battlerId].species].type1;
    gBattleMons[battlerId].type2 = gBaseStats[gBattleMons[battlerId].species].type2;
}

void TryToApplyMimicry(u8 battlerId, bool8 various)
{
    u32 moveType, move;

    GET_MOVE_TYPE(move, moveType);
    switch (gFieldStatuses)
    {
    case STATUS_FIELD_ELECTRIC_TERRAIN:
        moveType = TYPE_ELECTRIC;
        break;
    case STATUS_FIELD_MISTY_TERRAIN:
        moveType = TYPE_FAIRY;
        break;
    case STATUS_FIELD_GRASSY_TERRAIN:
        moveType = TYPE_GRASS;
        break;
    case STATUS_FIELD_PSYCHIC_TERRAIN:
        moveType = TYPE_PSYCHIC;
        break;
    default:
        moveType = 0;
        break;
    }

    if (moveType != 0 && !IS_BATTLER_OF_TYPE(battlerId, moveType))
    {
        SET_BATTLER_TYPE(battlerId, moveType);
        PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, battlerId, gBattlerPartyIndexes[battlerId])
        PREPARE_TYPE_BUFFER(gBattleTextBuff2, moveType);
        if (!various)
            BattleScriptPushCursorAndCallback(BattleScript_MimicryActivatesEnd3);
    }
}

void TryToRevertMimicry(void)
{
    s32 i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (GetBattlerAbility(i) == ABILITY_MIMICRY)
            RestoreBattlerOriginalTypes(i);
    }
}

static int GetAbilityNumber(int battler, int ability)
{
    int index;
    switch (BattlerHasInnateOrAbility(battler, ability))
    {
        case BATTLER_INNATE:
            return GetBattlerInnateNum(battler, ability) + 1;
        
        case BATTLER_ABILITY:
            return 0;
        
        default:
            return -1;
    }
}

int GetOncePerTurnAbilityCounter(int battler, int ability)
{
    int index = GetAbilityNumber(battler, ability);
    if (index < 0) return -1;

    return gTurnStructs[battler].turnAbilityTriggers[index];
}

void SetOncePerTurnAbilityCounter(int battler, int ability, int value)
{
    int index = GetAbilityNumber(battler, ability);
    if (index < 0) return;

    gTurnStructs[battler].turnAbilityTriggers[index] = value;
}

static int CheckAndSetOncePerTurnAbility(int battler, int ability)
{
    int index = GetAbilityNumber(battler, ability);
    if (index < 0) return FALSE;

    if (!gTurnStructs[battler].turnAbilityTriggers[index])
    {
        gTurnStructs[battler].turnAbilityTriggers[index]++;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

enum
{
    ENDTURN_ORDER,
    ENDTURN_REFLECT,
    ENDTURN_LIGHT_SCREEN,
    ENDTURN_AURORA_VEIL,
    ENDTURN_MIST,
    ENDTURN_LUCKY_CHANT,
    ENDTURN_SAFEGUARD,
    ENDTURN_TAILWIND,
    ENDTURN_WISH,
    ENDTURN_RAIN,
    ENDTURN_SANDSTORM,
    ENDTURN_SUN,
    ENDTURN_HAIL,
    ENDTURN_FOG,
    ENDTURN_GRAVITY,
    ENDTURN_WATER_SPORT,
    ENDTURN_MUD_SPORT,
    ENDTURN_TRICK_ROOM,
    ENDTURN_WONDER_ROOM,
    ENDTURN_MAGIC_ROOM,
    ENDTURN_ELECTRIC_TERRAIN,
    ENDTURN_MISTY_TERRAIN,
    ENDTURN_GRASSY_TERRAIN,
    ENDTURN_PSYCHIC_TERRAIN,
    ENDTURN_ION_DELUGE,
    ENDTURN_FAIRY_LOCK,
    ENDTURN_RETALIATE,
    ENDTURN_RAINBOW,
    ENDTURN_SEA_OF_FIRE,
    ENDTURN_SWAMP,
    ENDTURN_QUASH,
    ENDTURN_SMOKESCREEN,
    ENDTURN_FIELD_COUNT,
};

u8 DoFieldEndTurnEffects(void)
{
    u8 effect = 0;

    for (gBattlerAttacker = 0; gBattlerAttacker < gBattlersCount && gAbsentBattlerFlags & 1 << gBattlerAttacker; gBattlerAttacker++)
    {
    }
    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount && gAbsentBattlerFlags & 1 << gBattlerTarget; gBattlerTarget++)
    {
    }

    do
    {
        s32 i;
        u8 side;

        switch (gBattleStruct->turnCountersTracker)
        {
        case ENDTURN_ORDER:
            for (i = 0; i < gBattlersCount; i++)
            {
                gBattlerByTurnOrder[i] = i;
            }
            for (i = 0; i < gBattlersCount - 1; i++)
            {
                s32 j;
                for (j = i + 1; j < gBattlersCount; j++)
                {
                    if (GetWhoStrikesFirst(gBattlerByTurnOrder[i], gBattlerByTurnOrder[j], 0))
                        SwapTurnOrder(i, j);
                }
            }

            gBattleStruct->turnCountersTracker++;
            gBattleStruct->turnSideTracker = 0;
            // fall through
        case ENDTURN_REFLECT:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].reflectBattlerId;
                if (gSideStatuses[side] & SIDE_STATUS_REFLECT)
                {
                    if (!gSideTimers[side].started.reflect && --gSideTimers[side].reflectTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_REFLECT;
                        BattleScriptExecute(BattleScript_SideStatusWoreOff);
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_REFLECT);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect != 0)
                    break;
            }
            if (effect == 0)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_LIGHT_SCREEN:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].lightscreenBattlerId;
                if (gSideStatuses[side] & SIDE_STATUS_LIGHTSCREEN)
                {
                    if (!gSideTimers[side].started.lightscreen && --gSideTimers[side].lightscreenTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_LIGHTSCREEN;
                        BattleScriptExecute(BattleScript_SideStatusWoreOff);
                        gBattleCommunication[MULTISTRING_CHOOSER] = side;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_LIGHT_SCREEN);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_AURORA_VEIL:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = side;
                //Aurora Veil
                if (gSideStatuses[side] & SIDE_STATUS_AURORA_VEIL)
                {
                    if (!gSideTimers[side].started.auroraVeil && --gSideTimers[side].auroraVeilTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_AURORA_VEIL;
                        BattleScriptExecute(BattleScript_SideStatusWoreOff);
                        gBattleCommunication[MULTISTRING_CHOOSER] = side;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_AURORA_VEIL);
                        effect++;
                    }
                }
                //Spider Web
                if (gSideStatuses[side] & SIDE_STATUS_STICKY_WEB)
                {
                    if (!gSideTimers[side].started.spiderWeb && --gSideTimers[side].spiderWebTimer == 1)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_STICKY_WEB;
                        BattleScriptExecute(BattleScript_SideStatusWoreOff);
                        gBattleCommunication[MULTISTRING_CHOOSER] = side;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_STICKY_WEB);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_MIST:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].mistBattlerId;
                if (gSideTimers[side].mistTimer != 0
                    && !gSideTimers[side].started.mist
                    && --gSideTimers[side].mistTimer == 0)
                {
                    gSideStatuses[side] &= ~SIDE_STATUS_MIST;
                    BattleScriptExecute(BattleScript_SideStatusWoreOff);
                    gBattleCommunication[MULTISTRING_CHOOSER] = side;
                    PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_MIST);
                    effect++;
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_SAFEGUARD:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].safeguardBattlerId;
                if (gSideStatuses[side] & SIDE_STATUS_SAFEGUARD)
                {
                    if (!gSideTimers[side].started.safeguard && --gSideTimers[side].safeguardTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_SAFEGUARD;
                        BattleScriptExecute(BattleScript_SafeguardEnds);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_LUCKY_CHANT:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].luckyChantBattlerId;
                if (gSideStatuses[side] & SIDE_STATUS_LUCKY_CHANT)
                {
                    if (!gSideTimers[side].started.luckyChant && --gSideTimers[side].luckyChantTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_LUCKY_CHANT;
                        BattleScriptExecute(BattleScript_LuckyChantEnds);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_TAILWIND:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].tailwindBattlerId;
                if (gSideStatuses[side] & SIDE_STATUS_TAILWIND)
                {
                    if (!gSideTimers[side].started.tailwind && --gSideTimers[side].tailwindTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_TAILWIND;
                        BattleScriptExecute(BattleScript_TailwindEnds);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_WISH:
            while (gBattleStruct->turnSideTracker < gBattlersCount)
            {
                gActiveBattler = gBattlerByTurnOrder[gBattleStruct->turnSideTracker];
                if (gWishFutureKnock.wishCounter[gActiveBattler] != 0
                 && --gWishFutureKnock.wishCounter[gActiveBattler] == 0
                 && gBattleMons[gActiveBattler].hp != 0)
                {
                    gBattlerTarget = gActiveBattler;
                    BattleScriptExecute(BattleScript_WishComesTrue);
                    effect++;
                }
                gBattleStruct->turnSideTracker++;
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
            }
            break;
        case ENDTURN_RAIN:
            if (gBattleWeather & WEATHER_RAIN_ANY)
            {
                if (!(gBattleWeather & WEATHER_RAIN_PERMANENT)
                 && !(gBattleWeather & WEATHER_RAIN_PRIMAL))
                {
                    if (!gFieldTimers.started.weather && --gWishFutureKnock.weatherDuration == 0)
                    {
                        gBattleWeather &= ~WEATHER_RAIN_TEMPORARY;
                        gBattleWeather &= ~WEATHER_RAIN_DOWNPOUR;
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RAIN_STOPPED;
                    }
                    else if (gBattleWeather & WEATHER_RAIN_DOWNPOUR)
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DOWNPOUR_CONTINUES;
                    else
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RAIN_CONTINUES;
                }
                else if (gBattleWeather & WEATHER_RAIN_DOWNPOUR)
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DOWNPOUR_CONTINUES;
                }
                else
                {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RAIN_CONTINUES;
                }

                BattleScriptExecute(BattleScript_RainContinuesOrEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_SANDSTORM:
            if (gBattleWeather & WEATHER_SANDSTORM_ANY)
            {
                if (!(gBattleWeather & WEATHER_SANDSTORM_PERMANENT) && !gFieldTimers.started.weather && --gWishFutureKnock.weatherDuration == 0)
                {
                    gBattleWeather &= ~WEATHER_SANDSTORM_TEMPORARY;
                    gBattlescriptCurrInstr = BattleScript_SandStormHailEnds;
                }
                else
                {
                    gBattlescriptCurrInstr = BattleScript_DamagingWeatherContinues;
                }

                gBattleScripting.animArg1 = B_ANIM_SANDSTORM_CONTINUES;
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SANDSTORM;
                BattleScriptExecute(gBattlescriptCurrInstr);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_SUN:
            if (gBattleWeather & WEATHER_SUN_ANY)
            {
                if (!(gBattleWeather & WEATHER_SUN_PERMANENT)
                    && !(gBattleWeather & WEATHER_SUN_PRIMAL)
                    && !gFieldTimers.started.weather
                    && --gWishFutureKnock.weatherDuration == 0)
                {
                    gBattleWeather &= ~WEATHER_SUN_TEMPORARY;
                    gBattlescriptCurrInstr = BattleScript_SunlightFaded;
                }
                else
                {
                    gBattlescriptCurrInstr = BattleScript_SunlightContinues;
                }

                BattleScriptExecute(gBattlescriptCurrInstr);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_HAIL:
            if (gBattleWeather & WEATHER_HAIL_ANY)
            {
                if (!(gBattleWeather & WEATHER_HAIL_PERMANENT) && !gFieldTimers.started.weather && --gWishFutureKnock.weatherDuration == 0)
                {
                    gBattleWeather &= ~WEATHER_HAIL_TEMPORARY;
                    gBattlescriptCurrInstr = BattleScript_SandStormHailEnds;
                }
                else
                {
                    gBattlescriptCurrInstr = BattleScript_DamagingWeatherContinues;
                }

                gBattleScripting.animArg1 = B_ANIM_HAIL_CONTINUES;
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_HAIL;
                BattleScriptExecute(gBattlescriptCurrInstr);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_FOG:
            if (gBattleWeather & WEATHER_FOG_TEMPORARY && !gFieldTimers.started.weather && --gWishFutureKnock.weatherDuration == 0)
            {
                gBattleWeather &= ~WEATHER_FOG_ANY;
                BattleScriptExecute(BattleScript_FogEnds);
                effect++;
            }
            else if (gBattleWeather & WEATHER_FOG_ANY)
            {
                BattleScriptExecute(BattleScript_FogContinues);
                effect++;
            }
            else if (!gBattleWeather && gFieldTimers.fogReturnTimer)
            {
                if (gFieldTimers.fogReturnTimer > 50) gBattleWeather = WEATHER_FOG_PERMANENT;
                else
                {
                    gBattleWeather = WEATHER_FOG_TEMPORARY;
                    gWishFutureKnock.weatherDuration = gFieldTimers.fogReturnTimer;
                }
                gFieldTimers.fogReturnTimer = 0;
                BattleScriptExecute(BattleScript_FogReturns);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_TRICK_ROOM:
            if (gFieldStatuses & STATUS_FIELD_TRICK_ROOM && !gFieldTimers.started.trickRoom && --gFieldTimers.trickRoomTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_TRICK_ROOM);
                BattleScriptExecute(BattleScript_TrickRoomEnds);
                effect++;
            }

            if (gFieldStatuses & STATUS_FIELD_INVERSE_ROOM && !gFieldTimers.started.inverseRoom && --gFieldTimers.inverseRoomTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_INVERSE_ROOM);
                BattleScriptExecute(BattleScript_InverseRoomEnds);
                effect++;
            }

            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_WONDER_ROOM:
            if (gFieldStatuses & STATUS_FIELD_WONDER_ROOM && !gFieldTimers.started.wonderRoom && --gFieldTimers.wonderRoomTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_WONDER_ROOM);
                BattleScriptExecute(BattleScript_WonderRoomEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_MAGIC_ROOM:
            if (gFieldStatuses & STATUS_FIELD_MAGIC_ROOM && --gFieldTimers.magicRoomTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_MAGIC_ROOM);
                BattleScriptExecute(BattleScript_MagicRoomEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_ELECTRIC_TERRAIN:
            if (gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN
              && (!(gFieldStatuses & STATUS_FIELD_TERRAIN_PERMANENT) && !gFieldTimers.started.terrain && --gFieldTimers.terrainTimer == 0))
            {
                gFieldStatuses &= ~(STATUS_FIELD_ELECTRIC_TERRAIN | STATUS_FIELD_TERRAIN_PERMANENT);
                TryToRevertMimicry();
                BattleScriptExecute(BattleScript_ElectricTerrainEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_MISTY_TERRAIN:
            if (gFieldStatuses & STATUS_FIELD_MISTY_TERRAIN
              && (!(gFieldStatuses & STATUS_FIELD_TERRAIN_PERMANENT) && !gFieldTimers.started.terrain && --gFieldTimers.terrainTimer == 0))
            {
                gFieldStatuses &= ~(STATUS_FIELD_MISTY_TERRAIN);
                TryToRevertMimicry();
                BattleScriptExecute(BattleScript_MistyTerrainEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_GRASSY_TERRAIN:
            if (gFieldStatuses & STATUS_FIELD_GRASSY_TERRAIN)
            {
                if (!(gFieldStatuses & STATUS_FIELD_TERRAIN_PERMANENT) && !gFieldTimers.started.terrain && --gFieldTimers.terrainTimer == 0)
                {
                    gFieldStatuses &= ~(STATUS_FIELD_GRASSY_TERRAIN);
                    TryToRevertMimicry();
                }
                BattleScriptExecute(BattleScript_GrassyTerrainHeals);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_PSYCHIC_TERRAIN:
            if (gFieldStatuses & STATUS_FIELD_PSYCHIC_TERRAIN
              && (!(gFieldStatuses & STATUS_FIELD_TERRAIN_PERMANENT) && !gFieldTimers.started.terrain && --gFieldTimers.terrainTimer == 0))
            {
                gFieldStatuses &= ~(STATUS_FIELD_PSYCHIC_TERRAIN);
                TryToRevertMimicry();
                BattleScriptExecute(BattleScript_PsychicTerrainEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_WATER_SPORT:
            if (gFieldStatuses & STATUS_FIELD_WATERSPORT && --gFieldTimers.waterSportTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_WATERSPORT);
                BattleScriptExecute(BattleScript_WaterSportEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_MUD_SPORT:
            if (gFieldStatuses & STATUS_FIELD_MUDSPORT && !gFieldTimers.started.mudSport && --gFieldTimers.mudSportTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_MUDSPORT);
                BattleScriptExecute(BattleScript_MudSportEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_GRAVITY:
            if (gFieldStatuses & STATUS_FIELD_GRAVITY && !gFieldTimers.started.gravity && --gFieldTimers.gravityTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_GRAVITY);
                BattleScriptExecute(BattleScript_GravityEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_ION_DELUGE:
            gFieldStatuses &= ~(STATUS_FIELD_ION_DELUGE);
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_FAIRY_LOCK:
            if (gFieldStatuses & STATUS_FIELD_FAIRY_LOCK && !gFieldTimers.started.fairyLock && --gFieldTimers.fairyLockTimer == 0)
            {
                gFieldStatuses &= ~(STATUS_FIELD_FAIRY_LOCK);
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_RETALIATE:
            if (gSideTimers[B_SIDE_PLAYER].retaliateTimer > 0)
                gSideTimers[B_SIDE_PLAYER].retaliateTimer--;
            if (gSideTimers[B_SIDE_OPPONENT].retaliateTimer > 0)
                gSideTimers[B_SIDE_OPPONENT].retaliateTimer--;
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_RAINBOW:
            while (gBattleStruct->turnSideTracker < 2)
            {
                u8 side = gBattleStruct->turnSideTracker;
                if (gSideTimers[side].rainbowTimer)
                {
                    for (gBattlerAttacker = 0; gBattlerAttacker < gBattlersCount; gBattlerAttacker++)
                    {
                        if (GetBattlerSide(gBattlerAttacker) == side)
                            break;
                    }

                    if (gSideTimers[side].rainbowTimer && !gSideTimers[side].started.rainbow && --gSideTimers[side].rainbowTimer == 0)
                    {
                        BattleScriptExecute(BattleScript_RainbowDisappeared);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect != 0)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_SEA_OF_FIRE:
            while (gBattleStruct->turnSideTracker < 2)
            {
                u8 side = gBattleStruct->turnSideTracker;

                if (gSideTimers[side].fireSeaTimer)
                {
                    for (gBattlerAttacker = 0; gBattlerAttacker < gBattlersCount; gBattlerAttacker++)
                    {
                        if (GetBattlerSide(gBattlerAttacker) == side)
                            break;
                    }

                    if (gSideTimers[side].fireSeaTimer > 0 && !gSideTimers[side].started.fireSea && --gSideTimers[side].fireSeaTimer == 0)
                    {
                        BattleScriptExecute(BattleScript_TheSeaOfFireDisappeared);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect != 0)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_SWAMP:
            while (gBattleStruct->turnSideTracker < 2)
            {
                u8 side = gBattleStruct->turnSideTracker;
                if (gSideTimers[side].swampTimer)
                {
                    for (gBattlerAttacker = 0; gBattlerAttacker < gBattlersCount; gBattlerAttacker++)
                    {
                        if (GetBattlerSide(gBattlerAttacker) == side)
                            break;
                    }

                    if (gSideTimers[side].swampTimer && !gSideTimers[side].started.swamp && --gSideTimers[side].swampTimer == 0)
                    {
                        BattleScriptExecute(BattleScript_TheSwampDisappeared);
                        effect++;
                    }
                }
                gBattleStruct->turnSideTracker++;
                if (effect != 0)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_QUASH:
            if (gFieldTimers.quashTimer && !gFieldTimers.started.quash && --gFieldTimers.quashTimer == 0)
            {
                BattleScriptExecute(BattleScript_QuashEnds);
                effect++;
            }
            gBattleStruct->turnCountersTracker++;
            break;
        case ENDTURN_SMOKESCREEN:
            while (gBattleStruct->turnSideTracker < 2)
            {
                side = gBattleStruct->turnSideTracker++;
                gActiveBattler = gBattlerAttacker = gSideTimers[side].smokescreenBattler;
                if (gSideTimers[side].smokescreenTimer)
                {
                    if (!gSideTimers[side].started.smokescreen && --gSideTimers[side].smokescreenTimer == 0)
                    {
                        gSideStatuses[side] &= ~SIDE_STATUS_SMOKESCREEN;
                        BattleScriptExecute(BattleScript_SideStatusWoreOff);
                        gBattleCommunication[MULTISTRING_CHOOSER] = side;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_SMOKESCREEN);
                        effect++;
                    }
                }
                if (effect)
                    break;
            }
            if (!effect)
            {
                gBattleStruct->turnCountersTracker++;
                gBattleStruct->turnSideTracker = 0;
            }
            break;
        case ENDTURN_FIELD_COUNT:
            effect++;
            break;
        }
    } while (effect == 0);

    return (gBattleMainFunc != BattleTurnPassed);
}

enum
{
    ENDTURN_INGRAIN,
    ENDTURN_AQUA_RING,
    ENDTURN_ABILITIES,
    ENDTURN_ITEMS1,
    ENDTURN_LEECH_SEED,
    ENDTURN_POISON,
    ENDTURN_BAD_POISON,
    ENDTURN_BURN,
    ENDTURN_FROSTBITE,
    ENDTURN_BLEED,
    ENDTURN_NIGHTMARES,
    ENDTURN_CURSE,
    ENDTURN_SALT_CURE,
    ENDTURN_SYRUP,
    ENDTURN_WRAP,
    ENDTURN_OCTOLOCK,
    ENDTURN_UPROAR,
    ENDTURN_THRASH,
    ENDTURN_FLINCH,
    ENDTURN_DISABLE,
    ENDTURN_ENCORE,
    ENDTURN_MAGNET_RISE,
    ENDTURN_TELEKINESIS,
    ENDTURN_HEALBLOCK,
    ENDTURN_EMBARGO,
    ENDTURN_LOCK_ON,
    ENDTURN_GHASTLY_ECHO,
    ENDTURN_COILED_UP,
    ENDTURN_LASER_FOCUS,
    ENDTURN_TAUNT,
    ENDTURN_YAWN,
    ENDTURN_ITEMS2,
    ENDTURN_ORBS,
    ENDTURN_ROOST,
    ENDTURN_ELECTRIFY,
    ENDTURN_POWDER,
    ENDTURN_THROAT_CHOP,
    ENDTURN_SLOW_START,
    ENDTURN_PLASMA_FISTS,
    ENDTURN_TOXIC_WASTE_DAMAGE,
    ENDTURN_SEA_OF_FIRE_DAMAGE,
    ENDTURN_COMMANDER,
    ENDTURN_PARASITIC_SPORES_DAMAGE,
    ENDTURN_FUNERAL_PYRE_DAMAGE,
    ENDTURN_GENERIC_BATTLER_TIMERS,
    ENDTURN_BATTLER_COUNT,
};

// Ingrain, Leech Seed, Strength Sap and Aqua Ring
s32 GetDrainedBigRootHp(u32 battler, s32 hp)
{
    if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BIG_ROOT)
        hp = (hp * 3) / 2; // Buff Big Root's additional healing from 30% to 50%

    if (BATTLER_HAS_ABILITY(battler, ABILITY_ABSORBANT))
        gBattleMoveDamage = (gBattleMoveDamage * 3) / 2; // Buff Absorbant additional healing from 30% to 50%

    if (hp == 0)
        hp = 1;

    return hp * -1;
}

#define BATTLER_HAS_MAGIC_GUARD(battlerId) IsMagicGuardProtected(battlerId)

#define MAGIC_GUARD_CHECK \
if (IsMagicGuardProtected(gActiveBattler)) \
{\
    RecordAbilityBattle(gActiveBattler, ability);\
    gBattleStruct->turnEffectsTracker++;\
            break;\
}


#define TOXIC_BOOST_CHECK \
if (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_TOXIC_BOOST, ability)) \
{\
    RecordAbilityBattle(gActiveBattler, ability);\
    gBattleStruct->turnEffectsTracker++;\
            break;\
}

#define FLARE_BOOST_CHECK \
if (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_FLARE_BOOST, ability)) \
{\
    RecordAbilityBattle(gActiveBattler, ability);\
    gBattleStruct->turnEffectsTracker++;\
            break;\
}



u8 DoBattlerEndTurnEffects(void)
{
    u32 ability, i, effect = 0;

    if (AbilityBattleEffects(ABILITYEFFECT_COPY_STATS, 0, 0, 0, 0))
    {
        BattleScriptExecute(gBattlescriptCurrInstr);
        return TRUE;
    }

    gHitMarker |= (HITMARKER_GRUDGE | HITMARKER_SKIP_DMG_TRACK);
    while (gBattleStruct->turnEffectsBattlerId < gBattlersCount && gBattleStruct->turnEffectsTracker <= ENDTURN_BATTLER_COUNT)
    {
        gActiveBattler = gBattlerAttacker = gBattlerByTurnOrder[gBattleStruct->turnEffectsBattlerId];
        if (gAbsentBattlerFlags & 1 << gActiveBattler)
        {
            gBattleStruct->turnEffectsBattlerId++;
            continue;
        }

        ability = GetBattlerAbility(gActiveBattler);
        switch (gBattleStruct->turnEffectsTracker)
        {
        case ENDTURN_INGRAIN:  // ingrain
            if ((gStatuses3[gActiveBattler] & STATUS3_ROOTED)
             && !BATTLER_MAX_HP(gActiveBattler)
             && !BATTLER_HEALING_BLOCKED(gActiveBattler)
             && gBattleMons[gActiveBattler].hp != 0)
            {
                gBattleMoveDamage = GetDrainedBigRootHp(gActiveBattler, gBattleMons[gActiveBattler].maxHP / 8);
                BattleScriptExecute(BattleScript_IngrainTurnHeal);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_AQUA_RING:  // aqua ring
            if ((gStatuses3[gActiveBattler] & STATUS3_AQUA_RING)
             && !BATTLER_MAX_HP(gActiveBattler)
             && !BATTLER_HEALING_BLOCKED(gActiveBattler)
             && gBattleMons[gActiveBattler].hp != 0)
            {
                gBattleMoveDamage = GetDrainedBigRootHp(gActiveBattler, gBattleMons[gActiveBattler].maxHP / 16);
                BattleScriptExecute(BattleScript_AquaRingHeal);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_SYRUP:
            if (gVolatileStructs[gActiveBattler].syrupTimer)
            {
                gVolatileStructs[gActiveBattler].syrupTimer--;
                BattleScriptExecute(BattleScript_SyrupDropsSpeed);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ABILITIES:  // end turn abilities
            {
            u16 move = gLastResultingMoves[gActiveBattler];
            if (move == 0xFFFF) move = 0;
            if (AbilityBattleEffects(ABILITYEFFECT_ENDTURN, gActiveBattler, 0, 0, move))
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ITEMS1:  // item effects
            if (ItemBattleEffects(1, gActiveBattler, FALSE))
                effect++;
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ITEMS2:  // item effects again
            if (ItemBattleEffects(1, gActiveBattler, TRUE))
                effect++;
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ORBS:
            if (ItemBattleEffects(ITEMEFFECT_ORBS, gActiveBattler, FALSE))
                effect++;
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_LEECH_SEED:  // leech seed
            if (gStatuses3[gActiveBattler] & STATUS3_LEECHSEED
                && IsBattlerAlive(gActiveBattler)
                && IsBattlerAlive(gStatuses3[gActiveBattler] & STATUS3_LEECHSEED_BATTLER))
            {
                MAGIC_GUARD_CHECK;

                gBattlerTarget = gStatuses3[gActiveBattler] & STATUS3_LEECHSEED_BATTLER; // Notice gBattlerTarget is actually the HP receiver.
                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                gBattleScripting.animArg1 = gBattlerTarget;
                gBattleScripting.animArg2 = gBattlerAttacker;
                BattleScriptExecute(BattleScript_LeechSeedTurnDrain);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_TOXIC_WASTE_DAMAGE:
            {
            int triggerAbility = 0;
            int source;
            if ((source = IsAbilityOnField(ABILITY_TOXIC_SPILL))) triggerAbility = ABILITY_TOXIC_SPILL;
            else if ((source = IsAbilityOnField(ABILITY_TRASH_HEAP))) triggerAbility = ABILITY_TRASH_HEAP;
            else if (getMonotypeChampType() == TYPE_POISON && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) triggerAbility = TRUE;
            if (triggerAbility && gBattleMons[gActiveBattler].hp != 0 && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_POISON))
            {
                MAGIC_GUARD_CHECK;
                if(!BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_POISON_HEAL, ability))
				    TOXIC_BOOST_CHECK;

                if (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_POISON_HEAL, ability))
                {
                    if (!BATTLER_MAX_HP(gActiveBattler) && !BATTLER_HEALING_BLOCKED(gActiveBattler))
                    {
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                        if (gBattleMoveDamage == 0)
                            gBattleMoveDamage = 1;
                        gBattleMoveDamage *= -1;
                        BattleScriptExecute(BattleScript_PoisonHealActivates);
                        effect++;
                    }
                }
                else
                {
                    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    BattleScriptExecute(BattleScript_ToxicWasteTurnDmg);
                    if (triggerAbility != TRUE)
                    {
                        BattleScriptCall(BattleScript_AbilityPopUp);
                        gBattlerAbility = source - 1;
                    }
                    effect++;
                }
            }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_POISON:  // poison
            if ((gBattleMons[gActiveBattler].status1 & STATUS1_POISON)
                && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                if(ability != ABILITY_POISON_HEAL && !BattlerHasInnate(gActiveBattler, ABILITY_POISON_HEAL))
				    TOXIC_BOOST_CHECK;

                if (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_POISON_HEAL, ability))
                {
                    if (!BATTLER_MAX_HP(gActiveBattler) && !BATTLER_HEALING_BLOCKED(gActiveBattler))
                    {
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                        if (gBattleMoveDamage == 0)
                            gBattleMoveDamage = 1;
                        gBattleMoveDamage *= -1;
                        BattleScriptExecute(BattleScript_PoisonHealActivates);
                        effect++;
                    }
                }
                else
                {
                    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    BattleScriptExecute(BattleScript_PoisonTurnDmg);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_BAD_POISON:  // toxic poison
            if ((gBattleMons[gActiveBattler].status1 & STATUS1_TOXIC_POISON)
                && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                if(ability != ABILITY_POISON_HEAL && !BattlerHasInnate(gActiveBattler, ABILITY_POISON_HEAL))
				    TOXIC_BOOST_CHECK;

                if (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_POISON_HEAL, ability))
                {
                    if (!BATTLER_MAX_HP(gActiveBattler) && !BATTLER_HEALING_BLOCKED(gActiveBattler))
                    {
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                        if (gBattleMoveDamage == 0)
                            gBattleMoveDamage = 1;
                        gBattleMoveDamage *= -1;
                        BattleScriptExecute(BattleScript_PoisonHealActivates);
                        effect++;
                    }
                }
                else
                {
                    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 16;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    if ((gBattleMons[gActiveBattler].status1 & STATUS1_TOXIC_COUNTER) != STATUS1_TOXIC_TURN(15)) // not 16 turns
                        gBattleMons[gActiveBattler].status1 += STATUS1_TOXIC_TURN(1);
                    gBattleMoveDamage *= (gBattleMons[gActiveBattler].status1 & STATUS1_TOXIC_COUNTER) >> 8;
                    BattleScriptExecute(BattleScript_PoisonTurnDmg);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_SEA_OF_FIRE_DAMAGE:
            if (IsBattlerAlive(gActiveBattler) && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_FIRE) && gSideTimers[GetBattlerSide(gActiveBattler)].fireSeaTimer)
            {
                MAGIC_GUARD_CHECK;
				FLARE_BOOST_CHECK;

                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BtlController_EmitStatusAnimation(0, FALSE, STATUS1_BURN);
                MarkBattlerForControllerExec(gActiveBattler);
                BattleScriptExecute(BattleScript_HurtByTheSeaOfFire);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_COMMANDER:
            if (IsBattlerAlive(gActiveBattler) && GetAbilityState(gActiveBattler, ABILITY_COMMANDER) == COMMANDER_NEEDS_CANCELLING)
            {
                SetAbilityState(gActiveBattler, ABILITY_COMMANDER, COMMANDER_NOT_ACTIVE);
                gStatuses3[gActiveBattler] &= ~STATUS3_SEMI_INVULNERABLE;
                gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_COMMANDER;
                gStackBattler1 = gActiveBattler;
                BattleScriptExecute(BattleScript_CommanderEndsEnd2);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_PARASITIC_SPORES_DAMAGE:
            if (IsBattlerAlive(gActiveBattler) && gVolatileStructs[gActiveBattler].parasiticSpores && !IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_GHOST))
            {
                MAGIC_GUARD_CHECK;

                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_ParasiticSporesDamage);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_FUNERAL_PYRE_DAMAGE:
            {
            int source;
            if (IsBattlerAlive(gActiveBattler) && (source = IsAbilityOnField(ABILITY_FUNERAL_PYRE))
                && !(IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_GHOST) || IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_DARK)))
            {
                MAGIC_GUARD_CHECK;

                gBattleScripting.abilityPopupOverwrite = ABILITY_FUNERAL_PYRE;
                gBattleScripting.battlerPopupOverwrite = source;
                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 4;
                gBattleMoveDamage = max(gBattleMoveDamage, 1);
                BattleScriptExecute(BattleScript_FuneralPyreDamage);
                effect++;
            }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_BURN:  // burn
            if ((gBattleMons[gActiveBattler].status1 & STATUS1_BURN) && gBattleMons[gActiveBattler].hp != 0 &&
               ability != ABILITY_HEATPROOF &&
               !BattlerHasInnate(gActiveBattler, ABILITY_HEATPROOF))
            {
                MAGIC_GUARD_CHECK;
				FLARE_BOOST_CHECK;
			
                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / (B_BURN_DAMAGE >= GEN_7 ? 16 : 8);
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_BurnTurnDmg);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_FROSTBITE:  // frostbite
            if ((gBattleMons[gActiveBattler].status1 & STATUS1_FROSTBITE)
                && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                #if B_BURN_DAMAGE >= GEN_7
                    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 16;
                #else
                    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 8;
                #endif
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_FrostbiteTurnDmg);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_BLEED: // bleed
            if ((gBattleMons[gActiveBattler].status1 & STATUS1_BLEED || IsBloodStainAffected(gActiveBattler))
                && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                gBattleMoveDamage = BLEED_DAMAGE(gBattleMons[gActiveBattler].maxHP);
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_BleedTurnDmg);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_NIGHTMARES:  // spooky nightmares
            if ((gBattleMons[gActiveBattler].status2 & STATUS2_NIGHTMARE)
                && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                // R/S does not perform this sleep check, which causes the nightmare effect to
                // persist even after the affected Pokemon has been awakened by Shed Skin.
                if (gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)
                {
                    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 4;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    BattleScriptExecute(BattleScript_NightmareTurnDmg);
                    effect++;
                }
                else
                {
                    gBattleMons[gActiveBattler].status2 &= ~STATUS2_NIGHTMARE;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_CURSE:  // curse
            if (((gBattleMons[gActiveBattler].status2 & STATUS2_CURSED) || IsBattlerCursed(gActiveBattler))
                && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 4;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_CurseTurnDmg);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_SALT_CURE:
            if (gStatuses4[gActiveBattler] & STATUS4_SALT_CURE && gBattleMons[gActiveBattler].hp != 0)
            {
                MAGIC_GUARD_CHECK;
                gBattleMoveDamage = (IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_WATER) || IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_WATER)) ?
                                        gBattleMons[gActiveBattler].maxHP / 4 : gBattleMons[gActiveBattler].maxHP / 8;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_CurseTurnDmg);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_WRAP:  // wrap
            if(getMonotypeChampType() == TYPE_BUG && gBattleMons[gActiveBattler].hp != 0 && GET_BATTLER_SIDE(gActiveBattler) == B_SIDE_PLAYER){
                u16 trappedMove = MOVE_INFESTATION;
                MAGIC_GUARD_CHECK;
                gBattleScripting.animArg1 = trappedMove;
                gBattleScripting.animArg2 = trappedMove >> 8;
                PREPARE_MOVE_BUFFER(gBattleTextBuff1, trappedMove);
                gBattlescriptCurrInstr = BattleScript_WrapTurnDmg;

                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / ((B_BINDING_DAMAGE >= GEN_6) ? 8 : 16);

                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;

                BattleScriptExecute(gBattlescriptCurrInstr);
                effect++;
            }
            else if ((gBattleMons[gActiveBattler].status2 & STATUS2_WRAPPED) && gBattleMons[gActiveBattler].hp != 0)
            {
                if (--gVolatileStructs[gActiveBattler].wrapTurns != 0)  // damaged by wrap
                {
                    MAGIC_GUARD_CHECK;

                    gBattleScripting.animArg1 = gBattleStruct->wrappedMove[gActiveBattler];
                    gBattleScripting.animArg2 = gBattleStruct->wrappedMove[gActiveBattler] >> 8;
                    if(gBattleMoves[gBattleStruct->wrappedMove[gActiveBattler]].effect == EFFECT_TRAP){
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleStruct->wrappedMove[gActiveBattler]);
                    }
                    else{
                        gLastUsedAbility = ABILITY_GRIP_PINCER;
                        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, gLastUsedAbility);
                    }
                    gBattlescriptCurrInstr = BattleScript_WrapTurnDmg;

                    if (BATTLER_HAS_ABILITY(gBattleStruct->wrappedBy[gActiveBattler], ABILITY_GRAPPLER))
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / ((B_BINDING_DAMAGE >= GEN_6) ? 6 : 8);
                    else if (GetBattlerHoldEffect(gBattleStruct->wrappedBy[gActiveBattler], TRUE) == HOLD_EFFECT_BINDING_BAND)
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / ((B_BINDING_DAMAGE >= GEN_6) ? 6 : 8);
                    else
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / ((B_BINDING_DAMAGE >= GEN_6) ? 8 : 16);

                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                }
                else  // broke free
                {
                    gBattleMons[gActiveBattler].status2 &= ~(STATUS2_WRAPPED);
                    PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleStruct->wrappedMove[gActiveBattler]);
                    gBattlescriptCurrInstr = BattleScript_WrapEnds;
                }
                BattleScriptExecute(gBattlescriptCurrInstr);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_OCTOLOCK:
            if (gVolatileStructs[gActiveBattler].octolock 
             && !(BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_CLEAR_BODY)
                || BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_FULL_METAL_BODY)
                || GetBattlerHoldEffect(gActiveBattler, TRUE) == HOLD_EFFECT_CLEAR_AMULET))
            {
                gBattlerTarget = gActiveBattler;
                BattleScriptExecute(BattleScript_OctolockEndTurn);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_UPROAR:  // uproar
            if (gBattleMons[gActiveBattler].status2 & STATUS2_UPROAR)
            {
                for (gBattlerAttacker = 0; gBattlerAttacker < gBattlersCount; gBattlerAttacker++)
                {
                    if ((gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP)
                     && !IsSoundproof(gBattlerAttacker))
                    {
                        gBattleMons[gBattlerAttacker].status1 &= ~(STATUS1_SLEEP);
                        gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_NIGHTMARE);
                        BattleScriptExecute(BattleScript_MonWokeUpInUproar);
                        gActiveBattler = gBattlerAttacker;
                        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                        MarkBattlerForControllerExec(gActiveBattler);
                        break;
                    }
                }
                if (gBattlerAttacker != gBattlersCount)
                {
                    effect = 2;  // a pokemon was awaken
                    break;
                }
                else
                {
                    gBattlerAttacker = gActiveBattler;
                    gBattleMons[gActiveBattler].status2 -= STATUS2_UPROAR_TURN(1);  // uproar timer goes down
                    if (WasUnableToUseMove(gActiveBattler))
                    {
                        CancelMultiTurnMoves(gActiveBattler);
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_ENDS;
                    }
                    else if (gBattleMons[gActiveBattler].status2 & STATUS2_UPROAR)
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_CONTINUES;
                        gBattleMons[gActiveBattler].status2 |= STATUS2_MULTIPLETURNS;
                    }
                    else
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_ENDS;
                        CancelMultiTurnMoves(gActiveBattler);
                    }
                    BattleScriptExecute(BattleScript_PrintUproarOverTurns);
                    effect = 1;
                }
            }
            if (effect != 2)
                gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_THRASH:  // thrash
            if (gBattleMons[gActiveBattler].status2 & STATUS2_LOCK_CONFUSE)
            {
                gBattleMons[gActiveBattler].status2 -= STATUS2_LOCK_CONFUSE_TURN(1);
                if (WasUnableToUseMove(gActiveBattler))
                    CancelMultiTurnMoves(gActiveBattler);
                else if (!(gBattleMons[gActiveBattler].status2 & STATUS2_LOCK_CONFUSE)
                 && (gBattleMons[gActiveBattler].status2 & STATUS2_MULTIPLETURNS))
                {
                    gBattleMons[gActiveBattler].status2 &= ~(STATUS2_MULTIPLETURNS);
                    if (!(gBattleMons[gActiveBattler].status2 & STATUS2_CONFUSION))
                    {
                        gBattleScripting.moveEffect = MOVE_EFFECT_CONFUSION | MOVE_EFFECT_AFFECTS_USER;
                        SetMoveEffect(TRUE, 0);
                        if (gBattleMons[gActiveBattler].status2 & STATUS2_CONFUSION)
                            BattleScriptExecute(BattleScript_ThrashConfuses);
                        effect++;
                    }
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_FLINCH:  // reset flinch
            gBattleMons[gActiveBattler].status2 &= ~(STATUS2_FLINCHED);
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_DISABLE:  // disable
            if (gVolatileStructs[gActiveBattler].disableTimer != 0)
            {
                for (i = 0; i < MAX_MON_MOVES; i++)
                {
                    if (gVolatileStructs[gActiveBattler].disabledMove == gBattleMons[gActiveBattler].moves[i])
                        break;
                }
                if (i == MAX_MON_MOVES)  // pokemon does not have the disabled move anymore
                {
                    gVolatileStructs[gActiveBattler].disabledMove = 0;
                    gVolatileStructs[gActiveBattler].disableTimer = 0;
                }
                else if (--gVolatileStructs[gActiveBattler].disableTimer == 0)  // disable ends
                {
                    gVolatileStructs[gActiveBattler].disabledMove = 0;
                    BattleScriptExecute(BattleScript_DisabledNoMore);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ENCORE:  // encore
            if (gVolatileStructs[gActiveBattler].encoreTimer != 0)
            {
                if (gBattleMons[gActiveBattler].moves[gVolatileStructs[gActiveBattler].encoredMovePos] != gVolatileStructs[gActiveBattler].encoredMove)  // pokemon does not have the encored move anymore
                {
                    gVolatileStructs[gActiveBattler].encoredMove = 0;
                    gVolatileStructs[gActiveBattler].encoreTimer = 0;
                }
                else if (--gVolatileStructs[gActiveBattler].encoreTimer == 0
                 || gBattleMons[gActiveBattler].pp[gVolatileStructs[gActiveBattler].encoredMovePos] == 0)
                {
                    gVolatileStructs[gActiveBattler].encoredMove = 0;
                    gVolatileStructs[gActiveBattler].encoreTimer = 0;
                    BattleScriptExecute(BattleScript_EncoredNoMore);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_LOCK_ON:  // lock-on decrement
            if (gStatuses3[gActiveBattler] & STATUS3_ALWAYS_HITS)
                gStatuses3[gActiveBattler] -= STATUS3_ALWAYS_HITS_TURN(1);
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_GHASTLY_ECHO:
            if (gVolatileStructs[gActiveBattler].ghastlyEchoTimer && --gVolatileStructs[gActiveBattler].ghastlyEchoTimer == 0)
                gStatuses4[gActiveBattler] &= ~STATUS4_GHASTLY_ECHO;
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_COILED_UP:
            if((gStatuses4[gActiveBattler] & STATUS4_COILED) && (gBattleMoves[gLastMoves[gActiveBattler]].flags & FLAG_STRONG_JAW_BOOST))
                gStatuses4[gActiveBattler] &= ~(STATUS4_COILED);
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_TAUNT:  // taunt
            if (gVolatileStructs[gActiveBattler].tauntTimer && --gVolatileStructs[gActiveBattler].tauntTimer == 0)
            {
                BattleScriptExecute(BattleScript_BufferEndTurn);
                PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_TAUNT);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_YAWN:  // yawn
            if (gStatuses3[gActiveBattler] & STATUS3_YAWN)
            {
                gStatuses3[gActiveBattler] -= STATUS3_YAWN_TURN(1);
                if (!(gStatuses3[gActiveBattler] & STATUS3_YAWN) 
                 && !(gBattleMons[gActiveBattler].status1 & STATUS1_ANY)
                 && !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_VITAL_SPIRIT)
                 && !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_INSOMNIA)
				 && !UproarWakeUpCheck(gActiveBattler)
                 && !IsLeafGuardProtected(gActiveBattler))
                {
                    CancelMultiTurnMoves(gActiveBattler);
                    gEffectBattler = gActiveBattler;
                    if (IsBattlerTerrainAffected(gActiveBattler, STATUS_FIELD_ELECTRIC_TERRAIN))
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINPREVENTS_ELECTRIC;
                        BattleScriptExecute(BattleScript_TerrainPreventsEnd2);
                    }
                    else if (IsBattlerTerrainAffected(gActiveBattler, STATUS_FIELD_MISTY_TERRAIN))
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINPREVENTS_MISTY;
                        BattleScriptExecute(BattleScript_TerrainPreventsEnd2);
                    }
                    else
                    {
                        gBattleMons[gActiveBattler].status1 |= (Random() & 3) + 2;
                        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                        MarkBattlerForControllerExec(gActiveBattler);
                        BattleScriptExecute(BattleScript_YawnMakesAsleep);
                    }
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_LASER_FOCUS:
            if (gStatuses3[gActiveBattler] & STATUS3_LASER_FOCUS)
            {
                if (gVolatileStructs[gActiveBattler].laserFocusTimer == 0 || --gVolatileStructs[gActiveBattler].laserFocusTimer == 0)
                    gStatuses3[gActiveBattler] &= ~(STATUS3_LASER_FOCUS);
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_EMBARGO:
            if (gStatuses3[gActiveBattler] & STATUS3_EMBARGO)
            {
                if (gVolatileStructs[gActiveBattler].embargoTimer == 0 || --gVolatileStructs[gActiveBattler].embargoTimer == 0)
                {
                    gStatuses3[gActiveBattler] &= ~(STATUS3_EMBARGO);
                    BattleScriptExecute(BattleScript_EmbargoEndTurn);
                    effect++;
                }
            }

            gVolatileStructs[gActiveBattler].substituteDestroyedThisTurn = FALSE;

            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_MAGNET_RISE:
            if (gStatuses3[gActiveBattler] & STATUS3_MAGNET_RISE)
            {
                if (gVolatileStructs[gActiveBattler].magnetRiseTimer == 0 || --gVolatileStructs[gActiveBattler].magnetRiseTimer == 0)
                {
                    gStatuses3[gActiveBattler] &= ~(STATUS3_MAGNET_RISE);
                    BattleScriptExecute(BattleScript_BufferEndTurn);
                    PREPARE_STRING_BUFFER(gBattleTextBuff1, STRINGID_ELECTROMAGNETISM);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_TELEKINESIS:
            if (gStatuses3[gActiveBattler] & STATUS3_TELEKINESIS)
            {
                if (gVolatileStructs[gActiveBattler].telekinesisTimer == 0 || --gVolatileStructs[gActiveBattler].telekinesisTimer == 0)
                {
                    gStatuses3[gActiveBattler] &= ~(STATUS3_TELEKINESIS);
                    BattleScriptExecute(BattleScript_TelekinesisEndTurn);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_HEALBLOCK:
            if (gStatuses3[gActiveBattler] & STATUS3_HEAL_BLOCK)
            {
                if (gVolatileStructs[gActiveBattler].healBlockTimer == 0 || --gVolatileStructs[gActiveBattler].healBlockTimer == 0)
                {
                    gStatuses3[gActiveBattler] &= ~(STATUS3_HEAL_BLOCK);
                    BattleScriptExecute(BattleScript_BufferEndTurn);
                    PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_HEAL_BLOCK);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ROOST: // Return flying type.
            if (gBattleResources->flags->flags[gActiveBattler] & RESOURCE_FLAG_ROOST)
            {
                gBattleResources->flags->flags[gActiveBattler] &= ~(RESOURCE_FLAG_ROOST);
                gBattleMons[gActiveBattler].type1 = gBattleStruct->roostTypes[gActiveBattler][0];
                gBattleMons[gActiveBattler].type2 = gBattleStruct->roostTypes[gActiveBattler][1];
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_ELECTRIFY:
            gStatuses4[gActiveBattler] &= ~(STATUS4_ELECTRIFIED);
            gBattleStruct->turnEffectsTracker++;
        case ENDTURN_POWDER:
            gBattleMons[gActiveBattler].status2 &= ~(STATUS2_POWDER);
            gBattleStruct->turnEffectsTracker++;
        case ENDTURN_THROAT_CHOP:
            if (gVolatileStructs[gActiveBattler].throatChopTimer && --gVolatileStructs[gActiveBattler].throatChopTimer == 0)
            {
                BattleScriptExecute(BattleScript_ThroatChopEndTurn);
                effect++;
            }
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_SLOW_START:
            if(gVolatileStructs[gActiveBattler].slowStartTimer){
                gVolatileStructs[gActiveBattler].slowStartTimer--;

                if (gVolatileStructs[gActiveBattler].slowStartTimer == 0
                && (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_SLOW_START, ability)))
                {
                    BattleScriptExecute(BattleScript_SlowStartEnds);
                    effect++;
                }
                else if (gVolatileStructs[gActiveBattler].slowStartTimer == 0
                    && (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_LETHARGY, ability)))
                {
                    BattleScriptExecute(BattleScript_LethargyEnds);
                    effect++;
                }
            }
            
            if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_DISCIPLINE)          &&
                !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_GORILLA_TACTICS)    &&
                !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_SAGE_POWER)         &&
                !HOLD_EFFECT_CHOICE(GetBattlerHoldEffect(gActiveBattler, FALSE)) &&
                gVolatileStructs[gActiveBattler].disciplineCounter)
            {
                gVolatileStructs[gActiveBattler].disciplineCounter--;
                if (gVolatileStructs[gActiveBattler].disciplineCounter == 0)
                {
                    gBattleStruct->choicedMove[gActiveBattler] = MOVE_NONE;
                    BattleScriptExecute(BattleScript_DisciplineLockEnds);
                    effect++;
                }
            }
            gBattleStruct->turnEffectsTracker++;

            break;
        case ENDTURN_PLASMA_FISTS:
            gStatuses4[gActiveBattler] &= ~(STATUS4_PLASMA_FISTS);
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_GENERIC_BATTLER_TIMERS:
            if (!gVolatileStructs[gActiveBattler].started.fear)
            {
                gStatuses4[gActiveBattler] &= ~STATUS4_FEAR;
                gVolatileStructs[gActiveBattler].fear = FALSE;
            }
            #define CLEAR_ONE_TURN(flag) if (!gVolatileStructs[gActiveBattler].started.flag) gVolatileStructs[gActiveBattler].flag = FALSE;
            CLEAR_ONE_TURN(rapidResponse)
            CLEAR_ONE_TURN(readiedAction)
            CLEAR_ONE_TURN(showdownMode)
            CLEAR_ONE_TURN(violentRush)
            CLEAR_ONE_TURN(onTheProwl)
            #undef CLEAR_ONE_TURN
            gBattleStruct->turnEffectsTracker++;
            break;
        case ENDTURN_BATTLER_COUNT:  // done
            gLastChosenMove[gActiveBattler] = gChosenMoveByBattler[gActiveBattler];
            gBattleStruct->turnEffectsTracker = 0;
            gBattleStruct->turnEffectsBattlerId++;
            break;
        }

        if (effect != 0)
            return effect;

    }
    gHitMarker &= ~(HITMARKER_GRUDGE | HITMARKER_SKIP_DMG_TRACK);
    return 0;
}

bool8 HandleWishPerishSongOnTurnEnd(void)
{
    gHitMarker |= (HITMARKER_GRUDGE | HITMARKER_SKIP_DMG_TRACK);

    switch (gBattleStruct->wishPerishSongState)
    {
    case 0:
        while (gBattleStruct->wishPerishSongBattlerId < gBattlersCount)
        {
            gActiveBattler = gBattleStruct->wishPerishSongBattlerId;
            if (gAbsentBattlerFlags & 1 << gActiveBattler)
            {
                gBattleStruct->wishPerishSongBattlerId++;
                continue;
            }

            gBattleStruct->wishPerishSongBattlerId++;
            if (gWishFutureKnock.futureSightCounter[gActiveBattler] != 0
             && --gWishFutureKnock.futureSightCounter[gActiveBattler] == 0
             && gBattleMons[gActiveBattler].hp != 0)
            {
                if (gWishFutureKnock.futureSightMove[gActiveBattler] == MOVE_FUTURE_SIGHT)
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_FUTURE_SIGHT;
                else
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DOOM_DESIRE;

                PREPARE_MOVE_BUFFER(gBattleTextBuff1, gWishFutureKnock.futureSightMove[gActiveBattler]);

                gBattlerTarget = gActiveBattler;
                gBattlerAttacker = gWishFutureKnock.futureSightAttacker[gActiveBattler];
                gTurnStructs[gBattlerTarget].dmg = 0xFFFF;
                gCurrentMove = gWishFutureKnock.futureSightMove[gActiveBattler];
                SetTypeBeforeUsingMove(gCurrentMove, gActiveBattler);
                BattleScriptExecute(BattleScript_MonTookFutureAttack);

                if (gWishFutureKnock.futureSightCounter[gActiveBattler] == 0
                 && gWishFutureKnock.futureSightCounter[gActiveBattler ^ BIT_FLANK] == 0)
                {
                    gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] &= ~(SIDE_STATUS_FUTUREATTACK);
                }
                return TRUE;
            }
        }
        gBattleStruct->wishPerishSongState = 1;
        gBattleStruct->wishPerishSongBattlerId = 0;
        // fall through
    case 1:
        while (gBattleStruct->wishPerishSongBattlerId < gBattlersCount)
        {
            gActiveBattler = gBattlerAttacker = gBattlerByTurnOrder[gBattleStruct->wishPerishSongBattlerId];
            if (gAbsentBattlerFlags & 1 << gActiveBattler)
            {
                gBattleStruct->wishPerishSongBattlerId++;
                continue;
            }
            gBattleStruct->wishPerishSongBattlerId++;
            if (gStatuses3[gActiveBattler] & STATUS3_PERISH_SONG)
            {
                PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, gVolatileStructs[gActiveBattler].perishSongTimer);
                if (gVolatileStructs[gActiveBattler].perishSongTimer == 0)
                {
                    gStatuses3[gActiveBattler] &= ~STATUS3_PERISH_SONG;
                    gBattleMoveDamage = gBattleMons[gActiveBattler].hp;
                    gBattlescriptCurrInstr = BattleScript_PerishSongTakesLife;
                }
                else
                {
                    gVolatileStructs[gActiveBattler].perishSongTimer--;
                    gBattlescriptCurrInstr = BattleScript_PerishSongCountGoesDown;
                }
                BattleScriptExecute(gBattlescriptCurrInstr);
                return TRUE;
            }
        }
        // Hm...
        {
            u8 *state = &gBattleStruct->wishPerishSongState;
            *state = 2;
            gBattleStruct->wishPerishSongBattlerId = 0;
        }
        // fall through
    case 2:
        if ((gBattleTypeFlags & BATTLE_TYPE_ARENA)
         && gBattleStruct->arenaTurnCounter == 2
         && gBattleMons[0].hp != 0 && gBattleMons[1].hp != 0)
        {
            s32 i;

            for (i = 0; i < 2; i++)
                CancelMultiTurnMoves(i);

            gBattlescriptCurrInstr = BattleScript_ArenaDoJudgment;
            BattleScriptExecute(BattleScript_ArenaDoJudgment);
            gBattleStruct->wishPerishSongState++;
            return TRUE;
        }
        break;
    }

    gHitMarker &= ~(HITMARKER_GRUDGE | HITMARKER_SKIP_DMG_TRACK);

    return FALSE;
}

#define FAINTED_ACTIONS_MAX_CASE 7

bool8 HandleFaintedMonActions(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
        return FALSE;
    do
    {
        s32 i;
        switch (gBattleStruct->faintedActionsState)
        {
        case 0:
            gBattleStruct->faintedActionsBattlerId = 0;
            gBattleStruct->faintedActionsState++;
            for (i = 0; i < gBattlersCount; i++)
            {
                if (gAbsentBattlerFlags & 1 << i && !HasNoMonsToSwitch(i, PARTY_SIZE, PARTY_SIZE))
                    gAbsentBattlerFlags &= ~(1 << i);
            }
            // fall through
        case 1:
            do
            {
                gBattlerFainted = gBattlerTarget = gBattleStruct->faintedActionsBattlerId;
                if (gBattleMons[gBattleStruct->faintedActionsBattlerId].hp == 0
                 && !(gBattleStruct->givenExpMons & 1 << gBattlerPartyIndexes[gBattleStruct->faintedActionsBattlerId])
                 && !(gAbsentBattlerFlags & 1 << gBattleStruct->faintedActionsBattlerId))
                {
                    BattleScriptExecute(BattleScript_GiveExp);
                    gBattleStruct->faintedActionsState = 2;
                    return TRUE;
                }
            } while (++gBattleStruct->faintedActionsBattlerId != gBattlersCount);
            gBattleStruct->faintedActionsState = 3;
            break;
        case 2:
            OpponentSwitchInResetSentPokesToOpponentValue(gBattlerFainted);
            if (++gBattleStruct->faintedActionsBattlerId == gBattlersCount)
                gBattleStruct->faintedActionsState = 3;
            else
                gBattleStruct->faintedActionsState = 1;

            // Don't switch mons until all pokemon performed their actions or the battle's over.
            if (gBattleOutcome == 0
                && !NoAliveMonsForEitherParty()
                && gCurrentTurnActionNumber != gBattlersCount)
            {
                gAbsentBattlerFlags |= 1 << gBattlerFainted;
                return FALSE;
            }
            break;
        case 3:
            // Don't switch mons until all pokemon performed their actions or the battle's over.
            if (gBattleOutcome == 0
                && !NoAliveMonsForEitherParty()
                && gCurrentTurnActionNumber != gBattlersCount)
            {
                return FALSE;
            }
            gBattleStruct->faintedActionsBattlerId = 0;
            gBattleStruct->faintedActionsState++;
            // fall through
        case 4:
            do
            {
                gBattlerFainted = gBattlerTarget = gBattleStruct->faintedActionsBattlerId;
                if (gBattleMons[gBattleStruct->faintedActionsBattlerId].hp == 0
                 && !(gAbsentBattlerFlags & 1 << gBattleStruct->faintedActionsBattlerId))
                {
                    BattleScriptExecute(BattleScript_HandleFaintedMon);
                    gBattleStruct->faintedActionsState = 5;
                    return TRUE;
                }
            } while (++gBattleStruct->faintedActionsBattlerId != gBattlersCount);
            gBattleStruct->faintedActionsState = 6;
            break;
        case 5:
            if (++gBattleStruct->faintedActionsBattlerId == gBattlersCount)
                gBattleStruct->faintedActionsState = 6;
            else
                gBattleStruct->faintedActionsState = 4;
            break;
        case 6:
            if (ItemBattleEffects(1, 0, TRUE))
                return TRUE;
            gBattleStruct->faintedActionsState++;
            break;
        case FAINTED_ACTIONS_MAX_CASE:
            break;
        }
    } while (gBattleStruct->faintedActionsState != FAINTED_ACTIONS_MAX_CASE);
    return FALSE;
}

void TryClearRageAndFuryCutter(void)
{
    s32 i;
    for (i = 0; i < gBattlersCount; i++)
    {
        if ((gBattleMons[i].status2 & STATUS2_RAGE) && gChosenMoveByBattler[i] != MOVE_RAGE)
            gBattleMons[i].status2 &= ~(STATUS2_RAGE);
        if (gVolatileStructs[i].furyCutterCounter != 0 && gChosenMoveByBattler[i] != MOVE_FURY_CUTTER)
            gVolatileStructs[i].furyCutterCounter = 0;
    }
}

enum
{
    CANCELLER_FLAGS,
    CANCELLER_ASLEEP,
    CANCELLER_FROZEN,
    CANCELLER_TRUANT,
    CANCELLER_RECHARGE,
    CANCELLER_FLINCH,
    CANCELLER_DISABLED,
    CANCELLER_GRAVITY,
    CANCELLER_HEAL_BLOCKED,
    CANCELLER_TAUNTED,
    CANCELLER_IMPRISONED,
    CANCELLER_CONFUSED,
    CANCELLER_PARALYSED,
    CANCELLER_BIDE,
    CANCELLER_THAW,
    CANCELLER_POWDER_MOVE,
    CANCELLER_POWDER_STATUS,
    CANCELLER_THROAT_CHOP,
    CANCELLER_MULTIHIT_MOVES,
    CANCELLER_SKY_DROP,
    CANCELLER_END,
    CANCELLER_PSYCHIC_TERRAIN,
    CANCELLER_END2,
};

u8 AtkCanceller_UnableToUseMove(void)
{
    u8 effect = 0;
    s32 *bideDmg = &gBattleScripting.bideDmg;
    do
    {
        switch (gBattleStruct->atkCancellerTracker)
        {
        case CANCELLER_FLAGS: // flags clear
            gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_DESTINY_BOND);
            gStatuses3[gBattlerAttacker] &= ~(STATUS3_GRUDGE);
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_ASLEEP: // check being asleep
            if (gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP)
            {
                if (UproarWakeUpCheck(gBattlerAttacker))
                {
                    gBattleMons[gBattlerAttacker].status1 &= ~(STATUS1_SLEEP);
                    gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_NIGHTMARE);
                    BattleScriptCall(BattleScript_MoveUsedWokeUp);
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WOKE_UP_UPROAR;
                    effect = 2;
                }
                else
                {
                    u8 toSub;
                    if (BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_EARLY_BIRD))
                        toSub = 2;
                    else
                        toSub = 1;

                    if ((gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP) < toSub)
                        gBattleMons[gBattlerAttacker].status1 &= ~(STATUS1_SLEEP);
                    else
                        gBattleMons[gBattlerAttacker].status1 -= toSub;

                    if (gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP)
                    {
                        if (gChosenMove != MOVE_SNORE && gChosenMove != MOVE_SLEEP_TALK)
                        {
                            gBattlescriptCurrInstr = BattleScript_MoveUsedIsAsleep;
                            gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                            effect = 1;
                        }
                    }
                    else
                    {
                        gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_NIGHTMARE);
                        BattleScriptCall(BattleScript_MoveUsedWokeUp);
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_WOKE_UP;
                        effect = 2;
                    }
                }
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_FROZEN: // check being frozen
            if (gBattleMons[gBattlerAttacker].status1 & STATUS1_FREEZE && !(gBattleMoves[gCurrentMove].flags & FLAG_THAW_USER))
            {
                if (Random() % 5)
                {
                    gBattlescriptCurrInstr = BattleScript_MoveUsedIsFrozen;
                    gHitMarker |= HITMARKER_NO_ATTACKSTRING;
                    effect = 1;
                }
                else // unfreeze
                {
                    gBattleMons[gBattlerAttacker].status1 &= ~(STATUS1_FREEZE);
                    BattleScriptCall(BattleScript_MoveUsedUnfroze);
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DEFROSTED;
                    effect = 2;
                }
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_SKY_DROP:
            if (gVolatileStructs[gBattlerAttacker].skyDropped)
            {
                gBattlescriptCurrInstr = BattleScript_SkyDropInAir;
                gHitMarker |= HITMARKER_NO_ATTACKSTRING;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_TRUANT: // truant
            if (GetBattlerAbility(gBattlerAttacker) == ABILITY_TRUANT && gVolatileStructs[gBattlerAttacker].truantCounter)
            {
                CancelMultiTurnMoves(gBattlerAttacker);
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_LOAFING;
                gBattlerAbility = gBattlerAttacker;
                gBattlescriptCurrInstr = BattleScript_TruantLoafingAround;
                gMoveResultFlags |= MOVE_RESULT_MISSED;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_RECHARGE: // recharge
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_RECHARGE)
            {
                gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_RECHARGE);
                gVolatileStructs[gBattlerAttacker].rechargeTimer = 0;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedMustRecharge;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_FLINCH: // flinch
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_FLINCHED)
            {
                gRoundStructs[gBattlerAttacker].flinchImmobility = TRUE;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedFlinched;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_DISABLED: // disabled move
            if (gVolatileStructs[gBattlerAttacker].disabledMove == gCurrentMove && gVolatileStructs[gBattlerAttacker].disabledMove != 0)
            {
                gRoundStructs[gBattlerAttacker].usedDisabledMove = TRUE;
                gBattleScripting.battler = gBattlerAttacker;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedIsDisabled;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_HEAL_BLOCKED:
            if (IsHealBlockPreventingMove(gBattlerAttacker, gCurrentMove))
            {
                gRoundStructs[gBattlerAttacker].usedHealBlockedMove = TRUE;
                gBattleScripting.battler = gBattlerAttacker;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedHealBlockPrevents;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_GRAVITY:
            if (gFieldStatuses & STATUS_FIELD_GRAVITY && IsGravityPreventingMove(gCurrentMove))
            {
                gRoundStructs[gBattlerAttacker].usedGravityPreventedMove = TRUE;
                gBattleScripting.battler = gBattlerAttacker;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedGravityPrevents;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_TAUNTED: // taunt
            if (gVolatileStructs[gBattlerAttacker].tauntTimer && gBattleMoves[gCurrentMove].power == 0)
            {
                gRoundStructs[gBattlerAttacker].usedTauntedMove = TRUE;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedIsTaunted;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_IMPRISONED: // imprisoned
            if (GetImprisonedMovesCount(gBattlerAttacker, gCurrentMove))
            {
                gRoundStructs[gBattlerAttacker].usedImprisonedMove = TRUE;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedIsImprisoned;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_CONFUSED: // confusion
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_CONFUSION)
            {
                gBattleMons[gBattlerAttacker].status2 -= STATUS2_CONFUSION_TURN(1);
                if (gBattleMons[gBattlerAttacker].status2 & STATUS2_CONFUSION)
                {
                    if (Random() % ((B_CONFUSION_SELF_DMG_CHANCE >= GEN_7) ? 3 : 2) == 0) // confusion dmg
                    {
                        u8 moveType = TYPE_MYSTERY;
                        gBattleCommunication[MULTISTRING_CHOOSER] = TRUE;
                        gBattlerTarget = gBattlerAttacker;
                        gBattleMoveDamage = CalculateMoveDamage(MOVE_NONE, gBattlerAttacker, gBattlerAttacker, &moveType, IsAbilityOnSide(BATTLE_OPPOSITE(gBattlerAttacker), ABILITY_COSMIC_DAZE) ? 80 : 40, FALSE, FALSE, TRUE);
                        gRoundStructs[gBattlerAttacker].confusionSelfDmg = TRUE;
                        gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                        effect = 1;
                    }
                    else
                    {
                        gBattleCommunication[MULTISTRING_CHOOSER] = FALSE;
                        BattleScriptPushCursor();
                        effect = 3;
                    }
                    gBattlescriptCurrInstr = BattleScript_MoveUsedIsConfused;
                }
                else // snapped out of confusion
                {
                    BattleScriptCall(BattleScript_MoveUsedIsConfusedNoMore);
                    effect = 3;
                }
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_PARALYSED: // paralysis
            if ((gBattleMons[gBattlerAttacker].status1 & STATUS1_PARALYSIS) && (Random() % 4) == 0)
            {
                gRoundStructs[gBattlerAttacker].prlzImmobility = TRUE;
                // This is removed in Emerald for some reason
                //CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedIsParalyzed;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_BIDE: // bide
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_BIDE)
            {
                gBattleMons[gBattlerAttacker].status2 -= STATUS2_BIDE_TURN(1);
                if (gBattleMons[gBattlerAttacker].status2 & STATUS2_BIDE)
                {
                    gBattlescriptCurrInstr = BattleScript_BideStoringEnergy;
                }
                else
                {
                    // This is removed in Emerald for some reason
                    //gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_MULTIPLETURNS);
                    if (gTakenDmg[gBattlerAttacker])
                    {
                        gCurrentMove = MOVE_BIDE;
                        *bideDmg = gTakenDmg[gBattlerAttacker] * 2;
                        gBattlerTarget = gTakenDmgByBattler[gBattlerAttacker];
                        if (gAbsentBattlerFlags & 1 << gBattlerTarget)
                            gBattlerTarget = GetMoveTarget(MOVE_BIDE, 1);
                        gBattlescriptCurrInstr = BattleScript_BideAttack;
                    }
                    else
                    {
                        gBattlescriptCurrInstr = BattleScript_BideNoEnergyToAttack;
                    }
                }
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_THAW: // move thawing
            if (gBattleMons[gBattlerAttacker].status1 & STATUS1_FREEZE)
            {
                if (!(gBattleMoves[gCurrentMove].effect == EFFECT_BURN_UP && !IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_FIRE)))
                {
                    gBattleMons[gBattlerAttacker].status1 &= ~(STATUS1_FREEZE);
                    BattleScriptCall(BattleScript_MoveUsedUnfroze);
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_DEFROSTED_BY_MOVE;
                }
                effect = 2;
            }
            if (gBattleMons[gBattlerAttacker].status1 & STATUS1_FROSTBITE && (gBattleMoves[gCurrentMove].flags & FLAG_THAW_USER))
            {
                if (!(gBattleMoves[gCurrentMove].effect == EFFECT_BURN_UP && !IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_FIRE)))
                {
                    gBattleMons[gBattlerAttacker].status1 &= ~STATUS1_FROSTBITE;
                    BattleScriptCall(BattleScript_MoveUsedUnfrostbite);
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_FROSTBITE_HEALED_BY_MOVE;
                }
                effect = 2;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_POWDER_MOVE:
            if ((gBattleMoves[gCurrentMove].flags & FLAG_POWDER) && (gBattlerAttacker != gBattlerTarget) && !IsMyceliumMightActive(gBattlerAttacker)) // Rage Powder targets the user
            {
                if ((B_POWDER_GRASS >= GEN_6 && IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_GRASS))
                    || GetBattlerAbility(gBattlerTarget) == ABILITY_OVERCOAT
					|| BattlerHasInnate(gBattlerTarget, ABILITY_OVERCOAT))
                {
                    gBattlerAbility = gBattlerTarget;
                    effect = 1;
                }
                else if (GetBattlerHoldEffect(gBattlerTarget, TRUE) == HOLD_EFFECT_SAFETY_GOGGLES)
                {
                    RecordItemEffectBattle(gBattlerTarget, HOLD_EFFECT_SAFETY_GOGGLES);
                    gLastUsedItem = gBattleMons[gBattlerTarget].item;
                    effect = 1;
                }

                if (effect)
                    gBattlescriptCurrInstr = BattleScript_PowderMoveNoEffect;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_POWDER_STATUS:
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_POWDER)
            {
                u32 moveType;
                GET_MOVE_TYPE(gCurrentMove, moveType);
                if (moveType == TYPE_FIRE)
                {
                    gRoundStructs[gBattlerAttacker].powderSelfDmg = TRUE;
                    gBattleMoveDamage = BATTLER_HAS_MAGIC_GUARD(gBattlerAttacker) ? 0 : (gBattleMons[gBattlerAttacker].maxHP / 4);
                    gBattlescriptCurrInstr = BattleScript_MoveUsedPowder;
                    effect = 1;
                }
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_THROAT_CHOP:
            if (gVolatileStructs[gBattlerAttacker].throatChopTimer && gBattleMoves[gCurrentMove].flags & FLAG_SOUND)
            {
                gRoundStructs[gBattlerAttacker].usedThroatChopPreventedMove = TRUE;
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedIsThroatChopPrevented;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_MULTIHIT_MOVES:
        {
            if (gBattleMoves[gCurrentMove].effect == EFFECT_MULTI_HIT)
            {
                u16 ability = gBattleMons[gBattlerAttacker].ability;

                if (ability == ABILITY_SKILL_LINK || 
                    BattlerHasInnate(gBattlerAttacker, ABILITY_SKILL_LINK))
                {
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 5;
                }
                else if (ability == ABILITY_KUNOICHI_BLADE || 
                    BattlerHasInnate(gBattlerAttacker, ABILITY_KUNOICHI_BLADE))
                {
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 5;
                } 
                else if (ability == ABILITY_BATTLE_BOND
                && gCurrentMove == MOVE_WATER_SHURIKEN
                && gBattleMons[gBattlerAttacker].species == SPECIES_GRENINJA_ASH)
                {
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 3;
                }
                else
                {
                    if (GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_LOADED_DICE)
                    {
                        gTurnStructs[gBattlerAttacker].multiHitCounter = 4 + (Random() % 2);
                    }
                    else
                    {
                        // 2 and 3 hits: 33.3%
                        // 4 and 5 hits: 16.7%
                        gTurnStructs[gBattlerAttacker].multiHitCounter = Random() % 4;
                        if (gTurnStructs[gBattlerAttacker].multiHitCounter > 2)
                        {
                            gTurnStructs[gBattlerAttacker].multiHitCounter = (Random() % 3);
                            if (gTurnStructs[gBattlerAttacker].multiHitCounter < 2)
                                gTurnStructs[gBattlerAttacker].multiHitCounter = 2;
                            else
                                gTurnStructs[gBattlerAttacker].multiHitCounter = 3;
                        }
                        else
                            gTurnStructs[gBattlerAttacker].multiHitCounter += 3;
                    }
                }

                PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)
            }
            else if (IsTwoStrikesMove(gCurrentMove))
            {
                gTurnStructs[gBattlerAttacker].multiHitCounter = 2;
				PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)
            }
            else if (gBattleMoves[gCurrentMove].effect == EFFECT_DOUBLE_HIT)
            {
                gTurnStructs[gBattlerAttacker].multiHitCounter = gBattleMoves[gCurrentMove].argument ? gBattleMoves[gCurrentMove].argument : 2;
				PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)
            }
            else if (gBattleMoves[gCurrentMove].effect == EFFECT_TRIPLE_KICK)
            {
                gTurnStructs[gBattlerAttacker].multiHitCounter = 3;
				PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)
            }
            else if (gBattleMoves[gCurrentMove].effect == EFFECT_TEN_HITS)
            {
                gTurnStructs[gBattlerAttacker].multiHitCounter = 10;
				PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 2, 0)
            }
            #if B_BEAT_UP_DMG >= GEN_5
            else if (gBattleMoves[gCurrentMove].effect == EFFECT_BEAT_UP)
            {
                struct Pokemon* party;
                int i;

                if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
                    party = gPlayerParty;
                else
                    party = gEnemyParty;
                
                for (i = 0; i < PARTY_SIZE; i++)
				{
					if (GetMonData(&party[i], MON_DATA_HP)
					&& GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE
					&& !GetMonData(&party[i], MON_DATA_IS_EGG)
					&& !GetMonData(&party[i], MON_DATA_STATUS))
						gTurnStructs[gBattlerAttacker].multiHitCounter++;
				}

				gBattleCommunication[0] = 0; // For later
				PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)
            }
            #endif
            gBattleStruct->atkCancellerTracker++;
            break;
        }
        case CANCELLER_END:
            break;
        }

    } while (gBattleStruct->atkCancellerTracker != CANCELLER_END && gBattleStruct->atkCancellerTracker != CANCELLER_END2 && effect == 0);

    if (effect == 1)
    {
        gRoundStructs[gBattlerAttacker].attackCancelled = TRUE;
    }
    else if (effect == 2)
    {
        gActiveBattler = gBattlerAttacker;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
    }
    return effect;
}

// After Protean Activation.
u8 AtkCanceller_UnableToUseMove2(void)
{
    u8 effect = 0;

    do
    {
        switch (gBattleStruct->atkCancellerTracker)
        {
        case CANCELLER_END:
            gBattleStruct->atkCancellerTracker++;
        case CANCELLER_PSYCHIC_TERRAIN:
            if (IsBattlerTerrainAffected(gBattlerTarget, STATUS_FIELD_PSYCHIC_TERRAIN)
                && GetChosenMovePriority(gBattlerAttacker, gBattlerTarget) > 0
                && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget))
            {
                CancelMultiTurnMoves(gBattlerAttacker);
                gBattlescriptCurrInstr = BattleScript_MoveUsedPsychicTerrainPrevents;
                gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
                effect = 1;
            }
            gBattleStruct->atkCancellerTracker++;
            break;
        case CANCELLER_END2:
            break;
        }

    } while (gBattleStruct->atkCancellerTracker != CANCELLER_END2 && effect == 0);

    if (effect == 1)
    {
        gRoundStructs[gBattlerAttacker].attackCancelled = TRUE;
    }

    return effect;
}

bool8 HasNoMonsToSwitch(u8 battler, u8 partyIdBattlerOn1, u8 partyIdBattlerOn2)
{
    struct Pokemon *party;
    u8 id1, id2;
    s32 i;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        return FALSE;

    if (BATTLE_TWO_VS_ONE_OPPONENT && GetBattlerSide(battler) == B_SIDE_OPPONENT)
    {
        id2 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        id1 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        party = gEnemyParty;

        if (partyIdBattlerOn1 == PARTY_SIZE)
            partyIdBattlerOn1 = gBattlerPartyIndexes[id2];
        if (partyIdBattlerOn2 == PARTY_SIZE)
            partyIdBattlerOn2 = gBattlerPartyIndexes[id1];

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HP) != 0
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_NONE
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG
             && i != partyIdBattlerOn1 && i != partyIdBattlerOn2
             && i != *(gBattleStruct->monToSwitchIntoId + id2) && i != id1[gBattleStruct->monToSwitchIntoId])
                break;
        }
        return (i == PARTY_SIZE);
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        if (GetBattlerSide(battler) == B_SIDE_PLAYER)
            party = gPlayerParty;
        else
            party = gEnemyParty;

        id1 = ((battler & BIT_FLANK) / 2);
        for (i = id1 * 3; i < id1 * 3 + 3; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HP) != 0
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_NONE
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG)
                break;
        }
        return (i == id1 * 3 + 3);
    }
    else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_TOWER_LINK_MULTI)
        {
            if (GetBattlerSide(battler) == B_SIDE_PLAYER)
            {
                party = gPlayerParty;
                id2 = GetBattlerMultiplayerId(battler);
                id1 = GetLinkTrainerFlankId(id2);
            }
            else
            {
                party = gEnemyParty;
                if (battler == 1)
                    id1 = 0;
                else
                    id1 = 1;
            }
        }
        else
        {
            id2 = GetBattlerMultiplayerId(battler);

            if (GetBattlerSide(battler) == B_SIDE_PLAYER)
                party = gPlayerParty;
            else
                party = gEnemyParty;

            id1 = GetLinkTrainerFlankId(id2);
        }

        for (i = id1 * 3; i < id1 * 3 + 3; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HP) != 0
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_NONE
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG)
                break;
        }
        return (i == id1 * 3 + 3);
    }
    else if ((gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) && GetBattlerSide(battler) == B_SIDE_OPPONENT)
    {
        party = gEnemyParty;

        if (battler == 1)
            id1 = 0;
        else
            id1 = 3;

        for (i = id1; i < id1 + 3; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HP) != 0
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_NONE
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG)
                break;
        }
        return (i == id1 + 3);
    }
    else
    {
        if (GetBattlerSide(battler) == B_SIDE_OPPONENT)
        {
            id2 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
            id1 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
            party = gEnemyParty;
        }
        else
        {
            id2 = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
            id1 = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
            party = gPlayerParty;
        }

        if (partyIdBattlerOn1 == PARTY_SIZE)
            partyIdBattlerOn1 = gBattlerPartyIndexes[id2];
        if (partyIdBattlerOn2 == PARTY_SIZE)
            partyIdBattlerOn2 = gBattlerPartyIndexes[id1];

        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&party[i], MON_DATA_HP) != 0
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_NONE
             && GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG
             && i != partyIdBattlerOn1 && i != partyIdBattlerOn2
             && i != *(gBattleStruct->monToSwitchIntoId + id2) && i != id1[gBattleStruct->monToSwitchIntoId])
                break;
        }
        return (i == PARTY_SIZE);
    }
}

static const u16 sWeatherFlagsInfo[][3] =
{
    [ENUM_WEATHER_RAIN]         = {WEATHER_RAIN_TEMPORARY,      WEATHER_RAIN_PERMANENT,      HOLD_EFFECT_DAMP_ROCK},
    [ENUM_WEATHER_RAIN_PRIMAL]  = {WEATHER_RAIN_PRIMAL,         WEATHER_RAIN_PRIMAL,         HOLD_EFFECT_DAMP_ROCK},
    [ENUM_WEATHER_SUN]          = {WEATHER_SUN_TEMPORARY,       WEATHER_SUN_PERMANENT,       HOLD_EFFECT_HEAT_ROCK},
    [ENUM_WEATHER_SUN_PRIMAL]   = {WEATHER_SUN_PRIMAL,          WEATHER_SUN_PRIMAL,          HOLD_EFFECT_HEAT_ROCK},
    [ENUM_WEATHER_SANDSTORM]    = {WEATHER_SANDSTORM_TEMPORARY, WEATHER_SANDSTORM_PERMANENT, HOLD_EFFECT_SMOOTH_ROCK},
    [ENUM_WEATHER_HAIL]         = {WEATHER_HAIL_TEMPORARY,      WEATHER_HAIL_PERMANENT,      HOLD_EFFECT_ICY_ROCK},
    [ENUM_WEATHER_STRONG_WINDS] = {WEATHER_STRONG_WINDS,        WEATHER_STRONG_WINDS,        HOLD_EFFECT_NONE},
    [ENUM_WEATHER_FOG]          = {WEATHER_FOG_TEMPORARY,       WEATHER_FOG_PERMANENT,       HOLD_EFFECT_SMOKE_BALL},
};

bool32 TryChangeBattleWeather(u8 battler, u32 weatherEnumId, bool32 viaAbility)
{
    if(FlagGet(FLAG_PERMANENT_UNCHANGEABLE_WEATHER)){
        return FALSE;
    }
    if ((gBattleWeather & WEATHER_PRIMAL_ANY)
        && !BATTLER_HAS_ABILITY(battler, ABILITY_DESOLATE_LAND)
        && !BATTLER_HAS_ABILITY(battler, ABILITY_PRIMORDIAL_SEA)
        && !BATTLER_HAS_ABILITY(battler, ABILITY_DELTA_STREAM))
    {
        return FALSE;
    }
    else if (viaAbility && B_ABILITY_WEATHER <= GEN_5
        && !(gBattleWeather & sWeatherFlagsInfo[weatherEnumId][1]))
    {
        gBattleWeather = (sWeatherFlagsInfo[weatherEnumId][0] | sWeatherFlagsInfo[weatherEnumId][1]);
        return TRUE;
    }
    else if (!(gBattleWeather & (sWeatherFlagsInfo[weatherEnumId][0] | sWeatherFlagsInfo[weatherEnumId][1])))
    {
        gBattleWeather = (sWeatherFlagsInfo[weatherEnumId][0]);
        gFieldTimers.started.weather = TRUE;
        if (GetBattlerHoldEffect(battler, TRUE) == sWeatherFlagsInfo[weatherEnumId][2])
            gWishFutureKnock.weatherDuration = WEATHER_DURATION_EXTENDED;
        else
            gWishFutureKnock.weatherDuration = WEATHER_DURATION;

        return TRUE;
    }

    return FALSE;
}

bool32 SetPermanentWeather(u32 weatherEnumId)
{
    if(FlagGet(FLAG_PERMANENT_UNCHANGEABLE_WEATHER)){
        return FALSE;
    }
    else
    {
        gBattleWeather = (sWeatherFlagsInfo[weatherEnumId][0]);
        gWishFutureKnock.weatherDuration = 255;
        FlagSet(FLAG_PERMANENT_UNCHANGEABLE_WEATHER);

        return TRUE;
    }

    return FALSE;
}

bool8 UseOutOfTurnAttack(u8 battler, u8 target, u16 ability, u16 move, u8 movePower)
{
    if (gBattlerAttacker == battler) return FALSE;
    if (gTurnStructs[battler].dancerUsedMove) return FALSE;
    if (gBattleMons[battler].status1 & STATUS1_SLEEP) return FALSE;
    if (gBattleMons[battler].status1 & STATUS1_FREEZE) return FALSE;
    if (!IsBattlerAlive(battler)) return FALSE;

    // Set bit and save Dancer mon's original target
    gTurnStructs[battler].dancerUsedMove = TRUE;

    gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct)
    {
        .ability = ability,
        .move = move,
        .attacker = battler,
        .target = target,
        .movePower = movePower,
        .movePos = MAX_MON_MOVES,
    };

    return TRUE;

}

static bool32 TryChangeBattleTerrain(u32 battler, u32 statusFlag, u8 *timer)
{
    if (!(gFieldStatuses & statusFlag))
    {
        gFieldStatuses &= ~(STATUS_FIELD_MISTY_TERRAIN | STATUS_FIELD_GRASSY_TERRAIN | STATUS_FIELD_ELECTRIC_TERRAIN | STATUS_FIELD_PSYCHIC_TERRAIN);
        gFieldStatuses |= statusFlag;
        gFieldTimers.started.terrain = TRUE;

        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_TERRAIN_EXTENDER)
            *timer = TERRAIN_DURATION_EXTENDED;
        else
            *timer = TERRAIN_DURATION;

        gBattlerAttacker = gBattleScripting.battler = battler;
        return TRUE;
    }

    return FALSE;
}

// Ability,     form >, form <=, hp divided
static const u16 sHpTransformations[][4] =
{
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR,               SPECIES_MINIOR_CORE_RED,              2},
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR_METEOR_BLUE,   SPECIES_MINIOR_CORE_BLUE,             2},
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR_METEOR_GREEN,  SPECIES_MINIOR_CORE_GREEN,            2},
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR_METEOR_INDIGO, SPECIES_MINIOR_CORE_INDIGO,           2},
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR_METEOR_ORANGE, SPECIES_MINIOR_CORE_ORANGE,           2},
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR_METEOR_VIOLET, SPECIES_MINIOR_CORE_VIOLET,           2},
    {ABILITY_SHIELDS_DOWN, SPECIES_MINIOR_METEOR_YELLOW, SPECIES_MINIOR_CORE_YELLOW,           2},
    {ABILITY_SCHOOLING,    SPECIES_WISHIWASHI_SCHOOL,    SPECIES_WISHIWASHI,                   4},
};

bool32 ShouldChangeFormHpBased(u32 battler)
{
    u32 i;

    if (gBattleMons[battler].status2 & STATUS2_TRANSFORMED) return FALSE;

    for (i = 0; i < ARRAY_COUNT(sHpTransformations); i++)
    {
        if (GetBattlerAbility(battler) == sHpTransformations[i][0] || BattlerHasInnate(battler, sHpTransformations[i][0]))
        {
            if (sHpTransformations[i][0] == ABILITY_SCHOOLING && gBattleMons[battler].level < 20) continue;
            if (gBattleMons[battler].species == sHpTransformations[i][2]
                && gBattleMons[battler].hp > gBattleMons[battler].maxHP / sHpTransformations[i][3])
            {
                if (sHpTransformations[i][0] == ABILITY_SHIELDS_DOWN && GetAbilityState(battler, ABILITY_SHIELDS_DOWN))
                {
                    return FALSE;
                }
                gBattleScripting.abilityPopupOverwrite = sHpTransformations[i][0];
                UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, sHpTransformations[i][1]);
                gBattleMons[battler].species = sHpTransformations[i][1];
                return TRUE;
            }
            if (gBattleMons[battler].species == sHpTransformations[i][1]
                && gBattleMons[battler].hp <= gBattleMons[battler].maxHP / sHpTransformations[i][3])
            {
                gBattleScripting.abilityPopupOverwrite = sHpTransformations[i][0];
                UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, sHpTransformations[i][2]);
                gBattleMons[battler].species = sHpTransformations[i][2];
                return TRUE;
            }
        }
    }

    //Darmanitan
    if (gBattleMons[battler].species == SPECIES_DARMANITAN && 
	    (BATTLER_HAS_ABILITY(battler, ABILITY_ZEN_MODE)) &&
		gBattleMons[battler].hp != 0){
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_ZEN_MODE;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_DARMANITAN_ZEN_MODE);
            gBattleMons[battler].species = SPECIES_DARMANITAN_ZEN_MODE;
			return TRUE;
	}

    //Darmanitan Galarian
    if (gBattleMons[battler].species == SPECIES_DARMANITAN_GALARIAN && 
	    (BATTLER_HAS_ABILITY(battler, ABILITY_ZEN_MODE)) &&
		gBattleMons[battler].hp != 0){
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_ZEN_MODE;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_DARMANITAN_ZEN_MODE_GALARIAN);
            gBattleMons[battler].species = SPECIES_DARMANITAN_ZEN_MODE_GALARIAN;
			return TRUE;
	}

    //Castform
    if ((gBattleMons[battler].species == SPECIES_CASTFORM       ||
         gBattleMons[battler].species == SPECIES_CASTFORM_RAINY ||
         gBattleMons[battler].species == SPECIES_CASTFORM_SUNNY ||
         gBattleMons[battler].species == SPECIES_CASTFORM_SANDY ||
         gBattleMons[battler].species == SPECIES_CASTFORM_SNOWY) && 
	    BATTLER_HAS_ABILITY(battler, ABILITY_FORECAST) &&
		gBattleMons[battler].hp != 0){
		if (gBattleWeather & (WEATHER_RAIN_ANY) && gBattleMons[battler].species != SPECIES_CASTFORM_RAINY)
        {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FORECAST;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CASTFORM_RAINY);
            gBattleMons[battler].species = SPECIES_CASTFORM_RAINY;
			return TRUE;
        }
        else if (gBattleWeather & (WEATHER_SUN_ANY) && gBattleMons[battler].species != SPECIES_CASTFORM_SUNNY)
        {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FORECAST;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CASTFORM_SUNNY);
            gBattleMons[battler].species = SPECIES_CASTFORM_SUNNY;
			return TRUE;
        }
		else if (gBattleWeather & (WEATHER_HAIL_ANY) && gBattleMons[battler].species != SPECIES_CASTFORM_SNOWY)
        {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FORECAST;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CASTFORM_SNOWY);
            gBattleMons[battler].species = SPECIES_CASTFORM_SNOWY;
			return TRUE;
        }
		else if (gBattleWeather & (WEATHER_SANDSTORM_ANY) && gBattleMons[battler].species != SPECIES_CASTFORM_SANDY)
        {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FORECAST;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CASTFORM_SANDY);
            gBattleMons[battler].species = SPECIES_CASTFORM_SANDY;
			return TRUE;
        }else if(!(gBattleWeather & (WEATHER_ANY)) && gBattleMons[battler].species != SPECIES_CASTFORM){
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FORECAST;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CASTFORM);
            gBattleMons[battler].species = SPECIES_CASTFORM;
			return TRUE;
        }
        else{
			return FALSE;
        }
	}

    //Cherrim
    if (gBattleMons[battler].species == SPECIES_CHERRIM && 
	          BATTLER_HAS_ABILITY(battler, ABILITY_FLOWER_GIFT) &&
		      gBattleMons[battler].hp != 0){
			if (gBattleWeather & (WEATHER_SUN_ANY))
        {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FLOWER_GIFT;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CHERRIM_SUNSHINE);
            gBattleMons[battler].species = SPECIES_CHERRIM_SUNSHINE;
			return TRUE;
		}
	}
    else if(gBattleMons[battler].species == SPECIES_CHERRIM_SUNSHINE &&
	          BATTLER_HAS_ABILITY(battler, ABILITY_FLOWER_GIFT) &&
		      gBattleMons[battler].hp != 0){
			if (!(gBattleWeather & (WEATHER_SUN_ANY)))
        {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FLOWER_GIFT;
            gBattlerAttacker = battler;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_CHERRIM);
            gBattleMons[battler].species = SPECIES_CHERRIM;
			return TRUE;
		}
	}

    return FALSE;
}

static u8 ForewarnChooseMove(u32 battler)
{
    struct Forewarn {
        u8 battlerId;
        u8 power;
        u16 moveId;
    };
    u32 i, j, bestId, count;
    struct Forewarn *data = malloc(sizeof(struct Forewarn) * gBattlersCount * MAX_MON_MOVES);

    // Put all moves
    for (count = 0, i = 0; i < gBattlersCount; i++)
    {
        if (IsBattlerAlive(i) && GetBattlerSide(i) != GetBattlerSide(battler))
        {
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                if (gBattleMons[i].moves[j] == MOVE_NONE)
                    continue;
                data[count].moveId = gBattleMons[i].moves[j];
                data[count].battlerId = i;
                switch (gBattleMoves[data[count].moveId].effect)
                {
                case EFFECT_OHKO:
                    data[count].power = 150;
                    break;
                case EFFECT_COUNTER:
                case EFFECT_MIRROR_COAT:
                case EFFECT_METAL_BURST:
                    data[count].power = 120;
                    break;
                default:
                    if (gBattleMoves[data[count].moveId].power == 1)
                        data[count].power = 80;
                    else
                        data[count].power = gBattleMoves[data[count].moveId].power;
                    break;
                }
                count++;
            }
        }
    }

    for (bestId = 0, i = 1; i < count; i++)
    {
        if (data[i].power > data[bestId].power)
            bestId = i;
        else if (data[i].power == data[bestId].power && Random() & 1)
            bestId = i;
    }

    gBattlerTarget = data[bestId].battlerId;
    PREPARE_MOVE_BUFFER(gBattleTextBuff1, data[bestId].moveId)
    RecordKnownMove(gBattlerTarget, data[bestId].moveId);

    free(data);
}

bool32 TryPrimalReversion(u8 battlerId, int useReturn)
{
    if (GetBattlerHoldEffect(battlerId, FALSE) == HOLD_EFFECT_PRIMAL_ORB
     && GetPrimalReversionSpecies(gBattleMons[battlerId].species, gBattleMons[battlerId].item) != SPECIES_NONE)
    {
        gBattlerAttacker = battlerId;
        if (useReturn)
        {
            BattleScriptCall(BattleScript_PrimalReversionRet);
        }
        else
        {
            BattleScriptExecute(BattleScript_PrimalReversion);
        }
        return TRUE;
    }
    return FALSE;
}

void DisableSwitchInAbility(u8 battlerId, u16 ability)
{
    switch (BattlerHasInnateOrAbility(battlerId, ability))
    {
        case BATTLER_INNATE:
            gVolatileStructs[battlerId].switchInAbilityDone[GetBattlerInnateNum(battlerId, ability) + 1] = TRUE;
            return;
        case BATTLER_ABILITY:
            gVolatileStructs[battlerId].switchInAbilityDone[0] = TRUE;
            return;
    }
}

bool8 CheckAndSetSwitchInAbility(u8 battlerId, u16 ability)
{
    switch (BattlerHasInnateOrAbility(battlerId, ability))
    {
        case BATTLER_INNATE:
            if(!gVolatileStructs[battlerId].switchInAbilityDone[GetBattlerInnateNum(battlerId, ability) + 1]){
                gVolatileStructs[battlerId].switchInAbilityDone[GetBattlerInnateNum(battlerId, ability) + 1] = TRUE;
                gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ability;
                gBattlerAbility = gBattlerAttacker = battlerId;
                return TRUE;
            }
            return FALSE;
        case BATTLER_ABILITY:
            if(!gVolatileStructs[battlerId].switchInAbilityDone[0]){
                gVolatileStructs[battlerId].switchInAbilityDone[0] = TRUE;
                gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ability;
                gBattlerAbility = gBattlerAttacker = battlerId;
                return TRUE;
            }
            return FALSE;
        default:
            return FALSE;
    }
}

s8 GetSingleUseAbilityCounter(u8 battler, u16 ability) {
    
    switch (BattlerHasInnateOrAbility(battler, ability))
    {
        case BATTLER_INNATE:
            return gBattleStruct->singleuseability[gBattlerPartyIndexes[battler]][GetBattlerInnateNum(battler, ability) + 1][GetBattlerSide(battler)];
        case BATTLER_ABILITY:
            return gBattleStruct->singleuseability[gBattlerPartyIndexes[battler]][0][GetBattlerSide(battler)];
        default:
            return -1;
    }
}

void SetSingleUseAbilityCounter(u8 battler, u16 ability, u8 value) {
    
    switch (BattlerHasInnateOrAbility(battler, ability))
    {
        case BATTLER_INNATE:
            gBattleStruct->singleuseability[gBattlerPartyIndexes[battler]][GetBattlerInnateNum(battler, ability) + 1][GetBattlerSide(battler)] = value;
            return;
        case BATTLER_ABILITY:
            gBattleStruct->singleuseability[gBattlerPartyIndexes[battler]][0][GetBattlerSide(battler)] = value;
            return;
        default:
            return;
    }
}

void IncrementSingleUseAbilityCounter(u8 battler, u16 ability, u8 value) {
    SetSingleUseAbilityCounter(battler, ability, GetSingleUseAbilityCounter(battler, ability) + value);
}

union AbilityStates GetAbilityStateAs(u8 battler, u16 ability)
{
    return (union AbilityStates) { .intValue = GetAbilityState(battler, ability) };
}

u32 GetAbilityState(u8 battler, u16 ability) {
    
    switch (BattlerHasInnateOrAbility(battler, ability))
    {
        case BATTLER_INNATE:
            return gVolatileStructs[battler].abilityState[GetBattlerInnateNum(battler, ability) + 1];
        case BATTLER_ABILITY:
            return gVolatileStructs[battler].abilityState[0];
        default:
            return 0;
    }
}

void SetAbilityStateAs(u8 battler, u16 ability, union AbilityStates value)
{
    SetAbilityState(battler, ability, value.intValue);
}

void SetAbilityState(u8 battler, u16 ability, u32 value) {
    
    switch (BattlerHasInnateOrAbility(battler, ability))
    {
        case BATTLER_INNATE:
            gVolatileStructs[battler].abilityState[GetBattlerInnateNum(battler, ability) + 1] = value;
            return;
        case BATTLER_ABILITY:
            gVolatileStructs[battler].abilityState[0] = value;
            return;
        default:
            return;
    }
}

void IncrementAbilityState(u8 battler, u16 ability, u32 value) {
    SetAbilityState(battler, ability, GetAbilityState(battler, ability) + value);
}

static bool8 UseEntryMove(u8 battler, u16 ability, u16 extraMove, u8 movePower) {
    int target = gBattlersCount;
    u8 i;
    u8 opposingBattler = BATTLE_OPPOSITE(battler);
    
    if (!CheckAndSetSwitchInAbility(battler, ability)) return FALSE;
    
    //Checks Target
    for (i = 0; i < 2; opposingBattler ^= BIT_FLANK, i++)
    {
        if (IsBattlerAlive(opposingBattler) && gBattleMons[opposingBattler].hp != 1)
        {
            target = opposingBattler;
            break;
        }
    }

    //This is the stuff that has to be changed for each ability
    if(target < gBattlersCount && CanUseExtraMove(battler, target)){
        gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct) {
            .ability = ability,
            .attacker = battler,
            .move = extraMove,
            .movePower = movePower,
            .target = target,
        };
        return TRUE;
    }
    return FALSE;
}

static u16 UseAttackerFollowUpMove(u8 battler, int target, u16 ability, u16 extraMove, u8 movePower)
{
    gRoundStructs[battler].extraMoveUsed = TRUE;

    gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct) {
        .ability = ability,
        .attacker = battler,
        .target = target,
        .move = extraMove,
        .movePower = movePower,
        .movePos = MAX_MON_MOVES,
    };

    return TRUE;
}

static int AbilityHealMonStatus(u8 battler, u16 ability) {
    if (!(gBattleMons[battler].status1 & STATUS1_ANY)) return FALSE;

    if (gBattleMons[battler].status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON))
        StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
    if (gBattleMons[battler].status1 & STATUS1_SLEEP)
        StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
    if (gBattleMons[battler].status1 & STATUS1_PARALYSIS)
        StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
    if (gBattleMons[battler].status1 & STATUS1_BURN)
        StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
    if (gBattleMons[battler].status1 & (STATUS1_FREEZE | STATUS1_FROSTBITE))
        StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
    if (gBattleMons[battler].status1 & STATUS1_BLEED)
        StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);

    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ability;
    gBattleMons[battler].status1 = 0;
    gBattleMons[battler].status2 &= ~(STATUS2_NIGHTMARE);
    gBattleScripting.battler = gActiveBattler = battler;
    BattleScriptPushCursorAndCallback(BattleScript_ShedSkinActivates);
    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
    MarkBattlerForControllerExec(gActiveBattler);
    return TRUE;
}

static bool8 CanMoveHaveExtraFlinchChance(u16 move)
{
    switch (gBattleMoves[move].effect)
    {
        case EFFECT_FLINCH_HIT:
        case EFFECT_FAKE_OUT:
        case EFFECT_FLINCH_STATUS:
        case EFFECT_FLINCH_RECOIL_33:
        case EFFECT_FLINCH_RECOIL_50:
        case EFFECT_FLINCH_MINIMIZE_HIT:
        case EFFECT_FLING:
        case EFFECT_SECRET_POWER:
        case EFFECT_SNORE:
        case EFFECT_TWISTER:
            return gBattleMoves[move].secondaryEffectChance > 0;
            
        case EFFECT_TRIPLE_ARROWS:
            return FALSE;

        default:
            return TRUE;
    }
    return TRUE;
}

int WasMoveSuccessful()
{
    return !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
		&& !gRoundStructs[gBattlerAttacker].confusionSelfDmg
        && gBattlerAttacker != gBattlerTarget;
}

int DidMoveHit()
{
    return WasMoveSuccessful() && TARGET_TURN_DAMAGED;
			 
}

int ShouldApplyOnHitAffect(int applyTo)
{
    return DidMoveHit() && IsBattlerAlive(applyTo);
}

bool8 UseIntimidateClone(u8 battler, u16 abilityToCheck)
{
    u8 numAbility, numStats, statToLower, i, target;
    bool8 canLowerStat = FALSE;
    
    for(numAbility = 0; i < NUM_INTIMIDATE_CLONES; i++){
        if(gIntimidateCloneData[numAbility].ability == abilityToCheck)
            break;
    }

    if (numAbility >= NUM_INTIMIDATE_CLONES) return FALSE;
    
    numStats = gIntimidateCloneData[numAbility].numStatsLowered;

    for(i = 0; i < numStats; i++){
        statToLower = gIntimidateCloneData[numAbility].statsLowered[i];
        target = BATTLE_OPPOSITE(battler);
        //Target 1
        if(!IsBattlerImmuneToLowerStatsFromIntimidateClone(target, statToLower, abilityToCheck)){
            canLowerStat = TRUE;
            break;
        }

        //Target 2
        if(IsDoubleBattle()){
            target = BATTLE_PARTNER(target);
            if(!IsBattlerImmuneToLowerStatsFromIntimidateClone(target, statToLower, abilityToCheck)){
                canLowerStat = TRUE;
                break;
            }
        }
    }

    if (canLowerStat) { //Ability effect can be triggered
        BattleScriptPushCursorAndCallback(BattleScript_IntimidateActivatedNew);
        return TRUE;
    }

    return FALSE;
}

u8 getMonotypeChampType(void){
    if(VarGet(VAR_BATTLE_FIELD_EFFECT_TYPE) == BATTLE_FIELD_EFFECT_MONOCHAMP)
        return VarGet(VAR_BATTLE_FIELD_ID);
    else
        return TYPE_NONE;
}

bool8 TryToSetMonotypeChampEffect(u8 battler){
    //Mainly to announce the effect
    u16 champType = getMonotypeChampType();

    switch(champType){
        case TYPE_NORMAL:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Normal);
            return TRUE;
        break;
        case TYPE_FIGHTING:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Fighting);
            return TRUE;
        break;
        case TYPE_FLYING:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Flying);
            return TRUE;
        break;
        case TYPE_POISON:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Poison);
            return TRUE;
        break;
        case TYPE_GROUND:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Ground);
            return TRUE;
        break;
        case TYPE_ROCK:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Rock);
            return TRUE;
        break;
        case TYPE_BUG:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Bug);
            return TRUE;
        break;
        case TYPE_GHOST:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Ghost);
            return TRUE;
        break;
        case TYPE_STEEL:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Steel);
            return TRUE;
        break;
        case TYPE_FIRE:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Fire);
            return TRUE;
        break;
        case TYPE_WATER:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Water);
            return TRUE;
        break;
        case TYPE_GRASS:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Grass);
            return TRUE;
        break;
        case TYPE_ELECTRIC:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Electric);
            return TRUE;
        break;
        case TYPE_PSYCHIC:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Psychic);
            return TRUE;
        break;
        case TYPE_ICE:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Ice);
            return TRUE;
        break;
        case TYPE_DRAGON:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Dragon);
            return TRUE;
        break;
        case TYPE_DARK:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Dark);
            return TRUE;
        break;
        case TYPE_FAIRY:
            BattleScriptPushCursorAndCallback(BattleScript_SetMonotypeEffect_Fairy);
            return TRUE;
        break;
    }
    return FALSE;
}

bool8 TryToSetFieldEffect(u8 battler){
    u16 effect = VarGet(VAR_BATTLE_FIELD_EFFECT_TYPE);
    u16 fieldEffectId = VarGet(VAR_BATTLE_FIELD_ID);
    bool8 isTemporary = FALSE;

    //VarSet(VAR_BATTLE_FIELD_EFFECT_TYPE, 0);
    //VarSet(VAR_BATTLE_FIELD_ID, 0);
    
    switch(effect){
        //Weather
        case BATTLE_FIELD_EFFECT_WEATHER:
            switch (fieldEffectId)
            {
            case WEATHER_RAIN_ANY:
                if (TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN, isTemporary))
                {
                    BattleScriptPushCursorAndCallback(BattleScript_SetRainFromScript);
                    return TRUE;
                }
                else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
                {
                    BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                    return TRUE;
                }
                break;
            case WEATHER_SANDSTORM_ANY:
                if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, isTemporary))
                {
                    BattleScriptPushCursorAndCallback(BattleScript_SetSandstormFromScript);
                    return TRUE;
                }
                else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
                {
                    BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                    return TRUE;
                }
                break;
            case WEATHER_SUN_ANY:
                if (TryChangeBattleWeather(battler, ENUM_WEATHER_SUN, isTemporary))
                {
                    BattleScriptPushCursorAndCallback(BattleScript_SetSunFromScript);
                    return TRUE;
                }
                else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
                {
                    BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                    return TRUE;
                }
                break;
            case WEATHER_HAIL_ANY:
                if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, isTemporary))
                {
                    BattleScriptPushCursorAndCallback(BattleScript_SetHailFromScript);
                    return TRUE;
                }
                else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
                {
                    BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                    return TRUE;
                }
                break;
            /*case WEATHER_STRONG_WINDS:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
                break;*/
            }
        break;
        //Terrain
        case BATTLE_FIELD_EFFECT_TERRAIN:
            if(fieldEffectId & STATUS_FIELD_TERRAIN_ANY){
                u16 terrainFlags = fieldEffectId & STATUS_FIELD_TERRAIN_ANY; // only works for status flag (1 << 15)
                gFieldStatuses = terrainFlags | STATUS_FIELD_TERRAIN_PERMANENT;            // terrain is permanent
                switch (fieldEffectId & STATUS_FIELD_TERRAIN_ANY)
                {
                case STATUS_FIELD_ELECTRIC_TERRAIN:
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
                    break;
                case STATUS_FIELD_MISTY_TERRAIN:
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
                    break;
                case STATUS_FIELD_GRASSY_TERRAIN:
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
                    break;
                case STATUS_FIELD_PSYCHIC_TERRAIN:
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
                    break;
                }
                BattleScriptPushCursorAndCallback(BattleScript_OverworldTerrain);
                return TRUE;
            }
            break;
        //Room
        case BATTLE_FIELD_EFFECT_ROOM:
            switch (fieldEffectId){
            case STATUS_FIELD_TRICK_ROOM:
                if(!(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)){
                    //Enable Trick Room
                    gFieldTimers.started.trickRoom = TRUE;
                    gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
                    if(isTemporary)
                        gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION;
                    else
                        gFieldTimers.trickRoomTimer = ROOM_DURATION_MAX;
                    BattleScriptPushCursorAndCallback(BattleScript_SetTrickRoomFromScript);
                    return TRUE;
                }
            break;
            case STATUS_FIELD_MAGIC_ROOM:
                if(!(gFieldStatuses & STATUS_FIELD_MAGIC_ROOM)){
                    //Enable Magic Room
                    gFieldTimers.started.magicRoom = TRUE;
                    gFieldStatuses |= STATUS_FIELD_MAGIC_ROOM;
                    if(isTemporary)
                        gFieldTimers.magicRoomTimer= MAGIC_ROOM_DURATION;
                    else
                        gFieldTimers.magicRoomTimer = ROOM_DURATION_MAX;
                    BattleScriptPushCursorAndCallback(BattleScript_SetMagicRoomFromScript);
                    return TRUE;
                }
            break;
            case STATUS_FIELD_WONDER_ROOM:
                if(!(gFieldStatuses & STATUS_FIELD_WONDER_ROOM)){
                    //Enable Wonder Room
                    gFieldTimers.started.wonderRoom = TRUE;
                    gFieldStatuses |= STATUS_FIELD_WONDER_ROOM;
                    if(isTemporary)
                        gFieldTimers.wonderRoomTimer = WONDER_ROOM_DURATION;
                    else
                        gFieldTimers.wonderRoomTimer = ROOM_DURATION_MAX;
                    BattleScriptPushCursorAndCallback(BattleScript_SetTrickRoomFromScript);
                    return TRUE;
                }
            break;
            case STATUS_FIELD_INVERSE_ROOM:
                if(!(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)){
                    //Enable Inverse Room
                    gFieldTimers.started.inverseRoom = TRUE;
                    gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
                    if(isTemporary)
                        gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION;
                    else
                        gFieldTimers.inverseRoomTimer = ROOM_DURATION_MAX;
                    BattleScriptPushCursorAndCallback(BattleScript_SetInverseRoomFromScript);
                    return TRUE;
                }
            break;
            case STATUS_FIELD_GRAVITY:
                if(!(gFieldStatuses & STATUS_FIELD_GRAVITY)){
                    //Enable Trick Room
                    gFieldStatuses |= STATUS_FIELD_GRAVITY;
                    gFieldTimers.started.gravity = TRUE;
                    if(isTemporary)
                        gFieldTimers.gravityTimer = GRAVITY_DURATION;
                    else
                        gFieldTimers.gravityTimer = ROOM_DURATION_MAX;
                    BattleScriptPushCursorAndCallback(BattleScript_SetGravityFromScript);
                    return TRUE;
                }
            break;
            }
        break;
        case BATTLE_FIELD_EFFECT_MONOCHAMP:
            if(TryToSetMonotypeChampEffect(battler))
                return TRUE;
        break;
        //Other
        default:
            if (GetCurrentWeather() == WEATHER_RAIN_THUNDERSTORM && !(gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN))
            {
                // overworld weather started rain, so just do electric terrain anim
                gFieldStatuses = (STATUS_FIELD_ELECTRIC_TERRAIN | STATUS_FIELD_TERRAIN_PERMANENT);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
                BattleScriptPushCursorAndCallback(BattleScript_OverworldTerrain);
                return TRUE;
            }
        break;
    }

    return FALSE;
}

u8 AbilityBattleEffects(u8 caseID, u8 battler, u16 ability, u8 special, u16 moveArg)
{
    u8 effect = 0;
    u32 speciesAtk, speciesDef;
    u32 pidAtk, pidDef;
    u32 moveType, move;
    u32 i, j;
    u16 trainerNum;
    bool8 abilityEffect = FALSE;
    u8 opponent;
    u16 effectTargetFlag;

    if (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
        return 0;

    if (gBattlerAttacker >= gBattlersCount)
        gBattlerAttacker = battler;

    speciesAtk = gBattleMons[gBattlerAttacker].species;
    pidAtk = gBattleMons[gBattlerAttacker].personality;

    speciesDef = gBattleMons[gBattlerTarget].species;
    pidDef = gBattleMons[gBattlerTarget].personality;

    if (special)
        gLastUsedAbility = special;
    else {
        gLastUsedAbility = GetBattlerAbility(battler);
        abilityEffect = TRUE;
    }

    if (moveArg)
        move = moveArg;
    else
        move = gCurrentMove;

    GET_MOVE_TYPE(move, moveType);

    switch (caseID)
    {
    case ABILITYEFFECT_SWITCH_IN_TERRAIN:
        if(TryToSetFieldEffect(battler))
            effect++;
        break;
    case ABILITYEFFECT_SWITCH_IN_WEATHER:
        if (!(gBattleTypeFlags & BATTLE_TYPE_RECORDED))
        {
            switch (GetCurrentWeather())
            {
            case WEATHER_RAIN:
            case WEATHER_RAIN_THUNDERSTORM:
            case WEATHER_DOWNPOUR:
                if (!(gBattleWeather & WEATHER_RAIN_ANY))
                {
                    gBattleWeather = (WEATHER_RAIN_TEMPORARY | WEATHER_RAIN_PERMANENT);
                    gBattleScripting.animArg1 = B_ANIM_RAIN_CONTINUES;
                    effect++;
                }
                break;
            case WEATHER_SANDSTORM:
                if (!(gBattleWeather & WEATHER_SANDSTORM_ANY))
                {
                    gBattleWeather = (WEATHER_SANDSTORM_PERMANENT | WEATHER_SANDSTORM_TEMPORARY);
                    gBattleScripting.animArg1 = B_ANIM_SANDSTORM_CONTINUES;
                    effect++;
                }
                break;
            case WEATHER_DROUGHT:
                if (!(gBattleWeather & WEATHER_SUN_ANY))
                {
                    gBattleWeather = (WEATHER_SUN_PERMANENT | WEATHER_SUN_TEMPORARY);
                    gBattleScripting.animArg1 = B_ANIM_SUN_CONTINUES;
                    effect++;
                }
                break;
            //case WEATHER_FOG_DIAGONAL:  // added to Lavaridge Gym; doesn't trigger WEATHER_FOG_PERMANENT
            case WEATHER_FOG_HORIZONTAL:
                if (!(gBattleWeather & WEATHER_FOG_PERMANENT))
                {
                    gBattleWeather = WEATHER_FOG_PERMANENT;
                    gBattleScripting.animArg1 = B_ANIM_FOG_CONTINUES;
                    effect++;
                }
                break;
            }
        }
        if (effect)
        {
            gBattleCommunication[MULTISTRING_CHOOSER] = GetCurrentWeather();
            BattleScriptPushCursorAndCallback(BattleScript_OverworldWeatherStarts);
        }
        break;
    case ABILITYEFFECT_ON_SWITCHIN: // 0
        gBattleScripting.battler = battler;

        for (i = 0; i <= NUM_ABILITY_SLOTS + 1; i++)
        {
            effect += HandleSwitchInAbility(i, battler);
        }
        
        break;
    case ABILITYEFFECT_ENDTURN: // 1
        for (i = 0; i <= NUM_ABILITY_SLOTS + 1; i++)
        {
            HandleEndTurnAbility(i, battler);
        }
    case ABILITYEFFECT_MOVES_BLOCK: // 2
        effect = TestImmunityAbilities(battler, gBattlerAttacker, move, moveType, &gBattlescriptCurrInstr, &gBattleScripting.battlerPopupOverwrite, &gBattleScripting.abilityPopupOverwrite);


        if (effect)
        {
            SetActiveAbilityPopupOverride(gBattleScripting.abilityPopupOverwrite);
            if (gBattleScripting.battlerPopupOverwrite == BATTLE_PARTNER(battler))
                gBattleScripting.battler = BATTLE_PARTNER(battler);
            else
                gBattleScripting.battler = battler;
        }

        if (effect == 1)
        {
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS){
                gHitMarker |= HITMARKER_NO_PPDEDUCT;
                gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_MULTIPLETURNS);
            }

            if (gStatuses3[gBattlerAttacker] & STATUS3_SEMI_INVULNERABLE)
                gStatuses3[gBattlerAttacker] &= ~(STATUS3_SEMI_INVULNERABLE);
        }
        else if (effect == 2)
        {
            if (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS)
                gHitMarker |= HITMARKER_NO_PPDEDUCT;
        }

        
        break;
    case ABILITYEFFECT_ABSORBING: // 3
        if (move != MOVE_NONE)
        {
            int statId;
			
			effect = TestAbsorbingAbilities(battler, gBattlerAttacker, move, moveType, &statId, &gBattleScripting.abilityPopupOverwrite);

            if (effect == 1) // Drain Hp ability.
            {
                SetActiveAbilityPopupOverride(gBattleScripting.abilityPopupOverwrite);
                if (BATTLER_MAX_HP(battler) || BATTLER_HEALING_BLOCKED(battler))
                {
                    if ((gRoundStructs[gBattlerAttacker].notFirstStrike))
                        gBattlescriptCurrInstr = BattleScript_MonMadeMoveUseless;
                    else
                        gBattlescriptCurrInstr = BattleScript_MonMadeMoveUseless_PPLoss;
                }
                else
                {
                    if (gRoundStructs[gBattlerAttacker].notFirstStrike)
                        gBattlescriptCurrInstr = BattleScript_MoveHPDrain;
                    else
                        gBattlescriptCurrInstr = BattleScript_MoveHPDrain_PPLoss;

                    gBattleMoveDamage = gBattleMons[battler].maxHP / 4;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    gBattleMoveDamage *= -1;
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
                }
            }
            else if (effect == 2) // Boost Stat ability;
            {
                SetActiveAbilityPopupOverride(gBattleScripting.abilityPopupOverwrite);
                if (!CompareStat(battler, statId, MAX_STAT_STAGE, CMP_LESS_THAN))
                {
                    if ((gRoundStructs[gBattlerAttacker].notFirstStrike))
                        gBattlescriptCurrInstr = BattleScript_MonMadeMoveUseless;
                    else
                        gBattlescriptCurrInstr = BattleScript_MonMadeMoveUseless_PPLoss;
                }
                else
                {
                    if (gRoundStructs[gBattlerAttacker].notFirstStrike)
                        gBattlescriptCurrInstr = BattleScript_MoveStatDrain;
                    else
                        gBattlescriptCurrInstr = BattleScript_MoveStatDrain_PPLoss;

                    SET_STATCHANGER(statId, 1, FALSE);
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
                    ChangeStatBuffs(battler, StatBuffValue(1), statId, MOVE_EFFECT_AFFECTS_USER, NULL);
                    PREPARE_STAT_BUFFER(gBattleTextBuff1, statId);
                }
            }
            else if (effect == 3) // Flash Fire special case
            {
                if (!(gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE))
                {
                    SetActiveMultistringChooser(B_MSG_FLASH_FIRE_BOOST);
                    if (gRoundStructs[gBattlerAttacker].notFirstStrike)
                        gBattlescriptCurrInstr = BattleScript_FlashFireBoost;
                    else
                        gBattlescriptCurrInstr = BattleScript_FlashFireBoost_PPLoss;

                    gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_FLASH_FIRE;
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
                }
                else
                {
                    SetActiveMultistringChooser(B_MSG_FLASH_FIRE_NO_BOOST);
                    if (gRoundStructs[gBattlerAttacker].notFirstStrike)
                        gBattlescriptCurrInstr = BattleScript_FlashFireBoost;
                    else
                        gBattlescriptCurrInstr = BattleScript_FlashFireBoost_PPLoss;
                        
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
                }
            }
        }
        break;
    case ABILITYEFFECT_MOVE_END: // Think contact abilities.
        for (i = 0; i <= NUM_ABILITY_SLOTS + 1; i++)
        {
            effect += HandleDefenderAbility(i, battler, gBattlerAttacker, move);
        }
        break;
        
    case ABILITYEFFECT_MOVE_END_ATTACKER: // Same as above, but for attacker
        for (i = 0; i <= NUM_ABILITY_SLOTS + 1; i++)
        {
            effect += HandleAttackerAbility(i, battler, gBattlerTarget, move);
        }
        break;

    case ABILITYEFFECT_MOVE_END_OTHER: // Abilities that activate on *another* battler's moveend: Dancer, Soul-Heart, Receiver, Symbiosis
        // Dancer
        if(BATTLER_HAS_ABILITY(battler, ABILITY_DANCER)
            && !(gBattleStruct->lastMoveFailed & 1 << gBattlerAttacker)
            && (gBattleMoves[gCurrentMove].flags & FLAG_DANCE))
        {
            u8 target = GetBattlerSide(gBattlerTarget) == GetBattlerSide(gBattlerAttacker) ? gBattlerTarget : BATTLE_OPPOSITE(battler);
            if (UseOutOfTurnAttack(battler, target, ABILITY_DANCER, gCurrentMove, 0))
                return TRUE;
        }
        else if (BattlerHasAbility(battler, battler, ABILITY_PARROTING)
            && (gBattleMoves[gCurrentMove].flags & FLAG_SOUND))
        {
            u8 target = GetBattlerSide(gBattlerTarget) == GetBattlerSide(gBattlerAttacker) ? gBattlerTarget : BATTLE_OPPOSITE(battler);
            if (UseOutOfTurnAttack(battler, target, ABILITY_PARROTING, gCurrentMove, 0))
                return TRUE;
        }
        else if (BattlerHasAbility(battler, battler, ABILITY_LUNAR_AFFINITY) && gBattleMoves[gCurrentMove].lunar)
        {
            u8 target = GetBattlerSide(gBattlerTarget) == GetBattlerSide(gBattlerAttacker) ? gBattlerTarget : BATTLE_OPPOSITE(battler);
            if (UseOutOfTurnAttack(battler, target, ABILITY_LUNAR_AFFINITY, gCurrentMove, 0))
                return TRUE;
        }
        else if(BATTLER_HAS_ABILITY(battler, ABILITY_RETRIBUTION_BLOW)){
            if(GetBattlerSide(battler) != GetBattlerSide(gBattlerAttacker)
                && IsBattlerAlive(gBattlerAttacker)
                && DoesMoveBoostStats(gCurrentMove)){
                    if (UseOutOfTurnAttack(battler, gBattlerAttacker, ABILITY_RETRIBUTION_BLOW, MOVE_HYPER_BEAM, 0))
                        return TRUE;
            }
        }
        
        break;
    case ABILITYEFFECT_IMMUNITY: // 5
        for (battler = 0; battler < gBattlersCount; battler++)
        {

            //Innates
            //Purifying Salt
            if (BATTLER_HAS_ABILITY(battler, ABILITY_PURIFYING_SALT))
            {
                if (gBattleMons[battler].status1 & STATUS1_ANY)
                {
                    u32 status1 = gBattleMons[battler].status1;
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_PURIFYING_SALT;
                    if (status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON)) StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                    else if (status1 & STATUS1_BLEED) StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                    else if (status1 & STATUS1_BURN) StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    else if (status1 & STATUS1_FROSTBITE) StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                    else if (status1 & STATUS1_SLEEP) StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                    else StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                    effect = 1;
                }
            }
            if (BATTLER_HAS_ABILITY(battler, ABILITY_BLOOD_STIGMA))
            {
                if (gBattleMons[battler].status1 & STATUS1_ANY)
                {
                    u32 status1 = gBattleMons[battler].status1;
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_BLOOD_STIGMA;
                    if (status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON)) StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                    else if (status1 & STATUS1_BLEED) StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                    else if (status1 & STATUS1_BURN) StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    else if (status1 & STATUS1_FROSTBITE) StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                    else if (status1 & STATUS1_SLEEP) StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                    else StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                    effect = 1;
                }
            }

            if (BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE) && gBattleMons[battler].status1) {
                int status1 = gBattleMons[battler].status1;
                gBattleScripting.abilityPopupOverwrite = ABILITY_COMATOSE;
                if (status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON)) StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                else if (status1 & STATUS1_BLEED) StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                else if (status1 & STATUS1_BURN) StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                else if (status1 & STATUS1_FROSTBITE) StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                else if (status1 & STATUS1_SLEEP) StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                else StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                effect = 1;
            }

            if (BATTLER_HAS_ABILITY(battler, ABILITY_BLOOD_STAIN) && gBattleMons[battler].status1) {
                int status1 = gBattleMons[battler].status1;
                gBattleScripting.abilityPopupOverwrite = ABILITY_COMATOSE;
                if (status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON)) StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                else if (status1 & STATUS1_BLEED) StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                else if (status1 & STATUS1_BURN) StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                else if (status1 & STATUS1_FROSTBITE) StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                else if (status1 & STATUS1_SLEEP) StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                else StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                effect = 1;
            }

            //Immunity
            if(BATTLER_HAS_ABILITY(battler, ABILITY_IMMUNITY)){
                if (gBattleMons[battler].status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON | STATUS1_TOXIC_COUNTER))
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_IMMUNITY;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                    effect = 1;
                }
            }

            //Own Tempo
            if(BATTLER_HAS_ABILITY(battler, ABILITY_OWN_TEMPO)){
                if (gBattleMons[battler].status2 & STATUS2_CONFUSION)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_OWN_TEMPO;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_ConfusionJpn);
                    effect = 2;
                }
            }

            //Discipline
            if(BATTLER_HAS_ABILITY(battler, ABILITY_DISCIPLINE)){
                if (gBattleMons[battler].status2 & STATUS2_CONFUSION)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_DISCIPLINE;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_ConfusionJpn);
                    effect = 2;
                }
            }

            //Limber
            if(BATTLER_HAS_ABILITY(battler, ABILITY_LIMBER)){
                if (gBattleMons[battler].status1 & STATUS1_PARALYSIS)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_LIMBER;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                    effect = 1;
                }
            }

            //Insomnia
            if(BATTLER_HAS_ABILITY(battler, ABILITY_INSOMNIA)){
                if (gBattleMons[battler].status1 & STATUS1_SLEEP)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_INSOMNIA;
                    gBattleMons[battler].status2 &= ~(STATUS2_NIGHTMARE);
                    StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                    effect = 1;
                }
            }

            //Vital Spirit
            if(BATTLER_HAS_ABILITY(battler, ABILITY_VITAL_SPIRIT)){
                if (gBattleMons[battler].status1 & STATUS1_SLEEP)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_VITAL_SPIRIT;
                    gBattleMons[battler].status2 &= ~(STATUS2_NIGHTMARE);
                    StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                    effect = 1;
                }
            }

            //Water Veil
            if(BATTLER_HAS_ABILITY(battler, ABILITY_WATER_VEIL)){
                if (gBattleMons[battler].status1 & STATUS1_BURN)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_WATER_VEIL;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    effect = 1;
                }
            }

            //Purifying Waters
            if(BATTLER_HAS_ABILITY(battler, ABILITY_PURIFYING_WATERS)){
                if (gBattleMons[battler].status1 & STATUS1_BURN)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_PURIFYING_WATERS;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    effect = 1;
                }
            }

            //Water Bubble
            if(BATTLER_HAS_ABILITY(battler, ABILITY_WATER_BUBBLE)){
                if (gBattleMons[battler].status1 & STATUS1_BURN)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_WATER_BUBBLE;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    effect = 1;
                }
            }
            
            //Flame Bubble (i'll prob merge the code with water bubble later)
            if(BATTLER_HAS_ABILITY(battler, ABILITY_FLAME_BUBBLE)){
                if (gBattleMons[battler].status1 & STATUS1_BURN)
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FLAME_BUBBLE;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    effect = 1;
                }
            }

            //Magma Armor
            if(BATTLER_HAS_ABILITY(battler, ABILITY_MAGMA_ARMOR)){
                if (gBattleMons[battler].status1 & (STATUS1_FREEZE | STATUS1_FROSTBITE))
                {
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_MAGMA_ARMOR;
                    StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                    effect = 1;
                }
            }

            //Oblivious
            if(BATTLER_HAS_ABILITY(battler, ABILITY_OBLIVIOUS)){
                if (gBattleMons[battler].status2 & STATUS2_INFATUATION){
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_OBLIVIOUS;
                    effect = 3;
                }
                else if (gVolatileStructs[battler].tauntTimer != 0){
                    gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_OBLIVIOUS;
                    effect = 4;
                }
            }

            if (effect)
            {
                switch (effect)
                {
                case 1: // status cleared
                    gBattleMons[battler].status1 = 0;
                    BattleScriptCall(BattleScript_AbilityCuredStatus);
                    break;
                case 2: // get rid of confusion
                    gBattleMons[battler].status2 &= ~(STATUS2_CONFUSION);
                    BattleScriptCall(BattleScript_AbilityCuredStatus);
                    break;
                case 3: // get rid of infatuation
                    gBattleMons[battler].status2 &= ~(STATUS2_INFATUATION);
                    BattleScriptCall(BattleScript_BattlerGotOverItsInfatuation);
                    break;
                case 4: // get rid of taunt
                    gVolatileStructs[battler].tauntTimer = 0;
                    BattleScriptCall(BattleScript_BattlerShookOffTaunt);
                    break;
                }

                gBattleScripting.battler = gActiveBattler = gBattlerAbility = battler;
                BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                MarkBattlerForControllerExec(gActiveBattler);
                return effect;
            }
			
			//Vital Spirit & Insomnia
			if(BATTLER_HAS_ABILITY(battler, ABILITY_INSOMNIA) || 
               BATTLER_HAS_ABILITY(battler, ABILITY_VITAL_SPIRIT)){
                if (gBattleMons[battler].status1 & STATUS1_SLEEP)
                {
                    gBattleMons[battler].status2 &= ~(STATUS2_NIGHTMARE);
                    StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                    effect = 1;
                }
			}
			
			//Oblivious
			if(BATTLER_HAS_ABILITY(battler, ABILITY_OBLIVIOUS)){
                if (gBattleMons[battler].status2 & STATUS2_INFATUATION)
                    effect = 3;
                else if (gVolatileStructs[battler].tauntTimer != 0)
                    effect = 4;
			}
			
			// Water Veil & Water Bubble
			if(BATTLER_HAS_ABILITY(battler, ABILITY_WATER_VEIL) || 
               BATTLER_HAS_ABILITY(battler, ABILITY_WATER_BUBBLE) || 
               BATTLER_HAS_ABILITY(battler, ABILITY_FLAME_BUBBLE) ||
               BATTLER_HAS_ABILITY(battler, ABILITY_PURIFYING_WATERS)){
                if (gBattleMons[battler].status1 & STATUS1_BURN)
                {
                    StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    effect = 1;
                }
			}
			
        }
        break;
    case ABILITYEFFECT_FORECAST: // 6
        for (battler = 0; battler < gBattlersCount; battler++)
        {
            if (BATTLER_HAS_ABILITY(battler, ABILITY_FORECAST) || BATTLER_HAS_ABILITY(battler, ABILITY_FLOWER_GIFT))
            {
                if (ShouldChangeFormHpBased(battler))
                {
                    BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
                    return effect;
                }
            }
        }
        break;
    case ABILITYEFFECT_SYNCHRONIZE:
        if (gLastUsedAbility == ABILITY_SYNCHRONIZE && (gHitMarker & HITMARKER_SYNCHRONISE_EFFECT))
        {
            gHitMarker &= ~(HITMARKER_SYNCHRONISE_EFFECT);

            if (!(gBattleMons[gBattlerAttacker].status1 & STATUS1_ANY))
            {
                gBattleStruct->synchronizeMoveEffect &= ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                #if B_SYNCHRONIZE_TOXIC < GEN_5
                    if (gBattleStruct->synchronizeMoveEffect == MOVE_EFFECT_TOXIC)
                        gBattleStruct->synchronizeMoveEffect = MOVE_EFFECT_POISON;
                #endif

                gBattleScripting.moveEffect = gBattleStruct->synchronizeMoveEffect + MOVE_EFFECT_AFFECTS_USER;
                gBattleScripting.battler = gBattlerAbility = gBattlerTarget;
                PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ABILITY_SYNCHRONIZE);
                BattleScriptCall(BattleScript_SynchronizeActivates);
                gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
                effect++;
            }
        }
        break;
    case ABILITYEFFECT_ATK_SYNCHRONIZE: // 8
        if (gLastUsedAbility == ABILITY_SYNCHRONIZE && (gHitMarker & HITMARKER_SYNCHRONISE_EFFECT))
        {
            gHitMarker &= ~(HITMARKER_SYNCHRONISE_EFFECT);

            if (!(gBattleMons[gBattlerTarget].status1 & STATUS1_ANY))
            {
                gBattleStruct->synchronizeMoveEffect &= ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                if (gBattleStruct->synchronizeMoveEffect == MOVE_EFFECT_TOXIC)
                    gBattleStruct->synchronizeMoveEffect = MOVE_EFFECT_POISON;

                gBattleScripting.moveEffect = gBattleStruct->synchronizeMoveEffect;
                gBattleScripting.battler = gBattlerAbility = gBattlerAttacker;
                PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ABILITY_SYNCHRONIZE);
                BattleScriptCall(BattleScript_SynchronizeActivates);
                gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
                effect++;
            }
        }
        break;
    case ABILITYEFFECT_TRACE1:
    case ABILITYEFFECT_TRACE2:
        for (i = 0; i < gBattlersCount; i++)
        {
            if (gBattleMons[i].ability == ABILITY_TRACE && (gBattleResources->flags->flags[i] & RESOURCE_FLAG_TRACED))
            {
                u8 side = (GetBattlerPosition(i) ^ BIT_SIDE) & BIT_SIDE; // side of the opposing pokemon
                u8 target1 = GetBattlerAtPosition(side);
                u8 target2 = GetBattlerAtPosition(side + BIT_FLANK);

                if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                {
                    if (!IsRolePlayBannedAbility(gBattleMons[target1].ability) && gBattleMons[target1].hp != 0
                     && !IsRolePlayBannedAbility(gBattleMons[target2].ability) && gBattleMons[target2].hp != 0)
                        gStackBattler2 = GetBattlerAtPosition(((Random() & 1) * 2) | side), effect++;
                    else if (!IsRolePlayBannedAbility(gBattleMons[target1].ability) && gBattleMons[target1].hp != 0)
                        gStackBattler2 = target1, effect++;
                    else if (!IsRolePlayBannedAbility(gBattleMons[target2].ability) && gBattleMons[target2].hp != 0)
                        gStackBattler2 = target2, effect++;
                }
                else
                {
                    if (!IsRolePlayBannedAbility(gBattleMons[target1].ability) && gBattleMons[target1].hp != 0)
                        gStackBattler2 = target1, effect++;
                }

                if (effect)
                {
                    gBattleResources->flags->flags[i] &= ~(RESOURCE_FLAG_TRACED);
                    gBattleStruct->tracedAbility[i] = gLastUsedAbility = gBattleMons[gStackBattler2].ability;
                    gStackBattler1 = i;
                    
                    PREPARE_ABILITY_BUFFER(gBattleTextBuff1, gLastUsedAbility)

                    if (caseID == ABILITYEFFECT_TRACE1)
                    {
                        BattleScriptPushCursorAndCallback(BattleScript_TraceActivatesEnd3);
                    }
                    else
                    {
                        BattleScriptCall(BattleScript_TraceActivates);
                    }
                    break;
                }
            }
        }
        break;
    case ABILITYEFFECT_NEUTRALIZINGGAS:
        // Prints message only. separate from ABILITYEFFECT_ON_SWITCHIN bc activates before entry hazards
        for (i = 0; i < gBattlersCount; i++)
        {
            if (gBattleMons[i].ability == ABILITY_NEUTRALIZING_GAS && !(gBattleResources->flags->flags[i] & RESOURCE_FLAG_NEUTRALIZING_GAS))
            {
                gBattleResources->flags->flags[i] |= RESOURCE_FLAG_NEUTRALIZING_GAS;
                gBattlerAbility = i;
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_NEUTRALIZING_GAS;
                BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
                effect++;
            }
            
            if (effect)
                break;
        }
        break;
    case ABILITYEFFECT_AFTER_RECOIL:
        //Nosferatu
        if(BATTLER_HAS_ABILITY(battler, ABILITY_NOSFERATU)){
            bool8 activateAbilty = FALSE;
            u16 abilityToCheck = ABILITY_NOSFERATU; //For easier copypaste

            //Checks if the ability is triggered
            if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
             && IsBattlerAlive(gBattlerAttacker)
             && !gRoundStructs[gBattlerAttacker].confusionSelfDmg
             && !BATTLER_HEALING_BLOCKED(gBattlerAttacker)
             && IsMoveMakingContact(move, gBattlerAttacker)
             && !BATTLER_MAX_HP(gBattlerAttacker) 
             && TARGET_TURN_DAMAGED){
                activateAbilty = TRUE;
            }

            //This is the stuff that has to be changed for each ability
            if(activateAbilty){
                gBattleScripting.abilityPopupOverwrite = abilityToCheck;

                BattleScriptCall(BattleScript_NosferatuActivated);
                effect++;
            }
        }
        break;
    case ABILITYEFFECT_COPY_STATS:
        if (gBattleStruct->statStageCheckState == STAT_STAGE_CHECK_NEEDED)
        {
            u8 i, statId, j;
            bool8 found = FALSE;
            for (i = 0; i < gBattlersCount; i++)
            {
                bool8 hasEgoist = BATTLER_HAS_ABILITY(i, ABILITY_EGOIST);
                if (hasEgoist || (GetBattlerHoldEffect(i, TRUE) == HOLD_EFFECT_MIRROR_HERB && !found))
                {
                    bool8 foundForBattler = FALSE;
                    for (statId = STAT_ATK; statId < NUM_BATTLE_STATS; statId++)
                    {
                        u8 change = 0;
                        for (j = 0; j < gBattlersCount; j++)
                        {
                            if (GetBattlerSide(i) == GetBattlerSide(j)) continue;
                            if (gBattleStruct->statChangesToCheck[j][statId - 1] > 0)
                            {
                                found = foundForBattler = TRUE;
                                if (hasEgoist)
                                {
                                    SetAbilityStateAs(i, ABILITY_EGOIST, (union AbilityStates) { .statCopyState = (struct StatCopyState) { .inProgress = TRUE } });
                                }
                            }
                        }
                        if (foundForBattler) break;
                    }
                }
            }

            battler = IsAbilityOnField(ABILITY_SHARING_IS_CARING) - 1;
            if ((s8)battler >= 0)
            {
                found = TRUE;
                SetAbilityStateAs(battler, ABILITY_SHARING_IS_CARING, (union AbilityStates) { .statCopyState = (struct StatCopyState) { .inProgress = TRUE } });
            }

            if (found)
            {
                BattleScriptCall(BattleScript_PerformCopyStatEffects);
                effect++;
            }
            else
            {
                ZERO(gBattleStruct->statChangesToCheck)
                gBattleStruct->statStageCheckState = STAT_STAGE_CHECK_NOT_NEEDED;
            }
        }
        break;
        case ABILITYEFFECT_ATTACKER_FOLLOWUP_MOVE:
            #define CHECK_ABILITY(ability) (CheckAndSetOncePerTurnAbility(battler, ability))

            {
                u32 target = GetBattlerBattleMoveTargetFlags(move, battler);
                if (target == MOVE_TARGET_BOTH || target == MOVE_TARGET_FOES_AND_ALLY)
                {
                    // Move doesn't matter, just pick a single target move
                    gBattlerTarget = GetMoveTarget(MOVE_WEATHER_BALL, MOVE_TARGET_SELECTED + 1);
                    if (GetBattlerSide(gBattlerTarget) == GetBattlerSide(gBattlerAttacker)) break;
                }
            }

            //Two Step
            if(CHECK_ABILITY(ABILITY_TWO_STEP)){
                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    gBattleMoves[move].flags & FLAG_DANCE)
                {
                    u8 target = gBattlerTarget;
                    if (gBattlerAttacker == gBattlerTarget
                        || (gBattlerTarget == BATTLE_PARTNER(gBattlerAttacker)
                            && gBattleMoves[move].target == MOVE_TARGET_ALLY))
                        target = GetMoveTarget(MOVE_REVELATION_DANCE, 0);

                    if (CanUseExtraMove(gBattlerAttacker, target))
                    {
                        gBattlerTarget = target;
                        return UseAttackerFollowUpMove(battler, target, ABILITY_TWO_STEP, MOVE_REVELATION_DANCE, 50);
                    }
                }
            }

            if (!CanUseExtraMove(gBattlerAttacker, gBattlerTarget)) break;

            //Weather Cast
            if(CHECK_ABILITY(ABILITY_FORECAST)){
                switch (move) {
                    case MOVE_SUNNY_DAY:
                    case MOVE_RAIN_DANCE:
                    case MOVE_SANDSTORM:
                    case MOVE_HAIL:
                    case MOVE_EERIE_FOG:
                        if (!CanUseExtraMove(gBattlerAttacker, gBattlerTarget)) break;
                        return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_FORECAST, MOVE_WEATHER_BALL, 0);
                }
            }

            //Volcano Rage
            if(CHECK_ABILITY(ABILITY_VOLCANO_RAGE)){

                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    moveType == TYPE_FIRE){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_VOLCANO_RAGE, MOVE_ERUPTION, 50);
                }
            }

            //Frost Burn
            if(CHECK_ABILITY(ABILITY_FROST_BURN)){

                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    moveType == TYPE_FIRE){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_FROST_BURN, MOVE_ICE_BEAM, 40);
                }
            }

            //Pryo Shells
            if(CHECK_ABILITY(ABILITY_PYRO_SHELLS)){

                //Checks if the ability is triggered
                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    (gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST)){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_PYRO_SHELLS, MOVE_OUTBURST, 50);
                }
            }

            //Thundercall
            if(CHECK_ABILITY(ABILITY_THUNDERCALL)){

                //Checks if the ability is triggered
                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    moveType == TYPE_ELECTRIC){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_THUNDERCALL, MOVE_SMITE, 20);
                }
            }

            //Aftershock
            if(CHECK_ABILITY(ABILITY_AFTERSHOCK)){

                //Checks if the ability is triggered
                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    gBattleMoves[move].power){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_AFTERSHOCK, MOVE_MAGNITUDE, 65);
                }
            }

            //High Tide
            if(CHECK_ABILITY(ABILITY_HIGH_TIDE)){

                //Checks if the ability is triggered
                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    moveType == TYPE_WATER){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_HIGH_TIDE, MOVE_SURF, 50);
                }
            }

            //High Tide
            if(CHECK_ABILITY(ABILITY_CHUNKY_BASS_LINE)){

                //Checks if the ability is triggered
                if(!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) &&
                    gBattleMoves[move].flags & FLAG_SOUND){
                    return UseAttackerFollowUpMove(battler, gBattlerTarget, ABILITY_CHUNKY_BASS_LINE, MOVE_EARTHQUAKE, 40);
                }
            }
            #undef CHECK_ABILITY
        break;
        case ABILITYEFFECT_MOVE_END_EITHER:
        break;
    }

    // Restore state in case of clobbering
    if (effect)
    {
        gBattlerAbility = battler;
        ReadActiveScriptInitialStackState();
    }

    if (effect && gLastUsedAbility != 0xFF)
        RecordAbilityBattle(battler, gLastUsedAbility);

    return effect;
}

bool32 IsUnsuppressableAbility(u32 ability)
{
    switch (ability)
    {
    case ABILITY_FORECAST:
    case ABILITY_MULTITYPE:
    case ABILITY_ZEN_MODE:
    case ABILITY_STANCE_CHANGE:
    case ABILITY_FLAMMABLE_COAT:
    case ABILITY_POWER_CONSTRUCT:
    case ABILITY_SCHOOLING:
    case ABILITY_RKS_SYSTEM:
    case ABILITY_ZERO_TO_HERO:
    case ABILITY_SHIELDS_DOWN:
    case ABILITY_COMATOSE:
    case ABILITY_DISGUISE:
    case ABILITY_PATCHWORK:
    case ABILITY_GULP_MISSILE:
    case ABILITY_ICE_FACE:
    case ABILITY_AS_ONE_ICE_RIDER:
    case ABILITY_AS_ONE_SHADOW_RIDER:
    case ABILITY_CROWNED_KING:
    case ABILITY_CLUELESS:
    case ABILITY_BATTLE_BOND:
    case ABILITY_NEUTRALIZING_GAS:
    case ABILITY_HUNGER_SWITCH:
    case ABILITY_BLOOD_STAIN:
    case ABILITY_BLOOD_STIGMA:
        return TRUE;
    default:
        return FALSE;
    }
}

int IsPersistentOrUnsuppressableAbility(int ability)
{
    if (IsUnsuppressableAbility(ability)) return TRUE;

    switch (ability)
    {
        case ABILITY_EJECT_PACK_ABILITY:
        case ABILITY_CHEATING_DEATH:
        case ABILITY_GALLANTRY:
        case ABILITY_WISHMAKER:
        case ABILITY_GENERATOR:
        case ABILITY_RECURRING_NIGHTMARE:
        case ABILITY_ANTICIPATION:
        case ABILITY_COWARD:
        case ABILITY_ENERGIZED:
            return TRUE;
        
        default:
            return FALSE;
    }
}

bool32 IsNeutralizingGasOnField(void)
{
    u32 i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (IsBattlerAlive(i) && gBattleMons[i].ability == ABILITY_NEUTRALIZING_GAS && !(gStatuses3[i] & STATUS3_GASTRO_ACID))
            return TRUE;
    }

    return FALSE;
}

bool8 BattlerHasAbility(u8 battlerId, u8 attacker, u16 ability)
{
    u8 abilityIsInnate = FALSE;
    if (DoesBattlerHaveAbilityShield(battlerId))
    {
        return GetBattlerAbilityWithoutRemoval(battlerId) == ability || BattlerHasInnateWithoutRemoval(battlerId, ability);
    }

    if (BattlerHasInnateWithoutRemoval(battlerId, ability))
    {
        abilityIsInnate = TRUE;
    }
    else if (GetBattlerAbilityWithoutRemoval(battlerId) != ability)
    {
        return FALSE;
    }

    if ((!abilityIsInnate || B_NEUTRALIZING_GAS_WORKS_ON_INNATES) && BattlerAbilityWasRemoved(battlerId, ability))
    {
        return FALSE;
    }

    if (BattlerIgnoresAbility(attacker, battlerId, ability))
    {
        return FALSE;
    }

    return TRUE;
}
    
STATIC_ASSERT(B_NEUTRALIZING_GAS_WORKS_ON_INNATES, GetAllBattlerAbilitiesAssumesB_NEUTRALIZING_GAS_WORKS_ON_INNATESIsTRUE)

void GetAllBattlerAbilities(u16* abilities, int battler, int battlerAtk)
{
    int i;
    int level, species, personality, isPlayer;
    int ability;
    memset(abilities, ABILITY_NONE, sizeof(*abilities) * 4);

    if (!DoesBattlerHaveAbilityShield(battler) && BattlerAbilityIsSuppressed(battler, battlerAtk)) return;

    ability = GetBattlerAbilityWithoutRemoval(battler);
    if (!BattlerIgnoresAbility(battlerAtk, battler, ability)) abilities[0] = ability;

    level = gBattleMons[battler].level;
    species = gBattleMons[battler].species;
    personality = gBattleMons[battler].personality;
    isPlayer = GetBattlerSide(battler) == B_SIDE_PLAYER;
    for (i = 0; i < NUM_INNATE_PER_SPECIES; i++)
    {
        ability = GetInnateInSlot(level, species, i, personality, isPlayer);
        if (BattlerIgnoresAbility(battlerAtk, battler, ability)) continue;
        abilities[i + 1] = ability;
    }
}

u32 GetBattlerAbility(u8 battlerId)
{
    if (!DoesBattlerHaveAbilityShield(battlerId))
    {
        if (BattlerAbilityIsSuppressed(battlerId, gBattlerAttacker)) return ABILITY_NONE;
    }
    
    return GetBattlerAbilityWithoutRemoval(battlerId);
}

bool8 BattlerAbilityIsSuppressed(u8 battlerId, u8 battlerAttacker)
{
    if(BattlerAbilityWasRemoved(battlerId, gBattleMons[battlerId].ability))
        return TRUE;
    
    if (BattlerIgnoresAbility(battlerAttacker, battlerId, gBattleMons[battlerId].ability))
        return TRUE;

    return FALSE;
}

u32 GetBattlerAbilityWithoutRemoval(u8 battlerId)
{
    return gBattleMons[battlerId].ability;
}

bool8 BattlerIgnoresAbility(u8 sBattlerAttacker, u8 sBattlerTarget, u16 ability)
{
    u16 abilityAtk  = gBattleMons[sBattlerAttacker].ability;
    u16 species     = gBattleMons[sBattlerAttacker].species;
    u16 level       = gBattleMons[sBattlerAttacker].level;
    u32 personality = gBattleMons[sBattlerAttacker].personality;
    bool8 isEnemyMon = GetBattlerSide(sBattlerAttacker) == B_SIDE_OPPONENT;

    //Don't ignore own ability
    if (sBattlerAttacker == sBattlerTarget)
        return FALSE;

    //Check that the battler is currently attacking the target
    if (gBattlerByTurnOrder[gCurrentTurnActionNumber] != sBattlerAttacker               ||
        gActionsByTurnOrder[gBattlerByTurnOrder[sBattlerAttacker]] != B_ACTION_USE_MOVE ||
        gCurrentTurnActionNumber >= gBattlersCount)
        return FALSE;

    //Ability is unaffected by Mold Breaker
    if(sAbilitiesAffectedByMoldBreaker[ability] == 0)
        return FALSE;

    //Move ignores target ability regardless if it has Mold Breaker
    if(gBattleMoves[gCurrentMove].flags & FLAG_TARGET_ABILITY_IGNORED)
        return TRUE;

    //Check if the attacker has any Mold Breaker Variant
    switch(abilityAtk){
        case ABILITY_MOLD_BREAKER:
        case ABILITY_BLIND_RAGE:
        case ABILITY_TERAVOLT:
        case ABILITY_TURBOBLAZE:
            return !(gStatuses3[sBattlerAttacker] & STATUS3_GASTRO_ACID);
        default:
            if (gStatuses3[sBattlerAttacker] & STATUS3_GASTRO_ACID && B_NEUTRALIZING_GAS_WORKS_ON_INNATES) return FALSE;
            if(SpeciesHasInnate(species, ABILITY_MOLD_BREAKER, level, personality, isEnemyMon, isEnemyMon) ||
                SpeciesHasInnate(species, ABILITY_BLIND_RAGE, level, personality, isEnemyMon, isEnemyMon) ||
                SpeciesHasInnate(species, ABILITY_TERAVOLT, level, personality, isEnemyMon, isEnemyMon) ||
                SpeciesHasInnate(species, ABILITY_TURBOBLAZE, level, personality, isEnemyMon, isEnemyMon))
            return TRUE;
        break;
    }

    if (abilityAtk == ABILITY_MYCELIUM_MIGHT || SpeciesHasInnate(species, ABILITY_MYCELIUM_MIGHT, level, personality, isEnemyMon, isEnemyMon))
    {
        u16 move = GetChosenMove(sBattlerAttacker);
        if (IS_MOVE_STATUS(move) && gBattleMoves[move].target != MOVE_TARGET_USER)
            return TRUE;
    }
    
    return FALSE;
}

bool8 BattlerAbilityWasRemoved(u8 battlerId, u32 ability)
{
    if ((gStatuses3[battlerId] & STATUS3_GASTRO_ACID || IsNeutralizingGasOnField()) && !IsUnsuppressableAbility(ability))
        return TRUE;
    else
        return FALSE;
}

u32 IsAbilityOnSide(u32 battlerId, u32 ability)
{
    if (BATTLER_HAS_ABILITY(battlerId, ability))
        return battlerId + 1;
    else if (BATTLER_HAS_ABILITY(BATTLE_PARTNER(battlerId), ability))
        return BATTLE_PARTNER(battlerId) + 1;
    else
        return 0;
}

u32 IsAbilityOnOpposingSide(u32 battlerId, u32 ability)
{
    return IsAbilityOnSide(BATTLE_OPPOSITE(battlerId), ability);
}

u32 IsAbilityOnField(u32 ability)
{
    u32 i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (BATTLER_HAS_ABILITY(i, ability))
            return i + 1;
    }

    return 0;
}

u32 IsAbilityOnFieldExcept(u32 battlerId, u32 ability)
{
    u32 i;

    for (i = 0; i < gBattlersCount; i++)
    {
        if (i != battlerId && IsBattlerAlive(i) && (GetBattlerAbility(i) == ability || 
            BattlerHasInnate(i, ability)))
            return i + 1;
    }

    return 0;
}

u32 IsAbilityPreventingEscape(u32 battlerId)
{
    u32 id;
    if(ItemId_GetHoldEffect(gBattleMons[battlerId].item) == HOLD_EFFECT_SHED_SHELL)
        return 0;
    #if B_GHOSTS_ESCAPE >= GEN_6
        if (IS_BATTLER_OF_TYPE(battlerId, TYPE_GHOST))
            return 0;
    #endif
    #if B_SHADOW_TAG_ESCAPE >= GEN_4
        if ((id = IsAbilityOnOpposingSide(battlerId, ABILITY_SHADOW_TAG)) && GetBattlerAbility(battlerId) != ABILITY_SHADOW_TAG)
    #else
        if (id = IsAbilityOnOpposingSide(battlerId, ABILITY_SHADOW_TAG))
    #endif
        return id;
    if ((id = IsAbilityOnOpposingSide(battlerId, ABILITY_ARENA_TRAP)) && IsBattlerGrounded(battlerId))
        return id;
    if ((id = IsAbilityOnOpposingSide(battlerId, ABILITY_MAGNET_PULL)) && IS_BATTLER_OF_TYPE(battlerId, TYPE_STEEL))
        return id;

    return 0;
}

bool32 CanBattlerEscape(u32 battlerId) // no ability check
{
    if (GetBattlerHoldEffect(battlerId, TRUE) == HOLD_EFFECT_SHED_SHELL)
        return TRUE;
    else if ((B_GHOSTS_ESCAPE >= GEN_6 && !IS_BATTLER_OF_TYPE(battlerId, TYPE_GHOST)) && gBattleMons[battlerId].status2 & (STATUS2_ESCAPE_PREVENTION | STATUS2_WRAPPED))
        return FALSE;
    else if (gStatuses4[battlerId] & STATUS4_COMMANDED)
        return FALSE;
    else if (gStatuses3[battlerId] & STATUS3_ROOTED)
        return FALSE;
    else if (gFieldStatuses & STATUS_FIELD_FAIRY_LOCK)
        return FALSE;
    else if (GetBattlerSide(battlerId) == B_SIDE_PLAYER && getMonotypeChampType() == TYPE_FIGHTING)
        return FALSE;
    else if (gVolatileStructs[battlerId].fear)
        return FALSE;
    else
        return TRUE;
}

void BattleScriptExecute(const u8 *BS_ptr)
{
    gBattlescriptCurrInstr = BS_ptr;
    gBattleResources->battleCallbackStack->function[gBattleResources->battleCallbackStack->size++] = gBattleMainFunc;
    gBattleMainFunc = RunBattleScriptCommands_PopCallbacksStack;
    gCurrentActionFuncId = B_ACTION_USE_MOVE;
    BattleScriptSaveCurrentStackData();
}

void BattleScriptPushCursorAndCallback(const u8 *BS_ptr)
{
    BattleScriptCall(BS_ptr);
    gBattleResources->battleCallbackStack->function[gBattleResources->battleCallbackStack->size++] = gBattleMainFunc;
    gBattleMainFunc = RunBattleScriptCommands;
}

enum
{
    ITEM_NO_EFFECT, // 0
    ITEM_STATUS_CHANGE, // 1
    ITEM_EFFECT_OTHER, // 2
    ITEM_PP_CHANGE, // 3
    ITEM_HP_CHANGE, // 4
    ITEM_STATS_CHANGE, // 5
};

bool32 IsBattlerTerrainAffected(u8 battlerId, u32 terrainFlag)
{
    if(!TERRAIN_HAS_EFFECT)
        return FALSE;
    else if (!(gFieldStatuses & terrainFlag))
        return FALSE;
    else if (gStatuses3[battlerId] & STATUS3_SEMI_INVULNERABLE)
        return FALSE;
    
    return IsBattlerGrounded(battlerId);
}

bool8 IsSleepDisabled(u8 battlerId){
    //Sleep Clause
    struct Pokemon *party;
    u8 asleepmons = 0;
    u8 i;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if ((GetMonData(&party[i], MON_DATA_STATUS) & (STATUS1_SLEEP)) &&   
            GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG  &&
            GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_NONE)
            asleepmons++;
    }

    if(asleepmons != 0)
        return TRUE;
    else
        return FALSE;
}

bool8 IsSleepClauseDisablingMove(u8 battlerId, u16 move)
{
    u32 target = BATTLE_OPPOSITE(battlerId);
    u16 moveEffect = gBattleMoves[move].effect;
    bool8 isSleepingMove = FALSE;
    u16 partnerchosenmove = gChosenMoveByBattler[BATTLE_PARTNER(battlerId)];
    bool8 IsDoubleBattle = FALSE;
    bool8 partnerChoseSleepMove = FALSE;

    if(gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_EASY) //Sleep Clause is disabled in easy mode
        return FALSE;

    if((gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        IsDoubleBattle = TRUE;

    if(!IsBattlerAlive(BATTLE_PARTNER(battlerId)) || !IsDoubleBattle)
        partnerchosenmove = MOVE_NONE;

    switch(moveEffect){
        case EFFECT_SLEEP:
        case EFFECT_YAWN:
            isSleepingMove = TRUE;
        break;
        default:
            return FALSE;
        break;
    }

    if(IsDoubleBattle){
        switch(gBattleMoves[partnerchosenmove].effect){
            case EFFECT_SLEEP:
            case EFFECT_YAWN:
                partnerChoseSleepMove = TRUE;
            break;
        }
    }

    if(partnerChoseSleepMove && isSleepingMove && IsDoubleBattle) //To speed up things
        return TRUE;
    else if(IsSleepDisabled(target) && isSleepingMove)
        return TRUE;
    else
        return FALSE;
}

bool8 CanBeDisabled(u8 battlerId)
{
    if (gVolatileStructs[battlerId].disabledMove
        || IsAbilityOnSide(battlerId, ABILITY_AROMA_VEIL))
        return FALSE;
    return TRUE;
}

bool32 CanGetStatus(u8 battlerId)
{
    if (!IsBattlerAlive(battlerId)) return FALSE;
    if (gBattleMons[battlerId].status1) return FALSE;
    if (IsBattlerTerrainAffected(battlerId, STATUS_FIELD_MISTY_TERRAIN)) return FALSE;
    if (gSideStatuses[GetBattlerSide(battlerId)] & SIDE_STATUS_SAFEGUARD) return FALSE;
    if (IsAbilityStatusProtected(battlerId)) return FALSE;
    return TRUE;
}

bool32 CanSleep(u8 battlerId)
{
    u16 ability = GetBattlerAbility(battlerId);

    if (!(gBattleMons[battlerId].status1 & STATUS1_ANY) && IsMyceliumMightActive(gBattlerAttacker))
        return TRUE;

    if (!CanGetStatus(battlerId)) return FALSE;

    if (BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_INSOMNIA, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_VITAL_SPIRIT, ability)
        || IsAbilityOnSide(battlerId, ABILITY_SWEET_VEIL)
        || IsBattlerTerrainAffected(battlerId, STATUS_FIELD_ELECTRIC_TERRAIN))
        return FALSE;
    return TRUE;
}

bool32 CanBePoisoned(u8 battlerAttacker, u8 battlerTarget)
{
    u16 ability = GetBattlerAbility(battlerTarget);

    if (!(gBattleMons[battlerTarget].status1 & STATUS1_ANY) && IsMyceliumMightActive(battlerAttacker))
        return TRUE;

    if (!CanGetStatus(battlerTarget)) return FALSE;
        
    if (!(CanPoisonType(battlerAttacker, battlerTarget))
        || BATTLER_HAS_ABILITY_FAST(battlerTarget, ABILITY_IMMUNITY, ability))
        return FALSE;
    return TRUE;
}

bool32 CanBeBurned(u8 battlerId)
{
    u16 ability = GetBattlerAbility(battlerId);

    if (!(gBattleMons[battlerId].status1 & STATUS1_ANY) && IsMyceliumMightActive(gBattlerAttacker))
        return TRUE;

    if (!CanGetStatus(battlerId)) return FALSE;

    if (IS_BATTLER_OF_TYPE(battlerId, TYPE_FIRE)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_WATER_VEIL, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_PURIFYING_WATERS, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_WATER_BUBBLE, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_FLAME_BUBBLE, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_THERMAL_EXCHANGE, ability))
        return FALSE;
    return TRUE;
}

bool32 CanBeParalyzed(u8 battlerAttacker, u8 battlerTarget)
{
    if (gBattleMons[battlerTarget].status1 & STATUS1_ANY) return FALSE;
    if (IsMyceliumMightActive(battlerAttacker)) return TRUE;

    if (!CanGetStatus(battlerTarget)) return FALSE;

    if (!CanParalyzeType(battlerAttacker, battlerTarget)
        || !CanBeParalyzedIgnoreType(battlerAttacker, battlerTarget))
        return FALSE;
    return TRUE;
}

bool32 CanBeParalyzedIgnoreType(u8 battlerAttacker, u8 battlerTarget)
{
    u16 ability = GetBattlerAbility(battlerTarget);

    if (gBattleMons[battlerTarget].status1 & STATUS1_ANY) return FALSE;
    if (IsMyceliumMightActive(battlerAttacker)) return TRUE;

    if (!CanGetStatus(battlerTarget)) return FALSE;

    if (BATTLER_HAS_ABILITY_FAST(battlerTarget, ABILITY_LIMBER, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerTarget, ABILITY_JUGGERNAUT, ability)
        || BATTLER_HAS_ABILITY_FAST(battlerTarget, ABILITY_IRON_GIANT, ability))
        return FALSE;
    return TRUE;
}

bool32 CanBeFrozen(u8 battlerId)
{
    u16 ability = GetBattlerAbility(battlerId);

    if (!CanGetStatus(battlerId)) return FALSE;

    if (IS_BATTLER_OF_TYPE(battlerId, TYPE_ICE)
        || IsBattlerWeatherAffected(battlerId, WEATHER_SUN_ANY)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_MAGMA_ARMOR, battlerId))
        return FALSE;
    return TRUE;
}

bool32 CanGetFrostbite(u8 battlerId)
{
    if (!(gBattleMons[battlerId].status1 & STATUS1_ANY) && IsMyceliumMightActive(gBattlerAttacker))
        return TRUE;

    if (!CanGetStatus(battlerId)) return FALSE;

    if (IS_BATTLER_OF_TYPE(battlerId, TYPE_ICE)
        || BATTLER_HAS_ABILITY_FAST(battlerId, ABILITY_MAGMA_ARMOR, battlerId))
        return FALSE;
    return TRUE;
}

bool32 CanBleed(u8 battlerId)
{
    if (!(gBattleMons[battlerId].status1 & STATUS1_ANY) && IsMyceliumMightActive(gBattlerAttacker))
        return TRUE;

    if (!CanGetStatus(battlerId)) return FALSE;

    if (IS_BATTLER_OF_TYPE(battlerId, TYPE_ROCK)
        || IS_BATTLER_OF_TYPE(battlerId, TYPE_GHOST)
        || BATTLER_HAS_ABILITY(battlerId, ABILITY_BLOOD_BATH)
        || BATTLER_HAS_ABILITY(battlerId, ABILITY_BLOODLUST))
        return FALSE;
    return TRUE;
}

bool32 CanBeConfused(u8 battlerId)
{
    if (!(gBattleMons[battlerId].status2 & STATUS2_CONFUSION) && IsMyceliumMightActive(gBattlerAttacker))
        return TRUE;

    if (BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_OWN_TEMPO)
        || BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_DISCIPLINE)
        || gBattleMons[gEffectBattler].status2 & STATUS2_CONFUSION
        || IsBattlerTerrainAffected(battlerId, STATUS_FIELD_MISTY_TERRAIN))
        return FALSE;
    return TRUE;
}

int CanInfatuate(int battlerAtk, int battlerDef)
{
    int genderAtk, genderDef;
    if (gBattleMons[battlerDef].status2 & STATUS2_INFATUATION) return FALSE;
    if (battlerAtk == battlerDef) return FALSE;
    if (!IsBattlerAlive(battlerAtk)) return FALSE;

    if (IsMyceliumMightActive(battlerAtk)) return TRUE;
    if (BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_OBLIVIOUS)) return FALSE;
    if (IsAbilityOnSide(battlerDef, ABILITY_OBLIVIOUS)) return FALSE;
    
    if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PURE_LOVE)) return TRUE;

    genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[battlerAtk].species, gBattleMons[battlerAtk].personality);
    genderDef = GetGenderFromSpeciesAndPersonality(gBattleMons[battlerDef].species, gBattleMons[battlerDef].personality);

    if (genderAtk == MON_GENDERLESS) return FALSE;
    if (genderDef == MON_GENDERLESS) return FALSE;
    return genderAtk != genderDef;
}

// second argument is 1/X of current hp compared to max hp
bool32 HasEnoughHpToEatBerry(u32 battlerId, u32 hpFraction, u32 itemId)
{
    bool32 isBerry = (ItemId_GetPocket(itemId) == POCKET_BERRIES);

    if (gBattleMons[battlerId].hp == 0)
        return FALSE;
    if (gBattleScripting.overrideBerryRequirements)
        return TRUE;
    // Unnerve prevents consumption of opponents' berries.
    if (isBerry && IsUnnerveAbilityOnOpposingSide(battlerId))
        return FALSE;
    if (gBattleMons[battlerId].hp <= gBattleMons[battlerId].maxHP / hpFraction)
        return TRUE;

    if (hpFraction <= 4 && (GetBattlerAbility(battlerId) == ABILITY_GLUTTONY || BattlerHasInnate(battlerId, ABILITY_GLUTTONY)) && isBerry
         && gBattleMons[battlerId].hp <= gBattleMons[battlerId].maxHP / 2)
    {
        RecordAbilityBattle(battlerId, ABILITY_GLUTTONY);
        return TRUE;
    }

    return FALSE;
}

int HasRipenEffect(int battler)
{
    return BATTLER_HAS_ABILITY(battler, ABILITY_RIPEN) || BATTLER_HAS_ABILITY(battler, ABILITY_APPLE_PIE);
}

static u8 HealConfuseBerry(u32 battlerId, u32 itemId, u8 flavorId, bool32 end2)
{
    if (HasEnoughHpToEatBerry(battlerId, 4, itemId))
    {
        PREPARE_FLAVOR_BUFFER(gBattleTextBuff1, flavorId);

        gBattleMoveDamage = gBattleMons[battlerId].maxHP / GetBattlerHoldEffectParam(battlerId);
        if (gBattleMoveDamage == 0)
            gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;

        if (HasRipenEffect(battlerId))
        {
            gBattleMoveDamage *= 2;
            gBattlerAbility = battlerId;
        }
        gBattleScripting.battler = battlerId;
        if (end2)
        {
            if (GetFlavorRelationByNature(gBattleMons[battlerId].nature, flavorId) < 0)
                BattleScriptExecute(BattleScript_BerryConfuseHealEnd2);
            else
                BattleScriptExecute(BattleScript_ItemHealHP_RemoveItemEnd2);
        }
        else
        {
            BattleScriptCall(GetFlavorRelationByNature(gBattleMons[battlerId].nature, flavorId) < 0 ? BattleScript_BerryConfuseHealRet : BattleScript_ItemHealHP_RemoveItemRet);
        }

        return ITEM_HP_CHANGE;
    }
    return 0;
}

static u8 StatRaiseBerry(u32 battlerId, u32 itemId, u32 statId, bool32 end2)
{
    if (CompareStat(battlerId, statId, MAX_STAT_STAGE, CMP_LESS_THAN) && HasEnoughHpToEatBerry(battlerId, GetBattlerHoldEffectParam(battlerId), itemId))
    {
        BufferStatChange(battlerId, statId, STRINGID_STATROSE);
        if (HasRipenEffect(battlerId))
            SET_STATCHANGER(statId, 2, FALSE);
        else
            SET_STATCHANGER(statId, 1, FALSE);

        gBattleScripting.animArg1 = 14 + statId;
        gBattleScripting.animArg2 = 0;
        gStackBattler1 = battlerId;

        if (end2)
        {
            BattleScriptExecute(BattleScript_BerryStatRaiseEnd2);
        }
        else
        {
            BattleScriptCall(BattleScript_BerryStatRaiseRet);
        }
        return ITEM_STATS_CHANGE;
    }
    return 0;
}

static u8 RandomStatRaiseBerry(u32 battlerId, u32 itemId, bool32 end2)
{
    s32 i;
    u16 stringId;

    for (i = 0; i < 5; i++)
    {
        if (CompareStat(battlerId, STAT_ATK + i, MAX_STAT_STAGE, CMP_LESS_THAN))
            break;
    }
    if (i != 5 && HasEnoughHpToEatBerry(battlerId, GetBattlerHoldEffectParam(battlerId), itemId))
    {
        do
        {
            i = Random() % 5;
        } while (!CompareStat(battlerId, STAT_ATK + i, MAX_STAT_STAGE, CMP_LESS_THAN));

        PREPARE_STAT_BUFFER(gBattleTextBuff1, i + 1);
        stringId = (BATTLER_HAS_ABILITY(battlerId, ABILITY_CONTRARY)) ? STRINGID_STATFELL : STRINGID_STATROSE;
        gBattleTextBuff2[0] = B_BUFF_PLACEHOLDER_BEGIN;
        gBattleTextBuff2[1] = B_BUFF_STRING;
        gBattleTextBuff2[2] = STRINGID_STATSHARPLY;
        gBattleTextBuff2[3] = STRINGID_STATSHARPLY >> 8;
        gBattleTextBuff2[4] = B_BUFF_STRING;
        gBattleTextBuff2[5] = stringId;
        gBattleTextBuff2[6] = stringId >> 8;
        gBattleTextBuff2[7] = EOS;
        if (HasRipenEffect(battlerId))
            SET_STATCHANGER(i + 1, 4, FALSE);
        else
            SET_STATCHANGER(i + 1, 2, FALSE);

        gBattleScripting.animArg1 = 0x21 + i + 6;
        gBattleScripting.animArg2 = 0;
        gStackBattler1 = battlerId;
        if (end2)
        {
            BattleScriptExecute(BattleScript_BerryStatRaiseEnd2);
        }
        else
        {
            BattleScriptCall(BattleScript_BerryStatRaiseRet);
        }
        
        return ITEM_STATS_CHANGE;
    }
    return 0;
}

static u8 TrySetMicleBerry(u32 battlerId, u32 itemId, bool32 end2)
{
    if (HasEnoughHpToEatBerry(battlerId, 4, itemId))
    {
        gRoundStructs[battlerId].usedMicleBerry = TRUE;  // battler's next attack has increased accuracy

        if (end2)
        {
            BattleScriptExecute(BattleScript_MicleBerryActivateEnd2);
        }
        else
        {
            BattleScriptCall(BattleScript_MicleBerryActivateRet);
        }
        return ITEM_EFFECT_OTHER;
    }
    return 0;
}

static u8 DamagedStatBoostBerryEffect(u8 battlerId, u8 statId, u8 split)
{
    if (IsBattlerAlive(battlerId)
     && TARGET_TURN_DAMAGED
     && CompareStat(battlerId, statId, MAX_STAT_STAGE, CMP_LESS_THAN)
     && !DoesSubstituteBlockMove(gBattlerAttacker, battlerId, gCurrentMove)
     && GetBattleMoveSplit(gCurrentMove) == split)
    {
        BufferStatChange(battlerId, statId, STRINGID_STATROSE);

        gEffectBattler = battlerId;
        if (HasRipenEffect(battlerId))
            SET_STATCHANGER(statId, 2, FALSE);
        else
            SET_STATCHANGER(statId, 1, FALSE);

        gBattleScripting.animArg1 = 14 + statId;
        gBattleScripting.animArg2 = 0;
        gStackBattler1 = battlerId;
        BattleScriptCall(BattleScript_BerryStatRaiseRet);
        return ITEM_STATS_CHANGE;
    }
    return 0;
}

u8 TryHandleSeed(u8 battler, u32 terrainFlag, u8 statId, u16 itemId, bool32 execute)
{
    if (gFieldStatuses & terrainFlag && CompareStat(battler, statId, MAX_STAT_STAGE, CMP_LESS_THAN))
    {
        BufferStatChange(battler, statId, STRINGID_STATROSE);
        gLastUsedItem = itemId; // For surge abilities
        SET_STATCHANGER(statId, 1, FALSE);
        gBattleScripting.animArg1 = 0xE + statId;
        gBattleScripting.animArg2 = 0;
        gStackBattler1 = battler;
        if (execute)
        {
            BattleScriptExecute(BattleScript_BerryStatRaiseEnd2);
        }
        else
        {
            BattleScriptCall(BattleScript_BerryStatRaiseRet);
        }
        return ITEM_STATS_CHANGE;
    }
    return 0;
}

static u8 ItemHealHp(u32 battlerId, u32 itemId, bool32 end2, bool32 percentHeal)
{
    if (BATTLER_HEALING_BLOCKED(battlerId)) return 0;

    if (HasEnoughHpToEatBerry(battlerId, 2, itemId)
      && !(gBattleScripting.overrideBerryRequirements && gBattleMons[battlerId].hp == gBattleMons[battlerId].maxHP))
    {
        if (percentHeal)
            gBattleMoveDamage = (gBattleMons[battlerId].maxHP * GetBattlerHoldEffectParam(battlerId) / 100) * -1;
        else
            gBattleMoveDamage = GetBattlerHoldEffectParam(battlerId) * -1;

        // check ripen
        if (ItemId_GetPocket(itemId) == POCKET_BERRIES && HasRipenEffect(battlerId))
            gBattleMoveDamage *= 2;

        gBattlerAbility = battlerId;    // in SWSH, berry juice shows ability pop up but has no effect. This is mimicked here
        if (end2)
        {
            BattleScriptExecute(BattleScript_ItemHealHP_RemoveItemEnd2);
        }
        else
        {
            BattleScriptCall(BattleScript_ItemHealHP_RemoveItemRet);
        }
        return ITEM_HP_CHANGE;
    }
    return 0;
}

static bool32 UnnerveOn(u32 battlerId, u32 itemId)
{
    if (ItemId_GetPocket(itemId) == POCKET_BERRIES && IsUnnerveAbilityOnOpposingSide(battlerId))
        return TRUE;
    return FALSE;
}

static bool32 GetMentalHerbEffect(u8 battlerId)
{
    bool32 ret = FALSE;
    
    // Check infatuation
    if (gBattleMons[battlerId].status2 & STATUS2_INFATUATION)
    {
        gBattleMons[battlerId].status2 &= ~(STATUS2_INFATUATION);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_INFATUATION;  // STRINGID_TARGETGOTOVERINFATUATION
        StringCopy(gBattleTextBuff1, gStatusConditionString_LoveJpn);
        ret = TRUE;
    }
    #if B_MENTAL_HERB >= GEN_5
        // Check taunt
        if (gVolatileStructs[battlerId].tauntTimer != 0)
        {
            gVolatileStructs[battlerId].tauntTimer = gVolatileStructs[battlerId].tauntTimer2 = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_TAUNT;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_TAUNT);
            ret = TRUE;
        }
        // Check encore
        if (gVolatileStructs[battlerId].encoreTimer != 0)
        {
            gVolatileStructs[battlerId].encoredMove = 0;
            gVolatileStructs[battlerId].encoreTimerStartValue = gVolatileStructs[battlerId].encoreTimer = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_ENCORE;   // STRINGID_PKMNENCOREENDED
            ret = TRUE;
        }
        // Check torment
        if (gBattleMons[battlerId].status2 & STATUS2_TORMENT)
        {
            gBattleMons[battlerId].status2 &= ~(STATUS2_TORMENT);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_TORMENT;
            ret = TRUE;
        }
        // Check heal block
        if (gStatuses3[battlerId] & STATUS3_HEAL_BLOCK)
        {
            gStatuses3[battlerId] &= ~(STATUS3_HEAL_BLOCK);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_HEALBLOCK;
            ret = TRUE;
        }
        // Check disable
        if (gVolatileStructs[battlerId].disableTimer != 0)
        {
            gVolatileStructs[battlerId].disableTimer = gVolatileStructs[battlerId].disableTimerStartValue = 0;
            gVolatileStructs[battlerId].disabledMove = 0;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_MENTALHERBCURE_DISABLE;
            ret = TRUE;
        }
    #endif
    return ret;
}

u8 ItemBattleEffects(u8 caseID, u8 battlerId, bool8 moveTurn)
{
    int i = 0, moveType;
    u8 effect = ITEM_NO_EFFECT;
    u8 changedPP = 0;
    u8 battlerHoldEffect, atkHoldEffect;
    u8 atkHoldEffectParam;
    u16 atkItem;

    gLastUsedItem = gBattleMons[battlerId].item;
    battlerHoldEffect = GetBattlerHoldEffect(battlerId, TRUE);

    atkItem = gBattleMons[gBattlerAttacker].item;
    atkHoldEffect = GetBattlerHoldEffect(gBattlerAttacker, TRUE);
    atkHoldEffectParam = GetBattlerHoldEffectParam(gBattlerAttacker);

    switch (caseID)
    {
    case ITEMEFFECT_ON_SWITCH_IN:
        if (!gTurnStructs[battlerId].switchInItemDone)
        {
            switch (battlerHoldEffect)
            {
            case HOLD_EFFECT_DOUBLE_PRIZE:
                if (GetBattlerSide(battlerId) == B_SIDE_PLAYER && !gBattleStruct->moneyMultiplierItem)
                {
                    gBattleStruct->moneyMultiplier *= 2;
                    gBattleStruct->moneyMultiplierItem = 1;
                }
                break;
            case HOLD_EFFECT_RESTORE_STATS:
                for (i = 0; i < NUM_BATTLE_STATS; i++)
                {
                    if (gBattleMons[battlerId].statStages[i] < DEFAULT_STAT_STAGE)
                    {
                        gBattleMons[battlerId].statStages[i] = DEFAULT_STAT_STAGE;
                        effect = ITEM_STATS_CHANGE;
                    }
                }
                if (effect)
                {
                    gBattleScripting.battler = battlerId;
                    gPotentialItemEffectBattler = battlerId;
                    gActiveBattler = gBattlerAttacker = battlerId;
                    BattleScriptExecute(BattleScript_WhiteHerbEnd2);
                }
                break;
            case HOLD_EFFECT_CONFUSE_SPICY:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SPICY, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_DRY:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_DRY, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_SWEET:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SWEET, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_BITTER:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_BITTER, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_SOUR:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SOUR, TRUE);
                break;
            case HOLD_EFFECT_ATTACK_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_ATK, TRUE);
                break;
            case HOLD_EFFECT_DEFENSE_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_DEF, TRUE);
                break;
            case HOLD_EFFECT_SPEED_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPEED, TRUE);
                break;
            case HOLD_EFFECT_SP_ATTACK_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPATK, TRUE);
                break;
            case HOLD_EFFECT_SP_DEFENSE_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPDEF, TRUE);
                break;
            case HOLD_EFFECT_CRITICAL_UP:
                if (B_BERRIES_INSTANT >= GEN_4 && gVolatileStructs[battlerId].critBoost < 3 && HasEnoughHpToEatBerry(battlerId, GetBattlerHoldEffectParam(battlerId), gLastUsedItem))
                {
                    int increase = min(2, 3 - gVolatileStructs[battlerId].critBoost);
                    gVolatileStructs[battlerId].critBoost += increase;
                    BattleScriptExecute(BattleScript_BerryFocusEnergyEnd2);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            case HOLD_EFFECT_RANDOM_STAT_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = RandomStatRaiseBerry(battlerId, gLastUsedItem, TRUE);
                break;
            case HOLD_EFFECT_CURE_PAR:
                if (B_BERRIES_INSTANT >= GEN_4 && gBattleMons[battlerId].status1 & STATUS1_PARALYSIS && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_PARALYSIS);
                    BattleScriptExecute(BattleScript_BerryCurePrlzEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_PSN:
                if (B_BERRIES_INSTANT >= GEN_4 && gBattleMons[battlerId].status1 & STATUS1_PSN_ANY && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER);
                    BattleScriptExecute(BattleScript_BerryCurePsnEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_BRN:
                if (B_BERRIES_INSTANT >= GEN_4 && gBattleMons[battlerId].status1 & STATUS1_BURN && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_BURN);
                    BattleScriptExecute(BattleScript_BerryCureBrnEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_FRZ:
                if (B_BERRIES_INSTANT >= GEN_4 && gBattleMons[battlerId].status1 & STATUS1_FREEZE && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_FREEZE);
                    BattleScriptExecute(BattleScript_BerryCureFrzEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                if (gBattleMons[battlerId].status1 & STATUS1_FROSTBITE && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~STATUS1_FROSTBITE;
                    BattleScriptCall(BattleScript_BerryCureFsbRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_SLP:
                if (B_BERRIES_INSTANT >= GEN_4 && gBattleMons[battlerId].status1 & STATUS1_SLEEP && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_SLEEP);
                    gBattleMons[battlerId].status2 &= ~(STATUS2_NIGHTMARE);
                    BattleScriptExecute(BattleScript_BerryCureSlpEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_STATUS:
                if (B_BERRIES_INSTANT >= GEN_4 && (gBattleMons[battlerId].status1 & STATUS1_ANY || gBattleMons[battlerId].status2 & STATUS2_CONFUSION) && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    i = 0;
                    if (gBattleMons[battlerId].status1 & STATUS1_PSN_ANY)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_SLEEP)
                    {
                        gBattleMons[battlerId].status2 &= ~(STATUS2_NIGHTMARE);
                        StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_PARALYSIS)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_BURN)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_FREEZE || gBattleMons[battlerId].status1 & STATUS1_FROSTBITE)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status2 & STATUS2_CONFUSION)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_ConfusionJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_BLEED)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                        i++;
                    }
                    if (!(i > 1))
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_PROBLEM;
                    else
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_NORMALIZED_STATUS;
                    gBattleMons[battlerId].status1 = 0;
                    gBattleMons[battlerId].status2 &= ~(STATUS2_CONFUSION);
                    BattleScriptExecute(BattleScript_BerryCureChosenStatusEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_RESTORE_HP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = ItemHealHp(battlerId, gLastUsedItem, TRUE, FALSE);
                break;
            case HOLD_EFFECT_RESTORE_PCT_HP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = ItemHealHp(battlerId, gLastUsedItem, TRUE, TRUE);
                break;
            case HOLD_EFFECT_AIR_BALLOON:
                effect = ITEM_EFFECT_OTHER;
                gBattleScripting.battler = battlerId;
                BattleScriptPushCursorAndCallback(BattleScript_AirBaloonMsgIn);
                RecordItemEffectBattle(battlerId, HOLD_EFFECT_AIR_BALLOON);
                break;
            case HOLD_EFFECT_ROOM_SERVICE:
                if (TryRoomService(battlerId))
                {
                    BattleScriptExecute(BattleScript_BerryStatRaiseEnd2);
                    effect = ITEM_STATS_CHANGE;
                }
                break;
            case HOLD_EFFECT_SEEDS:
                switch (GetBattlerHoldEffectParam(battlerId))
                {
                case HOLD_EFFECT_PARAM_ELECTRIC_TERRAIN:
                    effect = TryHandleSeed(battlerId, STATUS_FIELD_ELECTRIC_TERRAIN, STAT_DEF, gLastUsedItem, TRUE);
                    break;
                case HOLD_EFFECT_PARAM_GRASSY_TERRAIN:
                    effect = TryHandleSeed(battlerId, STATUS_FIELD_GRASSY_TERRAIN, STAT_DEF, gLastUsedItem, TRUE);
                    break;
                case HOLD_EFFECT_PARAM_MISTY_TERRAIN:
                    effect = TryHandleSeed(battlerId, STATUS_FIELD_MISTY_TERRAIN, STAT_SPDEF, gLastUsedItem, TRUE);
                    break;
                case HOLD_EFFECT_PARAM_PSYCHIC_TERRAIN:
                    effect = TryHandleSeed(battlerId, STATUS_FIELD_PSYCHIC_TERRAIN, STAT_SPDEF, gLastUsedItem, TRUE);
                    break;
                }
                break;
            case HOLD_EFFECT_EJECT_PACK:
                if (gRoundStructs[battlerId].statFell
                 && gRoundStructs[battlerId].disableEjectPack == 0
                 && !(gCurrentMove == MOVE_PARTING_SHOT && CanBattlerSwitch(gBattlerAttacker))) // Does not activate if attacker used Parting Shot and can switch out
                {
                    gRoundStructs[battlerId].statFell = FALSE;
                    gStackBattler1 = battlerId;
                    effect = ITEM_STATS_CHANGE;
                    if (moveTurn)
                    {
                        BattleScriptCall(BattleScript_EjectPackActivate_Ret);
                    }
                    else
                    {
                        BattleScriptExecute(BattleScript_EjectPackActivate_End2);
                    }
                }
                break;
            }
            
            if (effect)
            {
                gTurnStructs[battlerId].switchInItemDone = TRUE;
                gActiveBattler = gBattlerAttacker = gPotentialItemEffectBattler = gBattleScripting.battler = battlerId;
                switch (effect)
                {
                case ITEM_STATUS_CHANGE:
                    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battlerId].status1);
                    MarkBattlerForControllerExec(gActiveBattler);
                    break;
                case ITEM_PP_CHANGE:
                    if (!(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED) && !(gVolatileStructs[battlerId].mimickedMoves & 1 << i))
                        gBattleMons[battlerId].pp[i] = changedPP;
                    break;
                }
            }
        }
        break;
    case 1:
        if (gBattleMons[battlerId].hp)
        {
            switch (battlerHoldEffect)
            {
            case HOLD_EFFECT_RESTORE_HP:
                if (!moveTurn)
                    effect = ItemHealHp(battlerId, gLastUsedItem, TRUE, FALSE);
                break;
            case HOLD_EFFECT_RESTORE_PCT_HP:
                if (!moveTurn)
                    effect = ItemHealHp(battlerId, gLastUsedItem, TRUE, TRUE);
                break;
            case HOLD_EFFECT_RESTORE_PP:
                if (!moveTurn)
                {
                    struct Pokemon *mon;
                    u8 ppBonuses;
                    u16 move;

                    mon = GetBattlerPartyData(battlerId);
                    for (i = 0; i < MAX_MON_MOVES; i++)
                    {
                        move = GetMonData(mon, MON_DATA_MOVE1 + i);
                        changedPP = GetMonData(mon, MON_DATA_PP1 + i);
                        ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);
                        if (move && changedPP == 0)
                            break;
                    }
                    if (i != MAX_MON_MOVES)
                    {
                        u8 maxPP = CalculatePPWithBonus(move, ppBonuses, i);
                        u8 ppRestored = GetBattlerHoldEffectParam(battlerId);

                        if (HasRipenEffect(battlerId))
                        {
                            ppRestored *= 2;
                            gBattlerAbility = battlerId;
                        }
                        if (changedPP + ppRestored > maxPP)
                            changedPP = maxPP;
                        else
                            changedPP = changedPP + ppRestored;

                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, move);

                        BattleScriptExecute(BattleScript_BerryPPHealEnd2);
                        BtlController_EmitSetMonData(0, i + REQUEST_PPMOVE1_BATTLE, 0, 1, &changedPP);
                        MarkBattlerForControllerExec(gActiveBattler);
                        effect = ITEM_PP_CHANGE;
                    }
                }
                break;
            case HOLD_EFFECT_RESTORE_STATS:
                for (i = 0; i < NUM_BATTLE_STATS; i++)
                {
                    if (gBattleMons[battlerId].statStages[i] < DEFAULT_STAT_STAGE)
                    {
                        gBattleMons[battlerId].statStages[i] = DEFAULT_STAT_STAGE;
                        effect = ITEM_STATS_CHANGE;
                    }
                }
                if (effect)
                {
                    gBattleScripting.battler = battlerId;
                    gPotentialItemEffectBattler = battlerId;
                    gActiveBattler = gBattlerAttacker = battlerId;
                    BattleScriptExecute(BattleScript_WhiteHerbEnd2);
                }
                break;
            case HOLD_EFFECT_BLACK_SLUDGE:
                if (IS_BATTLER_OF_TYPE(battlerId, TYPE_POISON))
                {
                    goto LEFTOVERS;
                }
                else if (!BATTLER_HAS_MAGIC_GUARD(battlerId) &&
						 !moveTurn)
                {
                    gBattleMoveDamage = gBattleMons[battlerId].maxHP / 8;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    BattleScriptExecute(BattleScript_ItemHurtEnd2);
                    effect = ITEM_HP_CHANGE;
                    RecordItemEffectBattle(battlerId, battlerHoldEffect);
                    PREPARE_ITEM_BUFFER(gBattleTextBuff1, gLastUsedItem);
                }
                break;
            case HOLD_EFFECT_LEFTOVERS:
            LEFTOVERS:
                if (gBattleMons[battlerId].hp < gBattleMons[battlerId].maxHP && !BATTLER_HEALING_BLOCKED(battlerId) && !moveTurn)
                {
                    gBattleMoveDamage = gBattleMons[battlerId].maxHP / 16;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    gBattleMoveDamage *= -1;
                    BattleScriptExecute(BattleScript_ItemHealHP_End2);
                    effect = ITEM_HP_CHANGE;
                    RecordItemEffectBattle(battlerId, battlerHoldEffect);
                }
                break;
            case HOLD_EFFECT_CONFUSE_SPICY:
                if (!moveTurn)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SPICY, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_DRY:
                if (!moveTurn)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_DRY, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_SWEET:
                if (!moveTurn)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SWEET, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_BITTER:
                if (!moveTurn)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_BITTER, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_SOUR:
                if (!moveTurn)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SOUR, TRUE);
                break;
            case HOLD_EFFECT_ATTACK_UP:
                if (!moveTurn)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_ATK, TRUE);
                break;
            case HOLD_EFFECT_DEFENSE_UP:
                if (!moveTurn)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_DEF, TRUE);
                break;
            case HOLD_EFFECT_SPEED_UP:
                if (!moveTurn)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPEED, TRUE);
                break;
            case HOLD_EFFECT_SP_ATTACK_UP:
                if (!moveTurn)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPATK, TRUE);
                break;
            case HOLD_EFFECT_SP_DEFENSE_UP:
                if (!moveTurn)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPDEF, TRUE);
                break;
            case HOLD_EFFECT_CRITICAL_UP:
                if (!moveTurn && !(gVolatileStructs[battlerId].critBoost < 3) && HasEnoughHpToEatBerry(battlerId, GetBattlerHoldEffectParam(battlerId), gLastUsedItem))
                {
                    int increase = min(2, 3 - gVolatileStructs[battlerId].critBoost);
                    gVolatileStructs[battlerId].critBoost += increase;
                    BattleScriptExecute(BattleScript_BerryFocusEnergyEnd2);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            case HOLD_EFFECT_RANDOM_STAT_UP:
                if (!moveTurn)
                    effect = RandomStatRaiseBerry(battlerId, gLastUsedItem, TRUE);
                break;
            case HOLD_EFFECT_CURE_PAR:
                if (gBattleMons[battlerId].status1 & STATUS1_PARALYSIS && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_PARALYSIS);
                    BattleScriptExecute(BattleScript_BerryCurePrlzEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_PSN:
                if (gBattleMons[battlerId].status1 & STATUS1_PSN_ANY && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER);
                    BattleScriptExecute(BattleScript_BerryCurePsnEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_BRN:
                if (gBattleMons[battlerId].status1 & STATUS1_BURN && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_BURN);
                    BattleScriptExecute(BattleScript_BerryCureBrnEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_FRZ:
                if (gBattleMons[battlerId].status1 & STATUS1_FREEZE && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_FREEZE);
                    BattleScriptExecute(BattleScript_BerryCureFrzEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                if (gBattleMons[battlerId].status1 & STATUS1_FROSTBITE && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~STATUS1_FROSTBITE;
                    BattleScriptExecute(BattleScript_BerryCureFsbEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_SLP:
                if (gBattleMons[battlerId].status1 & STATUS1_SLEEP && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_SLEEP);
                    gBattleMons[battlerId].status2 &= ~(STATUS2_NIGHTMARE);
                    BattleScriptExecute(BattleScript_BerryCureSlpEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_CONFUSION:
                if (gBattleMons[battlerId].status2 & STATUS2_CONFUSION && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status2 &= ~(STATUS2_CONFUSION);
                    BattleScriptExecute(BattleScript_BerryCureConfusionEnd2);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            case HOLD_EFFECT_CURE_STATUS:
                if ((gBattleMons[battlerId].status1 & STATUS1_ANY || gBattleMons[battlerId].status2 & STATUS2_CONFUSION) && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    i = 0;
                    if (gBattleMons[battlerId].status1 & STATUS1_PSN_ANY)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_SLEEP)
                    {
                        gBattleMons[battlerId].status2 &= ~(STATUS2_NIGHTMARE);
                        StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_PARALYSIS)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_BURN)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_FREEZE || gBattleMons[battlerId].status1 & STATUS1_FROSTBITE)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status2 & STATUS2_CONFUSION)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_ConfusionJpn);
                        i++;
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_BLEED)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                        i++;
                    }
                    if (!(i > 1))
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_PROBLEM;
                    else
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_NORMALIZED_STATUS;
                    gBattleMons[battlerId].status1 = 0;
                    gBattleMons[battlerId].status2 &= ~(STATUS2_CONFUSION);
                    BattleScriptExecute(BattleScript_BerryCureChosenStatusEnd2);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_MENTAL_HERB:
                if (GetMentalHerbEffect(battlerId))
                {
                    gStackBattler1 = battlerId;
                    BattleScriptExecute(BattleScript_MentalHerbCureEnd2);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            case HOLD_EFFECT_MICLE_BERRY:
                if (!moveTurn)
                    effect = TrySetMicleBerry(battlerId, gLastUsedItem, TRUE);
                break;
            }

            if (effect)
            {
                gActiveBattler = gBattlerAttacker = gPotentialItemEffectBattler = gBattleScripting.battler = battlerId;
                switch (effect)
                {
                case ITEM_STATUS_CHANGE:
                    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battlerId].status1);
                    MarkBattlerForControllerExec(gActiveBattler);
                    break;
                case ITEM_PP_CHANGE:
                    if (!(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED) && !(gVolatileStructs[battlerId].mimickedMoves & 1 << i))
                        gBattleMons[battlerId].pp[i] = changedPP;
                    break;
                }
            }
        }
        break;
    case ITEMEFFECT_BATTLER_MOVE_END:
        goto DO_ITEMEFFECT_MOVE_END;    // this hurts a bit to do, but is an easy solution
    case ITEMEFFECT_MOVE_END:
        for (battlerId = 0; battlerId < gBattlersCount; battlerId++)
        {
            gLastUsedItem = gBattleMons[battlerId].item;
            battlerHoldEffect = GetBattlerHoldEffect(battlerId, TRUE);
        DO_ITEMEFFECT_MOVE_END:
            switch (battlerHoldEffect)
            {
            case HOLD_EFFECT_MICLE_BERRY:
                if (B_HP_BERRIES >= GEN_4)
                    effect = TrySetMicleBerry(battlerId, gLastUsedItem, FALSE);
                break;
            case HOLD_EFFECT_RESTORE_HP:
                if (B_HP_BERRIES >= GEN_4)
                    effect = ItemHealHp(battlerId, gLastUsedItem, FALSE, FALSE);
                break;
            case HOLD_EFFECT_RESTORE_PCT_HP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = ItemHealHp(battlerId, gLastUsedItem, FALSE, TRUE);
                break;
            case HOLD_EFFECT_CONFUSE_SPICY:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SPICY, FALSE);
                break;
            case HOLD_EFFECT_CONFUSE_DRY:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_DRY, FALSE);
                break;
            case HOLD_EFFECT_CONFUSE_SWEET:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SWEET, FALSE);
                break;
            case HOLD_EFFECT_CONFUSE_BITTER:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_BITTER, FALSE);
                break;
            case HOLD_EFFECT_CONFUSE_SOUR:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = HealConfuseBerry(battlerId, gLastUsedItem, FLAVOR_SOUR, FALSE);
                break;
            case HOLD_EFFECT_ATTACK_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_ATK, FALSE);
                break;
            case HOLD_EFFECT_DEFENSE_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_DEF, FALSE);
                break;
            case HOLD_EFFECT_SPEED_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPEED, FALSE);
                break;
            case HOLD_EFFECT_SP_ATTACK_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPATK, FALSE);
                break;
            case HOLD_EFFECT_SP_DEFENSE_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = StatRaiseBerry(battlerId, gLastUsedItem, STAT_SPDEF, FALSE);
                break;
            case HOLD_EFFECT_RANDOM_STAT_UP:
                if (B_BERRIES_INSTANT >= GEN_4)
                    effect = RandomStatRaiseBerry(battlerId, gLastUsedItem, FALSE);
                break;
            case HOLD_EFFECT_CURE_PAR:
                if (gBattleMons[battlerId].status1 & STATUS1_PARALYSIS && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_PARALYSIS);
                    BattleScriptCall(BattleScript_BerryCureParRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_PSN:
                if (gBattleMons[battlerId].status1 & STATUS1_PSN_ANY && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER);
                    BattleScriptCall(BattleScript_BerryCurePsnRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_BRN:
                if (gBattleMons[battlerId].status1 & STATUS1_BURN && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_BURN);
                    BattleScriptCall(BattleScript_BerryCureBrnRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_FRZ:
                if (gBattleMons[battlerId].status1 & STATUS1_FREEZE && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_FREEZE);
                    BattleScriptCall(BattleScript_BerryCureFrzRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_SLP:
                if (gBattleMons[battlerId].status1 & STATUS1_SLEEP && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status1 &= ~(STATUS1_SLEEP);
                    gBattleMons[battlerId].status2 &= ~(STATUS2_NIGHTMARE);
                    BattleScriptCall(BattleScript_BerryCureSlpRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_CURE_CONFUSION:
                if (gBattleMons[battlerId].status2 & STATUS2_CONFUSION && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    gBattleMons[battlerId].status2 &= ~(STATUS2_CONFUSION);
                    BattleScriptCall(BattleScript_BerryCureConfusionRet);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            case HOLD_EFFECT_MENTAL_HERB:
                if (GetMentalHerbEffect(battlerId))
                {
                    gStackBattler1 = battlerId;
                    BattleScriptCall(BattleScript_MentalHerbCureRet);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            case HOLD_EFFECT_CURE_STATUS:
                if ((gBattleMons[battlerId].status1 & STATUS1_ANY || gBattleMons[battlerId].status2 & STATUS2_CONFUSION) && !UnnerveOn(battlerId, gLastUsedItem))
                {
                    if (gBattleMons[battlerId].status1 & STATUS1_PSN_ANY)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_PoisonJpn);
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_SLEEP)
                    {
                        gBattleMons[battlerId].status2 &= ~(STATUS2_NIGHTMARE);
                        StringCopy(gBattleTextBuff1, gStatusConditionString_SleepJpn);
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_PARALYSIS)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_ParalysisJpn);
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_BURN)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_BurnJpn);
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_FREEZE || gBattleMons[battlerId].status1 & STATUS1_FROSTBITE)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_IceJpn);
                    }
                    if (gBattleMons[battlerId].status2 & STATUS2_CONFUSION)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_ConfusionJpn);
                    }
                    if (gBattleMons[battlerId].status1 & STATUS1_BLEED)
                    {
                        StringCopy(gBattleTextBuff1, gStatusConditionString_BleedJpn);
                    }
                    gBattleMons[battlerId].status1 = 0;
                    gBattleMons[battlerId].status2 &= ~(STATUS2_CONFUSION);
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CURED_PROBLEM;
                    BattleScriptCall(BattleScript_BerryCureChosenStatusRet);
                    effect = ITEM_STATUS_CHANGE;
                }
                break;
            case HOLD_EFFECT_RESTORE_STATS:
                for (i = 0; i < NUM_BATTLE_STATS; i++)
                {
                    if (gBattleMons[battlerId].statStages[i] < DEFAULT_STAT_STAGE)
                    {
                        gBattleMons[battlerId].statStages[i] = DEFAULT_STAT_STAGE;
                        effect = ITEM_STATS_CHANGE;
                    }
                }
                if (effect)
                {
                    gBattleScripting.battler = battlerId;
                    gPotentialItemEffectBattler = battlerId;
                    BattleScriptCall(BattleScript_WhiteHerbRet);
                    return effect;
                }
                break;
            }

            if (effect)
            {
                gActiveBattler = gPotentialItemEffectBattler = gBattleScripting.battler = battlerId;
                if (effect == ITEM_STATUS_CHANGE)
                {
                    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                break;
            }
        }
        break;
case ITEMEFFECT_KINGSROCK:
        // Occur on each hit of a multi-strike move
        switch (atkHoldEffect)
        {
        case HOLD_EFFECT_FLINCH:
            #if B_SERENE_GRACE_BOOST >= GEN_5
                if (GetBattlerAbility(gBattlerAttacker) == ABILITY_SERENE_GRACE || BattlerHasInnate(gBattlerAttacker, ABILITY_SERENE_GRACE))
                {
                    atkHoldEffectParam *= 2;
                    atkHoldEffectParam = min(100, atkHoldEffectParam);
                }
                if (gSideTimers[GetBattlerSide(gBattlerAttacker)].rainbowTimer)
                {
                    atkHoldEffectParam *= 2;
                    atkHoldEffectParam = min(100, atkHoldEffectParam);
                }
            #endif
            if (gBattleMoveDamage != 0  // Need to have done damage
                && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
                && TARGET_TURN_DAMAGED
                && (Random() % 100) < atkHoldEffectParam
                && gBattleMoves[gCurrentMove].flags & FLAG_KINGS_ROCK_AFFECTED
                && gBattleMons[gBattlerTarget].hp)
            {
                gBattleScripting.moveEffect = MOVE_EFFECT_FLINCH;
                BattleScriptPushCursor();
                SetMoveEffect(FALSE, 0);
                BattleScriptPop();
            }
            break;
        case HOLD_EFFECT_BLUNDER_POLICY:
            if (gBattleStruct->blunderPolicy
             && gBattleMons[gBattlerAttacker].hp != 0
             && CompareStat(gBattlerAttacker, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
            {
                gBattleStruct->blunderPolicy = FALSE;
                gLastUsedItem = atkItem;
                gBattleScripting.statChanger = SET_STATCHANGER(STAT_SPEED, 2, FALSE);
                effect = ITEM_STATS_CHANGE;
                BattleScriptCall(BattleScript_AttackerItemStatRaise);
            }
            break;
        }
        break;
    case ITEMEFFECT_LIFEORB_SHELLBELL:
        // Occur after the final hit of a multi-strike move
        switch (atkHoldEffect)
        {
        case HOLD_EFFECT_SHELL_BELL:
            if (gTurnStructs[gBattlerTarget].dmg != 0
                && !(TestSheerForceFlag(gBattlerAttacker, gCurrentMove))
                && gBattlerAttacker != gBattlerTarget
                && gBattleMons[gBattlerAttacker].hp != gBattleMons[gBattlerAttacker].maxHP
                && gBattleMons[gBattlerAttacker].hp != 0
                && !BATTLER_HEALING_BLOCKED(gBattlerAttacker))
            {
                gLastUsedItem = atkItem;
                gPotentialItemEffectBattler = gBattlerAttacker;
                gBattleScripting.battler = gBattlerAttacker;
                gBattleMoveDamage = (gTurnStructs[gBattlerTarget].dmg / 4) * -1;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = -1;
                gTurnStructs[gBattlerTarget].dmg = 0;
                BattleScriptCall(BattleScript_ItemHealHP_Ret);
                effect = ITEM_HP_CHANGE;
            }
            break;
        case HOLD_EFFECT_LIFE_ORB:
            {
            int canProc = gBattlerAttacker == gBattlerByTurnOrder[gCurrentTurnActionNumber];
            if (canProc && gQueuedAttackCount)
            {
                // Delay life orb until all extra attack actions for the battler are processed
                int i;
                for (i = 1; i <= gQueuedAttackCount; i++)
                {
                    if (gQueuedExtraAttackData[i].attacker == gBattlerAttacker)
                    {
                        canProc = FALSE;
                        break;
                    }
                }
            }

            if (gTurnStructs[gBattlerAttacker].damagedMons
                && canProc
                && !(TestSheerForceFlag(gBattlerAttacker, gCurrentMove))
                && !BATTLER_HAS_MAGIC_GUARD(gBattlerAttacker)
				&& !(BattlerHasInnate(gBattlerAttacker, ABILITY_SHEER_FORCE)    && (gBattleMoves[gCurrentMove].flags & FLAG_SHEER_FORCE_BOOST))
                && !(GetBattlerAbility(gBattlerAttacker) == ABILITY_SHEER_FORCE && (gBattleMoves[gCurrentMove].flags & FLAG_SHEER_FORCE_BOOST))
                && gBattlerAttacker != gBattlerTarget
                && gBattleMons[gBattlerAttacker].hp != 0)
            {
                gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 10;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                effect = ITEM_HP_CHANGE;
                BattleScriptCall(BattleScript_ItemHurtRet);
                gLastUsedItem = gBattleMons[gBattlerAttacker].item;
            }
            }
            break;
        case HOLD_EFFECT_THROAT_SPRAY:  // Does NOT need to be a damaging move
            if (gRoundStructs[gBattlerAttacker].targetAffected
             && gBattleMons[gBattlerAttacker].hp != 0
             && gBattleMoves[gCurrentMove].flags & FLAG_SOUND
             && CompareStat(gBattlerAttacker, STAT_SPATK, MAX_STAT_STAGE, CMP_LESS_THAN)
             && !NoAliveMonsForEitherParty())   // Don't activate if battle will end
            {
                gLastUsedItem = atkItem;
                gBattleScripting.battler = gBattlerAttacker;
                gBattleScripting.statChanger = SET_STATCHANGER(STAT_SPATK, 1, FALSE);
                effect = ITEM_STATS_CHANGE;
                BattleScriptCall(BattleScript_AttackerItemStatRaise);
            }
            break;
        }
        break;
    case ITEMEFFECT_TARGET:
        if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
        {
            GET_MOVE_TYPE(gCurrentMove, moveType);
            switch (battlerHoldEffect)
            {
            case HOLD_EFFECT_RED_CARD:
            case HOLD_EFFECT_EJECT_BUTTON:
                if (TARGET_TURN_DAMAGED)
                {
                    gTurnStructs[battlerId].shouldTriggerSwitchItem = TRUE;
                }
                break;
            case HOLD_EFFECT_AIR_BALLOON:
                if (TARGET_TURN_DAMAGED)
                {
                    effect = ITEM_EFFECT_OTHER;
                    BattleScriptCall(BattleScript_AirBaloonMsgPop);
                }
                break;
            case HOLD_EFFECT_ROCKY_HELMET:
                if (TARGET_TURN_DAMAGED
                    && IsMoveMakingContact(gCurrentMove, gBattlerAttacker)
                    && IsBattlerAlive(gBattlerAttacker)
					&& !BATTLER_HAS_MAGIC_GUARD(gBattlerAttacker))
                {
                    gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 6;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    effect = ITEM_HP_CHANGE;
                    BattleScriptCall(BattleScript_RockyHelmetActivates);
                    PREPARE_ITEM_BUFFER(gBattleTextBuff1, gLastUsedItem);
                    RecordItemEffectBattle(battlerId, HOLD_EFFECT_ROCKY_HELMET);
                }
                break;
            case HOLD_EFFECT_WEAKNESS_POLICY:
                if (IsBattlerAlive(battlerId)
                    && TARGET_TURN_DAMAGED
                    && gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE)
                {
                    effect = ITEM_STATS_CHANGE;
                    BattleScriptCall(BattleScript_WeaknessPolicy);
                }
                break;
            case HOLD_EFFECT_SNOWBALL:
                if (IsBattlerAlive(battlerId)
                    && TARGET_TURN_DAMAGED
                    && moveType == TYPE_ICE)
                {
                    effect = ITEM_STATS_CHANGE;
                    BattleScriptCall(BattleScript_TargetItemStatRaise);
                    gBattleScripting.statChanger = SET_STATCHANGER(STAT_ATK, 1, FALSE);
                }
                break;
            case HOLD_EFFECT_LUMINOUS_MOSS:
                if (IsBattlerAlive(battlerId)
                    && TARGET_TURN_DAMAGED
                    && moveType == TYPE_WATER)
                {
                    effect = ITEM_STATS_CHANGE;
                    BattleScriptCall(BattleScript_TargetItemStatRaise);
                    gBattleScripting.statChanger = SET_STATCHANGER(STAT_SPDEF, 1, FALSE);
                }
                break;
            case HOLD_EFFECT_CELL_BATTERY:
                if (IsBattlerAlive(battlerId)
                    && TARGET_TURN_DAMAGED
                    && moveType == TYPE_ELECTRIC)
                {
                    effect = ITEM_STATS_CHANGE;
                    BattleScriptCall(BattleScript_TargetItemStatRaise);
                    gBattleScripting.statChanger = SET_STATCHANGER(STAT_ATK, 1, FALSE);
                }
                break;
            case HOLD_EFFECT_ABSORB_BULB:
                if (IsBattlerAlive(battlerId)
                    && TARGET_TURN_DAMAGED
                    && moveType == TYPE_WATER)
                {
                    effect = ITEM_STATS_CHANGE;
                    BattleScriptCall(BattleScript_TargetItemStatRaise);
                    gBattleScripting.statChanger = SET_STATCHANGER(STAT_SPATK, 1, FALSE);
                }
                break;
            case HOLD_EFFECT_JABOCA_BERRY:  // consume and damage attacker if used physical move
                if (IsBattlerAlive(battlerId)
                 && TARGET_TURN_DAMAGED
                 && !DoesSubstituteBlockMove(gBattlerAttacker, battlerId, gCurrentMove)
                 && IS_MOVE_PHYSICAL(gCurrentMove)
                 && !BATTLER_HAS_MAGIC_GUARD(gBattlerAttacker))
                {
                    gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 8;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    if (HasRipenEffect(battlerId))
                        gBattleMoveDamage *= 2;

                    effect = ITEM_HP_CHANGE;
                    BattleScriptCall(BattleScript_JabocaRowapBerryActivates);
                    PREPARE_ITEM_BUFFER(gBattleTextBuff1, gLastUsedItem);
                    RecordItemEffectBattle(battlerId, HOLD_EFFECT_ROCKY_HELMET);
                }
                break;
            case HOLD_EFFECT_ROWAP_BERRY:  // consume and damage attacker if used special move
                if (IsBattlerAlive(battlerId)
                 && TARGET_TURN_DAMAGED
                 && !DoesSubstituteBlockMove(gBattlerAttacker, battlerId, gCurrentMove)
                 && IS_MOVE_SPECIAL(gCurrentMove)
                 && !BATTLER_HAS_MAGIC_GUARD(gBattlerAttacker))
                {
                    gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 8;
                    if (gBattleMoveDamage == 0)
                        gBattleMoveDamage = 1;
                    if (HasRipenEffect(battlerId))
                        gBattleMoveDamage *= 2;

                    effect = ITEM_HP_CHANGE;
                    BattleScriptCall(BattleScript_JabocaRowapBerryActivates);
                    PREPARE_ITEM_BUFFER(gBattleTextBuff1, gLastUsedItem);
                    RecordItemEffectBattle(battlerId, HOLD_EFFECT_ROCKY_HELMET);
                }
                break;
            case HOLD_EFFECT_KEE_BERRY:  // consume and boost defense if used physical move
                effect = DamagedStatBoostBerryEffect(battlerId, STAT_DEF, SPLIT_PHYSICAL);
                break;
            case HOLD_EFFECT_MARANGA_BERRY:  // consume and boost sp. defense if used special move
                effect = DamagedStatBoostBerryEffect(battlerId, STAT_SPDEF, SPLIT_SPECIAL);
                break;
            case HOLD_EFFECT_STICKY_BARB:
                if (TARGET_TURN_DAMAGED
                  && (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
                  && IsMoveMakingContact(gCurrentMove, gBattlerAttacker)
                  && !DoesSubstituteBlockMove(gBattlerAttacker, battlerId, gCurrentMove)
                  && IsBattlerAlive(gBattlerAttacker)
                  && CanStealItem(gBattlerAttacker, gBattlerTarget, gBattleMons[gBattlerTarget].item)
                  && gBattleMons[gBattlerAttacker].item == ITEM_NONE)
                {
                    // No sticky hold checks.
                    gEffectBattler = battlerId; // gEffectBattler = target
                    StealTargetItem(gBattlerAttacker, gBattlerTarget);  // Attacker takes target's barb
                    BattleScriptCall(BattleScript_StickyBarbTransfer);
                    effect = ITEM_EFFECT_OTHER;
                }
                break;
            }
        }
        break;
    case ITEMEFFECT_ORBS:
        switch (battlerHoldEffect)
        {
        case HOLD_EFFECT_TOXIC_ORB:
            if (CanBePoisoned(battlerId, battlerId))
            {
                effect = ITEM_STATUS_CHANGE;
                gBattleMons[battlerId].status1 = STATUS1_TOXIC_POISON;
                BattleScriptExecute(BattleScript_ToxicOrb);
                RecordItemEffectBattle(battlerId, battlerHoldEffect);
            }
            break;
        case HOLD_EFFECT_FLAME_ORB:
            if (CanBeBurned(battlerId))
            {
                effect = ITEM_STATUS_CHANGE;
                gBattleMons[battlerId].status1 = STATUS1_BURN;
                BattleScriptExecute(BattleScript_FlameOrb);
                RecordItemEffectBattle(battlerId, battlerHoldEffect);
            }
            break;
        case HOLD_EFFECT_FROST_ORB:
            if (CanGetFrostbite(battlerId))
            {
                effect = ITEM_STATUS_CHANGE;
                gBattleMons[battlerId].status1 = STATUS1_FROSTBITE;
                BattleScriptExecute(BattleScript_FrostOrb);
                RecordItemEffectBattle(battlerId, battlerHoldEffect);
            }
            break;
        case HOLD_EFFECT_STICKY_BARB:   // Not an orb per se, but similar effect, and needs to NOT activate with pickpocket
            if (!BATTLER_HAS_MAGIC_GUARD(battlerId))
            {
                gBattleMoveDamage = gBattleMons[battlerId].maxHP / 8;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptExecute(BattleScript_ItemHurtEnd2);
                effect = ITEM_HP_CHANGE;
                RecordItemEffectBattle(battlerId, battlerHoldEffect);
                PREPARE_ITEM_BUFFER(gBattleTextBuff1, gLastUsedItem);
            }
            break;
        }

        if (effect == ITEM_STATUS_CHANGE)
        {
            gActiveBattler = battlerId;
            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battlerId].status1);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        break;
    }

    // Berry was successfully used on a Pokemon.
    if (effect && (gLastUsedItem >= FIRST_BERRY_INDEX && gLastUsedItem <= LAST_BERRY_INDEX))
    {
        gBattleStruct->ateBerry[battlerId & BIT_SIDE] |= gBitTable[gBattlerPartyIndexes[battlerId]];
        SetCudChew(battlerId, gLastUsedItem);
    }

    return effect;
}

void ClearFuryCutterDestinyBondGrudge(u8 battlerId)
{
    gVolatileStructs[battlerId].furyCutterCounter = 0;
    gBattleMons[battlerId].status2 &= ~(STATUS2_DESTINY_BOND);
    gStatuses3[battlerId] &= ~(STATUS3_GRUDGE);
}

void HandleAction_RunBattleScript(void) // identical to RunBattleScriptCommands
{
    if (gBattleControllerExecFlags == 0)
        gBattleScriptingCommandsTable[*gBattlescriptCurrInstr]();
}

u32 SetRandomTarget(u32 battlerId)
{
    u32 target;
    static const u8 targets[2][2] =
    {
        [B_SIDE_PLAYER] = {B_POSITION_OPPONENT_LEFT, B_POSITION_OPPONENT_RIGHT},
        [B_SIDE_OPPONENT] = {B_POSITION_PLAYER_LEFT, B_POSITION_PLAYER_RIGHT},
    };

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        target = GetBattlerAtPosition(targets[GetBattlerSide(battlerId)][Random() % 2]);
        if (!IsBattlerAlive(target))
            target ^= BIT_FLANK;
    }
    else
    {
        target = GetBattlerAtPosition(targets[GetBattlerSide(battlerId)][0]);
    }

    return target;
}

u32 GetMoveTarget(u16 move, u8 setTarget)
{
    u8 targetBattler = 0;
    u32 i, moveTarget, side;

    if (setTarget)
        moveTarget = setTarget - 1;
    else
        moveTarget = GetBattlerBattleMoveTargetFlags(move, gBattlerAttacker);
    
    // Special cases
    if (move == MOVE_CURSE)
    {
        if (IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_GHOST))
            moveTarget = MOVE_TARGET_SELECTED;
        else if (IsBattlerWeatherAffected(gBattlerAttacker, WEATHER_FOG_ANY))
            moveTarget = MOVE_TARGET_SELECTED;
        else
            moveTarget = MOVE_TARGET_USER;
    }
    
    switch (moveTarget)
    {
    case MOVE_TARGET_SELECTED:
        side = GetBattlerSide(gBattlerAttacker) ^ BIT_SIDE;
        if (IsAffectedByFollowMe(gBattlerAttacker, side, move))
        {
            targetBattler = gSideTimers[side].followmeTarget;
        }
        else
        {
            targetBattler = SetRandomTarget(gBattlerAttacker);
            if (gBattleMoves[move].type == TYPE_ELECTRIC
                && IsAbilityOnOpposingSide(gBattlerAttacker, ABILITY_LIGHTNING_ROD)
                && !BATTLER_HAS_ABILITY(targetBattler, ABILITY_LIGHTNING_ROD))
            {
                targetBattler ^= BIT_FLANK;
                RecordAbilityBattle(targetBattler, gBattleMons[targetBattler].ability);
                gTurnStructs[targetBattler].lightningRodRedirected = TRUE;
            }
            else if (gBattleMoves[move].type == TYPE_WATER
                && IsAbilityOnOpposingSide(gBattlerAttacker, ABILITY_STORM_DRAIN)
                && !BATTLER_HAS_ABILITY(targetBattler, ABILITY_STORM_DRAIN))
            {
                targetBattler ^= BIT_FLANK;
                RecordAbilityBattle(targetBattler, gBattleMons[targetBattler].ability);
                gTurnStructs[targetBattler].stormDrainRedirected = TRUE;
            }
        }
        break;
    case MOVE_TARGET_DEPENDS:
    case MOVE_TARGET_BOTH:
    case MOVE_TARGET_FOES_AND_ALLY:
    case MOVE_TARGET_OPPONENTS_FIELD:
        targetBattler = GetBattlerAtPosition((GetBattlerPosition(gBattlerAttacker) & BIT_SIDE) ^ BIT_SIDE);
        if (!IsBattlerAlive(targetBattler))
        {
            if (IsBattlerAlive(BATTLE_PARTNER(targetBattler)))
                targetBattler = BATTLE_PARTNER(targetBattler);
            else if (moveTarget == MOVE_TARGET_FOES_AND_ALLY && IsBattlerAlive(BATTLE_PARTNER(gBattlerAttacker)))
                targetBattler = BATTLE_PARTNER(gBattlerAttacker);
        }
        break;
    case MOVE_TARGET_RANDOM:
        side = GetBattlerSide(gBattlerAttacker) ^ BIT_SIDE;
        if (IsAffectedByFollowMe(gBattlerAttacker, side, move))
            targetBattler = gSideTimers[side].followmeTarget;
        else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && moveTarget & MOVE_TARGET_RANDOM)
            targetBattler = SetRandomTarget(gBattlerAttacker);
        else
            targetBattler = GetBattlerAtPosition((GetBattlerPosition(gBattlerAttacker) & BIT_SIDE) ^ BIT_SIDE);
        break;
    case MOVE_TARGET_USER_OR_SELECTED:
    case MOVE_TARGET_USER:
    default:
        targetBattler = gBattlerAttacker;
        break;
    case MOVE_TARGET_ALLY:
        if (IsBattlerAlive(BATTLE_PARTNER(gBattlerAttacker)))
            targetBattler = BATTLE_PARTNER(gBattlerAttacker);
        else
            targetBattler = gBattlerAttacker;
        break;
    }

    return targetBattler;
}

static bool32 IsMonEventLegal(u8 battlerId)
{
    if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT)
        return TRUE;
    if (GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES, NULL) != SPECIES_DEOXYS
        && GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES, NULL) != SPECIES_MEW)
            return TRUE;
    return GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_EVENT_LEGAL, NULL);
}

u8 IsMonDisobedient(void)
{
    s32 rnd;
    s32 calc;
    u8 obedienceLevel = 0;

    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        return 0;
    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT)
        return 0;

    if (IsMonEventLegal(gBattlerAttacker)) // only false if illegal Mew or Deoxys
    {
        if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && GetBattlerPosition(gBattlerAttacker) == 2)
            return 0;
        if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
            return 0;
        if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
            return 0;
        if (!IsOtherTrainer(gBattleMons[gBattlerAttacker].otId, gBattleMons[gBattlerAttacker].otName))
            return 0;
        if (FlagGet(FLAG_BADGE08_GET))
            return 0;

        obedienceLevel = 25;

        if (FlagGet(FLAG_BADGE02_GET))
            obedienceLevel = 40;
        if (FlagGet(FLAG_BADGE03_GET))
            obedienceLevel = 50;
        if (FlagGet(FLAG_BADGE04_GET))
            obedienceLevel = 60;
        if (FlagGet(FLAG_BADGE05_GET))
            obedienceLevel = 70;
        if (FlagGet(FLAG_BADGE06_GET))
            obedienceLevel = 80;
        if (FlagGet(FLAG_BADGE07_GET))
            obedienceLevel = 90;
    }

    //if (gBattleMons[gBattlerAttacker].level <= obedienceLevel)
        return 0;
    rnd = (Random() & 255);
    calc = (gBattleMons[gBattlerAttacker].level + obedienceLevel) * rnd >> 8;
    if (calc < obedienceLevel)
        return 0;

    // is not obedient
    if (gCurrentMove == MOVE_RAGE)
        gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_RAGE);
    if (gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP && (gCurrentMove == MOVE_SNORE || gCurrentMove == MOVE_SLEEP_TALK))
    {
        gBattlescriptCurrInstr = BattleScript_IgnoresWhileAsleep;
        return 1;
    }

    rnd = (Random() & 255);
    calc = (gBattleMons[gBattlerAttacker].level + obedienceLevel) * rnd >> 8;
    if (calc < obedienceLevel)
    {
        calc = CheckMoveLimitations(gBattlerAttacker, 1 << gCurrMovePos, 0xFF);
        if (calc == 0xF) // all moves cannot be used
        {
            // Randomly select, then print a disobedient string
            // B_MSG_LOAFING, B_MSG_WONT_OBEY, B_MSG_TURNED_AWAY, or B_MSG_PRETEND_NOT_NOTICE
            gBattleCommunication[MULTISTRING_CHOOSER] = Random() & (NUM_LOAF_STRINGS - 1);
            gBattlescriptCurrInstr = BattleScript_MoveUsedLoafingAround;
            return 1;
        }
        else // use a random move
        {
            do
            {
                gCurrMovePos = gChosenMovePos = Random() & (MAX_MON_MOVES - 1);
            } while (1 << gCurrMovePos & calc);

            gCalledMove = gBattleMons[gBattlerAttacker].moves[gCurrMovePos];
            gBattlescriptCurrInstr = BattleScript_IgnoresAndUsesRandomMove;
            gBattlerTarget = GetMoveTarget(gCalledMove, 0);
            gHitMarker |= HITMARKER_x200000;
            return 2;
        }
    }
    else
    {
        obedienceLevel = gBattleMons[gBattlerAttacker].level - obedienceLevel;

        calc = (Random() & 255);
        if (calc < obedienceLevel && CanSleep(gBattlerAttacker))
        {
            // try putting asleep
            int i;
            for (i = 0; i < gBattlersCount; i++)
            {
                if (gBattleMons[i].status2 & STATUS2_UPROAR)
                    break;
            }
            if (i == gBattlersCount)
            {
                gBattlescriptCurrInstr = BattleScript_IgnoresAndFallsAsleep;
                return 1;
            }
        }
        calc -= obedienceLevel;
        if (calc < obedienceLevel)
        {
            u8 moveType = TYPE_MYSTERY;
            gBattleMoveDamage = CalculateMoveDamage(MOVE_NONE, gBattlerAttacker, gBattlerAttacker, &moveType, 40, FALSE, FALSE, TRUE);
            gBattlerTarget = gBattlerAttacker;
            gBattlescriptCurrInstr = BattleScript_IgnoresAndHitsItself;
            gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
            return 2;
        }
        else
        {
            // Randomly select, then print a disobedient string
            // B_MSG_LOAFING, B_MSG_WONT_OBEY, B_MSG_TURNED_AWAY, or B_MSG_PRETEND_NOT_NOTICE
            gBattleCommunication[MULTISTRING_CHOOSER] = Random() & (NUM_LOAF_STRINGS - 1);
            gBattlescriptCurrInstr = BattleScript_MoveUsedLoafingAround;
            return 1;
        }
    }
}

bool8 IsItemNegated(u8 battlerId)
{
    if (gStatuses3[battlerId] & STATUS3_EMBARGO)
        return TRUE;
    if (isMagicRoomActive())
        return TRUE;
    if (GetBattlerAbility(battlerId) == ABILITY_KLUTZ)
        return TRUE;
    return FALSE;
}

u32 GetBattlerHoldEffect(u8 battlerId, bool32 checkNegating)
{
    if (checkNegating && IsItemNegated(battlerId)) return HOLD_EFFECT_NONE;

    gPotentialItemEffectBattler = battlerId;

    if (B_ENABLE_DEBUG && gBattleStruct->debugHoldEffects[battlerId] != 0 && gBattleMons[battlerId].item)
        return gBattleStruct->debugHoldEffects[battlerId];
    else if (gBattleMons[battlerId].item == ITEM_ENIGMA_BERRY)
        return gEnigmaBerries[battlerId].holdEffect;
    else
        return ItemId_GetHoldEffect(gBattleMons[battlerId].item);
}

bool8 DoesBattlerHaveAbilityShield(u8 battlerId)
{
    u8 i;
    if (GetBattlerHoldEffect(battlerId, FALSE) != HOLD_EFFECT_ABILITY_SHIELD) return FALSE;
    if (gStatuses3[battlerId] & STATUS3_EMBARGO) return FALSE;
    if (!(gFieldStatuses & STATUS_FIELD_MAGIC_ROOM)) return TRUE;
    for (i = 0; i < gBattlersCount; i++)
    {
        if (GetBattlerAbilityWithoutRemoval(i) == ABILITY_CLUELESS) return TRUE;
        if (BattlerHasInnateWithoutRemoval(i, ABILITY_CLUELESS)) return TRUE;
    }
    return FALSE;
}

u32 GetBattlerHoldEffectParam(u8 battlerId)
{
    if (gBattleMons[battlerId].item == ITEM_ENIGMA_BERRY)
        return gEnigmaBerries[battlerId].holdEffectParam;
    else
        return ItemId_GetHoldEffectParam(gBattleMons[battlerId].item);
}

bool32 IsMoveMakingContact(u16 move, u8 battlerAtk)
{
    if (!(gBattleMoves[move].flags & FLAG_MAKES_CONTACT))
    {
        if (move == MOVE_SHELL_SIDE_ARM && gSwapDamageCategory)
            return TRUE;
        else
            return FALSE;
    }
    else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_LONG_REACH))
    {
        return FALSE;
    }
    else if (GetBattlerHoldEffect(battlerAtk, TRUE) == HOLD_EFFECT_PROTECTIVE_PADS)
    {
        return FALSE;
    }
    else if (GetBattlerHoldEffect(battlerAtk, TRUE) == HOLD_EFFECT_PUNCHING_GLOVE && IS_IRON_FIST(battlerAtk, move))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

bool32 IsBattlerProtected(u8 battlerId, u16 move)
{
    int moveType;

    // Decorate bypasses protect and detect, but not crafty shield
    if (move == MOVE_DECORATE)
    {
        if (gSideStatuses[GetBattlerSide(battlerId)] & SIDE_STATUS_CRAFTY_SHIELD)
            return TRUE;
        else if (gRoundStructs[battlerId].protected)
            return FALSE;
    }

    GET_MOVE_TYPE(move, moveType)

    // Protective Pads doesn't stop Unseen Fist from bypassing Protect effects, so IsMoveMakingContact() isn't used here.
    // This means extra logic is needed to handle Shell Side Arm.
    if (gRoundStructs[battlerId].merculight && !IS_MOVE_STATUS(move) && GetTotalAccuracy(gBattlerAttacker, battlerId, move) < 101)
        return TRUE;
    if (gRoundStructs[battlerId].detected && GetTotalAccuracy(gBattlerAttacker, battlerId, move) < 101)
        return TRUE;
    else if ((BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_UNSEEN_FIST) || BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_FINAL_BLOW))
        && (gBattleMoves[move].flags & FLAG_MAKES_CONTACT || (move == MOVE_SHELL_SIDE_ARM && gSwapDamageCategory)))
        return FALSE;
    else if (BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_DEMOLITIONIST) && gVolatileStructs[gBattlerAttacker].readiedAction)
        return FALSE;
    else if (BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_PINNACLE_BLADE) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        return FALSE;
    else if (!(gBattleMoves[move].flags & FLAG_PROTECT_AFFECTED))
        return FALSE;
    else if (gBattleMoves[move].effect == MOVE_EFFECT_FEINT)
        return FALSE;
    else if (gRoundStructs[battlerId].protected)
        return TRUE;
    else if (gRoundStructs[battlerId].protectedThisTurn && gRoundStructs[gBattlerAttacker].extraMoveUsed != TRUE)
        return TRUE;
    else if (gSideStatuses[GetBattlerSide(battlerId)] & SIDE_STATUS_WIDE_GUARD
             && GetBattlerBattleMoveTargetFlags(move, battlerId) & (MOVE_TARGET_BOTH | MOVE_TARGET_FOES_AND_ALLY))
        return TRUE;
    else if (gRoundStructs[battlerId].banefulBunkered)
        return TRUE;
    else if (gRoundStructs[battlerId].obstructed && !IS_MOVE_STATUS(move))
        return TRUE;
    else if (gRoundStructs[battlerId].silkTrapped && !IS_MOVE_STATUS(move))
        return TRUE;
    else if (gRoundStructs[battlerId].burningBulwark && !IS_MOVE_STATUS(move))
        return TRUE;
    else if (gRoundStructs[battlerId].spikyShielded)
        return TRUE;
    else if (gRoundStructs[battlerId].kingsShielded && gBattleMoves[move].power != 0)
        return TRUE;
    else if (gRoundStructs[battlerId].angelsWrathProtected && gBattleMoves[move].power != 0)
        return TRUE;
    else if (gRoundStructs[battlerId].mindReader)
        return TRUE;
    else if (gSideStatuses[GetBattlerSide(battlerId)] & SIDE_STATUS_QUICK_GUARD
             && GetChosenMovePriority(gBattlerAttacker, battlerId) > 0)
        return TRUE;
    else if (gSideStatuses[GetBattlerSide(battlerId)] & SIDE_STATUS_CRAFTY_SHIELD
      && IS_MOVE_STATUS(move))
        return TRUE;
    else if (gSideStatuses[GetBattlerSide(battlerId)] & SIDE_STATUS_MAT_BLOCK
      && !IS_MOVE_STATUS(move))
        return TRUE;
    else if (gRoundStructs[battlerId].tanglingHusked && moveType == TYPE_FIRE)
        return TRUE;
    else
        return FALSE;
}

static int CheckGroundingEffects(u8 battlerId)
{
    if (GetBattlerHoldEffect(battlerId, TRUE) == HOLD_EFFECT_IRON_BALL)
        return TRUE;
    else if (IsGravityActive())
        return TRUE;
    else if (gStatuses3[battlerId] & STATUS3_ROOTED)
        return TRUE;
    else if (gStatuses3[battlerId] & STATUS3_SMACKED_DOWN)
        return TRUE;
    else if (getMonotypeChampType() == TYPE_GROUND && GetBattlerSide(battlerId) == B_SIDE_PLAYER)
        return TRUE;
    
    return FALSE;
}

static int CheckLevitatingEffects(u8 battlerId)
{
    if (gStatuses3[battlerId] & STATUS3_TELEKINESIS)
        return TRUE;
    else if (gStatuses3[battlerId] & STATUS3_MAGNET_RISE)
        return TRUE;
    else if (GetBattlerHoldEffect(battlerId, TRUE) == HOLD_EFFECT_AIR_BALLOON)
        return TRUE;
    else if (BATTLER_HAS_ABILITY(battlerId, ABILITY_LEVITATE))
        return TRUE;
    else if (BATTLER_HAS_ABILITY(battlerId, ABILITY_HUGE_WINGS))
        return TRUE;
    else if (BATTLER_HAS_ABILITY(battlerId, ABILITY_AERIALIST))
        return TRUE;
	else if (BATTLER_HAS_ABILITY(battlerId, ABILITY_DRAGONFLY)) //Dragonfly
        return TRUE;
    else if (BATTLER_HAS_ABILITY(battlerId, ABILITY_HOVER))
        return TRUE;

    return FALSE;
}

bool32 IsBattlerGroundedIgnoreType(u8 battlerId)
{
    if (CheckGroundingEffects(battlerId)) return TRUE;
    if (CheckLevitatingEffects(battlerId)) return FALSE;
    return TRUE;
}

bool32 IsBattlerGrounded(u8 battlerId)
{
    if (CheckGroundingEffects(battlerId)) return TRUE;
    if (CheckLevitatingEffects(battlerId)) return FALSE;
    if (IS_BATTLER_OF_TYPE(battlerId, TYPE_FLYING)) return FALSE;
    return TRUE;
}

bool32 IsBattlerAlive(u8 battlerId)
{
    if (gBattleMons[battlerId].hp == 0)
        return FALSE;
    else if (battlerId >= gBattlersCount)
        return FALSE;
    else if (gAbsentBattlerFlags & 1 << battlerId)
        return FALSE;
    else
        return TRUE;
}

u8 GetBattleMonMoveSlot(struct BattlePokemon *battleMon, u16 move)
{
    u8 i;

    for (i = 0; i < 4; i++)
    {
        if (battleMon->moves[i] == move)
            break;
    }
    return i;
}

u32 GetBattlerWeight(u8 battlerId)
{
    u32 i;
    u32 weight = GetPokedexHeightWeight(SpeciesToNationalPokedexNum(gBattleMons[battlerId].species), 1);
    u32 ability = GetBattlerAbility(battlerId);
    u32 holdEffect = GetBattlerHoldEffect(battlerId, TRUE);

    if (BATTLER_HAS_ABILITY(battlerId, ABILITY_HEAVY_METAL))
        weight *= 2;
	
    if (BATTLER_HAS_ABILITY(battlerId, ABILITY_LIGHT_METAL))
        weight /= 2;
	
	if (BATTLER_HAS_ABILITY(battlerId, ABILITY_LEAD_COAT))
        weight *= 3;
    
    if (BATTLER_HAS_ABILITY(battlerId, ABILITY_TERASTAL_TREASURE))
        weight *= 3;
	
	if (BATTLER_HAS_ABILITY(battlerId, ABILITY_CHROME_COAT))
        weight *= 3;

    if (holdEffect == HOLD_EFFECT_FLOAT_STONE)
        weight /= 2;

    for (i = 0; i < gVolatileStructs[battlerId].autotomizeCount; i++)
    {
        if (weight > 1000)
        {
            weight -= 1000;
        }
        else if (weight <= 1000)
        {
            weight = 1;
            break;
        }
    }

    if (weight == 0)
        weight = 1;

    return weight;
}

u32 CountBattlerStatIncreases(u8 battlerId, bool32 countEvasionAcc)
{
    u32 i;
    u32 count = 0;

    for (i = 0; i < NUM_BATTLE_STATS; i++)
    {
        if ((i == STAT_ACC || i == STAT_EVASION) && !countEvasionAcc)
            continue;
        if (gBattleMons[battlerId].statStages[i] > DEFAULT_STAT_STAGE) // Stat is increased.
            count += gBattleMons[battlerId].statStages[i] - DEFAULT_STAT_STAGE;
    }

    return count;
}

u32 GetMoveTargetCount(u16 move, u8 battlerAtk, u8 battlerDef)
{
    switch (GetBattlerBattleMoveTargetFlags(move, battlerAtk))
    {
    case MOVE_TARGET_BOTH:
        return IsBattlerAlive(battlerDef)
             + IsBattlerAlive(BATTLE_PARTNER(battlerDef));
    case MOVE_TARGET_FOES_AND_ALLY:
        return IsBattlerAlive(battlerDef)
             + IsBattlerAlive(BATTLE_PARTNER(battlerDef))
             + IsBattlerAlive(BATTLE_PARTNER(battlerAtk));
    case MOVE_TARGET_OPPONENTS_FIELD:
        return 1;
    case MOVE_TARGET_DEPENDS:
    case MOVE_TARGET_SELECTED:
    case MOVE_TARGET_RANDOM:
    case MOVE_TARGET_USER_OR_SELECTED:
        return IsBattlerAlive(battlerDef);
    case MOVE_TARGET_USER:
        return IsBattlerAlive(battlerAtk);
    default:
        return 0;
    }
}

#define MUL_MODIFIER(modifier, val) MulModifier(modifier, UQ_4_12(val))

void MulModifier(u16 *modifier, u16 val)
{
    *modifier = UQ_4_12_TO_INT((*modifier * val) + UQ_4_12_ROUND);
}

u32 ApplyModifier(u16 modifier, u32 val)
{
    return UQ_4_12_TO_INT((modifier * val) + UQ_4_12_ROUND);
}

static const u8 sFlailHpScaleToPowerTable[] =
{
    1, 200,
    4, 150,
    9, 100,
    16, 80,
    32, 40,
    48, 20
};

// format: min. weight (hectograms), base power
static const u16 sWeightToDamageTable[] =
{
    100, 20,
    250, 40,
    500, 60,
    1000, 80,
    2000, 100,
    0xFFFF, 0xFFFF
};

static const u8 sSpeedDiffPowerTable[] = {40, 60, 80, 120, 150};
static const u8 sHeatCrashPowerTable[] = {40, 40, 60, 80, 100, 120};
static const u8 sTrumpCardPowerTable[] = {200, 80, 60, 50, 40};

const struct TypePower gNaturalGiftTable[] =
{
    [ITEM_TO_BERRY(ITEM_CHERI_BERRY)] = {TYPE_FIRE, 80},
    [ITEM_TO_BERRY(ITEM_CHESTO_BERRY)] = {TYPE_WATER, 80},
    [ITEM_TO_BERRY(ITEM_PECHA_BERRY)] = {TYPE_ELECTRIC, 80},
    [ITEM_TO_BERRY(ITEM_RAWST_BERRY)] = {TYPE_GRASS, 80},
    [ITEM_TO_BERRY(ITEM_ASPEAR_BERRY)] = {TYPE_ICE, 80},
    [ITEM_TO_BERRY(ITEM_LEPPA_BERRY)] = {TYPE_FIGHTING, 80},
    [ITEM_TO_BERRY(ITEM_ORAN_BERRY)] = {TYPE_POISON, 80},
    [ITEM_TO_BERRY(ITEM_PERSIM_BERRY)] = {TYPE_GROUND, 80},
    [ITEM_TO_BERRY(ITEM_LUM_BERRY)] = {TYPE_FLYING, 80},
    [ITEM_TO_BERRY(ITEM_SITRUS_BERRY)] = {TYPE_PSYCHIC, 80},
    [ITEM_TO_BERRY(ITEM_FIGY_BERRY)] = {TYPE_BUG, 80},
    [ITEM_TO_BERRY(ITEM_WIKI_BERRY)] = {TYPE_ROCK, 80},
    [ITEM_TO_BERRY(ITEM_MAGO_BERRY)] = {TYPE_GHOST, 80},
    [ITEM_TO_BERRY(ITEM_AGUAV_BERRY)] = {TYPE_DRAGON, 80},
    [ITEM_TO_BERRY(ITEM_IAPAPA_BERRY)] = {TYPE_DARK, 80},
    [ITEM_TO_BERRY(ITEM_RAZZ_BERRY)] = {TYPE_STEEL, 80},
    [ITEM_TO_BERRY(ITEM_OCCA_BERRY)] = {TYPE_FIRE, 80},
    [ITEM_TO_BERRY(ITEM_PASSHO_BERRY)] = {TYPE_WATER, 80},
    [ITEM_TO_BERRY(ITEM_WACAN_BERRY)] = {TYPE_ELECTRIC, 80},
    [ITEM_TO_BERRY(ITEM_RINDO_BERRY)] = {TYPE_GRASS, 80},
    [ITEM_TO_BERRY(ITEM_YACHE_BERRY)] = {TYPE_ICE, 80},
    [ITEM_TO_BERRY(ITEM_CHOPLE_BERRY)] = {TYPE_FIGHTING, 80},
    [ITEM_TO_BERRY(ITEM_KEBIA_BERRY)] = {TYPE_POISON, 80},
    [ITEM_TO_BERRY(ITEM_SHUCA_BERRY)] = {TYPE_GROUND, 80},
    [ITEM_TO_BERRY(ITEM_COBA_BERRY)] = {TYPE_FLYING, 80},
    [ITEM_TO_BERRY(ITEM_PAYAPA_BERRY)] = {TYPE_PSYCHIC, 80},
    [ITEM_TO_BERRY(ITEM_TANGA_BERRY)] = {TYPE_BUG, 80},
    [ITEM_TO_BERRY(ITEM_CHARTI_BERRY)] = {TYPE_ROCK, 80},
    [ITEM_TO_BERRY(ITEM_KASIB_BERRY)] = {TYPE_GHOST, 80},
    [ITEM_TO_BERRY(ITEM_HABAN_BERRY)] = {TYPE_DRAGON, 80},
    [ITEM_TO_BERRY(ITEM_COLBUR_BERRY)] = {TYPE_DARK, 80},
    [ITEM_TO_BERRY(ITEM_BABIRI_BERRY)] = {TYPE_STEEL, 80},
    [ITEM_TO_BERRY(ITEM_CHILAN_BERRY)] = {TYPE_NORMAL, 80},
    [ITEM_TO_BERRY(ITEM_ROSELI_BERRY)] = {TYPE_FAIRY, 80},
    [ITEM_TO_BERRY(ITEM_BLUK_BERRY)] = {TYPE_FIRE, 90},
    [ITEM_TO_BERRY(ITEM_NANAB_BERRY)] = {TYPE_WATER, 90},
    [ITEM_TO_BERRY(ITEM_WEPEAR_BERRY)] = {TYPE_ELECTRIC, 90},
    [ITEM_TO_BERRY(ITEM_PINAP_BERRY)] = {TYPE_GRASS, 90},
    [ITEM_TO_BERRY(ITEM_POMEG_BERRY)] = {TYPE_ICE, 90},
    [ITEM_TO_BERRY(ITEM_KELPSY_BERRY)] = {TYPE_FIGHTING, 90},
    [ITEM_TO_BERRY(ITEM_QUALOT_BERRY)] = {TYPE_POISON, 90},
    [ITEM_TO_BERRY(ITEM_HONDEW_BERRY)] = {TYPE_GROUND, 90},
    [ITEM_TO_BERRY(ITEM_GREPA_BERRY)] = {TYPE_FLYING, 90},
    [ITEM_TO_BERRY(ITEM_TAMATO_BERRY)] = {TYPE_PSYCHIC, 90},
    [ITEM_TO_BERRY(ITEM_CORNN_BERRY)] = {TYPE_BUG, 90},
    [ITEM_TO_BERRY(ITEM_MAGOST_BERRY)] = {TYPE_ROCK, 90},
    [ITEM_TO_BERRY(ITEM_RABUTA_BERRY)] = {TYPE_GHOST, 90},
    [ITEM_TO_BERRY(ITEM_NOMEL_BERRY)] = {TYPE_DRAGON, 90},
    [ITEM_TO_BERRY(ITEM_SPELON_BERRY)] = {TYPE_DARK, 90},
    [ITEM_TO_BERRY(ITEM_PAMTRE_BERRY)] = {TYPE_STEEL, 90},
    [ITEM_TO_BERRY(ITEM_WATMEL_BERRY)] = {TYPE_FIRE, 100},
    [ITEM_TO_BERRY(ITEM_DURIN_BERRY)] = {TYPE_WATER, 100},
    [ITEM_TO_BERRY(ITEM_BELUE_BERRY)] = {TYPE_ELECTRIC, 100},
    [ITEM_TO_BERRY(ITEM_LIECHI_BERRY)] = {TYPE_GRASS, 100},
    [ITEM_TO_BERRY(ITEM_GANLON_BERRY)] = {TYPE_ICE, 100},
    [ITEM_TO_BERRY(ITEM_SALAC_BERRY)] = {TYPE_FIGHTING, 100},
    [ITEM_TO_BERRY(ITEM_PETAYA_BERRY)] = {TYPE_POISON, 100},
    [ITEM_TO_BERRY(ITEM_APICOT_BERRY)] = {TYPE_GROUND, 100},
    [ITEM_TO_BERRY(ITEM_LANSAT_BERRY)] = {TYPE_FLYING, 100},
    [ITEM_TO_BERRY(ITEM_STARF_BERRY)] = {TYPE_PSYCHIC, 100},
    [ITEM_TO_BERRY(ITEM_ENIGMA_BERRY)] = {TYPE_BUG, 100},
    [ITEM_TO_BERRY(ITEM_MICLE_BERRY)] = {TYPE_ROCK, 100},
    [ITEM_TO_BERRY(ITEM_CUSTAP_BERRY)] = {TYPE_GHOST, 100},
    [ITEM_TO_BERRY(ITEM_JABOCA_BERRY)] = {TYPE_DRAGON, 100},
    [ITEM_TO_BERRY(ITEM_ROWAP_BERRY)] = {TYPE_DARK, 100},
    [ITEM_TO_BERRY(ITEM_KEE_BERRY)] = {TYPE_FAIRY, 100},
    [ITEM_TO_BERRY(ITEM_MARANGA_BERRY)] = {TYPE_DARK, 100},
};

static u16 CalcMoveBasePower(u16 move, u8 battlerAtk, u8 battlerDef)
{
    u32 i;
    u16 basePower = gBattleMoves[move].power;
    u32 weight, hpFraction, speed;

    switch (gBattleMoves[move].effect)
    {
    case EFFECT_FLING:
    {
        u16 item = gTurnStructs[gActiveBattler].flungItem ? gTurnStructs[gActiveBattler].flungItem : gBattleMons[battlerAtk].item;
        switch (item)
        {
            case ITEM_IRON_BALL:
            case ITEM_BIG_NUGGET:
                basePower = 130;
                break;
        }
    }
        break;
    case EFFECT_ERUPTION:
        basePower = gBattleMons[battlerAtk].hp * basePower / gBattleMons[battlerAtk].maxHP;
        break;
    case EFFECT_RETURN:
        basePower = 10 * (gBattleMons[battlerAtk].friendship) / 25;
        break;
    case EFFECT_FRUSTRATION:
        basePower = 10 * (255 - gBattleMons[battlerAtk].friendship) / 25;
        break;
    case EFFECT_FURY_CUTTER:
        for (i = 1; i < gVolatileStructs[battlerAtk].furyCutterCounter; i++)
            basePower *= 2;
        break;
    case EFFECT_ROLLOUT:
        if (gProcessingExtraAttacks || !gVolatileStructs[battlerAtk].rolloutCounter) break;
        basePower = basePower << (gVolatileStructs[battlerAtk].rolloutCounter - 1);
        break;
    case EFFECT_MAGNITUDE:
        basePower = gBattleStruct->magnitudeBasePower;
        break;
    case EFFECT_PRESENT:
        basePower = gBattleStruct->presentBasePower;
        break;
    case EFFECT_TRIPLE_KICK:
        if (gTurnStructs[battlerAtk].multiHitCounter)
            basePower *= 4 - gTurnStructs[battlerAtk].multiHitCounter;
        break;
    case EFFECT_SPIT_UP:
        basePower = 100 * gVolatileStructs[battlerAtk].stockpileCounter;
        break;
    case EFFECT_REVENGE:
        if ((gRoundStructs[battlerAtk].physicalDmg
                && gRoundStructs[battlerAtk].physicalBattlerId == battlerDef)
            || (gRoundStructs[battlerAtk].specialDmg
                && gRoundStructs[battlerAtk].specialBattlerId == battlerDef))
            basePower *= 2;
        break;
    case EFFECT_WEATHER_BALL:
        if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_CHLOROPLAST) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_BIG_LEAVES))
            basePower *= 2;
        else if (gBattleWeather & WEATHER_ANY && WEATHER_HAS_EFFECT)
            basePower *= 2;
        break;
    case EFFECT_PURSUIT:
        if (gActionsByTurnOrder[GetBattlerTurnOrderNum(gBattlerTarget)] == B_ACTION_SWITCH)
            basePower *= 2;
        break;
    case EFFECT_NATURAL_GIFT:
        basePower = gNaturalGiftTable[ITEM_TO_BERRY(gBattleMons[battlerAtk].item)].power;
        break;
    case EFFECT_WAKE_UP_SLAP:
        if (gBattleMons[battlerDef].status1 & STATUS1_SLEEP || BATTLER_HAS_ABILITY(battlerDef, ABILITY_COMATOSE))
            basePower *= 2;
        break;
    case EFFECT_SMELLINGSALT:
        if (gBattleMons[battlerDef].status1 & STATUS1_PARALYSIS)
            basePower *= 2;
        break;
    case EFFECT_WRING_OUT:
        basePower = 120 * gBattleMons[battlerDef].hp / gBattleMons[battlerDef].maxHP;
        break;
    case EFFECT_HEX:
    case EFFECT_INFERNAL_PARADE:
        if (gBattleMons[battlerDef].status1 & STATUS1_ANY || BATTLER_HAS_ABILITY(battlerDef, ABILITY_COMATOSE) || IsBloodStainAffected(battlerDef))
            basePower *= 2;
        break;
    case EFFECT_ASSURANCE:
        if (gRoundStructs[battlerDef].damaged)
            basePower *= 2;
        break;
    case EFFECT_TRUMP_CARD:
        i = GetBattleMonMoveSlot(&gBattleMons[battlerAtk], move);
        if (i != 4)
        {
            if (gBattleMons[battlerAtk].pp[i] >= ARRAY_COUNT(sTrumpCardPowerTable))
                basePower = sTrumpCardPowerTable[ARRAY_COUNT(sTrumpCardPowerTable) - 1];
            else
                basePower = sTrumpCardPowerTable[gBattleMons[battlerAtk].pp[i]];
        }
        break;
    case EFFECT_TERROR_CHARGE:
        if (gVolatileStructs[battlerDef].isFirstTurn == 2)
            basePower *= 2;
        break;
    case EFFECT_ACROBATICS:
        if (gBattleMons[battlerAtk].item == ITEM_NONE
            // Edge case, because removal of items happens after damage calculation.
            || (gTurnStructs[battlerAtk].gemBoost && GetBattlerHoldEffect(battlerAtk, FALSE) == HOLD_EFFECT_GEMS))
            basePower = basePower * 3/2;
        break;
    case EFFECT_LOW_KICK:
        weight = GetBattlerWeight(battlerDef);
        for (i = 0; sWeightToDamageTable[i] != 0xFFFF; i += 2)
        {
            if (sWeightToDamageTable[i] > weight)
                break;
        }
        if (sWeightToDamageTable[i] != 0xFFFF)
            basePower = sWeightToDamageTable[i + 1];
        else
            basePower = 120;
        break;
    case EFFECT_HEAT_CRASH:
        weight = GetBattlerWeight(battlerAtk) / GetBattlerWeight(battlerDef);
        if (weight >= ARRAY_COUNT(sHeatCrashPowerTable))
            basePower = sHeatCrashPowerTable[ARRAY_COUNT(sHeatCrashPowerTable) - 1];
        else
            basePower = sHeatCrashPowerTable[weight];
        break;
    case EFFECT_PUNISHMENT:
        basePower = 60 + (CountBattlerStatIncreases(battlerDef, FALSE) * 20);
        if (basePower > 200)
            basePower = 200;
        break;
    case EFFECT_STORED_POWER:
        basePower += (CountBattlerStatIncreases(battlerAtk, TRUE) * 20);
        break;
    case EFFECT_ELECTRO_BALL:
        speed = GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL) / GetBattlerTotalSpeedStat(battlerDef, TOTAL_SPEED_FULL);
        if (speed >= ARRAY_COUNT(sSpeedDiffPowerTable))
            speed = ARRAY_COUNT(sSpeedDiffPowerTable) - 1;
        basePower = sSpeedDiffPowerTable[speed];
        break;
    case EFFECT_GYRO_BALL:
        basePower = ((25 * GetBattlerTotalSpeedStat(battlerDef, TOTAL_SPEED_FULL)) / GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL)) + 1;
        if (basePower > 150)
            basePower = 150;
        break;
    case EFFECT_ECHOED_VOICE:
        // gBattleStruct->sameMoveTurns incremented in ppreduce
        if (gBattleStruct->sameMoveTurns[battlerAtk] != 0)
        {
            basePower += (basePower * gBattleStruct->sameMoveTurns[battlerAtk]);
            if (basePower > 200)
                basePower = 200;
        }
        break;
    case EFFECT_PAYBACK:
        if (GetBattlerTurnOrderNum(battlerAtk) > GetBattlerTurnOrderNum(battlerDef)
            && (gVolatileStructs[battlerDef].isFirstTurn != 2 || B_PAYBACK_SWITCH_BOOST < GEN_5))
            basePower *= 2;
        break;
    case EFFECT_BOLT_BEAK:
        if (GetBattlerTurnOrderNum(battlerAtk) < GetBattlerTurnOrderNum(battlerDef))
            basePower *= 2;
        break;
    case EFFECT_ROUND:
        if (gChosenMoveByBattler[BATTLE_PARTNER(battlerAtk)] == MOVE_ROUND && !(gAbsentBattlerFlags & 1 << BATTLE_PARTNER(battlerAtk)))
            basePower *= 2;
        break;
    case EFFECT_FUSION_COMBO:
        if (gBattleMoves[gLastUsedMove].effect == EFFECT_FUSION_COMBO && move != gLastUsedMove)
            basePower *= 2;
        break;
    case EFFECT_LASH_OUT:
        if (gRoundStructs[battlerAtk].statFell)
            basePower *= 2;
        break;
    case EFFECT_EXPLOSION:
        if (move == MOVE_MISTY_EXPLOSION && GetCurrentTerrain() == STATUS_FIELD_MISTY_TERRAIN && IsBattlerGrounded(battlerAtk))
            MulModifier(&basePower, UQ_4_12(1.5));
        break;
    case EFFECT_DYNAMAX_DOUBLE_DMG:
        #ifdef B_DYNAMAX
        if (IsDynamaxed(battlerDef))
            basePower *= 2;
        #endif
        break;
    #if B_BEAT_UP_DMG >= GEN_5
    case EFFECT_BEAT_UP:
        basePower = CalcBeatUpPower();
        break;
    #endif
    case EFFECT_GRAV_APPLE:
        if (IsGravityActive())
            MulModifier(&basePower, UQ_4_12(1.5));
        break;
    case EFFECT_TERRAIN_PULSE:
        if ((gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) && IsBattlerGrounded(battlerAtk))
            basePower *= 2;
        break;
    case EFFECT_RISING_VOLTAGE:
        if (GetCurrentTerrain() == STATUS_FIELD_ELECTRIC_TERRAIN && IsBattlerGrounded(battlerDef))
            basePower *= 2;
        break;
    case EFFECT_EXPANDING_FORCE:
        if (GetCurrentTerrain() == STATUS_FIELD_PSYCHIC_TERRAIN)
            basePower = 120;
        break;
    case EFFECT_MISTY_TERRAIN_BOOST:
        if (GetCurrentTerrain() == STATUS_FIELD_MISTY_TERRAIN && IsBattlerGrounded(battlerAtk))
            basePower = basePower * 13 / 10;
        break;
    case EFFECT_MISC_HIT:
        switch (gBattleMoves[move].argument)
        {
            case MISC_EFFECT_FAINTED_MON_BOOST:
                basePower += 10 * gFaintedMonCount[GetBattlerSide(battlerAtk)];
                break;
            case MISC_EFFECT_ELECTRIC_TERRAIN_BOOST:
                if (IsBattlerTerrainAffected(battlerAtk, STATUS_FIELD_ELECTRIC_TERRAIN))
                    basePower = basePower * 3 / 2;
                break;
            case MISC_EFFECT_TOOK_DAMAGE_BOOST:
                basePower += 20 * min(3, gBattleStruct->timesDamaged[gBattlerPartyIndexes[battlerAtk]][GetBattlerSide(battlerAtk)]);
                break;
            case MISC_EFFECT_DOUBLE_DAMAGE:
                basePower *= 1 + ((Random() % 100) < gBattleMoves[move].secondaryEffectChance);
                break;
            case MISC_EFFECT_DOUBLE_DAMAGE_VS_BLEEDING:
                if (gBattleMons[battlerDef].status1 & STATUS1_BLEED || IsBloodStainAffected(battlerDef))
                    basePower *= 2;
                break;
        }
        break;
    }

    // move-specific base power changes
    switch (move)
    {
    case MOVE_WATER_SHURIKEN:
        if (gBattleMons[battlerAtk].species == SPECIES_GRENINJA_ASH)
            basePower = 20;
        break;
    case MOVE_OMINOUS_WIND:
        if (IsBattlerWeatherAffected(battlerAtk, WEATHER_FOG_ANY))
            basePower *= 2;
        break;
    case MOVE_DRAGON_DARTS:
        if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PARENTAL_BOND))
            basePower = basePower * 5 / 4;
        break;
    case MOVE_SELF_DESTRUCT:
        if (gRoundStructs[battlerAtk].physicalDmg || gRoundStructs[battlerAtk].specialDmg)
            basePower *= 2;
        break;
    case MOVE_DREAM_INVERSION:
        if (gBattleMons[battlerDef].status1 & STATUS1_SLEEP)
            basePower *= 2;
        break;
    }

    if(BATTLER_HAS_ABILITY(battlerAtk, ABILITY_ANGELS_WRATH)){
        switch(move){
        case MOVE_TACKLE:
            basePower = 100;
            break;
        case MOVE_POISON_STING:
            basePower = 120;
            break;
        case MOVE_ELECTROWEB:
            basePower = 155;
            break;
        case MOVE_BUG_BITE:
            basePower = 140;
            break;
        }
    }

    if (basePower == 0)
        basePower = 1;
    return basePower;
}

#define MUL(val) MUL_MODIFIER(modifier, val)
#define RESISTANCE(val) { MUL_MODIFIER(resistance, val); MUL_MODIFIER(modifier, val); }

static void CalculateOffensiveAbilityMultiplier(int ability, int battlerAtk, int battlerDef, int move, int moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16* resistance, u16* modifier)
{
    switch (ability)
    {
        case ABILITY_OLD_MARINER:
        case ABILITY_SEAWEED:
		    if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(battlerDef, TYPE_FIRE)) RESISTANCE(2);
            return;
        
        case ABILITY_RECKLESS:
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
            return;
        
        case ABILITY_POWER_EDGE:
        case ABILITY_MYSTIC_BLADES:
        case ABILITY_SWEEPING_EDGE_PLUS:
        case ABILITY_MOLTEN_BLADES:
        case ABILITY_KEEN_EDGE:
		    if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
            return;
        
        case ABILITY_PONY_POWER:
		    if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) {
                MUL(1.3 * 1.3);
            }
            return;
        
        case ABILITY_NIKA:
        case ABILITY_POWER_FISTS:
        case ABILITY_IRON_FIST:
            if (IS_IRON_FIST(battlerAtk, move)) MUL(1.3);
            return;
        
        case ABILITY_MYTHICAL_ARROWS:
        case ABILITY_ARCHER:
            if (gBattleMoves[move].arrowBased) MUL(1.3);
            return;
        
        case ABILITY_STRIKER:
            if (gBattleMoves[move].flags & FLAG_STRIKER_BOOST) MUL(1.3);
            return;
        
        case ABILITY_COMBAT_SPECIALIST:
            CalculateOffensiveAbilityMultiplier(ABILITY_IRON_FIST, battlerAtk, battlerDef, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier);
            CalculateOffensiveAbilityMultiplier(ABILITY_STRIKER, battlerAtk, battlerDef, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier);
            return;
        
        case ABILITY_HUNTERS_HORN:
        case ABILITY_MIGHTY_HORN:
        case ABILITY_UNICORN:
            if (gBattleMoves[move].hornBased) MUL(1.3);
            return;
        
        case ABILITY_SUPER_SLAMMER:
            if (gBattleMoves[move].hammerBased) MUL(1.3);
            return;
        
        case ABILITY_SOUL_CRUSHER:
            if (gBattleMoves[move].hammerBased) MUL(1.1);
            return;
            
        case ABILITY_FIELD_EXPLORER:
            if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
            return;
        
        case ABILITY_GIANT_WINGS:
            if (gBattleMoves[move].airBased) MUL(1.3);
            return;

        case ABILITY_HUGE_WINGS:
            if (gBattleMoves[move].airBased) MUL(1.3);
            if (moveType == TYPE_FLYING) MUL(1.25);
            return;
        
        case ABILITY_SHEER_FORCE:
            if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
            return;
        
        case ABILITY_ANALYTIC:
            if (GetBattlerTurnOrderNum(battlerAtk) == gBattlersCount - 1 && move != MOVE_FUTURE_SIGHT && move != MOVE_DOOM_DESIRE) MUL(1.3);
            return;
        
        case ABILITY_FLAMING_MAW:
        case ABILITY_STRONG_JAW:
        case ABILITY_DEVOURER:
        case ABILITY_MIND_CRUSH:
        case ABILITY_ROUSED_FANGS:
        case ABILITY_SHOCKING_MAW:
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.5);
            return;

        #define MUL_ATE(type) \
            if (moveType == type && gBattleStruct->ateBoost[battlerAtk]) MUL(1.1); \
            return;
        
        case ABILITY_PIXILATE:
            MUL_ATE(TYPE_FAIRY)
        
        case ABILITY_EMANATE:
        case ABILITY_ENLIGHTENED:
            MUL_ATE(TYPE_PSYCHIC)
        
        case ABILITY_POLLINATE:
        case ABILITY_STEEL_BEETLE:
            MUL_ATE(TYPE_BUG)
        
        case ABILITY_DRACONIZE:
            MUL_ATE(TYPE_DRAGON)
        
        case ABILITY_SUPERCONDUCTOR:
        case ABILITY_GALVANIZE:
            MUL_ATE(TYPE_ELECTRIC)
        
        case ABILITY_REFRIGERATE:
        case ABILITY_REFRIGERATOR:
        case ABILITY_CRYSTALLIZE:
            MUL_ATE(TYPE_ICE)
        
        case ABILITY_AERILATE:
            MUL_ATE(TYPE_FLYING)
        
        case ABILITY_FERTILIZE:
            MUL_ATE(TYPE_GRASS)
        
        case ABILITY_NORMALIZE:
            MUL_ATE(TYPE_NORMAL)
        
        case ABILITY_SOLAR_FLARE:
        case ABILITY_IMMOLATE:
            MUL_ATE(TYPE_FIRE)

        case ABILITY_MINERALIZE:
            MUL_ATE(TYPE_ROCK)
        
        case ABILITY_SPECTRAL_SHROUD:
        case ABILITY_SPECTRALIZE:
            MUL_ATE(TYPE_GHOST)
        
        case ABILITY_FIGHT_SPIRIT:
            MUL_ATE(TYPE_FIGHTING)

        case ABILITY_TECTONIZE:
            MUL_ATE(TYPE_GROUND)
        
        case ABILITY_HYDRATE:
            MUL_ATE(TYPE_WATER)
        
        case ABILITY_INTOXICATE:
            MUL_ATE(TYPE_POISON)

        case ABILITY_SLUDGY_MIX:
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(1.3);
            MUL_ATE(TYPE_POISON)
            return;
        
        #undef MUL_ATE

        case ABILITY_SAND_SONG:
            if (moveType == TYPE_GROUND && gBattleMoves[move].flags & FLAG_SOUND && gBattleStruct->ateBoost[battlerAtk]) MUL(1.2);
            return;

        case ABILITY_POWER_METAL:
            if (moveType == TYPE_STEEL && gBattleMoves[move].flags & FLAG_SOUND && gBattleStruct->ateBoost[battlerAtk]) MUL(1.2);
            return;

        case ABILITY_SNOW_SONG:
            if (moveType == TYPE_ICE && gBattleMoves[move].flags & FLAG_SOUND && gBattleStruct->ateBoost[battlerAtk]) MUL(1.2);
            return;

        case ABILITY_BANSHEE:
            if (moveType == TYPE_GHOST && gBattleMoves[move].flags & FLAG_SOUND && gBattleStruct->ateBoost[battlerAtk]) MUL(1.2);
            return;
        
        case ABILITY_AMPLIFIER:
        case ABILITY_PUNK_ROCK:
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(1.3);
            return;
        
        case ABILITY_BASS_BOOSTED:
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(1.3 * 1.3);
            return;
        
        case ABILITY_HYDRO_CIRCUIT:
        case ABILITY_TRANSISTOR:
            if (moveType == TYPE_ELECTRIC) MUL(1.5);
            return;
        
        case ABILITY_DRAGONS_MAW:
            if (moveType == TYPE_DRAGON) MUL(1.5);
            return;
        
        case ABILITY_LIQUID_VOICE:
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(1.2);
            return;
        
        case ABILITY_GORILLA_TACTICS:
            if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
            return;
        
        case ABILITY_SAGE_POWER:
            if (IS_MOVE_SPECIAL(move)) MUL(1.5);
            return;
        
        case ABILITY_EXPLOIT_WEAKNESS:
            if (gBattleMons[battlerDef].status1 & STATUS1_ANY) MUL(1.25);
            return;
        
        case ABILITY_AVENGER:
            if (gSideTimers[GET_BATTLER_SIDE(battlerAtk)].retaliateTimer) MUL(1.5);
            return;
        
        case ABILITY_LONG_REACH:
            if (IS_MOVE_PHYSICAL(move) && !(gBattleMoves[move].flags & FLAG_MAKES_CONTACT)) MUL(1.2);
            return;
        
        case ABILITY_TOUGH_CLAWS:
        case ABILITY_BIG_PECKS:
        case ABILITY_APEX_PREDATOR:
            if (IsMoveMakingContact(move, battlerAtk)) MUL(1.3);
            return;
        
        case ABILITY_VENOBLAZE_PINCERS:
            if (IS_MOVE_PHYSICAL(move)) MUL(1.2);
            return;
        
        case ABILITY_DREAMCATCHER:
            {
                int i;
                for (i = 0; i < gBattlersCount; i++)
                {
                    if (IsBattlerAlive(i) && gBattleMons[i].status1 & STATUS1_SLEEP)
                    {
                        MUL(2.0);
                        return;
                    }
                }
            }
            return;
        
        case ABILITY_RIVALRY:
            {
                int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[battlerAtk].species, gBattleMons[battlerAtk].personality);
                if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[battlerDef].species, gBattleMons[battlerDef].personality))
                    MUL(1.25);
            }
            return;
        
        case ABILITY_COSMIC_DAZE:
            if (gBattleMons[battlerDef].status2 & STATUS2_CONFUSION) MUL(2);
            return;
        
        case ABILITY_DRAGONSLAYER:
            if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_DRAGON)) RESISTANCE(1.5);
            return;
        
        case ABILITY_FAE_HUNTER:
            if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_FAIRY)) RESISTANCE(1.5);
            return;
        
        case ABILITY_MARINE_APEX:
            if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_WATER)) RESISTANCE(1.5);
            return;
        
        case ABILITY_LUMBERJACK:
            if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS)) RESISTANCE(1.5);
            return;
        
        case ABILITY_BLOOD_PRICE:
            MUL(1.3);
            return;
        
        case ABILITY_TOXIC_BOOST:
            if (gBattleMons[battlerAtk].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
            return;
        
        case ABILITY_IRON_BARRAGE:
        case ABILITY_MASTER_HAND:
        case ABILITY_MEGA_LAUNCHER:
            if (gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST) MUL(1.3);
            return;
        
        case ABILITY_HUSTLE:
            MUL(1.4);
            return;
        
        case ABILITY_KUNOICHI_BLADE:
        case ABILITY_TECHNICIAN:
            if (basePower <= 60) MUL(1.5);
            return;
        
        case ABILITY_WATER_BUBBLE:
        case ABILITY_FLAME_BUBBLE:
            if (moveType == TYPE_WATER) MUL(2.0);
            return;
        
        case ABILITY_DUNE_TERROR:
            if (moveType == TYPE_GROUND && IsBattlerWeatherAffected(battlerAtk, WEATHER_SANDSTORM_ANY)) MUL(1.2);
            return;
        
        case ABILITY_FLASH_FIRE:
        case ABILITY_ELEMENTAL_VORTEX:
             if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battlerAtk] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
             return;
        
        #define SWARM_LIKE(type) \
            if (moveType == type) \
            { \
                if (gBattleMons[battlerAtk].hp <= (gBattleMons[battlerAtk].maxHP / 3)) MUL(1.5); \
                else MUL(1.2); \
            } \
            return;
        
        case ABILITY_SWARM:
            SWARM_LIKE(TYPE_BUG)
        
        case ABILITY_TORRENT:
            SWARM_LIKE(TYPE_WATER)
        
        case ABILITY_BLAZE:
            SWARM_LIKE(TYPE_FIRE)
        
        case ABILITY_OVERGROW:
            SWARM_LIKE(TYPE_GRASS)
        
        case ABILITY_ROCKHARD_WILL:
            SWARM_LIKE(TYPE_ROCK)
        
        case ABILITY_VENGEANCE:
            SWARM_LIKE(TYPE_GHOST)

        case ABILITY_SHORT_CIRCUIT:
            SWARM_LIKE(TYPE_ELECTRIC)
        
        case ABILITY_FLOCK:
            SWARM_LIKE(TYPE_FLYING)
        
        case ABILITY_FIGHTER:
            SWARM_LIKE(TYPE_FIGHTING)

        case ABILITY_PSYCHIC_MIND:
            SWARM_LIKE(TYPE_PSYCHIC)
        
        #undef SWARM_LIKE
        
        #define SUPER_SWARM_LIKE(type) \
            if (moveType == type) \
            { \
                if (gBattleMons[battlerAtk].hp <= (gBattleMons[battlerAtk].maxHP / 3)) MUL(1.8); \
                else MUL(1.3); \
            } \
            return;
        
        case ABILITY_PURGATORY:
            SUPER_SWARM_LIKE(TYPE_GHOST)
        
        case ABILITY_HELLBLAZE:
            SUPER_SWARM_LIKE(TYPE_FIRE)
        
        case ABILITY_RIPTIDE:
            SUPER_SWARM_LIKE(TYPE_WATER)
        
        case ABILITY_FOREST_RAGE:
            SUPER_SWARM_LIKE(TYPE_GRASS)
        
        #undef SUPER_SWARM_LIKE

        case ABILITY_PLASMA_LAMP:
            if (moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC) MUL(1.2);
            return;
        
        case ABILITY_ANTARCTIC_BIRD:
            if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
            return;

        case ABILITY_ELECTROCYTES:
            if (moveType == TYPE_ELECTRIC) MUL(1.25);
            return;
        
        case ABILITY_NOCTURNAL:
            if (moveType == TYPE_DARK) MUL(1.25);
            return;
        
        case ABILITY_EARTHBOUND:
            if (moveType == TYPE_GROUND) MUL(1.25);
            return;
        
        case ABILITY_FOSSILIZED:
            if (moveType == TYPE_ROCK) MUL(1.2);
            return;
        
        case ABILITY_RAW_WOOD:
            if (moveType == TYPE_GRASS) MUL(1.2);
            return;
        
        case ABILITY_STEELWORKER:
            if (moveType == TYPE_STEEL) MUL(1.3);
            return;

        case ABILITY_ATOMIC_PUNCH:
            if (moveType == TYPE_STEEL) MUL(1.3);
            if (IS_IRON_FIST(battlerAtk, move)) MUL(1.3);
            return;
        
        case ABILITY_PLUS:
        case ABILITY_MINUS:
            if (BATTLER_HAS_ABILITY(BATTLE_PARTNER(battlerAtk), ABILITY_PLUS) || BATTLER_HAS_ABILITY(BATTLE_PARTNER(battlerDef), ABILITY_PLUS)) MUL(2.0);
            return;
        
        case ABILITY_STAKEOUT:
        case ABILITY_OVERWATCH:
            if (gVolatileStructs[battlerDef].isFirstTurn == 2) MUL(2.0);
            return;
        
        case ABILITY_ICE_COLD_HUNTER:
        case ABILITY_WHITEOUT:
        case ABILITY_SNOWY_WRATH:
            if (moveType == TYPE_ICE && IsBattlerWeatherAffected(battlerAtk, WEATHER_HAIL_ANY)) MUL(1.5);
            return;
        
        case ABILITY_GUTS:
            if (gBattleMons[battlerAtk].status1 & STATUS1_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
            return;
        
        case ABILITY_RAGE_POINT:
            if (gBattleMons[battlerAtk].status1 & STATUS1_ANY) MUL(1.5);
            return;
        
        case ABILITY_BLOOD_STIGMA:
            if (gBattleMons[battlerDef].status1 & STATUS1_BLEED || IsBloodStainAffected(battlerDef)) MUL(1.5);
            return;
        
        case ABILITY_DETERMINATION:
            if (gBattleMons[battlerAtk].status1 & STATUS1_ANY && IS_MOVE_SPECIAL(move)) MUL(1.5);
            return;
        
        case ABILITY_LETHARGY:
            if(gVolatileStructs[battlerAtk].slowStartTimer == 0 || gVolatileStructs[battlerAtk].slowStartTimer == 1) MUL(.2);
            else if(gVolatileStructs[battlerAtk].slowStartTimer == 2) MUL(.4);
            else if(gVolatileStructs[battlerAtk].slowStartTimer == 3) MUL(.6);
            else if(gVolatileStructs[battlerAtk].slowStartTimer == 4) MUL(.8);
            return;
        
        case ABILITY_FLOURISH:
            if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battlerAtk, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
            return;
        
        case ABILITY_ILLUSION:
            if (gBattleStruct->illusion[battlerAtk].on && !gBattleStruct->illusion[battlerAtk].broken) MUL(1.3);
            return;
        
        case ABILITY_LEVITATE:
            if (moveType == TYPE_FLYING) MUL(1.25);
            return;
        
        case ABILITY_AERIALIST:
            CalculateOffensiveAbilityMultiplier(ABILITY_LEVITATE, battlerAtk, battlerDef, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier);
            CalculateOffensiveAbilityMultiplier(ABILITY_FLOCK, battlerAtk, battlerDef, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier);
            return;
            
        case ABILITY_ROCKY_PAYLOAD:
            if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
            return;
        
        case ABILITY_COMBUSTION:
            if (moveType == TYPE_FIRE) MUL(1.5);
            return;

        case ABILITY_AQUATIC_DWELLER:
            if (moveType == TYPE_WATER) MUL(1.5);
            return;
        
        case ABILITY_ELECTRIC_BURST:
            if (moveType == TYPE_ELECTRIC && gBattleMons[battlerAtk].hp > 1) MUL(1.35);
            return;
        
        case ABILITY_INFERNAL_RAGE:
            if (moveType == TYPE_FIRE && gBattleMons[battlerAtk].hp > 1) MUL(1.35);
            return;
        
        case ABILITY_TINTED_LENS:
            if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
            return;

        case ABILITY_SNIPER:
            if (isCrit) MUL(1.5);
            return;
        
        case ABILITY_IRON_SERPENT:
        case ABILITY_WINGED_KING:
        case ABILITY_NEUROFORCE:
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.25);
            return;
        
        case ABILITY_FINAL_BLOW:
        case ABILITY_FATAL_PRECISION:
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.2);
            return;
        
        case ABILITY_ARCANE_FORCE:
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
            return;
        
        case ABILITY_RHYTHMIC:
            MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battlerAtk]);
            return;
        
        case ABILITY_HIGHER_RANK:
            if (GetMovePriority(battlerAtk, move, battlerDef) > 0) MUL(1.2);
            return;
        
        case ABILITY_COLOR_SPECTRUM:
            if (StabMultiplierInHalves(battlerAtk, moveType, GetBattlerAbility(battlerAtk), move) > 2) MUL(1.2);
            return;
    }
}

static void CalculateDefensiveAbilityMultiplier(int ability, int battlerAtk, int battlerDef, int move, int moveType, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier)
{
    switch (ability)
    {
        case ABILITY_OLD_MARINER:
        case ABILITY_SEAWEED:
		    if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS)) RESISTANCE(0.5);
            return;
        
        case ABILITY_WELL_BAKED_BODY:
            if (moveType == TYPE_FIRE) MUL(0.5);
            return;
        
        case ABILITY_DRY_SKIN:
        if (moveType == TYPE_FIRE) MUL(1.25);
            return;
        
        case ABILITY_FLUFFY:
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, battlerAtk)) MUL(0.5);
            return;
        
        case ABILITY_FLUFFIEST:
            if (moveType == TYPE_FIRE) RESISTANCE(4.0);
            if (IsMoveMakingContact(move, battlerAtk)) MUL(0.25);
            return;
        
        case ABILITY_LIQUIFIED:
            if (moveType == TYPE_WATER) RESISTANCE(2);
            if (IsMoveMakingContact(move, battlerAtk)) MUL(0.5);
            return;
        
        case ABILITY_PRETTY_PRINCESS:
            if (!IsUnaware(battlerAtk) &&
                HasAnyLoweredStat(battlerDef))
                    MUL(1.5);
            return;
        
        case ABILITY_HEATPROOF:
        case ABILITY_WATER_BUBBLE:
        case ABILITY_FLAME_BUBBLE:
        case ABILITY_IRON_GIANT:
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
            return;
        
        case ABILITY_CHRISTMAS_SPIRIT:
            if (IsBattlerWeatherAffected(battlerDef, WEATHER_HAIL_ANY)) MUL(.5);
            return;
        
        case ABILITY_DUNE_TERROR:
            if (IsBattlerWeatherAffected(battlerDef, WEATHER_SANDSTORM_ANY)) MUL(.65);
            return;
        
        case ABILITY_SHELL_ARMOR:
        case ABILITY_BATTLE_ARMOR:
        case ABILITY_DREAM_STATE:
            MUL(.8);
            return;
        
        case ABILITY_LEAD_COAT:
            if (IS_MOVE_PHYSICAL(move)) MUL(.6);
            return;
        
        case ABILITY_CHROME_COAT:
            if (IS_MOVE_SPECIAL(move)) MUL(.6);
            return;

        case ABILITY_TERASTAL_TREASURE:
            MUL(.6);
            return;
        
        case ABILITY_ULTRA_INSTINCT:
        case ABILITY_PARRY:
            MUL(.8);
            return;
        
        case ABILITY_FOSSILIZED:
            if (moveType == TYPE_ROCK) RESISTANCE(.5);
            return;
        
        case ABILITY_IMMUNITY:
            if (moveType == TYPE_POISON) RESISTANCE(.5);
            return;
        
        case ABILITY_THICK_FAT:
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
            return;

        case ABILITY_NOCTURNAL:
            if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
            return;

        case ABILITY_MAGMA_ARMOR:
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
            return;

        case ABILITY_OVERCOAT:
            if (IS_MOVE_SPECIAL(move)) MUL(.8);
            return;
        
        case ABILITY_RAW_WOOD:
            if (moveType == TYPE_GRASS) RESISTANCE(.5);
            return;
        
        case ABILITY_WATER_COMPACTION:
            if (moveType == TYPE_WATER) RESISTANCE(.5);
            return;
        
        case ABILITY_PURIFYING_SALT:
            if (moveType == TYPE_GHOST) RESISTANCE(.5);
            return;

        case ABILITY_BASS_BOOSTED:
        case ABILITY_PUNK_ROCK:
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(.5);
            return;
        
        case ABILITY_MULTISCALE:
        case ABILITY_SHADOW_SHIELD:
            if (BATTLER_MAX_HP(battlerDef)) MUL(.5);
            return;
        
        case ABILITY_DRAGONSLAYER:
            if (IS_BATTLER_OF_TYPE(battlerAtk, TYPE_DRAGON)) MUL(.5);
            return;
            
        case ABILITY_FILTER:
        case ABILITY_SOLID_ROCK:
        case ABILITY_PRISM_ARMOR:
        case ABILITY_FLAME_SHIELD:
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
            return;

        case ABILITY_PERMAFROST:
        case ABILITY_PERMAFROST_CLONE:
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.75);
            return;

        case ABILITY_PRIMAL_ARMOR:
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
            return;

        case ABILITY_PRISM_SCALES:
            if (IS_MOVE_SPECIAL(move)) MUL(.7);
            return;

        case ABILITY_ARCTIC_FUR:
            MUL(.65);
            return;

        case ABILITY_PRISMATIC_FUR:
            MUL(.5);
            return;
        
        case ABILITY_ICE_SCALES:
        case ABILITY_FIRE_SCALES:
            if (IS_MOVE_SPECIAL(move)) MUL(.5);
            return;
        
        case ABILITY_APPLE_ENLIGHTENMENT:
        case ABILITY_FUR_COAT:
            if (IS_MOVE_PHYSICAL(move)) MUL(.5);
            return;
        
        case ABILITY_SAND_GUARD:
            if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(battlerAtk, WEATHER_SANDSTORM_ANY)) MUL(.5);
            return;
        
        case ABILITY_RIVALRY:
            {
                int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[battlerAtk].species, gBattleMons[battlerAtk].personality);
                if (genderAtk == MON_MALE) genderAtk = MON_FEMALE;
                else if (genderAtk == MON_FEMALE) genderAtk = MON_MALE;
                if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[battlerDef].species, gBattleMons[battlerDef].personality))
                    MUL(.75);
            }
            return;
        
        case ABILITY_BAD_OMEN:
            if (isCrit) MUL(.5);
            return;
    }
}

#undef MUL
#undef RESISTANCE

u16 CalculateAbilityMultipliers(int battlerAtk, int battlerDef, int move, int moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16* resistanceMultiplier)
{
    u16 multiplier = UQ_4_12(1.0);
    u16 abilities[4] = {0};
    int i = 0;

    GET_ALL_BATTLER_ABILITIES(abilities, battlerAtk, battlerAtk);
    for (i = 0; i < NUM_ABILITY_SLOTS + 1; i++)
    {
        CalculateOffensiveAbilityMultiplier(abilities[i], battlerAtk, battlerDef, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistanceMultiplier, &multiplier);
    }
    
    GET_ALL_BATTLER_ABILITIES(abilities, battlerDef, battlerAtk);
    for (i = 0; i < NUM_ABILITY_SLOTS + 1; i++)
    {
        CalculateDefensiveAbilityMultiplier(abilities[i], battlerAtk, battlerDef, move, moveType, typeEffectivenessMultiplier, isCrit, resistanceMultiplier, &multiplier);
    }

    return multiplier;
}

u32 CalcMoveBasePowerAfterModifiers(u16 move, u8 fixedPower, u8 battlerAtk, u8 battlerDef, u8 moveType, bool32 updateFlags)
{
    u32 i, ability;
    u32 holdEffectAtk, holdEffectParamAtk;
    u16 basePower = CalcMoveBasePower(move, battlerAtk, battlerDef);
    u16 actualPower = fixedPower ? fixedPower : basePower;
    u16 holdEffectModifier;
    u16 modifier = UQ_4_12(1.0);
    u32 atkSide = GET_BATTLER_SIDE(battlerAtk);
	u8 numsleepmons = 0;
	
	for (i = 0; i < gBattlersCount; i++)
    {
        if ((gBattleMons[i].status1 & STATUS1_SLEEP) && IsBattlerAlive(i))
            numsleepmons++;
    }
	
	// Side abilities
	// Steely Spirit
	if(IsAbilityOnSide(battlerAtk, ABILITY_STEELY_SPIRIT) && moveType == TYPE_STEEL) MUL_MODIFIER(&modifier, 1.3);
	
	// Airborne
	if(IsAbilityOnSide(battlerAtk, ABILITY_AIRBORNE) && moveType == TYPE_FLYING) MUL_MODIFIER(&modifier, 1.3);

    // Attacker Partner's Abilities
    if (IsBattlerAlive(BATTLE_PARTNER(battlerAtk)))
    {
		// Battery
		if(BattlerHasAbility(BATTLE_PARTNER(battlerAtk), battlerAtk, ABILITY_BATTERY) && IS_MOVE_SPECIAL(move)) MUL_MODIFIER(&modifier, 1.3);
		
		// Power Spot
		if(BattlerHasAbility(BATTLE_PARTNER(battlerAtk), battlerAtk, ABILITY_POWER_SPOT)) MUL_MODIFIER(&modifier, 1.3);
		
		// Power Spot
		if(BattlerHasAbility(BATTLE_PARTNER(battlerAtk), battlerAtk, ABILITY_MOSH_PIT))
        {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL_MODIFIER(&modifier, 1.25);
            else MUL_MODIFIER(&modifier, 1.5);
        }
    }
	
    // Field Abilities
    if ((IsAbilityOnField(ABILITY_DARK_AURA) && moveType == TYPE_DARK)
        || (IsAbilityOnField(ABILITY_FAIRY_AURA) && moveType == TYPE_FAIRY)
        || (IsAbilityOnField(ABILITY_PIXIE_POWER) && moveType == TYPE_FAIRY))
    {
        if (IsAbilityOnField(ABILITY_AURA_BREAK))
            MulModifier(&modifier, UQ_4_12(0.75));
        else
            MulModifier(&modifier, UQ_4_12(1.33));
    }

    if (gBattleMoves[move].doubleDamageVsMega
        && (gBattleStruct->mega.evolvedSpecies[battlerDef]
            || gBattleStruct->mega.primalRevertedSpecies[battlerDef]))
    {
        MulModifier(&modifier, UQ_4_12(2.0));
    }
        

    holdEffectAtk = GetBattlerHoldEffect(battlerAtk, TRUE);
    holdEffectParamAtk = GetBattlerHoldEffectParam(battlerAtk);
    if (holdEffectParamAtk > 100)
        holdEffectParamAtk = 100;

    holdEffectModifier = UQ_4_12(1.0) + sPercentToModifier[holdEffectParamAtk];

    // attacker's hold effect
    switch (holdEffectAtk)
    {
    case HOLD_EFFECT_MUSCLE_BAND:
        if (IS_MOVE_PHYSICAL(move))
            MulModifier(&modifier, holdEffectModifier);
        break;
    case HOLD_EFFECT_WISE_GLASSES:
        if (IS_MOVE_SPECIAL(move))
            MulModifier(&modifier, holdEffectModifier);
        break;
    case HOLD_EFFECT_SOUL_DEW:
        if ((GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_LATIAS || GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_LATIOS)
            && !(gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
            && IS_MOVE_SPECIAL(move))
                MulModifier(&modifier, holdEffectModifier);
        break;
    case HOLD_EFFECT_GEMS:
        if (gTurnStructs[battlerAtk].gemBoost && gBattleMons[battlerAtk].item)
            MulModifier(&modifier, UQ_4_12(1.0) + sPercentToModifier[gTurnStructs[battlerAtk].gemParam]);
        break;
    case HOLD_EFFECT_BUG_POWER:
    case HOLD_EFFECT_STEEL_POWER:
    case HOLD_EFFECT_GROUND_POWER:
    case HOLD_EFFECT_ROCK_POWER:
    case HOLD_EFFECT_GRASS_POWER:
    case HOLD_EFFECT_DARK_POWER:
    case HOLD_EFFECT_FIGHTING_POWER:
    case HOLD_EFFECT_ELECTRIC_POWER:
    case HOLD_EFFECT_WATER_POWER:
    case HOLD_EFFECT_FLYING_POWER:
    case HOLD_EFFECT_POISON_POWER:
    case HOLD_EFFECT_ICE_POWER:
    case HOLD_EFFECT_GHOST_POWER:
    case HOLD_EFFECT_PSYCHIC_POWER:
    case HOLD_EFFECT_FIRE_POWER:
    case HOLD_EFFECT_DRAGON_POWER:
    case HOLD_EFFECT_NORMAL_POWER:
    case HOLD_EFFECT_FAIRY_POWER:
        for (i = 0; i < ARRAY_COUNT(sHoldEffectToType); i++)
        {
            if (holdEffectAtk == sHoldEffectToType[i][0])
            {
                if (moveType == sHoldEffectToType[i][1])
                    MulModifier(&modifier, holdEffectModifier);
                break;
            }
        }
        break;
    case HOLD_EFFECT_PLATE:
        if (moveType == ItemId_GetSecondaryId(gBattleMons[battlerAtk].item))
            MulModifier(&modifier, holdEffectModifier);
        break;
    }

    // move effect
    switch (gBattleMoves[move].effect)
    {
    case EFFECT_DOUBLE_DMG_IF_STATUS1:
        if (gBattleMons[battlerDef].status1 & (gBattleMoves[move].argument))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case EFFECT_FACADE:
        if (gBattleMons[battlerAtk].status1 & (STATUS1_BURN | STATUS1_PSN_ANY | STATUS1_PARALYSIS | STATUS1_FROSTBITE | STATUS1_BLEED) || IsBloodStainAffected(battlerAtk))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case EFFECT_BRINE:
        if (gBattleMons[battlerDef].hp <= (gBattleMons[battlerDef].maxHP / 2))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case EFFECT_VENOSHOCK:
        if (gBattleMons[battlerDef].status1 & STATUS1_PSN_ANY)
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case EFFECT_RETALIATE:
        if (gSideTimers[atkSide].retaliateTimer == 1)
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case EFFECT_SOLARBEAM:
        if (IsBattlerWeatherAffected(battlerAtk, (WEATHER_HAIL_ANY | WEATHER_SANDSTORM_ANY | WEATHER_RAIN_ANY | WEATHER_FOG_ANY))
            && !BattlerHasInnate(gBattlerAttacker, ABILITY_SOLAR_FLARE)
            && GetBattlerAbility(gBattlerAttacker) != ABILITY_SOLAR_FLARE
            && !BattlerHasInnate(gBattlerAttacker, ABILITY_BIG_LEAVES)
            && GetBattlerAbility(gBattlerAttacker) != ABILITY_BIG_LEAVES
			&& !BattlerHasInnate(gBattlerAttacker, ABILITY_CHLOROPLAST)
            && GetBattlerAbility(gBattlerAttacker) != ABILITY_CHLOROPLAST)
            MulModifier(&modifier, UQ_4_12(0.5));
        break;
    case EFFECT_STOMPING_TANTRUM:
        if (gBattleStruct->lastMoveFailed & 1 << battlerAtk)
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case EFFECT_BULLDOZE:
    case EFFECT_MAGNITUDE:
    case EFFECT_EARTHQUAKE:
        if (GetCurrentTerrain() == STATUS_FIELD_GRASSY_TERRAIN && !(gStatuses3[battlerDef] & STATUS3_SEMI_INVULNERABLE))
            MulModifier(&modifier, UQ_4_12(0.5));
        break;
    case EFFECT_KNOCK_OFF:
        #if B_KNOCK_OFF_DMG >= GEN_6
        if (gBattleMons[battlerDef].item != ITEM_NONE
            && CanBattlerGetOrLoseItem(battlerDef, gBattleMons[battlerDef].item))
            MulModifier(&modifier, UQ_4_12(1.5));
        #endif
        break;
    }

    // various effecs
    if (gRoundStructs[battlerAtk].helpingHand)
        MulModifier(&modifier, UQ_4_12(1.5));
    if (gStatuses4[battlerAtk] & STATUS4_GHASTLY_ECHO)
        MulModifier(&modifier, UQ_4_12(1.5));
    if (gStatuses3[battlerAtk] & STATUS3_CHARGED_UP && moveType == TYPE_ELECTRIC)
        MulModifier(&modifier, UQ_4_12(2.0));
    if (gStatuses3[battlerAtk] & STATUS3_ME_FIRST)
        MulModifier(&modifier, UQ_4_12(1.5));
    if (gVolatileStructs[battlerDef].fear)
        MulModifier(&modifier, UQ_4_12(1.5));

    if (GetCurrentTerrain() == STATUS_FIELD_GRASSY_TERRAIN && moveType == TYPE_GRASS && IsBattlerGrounded(battlerAtk) && !(gStatuses3[battlerAtk] & STATUS3_SEMI_INVULNERABLE))
        MulModifier(&modifier, (B_TERRAIN_TYPE_BOOST >= GEN_8) ? UQ_4_12(1.3) : UQ_4_12(1.5));
    if (GetCurrentTerrain() == STATUS_FIELD_ELECTRIC_TERRAIN && moveType == TYPE_ELECTRIC && IsBattlerGrounded(battlerAtk) && !(gStatuses3[battlerAtk] & STATUS3_SEMI_INVULNERABLE))
        MulModifier(&modifier, (B_TERRAIN_TYPE_BOOST >= GEN_8) ? UQ_4_12(1.3) : UQ_4_12(1.5));
    if (GetCurrentTerrain() == STATUS_FIELD_PSYCHIC_TERRAIN && moveType == TYPE_PSYCHIC && IsBattlerGrounded(battlerAtk) && !(gStatuses3[battlerAtk] & STATUS3_SEMI_INVULNERABLE))
        MulModifier(&modifier, (B_TERRAIN_TYPE_BOOST >= GEN_8) ? UQ_4_12(1.3) : UQ_4_12(1.5));
    if (GetCurrentTerrain() == STATUS_FIELD_MISTY_TERRAIN && moveType == TYPE_FAIRY && IsBattlerGrounded(battlerAtk) && !(gStatuses3[battlerAtk] & STATUS3_SEMI_INVULNERABLE))
        MulModifier(&modifier, (B_TERRAIN_TYPE_BOOST >= GEN_8) ? UQ_4_12(1.3) : UQ_4_12(1.5));
	
    return ApplyModifier(modifier, actualPower);
}

u32 CalculateStat(u8 battler, u8 statEnum, u8 secondaryStat, u16 move, bool8 isAttack, bool8 isCrit, bool8 isUnaware, bool8 calculatingSecondary) {
    u32 statBase = 0;
    u8 statStage = gBattleMons[battler].statStages[statEnum];
    u32 extraStat = 0;
    u8 extraStatLevel = 0;
    #define RUIN_CHECK(ability) if (IsAbilityOnFieldExcept(battler, ability) && !BATTLER_HAS_ABILITY(battler, ability)) statBase = statBase * 3 / 4;
    switch (statEnum)
    {
        case STAT_HP:
            return 0;
        case STAT_ATK:
            statBase = gBattleMons[battler].attack;
            extraStatLevel = gVolatileStructs[battler].extraAttackLevel;
            // Tablets of Ruin
            RUIN_CHECK(ABILITY_TABLETS_OF_RUIN)
                    
            // Huge Power
            if (BATTLER_HAS_ABILITY(battler, ABILITY_HUGE_POWER)) statBase *= 2;
                    
            // Pure Power
            if (BATTLER_HAS_ABILITY(battler, ABILITY_PURE_POWER)) statBase *= 2;

            // Dead Power
            if (BATTLER_HAS_ABILITY(battler, ABILITY_DEAD_POWER)) statBase = statBase * 3 / 2;
                    
            // Slow Start
            if (BATTLER_HAS_ABILITY(battler, ABILITY_SLOW_START)
                && gVolatileStructs[battler].slowStartTimer != 0)
                    statBase /= 2;

            // Violent Rush
            if(gVolatileStructs[battler].violentRush) statBase = statBase * 6 / 5;

            // Showdown Mode
            if(gVolatileStructs[battler].showdownMode) statBase = statBase * 6 / 5;

            // Huge Power on First Turn
            if(gVolatileStructs[battler].readiedAction) statBase *= 2;
                    
            // Hadron Engine
            if (BATTLER_HAS_ABILITY(battler, ABILITY_ORICHALCUM_PULSE)
                && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))
                    statBase = statBase * 4 / 3;

            // Burn
            if ((gBattleMons[battler].status1 & STATUS1_BURN)
                && gBattleMoves[move].effect != EFFECT_FACADE
                && !BATTLER_HAS_ABILITY(battler, ABILITY_FLARE_BOOST)
                && !BATTLER_HAS_ABILITY(battler, ABILITY_HEATPROOF)
                && !BATTLER_HAS_ABILITY(battler, ABILITY_IRON_GIANT)
                && !BATTLER_HAS_ABILITY(battler, ABILITY_GUTS))
                    statBase /= 2;

            HANDLE_STAT_CALC_ATK_OR_SPATK:
            // Big Leaves/Solar Power
            if ((BATTLER_HAS_ABILITY(battler, ABILITY_BIG_LEAVES) || BATTLER_HAS_ABILITY(battler, ABILITY_SOLAR_POWER))
                && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)
                && GetHighestAttackingStatId(battler, TRUE) == statEnum)
                    statBase = statBase * 3 / 2;

            // Sand Force
            if ((BATTLER_HAS_ABILITY(battler, ABILITY_SAND_FORCE))
                && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)
                && GetHighestAttackingStatId(battler, TRUE) == statEnum)
                    statBase = statBase * 3 / 2;

            // Fog Power
            if ((BATTLER_HAS_ABILITY(battler, ABILITY_ECTOPLASM))
                && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)
                && GetHighestAttackingStatId(battler, TRUE) == statEnum)
                    statBase = statBase * 3 / 2;
            
            // Supreme Overlord
            if (BATTLER_HAS_ABILITY(battler, ABILITY_SUPREME_OVERLORD))
                statBase = statBase * (10 + max(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
                    
            // Defeatist
            if (BATTLER_HAS_ABILITY(battler, ABILITY_DEFEATIST)
                && gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                    statBase /= 2;
            break;
        case STAT_SPATK:
            statBase = gBattleMons[battler].spAttack;
            extraStatLevel = gVolatileStructs[battler].extraSpAttackLevel;
            // Tablets of Ruin
            RUIN_CHECK(ABILITY_VESSEL_OF_RUIN)
                    
            // Majestic Bird
	        if (BATTLER_HAS_ABILITY(battler, ABILITY_MAJESTIC_BIRD)) statBase = statBase * 3 / 2;
                    
            // Feline Prowess
            if (BATTLER_HAS_ABILITY(battler, ABILITY_FELINE_PROWESS)) statBase *= 2;

            // Special Violent Rush
            if(gVolatileStructs[battler].rapidResponse) statBase = statBase * 6 / 5;
                
            // Flare Boost
            if (BATTLER_HAS_ABILITY(battler, ABILITY_FLARE_BOOST)
                && gBattleMons[battler].status1 & STATUS1_BURN)
                    statBase = statBase * 3 / 2;

            // Flower Gift
            {
            int flowerGiftUser;
            if ((flowerGiftUser = IsAbilityOnSide(battler, ABILITY_FLOWER_GIFT))
                && IsBattlerWeatherAffected(flowerGiftUser - 1, WEATHER_SUN_ANY))
                    statBase = statBase * 3 / 2;
            }

            // Hadron Engine
            if (BATTLER_HAS_ABILITY(battler, ABILITY_HADRON_ENGINE)
                && TERRAIN_HAS_EFFECT && gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
                    statBase = statBase * 4 / 3;
                
            // Frostbite
            if ((gBattleMons[battler].status1 & STATUS1_FROSTBITE)
                && gBattleMoves[move].effect != EFFECT_FACADE
                && !BATTLER_HAS_ABILITY(battler, ABILITY_DETERMINATION))
                    statBase /= 2;
            
            goto HANDLE_STAT_CALC_ATK_OR_SPATK;
            break;
        case STAT_DEF:
            if (isWonderRoomActive()) goto CALCULATE_STAT_SPDEF;
            CALCULATE_STAT_DEF:
            statBase = gBattleMons[battler].defense;
            extraStatLevel = gVolatileStructs[battler].extraDefenseLevel;

            // Sword of Ruin
            RUIN_CHECK(ABILITY_SWORD_OF_RUIN)

            // Sword of Damnation
            RUIN_CHECK(ABILITY_SWORD_OF_DAMNATION)

            // Last Stand
            if (BATTLER_HAS_ABILITY(battler, ABILITY_LAST_STAND))
            {
                statBase = statBase + (statBase * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
            }
                    
            // Marvel Scale
            if (BATTLER_HAS_ABILITY(battler, ABILITY_MARVEL_SCALE)
                && gBattleMons[battler].status1 & STATUS1_ANY)
                    statBase = statBase * 3 / 2;
                    
            // Grass Pelt
            if (BATTLER_HAS_ABILITY(battler, ABILITY_GRASS_PELT)
                && GetCurrentTerrain() == STATUS_FIELD_GRASSY_TERRAIN)
                    statBase = statBase * 3 / 2;
                    
            // Hail
            if (IS_BATTLER_OF_TYPE(battler, TYPE_ICE)
                && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
                    statBase = statBase * 3 / 2;
            break;
        case STAT_SPDEF:
            if (isWonderRoomActive()) goto CALCULATE_STAT_DEF;
            CALCULATE_STAT_SPDEF:
            statBase = gBattleMons[battler].spDefense;
            extraStatLevel = gVolatileStructs[battler].extraSpDefenseLevel;

            // Tablets of Ruin
            RUIN_CHECK(ABILITY_BEADS_OF_RUIN)

            // Last Stand
            if (BATTLER_HAS_ABILITY(battler, ABILITY_LAST_STAND))
            {
                statBase = statBase + (statBase * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
            }

            // Flower Gift
            {
            int flowerGiftUser;
            if ((flowerGiftUser = IsAbilityOnSide(battler, ABILITY_FLOWER_GIFT))
                && IsBattlerWeatherAffected(flowerGiftUser - 1, WEATHER_SUN_ANY))
                    statBase = statBase * 3 / 2;
            }
            
            // Sandstorm
            if (IS_BATTLER_OF_TYPE(battler, TYPE_ROCK)
                && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY))
                    statBase = statBase * 3 / 2;
            break;
        case STAT_SPEED:
            statBase = GetBattlerTotalSpeedStat(battler, calculatingSecondary ? TOTAL_SPEED_SECONDARY : TOTAL_SPEED_PRIMARY);
            extraStatLevel = gVolatileStructs[battler].extraSpeedLevel;
            break;
    }
    if (statEnum != STAT_SPEED && GetAbilityStateAs(battler, ABILITY_PROTOSYNTHESIS).paradoxBoost.statId == statEnum)
        statBase = statBase * 13 / 10;

    if (statEnum != STAT_SPEED && GetAbilityStateAs(battler, ABILITY_QUARK_DRIVE).paradoxBoost.statId == statEnum)
        statBase = statBase * 13 / 10;

    #undef RUIN_CHECK

    if (isUnaware) statStage = DEFAULT_STAT_STAGE;
    else if (isCrit && isAttack) statStage = max(statStage, DEFAULT_STAT_STAGE);
    else if (isCrit && !isAttack) statStage = min(statStage, DEFAULT_STAT_STAGE);

    if (gBattleMons[battler].status1 & STATUS1_BLEED || IsBloodStainAffected(battler)) statStage = min(statStage, DEFAULT_STAT_STAGE);
    
    if (!calculatingSecondary) {
        if (secondaryStat == statEnum && statEnum != STAT_SPEED) statBase = statBase * 6 / 5;
        else if (secondaryStat == statEnum && statEnum == STAT_SPEED) 
        {
            statBase *= gStatStageRatios[statStage][0];
            statBase /= gStatStageRatios[statStage][1];
            statBase += CalculateStat(battler, secondaryStat, 0, move, isAttack, isCrit, isUnaware, TRUE) / 5;
            return statBase;
        }
        else if (secondaryStat) statBase += CalculateStat(battler, secondaryStat, 0, move, isAttack, isCrit, isUnaware, TRUE) / 5;
    }

    statBase *= gStatStageRatios[statStage][0];
    statBase /= gStatStageRatios[statStage][1];
    if (extraStatLevel){
        statBase = statBase + ((statBase / 5 ) * extraStatLevel);
    }
        
    return statBase;
}

static u32 CalcAttackStat(u16 move, u8 battlerAtk, u8 battlerDef, u8 moveType, bool32 isCrit, bool32 updateFlags)
{
    u8 atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_ATK : STAT_SPATK;
    u8 secondaryAtkStatToUse = 0;
    u8 statBattler = battlerAtk;
    //Calculates Highest Attack Stat after stat boosts
    bool8 isUnaware = IsUnaware(battlerDef);
    u8 highestAttackStat = STAT_ATK;
    u32 atkStat;
    u16 modifier;

    if (gBattleMoves[move].effect == EFFECT_FOUL_PLAY)
    {
        isUnaware = IsUnaware(battlerAtk);
        statBattler = battlerDef;
    }
    else if (gBattleMoves[move].effect == EFFECT_BODY_PRESS)
    {
        atkStatToUse = STAT_DEF;
    }
    else {
        // Speed Force
        if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_SPEED_FORCE) && gBattleMoves[move].flags & FLAG_MAKES_CONTACT)
        {
            secondaryAtkStatToUse = STAT_SPEED;
        }
        // Terminal Velocity
        if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_TERMINAL_VELOCITY) && IS_MOVE_SPECIAL(move))
        {
            secondaryAtkStatToUse = STAT_SPEED;
        }
        // Slipstream
        if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_SLIPSTREAM))
        {
            secondaryAtkStatToUse = STAT_SPEED;
        }
        // Power Core
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_POWER_CORE))
        {
            secondaryAtkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_JUGGERNAUT) && gBattleMoves[move].flags & FLAG_MAKES_CONTACT)
        {
            secondaryAtkStatToUse = STAT_DEF;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_IRON_GIANT) && gBattleMoves[move].flags & FLAG_MAKES_CONTACT)
        {
            secondaryAtkStatToUse = STAT_DEF;
        }

	    // Equinox
        if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_EQUINOX))
        {
            u32 atk = CalculateStat(battlerAtk, STAT_ATK, secondaryAtkStatToUse, move, TRUE, isCrit, isUnaware, FALSE);
            u32 spAtk = CalculateStat(battlerAtk, STAT_SPATK, secondaryAtkStatToUse, move, TRUE, isCrit, isUnaware, FALSE);
            atkStatToUse = atk > spAtk ? STAT_ATK : STAT_SPATK;
        }
        // Ancient Idol
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_ANCIENT_IDOL))
        {
            atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MOMENTUM) && gBattleMoves[move].flags & FLAG_MAKES_CONTACT)
        {
            atkStatToUse = STAT_SPEED;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_IMPULSE) && !(gBattleMoves[move].flags & FLAG_MAKES_CONTACT))
        {
            atkStatToUse = STAT_SPEED;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MAXIMUM_ACCELERATION))
        {
            atkStatToUse = STAT_SPEED;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MIND_CRUSH) && gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        {
            atkStatToUse = STAT_SPATK;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_ROUSED_FANGS) && gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        {
            atkStatToUse = STAT_SPATK;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MYTHICAL_ARROWS) && gBattleMoves[move].arrowBased)
        {
            atkStatToUse = STAT_SPATK;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MYSTIC_BLADES) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        {
            atkStatToUse = STAT_SPATK;
        }
        else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PONY_POWER) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        {
            atkStatToUse = STAT_SPATK;
        }
    }

    atkStat = CalculateStat(statBattler, atkStatToUse, secondaryAtkStatToUse, move, TRUE, isCrit, isUnaware, FALSE);

    // apply attack stat modifiers
    modifier = UQ_4_12(1.0);

    // Fog
    if (IS_BATTLER_OF_TYPE(battlerDef, TYPE_GHOST) && IsBattlerWeatherAffected(battlerDef, WEATHER_FOG_ANY) && !gVolatileStructs[battlerDef].trickOrTreat)
        MUL_MODIFIER(&modifier, .8);

    //Infatuation
    if ((gBattleMons[battlerAtk].status2 & STATUS2_INFATUATION) && (gBattleMons[battlerAtk].status2 & STATUS2_INFATUATED_WITH(battlerDef)))   
        MulModifier(&modifier, UQ_4_12(0.5));

    // attacker's hold effect
    switch (GetBattlerHoldEffect(battlerAtk, TRUE))
    {
    case HOLD_EFFECT_THICK_CLUB:
        if ((GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_CUBONE
         || GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_MAROWAK)
         && IS_MOVE_PHYSICAL(move))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case HOLD_EFFECT_DEEP_SEA_TOOTH:
        if (gBattleMons[battlerAtk].species == SPECIES_CLAMPERL && IS_MOVE_SPECIAL(move))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case HOLD_EFFECT_LIGHT_BALL:
        if (GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_PIKACHU)
            MulModifier(&modifier, UQ_4_12(2.0));
        if (GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_PIKACHU_PARTNER)
            MulModifier(&modifier, UQ_4_12(1.6));
        if (GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_PIKACHU_PARTNER_MEGA)
            MulModifier(&modifier, UQ_4_12(1.6));
        if (GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_RAICHU)
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
    case HOLD_EFFECT_LEEK:
        if (GET_BASE_SPECIES_ID(gBattleMons[battlerAtk].species) == SPECIES_FARFETCHD
            && IS_MOVE_PHYSICAL(move))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case HOLD_EFFECT_CHOICE_BAND:
        if (IS_MOVE_PHYSICAL(move))
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
    case HOLD_EFFECT_CHOICE_SPECS:
        if (IS_MOVE_SPECIAL(move))
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
    }
	
    return ApplyModifier(modifier, atkStat);
}

static bool32 CanEvolve(u32 species)
{
    u32 i;

    for (i = 0; i < EVOS_PER_MON; i++)
    {
        if (gEvolutionTable[species][i].method
         && gEvolutionTable[species][i].method != EVO_MEGA_EVOLUTION
         && gEvolutionTable[species][i].method != EVO_MOVE_MEGA_EVOLUTION
         && gEvolutionTable[species][i].method != EVO_PRIMAL_REVERSION)
            return TRUE;
    }
    return FALSE;
}

void SetSwapDamageCategory(int battler, int target, int move)
{
    switch (gBattleMoves[move].splitFlag)
    {
        default:
            gSwapDamageCategory = FALSE;
            return;
        
        case USE_HIGHEST_OFFENSE:
            {
                int isUnaware = IsUnaware(battler);
                int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, FALSE, isUnaware, FALSE);
                int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, FALSE, isUnaware, FALSE);
                if (atk > spAtk) gSwapDamageCategory = gBattleMoves[move].split == SPLIT_SPECIAL;
                else if (atk < spAtk) gSwapDamageCategory = gBattleMoves[move].split == SPLIT_PHYSICAL;
                else gSwapDamageCategory = Random() % 2;
            }
            return;

        case USE_HIGHEST_DAMAGE:
            {
                int isUnaware = IsUnaware(battler);
                int isTargetUnaware = IsUnaware(target);
                // Atk / Def > SpAtk / SpDef is equivalent to Atk * SpDef > SpAtk * Def and doesn't have rounding issues
                int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, FALSE, isUnaware, FALSE) * CalculateStat(target, STAT_SPDEF, 0, move, TRUE, FALSE, isTargetUnaware, FALSE);
                int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, FALSE, isUnaware, FALSE) * CalculateStat(target, STAT_DEF, 0, move, TRUE, FALSE, isTargetUnaware, FALSE);
                if (atk > spAtk) gSwapDamageCategory = gBattleMoves[move].split == SPLIT_SPECIAL;
                else if (atk < spAtk) gSwapDamageCategory = gBattleMoves[move].split == SPLIT_PHYSICAL;
                else gSwapDamageCategory = Random() % 2;
            }
    }
}

u8 CalculateBattlerLowestDefense(u8 battler){
    u8 defense = gBattleMons[battler].defense;
    u8 specialDefense = gBattleMons[battler].spDefense;

    if (defense < specialDefense)
        return STAT_DEF;
    else
        return STAT_SPDEF;
}

u8 CalculateBattlerHighestAttack(u8 battler){
    u8 attack = gBattleMons[battler].attack;
    u8 specialAttack = gBattleMons[battler].spAttack;

    if (attack > specialAttack)
        return STAT_ATK;
    else
        return STAT_SPATK;
}

static u32 CalcDefenseStat(u16 move, u8 battlerAtk, u8 battlerDef, u8 moveType, bool32 isCrit, bool32 updateFlags)
{
    u8 defStatToUse = 0;
    u32 defStat;
    u8 noPositiveStatStages = isCrit || (gBattleMons[battlerDef].status2 & STATUS2_WRAPPED && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_GRIP_PINCER));
    u8 isUnaware = IsUnaware(battlerAtk) || gBattleMoves[move].flags & FLAG_STAT_STAGES_IGNORED;
    u16 modifier;

    if (gBattleMoves[move].splitFlag == HITS_SPDEF)
    {
        defStatToUse = STAT_SPDEF;
    }
    else if (gBattleMoves[move].splitFlag == HITS_DEF || IS_MOVE_PHYSICAL(move))
    {
        defStatToUse = STAT_DEF;
    }
    else // is special
    {
        defStatToUse = STAT_SPDEF;
    }

    if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_POWER_FISTS) && IS_IRON_FIST(battlerAtk, move))
    {
        defStatToUse = STAT_SPDEF;
    }
    else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MYTHICAL_ARROWS) && gBattleMoves[move].arrowBased)
    {
        defStatToUse = STAT_SPDEF;
    }
    else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MYSTIC_BLADES) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) 
    {
        defStatToUse = STAT_SPDEF;
    }
    else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PONY_POWER) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) 
    {
        defStatToUse = STAT_SPDEF;
    }
    else if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_SOUL_CRUSHER) && gBattleMoves[move].hammerBased)
    {
        defStatToUse = STAT_SPDEF;
    }
    if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_POWER_EDGE) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) 
    {
        defStatToUse = STAT_SPDEF;
    }
    
    if ((gBattleMons[battlerAtk].ability == ABILITY_ROUNDHOUSE || 
        BattlerHasInnate(battlerAtk, ABILITY_ROUNDHOUSE)) && 
        gBattleMoves[move].flags & FLAG_STRIKER_BOOST) 
    {
        u32 def = CalculateStat(battlerDef, STAT_DEF, 0, move, FALSE, noPositiveStatStages, isUnaware, FALSE);
        u32 spDef = CalculateStat(battlerDef, STAT_SPDEF, 0, move, FALSE, noPositiveStatStages, isUnaware, FALSE);
        defStat = min(def, spDef);
        defStatToUse = defStat == def ? STAT_DEF : STAT_SPDEF;
    }
    else {
        defStat = CalculateStat(battlerDef, defStatToUse, 0, move, FALSE, noPositiveStatStages, isUnaware, FALSE);
    }

    // apply defense stat modifiers
    modifier = UQ_4_12(1.0);

    // target's hold effects
    switch (GetBattlerHoldEffect(battlerDef, TRUE))
    {
    case HOLD_EFFECT_DEEP_SEA_SCALE:
        if (gBattleMons[battlerDef].species == SPECIES_CLAMPERL && defStatToUse == STAT_SPDEF)
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case HOLD_EFFECT_METAL_POWDER:
        if (gBattleMons[battlerDef].species == SPECIES_DITTO && defStatToUse == STAT_DEF && !(gBattleMons[battlerDef].status2 & STATUS2_TRANSFORMED))
            MulModifier(&modifier, UQ_4_12(2.0));
        break;
    case HOLD_EFFECT_EVIOLITE:
        if (CanEvolve(gBattleMons[battlerDef].species))
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
    case HOLD_EFFECT_ASSAULT_VEST:
        if (defStatToUse == STAT_SPDEF)
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
    case HOLD_EFFECT_TACTICAL_VEST:
        if (defStatToUse == STAT_DEF)
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
#if B_SOUL_DEW_BOOST <= GEN_6
    case HOLD_EFFECT_SOUL_DEW:
        if ((GET_BASE_SPECIES_ID(gBattleMons[battlerDef].species) == SPECIES_LATIAS || GET_BASE_SPECIES_ID(gBattleMons[battlerDef].species) == SPECIES_LATIOS)
         && !(gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
         && defStatToUse == STAT_SPDEF)
            MulModifier(&modifier, UQ_4_12(1.5));
        break;
#endif
    }

    return ApplyModifier(modifier, defStat);
}

u8 StabMultiplierInHalves(u8 battler, u8 moveType, u16 ability, u16 move)
{
    if (move == MOVE_STRUGGLE) return 2;
    if (IS_BATTLER_OF_TYPE(battler, moveType)
	    || BATTLER_HAS_ABILITY_FAST(battler, ABILITY_MYSTIC_POWER, ability)
	    || BATTLER_HAS_ABILITY_FAST(battler, ABILITY_ARCANE_FORCE, ability)
        || (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_LUNAR_ECLIPSE, ability) && (moveType == TYPE_FAIRY || moveType == TYPE_DARK))
        || (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_MOON_SPIRIT, ability) && (moveType == TYPE_FAIRY || moveType == TYPE_DARK))
        || (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_SOLAR_FLARE, ability) && moveType == TYPE_FIRE)
		|| (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_AURORA_BOREALIS, ability) && moveType == TYPE_ICE)
        || (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_AMPHIBIOUS, ability) && moveType == TYPE_WATER))
    {
        if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_ADAPTABILITY, ability) || BATTLER_HAS_ABILITY_FAST(battler, ABILITY_RKS_SYSTEM, ability))
            return 4;
        else
            return 3;
    }
    return 2;
}

u32 CalcFinalDmg(u32 dmg, u16 move, u8 battlerAtk, u8 battlerDef, u8 moveType, u16 typeEffectivenessModifier, bool32 isCrit, bool32 updateFlags)
{
    u32 percentBoost;
    u32 abilityAtk = GetBattlerAbility(battlerAtk);
    u32 abilityDef = GetBattlerAbility(battlerDef);
    u32 defSide = GET_BATTLER_SIDE(battlerDef);
    u16 finalModifier = UQ_4_12(1.0);
    u16 itemDef = gBattleMons[battlerDef].item;
    u16 ignored;

    // check multiple targets in double battle
    if (GetMoveTargetCount(move, battlerAtk, battlerDef) >= 2)
        MulModifier(&finalModifier, UQ_4_12(0.75));

    // take type effectiveness
    MulModifier(&finalModifier, typeEffectivenessModifier);

    MulModifier(&finalModifier, CalculateAbilityMultipliers(battlerAtk, battlerDef, move, moveType, CalcMoveBasePower(move, battlerAtk, battlerDef), typeEffectivenessModifier, isCrit, &ignored));

    // check crit
    if (isCrit)
    {
        if (gBattleMoves[move].effect == EFFECT_MISC_HIT && gBattleMoves[move].argument == MISC_EFFECT_INCREASED_CRIT_DAMAGE)
        {
            dmg = ApplyModifier(UQ_4_12(2.0), dmg);
        }
        else
        {
            dmg = ApplyModifier(UQ_4_12(1.5), dmg);
        }
    }

    #define CHECK_WEATHER_DOUBLE_BOOST(boost, drop) (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_WEATHER_DOUBLE_BOOST) ? UQ_4_12(boost) : UQ_4_12(drop))

    // check sunny/rain weather
    if (IsBattlerWeatherAffected(battlerAtk, WEATHER_RAIN_PERMANENT))
    {
        if (gBattleMoves[move].effect == EFFECT_WEATHER_BOOST)
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.2 * 1.2, 1.2), dmg);
        else if (moveType == TYPE_FIRE)
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.2, 0.5), dmg);
        else if (moveType == TYPE_WATER)
            dmg = ApplyModifier(UQ_4_12(1.2), dmg);
    }
    else if (IsBattlerWeatherAffected(battlerAtk, WEATHER_RAIN_TEMPORARY))
    {
        if (gBattleMoves[move].effect == EFFECT_WEATHER_BOOST)
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.5 * 1.5, 1.5), dmg);
        else if (moveType == TYPE_FIRE)
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.5, 0.5), dmg);
        else if (moveType == TYPE_WATER)
            dmg = ApplyModifier(UQ_4_12(1.5), dmg);
    }
    else if (IsBattlerWeatherAffected(battlerAtk, WEATHER_SUN_PERMANENT))
    {
        if (gBattleMoves[move].effect == EFFECT_WEATHER_BOOST)
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.2 * 1.2, 1.2), dmg);
        else if (moveType == TYPE_FIRE)
            dmg = ApplyModifier(UQ_4_12(1.2), dmg);
        else if (moveType == TYPE_WATER && !BATTLER_HAS_ABILITY(battlerAtk, ABILITY_NIKA))
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.2, 0.5), dmg);
    }
    else if (IsBattlerWeatherAffected(battlerAtk, WEATHER_SUN_TEMPORARY))
    {
        if (gBattleMoves[move].effect == EFFECT_WEATHER_BOOST)
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.5 * 1.5, 1.5), dmg);
        else if (moveType == TYPE_FIRE)
            dmg = ApplyModifier(UQ_4_12(1.5), dmg);
        else if (moveType == TYPE_WATER && !BATTLER_HAS_ABILITY(battlerAtk, ABILITY_NIKA))
            dmg = ApplyModifier(CHECK_WEATHER_DOUBLE_BOOST(1.5, 0.5), dmg);
    }

    #undef CHECK_WEATHER_DOUBLE_BOOST

    // check stab
    switch (StabMultiplierInHalves(battlerAtk, moveType, abilityAtk, move))
    {
        case 4:
            MulModifier(&finalModifier, UQ_4_12(2.0));
            break;
        case 3:
            MulModifier(&finalModifier, UQ_4_12(1.5));
            break;
    }

    // reflect, light screen, aurora veil
    if (((gSideStatuses[defSide] & SIDE_STATUS_REFLECT && IS_MOVE_PHYSICAL(move))
            || (gSideStatuses[defSide] & SIDE_STATUS_LIGHTSCREEN && IS_MOVE_SPECIAL(move))
            || (gSideStatuses[defSide] & SIDE_STATUS_AURORA_VEIL))
        && !BATTLER_HAS_ABILITY_FAST(battlerAtk, ABILITY_INFILTRATOR, abilityAtk)
        && !BATTLER_HAS_ABILITY_FAST(battlerAtk, ABILITY_MARINE_APEX, abilityAtk)
        && !(BATTLER_HAS_ABILITY_FAST(battlerAtk, ABILITY_PINNACLE_BLADE, abilityAtk) && gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        && !(isCrit)
        && !gRoundStructs[gBattlerAttacker].confusionSelfDmg)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
            MulModifier(&finalModifier, UQ_4_12(0.66));
        else
            MulModifier(&finalModifier, UQ_4_12(0.5));
    }
    
    switch (gTurnStructs[gBattlerAttacker].parentalBondTrigger) {
        case ABILITY_HYPER_AGGRESSIVE:
        case ABILITY_PARENTAL_BOND:
            if (gTurnStructs[gBattlerAttacker].parentalBondOn == 1)
                MulModifier(&finalModifier, UQ_4_12(0.25));
            break;
        case ABILITY_MULTI_HEADED:
            if(gBaseStats[gBattleMons[gBattlerAttacker].species].flags & F_TWO_HEADED){
                if (gTurnStructs[gBattlerAttacker].parentalBondOn == 1)
                    MulModifier(&finalModifier, UQ_4_12(0.25));
            } else {
                if(gTurnStructs[gBattlerAttacker].parentalBondOn == 2)
                    MulModifier(&finalModifier, UQ_4_12(0.2));
                else if(gTurnStructs[gBattlerAttacker].parentalBondOn == 1)
                    MulModifier(&finalModifier, UQ_4_12(0.15));
            }
            break;
        case ABILITY_MINION_CONTROL:
            if (gTurnStructs[gBattlerAttacker].parentalBondOn < gTurnStructs[gBattlerAttacker].parentalBondInitialCount)
                MulModifier(&finalModifier, UQ_4_12(0.1));
            break;
        case ABILITY_PRIMAL_MAW:
        case ABILITY_DEVOURER:
        case ABILITY_RAGING_BOXER:
            if (gTurnStructs[gBattlerAttacker].parentalBondOn == 1)
                MulModifier(&finalModifier, UQ_4_12(0.5));
            break;
        case ABILITY_DUAL_HAMMER:
        case ABILITY_DUAL_WIELD:
        case ABILITY_RAGING_MOTH:
		    MulModifier(&finalModifier, UQ_4_12(0.75));
            break;
    }
	
    // target's ally's abilities
    if (BATTLER_HAS_ABILITY(BATTLE_PARTNER(battlerDef), ABILITY_FRIEND_GUARD))
        MulModifier(&finalModifier, UQ_4_12(0.5)); // was 0.75

    // attacker's hold effect
    switch (GetBattlerHoldEffect(battlerAtk, TRUE))
    {
    case HOLD_EFFECT_METRONOME:
        percentBoost = min((gBattleStruct->sameMoveTurns[battlerAtk] * GetBattlerHoldEffectParam(battlerAtk)), 100);
        MulModifier(&finalModifier, UQ_4_12(1.0) + sPercentToModifier[percentBoost]);
        break;
    case HOLD_EFFECT_EXPERT_BELT:
        if (typeEffectivenessModifier >= UQ_4_12(2.0))
            MulModifier(&finalModifier, UQ_4_12(1.2));
        break;
    case HOLD_EFFECT_LIFE_ORB:
        MulModifier(&finalModifier, UQ_4_12(1.3));
        break;
    case HOLD_EFFECT_PUNCHING_GLOVE:
        if (IS_IRON_FIST(battlerAtk, move))
            MulModifier(&finalModifier, UQ_4_12(1.1));
        break;
    }

    // target's hold effect
    switch (GetBattlerHoldEffect(battlerDef, TRUE))
    {
    // berries reducing dmg
    case HOLD_EFFECT_RESIST_BERRY:
        if (moveType == GetBattlerHoldEffectParam(battlerDef)
            && (moveType == TYPE_NORMAL || typeEffectivenessModifier >= UQ_4_12(2.0))
            && !UnnerveOn(battlerDef, itemDef))
        {
            if (HasRipenEffect(battlerDef))
                MulModifier(&finalModifier, UQ_4_12(0.25));
            else
                MulModifier(&finalModifier, UQ_4_12(0.5));
            if (updateFlags)
                gTurnStructs[battlerDef].berryReduced = TRUE;
        }
        break;
    }

    if (gRoundStructs[battlerDef].glaiveRush)
        MulModifier(&finalModifier, UQ_4_12(2.0));

    if (gBattleMoves[move].flags & FLAG_DMG_UNDERGROUND && gStatuses3[battlerDef] & STATUS3_UNDERGROUND)
        MulModifier(&finalModifier, UQ_4_12(2.0));
    if (gBattleMoves[move].flags & FLAG_DMG_UNDERWATER  && gStatuses3[battlerDef] & STATUS3_UNDERWATER)
        MulModifier(&finalModifier, UQ_4_12(2.0));
    if (gBattleMoves[move].flags & FLAG_DMG_2X_IN_AIR   && gStatuses3[battlerDef] & STATUS3_ON_AIR)
        MulModifier(&finalModifier, UQ_4_12(2.0));

    if (typeEffectivenessModifier >= UQ_4_12(2.0) && gBattleMoves[move].effect == EFFECT_MISC_HIT && gBattleMoves[move].argument == MISC_EFFECT_SUPEREFFECTIVE_BOOST)
    {
        MulModifier(&finalModifier, UQ_4_12(4.0/3.0));
    }

    dmg = ApplyModifier(finalModifier, dmg);
    if (dmg == 0)
        dmg = 1;

    return dmg;
}

s32 DoMoveDamageCalcInternal(u16 move, u8 battlerAtk, u8 battlerDef, u8 moveType, s32 fixedBasePower,
                            bool32 isCrit, bool32 updateFlags, u16 typeEffectivenessModifier)
{
    s32 dmg;

    // Don't calculate damage if the move has no effect on target.
    if (typeEffectivenessModifier == UQ_4_12(0))
        return -1;

    gBattleMovePower = CalcMoveBasePowerAfterModifiers(move, fixedBasePower, battlerAtk, battlerDef, moveType, updateFlags);

    // long dmg basic formula
    dmg = ((gBattleMons[battlerAtk].level * 2) / 5) + 2;
    dmg *= gBattleMovePower;
    dmg *= CalcAttackStat(move, battlerAtk, battlerDef, moveType, isCrit, updateFlags);
    dmg /= CalcDefenseStat(move, battlerAtk, battlerDef, moveType, isCrit, updateFlags);
    dmg = (dmg / 50) + 2;

    // Calculate final modifiers.
    dmg = CalcFinalDmg(dmg, move, battlerAtk, battlerDef, moveType, typeEffectivenessModifier, isCrit, updateFlags);

    // Monotype Champ
    switch(getMonotypeChampType()){
        case TYPE_GRASS:
        case TYPE_PSYCHIC:
        case TYPE_ELECTRIC:
        case TYPE_POISON:
        case TYPE_STEEL:
            if(GetBattlerSide(battlerDef) != B_SIDE_PLAYER)
                dmg = dmg / 2;
        break;
        case TYPE_GROUND:
            // SE Moves are halved
            if(GetBattlerSide(battlerDef) != B_SIDE_PLAYER){
                if(typeEffectivenessModifier >= UQ_4_12(2.0))
                    dmg = dmg / 2;
            }
        break;
        case TYPE_FIGHTING:
            // SE Moves are halved
            if(GetBattlerSide(battlerDef) != B_SIDE_PLAYER){
                if(typeEffectivenessModifier >= UQ_4_12(2.0))
                    dmg = dmg / 2;
            }
            else if(GetBattlerSide(battlerDef) == B_SIDE_PLAYER && 
                    GetBattlerSide(battlerAtk) != B_SIDE_PLAYER && 
                    gBattleMoves[move].type == TYPE_FIGHTING){
                dmg = dmg * 2;
            }
        break;
        case TYPE_ICE:
            // Moves do double damage
            if(GetBattlerSide(battlerDef) == B_SIDE_PLAYER && GetBattlerSide(battlerAtk) != B_SIDE_PLAYER){
                dmg = dmg * 2;
            }
        break;
    }

    return dmg;
}

static s32 DoMoveDamageCalc(u16 move, u8 battlerAtk, u8 battlerDef, u8* moveType, s32 fixedBasePower,
                            bool32 isCrit, bool32 randomFactor, bool32 updateFlags, u16* typeEffectivenessModifier)
{
    s32 dmg;
    SetSwapDamageCategory(battlerAtk, battlerDef, move);
    *typeEffectivenessModifier = CalcTypeEffectivenessMultiplier(move, *moveType, battlerAtk, battlerDef, updateFlags);
    dmg = DoMoveDamageCalcInternal(move, battlerAtk, battlerDef, *moveType, fixedBasePower, isCrit, updateFlags, *typeEffectivenessModifier);

    if (gBattleMoves[move].type2 && gBattleMoves[move].type2 != *moveType && gBattleMoves[move].type2 != TYPE_MYSTERY)
    {
        u8 type2 = gBattleMoves[move].type2;
        u16 typeEffectivenessModifier2 = CalcTypeEffectivenessMultiplier(move, type2, battlerAtk, battlerDef, FALSE);
        s32 dmg2 = DoMoveDamageCalcInternal(move, battlerAtk, battlerDef, type2, fixedBasePower, isCrit, FALSE, typeEffectivenessModifier2);

        if (dmg2 > dmg)
        {
            *typeEffectivenessModifier = typeEffectivenessModifier2;
            dmg = dmg2;
            *moveType = type2;
            if (updateFlags) UpdateMoveResultFlags(typeEffectivenessModifier2);
        }
    }

    if (dmg < 0) return 0;

    // Add a random factor.
    if (randomFactor)
    {
        s32 roll = (IsAbilityOnSide(battlerDef, ABILITY_BAD_LUCK) || IsAbilityOnSide(battlerDef, ABILITY_BAD_OMEN)) ? 15 : Random() % 16;
        dmg *= 100 - roll;
        dmg /= 100;
    }

    if (dmg == 0)
        dmg = 1;

    return dmg;
}

s32 DoMoveDamageCalcBattleMenu(u16 move, u8 battlerAtk, u8 battlerDef, u8* moveType, bool32 isCrit, u8 randomFactor, u16* typeEffectivenessModifier)
{
    s32 dmg = DoMoveDamageCalc(move, battlerAtk, battlerDef, moveType, 0, isCrit, FALSE, FALSE, typeEffectivenessModifier);

    if (IsAbilityOnSide(battlerDef, ABILITY_BAD_LUCK) || IsAbilityOnSide(battlerDef, ABILITY_BAD_OMEN)) randomFactor = 16;

    // Add a random factor.
    dmg *= 100 - randomFactor;
    dmg /= 100;

    if (dmg == 0 && *typeEffectivenessModifier > 0)
        dmg = 1;

    return dmg;
}

s32 CalculateMoveDamage(u16 move, u8 battlerAtk, u8 battlerDef, u8* moveType, s32 fixedBasePower, bool32 isCrit, bool32 randomFactor, bool32 updateFlags)
{
    u16 typeEffectiveness;
    return DoMoveDamageCalc(move, battlerAtk, battlerDef, moveType, fixedBasePower, isCrit, randomFactor,
                            updateFlags, &typeEffectiveness);
}

// for AI - get move damage and effectiveness with one function call
s32 CalculateMoveDamageAndEffectiveness(u16 move, u8 battlerAtk, u8 battlerDef, u8* moveType, u16 *typeEffectivenessModifier)
{
    int val = DoMoveDamageCalc(move, battlerAtk, battlerDef, moveType, 0, FALSE, FALSE, FALSE, typeEffectivenessModifier);
    gSwapDamageCategory = FALSE;
    return val;
}

void MulByTypeEffectiveness(u16 *modifier, u16 move, u8 moveType, u8 battlerDef, u8 defType, u8 battlerAtk, bool32 recordAbilities)
{
    u16 mod = GetTypeModifier(moveType, defType);

    if (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_NORMALIZE) && moveType == TYPE_NORMAL && mod != UQ_4_12(0.0))
    {
        mod = UQ_4_12(1.0);
    }
    else if (mod == UQ_4_12(0.0) && GetBattlerHoldEffect(battlerDef, TRUE) == HOLD_EFFECT_RING_TARGET)
    {
        mod = UQ_4_12(1.0);
        if (recordAbilities)
            RecordItemEffectBattle(battlerDef, HOLD_EFFECT_RING_TARGET);
    }
    else if ((moveType == TYPE_FIGHTING || moveType == TYPE_NORMAL) && defType == TYPE_GHOST && gBattleMons[battlerDef].status2 & STATUS2_FORESIGHT && mod == UQ_4_12(0.0))
    {
        mod = UQ_4_12(1.0);
    }
    else if ((moveType == TYPE_FIGHTING || moveType == TYPE_NORMAL) && defType == TYPE_GHOST && (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_SCRAPPY) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_BLIND_RAGE) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MINDS_EYE)) && mod == UQ_4_12(0.0))
    {
        mod = UQ_4_12(1.0);
    }
    else if (moveType == TYPE_GHOST && defType == TYPE_NORMAL && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PHANTOM_PAIN))
    {   
        mod = UQ_4_12(1.0);
        if (recordAbilities)
            RecordAbilityBattle(battlerAtk, ABILITY_PHANTOM_PAIN);
    }
	else if (mod == UQ_4_12(0.0) && moveType == TYPE_ELECTRIC && defType == TYPE_GROUND && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_GROUND_SHOCK))
    {
		mod = UQ_4_12(0.5);
        if (recordAbilities)
            RecordAbilityBattle(battlerAtk, ABILITY_GROUND_SHOCK);
    }
	else if (moveType == TYPE_ELECTRIC && defType == TYPE_ELECTRIC && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_OVERCHARGE) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_DEPRAVITY))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(2.0); // super-effective
        if (recordAbilities)
            RecordAbilityBattle(battlerAtk, ABILITY_OVERCHARGE);
    }
	// Molten Down
	else if (moveType == TYPE_FIRE && defType == TYPE_ROCK && (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MOLTEN_DOWN) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PYROCLASTIC_FLOW)))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(2.0); // super-effective
    }
    // Magma Eater
    else if (moveType == TYPE_FIRE && defType == TYPE_ROCK && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MAGMA_EATER))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(2.0); // super-effective
        if (recordAbilities)
            RecordAbilityBattle(battlerAtk, ABILITY_MAGMA_EATER);
    }
	else if (moveType == TYPE_POISON && defType == TYPE_STEEL && (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_CORROSION) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_PYROCLASTIC_FLOW) || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_TRASH_HEAP)))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(2.0); // super-effective
    }
	else if (move == MOVE_POISON_STING && defType == TYPE_STEEL && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_ANGELS_WRATH))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(2.0); // super-effective
    }
	else if (move == MOVE_ELECTROWEB && defType == TYPE_GROUND && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_ANGELS_WRATH))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(2.0); // super-effective
    }
	else if (mod == UQ_4_12(0.0) && moveType == TYPE_DRAGON && defType == TYPE_FAIRY && BATTLER_HAS_ABILITY(battlerAtk, ABILITY_OVERWHELM))
    {
		//Has Innate Effect here too
        mod = UQ_4_12(1.0);
        if (recordAbilities)
            RecordAbilityBattle(battlerAtk, ABILITY_OVERWHELM);
    }
	else if (moveType == TYPE_FAIRY && (defType == TYPE_POISON || defType == TYPE_STEEL) && GetBattlerSide(battlerDef) == B_SIDE_PLAYER && getMonotypeChampType() == TYPE_FAIRY)
    {
        mod = UQ_4_12(2.0); // super-effective
    }
	else if (mod == UQ_4_12(0.0) && GetBattlerSide(battlerDef) == B_SIDE_PLAYER && getMonotypeChampType() == TYPE_DRAGON)
    {
        mod = UQ_4_12(1.0);
    }
	else if (getMonotypeChampType() == TYPE_ROCK)
    {
        if(GetBattlerSide(battlerDef) == B_SIDE_PLAYER && defType == TYPE_ROCK && moveType == TYPE_ROCK)
            mod = UQ_4_12(2.0); // super-effective
        else if(GetBattlerSide(battlerDef) != B_SIDE_PLAYER && moveType == TYPE_ROCK)
            mod = UQ_4_12(0.0); // Immune
    }

    if (moveType == TYPE_PSYCHIC && defType == TYPE_DARK && gStatuses3[battlerDef] & STATUS3_MIRACLE_EYED && mod == UQ_4_12(0.0))
        mod = UQ_4_12(1.0);
    if(gBattleMoves[move].effect == EFFECT_IGNORE_TYPE_IMMUNITY && mod == UQ_4_12(0.0))
        mod = UQ_4_12(1.0);
    if(gBattleMoves[move].effect == EFFECT_SE_AGAINST_TYPE_HIT && defType == gBattleMoves[move].argument)
        mod = UQ_4_12(2.0); // super-effective
    if (gBattleMoves[move].effect == EFFECT_FREEZE_DRY && defType == TYPE_WATER)
        mod = UQ_4_12(2.0); // super-effective
    if (gBattleMoves[move].effect == EFFECT_EXCALIBUR && defType == TYPE_DRAGON)
        mod = UQ_4_12(2.0); // super-effective
    if (gBattleMoves[move].effect == EFFECT_ACID && defType == TYPE_STEEL)
        mod = UQ_4_12(2.0);
    if (gBattleMoves[move].effect == EFFECT_SLUDGE && defType == TYPE_WATER)
        mod = UQ_4_12(2.0);
    if (gBattleMoves[move].effect == EFFECT_POISON_GAS && defType == TYPE_FLYING)
        mod = UQ_4_12(2.0);
    if (moveType == TYPE_GROUND && defType == TYPE_FLYING && IsBattlerGrounded(battlerDef) && mod == UQ_4_12(0.0))
        mod = UQ_4_12(1.0);
    if (moveType == TYPE_FIRE && gVolatileStructs[battlerDef].tarShot)
        mod = UQ_4_12(2.0); // super-effective

    // WEATHER_STRONG_WINDS weakens Super Effective moves against Flying-type Pokémon
    if (gBattleWeather & WEATHER_STRONG_WINDS && WEATHER_HAS_EFFECT)
    {
        if (defType == TYPE_FLYING && mod >= UQ_4_12(2.0))
            mod = UQ_4_12(1.0);
    }

    MulModifier(modifier, mod);
}

static void TryNoticeIllusionInTypeEffectiveness(u32 move, u32 moveType, u32 battlerAtk, u32 battlerDef, u16 resultingModifier, u32 illusionSpecies)
{
    // Check if the type effectiveness would've been different if the pokemon really had the types as the disguise.
    u16 presumedModifier = UQ_4_12(1.0);
    MulByTypeEffectiveness(&presumedModifier, move, moveType, battlerDef, gBaseStats[illusionSpecies].type1, battlerAtk, FALSE);
    if (gBaseStats[illusionSpecies].type2 != gBaseStats[illusionSpecies].type1)
        MulByTypeEffectiveness(&presumedModifier, move, moveType, battlerDef, gBaseStats[illusionSpecies].type2, battlerAtk, FALSE);

    if (presumedModifier != resultingModifier)
        RecordAbilityBattle(battlerDef, ABILITY_ILLUSION);
}

static void UpdateMoveResultFlags(u16 modifier)
{
    if (modifier == UQ_4_12(0.0))
    {
        gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
        gMoveResultFlags &= ~(MOVE_RESULT_NOT_VERY_EFFECTIVE | MOVE_RESULT_SUPER_EFFECTIVE);
    }
    else if (modifier == UQ_4_12(1.0))
    {
        gMoveResultFlags &= ~(MOVE_RESULT_NOT_VERY_EFFECTIVE | MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_DOESNT_AFFECT_FOE);
    }
    else if (modifier > UQ_4_12(1.0))
    {
        gMoveResultFlags |= MOVE_RESULT_SUPER_EFFECTIVE;
        gMoveResultFlags &= ~(MOVE_RESULT_NOT_VERY_EFFECTIVE | MOVE_RESULT_DOESNT_AFFECT_FOE);
    }
    else //if (modifier < UQ_4_12(1.0))
    {
        gMoveResultFlags |= MOVE_RESULT_NOT_VERY_EFFECTIVE;
        gMoveResultFlags &= ~(MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_DOESNT_AFFECT_FOE);
    }
}

static u16 CalcTypeEffectivenessMultiplierInternal(u16 move, u8 moveType, u8 battlerAtk, u8 battlerDef, bool32 recordAbilities, u16 modifier)
{
    u32 illusionSpecies;
    u16 modifier1, modifier2, modifier3, modifierInitial;
    u8 currentAttackBattler = gBattlerAttacker;
    u16 immunityAbility = 0;
    gBattlerAttacker = battlerAtk;
    modifierInitial = modifier;
    modifier1 = modifier2 = modifier3 = UQ_4_12(1.0);
    MulByTypeEffectiveness(&modifier1, move, moveType, battlerDef, gBattleMons[battlerDef].type1, battlerAtk, recordAbilities);
    if (gBattleMons[battlerDef].type2 != gBattleMons[battlerDef].type1)
        MulByTypeEffectiveness(&modifier2, move, moveType, battlerDef, gBattleMons[battlerDef].type2, battlerAtk, recordAbilities);
    if (gBattleMons[battlerDef].type3 != TYPE_MYSTERY && gBattleMons[battlerDef].type3 != gBattleMons[battlerDef].type2
        && gBattleMons[battlerDef].type3 != gBattleMons[battlerDef].type1)
        MulByTypeEffectiveness(&modifier3, move, moveType, battlerDef, gBattleMons[battlerDef].type3, battlerAtk, recordAbilities);

    MulModifier(&modifier, modifier1);
    MulModifier(&modifier, modifier2);
    MulModifier(&modifier, modifier3);

    if (recordAbilities && (illusionSpecies = GetIllusionMonSpecies(battlerDef)))
        TryNoticeIllusionInTypeEffectiveness(move, moveType, battlerAtk, battlerDef, modifier, illusionSpecies);

    if (moveType == TYPE_GROUND && !IsBattlerGroundedIgnoreType(battlerDef)
        && !(gBattleMoves[move].flags & FLAG_DMG_UNGROUNDED_IGNORE_TYPE_IF_FLYING)
        && !(BATTLER_HAS_ABILITY(battlerAtk, ABILITY_DESERT_SPIRIT) && IsBattlerWeatherAffected(battlerDef, WEATHER_SANDSTORM_ANY))) // Moves that ignore ground immunity
    {
        if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_LEVITATE))
            immunityAbility = ABILITY_LEVITATE;
        else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_HUGE_WINGS))
            immunityAbility = ABILITY_HUGE_WINGS;
        else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_AERIALIST))
            immunityAbility = ABILITY_AERIALIST;
        else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_DRAGONFLY))
            immunityAbility = ABILITY_DRAGONFLY;
        else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_HOVER))
            immunityAbility = ABILITY_HOVER;
        
        modifier = UQ_4_12(0.0);
    }
    else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_MOUNTAINEER) && moveType == TYPE_ROCK)
    {
        modifier = UQ_4_12(0.0);
        immunityAbility = ABILITY_MOUNTAINEER;
    }
    else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_GIFTED_MIND) && (moveType == TYPE_DARK || moveType == TYPE_GHOST || moveType == TYPE_BUG))
    {
        modifier = UQ_4_12(0.0);
        immunityAbility = ABILITY_GIFTED_MIND;
    }
    else if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_EVAPORATE) && moveType == TYPE_WATER)
    {
        modifier = UQ_4_12(0.0);
        immunityAbility = ABILITY_EVAPORATE;
    }

    if(BATTLER_HAS_ABILITY(battlerAtk, ABILITY_BONE_ZONE) && gBattleMoves[move].flags & FLAG_BONE_BASED && modifier < UQ_4_12(1.0)){
        modifier = UQ_4_12(1.0);
        if (modifier1) MulModifier(&modifier, modifier1);
        if (modifier2) MulModifier(&modifier, modifier2);
        if (modifier3) MulModifier(&modifier, modifier3);
        if (modifier < UQ_4_12(1.0)) MulModifier(&modifier, UQ_4_12(2.0));
    }

    // Thousand Arrows ignores type modifiers for flying mons
    if (!IsBattlerGrounded(battlerDef) && (gBattleMoves[move].flags & FLAG_DMG_UNGROUNDED_IGNORE_TYPE_IF_FLYING
        || (BATTLER_HAS_ABILITY(battlerAtk, ABILITY_DESERT_SPIRIT) && IsBattlerWeatherAffected(battlerDef, WEATHER_SANDSTORM_ANY)))
        && moveType == TYPE_GROUND
        && modifier == UQ_4_12(0))
    {
        modifier = UQ_4_12(1.0);
    }

    if (modifier > UQ_4_12(0.5)
        && gBattleMons[battlerDef].hp == gBattleMons[battlerDef].maxHP
        && BATTLER_HAS_ABILITY(battlerDef, ABILITY_TERA_SHELL))
        modifier = UQ_4_12(0.5);

    if (BATTLER_HAS_ABILITY(battlerDef, ABILITY_WONDER_GUARD) && modifier <= UQ_4_12(1.0) && gBattleMoves[move].power)
    {
        modifier = UQ_4_12(0.0);
        immunityAbility = ABILITY_WONDER_GUARD;
    }
    else if (GetBattlerAbility(battlerDef) == ABILITY_TELEPATHY && battlerDef == BATTLE_PARTNER(battlerAtk) && gBattleMoves[move].power)
    {
        modifier = UQ_4_12(0.0);
        immunityAbility = ABILITY_TELEPATHY;
    }

    
    if (recordAbilities && immunityAbility && !modifier)
    {
        SetActiveAbilityPopupOverride(immunityAbility);
        gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = immunityAbility;
        gMoveResultFlags |= (MOVE_RESULT_MISSED | MOVE_RESULT_DOESNT_AFFECT_FOE);
        gLastLandedMoves[battlerDef] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_DMG;
        RecordAbilityBattle(battlerDef, immunityAbility);
    }

    gBattlerAttacker = currentAttackBattler;

    return modifier;
}

u16 CalcTypeEffectivenessMultiplier(u16 move, u8 moveType, u8 battlerAtk, u8 battlerDef, bool32 recordAbilities)
{
    u16 modifier = UQ_4_12(1.0);

    if (move != MOVE_STRUGGLE && moveType != TYPE_MYSTERY)
    {
        modifier = CalcTypeEffectivenessMultiplierInternal(move, moveType, battlerAtk, battlerDef, recordAbilities, modifier);
    }

    if (modifier != UQ_4_12(0.0) && GetBattleMoveSplit(move) == SPLIT_STATUS)
        modifier = UQ_4_12(1.0);

    if (recordAbilities)
        UpdateMoveResultFlags(modifier);
    return modifier;
}

u16 CalcPartyMonTypeEffectivenessMultiplier(u16 move, u16 speciesDef, u16 abilityDef, u8 battlerDef)
{
    u16 modifier = UQ_4_12(1.0);
    u8 moveType = gBattleMoves[move].type;

    if (move != MOVE_STRUGGLE && moveType != TYPE_MYSTERY)
    {
        MulByTypeEffectiveness(&modifier, move, moveType, 0, gBaseStats[speciesDef].type1, 0, FALSE);
        if (gBaseStats[speciesDef].type2 != gBaseStats[speciesDef].type1)
            MulByTypeEffectiveness(&modifier, move, moveType, 0, gBaseStats[speciesDef].type2, 0, FALSE);

        if (moveType == TYPE_GROUND && (BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_LEVITATE, abilityDef) || BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_DRAGONFLY, abilityDef) || BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_HUGE_WINGS, abilityDef) || BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_HOVER, abilityDef) || BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_AERIALIST, abilityDef)) && !(gFieldStatuses & STATUS_FIELD_GRAVITY))
            modifier = UQ_4_12(0.0);
		if (moveType == TYPE_ROCK && (BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_MOUNTAINEER, abilityDef)))
            modifier = UQ_4_12(0.0);
		if ((moveType == TYPE_DARK || moveType == TYPE_GHOST || moveType == TYPE_BUG) && 
            (BATTLER_HAS_ABILITY_FAST(battlerDef, ABILITY_GIFTED_MIND, abilityDef))) 
            modifier = UQ_4_12(0.0);
        if (abilityDef == ABILITY_WONDER_GUARD && modifier <= UQ_4_12(1.0) && gBattleMoves[move].power)
            modifier = UQ_4_12(0.0);
    }

    UpdateMoveResultFlags(modifier);
    return modifier;
}

u16 GetTypeModifier(u8 atkType, u8 defType)
{
    if(IsInverseRoomActive()){
        if (B_FLAG_INVERSE_BATTLE != 0 && FlagGet(B_FLAG_INVERSE_BATTLE))
            return sTypeEffectivenessTable[atkType][defType];
        else
            return sInverseTypeEffectivenessTable[atkType][defType];
    }
    else{
        if (B_FLAG_INVERSE_BATTLE != 0 && FlagGet(B_FLAG_INVERSE_BATTLE))
            return sInverseTypeEffectivenessTable[atkType][defType];
        else
            return sTypeEffectivenessTable[atkType][defType];
    }
}

s32 GetStealthHazardDamage(u8 hazardType, u8 battlerId)
{
    u8 type1 = gBattleMons[battlerId].type1;
    u8 type2 = gBattleMons[battlerId].type2;
    u8 type3 = gBattleMons[battlerId].type3;
    u32 maxHp = gBattleMons[battlerId].maxHP;
    s32 dmg = 0;
    u16 modifier = UQ_4_12(1.0);

    MulModifier(&modifier, GetTypeModifier(hazardType, type1));
    if (type2 != type1)
        MulModifier(&modifier, GetTypeModifier(hazardType, type2));
    if (type3 != TYPE_MYSTERY && type3 != type1 && type3 != type2)
        MulModifier(&modifier, GetTypeModifier(hazardType, type3));

    switch (modifier)
    {
    case UQ_4_12(0.0):
        dmg = 0;
        break;
    case UQ_4_12(0.25):
        dmg = maxHp / 32;
        if (dmg == 0)
            dmg = 1;
        break;
    case UQ_4_12(0.5):
        dmg = maxHp / 16;
        if (dmg == 0)
            dmg = 1;
        break;
    case UQ_4_12(1.0):
        dmg = maxHp / 8;
        if (dmg == 0)
            dmg = 1;
        break;
    case UQ_4_12(2.0):
        dmg = maxHp / 4;
        if (dmg == 0)
            dmg = 1;
        break;
    case UQ_4_12(4.0):
        dmg = maxHp / 2;
        if (dmg == 0)
            dmg = 1;
        break;
    }

    return dmg;
}

bool32 IsPartnerMonFromSameTrainer(u8 battlerId)
{
    if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT && gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
        return FALSE;
    else if (GetBattlerSide(battlerId) == B_SIDE_PLAYER && gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
        return FALSE;
    else if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
        return FALSE;
    else
        return TRUE;
}

u16 GetMegaEvolutionSpecies(u16 preEvoSpecies, u16 heldItemId)
{
    u32 i;

    for (i = 0; i < EVOS_PER_MON; i++)
    {
        if (gEvolutionTable[preEvoSpecies][i].method == EVO_MEGA_EVOLUTION
         && gEvolutionTable[preEvoSpecies][i].param == heldItemId)
            return gEvolutionTable[preEvoSpecies][i].targetSpecies;
    }
    return SPECIES_NONE;
}


u16 GetPrimalReversionSpecies(u16 preEvoSpecies, u16 heldItemId)
{
    u32 i;

    for (i = 0; i < EVOS_PER_MON; i++)
    {
        if (gEvolutionTable[preEvoSpecies][i].method == EVO_PRIMAL_REVERSION
         && gEvolutionTable[preEvoSpecies][i].param == heldItemId)
            return gEvolutionTable[preEvoSpecies][i].targetSpecies;
    }
    return SPECIES_NONE;
}

u16 GetWishMegaEvolutionSpecies(u16 preEvoSpecies, u16 moveId1, u16 moveId2, u16 moveId3, u16 moveId4)
{
    u32 i, par;

    for (i = 0; i < EVOS_PER_MON; i++)
    {
        if (gEvolutionTable[preEvoSpecies][i].method == EVO_MOVE_MEGA_EVOLUTION)
        {
            par = gEvolutionTable[preEvoSpecies][i].param;
            if (par == moveId1 || par == moveId2 || par == moveId3 || par == moveId4)
                return gEvolutionTable[preEvoSpecies][i].targetSpecies;
        }
    }
    return SPECIES_NONE;
}

bool32 CanMegaEvolve(u8 battlerId)
{
    u32 itemId, holdEffect, species;
    struct Pokemon *mon;
    u8 battlerPosition = GetBattlerPosition(battlerId);
    u8 partnerPosition = GetBattlerPosition(BATTLE_PARTNER(battlerId));
    u8 step = 0;
    struct MegaEvolutionData *mega = &(((struct ChooseMoveStruct*)(&gBattleResources->bufferA[gActiveBattler][4]))->mega);

    // Check if Player has a Mega Ring and the appropriate flag is set
    if ((GetBattlerPosition(battlerId) == B_POSITION_PLAYER_LEFT || (!(gBattleTypeFlags & BATTLE_TYPE_MULTI) && GetBattlerPosition(battlerId) == B_POSITION_PLAYER_RIGHT))
     && (!CheckBagHasItem(ITEM_MEGA_BRACELET, 1) || !FlagGet(FLAG_SYS_RECEIVED_KEYSTONE)))
    {
        if(GetBattlerSide(battlerId) == B_SIDE_PLAYER)
            return FALSE;
    }

    // Gets mon data.
    if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT)
        mon = &gEnemyParty[gBattlerPartyIndexes[battlerId]];
    else
        mon = &gPlayerParty[gBattlerPartyIndexes[battlerId]];

    species = GetMonData(mon, MON_DATA_SPECIES);
    itemId  = GetMonData(mon, MON_DATA_HELD_ITEM);

    // Check if trainer already mega evolved a pokemon.
    if(GetBattlerSide(battlerId) == B_SIDE_PLAYER){
        //There can be a lot of primal mons per battle, it's only checked with the player
        if (mega->alreadyEvolved[battlerPosition] &&
            ItemId_GetHoldEffect(itemId) != HOLD_EFFECT_PRIMAL_ORB) 
            return FALSE;

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
        {
            if (IsPartnerMonFromSameTrainer(battlerId)
                && ItemId_GetHoldEffect(itemId) != HOLD_EFFECT_PRIMAL_ORB //There can be a lot of primal mons per battle
                && (mega->alreadyEvolved[partnerPosition] || (mega->toEvolve & 1 << BATTLE_PARTNER(battlerId))))
                return FALSE;
        }
    }

    // Check if there is an entry in the evolution table for regular Mega Evolution.
    if (GetMegaEvolutionSpecies(species, itemId) != SPECIES_NONE)
    {
        if (B_ENABLE_DEBUG && gBattleStruct->debugHoldEffects[battlerId])
            holdEffect = gBattleStruct->debugHoldEffects[battlerId];
        else if (itemId == ITEM_ENIGMA_BERRY)
            holdEffect = gEnigmaBerries[battlerId].holdEffect;
        else
            holdEffect = ItemId_GetHoldEffect(itemId);

        gBattleStruct->mega.isWishMegaEvo = FALSE;
        gBattleStruct->mega.isPrimalReversion = FALSE;
        return TRUE;
    }

    // Check if there is an entry in the evolution table for regular Primal Reversion.
    if (GetPrimalReversionSpecies(species, itemId) != SPECIES_NONE)
    {
        if (B_ENABLE_DEBUG && gBattleStruct->debugHoldEffects[battlerId])
            holdEffect = gBattleStruct->debugHoldEffects[battlerId];
        else if (itemId == ITEM_ENIGMA_BERRY)
            holdEffect = gEnigmaBerries[battlerId].holdEffect;
        else
            holdEffect = ItemId_GetHoldEffect(itemId);

        gBattleStruct->mega.isWishMegaEvo = FALSE;
        gBattleStruct->mega.isPrimalReversion = TRUE;
        return TRUE;
    }

    // Check if there is an entry in the evolution table for Wish Mega Evolution.
    if (GetWishMegaEvolutionSpecies(species, GetMonData(mon, MON_DATA_MOVE1), GetMonData(mon, MON_DATA_MOVE2), GetMonData(mon, MON_DATA_MOVE3), GetMonData(mon, MON_DATA_MOVE4)))
    {
        gBattleStruct->mega.isWishMegaEvo = TRUE;
        return TRUE;
    }

    // No checks passed, the mon CAN'T mega evolve.
    return FALSE;
}

void UndoMegaEvolution(u32 monId)
{
    u16 species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
    u16 baseSpecies = gBattleStruct->mega.playerBaseSpecies[monId];
    bool8 multibattle = VarGet(VAR_0x8004) == SPECIAL_BATTLE_MULTI;

    if(gSaveBlock2Ptr->permanentMegaMode && species != SPECIES_ZYGARDE_COMPLETE)
        return;

    if(baseSpecies != SPECIES_NONE && species != baseSpecies)
        SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &baseSpecies);

    if (gBattleStruct->mega.evolvedPartyIds[B_SIDE_PLAYER] & 1 << monId)
    {
        gBattleStruct->mega.evolvedPartyIds[B_SIDE_PLAYER] &= ~(1 << monId);
        if(multibattle && gBattleStruct->mega.playerEvolvedSpecies == SPECIES_NONE) //This fixes a problem with multis and mega evolutions
            SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &baseSpecies);
        else
            SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &gBattleStruct->mega.playerEvolvedSpecies);
        
        CalculateMonStats(&gPlayerParty[monId]);
    }
    else if (gBattleStruct->mega.primalRevertedPartyIds[B_SIDE_PLAYER] & 1 << monId)
    {
        gBattleStruct->mega.primalRevertedPartyIds[B_SIDE_PLAYER] &= ~(1 << monId);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &baseSpecies);
        CalculateMonStats(&gPlayerParty[monId]);
    }
    // While not exactly a mega evolution, Zygarde follows the same rules.
    else if (GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, NULL) == SPECIES_ZYGARDE_COMPLETE)
    {
        SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &gBattleStruct->changedSpecies[monId]);
        gBattleStruct->changedSpecies[monId] = 0;
        CalculateMonStats(&gPlayerParty[monId]);
    }
}

void DoBurmyFormChange(u32 monId)
{
    u16 newSpecies, currSpecies;
    struct Pokemon *party = gPlayerParty;

    currSpecies = GetMonData(&party[monId], MON_DATA_SPECIES, NULL);

    if ((GET_BASE_SPECIES_ID(currSpecies) == SPECIES_BURMY) 
        && (gBattleStruct->appearedInBattle & 1 << monId) // Burmy appeared in battle
        && GetMonData(&party[monId], MON_DATA_HP, NULL) != 0) // Burmy isn't fainted
    {
        switch (gBattleTerrain)
        {  
            case BATTLE_TERRAIN_GRASS:
            case BATTLE_TERRAIN_LONG_GRASS:
            case BATTLE_TERRAIN_POND:
            case BATTLE_TERRAIN_MOUNTAIN:
            case BATTLE_TERRAIN_PLAIN:
                newSpecies = SPECIES_BURMY;
                break;
            case BATTLE_TERRAIN_CAVE:
            case BATTLE_TERRAIN_SAND:
                newSpecies = SPECIES_BURMY_SANDY_CLOAK;
                break;
            case BATTLE_TERRAIN_BUILDING:
                newSpecies = SPECIES_BURMY_TRASH_CLOAK;
                break;
            default: // Don't change form if last battle was water-related
                newSpecies = SPECIES_NONE;
                break;
        }

        if (newSpecies != SPECIES_NONE)
        {
            SetMonData(&party[monId], MON_DATA_SPECIES, &newSpecies);
            CalculateMonStats(&party[monId]);
        }
    }
}

void UndoFormChange(u32 monId, u32 side, bool32 isSwitchingOut)
// || gBattleMons[battlerDef].status2 & STATUS2_TRANSFORMED check for transformed mon before reverting
{
    u32 i, currSpecies;
    struct Pokemon *party = (side == B_SIDE_PLAYER) ? gPlayerParty : gEnemyParty;
    static const u16 species[][3] =
    {
        // Changed Form ID                      Default Form ID               Should change on switch
        {SPECIES_MIMIKYU_BUSTED,                SPECIES_MIMIKYU,                FALSE},
        {SPECIES_MIMIKYU_RAYQUAZA_BUSTED,       SPECIES_MIMIKYU_RAYQUAZA,       FALSE},
        {SPECIES_GRENINJA_ASH,                  SPECIES_GRENINJA_BATTLE_BOND,   FALSE},
        {SPECIES_EISCUE_NOICE_FACE,             SPECIES_EISCUE,                 FALSE},
        {SPECIES_PALAFIN_HERO,                  SPECIES_PALAFIN,                FALSE},
        {SPECIES_MINIOR_CORE_RED,               SPECIES_MINIOR,                 TRUE},
        {SPECIES_MINIOR_CORE_BLUE,              SPECIES_MINIOR_METEOR_BLUE,     TRUE},
        {SPECIES_MINIOR_CORE_GREEN,             SPECIES_MINIOR_METEOR_GREEN,    TRUE},
        {SPECIES_MINIOR_CORE_INDIGO,            SPECIES_MINIOR_METEOR_INDIGO,   TRUE},
        {SPECIES_MINIOR_CORE_ORANGE,            SPECIES_MINIOR_METEOR_ORANGE,   TRUE},
        {SPECIES_MINIOR_CORE_VIOLET,            SPECIES_MINIOR_METEOR_VIOLET,   TRUE},
        {SPECIES_MINIOR_CORE_YELLOW,            SPECIES_MINIOR_METEOR_YELLOW,   TRUE},
        {SPECIES_AEGISLASH_BLADE,               SPECIES_AEGISLASH,              TRUE},
        {SPECIES_AEGISLASH_BLADE_REDUX,         SPECIES_AEGISLASH_REDUX,        TRUE},
        {SPECIES_AEGISLASH_BLADE_REDUX_MEGA,    SPECIES_AEGISLASH_REDUX_MEGA,   TRUE},
        {SPECIES_DARMANITAN_ZEN_MODE,           SPECIES_DARMANITAN,             TRUE},
        {SPECIES_WISHIWASHI_SCHOOL,             SPECIES_WISHIWASHI,             TRUE},
        {SPECIES_CRAMORANT_GORGING,             SPECIES_CRAMORANT,              TRUE},
        {SPECIES_CRAMORANT_GULPING,             SPECIES_CRAMORANT,              TRUE},
        {SPECIES_MORPEKO_HANGRY,                SPECIES_MORPEKO,                TRUE},
        {SPECIES_CASTFORM_RAINY,                SPECIES_CASTFORM,               TRUE},
        {SPECIES_CASTFORM_SNOWY,                SPECIES_CASTFORM,               TRUE},
        {SPECIES_CASTFORM_SUNNY,                SPECIES_CASTFORM,               TRUE},
        {SPECIES_CASTFORM_SANDY,                SPECIES_CASTFORM,               TRUE},
        {SPECIES_DARMANITAN_ZEN_MODE_GALARIAN,  SPECIES_DARMANITAN_GALARIAN,    TRUE},
        {SPECIES_LUMBERING_SLOTH_ENGULFED,      SPECIES_LUMBERING_SLOTH,        TRUE},
    };

    currSpecies = GetMonData(&party[monId], MON_DATA_SPECIES, NULL);
    for (i = 0; i < ARRAY_COUNT(species); i++)
    {
        if (currSpecies == species[i][0] && (!isSwitchingOut || species[i][2] == TRUE))
        {
            SetMonData(&party[monId], MON_DATA_SPECIES, &species[i][1]);
            CalculateMonStats(&party[monId]);
            break;
        }
    }
}

bool32 DoBattlersShareType(u32 battler1, u32 battler2)
{
    s32 i;
    u8 types1[3] = {gBattleMons[battler1].type1, gBattleMons[battler1].type2, gBattleMons[battler1].type3};
    u8 types2[3] = {gBattleMons[battler2].type1, gBattleMons[battler2].type2, gBattleMons[battler2].type3};

    if (types1[2] == TYPE_MYSTERY)
        types1[2] = types1[0];
    if (types2[2] == TYPE_MYSTERY)
        types2[2] = types2[0];

    for (i = 0; i < 3; i++)
    {
        if (types1[i] == types2[0] || types1[i] == types2[1] || types1[i] == types2[2])
            return TRUE;
    }

    return FALSE;
}

bool32 CanBattlerGetOrLoseItem(u8 battlerId, u16 itemId)
{
    u16 species = gBattleMons[battlerId].species;
    u16 holdEffect = ItemId_GetHoldEffect(itemId);
    
    // Mail can be stolen now
    if (itemId == ITEM_ENIGMA_BERRY)
        return FALSE;
    else if (holdEffect == HOLD_EFFECT_PRIMAL_ORB)
        return FALSE;
    else if (holdEffect == HOLD_EFFECT_MEGA_STONE)
        return FALSE;
    else if (GET_BASE_SPECIES_ID(species) == SPECIES_GENESECT && holdEffect == HOLD_EFFECT_DRIVE)
        return FALSE;
    else if (GET_BASE_SPECIES_ID(species) == SPECIES_SILVALLY && holdEffect == HOLD_EFFECT_MEMORY)
        return FALSE;
    else if (GET_BASE_SPECIES_ID(species) == SPECIES_ARCEUS && holdEffect == HOLD_EFFECT_PLATE)
        return FALSE;
#ifdef HOLD_EFFECT_Z_CRYSTAL
    else if (holdEffect == HOLD_EFFECT_Z_CRYSTAL)
        return FALSE;
#endif
    else
        return TRUE;
}

u32 GetIllusionMonSpecies(u32 battlerId)
{
    struct Pokemon *illusionMon = GetIllusionMonPtr(battlerId);
    if (illusionMon != NULL)
        return GetMonData(illusionMon, MON_DATA_SPECIES);
    return SPECIES_NONE;
}

struct Pokemon *GetIllusionMonPtr(u32 battlerId)
{
    if (gBattleStruct->illusion[battlerId].broken)
        return NULL;
    if (!gBattleStruct->illusion[battlerId].set)
    {
        if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
            SetIllusionMon(&gPlayerParty[gBattlerPartyIndexes[battlerId]], battlerId);
        else
            SetIllusionMon(&gEnemyParty[gBattlerPartyIndexes[battlerId]], battlerId);
    }
    if (!gBattleStruct->illusion[battlerId].on)
        return NULL;

    return gBattleStruct->illusion[battlerId].mon;
}

void ClearIllusionMon(u32 battlerId)
{
    ZERO(gBattleStruct->illusion[battlerId])
}

bool32 SetIllusionMon(struct Pokemon *mon, u32 battlerId)
{
    struct Pokemon *party, *partnerMon;
    s32 i, id;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u32 personality = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u16 ability = RandomizeAbility(GetMonAbility(mon), species, personality);
    bool8 isEnemyMon = GetBattlerSide(battlerId) == B_SIDE_OPPONENT;

    gBattleStruct->illusion[battlerId].set = 1;
    if (ability != ABILITY_ILLUSION && !MonHasInnate(mon, ABILITY_ILLUSION, isEnemyMon))
        return FALSE;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    if (IsBattlerAlive(BATTLE_PARTNER(battlerId)))
        partnerMon = &party[gBattlerPartyIndexes[BATTLE_PARTNER(battlerId)]];
    else
        partnerMon = mon;

    // Find last alive non-egg pokemon.
    for (i = PARTY_SIZE - 1; i >= 0; i--)
    {
        id = i;
        if (GetMonData(&party[id], MON_DATA_SANITY_HAS_SPECIES)
            && GetMonData(&party[id], MON_DATA_HP)
            && !GetMonData(&party[id], MON_DATA_IS_EGG)
            && &party[id] != mon
            && &party[id] != partnerMon)
        {
            gBattleStruct->illusion[battlerId].on = 1;
            gBattleStruct->illusion[battlerId].broken = 0;
            gBattleStruct->illusion[battlerId].partyId = id;
            gBattleStruct->illusion[battlerId].mon = &party[id];
            return TRUE;
        }
    }

    return FALSE;
}

bool8 ShouldGetStatBadgeBoost(u16 badgeFlag, u8 battlerId)
{
    if (B_BADGE_BOOST != GEN_3)
        return FALSE;
    else if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_FRONTIER))
        return FALSE;
    else if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        return FALSE;
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && gTrainerBattleOpponent_A == TRAINER_SECRET_BASE)
        return FALSE;
    else if (FlagGet(badgeFlag))
        return TRUE;
    else
        return FALSE;
}

u8 GetBattleMoveSplit(u32 moveId)
{
    if (gSwapDamageCategory) // Photon Geyser, Shell Side Arm, Light That Burns the Sky
        return gBattleMoves[moveId].split == SPLIT_PHYSICAL ? SPLIT_SPECIAL : SPLIT_PHYSICAL;
    else if (IS_MOVE_STATUS(moveId) || B_PHYSICAL_SPECIAL_SPLIT >= GEN_4)
        return gBattleMoves[moveId].split;
    else if (gBattleMoves[moveId].type < TYPE_MYSTERY)
        return SPLIT_PHYSICAL;
    else
        return SPLIT_SPECIAL;
}

static bool32 TryRemoveScreens(u8 battler)
{
    bool32 removed = FALSE;
    u8 battlerSide = GetBattlerSide(battler);
    u8 enemySide = GetBattlerSide(BATTLE_OPPOSITE(battler));

    // try to remove from battler's side
    if (gSideStatuses[battlerSide] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_AURORA_VEIL))
    {
        gSideStatuses[battlerSide] &= ~(SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_AURORA_VEIL);
        gSideTimers[battlerSide].reflectTimer = 0;
        gSideTimers[battlerSide].lightscreenTimer = 0;
        gSideTimers[battlerSide].auroraVeilTimer = 0;
        removed = TRUE;
    }

    // try to remove from battler opponent's side
    if (gSideStatuses[enemySide] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_AURORA_VEIL))
    {
        gSideStatuses[enemySide] &= ~(SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_AURORA_VEIL);
        gSideTimers[enemySide].reflectTimer = 0;
        gSideTimers[enemySide].lightscreenTimer = 0;
        gSideTimers[enemySide].auroraVeilTimer = 0;
        removed = TRUE;
    }

    return removed;
}

static bool32 IsUnnerveAbilityOnOpposingSide(u8 battlerId)
{
    if (IsAbilityOnOpposingSide(battlerId, ABILITY_UNNERVE)
      || IsAbilityOnOpposingSide(battlerId, ABILITY_AS_ONE_ICE_RIDER)
      || IsAbilityOnOpposingSide(battlerId, ABILITY_AS_ONE_SHADOW_RIDER)
      || IsAbilityOnOpposingSide(battlerId, ABILITY_CROWNED_KING))
        return TRUE;
    return FALSE;
}

bool32 TestMoveFlags(u16 move, u32 flag)
{
    if (gBattleMoves[move].flags & flag)
        return TRUE;
    return FALSE;
}

struct Pokemon *GetBattlerPartyData(u8 battlerId)
{
    struct Pokemon *mon;
    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
        mon = &gPlayerParty[gBattlerPartyIndexes[battlerId]];
    else
        mon = &gEnemyParty[gBattlerPartyIndexes[battlerId]];
    return mon;
}

//Make sure the input bank is any bank on the specific mon's side
bool32 CanFling(u8 battlerId)
{
    u16 item = gBattleMons[battlerId].item;

    if (item == ITEM_NONE
      || IsItemNegated(battlerId)
      || !CanBattlerGetOrLoseItem(battlerId, item))
        return FALSE;

    return TRUE;
}

// ability checks
bool32 IsRolePlayBannedAbilityAtk(u16 ability)
{
    u32 i;
    if (IsPersistentOrUnsuppressableAbility(ability)) return TRUE;
    return FALSE;
}

bool32 IsRolePlayBannedAbility(u16 ability)
{
    u32 i;
    if (!ability) return TRUE;
    if (IsPersistentOrUnsuppressableAbility(ability)) return TRUE;
    for (i = 0; i < ARRAY_COUNT(sRolePlayBannedAbilities); i++)
    {
        if (ability == sRolePlayBannedAbilities[i])
            return TRUE;
    }
    return FALSE;
}

bool32 IsWorrySeedBannedAbility(u16 ability)
{
    u32 i;
    if (IsPersistentOrUnsuppressableAbility(ability)) return TRUE;
    return FALSE;
}

bool32 IsGastroAcidBannedAbility(u16 ability)
{
    u32 i;
    if (IsPersistentOrUnsuppressableAbility(ability)) return TRUE;
    return FALSE;
}

bool32 IsEntrainmentBannedAbilityAttacker(u16 ability)
{
    u32 i;
    if (IsPersistentOrUnsuppressableAbility(ability)) return TRUE;
    for (i = 0; i < ARRAY_COUNT(sSkillSwapBannedAbilities); i++)
    {
        if (ability == sSkillSwapBannedAbilities[i])
            return TRUE;
    }
    return FALSE;
}

bool32 IsEntrainmentTargetOrSimpleBeamBannedAbility(u16 ability)
{
    u32 i;
    if (IsPersistentOrUnsuppressableAbility(ability)) return TRUE;
    for (i = 0; i < ARRAY_COUNT(sEntrainmentTargetSimpleBeamBannedAbilities); i++)
    {
        if (ability == sEntrainmentTargetSimpleBeamBannedAbilities[i])
            return TRUE;
    }
    return FALSE;
}

// Sort an array of battlers by speed
// Useful for effects like pickpocket, eject button, red card, dancer
void SortBattlersBySpeed(u8 *battlers, bool8 slowToFast)
{
    int i, j, currSpeed, currBattler;
    u16 speeds[4] = {0};
    
    for (i = 0; i < gBattlersCount; i++)
        speeds[i] = GetBattlerTotalSpeedStat(battlers[i], TOTAL_SPEED_FULL);

    for (i = 1; i < gBattlersCount; i++)
    {
        currBattler = battlers[i];
        currSpeed = speeds[i];
        j = i - 1;

        if (slowToFast)
        {
            while (j >= 0 && speeds[j] > currSpeed)
            {
                battlers[j + 1] = battlers[j];
                speeds[j + 1] = speeds[j];
                j = j - 1;
            }
        }
        else
        {
            while (j >= 0 && speeds[j] < currSpeed)
            {
                battlers[j + 1] = battlers[j];
                speeds[j + 1] = speeds[j];
                j = j - 1;
            }
        }

        battlers[j + 1] = currBattler;
        speeds[j + 1] = currSpeed;
    }
}

void TryRestoreStolenItems(void)
{
    u32 i;
    u16 stolenItem = ITEM_NONE;
    
    if (B_RESTORE_ALL_ITEMS)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, NULL) != gBattleStruct->itemStolen[i].originalItem)
            {
                stolenItem = gBattleStruct->itemStolen[i].originalItem;
                SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &stolenItem);
            }
        }
    }
    else
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (gBattleStruct->itemStolen[i].stolen)
            {
                stolenItem = gBattleStruct->itemStolen[i].originalItem;
                if (stolenItem != ITEM_NONE && ItemId_GetPocket(stolenItem) != POCKET_BERRIES)
                    SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &stolenItem);  // Restore stolen non-berry items
            }
        }
    }
}

bool32 CanStealItem(u8 battlerStealing, u8 battlerItem, u16 item)
{
    u8 stealerSide = GetBattlerSide(battlerStealing);

    if (!item) return FALSE;
    if (gBattleMons[battlerStealing].item) return FALSE;
    
    if (!CanBattlerGetOrLoseItem(battlerItem, item)      // Battler with item cannot have it stolen
      || !CanBattlerGetOrLoseItem(battlerStealing, item)) // Stealer cannot take the item
        return FALSE;
    
    return TRUE;
}

void TrySaveExchangedItem(u8 battlerId, u16 stolenItem)
{
    // Because BtlController_EmitSetMonData does SetMonData, we need to save the stolen item only if it matches the battler's original
    // So, if the player steals an item during battle and has it stolen from it, it will not end the battle with it (naturally)
    #if B_TRAINERS_KNOCK_OFF_ITEMS == TRUE
    // If regular trainer battle and mon's original item matches what is being stolen, save it to be restored at end of battle
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER
      && !(gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
      && GetBattlerSide(battlerId) == B_SIDE_PLAYER
      && stolenItem == gBattleStruct->itemStolen[gBattlerPartyIndexes[battlerId]].originalItem)
        gBattleStruct->itemStolen[gBattlerPartyIndexes[battlerId]].stolen = TRUE;
    #endif
}

bool32 IsBattlerAffectedByHazards(u8 battlerId, bool32 stealthRock)
{
    bool32 ret = TRUE;
    u32 holdEffect = GetBattlerHoldEffect(gActiveBattler, TRUE);
    if (holdEffect == HOLD_EFFECT_HEAVY_DUTY_BOOTS)
    {
        ret = FALSE;
        RecordItemEffectBattle(battlerId, holdEffect);
    }
	else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_SHIELD_DUST))
    {
        ret = FALSE;
    }
	else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_MOUNTAINEER) && stealthRock)
    {
        ret = FALSE;
    }
    return ret;
}

bool32 TestSheerForceFlag(u8 battler, u16 move)
{
    if ((GetBattlerAbility(battler) == ABILITY_SHEER_FORCE || BattlerHasInnate(battler, ABILITY_SHEER_FORCE)) && 
         gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST)
        return TRUE;
    else
        return FALSE;
}

int StatLowerableOrMirrorArmor(int battler, int stat)
{
    if (CompareStat(battler, stat, MIN_STAT_STAGE, CMP_GREATER_THAN)) return TRUE;
    
    return BATTLER_HAS_ABILITY(battler, ABILITY_MIRROR_ARMOR);
}

// This function is the body of "jumpifstat", but can be used dynamically in a function
bool32 CompareStat(u8 battlerId, u8 statId, u8 cmpTo, u8 cmpKind)
{
    bool8 ret = FALSE;
    u8 statValue = gBattleMons[battlerId].statStages[statId];
    
    // Because this command is used as a way of checking if a stat can be lowered/raised,
    // we need to do some modification at run-time.
    if (GetBattlerAbility(battlerId) == ABILITY_CONTRARY || BattlerHasInnate(battlerId, ABILITY_CONTRARY))
    {
        if (cmpKind == CMP_GREATER_THAN)
            cmpKind = CMP_LESS_THAN;
        else if (cmpKind == CMP_LESS_THAN)
            cmpKind = CMP_GREATER_THAN;

        if (cmpTo == MIN_STAT_STAGE)
            cmpTo = MAX_STAT_STAGE;
        else if (cmpTo == MAX_STAT_STAGE)
            cmpTo = MIN_STAT_STAGE;
    }

    switch (cmpKind)
    {
    case CMP_EQUAL:
        if (statValue == cmpTo)
            ret = TRUE;
        break;
    case CMP_NOT_EQUAL:
        if (statValue != cmpTo)
            ret = TRUE;
        break;
    case CMP_GREATER_THAN:
        if (statValue > cmpTo)
            ret = TRUE;
        break;
    case CMP_LESS_THAN:
        if (statValue < cmpTo)
            ret = TRUE;
        break;
    case CMP_COMMON_BITS:
        if (statValue & cmpTo)
            ret = TRUE;
        break;
    case CMP_NO_COMMON_BITS:
        if (!(statValue & cmpTo))
            ret = TRUE;
        break;
    }
    
    return ret;
}

void BufferStatChange(u8 battlerId, u8 statId, u8 stringId)
{
    bool8 hasContrary = FALSE;
    
    if(GetBattlerAbility(battlerId) == ABILITY_CONTRARY || BattlerHasInnate(battlerId, ABILITY_CONTRARY))
        hasContrary = TRUE;

    PREPARE_STAT_BUFFER(gBattleTextBuff1, statId);
    if (stringId == STRINGID_STATFELL)
    {
        if (hasContrary)
            PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_STATROSE)
        else
            PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_STATFELL)
    }
    else if (stringId == STRINGID_STATROSE)
    {
        if (hasContrary)
            PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_STATFELL)
        else
            PREPARE_STRING_BUFFER(gBattleTextBuff2, STRINGID_STATROSE)
    }
    else
    {
        PREPARE_STRING_BUFFER(gBattleTextBuff2, stringId)
    }
}

bool32 TryRoomService(u8 battlerId)
{
    if (IsTrickRoomActive() && CompareStat(battlerId, STAT_SPEED, MIN_STAT_STAGE, CMP_GREATER_THAN))
    {
        BufferStatChange(battlerId, STAT_SPEED, STRINGID_STATFELL);
        gEffectBattler = gBattleScripting.battler = battlerId;
        SET_STATCHANGER(STAT_SPEED, 1, TRUE);
        gBattleScripting.animArg1 = 0xE + STAT_SPEED;
        gBattleScripting.animArg2 = 0;
        gLastUsedItem = gBattleMons[battlerId].item;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Move Checks
bool8 IsTwoStrikesMove(u16 move)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sTwoStrikeMoves); i++)
    {
        if (move == sTwoStrikeMoves[i])
            return TRUE;
    }
    return FALSE;
}

bool32 BlocksPrankster(u16 move, u8 battlerPrankster, u8 battlerDef, bool32 checkTarget)
{
    #if B_PRANKSTER_DARK_TYPES >= GEN_7
    if (!gTurnStructs[battlerPrankster].pranksterElevated)
        return FALSE;
    if (GetBattlerSide(battlerPrankster) == GetBattlerSide(battlerDef))
        return FALSE;
    if (checkTarget && (gBattleMoves[move].target & (MOVE_TARGET_OPPONENTS_FIELD | MOVE_TARGET_DEPENDS)))
        return FALSE;
    if (!IS_BATTLER_OF_TYPE(battlerDef, TYPE_DARK))
        return FALSE;
    if (gStatuses3[battlerDef] & STATUS3_SEMI_INVULNERABLE)
        return FALSE;
    
    return TRUE;
    #endif
    return FALSE;
}

u16 GetUsedHeldItem(u8 battler)
{
    return gBattleStruct->usedHeldItems[gBattlerPartyIndexes[battler]][GetBattlerSide(battler)];
}

bool32 IsBattlerWeatherAffected(u8 battlerId, u32 weatherFlags)
{
    if (gBattleWeather & weatherFlags && WEATHER_HAS_EFFECT)
    {
        // given weather is active -> check if its sun, rain against utility umbrella ( since only 1 weather can be active at once)
        if (gBattleWeather & (WEATHER_SUN_ANY | WEATHER_RAIN_ANY) && GetBattlerHoldEffect(battlerId, TRUE) == HOLD_EFFECT_UTILITY_UMBRELLA)
            return FALSE; // utility umbrella blocks sun, rain effects
    
        return TRUE;
    }
    return FALSE;
}

bool32 DoesBattlerIgnoreAbilityorInnateChecks(u8 battler)
{
    u16 ability = gBattleMons[battler].ability;

    if( ability == ABILITY_MOLD_BREAKER                 ||
        ability == ABILITY_BLIND_RAGE                   ||
        ability == ABILITY_TERAVOLT                     ||
        ability == ABILITY_TURBOBLAZE                   ||
        BattlerHasInnate(battler, ABILITY_MOLD_BREAKER) || 
        BattlerHasInnate(battler, ABILITY_BLIND_RAGE)   || 
        BattlerHasInnate(battler, ABILITY_TERAVOLT)     || 
        BattlerHasInnate(battler, ABILITY_TURBOBLAZE)   ||
        (getMonotypeChampType() == TYPE_STEEL && GetBattlerSide(battler) != B_SIDE_PLAYER))
        return TRUE;

    return FALSE;
}

static bool8 DoesMoveBoostStats(u16 move){
    switch(gBattleMoves[move].effect){
        //Multiple Stats Up
        case EFFECT_CALM_MIND:
        //Attack
        case EFFECT_ATTACK_UP:
        case EFFECT_ATTACK_UP_2:
        case EFFECT_HOWL:
        //Defense
        case EFFECT_DEFENSE_UP:
        case EFFECT_DEFENSE_UP_2: 
        //Special Attack
        case EFFECT_SPECIAL_ATTACK_UP:
        case EFFECT_SPECIAL_ATTACK_UP_2:
        case EFFECT_SPECIAL_ATTACK_UP_3:
		//Special Defense
		case EFFECT_SPECIAL_DEFENSE_UP_2:
		//Speed
		case EFFECT_SPEED_UP_HIT:
		case EFFECT_SPEED_UP_2:
	    //Accuracy
		//case EFFECT_ACCURACY_UP:
            return TRUE;
        break;
        case EFFECT_ATTACK_UP_HIT:
		case EFFECT_SPECIAL_DEFENSE_UP:
        case EFFECT_SP_ATTACK_UP_HIT:
        case EFFECT_DEFENSE_UP_HIT:
        case EFFECT_ALL_STATS_UP_HIT:
            if(gBattleMoves[move].secondaryEffectChance == 100 || (gBattleScripting.moveEffect & MOVE_EFFECT_CERTAIN) || FlagGet(FLAG_LAST_MOVE_SECONDARY_EFFECT_ACTIVATED))
                return TRUE;
        break;
    }
    return FALSE;
}

bool8 HasAnyLoweredStat(u8 battler){
    u8 i;
    for(i = STAT_ATK; i < NUM_BATTLE_STATS; i++){
        if(CompareStat(battler, i, DEFAULT_STAT_STAGE, CMP_LESS_THAN))
            return TRUE;
    }
    return FALSE;
}

s32 GetCurrentTerrain(void)
{
    if(!TERRAIN_HAS_EFFECT)
        return 0;
    else if(gFieldStatuses & STATUS_FIELD_MISTY_TERRAIN)
        return STATUS_FIELD_MISTY_TERRAIN;
    else if(gFieldStatuses & STATUS_FIELD_PSYCHIC_TERRAIN)
        return STATUS_FIELD_PSYCHIC_TERRAIN;
    else if(gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
        return STATUS_FIELD_ELECTRIC_TERRAIN;
    else if(gFieldStatuses & STATUS_FIELD_GRASSY_TERRAIN)
        return STATUS_FIELD_GRASSY_TERRAIN;
    else
        return 0;
}

u8 BattlerHasInnateOrAbility(u8 battler, u16 ability){
    if(BattlerHasInnate(battler, ability))
        return BATTLER_INNATE;
    else if(GetBattlerAbility(battler) == ability)
        return BATTLER_ABILITY;
    else
        return BATTLER_NONE;
}

bool8 IsTrickRoomActive(void){
    if(IsAbilityOnField(ABILITY_CLUELESS))
        return FALSE;
    else if((gFieldStatuses & STATUS_FIELD_TRICK_ROOM) || (getMonotypeChampType() == TYPE_NORMAL && gBattleResults.battleTurnCounter % 2 == 1))
        return TRUE;
    else
        return FALSE;
}

bool8 IsInverseRoomActive(void){
    if(IsAbilityOnField(ABILITY_CLUELESS))
        return FALSE;
    else if(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)
        return TRUE;
    else
        return FALSE;
}

bool8 IsGravityActive(void){
    if(IsAbilityOnField(ABILITY_CLUELESS))
        return FALSE;
    else if(gFieldStatuses & STATUS_FIELD_GRAVITY)
        return TRUE;
    else
        return FALSE;
}

bool8 isMagicRoomActive(void){
    if(IsAbilityOnField(ABILITY_CLUELESS))
        return FALSE;
    else if(gFieldStatuses & STATUS_FIELD_MAGIC_ROOM)
        return TRUE;
    else
        return FALSE;
}

bool8 isWonderRoomActive(void){
    if(IsAbilityOnField(ABILITY_CLUELESS))
        return FALSE;
    else if((gFieldStatuses & STATUS_FIELD_WONDER_ROOM) || (getMonotypeChampType() == TYPE_NORMAL && gBattleResults.battleTurnCounter % 2 == 0))
        return TRUE;
    else
        return FALSE;
}

bool8 CanUseExtraMove(u8 sBattlerAttacker, u8 sBattlerTarget){
    if(sBattlerAttacker != sBattlerTarget
        && IsBattlerAlive(sBattlerAttacker)
        && IsBattlerAlive(sBattlerTarget)
        && !gRoundStructs[sBattlerAttacker].attackCancelled)
        return TRUE;
    else
        return FALSE;
}

u8 GetHighestStatId(u8 battlerId, u8 includeStatStages)
{
    u8 i;
    u8 highestId = STAT_ATK;
    u32 highestStat = 0;

    for (i = STAT_ATK; i < NUM_STATS; i++)
    {
        u16 statVal = *(&gBattleMons[battlerId].attack + (i - 1));
        if (includeStatStages)
        {
            u8 statStage = gBattleMons[battlerId].statStages[i];
            if (gBattleMons[battlerId].status1 & STATUS1_BLEED || IsBloodStainAffected(battlerId)) statStage = min(statStage, DEFAULT_STAT_STAGE);

            statVal = statVal * gStatStageRatios[statStage][0] / gStatStageRatios[statStage][1];
        }
        if (statVal > highestStat)
        {
            highestStat = statVal;
            highestId = i;
        }
    }
    return highestId;
}

u8 GetHighestAttackingStatId(u8 battlerId, u8 includeStatStages)
{
    u8 i;
    u8 highestId = STAT_ATK;
    u32 highestStat = 0;

    for (i = STAT_ATK; i <= STAT_SPATK; i += STAT_SPATK - STAT_ATK)
    {
        u16 statVal = *(&gBattleMons[battlerId].attack + (i - 1));
        if (includeStatStages)
        {
            u8 statStage = gBattleMons[battlerId].statStages[i];
            if (gBattleMons[battlerId].status1 & STATUS1_BLEED || IsBloodStainAffected(battlerId)) statStage = min(statStage, DEFAULT_STAT_STAGE);

            statVal = statVal * gStatStageRatios[statStage][0] / gStatStageRatios[statStage][1];
        }
        if (statVal > highestStat)
        {
            highestStat = statVal;
            highestId = i;
        }
    }
    return highestId;
}

u8 GetHighestDefendingStatId(u8 battlerId, u8 includeStatStages)
{
    u8 i;
    u8 highestId = STAT_DEF;
    u32 highestStat = 0;

    for (i = STAT_DEF; i > STAT_SPDEF; i += STAT_SPDEF - STAT_DEF)
    {
        u16 statVal = *(&gBattleMons[battlerId].attack + (i - 1));
        if (includeStatStages)
        {
            u8 statStage = gBattleMons[battlerId].statStages[i];
            if (gBattleMons[battlerId].status1 & STATUS1_BLEED || IsBloodStainAffected(battlerId)) statStage = min(statStage, DEFAULT_STAT_STAGE);

            statVal = statVal * gStatStageRatios[statStage][0] / gStatStageRatios[statStage][1];
        }
        if (statVal > highestStat)
        {
            highestStat = statVal;
            highestId = i;
        }
    }
    return highestId;
}

u8 TranslateStatId(u8 statId, u8 battlerId)
{
    if ((statId & STAT_HIGHEST_MASK) == STAT_HIGHEST_ATTACKING) return GetHighestAttackingStatId(battlerId, statId & STAT_USE_STAT_BOOSTS_IN_CALC);
    if ((statId & STAT_HIGHEST_MASK) == STAT_HIGHEST_DEFENDING) return GetHighestDefendingStatId(battlerId, statId & STAT_USE_STAT_BOOSTS_IN_CALC);
    if ((statId & STAT_HIGHEST_MASK) == STAT_HIGHEST_TOTAL) return GetHighestStatId(battlerId, statId & STAT_USE_STAT_BOOSTS_IN_CALC);
    return statId;
}

bool32 IsAlly(u32 battlerAtk, u32 battlerDef)
{
    return (GetBattlerSide(battlerAtk) == GetBattlerSide(battlerDef));
}

u16 GetInnateInSlot(int level, u16 species, u8 position, u32 personality, u8 isPlayer)
{
    if (isPlayer)
    {
        switch (position)
        {
            case 0:
                if (level < INNATE_1_LEVEL && gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_ELITE) return ABILITY_NONE;
                break;
            case 1:
                if (level < INNATE_2_LEVEL && gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_ELITE) return ABILITY_NONE;
                break;
            case 2:
                if (level < INNATE_3_LEVEL && gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_ELITE) return ABILITY_NONE;
                break;
        }
    }
    return isPlayer ? RandomizeInnate(gBaseStats[species].innates[position], species, personality) : gBaseStats[species].innates[position];
}

void UpdateAbilityStateIndicesForNewSpecies(u8 battler, u16 newSpecies)
{
    u32 personality = gBattleMons[battler].personality;
    bool8 isPlayer = GetBattlerSide(battler) == B_SIDE_PLAYER;
    int level = gBattleMons[battler].level;
    u16 newAbilities[] = { 
        gBaseStats[newSpecies].abilities[gBattleMons[battler].abilityNum],
        GetInnateInSlot(level, newSpecies, 0, personality, isPlayer),
        GetInnateInSlot(level, newSpecies, 1, personality, isPlayer),
        GetInnateInSlot(level, newSpecies, 2, personality, isPlayer),
        };
    UpdateAbilityStateIndices(battler, newAbilities);
}

void UpdateAbilityStateIndicesForNewAbility(u8 battler, u16 newAbility)
{
    u16 species = gBattleMons[battler].species;
    u32 personality = gBattleMons[battler].personality;
    bool8 isPlayer = GetBattlerSide(battler) == B_SIDE_PLAYER;
    int level = gBattleMons[battler].level;
    u16 newAbilities[] = { 
        newAbility,
        GetInnateInSlot(level, species, 0, personality, isPlayer),
        GetInnateInSlot(level, species, 1, personality, isPlayer),
        GetInnateInSlot(level, species, 2, personality, isPlayer),
        };
    UpdateAbilityStateIndices(battler, newAbilities);
}

void UpdateAbilityStateIndices(u8 battler, u16 newAbilities[])
{
    u8 i, j;
    u8 switchInAbilityDone[NUM_INNATE_PER_SPECIES + 1] = {0};
    u8 turnAbilityTriggers[NUM_INNATE_PER_SPECIES + 1] = {0};
    u32 abilityState[NUM_INNATE_PER_SPECIES + 1] = {0};
    u16 species = gBattleMons[battler].species;
    u32 personality = gBattleMons[battler].personality;
    bool8 isPlayer = GetBattlerSide(battler) == B_SIDE_PLAYER;
    int level = gBattleMons[battler].level;
    u16 oldAbilities[] = { 
        gBattleMons[battler].ability,
        GetInnateInSlot(level, species, 0, personality, isPlayer),
        GetInnateInSlot(level, species, 1, personality, isPlayer),
        GetInnateInSlot(level, species, 2, personality, isPlayer),
        };

    for (i = 0; i < NUM_INNATE_PER_SPECIES + 1; i++)
    {
        for (j = 0; j < NUM_INNATE_PER_SPECIES + 1; j++)
        {
            if (newAbilities[i] == oldAbilities[j]) break;
        }
        if (j >= NUM_INNATE_PER_SPECIES + 1) continue;
        switchInAbilityDone[i] = gVolatileStructs[battler].switchInAbilityDone[j];
        turnAbilityTriggers[i] = gTurnStructs[battler].turnAbilityTriggers[j];
        abilityState[i] = gVolatileStructs[battler].abilityState[j];
    }

    ARRAY_COPY(gVolatileStructs[battler].switchInAbilityDone, switchInAbilityDone)
    ARRAY_COPY(gTurnStructs[battler].turnAbilityTriggers, turnAbilityTriggers);
    ARRAY_COPY(gVolatileStructs[battler].abilityState, abilityState);
}

u16 IsSoundproof(u8 battler)
{
    if (BATTLER_HAS_ABILITY(battler, ABILITY_SOUNDPROOF)) return ABILITY_SOUNDPROOF;
    if (BATTLER_HAS_ABILITY(battler, ABILITY_PARROTING)) return ABILITY_PARROTING;
    if (IsAbilityOnSide(battler, ABILITY_NOISE_CANCEL)) return ABILITY_NOISE_CANCEL;
    return 0;
}

u8 GetTurnBattler()
{
    if (gProcessingExtraAttacks)
        return gQueuedExtraAttackData[0].attacker;
    else
        return gBattlerByTurnOrder[gCurrentTurnActionNumber];
}

bool32 IsHealingMoveEffect(u16 effect)
{
    switch (effect)
    {
    case EFFECT_ABSORB:
    case EFFECT_MORNING_SUN:
    case EFFECT_MOONLIGHT:
    case EFFECT_RESTORE_HP:
    case EFFECT_REST:
    case EFFECT_ROOST:
    case EFFECT_WISH:
    case EFFECT_HEALING_WISH:
    case EFFECT_REVIVAL_BLESSING:
    case EFFECT_SOFTBOILED:
    case EFFECT_SYNTHESIS:
    case EFFECT_SHORE_UP:
    case EFFECT_JUNGLE_HEALING:
    case EFFECT_HEAL_PULSE:
    case EFFECT_MATCHA_GOTCHA:
    case EFFECT_STRENGTH_SAP:
    case EFFECT_DRAIN_BRAIN:
        return TRUE;
    default:
        return FALSE;
    }
}

bool8 IsBattlerCursed(u8 battler)
{
    if (getMonotypeChampType() == TYPE_GHOST && GetBattlerSide(battler) == B_SIDE_PLAYER)
        return TRUE;
    else
        return FALSE;
}

void MakePlayerTeamAsleep(void){
    u8 i;
    u32 status = STATUS1_SLEEP_TURN(3);

    for(i = 0; i < PARTY_SIZE; i++){
        if(GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
            SetMonData(&gPlayerParty[i], MON_DATA_STATUS, &status);
    }
}

int IsMagicGuardProtected(int battler)
{
    int ability = GetBattlerAbility(battler);

    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_MAGIC_GUARD, ability)) return TRUE;
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_IMPENETRABLE, ability)) return TRUE;
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_APPLE_ENLIGHTENMENT, ability)) return TRUE;

    return FALSE;
}

int TestAbsorbingAbilities(int battler, int battlerAtk, int move, int moveType, int *statId, u16 *ability)
{
    
    // Aerodynamics
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_AERODYNAMICS)){
        if (moveType == TYPE_FLYING){
            *statId = STAT_SPEED;
            *ability = ABILITY_AERODYNAMICS;
            return 2;
        }
    }
    
    // Volt Absorb
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_VOLT_ABSORB)){
        if (moveType == TYPE_ELECTRIC){
            *ability = ABILITY_VOLT_ABSORB;
            return 1;
        }
    }

    // Earth Eater
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_EARTH_EATER)){
        if (moveType == TYPE_GROUND){
            *ability = ABILITY_EARTH_EATER;
            return 1;
        }
    }
    
    // Water Absorb
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_WATER_ABSORB)){
        if (moveType == TYPE_WATER){
            *ability = ABILITY_WATER_ABSORB;
            return 1;
        }
    }
    
    // Water Absorb
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_OLD_MARINER)){
        if (moveType == TYPE_WATER){
            *ability = ABILITY_OLD_MARINER;
            return 1;
        }
    }
    
    // Dry Skin
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_DRY_SKIN)){
        if (moveType == TYPE_WATER){
            *ability = ABILITY_DRY_SKIN;
            return 1;
        }
    }
    
    // Poison Absorb
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_POISON_ABSORB)){
        if (moveType == TYPE_POISON){
            *ability = ABILITY_POISON_ABSORB;
            return 1;
        }
    }
    
    // Lighting Rod
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_LIGHTNING_ROD)){
        if (moveType == TYPE_ELECTRIC){
            *ability = ABILITY_LIGHTNING_ROD;
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return 2;
        }
    }

    // Storm Drain
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_STORM_DRAIN)){
        if (moveType == TYPE_WATER){
            *ability = ABILITY_STORM_DRAIN;
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return 2;
        }
    }
    
    // Flash Fire
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_FLASH_FIRE)) {
        if (moveType == TYPE_FIRE)
        {
            *ability = ABILITY_FLASH_FIRE;
            return 3;
        }
    }

    // Elemental Vortex (will also compress this into flash fire later)
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_ELEMENTAL_VORTEX)) {
        if (moveType == TYPE_FIRE)
        {
            *ability = ABILITY_ELEMENTAL_VORTEX;
            return 3;
        }

        if (moveType == TYPE_WATER){
            *ability = ABILITY_ELEMENTAL_VORTEX;
            return 1;
        }
    }

    // Sap Sipper
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_SAP_SIPPER)){
        if (moveType == TYPE_GRASS){
            *ability = ABILITY_SAP_SIPPER;
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return 2;
        }
    }

    // Ice Dew
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_ICE_DEW)){
        if (moveType == TYPE_ICE){
            *ability = ABILITY_ICE_DEW;
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return 2;
        }
    }
    
    //Motor Drive
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_MOTOR_DRIVE)){
        if (moveType == TYPE_ELECTRIC){
            *ability = ABILITY_MOTOR_DRIVE;
            *statId = STAT_SPEED;
            return 2;
        }
    }

    // Wind Rider
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_WIND_RIDER)){
        if (gBattleMoves[move].airBased){
            *ability = ABILITY_WIND_RIDER;
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return 2;
        }
    }
    
    // Justified
    if (BattlerHasAbility(battler, battlerAtk, ABILITY_JUSTIFIED)){
        if (moveType == TYPE_DARK){
            *ability = ABILITY_JUSTIFIED;
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return 2;
        }
    }

    return 0;
}

int TestImmunityAbilities(int battler, int attacker, int move, int moveType, const u8 ** immunityScript, u8* overrideBattler, u16* abilityPopup)
{
    int ability;
    int actualAttacker = gBattlerAttacker;
    int triggeringBattler;
    int effect = 0;
    gBattlerAttacker = attacker;

    if ((ability = IsSoundproof(battler))
        && !(gBattleMoves[move].target & MOVE_TARGET_USER)
        && gBattleMoves[move].flags & FLAG_SOUND)
    {
        if (ability == ABILITY_NOISE_CANCEL && !BATTLER_HAS_ABILITY(battler, ABILITY_NOISE_CANCEL)) {
            *overrideBattler = BATTLE_PARTNER(battler) + 1;
        }

        *abilityPopup = ability;
        *immunityScript = BattleScript_SoundproofProtected;
        effect = 2;
    }
    else if (BATTLER_HAS_ABILITY(battler, ABILITY_WEATHER_CONTROL)
        && !(GetBattlerBattleMoveTargetFlags(move, battler) & MOVE_TARGET_USER)
        && gBattleMoves[move].flags & FLAG_WEATHER_BASED)
    {
        *abilityPopup = ABILITY_WEATHER_CONTROL;
        *immunityScript = BattleScript_SoundproofProtected;
        effect = 2;
    }
    else if (BATTLER_HAS_ABILITY(battler, ABILITY_DELTA_STREAM)
        && !(gBattleMoves[move].target & MOVE_TARGET_USER)
        && gBattleMoves[move].flags & FLAG_WEATHER_BASED)
    {
        *abilityPopup = ABILITY_DELTA_STREAM;
        *immunityScript = BattleScript_SoundproofProtected;
        effect = 2;
    }
    else if (BATTLER_HAS_ABILITY(battler, ABILITY_BULLETPROOF)
        && gBattleMoves[move].flags & FLAG_BALLISTIC)
    {
        *abilityPopup = ABILITY_BULLETPROOF;
        *immunityScript = BattleScript_SoundproofProtected;
        effect = 2;
    }
    else if ((triggeringBattler = IsAbilityOnField(ABILITY_RADIANCE)) && moveType == TYPE_DARK)
    {
        triggeringBattler--;
        *abilityPopup = ABILITY_RADIANCE;
        if (triggeringBattler != battler) *overrideBattler = triggeringBattler;
        *immunityScript = BattleScript_RadianceProtected;
        effect = 1;
    }
    //Queenly Majesty
    else if(!gProcessingExtraAttacks
        && (triggeringBattler = IsAbilityOnSide(battler, ABILITY_QUEENLY_MAJESTY))
        && GetMovePriority(gBattlerAttacker, move, battler) > 0
        && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(battler)
        && !(move == MOVE_SCRATCH && BattlerHasInnateOrAbility(gBattlerAttacker, ABILITY_CHEAP_TACTICS))
        )
    {
        triggeringBattler--;
        *abilityPopup = ABILITY_QUEENLY_MAJESTY;
        if (triggeringBattler != battler) *overrideBattler = triggeringBattler;
        *immunityScript = BattleScript_DazzlingProtected;
        effect = 1;
    }
    //Dazzling
    else if(!gProcessingExtraAttacks
        && (triggeringBattler = IsAbilityOnSide(battler, ABILITY_DAZZLING))
        && GetMovePriority(gBattlerAttacker, move, battler) > 0
        && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(battler))
    {
        triggeringBattler--;
        *abilityPopup = ABILITY_DAZZLING;
        if (triggeringBattler != battler) *overrideBattler = triggeringBattler;
        *immunityScript = BattleScript_DazzlingProtected;
        effect = 2;
    }
    //Unicorn
    else if(!gProcessingExtraAttacks
        && (triggeringBattler = IsAbilityOnSide(battler, ABILITY_UNICORN))
        && GetMovePriority(gBattlerAttacker, move, battler) > 0
        && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(battler))
    {
        triggeringBattler--;
        *abilityPopup = ABILITY_UNICORN;
        if (triggeringBattler != battler) *overrideBattler = triggeringBattler;
        *immunityScript = BattleScript_DazzlingProtected;
        effect = 2;
    }
    //Armor Tail
    else if(!gProcessingExtraAttacks
        && (triggeringBattler = IsAbilityOnSide(battler, ABILITY_ARMOR_TAIL))
        && GetMovePriority(gBattlerAttacker, move, battler) > 0
        && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(battler))
    {
        triggeringBattler--;
        *abilityPopup = ABILITY_ARMOR_TAIL;
        if (triggeringBattler != battler) *overrideBattler = triggeringBattler;
        *immunityScript = BattleScript_DazzlingProtected;
        effect = 1;
    }
    //Sand Guard
    else if(!gProcessingExtraAttacks
        && BATTLER_HAS_ABILITY(battler, ABILITY_SAND_GUARD)
        && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)
        && GetMovePriority(gBattlerAttacker, move, battler) > 0
        && GetBattlerSide(gBattlerAttacker) != GetBattlerSide(battler))
    {
        *abilityPopup = ABILITY_SAND_GUARD;
        *immunityScript = BattleScript_DazzlingProtected;
        effect = 2;
    }
    else if (!gProcessingExtraAttacks
        && BlocksPrankster(move, gBattlerAttacker, gBattlerTarget, TRUE)
        && !(IS_MOVE_STATUS(move) && BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_MAGIC_BOUNCE)))
    {
        *immunityScript = BattleScript_DarkTypePreventsPrankster;
        effect = 2;
    }
    else if (BATTLER_HAS_ABILITY(battler, ABILITY_GOOD_AS_GOLD)
        && battler != gBattlerAttacker
        && IS_MOVE_STATUS(move))
    {
        *abilityPopup = ABILITY_GOOD_AS_GOLD;
        *immunityScript = BattleScript_SoundproofProtected;
        effect = 2;
    }

    gBattlerAttacker = actualAttacker;
    return effect;
}

int IsBloodStainAffected(int battler)
{
    if (IS_BATTLER_OF_TYPE(battler, TYPE_GHOST)) return FALSE;
    if (IS_BATTLER_OF_TYPE(battler, TYPE_ROCK)) return FALSE;
    return BATTLER_HAS_ABILITY(battler, ABILITY_BLOOD_STAIN);
}

int IsUnaware(int battler)
{
    int ability = GetBattlerAbility(battler);
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_UNAWARE, ability)) return TRUE;
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_CONTEMPT, ability)) return TRUE;
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_SWORD_OF_DAMNATION, ability)) return TRUE;
    return FALSE;
}

#define ABILITY_STATUS_EFFECT(effect) \
gBattleScripting.moveEffect = effect; \
BattleScriptCall(BattleScript_AbilityStatusEffect); \
gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;

int HandleAttackerAbility(int abilityNumber, int battler, int target, int move) {
    u16 abilities[4] = {0};
    int i, ability, moveType;

    GET_MOVE_TYPE(move, moveType)
    gBattlerAbility = battler;

    if (abilityNumber > NUM_ABILITY_SLOTS)
    {
        return HandleMiscAbilityMoveEffects(battler, target, move);
    }

    GET_ALL_BATTLER_ABILITIES(abilities, battler, battler);
    ability = gBattleScripting.abilityPopupOverwrite = abilities[abilityNumber];

    if (HandleAttackerOrDefenderAbility(ability, battler, target, move)) return TRUE;
    
    switch (ability)
    {
        case ABILITY_SHIELDS_DOWN:
            if (!IsBattlerAlive(battler)) break;
            if (gBattleMoves[move].effect != EFFECT_SHELL_SMASH) break;
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;

            for (i = 0; i < ARRAY_COUNT(sHpTransformations); i++)
            {
                if (sHpTransformations[i][0] == ability && gBattleMons[battler].species == sHpTransformations[i][1])
                    break;
            }
        
            if (i < ARRAY_COUNT(sHpTransformations))
            {
                UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, sHpTransformations[i][2]);
                SetAbilityState(battler, ability, TRUE);
                gBattleMons[battler].species = sHpTransformations[i][2];
                BattleScriptCall(BattleScript_AttackerFormChange);
                return TRUE;
            }
            break;

        case ABILITY_GULP_MISSILE:
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;
            if (gBattleMons[battler].species != SPECIES_CRAMORANT) break;

            if ((gCurrentMove == MOVE_SURF && TARGET_TURN_DAMAGED)
                || gStatuses3[gBattlerAttacker] & STATUS3_UNDERWATER
                || (gCurrentMove == MOVE_DIVE && gBattleScripting.acceleratedTwoTurn))
            {
                u16 newSpecies = gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2 ? SPECIES_CRAMORANT_GORGING : SPECIES_CRAMORANT_GULPING;
                UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, newSpecies);
                gBattleMons[battler].species = newSpecies;
                BattleScriptCall(BattleScript_AttackerFormChange);
                return TRUE;
            }
            break;

        case ABILITY_HYDRO_CIRCUIT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (moveType != TYPE_WATER) break;

            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        
        case ABILITY_VITALITY_STRIKE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (!IS_IRON_FIST(battler, move)) break;

            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        
        case ABILITY_PURE_LOVE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (!(gBattleMons[target].status2 & STATUS2_INFATUATION)) break;

            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        
        case ABILITY_GROWING_TOOTH:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)) break;
            if (!ChangeStatBuffs(battler, StatBuffValue(1), STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL)) break;

            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_AttackBoostActivates);
            return TRUE;
        
        case ABILITY_SPINNING_TOP:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_FIGHTING) break;
            if (!CheckAndSetOncePerTurnAbility(battler, ability)) break;
            
            i = FALSE;
            if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_HAZARDS_ANY || gSideTimers[GetBattlerSide(battler)].hotCoals)
            {
                gSideStatuses[GetBattlerSide(battler)] &= ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES_DAMAGED | SIDE_STATUS_STICKY_WEB);
                gSideTimers[GetBattlerSide(battler)].hotCoals = TRUE;
                BattleScriptCall(BattleScript_AnnounceRemovedHazards);
                gBattleScripting.battler = battler;
                i = TRUE;
            }
            if (ChangeStatBuffs(battler, StatBuffValue(1), STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))
            {
                BattleScriptCall(BattleScript_AnnounceAbilitySpeedBoost);
                gBattleScripting.battler = battler;
                i = TRUE;
            }

            if (i)
            {
                BattleScriptCall(BattleScript_AbilityPopUp);
                return TRUE;
            }
            break;
        
        case ABILITY_VITAL_SPIRIT:
            if (AbilityHealMonStatus(battler, ability)) return TRUE;
            break;
        
        case ABILITY_HARDENED_SHEATH:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!gBattleMoves[move].hornBased) break;
            if (!ChangeStatBuffs(battler, StatBuffValue(1), STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL)) break;

            BattleScriptCall(BattleScript_AttackBoostActivates);
            gBattleScripting.battler = battler;
            return TRUE;
        
        case ABILITY_LOUD_BANG:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeConfused(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_SOUND)) break;
            if (Random() % 2) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_CONFUSION)
            return TRUE;
        
        case ABILITY_PIERCING_SOLO:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBleed(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_SOUND)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_BLEED)
            return TRUE;
        
        case ABILITY_DENTING_BLOWS:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!gBattleMoves[move].hammerBased) break;
            if (!StatLowerableOrMirrorArmor(target, STAT_DEF)) break;

            i = GetOncePerTurnAbilityCounter(battler, ability);
            if (i & (1 << target)) break;

            SetOncePerTurnAbilityCounter(battler, ability, i | (1 << target));
            ABILITY_STATUS_EFFECT(MOVE_EFFECT_DEF_MINUS_1)
            return TRUE;

        case ABILITY_WHIPLASH:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (Random() % 2) break;
            if (!IS_MOVE_PHYSICAL(move)) break;
            if (!StatLowerableOrMirrorArmor(target, STAT_DEF)) break;

            i = GetOncePerTurnAbilityCounter(battler, ability);
            if (i & (1 << target)) break;

            SetOncePerTurnAbilityCounter(battler, ability, i | (1 << target));
            ABILITY_STATUS_EFFECT(MOVE_EFFECT_DEF_MINUS_1)
            return TRUE;

        case ABILITY_BEAUTIFUL_MUSIC:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (Random() % 2) break;
            if (!CanInfatuate(battler, target)) break;

            gBattleMons[target].status2 |= STATUS2_INFATUATED_WITH(gBattlerAttacker);
            BattleScriptCall(BattleScript_BeautifulMusicActivates);
            return TRUE;
        
        case ABILITY_RESONANCE:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBleed(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_SOUND)) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_BLEED)
            return TRUE;
        
        case ABILITY_TOXIC_CHAIN:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBePoisoned(battler, target)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_TOXIC)
            return TRUE;
        
        case ABILITY_ELECTRIC_BURST:
            if (moveType != TYPE_ELECTRIC) break;

        HANDLE_ABILITY_RECOIL_EFFECT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (gBattleMons[gBattlerAttacker].hp <= 1) break;
            if (BATTLER_HAS_MAGIC_GUARD(gBattlerAttacker)) break;
            
            gBattleMoveDamage = gTurnStructs[target].dmg / 10;
            if (!gBattleMoveDamage) gBattleMoveDamage = 1;
            else if (gBattleMoveDamage >= gBattleMons[gBattlerAttacker].hp) gBattleMoveDamage = gBattleMons[gBattlerAttacker].hp - 1;
            BattleScriptCall(BattleScript_UserGetsReckoilDamaged);
            return TRUE;
        
        case ABILITY_INFERNAL_RAGE:
            if (moveType != TYPE_FIRE) break;
            goto HANDLE_ABILITY_RECOIL_EFFECT;
        
        case ABILITY_ARCHMAGE:
            if (!DidMoveHit()) break;
            if (IS_MOVE_STATUS(move)) break;
            if (Random() % 100 >= 30) break;
            
            switch (moveType) {
                case TYPE_POISON:
                    if (!IsBattlerAlive(target)) break;
                    if (!CanBePoisoned(battler, target)) break;
                    
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_TOXIC)
                    return TRUE;
                
                case TYPE_ICE:
                    if (!IsBattlerAlive(target)) break;
                    if (!CanGetFrostbite(target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_FROSTBITE)
                    return TRUE;
                
                case TYPE_WATER:
                    if (!IsBattlerAlive(target)) break;
                    if (!CanBeConfused(target)) break;
                    
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_CONFUSION)
                    return TRUE;
                
                case TYPE_FIRE:
                    if (!IsBattlerAlive(target)) break;
                    if (!CanBeBurned(target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_BURN);
                    return TRUE;
                
                case TYPE_ELECTRIC:
                    if (!IsBattlerAlive(target)) break;
                    if (!TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer)) break;

                    BattleScriptCall(BattleScript_Archmage_Effect_Type_Electric);
                    return TRUE;
                
                case TYPE_PSYCHIC:
                    if (!IsBattlerAlive(target)) break;
                    if (!TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer)) break;

                    BattleScriptCall(BattleScript_Archmage_Effect_Type_Psychic);
                    return TRUE;
                
                case TYPE_FAIRY:
                    if (!IsBattlerAlive(target)) break;
                    if (!TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer)) break;

                    BattleScriptCall(BattleScript_Archmage_Effect_Type_Fairy);
                    return TRUE;
                
                case TYPE_GRASS:
                    if (!IsBattlerAlive(target)) break;
                    if (!TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer)) break;

                    BattleScriptCall(BattleScript_Archmage_Effect_Type_Fairy);
                    return TRUE;
                
                case TYPE_NORMAL:
                    if (!IsBattlerAlive(target)) break;
                    if (gVolatileStructs[target].encoreTimer) break;
                    if (IsAbilityOnSide(target, ABILITY_AROMA_VEIL)) break;
                    if (BATTLER_HAS_ABILITY(target, ABILITY_OBLIVIOUS)) break;
                    
                    BattleScriptCall(BattleScript_Archmage_Effect_Type_Normal);
                    return TRUE;
                
                case TYPE_ROCK:
                    if (gSideStatuses[GetBattlerSide(target)] & SIDE_STATUS_STEALTH_ROCK) break;

                    gSideStatuses[GetBattlerSide(target)] |= (SIDE_STATUS_STEALTH_ROCK);
                    gSideTimers[GetBattlerSide(target)].stealthRockType= TYPE_ROCK;
                    BattleScriptCall(BattleScript_Archmage_Effect_Type_Rock);
                    return TRUE;

                case TYPE_GHOST:
                    if (!IsBattlerAlive(target)) break;
                    if (!CanBeDisabled(target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_DISABLE)
                    return TRUE;
                
                case TYPE_DARK:
                    if (!IsBattlerAlive(target)) break;
                    if (!CanBleed(target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_BLEED)
                    return TRUE;
                
                case TYPE_FIGHTING:
                    if (!IsBattlerAlive(target)) break;
                    if (CompareStat(battler, STAT_SPATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_SP_ATK_PLUS_1 | MOVE_EFFECT_AFFECTS_USER)
                    return TRUE;
                
                case TYPE_FLYING:
                    if (!IsBattlerAlive(target)) break;
                    if (CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_EQUAL)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_SPD_PLUS_1 | MOVE_EFFECT_AFFECTS_USER)
                    return TRUE;
                
                case TYPE_BUG:
                    // TODO: Set sticky web
                    break;
                
                case TYPE_DRAGON:
                    if (!IsBattlerAlive(target)) break;
                    if (!StatLowerableOrMirrorArmor(target, STAT_ATK)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_ATK_MINUS_1)
                    return TRUE;
                
                case TYPE_GROUND:
                    if (!IsBattlerAlive(target)) break;
                    if (gBattleMons[target].status2 & STATUS2_ESCAPE_PREVENTION) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_PREVENT_ESCAPE)
                    return TRUE;
                
                case TYPE_STEEL:
                    if (!IsBattlerAlive(target)) break;
                    if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_DEF_PLUS_1 | MOVE_EFFECT_AFFECTS_USER)
                    return TRUE;
            }
            break;

        case ABILITY_SOLENOGLYPHS:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBePoisoned(battler, target)) break;
            if (!(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)) break;
            if (Random() % 2) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_TOXIC)
            return TRUE;

        case ABILITY_FROSTMAW:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanGetFrostbite(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)) break;
            if (Random() % 2) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_FROSTBITE)
            return TRUE;
        
        case ABILITY_ASSASSINS_TOOLS:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!IsMoveMakingContact(move, battler)) break;

            switch (Random() % 3)
            {
                case 0:
                    if (!CanBePoisoned(battler, target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_POISON)
                    return TRUE;
                
                case 1:
                    if (!CanBeParalyzed(battler, target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
                    return TRUE;
                
                case 2:
                    if (!CanBleed(target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_BLEED)
                    return TRUE;
            }
            break;
        
        case ABILITY_FLAMING_JAWS:
        case ABILITY_FLAMING_MAW:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeBurned(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)) break;
            if (Random() % 2) break;

        case ABILITY_ARC_FLASH:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeParalyzed(battler, target)) break;
            if (Random() % 2) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
            return TRUE;
        
        case ABILITY_RADIO_JAM:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeDisabled(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_SOUND)) break;
            if (Random() % 100 >= 20) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_DISABLE)
            return TRUE;
        
        case ABILITY_DEMOLITIONIST:
            if (!DidMoveHit()) break;
            if (!gVolatileStructs[battler].readiedAction) break;
            {
            int opposingSide = GetBattlerSide(target);
            if (!gSideTimers[opposingSide].reflectTimer && !gSideTimers[opposingSide].lightscreenTimer && !gSideTimers[opposingSide].auroraVeilTimer) break;
            BattleScriptCall(BattleScript_AttackerShattersScreens);
            BattleScriptCall(BattleScript_AbilityPopUp);
            return TRUE;
            }

        
        case ABILITY_PINNACLE_BLADE:
            if (!DidMoveHit()) break;
            if (!(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)) break;

            {
            int shouldApply = FALSE;
            int opposingSide = GetBattlerSide(target);

            if (gVolatileStructs[target].substituteHP)
            {
                gVolatileStructs[target].substituteHP = 0;
                BattleScriptCall(BattleScript_AttackerDestroysSubstitute);
                shouldApply = TRUE;
            }

            if (IsBattlerAlive(target))
            {
                if (gSideTimers[opposingSide].reflectTimer
                    || gSideTimers[opposingSide].lightscreenTimer
                    || gSideTimers[opposingSide].auroraVeilTimer)
                {
                    BattleScriptCall(BattleScript_AttackerShattersScreens);
                    shouldApply = TRUE;
                }

                if (IS_BATTLER_PROTECTED(target))
                {
                    gBattleScripting.moveEffect = MOVE_EFFECT_FEINT;
                    SetMoveEffect(TRUE, TRUE);
                    shouldApply = TRUE;
                }
            }

            if (shouldApply)
            {
                BattleScriptCall(BattleScript_AbilityPopUp);
                return TRUE;
            }
            }
            break;

        case ABILITY_FEARMONGER:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeParalyzed(battler, target)) break;
            if (!IsMoveMakingContact(move, battler)) break;
            if (Random() % 100 >= 10) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
            return TRUE;
        
        case ABILITY_YUKI_ONNA:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanInfatuate(battler, target)) break;
            if (Random() % 100 >= 10) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_ATTRACT)
            return TRUE;
        
        case ABILITY_STUN_SHOCK:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (Random() % 100 >= 60) break;
            switch (Random() % 2)
            {
                case 0:
                    if (!CanBePoisoned(battler, target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_POISON)
                    return TRUE;
                
                case 1:
                    if (!CanBeParalyzed(battler, target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
                    return TRUE;
            }
            break;
        
        case ABILITY_SHOCKING_JAWS:
        case ABILITY_SHOCKING_MAW:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeParalyzed(battler, target)) break;
            if (!(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)) break;
            if (Random() % 2) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
            return TRUE;

        case ABILITY_VENOBLAZE_PINCERS:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!IS_MOVE_PHYSICAL(move)) break;
            if (Random() % 100 >= 20) break;

            switch (Random() % 2)
            {
                case 0:
                    if (!CanBeBurned(target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_BURN)
                    return TRUE;
                
                case 1:
                    if (!CanBeParalyzed(battler, target)) break;
                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
                    return TRUE;
            }
            break;
        
        case ABILITY_MOLTEN_BLADES:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBeBurned(target)) break;
            if (!(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)) break;
            if (Random() % 100 >= 20) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_BURN)
            return TRUE;
        
        case ABILITY_DEAD_POWER:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (gBattleMons[target].status2 & STATUS2_CURSED) break;
            if (!IsMoveMakingContact(move, battler)) break;
            if (Random() % 100 >= 20) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_CURSE)
            return TRUE;
        
        case ABILITY_SPECTRAL_SHROUD:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanBePoisoned(battler, target)) break;
            if (!gBattleStruct->ateBoost[battler]) break;
            if (moveType != TYPE_GHOST) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_TOXIC)
            return TRUE;
        
        case ABILITY_ANGELS_WRATH:
            switch (move) {
                case MOVE_TACKLE:
                    if (!ShouldApplyOnHitAffect(target)) break;
                    if (!gVolatileStructs[target].encoreTimer) break;
                    if (!gVolatileStructs[target].disableTimer) break;

                    gVolatileStructs[target].encoreTimer = 2;
                    gVolatileStructs[target].encoredMove = gBattleMons[target].moves[0];

                    gVolatileStructs[target].disableTimer = gVolatileStructs[target].disableTimerStartValue = 2;
                    gVolatileStructs[target].disabledMove = gBattleMons[target].moves[0];
                    
                    BattleScriptCall(BattleScript_AngelsWrath_Effect_Tackle);
                    return TRUE;
                
                case MOVE_STRING_SHOT:
                    if (!WasMoveSuccessful()) break;

                    i = GetBattlerSide(target);
                    if (gSideStatuses[i] & SIDE_STATUS_STEALTH_ROCK
                        && gSideStatuses[i] & SIDE_STATUS_TOXIC_SPIKES
                        && gSideStatuses[i] & SIDE_STATUS_SPIKES
                        && gSideStatuses[i] & SIDE_STATUS_STICKY_WEB)
                        break;
                        
                    gSideStatuses[i] |= (SIDE_STATUS_STEALTH_ROCK);
                    gSideTimers[i].stealthRockType = TYPE_ROCK;

                    gSideStatuses[i] |= (SIDE_STATUS_TOXIC_SPIKES);
                    gSideTimers[i].toxicSpikesAmount++;
                    if (gSideTimers[i].toxicSpikesAmount > 2) gSideTimers[i].toxicSpikesAmount = 2;
                    
                    gSideStatuses[i] |= (SIDE_STATUS_SPIKES);
                    gSideTimers[i].spikesAmount++;
                    if (gSideTimers[i].spikesAmount > 3) gSideTimers[i].spikesAmount = 3;

                    gSideStatuses[i] |= (SIDE_STATUS_STICKY_WEB);

                    BattleScriptCall(BattleScript_AngelsWrath_Effect_String_Shot);
                    return TRUE;
                
                case MOVE_HARDEN:
                    if (!WasMoveSuccessful()) break;
                    
                    {
                    int activated = FALSE;
                    for (i = 1; i < NUM_STATS; i++)
                    {
                        if (i == STAT_DEF) continue;
                        activated |= ChangeStatBuffs(battler, StatBuffValue(1), i, MOVE_EFFECT_AFFECTS_USER, NULL);
                    }

                    if (activated)
                    {
                        BattleScriptCall(BattleScript_AngelsWrath_Effect_Harden);
                        return TRUE;
                    }
                    }
                    break;
                
                case MOVE_IRON_DEFENSE:
                    if (!WasMoveSuccessful()) break;

                    gRoundStructs[battler].angelsWrathProtected = TRUE;
                    BattleScriptCall(BattleScript_AngelsWrath_Effect_Iron_Defense);
                    return TRUE;

                case MOVE_ELECTROWEB:
                    if (!ShouldApplyOnHitAffect(target)) break;
                    if (gBattleMons[target].status2 & STATUS2_ESCAPE_PREVENTION) break;
                    if (gBattleMons[target].statStages[STAT_SPEED] == MIN_STAT_STAGE) break;

                    gBattleMons[target].statStages[STAT_SPEED] = MIN_STAT_STAGE;
                    gBattleMons[target].status2 |= (STATUS2_ESCAPE_PREVENTION);
                    BattleScriptCall(BattleScript_AngelsWrath_Effect_Electroweb);
                    return TRUE;
                
                case MOVE_BUG_BITE:
                    if (!ShouldApplyOnHitAffect(battler)) break;
                    if (BATTLER_MAX_HP(battler)) break;
                    if (BATTLER_HEALING_BLOCKED(battler)) break;

                    BattleScriptCall(BattleScript_AngelsWrath_Effect_Bug_Bite_2);
                    return TRUE;
            }
            break;
        
        case ABILITY_ELEMENTAL_CHARGE:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (Random() % 100 >= 20) break;

            switch (moveType)
            {
                case TYPE_ELECTRIC:
                    if (!CanBeParalyzed(battler, target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS)
                    return TRUE;
                
                case TYPE_FIRE:
                    if (!CanBeBurned(target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_BURN)
                    return TRUE;
                
                case TYPE_ICE:
                    if (!CanGetFrostbite(target)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_FROSTBITE)
                    return TRUE;
            }
            break;
        
        case ABILITY_STENCH:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanMoveHaveExtraFlinchChance(move)) break;
            if (Random() % 100 >= 10) break;

            gBattleScripting.moveEffect = MOVE_EFFECT_FLINCH;
            SetMoveEffect(FALSE, 0);
            break;
        
        case ABILITY_HAUNTING_FRENZY:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (!CanMoveHaveExtraFlinchChance(move)) break;
            if (Random() % 100 >= 20) break;

            gBattleScripting.moveEffect = MOVE_EFFECT_FLINCH;
            SetMoveEffect(FALSE, 0);
            break;
        
        case ABILITY_ABSORBANT:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) break;
            if (gStatuses3[target] & STATUS3_LEECHSEED)
            if (gBattleMoves[move].effect != EFFECT_ABSORB && gBattleMoves[move].effect != EFFECT_DREAM_EATER) break;

            gStatuses3[target] |= battler;
            gStatuses3[target] |= STATUS3_LEECHSEED;
            BattleScriptCall(BattleScript_AbsorbantActivated);
            return TRUE;
        
        case ABILITY_FUNGAL_INFECTION:
            if (!ShouldApplyOnHitAffect(target)) break;
            if (IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) break;
            if (gStatuses3[target] & STATUS3_LEECHSEED)
            if (!IsMoveMakingContact(move, battler)) break;

            gStatuses3[target] |= battler;
            gStatuses3[target] |= STATUS3_LEECHSEED;
            BattleScriptCall(BattleScript_AbsorbantActivated);
            return TRUE;
        
        case ABILITY_GRIP_PINCER:
            if (!ShouldApplyOnHitAffect(gBattlerTarget)) break;
            if (!IsBattlerAlive(battler)) break;
            if (!IsMoveMakingContact(move, battler)) break;
            if (gBattleMons[target].status2 & STATUS2_WRAPPED) break;
            if (Random() % 2) break;

            gBattleMons[target].status2 |= STATUS2_WRAPPED;
            if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_GRIP_CLAW)
                gVolatileStructs[target].wrapTurns = 7;
            else
                gVolatileStructs[target].wrapTurns = (Random() % 2) + 4;

            gBattleStruct->wrappedMove[target] = gCurrentMove;
            gBattleStruct->wrappedBy[target] = battler;
            BattleScriptCall(BattleScript_GripPincerActivated);
            return TRUE;
    }

    return FALSE;
}

int CheckHalfHpAbility(int battlerDef, int battlerAtk)
{
    if (!ShouldApplyOnHitAffect(battlerDef)) return FALSE;
    if (gBattleStruct->hpBefore[battlerDef] <= gBattleMons[battlerDef].maxHP / 2) return FALSE;
    if (gBattleMons[battlerDef].hp > gBattleMons[battlerDef].maxHP / 2) return FALSE;
    if (gTurnStructs[battlerAtk].multiHitCounter > 1) return FALSE;
    return TRUE;
}

static int HandleDefenderAbilityAs(int ability, int battler, int attacker, int move, int moveType);

int HandleDefenderAbility(int abilityNumber, int battler, int attacker, int move)
{
    u16 abilities[4] = {0};
    int i, ability, moveType;

    GET_MOVE_TYPE(move, moveType)
    gBattlerAbility = battler;

    if (abilityNumber > NUM_ABILITY_SLOTS)
    {
        return HandleMiscAbilityMoveEffects(battler, attacker, move);
    }

    GET_ALL_BATTLER_ABILITIES(abilities, battler, battler);
    ability = gBattleScripting.abilityPopupOverwrite = abilities[abilityNumber];

    if (HandleAttackerOrDefenderAbility(ability, battler, attacker, move)) return TRUE;

    return HandleDefenderAbilityAs(ability, battler, attacker, move, moveType);
}

int HandleDefenderAbilityAs(int ability, int battler, int attacker, int move, int moveType)
{
    switch (ability)
    {
        case ABILITY_LINGERING_AROMA:
        case ABILITY_MUMMY:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (GetBattlerAbility(battler) != ability) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (IsPersistentOrUnsuppressableAbility(gBattleMons[attacker].ability)) break;
            if (DoesBattlerHaveAbilityShield(attacker)) break;

            UpdateAbilityStateIndicesForNewAbility(attacker, ability);
            gLastUsedAbility = gBattleMons[attacker].ability = ability;
            BattleScriptCall(BattleScript_MummyActivates);
            return TRUE;
        
        case ABILITY_WANDERING_SPIRIT:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (GetBattlerAbility(battler) != ability) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (IsPersistentOrUnsuppressableAbility(gBattleMons[attacker].ability)) break;
            if (DoesBattlerHaveAbilityShield(attacker)) break;

            UpdateAbilityStateIndicesForNewAbility(attacker, ability);
            gLastUsedAbility = gBattleMons[attacker].ability = ability;
            UpdateAbilityStateIndicesForNewAbility(battler, gLastUsedAbility);
            gBattleMons[battler].ability = gLastUsedAbility;
            BattleScriptCall(BattleScript_WanderingSpiritActivates);
            return TRUE;
        
        case ABILITY_GULP_MISSILE:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            {
            int species = gBattleMons[battler].species;
            if (species != SPECIES_CRAMORANT_GORGING && species != SPECIES_CRAMORANT_GULPING) break;
            gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = species;
            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_CRAMORANT);
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 4;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            BattleScriptCall(species == SPECIES_CRAMORANT_GORGING ? BattleScript_GulpMissileGorging : BattleScript_GulpMissileGulping);
            return TRUE;
            }
        
        case ABILITY_ILLUSION:
            if (!DidMoveHit()) break;
            if (!gBattleStruct->illusion[battler].on) break;
            if (gBattleStruct->illusion[battler].broken) break;

            BattleScriptCall(BattleScript_IllusionOff);
            return TRUE;
        
        case ABILITY_EMERGENCY_EXIT:
        case ABILITY_WIMP_OUT:
            if (!CheckHalfHpAbility(battler, attacker)) break;
            if (TestSheerForceFlag(attacker, gCurrentMove)) break;
            if (!CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER) break;
            if (gBattleTypeFlags & BATTLE_TYPE_ARENA) break;
            if (!CountUsablePartyMons(battler)) break;
            gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
            break;
        
        case ABILITY_RESTRAINING_ORDER:
            {
            RestrainingOrderState state = GetAbilityState(battler, ABILITY_RESTRAINING_ORDER);

            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER) break;
            if (gBattleTypeFlags & BATTLE_TYPE_ARENA) break;
            if (!CountUsablePartyMons(battler)) break;

            SetAbilityState(battler, ABILITY_RESTRAINING_ORDER, RESTRAINING_ORDER_ACTIVATING);
            break;            
            }
        
        case ABILITY_THERMAL_EXCHANGE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_FIRE) break;
            if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_FURNACE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_ROCK) break;
            if (CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_SPEED, 2);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_WELL_BAKED_BODY:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_FIRE) break;
            if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_DEF, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_EVAPORATE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_WATER) break;
            if (!TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer)) break;

            BattleScriptCall(BattleScript_MistySurgeActivatesRet);
            return TRUE;
        
        case ABILITY_COLD_REBOUND:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
            break;
        
        case ABILITY_SNAP_TRAP_WHEN_HIT:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
            break;
        
        case ABILITY_PARRY:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
            break;
        
        case ABILITY_ULTRA_INSTINCT:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
            break;
        
        case ABILITY_ICE_DOWNFALL:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
            break;
        
        case ABILITY_ATOMIC_BURST:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!(gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE)) break;

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_HYPER_BEAM, 50);
            break;
        
        case ABILITY_LOOSE_ROCKS:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK) break;

            BattleScriptCall(BattleScript_DefenderSetsStealthRock);
            return TRUE;
        
        case ABILITY_WIND_POWER:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!gBattleMoves[move].airBased) break;
            if (gStatuses3[battler] & STATUS3_CHARGED_UP) break;

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
            BattleScriptCall(BattleScript_ElectromorphosisActivates);
            return TRUE;
        
        case ABILITY_ELECTROMORPHOSIS:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (gStatuses3[battler] & STATUS3_CHARGED_UP) break;

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
            BattleScriptCall(BattleScript_ElectromorphosisActivates);
            return TRUE;

        case ABILITY_FLAMMABLE_COAT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_FIRE) break;
            if (gBattleMons[battler].species != SPECIES_LUMBERING_SLOTH) break;
            if (gBattleMons[battler].status2 & STATUS2_TRANSFORMED) break;

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_LUMBERING_SLOTH_ENGULFED);
            gBattleMons[battler].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
            BattleScriptCall(BattleScript_TargetFormChange);
            return TRUE;
        
        case ABILITY_ROUGH_SKIN:
        case ABILITY_IRON_BARBS:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (BATTLER_HAS_MAGIC_GUARD(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 8;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            BattleScriptCall(BattleScript_IronBarbsActivates);
            return TRUE;

        case ABILITY_DOUBLE_IRON_BARBS:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (BATTLER_HAS_MAGIC_GUARD(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 6;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            BattleScriptCall(BattleScript_IronBarbsActivates);
            return TRUE;
        
        case ABILITY_RATTLED:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_DARK && moveType != TYPE_BUG && moveType != TYPE_GHOST) break;
            if (CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_EQUAL)) break;
            
            SET_STATCHANGER(STAT_SPEED, 1, FALSE);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_CURSED_BODY:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (gVolatileStructs[attacker].disabledMove) break;
            if (attacker == battler) break;
            if (IsAbilityOnSide(attacker, ABILITY_AROMA_VEIL)) break;
            if (!gBattleMons[attacker].pp[gChosenMovePos]) break;
            if (Random() % 100 >= 30) break;

            gVolatileStructs[attacker].disabledMove = gChosenMove;
            gVolatileStructs[attacker].disableTimer = 4;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove);
            BattleScriptCall(BattleScript_CursedBodyActivates);
            return TRUE;
        
        case ABILITY_SPITEFUL:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (move == MOVE_STRUGGLE) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (!gBattleMons[attacker].pp[gChosenMovePos]) break;

            BattleScriptCall(BattleScript_AbilitySpiteful);
            return TRUE;
        
        case ABILITY_COTTON_DOWN:
            if (!ShouldApplyOnHitAffect(battler)) break;

            gEffectBattler = battler;
            BattleScriptCall(BattleScript_CottonDownActivates);
            return TRUE;
        
        case ABILITY_STEAM_ENGINE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_EQUAL)) break;
            if (moveType != TYPE_FIRE && moveType != TYPE_WATER) break;

            SetStatChanger(STAT_SPEED, 12);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_SAND_SPIT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (gBattleWeather & WEATHER_SANDSTORM_ANY) break;
            if (gBattleWeather & WEATHER_PRIMAL_ANY)
            {
                BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
                return TRUE;
            }
            else if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, TRUE))
            {
                gBattleScripting.battler = battler;
                BattleScriptCall(BattleScript_SandSpitActivates);
                return TRUE;
            }
            break;
        
        case ABILITY_CRYO_PROFICIENCY:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (gBattleWeather & WEATHER_HAIL_ANY) break;
            if (gBattleWeather & WEATHER_PRIMAL_ANY)
            {
                BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
                return TRUE;
            }
            else if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE))
            {
                gBattleScripting.battler = battler;
                BattleScriptCall(BattleScript_CryoProficiencyActivates);
                return TRUE;
            }
            break;
        
        case ABILITY_PERISH_BODY:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!IsBattlerAlive(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (gStatuses3[attacker] & STATUS3_PERISH_SONG) break;

            if (!(gStatuses3[battler] & STATUS3_PERISH_SONG))
            {
                gStatuses3[battler] |= STATUS3_PERISH_SONG;
                gVolatileStructs[battler].perishSongTimer = 3;
                gVolatileStructs[battler].perishSongTimerStartValue = 3;
            }
            gStatuses3[attacker] |= STATUS3_PERISH_SONG;
            gVolatileStructs[attacker].perishSongTimer = 3;
            gVolatileStructs[attacker].perishSongTimerStartValue = 3;
            BattleScriptCall(BattleScript_PerishBodyActivates);
            return TRUE;
        
        case ABILITY_GUILT_TRIP:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (IsBattlerAlive(battler)) break;
            if (CompareStat(gBattlerAttacker, STAT_ATK, MIN_STAT_STAGE, CMP_EQUAL)
                && CompareStat(gBattlerAttacker, STAT_SPATK, MIN_STAT_STAGE, CMP_EQUAL))
                break;

            BattleScriptCall(BattleScript_GuiltTrip);
            return TRUE;
        
        case ABILITY_ILL_WILL:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (move == MOVE_STRUGGLE) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (!gBattleMons[attacker].pp[gChosenMovePos]) break;

            gBattleMons[attacker].pp[gChosenMovePos] = 0;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove)
            gActiveBattler = battler;
            BtlController_EmitSetMonData(0, gChosenMovePos + REQUEST_PPMOVE1_BATTLE, 0, 1, &gBattleMons[gActiveBattler].pp[gChosenMovePos]);
            BattleScriptCall(BattleScript_IllWillTakesPp);
            return TRUE;
        
        case ABILITY_INNARDS_OUT:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (IsBattlerAlive(battler)) break;
            if (BATTLER_HAS_MAGIC_GUARD(attacker)) break;

            gBattleMoveDamage = gTurnStructs[battler].dmg;
            BattleScriptCall(BattleScript_AftermathDmg);
            return TRUE;
        
        case ABILITY_AFTERMATH:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (IsBattlerAlive(battler)) break;
            if (BATTLER_HAS_MAGIC_GUARD(attacker)) break;

            gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
            if (!gBattleMoveDamage) gBattleMoveDamage = 1;
            BattleScriptCall(BattleScript_AftermathDmg);
            return TRUE;
        
        case ABILITY_PATCHWORK:
            if (!GetSingleUseAbilityCounter(battler, ABILITY_PATCHWORK)) break;
            if (!IsBattlerAlive(attacker)) break;
            if (gBattleMons[attacker].status2 & STATUS2_CURSED) break;

            SetSingleUseAbilityCounter(battler, ABILITY_PATCHWORK, 0);
            ABILITY_STATUS_EFFECT(MOVE_EFFECT_CURSE | MOVE_EFFECT_AFFECTS_USER)
            return TRUE;
        
        case ABILITY_EFFECT_SPORE:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_GRASS)) break;
            if (BATTLER_HAS_ABILITY(attacker, ABILITY_OVERCOAT)) break;
            if (BATTLER_HAS_ABILITY(attacker, ABILITY_EFFECT_SPORE)) break;
            if (GetBattlerHoldEffect(attacker, TRUE) == HOLD_EFFECT_SAFETY_GOGGLES) break;
            if (Random() % 100 >= 30) break;

            switch (Random() % 3)
            {
                case 0:
                    if (!CanBePoisoned(battler, attacker)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_POISON | MOVE_EFFECT_AFFECTS_USER)
                    return TRUE;
                
                case 1:
                    if (!CanBeParalyzed(battler, attacker)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS | MOVE_EFFECT_AFFECTS_USER)
                    return TRUE;
                
                case 2:
                    if (!CanSleep(attacker)) break;

                    ABILITY_STATUS_EFFECT(MOVE_EFFECT_SLEEP | MOVE_EFFECT_AFFECTS_USER)
                    return TRUE;
            }
            break;
        
        case ABILITY_INFLATABLE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL) && CompareStat(battler, STAT_SPDEF, MAX_STAT_STAGE, CMP_EQUAL)) break;
            if (moveType != TYPE_FIRE && moveType != TYPE_FLYING) break;
            BattleScriptCall(BattleScript_InflatableActivates);
            gBattleScripting.battler = battler;
            return TRUE;

        case ABILITY_BALLOON_BOMBER:
            if (HandleDefenderAbilityAs(ABILITY_INFLATABLE, battler, attacker, move, moveType)) return TRUE;
            if (HandleDefenderAbilityAs(ABILITY_AFTERMATH, battler, attacker, move, moveType)) return TRUE;
            break;
        
        case ABILITY_WATER_COMPACTION:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (moveType != TYPE_WATER) break;
            if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_DEF, 2);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_VENGEFUL_SPIRIT:
        case ABILITY_HAUNTED_SPIRIT:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (IsBattlerAlive(battler)) break;
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_GHOST)) break;
            if (gBattleMons[attacker].status2 & STATUS2_CURSED) break;
            if (!IsMoveMakingContact(move, attacker)) break;

            gBattleMons[attacker].status2 |= STATUS2_CURSED;
            BattleScriptCall(BattleScript_HauntedSpiritActivated);
            return TRUE;
        
        case ABILITY_MAGICAL_DUST:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_PSYCHIC)) break;

            
            gBattleMons[attacker].type3 = TYPE_PSYCHIC;
            PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[attacker].type3);
            BattleScriptCall(BattleScript_AttackerBecameTheType);
            return TRUE;
        
        case ABILITY_WEAK_ARMOR:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!IS_MOVE_PHYSICAL(move)) break;
            if (CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_EQUAL) && CompareStat(battler, STAT_DEF, MIN_STAT_STAGE, CMP_EQUAL)) break;

            if (gBattleMoves[move].effect == EFFECT_HIT_ESCAPE && CanBattlerSwitch(attacker))
                gRoundStructs[battler].disableEjectPack = TRUE;  // Set flag for target

            BattleScriptCall(BattleScript_WeakArmorActivates);
            return TRUE;

        case ABILITY_ARC_FLASH:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!CanBeBurned(attacker)) break;
            if (Random() % 2) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_BURN | MOVE_EFFECT_AFFECTS_USER)
            return TRUE;
        
        case ABILITY_CROWNED_SHIELD:
        case ABILITY_STAMINA:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

            if (gIsCriticalHit)
            {
                SetStatChanger(STAT_DEF, 12);
                BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
            }
            else
            {
                SetStatChanger(STAT_DEF, 1);
                BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            }
            return TRUE;
            
        case ABILITY_FORTITUDE:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (CompareStat(battler, STAT_SPDEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

            if (gIsCriticalHit)
            {
                SetStatChanger(STAT_SPDEF, 12);
                BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
            }
            else
            {
                SetStatChanger(STAT_SPDEF, 1);
                BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            }
            return TRUE;
        
        case ABILITY_RAGE_POINT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!gIsCriticalHit) break;
            if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_EQUAL) && CompareStat(battler, STAT_SPATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

            BattleScriptCall(BattleScript_RagePointActivates);
            return TRUE;
            
        case ABILITY_CROWNED_SWORD:
        case ABILITY_ANGER_POINT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

            if (gIsCriticalHit)
            {
                SetStatChanger(STAT_ATK, 12);
                BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
            }
            else
            {
                SetStatChanger(STAT_ATK, 1);
                BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            }
            return TRUE;

        case ABILITY_TIPPING_POINT:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (CompareStat(battler, STAT_SPATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

            if (gIsCriticalHit)
            {
                SetStatChanger(STAT_SPATK, 12);
                BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
            }
            else
            {
                SetStatChanger(STAT_SPATK, 1);
                BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            }
            return TRUE;
        
        case ABILITY_BERSERK:
        case ABILITY_BERSERKER_RAGE:
        case ABILITY_UNLOCKED_POTENTIAL:
            if (!CheckHalfHpAbility(battler, attacker)) break;
            if (GetAbilityState(battler, ability)) break;
            if (CompareStat(battler, STAT_SPATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetAbilityState(battler, ability, TRUE);
            SetStatChanger(STAT_SPATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        
        case ABILITY_ANGER_SHELL:
            if (!CheckHalfHpAbility(battler, attacker)) break;
            if (GetAbilityState(battler, ability)) break;

            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_AngerShell);
            return TRUE;
        
        case ABILITY_NO_TURNING_BACK:
            if (!CheckHalfHpAbility(battler, attacker)) break;
            if (GetAbilityState(battler, ability)) break;

            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_NoTurningBack);
            return TRUE;
        
        case ABILITY_ITCHY_DEFENSE:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (gBattleMons[attacker].status2 & STATUS2_WRAPPED) break;

            gBattleMons[attacker].status2 |= STATUS2_WRAPPED;
            if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_GRIP_CLAW)
                gVolatileStructs[attacker].wrapTurns = 7;
            else
                gVolatileStructs[attacker].wrapTurns = (Random() % 2) + 4;
            
            gBattleStruct->wrappedMove[attacker] = MOVE_INFESTATION;
            gBattleStruct->wrappedBy[attacker] = battler;
                
            BattleScriptCall(BattleScript_AttackerBecameInfested);
            return TRUE;
        
        case ABILITY_LOOSE_QUILLS:
        case ABILITY_SCRAPYARD:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (gSideTimers[GetBattlerSide(attacker)].spikesAmount >= 3) break;

            BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
            return TRUE;

        case ABILITY_TOXIC_DEBRIS:
            if (!ShouldApplyOnHitAffect(battler)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (gSideTimers[GetBattlerSide(attacker)].toxicSpikesAmount >= 2) break;

            BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
            return TRUE;
        
        case ABILITY_VOODOO_POWER:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IS_MOVE_SPECIAL(move)) break;
            if (!CanBleed(attacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED)
            return TRUE;
        
        case ABILITY_SEED_SOWER:
            if (!ShouldApplyOnHitAffect(battler)) break;
            
            BattleScriptCall(BattleScript_SeedSower);
            return TRUE;
        
        case ABILITY_SUPERSWEET_SYRUP:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (gStatuses3[attacker] & STATUS3_EMBARGO) break;
            if (!gBattleMons[attacker].item) break;
            
            gBattleScripting.abilityPopupOverwrite = ABILITY_SUPERSWEET_SYRUP;
            gVolatileStructs[attacker].embargoTimer = 2;
            gStatuses3[attacker] |= STATUS3_EMBARGO;
            gLastUsedItem = gBattleMons[attacker].item;
            BattleScriptCall(BattleScript_AnnounceAttackerItemDisabled);
            return TRUE;
        
        case ABILITY_CUTE_CHARM:
        case ABILITY_PRIM_AND_PROPER:
        case ABILITY_PURE_LOVE:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!IsMoveMakingContact(move, attacker)) break;
            if (!CanInfatuate(battler, attacker)) break;
            if (Random() % 100 >= 30) break;

            gBattleMons[attacker].status2 |= STATUS2_INFATUATED_WITH(battler);
            BattleScriptCall(BattleScript_CuteCharmActivates);
            return TRUE;
        
        case ABILITY_GOOEY:
        case ABILITY_TANGLING_HAIR:
        case ABILITY_SUPER_HOT_GOO:
            if (!ShouldApplyOnHitAffect(attacker)) break;
            if (!StatLowerableOrMirrorArmor(attacker, STAT_SPEED)) break;

            gBattleScripting.moveEffect = MOVE_EFFECT_SPD_MINUS_1;
            BattleScriptCall(BattleScript_GooeyActivates);
            gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
            return TRUE;
    }

    return FALSE;
}

int HandleAttackerOrDefenderAbility(int ability, int battler, int opponent, int move)
{
    int effectTargetFlag = opponent == gBattlerAttacker ? MOVE_EFFECT_AFFECTS_USER : 0;
    int moveType;

    GET_MOVE_TYPE(move, moveType)

    switch (ability)
    {
        case ABILITY_BLOOD_STAIN:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (BATTLER_HAS_ABILITY(opponent, ABILITY_BLOOD_STAIN)) break;
            if (IsPersistentOrUnsuppressableAbility(GetBattlerAbility(opponent))) break;
            if (DoesBattlerHaveAbilityShield(opponent)) break;

            UpdateAbilityStateIndicesForNewAbility(opponent, ability);
            gBattleMons[opponent].ability = ability;
            gStackBattler1 = opponent;
            BattleScriptCall(BattleScript_BloodStainActivates);
            return TRUE;
        
        case ABILITY_SOUL_LINKER:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!IsBattlerAlive(battler)) break;
            if (BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER)) break;
            if (move == MOVE_PAIN_SPLIT) break;

            BattleScriptCall(BattleScript_AttackerSoulLinker);
            return TRUE;
        
        case ABILITY_DAMP:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (IS_BATTLER_OF_TYPE(opponent, TYPE_WATER)) break;

            gBattleMons[opponent].type1 = TYPE_WATER;
            gBattleMons[opponent].type2 = TYPE_WATER;
            gBattleMons[opponent].type3 = TYPE_MYSTERY;
            PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_WATER);
            BattleScriptCall(opponent == gBattlerAttacker ? BattleScript_AttackerBecameTheTypeFull : BattleScript_DefenderBecameTheTypeFull);
            return TRUE;
        
        case ABILITY_WHITE_NOISE:
        case ABILITY_STATIC:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!CanBeParalyzed(battler, opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_PARALYSIS | effectTargetFlag)
            return TRUE;
        
        case ABILITY_SUPER_HOT_GOO:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (!CanBeBurned(opponent)) break;
            if (Random() % 100 >= 30) break;

            gBattleScripting.moveEffect = MOVE_EFFECT_BURN | effectTargetFlag;
            gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
            SetMoveEffect(FALSE, FALSE);

            if (opponent != gBattlerAttacker || !HandleDefenderAbilityAs(ABILITY_SUPER_HOT_GOO, battler, opponent, move, moveType))
                BattleScriptCall(BattleScript_AbilityPopUp);
            
            return TRUE;

        case ABILITY_FLAME_BODY:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!CanBeBurned(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_BURN | effectTargetFlag)
            return TRUE;
        
        case ABILITY_FRAGRANT_DAZE:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!CanBeConfused(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_CONFUSION | effectTargetFlag)
            return TRUE;
        
        case ABILITY_POISON_POINT:
        case ABILITY_POISON_TOUCH:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!CanBePoisoned(battler, opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_POISON | effectTargetFlag)
            return TRUE;

        case ABILITY_CRYO_PROFICIENCY:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (!CanGetFrostbite(opponent)) break;
            if (Random() % 100 >= 30) break;

            gBattleScripting.moveEffect = MOVE_EFFECT_FROSTBITE | effectTargetFlag;
            gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
            SetMoveEffect(FALSE, FALSE);

            if (opponent != gBattlerAttacker || !HandleDefenderAbilityAs(ABILITY_CRYO_PROFICIENCY, battler, opponent, move, moveType))
                BattleScriptCall(BattleScript_AbilityPopUp);
            
            return TRUE;

        case ABILITY_FREEZING_POINT:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!CanGetFrostbite(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_FROSTBITE | effectTargetFlag)
            return TRUE;
        
        case ABILITY_SPIKE_ARMOR:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!CanBleed(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (Random() % 100 >= 30) break;

            ABILITY_STATUS_EFFECT(MOVE_EFFECT_BLEED | effectTargetFlag)
            return TRUE;
        
        case ABILITY_MENACING_SITUATION:
            if (!ShouldApplyOnHitAffect(opponent)) break;
            if (!IsMoveMakingContact(move, gBattlerAttacker)) break;
            if (gVolatileStructs[opponent].fear) break;
            if (Random() % 100 >= 30) break;

            gStackBattler1 = battler;
            gStackBattler2 = opponent;
            BattleScriptCall(BattleScript_AbilitySetFear);
            return TRUE;
        
        case ABILITY_ENTRANCE:
            {
            int flag = GetAbilityState(battler, ability);
            int any = FALSE;
            int realAttacker = gBattlerAttacker;
            int realTarget = gBattlerTarget;
            gBattlerAttacker = battler;
            SetAbilityState(battler, ability, 0);

            for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
            {
                if (!(flag & (1 << gBattlerTarget))) continue;
                if (!IsBattlerAlive(gBattlerTarget)) continue;
                if (!CanInfatuate(battler, gBattlerTarget)) continue;

                gBattleScripting.moveEffect = MOVE_EFFECT_ATTRACT;
                gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
                SetMoveEffect(FALSE, FALSE);
                any = TRUE;
            }
            gBattlerAttacker = realAttacker;
            gBattlerTarget = realTarget;
            if (any)
            {
                BattleScriptCall(BattleScript_AbilityPopUp);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_POISON_PUPPETEER:
            {
            int flag = GetAbilityState(battler, ability);
            int any = FALSE;
            int realAttacker = gBattlerAttacker;
            int realTarget = gBattlerTarget;
            gBattlerAttacker = battler;
            SetAbilityState(battler, ability, 0);

            for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
            {
                if (!(flag & (1 << gBattlerTarget))) continue;
                if (!IsBattlerAlive(gBattlerTarget)) continue;
                if (!CanBeConfused(gBattlerTarget)) continue;

                gBattleScripting.moveEffect = MOVE_EFFECT_CONFUSION;
                gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
                SetMoveEffect(FALSE, FALSE);
                any = TRUE;
            }
            gBattlerAttacker = realAttacker;
            gBattlerTarget = realTarget;
            if (any)
            {
                BattleScriptCall(BattleScript_AbilityPopUp);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_BLOODLUST:
        case ABILITY_BLOOD_BATH:
            {
            int flag = GetAbilityState(battler, ability);
            int any = FALSE;
            int realAttacker = gBattlerAttacker;
            int realTarget = gBattlerTarget;
            gBattlerAttacker = battler;
            SetAbilityState(battler, ability, 0);

            for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++)
            {
                if (!(flag & (1 << gBattlerTarget))) continue;
                if (!IsBattlerAlive(gBattlerTarget)) continue;
                if (!gVolatileStructs[gBattlerTarget].fear) continue;

                gBattleScripting.moveEffect = MOVE_EFFECT_FEAR;
                gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
                SetMoveEffect(FALSE, FALSE);
                any = TRUE;
            }
            gBattlerAttacker = realAttacker;
            gBattlerTarget = realTarget;
            if (any)
            {
                BattleScriptCall(BattleScript_AbilityPopUp);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_NONE:
    }

    return FALSE;
}

int HandleMiscAbilityMoveEffects(int battler, int opponent, int move)
{
    int effect = 0;

    if (gVolatileStructs[battler].parasiticSpores
        && ShouldApplyOnHitAffect(opponent)
        && IsMoveMakingContact(move, gBattlerAttacker)
        && !gVolatileStructs[opponent].parasiticSpores)
    {
        gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_PARASITIC_SPORES;
        gVolatileStructs[opponent].parasiticSpores = TRUE;
        gStackBattler1 = battler;
        gStackBattler2 = opponent;
        BattleScriptCall(BattleScript_ParasiticSporesSpread);
        if (BATTLER_HAS_ABILITY(battler, ABILITY_PARASITIC_SPORES)) BattleScriptCall(BattleScript_AbilityPopUp);
        effect++;
    }

    {
        u8 commander = IsAbilityOnSide(battler, ABILITY_COMMANDER);
        if (commander-- && IsBattlerAlive(commander) && GetAbilityState(commander, ABILITY_COMMANDER) == COMMANDER_NEEDS_CANCELLING)
        {
            SetAbilityState(commander, ABILITY_COMMANDER, COMMANDER_NOT_ACTIVE);
            gStatuses3[commander] &= ~STATUS3_SEMI_INVULNERABLE;
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_COMMANDER;
            gStackBattler1 = commander;
            BattleScriptCall(BattleScript_CommanderEnds);
            effect++;
        }
    }
}

static int HandleSwitchInAbilityAs(int ability, int battler);

int HandleSwitchInAbility(int abilityNumber, int battler)
{
    u16 abilities[4] = {0};
    int ability;

    if (abilityNumber > NUM_ABILITY_SLOTS)
    {
        int effect = 0;
        {
        int commander = IsAbilityOnSide(battler, ABILITY_COMMANDER);
        if (commander--
            && !GetAbilityState(commander, ABILITY_COMMANDER)
            && IsBattlerAlive(BATTLE_PARTNER(commander))
            && GET_BASE_SPECIES_ID(gBattleMons[commander].species) == SPECIES_TATSUGIRI
            && GET_BASE_SPECIES_ID(gBattleMons[BATTLE_PARTNER(commander)].species) == SPECIES_DONDOZO)
        {
            int partner = BATTLE_PARTNER(commander);
            gStatuses3[commander] |= STATUS3_SEMI_INVULNERABLE;
            gStatuses4[partner] |= STATUS4_COMMANDED;
            SetAbilityState(commander, ABILITY_COMMANDER, COMMANDER_ACTIVE);
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_COMMANDER;
            gStackBattler1 = commander;
            gStackBattler2 = partner;
            BattleScriptPushCursorAndCallback(BattleScript_CommanderActivates);
            effect++;
        }
        }
        
        // Dragon Monotype
        if(getMonotypeChampType() == TYPE_DRAGON && GetBattlerSide(battler) != B_SIDE_PLAYER) {
            gBattleScripting.abilityPopupOverwrite = gLastUsedAbility = ABILITY_FEARMONGER;
            effect += UseIntimidateClone(battler, ABILITY_FEARMONGER);
        }

        // Totem Boost
        if (FlagGet(FLAG_TOTEM_BATTLE) 
            && GetBattlerSide(battler) != B_SIDE_PLAYER 
            && gBattleResults.battleTurnCounter == 0
            && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
            && !gBattleMons[battler].wasalreadytotemboosted) {
            FlagSet(FLAG_SMART_AI);
            
            gBattlerAttacker = battler;
            gBattleMons[battler].wasalreadytotemboosted = TRUE;

            gBattleMons[battler].statStages[STAT_ATK]     = gBattleMons[battler].statStages[STAT_ATK]     + VarGet(VAR_TOTEM_POKEMON_ATK_BOOST);
            gBattleMons[battler].statStages[STAT_DEF]     = gBattleMons[battler].statStages[STAT_DEF]     + VarGet(VAR_TOTEM_POKEMON_DEF_BOOST);
            gBattleMons[battler].statStages[STAT_SPATK]   = gBattleMons[battler].statStages[STAT_SPATK]   + VarGet(VAR_TOTEM_POKEMON_SP_ATK_BOOST);
            gBattleMons[battler].statStages[STAT_SPDEF]   = gBattleMons[battler].statStages[STAT_SPDEF]   + VarGet(VAR_TOTEM_POKEMON_SP_DEF_BOOST);
            gBattleMons[battler].statStages[STAT_SPEED]   = gBattleMons[battler].statStages[STAT_SPEED]   + VarGet(VAR_TOTEM_POKEMON_SPEED_BOOST);
            gBattleMons[battler].statStages[STAT_ACC]     = gBattleMons[battler].statStages[STAT_ACC]     + VarGet(VAR_TOTEM_POKEMON_ACCURACY_BOOST);
            gBattleMons[battler].statStages[STAT_EVASION] = gBattleMons[battler].statStages[STAT_EVASION] + VarGet(VAR_TOTEM_POKEMON_EVASION_BOOST);

            SET_STATCHANGER(STAT_ATK, 1, FALSE); //Just for the animation
            switch(VarGet(VAR_TOTEM_MESSAGE)) {
                case TOTEM_FIGHT_HAXORUS:
                    BattleScriptPushCursorAndCallback(BattleScript_HaxorusTotemBoostActivated);
                break;
                default:
                    BattleScriptPushCursorAndCallback(BattleScript_WildTotemBoostActivated);
                break;
            }
            effect++;

            VarSet(VAR_TOTEM_POKEMON_ATK_BOOST,      0);
            VarSet(VAR_TOTEM_POKEMON_DEF_BOOST,      0);
            VarSet(VAR_TOTEM_POKEMON_SP_ATK_BOOST,   0);
            VarSet(VAR_TOTEM_POKEMON_SP_DEF_BOOST,   0);
            VarSet(VAR_TOTEM_POKEMON_SPEED_BOOST,    0);
            VarSet(VAR_TOTEM_POKEMON_ACCURACY_BOOST, 0);
            VarSet(VAR_TOTEM_POKEMON_EVASION_BOOST,  0);
            VarSet(VAR_TOTEM_MESSAGE,                0);
        }

        return effect;
    }

    GET_ALL_BATTLER_ABILITIES(abilities, battler, battler);
    ability = gBattleScripting.abilityPopupOverwrite = abilities[abilityNumber];
    gBattlerAbility = battler;

    switch (ability)
    {
        case ABILITY_TRACE:
            break;
        
        default:
            if (!CheckAndSetSwitchInAbility(battler, ability)) return FALSE;
    }

    #define ANNOUNCE_SIMPLE_ABILITY(abilityToAnnounce, announceMessage) \
        case abilityToAnnounce: \
            gBattleCommunication[MULTISTRING_CHOOSER] = announceMessage; \
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg); \
            return TRUE;
        
    return HandleSwitchInAbilityAs(ability, battler);
}

int HandleSwitchInAbilityAs(int ability, int battler)
{
    int i;

    switch (ability)
    {
        case ABILITY_ZERO_TO_HERO:
            if (gBattleMons[battler].species != SPECIES_PALAFIN) break;
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_PALAFIN_HERO);
            gBattleMons[battler].species = SPECIES_PALAFIN_HERO;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            return TRUE;
            
        case ABILITY_PARASITIC_SPORES:
            if (gVolatileStructs[battler].parasiticSpores) break;
            
            gVolatileStructs[battler].parasiticSpores = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_PARASITIC_SPORES;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;

        case ABILITY_ANTICIPATION:
            {
            int side = GetBattlerSide(battler);
            int any = FALSE;
            int j;

            for (i = 0; i < gBattlersCount; i++)
            {
                if (IsBattlerAlive(i) && side != GetBattlerSide(i))
                {
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        int move = gBattleMons[i].moves[j];
                        int moveType = gBattleMoves[move].type;
                        if (CalcTypeEffectivenessMultiplier(move, moveType, i, battler, FALSE) >= UQ_4_12(2.0))
                        {
                            any = TRUE;
                            break;
                        }
                    }
                }
            }

            if (any)
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_ANTICIPATION;
                BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_HOSPITALITY:
            gBattlerTarget = BATTLE_PARTNER(battler);
            if (!IsBattlerAlive(gBattlerTarget)) break;
            if (BATTLER_MAX_HP(gBattlerTarget)) break;

            gBattleMoveDamage = -gBattleMons[gBattlerTarget].maxHP / 4;
            if (!gBattleMoveDamage) gBattleMoveDamage = -1;
            BattleScriptPushCursorAndCallback(BattleScript_Hospitality);
            return TRUE;

        ANNOUNCE_SIMPLE_ABILITY(ABILITY_DARK_AURA, B_MSG_SWITCHIN_DARKAURA)

        case ABILITY_PIXIE_POWER:
        ANNOUNCE_SIMPLE_ABILITY(ABILITY_FAIRY_AURA, B_MSG_SWITCHIN_FAIRYAURA)
        
        ANNOUNCE_SIMPLE_ABILITY(ABILITY_AURA_BREAK, B_MSG_SWITCHIN_AURABREAK)

        ANNOUNCE_SIMPLE_ABILITY(ABILITY_COMATOSE, B_MSG_SWITCHIN_COMATOSE)
        
        case ABILITY_AS_ONE_SHADOW_RIDER:
        ANNOUNCE_SIMPLE_ABILITY(ABILITY_AS_ONE_ICE_RIDER, B_MSG_SWITCHIN_ASONE)

        ANNOUNCE_SIMPLE_ABILITY(ABILITY_CROWNED_KING, B_MSG_SWITCHIN_CROWNEDKING)

        case ABILITY_ICE_FACE:
            if (gBattleMons[battler].species != SPECIES_EISCUE_NOICE_FACE) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) break;
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_EISCUE);
            gBattleMons[battler].species = SPECIES_EISCUE;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            return TRUE;
        
        case ABILITY_DISGUISE:
        case ABILITY_PATCHWORK:
            if (gBattleMons[battler].species != SPECIES_MIMIKYU_BUSTED && gBattleMons[battler].species != SPECIES_MIMIKYU_RAYQUAZA_BUSTED) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) break;
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;

            {
            int newSpecies = gBattleMons[battler].species == SPECIES_MIMIKYU_BUSTED ? SPECIES_MIMIKYU : SPECIES_MIMIKYU_RAYQUAZA;
            UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
            gBattleMons[battler].species = newSpecies;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            }
            return TRUE;
        
        case ABILITY_FLARE_BOOST:
            if (!CanBeBurned(battler)) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) break;

            gBattleMons[battler].status1 |= STATUS1_BURN;
            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
            MarkBattlerForControllerExec(battler);
            BattleScriptPushCursorAndCallback(BattleScript_FlareBoostEnd3);
            return TRUE;
        
        case ABILITY_IMPOSTER:
            gBattlerTarget = BATTLE_OPPOSITE(battler);
            if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
            if (!IsBattlerAlive(gBattlerTarget)) break;
            if (gBattleMons[gBattlerTarget].status2 & (STATUS2_TRANSFORMED | STATUS2_SUBSTITUTE)) break;
            if (gBattleMons[battler].status2 & STATUS2_TRANSFORMED) break;
            if (gBattleStruct->illusion[gBattlerTarget].on) break;
            if (gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_ImposterActivates);
            return TRUE;
        
        case ABILITY_TRACE:
            if (gTurnStructs[battler].traced) break;

            gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_TRACED;
            gTurnStructs[battler].traced = TRUE;
            break;
        
        case ABILITY_SCHOOLING:
            if (gBattleMons[battler].level < 20) break;
        case ABILITY_SHIELDS_DOWN:
        case ABILITY_FORECAST:
        case ABILITY_FLOWER_GIFT:
        case ABILITY_ZEN_MODE:
            if (!ShouldChangeFormHpBased(battler)) break;
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            return TRUE;
        
        case ABILITY_MIMICRY:
            if (!IsBattlerAlive(battler)) break;
            if (!(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)) break;

            TryToApplyMimicry(battler, FALSE);
            return TRUE;

        case ABILITY_CURIOUS_MEDICINE:
            if (!IsDoubleBattle()) break;
            if (!IsBattlerAlive(BATTLE_PARTNER(battler))) break;
            if (!TryResetBattlerStatChanges(BATTLE_PARTNER(battler), RESET_ALL_STATS)) break;
            
            gEffectBattler = BATTLE_PARTNER(battler);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CURIOUS_MEDICINE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_FRISK:
            BattleScriptPushCursorAndCallback(BattleScript_FriskActivates);
            return TRUE;
        
        case ABILITY_SEABORNE:
        case ABILITY_DRIZZLE:
            if (TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN, TRUE))
            {
                BattleScriptPushCursorAndCallback(BattleScript_DrizzleActivates);
                return TRUE;
            }
            else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
            {
                BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                return TRUE;
            }
            break;
        
        case ABILITY_SAND_STREAM:
        case ABILITY_DESERT_SPIRIT:
            if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, TRUE))
            {
                BattleScriptPushCursorAndCallback(BattleScript_SandstreamActivates);
                return TRUE;
            }
            else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
            {
                BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                return TRUE;
            }
            break;
        
        case ABILITY_DROUGHT:
        case ABILITY_ORICHALCUM_PULSE:
            if (TryChangeBattleWeather(battler, ENUM_WEATHER_SUN, TRUE))
            {
                BattleScriptPushCursorAndCallback(BattleScript_DroughtActivates);
                return TRUE;
            }
            else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
            {
                BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                return TRUE;
            }
            break;
            
        case ABILITY_LOW_VISIBILITY:
            if (TryChangeBattleWeather(battler, ENUM_WEATHER_FOG, TRUE))
            {
                BattleScriptPushCursorAndCallback(BattleScript_BadOmensActivates);
                return TRUE;
            }
            else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
            {
                BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                return TRUE;
            }
            break;
            
        case ABILITY_SNOWY_WRATH:
        case ABILITY_SNOW_WARNING:
            if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE))
            {
                BattleScriptPushCursorAndCallback(BattleScript_SnowWarningActivates);
                return TRUE;
            }
            else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
            {
                BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
                return TRUE;
            }
            break;
        
        case ABILITY_DESOLATE_LAND:
            if (!TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
            return TRUE;
        
        case ABILITY_PRIMORDIAL_SEA:
            if (!TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
            return TRUE;
        
        case ABILITY_DELTA_STREAM:
            if (!TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
            return TRUE;
        
        case ABILITY_ELECTRIC_SURGE:
        case ABILITY_HADRON_ENGINE:
            if (!TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer)) break;
            
            for (i = 0; i < gBattlersCount; i++)
            {
                DisableSwitchInAbility(i, ABILITY_GENERATOR);
                DisableSwitchInAbility(i, ABILITY_ENERGIZED);
            }
            BattleScriptPushCursorAndCallback(BattleScript_ElectricSurgeActivates);
            return TRUE;
            
        case ABILITY_GRASSY_SURGE:
            if (!TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_GrassySurgeActivates);
            return TRUE;
            
        case ABILITY_MISTY_SURGE:
            if (!TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_MistySurgeActivates);
            return TRUE;
            
        case ABILITY_PSYCHIC_SURGE:
            if (!TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_PsychicSurgeActivates);
            return TRUE;
        
        case ABILITY_ENERGIZED:
        case ABILITY_GENERATOR:
            if (gStatuses3[battler] & STATUS3_CHARGED_UP) break;

            gStackBattler1 = gBattlerAttacker;
            if (TERRAIN_HAS_EFFECT && gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
            {
                BattleScriptPushCursorAndCallback(BattleScript_GeneratorActivates);
                return TRUE;
            }
            else if (!GetSingleUseAbilityCounter(battler, ABILITY_GENERATOR))
            {
                SetSingleUseAbilityCounter(battler, ABILITY_GENERATOR, TRUE);
                BattleScriptPushCursorAndCallback(BattleScript_GeneratorActivates);
                return TRUE;
            }
            break;
        
        case ABILITY_SCREEN_CLEANER:
            if (!TryRemoveScreens(battler)) break;

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_SCREENCLEANER;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        ANNOUNCE_SIMPLE_ABILITY(ABILITY_UNNERVE, B_MSG_SWITCHIN_UNNERVE)

        case ABILITY_WIND_RIDER:
            if (!(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND)) break;

            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityHighestAttackingStatRaiseOnSwitchIn);
            return TRUE;
        
        case ABILITY_DOWNLOAD:
            gBattlerTarget = BATTLE_OPPOSITE(battler);
            if (!IsBattlerAlive(battler)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
            if (!IsBattlerAlive(battler)) break;

            {
            int stat = GetHighestDefendingStatId(battler, TRUE) == STAT_DEF ? STAT_SPATK : STAT_ATK;
            if (!ChangeStatBuffs(battler, StatBuffValue(1), stat, MOVE_EFFECT_AFFECTS_USER, NULL)) break;
            SetStatChanger(stat, 1);
            PREPARE_STAT_BUFFER(gBattleTextBuff1, stat);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            }
            return TRUE;
        
        case ABILITY_LETS_ROLL:
            if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_DEF, 1);
            gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
            return TRUE;
        
        case ABILITY_SLOW_START:
            gVolatileStructs[battler].slowStartTimer = 5;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_SLOWSTART;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_LETHARGY:
            TryResetBattlerStatChanges(battler, RESET_ALL_STATS);
            gVolatileStructs[battler].slowStartTimer = 5;
            BattleScriptPushCursorAndCallback(BattleScript_LethargyEnters);
            return TRUE;
        
        case ABILITY_SEA_GUARDIAN:
            if (!IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) break;

            {
            int stat = GetHighestStatId(battler, TRUE);
            if (!ChangeStatBuffs(battler, StatBuffValue(1), stat, MOVE_EFFECT_AFFECTS_USER, NULL)) break;
            SetStatChanger(stat, 1);
            PREPARE_STAT_BUFFER(gBattleTextBuff1, stat);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            }
            return TRUE;
        
        case ABILITY_SUN_WORSHIP:
            if (!IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) break;

            {
            int stat = GetHighestStatId(battler, TRUE);
            if (!ChangeStatBuffs(battler, StatBuffValue(1), stat, MOVE_EFFECT_AFFECTS_USER, NULL)) break;
            SetStatChanger(stat, 1);
            PREPARE_STAT_BUFFER(gBattleTextBuff1, stat);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            }
            return TRUE;
        
        case ABILITY_GREATER_SPIRIT:
            if (!IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) break;

            {
            int stat = GetHighestStatId(battler, TRUE);
            if (!ChangeStatBuffs(battler, StatBuffValue(1), stat, MOVE_EFFECT_AFFECTS_USER, NULL)) break;
            SetStatChanger(stat, 1);
            PREPARE_STAT_BUFFER(gBattleTextBuff1, stat);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            }
            return TRUE;
        
        case ABILITY_TRASH_HEAP:
        case ABILITY_TOXIC_SPILL:
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
            return TRUE;
        
        case ABILITY_CHEATING_DEATH:
            {
            int uses = 2 - GetSingleUseAbilityCounter(battler, ABILITY_CHEATING_DEATH);
            if (!uses) break;

            if (uses == 1)
                BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
            else {
                ConvertIntToDecimalStringN(gBattleTextBuff4, uses, STR_CONV_MODE_LEFT_ALIGN, 2);
                BattleScriptPushCursorAndCallback(BattleScript_BattlerHasNoDamageHits);
            }
            }
            return TRUE;
        
        case ABILITY_GALLANTRY:
            if (GetSingleUseAbilityCounter(battler, ABILITY_GALLANTRY) >= 1) break;

            BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
            return TRUE;
        
        case ABILITY_COWARD:
            if (GetSingleUseAbilityCounter(battler, ABILITY_COWARD)) break;
            
            SetSingleUseAbilityCounter(battler, ABILITY_COWARD, TRUE);
            gRoundStructs[battler].protectedThisTurn = TRUE;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
            return TRUE;
        
        case ABILITY_ATLAS:
            if (gFieldStatuses & STATUS_FIELD_GRAVITY) break;
            
            gFieldTimers.started.gravity = TRUE;
            gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
            gFieldStatuses |= STATUS_FIELD_GRAVITY;
            BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
            return TRUE;
        
        case ABILITY_GRAVITY_WELL:
            if (gFieldStatuses & STATUS_FIELD_GRAVITY) break;
            
            gFieldTimers.started.gravity = TRUE;
            gFieldTimers.gravityTimer = GRAVITY_DURATION;
            gFieldStatuses |= STATUS_FIELD_GRAVITY;
            BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
            return TRUE;
        
        case ABILITY_PICKUP:
            if (!(gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_HAZARDS_ANY) && !gSideTimers[GetBattlerSide(gActiveBattler)].hotCoals) break;
            
            gSideStatuses[GetBattlerSide(gActiveBattler)] &= ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES | SIDE_STATUS_STICKY_WEB);
            gSideTimers[GetBattlerSide(gActiveBattler)].hotCoals = FALSE;
            BattleScriptPushCursorAndCallback(BattleScript_PickUpActivate);
            return TRUE;

        case ABILITY_FOREWARN:
            gBattlerTarget = BATTLE_OPPOSITE(battler);
            if (!IsBattlerAlive(gBattlerTarget) || gWishFutureKnock.futureSightCounter[gBattlerTarget]) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
            if (!IsBattlerAlive(gBattlerTarget)) break;
            if (gWishFutureKnock.futureSightCounter[gBattlerTarget]) break;

            gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] |= SIDE_STATUS_FUTUREATTACK;
            gWishFutureKnock.futureSightMove[gBattlerTarget] = MOVE_FUTURE_SIGHT;
            gWishFutureKnock.futureSightPower[gBattlerTarget] = 50;
            gWishFutureKnock.futureSightAttacker[gBattlerTarget] = battler;
            gWishFutureKnock.futureSightCounter[gBattlerTarget] = 3;

            BattleScriptPushCursorAndCallback(BattleScript_ForewarnReworkActivates);
            return TRUE;
        
        case ABILITY_LOW_BLOW:
            UseEntryMove(battler, ability, MOVE_FEINT_ATTACK, 0);
            break;
        
        case ABILITY_CHEAP_TACTICS:
            UseEntryMove(battler, ability, MOVE_SCRATCH, 0);
            break;
        
        case ABILITY_DUST_CLOUD:
            UseEntryMove(battler, ability, MOVE_SAND_ATTACK, 0);
            break;
        
        case ABILITY_TRICKSTER:
            UseEntryMove(battler, ability, MOVE_DISABLE, 0);
            break;
        
        case ABILITY_SUPPRESS:
            UseEntryMove(battler, ability, MOVE_TORMENT, 0);
            break;
        
        case ABILITY_DOOMBRINGER:
            UseEntryMove(battler, ability, MOVE_DOOM_DESIRE, 0);
            break;
        
        case ABILITY_CHANGE_OF_HEART:
            UseEntryMove(battler, ability, MOVE_HEART_SWAP, 0);
            break;
        
        case ABILITY_TELEKINETIC:
            UseEntryMove(battler, ability, MOVE_TELEKINESIS, 0);
            break;
        
        case ABILITY_POWDER_BURST:
            UseEntryMove(battler, ability, MOVE_POWDER, 0);
            break;
        
        case ABILITY_MONSTER_MASH:
            UseEntryMove(battler, ability, MOVE_TRICK_OR_TREAT, 0);
            break;
        
        case ABILITY_PHANTOM_THIEF:
            UseEntryMove(battler, ability, MOVE_SPECTRAL_THIEF, 40);
            break;
        
        case ABILITY_WEB_SPINNER:
            UseEntryMove(battler, ability, MOVE_STRING_SHOT, 0);
            break;
        
        case ABILITY_DRACO_MORALE:
            UseEntryMove(battler, ability, MOVE_DRAGON_CHEER, 0);
            break;
        
        case ABILITY_DREAM_WHIMSY:
            UseEntryMove(battler, ability, MOVE_YAWN, 0);
            break;
        
        case ABILITY_WILDFIRE:
            UseEntryMove(battler, ability, MOVE_FIRE_SPIN, 0);
            break;
        
        case ABILITY_JUMP_SCARE:
            UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
            break;
        
        case ABILITY_TAR_TOSS:
            UseEntryMove(battler, ability, MOVE_TAR_SHOT, 0);
            break;
        
        case ABILITY_WISHMAKER:
            {
            int counter = GetSingleUseAbilityCounter(battler, ABILITY_WISHMAKER);
            if (counter >= 3) break;
            if (!UseEntryMove(battler, ABILITY_WISHMAKER, MOVE_WISH, 0)) break;
            
            SetSingleUseAbilityCounter(battler, ABILITY_WISHMAKER, counter + 1);
            return TRUE;
            }
        
        case ABILITY_SOOTHING_AROMA:
            {
            int anyStatus = FALSE;
            struct Pokemon *party;

            if (GetBattlerSide(battler) == B_SIDE_PLAYER)
                party = gPlayerParty;
            else
                party = gEnemyParty;
                
            for (i = 0; i < PARTY_SIZE; i++) {
                u32 status1 = GetMonData(&party[i], MON_DATA_STATUS);
                if (status1 & STATUS1_ANY) {
                    anyStatus = TRUE;
                    break;
                }
            }

            if (anyStatus) {
                BattleScriptPushCursorAndCallback(BattleScript_EffectSoothingAroma);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_BUTTER_UP:
            {
            int healPartner = FALSE;
            gBattlerTarget = BATTLE_PARTNER(battler);
            if (IsBattlerAlive(gBattlerTarget) && !BATTLER_MAX_HP(gBattlerTarget))
            {
                healPartner = TRUE;
                gBattleMoveDamage = -gBattleMons[gBattlerTarget].maxHP / 4;
                if (!gBattleMoveDamage) gBattleMoveDamage = -1;
                BattleScriptPushCursorAndCallback(BattleScript_Hospitality);
            }

            if (HandleSwitchInAbilityAs(ABILITY_SOOTHING_AROMA, battler)) return TRUE;
            else if (healPartner)
            {
                BattleScriptPushCursorAndCallback(BattleScript_AbilityPopUpEnd3);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_FEARMONGER:
        case ABILITY_INTIMIDATE:
        case ABILITY_SCARE:
        case ABILITY_YUKI_ONNA:
        case ABILITY_MONKEY_BUSINESS:
        case ABILITY_MALICIOUS:
        case ABILITY_TERRIFY:
            if (UseIntimidateClone(battler, ability)) return TRUE;
            break;
        
        case ABILITY_GLEAM_EYES:
            if (UseIntimidateClone(battler, ABILITY_GLEAM_EYES))
            {
                gBattleStruct->friskedAbility = TRUE;
            }
            else
            {
                BattleScriptPushCursorAndCallback(BattleScript_FriskActivates);
            }
            return TRUE;
        
        case ABILITY_MAJESTIC_MOTH:
            {
            int stat = GetHighestStatId(battler, TRUE);
            if (!ChangeStatBuffs(battler, StatBuffValue(1), stat, MOVE_EFFECT_AFFECTS_USER, NULL)) break;
            
            SetStatChanger(stat, 1);
            gBattlerAttacker = battler;
            PREPARE_STAT_BUFFER(gBattleTextBuff1, stat);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
            }
        
        case ABILITY_PURIFYING_WATERS:
        case ABILITY_WATER_VEIL:
            if (gStatuses3[battler] & STATUS3_AQUA_RING) break;

            gStatuses3[battler] |= STATUS3_AQUA_RING;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
            return TRUE;
        
        case ABILITY_COIL_UP:
        case ABILITY_SIDEWINDER:
            if (gStatuses4[battler] & STATUS4_COILED) break;

            gStatuses4[battler] |= STATUS4_COILED;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
            return TRUE;
        
        case ABILITY_AIR_BLOWER:
            if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND) break;
            {
            int side = GetBattlerSide(battler);
            gSideTimers[side].started.tailwind = TRUE;
            gSideStatuses[side] |= SIDE_STATUS_TAILWIND;
            gSideTimers[side].tailwindBattlerId = gBattlerAttacker;
            gSideTimers[side].tailwindTimer = TAILWIND_DURATION_SHORT;
            
            DisableSwitchInAbility(battler, ABILITY_WIND_RIDER);
            DisableSwitchInAbility(BATTLE_PARTNER(battler), ABILITY_WIND_RIDER);

            BattleScriptPushCursorAndCallback(BattleScript_AirBlowerActivated);
            }
            return TRUE;
        
        case ABILITY_PASTEL_VEIL:
            if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_SAFEGUARD) break;
            {
            int side = GetBattlerSide(battler);
            gSideTimers[side].started.safeguard = TRUE;
            gSideStatuses[side] |= SIDE_STATUS_SAFEGUARD;
            gSideTimers[side].safeguardBattlerId = gBattlerAttacker;
            gSideTimers[side].safeguardTimer = SCREEN_DURATION;
            BattleScriptPushCursorAndCallback(BattleScript_PastelVeilActivated);
            }
            return TRUE;
        
        case ABILITY_NORTH_WIND:
            if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_AURORA_VEIL) break;
            {
            int side = GetBattlerSide(battler);
            gSideTimers[side].started.auroraVeil = TRUE;
            gSideStatuses[side] |= SIDE_STATUS_AURORA_VEIL;
            if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_LIGHT_CLAY)
                gSideTimers[side].auroraVeilTimer = SCREEN_DURATION;
            else
                gSideTimers[side].auroraVeilTimer = SCREEN_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_NorthWindActivated);
            }
            return TRUE;
        
        case ABILITY_SPIDER_LAIR:
            if (gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB) break;
            {
            int side = BATTLE_OPPOSITE(battler);
            gSideTimers[side].started.spiderWeb = TRUE;
            gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
            gSideTimers[side].spiderWebTimer = 5;
            BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
            }
            return TRUE;
        
        case ABILITY_INTREPID_SWORD:
        case ABILITY_CROWNED_SWORD:
            if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_ATK, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            break;
        
        case ABILITY_CROWNED_SHIELD:
        case ABILITY_DAUNTLESS_SHIELD:
            if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_EQUAL)) break;

            SetStatChanger(STAT_DEF, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            break;
        
        case ABILITY_FUNERAL_PYRE:
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_FUNERAL_PYRE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_WHITE_SMOKE:
            if (gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer) break;
            {
            int side = GET_BATTLER_SIDE(battler);
            gSideTimers[side].smokescreenTimer = 5;
            gSideTimers[side].started.smokescreen = TRUE;
            gSideTimers[side].smokescreenBattler = battler;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_WHITE_SMOKE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);            
            }
            return TRUE;
        
        case ABILITY_HOT_COALS:
            if (gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals) break;

            gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_HOT_COALS;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_PRESSURE:
            {
            int loweredStats = 0;
            for (i = 0; i < gBattlersCount; i++)
            {
                if (!IsBattlerAlive(i)) continue;
                loweredStats |= TryResetBattlerStatChanges(i, i == battler ? RESET_STAT_DROPS : RESET_STAT_BUFFS);
            }

            if (loweredStats)
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_PRESSURE;
                BattleScriptPushCursorAndCallback(BattleScript_PressureRemoveStats);
            }
            else
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_PRESSURE;
                BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            }
            }
            return TRUE;
        
        case ABILITY_PETRIFY:
            {
            int loweredStats = 0;
            for (i = BATTLE_OPPOSITE(GET_BATTLER_SIDE(battler)); i < gBattlersCount; i += 2)
            {
                if (!IsBattlerAlive(i)) continue;
                loweredStats |= TryResetBattlerStatChanges(i, RESET_STAT_BUFFS);
            }

            if (loweredStats)
            {
                BattleScriptPushCursorAndCallback(BattleScript_PetrifyRemoveStats);
            }
            else
            {
                BattleScriptPushCursorAndCallback(BattleScript_Petrify);
            }
            }
            return TRUE;
        
        case ABILITY_AIR_LOCK:
        case ABILITY_CLUELESS:
        case ABILITY_CLOUD_NINE:
            BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
            return TRUE;
        
        case ABILITY_TWISTED_DIMENSION:
            if (gFieldStatuses & STATUS_FIELD_TRICK_ROOM) break;
            
            gFieldTimers.started.trickRoom = TRUE;
            gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
            gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
            return TRUE;
        
        case ABILITY_INVERSE_ROOM:
            if (gFieldStatuses & STATUS_FIELD_INVERSE_ROOM) break;
            
            gFieldTimers.started.inverseRoom = TRUE;
            gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
            gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
            return TRUE;
        
        case ABILITY_PERMANENCE:
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_PERMANENCE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_SALT_CIRCLE:
            {
            int anyBlocked = FALSE;
            gBattlerTarget = BATTLE_OPPOSITE(battler);

            if (IsBattlerAlive(gBattlerTarget) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION)) {
                gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
                gVolatileStructs[gBattlerTarget].battlerPreventingEscape = battler;
                anyBlocked = TRUE;
            }

            gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
            if (IsBattlerAlive(gBattlerTarget) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION)) {
                gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
                gVolatileStructs[gBattlerTarget].battlerPreventingEscape = battler;
                anyBlocked = TRUE;
            }

            if (anyBlocked) {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_SALT_CIRCLE;
                BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_SUPREME_OVERLORD:
            if (!gFaintedMonCount[GetBattlerSide(battler)]) break;

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_SUPREME_OVERLORD;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_BERSERK_DNA:
            if (CanBeConfused(battler))
            {
                gBattleMons[battler].status2 |= STATUS2_CONFUSION_TURN(3);
                BattleScriptPushCursorAndCallback(BattleScript_BerserkDNA);
            }
            else BattleScript_BerserkDNANoConfusion;
            return TRUE;
        
        case ABILITY_PROTOSYNTHESIS:
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))
            {
                struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_WEATHER_ACTIVE };
                SetAbilityStateAs(battler, ABILITY_PROTOSYNTHESIS, (union AbilityStates) { .paradoxBoost = boost });
                PREPARE_STAT_BUFFER(gBattleTextBuff1, boost.statId);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_WEATHER;
                BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
                return TRUE;
            }
            
            if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY)
            {
                struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_BOOSTER_ENERGY };
                SetAbilityStateAs(battler, ABILITY_PROTOSYNTHESIS, (union AbilityStates) { .paradoxBoost = boost });
                PREPARE_STAT_BUFFER(gBattleTextBuff1, boost.statId);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
                RemoveItem(battler);
                BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
                return TRUE;
            }
            break;
        
        case ABILITY_QUARK_DRIVE:
            if (TERRAIN_HAS_EFFECT && gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
            {
                struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_WEATHER_ACTIVE };
                SetAbilityStateAs(battler, ABILITY_QUARK_DRIVE, (union AbilityStates) { .paradoxBoost = boost });
                PREPARE_STAT_BUFFER(gBattleTextBuff1, boost.statId);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_TERRAIN;
                BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
                return TRUE;
            }

            if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY)
            {
                struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_BOOSTER_ENERGY };
                SetAbilityStateAs(battler, ABILITY_QUARK_DRIVE, (union AbilityStates) { .paradoxBoost = boost });
                PREPARE_STAT_BUFFER(gBattleTextBuff1, boost.statId);
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
                RemoveItem(battler);
                BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
                return TRUE;
            }
            break;
        
        case ABILITY_FURNACE:
            if (!(gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_STEALTH_ROCK)) break;
            if (gSideTimers[GetBattlerSide(gActiveBattler)].stealthRockType != TYPE_ROCK) break;
            if (!IsBattlerAlive(battler)) break;
            if (!ChangeStatBuffs(battler, StatBuffValue(2), STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL)) break;

            SetStatChanger(STAT_SPEED, 2);
            PREPARE_STAT_BUFFER(gBattleTextBuff1, STAT_SPEED);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        
        case ABILITY_COSTAR:
            if (!IsBattlerAlive(BATTLE_PARTNER(battler))) break;
            {
            int anyChanged = FALSE;
            for (i = STAT_ATK; i < NUM_BATTLE_STATS; i++)
            {
                if (gBattleMons[battler].statStages[i] != gBattleMons[BATTLE_PARTNER(battler)].statStages[i])
                {
                    gBattleMons[battler].statStages[i] = gBattleMons[BATTLE_PARTNER(battler)].statStages[i];
                    anyChanged = TRUE;
                }
            }

            if (anyChanged)
            {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COSTAR;
                BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_BLIND_RAGE:
        case ABILITY_MOLD_BREAKER:
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_MOLDBREAKER;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_WATCH_YOUR_STEP:
            {
            u8 targetSide = GetBattlerSide(BATTLE_OPPOSITE(battler));
            if (gSideTimers[targetSide].spikesAmount >= 3) break;

            gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
            gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
            BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
            }
            return TRUE;
        
        case ABILITY_LAWNMOWER:
            if (!(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)) break;

            BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
            return TRUE;
        
        case ABILITY_VIOLENT_RUSH:
            gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_VIOLENT_RUSH;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_RAPID_RESPONSE:
            gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_RAPID_RESPONSE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_DEMOLITIONIST:
        case ABILITY_READIED_ACTION:
            gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_READIED_ACTION;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_ON_THE_PROWL:
        case ABILITY_OVERWATCH:
            gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_ON_THE_PROWL;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_SHOWDOWN_MODE:
            gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_SHOWDOWN_MODE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        
        case ABILITY_PHANTOM:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_GHOST)) break;

            gBattleMons[battler].type3 = TYPE_GHOST;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_AQUATIC:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_WATER)) break;

            gBattleMons[battler].type3 = TYPE_WATER;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
        
        case ABILITY_GROUNDED:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_GROUND)) break;

            gBattleMons[battler].type3 = TYPE_GROUND;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_FAIRY_TALE:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_FAIRY)) break;

            gBattleMons[battler].type3 = TYPE_FAIRY;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_ICE_AGE:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_ICE)) break;

            gBattleMons[battler].type3 = TYPE_ICE;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_DRAGONFLY:
        case ABILITY_HALF_DRAKE:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_DRAGON)) break;

            gBattleMons[battler].type3 = TYPE_DRAGON;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_METALLIC:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_STEEL)) break;

            gBattleMons[battler].type3 = TYPE_STEEL;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_HOVER:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_PSYCHIC)) break;

            gBattleMons[battler].type3 = TYPE_PSYCHIC;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_TERAVOLT:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_ELECTRIC)) break;

            gBattleMons[battler].type3 = TYPE_ELECTRIC;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_TURBOBLAZE:
            if (IS_BATTLER_OF_TYPE(battler, TYPE_FIRE)) break;

            gBattleMons[battler].type3 = TYPE_FIRE;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        
        case ABILITY_POWER_OF_ALCHEMY:
            gBattlerTarget = BATTLE_OPPOSITE(battler);
            if (!IsBattlerAlive(gBattlerTarget)
                || !gBattleMons[gBattlerTarget].item
                || gBattleMons[gBattlerTarget].item == ITEM_BIG_NUGGET
                || !CanBattlerGetOrLoseItem(gBattlerTarget, gBattleMons[gBattlerTarget].item))
                gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
            if (!IsBattlerAlive(gBattlerTarget)
                || !gBattleMons[gBattlerTarget].item
                || gBattleMons[gBattlerTarget].item == ITEM_BIG_NUGGET
                || !CanBattlerGetOrLoseItem(gBattlerTarget, gBattleMons[gBattlerTarget].item))
                break;

            gStackBattler2 = gBattlerTarget;
            gLastUsedItem = gBattleMons[gBattlerTarget].item;
            if (gBattleMons[gBattlerTarget].item == ITEM_BLACK_SLUDGE)
            {
                gBattleMons[gBattlerTarget].item = ITEM_BIG_NUGGET;
                BattleScriptPushCursorAndCallback(BattleScript_PowerOfAlchemyGold);
            }
            else
            {
                gBattleMons[gBattlerTarget].item = ITEM_BLACK_SLUDGE;
                BattleScriptPushCursorAndCallback(BattleScript_PowerOfAlchemySludge);
            }
            return TRUE;
        
        case ABILITY_REJECTION:
            if (gFieldTimers.quashTimer) break;

            gFieldTimers.quashTimer = QUASH_DURATION;
            gFieldTimers.started.quash = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_REJECTION;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;

    }

    return FALSE;
}

static int HandleEndTurnAbilityAs(int ability, int battler);

int HandleEndTurnAbility(int abilityNumber, int battler)
{
    u16 abilities[4] = {0};
    int ability;

    gBattlerAttacker = gBattlerAbility = battler;

    if (abilityNumber > NUM_ABILITY_SLOTS)
    {
        // TODO: Handle non-ability actions that happen in this pass
        return FALSE;
    }

    GET_ALL_BATTLER_ABILITIES(abilities, battler, battler);
    ability = gBattleScripting.abilityPopupOverwrite = abilities[abilityNumber];

    return HandleEndTurnAbilityAs(ability, battler);
}

int HandleEndTurnAbilityAs(int ability, int battler)
{
    int i = 0;

    switch (ability)
    {
        case ABILITY_SCHOOLING:
            if (gBattleMons[battler].level < 20) break;
        case ABILITY_FLOWER_GIFT:
        case ABILITY_FORECAST:
        case ABILITY_SHIELDS_DOWN:
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;
            if (!ShouldChangeFormHpBased(battler)) break;

            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            return TRUE;
        
        case ABILITY_POWER_CONSTRUCT:
            if (gBattleMons[battler].species != SPECIES_ZYGARDE && gBattleMons[battler].species != SPECIES_ZYGARDE_10) break;
            if (gBattleMons[battler].hp > gBattleMons[battler].maxHP / 2) break;
            if (gBattleMons[battler].status2 && STATUS2_TRANSFORMED) break;

            gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = gBattleMons[battler].species;
            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, SPECIES_ZYGARDE_COMPLETE);
            gBattleMons[battler].species = SPECIES_ZYGARDE_COMPLETE;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            return TRUE;
        
        case ABILITY_HUNGER_SWITCH:
            if (gBattleMons[battler].status2 & STATUS2_TRANSFORMED) break;
            {
            int species = SPECIES_NONE;
            if (gBattleMons[battler].species == SPECIES_MORPEKO) species = SPECIES_MORPEKO_HANGRY;
            else if (gBattleMons[battler].species == SPECIES_MORPEKO_HANGRY) species = SPECIES_MORPEKO;
            else break;

            UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, species);
            gBattleMons[battler].species = species;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3NoPopup);
            }
            return TRUE;
        
        case ABILITY_DRY_SKIN:
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !BATTLER_HAS_MAGIC_GUARD(battler))
            {
                gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
                if (gBattleMoveDamage == 0)
                    gBattleMoveDamage = 1;
                BattleScriptPushCursorAndCallback(BattleScript_SolarPowerActivates);
                return TRUE;
            }
            else if (HandleEndTurnAbilityAs(ABILITY_RAIN_DISH, battler)) return TRUE;
            break;
        
        case ABILITY_PURIFYING_WATERS:
        case ABILITY_HYDRATION:
            if (!IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) break;

            if (AbilityHealMonStatus(gActiveBattler, ABILITY_PURIFYING_WATERS)) return TRUE;
            break;
        
        case ABILITY_SHED_SKIN:
            if (Random() % 100 >= 30) break;

            if (AbilityHealMonStatus(gActiveBattler, ABILITY_SHED_SKIN)) return TRUE;
            break;
        
        case ABILITY_SPEED_BOOST:
            if (gVolatileStructs[battler].isFirstTurn == 2) break;
            if (!ChangeStatBuffs(battler, StatBuffValue(1), STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL)) break;
            
            BattleScriptPushCursorAndCallback(BattleScript_SpeedBoostActivates);
            gBattleScripting.battler = battler;
            return TRUE;
        
        case ABILITY_MOODY:
            if (gVolatileStructs[battler].isFirstTurn == 2) break;
            {
            int validToRaise = 0, validToLower = 0;

            for (i = STAT_ATK; i < NUM_STATS; i++)
            {
                if (CompareStat(battler, i, MIN_STAT_STAGE, CMP_GREATER_THAN))
                    validToLower |= 1 << i;
                if (CompareStat(battler, i, MAX_STAT_STAGE, CMP_LESS_THAN))
                    validToRaise |= 1 << i;
            }

            if (!validToLower && !validToRaise) break;

            if (validToRaise)
            {
                do
                {
                    i = (Random() % NUM_STATS - STAT_ATK) + STAT_ATK;
                } while (!(validToRaise & (1 << i)));
                SetStatChanger(i, 2);
                validToLower &= ~(1 << i);
            }
            if (validToLower)
            {
                do
                {
                    i = (Random() % NUM_STATS - STAT_ATK) + STAT_ATK;
                } while (!(validToLower & (1 << i)));
                SET_STATCHANGER2(gBattleScripting.savedStatChanger, i, 1, TRUE);
            }
            BattleScriptPushCursorAndCallback(BattleScript_MoodyActivates);
            }
            return TRUE;
        
        case ABILITY_TRUANT:
            gVolatileStructs[battler].truantCounter ^= 1;
            break;
        
        case ABILITY_BAD_DREAMS:
            BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
            return TRUE;
        
        case ABILITY_SWEET_DREAMS:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (!(gBattleMons[battler].status1 & STATUS1_SLEEP) && !BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE)) break;

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(BattleScript_SweetDreamsActivates);
            return TRUE;
        
        case ABILITY_HEALER:
            if (Random() % 100 >= 30) break;

            if (IsBattlerAlive(BATTLE_PARTNER(battler)) && gBattleMons[BATTLE_PARTNER(battler)].status1 & STATUS1_ANY)                    
            {
                gEffectBattler = battler;
                gBattleScripting.battler = BATTLE_PARTNER(battler);
                BattleScriptPushCursorAndCallback(BattleScript_HealerActivates);
                return TRUE;
            }
            else if (IsBattlerAlive(battler) && gBattleMons[battler].status1 & STATUS1_ANY)                    
            {
                if (AbilityHealMonStatus(battler, ability)) return TRUE;
            }
            break;
        
        case ABILITY_HARVEST:
        case ABILITY_BIG_LEAVES:
            if (gBattleMons[battler].item) break;
            if (gBattleStruct->changedItems[battler]) break;
            if (ItemId_GetPocket(GetUsedHeldItem(battler)) != POCKET_BERRIES) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && Random() % 2) break;

            BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
            return TRUE;
        
        case ABILITY_SELF_REPAIR:
        case ABILITY_APPLE_PIE:
        case ABILITY_SELF_SUFFICIENT:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (gVolatileStructs[battler].isFirstTurn == 2) break;

            gBattleMoveDamage = gBattleMons[battler].maxHP / 16;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
            return TRUE;

        case ABILITY_CELESTIAL_BLESSING:
        case ABILITY_ETERNAL_BLESSING:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (gVolatileStructs[battler].isFirstTurn == 2) break;
            if (!IsBattlerTerrainAffected(battler, STATUS_FIELD_MISTY_TERRAIN)) break;

            gBattleMoveDamage = gBattleMons[battler].maxHP / 12;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
            return TRUE;

        case ABILITY_PEACEFUL_SLUMBER:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (gVolatileStructs[battler].isFirstTurn == 2) break;

            i = gBattleMons[battler].status1 & STATUS1_SLEEP || BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE);

            gBattleMoveDamage = gBattleMons[battler].maxHP / 16;

            if (i) gBattleMoveDamage += gBattleMons[battler].maxHP / 8;

            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(i ? BattleScript_SweetDreamsActivates : BattleScript_SelfSufficientActivates);
            return TRUE;
        
        case ABILITY_RAIN_DISH:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (gVolatileStructs[battler].isFirstTurn == 2) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) break;

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
            return TRUE;
        
        case ABILITY_WHITE_NOISE:
        case ABILITY_PEACEFUL_REST:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (gVolatileStructs[battler].isFirstTurn == 2) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) break;

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
            return TRUE;
        
        case ABILITY_ICE_BODY:
            if (BATTLER_MAX_HP(battler)) break;
            if (BATTLER_HEALING_BLOCKED(battler)) break;
            if (gVolatileStructs[battler].isFirstTurn == 2) break;
            if (!IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) break;

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
            return TRUE;
        
        case ABILITY_CUD_CHEW:
            {
            struct CudChewState state = GetAbilityStateAs(battler, ABILITY_CUD_CHEW).cudChewState;
            if (state.setThisTurn)
            {
                SetAbilityStateAs(battler, ABILITY_CUD_CHEW, (union AbilityStates) { .cudChewState = { .itemId = state.itemId } });
            }
            else if (state.itemId)
            {
                // attacker temporarily gains their item
                gBattleStruct->changedItems[battler] = gBattleMons[battler].item;
                gBattleMons[battler].item = state.itemId;
                gBattleScripting.abilityPopupOverwrite = ABILITY_CUD_CHEW;

                SetAbilityStateAs(battler, ABILITY_CUD_CHEW, (union AbilityStates) { .cudChewState = { .activating = TRUE } });
                
                BattleScriptPushCursorAndCallback(BattleScript_CudChew);
                return TRUE;
            }
            }
            break;
        
        case ABILITY_COLOR_SPECTRUM:
            {
            int newType;
            do
            {
                newType = Random() % NUMBER_OF_MON_TYPES;
            } while (newType == TYPE_MYSTERY || newType == TYPE_STELLAR || IS_BATTLER_OF_TYPE(gActiveBattler, newType));

            gBattleMons[gActiveBattler].type1 = newType;
            gBattleMons[gActiveBattler].type2 = newType;
            gBattleMons[gActiveBattler].type3 = TYPE_MYSTERY;
            gBattlerAbility = gActiveBattler;
            gBattleScripting.abilityPopupOverwrite = ABILITY_COLOR_SPECTRUM;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerBecameTheTypeFullEnd3);
            }
            return TRUE;

        case ABILITY_BLOOD_PRICE:
            if (IS_MOVE_STATUS(gLastResultingMoves[battler])) break;
            if (BATTLER_HAS_MAGIC_GUARD(battler)) break;
            if (gRoundStructs[battler].confusionSelfDmg) break;

            gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 10;
            if (gBattleMoveDamage == 0)
                gBattleMoveDamage = 1;
            BattleScriptPushCursorAndCallback(BattleScript_AbilitySelfDamage);
            return TRUE;
    }

    return FALSE;
}