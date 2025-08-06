#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "constants/battle.h"
#include "generated/constants/abilities.h"

typedef enum {
    INFILTRATE_NONE = 0,
    INFILTRATE_SCREENS = 1 << 0,
    INFILTRATE_SUBSTITUTE = 1 << 1,
    INFILTRATE_BREAK_SCREENS = 1 << 2,
} InfiltrateType;

typedef enum {
    ABILITY_BS_PUSH_CURSOR_AND_CALLBACK,
    ABILITY_BS_CALL,
    ABILITY_BS_EXECUTE,
} AbilityCallType;

typedef enum {
    MULTIHIT_SINGLE,
    MULTIHIT_TWO_TO_FIVE,
    MULTIHIT_FOUR_OR_FIVE,
    MULTIHIT_TWO,
    MULTIHIT_THREE,
    MULTIHIT_FIVE,
    MULTIHIT_TRIPLE_KICK,
    MULTIHIT_TEN_CAN_MISS,
    MULTIHIT_TEN,
    MULTIHIT_BEAT_UP,
    PARENTAL_BOND_START,
    PARENTAL_BOND_HYPER_AGGRESSIVE = PARENTAL_BOND_START,
    PARENTAL_BOND_PRIMAL_MAW,
    PARENTAL_BOND_DUAL_WIELD,
    PARENTAL_BOND_MINION_CONTROL,
    PARENTAL_BOND_THREE_HEADED,
    PARENTAL_BOND_ICE_COLD_HUNTER,
} MultihitType;

typedef enum NonStackingState {
    NON_STACKING_NONE = 0,
    NON_STACKING_RUIN = 1 << 0,
} NonStackingState;

typedef enum {
    ACCURACY_NO_RESULT = 0,
    ACCURACY_MULTIPLICATIVE,
    ACCURACY_HITS_IF_POSSIBLE,
    ACCURACY_ALWAYS_MISSES,
    ACCURACY_ALWAYS_HITS,
} AccuracyPriority;

#define NEVER_CRIT -2
#define ALWAYS_CRIT 3

typedef enum {
    CHECK_NONE = 0,
    CHECK_SLEEP = 1,
    CHECK_POISON = 1 << 1,
    CHECK_BURN = 1 << 2,
    CHECK_PARALYSIS = 1 << 3,
    CHECK_FROSTBITE = 1 << 4,
    CHECK_BLEED = 1 << 5,
    CHECK_STATUS1 = CHECK_SLEEP | CHECK_POISON | CHECK_BURN | CHECK_PARALYSIS | CHECK_FROSTBITE | CHECK_BLEED,
    CHECK_CONFUSION = 1 << 6,
    CHECK_INFATUATE = 1 << 7,
    CHECK_RESTRICTING = 1 << 8,
    CHECK_HEAL_BLOCK = 1 << 9,
    CHECK_DRENCH = 1 << 10,
} StatusCheckEnum;

typedef enum MoveFlag {
    MOVE_FLAG_PUNCH = 1,
    MOVE_FLAG_KICK,
    MOVE_FLAG_SOUND,
    MOVE_FLAG_MEGA_LAUNCHER,
    MOVE_FLAG_DANCE,
} MoveFlag;

#ifdef __cplusplus
enum class AbilityApplyOn {
    APPLY_ON_SELF = 0,
    APPLY_IGNORE_SELF = 1 << 2,
    APPLY_ON_ALLY = 1 << 0,
    APPLY_ON_ALLY_ONLY = APPLY_ON_ALLY | APPLY_IGNORE_SELF,
    APPLY_ON_FOE_OR_SELF = 1 << 1,
    APPLY_ON_FOE = APPLY_ON_FOE_OR_SELF | APPLY_IGNORE_SELF,
    APPLY_ON_ANY = APPLY_ON_ALLY | APPLY_ON_FOE_OR_SELF,
    APPLY_ON_OTHER = APPLY_ON_ANY | APPLY_IGNORE_SELF,
};

enum class AbilityApplyOnWithTarget {
    APPLY_ON_SELF = 0,
    APPLY_IGNORE_SELF = 1 << 2,
    APPLY_ON_ALLY = 1 << 0,
    APPLY_ON_ALLY_ONLY = APPLY_ON_ALLY | APPLY_IGNORE_SELF,
    APPLY_ON_FOE_OR_SELF = 1 << 1,
    APPLY_ON_FOE = APPLY_ON_FOE_OR_SELF | APPLY_IGNORE_SELF,
    APPLY_ON_ANY = APPLY_ON_ALLY | APPLY_ON_FOE_OR_SELF,
    APPLY_ON_OTHER = APPLY_ON_ANY | APPLY_IGNORE_SELF,
    APPLY_ON_ATTACKER = 1 << 3,
    APPLY_ON_TARGET = 2 << 3,
    APPLY_ON_ATTACKER_OR_TARGET = APPLY_ON_ATTACKER | APPLY_ON_TARGET,
};

struct AbilityApplyOnHack {
   private:
    int value;

   public:
    AbilityApplyOnHack(int toConvert) { value = toConvert; }
    operator AbilityApplyOn() { return static_cast<AbilityApplyOn>(value); }
    operator AbilityApplyOnWithTarget() { return static_cast<AbilityApplyOnWithTarget>(value); }
};

#define APPLY_ON_SELF AbilityApplyOnHack(0)
#define APPLY_IGNORE_SELF AbilityApplyOnHack(1 << 2)
#define APPLY_ON_ALLY AbilityApplyOnHack(1 << 0)
#define APPLY_ON_ALLY_ONLY AbilityApplyOnHack(APPLY_ON_ALLY | APPLY_IGNORE_SELF)
#define APPLY_ON_FOE_OR_SELF AbilityApplyOnHack(1 << 1)
#define APPLY_ON_FOE AbilityApplyOnHack(APPLY_ON_FOE_OR_SELF | APPLY_IGNORE_SELF)
#define APPLY_ON_ANY AbilityApplyOnHack(APPLY_ON_ALLY | APPLY_ON_FOE_OR_SELF)
#define APPLY_ON_OTHER AbilityApplyOnHack(APPLY_ON_ANY | APPLY_IGNORE_SELF)
#define APPLY_ON_ATTACKER AbilityApplyOnWithTarget::APPLY_ON_ATTACKER
#define APPLY_ON_TARGET AbilityApplyOnWithTarget::APPLY_ON_TARGET
#define APPLY_ON_ATTACKER_OR_TARGET AbilityApplyOnWithTarget::APPLY_ON_ATTACKER_OR_TARGET

#else
typedef enum {
    APPLY_ON_SELF = 0,
    APPLY_IGNORE_SELF = 1 << 2,
    APPLY_ON_ALLY = 1 << 0,
    APPLY_ON_ALLY_ONLY = APPLY_ON_ALLY | APPLY_IGNORE_SELF,
    APPLY_ON_FOE_OR_SELF = 1 << 1,
    APPLY_ON_FOE = APPLY_ON_FOE_OR_SELF | APPLY_IGNORE_SELF,
    APPLY_ON_ANY = APPLY_ON_ALLY | APPLY_ON_FOE_OR_SELF,
    APPLY_ON_OTHER = APPLY_ON_ANY | APPLY_IGNORE_SELF,
} AbilityApplyOn;

typedef enum {
    APPLY_ON_ATTACKER = 1 << 3,
    APPLY_ON_TARGET = 2 << 3,
    APPLY_ON_ATTACKER_OR_TARGET = APPLY_ON_ATTACKER | APPLY_ON_TARGET,
} AbilityApplyOnWithTarget;
#endif

typedef enum {
    TERRAIN_NONE = 0,
    TERRAIN_GRASSY = 1,
    TERRAIN_ELECTRIC = 2,
    TERRAIN_PSYCHIC = 3,
    TERRAIN_MISTY = 4,
    TERRAIN_TOXIC = 5,
} TerrainType;

class Ability {
    virtual AbilityEnum id() { return ABILITY_NONE; }
#define ON_ENTRY AbilityEnum ability, int battler
#define DELEGATE_ENTRY ability, battler
    virtual int onEntry(ON_ENTRY) { return 0; }
#define ON_ABSORB int battler, MoveEnum move, Type moveType, int *statId
#define DELEGATE_ABSORB battler, move, moveType, statId
    virtual int onAbsorb(ON_ABSORB) { return 0; }
#define ON_IMMUNE int battler, int attacker, MoveEnum move, Type moveType, const u8 **immunityScript
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
    virtual int onImmune(ON_IMMUNE) { return 0; }
    virtual AbilityApplyOn onImmuneFor() { return APPLY_ON_SELF; }
#define ON_INFILTRATE int battler, MoveEnum move
#define DELEGATE_INFILTRATE battler, move
    virtual InfiltrateType onInfiltrate(ON_INFILTRATE) { return INFILTRATE_NONE; }
#define ON_DISGUISE int battler, int testOnly
#define DELEGATE_DISGUISE battler, testOnly
    virtual SpeciesEnum onDisguise(ON_DISGUISE) { return SPECIES_NONE; }
#define ON_WEATHER AbilityEnum ability, int battler
#define DELEGATE_WEATHER ability, battler
    virtual int onWeather(ON_WEATHER) { return 0; }
#define ON_TERRAIN AbilityEnum ability, int battler
#define DELEGATE_TERRAIN ability, battler
    virtual int onTerrain(ON_TERRAIN) { return 0; }
#define ON_END_TURN AbilityEnum ability, int battler
#define DELEGATE_END_TURN ability, battler
    virtual int onEndTurn(ON_END_TURN) { return 0; }
#define ON_ATTACKER AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
    virtual int onAttacker(ON_ATTACKER) { return 0; }
#define ON_DEFENDER AbilityEnum ability, int battler, int attacker, MoveEnum move, Type moveType
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
    virtual int onDefender(ON_DEFENDER) { return 0; }
#define ON_EITHER                                                                                      \
    static int onEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, Type moveType); \
    int onAttacker(ON_ATTACKER) override { return onEither(DELEGATE_ATTACKER); }                       \
    int onDefender(ON_DEFENDER) override { return onEither(DELEGATE_DEFENDER); }                       \
    static int onEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, Type moveType)

#define ON_RECOIL int damage, int battler, Type moveType
#define DELEGATE_RECOIL damage, battler, moveType
    virtual int onRecoil(ON_RECOIL) { return 0; }
#define ON_REACTIVE AbilityEnum ability, int battler, AbilityCallType callType
#define DELEGATE_REACTIVE ability, battler
    virtual int onReactive(ON_REACTIVE) { return 0; }
#define ON_BATTLER_FAINTS AbilityEnum ability, int battler, int attacker, int fainted, MoveEnum move, Type moveType
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
    virtual int onBattlerFaints(ON_BATTLER_FAINTS) { return 0; }
    virtual AbilityApplyOnWithTarget onBattlerFaintsFor() { return APPLY_ON_SELF; }
#define ON_PARENTAL_BOND int battler, MoveEnum move, Type moveType
#define DELEGATE_PARENTAL_BOND battler, move, moveType
    virtual MultihitType onParentalBond(ON_PARENTAL_BOND) { return MULTIHIT_SINGLE; }
#define ON_OFFENSIVE_MULTIPLIER                                                                                                                              \
    int battler, AbilityEnum ability, int target, MoveEnum move, Type moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16 *resistance, \
        u16 *modifier
#define DELEGATE_OFFENSIVE_MULTIPLIER battler, ability, target, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier
    virtual void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) { return; }
    virtual AbilityApplyOn onOffensiveMultiplierFor() { return APPLY_ON_SELF; }
#define ON_DEFENSIVE_MULTIPLIER \
    int battler, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16 *resistance, u16 *modifier
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
    virtual void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) { return; }
#define ON_MOVE_TYPE AbilityEnum ability, MoveEnum move, Type moveType, u8 *ateBoost
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
    virtual int onMoveType(ON_MOVE_TYPE) { return 0; }
#define ON_STAB Type moveType
#define DELEGATE_STAB moveType
    virtual int onStab(ON_STAB) { return 0; }
#define ON_STAT AbilityEnum ability, int battler, int statId, u32 *stat, NonStackingState *flags
#define DELEGATE_STAT ability, battler, statId, stat, flags
    virtual void onStat(ON_STAT) { return; }
    virtual AbilityApplyOn onStatFor() { return APPLY_ON_SELF; }
#define ON_ACCURACY AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType, int *accuracy
#define DELEGATE_ACCURACY ability, battler, target, move, moveType, accuracy
    virtual AccuracyPriority onAccuracy(ON_ACCURACY) { return ACCURACY_NO_RESULT; }
    virtual AbilityApplyOnWithTarget onAccuracyFor() { return APPLY_ON_SELF; }
#define ON_SWAP_SPLIT int battler, MoveEnum move
#define DELEGATE_SWAP_SPLIT battler, move
    virtual int onSwapSplit(ON_SWAP_SPLIT) { return 0; }
#define ON_CHOOSE_OFFENSIVE_STAT \
    int battler, MoveEnum move, int ignoreOffensiveStatDrops, int targetUnaware, u8 *atkStatToUse, u8 secondaryAtkStatToUse[NUM_STATS]
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
    virtual void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) { return; }
#define ON_CHOOSE_DEFENSIVE_STAT int battler, int target, MoveEnum move, int ignoreDefensiveStatBoosts, int battlerUnaware
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
    virtual int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) { return 0; }
    virtual AbilityApplyOnWithTarget onChooseDefensiveStatFor() { return APPLY_ON_SELF; }
#define ON_PRIORITY int battler, int target, MoveEnum move
#define DELEGATE_PRIORITY battler, target, move
    virtual int onPriority(ON_PRIORITY) { return 0; }
#define ON_EXIT AbilityEnum ability, int battler
#define DELEGATE_EXIT ability, battler
    virtual int onExit(ON_EXIT) { return 0; }
#define ON_CRIT int battler, int target, MoveEnum move, u16 typeEffectiveness
#define DELEGATE_CRIT battler, target, move, typeEffectiveness
    virtual int onCrit(ON_CRIT) { return 0; }
    virtual AbilityApplyOnWithTarget onCritFor() { return APPLY_ON_SELF; }
#define ON_TYPE_EFFECTIVENESS int defType, MoveEnum move, Type moveType, u16 *mod
#define DELEGATE_TYPE_EFFECTIVENESS defType, move, moveType, mod
    virtual int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) { return 0; }
    virtual AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() { return APPLY_ON_SELF; }
#define ON_COPY_MOVE AbilityEnum ability, int battler, int attacker, int target, MoveEnum move
#define DELEGATE_COPY_MOVE ability, battler, attacker, target, move
    virtual int onCopyMove(ON_COPY_MOVE) { return 0; }
#define ON_AFTER_TYPE_EFFECTIVENESS int battler, AbilityEnum ability, int target, MoveEnum move, Type moveType, u16 *mod, u16 mod1, u16 mod2, u16 mod3
#define DELEGATE_AFTER_TYPE_EFFECTIVENESS battler, target, move, moveType, mod, mod1, mod2, mod3
    virtual void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) { return; }
#define ON_MODIFY_EFFECT_CHANCE int battler, MoveEnum move, MoveEffectEnum moveEffect, int *effectChance
#define DELEGATE_MODIFY_EFFECT_CHANCE battler, move, moveEffect, effectChance
    virtual void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) { return; }
    virtual AbilityApplyOn onModifyEffectChanceFor() { return APPLY_ON_SELF; }
#define ABILITY_ON_CAN_STATUS_TYPE int battler, MoveEnum move, StatusCheckEnum status
#define DELEGATE_ON_CAN_STATUS_TYPE battler, move, status
    virtual int onCanStatusType(ABILITY_ON_CAN_STATUS_TYPE) { return 0; }
#define ABILITY_ON_STATUS_IMMUNE int battler, int target, AbilityEnum ability, StatusCheckEnum status
#define DELEGATE_ON_STATUS_IMMUNE int battler, target, ability, status
    virtual int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) { return 0; }
    virtual AbilityApplyOn onStatusImmuneFor() { return APPLY_ON_SELF; }
#define ABILITY_ON_TRAP int switchingBattler
#define DELEGATE_ON_TRAP switchingBattler
    virtual int onTrap(ABILITY_ON_TRAP) { return 0; }
#define ABILITY_ON_BEFORE_ATTACK int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType
#define DELEGATE_ON_BEFORE_ATTACK battler, attacker, ability, move, moveType
    virtual int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) { return 0; }
    virtual AbilityApplyOnWithTarget onBeforeAttackFor() { return APPLY_ON_SELF; }
#define ON_PREEMPT_ACTION u8 battler, AbilityEnum ability, u8 turnBattler
#define DELEGATE_PREEMPT_ACTION battler, ability, turnBattler
    virtual int onPreemptAction(ON_PREEMPT_ACTION) { return 0; }
#define ON_MODIFY_MOVE_FLAGS int battler, MoveEnum move, MoveFlag flag
#define DELEGATE_MODIFY_MOVE_FLAGS battler, move, flag
    virtual int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) { return 0; }
#define ON_MOLD_BREAKER int battler, MoveEnum move
#define DELEGATE_MOLD_BREAKER battler, move, moveType
    virtual int onMoldBreaker(ON_MOLD_BREAKER) { return 0; }
#define ON_REVIVE int battler
#define DELEGATE_REVIVE battler
    virtual int onRevive(ON_REVIVE) { return 0; }
    virtual MoveEffectEnum setStateOnEffect() { return MOVE_EFFECT_NONE; }
    virtual TerrainType allowTerrainIfAirborne() { return TERRAIN_NONE; }
    virtual Type redirectType() { return TYPE_NONE; }
    virtual int ruinStat() { return 0; }
    virtual int noDamageHits() { return false; }
    virtual bool breakable() { return false; }
    virtual bool unsuppressable() { return false; }
    virtual bool persistent() { return false; }
    virtual bool randomizerBanned() { return false; }
    virtual bool unaware() { return false; }
    virtual bool absorbUp2() { return false; }
    virtual bool isSoundproof() { return false; }
    virtual bool magicGuard() { return false; }
    virtual bool noRecoil() { return false; }
    virtual bool halfRecoil() { return false; }
    virtual bool chloroplast() { return false; }
    virtual bool skillLink() { return false; }
    virtual bool resistsFortKnox() { return false; }
    virtual bool fortKnox() { return false; }
    virtual bool adaptability() { return false; }
    virtual bool magicBounce() { return false; }
    virtual bool levitate() { return false; }
    virtual bool megaLauncherBoost() { return false; }
    virtual bool unnerve() { return false; }
    virtual bool negatesBurnAtkDrop() { return false; }
    virtual bool negatesFrzSpatkDrop() { return false; }
    virtual bool canInfatuateAny() { return false; }
    virtual bool removesStatusOnImmunity() { return false; }
    virtual bool tauntImmune() { return false; }
    virtual bool shadowTag() { return false; }
    virtual bool foesMinRoll() { return false; }
    virtual bool powderImmune() { return false; }
    virtual bool sandImmune() { return false; }
    virtual bool hailImmune() { return false; }
    virtual bool toxicTerrainImmune() { return false; }
    virtual bool stealthRockImmune() { return false; }
};

typedef struct AbilitiesWrapper {
    Ability abilities[ABILITIES_COUNT];
} AbilitiesWrapper;

extern const AbilitiesWrapper gAbilitiesWrapper;
#define gAbilities gAbilitiesWrapper.abilities

int IsApplyOnFlagAppropriate(int contextBattler, int sourceBattler, AbilityApplyOn flag);
int IsTargettedApplyOnFlagAppropriate(int contextBattler, int sourceBattler, int attacker, int target, AbilityApplyOnWithTarget flag);
int DoesMoveMatchFlag(int battler, MoveEnum move, MoveFlag flag);

#ifdef __cplusplus
}
#endif