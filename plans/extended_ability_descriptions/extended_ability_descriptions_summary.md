# Extended Ability Descriptions - Summary & Action Plan

## Quick Facts
- **Total abilities**: 867
- **Current desc average**: 46 characters (single line)
- **Extended desc target**: Multi-line detailed explanations
- **UI Status**: Already implemented by Riolu
- **Tech Status**: Needs codegen integration

## Key Findings from Analysis

### Writing Style (from 867 existing descriptions)
- **Voice**: 91% active voice (start with verbs like "Boosts", "Increases")
- **Numbers**: Use "50%" format, not "50 percent"
- **Common terms**: move, type, boost, damage, stat, turn
- **Structure**: Main effect first, then conditions
- **Line breaks**: Already use `\n` for multi-line descriptions

### Categories Breakdown
1. **Stat Modification** - 334 abilities (largest group)
2. **Type-Based** - 246 abilities  
3. **Damage Calculation** - 245 abilities
4. **Uncategorized** - 152 abilities (needs review)
5. **Status/Condition** - 106 abilities
6. **HP/Healing** - 102 abilities
7. **Weather/Terrain** - 102 abilities
8. Other smaller categories...

## Current Technical Status

### ✅ What's Already Working
- **Codegen system** generates `include/generated/data/abilities/ability_text.hh`
- **UI is ready and discovered**:
  - **10-15 lines available** (huge popup window)
  - **~32-35 chars per line**
  - **350-525 total characters possible**
  - Can use empty lines for readability
  - Access: Press A twice from ability screen
- **Data flow**: AbilityList.textproto → codegen → ability_text.hh

### ✅ Decisions Made
- **Field strategy**: Add new `extended_description` field (Option B)
- **Preserve existing descriptions** - never modify working system
- **Batch size**: 10 abilities per batch (not 50)

### ⚠️ CRITICAL UPDATE (Jan 2025)
- **UI is in separate branch**: `SummaryScreenChanges` (never merged)
- **Branch is 6 months old** - expect merge conflicts
- **Must commit current work first** (35+ files)
- **Then merge UI branch into upcoming**

## Immediate Action Items (REVISED - Branch Merge Required)

### 1. Commit Current Work (URGENT) 🚨
- [ ] Commit and push 35+ files to upcoming branch
- [ ] Use descriptive commit message for all changes

### 2. Merge UI Branch 🔀
- [ ] Merge `SummaryScreenChanges` branch into `upcoming`
- [ ] Resolve expected merge conflicts (6 months old)
- [ ] Test UI functionality after merge

### 3. THEN Start Writing ✍️
- [ ] Weather abilities first (10 abilities)
- [ ] Verify each in code - NO assumptions
- [ ] Use empty lines for structure
- [ ] Track in first_production_batch.txt

### 4. Proto Integration
- [ ] Examine merged UI code for integration points
- [ ] Add `extended_description` field
- [ ] Update codegen
- [ ] Connect to UI

## Next Steps
1. **Write first 10 weather abilities NOW**
2. Verify mechanics in abilities.cc
3. Track progress systematically
4. Proto integration when code available