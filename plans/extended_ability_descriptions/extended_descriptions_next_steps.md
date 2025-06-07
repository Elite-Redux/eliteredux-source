# Extended Ability Descriptions - Next Steps

## What We Know
1. **867 abilities** need extended descriptions
2. **UI is ready** (Riolu confirmed Jan 25, 2025)
3. **Codegen already works** - generates ability_text.hh
4. **No manual line breaks needed** - codegen handles text wrapping
5. **Style guide complete** - 91% active voice, consistent formatting
6. **Must add new field** - DO NOT modify existing description field

## Status Update (Jan 2025)

### ⚠️ CRITICAL DISCOVERY
- **UI code is in branch**: `SummaryScreenChanges` on GitHub
- **Never merged**: 6 months old, expect conflicts
- **Action required**: Commit current work FIRST, then merge

### Answered Questions
- **Field strategy**: ✅ Add new `extended_description` field (Option B)
- **Preserve existing system**: ✅ Never modify working description field
- **UI location**: ✅ Found in SummaryScreenChanges branch

### ✅ UI Constraints Discovered (via screenshots)
- **Lines available**: 10-15 lines (huge popup)
- **Characters per line**: ~32-35 characters  
- **Total capacity**: 350-525 characters
- **Text wrapping**: Automatic (no manual breaks needed)
- **Empty lines**: Can use for formatting/readability
- **Access method**: Press A twice from ability screen
- **Navigation**: Can scroll between abilities in extended view

### Critical Requirement
- **100% Accuracy**: Every extended description must be verified against code
  - Check abilities.cc for implementation
  - Verify percentages and calculations
  - Look up interactions in battle scripts
  - NO guessing or assumptions

## Ready to Go
- Test batch of 10 extended descriptions prepared
- Categorization complete (39 batches of ~50 abilities)
- Style guide ready
- Python scripts for analysis ready

## Recommended Approach (UPDATED)

### Phase 1: Version Control 🚨
1. **Commit current work** (35+ files)
   - Use `/project:git-sync` or manual git commands
   - Clear commit message describing all changes

### Phase 2: Branch Merge 🔀
1. **Fetch and merge** `SummaryScreenChanges` branch
2. **Resolve conflicts** (6 months of changes)
3. **Test UI** functionality after merge
4. **Document** any significant code changes found

### Phase 3: Integration & Writing ✍️
1. **Examine merged UI code** for integration points
2. **Add proto field** for extended_description
3. **Update codegen** to generate the field
4. **Test with 10 abilities** first

### Phase 4: Production
1. **Weather abilities** first (10 abilities)
2. **Verify each in code** - NO assumptions
3. **Use formatting** with empty lines
4. **Continue through ~87 batches**

## Time Estimate
- 10 abilities: 30 minutes
- 50 abilities: 2-3 hours per batch
- Total: ~80-100 hours for all 867 abilities
- Can be done over several weeks