#ifndef GUARD_BATTLE_EVENTS_H
#define GUARD_BATTLE_EVENTS_H


#define BATTLE_EVENTS_MAX_REGISTERABLE 16

 // I could unionize both of these u8 but not sure it's worth.
extern u8 gNbBattleEvents;
extern u8 gCurrBattleEvent;
extern u8 gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE];
void RegisterBattleEvent(u8 battleEvent);
void UnregisterBattlesEvents();
// will iterate over the battleEvents
u8 ExecBattleEvents(u8 execEnum);
u8 BattleEventExec(u8 battleEvent, u8 execEnum);
u8 BattleEventBeforeFirstTurnExec(u8 battleEvent);
u8 BattleEventEndTurnExec(u8 battleEvent);
// enum to communicate to execBattleEvent as execEnum parameter
enum {
    EXEC_BATTLE_EVENT_BEFORE_FIRST_TURN,
    EXEC_BATTLE_EVENT_END_OF_TURN,
};

// enum that returns from execBattleEvents to communicate if a rendering is needed or if all battleEvents were done correctly

enum {
    EXEC_BATTLE_EVENTS_ALL_CLEAR,
    EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL,
};


#endif // GUARD_BATTLE_EVENTS_H