#include "behavior/ability/behavior.hh"

extern "C" {
#include "global.h"
#include "battle_util.h"
}
#define ENUM_AND(enumType) \
    inline enumType operator&(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }

ENUM_AND(ApplyOn)
ENUM_AND(ApplyOnTarget)

template <typename T>
AbilityEnum HasAbilityWithTag(int battler, bool checkMoldBreaker = std::is_assignable_v<Breakable, T>) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        FILTER(dispatchTo<T>(ability))
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        return ability;
    }
    return ABILITY_NONE;
}

template <AbilityEnum Id>
AbilityEnum HasAbilityOrClone(int battler, bool checkMoldBreaker = std::is_assignable_v<Breakable, AbilityImpl<Id>>) {
    return HasAbilityWithTag<AbilityImpl<Id>>(battler, checkMoldBreaker);
}

template <AbilityEnum Id>
AbilityEnum HasAbilityOrCloneMatchingCondition(int battler,
                                               bool (*predicate)(const AbilityImpl<Id>*),
                                               bool checkMoldBreaker = std::is_assignable_v<Breakable, AbilityImpl<Id>>) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        const auto ptr = dispatchTo<AbilityImpl<Id>>(ability);
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        FILTER(ptr && predicate(ptr))
        return ability;
    }
    return ABILITY_NONE;
}

inline bool CheckApplyOn(ApplyOn actual, ApplyOn expected) {
    if (expected == ApplyOn::SELF) {
        return (actual & ApplyOn::IGNORE_SELF) == ApplyOn::SELF;
    } else {
        return static_cast<int>(actual & expected) > 0;
    }
}

template <AbilityEnum Id>
int GetAbilityOrCloneIndex(int battler, bool checkMoldBreaker = std::is_assignable_v<Breakable, AbilityImpl<Id>>) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        FILTER(dispatchTo<AbilityImpl<Id>>(ability))
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        return i;
    }
    return -1;
}

template <AbilityEnum Id>
u32 GetSharedAbilityState(int battler) {
    int idx = GetAbilityOrCloneIndex<Id>(battler);
    return idx >= 0 ? GetAbilityStateByIndex(battler, idx) : 0;
}

template <AbilityEnum Id>
void SetSharedAbilityState(int battler, u32 state) {
    int idx = GetAbilityOrCloneIndex<Id>(battler);
    SetAbilityStateByIndex(battler, idx, state);
}

template <AbilityEnum Id>
int IsAbilityOnOppositeSide(int battler) {
    if (IsBattlerAlive(BATTLE_OPPOSITE(battler)) && HasAbilityOrClone<Id>(BATTLE_OPPOSITE(battler))) return BATTLE_OPPOSITE(battler) + 1;
    if (IsBattlerAlive(BATTLE_OPPOSITE(BATTLE_PARTNER(battler))) && HasAbilityOrClone<Id>(BATTLE_OPPOSITE(BATTLE_PARTNER(battler)))) return BATTLE_OPPOSITE(BATTLE_PARTNER(battler)) + 1;
    return 0;
}

AbilityEnum HasChloroplast(int battler) { return HasAbilityOrClone<ABILITY_CHLOROPLAST>(battler); }
int IsBreakable(AbilityEnum ability) { return dispatchTo<Breakable>(ability) && !dispatchTo<OverrideBreakable>(ability); }
int IsUnsuppressable(AbilityEnum ability) { return dispatchTo<Unsuppressable>(ability) != nullptr; }
int IsPersistentOrUnsuppressable(AbilityEnum ability) { return IsUnsuppressable(ability) || dispatchTo<Persistent>(ability); }
int IsRandomizerBanned(AbilityEnum ability) { return dispatchTo<RandomizerBanned>(ability) != nullptr; }
AbilityEnum HasUnaware(int battler) { return HasAbilityOrClone<ABILITY_UNAWARE>(battler); }
AbilityEnum IsSoundproof(int battler) {
    AbilityEnum baseIsSoundproof = HasAbilityOrClone<ABILITY_SOUNDPROOF>(battler);
    if (baseIsSoundproof) return baseIsSoundproof;
    if (IsBattlerAlive(BATTLE_PARTNER(battler)))
        return HasAbilityOrCloneMatchingCondition<ABILITY_SOUNDPROOF>(
            BATTLE_PARTNER(battler), +[](const AbilityImpl<ABILITY_SOUNDPROOF>* it) { return CheckApplyOn(it->onImmuneFor(), ApplyOn::ALLY); });
    return ABILITY_NONE;
}
AbilityEnum HasNoRecoil(int battler) { return HasAbilityOrClone<ABILITY_ROCK_HEAD>(battler, false); }
int RecoilReductionCount(int battler) {
    int count = 0;
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        FILTER(dispatchTo<AbilityImpl<ABILITY_LIMBER>>(ability))
        FILTER_NOT(IsSuppressed(battler, ability, false))
        count++;
    }
    return count;
}
AbilityEnum HasSkillLink(int battler) { return HasAbilityOrClone<ABILITY_SKILL_LINK>(battler); }
int ResistsFortKnox(AbilityEnum ability) { return dispatchTo<IgnoresFortKnox>(ability) != nullptr; }
AbilityEnum HasAdaptability(int battler) { return HasAbilityOrClone<ABILITY_ADAPTABILITY>(battler); }
AbilityEnum HasMagicGuard(int battler) { return HasAbilityOrClone<ABILITY_MAGIC_GUARD>(battler); }
AbilityEnum HasMagicBounce(int battler) { return HasAbilityOrClone<ABILITY_MAGIC_BOUNCE>(battler); }
int IsGroundImmuneAbility(AbilityEnum ability) { return dispatchTo<GroundImmune>(ability) != nullptr; }
AbilityEnum HasGroundImmuneAbility(int battler) { return HasAbilityWithTag<GroundImmune>(battler); }
AbilityEnum HasMegaLauncherBoost(int battler) { return HasAbilityOrClone<ABILITY_MEGA_LAUNCHER>(battler); }
AbilityEnum HasUnnerve(int battler) { return HasAbilityOrClone<ABILITY_UNNERVE>(battler); }
AbilityEnum IgnoresBurnAttackDrop(int battler) { return HasAbilityWithTag<NegateBurnAtkDrop>(battler); }
AbilityEnum IgnoresFrostbiteSpatkDrop(int battler) { return HasAbilityWithTag<NegateFrzSpatkDrop>(battler); }
AbilityEnum CanInfatuateAny(int battler) { return HasAbilityWithTag<InfatuatesAny>(battler); }
int RemovesStatusAtTurnEnd(AbilityEnum ability) { return dispatchTo<RemovesStatusOnImmunity>(ability) != nullptr; }
AbilityEnum IsTauntImmune(int battler) { return HasAbilityWithTag<TauntImmune>(battler); }
AbilityEnum FoesMinRoll(int battler) {
    AbilityEnum ability = HasAbilityWithTag<ForcesMinRolls>(battler);
    if (ability != ABILITY_NONE) return ability;
    if (IsBattlerAlive(BATTLE_PARTNER(battler))) return HasAbilityWithTag<ForcesMinRolls>(BATTLE_PARTNER(battler));
    return ABILITY_NONE;
}
AbilityEnum HasPowderImmuneAbility(int battler) { return HasAbilityWithTag<PowderImmune>(battler); }
AbilityEnum HasSandImmuneAbility(int battler) {
    AbilityEnum ability = HasAbilityWithTag<SandImmune>(battler);
    if (ability) return ability;
    if (IsBattlerAlive(BATTLE_PARTNER(battler))) return HasAbilityOrClone<ABILITY_DESERT_CLOAK>(BATTLE_PARTNER(battler), false);
    return ABILITY_NONE;
}
AbilityEnum HasHailImmuneAbility(int battler) { return HasAbilityWithTag<HailImmune>(battler); }
AbilityEnum IsToxicTerrainImmune(int battler) { return HasAbilityWithTag<ToxicTerrainImmune>(battler); }
AbilityEnum IsStealthRockImmune(int battler) { return HasAbilityWithTag<StealthRockImmune>(battler); }
int IsAbsorbUp2(AbilityEnum ability) { return dispatchTo<AbsorbUp2>(ability) != nullptr; }
AbilityEnum HasSturdy(int battler) { return HasAbilityOrClone<ABILITY_STURDY>(battler); }

AbilityEnum HasMyceliumMight(int battler) { return HasAbilityOrClone<ABILITY_MYCELIUM_MIGHT>(battler); }
AbilityEnum HasGoodAsGold(int battler) { return HasAbilityOrClone<ABILITY_GOOD_AS_GOLD>(battler); }
AbilityEnum HasComatose(int battler) { return HasAbilityOrClone<ABILITY_COMATOSE>(battler); }
int HasCowardTriggered(int battler) {
    int idx = GetAbilityOrCloneIndex<ABILITY_COWARD>(battler);
    return idx >= 0 ? GetSingleUseAbilityCountByIndex(battler, idx) : 0;
}
AbilityEnum HasQuickFeet(int battler) { return HasAbilityOrClone<ABILITY_QUICK_FEET>(battler); }
AbilityEnum HasQuickDraw(int battler) { return HasAbilityOrClone<ABILITY_QUICK_DRAW>(battler); }
AbilityEnum IgnoresEvasion(int battler) { return HasAbilityWithTag<BlocksStatDrops<STAT_ACC>>(battler); }
int GetAvailableAnticipationIndex(int target) {
    for (int i = ARRAY_COUNT(gBattleMons[target].abilities); i--;) {
        auto ability = gBattleMons[target].abilities[i];
        FILTER(dispatchTo<AbilityImpl<ABILITY_ANTICIPATION>>(ability))
        FILTER_NOT(IsSuppressed(target, ability, std::is_assignable_v<Breakable, AbilityImpl<ABILITY_ANTICIPATION>>))
        FILTER_NOT(GetSingleUseAbilityCountByIndex(target, i))
        return i;
    }
    return -1;
}
int IsPressureAffected(int battler) { 
    CHECK_NOT(HasAbilityOrClone<ABILITY_PRESSURE>(battler));
    return IsAbilityOnOppositeSide<ABILITY_PRESSURE>(battler); 
}
u32 GetUnburdenState(int battler) { return GetSharedAbilityState<ABILITY_UNBURDEN>(battler); }
void SetUnburdenState(int battler, u32 value) { SetSharedAbilityState<ABILITY_UNBURDEN>(battler, value); }
AbilityEnum HasMirrorArmor(int battler) { return HasAbilityOrClone<ABILITY_MIRROR_ARMOR>(battler); }
AbilityEnum HasShieldDust(int battler) { return HasAbilityOrClone<ABILITY_SHIELD_DUST>(battler); }
AbilityEnum HasInnerFocus(int battler) { return HasAbilityOrClone<ABILITY_INNER_FOCUS>(battler); }
AbilityEnum HasGrappler(int battler) { return HasAbilityOrClone<ABILITY_GRAPPLER>(battler); }
AbilityEnum HasBadCompany(int battler) { return HasAbilityOrClone<ABILITY_BAD_COMPANY>(battler); }
AbilityEnum HasMinionControl(int battler) { return HasAbilityOrClone<ABILITY_MINION_CONTROL>(battler); }
AbilityEnum HasAccelerate(int battler) { return HasAbilityOrClone<ABILITY_ACCELERATE>(battler); }
AbilityEnum HasContrary(int battler) { return HasAbilityOrClone<ABILITY_CONTRARY>(battler); }
AbilityEnum HasClearBody(int battler) { return HasAbilityOrClone<ABILITY_CLEAR_BODY>(battler); }
AbilityEnum BlocksAllStatDrops(int battler, int fromSelf) {
    AbilityEnum result;
    if ((result = HasAbilityOrClone<ABILITY_CLEAR_BODY>(battler))) return result;
    if (fromSelf && (result = HasAbilityWithTag<BlocksSelfStatDrops>(battler))) return result;
    return ABILITY_NONE;
}
AbilityEnum BlocksStatDropsOfType(int battler, int stat) {
    switch (stat) {
        case STAT_ATK:
            return HasAbilityWithTag<BlocksStatDrops<STAT_ATK>>(battler);
        case STAT_DEF:
            return HasAbilityWithTag<BlocksStatDrops<STAT_DEF>>(battler);
        case STAT_SPATK:
            return HasAbilityWithTag<BlocksStatDrops<STAT_SPATK>>(battler);
        case STAT_SPDEF:
            return HasAbilityWithTag<BlocksStatDrops<STAT_SPDEF>>(battler);
        case STAT_SPEED:
            return HasAbilityWithTag<BlocksStatDrops<STAT_SPEED>>(battler);
        case STAT_ACC:
            return IgnoresEvasion(battler);
    }
    return ABILITY_NONE;
}
