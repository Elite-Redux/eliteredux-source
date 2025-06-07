# Extended Ability Descriptions Implementation Plan

## MASTER PLAN - Start Here

**For new sessions**: When continuing work on extended ability descriptions, start by reading this file and checking the project structure below.

## Overview
Implement extended ability descriptions for Elite Redux. The UI has been created by BelialClover (Riolu), but the system needs to be integrated with codegen and populated with actual descriptions.

## Project Structure
This task has multiple components organized as follows:

### Planning Documents (`/plans/extended_ability_descriptions/`)
- **MASTER_PLAN.md** (this file) - Start here, contains overview and links
- **extended_ability_descriptions_summary.md** - Quick facts and current status
- **extended_ability_descriptions_technical.md** - Technical implementation details
- **codegen_integration_steps.md** - Specific steps for codegen integration
- **extended_descriptions_next_steps.md** - Immediate action items

### Scripts and Data (`/scripts/ability_tools/`)
- **Analysis scripts** - Style analysis, categorization
- **test_batch_extended_descriptions.txt** - Test batch of 10 abilities
- **ability_style_guide.md** - Writing guidelines from analysis
- **batch_processing_plan.md** - 39 batches organized by category
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
- **No manual line breaks needed** - codegen handles automatic text wrapping
- Consider clone abilities handling

### 3. Output
- Generated file: `generated/data/abilities/ability_text.hh`
- Build with `make` or `make` in `tools/codegen/`

## Content Requirements

### Extended Description Guidelines
- **Length**: TBD (need to determine based on UI constraints)
- **Format**: Detailed explanation of ability mechanics
- **Content**: Should include:
  - Exact effects and percentages
  - Interaction with other abilities
  - Special conditions or exceptions
  - Battle scenarios where it's useful

### Batch Processing Strategy
1. **Revised batch size**: 10 abilities per batch (~87 batches total)
2. **Verification required**: Look up each ability in code
3. **Pattern documentation**: Track learned patterns to speed up similar abilities
4. **No guessing**: 100% accuracy required, verify all mechanics
5. **Workflow**: Use lookup_ability.py script for efficient searching

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

## Current Task Status (Jan 2025)

### Completed
- ✅ Style analysis complete (867 abilities analyzed)
- ✅ Categorization complete (revised to ~87 batches of 10)
- ✅ Test batches prepared (with and without line breaks)
- ✅ Lookup script created for verification workflow
- ✅ Pattern documentation system established

### ⚠️ CRITICAL DISCOVERY (Jan 2025)
- **UI code is in branch**: `SummaryScreenChanges` (GitHub)
- **Status**: Never merged into upcoming branch
- **Age**: 6 months old - expect merge conflicts
- **Action Required**: Commit current work, then merge branch

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

## Immediate Next Steps (UPDATED - Branch Merge Required First)

1. **URGENT - Commit Current Work** 🚨:
   - Commit and push 35+ modified files
   - Use clear commit message describing all changes
   - Ensure upcoming branch is up to date

2. **Merge SummaryScreenChanges Branch** 🔀:
   - Fetch and merge the 6-month-old UI branch
   - Resolve expected merge conflicts carefully
   - Test that UI still functions after merge
   - Document any significant changes found

3. **THEN Start Writing Abilities**:
   - Begin with weather abilities (high impact)
   - Verify each ability in code
   - Use empty lines for readability
   - Track progress in first_production_batch.txt

4. **Proto Integration** (after merge):
   - Examine merged UI code structure
   - Add `extended_description` field to proto
   - Update codegen to generate the field
   - Connect to Riolu's UI implementation

## Resources & References
- Discord discussion: Darky & Mawootad (Jan 2025)
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