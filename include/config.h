#ifndef GUARD_CONFIG_H
#define GUARD_CONFIG_H

// In the Generation 3 games, Asserts were used in various debug builds.
// Ruby/Sapphire and Emerald do not have these asserts while Fire Red
// still has them in the ROM. This is because the developers forgot
// to define NDEBUG before release, however this has been changed as
// Ruby's actual debug build does not use the AGBPrint features.
#define NDEBUG

// To enable print debugging, comment out "#define NDEBUG". This allows
// the various AGBPrint functions to be used. (See include/gba/isagbprint.h).
// Some emulators support a debug console window: uncomment NoCashGBAPrint()
// and NoCashGBAPrintf() in libisagbprn.c to use no$gba's own proprietary
// printing system. Use NoCashGBAPrint() and NoCashGBAPrintf() like you
// would normally use AGBPrint() and AGBPrintf().

#define ENGLISH

#ifdef ENGLISH
#define UNITS_IMPERIAL
#else
#define UNITS_METRIC
#endif

// Uncomment to fix some identified minor bugs
#define BUGFIX

// Various undefined behavior bugs may or may not prevent compilation with
// newer compilers. So always fix them when using a modern compiler.
#if MODERN || defined(BUGFIX)
#ifndef UBFIX
#define UBFIX
#endif
#endif

#define USE_GENERATED_SPECIES FALSE
#define USE_GENERATED_MOVES FALSE

#if USE_GENERATED_SPECIES
#define SPECIES_DEFINITIONS "generated/constants/species.h"
#else
#define SPECIES_DEFINITIONS "constants/species.h"
#endif

#if USE_GENERATED_MOVES
#define MOVE_BEHAVIOR_DEFINITIONS "generated/constants/battle_move_effects"
#define MOVE_EFFECT_DEFINITIONS "generated/constants/move_effects.h"
#else
#define MOVE_BEHAVIOR_DEFINITIONS "constants/battle_move_effects.h"
#define MOVE_EFFECT_DEFINITIONS "constants/move_effects.h"
#endif

#endif // GUARD_CONFIG_H
