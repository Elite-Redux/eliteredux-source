# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Building the ROM
```bash
# First-time setup: Install agbcc compiler
git clone https://github.com/pret/agbcc
cd agbcc
./build.sh
./install.sh ../eliteredux-source
cd ..

# Build with modern GCC (recommended)
# Replace X with number of CPU cores (e.g., -j24 for 24 cores)
make modern -jX

# If you encounter compilation errors, clean first:
make clean
make modern -jX

# Compare build against original ROM
make compare
```

### Building Tools
```bash
# Build all tools
make tools

# Clean and rebuild tools
make clean-tools
make tools
```

## Working with Large Files

Many data files in this project are extremely large (e.g., `src/data/trainers.h` has 15,000+ lines). When working with these files:

1. **Use Python scripts to modify large files**:
   - Create a Python script to make the necessary changes
   - Run the script and verify the output
   - This approach is more reliable than direct editing

2. **Read files strategically**:
   - Use grep/search to find specific sections
   - Read only the relevant portions
   - Avoid loading entire large files when possible

Example workflow for editing trainers:
```python
# create edit_trainers.py
# make targeted changes
# run: python edit_trainers.py
# verify changes with diff or targeted reads
```

## Architecture Overview

### Multi-Ability System
The core feature of Elite Redux is the 4-ability system:
- **1 Changeable Ability**: Stored in the regular ability slot, can be changed among up to 3 choices
- **3 Fixed Innate Abilities**: Stored as separate abilities but always active
- Implementation in `src/abilities.cc` and battle engine files

### Data Organization
Game data is primarily edited directly in C header and source files:
- **Pokemon Data**: Base stats, evolutions, learnsets in various `.h` files
- **Trainer Data**: `src/data/trainers.h` and `src/data/trainer_parties.h`
- **Move Data**: Move effects, descriptions, and properties
- **Ability Data**: Ability effects and descriptions
- **Item Data**: Item effects, descriptions, and properties
- **Strings**: Various text strings throughout the codebase

Proto files are a compilation tool that helps generate some of this data, but developers primarily work with the C files directly.

### Map and Script System
- Maps use Pory scripting language (`.pory` files)
- Compiled to `.inc` files via poryscript tool
- Scripts handle trainer battles, events, and interactions

### Graphics Pipeline
- Sprites processed through `tools/gbagfx`
- Pokemon sprites support gender differences and forms
- UI elements for ability pop-ups and battle interface

## Development Workflow

### Making Game Data Changes
1. Edit the appropriate `.h` or `.c` file directly
2. For large files, use Python scripts to make changes
3. Run `make clean` if encountering errors
4. Rebuild with `make modern -jX`

### Common File Locations
- **Trainers**: `src/data/trainers.h`, `src/data/trainer_parties.h`
- **Pokemon**: Various files in `src/data/pokemon/`
- **Moves**: Battle move data files
- **Abilities**: `src/abilities.cc` and related headers
- **Items**: Item data headers
- **Strings**: `src/strings.c` and various other locations

## Critical Rules

1. **NEVER autocommit or push to GitHub without explicit approval**
2. **Always use `make clean` when encountering compilation errors**
3. **Use Python scripts for editing large files (15k+ lines)**
4. **Test changes thoroughly before committing**
5. **DO NOT run `make modern` or other build commands** - the output is too large for the context window. Instead, inform the user when they need to build

## Important Notes

- Always use `make modern -jX` for development (better warnings/errors)
- The `upcoming` branch is the active development branch
- Many files are auto-generated - be careful about which files to edit
- When in doubt about file size, check before attempting to read/edit directly

## Knowledge Base

Detailed documentation about specific systems can be found in the `knowledge/` directory:
- **difficulty_system.md** - How the 4-tier difficulty system works
- **adding_trainers.md** - Guide for adding new trainers and parties
- **hell_mode_implementation.md** - Process for implementing Hell Mode