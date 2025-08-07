#include "behavior/ability/constants.hh"
#include <concepts>

#define extends \
   public       \
    virtual

#define APPLIES_ON(name, applyType, ON_NAME)              \
    class On##name##Base : extends Ability {              \
        ON_NAME = 0;                                      \
        virtual applyType on##name##For() = 0;            \
    };                                                    \
    template <applyType For = applyType::SELF>            \
    class On##name : extends On##name##Base {             \
        virtual applyType on##name##For() { return For; } \
    };

#define APPLIES_ON_BREAKABLE(name, applyType, ON_NAME)    \
    class On##name##Base : extends Breakable {            \
        ON_NAME = 0;                                      \
        virtual applyType on##name##For() = 0;            \
    };                                                    \
    template <applyType For = applyType::SELF>            \
    class On##name : extends On##name##Base {             \
        virtual applyType on##name##For() { return For; } \
    };

template <typename T, typename U, typename Target>
concept BothAre = std::is_assignable_v<Target, T> && std::is_assignable_v<Target, U>;

class OnEntry : extends Ability {
#define ON_ENTRY virtual int onEntry(AbilityEnum ability, int battler)
#define DELEGATE_ENTRY ability, battler
    ON_ENTRY = 0;
};

class OnAbsorb : extends Breakable {
#define ON_ABSORB virtual int onAbsorb(int battler, MoveEnum move, Type moveType, int *statId)
#define DELEGATE_ABSORB battler, move, moveType, statId
    ON_ABSORB { return 0; }
};

#define ON_IMMUNE virtual int onImmune(int battler, int attacker, MoveEnum move, Type moveType, const u8 **immunityScript)
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
APPLIES_ON_BREAKABLE(Immune, ApplyOn, ON_IMMUNE)

class OnInfiltrate : extends Ability {
#define ON_INFILTRATE virtual InfiltrateType onInfiltrate(int battler, MoveEnum move)
#define DELEGATE_INFILTRATE battler, move
    ON_INFILTRATE = 0;
};

class OnDisguise : extends Ability {
#define ON_DISGUISE virtual SpeciesEnum onDisguise(int battler, int testOnly)
#define DELEGATE_DISGUISE battler, testOnly
    ON_DISGUISE = 0;
};

class OnWeather : extends Ability {
#define ON_WEATHER virtual int onWeather(AbilityEnum ability, int battler)
#define DELEGATE_WEATHER ability, battler
    ON_WEATHER = 0;
};

class OnTerrain : extends Ability {
#define ON_TERRAIN virtual int onTerrain(AbilityEnum ability, int battler)
#define DELEGATE_TERRAIN ability, battler
    ON_TERRAIN = 0;
};

class OnEndTurn : extends Ability {
#define ON_END_TURN virtual int onEndTurn(AbilityEnum ability, int battler)
#define DELEGATE_END_TURN ability, battler
    ON_END_TURN = 0;
};

class OnAttacker : extends Ability {
#define ON_ATTACKER virtual int onAttacker(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType)
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
    ON_ATTACKER = 0;
};

class OnDefender : extends Ability {
#define ON_DEFENDER virtual int onDefender(AbilityEnum ability, int battler, int attacker, MoveEnum move, Type moveType)
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
    ON_DEFENDER = 0;
};

class OnEither : extends OnAttacker, extends OnDefender {};
#define ON_EITHER                                                                                      \
    static int onEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, Type moveType); \
    ON_ATTACKER { return onEither(DELEGATE_ATTACKER); }                                                \
    ON_DEFENDER { return onEither(DELEGATE_DEFENDER); }                                                \
    static int onEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, Type moveType)

class OnRecoil : extends Ability {
#define ON_RECOIL virtual int onRecoil(int damage, int battler, Type moveType)
#define DELEGATE_RECOIL damage, battler, moveType
    ON_RECOIL = 0;
};

class OnReactive : extends Ability {
#define ON_REACTIVE virtual int onReactive(AbilityEnum ability, int battler, AbilityCallType callType)
#define DELEGATE_REACTIVE ability, battler
    ON_REACTIVE = 0;
};

#define ON_BATTLER_FAINTS virtual int onBattlerFaints(AbilityEnum ability, int battler, int attacker, int fainted, MoveEnum move, Type moveType)
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
#define SELF ATTACKER
APPLIES_ON(BattlerFaints, ApplyOnTarget, ON_BATTLER_FAINTS)
#undef SELF

class OnParentalBond : extends Ability {
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

class OnDefensiveMultiplier : extends Breakable {
#define ON_DEFENSIVE_MULTIPLIER         \
    virtual void onDefensiveMultiplier( \
        int battler, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16 *resistance, u16 *modifier)
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
    ON_DEFENSIVE_MULTIPLIER = 0;
};

class OnMoveType : extends Ability {
#define ON_MOVE_TYPE virtual int onMoveType(AbilityEnum ability, MoveEnum move, Type moveType, u8 *ateBoost)
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
    ON_MOVE_TYPE = 0;
};

class OnStab : extends Ability {
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

class OnSwapSplit : extends Ability {
#define ON_SWAP_SPLIT virtual int onSwapSplit(int battler, MoveEnum move)
#define DELEGATE_SWAP_SPLIT battler, move
    ON_SWAP_SPLIT = 0;
};

class OnChooseOffensiveStat : extends Ability {
#define ON_CHOOSE_OFFENSIVE_STAT        \
    virtual void onChooseOffensiveStat( \
        int battler, MoveEnum move, int ignoreOffensiveStatDrops, int targetUnaware, u8 *atkStatToUse, u8 secondaryAtkStatToUse[NUM_STATS])
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
    ON_CHOOSE_OFFENSIVE_STAT = 0;
};

#define ON_CHOOSE_DEFENSIVE_STAT virtual int onChooseDefensiveStat(int battler, int target, MoveEnum move, int ignoreDefensiveStatBoosts, int battlerUnaware)
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
APPLIES_ON(ChooseDefensiveStat, ApplyOnTarget, ON_CHOOSE_DEFENSIVE_STAT)

class OnPriority : extends Ability {
#define ON_PRIORITY virtual int onPriority(int battler, int target, MoveEnum move)
#define DELEGATE_PRIORITY battler, target, move
    ON_PRIORITY = 0;
};

class OnExit : extends Ability {
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

class OnCopyMove : extends Ability {
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

class OnCanStatusType : extends Ability {
#define ON_CAN_STATUS_TYPE virtual int onCanStatusType(int battler, MoveEnum move, StatusCheckEnum status)
#define DELEGATE_CAN_STATUS_TYPE battler, move, status
    ON_CAN_STATUS_TYPE = 0;
};

#define ON_STATUS_IMMUNE virtual int onStatusImmune(int battler, int target, AbilityEnum ability, StatusCheckEnum status)
#define DELEGATE_STATUS_IMMUNE int battler, target, ability, status
APPLIES_ON_BREAKABLE(StatusImmune, ApplyOn, ON_STATUS_IMMUNE)

class OnTrap : extends Ability {
#define ON_TRAP virtual int onTrap(int switchingBattler)
#define DELEGATE_TRAP switchingBattler
    ON_TRAP = 0;
};

#define ON_BEFORE_ATTACK virtual int onBeforeAttack(int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType)
#define DELEGATE_BEFORE_ATTACK battler, attacker, ability, move, moveType
APPLIES_ON(BeforeAttack, ApplyOnTarget, ON_BEFORE_ATTACK)

class OnPreemptAction : extends Ability {
#define ON_PREEMPT_ACTION virtual int onPreemptAction(u8 battler, AbilityEnum ability, u8 turnBattler)
#define DELEGATE_PREEMPT_ACTION battler, ability, turnBattler
    ON_PREEMPT_ACTION = 0;
};

class OnModifyMoveFlags : extends Ability {
#define ON_MODIFY_MOVE_FLAGS virtual int onModifyMoveFlags(int battler, MoveEnum move, MoveFlag flag)
#define DELEGATE_MODIFY_MOVE_FLAGS battler, move, flag
    ON_MODIFY_MOVE_FLAGS = 0;
};

class OnMoldBreaker : extends Ability {
#define ON_MOLD_BREAKER virtual int onMoldBreaker(int battler, MoveEnum move)
#define DELEGATE_MOLD_BREAKER battler, move, moveType
    ON_MOLD_BREAKER = 0;
};

class OnRevive : extends Persistent {
#define ON_REVIVE virtual int onRevive(int battler)
#define DELEGATE_REVIVE battler
    ON_REVIVE = 0;
};

template <MoveEffectEnum Effect>
class SetStateOnEffect : extends Ability {
    MoveEffectEnum setStateOnEffect() { return Effect; }
};

template <TerrainType Terrain>
class AllowTerrainIfAirborne : extends Ability {
    TerrainType allowTerrainIfAirborne() { return Terrain; }
};

#define ENUM_WRAPPER(Name, name, type, BaseType)             \
    class Name##Base : extends BaseType {                    \
        virtual type name() = 0;                             \
    };                                                       \
    template <type Value>                                    \
    class Name : extends name {                              \
        virtual type name() { return static_cast<type>(0); } \
    };

ENUM_WRAPPER(Redirects, redirectType, Type, OnAbsorb);
ENUM_WRAPPER(NoDamageHits, noDamageHits, int, Ability);

class Breakable : public virtual Ability {
    virtual bool breakable() { return true; }
};
class OverrideBreakable : public virtual Breakable {
    virtual bool breakable() { return false; }
};
class RandomizerBanned : extends Ability {};
class NotImplemented : extends RandomizerBanned {};
class Unsuppressable : extends Ability {};
class Persistent : extends Ability {};
class FormChange : extends RandomizerBanned, extends Unsuppressable {};

#define MERGE_OPERATOR(Name, name, ON_NAME, DELEGATE_NAME, op)                        \
    template <typename T, typename U>                                                 \
    class Merge##Name##Impl : extends T, extends U {                                  \
        ON_NAME override { return T::name(DELEGATE_NAME) op U::name(DELEGATE_NAME); } \
    };                                                                                \
    template <typename T, typename U>                                                 \
    class Merge##Name : extends std::conditional<BothAre<T, U, OnEntry>, Merge##Name##Impl<T, U>, Ability> {};

#define MERGE_VOID(Name, name, ON_NAME, DELEGATE_NAME) \
    template <typename T, typename U>                  \
    class Merge##Name##Impl : extends T, extends U {   \
        ON_NAME override {                             \
            T::name(DELEGATE_NAME);                    \
            U::name(DELEGATE_NAME);                    \
        }                                              \
    };                                                 \
    template <typename T, typename U>                  \
    class Merge##Name : extends std::conditional<BothAre<T, U, OnEntry>, Merge##Name##Impl<T, U>, Ability> {};

MERGE_OPERATOR(OnEntry, onEntry, ON_ENTRY, DELEGATE_ENTRY, |)
MERGE_VOID(OnDefender, onDefender, ON_DEFENDER, DELEGATE_DEFENDER)
MERGE_OPERATOR(OnExit, onExit, ON_EXIT, DELEGATE_EXIT, |)
MERGE_VOID(OnModifyEffectChanceBase, onModifyEffectChance, ON_MODIFY_EFFECT_CHANCE, DELEGATE_MODIFY_EFFECT_CHANCE)
MERGE_OPERATOR(OnEntry, onEntry, ON_ENTRY, DELEGATE_ENTRY, |)
MERGE_OPERATOR(OnAbsorb, onAbsorb, ON_ABSORB, DELEGATE_ABSORB, |)
MERGE_OPERATOR(OnTypeEffectivenessBase, onTypeEffectiveness, ON_TYPE_EFFECTIVENESS, DELEGATE_TYPE_EFFECTIVENESS, ||)
MERGE_OPERATOR(OnEndTurn, onEndTurn, ON_END_TURN, DELEGATE_END_TURN, |)
MERGE_VOID(OnStatBase, onStat, ON_STAT, DELEGATE_STAT)

template <typename T, typename U>
class Merged : extends T,
               extends U,
               extends MergeOnEntry<T, U>,
               extends MergeOnDefender<T, U>,
               extends MergeOnExit<T, U>,
               extends MergeOnModifyEffectChanceBase<T, U>,
               extends MergeOnEntry<T, U>,
               extends MergeOnAbsorb<T, U>,
               extends MergeOnTypeEffectivenessBase<T, U>,
               extends MergeOnEndTurn<T, U>,
               extends MergeOnStatBase<T, U> {};