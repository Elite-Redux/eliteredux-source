#ifndef GUARD_CONSTANTS_POKEMON_CONFIG_H
#define GUARD_CONSTANTS_POKEMON_CONFIG_H

#include "constants/expansion_branches.h"

#ifndef GEN_3
#define GEN_3 0
#define GEN_4 1
#define GEN_5 2
#define GEN_6 3
#define GEN_7 4
#define GEN_8 5
#endif

#define P_UPDATED_TYPES         GEN_8 // In Gen6+, several Pokémon were changed to be partially or fully Fairy type.
#define P_UPDATED_STATS         GEN_8 // Since Gen 6, Pokémon stats are updated with each passing generation.
#define P_UPDATED_ABILITIES     GEN_8 // Since Gen 6, certain Pokémon have their abilities changed. Requires BATTLE_ENGINE for Gen4+ abilities.
#define P_UPDATED_EGG_GROUPS    GEN_8 // Since Gen 8, certain Pokémon have gained new egg groups.
#define P_SHEDINJA_BALL         GEN_8 // Since Gen 4, Shedinja requires a Poké Ball for its evolution. In Gen 3, Shedinja inherits Nincada's Ball.

//Innate Disable Configuration
#define P_DISABLE_FIRST_DIFFICULTY DIFFICULTY_HELL //First difficulty that disables Innate until a certain level requirement is met

//Elite Mode - Unused in-game because Hell mode is the first mode with disabled innates
#define INNATE_1_LEVEL_ELITE 1
#define INNATE_2_LEVEL_ELITE 17
#define INNATE_3_LEVEL_ELITE 24
//Hell Mode
#define INNATE_1_LEVEL_HELL  10
#define INNATE_2_LEVEL_HELL  17 
#define INNATE_3_LEVEL_HELL  24

#define HELL_MODE_EXTRA_LEVELS      5                //Number of extra levels after getting the flag HELL_MODE_5_EXTRA_LEVELS_FLAG
#define HELL_MODE_EXTRA_LEVELS_FLAG FLAG_BADGE03_GET //Flag when enabled sets all the trainer Pokémon some levels above their normal levels (defined above)
#define HELL_MODE_0_IVS_FLAG        FLAG_BADGE05_GET //Flag when enabled sets all your Pokémon IVs to 0 (Only on Hard Mode)

#endif // GUARD_CONSTANTS_POKEMON_CONFIG_H
