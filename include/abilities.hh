#ifndef GUARD_ABILITIES_H
#define GUARD_ABILITIES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "constants/battle.h"
#include "generated/constants/abilities.h"

typedef int (*AbilityOnEntryHandler)(AbilityEnum ability, int battler);
typedef int (*AbilityOnAbsorbHandler)(int battler, MoveEnum move, Type moveType, int* statId);
typedef int (*AbilityOnImmuneHandler)(int battler, int attacker, MoveEnum move, Type moveType, const u8** immunityScript);
typedef enum {
    INFILTRATE_NONE = 0,
    INFILTRATE_SCREENS = 1 << 0,
    INFILTRATE_SUBSTITUTE = 1 << 1,
    INFILTRATE_BREAK_SCREENS = 1 << 2,
} InfiltrateType;
typedef InfiltrateType (*AbilityOnInfiltrateHandler)(int battler, MoveEnum move, Type moveType);
typedef SpeciesEnum (*AbilityOnDisguiseHandler)(int battler, int checkOnly);
typedef int (*AbilityOnWeatherHandler)(AbilityEnum ability, int battler);
typedef int (*AbilityOnTerrainHandler)(AbilityEnum ability, int battler);
typedef int (*AbilityOnEndTurnHandler)(AbilityEnum ability, u8 battler);
typedef int (*AbilityOnAttackerHandler)(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType);
typedef int (*AbilityOnDefenderHandler)(AbilityEnum ability, int battler, int attacker, MoveEnum move, Type moveType);
typedef int (*AbilityOnRecoilHandler)(int damage, int battler, Type moveType);
typedef enum {
    ABILITY_BS_PUSH_CURSOR_AND_CALLBACK,
    ABILITY_BS_CALL,
    ABILITY_BS_EXECUTE,
} AbilityCallType;
typedef int (*AbilityOnReactiveHandler)(AbilityEnum ability, int battler, AbilityCallType callType);
typedef int (*AbilityOnBattlerFaintsHandler)(AbilityEnum ability, int battler, int attacker, int fainted, MoveEnum move, Type moveType);
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
typedef MultihitType (*AbilityOnParentalBondHandler)(int battler, MoveEnum move, Type moveType);
typedef void (*AbilityOnOffensiveMultiplierHandler)(int battler,
                                                    AbilityEnum ability,
                                                    int target,
                                                    MoveEnum move,
                                                    Type moveType,
                                                    int basePower,
                                                    int typeEffectivenessMultiplier,
                                                    int isCrit,
                                                    u16* resistance,
                                                    u16* modifier);
typedef void (*AbilityOnDefensiveMultiplierHandler)(
    int battler, AbilityEnum ability, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier);
typedef enum NonStackingState {
    NON_STACKING_NONE = 0,
    NON_STACKING_RUIN = 1 << 0,
    NON_STACKING_ETERNAL_FLOWER = 1 << 1,
} NonStackingState;
typedef void (*AbilityOnStatHandler)(AbilityEnum ability, int battler, int statId, u32* stat, NonStackingState* flags);
typedef enum {
    ACCURACY_NO_RESULT = 0,
    ACCURACY_MULTIPLICATIVE,
    ACCURACY_HITS_IF_POSSIBLE,
    ACCURACY_ALWAYS_MISSES,
    ACCURACY_ALWAYS_HITS,
} AccuracyPriority;
typedef AccuracyPriority (*AbilityOnAccuracyHandler)(AbilityEnum ability, int battler, int target, MoveEnum move, Type moveType, int* accuracy);
typedef int (*AbilityOnSwapSplitHandler)(int battler, MoveEnum move, Type moveType);
typedef void (*AbilityOnChooseOffensiveStat)(
    int battler, MoveEnum move, int ignoreOffensiveStatDrops, int targetUnaware, u8* atkStatToUse, u8 secondaryAtkStatToUse[NUM_STATS]);
typedef int (*AbilityOnChooseDefensiveStat)(int battler, int target, MoveEnum move, int ignoreDefensiveStatBoosts, int battlerUnaware);
typedef int (*AbilityOnStab)(Type moveType);
typedef int (*AbilityOnPriority)(int battler, int target, MoveEnum move);
typedef int (*AbilityOnMoveType)(AbilityEnum ability, MoveEnum move, Type moveType, u8* ateBoost);
typedef int (*AbilityOnSwitchOut)(AbilityEnum ability, int battler);
typedef int (*AbilityOnCrit)(int battler, int target, MoveEnum move, u16 typeEffectiveness);
#define NEVER_CRIT -2
#define ALWAYS_CRIT 3
typedef int (*AbilityOnTypeEffectiveness)(int defType, MoveEnum move, Type moveType, u16* mod);
typedef int (*AbilityOnCopyMove)(AbilityEnum ability, int battler, int attacker, int target, MoveEnum move);
typedef void (*AbilityOnAfterTypeEffectiveness)(
    int battler, AbilityEnum ability, int target, MoveEnum move, Type moveType, u16* mod, u16 mod1, u16 mod2, u16 mod3);
typedef void (*AbilityOnModifyEffectChance)(int battler, MoveEnum move, MoveEffectEnum moveEffect, int* effectChance);
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
typedef int (*AbilityOnCanStatusType)(int battler, MoveEnum move, StatusCheckEnum status);
typedef int (*AbilityOnStatusImmune)(int battler, int target, AbilityEnum ability, StatusCheckEnum status);
typedef int (*AbilityOnTrap)(int escapingBattler);
typedef int (*AbilityOnBeforeAttack)(int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType);
typedef int (*AbilityOnPreemptAction)(u8 battler, AbilityEnum ability, u8 turnBattler);
typedef enum MoveFlag {
    MOVE_FLAG_PUNCH = 1,
    MOVE_FLAG_KICK,
    MOVE_FLAG_SOUND,
    MOVE_FLAG_MEGA_LAUNCHER,
    MOVE_FLAG_DANCE,
    MOVE_FLAG_KEEN_EDGE,
} MoveFlag;
typedef int (*AbilityOnModifyMoveFlags)(int battler, MoveEnum move, Type type, MoveFlag flag);
typedef int (*AbilityOnMoldBreaker)(int battler, MoveEnum move);
typedef int (*AbilityOnRevive)(int battler);

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

typedef enum {
    TERRAIN_NONE = 0,
    TERRAIN_GRASSY = 1 << 0,
    TERRAIN_ELECTRIC = 1 << 1,
    TERRAIN_PSYCHIC = 1 << 2,
    TERRAIN_MISTY = 1 << 3,
    TERRAIN_TOXIC = 1 << 4,
} TerrainType;

#ifdef __cplusplus
#define AbilityApplyOnWithTarget int
// Because Type is defined in a C header file C++ and C end up with different alignments for the enum when packed into a bit field for some reason
#define Type u16
#else
#endif

typedef struct Ability {
    const u8* name;
    const u8* description;
    const u8* expandedDescription;
    AbilityOnEntryHandler onEntry;
    AbilityOnAbsorbHandler onAbsorb;
    AbilityOnImmuneHandler onImmune;
    AbilityOnInfiltrateHandler onInfiltrate;
    AbilityOnDisguiseHandler onDisguise;
    AbilityOnWeatherHandler onWeather;
    AbilityOnTerrainHandler onTerrain;
    AbilityOnEndTurnHandler onEndTurn;
    AbilityOnAttackerHandler onAttacker;
    AbilityOnDefenderHandler onDefender;
    AbilityOnRecoilHandler onRecoil;
    AbilityOnReactiveHandler onReactive;
    AbilityOnBattlerFaintsHandler onBattlerFaints;
    AbilityOnParentalBondHandler onParentalBond;
    AbilityOnOffensiveMultiplierHandler onOffensiveMultiplier;
    AbilityOnDefensiveMultiplierHandler onDefensiveMultiplier;
    AbilityOnMoveType onMoveType;
    AbilityOnStab onStab;
    AbilityOnStatHandler onStat;
    AbilityOnAccuracyHandler onAccuracy;
    AbilityOnSwapSplitHandler onSwapSplit;
    AbilityOnChooseOffensiveStat onChooseOffensiveStat;
    AbilityOnChooseDefensiveStat onChooseDefensiveStat;
    AbilityOnPriority onPriority;
    AbilityOnSwitchOut onExit;
    AbilityOnCrit onCrit;
    AbilityOnTypeEffectiveness onTypeEffectiveness;
    AbilityOnCopyMove onCopyMove;
    AbilityOnAfterTypeEffectiveness onAfterTypeEffectiveness;
    AbilityOnModifyEffectChance onModifyEffectChance;
    AbilityOnCanStatusType onCanStatusType;
    AbilityOnStatusImmune onStatusImmune;
    AbilityOnTrap onTrap;
    AbilityOnBeforeAttack onBeforeAttack;
    AbilityOnPreemptAction onPreemptAction;
    AbilityOnModifyMoveFlags onModifyMoveFlags;
    AbilityOnMoldBreaker onMoldBreaker;
    AbilityOnRevive onRevive;
    AbilityApplyOn onImmuneFor:3;
    AbilityApplyOnWithTarget onBattlerFaintsFor:5;
    AbilityApplyOn onOffensiveMultiplierFor:3;
    AbilityApplyOn onStatFor:3;
    AbilityApplyOnWithTarget onAccuracyFor:5;
    AbilityApplyOnWithTarget onChooseDefensiveStatFor:5;
    AbilityApplyOnWithTarget onCritFor:5;
    AbilityApplyOnWithTarget onAfterTypeEffectivenessFor:5;
    AbilityApplyOn onModifyEffectChanceFor:3;
    AbilityApplyOn onStatusImmuneFor:3;
    AbilityApplyOnWithTarget onBeforeAttackFor:5;
    TerrainType allowTerrainIfAirborne:5;
    MoveEffectEnum setStateOnEffect;
    u16 redirectType:5;
    u16 ruinStat:3;
    u16 noDamageHits:2;
    u16 breakable:1;
    u16 unsuppressable:1;
    u16 persistent:1;
    u16 randomizerBanned:1;
    u16 unaware:1;
    u16 absorbUp2:1;
    u16 isSoundproof:1;
    u16 magicGuard:1;
    u16 noRecoil:1;
    u16 halfRecoil:1;
    u16 chloroplast:1;
    u16 skillLink:1;
    u16 resistsFortKnox:1;
    u16 fortKnox:1;
    u16 adaptability:1;
    u16 magicBounce:1;
    u16 levitate:1;
    u16 megaLauncherBoost:1;
    u16 unnerve:1;
    u16 negatesBurnAtkDrop:1;
    u16 negatesFrzSpatkDrop:1;
    u16 canInfatuateAny:1;
    u16 removesStatusOnImmunity:1;
    u16 tauntImmune:1;
    u16 shadowTag:1;
    u16 foesMinRoll:1;
    u16 powderImmune:1;
    u16 sandImmune:1;
    u16 hailImmune:1;
    u16 toxicTerrainImmune:1;
    u16 stealthRockImmune:1;
    u16 redCardEffect:1;
    u16 omniStab:1;
    Type addsType:5;
    u16 auraBreak:1;
    u16 suctionCups:1;
    u16 steadfast:1;
} Ability;

#ifdef __cplusplus
#undef AbilityApplyOnWithTarget
#undef Type
#endif

extern const Ability gAbilities[ABILITIES_COUNT];

int IsApplyOnFlagAppropriate(int contextBattler, int sourceBattler, AbilityApplyOn flag);
int IsTargettedApplyOnFlagAppropriate(int contextBattler, int sourceBattler, int attacker, int target, AbilityApplyOnWithTarget flag);
int DoesMoveMatchFlag(int battler, MoveEnum move, Type type, MoveFlag flag);

#ifdef __cplusplus
}
#endif

#endif
