#ifndef GUARD_BATTLE_EVENTS_H
#define GUARD_BATTLE_EVENTS_H


#define BATTLE_EVENTS_MAX_REGISTERABLE 16

extern u8 gNbBattleEvents;
extern u8 gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE];
void registerBattleEvent(u8 battleEvent);
void unregisterBattlesEvents();
// exec it whenever battle events needs to executed
void execBattleEvents(u8 execEnum);
void battleEventsMegaSwitch(u8 battleEvent, u8 execEnum);

enum {
    EXEC_BATTLE_EVENT_BEFORE_FIRST_TURN
};

#endif // GUARD_BATTLE_EVENTS_H