#include "global.h"
#include "constants/abilities.h"

#ifndef GUARD_ABILITIES_H
#define GUARD_ABILITIES_H

typedef int (* AbilityOnSwitchHandler)(int ability, int battler);
typedef int (* AbilityOnAbsorbHandler)(int battler, int move, int moveType, int *statId);

typedef struct Ability
{
    const u8* name;
    const u8* description;
    const AbilityOnSwitchHandler onSwitch;
    const AbilityOnAbsorbHandler onAbsorb;
    u16 breakable:1;
    u16 unsuppressable:1;
    u16 persistent:1;
    u16 randomizerBanned:1;
    u16 unaware:1;
    u16 absorbUp2:1;
    u16 redirectType:5;
} Ability;

extern const Ability gAbilities[ABILITIES_COUNT];

#endif
