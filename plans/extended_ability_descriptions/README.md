# Extended Ability Descriptions Project

This folder contains all planning documents for the extended ability descriptions task.

## Quick Start for New Sessions

When told to "Keep working on extended ability descriptions", start here:

1. **Read MASTER_PLAN.md** - Contains overview and project structure
2. **Check current status** in extended_ability_descriptions_summary.md
3. **Review immediate tasks** in extended_descriptions_next_steps.md

## File Structure

### Core Documents
- **MASTER_PLAN.md** - Main planning document, project overview
- **extended_ability_descriptions_summary.md** - Quick facts and status
- **extended_descriptions_next_steps.md** - Immediate action items

### Technical Documents
- **extended_ability_descriptions_technical.md** - Implementation details
- **codegen_integration_steps.md** - Proto and codegen integration

### Related Directories
- `/scripts/ability_tools/` - Analysis scripts and test data
- `/proto/AbilityList.textproto` - Source ability data
- `/include/generated/data/abilities/` - Generated output

## Key Facts
- **867 total abilities** need extended descriptions
- **UI is ready** (confirmed by Riolu/BelialClover Jan 25, 2025)
- **No manual line breaks needed** - codegen handles text wrapping
- **39 batches** of ~50 abilities each planned

## Current Status
1. ✅ UI constraints discovered: **10-15 lines**, ~32-35 chars/line, **350-525 chars possible**
2. ✅ Can use empty lines for readability and structure
3. ✅ Field strategy confirmed: Add new `extended_description` field  
4. ⏳ Waiting for UI code location from Riolu (but can start writing!)

## UI Access (Discovered)
1. Go to Pokemon summary → Abilities screen
2. Press A → Header changes to "More Info (A)"
3. Press A again → Extended descriptions popup opens
4. Navigate between abilities with arrow keys

## Next Actions
1. **START WRITING NOW!** We have 10-15 lines available!
   - Begin with weather abilities (first_production_batch.txt)
   - Verify each ability's mechanics in code
   - Use empty lines to structure content
   
2. **In parallel**: Get UI code location from Riolu for proto integration

3. **Workflow**: Write → Verify → Track → Next batch