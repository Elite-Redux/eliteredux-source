#include "behavior/ability/constants.hh"

#define is \
   public  \
    virtual

#define APPLIES_ON(name, applyType, ON_NAME)                    \
    struct On##name##Base {                                     \
        ON_NAME = 0;                                            \
        virtual applyType on##name##For() const = 0;            \
    };                                                          \
    template <applyType For = applyType::SELF>                  \
    struct On##name : is On##name##Base {                       \
        virtual applyType on##name##For() const { return For; } \
    };

#define APPLIES_ON_BREAKABLE(name, applyType, ON_NAME)          \
    struct On##name##Base : is Breakable {                      \
        ON_NAME = 0;                                            \
        virtual applyType on##name##For() const = 0;            \
    };                                                          \
    template <applyType For = applyType::SELF>                  \
    struct On##name : is On##name##Base {                       \
        virtual applyType on##name##For() const { return For; } \
    };

template <typename T, typename U, typename Target>
concept BothAre = std::is_assignable_v<Target, T> && std::is_assignable_v<Target, U>;

struct Breakable {
    virtual bool breakable() { return true; }
};
struct OverrideBreakable : public virtual Breakable {
    virtual bool breakable() { return false; }
};
struct RandomizerBanned {};
struct NotImplemented : is RandomizerBanned {};
struct Unsuppressable {};
struct Persistent {};
struct FormChangeAbility : is RandomizerBanned, is Unsuppressable {};

template <AbilityEnum Id>
struct AbilityImpl : is NotImplemented {
    static const AbilityImpl<Id> instance;
};

struct OnEntry {
#define ON_ENTRY virtual int onEntry(AbilityEnum ability, int battler) const
#define DELEGATE_ENTRY ability, battler
    ON_ENTRY = 0;
};

struct OnAbsorb : is Breakable {
#define ON_ABSORB virtual int onAbsorb(int battler, MoveEnum move, Type moveType, int *statId) const
#define DELEGATE_ABSORB battler, move, moveType, statId
    ON_ABSORB { return 0; }
};

#define ON_IMMUNE_ARGS int battler, int attacker, MoveEnum move, Type moveType, const u8 **immunityScript
#define ON_IMMUNE virtual int onImmune(ON_IMMUNE_ARGS) const
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
APPLIES_ON_BREAKABLE(Immune, ApplyOn, ON_IMMUNE)

struct OnInfiltrate {
#define ON_INFILTRATE virtual InfiltrateType onInfiltrate(int battler, MoveEnum move) const
#define DELEGATE_INFILTRATE battler, move
    ON_INFILTRATE = 0;
};

struct OnDisguise {
#define ON_DISGUISE virtual SpeciesEnum onDisguise(int battler, int testOnly) const
#define DELEGATE_DISGUISE battler, testOnly
    ON_DISGUISE = 0;
};

struct OnWeather {
#define ON_WEATHER virtual int onWeather(AbilityEnum ability, int battler) const
#define DELEGATE_WEATHER ability, battler
    ON_WEATHER = 0;
};

struct OnTerrain {
#define ON_TERRAIN virtual int onTerrain(AbilityEnum ability, int battler) const
#define DELEGATE_TERRAIN ability, battler
    ON_TERRAIN = 0;
};

struct OnEndTurn {
#define ON_END_TURN virtual int onEndTurn(AbilityEnum ability, int battler) const
#define DELEGATE_END_TURN ability, battler
    ON_END_TURN = 0;
};

struct OnAttacker {
#define ON_ATTACKER virtual int onAttacker(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType) const
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
    ON_ATTACKER = 0;
};

struct OnDefender {
#define ON_DEFENDER virtual int onDefender(AbilityEnum ability, int battler, int attacker, MoveEnum move, Type moveType) const
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
    ON_DEFENDER = 0;
};

struct OnEither : is OnAttacker, is OnDefender {};
#define ON_EITHER                                       \
    ON_ATTACKER { return onEither(DELEGATE_ATTACKER); } \
    ON_DEFENDER { return onEither(DELEGATE_DEFENDER); } \
    static int onEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, Type moveType)

struct OnRecoil {
#define ON_RECOIL virtual int onRecoil(int damage, int battler, Type moveType) const
#define DELEGATE_RECOIL damage, battler, moveType
    ON_RECOIL = 0;
};

struct OnReactive {
#define ON_REACTIVE virtual int onReactive(AbilityEnum ability, int battler, AbilityCallType callType) const
#define DELEGATE_REACTIVE ability, battler
    ON_REACTIVE = 0;
};

#define ON_BATTLER_FAINTS virtual int onBattlerFaints(AbilityEnum ability, int battler, int attacker, int fainted, MoveEnum move, Type moveType) const
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
APPLIES_ON(BattlerFaints, ApplyOnTarget, ON_BATTLER_FAINTS)

struct OnParentalBond {
#define ON_PARENTAL_BOND virtual MultihitType onParentalBond(int battler, MoveEnum move, Type moveType) const
#define DELEGATE_PARENTAL_BOND battler, move, moveType
    ON_PARENTAL_BOND = 0;
};

#define ON_OFFENSIVE_MULTIPLIER                                         \
    virtual void onOffensiveMultiplier(int battler,                     \
                                       AbilityEnum ability,             \
                                       int target,                      \
                                       MoveEnum move,                   \
                                       Type moveType,                   \
                                       int basePower,                   \
                                       int typeEffectivenessMultiplier, \
                                       int isCrit,                      \
                                       u16 *resistance,                 \
                                       u16 *modifier) const
#define DELEGATE_OFFENSIVE_MULTIPLIER battler, ability, target, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier
APPLIES_ON(OffensiveMultiplier, ApplyOn, ON_OFFENSIVE_MULTIPLIER)

#define ON_DEFENSIVE_MULTIPLIER         \
    virtual void onDefensiveMultiplier( \
        int battler, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16 *resistance, u16 *modifier) const
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
APPLIES_ON_BREAKABLE(DefensiveMultiplier, ApplyOn, ON_DEFENSIVE_MULTIPLIER)

struct OnMoveType {
#define ON_MOVE_TYPE virtual int onMoveType(AbilityEnum ability, MoveEnum move, Type moveType, u8 *ateBoost) const
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
    ON_MOVE_TYPE = 0;
};

struct OnStab {
#define ON_STAB virtual int onStab(Type moveType) const
#define DELEGATE_STAB moveType
    ON_STAB = 0;
};

struct OnMakeSpread {
#define ON_MAKE_SPREAD virtual int onMakeSpread(int battler, MoveEnum move) const
#define DELEGATE_MAKE_SPREAD battler, move
    ON_MAKE_SPREAD = 0;
};

#define ON_STAT virtual void onStat(AbilityEnum ability, int battler, int statId, u32 *stat, NonStackingState *flags) const
#define DELEGATE_STAT ability, battler, statId, stat, flags
APPLIES_ON(Stat, ApplyOn, ON_STAT)

#define ON_ACCURACY virtual AccuracyPriority onAccuracy(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType, int *accuracy) const
#define DELEGATE_ACCURACY ability, battler, target, move, moveType, accuracy
APPLIES_ON(Accuracy, ApplyOnTarget, ON_ACCURACY)

struct OnSwapSplit {
#define ON_SWAP_SPLIT virtual int onSwapSplit(int battler, MoveEnum move) const
#define DELEGATE_SWAP_SPLIT battler, move
    ON_SWAP_SPLIT = 0;
};

struct OnChooseOffensiveStat {
#define ON_CHOOSE_OFFENSIVE_STAT        \
    virtual void onChooseOffensiveStat( \
        int battler, MoveEnum move, int ignoreOffensiveStatDrops, int targetUnaware, u8 *atkStatToUse, u8 secondaryAtkStatToUse[NUM_STATS]) const
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
    ON_CHOOSE_OFFENSIVE_STAT = 0;
};

#define ON_CHOOSE_DEFENSIVE_STAT \
    virtual int onChooseDefensiveStat(int battler, int target, MoveEnum move, int ignoreDefensiveStatBoosts, int battlerUnaware) const
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
APPLIES_ON(ChooseDefensiveStat, ApplyOnTarget, ON_CHOOSE_DEFENSIVE_STAT)

struct OnPriority {
#define ON_PRIORITY virtual int onPriority(int battler, int target, MoveEnum move) const
#define DELEGATE_PRIORITY battler, target, move
    ON_PRIORITY = 0;
};

struct OnExit {
#define ON_EXIT virtual int onExit(AbilityEnum ability, int battler) const
#define DELEGATE_EXIT ability, battler
    ON_EXIT = 0;
};

#define ON_CRIT virtual int onCrit(int battler, int target, MoveEnum move, u16 typeEffectiveness) const
#define DELEGATE_CRIT battler, target, move, typeEffectiveness
APPLIES_ON(Crit, ApplyOnTarget, ON_CRIT)

struct OnTypeEffectiveness {
#define ON_TYPE_EFFECTIVENESS virtual int onTypeEffectiveness(int defType, MoveEnum move, Type moveType, u16 *mod) const
#define DELEGATE_TYPE_EFFECTIVENESS defType, move, moveType, mod
    ON_TYPE_EFFECTIVENESS = 0;
};

struct OnCopyMove {
#define ON_COPY_MOVE virtual int onCopyMove(AbilityEnum ability, int battler, int attacker, int target, MoveEnum move) const
#define DELEGATE_COPY_MOVE ability, battler, attacker, target, move
    ON_COPY_MOVE = 0;
};

struct OnAttackerAfterTypeEffectiveness {
#define ON_ATTACKER_AFTER_TYPE_EFFECTIVENESS \
    virtual void onAttackerAfterTypeEffectiveness(int battler, int target, MoveEnum move, Type moveType, u16 *mod, u16 mod1, u16 mod2, u16 mod3) const
#define DELEGATE_ATTACKER_AFTER_TYPE_EFFECTIVENESS battler, target, move, moveType, mod, mod1, mod2, mod3
    ON_ATTACKER_AFTER_TYPE_EFFECTIVENESS = 0;
};

struct OnDefenderAfterTypeEffectiveness {
#define ON_DEFENDER_AFTER_TYPE_EFFECTIVENESS \
    virtual void onDefenderAfterTypeEffectiveness(int battler, AbilityEnum ability, int attacker, MoveEnum move, Type moveType, u16 *mod) const
#define DELEGATE_DEFENDER_AFTER_TYPE_EFFECTIVENESS battler, ability, attacker, move, moveType, mod
    ON_DEFENDER_AFTER_TYPE_EFFECTIVENESS = 0;
};

#define ON_MODIFY_EFFECT_CHANCE virtual void onModifyEffectChance(int battler, MoveEnum move, MoveEffectEnum moveEffect, int *effectChance) const
#define DELEGATE_MODIFY_EFFECT_CHANCE battler, move, moveEffect, effectChance
APPLIES_ON(ModifyEffectChance, ApplyOn, ON_MODIFY_EFFECT_CHANCE)

struct OnCanStatusType {
#define ON_CAN_STATUS_TYPE virtual int onCanStatusType(int battler, MoveEnum move, StatusCheckEnum status) const
#define DELEGATE_CAN_STATUS_TYPE battler, move, status
    ON_CAN_STATUS_TYPE = 0;
};

#define ON_STATUS_IMMUNE virtual int onStatusImmune(int battler, int target, AbilityEnum ability, StatusCheckEnum status) const
#define DELEGATE_STATUS_IMMUNE int battler, target, ability, status
APPLIES_ON_BREAKABLE(StatusImmune, ApplyOn, ON_STATUS_IMMUNE)

struct OnTrap {
#define ON_TRAP virtual int onTrap(int switchingBattler) const
#define DELEGATE_TRAP switchingBattler
    ON_TRAP = 0;
};

#define ON_BEFORE_ATTACK virtual int onBeforeAttack(int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType) const
#define DELEGATE_BEFORE_ATTACK battler, attacker, ability, move, moveType
APPLIES_ON(BeforeAttack, ApplyOnTarget, ON_BEFORE_ATTACK)

struct OnPreemptAction {
#define ON_PREEMPT_ACTION virtual int onPreemptAction(u8 battler, AbilityEnum ability, u8 turnBattler) const
#define DELEGATE_PREEMPT_ACTION battler, ability, turnBattler
    ON_PREEMPT_ACTION = 0;
};

struct OnModifyMoveFlags {
#define ON_MODIFY_MOVE_FLAGS_ARGS int battler, MoveEnum move, MoveFlag flag
#define ON_MODIFY_MOVE_FLAGS virtual int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS_ARGS) const
#define DELEGATE_MODIFY_MOVE_FLAGS battler, move, flag
    ON_MODIFY_MOVE_FLAGS = 0;
};

struct OnMoldBreaker {
#define ON_MOLD_BREAKER virtual int onMoldBreaker(int battler, MoveEnum move) const
#define DELEGATE_MOLD_BREAKER battler, move, moveType
    ON_MOLD_BREAKER = 0;
};

struct OnRevive : is Persistent {
#define ON_REVIVE virtual int onRevive(int battler) const
#define DELEGATE_REVIVE battler
    ON_REVIVE = 0;
};

struct SetStateOnEffectBase {
    virtual MoveEffectEnum setStateOnEffect() const = 0;
};
template <MoveEffectEnum Effect>
struct SetStateOnEffect : is SetStateOnEffectBase {
    MoveEffectEnum setStateOnEffect() const { return Effect; }
};

#define ENUM_WRAPPER(Name, name, type, BaseType)    \
    struct Name##Base : is BaseType {               \
        virtual type name() const = 0;              \
    };                                              \
    template <type Value>                           \
    struct Name : is Name##Base {                   \
        virtual type name() const { return Value; } \
    };

struct Placeholder {};

ENUM_WRAPPER(Redirects, redirectType, Type, OnAbsorb)
ENUM_WRAPPER(AllowTerrainIfAirborne, allowTerrainIfAirborne, TerrainType, Placeholder)

#define MERGE_OPERATOR(Name, name, ON_NAME, DELEGATE_NAME, op)                        \
    template <typename T, typename U>                                                 \
    struct Merge##Name##Impl : is T, is U {                                           \
        ON_NAME override { return T::name(DELEGATE_NAME) op U::name(DELEGATE_NAME); } \
    };                                                                                \
    template <typename T, typename U>                                                 \
    struct Merge##Name : is std::conditional_t<BothAre<T, U, Name>, Merge##Name##Impl<T, U>, Placeholder> {};

#define MERGE_VOID(Name, name, ON_NAME, DELEGATE_NAME) \
    template <typename T, typename U>                  \
    struct Merge##Name##Impl : is T, is U {            \
        ON_NAME override {                             \
            T::name(DELEGATE_NAME);                    \
            U::name(DELEGATE_NAME);                    \
        }                                              \
    };                                                 \
    template <typename T, typename U>                  \
    struct Merge##Name : is std::conditional_t<BothAre<T, U, Name>, Merge##Name##Impl<T, U>, Placeholder> {};

MERGE_OPERATOR(OnEntry, onEntry, ON_ENTRY, DELEGATE_ENTRY, |)
MERGE_VOID(OnDefender, onDefender, ON_DEFENDER, DELEGATE_DEFENDER)
MERGE_OPERATOR(OnExit, onExit, ON_EXIT, DELEGATE_EXIT, |)
MERGE_VOID(OnModifyEffectChanceBase, onModifyEffectChance, ON_MODIFY_EFFECT_CHANCE, DELEGATE_MODIFY_EFFECT_CHANCE)
MERGE_OPERATOR(OnAbsorb, onAbsorb, ON_ABSORB, DELEGATE_ABSORB, |)
MERGE_OPERATOR(OnTypeEffectiveness, onTypeEffectiveness, ON_TYPE_EFFECTIVENESS, DELEGATE_TYPE_EFFECTIVENESS, ||)
MERGE_OPERATOR(OnEndTurn, onEndTurn, ON_END_TURN, DELEGATE_END_TURN, |)
MERGE_VOID(OnStatBase, onStat, ON_STAT, DELEGATE_STAT)
MERGE_VOID(OnOffensiveMultiplierBase, onOffensiveMultliplier, ON_OFFENSIVE_MULTIPLIER, DELEGATE_OFFENSIVE_MULTIPLIER)

template <typename T, typename U>
struct MergedRaw : is MergeOnEntry<T, U>,
                   is MergeOnDefender<T, U>,
                   is MergeOnExit<T, U>,
                   is MergeOnModifyEffectChanceBase<T, U>,
                   is MergeOnAbsorb<T, U>,
                   is MergeOnTypeEffectiveness<T, U>,
                   is MergeOnEndTurn<T, U>,
                   is MergeOnStatBase<T, U>,
                   is MergeOnOffensiveMultiplierBase<T, U> {};

template <AbilityEnum T, AbilityEnum U>
struct MergedNone : is AbilityImpl<T>, is AbilityImpl<U> {};

template <AbilityEnum T, AbilityEnum U>
struct Merged : is std::conditional<std::is_assignable_v<AbilityImpl<T>, MergedRaw<AbilityImpl<T>, AbilityImpl<U>>>,
                                    MergedRaw<AbilityImpl<T>, AbilityImpl<U>>,
                                    MergedNone<T, U>> {};