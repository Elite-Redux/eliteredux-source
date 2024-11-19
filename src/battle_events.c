#include "global.h"
#include "gba/gba.h"
#include "battle_events.h"
#include "constants/battle_events.h"
#include "data/text/battle_events.h"
#include "constants/battle_string_ids.h"
#include "battle_main.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_scripts.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"
#include "string_util.h"
#include "strings.h"
#include "sound.h"
#include "constants/songs.h"
#include "field_message_box.h"


static u8 gNbBattleEvents = 0;
static u8 gCurrBattleEvent = 0;
EWRAM_DATA u8 gLastBattleEvent = 0;

EWRAM_DATA struct BattleEvent gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE] = { 0 };

const u8 sText_WarnMaxBattleEventReached[] = _("Warning you have registered\ntoo many battle events.");
void RegisterBattleEvent(u8 battleEventID, u8 battleEventData0, u8 battleEventData1) {
    //reached the limit
    if (gNbBattleEvents == BATTLE_EVENTS_MAX_REGISTERABLE) {
        //I sure hope this will trigger well enough so it may warn the user.
        PlaySE(SE_LOW_HEALTH);
        ShowFieldMessage(sText_WarnMaxBattleEventReached);
        return;
    }
        gBattleEvents[gNbBattleEvents++] = (struct BattleEvent) {
        .id = battleEventID,
        .data0 = battleEventData0 & 0xF, // bitmasked because it's only 4 bits per data
        .data1 = battleEventData1 & 0xF,
    };
}

// clear all battle Events
void UnregisterBattlesEvents() {
    gNbBattleEvents = 0;
    gCurrBattleEvent = 0;
}

/**
 * Three reasons to unregister
 * - remove the need to process the data, economising a fair share of cycles. (edit, this may be irrelevant)
 * - Allow for one time uses mid battle even if we could use the data for that
 * - It's just that if we unregister it with RUN_BATTLESCRIPT_UNREGISTER it lower the chances of forgetting
 */
void UnregisterCurrentBattleEvent() {
    gBattleEvents[gCurrBattleEvent--] = gBattleEvents[--gNbBattleEvents];
    // i do wonder if it's worth to shift the array afterwards so less cycles are needed for that
    // probably overkill
}

// entry point of battleEvents in battle.
u8 ExecBattleEvents() {
    // it goes by the principle that it will be executed in loop until it returns ALL CLEAR
    while (gCurrBattleEvent < gNbBattleEvents) {
        if (BattleEventExec(&gBattleEvents[gCurrBattleEvent]) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL) {
            gCurrBattleEvent++;
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
        } 
        gCurrBattleEvent++;
            
    }
    // reset so it can be reexecuted later in a battle
    gCurrBattleEvent = 0;
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

//exec only one battle Event
u8 BattleEventExec(struct BattleEvent *battleEvent) {
    if (battleEvent->id == BATTLE_EVENT_NONE)
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;

    gLastBattleEvent = battleEvent->id;

    switch (gBattleResults.battleTurnCounter)
    {
    case 0:
        if (BattleEventBeforeFirstTurnExec(battleEvent) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
        break;
    default:
        if (BattleEventStartTurnExec(battleEvent) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
        break;
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

// you cannot fathom my lazyness
#define RUN_BATTLESCRIPT(bs)    BattleScriptExecute(bs); \
    return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
#define RUN_BATTLESCRIPT_UNREGISTER(bs)   UnregisterCurrentBattleEvent();\
    BattleScriptExecute(bs); \
    return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
#define SET_STR1(str)   StringExpandPlaceholders(gStringVar1, str);
#define SET_STR2(str1, str2) StringExpandPlaceholders(gStringVar1, str1);\
    StringExpandPlaceholders(gStringVar2, str2);

// I couldn't find some reliable substitute sometimes, probably missed some.
const u8 sText_Strike[] = _("Strike");
const u8 sText_Defend[] = _("Defend");
const u8 sText_Rush[] = _("Rush");
const u8 sText_Aim[] = _("Aim");
const u8 sText_Focus[] = _("Focus");
const u8 sText_Toxic[] = _("Sharp poison");
const u8 sText_Frostbite[] = _("Frostbite");
const u8 sText_LeechSeed[] = _("Leech seed");

// do not put BattleEventBeforeFirstTurn related BEvents in it.
const u8 sOnSwitchInForbiddenBattleEvent[] = {
    BATTLE_EVENT_STEADY_OFFENSE,
    BATTLE_EVENT_STEADY_DEFENSE,
    BATTLE_EVENT_STEADY_SPECIAL,
    BATTLE_EVENT_STEADY_SPDEF,
    BATTLE_EVENT_STEADY_SPEED,
    BATTLE_EVENT_STEADY_ACCURACY,
    BATTLE_EVENT_STEADY_CRIT,
};

bool8 IsBattleEventForbiddenOnSwitchIn(u8 battleEventID)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sOnSwitchInForbiddenBattleEvent); i++)
    {
        if (battleEventID == sOnSwitchInForbiddenBattleEvent[i])
            return TRUE;
    }
    return FALSE;
}

bool8 AffectNStatusOnTeamFromLastToFirst(u32 status, u8 n) {
    u8 i;
    if (n == 0)
        n = 1;
    for (i = gPlayerPartyCount - 1; i > 0 ; i--) {
        // do not overwrite a status already existing if we want to have multiple possible status registerable.
        if (gPlayerParty[i].status == STATUS1_NONE) {
            gPlayerParty[i].status = status;
            if ((--n) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

// will always set the data in slot 2 (data1) of the battleEvent
bool8 HasNumberOfTurnsStayedReached(struct BattleEvent *battleEvent, u8 pos) {
    if (gVolatileStructs[pos].isFirstTurn) {
        battleEvent->data1 = 0;
    } else {
        battleEvent->data1 += 1;
    }
    return battleEvent->data1 >= battleEvent->data0;
}

void SetSubstituteBattleEvent(void)
{
    u32 hp = gBattleMons[gBattlerAttacker].maxHP / 4;
    hp = max(hp, 1);

    gBattleMons[gBattlerAttacker].status2 |= STATUS2_SUBSTITUTE;
    gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_WRAPPED);
    gVolatileStructs[gBattlerAttacker].substituteHP = hp;
    gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE;
}

bool8 DepleteTeamPowerPointOfMove(u16 moveId) {
    bool8 hasBeenModified;
    u8 i, j;
    u32 move, wtf;
    for (i = 0; i < gPlayerPartyCount; i++) {
        // if the move is the one targetted it totally depletes its PP
        for (j = 0; j < MAX_MON_MOVES; j++) {
            move = GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + j);
            if (move != moveId)
                continue;
            wtf = 0;
            SetMonData(&gPlayerParty[i], MON_DATA_PP1 + j, &wtf);
            //yeah huh you need to apply this to the gbattlers too
            if (i == 0)
                gBattleMons[B_POSITION_PLAYER_LEFT].pp[j] = 0;
            if (i == 1)
                gBattleMons[B_POSITION_PLAYER_RIGHT].pp[j] = 0;
            hasBeenModified = TRUE;
        }
    }
    return hasBeenModified;
}

#define SET_EXTRA_STATS_LEVEL_TO_BATTLER(battler, stat, level) gVolatileStructs[battler].stat += level;
#define SET_EXTRA_STATS_LEVEL(stat, level)\
SET_EXTRA_STATS_LEVEL_TO_BATTLER(B_POSITION_OPPONENT_LEFT, stat, level)\
if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE) && (gBattleMons[B_POSITION_OPPONENT_RIGHT].hp > 0))\
    SET_EXTRA_STATS_LEVEL_TO_BATTLER(B_POSITION_OPPONENT_RIGHT, stat, level);
// this is run once pokemon have landed before their ability have popped
u8 BattleEventBeforeFirstTurnExec(struct BattleEvent *battleEvent) {
    switch (battleEvent->id)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_POSTURE_OFFENSE:
        SET_STR2(gText_Attack, sText_Strike)
        if (battleEvent->data0 == 0)
            battleEvent->data0 = 4;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff3, 3, battleEvent->data0 * 20);
        SET_STATCHANGER(STAT_ATK, battleEvent->data0, FALSE);
        //SET_RAW_STATS_LEVEL_OPPONENTS(STAT_ATK, battleEvent->data0)
        SET_EXTRA_STATS_LEVEL(extraAttackLevel, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPosture);
    case BATTLE_EVENT_POSTURE_DEFENSE:
        SET_STR2(gText_Defense, sText_Defend)
        if (battleEvent->data0 == 0)
            battleEvent->data0 = 4;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff3, 3, battleEvent->data0 * 20);
        SET_STATCHANGER(STAT_DEF, battleEvent->data0, FALSE);
        //SET_RAW_STATS_LEVEL_OPPONENTS(STAT_DEF, battleEvent->data0)
        SET_EXTRA_STATS_LEVEL(extraDefenseLevel, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPosture);
    case BATTLE_EVENT_POSTURE_SPECIAL:
        SET_STR2(gText_SpAtk, sText_Strike)
        if (battleEvent->data0 == 0)
            battleEvent->data0 = 4;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff3, 3, battleEvent->data0 * 20);
        SET_STATCHANGER(STAT_ATK, battleEvent->data0, FALSE);
        //SET_RAW_STATS_LEVEL_OPPONENTS(STAT_ATK, battleEvent->data0)
        SET_EXTRA_STATS_LEVEL(extraSpAttackLevel, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPosture);
    case BATTLE_EVENT_POSTURE_SPDEF:
        SET_STR2(gText_SpDef, sText_Defend)
        if (battleEvent->data0 == 0)
            battleEvent->data0 = 4;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff3, 3, battleEvent->data0 * 20);
        SET_STATCHANGER(STAT_SPDEF, battleEvent->data0, FALSE);
        //SET_RAW_STATS_LEVEL_OPPONENTS(STAT_SPDEF, battleEvent->data0)
        SET_EXTRA_STATS_LEVEL(extraSpDefenseLevel, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPosture);
    case BATTLE_EVENT_POSTURE_SPEED:
        SET_STR2(gText_Speed, sText_Rush)
        if (battleEvent->data0 == 0)
            battleEvent->data0 = 4;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff3, 3, battleEvent->data0 * 20);
        SET_STATCHANGER(STAT_SPEED, battleEvent->data0, FALSE);
        //SET_RAW_STATS_LEVEL_OPPONENTS(STAT_SPEED, battleEvent->data0)
        SET_EXTRA_STATS_LEVEL(extraSpeedLevel, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPosture);
    case BATTLE_EVENT_POSTURE_ACCURACY:
        // NOT IMPLEMENTED BECAUSE OF RAW STATS
        //SET_STR2(gText_Accuracy2, sText_Aim)
        //if (battleEvent->data0 == 0)
        //    battleEvent->data0 = 4;
        //PREPARE_BYTE_NUMBER_BUFFER(gStringVar3, 1, battleEvent->data0);
        //SET_STATCHANGER(STAT_ACC, battleEvent->data0, FALSE);
        //RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPosture);
        break;
    case BATTLE_EVENT_POSTURE_CRIT: // not implemented fully
        SET_STR2(gText_Critical, sText_Focus)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillPostureCrit)

    case BATTLE_EVENT_LAST_PARALYZED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_PARALYSIS, battleEvent->data0))
            PlaySE(SE_M_THUNDERBOLT2);
        SET_STR1(gText_Paralysis)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_BURNED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_BURN, battleEvent->data0))
            PlaySE(SE_M_FLAME_WHEEL);
        SET_STR1(gText_Burn)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_SLEEP:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_SLEEP, battleEvent->data0))
            PlaySE(SE_M_SNORE);
        SET_STR1(gText_Sleep)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_FROSTBITE:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_FROSTBITE, battleEvent->data0))
            PlaySE(SE_M_ICY_WIND); // TODO PROBABLY WRONG SE
        SET_STR1(sText_Frostbite)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_BLEED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_BLEED, battleEvent->data0))
            PlaySE(SE_M_BUBBLE);
        SET_STR1(gText_Bleed)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_POISONED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_POISON, battleEvent->data0))
            PlaySE12WithPanning(SE_M_TOXIC, 13);
        SET_STR1(gText_Poison)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_TOXIC:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_TOXIC_POISON, battleEvent->data0))
            PlaySE(SE_M_TOXIC);
        SET_STR1(sText_Toxic)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillStatusOnTeam)

    case BATTLE_EVENT_STEALTH_ROCK:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillTerrainStealthRock)
    case BATTLE_EVENT_TOXIC_SPIKES:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount = 2;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillTerrainToxicSpikes)
    case BATTLE_EVENT_SPIKES:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_SPIKES;
        gSideTimers[B_SIDE_PLAYER].spikesAmount = battleEvent->data0 ? battleEvent->data0 & 0x03 : 1;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillTerrainSpikes)
    case BATTLE_EVENT_EMBARGO:
        gStatuses3[B_POSITION_PLAYER_LEFT] |= STATUS3_EMBARGO;
        gVolatileStructs[B_POSITION_PLAYER_LEFT].embargoTimer = battleEvent->data0;
        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gPlayerPartyCount >= 2) {
            gStatuses3[B_POSITION_PLAYER_RIGHT] |= STATUS3_EMBARGO;
            gVolatileStructs[B_POSITION_PLAYER_RIGHT].embargoTimer = battleEvent->data0;
        }
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillEmbargo)
    case BATTLE_EVENT_REFLECT:
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_REFLECT;
        gSideTimers[B_SIDE_OPPONENT].reflectTimer = battleEvent->data0;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillReflect)
    case BATTLE_EVENT_LIGHTSCREEN:
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_LIGHTSCREEN;
        gSideTimers[B_SIDE_OPPONENT].lightscreenTimer = battleEvent->data0;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillLightscreen)
    case BATTLE_EVENT_LUCKY_CHANT:
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_LUCKY_CHANT;
        gSideTimers[B_SIDE_OPPONENT].luckyChantTimer = battleEvent->data0;
        PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, battleEvent->data0);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillLuckyChant)

    case BATTLE_EVENT_PERMA_WIDE_GUARD:
        if (gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_WIDE_GUARD)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_WIDE_GUARD;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillPermaWideGuard);

    case BATTLE_EVENT_NO_PROTECT:
        if (!DepleteTeamPowerPointOfMove(MOVE_PROTECT))
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillNoProtect)
    

    case BATTLE_EVENT_TENSE_BATTLE:
        CalculateEnemyPartyCount();
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    }
        
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

// ran once the turn has reached its end before the player can get its hand on control again
// which is the start, the end of the end is in fact the start, how philosophic. 
u8 BattleEventStartTurnExec(struct BattleEvent *battleEvent) {
    //"fainted player: %d, fainted trainer %d", gFaintedMonCount[0], gFaintedMonCount[1]
    //gQueuedExtraAttackData if I want to add attack to the queue?
    //gQuashedBattlers++; how do I use that even?
    //moveSecondaryEffectChance if i want to apply serene grace
    // sText_PkmnRevivedReadyToFight => 

    // prevent some abitlity to be executed once a pokemon has landed.
    if (gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn == 1 && IsBattleEventForbiddenOnSwitchIn(battleEvent->id))
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    switch (battleEvent->id)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEADY_OFFENSE:
        SET_STR1(gText_Attack)
        
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadyOffense);
    case BATTLE_EVENT_STEADY_DEFENSE:
        SET_STR1(gText_Defense);
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadyDefense);
    case BATTLE_EVENT_STEADY_SPECIAL:
        SET_STR1(gText_SpAtk);
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadySpecial);
    case BATTLE_EVENT_STEADY_SPDEF:
        SET_STR1(gText_SpDef);
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadySpedef);
    case BATTLE_EVENT_STEADY_SPEED:
        SET_STR1(gText_Speed);
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadySpeed);
    case BATTLE_EVENT_STEADY_ACCURACY:
        SET_STR1(gText_Accuracy2);
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadyAccuracy);
    case BATTLE_EVENT_STEADY_CRIT:
        SET_STR1(gText_Critical);
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillSteadyCrit);
    
    case BATTLE_EVENT_ONSWITCH_MAT_BLOCK:
        if (gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn != 1) {
            battleEvent->data1 = 0;
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        }
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_MAT_BLOCK;
        if (!battleEvent->data1 && battleEvent->data0 > 1) {
            battleEvent->data1 = 1;
            battleEvent->data0--;
            RUN_BATTLESCRIPT(BattleScript_ExtraSkillMatBlock)
        }
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillMatBlock)
    
    case BATTLE_EVENT_ONSTAY_FORESIGHT:
        if (!HasNumberOfTurnsStayedReached(battleEvent, B_POSITION_PLAYER_LEFT) || gBattleMons[B_SIDE_PLAYER].status2 & STATUS2_FORESIGHT)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR; 
        gBattleMons[B_SIDE_PLAYER].status2 |= STATUS2_FORESIGHT;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillForesight)  
    case BATTLE_EVENT_ONSTAY_LEECH_SEED:
        if (!HasNumberOfTurnsStayedReached(battleEvent, B_POSITION_PLAYER_LEFT) || gStatuses3[B_SIDE_PLAYER] & STATUS3_LEECHSEED)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        SET_STR1(sText_LeechSeed)
        gStatuses3[B_SIDE_PLAYER] |= STATUS3_LEECHSEED;
        gStatuses3[B_SIDE_PLAYER] |= B_SIDE_OPPONENT;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillLeechSeed)
    case BATTLE_EVENT_ONSTAY_MAGNET_RISE:
        if (!HasNumberOfTurnsStayedReached(battleEvent, B_POSITION_OPPONENT_LEFT) || gStatuses3[B_SIDE_OPPONENT] & STATUS3_MAGNET_RISE)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR; 
        gStatuses3[B_SIDE_OPPONENT] |= STATUS3_MAGNET_RISE;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillMagnetRise)
    
    case BATTLE_EVENT_LAST_STAND:
        if (gFaintedMonCount[1] != battleEvent->data0)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        SET_EXTRA_STATS_LEVEL(extraAttackLevel, 5);
        SET_EXTRA_STATS_LEVEL(extraDefenseLevel, 5);
        SET_EXTRA_STATS_LEVEL(extraSpAttackLevel, 5);
        SET_EXTRA_STATS_LEVEL(extraSpDefenseLevel, 5);
        SET_EXTRA_STATS_LEVEL(extraSpeedLevel, 5);
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillLastStand);
    case BATTLE_EVENT_SUBSTITUTE:
        if (gFaintedMonCount[1] != battleEvent->data0)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        gBattlerAttacker = B_SIDE_OPPONENT;
        SetSubstituteBattleEvent();
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_ExtraSkillSubstitute);

    case BATTLE_EVENT_PERMA_HEAL_BLOCK:
        if (gStatuses3[B_SIDE_PLAYER] & STATUS3_HEAL_BLOCK)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        gStatuses3[B_SIDE_PLAYER] |= STATUS3_HEAL_BLOCK;
        gVolatileStructs[B_SIDE_PLAYER].healBlockTimer = 5;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillPermaHealBlock);
    case BATTLE_EVENT_PERMA_NIGHTMARE:
        if (gBattleMons[B_SIDE_PLAYER].status2 & STATUS2_NIGHTMARE)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        if (!(gBattleMons[B_SIDE_PLAYER].status1 & STATUS1_SLEEP))
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        gBattleMons[B_SIDE_PLAYER].status2 |= STATUS2_NIGHTMARE;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillPermaNightmare);
    case BATTLE_EVENT_PERMA_WIDE_GUARD:
        if (gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_WIDE_GUARD)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_WIDE_GUARD;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillPermaWideGuard);

    case BATTLE_EVENT_ONDS_COPY_STATS:
        if (!gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn || gSideTimers[B_POSITION_OPPONENT_LEFT].retaliateTimer != 1)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        RUN_BATTLESCRIPT(BattleScript_ExtraSkillCopyStats);


    case BATTLE_EVENT_TENSE_BATTLE:
        // play the gym battle music of gen 4 in tense moments  
        // if 50% or more of the pokemon in battle on both sides have been defeated, then play the music 
        if (gFaintedMonCount[1] >= (gEnemyPartyCount / 2) && gFaintedMonCount[0] >= (gPlayerPartyCount / 2) ) {
            PlayBGM(DP_SEQ_BA_GYM);
            UnregisterCurrentBattleEvent();
        } 
        // if the player have 3 pokemon less than the gym leader then play the music
        else if ((gPlayerPartyCount - gFaintedMonCount[0]) <= ((gEnemyPartyCount - gFaintedMonCount[1]) - 3)) {
            PlayBGM(DP_SEQ_BA_GYM);
            UnregisterCurrentBattleEvent();
        }
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}
