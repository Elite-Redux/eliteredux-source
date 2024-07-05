#include "global.h"
#include "gba/gba.h"
#include "battle_events.h"
#include "constants/battle_events.h"

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

void execBattleEvents(){
    u8 i;
    for (i = 0; i < gNbBattleEvents; i++){
        battleEventsMegaSwitch(gBattleEvents[i]);
    }
}

void battleEventsMegaSwitch(u8 battleEvent){
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    }
}