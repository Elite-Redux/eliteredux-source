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
EWRAM_DATA u8 gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE] = { BATTLE_EVENT_NONE };
EWRAM_DATA u8 gBattleEventsData[BATTLE_EVENTS_MAX_REGISTERABLE] = { 0 };

void RegisterBattleEvent(u8 battleEvent, u8 battleEventData){
    //reached the limit
    if (gNbBattleEvents == BATTLE_EVENTS_MAX_REGISTERABLE) {
        //how could i warn btw? shout in a message box?
        return;
    }
    gBattleEvents[gNbBattleEvents] = battleEvent;
    gBattleEventsData[gNbBattleEvents] = battleEventData;
    gNbBattleEvents += 1;
}

// clear all battle Events
void UnregisterBattlesEvents(){
    u8 i;
    for (i = 0; i < gNbBattleEvents; i++){
        gBattleEvents[i] = BATTLE_EVENT_NONE;
    }
    gNbBattleEvents = 0;
    gCurrBattleEvent = 0;
}

void UnregisterCurrentEventData(){
    gBattleEventsData[gCurrBattleEvent - 1] = BATTLE_EVENT_NONE;
    // i do wonder if it's worth to shift the array afterwards so less cycles are needed for that
    // probably overkill
}

u8 GetCurrentBattleEventData(){
    return gBattleEventsData[gCurrBattleEvent - 1];
}

u8 ExecBattleEvents(u8 execEnum){
    // it goes by the principle that it will be executed in loop until it returns ALL CLEAR
    while (gCurrBattleEvent < gNbBattleEvents){
        gCurrBattleEvent++;
        if (BattleEventExec(gBattleEvents[gCurrBattleEvent - 1], execEnum) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    }
    // reset so it can be reexecuted later in a battle
    gCurrBattleEvent = 0;
    return EXEC_BATTLE_EVENTS_ALL_CLEAR; 
}

//exec only one battle Event
u8 BattleEventExec(u8 battleEvent, u8 execEnum){
    if (battleEvent == BATTLE_EVENT_NONE)
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    switch (execEnum)
    {
    case EXEC_BATTLE_EVENT_BEFORE_FIRST_TURN:
        if (BattleEventBeforeFirstTurnExec(battleEvent) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
        break;
    case EXEC_BATTLE_EVENT_END_OF_TURN:
        if (BattleEventEndTurnExec(battleEvent) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
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
#define RUN_BATTLESCRIPT_UNREGISTER(bs)   UnregisterCurrentEventData();\
    BattleScriptExecute(bs); \
    return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
#define SET_STR1(str)   StringExpandPlaceholders(gStringVar1, str);
#define SET_STR2(str1, str2) StringExpandPlaceholders(gStringVar1, str1);\
    StringExpandPlaceholders(gStringVar2, str2);


const u8 sText_Strike[] = _("Strike");
const u8 sText_Defend[] = _("Defend");
const u8 sText_Rush[] = _("Rush");
const u8 sText_Aim[] = _("Aim");
const u8 sText_Focus[] = _("Focus");
const u8 sText_Toxic[] = _("Sharp poison");
static const u8 sText_Frostbite[] = _("Frostbite");

const u8 sOnSwitchInForbiddenBattleEvent[] = {
    BATTLE_EVENT_STEADY_OFFENSE,
    BATTLE_EVENT_STEADY_DEFENSE,
    BATTLE_EVENT_STEADY_SPECIAL,
    BATTLE_EVENT_STEADY_SPDEF,
    BATTLE_EVENT_STEADY_SPEED,
    BATTLE_EVENT_STEADY_ACCURACY,
    BATTLE_EVENT_STEADY_CRIT,
};

bool8 IsBattleEventForbiddenOnSwitchIn(u8 battleEvent)
{
    u32 i;
    if (!battleEvent) return TRUE;
    for (i = 0; i < ARRAY_COUNT(sOnSwitchInForbiddenBattleEvent); i++)
    {
        if (battleEvent == sOnSwitchInForbiddenBattleEvent[i])
            return TRUE;
    }
    return FALSE;
}

bool8 AffectNStatusOnTeamFromLastToFirst(u32 status, u8 n){
    u8 i;
    if (n == 0)
        n = 1;
    for (i = gPlayerPartyCount - 1; i > 0 ; i--){
        if (gPlayerParty[i].status == STATUS1_NONE){
            gPlayerParty[i].status = status;
            if ((--n) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

// ran once pokemon have landed before their ability have popped
u8 BattleEventBeforeFirstTurnExec(u8 battleEvent){
    u8 data;
    switch (battleEvent)
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
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_PARALYSIS, GetCurrentBattleEventData()))
            PlaySE(SE_M_THUNDERBOLT2);
        SET_STR1(gText_Paralysis)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_BURNED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_BURN, GetCurrentBattleEventData()))
            PlaySE(SE_M_FLAME_WHEEL);
        SET_STR1(gText_Burn)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_SLEEP:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_SLEEP, GetCurrentBattleEventData()))
            PlaySE(SE_M_SNORE);
        SET_STR1(gText_Sleep)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_FROSTBITE:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_FROSTBITE, GetCurrentBattleEventData()))
            PlaySE(SE_M_ICY_WIND); // TODO PROBABLY WRONG SE
        SET_STR1(sText_Frostbite)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_BLEED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_BLEED, GetCurrentBattleEventData()))
            PlaySE(SE_M_BUBBLE);
        SET_STR1(gText_Bleed)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_POISONED:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_POISON, GetCurrentBattleEventData()))
            PlaySE12WithPanning(SE_M_TOXIC, 13); // UNTESTED PROBABLY BROKEN BUT XD! 
        SET_STR1(gText_Poison)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_LAST_TOXIC:
        if (AffectNStatusOnTeamFromLastToFirst(STATUS1_TOXIC_POISON, GetCurrentBattleEventData()))
            PlaySE(SE_M_TOXIC);
        SET_STR1(sText_Toxic)
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillStatusOnTeam)
    case BATTLE_EVENT_STEALTH_ROCK:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        RUN_BATTLESCRIPT_UNREGISTER(BattleScript_GymSkillTerrainStealthRock)
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

// ran once the turn has reached its end before the player can get its hand on control again
u8 BattleEventEndTurnExec(u8 battleEvent){
    /*MgbaOpen();
    MgbaPrintf(MGBA_LOG_WARN, "fainted player: %d, fainted trainer %d", gFaintedMonCount[0], gFaintedMonCount[1]);
    MgbaClose();*/
    //moveSecondaryEffectChance if i want to apply serene grace
    // prevent some abitlity to be executed once a pokemon has landed because it's too OP and most importantly bugged af.
    if (gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn == 2 && IsBattleEventForbiddenOnSwitchIn(battleEvent))
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    switch (battleEvent)
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
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}