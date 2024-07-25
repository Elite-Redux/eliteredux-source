#ifndef GUARD_BATTLE_EVENTS_H
#define GUARD_BATTLE_EVENTS_H


#define BATTLE_EVENTS_MAX_REGISTERABLE 16


extern u8 gNbBattleEvents;
extern u8 gCurrBattleEvent;

/**
 * Structure that holds one battle event ID and two 4bits data.
 * Why 4 bits? because it sounds good enough, can be upgraded later.
 */
struct BattleEvent
{
    u8 id;
    u8 data0:4;
    u8 data1:4;
};
extern struct BattleEvent gBattleEvents[BATTLE_EVENTS_MAX_REGISTERABLE];
//extern u8 gBattleEventsData[BATTLE_EVENTS_MAX_REGISTERABLE];
void RegisterBattleEvent(u8 battleEvent, u8 battleEventData0, u8 battleEventData1);
void UnregisterBattlesEvents();
// will iterate over the battleEvents
u8 ExecBattleEvents(u8 execEnum);
u8 BattleEventExec(struct BattleEvent *battleEvent, u8 execEnum);
u8 BattleEventBeforeFirstTurnExec(struct BattleEvent*);
u8 BattleEventStartTurnExec(struct BattleEvent*);
// enum to communicate to execBattleEvent as execEnum parameter
enum {
    EXEC_BATTLE_EVENT_BEFORE_FIRST_TURN,
    EXEC_BATTLE_EVENT_START_OF_TURN,
};

// enum that returns from execBattleEvents to communicate if a rendering is needed or if all battleEvents were done correctly

enum {
    EXEC_BATTLE_EVENTS_ALL_CLEAR,
    EXEC_BATTLE_EVENTS_NEEDS_SCRIPT_CALL,
};


#endif // GUARD_BATTLE_EVENTS_H