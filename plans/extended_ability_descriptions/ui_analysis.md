# Extended Ability Description UI Analysis

Based on screenshots analyzed on [current date].

## UI Flow

1. **Main Ability Screen** - Shows Pokemon's 4 abilities with short descriptions
2. **Press A** - Header changes to "More Info (A)"  
3. **Press A Again** - Opens extended description popup

## Extended Description Display Constraints

### Text Area Dimensions
- **Lines available**: 10-15 lines (based on empty space in screenshots)
- **Characters per line**: ~32-35 characters
- **Total capacity**: 350-525 characters possible
- **Recommended usage**: 5-8 lines with empty lines for readability

### Observed Examples

#### Drought (Short Extended)
```
Moves inflict burn 5x as often.
```
- Only 1 line used
- Plenty of empty space below

#### Blaze (Multi-line Extended)
```
Boosts Fire-type moves by 1.2x or
1.5x when under 1/3 HP. 50% is an
ability activated chance. Fire
type starter Pokemon like
charmander.
```
- 5 lines used
- Shows automatic text wrapping
- ~32-35 characters per line

## Key Findings

1. **Automatic text wrapping** - No manual line breaks needed
2. **Generous space** - Can fit detailed explanations (5+ lines)
3. **Same width as main screen** - Consistent with normal ability descriptions
4. **Clean presentation** - White text on gray background
5. **Navigation** - Can scroll between abilities while in extended view

## Recommendations for Extended Descriptions

- **Target length**: 3-5 lines (100-175 characters)
- **Maximum tested**: 5 lines (up to ~175 characters)
- **Style**: Complete sentences, detailed mechanics
- **No manual formatting** - Let codegen handle wrapping

## Notes
- The UI is already fully implemented and working
- Pressing A twice from ability screen opens extended descriptions
- Can navigate between abilities while viewing extended descriptions
- Title bar shows current ability name