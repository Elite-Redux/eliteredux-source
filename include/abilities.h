#include "global.h"
#include "constants/abilities.h"

#ifndef GUARD_ABILITIES_H
#define GUARD_ABILITIES_H

typedef int (* AbilityOnSwitchHandler)(int ability, int battler);

typedef struct Ability
{
    const u8* name;
    const u8* description;
    const AbilityOnSwitchHandler onSwitch;
    u16 breakable:1;
    u16 unsuppressable:1;
    u16 persistent:1;
} Ability;

extern const Ability gAbilities[ABILITIES_COUNT];

#endif
