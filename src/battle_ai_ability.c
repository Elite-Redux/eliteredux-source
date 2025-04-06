#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_scripts.h"
#include "battle_main.h"
#include "mgba_printf/mgba.h"
#include "generated/constants/abilities.h"
#include "generated/constants/species.h"
#include "generated/constants/battle_move_effects.h"
#include "constants/items.h"
#include "item.h"
#include "generated/constants/moves.h"
#include "battle_anim.h"
#include "constants/hold_effects.h"
#include "battle_ai_new.h"
#include "battle_ai_new_util.h"
#include "battle_ai_scoring.h"

// TODO: Gym Skill
// TODO: Commander

int ScoreIntimidate(int battlerDef, int stat, int change, AbilityEnum ability, int both) {
    int score = 0;
    int statActual = stat;

    if (statActual == STAT_HIGHEST_ATTACKING) stat = GetHighestAttackingStatId(battlerDef, TRUE);
    if (statActual == STAT_HIGHEST_DEFENDING) stat = GetHighestDefendingStatId(battlerDef, TRUE);
    if (IsBattlerImmuneToLowerStatsFromIntimidateClone(battlerDef)) score += AI_SCORE_STAT(battlerDef, stat, change);

    if (both) {
        if (statActual == STAT_HIGHEST_ATTACKING) stat = GetHighestAttackingStatId(BATTLE_PARTNER(battlerDef), TRUE);
        if (statActual == STAT_HIGHEST_DEFENDING) stat = GetHighestDefendingStatId(BATTLE_PARTNER(battlerDef), TRUE);
        if (IsBattlerImmuneToLowerStatsFromIntimidateClone(BATTLE_PARTNER(battlerDef))) score += AI_SCORE_STAT(BATTLE_PARTNER(battlerDef), stat, change);
    }

    return score;
}

SpeciesEnum GetHpFormChangeSpecies(int battler, struct AiData* aiData) { return SPECIES_NONE; }

int ScoreAttackAbility(AbilityEnum ability, int battlerAtk, int battlerDef, int move, int moveType, struct MoveState* moveState, struct AiData* aiData) {
    int score = 0;

    switch (ability) {
        case ABILITY_VITAL_SPIRIT:
        case ABILITY_GULP_MISSILE:
        case ABILITY_SHIELDS_DOWN:
        case ABILITY_ANGELS_WRATH:
            break;

        default:
            if (!moveState->damage) return 0;
            break;
    }

    switch (ability) {
        case ABILITY_SHIELDS_DOWN:
            REQUIRE(gBattleMoves[move].effect == EFFECT_SHELL_SMASH)
            REQUIRE_NOT(gBattleMons[battlerAtk].status2 && STATUS2_TRANSFORMED)
            switch (gBattleMons[battlerAtk].species) {
                case SPECIES_MINIOR_CORE_BLUE:
                case SPECIES_MINIOR_CORE_GREEN:
                case SPECIES_MINIOR_CORE_INDIGO:
                case SPECIES_MINIOR_CORE_ORANGE:
                case SPECIES_MINIOR_CORE_RED:
                case SPECIES_MINIOR_CORE_VIOLET:
                case SPECIES_MINIOR_CORE_YELLOW:
                    return AI_SCORE_FORM_CHANGE(battlerAtk, SPECIES_MINIOR);
            }
            break;

        case ABILITY_GULP_MISSILE:
            REQUIRE_NOT(gBattleMons[battlerAtk].status2 && STATUS2_TRANSFORMED)
            REQUIRE(gBattleMons[battlerAtk].species == SPECIES_CRAMORANT)
            REQUIRE(move == MOVE_SURF || move == MOVE_TRIPLE_DIVE || move == MOVE_DIVE)
            // TODO: Half HP calc
            return AI_SCORE_FORM_CHANGE(battlerAtk, SPECIES_CRAMORANT_GORGING);

        case ABILITY_HYDRO_CIRCUIT:
            REQUIRE(moveType == TYPE_WATER)
            return AI_SCORE_ABSORB_MOVE(50);

        case ABILITY_VITALITY_STRIKE:
            REQUIRE(IsIronFistBoosted(battlerAtk, move))
            return AI_SCORE_ABSORB_MOVE(50);

        case ABILITY_PURE_LOVE:
            REQUIRE(gBattleMons[battlerDef].status2 & STATUS2_INFATUATION)
            return AI_SCORE_ABSORB_MOVE(50);

        case ABILITY_GROWING_TOOTH:
            REQUIRE(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return AI_SCORE_STAT(battlerAtk, STAT_ATK, 1);

        case ABILITY_SPINNING_TOP:
            REQUIRE(moveType == TYPE_FIGHTING)
            // TODO: Once per turn
            return AI_SCORE_CLEAR_HAZARDS(battlerAtk) + AI_SCORE_STAT(battlerAtk, STAT_SPEED, 1);

        case ABILITY_VITAL_SPIRIT:
            REQUIRE(moveType == TYPE_FIGHTING)
            return AI_SCORE_CURE_STATUS(battlerAtk);

        case ABILITY_HARDENED_SHEATH:
            REQUIRE(gBattleMoves[move].hornBased)
            return AI_SCORE_ATTACK_UP(battlerAtk, 1);

        case ABILITY_LOUD_BANG:
            REQUIRE(gBattleMoves[move].flags & FLAG_SOUND)
            return AI_SCORE_ADJUST(50, AI_SCORE_CONFUSION(battlerDef));

        case ABILITY_PIERCING_SOLO:
            REQUIRE(gBattleMoves[move].flags & FLAG_SOUND)
            return AI_SCORE_ADJUST(30, AI_SCORE_BLEED(battlerDef));

        case ABILITY_TO_THE_BONE:
        case ABILITY_RAZOR_SHARP:
            return AI_SCORE_ADJUST(moveState->critChance, AI_SCORE_BLEED(battlerDef));

        case ABILITY_KNOW_YOUR_PLACE:
            REQUIRE(moveState->contact)
            return AI_SCORE_DAZE(battlerDef);

        case ABILITY_DENTING_BLOWS:
            REQUIRE(gBattleMoves[move].hammerBased)
            // TODO: Once per turn
            return AI_SCORE_STAT(battlerDef, STAT_DEF, -1);

        case ABILITY_WHIPLASH:
            REQUIRE(IS_MOVE_PHYSICAL(move))
            // TODO: Once per turn
            return AI_SCORE_ADJUST(50, AI_SCORE_STAT(battlerDef, STAT_DEF, -1));

        case ABILITY_BEAUTIFUL_MUSIC:
            REQUIRE(gBattleMoves[move].flags & FLAG_SOUND)
            return AI_SCORE_ADJUST(50, AI_SCORE_ATTRACT(battlerAtk, battlerDef));

        case ABILITY_RESONANCE:
            REQUIRE((gBattleMoves[move].flags & FLAG_SOUND))
            return AI_SCORE_BLEED(battlerDef);

        case ABILITY_TOXIC_CHAIN:
            return AI_SCORE_ADJUST(30, AI_SCORE_TOXIC(battlerDef));

        case ABILITY_ELECTRIC_BURST:
            REQUIRE(moveType == TYPE_ELECTRIC)
            // TODO: Can't faint
            return AI_SCORE_RECOIL(battlerAtk, 10, TRUE);

        case ABILITY_INFERNAL_RAGE:
            REQUIRE(moveType == TYPE_FIRE)
            return AI_SCORE_RECOIL(battlerAtk, 10, TRUE);

        case ABILITY_ARCHMAGE:
            REQUIRE_NOT(IS_MOVE_STATUS(move))
            switch (moveType) {
                case TYPE_POISON:
                    score = AI_SCORE_TOXIC(battlerDef);
                    break;

                case TYPE_ICE:
                    score = AI_SCORE_FROSTBITE_MOVE(battlerDef);
                    break;

                case TYPE_WATER:
                    score = AI_SCORE_CONFUSION(battlerDef);
                    break;

                case TYPE_FIRE:
                    score = AI_SCORE_BURN_MOVE(battlerDef);
                    break;

                case TYPE_ELECTRIC:
                    score = AI_SCORE_ELECTRIC_TERRAIN;
                    break;

                case TYPE_PSYCHIC:
                    score = AI_SCORE_PSYCHIC_TERRAIN;
                    break;

                case TYPE_FAIRY:
                    score = AI_SCORE_MISTY_TERRAIN;
                    break;

                case TYPE_GRASS:
                    score = AI_SCORE_GRASSY_TERRAIN;
                    break;

                case TYPE_NORMAL:
                    score = AI_SCORE_ENCORE;
                    break;

                case TYPE_ROCK:
                    score = AI_SCORE_STEALTH_ROCK(battlerDef, TYPE_ROCK);
                    break;

                case TYPE_GHOST:
                    score = AI_SCORE_DISABLE(battlerDef);
                    break;

                case TYPE_DARK:
                    score = AI_SCORE_BLEED(battlerDef);
                    break;

                case TYPE_FIGHTING:
                    score = AI_SCORE_STAT(battlerAtk, STAT_SPATK, 1);
                    break;

                case TYPE_FLYING:
                    score = AI_SCORE_STAT(battlerAtk, STAT_SPEED, 1);
                    break;

                case TYPE_BUG:
                    // TODO: Set sticky web
                    break;

                case TYPE_DRAGON:
                    score = AI_SCORE_STAT(battlerDef, STAT_ATK, -1);
                    break;

                case TYPE_GROUND:
                    score = AI_SCORE_TRAP(battlerDef);
                    break;

                case TYPE_STEEL:
                    score = AI_SCORE_STAT(battlerAtk, STAT_DEF, 1);
                    break;
            }
            return AI_SCORE_ADJUST(30, score);

        case ABILITY_SOLENOGLYPHS:
            REQUIRE(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return AI_SCORE_ADJUST(50, AI_SCORE_TOXIC(battlerDef));

        case ABILITY_FROSTMAW:
            REQUIRE(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return AI_SCORE_ADJUST(50, AI_SCORE_FROSTBITE_MOVE(battlerDef));

        case ABILITY_ASSASSINS_TOOLS:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(10, AI_SCORE_POISON_MOVE(battlerDef) + AI_SCORE_PARALYSIS(battlerDef) + AI_SCORE_BLEED(battlerDef));

        case ABILITY_DEEP_CUTS:
            REQUIRE(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            return AI_SCORE_ADJUST(50, AI_SCORE_BLEED(battlerDef));

        case ABILITY_FLAMING_JAWS:
        case ABILITY_FLAMING_MAW:
            REQUIRE(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return AI_SCORE_ADJUST(50, AI_SCORE_BURN_MOVE(battlerDef));

        case ABILITY_ARC_FLASH:
            return AI_SCORE_ADJUST(50, AI_SCORE_PARALYSIS(battlerDef));

        case ABILITY_RADIO_JAM:
            REQUIRE(gBattleMoves[move].flags & FLAG_SOUND)
            return AI_SCORE_ADJUST(20, AI_SCORE_DISABLE(battlerDef));

        case ABILITY_DEMOLITIONIST:
            REQUIRE(gVolatileStructs[battlerAtk].readiedAction)
            return AI_SCORE_BREAK_SCREENS(battlerDef);

        case ABILITY_PINNACLE_BLADE:
            REQUIRE(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            return AI_SCORE_BREAK_PROTECT + AI_SCORE_BREAK_SCREENS(battlerDef) + AI_SCORE_BREAK_SUBSTITUTE;

        case ABILITY_FEARMONGER:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(10, AI_SCORE_PARALYSIS(battlerDef));

        case ABILITY_YUKI_ONNA:
            return AI_SCORE_ADJUST(10, AI_SCORE_ATTRACT(battlerAtk, battlerDef));

        case ABILITY_STUN_SHOCK:
            return AI_SCORE_ADJUST(30, AI_SCORE_POISON_MOVE(battlerDef) + AI_SCORE_PARALYSIS(battlerDef));

        case ABILITY_SHOCKING_JAWS:
        case ABILITY_SHOCKING_MAW:
            REQUIRE(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return AI_SCORE_ADJUST(50, AI_SCORE_PARALYSIS(battlerDef));

        case ABILITY_VENOBLAZE_PINCERS:
            REQUIRE(IS_MOVE_PHYSICAL(move))
            return AI_SCORE_ADJUST(10, AI_SCORE_TOXIC(battlerDef) + AI_SCORE_BURN_MOVE(battlerDef));

        case ABILITY_MOLTEN_BLADES:
            REQUIRE(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            return AI_SCORE_ADJUST(20, AI_SCORE_BURN_MOVE(battlerDef));

        case ABILITY_DEAD_POWER:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(20, AI_SCORE_CURSE(battlerDef));

        case ABILITY_SPECTRAL_SHROUD:
            REQUIRE(gBattleStruct->ateBoost[battlerAtk])
            REQUIRE(moveType == TYPE_GHOST)
            return AI_SCORE_ADJUST(30, AI_SCORE_TOXIC(battlerDef));

        case ABILITY_ANGELS_WRATH:
            switch (move) {
                case MOVE_TACKLE:
                    REQUIRE(moveState->damage)
                    return AI_SCORE_DISABLE(battlerDef) + AI_SCORE_ENCORE;

                case MOVE_STRING_SHOT:
                    REQUIRE_NOT(gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_STEALTH_ROCK ||
                                gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_TOXIC_SPIKES ||
                                gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_SPIKES ||
                                gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_STICKY_WEB)
                    return AI_SCORE_STEALTH_ROCK(battlerDef, TYPE_ROCK) + AI_SCORE_TOXIC_SPIKES(battlerDef) + AI_SCORE_SPIKES(battlerDef) + AI_SCORE_STICKY_WEB;

                case MOVE_HARDEN:
                    return AI_SCORE_STAT(battlerAtk, STAT_ATK, 1) + AI_SCORE_STAT(battlerAtk, STAT_SPEED, 1) + AI_SCORE_STAT(battlerAtk, STAT_SPATK, 1) +
                           AI_SCORE_STAT(battlerAtk, STAT_SPDEF, 1);

                case MOVE_IRON_DEFENSE:
                    return AI_SCORE_PROTECT;

                case MOVE_ELECTROWEB:
                    REQUIRE(moveState->damage);
                    // TODO: Raw stat change
                    return AI_SCORE_TRAP(battlerDef) + AI_SCORE_STAT(battlerDef, STAT_SPEED, -12);

                case MOVE_BUG_BITE:
                    REQUIRE(moveState->damage);
                    return AI_SCORE_ABSORB_MOVE(100);
            }
            return 0;

        case ABILITY_ELEMENTAL_CHARGE:
            switch (moveType) {
                case TYPE_ELECTRIC:
                    score = AI_SCORE_PARALYSIS(battlerDef);
                    break;

                case TYPE_FIRE:
                    score = AI_SCORE_BURN_MOVE(battlerDef);
                    break;

                case TYPE_ICE:
                    score = AI_SCORE_FROSTBITE_MOVE(battlerDef);
                    break;
            }
            return AI_SCORE_ADJUST(20, score);

        case ABILITY_STENCH:
            REQUIRE(CanMoveHaveExtraFlinchChance(move))
            return AI_SCORE_ADJUST(10, AI_SCORE_FLINCH(battlerDef));

        case ABILITY_HAUNTING_FRENZY:
            REQUIRE(CanMoveHaveExtraFlinchChance(move))
            return AI_SCORE_ADJUST(20, AI_SCORE_FLINCH(battlerDef));

        case ABILITY_FROM_THE_SHADOWS:
            // TODO: From the Shadows
            return 0;

        case ABILITY_ABSORBANT:
            REQUIRE(gBattleMoves[move].effect == EFFECT_ABSORB || gBattleMoves[move].effect == EFFECT_DREAM_EATER)
            return AI_SCORE_LEECH_SEED;

        case ABILITY_FUNGAL_INFECTION:
            REQUIRE(moveState->contact)
            return AI_SCORE_LEECH_SEED;

        case ABILITY_GRIP_PINCER:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(50, AI_SCORE_WRAP(battlerAtk, battlerDef));
    }

    return 0;
}

#define REQUIRE_HALF_HP

int ScoreDefenseAbility(AbilityEnum ability, int battlerAtk, int battlerDef, int move, int moveType, struct MoveState* moveState, struct AiData* aiData) {
    int i, score = 0;

    if (!moveState->damage) return 0;

    switch (ability) {
        case ABILITY_RECURRING_NIGHTMARE:
            REQUIRE(IsWeatherActive(WEATHER_FOG_ANY))
            REQUIRE(!GetSingleUseAbilityCounter(battlerDef, ability))
            REQUIRE(moveState->seeKo)
            return AI_SCORE_SWITCH(battlerDef) + AI_SCORE_REVIVE(battlerDef, 25);

        case ABILITY_LINGERING_AROMA:
        case ABILITY_MUMMY:
            REQUIRE(moveState->contact)
            REQUIRE_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(battlerAtk)))
            REQUIRE_NOT(DoesBattlerHaveAbilityShield(battlerAtk))
            REQUIRE_NOT(BattlerHasAbility(battlerAtk, ability, FALSE))
            return AI_SCORE_REPLACE_ABILITY(battlerAtk, ability);

        case ABILITY_WANDERING_SPIRIT:
            REQUIRE(moveState->contact)
            REQUIRE_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(battlerAtk)))
            REQUIRE_NOT(DoesBattlerHaveAbilityShield(battlerAtk))
            REQUIRE_NOT(BattlerHasAbility(battlerAtk, ability, FALSE))
            REQUIRE_NOT(BattlerHasAbility(battlerDef, GetBattlerAbility(battlerAtk), FALSE))
            return AI_SCORE_REPLACE_ABILITY(battlerAtk, ability) + AI_SCORE_REPLACE_ABILITY(battlerDef, GetBattlerAbility(battlerAtk));

        case ABILITY_GULP_MISSILE: {
            SpeciesEnum species = gBattleMons[battlerDef].species;
            REQUIRE(species == SPECIES_CRAMORANT_GORGING || species == SPECIES_CRAMORANT_GULPING)
            // TODO: Score turning back into normal cramorant
            return AI_SCORE_LOSE_HP(battlerAtk, 25) +
                   (species == SPECIES_CRAMORANT_GORGING ? AI_SCORE_STAT(battlerAtk, STAT_DEF, -1) : AI_SCORE_PARALYSIS(battlerDef));
        }

        case ABILITY_EMERGENCY_EXIT:
        case ABILITY_WIMP_OUT:
            REQUIRE_HALF_HP
            REQUIRE_NOT(TestSheerForceFlag(battlerAtk, gCurrentMove))
            return AI_SCORE_SWITCH(battlerDef);

        case ABILITY_RESTRAINING_ORDER:
            REQUIRE(!GetAbilityState(battlerDef, ability))
            return AI_SCORE_RANDOM_SWITCH(battlerAtk);

        case ABILITY_THERMAL_EXCHANGE:
            REQUIRE(moveType == TYPE_FIRE)
            return AI_SCORE_STAT(battlerDef, STAT_ATK, 1);

        case ABILITY_FURNACE:
            REQUIRE(moveType == TYPE_ROCK)
            return AI_SCORE_STAT(battlerDef, STAT_SPEED, 2);

        case ABILITY_WELL_BAKED_BODY:
            REQUIRE(moveType == TYPE_FIRE)
            return AI_SCORE_STAT(battlerDef, STAT_DEF, 1);

        case ABILITY_EVAPORATE:
            REQUIRE(moveType == TYPE_WATER)
            return AI_SCORE_MISTY_TERRAIN;

        case ABILITY_COLD_REBOUND:
            REQUIRE(moveState->contact)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_ICY_WIND, 0);

        case ABILITY_WILDFIRE:
            REQUIRE(moveState->contact)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_FIRE_SPIN, 0);

        case ABILITY_SNAP_TRAP_WHEN_HIT:
            REQUIRE(moveState->contact)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_SNAP_TRAP, 50);

        case ABILITY_PARRY:
            REQUIRE(moveState->contact)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_MACH_PUNCH, 0);

        case ABILITY_VICTORY_BOMB:
            REQUIRE(moveState->seeKo)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_EXPLOSION, 0);

        case ABILITY_ULTRA_INSTINCT:
            REQUIRE(moveState->contact)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_VACUUM_WAVE, 0);

        case ABILITY_ICE_DOWNFALL:
            REQUIRE(moveState->contact)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_ICICLE_CRASH, 60);

        case ABILITY_ATOMIC_BURST:
            REQUIRE(moveState->superEffective)
            return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_HYPER_BEAM, 50);

        case ABILITY_LOOSE_ROCKS:
            REQUIRE(moveState->contact)
            return AI_SCORE_STEALTH_ROCK(battlerDef, TYPE_ROCK);

        case ABILITY_WIND_POWER:
            REQUIRE(gBattleMoves[move].airBased)
            return AI_SCORE_CHARGE(battlerDef);

        case ABILITY_ELECTROMORPHOSIS:
            return AI_SCORE_CHARGE(battlerDef);

        case ABILITY_FLAMMABLE_COAT:
            REQUIRE(moveType == TYPE_FIRE)
            REQUIRE(gBattleMons[battlerDef].species == SPECIES_LUMBERING_SLOTH)
            REQUIRE_NOT(gBattleMons[battlerDef].status2 & STATUS2_TRANSFORMED)

            return AI_SCORE_FORM_CHANGE(battlerAtk, SPECIES_LUMBERING_SLOTH_ENGULFED);

        case ABILITY_ROUGH_SKIN:
        case ABILITY_IRON_BARBS:
            REQUIRE(moveState->contact)
            return AI_SCORE_LOSE_HP(battlerAtk, 13);

        case ABILITY_DOUBLE_IRON_BARBS:
            REQUIRE(moveState->contact)
            return AI_SCORE_LOSE_HP(battlerAtk, 17);

        case ABILITY_RATTLED:
            REQUIRE(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
            return AI_SCORE_STAT(battlerDef, STAT_SPEED, 1);

        case ABILITY_CURSED_BODY:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_DISABLE(battlerAtk));

        case ABILITY_SPITEFUL:
            REQUIRE(moveState->contact)
            return AI_SCORE_PP_DOWN(battlerAtk, 5);

        case ABILITY_COTTON_DOWN:
            for (i = GET_BATTLER_SIDE(BATTLE_OPPOSITE(battlerDef)); i < gBattlersCount; i += 2) {
                if (i == battlerDef) continue;
                score += AI_SCORE_STAT(i, STAT_SPEED, -1);
            }
            return score;

        case ABILITY_STEAM_ENGINE:
            REQUIRE(moveType == TYPE_FIRE || moveType == TYPE_WATER)
            return AI_SCORE_SPEED_UP(battlerDef, 12);

        case ABILITY_SAND_SPIT:
            return AI_SCORE_SANDSTORM;

        case ABILITY_CRYO_PROFICIENCY:
            return AI_SCORE_HAIL;

        case ABILITY_PERISH_BODY:
            REQUIRE(moveState->contact)
            return AI_SCORE_PERISH_SONG(battlerAtk) + AI_SCORE_PERISH_SONG(battlerDef);

        case ABILITY_GUILT_TRIP:
            REQUIRE(moveState->seeKo)
            return AI_SCORE_ATTACK_UP(battlerAtk, -2) + AI_SCORE_SPATK_UP(battlerAtk, -2);

        case ABILITY_ILL_WILL:
            REQUIRE(moveState->seeKo)
            return AI_SCORE_PP_DOWN(battlerAtk, 100);

        case ABILITY_INNARDS_OUT:
            REQUIRE(moveState->seeKo)
            return AI_SCORE_INNARDS_OUT(battlerAtk, battlerDef);

        case ABILITY_AFTERMATH:
            REQUIRE(moveState->seeKo)
            REQUIRE(moveState->contact)
            return AI_SCORE_LOSE_HP(battlerAtk, 25);

        case ABILITY_PATCHWORK:
            REQUIRE(moveState->breakShield);
            return AI_SCORE_CURSE(battlerAtk);

        case ABILITY_EFFECT_SPORE:
            REQUIRE(moveState->contact)
            REQUIRE_NOT(IS_BATTLER_OF_TYPE(battlerAtk, TYPE_GRASS))
            REQUIRE_NOT(BattlerHasAbility(battlerAtk, ABILITY_OVERCOAT, FALSE))
            REQUIRE_NOT(BattlerHasAbility(battlerAtk, ABILITY_EFFECT_SPORE, FALSE))
            REQUIRE(GetBattlerHoldEffect(battlerAtk, TRUE) != HOLD_EFFECT_SAFETY_GOGGLES)
            return AI_SCORE_ADJUST(10, AI_SCORE_PARALYSIS(battlerAtk) + AI_SCORE_POISON_MOVE(battlerAtk) + AI_SCORE_PARALYSIS(battlerAtk));

        case ABILITY_INFLATABLE:
            REQUIRE(moveType == TYPE_FIRE || moveType == TYPE_FLYING)
            return AI_SCORE_DEFENSE_UP(battlerDef, 1) + AI_SCORE_SPDEF_UP(battlerDef, 1);

        case ABILITY_BALLOON_BOMBER:
            return ScoreDefenseAbility(ABILITY_INFLATABLE, battlerAtk, battlerDef, move, moveType, moveState, aiData) +
                   ScoreDefenseAbility(ABILITY_AFTERMATH, battlerAtk, battlerDef, move, moveType, moveState, aiData);

        case ABILITY_WATER_COMPACTION:
            REQUIRE(moveType == TYPE_WATER)
            return AI_SCORE_STAT(battlerDef, STAT_DEF, 2);

        case ABILITY_VENGEFUL_SPIRIT:
        case ABILITY_HAUNTED_SPIRIT:
            REQUIRE(moveState->seeKo)
            REQUIRE(moveState->contact)
            return AI_SCORE_CURSE(battlerAtk);

        case ABILITY_MAGICAL_DUST:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_PSYCHIC);

        case ABILITY_WEAK_ARMOR:
            REQUIRE(IS_MOVE_PHYSICAL(move))
            return AI_SCORE_STAT(battlerDef, STAT_SPEED, 2) + AI_SCORE_STAT(battlerDef, STAT_DEF, -1);

        case ABILITY_ARC_FLASH:
            return AI_SCORE_ADJUST(50, AI_SCORE_BURN_MOVE(battlerAtk));

        case ABILITY_CROWNED_SHIELD:
        case ABILITY_STAMINA:
            return AI_SCORE_STAT(battlerDef, STAT_DEF, 1) + AI_SCORE_ADJUST(moveState->critChance, AI_SCORE_STAT(battlerDef, STAT_DEF, 12));

        case ABILITY_FORTITUDE:
            return AI_SCORE_STAT(battlerDef, STAT_SPDEF, 1) + AI_SCORE_ADJUST(moveState->critChance, AI_SCORE_STAT(battlerDef, STAT_SPDEF, 12));

        case ABILITY_RAGE_POINT:
            return AI_SCORE_ADJUST(moveState->critChance, AI_SCORE_STAT(battlerDef, STAT_ATK, 1) + AI_SCORE_STAT(battlerDef, STAT_SPATK, 1));

        case ABILITY_APE_SHIFT:
            score = ScoreAttackAbility(ABILITY_ANGER_POINT, battlerAtk, battlerDef, move, moveType, moveState, aiData);
            {
                SpeciesEnum species = GetHpFormChangeSpecies(battlerAtk, aiData);
                if (species == SPECIES_SLAKING_MEGA_APE_SHIFT) score += AI_SCORE_CURE_STATUS(battlerAtk);
                return score + AI_SCORE_FORM_CHANGE(battlerAtk, species);
            }

        case ABILITY_CROWNED_SWORD:
        case ABILITY_ANGER_POINT:
            return AI_SCORE_STAT(battlerDef, STAT_ATK, 1) + AI_SCORE_ADJUST(moveState->critChance, AI_SCORE_STAT(battlerDef, STAT_ATK, 12));

        case ABILITY_TIPPING_POINT:
            return AI_SCORE_STAT(battlerDef, STAT_SPATK, 1) + AI_SCORE_ADJUST(moveState->critChance, AI_SCORE_STAT(battlerDef, STAT_SPATK, 12));

        case ABILITY_BERSERK:
        case ABILITY_BERSERKER_RAGE:
        case ABILITY_UNLOCKED_POTENTIAL:
            REQUIRE_HALF_HP
            REQUIRE_NOT(GetAbilityState(battlerDef, ability))
            return AI_SCORE_STAT(battlerDef, STAT_SPATK, 1);

        case ABILITY_ANGER_SHELL:
            REQUIRE_HALF_HP
            REQUIRE_NOT(GetAbilityState(battlerDef, ability))
            score += AI_SCORE_STAT(battlerDef, STAT_ATK, 2);
            score += AI_SCORE_STAT(battlerDef, STAT_SPATK, 2);
            score += AI_SCORE_STAT(battlerDef, STAT_SPEED, 2);
            score += AI_SCORE_STAT(battlerDef, STAT_DEF, -1);
            score += AI_SCORE_STAT(battlerDef, STAT_SPDEF, -1);
            return score;

        case ABILITY_NO_TURNING_BACK:
            REQUIRE_HALF_HP
            REQUIRE_NOT(GetAbilityState(battlerDef, ability))
            score += AI_SCORE_STAT(battlerDef, STAT_ATK, 1);
            score += AI_SCORE_STAT(battlerDef, STAT_SPATK, 1);
            score += AI_SCORE_STAT(battlerDef, STAT_SPEED, 1);
            score += AI_SCORE_STAT(battlerDef, STAT_DEF, 1);
            score += AI_SCORE_STAT(battlerDef, STAT_SPDEF, 1);
            return score + AI_SCORE_NO_ESCAPE(battlerDef);

        case ABILITY_ITCHY_DEFENSE:
            REQUIRE(moveState->contact)
            return AI_SCORE_WRAP(battlerDef, battlerAtk);

        case ABILITY_LOOSE_QUILLS:
        case ABILITY_SCRAPYARD:
            REQUIRE(moveState->contact)
            return AI_SCORE_SPIKES(battlerDef);

        case ABILITY_TOXIC_DEBRIS:
            REQUIRE(moveState->contact)
            return AI_SCORE_TOXIC_SPIKES(battlerDef);

        case ABILITY_VOODOO_POWER:
            REQUIRE(IS_MOVE_SPECIAL(move))
            return AI_SCORE_ADJUST(30, AI_SCORE_BLEED(battlerAtk));

        case ABILITY_SEED_SOWER:
            REQUIRE_NOT(IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN))
            return AI_SCORE_GRASSY_TERRAIN + AI_SCORE_CURE_PARTY_STATUS(battlerDef);

        case ABILITY_SUPERSWEET_SYRUP:
            REQUIRE(moveState->contact)
            return AI_SCORE_EMBARGO(battlerAtk);

        case ABILITY_CUTE_CHARM:
        case ABILITY_PRIM_AND_PROPER:
        case ABILITY_PURE_LOVE:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_ATTRACT(battlerDef, battlerAtk));

        case ABILITY_GOOEY:
        case ABILITY_TANGLING_HAIR:
        case ABILITY_SUPER_HOT_GOO:
            REQUIRE(moveState->contact)
            return AI_SCORE_SPEED_UP(battlerAtk, -1);
    }

    return score;
}

// battlerAtk: battler with ability
int ScoreEitherAbility(AbilityEnum ability, int battlerAtk, int battlerDef, int move, int moveType, struct MoveState* moveState, struct AiData* aiData) {
    switch (ability) {
        case ABILITY_BLOOD_STAIN:
            REQUIRE(moveState->contact)
            REQUIRE_NOT(BattlerHasAbility(battlerDef, ABILITY_BLOOD_STAIN, FALSE))
            REQUIRE_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(battlerDef)))
            REQUIRE_NOT(DoesBattlerHaveAbilityShield(battlerDef))
            return AI_SCORE_REPLACE_ABILITY(battlerDef, ABILITY_BLOOD_STAIN);

        case ABILITY_SOUL_LINKER:
            REQUIRE_NOT(moveState->seeKo)
            return AI_SCORE_RECOIL(battlerDef, 100, TRUE);

        case ABILITY_DAMP:
            REQUIRE(moveState->contact)
            return AI_SCORE_SET_TYPE(battlerDef, TYPE_WATER);

        case ABILITY_WHITE_NOISE:
        case ABILITY_STATIC:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_PARALYSIS(battlerDef));

        case ABILITY_FLAME_BODY:
        case ABILITY_SUPER_HOT_GOO:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_BURN_MOVE(battlerDef));

        case ABILITY_FRAGRANT_DAZE:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_CONFUSION(battlerDef));

        case ABILITY_POISON_POINT:
        case ABILITY_POISON_TOUCH:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_POISON_MOVE(battlerDef));

        case ABILITY_FREEZING_POINT:
        case ABILITY_CRYO_PROFICIENCY:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_FROSTBITE_MOVE(battlerDef));

        case ABILITY_SPIKE_ARMOR:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_BLEED(battlerDef));

        case ABILITY_MENACING_SITUATION:
            REQUIRE(moveState->contact)
            return AI_SCORE_ADJUST(30, AI_SCORE_FEAR(battlerDef));
    }
    return 0;
}

int ScoreSwitchInFormShift(AbilityEnum ability, int battlerAtk, int battlerDef, int move, int moveType, struct AiData* aiData) {
    int score;

    switch (ability) {
        case ABILITY_ZERO_TO_HERO:

        case ABILITY_ICE_FACE:
            REQUIRE(IsWeatherActive(WEATHER_HAIL_ANY))
            REQUIRE(gBattleMons[battlerAtk].species == SPECIES_EISCUE_NOICE_FACE)
            REQUIRE_NOT(gBattleMons[battlerAtk].status2 && STATUS2_TRANSFORMED)
            return AI_SCORE_FORM_CHANGE(battlerAtk, SPECIES_EISCUE_NOICE_FACE);

        case ABILITY_DISGUISE:
        case ABILITY_PATCHWORK:
            REQUIRE(gBattleMons[battlerAtk].species == SPECIES_MIMIKYU_BUSTED || gBattleMons[battlerAtk].species == SPECIES_MIMIKYU_RAYQUAZA_BUSTED)
            REQUIRE(IsWeatherActive(WEATHER_FOG_ANY))
            REQUIRE_NOT(gBattleMons[battlerAtk].status2 && STATUS2_TRANSFORMED)
            return AI_SCORE_FORM_CHANGE(battlerAtk, gBattleMons[battlerAtk].species == SPECIES_MIMIKYU_BUSTED ? SPECIES_MIMIKYU : SPECIES_MIMIKYU_RAYQUAZA);

        case ABILITY_SCHOOLING:
            REQUIRE(gBattleMons[battlerAtk].level >= 20)
            FALLTHROUGH
        case ABILITY_SHIELDS_DOWN:
            return AI_SCORE_FORM_CHANGE(battlerAtk, GetHpFormChangeSpecies(battlerAtk, aiData));

        case ABILITY_FLOWER_GIFT:
            REQUIRE(IsWeatherActive(WEATHER_SUN_ANY))
            return AI_SCORE_FORM_CHANGE(battlerAtk, SPECIES_CHERRIM_SUNSHINE);

        case ABILITY_FORECAST:
            // TODO
            return 0;

        case ABILITY_ZEN_MODE:
            // TODO
            return 0;

        case ABILITY_APE_SHIFT: {
            SpeciesEnum species = GetHpFormChangeSpecies(battlerAtk, aiData);
            if (species == SPECIES_SLAKING_MEGA_APE_SHIFT) score += AI_SCORE_CURE_STATUS(battlerAtk);
            return score + AI_SCORE_FORM_CHANGE(battlerAtk, species);
        }
    }

    return 0;
}

int ScoreSwitchInAbility(AbilityEnum ability, int battlerAtk, int battlerDef, int move, int moveType, struct AiData* aiData) {
    int score;

    switch (ability) {
        case ABILITY_PARASITIC_SPORES:
            return AI_SCORE_GET_PARASITIC_SPORES(battlerAtk);

        case ABILITY_ANTICIPATION:
            REQUIRE_NOT(GetSingleUseAbilityCounter(battlerAtk, ABILITY_ANTICIPATION))
            return AI_SCORE_SET_ANTICIPATION;

        case ABILITY_HOSPITALITY:
            return AI_SCORE_HEAL(BATTLE_PARTNER(battlerAtk), 25);

        case ABILITY_FLARE_BOOST:
            REQUIRE(IsWeatherActive(WEATHER_FOG_ANY))
            return AI_SCORE_BURN_MOVE(battlerAtk);

        case ABILITY_IMPOSTER:
            REQUIRE_NOT(gBattleMons[battlerDef].status2 & (STATUS2_TRANSFORMED | STATUS2_SUBSTITUTE))
            REQUIRE_NOT(gBattleMons[battlerDef].status2 & STATUS2_TRANSFORMED)
            REQUIRE_NOT(gStatuses3[battlerDef] & STATUS3_SEMI_INVULNERABLE)
            return AI_SCORE_TRANSFORM;

        case ABILITY_TRACE:
            REQUIRE_NOT(IsRolePlayBannedAbility(GetBattlerAbility(battlerDef)))
            return AI_SCORE_REPLACE_ABILITY(battlerAtk, GetBattlerAbility(battlerDef));

        case ABILITY_MIMICRY:
            REQUIRE(TERRAIN_HAS_EFFECT)
            switch (gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) {
                case STATUS_FIELD_ELECTRIC_TERRAIN:
                    return AI_SCORE_SET_TYPE(battlerAtk, TYPE_ELECTRIC);
                case STATUS_FIELD_MISTY_TERRAIN:
                    return AI_SCORE_SET_TYPE(battlerAtk, TYPE_FAIRY);
                case STATUS_FIELD_PSYCHIC_TERRAIN:
                    return AI_SCORE_SET_TYPE(battlerAtk, TYPE_PSYCHIC);
                case STATUS_FIELD_GRASSY_TERRAIN:
                    return AI_SCORE_SET_TYPE(battlerAtk, TYPE_GRASS);
            }
            break;

        case ABILITY_CURIOUS_MEDICINE:
            return AI_SCORE_RESET_STAT_CHANGES(BATTLE_PARTNER(battlerAtk));

        case ABILITY_FRISK:
            return AI_SCORE_EMBARGO(battlerDef) + AI_SCORE_EMBARGO(BATTLE_PARTNER(battlerDef));

        case ABILITY_SEABORNE:
        case ABILITY_DRIZZLE:
            return AI_SCORE_RAIN;

        case ABILITY_SAND_STREAM:
        case ABILITY_DESERT_SPIRIT:
            return AI_SCORE_SANDSTORM;

        case ABILITY_DROUGHT:
        case ABILITY_ORICHALCUM_PULSE:
            return AI_SCORE_SUN;

        case ABILITY_LOW_VISIBILITY:
            return AI_SCORE_FOG;

        case ABILITY_SNOWY_WRATH:
        case ABILITY_SNOW_WARNING:
            return AI_SCORE_HAIL;

        case ABILITY_DESOLATE_LAND:
            return AI_SCORE_PRIMAL_SUN;

        case ABILITY_PRIMORDIAL_SEA:
            return AI_SCORE_PRIMAL_RAIN;

        case ABILITY_DELTA_STREAM:
            return AI_SCORE_STRONG_WINDS;

        case ABILITY_ELECTRIC_SURGE:
        case ABILITY_HADRON_ENGINE:
            return AI_SCORE_ELECTRIC_TERRAIN;

        case ABILITY_GRASSY_SURGE:
            return AI_SCORE_GRASSY_TERRAIN;

        case ABILITY_MISTY_SURGE:
            return AI_SCORE_MISTY_TERRAIN;

        case ABILITY_PSYCHIC_SURGE:
            return AI_SCORE_PSYCHIC_TERRAIN;

        case ABILITY_ENERGIZED:
        case ABILITY_GENERATOR:
            REQUIRE(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN) || !GetSingleUseAbilityCounter(battlerAtk, ability))
            return AI_SCORE_CHARGE(battlerAtk);

        case ABILITY_SCREEN_CLEANER:
            return AI_SCORE_BREAK_SCREENS(battlerAtk) + AI_SCORE_BREAK_SCREENS(battlerDef);

        case ABILITY_WIND_RIDER:
            REQUIRE(gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_TAILWIND)
            return AI_SCORE_STAT(battlerAtk, GetHighestAttackingStatId(battlerAtk, TRUE), 1);

        case ABILITY_DOWNLOAD:
            return AI_SCORE_STAT(battlerAtk, GetHighestDefendingStatId(battlerDef, TRUE) == STAT_DEF ? STAT_SPATK : STAT_ATK, 1);

        case ABILITY_LETS_ROLL:
            return AI_SCORE_STAT(battlerAtk, STAT_DEF, 1) + AI_SCORE_DEFENSE_CURL;

        case ABILITY_SLOW_START:
            // TODO: Slow Start switch scoring
            return 0;

        case ABILITY_LETHARGY:
            // TODO: Slow Start switch scoring
            return 0;

        case ABILITY_SEA_GUARDIAN:
            REQUIRE(IsWeatherActive(WEATHER_RAIN_ANY));
            return AI_SCORE_STAT(battlerAtk, GetHighestStatId(battlerAtk, TRUE), 1);

        case ABILITY_SUN_WORSHIP:
            REQUIRE(IsWeatherActive(WEATHER_SUN_ANY));
            return AI_SCORE_STAT(battlerAtk, GetHighestStatId(battlerAtk, TRUE), 1);

        case ABILITY_GREATER_SPIRIT:
            REQUIRE(IsWeatherActive(WEATHER_FOG_ANY));
            return AI_SCORE_STAT(battlerAtk, GetHighestStatId(battlerAtk, TRUE), 1);

        case ABILITY_COWARD:
            REQUIRE_NOT(GetSingleUseAbilityCounter(battlerAtk, ability))
            return AI_SCORE_PROTECT;

        case ABILITY_GRAVITY_WELL:
        case ABILITY_ATLAS:
            return AI_SCORE_GRAVITY;

        case ABILITY_PICKUP:
            return AI_SCORE_CLEAR_HAZARDS(battlerAtk);

        case ABILITY_FOREWARN:
            // TODO: Set damage to 50
            return AI_SCORE_FUTURE_SIGHT;

        case ABILITY_LOW_BLOW:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_FEINT_ATTACK, 40);

        case ABILITY_CHEAP_TACTICS:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_SCRATCH, 0);

        case ABILITY_DUST_CLOUD:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_SAND_ATTACK, 0);

        case ABILITY_TRICKSTER:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_DISABLE, 0);

        case ABILITY_SUPPRESS:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_TORMENT, 0);

        case ABILITY_DOOMBRINGER:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_DOOM_DESIRE, 0);

        case ABILITY_CHANGE_OF_HEART:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_HEART_SWAP, 0);

        case ABILITY_TELEKINETIC:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_TELEKINESIS, 0);

        case ABILITY_POWDER_BURST:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_POWDER, 0);

        case ABILITY_MONSTER_MASH:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_TRICK_OR_TREAT, 0);

        case ABILITY_PHANTOM_THIEF:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_SPECTRAL_THIEF, 40);

        case ABILITY_WEB_SPINNER:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_STRING_SHOT, 0);

        case ABILITY_DRACO_MORALE:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_DRAGON_CHEER, 0);

        case ABILITY_DREAM_WHIMSY:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_YAWN, 0);

        case ABILITY_JUMP_SCARE:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_ASTONISH, 0);

        case ABILITY_TAR_TOSS:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_TAR_SHOT, 0);

        case ABILITY_WIND_RAGE:
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_DEFOG, 0);

        case ABILITY_WISHMAKER:
            REQUIRE(GetSingleUseAbilityCounter(battlerAtk, ABILITY_WISHMAKER) < 3)
            return AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, MOVE_WISH, 0);

        case ABILITY_SOOTHING_AROMA:
            return AI_SCORE_CURE_PARTY_STATUS(battlerAtk);

        case ABILITY_BUTTER_UP:
            return AI_SCORE_CURE_PARTY_STATUS(battlerAtk) - AI_SCORE_CURE_STATUS(BATTLE_PARTNER(battlerAtk)) +
                   AI_SCORE_CURE_STATUS_AND_HEAL(BATTLE_PARTNER(battlerAtk), 25);

        case ABILITY_YUKI_ONNA:
        case ABILITY_FEARMONGER:
            return ScoreIntimidate(battlerDef, STAT_ATK, -1, ability, TRUE) + ScoreIntimidate(battlerDef, STAT_SPATK, -1, ability, TRUE);

        case ABILITY_INTIMIDATE:
            return ScoreIntimidate(battlerDef, STAT_ATK, -1, ability, TRUE);

        case ABILITY_SCARE:
            return ScoreIntimidate(battlerDef, STAT_SPATK, -1, ability, TRUE);

        case ABILITY_MONKEY_BUSINESS:
            return ScoreIntimidate(battlerDef, STAT_ATK, -1, ability, FALSE) + ScoreIntimidate(battlerDef, STAT_DEF, -1, ability, FALSE);

        case ABILITY_MALICIOUS:
            return ScoreIntimidate(battlerDef, STAT_HIGHEST_ATTACKING, -1, ability, TRUE) +
                   ScoreIntimidate(battlerDef, STAT_HIGHEST_DEFENDING, -1, ability, TRUE);

        case ABILITY_TERRIFY:
            return ScoreIntimidate(battlerDef, STAT_SPATK, -2, ability, TRUE);

        case ABILITY_GLEAM_EYES:
            return ScoreIntimidate(battlerDef, STAT_SPATK, -1, ability, TRUE) + AI_SCORE_EMBARGO(battlerDef);

        case ABILITY_MAJESTIC_MOTH:
            return AI_SCORE_STAT(battlerAtk, GetHighestStatId(battlerAtk, TRUE), 1);

        case ABILITY_PURIFYING_WATERS:
        case ABILITY_WATER_VEIL:
            return AI_SCORE_AQUA_RING;

        case ABILITY_COIL_UP:
        case ABILITY_SIDEWINDER:
            return AI_SCORE_COILED_UP;

        case ABILITY_AIR_BLOWER:
            return AI_SCORE_TAILWIND;

        case ABILITY_PASTEL_VEIL:
            return AI_SCORE_SAFEGUARD;

        case ABILITY_NORTH_WIND:
            return AI_SCORE_AURORA_VEIL(battlerAtk, SCREEN_DURATION);

        case ABILITY_SPIDER_LAIR:
            return AI_SCORE_STICKY_WEB;

        case ABILITY_INTREPID_SWORD:
        case ABILITY_CROWNED_SWORD:
            return AI_SCORE_STAT(battlerAtk, STAT_ATK, 1);

        case ABILITY_CROWNED_SHIELD:
        case ABILITY_DAUNTLESS_SHIELD:
            return AI_SCORE_STAT(battlerAtk, STAT_DEF, 1);

        case ABILITY_WHITE_SMOKE:
            return AI_SCORE_SMOKESCREEN;

        case ABILITY_HOT_COALS:
            return AI_SCORE_HOT_COALS;

        case ABILITY_PRESSURE:
            return AI_SCORE_CLEAR_STAT_BUFFS(battlerDef) + AI_SCORE_CLEAR_STAT_BUFFS(BATTLE_PARTNER(battlerDef)) +
                   AI_SCORE_CLEAR_STAT_BUFFS(BATTLE_PARTNER(battlerAtk));

        case ABILITY_PETRIFY:
            return AI_SCORE_CLEAR_STAT_BUFFS(battlerDef) + AI_SCORE_CLEAR_STAT_BUFFS(BATTLE_PARTNER(battlerDef)) +
                   ScoreIntimidate(battlerDef, STAT_SPEED, -1, ability, TRUE);

        case ABILITY_AIR_LOCK:
        case ABILITY_CLUELESS:
        case ABILITY_CLOUD_NINE:
            return AI_SCORE_REMOVE_WEATHER;

        case ABILITY_TWISTED_DIMENSION:
            return AI_SCORE_TRICK_ROOM(TRICK_ROOM_DURATION_SHORT);

        case ABILITY_INVERSE_ROOM:
            return AI_SCORE_INVERSE_ROOM(INVERSE_ROOM_DURATION_SHORT);

        case ABILITY_SALT_CIRCLE:
            return AI_SCORE_TRAP(battlerDef) + AI_SCORE_TRAP(BATTLE_PARTNER(battlerDef));

        case ABILITY_BERSERK_DNA:
            return AI_SCORE_CONFUSION(battlerAtk) + AI_SCORE_STAT(battlerAtk, GetHighestStatId(battlerAtk, TRUE), 2);

        case ABILITY_PROTOSYNTHESIS:
            if (IsWeatherActive(WEATHER_SUN_ANY)) return AI_SCORE_PARADOX_BOOST;

            if (GetBattlerHoldEffect(battlerAtk, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY) return AI_SCORE_PARADOX_BOOST + AI_SCORE_LOSE_ITEM(battlerAtk);
            break;

        case ABILITY_QUARK_DRIVE:
            if (IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) return AI_SCORE_PARADOX_BOOST;

            if (GetBattlerHoldEffect(battlerAtk, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY) return AI_SCORE_PARADOX_BOOST + AI_SCORE_LOSE_ITEM(battlerAtk);
            break;

        case ABILITY_FURNACE:
            REQUIRE(gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_STEALTH_ROCK)
            REQUIRE(gSideTimers[GetBattlerSide(battlerAtk)].stealthRockType == TYPE_ROCK)
            return AI_SCORE_STAT(battlerAtk, STAT_SPEED, 2);

        case ABILITY_COSTAR:
            return AI_SCORE_GET_STATS_OF(battlerAtk, BATTLE_PARTNER(battlerAtk));

        case ABILITY_WATCH_YOUR_STEP:
            return AI_SCORE_SPIKES(battlerDef) + AI_SCORE_SPIKES(battlerDef);

        case ABILITY_LAWNMOWER:
            REQUIRE(IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
            if (gFieldStatuses & (STATUS_FIELD_PSYCHIC_TERRAIN | STATUS_FIELD_MISTY_TERRAIN))
                score = AI_SCORE_STAT(battlerAtk, STAT_SPDEF, 1);
            else
                score = AI_SCORE_STAT(battlerAtk, STAT_DEF, 1);
            return score + AI_SCORE_REMOVE_TERRAIN;

        case ABILITY_SHOWDOWN_MODE:
        case ABILITY_VIOLENT_RUSH:
            return AI_SCORE_VIOLENT_RUSH;

        case ABILITY_RAPID_RESPONSE:
            return AI_SCORE_RAPID_RESPONSE;

        case ABILITY_DEMOLITIONIST:
        case ABILITY_READIED_ACTION:
            return AI_SCORE_READIED_ACTION;

        case ABILITY_ON_THE_PROWL:
        case ABILITY_OVERWATCH:
            return AI_SCORE_ON_THE_PROWL;

        case ABILITY_PHANTOM:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_GHOST);

        case ABILITY_AQUATIC:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_WATER);

        case ABILITY_GROUNDED:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_GROUND);

        case ABILITY_FAIRY_TALE:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_FAIRY);

        case ABILITY_ICE_AGE:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_ICE);

        case ABILITY_DRAGONFLY:
        case ABILITY_HALF_DRAKE:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_DRAGON);

        case ABILITY_METALLIC:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_STEEL);

        case ABILITY_HOVER:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_PSYCHIC);

        case ABILITY_TERAVOLT:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_ELECTRIC);

        case ABILITY_TURBOBLAZE:
            return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_FIRE);

        case ABILITY_POWER_OF_ALCHEMY:
            if (!gBattleMons[battlerDef].item || gBattleMons[battlerDef].item == ITEM_BIG_NUGGET ||
                !CanBattlerGetOrLoseItem(battlerDef, gBattleMons[battlerDef].item))
                battlerDef = BATTLE_PARTNER(battlerDef);
            REQUIRE(gBattleMons[battlerDef].item)
            REQUIRE(gBattleMons[battlerDef].item != ITEM_BIG_NUGGET)
            REQUIRE(CanBattlerGetOrLoseItem(battlerDef, gBattleMons[battlerDef].item))
            return AI_SCORE_LOSE_ITEM(battlerDef) +
                   AI_SCORE_GIVE_ITEM(battlerDef, gBattleMons[battlerDef].item == ITEM_BLACK_SLUDGE ? ITEM_BIG_NUGGET : ITEM_BLACK_SLUDGE);

        case ABILITY_REJECTION:
            return AI_SCORE_QUASH;
    }

    return 0;
}