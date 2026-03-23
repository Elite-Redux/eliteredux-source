
extern "C" {
#include "global.h"
#include "generated/constants/abilities.h"
#include "generated/constants/battle_move_effects.h"
#include "generated/constants/hold_effects.h"
#include "generated/constants/items.h"
#include "generated/constants/species.h"
#include "battle.h"
#include "battle_anim.h"
}

#include "abilities.hh"
#include "type_utils.hh"

#define BATTLER_NONE MAX_BATTLERS_COUNT
#define BATTLER_ATTACKER battlerAtk
#define BATTLER_TARGET battlerDef

int StatusCondition(Status1 status, u8 battler) {
    if (gBattleMons[battler].status1 & status) return TRUE;
    if (status & STATUS1_SLEEP && IsComatose(battler)) return TRUE;
    if (status & STATUS1_BLEED && IsBloodStainAffected(battler)) return TRUE;
    return FALSE;
}

int StatusCondition(Status2 status, u8 battler) { return gBattleMons[battler].status2 & status; }

int WeatherCondition(Weather weather, u8 battler, bool forMove) {
    if (battler == BATTLER_NONE)
        return IsWeatherActive(weather);
    else if (IsBattlerWeatherAffected(battler, weather))
        return TRUE;
    else if (forMove) {
        if (weather & WEATHER_SUN_ANY && HasChloroplast(battler)) return TRUE;
        if (weather & WEATHER_HAIL_ANY && HasAuroraBorealis(battler)) return TRUE;
    }
    return FALSE;
}

int Damaged(u8 battler, u8 by) {
    if (!gRoundStructs[battler].damaged) return FALSE;
    if (by == BATTLER_NONE) return FALSE;
    return gRoundStructs[battler].physicalBattlerId == by || gRoundStructs[battler].specialBattlerId == by;
}

int Switching(u8 battler) { return gActionsByTurnOrder[GetBattlerTurnOrderNum(battler)] == B_ACTION_SWITCH; }

int ActsAfter(u8 before, u8 after, bool failIfSwitching) {
    int condition;
    if (after == BATTLER_NONE && before == BATTLER_NONE) return FALSE;

    if (before == BATTLER_NONE)
        condition = gCurrentTurnActionNumber > GetBattlerTurnOrderNum(after);
    else if (after == BATTLER_NONE)
        condition = gCurrentTurnActionNumber < GetBattlerTurnOrderNum(before);
    else
        condition = GetBattlerTurnOrderNum(after) > GetBattlerTurnOrderNum(before);

    if (condition && failIfSwitching && before != BATTLER_NONE && gVolatileStructs[before].isFirstTurn == 2) return FALSE;

    return condition;
}

int TerrainCondition(TerrainType terrain, u8 battler) {
    u32 flag = TerrainTypeToFieldStatus(terrain);
    if (!flag) {
        if (battler == BATTLER_NONE)
            return !IsTerrainActive(STATUS_FIELD_TERRAIN_ANY);
        else
            return !IsBattlerTerrainAffected(battler, STATUS_FIELD_TERRAIN_ANY);
    } else {
        if (battler == BATTLER_NONE)
            return IsTerrainActive(flag);
        else
            return IsBattlerTerrainAffected(battler, flag);
    }
}

typedef enum FieldEffect {
    FIELD_EFFECT_GRAVITY = 1,
} FieldEffect;

int FieldEffectCondition(FieldEffect effect) {
    switch (effect) {
        case FIELD_EFFECT_GRAVITY:
            return IsGravityActive();
        default:
            return FALSE;
    }
}

int AbilityCondition(AbilityEnum ability, u8 battler, bool checkMoldBreaker) { return BattlerHasAbility(battler, ability, (int)checkMoldBreaker); }

typedef enum HpStatus {
    HP_LOW = 1,
} HpStatus;

int Hp(HpStatus type, u8 battler) {
    switch (type) {
        case HP_LOW:
            return gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2;
        default:
            return FALSE;
    }
}

int RecentFainted(u8 battler) { return gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer == 1; }

int SpeciesCondition(SpeciesEnum species, u8 battler, bool exact) {
    if (exact)
        return gBattleMons[battler].species == species;
    else
        return GET_BASE_SPECIES_ID(gBattleMons[battler].species) == species;
}

inline int ItemCondition(ItemEnum item, u8 battler, bool checkDisabling) {
    if (gBattleMons[battler].item != item) return FALSE;
    if (!checkDisabling) return TRUE;
    return !IsItemNegated(battler);
}

inline int ItemCondition(HoldEffectEnum holdEffect, u8 battler, bool checkDisabling) {
    return GetBattlerHoldEffect(battler, (int)checkDisabling) == holdEffect;
}

#define CUSTOM_MOVE_CONDITION_CALL BATTLER_ATTACKER, BATTLER_TARGET, move
#define CUSTOM_MOVE_CONDITION_PARAMS opt u8 battlerAtk, opt u8 battlerDef, opt MoveEnum move

template <MoveBehaviorEnum Case>
int CustomMoveCondition(CUSTOM_MOVE_CONDITION_PARAMS) = delete;

#define CUSTOM_MOVE_DAMAGE_CALL baseDamage, BATTLER_ATTACKER, BATTLER_TARGET, move
#define CUSTOM_MOVE_DAMAGE_PARAMS opt u32 baseDamage, opt u8 battlerAtk, opt u8 battlerDef, opt MoveEnum move

template <MoveBehaviorEnum Case>
u32 CustomMoveDamage(CUSTOM_MOVE_DAMAGE_PARAMS) = delete;

template <>
int CustomMoveCondition<EFFECT_FUSION_COMBO>(CUSTOM_MOVE_CONDITION_PARAMS) {
    return gBattleMoves[gLastUsedMove].effect == EFFECT_FUSION_COMBO && move != gLastUsedMove;
}

template <>
int CustomMoveCondition<EFFECT_ACROBATICS>(CUSTOM_MOVE_CONDITION_PARAMS) {
    return gBattleMons[battlerAtk].item == ITEM_NONE
           // Edge case, because removal of items happens after damage calculation.
           || (gTurnStructs[battlerAtk].gemBoost && GetBattlerHoldEffect(battlerAtk, FALSE) == HOLD_EFFECT_GEMS);
}

template <>
int CustomMoveCondition<EFFECT_ROUND>(CUSTOM_MOVE_CONDITION_PARAMS) {
    return gChosenMoveByBattler[BATTLE_PARTNER(battlerAtk)] == MOVE_ROUND && !(gAbsentBattlerFlags & 1 << BATTLE_PARTNER(battlerAtk));
}

template <>
u32 CustomMoveDamage<EFFECT_ERUPTION>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    return gBattleMons[battlerAtk].hp * baseDamage / gBattleMons[battlerAtk].maxHP;
}

static const u16 sWeightToDamageTable[] = {100, 20, 250, 40, 500, 60, 1000, 80, 2000, 100, 0xFFFF, 0xFFFF};

template <>
u32 CustomMoveDamage<EFFECT_LOW_KICK>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    int weight = GetBattlerWeight(battlerDef);

    for (int i = 0; i < ARRAY_COUNT(sWeightToDamageTable); i += 2) {
        if (sWeightToDamageTable[i] > weight) return sWeightToDamageTable[i + 1];
    }
    return 120;
}

static const u8 sHeatCrashPowerTable[] = {40, 40, 60, 80, 100, 120};

template <>
u32 CustomMoveDamage<EFFECT_HEAT_CRASH>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    int weightRatio = GetBattlerWeight(battlerAtk) / GetBattlerWeight(battlerDef);
    if (weightRatio >= ARRAY_COUNT(sHeatCrashPowerTable))
        return sHeatCrashPowerTable[ARRAY_COUNT(sHeatCrashPowerTable) - 1];
    else
        return sHeatCrashPowerTable[weightRatio];
}

template <>
u32 CustomMoveDamage<EFFECT_PUNISHMENT>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    int basePower = 60 + (CountBattlerStatIncreases(battlerDef, FALSE) * 20);
    if (basePower > 200) basePower = 200;
    return basePower;
}

template <>
u32 CustomMoveDamage<EFFECT_STORED_POWER>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    return baseDamage + CountBattlerStatIncreases(battlerAtk, TRUE) * 20;
}

static const u8 sSpeedDiffPowerTable[] = {40, 60, 80, 120, 150};

template <>
u32 CustomMoveDamage<EFFECT_ELECTRO_BALL>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    int speed = GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL, move) / GetBattlerTotalSpeedStat(battlerDef, TOTAL_SPEED_FULL, move);
    if (speed >= ARRAY_COUNT(sSpeedDiffPowerTable)) return ARRAY_COUNT(sSpeedDiffPowerTable) - 1;
    return sSpeedDiffPowerTable[speed];
}

template <>
u32 CustomMoveDamage<EFFECT_GYRO_BALL>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    int basePower = ((25 * GetBattlerTotalSpeedStat(battlerDef, TOTAL_SPEED_FULL, move)) / GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL, move)) + 1;
    if (basePower > 150) basePower = 150;
    return basePower;
}

template <>
u32 CustomMoveDamage<EFFECT_ECHOED_VOICE>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    baseDamage *= (1 + gBattleStruct->sameMoveTurns[battlerAtk]);
    if (baseDamage > 200) baseDamage = 200;
    return baseDamage;
}

template <>
u32 CustomMoveDamage<EFFECT_LASH_OUT>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    baseDamage += CountBattlerStatDecreases(battlerAtk) * 20;
    if (baseDamage > 140) baseDamage = 140;
    return baseDamage;
}

static const u8 sTrumpCardPowerTable[] = {200, 80, 60, 50, 40};

template <>
u32 CustomMoveDamage<EFFECT_TRUMP_CARD>(CUSTOM_MOVE_DAMAGE_PARAMS) {
    int i = GetBattleMonMoveSlot(&gBattleMons[battlerAtk], move);
    if (i != 4) {
        if (gBattleMons[battlerAtk].pp[i] >= ARRAY_COUNT(sTrumpCardPowerTable))
            baseDamage = sTrumpCardPowerTable[ARRAY_COUNT(sTrumpCardPowerTable) - 1];
        else
            baseDamage = sTrumpCardPowerTable[gBattleMons[battlerAtk].pp[i]];
    }
}

#include "generated/data/move_damage_modifiers.hh"
