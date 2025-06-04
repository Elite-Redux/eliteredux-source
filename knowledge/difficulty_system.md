# Difficulty System in Elite Redux

## Overview

Elite Redux has a 4-tier difficulty system:
- **Easy Mode** (0) - Standard gameplay
- **Ace Mode** (1) - Increased challenge
- **Elite Mode** (2) - Significantly harder
- **Hell Mode** (3) - Extreme difficulty with illegal movesets/abilities

## Key Files

### Constants
- `include/constants/global.h` - Defines DIFFICULTY_* constants

### UI Integration
- `src/ui_intro_options.c` - Difficulty selection in intro options
- `src/menu.c` - Save screen difficulty display
- `src/hall_of_fame.c` - Hall of Fame difficulty display
- `src/strings.c` - Difficulty mode text strings

### Trainer Data
- `src/data/trainers.h` - Trainer definitions with difficulty-specific parties
- `src/data/trainer_parties.h` - Pokemon team definitions for each difficulty
- `include/data.h` - Trainer struct definition with Hell mode fields

### Battle System
- `src/battle_main.c` - Party selection based on difficulty (CreateNPCTrainerParty function)

## Trainer Structure

Each trainer in `src/data/trainers.h` can have multiple party definitions:

```c
[TRAINER_NAME] = {
    // Regular parties
    .partySize = ARRAY_COUNT(sParty_TrainerName),
    .party = {.ItemCustomMoves = sParty_TrainerName},
    
    // Insane/Elite mode parties
    .partySizeInsane = ARRAY_COUNT(sParty_TrainerNameInsane),
    .partyInsane = {.ItemCustomMoves = sParty_TrainerNameInsane},
    
    // Hell mode parties
    .partySizeHell = ARRAY_COUNT(sParty_TrainerNameHell),
    .partyHell = {.ItemCustomMoves = sParty_TrainerNameHell},
    
    // Double battle variants (optional)
    .partySizeDouble = ...,
    .partyDouble = ...,
    .partySizeInsaneDouble = ...,
    .partyInsaneDouble = ...,
    .partySizeHellDouble = ...,
    .partyHellDouble = ...,
}
```

## Party Selection Logic

The game selects which party to use based on:
1. Current difficulty setting (`gSaveBlock2Ptr->difficultyMode`)
2. Battle type (single vs double)
3. Available party definitions

Priority order for party selection:
- Hell difficulty → Hell party (if exists) → Elite party → Regular party
- Elite difficulty → Elite/Insane party (if exists) → Regular party
- Other difficulties → Regular party

## Naming Conventions

### Party Arrays
- Regular: `sParty_TrainerName`
- Insane/Elite: `sParty_TrainerNameInsane`
- Hell: `sParty_TrainerNameHell`
- Double variants: Append `Double` (e.g., `sParty_TrainerNameHellDouble`)

### Important: Numbered Trainers
For trainers with numbers (e.g., Wallace2, Wallace3):
- Correct: `sParty_Wallace2Hell`
- Incorrect: `sParty_WallaceHell2`

The number comes BEFORE the difficulty suffix.

## Common Issues and Solutions

### Missing Party Definitions
If a trainer references a Hell party that doesn't exist, compilation will fail with "undeclared" errors.
Solution: Create the missing party in `trainer_parties.h`

### Naming Mismatches
Ensure the party name in `ARRAY_COUNT()` matches the name in `.ItemCustomMoves`

### Merge Conflicts
When merging branches, trainer files often have conflicts due to their size. Carefully resolve by:
1. Keeping both versions if they add different content
2. Ensuring no duplicate trainer IDs
3. Checking for proper array closure (closing braces and semicolons)

## AI Behavior

Hell Mode is intended to have:
- Smarter AI that makes better decisions
- Access to illegal movesets not normally learnable
- Abilities that Pokemon can't normally have
- Potentially higher stats or perfect IVs/EVs

The AI flags can be customized per trainer in the `.aiFlags` field.