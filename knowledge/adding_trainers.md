# Adding Trainers and Trainer Parties

## Overview

This guide explains how to add new trainers or modify existing trainers in Elite Redux.

## Step 1: Define Trainer ID

Add a new trainer ID in `include/constants/opponents.h`:

```c
#define TRAINER_NEW_TRAINER 1234  // Replace with next available ID
```

## Step 2: Create Trainer Parties

Add party definitions in `src/data/trainer_parties.h`:

### Basic Party (No held items, default moves)
```c
static const struct TrainerMonNoItemDefaultMoves sParty_NewTrainer[] = {
    {
    .lvl = 50,
    .species = SPECIES_PIKACHU,
    .iv = 31,
    .ability = 0,  // 0-2 for regular abilities, 3 for HA
    .nature = NATURE_JOLLY,
    .evs = {0, 252, 0, 0, 0, 252},  // HP, Atk, Def, SpA, SpD, Spe
    },
};
```

### Advanced Party (With held items and custom moves)
```c
static const struct TrainerMonItemCustomMoves sParty_NewTrainerInsane[] = {
    {
    .lvl = 0,  // 0 = scales with player's highest level
    .species = SPECIES_PIKACHU,
    .heldItem = ITEM_LIGHT_BALL,
    .ability = 2,
    .evs = {4, 252, 0, 0, 0, 252},
    .nature = NATURE_JOLLY,
    .moves = MOVE_FAKE_OUT, MOVE_VOLT_TACKLE, MOVE_EXTREME_SPEED, MOVE_SURF
    },
};
```

### Hell Mode Party (Can have illegal moves/abilities)
```c
static const struct TrainerMonItemCustomMoves sParty_NewTrainerHell[] = {
    {
    .lvl = 0,
    .species = SPECIES_PIKACHU,
    .heldItem = ITEM_PIKACHUNITE_Z,
    .ability = 0,
    .evs = {4, 252, 0, 0, 0, 252},
    .nature = NATURE_JOLLY,
    .moves = MOVE_THOUSAND_ARROWS, MOVE_BLUE_FLARE, MOVE_DRAGON_ASCENT, MOVE_JUDGMENT
    },
};
```

## Step 3: Define Trainer

Add trainer definition in `src/data/trainers.h`:

```c
[TRAINER_NEW_TRAINER] = 
{
    .partyFlags = F_TRAINER_PARTY_HELD_ITEM | F_TRAINER_PARTY_CUSTOM_MOVESET,
    .trainerClass = TRAINER_CLASS_YOUNGSTER,
    .encounterMusic_gender = TRAINER_ENCOUNTER_MUSIC_MALE,
    .trainerPic = TRAINER_PIC_YOUNGSTER,
    .trainerName = _("Johnny"),
    .items = {ITEM_FULL_RESTORE, ITEM_FULL_RESTORE, ITEM_NONE, ITEM_NONE},
    .doubleBattle = FALSE,
    .aiFlags = AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY,
    
    // Regular party
    .partySize = ARRAY_COUNT(sParty_NewTrainer),
    .party = {.ItemCustomMoves = sParty_NewTrainer},
    
    // Elite/Insane party
    .partySizeInsane = ARRAY_COUNT(sParty_NewTrainerInsane),
    .partyInsane = {.ItemCustomMoves = sParty_NewTrainerInsane},
    
    // Hell party
    .partySizeHell = ARRAY_COUNT(sParty_NewTrainerHell),
    .partyHell = {.ItemCustomMoves = sParty_NewTrainerHell},
},
```

## Party Flags

Choose the appropriate flag combination:
- `0` - No items, default moves
- `F_TRAINER_PARTY_CUSTOM_MOVESET` - Custom moves, no items
- `F_TRAINER_PARTY_HELD_ITEM` - Held items, default moves
- `F_TRAINER_PARTY_HELD_ITEM | F_TRAINER_PARTY_CUSTOM_MOVESET` - Both

## AI Flags

Common AI flag combinations:
- Basic: `AI_FLAG_CHECK_BAD_MOVE`
- Smart: `AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY`
- Advanced: Add `AI_FLAG_SETUP_FIRST_TURN | AI_FLAG_SMART_SWITCHING | AI_FLAG_HP_AWARE`
- Ace Trainer: Add `AI_FLAG_ACE_POKEMON` (saves best Pokemon for last)

## Special Fields

### Level Scaling
- Set `.lvl = 0` to scale with player's highest level Pokemon
- Positive values set fixed levels

### Double Battles
- Set `.doubleBattle = TRUE`
- Use separate double battle parties: `partyDouble`, `partyInsaneDouble`, `partyHellDouble`

### Mega Evolution
- Give appropriate Mega Stone as held item
- AI will automatically Mega Evolve when beneficial

### Special Attributes
- `.isAlpha = 1` - Makes Pokemon an Alpha (larger size)
- `.zeroSpeedIvs = 1` - Sets Speed IVs to 0 (for Trick Room teams)
- `.hpType = TYPE_FIRE` - Sets Hidden Power type

## Automation Scripts

### add_hell_parties.py
Adds Hell mode fields to all trainers in trainers.h

### add_placeholder_parties.py  
Creates placeholder Hell parties by copying Insane parties

### update_trainer_links.py
Updates Hell party references to point to actual party arrays

### cleanup_trainers.py
Removes unnecessary HellDouble fields from trainers without double battles

## Tips

1. Always check that party names match between trainers.h and trainer_parties.h
2. For numbered trainers (e.g., Roxanne2), use `sParty_Roxanne2Hell` not `sParty_RoxanneHell2`
3. Use `make clean && make modern -jX` if encountering compilation errors
4. Test battles in-game on different difficulties to ensure parties work correctly