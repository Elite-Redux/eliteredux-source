# Hell Mode Implementation Process

## Overview

This document describes the process for implementing Hell Mode, the 4th difficulty tier in Elite Redux.

## Implementation Steps

### 1. Add Hell Mode Fields to Trainer Structure

The trainer structure in `include/data.h` needs Hell mode fields:
```c
struct Trainer {
    // ... existing fields ...
    u8 partySizeHell;
    u8 partySizeHellDouble;
    union TrainerMonPtr partyHell;
    union TrainerMonPtr partyHellDouble;
};
```

### 2. Update All Trainer Definitions

Run `add_hell_parties.py` to add placeholder Hell fields to all trainers:
```bash
python3 add_hell_parties.py
```

This adds:
- `.partySizeHell = 0,`
- `.partyHell = {.ItemCustomMoves = NULL},`
- `.partySizeHellDouble = 0,` (if trainer has double battles)
- `.partyHellDouble = {.ItemCustomMoves = NULL},`

### 3. Create Hell Party Arrays

Run `add_placeholder_parties.py` to create Hell party arrays:
```bash
python3 add_placeholder_parties.py
```

This creates Hell party arrays by copying Insane parties:
- Finds all `sParty_*Insane` arrays
- Creates corresponding `sParty_*Hell` arrays immediately after

### 4. Link Hell Parties to Trainers

Run `update_trainer_links.py` to update trainer definitions:
```bash
python3 update_trainer_links.py
```

This updates placeholder NULL references to point to actual Hell arrays:
- Changes `partySizeHell = 0` to `ARRAY_COUNT(sParty_TrainerHell)`
- Changes `partyHell = NULL` to actual party reference

### 5. Clean Up Unnecessary Fields

Run `cleanup_trainers.py` to remove unneeded HellDouble fields:
```bash
python3 cleanup_trainers.py
```

This removes HellDouble fields from trainers that don't have double battles.

## Common Issues and Fixes

### Merge Conflicts

When merging branches with trainer changes:
1. Git conflict markers appear in trainers.h or trainer_parties.h
2. Manually resolve or use a script to remove conflict markers
3. Ensure all arrays are properly closed with `};`

### Missing Party Definitions

Error: `'sParty_TrainerHell' undeclared`

Fix: Create the missing party in trainer_parties.h or run a script to auto-generate

### Naming Mismatches

Error: `'sParty_WallaceHell2' undeclared`

Fix: Ensure consistent naming - should be `sParty_Wallace2Hell` (number before suffix)

### Party Type Mismatches

Ensure the party type matches the trainer's `partyFlags`:
- No flags → `TrainerMonNoItemDefaultMoves`
- `F_TRAINER_PARTY_CUSTOM_MOVESET` → `TrainerMonNoItemCustomMoves`
- `F_TRAINER_PARTY_HELD_ITEM` → `TrainerMonItemDefaultMoves`
- Both flags → `TrainerMonItemCustomMoves`

## Script Maintenance

### Essential Scripts to Keep

1. **add_hell_parties.py** - Adds Hell fields to trainers
2. **add_placeholder_parties.py** - Creates Hell party arrays
3. **update_trainer_links.py** - Links parties to trainers
4. **cleanup_trainers.py** - Removes unnecessary fields

### Temporary/Debug Scripts to Remove

- Scripts that check syntax errors
- One-off fix scripts for specific issues
- Scripts created during debugging

## Testing Hell Mode

1. Start new game and select Hell difficulty
2. Battle various trainers to ensure:
   - Hell parties are being used
   - No crashes or missing Pokemon
   - AI behaves as expected
3. Check save screen shows "Hell Mode"
4. Complete game to see Hell Mode in Hall of Fame

## Future Enhancements

### Customizing Hell Parties

Currently Hell parties are copies of Insane parties. To differentiate:

1. Give Pokemon illegal moves they can't normally learn
2. Use abilities outside their normal ability pool
3. Optimize EV spreads beyond normal limits
4. Use custom held items or Z-Crystals/Mega Stones
5. Increase team sizes where appropriate

### AI Improvements

Hell Mode should have the smartest AI:
```c
.aiFlags = AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | 
           AI_FLAG_CHECK_VIABILITY | AI_FLAG_SETUP_FIRST_TURN |
           AI_FLAG_SMART_SWITCHING | AI_FLAG_HP_AWARE |
           AI_FLAG_ACE_POKEMON | AI_FLAG_OMNISCIENT
```

### Difficulty Scaling

Consider adding:
- Level bonuses for Hell mode (+5 levels?)
- Perfect IVs/EVs for all Hell mode Pokemon
- Increased item usage by trainers
- More Full Restores and battle items