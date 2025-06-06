# Codegen Integration Steps for Extended Descriptions

## Current State
The proto structure is simple:
```proto
message Ability {
  AbilityEnum id = 1;
  string name = 2;
  string description = 3;
}
```

## Required Changes

### 1. Proto Definition Update
Add to `proto/AbilityList.proto`:
```proto
message Ability {
  AbilityEnum id = 1;
  string name = 2;
  string description = 3;
  string extended_description = 4;  // NEW FIELD
}
```

### 2. Codegen Updates (in tools/codegen/)
Need to:
- Update the Kotlin code to generate extended_description field
- Handle line wrapping for multi-line descriptions
- Generate output in `generated/data/abilities/ability_text.hh`

### 3. Test with Sample Data
Add a few extended descriptions to test:
```
ability {
  id: ABILITY_DRIZZLE
  name: "Drizzle"
  description: "Summons rain on entry. Lasts 8 turns."
  extended_description: "When this Pokemon enters battle, it automatically summons rain weather that lasts for 8 turns. Rain boosts Water-type moves by 50% and reduces Fire-type moves by 50%. Also enables 100% accuracy for Thunder and Hurricane."
}
```

### 4. UI Integration Check
- Verify the UI can display multi-line descriptions
- Check character/line limits
- Test with various description lengths

## Commands to Run
1. Update proto file
2. `make clean` 
3. `make tools/codegen`
4. Check generated output
5. Test in-game