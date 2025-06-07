# Trainer Tools

This directory contains Python scripts for managing trainers and their parties across different difficulty modes.

## Scripts

### 1. add_hell_parties.py
Adds Hell mode fields to all trainers in `src/data/trainers.h`. This script adds the necessary structure for Hell mode parties to each trainer definition.

**Usage:** `python3 add_hell_parties.py`

### 2. add_placeholder_parties.py
Creates placeholder Hell party arrays in `src/data/trainer_parties.h` by copying existing Insane party definitions. This provides a starting point for Hell mode teams.

**Usage:** `python3 add_placeholder_parties.py`

### 3. update_trainer_links.py
Links Hell party definitions in trainers.h to their corresponding arrays in trainer_parties.h. Updates NULL references to point to actual party arrays.

**Usage:** `python3 update_trainer_links.py`

### 4. cleanup_trainers.py
Removes unnecessary HellDouble fields from trainers that don't have double battle variants. Helps keep the trainer data clean and consistent.

**Usage:** `python3 cleanup_trainers.py`

## Workflow for Adding Hell Mode

1. Run `add_hell_parties.py` to add Hell fields to trainers
2. Run `add_placeholder_parties.py` to create Hell party arrays
3. Run `update_trainer_links.py` to link everything together
4. Run `cleanup_trainers.py` to remove unnecessary fields
5. Manually customize Hell parties as needed for increased difficulty

## Notes

- Hell mode parties start as copies of Insane/Elite parties
- These need to be manually edited to add illegal moves, abilities, and optimized strategies
- Always backup trainer files before running these scripts
- Run from the project root directory, not from this folder