# Start Writing Extended Descriptions NOW

## Why We Can Start Now

1. **UI is working** - Screenshots confirm it's implemented
2. **Space is generous** - 10-15 lines available
3. **Format is flexible** - Can use empty lines for readability
4. **Proto integration can wait** - We can write content first

## Writing Process

### Step 1: Verify Ability Code
```bash
# For each ability:
grep -n "ABILITY_NAME" src/abilities.cc
grep -n "ABILITY_NAME" src/battle_*.c
# Check implementation details
```

### Step 2: Write Description
- **Lines 1-2**: Core effect (what it does)
- **Empty line**: For readability
- **Lines 4-6**: Mechanics (percentages, conditions)
- **Empty line**: If needed
- **Lines 8-10**: Interactions/exceptions

### Step 3: Track in Spreadsheet
Create a simple tracking system:
- Ability ID
- Current description
- Extended description
- Verified? (Y/N)
- Character count

## First Production Batch (10 Abilities)

### Weather Abilities (High Priority)
1. ABILITY_DRIZZLE
2. ABILITY_DROUGHT
3. ABILITY_SAND_STREAM
4. ABILITY_SNOW_WARNING
5. ABILITY_PRIMORDIAL_SEA
6. ABILITY_DESOLATE_LAND
7. ABILITY_DELTA_STREAM
8. ABILITY_ELECTRIC_SURGE
9. ABILITY_GRASSY_SURGE
10. ABILITY_PSYCHIC_SURGE

These are high-impact competitive abilities that players need to understand fully.

## Workflow for Each Ability

1. **Look up current description**
   ```bash
   grep -A 3 "id: ABILITY_NAME" proto/AbilityList.textproto
   ```

2. **Find implementation**
   ```bash
   grep -n "case ABILITY_NAME" src/abilities.cc
   grep -n "ABILITY_NAME" src/battle_util.c
   ```

3. **Write extended description**
   - Verify mechanics in code
   - Include all effects
   - Add interactions
   - Use empty lines

4. **Save to batch file**
   ```
   ability {
     id: ABILITY_NAME
     name: "Name"
     description: "Current short description"
     extended_description: "New extended description with empty lines for readability"
   }
   ```

## Deliverable

A file ready to merge into AbilityList.textproto once the extended_description field is added.

## Time Estimate

- 10 abilities with full verification: 2-3 hours
- Can work on this immediately
- Proto integration can happen later