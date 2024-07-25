#include "global.h"
#include "gba/gba.h"
#include "battle_events.h"
#include "constants/battle_events.h"
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


static u8 gNbBattleEvents;
static u8 gCurrBattleEvent;
EWRAM_DATA struct BattleEvent gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE] = { 
    { .id = BATTLE_EVENT_NONE, 
    .data0=0, 
    .data1=0 }
 };


void RegisterBattleEvent(u8 battleEventID, u8 battleEventData0, u8 battleEventData1){
    //reached the limit
    if (gNbBattleEvents == BATTLE_EVENTS_MAX_REGISTERABLE) {
        //how could i warn btw? shout in a message box?
        return;
    }
    // using a pointer over gBattleEvents would be nicer but i suppose it will stay like that because C sucks
    gBattleEvents[gNbBattleEvents].id = battleEventID;
    gBattleEvents[gNbBattleEvents].data0 = battleEventData0 & 0xF;
    gBattleEvents[gNbBattleEvents].data1 = battleEventData1 & 0xF;
    gNbBattleEvents += 1;
}

// clear all battle Events
void UnregisterBattlesEvents(){
    u8 i;
    for (i = 0; i < gNbBattleEvents; i++){
        // a dangerous alternative would be to use memset xd
        gBattleEvents[i].id = BATTLE_EVENT_NONE;
        gBattleEvents[i].data0 = 0;
        gBattleEvents[i].data1 = 0;
    }
    gNbBattleEvents = 0;
    gCurrBattleEvent = 0;
}

/**
 * Two reasons to unregister
 * - remove the need to process the data, economising a fair share of cycles.
 * - Allow for one time uses mid battle even if we could use the data for that
 *   It's just that if we unregister it with RUN_BATTLESCRIPT_UNREGISTER it lower the chances of forgetting
 */
void UnregisterCurrentBattleEvent(){
    gBattleEvents[gCurrBattleEvent].id = BATTLE_EVENT_NONE;
    gBattleEvents[gCurrBattleEvent].data0 = 0;
    gBattleEvents[gCurrBattleEvent].data1 = 0;
    // i do wonder if it's worth to shift the array afterwards so less cycles are needed for that
    // probably overkill
}

u8 ExecBattleEvents(u8 execEnum){
    // it goes by the principle that it will be executed in loop until it returns ALL CLEAR
    while (gCurrBattleEvent < gNbBattleEvents){
        if (BattleEventExec(gBattleEvents[gCurrBattleEvent], execEnum) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL){
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
u8 BattleEventExec(struct BattleEvent battleEvent, u8 execEnum){
    if (battleEvent.id == BATTLE_EVENT_NONE)
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        
    switch (execEnum)
    {
    case EXEC_BATTLE_EVENT_BEFORE_FIRST_TURN:
        if (BattleEventBeforeFirstTurnExec(battleEvent) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
        break;
    case EXEC_BATTLE_EVENT_START_OF_TURN:
        if (BattleEventStartTurnExec(battleEvent) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
        break;
    default:
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

bool8 AffectNStatusOnTeamFromLastToFirst(u32 status, u8 n){
    u8 i;
    if (n == 0)
        n = 1;
    for (i = gPlayerPartyCount - 1; i > 0 ; i--){
        // do not overwrite a status already existing if we want to have multiple possible status registerable.
        if (gPlayerParty[i].status == STATUS1_NONE){
            gPlayerParty[i].status = status;
            if ((--n) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

// ran once pokemon have landed before their ability have popped
u8 BattleEventBeforeFirstTurnExec(struct BattleEvent battleEvent){
    u8 data;
    switch (battleEvent.id)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_POSTURE_OFFENSE:
        SET_STR2(gText_Attack, sText_Strike)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureOffensive)
    case BATTLE_EVENT_POSTURE_DEFENSE:
        SET_STR2(gText_Defense, sText_Defend)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureDefensive)
    case BATTLE_EVENT_POSTURE_SPECIAL:
        SET_STR2(gText_SpAtk, sText_Strike)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureSpecial)
    case BATTLE_EVENT_POSTURE_SPDEF:
        SET_STR2(gText_SpDef, sText_Defend)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureSpdef)
    case BATTLE_EVENT_POSTURE_SPEED:
        SET_STR2(gText_Speed, sText_Rush)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureSpeed)
    case BATTLE_EVENT_POSTURE_ACCURACY:
        SET_STR2(gText_Accuracy2, sText_Aim)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureAccuracy)
    case BATTLE_EVENT_POSTURE_CRIT:
        SET_STR2(gText_Critical, sText_Focus)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillPostureCrit)
    case BATTLE_EVENT_LAST_PARALYZED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_PARALYSIS, battleEvent.data0))
            PlaySE(SE_M_THUNDERBOLT2);
        SET_STR1(gText_Paralysis)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_BURNED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_BURN, battleEvent.data0))
            PlaySE(SE_M_FLAME_WHEEL);
        SET_STR1(gText_Burn)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_SLEEP:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_SLEEP, battleEvent.data0))
            PlaySE(SE_M_SNORE);
        SET_STR1(gText_Sleep)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_FROSTBITE:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_FROSTBITE, battleEvent.data0))
            PlaySE(SE_M_ICY_WIND); // TODO PROBABLY WRONG SE
        SET_STR1(sText_Frostbite)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_BLEED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_BLEED, battleEvent.data0))
            PlaySE(SE_M_BUBBLE);
        SET_STR1(gText_Bleed)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_POISONED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_POISON, battleEvent.data0))
            PlaySE12WithPanning(SE_M_TOXIC, 13);
        SET_STR1(gText_Poison)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_TOXIC:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_TOXIC_POISON, battleEvent.data0))
            PlaySE(SE_M_TOXIC);
        SET_STR1(sText_Toxic)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_STEALTH_ROCK:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillTerrainStealthRock)
    /*case BATTLE_EVENT_TOXIC_SPIKES:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillTerrainToxicSpikes)
    case BATTLE_EVENT_SPIKES:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_SPIKES;
        gSideTimers[B_SIDE_PLAYER].spikesAmount = (battleEvent.data0 & 0x03) || 1
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillTerrainSpikes)
    case BATTLE_EVENT_:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillTerrainToxicSpikes)*/
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

// ran once the turn has reached its end before the player can get its hand on control again
// which is the start, the end of the end is in fact the start, how philosophic. 
u8 BattleEventStartTurnExec(struct BattleEvent battleEvent){
    /*MgbaOpen();
    MgbaPrintf(MGBA_LOG_WARN, "fainted player: %d, fainted trainer %d", gFaintedMonCount[0], gFaintedMonCount[1]);
    MgbaClose();*/
    // to put matblock gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_MAT_BLOCK;
    //gQueuedExtraAttackData if I want to add attack to the queue?
    //gQuashedBattlers++; how do I use that even?
    // PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 2, magnitude); // to indicade the number left of somethingTher.
    //moveSecondaryEffectChance if i want to apply serene grace
    // prevent some abitlity to be executed once a pokemon has landed because it's too OP and most importantly bugged af.
    if (gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn == 1 && IsBattleEventForbiddenOnSwitchIn(battleEvent.id))
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    switch (battleEvent.id)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEADY_OFFENSE:
        SET_STR1(gText_Attack)
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadyOffense);
    case BATTLE_EVENT_STEADY_DEFENSE:
        SET_STR1(gText_Defense);
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadyDefense);
    case BATTLE_EVENT_STEADY_SPECIAL:
        SET_STR1(gText_SpAtk);
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadySpecial);
    case BATTLE_EVENT_STEADY_SPDEF:
        SET_STR1(gText_SpDef);
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadySpedef);
    case BATTLE_EVENT_STEADY_SPEED:
        SET_STR1(gText_Speed);
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadySpeed);
    case BATTLE_EVENT_STEADY_ACCURACY:
        SET_STR1(gText_Accuracy2);
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadyAccuracy);
    case BATTLE_EVENT_STEADY_CRIT:
        SET_STR1(gText_Critical);
        RUN_BATTLESCRIPT(BattleScript_GymSkillSteadyCrit);
    
    case BATTLE_EVENT_LAST_STAND:
        if (gFaintedMonCount[1] != battleEvent.data0)
            return EXEC_BATTLE_EVENTS_ALL_CLEAR;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillLastStand);
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}
