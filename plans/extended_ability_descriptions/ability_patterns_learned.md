# Ability Patterns Learned

This document tracks verified patterns to avoid re-researching similar abilities.

## Verified Patterns

### Weather Abilities
- **Duration**: "Lasts 8 turns" (verified in code)
- **Overrides**: New weather replaces old weather
- **Effects**: [To be documented as we verify]

### Stat Multipliers
- **"Doubles Attack"**: Multiplies final stat, not base stat
- **Stacking**: [To be verified]

### Type Immunities
- **Standard healing**: "Heals 25% of max HP" for absorb abilities
- **Immunity effects**: [To be documented]

### Status Immunities
- **Cure on gain**: If Pokemon has status when gaining immunity ability, status is cured
- **Blocks all sources**: Includes moves, abilities, and items

### Contact Effects
- **Standard chance**: "30% chance" for most contact abilities
- **Bidirectional**: "Also works on offense" means triggers when attacking too

### Damage Modifiers
- **Percentage format**: Always use "50%" not "1.5x" or "half"
- **Stacking**: Multiplicative unless specified

## Common Phrases (Verified)
- "on entry" - When Pokemon is sent out
- "on contact" - Physical moves that make contact
- "direct damage" - Excludes status, weather, hazards

## To Be Verified
- Exact interaction priorities
- Which abilities can be traced/copied
- How abilities interact with each other

---
*Update this document as patterns are verified in code*