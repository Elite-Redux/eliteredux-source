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
        if (BattleEventsMegaSwitch(gBattleEvents[gCurrBattleEvent - 1], execEnum) == EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL)
            return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    }
    // reset so it can be reexecuted later in a battle
    gCurrBattleEvent = 0;
    return EXEC_BATTLE_EVENTS_ALL_CLEAR; 
}

u8 BattleEventsMegaSwitch(u8 battleEvent, u8 execEnum){
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

u8 BattleEventBeforeFirstTurnExec(u8 battleEvent){
    u8 i;
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEALTH_ROCK_START:
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
    case BATTLE_EVENT_LEADER_1RST_PLUS_FOUR_DEF_BOOST:
        gBattleMons[B_POSITION_OPPONENT_LEFT].statStages[STAT_DEF] = min(12, gBattleMons[B_POSITION_OPPONENT_LEFT].statStages[STAT_DEF] + 4);
        BattleScriptExecute(BattleScript_GymSkillFourTimesBoost);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}

u8 BattleEventEndTurnExec(u8 battleEvent){
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case STEADY_DEFENSE:
        gBattleMons[B_POSITION_OPPONENT_LEFT].statStages[STAT_DEF] = min(12, gBattleMons[B_POSITION_OPPONENT_LEFT].statStages[STAT_DEF] + 1);
        BattleScriptExecute(BattleScript_GymSkillEndOfTurnBoost);
        return EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL;
    }
    return EXEC_BATTLE_EVENTS_ALL_CLEAR;
}