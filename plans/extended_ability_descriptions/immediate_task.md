# Immediate Task: Pattern Learning Batch

## UPDATE: UI Constraints Discovered!
- **5+ lines available**, ~32-35 chars per line
- **Target length**: 120-160 characters total
- **Maximum**: ~175 characters
- **No manual line breaks** - automatic wrapping

While waiting for Riolu's UI code location, we can start learning the ability system patterns.

## Task: First Learning Batch (5 Abilities)

### Selected Abilities (Different Pattern Types)
1. **ABILITY_DRIZZLE** - Weather summoning pattern
2. **ABILITY_INTIMIDATE** - Stat modification on entry pattern  
3. **ABILITY_WATER_ABSORB** - Type immunity/absorption pattern
4. **ABILITY_STATIC** - Contact effect pattern
5. **ABILITY_HUGE_POWER** - Stat multiplier pattern

### For Each Ability:
1. Use `python lookup_ability.py ABILITY_NAME`
2. Verify in abilities.cc:
   - Exact percentages
   - Trigger conditions
   - Special interactions
3. Write extended description
4. Document any patterns learned

### Deliverable:
- 5 verified extended descriptions
- Updated ability_patterns_learned.md
- Time estimate for future batches

### Why These 5?
- Each represents a common pattern type
- Mix of simple and complex
- Patterns apply to many other abilities
- Good test of the verification process

## Next Steps After This:
1. Review time taken and adjust batch size if needed
2. Create template descriptions for each pattern type
3. Begin systematic batch processing

This learning batch will establish our workflow and help us work more efficiently on the remaining 862 abilities.