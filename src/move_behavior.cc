extern "C" {
#include "global.h"
#include "generated/constants/abilities.h"
#include "generated/constants/battle_move_effects.h"
#include "generated/constants/hold_effects.h"
#include "generated/constants/items.h"
#include "generated/constants/moves.h"
#include "generated/constants/move_effects.h"
#include "generated/constants/species.h"
#include "battle_util.h"
#include "battle.h"
#include "item.h"
#include "battle_anim.h"
#include "random.h"
}

#undef STAT_HP
#undef STAT_ATK
#undef STAT_DEF
#undef STAT_SPEED
#undef STAT_SPATK
#undef STAT_SPDEF
#undef STAT_ACC
#undef STAT_EVASION
#undef WEATHER_ANY

#include "generated/data/behavior/move/enums.hh"

int GetStandardBattlerTarget(BattlerTarget target, u8 battlerAtk, u8 battlerDef) {
    switch (target) {
        case BattlerTarget::TARGET_LOOP_TARGET:
        case BattlerTarget::TARGET_MOVE_TARGET:
            return battlerDef;
        case BattlerTarget::TARGET_FOE:
            return BATTLE_OPPOSITE(battlerAtk);
        case BattlerTarget::TARGET_FOE_PARTNER:
            return BATTLE_PARTNER(BATTLE_OPPOSITE(battlerAtk));
        case BattlerTarget::TARGET_MOVE_USER:
            return battlerAtk;
        case BattlerTarget::TARGET_PARTNER:
            return BATTLE_PARTNER(battlerAtk);
        default:
            return MAX_BATTLERS_COUNT;
    }
}

template <>
int resolveMoveRequirementAbility<ABILITY_CHLOROPLAST>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return HasChloroplast(battler);
}

template <>
int resolveMoveRequirementAbility<ABILITY_AURORA_BOREALIS>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return BattlerHasAbility(battler, ABILITY_PARENTAL_BOND, FALSE);
}

template <>
int resolveMoveRequirementAbility<ABILITY_PARENTAL_BOND>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return BattlerHasAbility(battler, ABILITY_PARENTAL_BOND, FALSE);
}

template <SpeciesEnum Species>
int resolveMoveRequirementSpecies(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    return GET_BASE_SPECIES_ID(gBattleMons[GetStandardBattlerTarget(target, battlerAtk, battlerDef)].species) == Species;
}

template <SpeciesEnum Species>
int resolveMoveRequirementSpecies(BattlerTarget target, MoveEnum move, struct Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2) {
    if (target != BattlerTarget::TARGET_MOVE_USER) return FALSE;
    return species == Species;
}

int resolveMoveRequirementHoldEffectMatchesArgument(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return ItemId_GetHoldEffect(gBattleMons[battler].item) == static_cast<HoldEffectEnum>(gBattleMoves[move].argument);
}

template <HoldEffect Effect>
int resolveMoveRequirementHoldEffect(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return ItemId_GetHoldEffect(gBattleMons[battler].item) == static_cast<HoldEffectEnum>(Effect);
}

Type resolveTypeChangeHiddenPower(MoveEnum move, u8 battler) { return gBattleMons[battler].hpType; }

Type resolveTypeChangeItemType(MoveEnum move, u8 battler) { return static_cast<Type>(ItemId_GetSecondaryId(gBattleMons[battler].item)); }

template <ItemEnum Item>
int resolveMoveRequirementItem(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gBattleMons[battler].item == Item && !IsItemNegated(battler);
}

Type resolveTypeChangeFirstType(MoveEnum move, u8 battler) {
    if (gBattleMons[battler].type1 != TYPE_MYSTERY) return gBattleMons[battler].type1;
    if (gBattleMons[battler].type2 != TYPE_MYSTERY) return gBattleMons[battler].type2;
    return gBattleMons[battler].type3;
}

Type resolveTypeChangeSecondType(MoveEnum move, u8 battler) {
    if (gBattleMons[battler].type2 != TYPE_MYSTERY) return gBattleMons[battler].type2;
    if (gBattleMons[battler].type1 != TYPE_MYSTERY) return gBattleMons[battler].type1;
    return gBattleMons[battler].type3;
}

Type resolveTypeChangeWeather(MoveEnum move, u8 battler) {
    if (resolveMoveRequirementAbility<ABILITY_CHLOROPLAST>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_FIRE;
    if (resolveMoveRequirementAbility<ABILITY_AURORA_BOREALIS>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_FIRE;
    if (!IsWeatherActive(WEATHER_ANY_VALUE)) return TYPE_NORMAL;
    if (gBattleWeather & WEATHER_FOG_ANY) return TYPE_GHOST;
    if (gBattleWeather & WEATHER_SUN_ANY) return TYPE_FIRE;
    if (gBattleWeather & WEATHER_RAIN_ANY) return TYPE_WATER;
    if (gBattleWeather & WEATHER_SANDSTORM_ANY) return TYPE_ROCK;
    if (gBattleWeather & WEATHER_HAIL_ANY) return TYPE_ICE;
    return TYPE_NORMAL;
}

Type resolveTypeChangeNaturalGift(MoveEnum move, u8 battler) { return gNaturalGiftTable[gBattleMons[battler].item - FIRST_BERRY_INDEX].type; }

Type resolveTypeChangeTerrain(MoveEnum move, u8 battler) {
    if (resolveMoveRequirementTerrainAffected<Terrain::TERRAIN_ELECTRIC>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_ELECTRIC;
    if (resolveMoveRequirementTerrainAffected<Terrain::TERRAIN_GRASSY>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_GRASS;
    if (resolveMoveRequirementTerrainAffected<Terrain::TERRAIN_MISTY>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_FAIRY;
    if (resolveMoveRequirementTerrainAffected<Terrain::TERRAIN_PSYCHIC>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_PSYCHIC;
    if (resolveMoveRequirementTerrainAffected<Terrain::TERRAIN_TOXIC>(BattlerTarget::TARGET_MOVE_USER, move, battler, battler)) return TYPE_POISON;
    return TYPE_NORMAL;
}

template <Type T>
Type resolveTypeChangeType(MoveEnum move, u8 battler) {
    return T;
}

int resolveMoveRequirementHoldEffectMatchesArgument(
    BattlerTarget target, MoveEnum move, Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2) {
    if (target != BattlerTarget::TARGET_MOVE_USER) return FALSE;
    return ItemId_GetHoldEffect(item) == static_cast<HoldEffectEnum>(gBattleMoves[move].argument);
}

Type resolveTypeChangeItemType(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) { return static_cast<Type>(ItemId_GetSecondaryId(item)); }

Type resolveTypeChangeHiddenPower(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) {
    return static_cast<Type>(GetMonData(mon, MON_DATA_HP_TYPE));
}

template <HoldEffect Effect>
int resolveMoveRequirementHoldEffect(BattlerTarget target, MoveEnum move, Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2) {
    if (target != BattlerTarget::TARGET_MOVE_USER) return FALSE;
    return ItemId_GetHoldEffect(item) == static_cast<HoldEffectEnum>(Effect);
}

Type resolveTypeChangeFirstType(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) { return type1 != TYPE_MYSTERY ? type1 : type2; }

template <Type T>
Type resolveTypeChangeType(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) {
    return T;
}

Type resolveTypeChangeWeather(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) { return TYPE_NORMAL; }

Type resolveTypeChangeTerrain(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) { return TYPE_NORMAL; }

Type resolveTypeChangeNaturalGift(MoveEnum move, Pokemon* mon, ItemEnum item, Type type1, Type type2) {
    return gNaturalGiftTable[item - FIRST_BERRY_INDEX].type;
}

int resolveAdjustPowerFlat(int basePower, u8 battlerAtk, u8 battlerDef, int flat) { return flat; }

int resolveAdjustPowerHitCount(int basePower, u8 battlerAtk, u8 battlerDef) { return basePower * (4 - gTurnStructs[battlerAtk].multiHitCounter); }

int resolveAdjustPowerMultiplier(int basePower, u8 battlerAtk, u8 battlerDef, float multiplier) { return basePower * multiplier; }

int resolveAdjustPowerNaturalGift(int basePower, u8 battlerAtk, u8 battlerDef) {
    if (ItemId_GetPocket(gBattleMons[battlerAtk].item) == POCKET_BERRIES) {
        return gNaturalGiftTable[gBattleMons[battlerAtk].item - FIRST_BERRY_INDEX].power;
    } else {
        return 0;
    }
}

template <BattlerTarget Target>
int resolveAdjustPowerPercentHp(int basePower, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(Target, battlerAtk, battlerDef);
    return basePower * gBattleMons[battler].hp / gBattleMons[battler].maxHP;
}

int resolveAdjustPowerRolloutCounter(int basePower, u8 battlerAtk, u8 battlerDef) { return basePower << (gVolatileStructs[battlerAtk].rolloutCounter - 1); }

int resolveMoveRequirementFusionCombo(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    return gBattleMoves[gLastUsedMove].effect == EFFECT_FUSION_COMBO && move != gLastUsedMove;
}

template <>
int resolveMoveRequirementStatus<MoveStatusEffect::STATUS_SLEEP>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    // TODO: Make this HasComatose
    return gBattleMons[battler].status1 & STATUS1_SLEEP || BattlerHasAbility(battler, ABILITY_COMATOSE, FALSE);
}

template <>
int resolveMoveRequirementStatus<MoveStatusEffect::STATUS_POISON>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return IsPoisonedForMove(battler);
}

template <>
int resolveMoveRequirementStatus<MoveStatusEffect::STATUS_BLEED>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gBattleMons[battler].status1 & STATUS1_BLEED || IsBloodStainAffected(battler);
}

template <>
int resolveMoveRequirementStatus<MoveStatusEffect::STATUS_STATUS1_ANY>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return HasAnyStatusOrAbility(battler);
}

template <>
int resolveMoveRequirementStatus<MoveStatusEffect::STATUS_ATTRACT>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gBattleMons[battler].status2 & STATUS2_INFATUATION;
}

template <>
int resolveMoveRequirementStatus<MoveStatusEffect::STATUS_GRAVITY>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    return gFieldTimers.gravityTimer;
}

int resolveMoveRequirementRelativeSpeed(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef, int threshold) {
    if (target == BattlerTarget::TARGET_MOVE_USER) {
        return (GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL) / GetBattlerTotalSpeedStat(battlerDef, TOTAL_SPEED_FULL)) >= threshold;
    } else {
        return (GetBattlerTotalSpeedStat(GetStandardBattlerTarget(target, battlerAtk, battlerDef), TOTAL_SPEED_FULL) /
                GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL)) >= threshold;
    }
}

int resolveMoveRequirementRelativeWeight(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef, int threshold) {
    if (target == BattlerTarget::TARGET_MOVE_USER) {
        return (GetBattlerWeight(battlerAtk) / GetBattlerWeight(battlerDef)) >= threshold;
    } else {
        return (GetBattlerWeight(GetStandardBattlerTarget(target, battlerAtk, battlerDef)) / GetBattlerWeight(battlerAtk)) >= threshold;
    }
}

template <BattlerTarget DamagedBy>
int resolveMoveRequirementDamagedBy(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    int damagedBy = GetStandardBattlerTarget(DamagedBy, battlerAtk, battlerDef);
    return (gRoundStructs[battler].physicalDmg && gRoundStructs[battler].physicalBattlerId == damagedBy) ||
           (gRoundStructs[battler].specialDmg && gRoundStructs[battler].specialBattlerId == damagedBy);
}

template <>
int resolveMoveRequirementDamagedBy<BattlerTarget::TARGET_ALL>(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gRoundStructs[battler].physicalDmg || gRoundStructs[battler].specialDmg;
}

int resolveMoveRequirementCanLoseItem(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gBattleMons[battler].item != ITEM_NONE && CanBattlerGetOrLoseItem(battler, gBattleMons[battler].item);
}

int resolveMoveRequirementWeight(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef, int weight) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return GetBattlerWeight(battler) >= weight;
}

int resolveMoveRequirementHasArgumentStatus(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    switch (gBattleMoves[move].argument) {
        case STATUS1_BLEED:
            return resolveMoveRequirementStatus<MoveStatusEffect::STATUS_BLEED>(target, move, battlerAtk, battlerDef);
            break;
        case STATUS1_POISON_ANY:
            return resolveMoveRequirementStatus<MoveStatusEffect::STATUS_POISON>(target, move, battlerAtk, battlerDef);
            break;
        case STATUS1_SLEEP:
            return resolveMoveRequirementStatus<MoveStatusEffect::STATUS_SLEEP>(target, move, battlerAtk, battlerDef);
            break;
        default:
            return gBattleMons[GetStandardBattlerTarget(target, battlerAtk, battlerDef)].status1 & gBattleMoves[move].argument;
    }
}

int resolveMoveRequirementHpPercent(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef, int threshold) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gBattleMons[battler].hp * 100 > gBattleMons[battler].maxHP * threshold;
}

template <>
int resolveCustomScalingMeasurement<PowerMeasurement::MEASUREMENT_ATTACKER_NEGATIVE_STAT_BUFFS>(int basePower, u8 battlerAtk, u8 battlerDef) {
    return CountBattlerStatDecreases(battlerAtk);
}

template <>
int resolveCustomScalingMeasurement<PowerMeasurement::MEASUREMENT_ATTACKER_POSITIVE_STAT_BUFFS>(int basePower, u8 battlerAtk, u8 battlerDef) {
    return CountBattlerStatIncreases(battlerAtk, TRUE);
}

template <>
int resolveCustomScalingMeasurement<PowerMeasurement::MEASUREMENT_FAINTED_BATTLERS>(int basePower, u8 battlerAtk, u8 battlerDef) {
    return gFaintedMonCount[GET_BATTLER_SIDE(battlerAtk)];
}

template <>
int resolveCustomScalingMeasurement<PowerMeasurement::MEASUREMENT_TARGET_OVER_ATTACKER_SPEED>(int basePower, u8 battlerAtk, u8 battlerDef) {
    return GetBattlerTotalSpeedStat(battlerDef, TOTAL_SPEED_FULL) / GetBattlerTotalSpeedStat(battlerAtk, TOTAL_SPEED_FULL);
}

template <>
int resolveCustomScalingMeasurement<PowerMeasurement::MEASUREMENT_TARGET_POSITIVE_STAT_BUFFS>(int basePower, u8 battlerAtk, u8 battlerDef) {
    return CountBattlerStatIncreases(battlerDef, TRUE);
}

template <>
int resolveCustomScalingMeasurement<PowerMeasurement::MEASUREMENT_TOOK_DAMAGE>(int basePower, u8 battlerAtk, u8 battlerDef) {
    return gBattleStruct->timesDamaged[gBattlerPartyIndexes[battlerAtk]][GetBattlerSide(battlerAtk)];
}

int resolveMoveRequirementLastMoveFailed(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    return gBattleStruct->lastMoveFailed & 1 << GetStandardBattlerTarget(target, battlerAtk, battlerDef);
}

template <Weather WeatherEnum>
int resolveMoveRequirementWeatherAffected(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    if (target != BattlerTarget::TARGET_FIELD) {
        int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
        switch (WeatherEnum) {
            case Weather::WEATHER_FOG:
                return IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY);
            case Weather::WEATHER_HAIL:
                return IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY) || BattlerHasAbility(battler, ABILITY_AURORA_BOREALIS, FALSE);
            case Weather::WEATHER_RAIN:
                return IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY);
            case Weather::WEATHER_SANDSTORM:
                return IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY);
            case Weather::WEATHER_SUN:
                return IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || HasChloroplast(battler);
            case Weather::WEATHER_ANY:
                return IsBattlerWeatherAffected(battler, WEATHER_ANY_VALUE) || BattlerHasAbility(battler, ABILITY_AURORA_BOREALIS, FALSE) ||
                       HasChloroplast(battler);
        }
    } else {
        switch (WeatherEnum) {
            case Weather::WEATHER_FOG:
                return IsWeatherActive(WEATHER_FOG_ANY);
            case Weather::WEATHER_HAIL:
                return IsWeatherActive(WEATHER_HAIL_ANY);
            case Weather::WEATHER_RAIN:
                return IsWeatherActive(WEATHER_RAIN_ANY);
            case Weather::WEATHER_SANDSTORM:
                return IsWeatherActive(WEATHER_SANDSTORM_ANY);
            case Weather::WEATHER_SUN:
                return IsWeatherActive(WEATHER_SUN_ANY);
            case Weather::WEATHER_ANY:
                return IsWeatherActive(WEATHER_ANY_VALUE);
        }
    }
}

template <Terrain TerrainEnum>
int resolveMoveRequirementTerrainAffected(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    if (target != BattlerTarget::TARGET_FIELD) {
        int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
        switch (TerrainEnum) {
            case Terrain::TERRAIN_ELECTRIC:
                return IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN);
            case Terrain::TERRAIN_GRASSY:
                return IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN);
            case Terrain::TERRAIN_MISTY:
                return IsBattlerTerrainAffected(battler, STATUS_FIELD_MISTY_TERRAIN);
            case Terrain::TERRAIN_PSYCHIC:
                return IsBattlerTerrainAffected(battler, STATUS_FIELD_PSYCHIC_TERRAIN);
            case Terrain::TERRAIN_TOXIC:
                return IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN);
            case Terrain::TERRAIN_ANY:
                return IsBattlerTerrainAffected(battler, STATUS_FIELD_TERRAIN_ANY);
        }
    } else {
        switch (TerrainEnum) {
            case Terrain::TERRAIN_ELECTRIC:
                return IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN);
            case Terrain::TERRAIN_GRASSY:
                return IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN);
            case Terrain::TERRAIN_MISTY:
                return IsTerrainActive(STATUS_FIELD_MISTY_TERRAIN);
            case Terrain::TERRAIN_PSYCHIC:
                return IsTerrainActive(STATUS_FIELD_PSYCHIC_TERRAIN);
            case Terrain::TERRAIN_TOXIC:
                return IsTerrainActive(STATUS_FIELD_TOXIC_TERRAIN);
            case Terrain::TERRAIN_ANY:
                return IsTerrainActive(STATUS_FIELD_TERRAIN_ANY);
        }
    }
}

int resolveMoveRequirementHasTakenTurn(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    return gCurrentTurnActionNumber > GetBattlerTurnOrderNum(GetStandardBattlerTarget(target, battlerAtk, battlerDef));
}

int resolveMoveRequirementRandom(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef, int percent) { return (Random() % 100) < percent; }

int resolveMoveRequirementUsedSameMoveBehavior(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gBattleMoves[gChosenMoveByBattler[battler]].effect == gBattleMoves[move].effect && !(gAbsentBattlerFlags & 1 << battler);
}

int resolveMoveRequirementAllyFaintedRecently(BattlerTarget target, MoveEnum move, u8 battlerAtk, u8 battlerDef) {
    int battler = GetStandardBattlerTarget(target, battlerAtk, battlerDef);
    return gSideTimers[GetBattlerSide(battler)].retaliateTimer == 1;
}

#include "generated/data/behavior/move/base_power.hh"
#include "generated/data/behavior/move/type_change.hh"

extern "C" {
int AdjustMovePowerC(u8 battlerAtk, u8 battlerDef, MoveEnum move, int basePower) { return AdjustMovePower(battlerAtk, battlerDef, move, basePower); }
Type GetBattleMoveTypeC(u8 battlerAtk, u8 battlerDef, MoveEnum move, Type baseType) { return GetBattleMoveType(battlerAtk, battlerDef, move, baseType); }
Type GetOutOfBattleMoveTypeC(struct Pokemon* mon, SpeciesEnum species, ItemEnum item, Type type1, Type type2, MoveEnum move, Type baseType) {
    return GetOutOfBattleMoveType(mon, species, item, type1, type2, move, baseType);
}
}
