#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_scripts.h"
#include "battle_main.h"
#include "mgba_printf/mgba.h"
#include "constants/abilities.h"
#include "constants/species.h"
#include "constants/battle_move_effects.h"
#include "constants/items.h"
#include "item.h"
#include "constants/moves.h"
#include "battle_anim.h"
#include "constants/hold_effects.h"
#include "battle_ai_new.h"
#include "battle_ai_scoring.h"

#define REQUIRE(check) if (!(check)) break;
#define REQUIRE_NOT(check) if (check) break;

// TODO: Gym Skill
// TODO: Commander

int ScoreAttackAbility(int ability, int battlerAtk, int battlerDef, int move, int moveType, struct AiData* aiData)
{
    int score = 0;

    switch (ability)
    {
    case ABILITY_VITAL_SPIRIT:
    case ABILITY_GULP_MISSILE:
    case ABILITY_SHIELDS_DOWN:
    case ABILITY_ANGELS_WRATH:
        break;
    
    default:
        if (!aiData->moveState.damage) return 0;
        break;
    }

    switch (ability)
    {
    case ABILITY_SHIELDS_DOWN:
        REQUIRE(gBattleMoves[move].effect == EFFECT_SHELL_SMASH)
        REQUIRE_NOT(gBattleMons[battlerAtk].status2 && STATUS2_TRANSFORMED)
        switch (gBattleMons[battlerAtk].species)
        {
        case SPECIES_MINIOR_CORE_BLUE:
        case SPECIES_MINIOR_CORE_GREEN:
        case SPECIES_MINIOR_CORE_INDIGO:
        case SPECIES_MINIOR_CORE_ORANGE:
        case SPECIES_MINIOR_CORE_RED:
        case SPECIES_MINIOR_CORE_VIOLET:
        case SPECIES_MINIOR_CORE_YELLOW:
            return 0; // TODO: Minior shell smash scoring
        }
        break;
    
    case ABILITY_GULP_MISSILE:
        REQUIRE_NOT(gBattleMons[battlerAtk].status2 && STATUS2_TRANSFORMED)
        REQUIRE(gBattleMons[battlerAtk].species == SPECIES_CRAMORANT)
        REQUIRE(move == MOVE_SURF || move == MOVE_DIVE)
        return 0; // TODO: Cramorant scoring

    case ABILITY_HYDRO_CIRCUIT:
        REQUIRE(moveType == TYPE_WATER)
        return AI_SCORE_ABSORB_MOVE(50);
    
    case ABILITY_VITALITY_STRIKE:
        REQUIRE(IS_IRON_FIST(battlerAtk, move))
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
        return AI_SCORE_ADJUST(aiData->moveState.critChance, AI_SCORE_BLEED(battlerDef));
    
    case ABILITY_KNOW_YOUR_PLACE:
        REQUIRE(aiData->moveState.contact)
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
        switch (moveType)
        {
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
            score = AI_SCORE_MISTY_TERRAIN(battlerAtk);
            break;
        
        case TYPE_GRASS:
            score = AI_SCORE_GRASSY_TERRAIN;
            break;
        
        case TYPE_NORMAL:
            score = AI_SCORE_ENCORE;
            break;
        
        case TYPE_ROCK:
            score = AI_SCORE_STEALTH_ROCK(battlerAtk, TYPE_ROCK);
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
            score = AI_SCORE_TRAP;
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
        REQUIRE(aiData->moveState.contact)
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
        return AI_SCORE_BREAK_SCREENS;
    
    case ABILITY_PINNACLE_BLADE:
        REQUIRE(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        return AI_SCORE_BREAK_PROTECT + AI_SCORE_BREAK_SCREENS + AI_SCORE_BREAK_SUBSTITUTE;

    case ABILITY_FEARMONGER:
        REQUIRE(aiData->moveState.contact)
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
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(20, AI_SCORE_CURSE(battlerDef));
    
    case ABILITY_SPECTRAL_SHROUD:
        REQUIRE(gBattleStruct->ateBoost[battlerAtk])
        REQUIRE(moveType == TYPE_GHOST)
        return AI_SCORE_ADJUST(30, AI_SCORE_TOXIC(battlerDef));
    
    case ABILITY_ANGELS_WRATH:
        switch (move) {
        case MOVE_TACKLE:
            REQUIRE(aiData->moveState.damage)
            return AI_SCORE_DISABLE(battlerDef) + AI_SCORE_ENCORE;
        
        case MOVE_STRING_SHOT:
            REQUIRE_NOT(gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_STEALTH_ROCK
                || gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_TOXIC_SPIKES
                || gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_SPIKES
                || gSideStatuses[GetBattlerSide(battlerAtk)] & SIDE_STATUS_STICKY_WEB)
            return AI_SCORE_STEALTH_ROCK(battlerAtk, TYPE_ROCK) + AI_SCORE_TOXIC_SPIKES(battlerAtk) + AI_SCORE_SPIKES(battlerAtk) + AI_SCORE_STICKY_WEB;
        
        case MOVE_HARDEN:
            return AI_SCORE_STAT(battlerAtk, STAT_ATK, 1) + AI_SCORE_STAT(battlerAtk, STAT_SPEED, 1) + AI_SCORE_STAT(battlerAtk, STAT_SPATK, 1) + AI_SCORE_STAT(battlerAtk, STAT_SPDEF, 1);
        
        case MOVE_IRON_DEFENSE:
            return AI_SCORE_PROTECT;

        case MOVE_ELECTROWEB:
            REQUIRE(aiData->moveState.damage);
            // TODO: Raw stat change
            return AI_SCORE_TRAP + AI_SCORE_STAT(battlerDef, STAT_SPEED, -12);
        
        case MOVE_BUG_BITE:
            REQUIRE(aiData->moveState.damage);
            return AI_SCORE_ABSORB_MOVE(100);
        }
        return 0;
    
    case ABILITY_ELEMENTAL_CHARGE:
        switch (moveType)
        {
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
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_LEECH_SEED;
    
    case ABILITY_GRIP_PINCER:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(50, AI_SCORE_WRAP(battlerAtk, battlerDef));
    }

    return 0;
}

#define REQUIRE_HALF_HP

int ScoreDefenseAbility(int ability, int battlerAtk, int battlerDef, int move, int moveType, struct AiData* aiData)
{
    int i, score = 0;

    if (!aiData->moveState.damage) return 0;

    switch (ability)
    {
    case ABILITY_LINGERING_AROMA:
    case ABILITY_MUMMY:
        REQUIRE(aiData->moveState.contact)
        REQUIRE_NOT(IsPersistentOrUnsuppressableAbility(gBattleMons[battlerAtk].ability))
        REQUIRE_NOT(DoesBattlerHaveAbilityShield(battlerAtk))
        return AI_SCORE_REPLACE_ABILITY(battlerAtk, ability);
    
    case ABILITY_WANDERING_SPIRIT:
        REQUIRE(aiData->moveState.contact)
        REQUIRE_NOT(IsPersistentOrUnsuppressableAbility(gBattleMons[battlerAtk].ability))
        REQUIRE_NOT(DoesBattlerHaveAbilityShield(battlerAtk))
        REQUIRE_NOT(HasAbility(battlerDef, gBattleMons[battlerAtk].ability, aiData))
        return AI_SCORE_REPLACE_ABILITY(battlerAtk, ability) + AI_SCORE_REPLACE_ABILITY(battlerDef, gBattleMons[battlerAtk].ability);
    
    case ABILITY_GULP_MISSILE:
        {
        int species = gBattleMons[battlerDef].species;
        REQUIRE(species == SPECIES_CRAMORANT_GORGING || species == SPECIES_CRAMORANT_GULPING)
        // TODO: Score turning back into normal cramorant
        return AI_SCORE_LOSE_HP(battlerAtk, 25)
            + (species == SPECIES_CRAMORANT_GORGING ? AI_SCORE_STAT(battlerAtk, STAT_DEF, -1) : AI_SCORE_PARALYSIS(battlerDef));
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
        return AI_SCORE_MISTY_TERRAIN(battlerDef);
    
    case ABILITY_COLD_REBOUND:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_ICY_WIND, 0);

    case ABILITY_WILDFIRE:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_FIRE_SPIN, 0);
    
    case ABILITY_SNAP_TRAP_WHEN_HIT:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_SNAP_TRAP, 50);
    
    case ABILITY_PARRY:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_MACH_PUNCH, 0);

    case ABILITY_VICTORY_BOMB:
        REQUIRE(aiData->moveState.seeKo)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_EXPLOSION, 0);
    
    case ABILITY_ULTRA_INSTINCT:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_VACUUM_WAVE, 0);
    
    case ABILITY_ICE_DOWNFALL:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_ICICLE_CRASH, 60);
    
    case ABILITY_ATOMIC_BURST:
        REQUIRE(aiData->moveState.superEffective)
        return AI_SCORE_EXTRA_MOVE(battlerDef, battlerAtk, MOVE_HYPER_BEAM, 50);
    
    case ABILITY_LOOSE_ROCKS:
        REQUIRE(aiData->moveState.contact)
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

        return AI_SCORE_LUMBERING_SLOTH_ENGULFED;
    
    case ABILITY_ROUGH_SKIN:
    case ABILITY_IRON_BARBS:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_LOSE_HP(battlerAtk, 13);

    case ABILITY_DOUBLE_IRON_BARBS:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_LOSE_HP(battlerAtk, 17);
    
    case ABILITY_RATTLED:
        REQUIRE(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
        return AI_SCORE_STAT(battlerDef, STAT_SPEED, 1);
    
    case ABILITY_CURSED_BODY:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_DISABLE(battlerAtk));
    
    case ABILITY_SPITEFUL:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_PP_DOWN(battlerAtk, 5);
    
    case ABILITY_COTTON_DOWN:
        for (i = 0; i < gBattlersCount; i++)
        {
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
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_PERISH_SONG(battlerAtk) + AI_SCORE_PERISH_SONG(battlerDef);
    
    case ABILITY_GUILT_TRIP:
        REQUIRE(aiData->moveState.seeKo)
        return AI_SCORE_ATTACK_UP(battlerAtk, -2) + AI_SCORE_SPATK_UP(battlerAtk, -2);
    
    case ABILITY_ILL_WILL:
        REQUIRE(aiData->moveState.seeKo)
        return AI_SCORE_PP_DOWN(battlerAtk, 100);
    
    case ABILITY_INNARDS_OUT:
        REQUIRE(aiData->moveState.seeKo)
        return AI_SCORE_INNARDS_OUT(battlerAtk, battlerDef);
    
    case ABILITY_AFTERMATH:
        REQUIRE(aiData->moveState.seeKo)
        return AI_SCORE_LOSE_HP(battlerAtk, 25);
    
    case ABILITY_PATCHWORK:
        REQUIRE(aiData->moveState.breakDisguise);
        return AI_SCORE_CURSE(battlerAtk);
    
    case ABILITY_EFFECT_SPORE:
        REQUIRE(aiData->moveState.contact)
        REQUIRE_NOT(IS_BATTLER_OF_TYPE(battlerAtk, TYPE_GRASS))
        REQUIRE_NOT(HasAbility(battlerAtk, ABILITY_OVERCOAT, aiData))
        REQUIRE_NOT(HasAbility(battlerAtk, ABILITY_EFFECT_SPORE, aiData))
        REQUIRE(GetBattlerHoldEffect(battlerAtk, TRUE) != HOLD_EFFECT_SAFETY_GOGGLES)
        return AI_SCORE_ADJUST(10, AI_SCORE_PARALYSIS(battlerAtk) + AI_SCORE_POISON_MOVE(battlerAtk) + AI_SCORE_PARALYSIS(battlerAtk));
    
    case ABILITY_INFLATABLE:
        REQUIRE(moveType == TYPE_FIRE || moveType == TYPE_FLYING)
        return AI_SCORE_DEFENSE_UP(battlerDef, 1) + AI_SCORE_SPDEF_UP(battlerDef, 1);

    case ABILITY_BALLOON_BOMBER:
        return ScoreDefenseAbility(ABILITY_INFLATABLE, battlerAtk, battlerDef, move, moveType, aiData)
            + ScoreDefenseAbility(ABILITY_AFTERMATH, battlerAtk, battlerDef, move, moveType, aiData);
    
    case ABILITY_WATER_COMPACTION:
        REQUIRE(moveType == TYPE_WATER)
        return AI_SCORE_STAT(battlerDef, STAT_DEF, 2);
    
    case ABILITY_VENGEFUL_SPIRIT:
    case ABILITY_HAUNTED_SPIRIT:
        REQUIRE(aiData->moveState.seeKo)
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_CURSE(battlerAtk);
    
    case ABILITY_MAGICAL_DUST:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADD_TYPE(battlerAtk, TYPE_PSYCHIC);
    
    case ABILITY_WEAK_ARMOR:
        REQUIRE(IS_MOVE_PHYSICAL(move))
        return AI_SCORE_STAT(battlerDef, STAT_SPEED, 2) + AI_SCORE_STAT(battlerDef, STAT_DEF, -1);

    case ABILITY_ARC_FLASH:
        return AI_SCORE_ADJUST(50, AI_SCORE_BURN_MOVE(battlerAtk));
    
    case ABILITY_CROWNED_SHIELD:
    case ABILITY_STAMINA:
        return AI_SCORE_STAT(battlerDef, STAT_DEF, 1) + AI_SCORE_ADJUST(aiData->moveState.critChance, AI_SCORE_STAT(battlerDef, STAT_DEF, 12));
        
    case ABILITY_FORTITUDE:
        return AI_SCORE_STAT(battlerDef, STAT_SPDEF, 1) + AI_SCORE_ADJUST(aiData->moveState.critChance, AI_SCORE_STAT(battlerDef, STAT_SPDEF, 12));
    
    case ABILITY_RAGE_POINT:
        return AI_SCORE_ADJUST(aiData->moveState.critChance, AI_SCORE_STAT(battlerDef, STAT_ATK, 1) + AI_SCORE_STAT(battlerDef, STAT_SPATK, 1));
    
    case ABILITY_APE_SHIFT:
        score = ScoreAttackAbility(ABILITY_ANGER_POINT, battlerAtk, battlerDef, move, moveType, aiData);
        REQUIRE(gBattleMons[battlerDef].species == SPECIES_SLAKING_MEGA)
        REQUIRE_NOT(gBattleMons[battlerDef].status2 & STATUS2_TRANSFORMED)
        REQUIRE_HALF_HP
        return score + AI_SCORE_APE_SHIFT(battlerDef);
        
    case ABILITY_CROWNED_SWORD:
    case ABILITY_ANGER_POINT:
        return AI_SCORE_STAT(battlerDef, STAT_ATK, 1) + AI_SCORE_ADJUST(aiData->moveState.critChance, AI_SCORE_STAT(battlerDef, STAT_ATK, 12));

    case ABILITY_TIPPING_POINT:
        return AI_SCORE_STAT(battlerDef, STAT_SPATK, 1) + AI_SCORE_ADJUST(aiData->moveState.critChance, AI_SCORE_STAT(battlerDef, STAT_SPATK, 12));
    
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
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_WRAP(battlerDef, battlerAtk);
    
    case ABILITY_LOOSE_QUILLS:
    case ABILITY_SCRAPYARD:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_SPIKES(battlerDef);

    case ABILITY_TOXIC_DEBRIS:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_TOXIC_SPIKES(battlerDef);
    
    case ABILITY_VOODOO_POWER:
        REQUIRE(IS_MOVE_SPECIAL(move))
        return AI_SCORE_ADJUST(30, AI_SCORE_BLEED(battlerAtk));
    
    case ABILITY_SEED_SOWER:
        REQUIRE_NOT(IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN))
        return AI_SCORE_GRASSY_TERRAIN + AI_SCORE_CURE_PARTY_STATUS(battlerDef);
    
    case ABILITY_SUPERSWEET_SYRUP:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_EMBARGO(battlerAtk);
    
    case ABILITY_CUTE_CHARM:
    case ABILITY_PRIM_AND_PROPER:
    case ABILITY_PURE_LOVE:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_ATTRACT(battlerDef, battlerAtk));
    
    case ABILITY_GOOEY:
    case ABILITY_TANGLING_HAIR:
    case ABILITY_SUPER_HOT_GOO:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_SPEED_UP(battlerAtk, -1);
    }

    return score;
}

// battlerAtk: battler with ability
int ScoreEitherAbility(int ability, int battlerAtk, int battlerDef, int move, int moveType, struct AiData* aiData)
{
    switch (ability)
    {
    case ABILITY_BLOOD_STAIN:
        REQUIRE(aiData->moveState.contact)
        REQUIRE_NOT(HasAbility(battlerDef, ABILITY_BLOOD_STAIN, aiData))
        REQUIRE_NOT(IsPersistentOrUnsuppressableAbility(gBattleMons[battlerDef].ability))
        REQUIRE_NOT(DoesBattlerHaveAbilityShield(battlerDef))
        return AI_SCORE_REPLACE_ABILITY(battlerDef, ABILITY_BLOOD_STAIN);
    
    case ABILITY_SOUL_LINKER:
        REQUIRE_NOT(aiData->moveState.seeKo)
        return AI_SCORE_RECOIL(battlerDef, 100, TRUE);
    
    case ABILITY_DAMP:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_SET_TYPE(battlerDef, TYPE_WATER);
    
    case ABILITY_WHITE_NOISE:
    case ABILITY_STATIC:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_PARALYSIS(battlerDef));
    
    case ABILITY_FLAME_BODY:
    case ABILITY_SUPER_HOT_GOO:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_BURN_MOVE(battlerDef));
    
    case ABILITY_FRAGRANT_DAZE:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_CONFUSION(battlerDef));
    
    case ABILITY_POISON_POINT:
    case ABILITY_POISON_TOUCH:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_POISON_MOVE(battlerDef));

    case ABILITY_FREEZING_POINT:
    case ABILITY_CRYO_PROFICIENCY:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_FROSTBITE_MOVE(battlerDef));
    
    case ABILITY_SPIKE_ARMOR:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_BLEED(battlerDef));
    
    case ABILITY_MENACING_SITUATION:
        REQUIRE(aiData->moveState.contact)
        return AI_SCORE_ADJUST(30, AI_SCORE_FEAR(battlerDef));
    
    case ABILITY_ENTRANCE:
        // TODO: Entrance handled in confusion scoring
        return 0;
    
    case ABILITY_POISON_PUPPETEER:
        // TODO: Poison Puppeteer handled in poison scoring
        return 0;
    
    case ABILITY_BLOODLUST:
    case ABILITY_BLOOD_BATH:
        // TODO: Bloodlust/Blood Bath handled in bleed scoring
        return 0;
    }
    return 0;
}

int ScoreSwitchInAbility(int ability, int battlerAtk, int battlerDef, int move, int moveType, struct AiData* aiData)
{
    
}