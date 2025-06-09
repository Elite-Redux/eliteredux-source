# Extended Ability Descriptions Implementation Plan

## MASTER PLAN - Start Here

**For new sessions**: When continuing work on extended ability descriptions, start by reading this file.

## Quick Reference
- **Total abilities**: 867+ (growing daily)
- **UI constraints**: 11 usable lines, 30 chars/line, 280-300 chars total
- **Text wrapping**: Automatic by codegen (no manual line breaks)
- **Storage**: `knowledge/extended_ability_descriptions/`
- **Current status**: Ready to start writing content

## Overview
Implement extended ability descriptions for Elite Redux. The UI has been created by BelialClover (Riolu), but the system needs to be integrated with codegen and populated with actual descriptions.

## Project Structure

### Planning Documents (`/plans/extended_ability_descriptions/`)
- **MASTER_PLAN.md** (this file) - Complete project overview and guide
- **ui_analysis.md** - UI constraints and display specifications

### Storage Structure (`/knowledge/`)
- **abilities/** - Detailed mechanical documentation (Discord-friendly)
- **extended_ability_descriptions/** - Extended descriptions for in-game use
  - `extended_descriptions.txt` - Implementation-ready descriptions
  - `progress.md` - Tracking completion status
  - `writing_patterns.md` - Verified patterns for consistent writing

### Scripts and Data (`/scripts/ability_tools/`)
- Analysis scripts for style and categorization
- Test batches and writing guidelines
- Generated JSON files with categorized data

## Scope
- **Total abilities: 867** (confirmed by counting)
- Many abilities overlap in multiple categories
- Average current description: 46 characters
- Extended descriptions will be much longer (multi-line)

## Current Status
- ✅ UI implemented by Riolu (Jan 25, 2025)
- ✅ Codegen system already generates ability text
- ✅ Multi-line support exists (using \n)
- ✅ Writing style guide created
- ✅ 867 abilities categorized
- 🔍 Need to find UI implementation location
- ❌ Extended descriptions not written

## Technical Requirements

### 1. Codegen Integration (Priority: High)
According to Mawootad, three components are needed:
1. **Ability struct modification** - Possibly done
2. **Ability merge method in abilities.cc** - Possibly done  
3. **Proto definition** - Not done

### 2. Field Generation
- Add field generation to codegen (similar to normal ability descriptions)
- Determine pixel count and line limit for the UI
- **No manual line breaks needed** - codegen handles all text wrapping automatically
- Consider clone abilities handling

### 3. Output
- Generated file: `generated/data/abilities/ability_text.hh`
- Build with `make` or `make` in `tools/codegen/`

## Content Requirements

### Extended Description Guidelines
- **Length**: 350-525 characters (10-15 lines at ~32-35 chars/line)
- **Format**: Continuous text, no manual line breaks
- **Writing style** (from analysis of 867 abilities):
  - 91% active voice (start with verbs)
  - Use "50%" format, not "50 percent"
  - Common terms: move, type, boost, damage, stat, turn
  - Structure: Main effect first, then conditions
- **Content**: Should include:
  - Exact effects and percentages
  - Interaction with other abilities
  - Special conditions or exceptions
  - Comparisons to similar abilities (e.g., "unlike Parental Bond")

### Batch Processing Strategy
1. **Batch size**: 10 abilities per batch (~87 batches total)
2. **Verification required**: Use `/project:analyze-ability` for each ability
3. **Pattern documentation**: Update ability_patterns_learned.md as you go
4. **No guessing**: 100% accuracy required, verify all mechanics
5. **Priority order**:
   - Weather/terrain abilities (high competitive impact)
   - Elite Redux custom abilities (unique to ER)
   - Original Pokemon abilities (following patterns)

## Implementation Steps

### Phase 0: Style Analysis & Planning
- [ ] Extract all current ability descriptions from AbilityList.textproto
- [ ] Analyze writing style patterns:
  - [ ] Sentence structure
  - [ ] Technical terminology used
  - [ ] How percentages/numbers are presented
  - [ ] Passive vs active voice
  - [ ] Length patterns
- [ ] Create Elite Redux Ability Description Style Guide
- [ ] Categorize all abilities by type/effect for batch processing
- [ ] Prioritize abilities (custom ER abilities first? Most-used first?)

### Phase 1: Technical Setup
- [ ] Explore current codebase implementation
- [ ] Add proto definition for extended descriptions
- [ ] Implement/verify ability struct changes
- [ ] Implement/verify merge method in abilities.cc
- [ ] Add field generation to codegen
- [ ] Test codegen output

### Phase 2: UI Constraints
- [ ] Determine exact character/line limits
- [ ] Test text rendering in-game
- [ ] Document formatting requirements
- [ ] Create length validation tool

### Phase 3: Content Creation Strategy
- [ ] Develop templates for common ability patterns
- [ ] Create batch processing approach:
  1. **Custom ER Abilities** (500+)
     - [ ] Signature abilities
     - [ ] Weather/terrain abilities
     - [ ] Stat modification abilities
     - [ ] Type-changing abilities
     - [ ] Damage calculation abilities
     - [ ] Status/condition abilities
     - [ ] Item/berry interaction abilities
     - [ ] Switch/pivot abilities
     - [ ] Protection/immunity abilities
  2. **Original Pokemon Abilities** (200-300)
     - [ ] Gen 1-3 abilities
     - [ ] Gen 4-6 abilities
     - [ ] Gen 7-9 abilities
- [ ] Set up review process for accuracy
- [ ] Create automation scripts for similar abilities

### Phase 4: Implementation & Testing
- [ ] Implement descriptions in batches of ~50
- [ ] Test each batch in-game
- [ ] Check for overflow/display issues
- [ ] Verify clone abilities work correctly
- [ ] Community review for accuracy

### Phase 5: Maintenance
- [ ] Document process for adding new abilities
- [ ] Create validation scripts
- [ ] Set up quick reference for common patterns

## Current Task Status (June 2025)

### Completed
- ✅ Style analysis complete (867 abilities analyzed)
- ✅ Categorization complete (revised to ~87 batches of 10)
- ✅ Test batches prepared (with and without line breaks)
- ✅ Lookup script created for verification workflow
- ✅ Pattern documentation system established

### ✅ UI Update (June 2025)
- **UI implementation**: Riolu will code fresh implementation
- **Branch**: Will be implemented directly in upcoming branch
- **Status**: No old branch merge needed
- **Action**: Focus on content creation while UI is being coded

### ✅ Discovered via UI Screenshots
- Display shows **10-15 lines available** (huge popup window)
- ~32-35 chars per line
- Total capacity: **350-525 characters possible**
- Can use empty lines for readability
- Automatic text wrapping (no manual breaks)
- Access: Press A twice from ability screen

### Ready to Start NOW
- 🟢 **Start writing extended descriptions immediately!**
- 🟢 10-15 lines available with formatting options
- 🟢 Can use empty lines for better readability
- 🟢 Proto integration can happen in parallel

## Immediate Next Steps

1. **Start Writing Extended Descriptions** ✍️:
   - Begin with weather abilities (high impact)
   - Verify each ability in code
   - Write continuous text (no manual line breaks)
   - Track progress in extended_descriptions.txt

2. **Proto Integration** (Riolu will handle):
   - Add `extended_description` field to proto
   - Update codegen to generate the field
   - Connect to new UI implementation

3. **Content Creation Workflow**:
   - Use `/project:analyze-ability` for mechanics
   - Write 350-525 character descriptions
   - Store in `knowledge/extended_ability_descriptions/`
   - Update progress tracker

## Resources & References
- Discord discussion: Darky & Mawootad (June 2025)
- Related files:
  - `src/abilities.cc`
  - `proto/AbilityList.textproto`
  - `tools/codegen/` (Kotlin code)
  - UI implementation by Riolu
- Analysis tools:
  - `scripts/ability_tools/analyze_ability_style.py`
  - `scripts/ability_tools/categorize_abilities.py`

## Notes
- No GitHub issues/PRs used for ER
- Main contributors: Mawootad, Tuff, Riolu, James
- Consider automation for bulk description generation
- With 700-800 abilities, this is a multi-week project requiring systematic approach