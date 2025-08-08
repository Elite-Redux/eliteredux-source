#include "behavior/ability/behavior.hh"

extern "C" {
#include "global.h"
}
#define ENUM_AND(enumType) \
    inline enumType operator&(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }

ENUM_AND(ApplyOn)
ENUM_AND(ApplyOnTarget)

// void onAbility(int battler, bool checkMoldBreaker, bool (*predicate)(AbilityEnum)) {
//     for (int idx = TOTAL_ABILITY_COUNT - 1; idx >= 0; idx--) {
//         AbilityEnum ability = gBattleMons[battler].abilities[idx];
//         FILTER(predicate(ability))
//         FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
//         callback;
//     }
// }

template <typename T>
AbilityEnum HasAbilityWithTag(int battler, bool checkMoldBreaker = std::is_assignable_v<Breakable, T>) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        FILTER(IsSuppressed(battler, ability, checkMoldBreaker))
        if (dispatchTo<T>(ability)) return ability;
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
        FILTER(IsSuppressed(battler, ability, checkMoldBreaker))
        const auto ptr = dispatchTo<AbilityImpl<Id>>(ability);
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
        FILTER(IsSuppressed(battler, ability, false))
        if (dispatchTo<AbilityImpl<ABILITY_LIMBER>>(ability)) return count++;
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
