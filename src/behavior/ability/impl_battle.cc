#include "behavior/ability/behavior.hh"
#include <tuple>

extern "C" {
#include "global.h"
#include "battle_util.h"
}
#define ENUM_AND(enumType) \
    inline enumType operator&(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }
#define ENUM_NOT(enumType) \
    inline enumType operator~(enumType a) { return static_cast<enumType>(~static_cast<int>(a)); }

ENUM_AND(ApplyOn)
ENUM_AND(ApplyOnTarget)
ENUM_OR(ApplyOn)
ENUM_OR(ApplyOnTarget)
ENUM_NOT(ApplyOn)
ENUM_NOT(ApplyOnTarget)

template <typename T>
consteval bool IsBreakable() {
    return std::is_assignable_v<Breakable, T> && !std::is_assignable_v<OverrideBreakable, T>;
}

template <AbilityEnum Id>
consteval bool IsBreakable() {
    return IsBreakable<AbilityImpl<Id>>();
}

template <typename T, typename Func>
AbilityEnum HasAbilityWithTagMatchingCondition(int battler, Func predicate, bool checkMoldBreaker = IsBreakable<T>()) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        AbilityEnum ability = gBattleMons[battler].abilities[i];
        const T* ptr = dispatchTo<T>(ability);
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        FILTER(ptr && predicate(ptr, ability))
        return ability;
    }
    return ABILITY_NONE;
}

template <typename T>
AbilityEnum HasAbilityWithTag(int battler, bool checkMoldBreaker = IsBreakable<T>()) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        FILTER(dispatchTo<T>(ability))
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        return ability;
    }
    return ABILITY_NONE;
}

template <AbilityEnum Id>
AbilityEnum HasAbilityOrClone(int battler, bool checkMoldBreaker = IsBreakable<Id>()) {
    return HasAbilityWithTag<AbilityImpl<Id>>(battler, checkMoldBreaker);
}

template <AbilityEnum Id, typename Func>
AbilityEnum HasAbilityOrCloneMatchingCondition(int battler, Func predicate, bool checkMoldBreaker = IsBreakable<Id>()) {
    return HasAbilityWithTagMatchingCondition<AbilityImpl<Id>>(battler, predicate, checkMoldBreaker);
}

inline bool CheckApplyOn(ApplyOn actual, ApplyOn expected) {
    if (expected == ApplyOn::SELF) {
        return (actual & ApplyOn::IGNORE_SELF) == ApplyOn::SELF;
    }

    return static_cast<int>(actual & (expected & ~ApplyOn::IGNORE_SELF)) > 0;
}

inline bool CheckApplyOnWithTarget(ApplyOnTarget actual, ApplyOnTarget expected) {
    if (expected == ApplyOnTarget::SELF) {
        return !static_cast<int>(actual & ApplyOnTarget::IGNORE_SELF);
    }
    if (expected == ApplyOnTarget::ATTACKER_OR_TARGET) {
        return !static_cast<int>(actual & ApplyOnTarget::IGNORE_SELF) || static_cast<int>(actual & ApplyOnTarget::ATTACKER_OR_TARGET);
    }

    return static_cast<int>(actual & (expected & ~ApplyOnTarget::IGNORE_SELF)) > 0;
}

template <AbilityEnum Id>
int GetAbilityOrCloneIndex(int battler, bool checkMoldBreaker = IsBreakable<Id>()) {
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
    if (IsBattlerAlive(BATTLE_OPPOSITE(BATTLE_PARTNER(battler))) && HasAbilityOrClone<Id>(BATTLE_OPPOSITE(BATTLE_PARTNER(battler))))
        return BATTLE_OPPOSITE(BATTLE_PARTNER(battler)) + 1;
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
            BATTLE_PARTNER(battler), [](const AbilityImpl<ABILITY_SOUNDPROOF>* it, auto&) { return CheckApplyOn(it->onImmuneFor(), ApplyOn::ALLY); });
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
        FILTER_NOT(IsSuppressed(target, ability, IsBreakable<ABILITY_ANTICIPATION>()))
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

int IsSkillSwapBanned(AbilityEnum ability) { return dispatchTo<SkillSwapBanned>(ability) != nullptr; }
int IsRolePlayBanned(AbilityEnum ability) { return dispatchTo<RolePlayBanned>(ability) != nullptr; }
int IsSimpleBeamBanned(AbilityEnum ability) { return dispatchTo<AbilityImpl<ABILITY_TRUANT>>(ability) != nullptr; }
AbilityEnum IgnoresRedirection(int battler) { return HasAbilityOrClone<ABILITY_STALWART>(battler); }
AbilityEnum AbilityMakesMoveSpread(int battler, MoveEnum move) {
    CHECK(gBattleMoves[move].target == MOVE_TARGET_SELECTED)
    return HasAbilityWithTagMatchingCondition<OnMakeSpread>(battler,
                                                            [&](const OnMakeSpread* impl, auto&) -> auto { return impl->onMakeSpread(battler, move); });
}
int IsAlwaysStab(AbilityEnum ability) { return dispatchTo<AlwaysStab>(ability) != nullptr; }
int AbilityGetsBonusStab(AbilityEnum ability, Type type) {
    auto ptr = dispatchTo<OnStab>(ability);
    if (!ptr) return FALSE;
    return ptr->onStab(type);
}
int GetsBonusStab(int battler, Type type) {
    return HasAbilityWithTagMatchingCondition<OnStab>(battler, [&](const OnStab* impl, auto&) -> auto { return impl->onStab(type); });
}

int TestAbsorbingAbilities(int battler, MoveEnum move, Type moveType, AbilityEnum* absorbingAbility, int* statId) {
    int result = 0;
    *absorbingAbility = HasAbilityWithTagMatchingCondition<OnAbsorb>(
        battler, [&](const OnAbsorb* impl, auto&) -> auto { return (result = impl->onAbsorb(battler, move, moveType, statId)); });
    return result;
}

template <typename T, typename Result, typename Func, typename ForSelector>
std::tuple<AbilityEnum, Result, u8> TestAllBattlers(u8 battler, ForSelector selector, Func transform, bool checkMoldBreaker = IsBreakable<T>()) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        const auto ptr = dispatchTo<T>(ability);
        FILTER(ptr)
        FILTER(CheckApplyOn(selector(ptr), ApplyOn::SELF))
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        Result result = transform(ptr, ability);
        if (result) return std::tuple(ability, result, battler);
    }
    u8 ally = BATTLE_PARTNER(battler);
    if (IsBattlerAlive(ally)) {
        for (int i = ARRAY_COUNT(gBattleMons[ally].abilities); i--;) {
            auto ability = gBattleMons[ally].abilities[i];
            const auto ptr = dispatchTo<T>(ability);
            FILTER(ptr)
            FILTER(CheckApplyOn(selector(ptr), ApplyOn::ALLY))
            FILTER_NOT(IsSuppressed(ally, ability, checkMoldBreaker))
            Result result = transform(ptr, ability);
            if (result) return std::tuple(ability, result, ally);
        }
    }
    u8 opponent = BATTLE_OPPOSITE(battler);
    if (IsBattlerAlive(opponent)) {
        for (int i = ARRAY_COUNT(gBattleMons[opponent].abilities); i--;) {
            auto ability = gBattleMons[opponent].abilities[i];
            const auto ptr = dispatchTo<T>(ability);
            FILTER(ptr)
            FILTER(CheckApplyOn(selector(ptr), ApplyOn::FOE))
            FILTER_NOT(IsSuppressed(opponent, ability, checkMoldBreaker))
            Result result = transform(ptr, ability);
            if (result) return std::tuple(ability, result, opponent);
        }
    }
    opponent = BATTLE_PARTNER(opponent);
    if (IsBattlerAlive(opponent)) {
        for (int i = ARRAY_COUNT(gBattleMons[opponent].abilities); i--;) {
            auto ability = gBattleMons[opponent].abilities[i];
            const auto ptr = dispatchTo<T>(ability);
            FILTER(ptr)
            FILTER(CheckApplyOn(selector(ptr), ApplyOn::FOE))
            FILTER_NOT(IsSuppressed(opponent, ability, checkMoldBreaker))
            Result result = transform(ptr, ability);
            if (result) return std::tuple(ability, result, opponent);
        }
    }
    return std::tuple(ABILITY_NONE, Result(), 0);
}

template <typename T, typename Result, typename Func, typename ForSelector>
inline std::tuple<AbilityEnum, Result, u8> TestAllBattlersWithAttackerInternal(u8 battler, ApplyOnTarget flags, ForSelector selector, Func transform, bool checkMoldBreaker) {
    for (int i = ARRAY_COUNT(gBattleMons[battler].abilities); i--;) {
        auto ability = gBattleMons[battler].abilities[i];
        const auto ptr = dispatchTo<T>(ability);
        FILTER(ptr)
        FILTER(CheckApplyOnWithTarget(selector(ptr), flags))
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))
        Result result = transform(ptr, ability, battler);
        if (result) return std::tuple(ability, result, battler);
    }
    return std::tuple(ABILITY_NONE, Result(), 0);
}

template <typename T, typename Result, typename Func, typename ForSelector>
std::tuple<AbilityEnum, Result, u8> TestAllBattlersWithAttacker(
    u8 attacker, u8 target, ForSelector selector, Func transform, bool allowDeadAttacker = true, bool checkMoldBreaker = IsBreakable<T>()) {
    if (allowDeadAttacker || IsBattlerAlive(attacker)) {
        auto flags = attacker == target ? ApplyOnTarget::ATTACKER_OR_TARGET : ApplyOnTarget::SELF;
        auto ret = TestAllBattlersWithAttackerInternal<T, Result>(attacker, flags, selector, transform, checkMoldBreaker);
        if (std::get<1>(ret)) return ret;
    }
    if (attacker != target) {
        u8 battler = target;
        if (IsBattlerAlive(battler)) {
            auto flags = ApplyOnTarget::TARGET | (BATTLE_PARTNER(attacker) == battler ? ApplyOnTarget::ALLY : ApplyOnTarget::FOE);
            auto ret = TestAllBattlersWithAttackerInternal<T, Result>(battler, flags, selector, transform, checkMoldBreaker);
            if (std::get<1>(ret)) return ret;
        }
    }
    u8 ally = BATTLE_PARTNER(attacker);
    if (ally != attacker && ally != target) {
        u8 battler = ally;
        if (IsBattlerAlive(battler)) {
            auto flags = battler == target ? ApplyOnTarget::ALLY | ApplyOnTarget::ALLY_IS_TARGET : ApplyOnTarget::ALLY;
            auto ret = TestAllBattlersWithAttackerInternal<T, Result>(battler, flags, selector, transform, checkMoldBreaker);
            if (std::get<1>(ret)) return ret;
        }
    }
    u8 opponent = BATTLE_OPPOSITE(attacker);
    if (opponent != attacker && opponent != target) {
        u8 battler = opponent;
        if (IsBattlerAlive(battler)) {
            auto flags = GetBattlerSide(opponent) == GetBattlerSide(target) ? ApplyOnTarget::FOE | ApplyOnTarget::ALLY_IS_TARGET
                                                                            : ApplyOnTarget::FOE | ApplyOnTarget::FOE_IS_TARGET;
            auto ret = TestAllBattlersWithAttackerInternal<T, Result>(battler, flags, selector, transform, checkMoldBreaker);
            if (std::get<1>(ret)) return ret;
        }
    }
    opponent = BATTLE_PARTNER(opponent);
    if (opponent != attacker && opponent != target) {
        u8 battler = opponent;
        if (IsBattlerAlive(battler)) {
            auto flags = GetBattlerSide(opponent) == GetBattlerSide(target) ? ApplyOnTarget::FOE | ApplyOnTarget::ALLY_IS_TARGET
                                                                            : ApplyOnTarget::FOE | ApplyOnTarget::FOE_IS_TARGET;
            auto ret = TestAllBattlersWithAttackerInternal<T, Result>(battler, flags, selector, transform, checkMoldBreaker);
            if (std::get<1>(ret)) return ret;
        }
    }
    return std::tuple(ABILITY_NONE, Result(), 0);
}

int TestAllImmunityAbilities(int battler, int attacker, MoveEnum move, Type moveType, const u8** immunityScript, u8* overrideBattler, u16* abilityPopup) {
    auto [ability, result, from] = TestAllBattlers<OnImmuneBase, int>(
        battler,
        [](const OnImmuneBase* impl) -> auto { return impl->onImmuneFor(); },
        [&](const OnImmuneBase* impl, auto&) -> auto { return impl->onImmune(battler, attacker, move, moveType, immunityScript); });

    if (result) {
        *abilityPopup = ability;
        *overrideBattler = from;
    }
    return result;
}

void CalcOffensiveMultipliers(
    int battler, int target, MoveEnum move, Type moveType, int basePower, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier) {
    TestAllBattlers<OnOffensiveMultiplierBase, bool>(
        battler,
        [](const OnOffensiveMultiplierBase* impl) -> auto { return impl->onOffensiveMultiplierFor(); },
        [&](const OnOffensiveMultiplierBase* impl, auto ability) -> bool {
            impl->onOffensiveMultiplier(battler, ability, target, move, moveType, basePower, typeEffectivenessModifier, isCrit, resistance, modifier);
            return false;
        });
}

void CalcDefensiveMultipliers(
    int battler, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier) {
    TestAllBattlers<OnDefensiveMultiplierBase, bool>(
        battler,
        [](const OnDefensiveMultiplierBase* impl) -> auto { return impl->onDefensiveMultiplierFor(); },
        [&](const OnDefensiveMultiplierBase* impl, auto&) -> auto {
            impl->onDefensiveMultiplier(battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier);
            return false;
        });
}

AbilityEnum Infiltrates(int battler, MoveEnum move, InfiltrateType type) {
    return HasAbilityWithTagMatchingCondition<OnInfiltrate>(battler,
                                                            [&](const OnInfiltrate* impl, auto&) -> auto { return impl->onInfiltrate(battler, move) & type; });
}

AbilityEnum DoesDisguiseBlockMoveInternal(int target, MoveEnum move, SpeciesEnum* newSpecies, int testOnly) {
    SpeciesEnum resultSpecies;
    CHECK_NOT(gBattleMons[target].status2 & STATUS2_TRANSFORMED)
    CHECK_NOT(IS_MOVE_STATUS(move))
    CHECK_NOT(gHitMarker & HITMARKER_IGNORE_DISGUISE && move != MOVE_SUCKER_PUNCH);
    AbilityEnum ability = HasAbilityWithTagMatchingCondition<OnDisguise>(
        target, [&](const OnDisguise* impl, auto&) -> auto { return (resultSpecies = impl->onDisguise(target, testOnly)); });
    if (ability && newSpecies) *newSpecies = resultSpecies;
    return ability;
}
AbilityEnum TestDoesDisguiseBlockMove(int target, MoveEnum move) { return DoesDisguiseBlockMoveInternal(target, move, nullptr, TRUE); }
AbilityEnum DoesDisguiseBlockMove(int target, MoveEnum move, SpeciesEnum* newSpecies) { return DoesDisguiseBlockMoveInternal(target, move, newSpecies, FALSE); }

void HandleOnWeather(int battler) {
    HasAbilityWithTagMatchingCondition<OnWeather>(battler, [&](const OnWeather* impl, auto ability) -> auto {
        if (impl->onWeather(ability, battler)) {
            gBattlerAbility = battler;
            gBattleScripting.abilityPopupOverwrite = ability;
            BattleScriptCall(BattleScript_AbilityPopUp);
        }
        return false;
    });
}
void HandleOnTerrain(int battler) {
    HasAbilityWithTagMatchingCondition<OnTerrain>(battler, [&](const OnTerrain* impl, auto ability) -> auto {
        if (impl->onTerrain(ability, battler)) {
            gBattlerAbility = battler;
            gBattleScripting.abilityPopupOverwrite = ability;
            BattleScriptCall(BattleScript_AbilityPopUp);
        }
        return false;
    });
}
int InvokeToxicWasteForMonotypeChamp() {
    return dispatchTo<AbilityImpl<ABILITY_TOXIC_SPILL>>(ABILITY_TOXIC_SPILL)->onEndTurn(ABILITY_NONE, MAX_BATTLERS_COUNT);
}

template <typename T, typename Func>
int PerformOnGeneric(int battler, AbilityEnum ability, Func predicate) {
    auto ptr = dispatchTo<T>(ability);
    CHECK(ptr)
    CHECK(!IsSuppressed(battler, ability, IsBreakable<T>()))

    gBattleScripting.abilityPopupOverwrite = ability;
    int result = predicate(ptr);

    if (result & 1) BattleScriptCall(BattleScript_AbilityPopUp);
    return result;
}

int PerformOnEntry(int battler, AbilityEnum ability) {
    return PerformOnGeneric<OnEntry>(battler, ability, [&](const OnEntry* impl) -> int {
        if (!dispatchTo<AbilityImpl<ABILITY_TRACE>>(ability)) {
            CHECK(CheckAndSetSwitchInAbility(battler, ability))
        }
        gBattlerAbility = gBattleScripting.battler = battler;
        return impl->onEntry(ability, battler);
    });
}

int PerformOnEndTurn(int battler, AbilityEnum ability) {
    return PerformOnGeneric<OnEndTurn>(battler, ability, [&](const OnEndTurn* impl) -> int { return impl->onEndTurn(ability, battler); });
}

int PerformOnAttacker(int battler, int target, AbilityEnum ability, MoveEnum move, Type moveType) {
    return PerformOnGeneric<OnAttacker>(
        battler, ability, [&](const OnAttacker* impl) -> int { return impl->onAttacker(ability, battler, target, move, moveType); });
}

int PerformOnDefender(int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType) {
    return PerformOnGeneric<OnDefender>(
        battler, ability, [&](const OnDefender* impl) -> int { return impl->onDefender(ability, battler, attacker, move, moveType); });
}

void HandleRecoilAbilities(int battler, int moveDamage, Type moveType) {
    HasAbilityWithTagMatchingCondition<OnRecoil>(battler, [&](const OnRecoil* impl, auto ability) -> bool {
        int damage = impl->onRecoil(moveDamage, battler, moveType);
        CHECK(damage)
        if (!gBattleMoveDamage) BattleScriptCall(BattleScript_MoveEffectRecoil);
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptCall(BattleScript_AbilityPopUp);
        gBattleMoveDamage += damage;
        return false;
    });
}

int HandleAllOnReactive(AbilityCallType callType) {
    bool any = false;
    for (int i = 0; i < gBattlersCount; i++) {
        FILTER(IsBattlerAlive(i))
        HasAbilityWithTagMatchingCondition<OnReactive>(i, [&](const OnReactive* impl, auto ability) -> bool {
            CHECK(impl->onReactive(ability, i, callType))
            any = true;
            return false;
        });
    }
    return any;
}

void HandleOnBattlerFaints(int attacker, int fainted, MoveEnum move, Type moveType) {
    TestAllBattlersWithAttacker<OnBattlerFaintsBase, bool>(
        attacker,
        fainted,
        [](const OnBattlerFaintsBase* impl) -> ApplyOnTarget { return impl->onBattlerFaintsFor(); },
        [&](const OnBattlerFaintsBase* impl, AbilityEnum ability, auto abilityBattler) -> bool {
            int result = impl->onBattlerFaints(ability, abilityBattler, attacker, fainted, move, moveType);
            if (result & 1) {
                gBattleScripting.abilityPopupOverwrite = ability;
                BattleScriptCall(BattleScript_AbilityPopUpStack);
            }
            return false;
        },
        /* allowDeadAttacker= */ false);
}

MultihitType HandleParentalBond(int battler, int hasFortKnox, MoveEnum move, Type moveType) {
    MultihitType bondType = MULTIHIT_SINGLE;
    HasAbilityWithTagMatchingCondition<OnParentalBond>(battler, [&](const OnParentalBond* impl, AbilityEnum ability) -> MultihitType {
        CHECK(!hasFortKnox || dispatchTo<IgnoresFortKnox>(ability))
        return impl->onParentalBond(battler, move, moveType);
    });
    return bondType;
}

int OnMoveTypeSingleAbility(AbilityEnum ability, MoveEnum move, Type moveType) {
    auto ptr = dispatchTo<OnMoveType>(ability);
    CHECK(ptr)
    u8 _;
    return ptr->onMoveType(ability, move, moveType, &_);
}

int OnMoveTypeForBattler(int battler, MoveEnum move, Type moveType, u8* ateBoost) {
    int result = 0;
    HasAbilityWithTagMatchingCondition<OnMoveType>(
        battler, [&](const OnMoveType* impl, auto ability) -> auto { return impl->onMoveType(ability, move, moveType, ateBoost); });
    return result;
}

void CalculateStatsFromAbilities(int battler, int statId, u32* stat) {
    NonStackingState flags = NON_STACKING_NONE;
    TestAllBattlers<OnStatBase, bool>(
        battler,
        [](const OnStatBase* impl) -> auto { return impl->onStatFor(); },
        [&](const OnStatBase* impl, auto ability) -> bool {
            impl->onStat(ability, battler, statId, stat, &flags);
            return false;
        });
}

AccuracyPriority CalculateAccuracyFromAbilities(int attacker, int target, MoveEnum move, Type moveType, int* accuracy) {
    AccuracyPriority result = ACCURACY_NO_RESULT;
    TestAllBattlersWithAttacker<OnAccuracyBase, bool>(
        attacker,
        target,
        [](const OnAccuracyBase* impl) -> auto { return impl->onAccuracyFor(); },
        [&](const OnAccuracyBase* impl, auto ability, auto&) -> bool {
            AccuracyPriority newResult = impl->onAccuracy(ability, attacker, target, move, moveType, accuracy);
            result = max(newResult, result);
            return result == ACCURACY_ALWAYS_HITS;
        });
    return result;
}

AbilityEnum ShouldSwapSplit(int battler, MoveEnum move) {
    return HasAbilityWithTagMatchingCondition<OnSwapSplit>(battler, [&](const OnSwapSplit* impl, auto&) -> bool { return impl->onSwapSplit(battler, move); });
}

void HandleChooseOffensiveStat(int battler, MoveEnum move, int isCrit, int isUnaware, u8* atkStatToUse, u8 secondaryAttackStatToUse[NUM_STATS]) {
    HasAbilityWithTagMatchingCondition<OnChooseOffensiveStat>(battler, [&](const OnChooseOffensiveStat* impl, auto&) -> bool {
        impl->onChooseOffensiveStat(battler, move, isCrit, isUnaware, atkStatToUse, secondaryAttackStatToUse);
        return false;
    });
}

int HandleChooseDefensiveStat(int battlerAtk, int battlerDef, MoveEnum move, int noPositiveStatStages, int isUnaware) {
    auto [_, result, __] = TestAllBattlersWithAttacker<OnChooseDefensiveStatBase, int>(
        battlerAtk,
        battlerDef,
        [](const OnChooseDefensiveStatBase* impl) -> auto { return impl->onChooseDefensiveStatFor(); },
        [&](const OnChooseDefensiveStatBase* impl, auto&, auto&) -> int {
            return impl->onChooseDefensiveStat(battlerAtk, battlerDef, move, noPositiveStatStages, isUnaware);
        });
    return result;
}

int CalculatePriorityModifier(int battlerAtk, int battlerDef, MoveEnum move) {
    int prio = 0;
    HasAbilityWithTagMatchingCondition<OnPriority>(battlerAtk, [&](const OnPriority* impl, auto&) -> auto {
        prio += impl->onPriority(battlerAtk, battlerDef, move);
        return false;
    });
    return prio;
}

int CalculateCritModifier(int battlerAtk, int battlerDef, MoveEnum move, u16 typeEffectiveness) {
    int mod = 0;
    auto [_, result, __] = TestAllBattlersWithAttacker<OnCritBase, int>(
        battlerAtk,
        battlerDef,
        [](const OnCritBase* impl) -> auto { return impl->onCritFor(); },
        [&](const OnCritBase* impl, auto&, u8 battler) -> int {
            int newMod = impl->onCrit(battler, battlerDef, move, typeEffectiveness);
            mod += newMod;
            return newMod == NEVER_CRIT;
        });
    return result == NEVER_CRIT ? NEVER_CRIT : mod;
}

int TryChangeTypeEffectiveness(int battler, Type defType, MoveEnum move, Type moveType, u16* modifier) {
    return HasAbilityWithTagMatchingCondition<OnTypeEffectiveness>(
        battler, [&](const OnTypeEffectiveness* impl, auto&) -> auto { return impl->onTypeEffectiveness(defType, move, moveType, modifier); });
}

int HandleCopyMove(int battler, int battlerAtk, int battlerDef, MoveEnum move) {
    int target = GetBattlerSide(battler) == GetBattlerSide(battlerDef) ? BATTLE_OPPOSITE(battler) : battlerDef;
    return HasAbilityWithTagMatchingCondition<OnCopyMove>(
        battler, [&](const OnCopyMove* impl, auto ability) -> auto { return impl->onCopyMove(ability, battler, battlerAtk, target, move); });
}

AbilityEnum HandleOnAfterTypeEffectiveness(
    int battlerAtk, int battlerDef, MoveEnum move, Type moveType, u16* modifier, u16 modifier1, u16 modifier2, u16 modifier3) {
    AbilityEnum immunityAbility = ABILITY_NONE;
    HasAbilityWithTagMatchingCondition<OnDefenderAfterTypeEffectiveness>(battlerDef, [&](const OnDefenderAfterTypeEffectiveness* impl, auto ability) -> auto {
        int wasImmune = !*modifier;
        impl->onDefenderAfterTypeEffectiveness(battlerDef, ability, battlerAtk, move, moveType, modifier);
        if (!wasImmune && !*modifier) immunityAbility = ability;
        return false;
    });
    HasAbilityWithTagMatchingCondition<OnAttackerAfterTypeEffectiveness>(battlerDef, [&](const OnAttackerAfterTypeEffectiveness* impl, auto ability) -> auto {
        int wasImmune = !*modifier;
        impl->onAttackerAfterTypeEffectiveness(battlerAtk, battlerDef, move, moveType, modifier, modifier1, modifier2, modifier3);
        if (!wasImmune && !*modifier) immunityAbility = ability;
        return false;
    });
    return immunityAbility;
}
