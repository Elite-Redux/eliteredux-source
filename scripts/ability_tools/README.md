# Ability Tools

This directory contains scripts and data for working with ability descriptions and codegen.

## Scripts

### Analysis Scripts (Run Once)
- `analyze_ability_style.py` - Analyzed writing style → `ability_style_guide.md`
- `categorize_abilities.py` - Categorized all 867 abilities → `batch_processing_plan.md`
- `find_extended_desc_code.py` - Searched for UI implementation (no results)

### Data Files
- `test_batch_extended_descriptions.txt` - Test batch of 10 extended descriptions
- `ability_style_guide.md` - Writing guidelines (91% active voice, etc.)
- `batch_processing_plan.md` - 39 batches organized by category
- `extended_descriptions_draft.txt` - Initial draft file

## Quick Start

1. **Analyze current style**:
   ```bash
   python3 analyze_ability_style.py
   ```

2. **Categorize abilities**:
   ```bash
   python3 categorize_abilities.py
   ```

3. **Find existing code**:
   ```bash
   python3 find_extended_desc_code.py
   ```

## Workflow

1. Run analysis scripts to understand current state
2. Review generated reports and style guide
3. Start technical implementation (codegen integration)
4. Draft extended descriptions in batches using categorization
5. Validate and test descriptions
6. Integrate with codegen system

## Project Scope

- **500+ custom Elite Redux abilities**
- **200-300 original Pokemon abilities (Gen 1-9)**
- **Total: 700-800+ abilities needing extended descriptions**

## Related Files

- `/plans/extended_ability_descriptions.md` - Overall project plan
- `/src/abilities.cc` - Ability implementations
- `/proto/AbilityList.textproto` - Ability data
- `/tools/codegen/` - Codegen system
- `generated/data/abilities/ability_text.hh` - Generated output (after codegen)