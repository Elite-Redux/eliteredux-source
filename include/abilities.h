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
} Ability;

extern const Ability gAbilities[ABILITIES_COUNT];

#endif
