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

static u8 gNbBattleEvents;
static u8 gCurrBattleEvent;
EWRAM_DATA u8 gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE] = { BATTLE_EVENT_NONE };

void RegisterBattleEvent(u8 battleEvent){
    //reached the limit
    if (gNbBattleEvents == BATTLE_EVENTS_MAX_REGISTERABLE) {
        //how could i warn btw? shout in a message box?
        return;
    }
    gBattleEvents[gNbBattleEvents] = battleEvent;
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

const u8 gText_Strike[] = _("Strike");
const u8 gText_Defend[] = _("Defend");
const u8 gText_Rush[] = _("Rush");
const u8 gText_Aim[] = _("Aim");
const u8 gText_Focus[] = _("Focus");

const u8 sOnSwitchInForbiddenBattleEvent[] = {
    BATTLE_EVENT_STEADY_OFFENSE,
    BATTLE_EVENT_STEADY_DEFENSE,
    BATTLE_EVENT_STEADY_SPECIAL,
    BATTLE_EVENT_STEADY_SPDEF,
    BATTLE_EVENT_STEADY_SPEED,
    BATTLE_EVENT_STEADY_ACCURACY,
    BATTLE_EVENT_STEADY_CRIT,
};

bool8 isBattleEventForbiddenOnSwitchIn(u8 battleEvent)
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
if (gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn == 2) 
            return EXEC_BATTLE_EVENTS_ALL_CLEAR; // switched in and don't apply then because it's buggy and too OP

// ran once pokemon have landed before their ability have popped
u8 BattleEventBeforeFirstTurnExec(u8 battleEvent){
    u8 i;
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEALTH_ROCK:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        BattleScriptExecute(BattleScript_GymSkillTerrainStealthRock);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_LAST_PARALYZED:
        for (i = gPlayerPartyCount - 1; i > 0 ; i--){
            if (gPlayerParty[i].status == STATUS1_NONE){
                gPlayerParty[i].status = STATUS1_PARALYSIS;
                break;
            }
        }
        break;
    case BATTLE_EVENT_POSTURE_OFFENSE:
        StringExpandPlaceholders(gStringVar1, gText_Attack);
        StringExpandPlaceholders(gStringVar2, gText_Strike);
        BattleScriptExecute(BattleScript_GymSkillPostureOffensive);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_POSTURE_DEFENSE:
        StringExpandPlaceholders(gStringVar1, gText_Defense);
        StringExpandPlaceholders(gStringVar2, gText_Defend);
        BattleScriptExecute(BattleScript_GymSkillPostureDefensive);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_POSTURE_SPECIAL:
        StringExpandPlaceholders(gStringVar1, gText_SpAtk);
        StringExpandPlaceholders(gStringVar2, gText_Strike);
        BattleScriptExecute(BattleScript_GymSkillPostureSpecial);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_POSTURE_SPDEF:
        StringExpandPlaceholders(gStringVar1, gText_SpDef);
        StringExpandPlaceholders(gStringVar2, gText_Defend);
        BattleScriptExecute(BattleScript_GymSkillPostureSpdef);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_POSTURE_SPEED:
        StringExpandPlaceholders(gStringVar1, gText_Speed);
        StringExpandPlaceholders(gStringVar2, gText_Rush);
        BattleScriptExecute(BattleScript_GymSkillPostureSpeed);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_POSTURE_ACCURACY:
        StringExpandPlaceholders(gStringVar1, gText_Accuracy2);
        StringExpandPlaceholders(gStringVar2, gText_Aim);
        BattleScriptExecute(BattleScript_GymSkillPostureAccuracy);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_POSTURE_CRIT:
        StringExpandPlaceholders(gStringVar1, gText_Critical);
        StringExpandPlaceholders(gStringVar2, gText_Focus);
        BattleScriptExecute(BattleScript_GymSkillPostureCrit);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

// ran once the turn has reached its end before the player can get its hand on control again
u8 BattleEventEndTurnExec(u8 battleEvent){
    // prevent some abitlity to be executed once a pokemon has landed because it's too OP and most importantly bugged af.
    if (gVolatileStructs[B_POSITION_OPPONENT_LEFT].isFirstTurn == 2 && isBattleEventForbiddenOnSwitchIn(battleEvent))
        return EXEC_BATTLE_EVENTS_ALL_CLEAR;
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEADY_OFFENSE:
        StringExpandPlaceholders(gStringVar1, gText_Attack);
        BattleScriptExecute(BattleScript_GymSkillSteadyOffense);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_STEADY_DEFENSE:
        StringExpandPlaceholders(gStringVar1, gText_Defense);
        BattleScriptExecute(BattleScript_GymSkillSteadyDefense);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_STEADY_SPECIAL:
        StringExpandPlaceholders(gStringVar1, gText_SpAtk);
        BattleScriptExecute(BattleScript_GymSkillSteadySpecial);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_STEADY_SPDEF:
        StringExpandPlaceholders(gStringVar1, gText_SpDef);
        BattleScriptExecute(BattleScript_GymSkillSteadySpedef);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_STEADY_SPEED:
        StringExpandPlaceholders(gStringVar1, gText_Speed);
        BattleScriptExecute(BattleScript_GymSkillSteadySpeed);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_STEADY_ACCURACY:
        StringExpandPlaceholders(gStringVar1, gText_Accuracy2);
        BattleScriptExecute(BattleScript_GymSkillSteadyAccuracy);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    case BATTLE_EVENT_STEADY_CRIT:
        StringExpandPlaceholders(gStringVar1, gText_Critical);
        BattleScriptExecute(BattleScript_GymSkillSteadyCrit);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}