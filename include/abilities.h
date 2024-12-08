#ifndef GUARD_ABILITIES_H
#define GUARD_ABILITIES_H

#include "constants/abilities.h"
#include "global.h"

typedef int (*AbilityOnSwitchHandler)(int ability, int battler);
typedef int (*AbilityOnAbsorbHandler)(int battler, int move, int moveType, int* statId);
typedef int (*AbilityOnImmuneHandler)(int battler, int attacker, int move, int moveType, const u8** immunityScript);
typedef enum {
    INFILTRATE_SCREENS = 1 << 0,
    INFILTRATE_SUBSTITUTE = 1 << 1,
    INFILTRATE_BREAK_SCREENS = 1 << 2,
} InfiltrateType;
typedef InfiltrateType (*AbilityOnInfiltrateHandler)(int battler, int move);
typedef int (*AbilityOnDisguiseHandler)(int battler, int checkOnly);
typedef int (*AbilityOnWeatherHandler)(int ability, int battler);
typedef int (*AbilityOnTerrainHandler)(int ability, int battler);
typedef int (*AbilityOnEndTurnHandler)(int ability, int battler);
typedef int (*AbilityOnAttackerHandler)(int ability, int battler, int target, int move, int moveType);
typedef int (*AbilityOnDefenderHandler)(int ability, int battler, int attacker, int move, int moveType);
typedef int (*AbilityOnRecoilHandler)(int damage, int battler, int moveType);
typedef int (*AbilityOnReactiveHandler)(int ability, int battler);
typedef int (*AbilityOnBattlerFaintsHandler)(int ability, int battler, int fainted, int move, int moveType);
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
typedef MultihitType (*AbilityOnParentalBondHandler)(int battler, int move, int moveType);
typedef void (*AbilityOnOffensiveMultiplierHandler)(
    int battler, int target, int move, int moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16* resistance, u16* modifier);
typedef void (*AbilityOnDefensiveMultiplierHandler)(
    int battler, int attacker, int move, int moveType, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier);

typedef enum {
    APPLY_ON_SELF = 0,
    APPLY_IGNORE_SELF = 1 << 2,
    APPLY_ON_ALLY = 1 << 0,
    APPLY_ON_ALLY_ONLY = APPLY_ON_ALLY | APPLY_IGNORE_SELF,
    APPLY_ON_FOE = 1 << 1 | APPLY_IGNORE_SELF,
    APPLY_ON_ANY = APPLY_ON_ALLY | APPLY_ON_FOE,
} AbilityApplyOn;

typedef enum {
    APPLY_ON_ATTACKER = 1 << 3,
    APPLY_ON_TARGET = 2 << 3,
    APPLY_ON_ATTACKER_OR_TARGET = APPLY_ON_ATTACKER | APPLY_ON_TARGET,
} AbilityApplyOnWithTarget;

typedef struct Ability {
    const u8* name;
    const u8* description;
    const AbilityOnSwitchHandler onSwitch;
    const AbilityOnAbsorbHandler onAbsorb;
    const AbilityOnImmuneHandler onImmune;
    const AbilityOnInfiltrateHandler onInfiltrate;
    const AbilityOnDisguiseHandler onDisguise;
    const AbilityOnWeatherHandler onWeather;
    const AbilityOnTerrainHandler onTerrain;
    const AbilityOnEndTurnHandler onEndTurn;
    const AbilityOnAttackerHandler onAttacker;
    const AbilityOnDefenderHandler onDefender;
    const AbilityOnRecoilHandler onRecoil;
    const AbilityOnReactiveHandler onReactive;
    const AbilityOnBattlerFaintsHandler onBattlerFaints;
    const AbilityOnParentalBondHandler onParentalBond;
    const AbilityOnOffensiveMultiplierHandler onOffensiveMultiplier;
    const AbilityOnDefensiveMultiplierHandler onDefensiveMultiplier;
    u16 redirectType:5;
    AbilityApplyOn onImmuneFor:3;
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
    AbilityApplyOnWithTarget onBattlerFaintsFor:5;
    AbilityApplyOn onOffensiveMultiplierFor:3;
    u16 skillLink:1;
    u16 resistsFortKnox:1;
    u16 fortKnox:1;
} Ability;

extern const Ability gAbilities[ABILITIES_COUNT];

int IsApplyOnFlagAppropriate(int contextBattler, int sourceBattler, AbilityApplyOn flag);
int IsTargettedApplyOnFlagAppropriate(int contextBattler, int sourceBattler, int attacker, int target, AbilityApplyOnWithTarget flag);

#endif
