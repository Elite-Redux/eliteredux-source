# Split Debug Into Dev and Public Modes

## Summary

- Keep the current debug system as the full-power developer mode.
- Add a second compile-time flag, `TX_DEBUG_SYSTEM_PUBLIC`, next to `TX_DEBUG_SYSTEM_ENABLE` in `include/debug.h`, with default `FALSE` so current dev behavior stays unchanged.
- When `TX_DEBUG_SYSTEM_PUBLIC == TRUE`, expose a limited player-safe debug menu.
- In public mode, disable debug entirely for `gameDifficulty >= DIFFICULTY_HELL` so Hell and any future harder challenge modes are automatically blocked.
- Create `docs/illegal-mons.md` as the source-of-truth document for the public blocklist, including both display names and in-code symbols.

## Implementation Changes

- Add a small central debug-policy layer in `src/debug.c` or a nearby helper so all entrypoints use the same checks:
- `debug compiled in` = `TX_DEBUG_SYSTEM_ENABLE`
- `public debug rules active` = `TX_DEBUG_SYSTEM_PUBLIC`
- `debug allowed for this save` = current difficulty is below `DIFFICULTY_HELL`
- Apply that policy to every debug entrypoint:
- overworld hotkey in `src/field_control_avatar.c`
- classic start-menu path in `src/start_menu.c`
- UI start-menu path in `src/ui_start_menu.c`
- Public debug menu contents:
- hide `Fly`, `Warp`, `Goto DebugMap`, and `CHEAT Start`
- hide `Toggle Collision`, `GodMode`, and `Autowin`
- keep only non-cheat utility/debug features that do not enable traversal abuse, forced wins, or illegal content access
- Public Pokemon giving:
- keep only the simple `species + level` give flow
- remove the complex custom-mon flow entirely from the public menu
- Public legality enforcement:
- add explicit public blocklists for species/forms and for items that unlock blocked forms
- enforce species blocking in the simple give flow before creation
- enforce item blocking in single-item give and bulk fill/give-all paths so public debug cannot backdoor blocked forms
- keep the runtime lists in C code; the markdown file is documentation and review support, not runtime config

## Blocklist Content

- Create `docs/illegal-mons.md` with flat entries in the format `Display Name | Code Symbol(s) | Notes`.
- Seed it with the current list, including these resolved mappings:
- `Articuno EX` -> `SPECIES_ARTICUNO_EX`, `SPECIES_ARTICUNO_EX_MEGA`
- `Moltres EX` -> `SPECIES_MOLTRES_EX`, `SPECIES_MOLTRES_EX_MEGA`
- `Zapdos EX` -> `SPECIES_ZAPDOS_EX`, `SPECIES_ZAPDOS_EX_MEGA`
- `Redux Weavile` -> `SPECIES_WEAVILE_REDUX`, `SPECIES_WEAVILE_REDUX_MEGA`
- `Weavilite R` -> `ITEM_WEAVILEITE_R`
- `Chien Paoite` -> `ITEM_CHIEN_PAOITE`
- `Bewear Angry` -> `SPECIES_BEWEAR_ANGRY`
- `Abomasite S` -> `ITEM_ABOMASITE_S`
- `Darkrai Nightmare` -> `SPECIES_DARKRAI_NIGHTMARE`
- `Espeon Primal` -> `SPECIES_ESPEON_GALAXY`, `ITEM_GALACTIC_ORB`
- `Galactic Orb` -> `ITEM_GALACTIC_ORB`
- `Lucky Orb` -> `ITEM_VICTINI_ORB`
- `Dream Orb` -> `ITEM_SNORLAX_ORB`
- `Spectrier Cloud` -> `SPECIES_SPECTRIER_CLOUD`
- `Mawile Redux B` -> `SPECIES_MAWILE_REDUX_B`, `SPECIES_MAWILE_REDUX_B_MEGA`
- `Mawilite R B` -> `ITEM_MAWILITE_R_B`
- `Solrock System` -> `SPECIES_SOLROCK_SYSTEM`
- `Ribombee R` -> `SPECIES_RIBOMBEE_REDUX`, `SPECIES_RIBOMBEE_REDUX_MEGA`
- `Ribombite R` -> `ITEM_RIBOMBITE_R`
- `Mimikyu Apex` -> `SPECIES_MIMIKYU_APEX`
- `Phantom Meteor` -> `ITEM_PHANTOM_METEOR`
- `Iron Heart` -> `SPECIES_LEDIAN_PARADOX`
- `Dracovishite` -> `ITEM_DRACOVISHITE`, `SPECIES_DRACOVISH_MEGA`
- `Flygon Redux B` -> `SPECIES_FLYGON_REDUX_B`, `SPECIES_FLYGON_REDUX_B_MEGA`
- `Flygonite R B` -> `ITEM_FLYGONITE_R_B`
- `Dragonite Delivery` -> `SPECIES_DRAGONITE_DELIVERY`
- `Wigglytuff Apex` -> `SPECIES_WIGGLYTUFF_APEX`
- `Ancient Apple` -> `ITEM_WIGGLITUFF_ORB`
- `Kartana Fallen` -> `SPECIES_KARTANA_FALLEN`
- `Calyrex Cloud Rider` -> `SPECIES_CALYREX_CLOUD_RIDER`
- Expand bundle labels instead of leaving them ambiguous:
- `Their Megas` becomes the three EX mega forms
- `Mono Champ Items + Forms` becomes the explicit Mono Champ reward species/items represented by the entries above

## Test Plan

- Build with `TX_DEBUG_SYSTEM_PUBLIC == FALSE`:
- debug behavior stays identical to the current repo
- overworld hotkey and existing dev cheats still work
- Build with `TX_DEBUG_SYSTEM_PUBLIC == TRUE` on Easy/Ace/Elite:
- debug opens normally
- removed public-forbidden actions are absent
- simple Pokemon give still works for allowed species
- complex custom-mon creation is absent
- blocked species/forms cannot be created
- blocked items cannot be granted through item give or bulk-fill paths
- Public build on Hell:
- all debug entrypoints are blocked
- no overworld hotkey, no debug start-menu action, no UI debug route
- Regression check:
- dev mode remains untouched when `TX_DEBUG_SYSTEM_PUBLIC == FALSE`

## Assumptions

- `TX_DEBUG_SYSTEM_PUBLIC` is the public/debug split flag name.
- Default value is `FALSE`.
- Future challenge modes means any future difficulty at or above Hell, so the gate should be `>= DIFFICULTY_HELL`.
- Public debug keeps only simple Pokemon giving and removes the complex custom-mon path entirely.
- `docs/illegal-mons.md` will be created during implementation; the runtime blocklist remains an explicit C table.
