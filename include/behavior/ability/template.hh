#include "behavior/ability/constants.hh"

#define extends \
   public       \
    virtual

#define APPLIES_ON(name, applyType, ON_NAME)                    \
    struct On##name##Base {                                     \
        ON_NAME = 0;                                            \
        virtual applyType on##name##For() const = 0;            \
    };                                                          \
    template <applyType For = applyType::SELF>                  \
    struct On##name : extends On##name##Base {                  \
        virtual applyType on##name##For() const { return For; } \
    };

#define APPLIES_ON_BREAKABLE(name, applyType, ON_NAME)          \
    struct On##name##Base : extends Breakable {                 \
        ON_NAME = 0;                                            \
        virtual applyType on##name##For() const = 0;            \
    };                                                          \
    template <applyType For = applyType::SELF>                  \
    struct On##name : extends On##name##Base {                  \
        virtual applyType on##name##For() const { return For; } \
    };

template <AbilityEnum Id>
struct AbilityImpl {
    static const AbilityImpl<Id> instance;
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
struct NotImplemented : extends RandomizerBanned {};
struct Unsuppressable {};
struct Persistent {};
struct FormChangeAbility : extends RandomizerBanned, extends Unsuppressable {};

struct OnEntry {
#define ON_ENTRY virtual int onEntry(AbilityEnum ability, int battler)
#define DELEGATE_ENTRY ability, battler
    ON_ENTRY = 0;
};

struct OnAbsorb : extends Breakable {
#define ON_ABSORB virtual int onAbsorb(int battler, MoveEnum move, Type moveType, int *statId)
#define DELEGATE_ABSORB battler, move, moveType, statId
    ON_ABSORB { return 0; }
};

#define ON_IMMUNE_ARGS int battler, int attacker, MoveEnum move, Type moveType, const u8 **immunityScript
#define ON_IMMUNE virtual int onImmune(ON_IMMUNE_ARGS)
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
APPLIES_ON_BREAKABLE(Immune, ApplyOn, ON_IMMUNE)

struct OnInfiltrate {
#define ON_INFILTRATE virtual InfiltrateType onInfiltrate(int battler, MoveEnum move)
#define DELEGATE_INFILTRATE battler, move
    ON_INFILTRATE = 0;
};

struct OnDisguise {
#define ON_DISGUISE virtual SpeciesEnum onDisguise(int battler, int testOnly)
#define DELEGATE_DISGUISE battler, testOnly
    ON_DISGUISE = 0;
};

struct OnWeather {
#define ON_WEATHER virtual int onWeather(AbilityEnum ability, int battler)
#define DELEGATE_WEATHER ability, battler
    ON_WEATHER = 0;
};

struct OnTerrain {
#define ON_TERRAIN virtual int onTerrain(AbilityEnum ability, int battler)
#define DELEGATE_TERRAIN ability, battler
    ON_TERRAIN = 0;
};

struct OnEndTurn {
#define ON_END_TURN virtual int onEndTurn(AbilityEnum ability, int battler)
#define DELEGATE_END_TURN ability, battler
    ON_END_TURN = 0;
};

struct OnAttacker {
#define ON_ATTACKER virtual int onAttacker(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType)
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
    ON_ATTACKER = 0;
};

struct OnDefender {
#define ON_DEFENDER virtual int onDefender(AbilityEnum ability, int battler, int attacker, MoveEnum move, Type moveType)
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
    ON_DEFENDER = 0;
};

struct OnEither : extends OnAttacker, extends OnDefender {};
#define ON_EITHER                                       \
    ON_ATTACKER { return onEither(DELEGATE_ATTACKER); } \
    ON_DEFENDER { return onEither(DELEGATE_DEFENDER); } \
    static int onEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, Type moveType)

struct OnRecoil {
#define ON_RECOIL virtual int onRecoil(int damage, int battler, Type moveType)
#define DELEGATE_RECOIL damage, battler, moveType
    ON_RECOIL = 0;
};

struct OnReactive {
#define ON_REACTIVE virtual int onReactive(AbilityEnum ability, int battler, AbilityCallType callType)
#define DELEGATE_REACTIVE ability, battler
    ON_REACTIVE = 0;
};

#define ON_BATTLER_FAINTS virtual int onBattlerFaints(AbilityEnum ability, int battler, int attacker, int fainted, MoveEnum move, Type moveType)
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
#define SELF ATTACKER
APPLIES_ON(BattlerFaints, ApplyOnTarget, ON_BATTLER_FAINTS)
#undef SELF

struct OnParentalBond {
#define ON_PARENTAL_BOND virtual MultihitType onParentalBond(int battler, MoveEnum move, Type moveType)
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
                                       u16 *modifier)
#define DELEGATE_OFFENSIVE_MULTIPLIER battler, ability, target, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier
APPLIES_ON(OffensiveMultiplier, ApplyOn, ON_OFFENSIVE_MULTIPLIER)

struct OnDefensiveMultiplier : extends Breakable {
#define ON_DEFENSIVE_MULTIPLIER         \
    virtual void onDefensiveMultiplier( \
        int battler, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16 *resistance, u16 *modifier)
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
    ON_DEFENSIVE_MULTIPLIER = 0;
};

struct OnMoveType {
#define ON_MOVE_TYPE virtual int onMoveType(AbilityEnum ability, MoveEnum move, Type moveType, u8 *ateBoost)
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
    ON_MOVE_TYPE = 0;
};

struct OnStab {
#define ON_STAB virtual int onStab(Type moveType)
#define DELEGATE_STAB moveType
    ON_STAB = 0;
};

#define ON_STAT virtual void onStat(AbilityEnum ability, int battler, int statId, u32 *stat, NonStackingState *flags)
#define DELEGATE_STAT ability, battler, statId, stat, flags
APPLIES_ON(Stat, ApplyOn, ON_STAT)

#define ON_ACCURACY virtual AccuracyPriority onAccuracy(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType, int *accuracy)
#define DELEGATE_ACCURACY ability, battler, target, move, moveType, accuracy
APPLIES_ON(Accuracy, ApplyOnTarget, ON_ACCURACY)

struct OnSwapSplit {
#define ON_SWAP_SPLIT virtual int onSwapSplit(int battler, MoveEnum move)
#define DELEGATE_SWAP_SPLIT battler, move
    ON_SWAP_SPLIT = 0;
};

struct OnChooseOffensiveStat {
#define ON_CHOOSE_OFFENSIVE_STAT        \
    virtual void onChooseOffensiveStat( \
        int battler, MoveEnum move, int ignoreOffensiveStatDrops, int targetUnaware, u8 *atkStatToUse, u8 secondaryAtkStatToUse[NUM_STATS])
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
    ON_CHOOSE_OFFENSIVE_STAT = 0;
};

#define ON_CHOOSE_DEFENSIVE_STAT virtual int onChooseDefensiveStat(int battler, int target, MoveEnum move, int ignoreDefensiveStatBoosts, int battlerUnaware)
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
APPLIES_ON(ChooseDefensiveStat, ApplyOnTarget, ON_CHOOSE_DEFENSIVE_STAT)

struct OnPriority {
#define ON_PRIORITY virtual int onPriority(int battler, int target, MoveEnum move)
#define DELEGATE_PRIORITY battler, target, move
    ON_PRIORITY = 0;
};

struct OnExit {
#define ON_EXIT virtual int onExit(AbilityEnum ability, int battler)
#define DELEGATE_EXIT ability, battler
    ON_EXIT = 0;
};

#define ON_CRIT virtual int onCrit(int battler, int target, MoveEnum move, u16 typeEffectiveness)
#define DELEGATE_CRIT battler, target, move, typeEffectiveness
APPLIES_ON(Crit, ApplyOnTarget, ON_CRIT)

#define ON_TYPE_EFFECTIVENESS virtual int onTypeEffectiveness(int defType, MoveEnum move, Type moveType, u16 *mod)
#define DELEGATE_TYPE_EFFECTIVENESS defType, move, moveType, mod
APPLIES_ON(TypeEffectiveness, ApplyOnTarget, ON_TYPE_EFFECTIVENESS)

struct OnCopyMove {
#define ON_COPY_MOVE virtual int onCopyMove(AbilityEnum ability, int battler, int attacker, int target, MoveEnum move)
#define DELEGATE_COPY_MOVE ability, battler, attacker, target, move
    ON_COPY_MOVE = 0;
};

#define ON_AFTER_TYPE_EFFECTIVENESS \
    virtual void onAfterTypeEffectiveness(int battler, AbilityEnum ability, int target, MoveEnum move, Type moveType, u16 *mod, u16 mod1, u16 mod2, u16 mod3)
#define DELEGATE_AFTER_TYPE_EFFECTIVENESS battler, target, move, moveType, mod, mod1, mod2, mod3
APPLIES_ON(AfterTypeEffectiveness, ApplyOnTarget, ON_AFTER_TYPE_EFFECTIVENESS)

#define ON_MODIFY_EFFECT_CHANCE virtual void onModifyEffectChance(int battler, MoveEnum move, MoveEffectEnum moveEffect, int *effectChance)
#define DELEGATE_MODIFY_EFFECT_CHANCE battler, move, moveEffect, effectChance
APPLIES_ON(ModifyEffectChance, ApplyOn, ON_MODIFY_EFFECT_CHANCE)

struct OnCanStatusType {
#define ON_CAN_STATUS_TYPE virtual int onCanStatusType(int battler, MoveEnum move, StatusCheckEnum status)
#define DELEGATE_CAN_STATUS_TYPE battler, move, status
    ON_CAN_STATUS_TYPE = 0;
};

#define ON_STATUS_IMMUNE virtual int onStatusImmune(int battler, int target, AbilityEnum ability, StatusCheckEnum status)
#define DELEGATE_STATUS_IMMUNE int battler, target, ability, status
APPLIES_ON_BREAKABLE(StatusImmune, ApplyOn, ON_STATUS_IMMUNE)

struct OnTrap {
#define ON_TRAP virtual int onTrap(int switchingBattler)
#define DELEGATE_TRAP switchingBattler
    ON_TRAP = 0;
};

#define ON_BEFORE_ATTACK virtual int onBeforeAttack(int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType)
#define DELEGATE_BEFORE_ATTACK battler, attacker, ability, move, moveType
APPLIES_ON(BeforeAttack, ApplyOnTarget, ON_BEFORE_ATTACK)

struct OnPreemptAction {
#define ON_PREEMPT_ACTION virtual int onPreemptAction(u8 battler, AbilityEnum ability, u8 turnBattler)
#define DELEGATE_PREEMPT_ACTION battler, ability, turnBattler
    ON_PREEMPT_ACTION = 0;
};

struct OnModifyMoveFlags {
#define ON_MODIFY_MOVE_FLAGS_ARGS int battler, MoveEnum move, MoveFlag flag
#define ON_MODIFY_MOVE_FLAGS virtual int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS_ARGS)
#define DELEGATE_MODIFY_MOVE_FLAGS battler, move, flag
    ON_MODIFY_MOVE_FLAGS = 0;
};

struct OnMoldBreaker {
#define ON_MOLD_BREAKER virtual int onMoldBreaker(int battler, MoveEnum move)
#define DELEGATE_MOLD_BREAKER battler, move, moveType
    ON_MOLD_BREAKER = 0;
};

struct OnRevive : extends Persistent {
#define ON_REVIVE virtual int onRevive(int battler)
#define DELEGATE_REVIVE battler
    ON_REVIVE = 0;
};

template <MoveEffectEnum Effect>
struct SetStateOnEffect {
    MoveEffectEnum setStateOnEffect() { return Effect; }
};

template <TerrainType Terrain>
struct AllowTerrainIfAirborne {
    TerrainType allowTerrainIfAirborne() { return Terrain; }
};

#define ENUM_WRAPPER(Name, name, type, BaseType)             \
    struct Name##Base : extends BaseType {                   \
        virtual type name() = 0;                             \
    };                                                       \
    template <type Value>                                    \
    struct Name : extends Name##Base {                       \
        virtual type name() { return static_cast<type>(0); } \
    };

ENUM_WRAPPER(Redirects, redirectType, Type, OnAbsorb);

struct Placeholder {};

#define MERGE_OPERATOR(Name, name, ON_NAME, DELEGATE_NAME, op)                        \
    template <typename T, typename U>                                                 \
    struct Merge##Name##Impl : extends T, extends U {                                 \
        ON_NAME override { return T::name(DELEGATE_NAME) op U::name(DELEGATE_NAME); } \
    };                                                                                \
    template <typename T, typename U>                                                 \
    struct Merge##Name : extends std::conditional_t<BothAre<T, U, Name>, Merge##Name##Impl<T, U>, Placeholder> {};

#define MERGE_VOID(Name, name, ON_NAME, DELEGATE_NAME) \
    template <typename T, typename U>                  \
    struct Merge##Name##Impl : extends T, extends U {  \
        ON_NAME override {                             \
            T::name(DELEGATE_NAME);                    \
            U::name(DELEGATE_NAME);                    \
        }                                              \
    };                                                 \
    template <typename T, typename U>                  \
    struct Merge##Name : extends std::conditional_t<BothAre<T, U, Name>, Merge##Name##Impl<T, U>, Placeholder> {};

MERGE_OPERATOR(OnEntry, onEntry, ON_ENTRY, DELEGATE_ENTRY, |)
MERGE_VOID(OnDefender, onDefender, ON_DEFENDER, DELEGATE_DEFENDER)
MERGE_OPERATOR(OnExit, onExit, ON_EXIT, DELEGATE_EXIT, |)
MERGE_VOID(OnModifyEffectChanceBase, onModifyEffectChance, ON_MODIFY_EFFECT_CHANCE, DELEGATE_MODIFY_EFFECT_CHANCE)
MERGE_OPERATOR(OnAbsorb, onAbsorb, ON_ABSORB, DELEGATE_ABSORB, |)
MERGE_OPERATOR(OnTypeEffectivenessBase, onTypeEffectiveness, ON_TYPE_EFFECTIVENESS, DELEGATE_TYPE_EFFECTIVENESS, ||)
MERGE_OPERATOR(OnEndTurn, onEndTurn, ON_END_TURN, DELEGATE_END_TURN, |)
MERGE_VOID(OnStatBase, onStat, ON_STAT, DELEGATE_STAT)
MERGE_VOID(OnOffensiveMultiplierBase, onOffensiveMultliplier, ON_OFFENSIVE_MULTIPLIER, DELEGATE_OFFENSIVE_MULTIPLIER)

template <typename T, typename U>
struct MergedRaw : extends MergeOnEntry<T, U>,
                   extends MergeOnDefender<T, U>,
                   extends MergeOnExit<T, U>,
                   extends MergeOnModifyEffectChanceBase<T, U>,
                   extends MergeOnAbsorb<T, U>,
                   extends MergeOnTypeEffectivenessBase<T, U>,
                   extends MergeOnEndTurn<T, U>,
                   extends MergeOnStatBase<T, U>,
                   extends MergeOnOffensiveMultiplierBase<T, U> {};

template <AbilityEnum T, AbilityEnum U>
struct Merged : extends MergedRaw<AbilityImpl<T>, AbilityImpl<U>> {};