#include "global.h"
#include "gba/gba.h"
#include "battle_events.h"
#include "constants/battle_events.h"
#include "battle_main.h"
#include "battle.h"

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

void changeStats(u8 battler, u8 stat, s8 stages){
    u32 ability;
    if (stages == 0){
        // maybe for debugging to yell in the console or something because something went wrong
        return; // may not be necessary, just better safe imo
    }
    ability = GetBattlerAbility(battler);

    // Handle Contrary and Simple
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_CONTRARY, ability)){
        stages *= -1;
    }
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_SIMPLE, ability)){
        stages = min(max(stages * 2, -6), 6);
    }
    // Handle stats reductions immunity abilities
    if (!gSideTimers[GET_BATTLER_SIDE(battler)].mistTimer
            && !BATTLER_HAS_ABILITY_FAST(battler, ABILITY_CLEAR_BODY, ability)
            && !BATTLER_HAS_ABILITY_FAST(battler, ABILITY_FULL_METAL_BODY, ability)
            && !BATTLER_HAS_ABILITY_FAST(battler, ABILITY_WHITE_SMOKE, ability)
            && !(BATTLER_HAS_ABILITY_FAST(battler, ABILITY_KEEN_EYE, ability) && currStat == STAT_ACC)
            && !(BATTLER_HAS_ABILITY_FAST(battler, ABILITY_MINDS_EYE, ability) && currStat == STAT_ACC)
            && !(BATTLER_HAS_ABILITY_FAST(battler, ABILITY_HYPER_CUTTER, ability) && currStat == STAT_ATK)
            && GetBattlerHoldEffect(battler, TRUE) != HOLD_EFFECT_CLEAR_AMULET)
    {
        if (stages < 0)
        {
            return
        }
    }
    // this seems to be to show the animation
    // maybe having a statAnimId with customization for this one?
    //BtlController_EmitBattleAnimation(0, B_ANIM_STATS_CHANGE, statAnimId);
    //MarkBattlerForControllerExec(battler);
}

void battleEventsMegaSwitch(u8 battleEvent){
    u8 i;
    if (gBattleResults.battleTurnCounter > 0) return;
    // TURN 0 switch statements
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEALTH_ROCK_START:
        gSideStatuses[B_SIDE_PLAYER] |= ~(SIDE_STATUS_STEALTH_ROCK);
        break;
    case BATTLE_EVENT_LAST_PARALYZED:
        for (i = gPlayerPartyCount - 1; gPlayerPartyCount > 0 ; i--){
            if (gPlayerParty[i].status == STATUS1_NONE){
                gPlayerParty[i].status = STATUS1_PARALYSIS;
            }
        }
        break;
    case BATTLE_EVENT_LEADER_1RST_PLUS_FOUR_DEF_BOOST:
        gBattleMons[B_POSITION_OPPONENT_LEFT].statStages[STAT_DEF] = 4;
        break;
    }

}