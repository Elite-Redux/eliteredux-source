#include "constants/abilities.h"
#include "global.h"

#ifndef GUARD_ABILITIES_H
#define GUARD_ABILITIES_H

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

typedef enum {
    APPLY_ON_SELF = 0,
    APPLY_ON_ALLY = 1,
    APPLY_ON_FOE = 2,
    APPLY_ON_ANY = 3,
} AbilityApplyOn;

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
    u16 breakable:1;
    u16 unsuppressable:1;
    u16 persistent:1;
    u16 randomizerBanned:1;
    u16 unaware:1;
    u16 absorbUp2:1;
    u16 redirectType:5;
    u16 isSoundproof:1;
    u16 noDamageHits:2;
    AbilityApplyOn onImmuneFor:2;
} Ability;

extern const Ability gAbilities[ABILITIES_COUNT];

int IsApplyOnFlagAppropriate(int applyTo, int from, AbilityApplyOn flag);

#endif
