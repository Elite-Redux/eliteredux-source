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
/*
void changeStats(u8 battler, u8 stat, u8 stages){
    u32 ability;
    u32 currStat = 0;
    u32 statAnimId = 0;
    u32 changeableStatsCount = 0;
    u32 statsToCheck = 0;
    u32 startingStatAnimId = 0;
    u32 flags = gBattlescriptCurrInstr[3];

    ability = GetBattlerAbility(battler);

    // Handle Contrary and Simple
    if (BATTLER_HAS_ABILITY_FAST(battler, ABILITY_CONTRARY, ability)){
        flags ^= STAT_CHANGE_NEGATIVE;
    }

    if (BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_SIMPLE, ability))
        flags |= STAT_CHANGE_BY_TWO;

    if (gBattlerAttacker != gActiveBattler && BATTLER_HAS_ABILITY_FAST(gBattlerAttacker, ABILITY_SUBDUE, ability))
        flags |= STAT_CHANGE_BY_TWO;

    if (flags & STAT_CHANGE_NEGATIVE) // goes down
    {
        if (flags & STAT_CHANGE_BY_TWO)
            startingStatAnimId = STAT_ANIM_MINUS2 - 1;
        else
            startingStatAnimId = STAT_ANIM_MINUS1 - 1;

        while (statsToCheck != 0)
        {
            if (statsToCheck & 1)
            {
                if (flags & STAT_CHANGE_CANT_PREVENT)
                {
                    if (gBattleMons[gActiveBattler].statStages[currStat] > MIN_STAT_STAGE)
                    {
                        statAnimId = startingStatAnimId + currStat;
                        changeableStatsCount++;
                    }
                }
                else if (!gSideTimers[GET_BATTLER_SIDE(gActiveBattler)].mistTimer
                        && !BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_CLEAR_BODY, ability)
						&& !BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_FULL_METAL_BODY, ability)
                        && !BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_WHITE_SMOKE, ability)
						&& !(BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_KEEN_EYE, ability) && currStat == STAT_ACC)
						&& !(BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_MINDS_EYE, ability) && currStat == STAT_ACC)
                        && !(BATTLER_HAS_ABILITY_FAST(gActiveBattler, ABILITY_HYPER_CUTTER, ability) && currStat == STAT_ATK)
                        && GetBattlerHoldEffect(gActiveBattler, TRUE) != HOLD_EFFECT_CLEAR_AMULET)
                {
                    if (gBattleMons[gActiveBattler].statStages[currStat] > MIN_STAT_STAGE)
                    {
                        statAnimId = startingStatAnimId + currStat;
                        changeableStatsCount++;
                    }
                }
            }
            statsToCheck >>= 1, currStat++;
        }

        if (changeableStatsCount > 1) // more than one stat, so the color is gray
        {
            if (flags & STAT_CHANGE_BY_TWO)
                statAnimId = STAT_ANIM_MULTIPLE_MINUS2;
            else
                statAnimId = STAT_ANIM_MULTIPLE_MINUS1;
        }
    }
    else // goes up
    {
        if (flags & STAT_CHANGE_BY_TWO)
            startingStatAnimId = STAT_ANIM_PLUS2 - 1;
        else
            startingStatAnimId = STAT_ANIM_PLUS1 - 1;

        while (statsToCheck != 0)
        {
            if (statsToCheck & 1 && gBattleMons[gActiveBattler].statStages[currStat] < MAX_STAT_STAGE)
            {
                statAnimId = startingStatAnimId + currStat;
                changeableStatsCount++;
            }
            statsToCheck >>= 1, currStat++;
        }

        if (changeableStatsCount > 1) // more than one stat, so the color is gray
        {
            if (flags & STAT_CHANGE_BY_TWO)
                statAnimId = STAT_ANIM_MULTIPLE_PLUS2;
            else
                statAnimId = STAT_ANIM_MULTIPLE_PLUS1;
        }
    }

    if (flags & STAT_CHANGE_MULTIPLE_STATS && changeableStatsCount < 2)
    {
        gBattlescriptCurrInstr += 4;
    }
    else if (changeableStatsCount != 0 && !gBattleScripting.statAnimPlayed)
    {
        BtlController_EmitBattleAnimation(0, B_ANIM_STATS_CHANGE, statAnimId);
        MarkBattlerForControllerExec(gActiveBattler);
        if (flags & STAT_CHANGE_MULTIPLE_STATS && changeableStatsCount > 1)
            gBattleScripting.statAnimPlayed = TRUE;
        gBattlescriptCurrInstr += 4;
    }
    else
    {
        gBattlescriptCurrInstr += 4;
    }
}*/

void battleEventsMegaSwitch(u8 battleEvent){
    u8 i;
    if (gBattleResults.battleTurnCounter > 0) return;
    // TURN 0 switch statements
    switch (battleEvent)
    {
    case BATTLE_EVENT_NONE:
        break;
    case BATTLE_EVENT_STEALTH_ROCK_START:
        gSideStatuses[B_SIDE_PLAYER] &= ~(SIDE_STATUS_STEALTH_ROCK);
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