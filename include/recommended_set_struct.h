#ifndef GUARD_RECOMMENDED_SET_STRUCT_H
#define GUARD_RECOMMENDED_SET_STRUCT_H

#include "global.h"

#define MAX_RECOMMENDED_LEARNSET_DESCRIPTION 20

enum
{
    LEARNSET_VARIATION_MOVE_1,
    LEARNSET_VARIATION_MOVE_2,
    LEARNSET_VARIATION_MOVE_3,
    LEARNSET_VARIATION_MOVE_4,
    LEARNSET_VARIATION_ABILITY,
    LEARNSET_VARIATION_ITEM,
};

struct Variation
{
    u16 type:3;
    u16 modification:13;
};

struct RecommendedLearnset
{
    const u8 description[MAX_RECOMMENDED_LEARNSET_DESCRIPTION];
    AbilityEnum ability;
    u16 item;
    u16 moves[MAX_MON_MOVES];
    struct Variation variations[3];
    u8 evs[NUM_STATS];
};

#endif