# Extended Ability Descriptions - Technical Status

## Current Implementation Status

### What's Already Done
1. **Codegen System**: Already generates `include/generated/data/abilities/ability_text.hh`
2. **Multi-line Support**: Descriptions already use `\n` for line breaks
3. **Ability Structure**: Defined in `src/abilities.cc` as:
   ```cpp
   typedef struct AbilityKVPair {
       u16 key;
       Ability ability;
   } AbilityKVPair;
   ```
4. **UI Implementation**: Riolu confirmed on Jan 25, 2025 that UI is ready

### Current Data Flow
1. **Source**: `proto/AbilityList.textproto` contains ability data
2. **Generator**: `er.ability.AbilityTextGenerator` (in tools/codegen/)
3. **Output**: `include/generated/data/abilities/ability_text.hh`
4. **Usage**: Ability text array `sAbilityText[]` used in game

### What Needs to Be Done

#### Option A: Use Existing Description Field
The current descriptions are already multi-line capable. We could:
1. Simply expand the existing `description` field in AbilityList.textproto
2. Make descriptions longer and more detailed
3. No code changes needed, just content

**Example**:
```protobuf
ability {
  id: ABILITY_DRIZZLE
  name: "Drizzle"
  description: "Summons rain on entry. Lasts 8\nturns.\n\nRain increases Water-type move\npower by 50% and decreases Fire-\ntype move power by 50%. Thunder\nand Hurricane have 100% accuracy."
}
```

#### Option B: Add Extended Description Field
If we want separate short/long descriptions:
1. Add `extended_description` field to AbilityList.proto
2. Update codegen to handle new field
3. Update UI to show extended description when requested

### Immediate Action: Verify UI Location
Since Riolu said the UI is ready, we need to:
1. Find where the extended description UI is implemented
2. Understand how it's triggered (button press? menu option?)
3. Check character/line limits for display

### Character Limits - CONFIRMED via UI Screenshots
- **Current descriptions**: ~46 characters (2 lines)
- **Extended descriptions**: 
  - 5+ lines available
  - ~32-35 characters per line
  - ~160-175 total characters comfortably
- **Text wrapping**: Automatic by codegen
- **No manual line breaks needed** - write as continuous text