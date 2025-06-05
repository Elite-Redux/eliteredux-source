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

# Build the ROM
# Replace X with number of CPU cores (e.g., -j24 for 24 cores)
# Note: Both 'make' and 'make modern' build with modern GCC - they are equivalent
make -jX
# OR
make modern -jX

# If you encounter compilation errors, clean first:
make clean
make -jX
# Note: 'make clean' may show errors about missing 'clean' targets in some tool subdirectories
# These errors are harmless - the important ROM files are cleaned successfully
# Alternative: Use 'make mostlyclean' to avoid the tool cleaning errors entirely

# Compare build against original ROM
make compare
```

### Building Tools
```bash
# Build all tools
make tools

# Clean and rebuild tools
make clean-tools  # May show 'No rule to make target' errors - these are harmless
make tools
```

## Working with Large Files

Many data files in this project are extremely large (e.g., `src/data/trainers.h` has 15,000+ lines). When working with these files:

1. **Use Python scripts to modify large files**:
   - Create a Python script to make the necessary changes
   - Run the script and verify the output
   - This approach is more reliable than direct editing
   - **IMPORTANT**: Put python scripts in the `scripts/` directory (not in `tools/`)
   - Example directories: `scripts/trainer_tools/`, `scripts/wiki_tools/`

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
4. Rebuild with `make -jX` or `make modern -jX` (both are equivalent)

### Common File Locations
- **Trainers**: `src/data/trainers.h`, `src/data/trainer_parties.h`
- **Pokemon**: Various files in `src/data/pokemon/`
- **Moves**: Battle move data files
- **Abilities**: `src/abilities.cc` and related headers
- **Items**: Item data headers
- **Strings**: `src/strings.c` and various other locations

### Script Organization
- **Python Scripts**: All Python scripts go in `scripts/` directory
- **Trainer Scripts**: `scripts/trainer_tools/`
- **Wiki Scripts**: `scripts/wiki_tools/`
- **Never put Python scripts in `tools/`** - that directory is only for C programs that need compilation

## Critical Rules

1. **NEVER autocommit or push to GitHub unless explicitly requested by the user**
2. **Always use `make clean` when encountering compilation errors** (ignore harmless 'No rule to make target' errors from tool subdirectories)
3. **Use Python scripts for editing large files (15k+ lines)**
4. **Test changes thoroughly before committing**
5. **DO NOT run `make` or `make modern` or other build commands** - the output is too large for the context window. Instead, inform the user when they need to build
6. **ALWAYS consider text length limits** - GBA UI elements have fixed sizes. Keep text concise to prevent overflow:
   - Start Menu descriptions: ~20 characters per line
   - Dialog boxes: Check line breaks and total length
   - Menu items: Keep names short and clear
   - Test in-game or count characters when editing UI text

## Important Notes

- Both `make -jX` and `make modern -jX` are equivalent and build with modern GCC (better warnings/errors)
- The `upcoming` branch is the active development branch
- Many files are auto-generated - be careful about which files to edit
- When in doubt about file size, check before attempting to read/edit directly
- The build system attempts to compile all directories under `tools/` - use `scripts/` for Python scripts instead

## Knowledge Base

Detailed documentation about specific systems can be found in the `knowledge/` directory:
- **difficulty_system.md** - How the 4-tier difficulty system works
- **adding_trainers.md** - Guide for adding new trainers and parties
- **hell_mode_implementation.md** - Process for implementing Hell Mode
- **wiki_system.md** - In-game Wiki system structure and content editing

## Memory: Ability and Data Management
- Abilities are now added in `abilities.cc` and `abilitylist.textproto`, not manually via multiple .c and .h files
- `abilities.cc` contains code definitions
- `abilitylist.textproto` currently holds names, descriptions, and ability effects
- `specieslist.textproto` contains Pokemon data
- `battle_util` is still used occasionally
- These files are crucial for project development

## Memory: Data Storage Strategy
- Single file with all data in one location
- Separate files only for sprite/palette files
- Textproto-based codegen and unified abilities file in `abilities.cc` is the main configuration method

## Memory: Learning and Knowledge Management
- When you learn new stuff, especially when you had to search a lot to understand how something works and is connected, don't hesitate to write into CLAUDE.md so next time it will be much faster > like training the AI so it gets better and better!

## In-Game Wiki System

The in-game wiki provides comprehensive help content accessible from the Start Menu or during battles (L button by default).

### Wiki Content Structure
- **Content Source**: `docs/er-wiki-google-docs.md` - Markdown file containing all wiki content
- **Parser Tool**: `scripts/wiki_tools/parse_wiki_markdown.py` - Converts markdown to protobuf format
- **Protobuf Data**: `proto/HelpArticles.textproto` - Generated wiki content in protobuf format
- **Generated Code**: `include/generated/data/text/help_articles.h` - C header with final wiki data

### Adding/Updating Wiki Content
1. Edit `docs/er-wiki-google-docs.md` following the existing format:
   - Categories: `## Category Name`
   - Entries: `### Entry Title`
   - Content lines: `* Content here` (9 lines per page max, 5 pages per entry max)
   - Blank lines: Just `*`

2. Run the parser:
   ```bash
   python3 scripts/wiki_tools/parse_wiki_markdown.py
   ```

3. Rebuild the codegen tools:
   ```bash
   make clean
   make tools/codegen
   ```

4. Build the ROM as usual

### Important Notes
- The GBA font system has limited character support - avoid special characters
- Keep text concise due to display constraints (46 characters per line approx)
- The parser automatically cleans problematic characters (accents, curly quotes, etc.)
- Wiki can be accessed from Start Menu > Wiki or during battles with L button