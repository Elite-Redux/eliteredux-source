# Extended Ability Descriptions Implementation Guide

This guide explains how the extended ability descriptions feature works in Elite Redux and how to implement it in a clean codebase.

## How Extended Ability Descriptions Work

The extended ability descriptions feature allows players to view detailed information about abilities on the Pokemon Summary Screen's ability page.

### User Flow:
1. Navigate to the Ability page in Pokemon Summary Screen
2. Press A button to enter ability selection mode (ModifyMode)
3. While in ModifyMode, press A button again to toggle expanded ability mode
4. In expanded mode:
   - The larger right pane (PSS_LABEL_PANE_RIGHT) displays extended ability text
   - Uses a different tilemap (gSummaryScreenPageAbilityExpandedTilemap)
   - Navigate between abilities with D-pad (Up/Down cycles through abilities)
   - Exit expanded mode by pressing A again or B to exit ModifyMode entirely

## Core Components Required

### 1. Data Structure Members

Add these to the `PokemonSummaryScreenData` struct in `pokemon_summary_screen.c`:

```c
struct PokemonSummaryScreenData
{
    // ... existing members ...
    
    bool8 expandedAbilityMode;    // Whether we're in expanded description mode
    u8 currentAbilityIndex;       // Which ability is selected (0 = changeable, 1-3 = innates)
    bool8 ModifyMode;             // Whether we're in ability/stat modification mode
    u8 sCurrentModifyIndex;       // Current stat being modified (used for EV modification)
};
```

### 2. Key Functions

#### ChangeIntoExpandedAbilityMode()
Toggles between normal and expanded ability description modes:

```c
static void ChangeIntoExpandedAbilityMode(void){
    PlaySE(SE_SELECT);

    if(sMonSummaryScreen->expandedAbilityMode){
        // Exit expanded mode - restore normal tilemap
        sMonSummaryScreen->expandedAbilityMode = FALSE;
        LZDecompressWram(sPageTilemaps[sMonSummaryScreen->currPageIndex], 
                         sMonSummaryScreen->bgTilemapBufferPage);
        ScheduleBgCopyTilemapToVram(2);
        BufferMonPokemonAbilityAndInnates();  // Redraw normal ability display
    }
    else{
        // Enter expanded mode
        sMonSummaryScreen->expandedAbilityMode = TRUE;
        LZDecompressWram(gSummaryScreenPageAbilityExpandedTilemap, 
                         sMonSummaryScreen->bgTilemapBufferPage);
        ScheduleBgCopyTilemapToVram(2);
        BufferMonPokemonExpandedAbilityAndInnates();  // Draw expanded descriptions
    }

    FillWindowPixelBuffer(PSS_LABEL_PANE_TITLE, PIXEL_FILL(0));
    ScheduleBgCopyTilemapToVram(0);
    PutWindowTilemap(PSS_LABEL_PANE_TITLE);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
}
```

#### BufferMonPokemonExpandedAbilityAndInnates()
Renders the extended ability description text:

```c
static void BufferMonPokemonExpandedAbilityAndInnates(void)
{
    struct PokeSummary *sum = &sMonSummaryScreen->summary;
    u8 y;
    u16 abilityToShow = ABILITY_NONE;
    u8 offset;
    u8 font = FONT_NORMAL;
    u16 species = sum->species;
    u8 abilityNum;

    // Get ability number for changeable ability
    if (!sMonSummaryScreen->isBoxMon)
        abilityNum = GetMonData(&gPlayerParty[sMonSummaryScreen->curMonIndex], MON_DATA_ABILITY_NUM);
    else {
        struct BoxPokemon *boxMon = sMonSummaryScreen->monList.boxMons;
        abilityNum = GetBoxMonData(&boxMon[sMonSummaryScreen->curMonIndex], MON_DATA_ABILITY_NUM);
    }

    // Select which ability to show based on currentAbilityIndex
    if(sMonSummaryScreen->currentAbilityIndex)
        abilityToShow = gBaseStats[species].innates[sMonSummaryScreen->currentAbilityIndex - 1];
    else
        abilityToShow = gBaseStats[species].abilities[abilityNum];

    DynamicPlaceholderTextUtil_Reset();
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(0, sMemoNatureTextColor);
    DynamicPlaceholderTextUtil_SetPlaceholderPtr(1, sMemoMiscTextColor);
    BufferCharacteristicString();
    
    y = 0;
    
    FillWindowPixelBuffer(PSS_LABEL_PANE_RIGHT, PIXEL_FILL(0));

    // Display ability name centered
    DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVar4, gAbilities[abilityToShow].name);
    offset = GetStringCenterAlignXOffset(font, gStringVar4, ABILITY_NAME_COLUMN_SIZE);
    PrintTextOnWindow(PSS_LABEL_PANE_RIGHT, gStringVar4, offset, y, 4, PSS_COLOR_WHITE_BLACK_SHADOW);

    // Main Ability Description
    // TODO: Integrate with codegen extended descriptions
    // Temporary placeholder text for extended descriptions
    static const u8 sExtendedDescPlaceholder[] = _("Extended ability descriptions\\ncoming soon!\\n\\nThis feature will show detailed\\ninformation about abilities.");
    DynamicPlaceholderTextUtil_ExpandPlaceholders(gStringVar4, sExtendedDescPlaceholder);
    PrintSmallTextOnWindow(PSS_LABEL_PANE_RIGHT, gStringVar4, 0, (y + 12), 0, PSS_COLOR_BLACK_GRAY_SHADOW);
    
    ScheduleBgCopyTilemapToVram(0);
    PutWindowTilemap(PSS_LABEL_PANE_RIGHT);
}
```

### 3. Graphics Resources

#### Tilemap Resource
Add to `src/graphics.c`:

```c
const u32 gSummaryScreenPageAbilityExpandedTilemap[] = INCBIN_U32("graphics/summary_screen/page_abilities_expanded.bin.lz");
```

Add to `include/graphics.h`:

```c
extern const u32 gSummaryScreenPageAbilityExpandedTilemap[];
```

## Implementation Steps for Clean Codebase

### Step 1: Add Structure Members
Add the four data members to the `PokemonSummaryScreenData` struct (expandedAbilityMode, currentAbilityIndex, ModifyMode, sCurrentModifyIndex).

### Step 2: Initialize New Members
In `CB2_InitSummaryScreen()`, initialize the new members:

```c
sMonSummaryScreen->ModifyMode = FALSE;
sMonSummaryScreen->sCurrentModifyIndex = 0;
sMonSummaryScreen->expandedAbilityMode = FALSE;
sMonSummaryScreen->currentAbilityIndex = 0;
```

### Step 3: Add the Core Functions
1. Add `ChangeIntoExpandedAbilityMode()` function
2. Add `BufferMonPokemonExpandedAbilityAndInnates()` function
3. Add function declarations at the top of the file

### Step 4: Modify LoadGraphics()
Update `LoadGraphics()` to use the correct tilemap based on mode:

```c
static void LoadGraphics(void)
{
    // ... existing code ...
    
    // In LoadCurrentPageTilemap section:
    if(sMonSummaryScreen->expandedAbilityMode && sMonSummaryScreen->currPageIndex == PSS_PAGE_ABILITY)
        LZDecompressWram(gSummaryScreenPageAbilityExpandedTilemap, sMonSummaryScreen->bgTilemapBufferPage);
    else
        LZDecompressWram(sPageTilemaps[sMonSummaryScreen->currPageIndex], sMonSummaryScreen->bgTilemapBufferPage);
    
    // ... rest of function ...
}
```

### Step 5: Update Input Handling
In `Task_HandleInput()`, add handling for A button on ability page:

```c
else if (JOY_NEW(A_BUTTON))
{
    switch (sMonSummaryScreen->currPageIndex){
        case PSS_PAGE_ABILITY:
            if(sMonSummaryScreen->ModifyMode){
                ChangeIntoExpandedAbilityMode();
            }
            else{
                sMonSummaryScreen->currentAbilityIndex = 0;
                sMonSummaryScreen->ModifyMode = TRUE;
                sMonSummaryScreen->expandedAbilityMode = FALSE;
                PrintAbilityAndInnates();
                PrintInfoBar(sMonSummaryScreen->currPageIndex, TRUE);
            }
            break;
        // ... other cases ...
    }
}
```

### Step 6: Add Navigation Logic
Update D-pad handling to cycle through abilities when in ModifyMode:

```c
if (JOY_NEW(DPAD_UP))
{
    if(sMonSummaryScreen->currPageIndex == PSS_PAGE_ABILITY){
        if(sMonSummaryScreen->ModifyMode){
            if(sMonSummaryScreen->currentAbilityIndex == 0)
                sMonSummaryScreen->currentAbilityIndex = NUM_MAX_ABILITIES_PER_MON - 1;
            else
                sMonSummaryScreen->currentAbilityIndex--;
            
            if(sMonSummaryScreen->expandedAbilityMode)
                BufferMonPokemonExpandedAbilityAndInnates();
            else
                PrintAbilityAndInnates();
        }
    }
    // ... handle other pages ...
}
```

### Step 7: Create Tilemap Binary
Create the expanded tilemap file at `graphics/summary_screen/page_abilities_expanded.bin.lz`. This should be a modified version of the regular ability page tilemap with a larger text area.

## Potential Issues When Implementing

### 1. Placeholder Text
Currently shows placeholder text: "Extended ability descriptions coming soon!" This needs to be integrated with the codegen system to pull actual extended descriptions from the ability data.

### 2. Codegen Integration
The extended descriptions need to be:
- Added to the ability data structure in the codegen system
- Generated into the appropriate header files
- Accessible through a function like `GetAbilityExtendedDescription(abilityId)`

### 3. Text Length Management
- Extended descriptions must fit within the PSS_LABEL_PANE_RIGHT window
- May need text wrapping or scrolling for very long descriptions
- Consider line count limits (currently about 5-6 lines max)

### 4. Memory Considerations
- Longer text strings will increase ROM size
- May need to optimize storage format for extended descriptions

### 5. Navigation Edge Cases
- Ensure proper bounds checking when cycling through abilities
- Handle Pokemon with fewer than 4 abilities properly
- Prevent navigation to empty ability slots

### 6. Visual Polish
- May need custom font size for better readability
- Consider adding visual indicators for which ability is selected
- Ensure smooth transitions between normal and expanded modes

## Testing Checklist

1. ✓ Can enter ModifyMode on ability page with A button
2. ✓ Can toggle expanded mode with A button while in ModifyMode
3. ✓ Can navigate between abilities with D-pad
4. ✓ Proper display of ability names in expanded mode
5. ✓ Can exit expanded mode with A or B button
6. ✓ Tilemap switches correctly between modes
7. ✓ No graphical glitches during transitions
8. ✓ Works correctly for both party and box Pokemon
9. ✓ Handles Pokemon with fewer than 4 abilities
10. ✓ Memory is properly managed (no leaks or corruption)

## Future Enhancements

1. **Actual Extended Descriptions**: Replace placeholder with real ability descriptions from codegen
2. **Scrolling Support**: For very long descriptions that don't fit on one screen
3. **Rich Text**: Support for colored text, icons, or formatting in descriptions
4. **Ability Stats**: Show numerical values, percentages, or other ability-specific data
5. **Cross-References**: Link to related abilities or moves
6. **Animation**: Smooth transitions when switching between abilities