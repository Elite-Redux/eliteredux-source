#pragma once

#ifdef __cplusplus

extern "C" {
#endif

#include "behavior/constants.hh"
#include "global.h"
#include "constants/battle.h"
#include "generated/constants/abilities.h"

// typedef enum {
//     INFILTRATE_NONE = 0,
//     INFILTRATE_SCREENS = 1 << 0,
//     INFILTRATE_SUBSTITUTE = 1 << 1,
//     INFILTRATE_BREAK_SCREENS = 1 << 2,
// } InfiltrateType;

// typedef enum {
//     ABILITY_BS_PUSH_CURSOR_AND_CALLBACK,
//     ABILITY_BS_CALL,
//     ABILITY_BS_EXECUTE,
// } AbilityCallType;

// typedef enum {
//     MULTIHIT_SINGLE,
//     MULTIHIT_TWO_TO_FIVE,
//     MULTIHIT_FOUR_OR_FIVE,
//     MULTIHIT_TWO,
//     MULTIHIT_THREE,
//     MULTIHIT_FIVE,
//     MULTIHIT_TRIPLE_KICK,
//     MULTIHIT_TEN_CAN_MISS,
//     MULTIHIT_TEN,
//     MULTIHIT_BEAT_UP,
//     PARENTAL_BOND_START,
//     PARENTAL_BOND_HYPER_AGGRESSIVE = PARENTAL_BOND_START,
//     PARENTAL_BOND_PRIMAL_MAW,
//     PARENTAL_BOND_DUAL_WIELD,
//     PARENTAL_BOND_MINION_CONTROL,
//     PARENTAL_BOND_THREE_HEADED,
//     PARENTAL_BOND_ICE_COLD_HUNTER,
// } MultihitType;

// typedef enum NonStackingState {
//     NON_STACKING_NONE = 0,
//     NON_STACKING_RUIN_ATK = 1 << STAT_ATK,
//     NON_STACKING_RUIN_DEF = 1 << STAT_DEF,
//     NON_STACKING_RUIN_SPATK = 1 << STAT_SPATK,
//     NON_STACKING_RUIN_SPDEF = 1 << STAT_SPDEF,
// } NonStackingState;

// typedef enum {
//     ACCURACY_NO_RESULT = 0,
//     ACCURACY_MULTIPLICATIVE,
//     ACCURACY_HITS_IF_POSSIBLE,
//     ACCURACY_ALWAYS_MISSES,
//     ACCURACY_ALWAYS_HITS,
// } AccuracyPriority;

// #define NEVER_CRIT -2
// #define ALWAYS_CRIT 3

// typedef enum {
//     CHECK_NONE = 0,
//     CHECK_SLEEP = 1,
//     CHECK_POISON = 1 << 1,
//     CHECK_BURN = 1 << 2,
//     CHECK_PARALYSIS = 1 << 3,
//     CHECK_FROSTBITE = 1 << 4,
//     CHECK_BLEED = 1 << 5,
//     CHECK_STATUS1 = CHECK_SLEEP | CHECK_POISON | CHECK_BURN | CHECK_PARALYSIS | CHECK_FROSTBITE | CHECK_BLEED,
//     CHECK_CONFUSION = 1 << 6,
//     CHECK_INFATUATE = 1 << 7,
//     CHECK_RESTRICTING = 1 << 8,
//     CHECK_HEAL_BLOCK = 1 << 9,
//     CHECK_DRENCH = 1 << 10,
// } StatusCheckEnum;

// typedef enum MoveFlag {
//     MOVE_FLAG_PUNCH = 1,
//     MOVE_FLAG_KICK,
//     MOVE_FLAG_SOUND,
//     MOVE_FLAG_MEGA_LAUNCHER,
//     MOVE_FLAG_DANCE,
// } MoveFlag;

// typedef enum {
//     TERRAIN_NONE = 0,
//     TERRAIN_GRASSY = 1,
//     TERRAIN_ELECTRIC = 2,
//     TERRAIN_PSYCHIC = 3,
//     TERRAIN_MISTY = 4,
//     TERRAIN_TOXIC = 5,
// } TerrainType;

int IsApplyOnFlagAppropriate(int contextBattler, int sourceBattler, AbilityApplyOn flag);
int IsTargettedApplyOnFlagAppropriate(int contextBattler, int sourceBattler, int attacker, int target, AbilityApplyOnWithTarget flag);

#ifdef __cplusplus

enum class ApplyOn {
    SELF = 0,
    IGNORE_SELF = 1 << 2,
    ALLY = 1 << 0,
    ALLY_ONLY = ALLY | IGNORE_SELF,
    FOE_OR_SELF = 1 << 1,
    FOE = FOE_OR_SELF | IGNORE_SELF,
    ANY = ALLY | FOE_OR_SELF,
    OTHER = ANY | IGNORE_SELF,
};

enum class ApplyOnTarget {
    SELF = 0,
    IGNORE_SELF = 1 << 2,
    ALLY = 1 << 0,
    ALLY_ONLY = ALLY | IGNORE_SELF,
    FOE_OR_SELF = 1 << 1,
    FOE = FOE_OR_SELF | IGNORE_SELF,
    ANY = ALLY | FOE_OR_SELF,
    OTHER = ANY | IGNORE_SELF,
    ATTACKER_OR_TARGET = 1 << 3,
    TARGET = ATTACKER_OR_TARGET | IGNORE_SELF,
    ALLY_IS_TARGET = 1 << 4 | IGNORE_SELF,
    FOE_IS_TARGET = 1 << 5 | IGNORE_SELF,
};
}
#endif