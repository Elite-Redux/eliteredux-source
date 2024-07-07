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
EWRAM_DATA u8 gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE] = { BATTLE_EVENT_NONE };


void registerBattleEvent(u8 battleEvent){
    //reached the limit
    if (gNbBattleEvents == BATTLE_EVENTS_MAX_REGISTERABLE) {
        //how could i warn btw? shout in a message box?
        return;
    }
    gBattleEvents[gNbBattleEvents] = battleEvent;
    gNbBattleEvents += 1;
}

void unregisterBattlesEvents(){
    u8 i;
    for (i = 0; i < gNbBattleEvents; i++){
        gBattleEvents[i] = BATTLE_EVENT_NONE;
    }
    gNbBattleEvents = 0;
}

void execBattleEvents(u8 execEnum){
    u8 i;
    MgbaOpen();
    MgbaPrintf(MGBA_LOG_WARN, "gNbBattleEvents: %d", gNbBattleEvents);
    MgbaClose();
    for (i = 0; i < gNbBattleEvents; i++){
        battleEventsMegaSwitch(gBattleEvents[i], execEnum);
    }
}

void battleEventsMegaSwitch(u8 battleEvent, u8 execEnum){
    u8 i;
    if (execEnum != EXEC_BATTLE_EVENT_BEFORE_FIRST_TURN) return;
    // TURN 0 switch statements
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEALTH_ROCK_START:
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        //BattleScriptPushCursorAndCallback(BattleScript_GymSkillTerrainStealthRock);
        break;
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
        //BattleScriptPushCursorAndCallback(BattleScript_GymSkillFourTimesBoost);
        break;
    }

}