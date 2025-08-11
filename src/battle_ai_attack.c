#include "battle_ai_attack.h"

#include "behavior/ability/impl_battle.hh"
#include "battle.h"
#include "battle_ai_new.h"
#include "battle_ai_new_util.h"
#include "battle_ai_scoring.h"
#include "battle_anim.h"
#include "battle_main.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "generated/constants/abilities.h"
#include "generated/constants/battle_move_effects.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "generated/constants/moves.h"
#include "generated/constants/species.h"
#include "global.h"
#include "item.h"
#include "mgba_printf/mgba.h"

#define AI_GET_MOVE_EFFECT_CHANCE 0

int GetFullChance(int battlerAttack, int move, int moveEffect, int chance, struct AiData* aiData) {
    return GetMoveEffectChance(battlerAttack, move, moveEffect, chance);
}

int CheckPowder(int battlerAtk, int move) {
    int type;
    GET_MOVE_TYPE(move, type)
    return (type == TYPE_FIRE && gBattleMons[battlerAtk].status2 & STATUS2_POWDER);
}

// int ScoreDamage(int battlerAtk, int battlerDef, int move, u8* moveType, u16* effectiveness, struct MoveState* moveState, struct AiData* aiData)
// {
//     int damage, ignored, absorption, statId;
//     AbilityEnum ability;
//     // TODO: Handle fixed damage
//     damage = CalculateMoveDamageAndEffectiveness(move, battlerAtk, battlerDef, moveType, effectiveness);
//     switch (absorption)
//     {
//     case 1:
//         return AI_SCORE_HEAL(battlerDef, 25);
//     case 2:
//         return AI_SCORE_STAT(battlerDef, statId, ability == ABILITY_WELL_BAKED_BODY ? 2 : 1);
//     case 3:
//         return AI_SCORE_FLASH_FIRE;
//     }
//     if (!*effectiveness) return 0;
//     if (TestImmunityAbilities(battlerDef, battlerAtk, move, *moveType, (const u8**) &ignored, (u8*) &ignored, (u16*) ignored))
//     {
//         *effectiveness = 0;
//         return 0;
//     }
//     return (moveState->damage = AdjustForMultihit(damage, battlerAtk, move, moveState, aiData));
//     return 0;
// }

#define AI_CALC_DAMAGE

int ScoreArgument(int battlerAtk, int battlerDef, int move, struct MoveState* moveState, struct AiData* aiData) {
    int argument = gBattleMoves[move].argument;
    // int certain = argument & MOVE_EFFECT_CERTAIN;
    // TODO: Handle this if needed
    // int ignoreTypeImmunities = argument & MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES;
    // int applyTo = argument & MOVE_EFFECT_AFFECTS_USER ? battlerAtk : battlerDef;
    argument &= ~(MOVE_EFFECT_CERTAIN | MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES | MOVE_EFFECT_AFFECTS_USER);
    switch (argument) {
        case MOVE_EFFECT_POISON:
            return AI_SCORE_POISON_MOVE(applyTo);
        case MOVE_EFFECT_BURN:
            return AI_SCORE_BURN_MOVE(applyTo);
        case MOVE_EFFECT_FROSTBITE:
            return AI_SCORE_FROSTBITE_MOVE(applyTo);
        case MOVE_EFFECT_PARALYSIS:
            return AI_SCORE_PARALYSIS(applyTo);
        case MOVE_EFFECT_TOXIC:
            return AI_SCORE_TOXIC(battlerDef);
        case MOVE_EFFECT_BLEED:
            return AI_SCORE_BLEED(battlerDef);
        case MOVE_EFFECT_CONFUSION:
            return AI_SCORE_CONFUSION(battlerDef);
        case MOVE_EFFECT_FLINCH:
            return AI_SCORE_FLINCH(battlerDef);
        case MOVE_EFFECT_ATK_PLUS_1:
            return AI_SCORE_ATTACK_UP(applyTo, 1);
        case MOVE_EFFECT_SP_ATK_PLUS_1:
            return AI_SCORE_SPATK_UP(applyTo, 1);
        case MOVE_EFFECT_DEF_MINUS_1:
            return AI_SCORE_DEFENSE_UP(applyTo, -1);
        case MOVE_EFFECT_SPD_MINUS_1:
            return AI_SCORE_SPEED_UP(applyTo, -1);
        case MOVE_EFFECT_SP_ATK_MINUS_1:
            return AI_SCORE_SPATK_UP(applyTo, -1);
        case MOVE_EFFECT_SPD_MINUS_2:
            return AI_SCORE_SPEED_UP(applyTo, -1);
        case MOVE_EFFECT_FEINT:
            return AI_SCORE_BREAK_PROTECT;
        case MOVE_EFFECT_GLAIVE_RUSH:
            // TODO: Delay effect
            return AI_SCORE_GLAIVE_RUSH;
        case MOVE_EFFECT_SALT_CURE:
            return AI_SCORE_SALT_CURE;
        case MOVE_EFFECT_ORDER_UP:
            if (gBattleMons[battlerAtk].species == SPECIES_DONDOZO && IsBattlerAlive(BATTLE_PARTNER(battlerAtk)) &&
                GetAbilityState(BATTLE_PARTNER(battlerAtk), ABILITY_COMMANDER)) {
                switch (gBattleMons[BATTLE_PARTNER(battlerAtk)].species) {
                    case SPECIES_TATSUGIRI:
                    case SPECIES_TATSUGIRI_CURLY:
                        return AI_SCORE_ATTACK_UP(applyTo, 1);
                    case SPECIES_TATSUGIRI_DROOPY:
                        return AI_SCORE_DEFENSE_UP(applyTo, 1);
                    case SPECIES_TATSUGIRI_STRETCHY:
                        return AI_SCORE_SPDEF_UP(applyTo, 1);
                }
            }
            break;
        case MOVE_EFFECT_WATER_PLEDGE:
            if (IsWeatherActive(WEATHER_SUN_ANY)) return AI_SCORE_RAINBOW;
            if (IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN)) return AI_SCORE_SWAMP;
            break;
        case MOVE_EFFECT_FIRE_PLEDGE:
            if (IsWeatherActive(WEATHER_RAIN_ANY)) return AI_SCORE_RAINBOW;
            if (IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN)) return AI_SCORE_FIRE_SEA;
            break;
        case MOVE_EFFECT_GRASS_PLEDGE:
            if (IsWeatherActive(WEATHER_SUN_ANY)) return AI_SCORE_FIRE_SEA;
            if (IsWeatherActive(WEATHER_RAIN_ANY)) return AI_SCORE_SWAMP;
            break;
        case MOVE_EFFECT_SYRUP:
            return AI_SCORE_SYRUP;
            break;
        case MOVE_EFFECT_DIRE_CLAW:
            return (AI_SCORE_POISON_MOVE(applyTo) + AI_SCORE_BLEED(applyTo) + AI_SCORE_PARALYSIS(applyTo)) / 3;
        case MOVE_EFFECT_YAWN:
            return AI_SCORE_DROWSY;
        case MOVE_EFFECT_PSYCHIC_NOISE:
            return AI_SCORE_HEAL_BLOCK(2);
        case MOVE_EFFECT_HIGHEST_STAT_EXCEPT_SPEED_PLUS_1:
            return AI_SCORE_STAT(applyTo, GetHighestStatIdExcept(applyTo, FALSE, STAT_SPEED), 1);
        case MOVE_EFFECT_MAKE_IT_RAIN:
            // TODO: Delay
            return AI_SCORE_SPATK_UP(battlerAtk, -1);
        case MOVE_EFFECT_SCALE_SHOT:
            // TODO: Delay
            return AI_SCORE_STAT(battlerAtk, STAT_SPEED, 1) + AI_SCORE_STAT(battlerAtk, STAT_DEF, -1);
        case MOVE_EFFECT_WYRM_WIND:
            // TODO: Delay
            return AI_SCORE_STAT(battlerAtk, STAT_SPEED, 1) + AI_SCORE_STAT(battlerAtk, STAT_SPDEF, -1);
    }

    return 0;
}

#define LOCAL_LABEL(label) __LOCAL_LABEL__(label)
#define __LOCAL_LABEL__(label) AI_ScoreMoveHit_##label
#define CASE_AND_LABEL(label) \
    case label:               \
        LOCAL_LABEL(label) :
#define GOTO(label) goto LOCAL_LABEL(label)

int ScoreMoveHit(int battlerAtk, int battlerDef, int moveEffect, int move, int turn, u8 moveType, struct MoveState* moveState, struct AiData* aiData) {
    int score;

    if (move == MOVE_NONE) return 0;
    if (!IsBattlerAlive(battlerDef)) return 0;

    SetTypeBeforeUsingMove(move, battlerAtk);
    if (IS_MOVE_STATUS(move) && CheckPowder(battlerAtk, move)) {
        return AI_SCORE_LOSE_HP(battlerAtk, 25);
    }

    switch (gBattleMoves[move].effect) {
        CASE_AND_LABEL(EFFECT_MIRROR_MOVE)
        CASE_AND_LABEL(EFFECT_BIDE)
        // TODO: Handle weird moves
        CASE_AND_LABEL(EFFECT_PLACEHOLDER)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_MULTI_HIT)
        CASE_AND_LABEL(EFFECT_RAMPAGE)  // TODO: Negative score for locking
        CASE_AND_LABEL(EFFECT_HIT)
        AI_CALC_DAMAGE;
        return score;

        CASE_AND_LABEL(EFFECT_SLEEP)
        return AI_SCORE_SLEEP_MOVE(battlerDef);

        CASE_AND_LABEL(EFFECT_POISON_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_POISON_MOVE(battlerDef));

        CASE_AND_LABEL(EFFECT_ABSORB)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ABSORB_MOVE(gBattleMoves[move].argument || 50);

        CASE_AND_LABEL(EFFECT_BURN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BURN_MOVE(battlerDef));

        CASE_AND_LABEL(EFFECT_FREEZE_HIT)
        CASE_AND_LABEL(EFFECT_FROSTBITE_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_FROSTBITE_MOVE(battlerDef));

        CASE_AND_LABEL(EFFECT_PARALYZE_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_PARALYSIS(battlerDef));

        CASE_AND_LABEL(EFFECT_REVENGE)
        // TODO: Handle
        AI_CALC_DAMAGE;
        return score;

        CASE_AND_LABEL(EFFECT_EXPLOSION)  // TODO: Negative score for dying
        if (move == MOVE_SELF_DESTRUCT) GOTO(EFFECT_REVENGE);
        AI_CALC_DAMAGE;
        return score;

        CASE_AND_LABEL(EFFECT_DREAM_EATER)
        if (!IsSleeping(battlerDef, aiData)) return AI_SCORE_IMMUNE;
        GOTO(EFFECT_ABSORB);

        CASE_AND_LABEL(EFFECT_ATTACK_UP)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SPEED_UP)
        return AI_SCORE_SPEED_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_UP)
        return AI_SCORE_SPATK_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SPECIAL_DEFENSE_UP)
        return AI_SCORE_SPDEF_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_ACCURACY_UP)
        return AI_SCORE_ACC_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_EVASION_UP)
        return AI_SCORE_EVASION_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_UP_3)
        return AI_SCORE_SPATK_UP(battlerAtk, 3);

        CASE_AND_LABEL(EFFECT_ATTACK_DOWN)
        return AI_SCORE_ATTACK_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_DEFENSE_DOWN)
        return AI_SCORE_DEFENSE_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_SPEED_DOWN)
        return AI_SCORE_SPEED_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_DOWN)
        return AI_SCORE_SPATK_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_SPECIAL_DEFENSE_DOWN)
        return AI_SCORE_SPDEF_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_ACCURACY_DOWN)
        return AI_SCORE_ACC_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_EVASION_DOWN)
        return AI_SCORE_EVASION_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_HAZE)
        return AI_SCORE_HAZE;

        CASE_AND_LABEL(EFFECT_ROAR)
        return AI_SCORE_RANDOM_SWITCH(battlerDef);

        CASE_AND_LABEL(EFFECT_CONVERSION)  // TODO: Score for type change
        return AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_FLINCH_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_FLINCH(battlerDef));

        CASE_AND_LABEL(EFFECT_RESTORE_HP)  // TODO: Heal block
        return AI_SCORE_HEAL(battlerAtk, 50);

        CASE_AND_LABEL(EFFECT_TOXIC)
        return AI_SCORE_TOXIC(battlerDef);

        CASE_AND_LABEL(EFFECT_PAY_DAY)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_LIGHT_SCREEN)
        return AI_SCORE_LIGHTSCREEN;

        CASE_AND_LABEL(EFFECT_TRI_ATTACK)
        AI_CALC_DAMAGE;
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_PARALYSIS(battlerDef));
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BURN_MOVE(battlerDef));
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_FROSTBITE_MOVE(battlerDef));
        return score;

        CASE_AND_LABEL(EFFECT_REST)
        if (IsSleeping(battlerAtk, aiData)) return AI_SCORE_IMMUNE;
        score = AI_SCORE_HEAL(battlerAtk, 100);
        score += AI_SCORE_CURE_STATUS(battlerAtk);
        score += AI_SCORE_SLEEP_MOVE(battlerAtk);
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_OHKO)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_DRAGON_RAGE)
        CASE_AND_LABEL(EFFECT_SUPER_FANG)
        CASE_AND_LABEL(EFFECT_FUSION_COMBO)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_TRAP)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_WRAP(battlerAtk, battlerDef);

        CASE_AND_LABEL(EFFECT_HEAL_BLOCK)
        return AI_SCORE_HEAL_BLOCK(5);

        CASE_AND_LABEL(EFFECT_DOUBLE_HIT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RECOIL_IF_MISS)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_MIST)
        return AI_SCORE_MIST;

        CASE_AND_LABEL(EFFECT_FOCUS_ENERGY)
        return AI_SCORE_CRIT_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_RECOIL_25)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 25, FALSE);

        CASE_AND_LABEL(EFFECT_CONFUSE)
        return AI_SCORE_CONFUSION(battlerDef);

        CASE_AND_LABEL(EFFECT_ATTACK_UP_2)
        return AI_SCORE_ATTACK_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_DEFENSE_UP_2)
        return AI_SCORE_DEFENSE_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_SPEED_UP_2)
        return AI_SCORE_SPEED_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_UP_2)
        return AI_SCORE_SPATK_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_SPECIAL_DEFENSE_UP_2)
        return AI_SCORE_SPDEF_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_ACCURACY_UP_2)
        return AI_SCORE_ACC_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_EVASION_UP_2)
        return AI_SCORE_EVASION_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_TRANSFORM)
        return AI_SCORE_TRANSFORM;

        CASE_AND_LABEL(EFFECT_ATTACK_DOWN_2)
        return AI_SCORE_ATTACK_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_DEFENSE_DOWN_2)
        return AI_SCORE_DEFENSE_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_SPEED_DOWN_2)
        return AI_SCORE_SPEED_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_DOWN_2)
        return AI_SCORE_SPATK_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_SPECIAL_DEFENSE_DOWN_2)
        return AI_SCORE_SPDEF_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_ACCURACY_DOWN_2)
        return AI_SCORE_ACC_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_EVASION_DOWN_2)
        return AI_SCORE_EVASION_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_REFLECT)
        return AI_SCORE_REFLECT;

        CASE_AND_LABEL(EFFECT_POISON)
        return AI_SCORE_POISON_MOVE(battlerDef);

        CASE_AND_LABEL(EFFECT_PARALYZE)
        return AI_SCORE_PARALYSIS(battlerDef);

        CASE_AND_LABEL(EFFECT_ATTACK_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ATTACK_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_DEFENSE_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_DEFENSE_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_SPEED_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPEED_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPATK_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_SPECIAL_DEFENSE_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPDEF_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_ACCURACY_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ACC_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_EVASION_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_EVASION_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_TWO_TURNS_ATTACK)
        // TODO: Two-turn attacks
        if (turn == 0) return AI_SCORE_IMMUNE;
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_CONFUSE_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_CONFUSION(battlerDef));

        CASE_AND_LABEL(EFFECT_TWINEEDLE)
        GOTO(EFFECT_POISON_HIT);

        CASE_AND_LABEL(EFFECT_VITAL_THROW)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SUBSTITUTE)
        return AI_SCORE_LOSE_HP(battlerDef, 25) + AI_SCORE_SUBSTITUTE;

        CASE_AND_LABEL(EFFECT_RECHARGE)  // TODO: Handle recharge
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RAGE)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_MIMIC)
        return AI_SCORE_IMMUNE;  // TODO: Mimic

        CASE_AND_LABEL(EFFECT_METRONOME)
        return AI_SCORE_IMMUNE;  // TODO: Metronome

        CASE_AND_LABEL(EFFECT_LEECH_SEED)
        return AI_SCORE_LEECH_SEED;

        CASE_AND_LABEL(EFFECT_DO_NOTHING)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_DISABLE)
        return AI_SCORE_DISABLE(battlerDef);

        CASE_AND_LABEL(EFFECT_LEVEL_DAMAGE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_PSYWAVE)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_COUNTER)
        return AI_SCORE_COUNTER;

        CASE_AND_LABEL(EFFECT_ENCORE)
        return AI_SCORE_ENCORE;

        CASE_AND_LABEL(EFFECT_PAIN_SPLIT)
        return AI_SCORE_PAIN_SPLIT;

        CASE_AND_LABEL(EFFECT_SNORE)
        if (!IsSleeping(battlerAtk, aiData)) return AI_SCORE_IMMUNE;
        GOTO(EFFECT_FLINCH_HIT);

        CASE_AND_LABEL(EFFECT_CONVERSION_2)
        // TODO: Handle type conversion
        return AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_LOCK_ON)
        return AI_SCORE_LOCK_ON;

        CASE_AND_LABEL(EFFECT_SKETCH)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_HAMMER_ARM)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_SPEED_UP(battlerAtk, -1);

        CASE_AND_LABEL(EFFECT_SLEEP_TALK)
        // TODO: Resolve this last so that it can just add up the scores of the other moves on the mon
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_DESTINY_BOND)
        return AI_SCORE_DESTINY_BOND;

        CASE_AND_LABEL(EFFECT_FLAIL)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SPITE)
        return AI_SCORE_PP_DOWN(battlerDef, 4);

        CASE_AND_LABEL(EFFECT_FALSE_SWIPE)
        moveState->falseSwipe = TRUE;
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_HEAL_BELL)
        return AI_SCORE_HEAL(battlerAtk, 30) + AI_SCORE_CURE_PARTY_STATUS(battlerAtk);

        CASE_AND_LABEL(EFFECT_ALWAYS_CRIT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_TRIPLE_KICK)
        GOTO(EFFECT_HIT);  // TODO: Handle triple kick

        CASE_AND_LABEL(EFFECT_THIEF)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_THIEF;

        CASE_AND_LABEL(EFFECT_MEAN_LOOK)
        return AI_SCORE_TRAP(battlerDef);

        CASE_AND_LABEL(EFFECT_NIGHTMARE)
        if (!IsSleeping(battlerDef, aiData)) return AI_SCORE_IMMUNE;
        AI_CALC_DAMAGE;
        return score + AI_SCORE_NIGHTMARE;

        CASE_AND_LABEL(EFFECT_MINIMIZE)
        return AI_SCORE_EVASION_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_CURSE)
        if (IS_BATTLER_OF_TYPE(battlerAtk, TYPE_GHOST) || IsBattlerWeatherAffected(battlerAtk, WEATHER_FOG_ANY)) {
            return AI_SCORE_LOSE_HP(battlerDef, 50) + AI_SCORE_CURSE(battlerDef);
        } else {
            return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_DEFENSE_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, -1);
        }

        CASE_AND_LABEL(EFFECT_HEALING_WISH)
        // TODO: Healing wish
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_PROTECT)
        return AI_SCORE_PROTECT;

        CASE_AND_LABEL(EFFECT_SPIKES)
        return AI_SCORE_SPIKES(battlerDef);

        CASE_AND_LABEL(EFFECT_FORESIGHT)
        // TODO: Foresight
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_PERISH_SONG)
        return AI_SCORE_PERISH_SONG(battlerDef);

        CASE_AND_LABEL(EFFECT_SANDSTORM)
        return AI_SCORE_SANDSTORM;

        CASE_AND_LABEL(EFFECT_ENDURE)
        return AI_SCORE_ENDURE;

        CASE_AND_LABEL(EFFECT_ROLLOUT)
        // TODO: Rollout
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_SWAGGER)
        return AI_SCORE_ATTACK_UP(battlerDef, 2) + AI_SCORE_CONFUSION(battlerDef);

        CASE_AND_LABEL(EFFECT_FURY_CUTTER)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_ATTRACT)
        return AI_SCORE_ATTRACT(battlerAtk, battlerDef);

        CASE_AND_LABEL(EFFECT_RETURN)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_PRESENT)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_FRUSTRATION)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SAFEGUARD)
        return AI_SCORE_SAFEGUARD;

        CASE_AND_LABEL(EFFECT_UNUSED_125)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_MAGNITUDE)
        // TODO: Magnitude
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_BATON_PASS)
        // TODO: Baton Pass
        return AI_SCORE_SWITCH(battlerAtk);

        CASE_AND_LABEL(EFFECT_PURSUIT)
        AI_CALC_DAMAGE;
        // TODO: Pursuit
        return score;

        CASE_AND_LABEL(EFFECT_RAPID_SPIN)
        AI_CALC_DAMAGE;
        // TODO: Add hazard clearing
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPEED_UP(battlerAtk, 1));

        CASE_AND_LABEL(EFFECT_SONICBOOM)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_CAPTIVATE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_MORNING_SUN)
        if (SeesSunlight(battlerAtk, aiData)) return AI_SCORE_HEAL(battlerAtk, 66);
        if (IsBattlerWeatherAffected(battlerAtk, WEATHER_RAIN_ANY)) return AI_SCORE_HEAL(battlerAtk, 25);
        return AI_SCORE_HEAL(battlerAtk, 50);

        CASE_AND_LABEL(EFFECT_SYNTHESIS)
        GOTO(EFFECT_MORNING_SUN);

        CASE_AND_LABEL(EFFECT_MOONLIGHT)
        if (BattlerHasAbility(battlerAtk, ABILITY_MOON_SPIRIT, FALSE)) return AI_SCORE_HEAL(battlerAtk, 66);
        GOTO(EFFECT_MORNING_SUN);

        CASE_AND_LABEL(EFFECT_HIDDEN_POWER)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RAIN_DANCE)
        return AI_SCORE_RAIN;

        CASE_AND_LABEL(EFFECT_SUNNY_DAY)
        return AI_SCORE_SUN;

        CASE_AND_LABEL(EFFECT_DEFENSE_UP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_DEFENSE_UP(battlerAtk, 1));

        CASE_AND_LABEL(EFFECT_ATTACK_UP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ATTACK_UP(battlerAtk, 1));

        CASE_AND_LABEL(EFFECT_ALL_STATS_UP_HIT)
        AI_CALC_DAMAGE;
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ATTACK_UP(battlerAtk, 1));
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_DEFENSE_UP(battlerAtk, 1));
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPEED_UP(battlerAtk, 1));
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPATK_UP(battlerAtk, 1));
        score += AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPDEF_UP(battlerAtk, 1));
        return score;

        CASE_AND_LABEL(EFFECT_FELL_STINGER)
        // TODO: Fell Stinger
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_BELLY_DRUM)
        return AI_SCORE_ATTACK_UP(battlerAtk, 12) + AI_SCORE_LOSE_HP(battlerDef, 50);

        CASE_AND_LABEL(EFFECT_PSYCH_UP)
        return AI_SCORE_GET_STATS_OF(battlerAtk, battlerDef);

        CASE_AND_LABEL(EFFECT_MIRROR_COAT)
        return AI_SCORE_MIRROR_COAT;

        CASE_AND_LABEL(EFFECT_SKULL_BASH)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_TWISTER)
        // TODO: Make Twister trap
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_EARTHQUAKE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_FUTURE_SIGHT)
        return AI_SCORE_FUTURE_SIGHT;

        CASE_AND_LABEL(EFFECT_GUST)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_FLINCH_MINIMIZE_HIT)
        GOTO(EFFECT_FLINCH_HIT);

        CASE_AND_LABEL(EFFECT_SOLARBEAM)
        // TODO: Solarbeam
        GOTO(EFFECT_TWO_TURNS_ATTACK);

        CASE_AND_LABEL(EFFECT_THUNDER)
        GOTO(EFFECT_PARALYZE_HIT);

        CASE_AND_LABEL(EFFECT_SWITCH_ARGUMENT)
        // TODO: Safe passage
        if (move == MOVE_SAFE_PASSAGE) score = AI_SCORE_SAFE_PASSAGE;
        return score + AI_SCORE_SWITCH(battlerAtk);

        CASE_AND_LABEL(EFFECT_BEAT_UP)
        // TODO: Beat up
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SEMI_INVULNERABLE)
        if (turn == 0) return AI_SCORE_PROTECT;
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_DEFENSE_CURL)
        return AI_SCORE_DEFENSE_CURL + AI_SCORE_DEFENSE_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SOFTBOILED)
        GOTO(EFFECT_RESTORE_HP);

        CASE_AND_LABEL(EFFECT_FAKE_OUT)
        // TODO: Check usable
        GOTO(EFFECT_FLINCH_HIT);

        CASE_AND_LABEL(EFFECT_UPROAR)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_STOCKPILE)
        // TODO: Stockpile Spit Up/Swallow interaction
        return AI_SCORE_DEFENSE_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SPIT_UP)
        AI_CALC_DAMAGE;
        if (gVolatileStructs[battlerAtk].stockpileDef) score += AI_SCORE_DEFENSE_UP(battlerAtk, -1);
        if (gVolatileStructs[battlerAtk].stockpileSpDef) score += AI_SCORE_SPDEF_UP(battlerAtk, -1);
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SWALLOW)
        if (gVolatileStructs[battlerAtk].stockpileDef) score += AI_SCORE_DEFENSE_UP(battlerAtk, -1);
        if (gVolatileStructs[battlerAtk].stockpileSpDef) score += AI_SCORE_SPDEF_UP(battlerAtk, -1);
        return AI_SCORE_HEAL(battlerAtk, 50);

        CASE_AND_LABEL(EFFECT_WORRY_SEED)
        return AI_SCORE_REPLACE_ABILITY(battlerAtk, ABILITY_INSOMNIA) + AI_SCORE_FEAR(battlerDef);

        CASE_AND_LABEL(EFFECT_HAIL)
        return AI_SCORE_HAIL;

        CASE_AND_LABEL(EFFECT_TORMENT)
        return AI_SCORE_TORMENT;

        CASE_AND_LABEL(EFFECT_FLATTER)
        return AI_SCORE_SPATK_UP(battlerDef, 2) + AI_SCORE_CONFUSION(battlerDef);

        CASE_AND_LABEL(EFFECT_WILL_O_WISP)
        return AI_SCORE_BURN_MOVE(battlerDef);

        CASE_AND_LABEL(EFFECT_MEMENTO)
        // TODO: Memento
        return AI_SCORE_SPATK_UP(battlerDef, -2) + AI_SCORE_SPDEF_UP(battlerDef, -2);

        CASE_AND_LABEL(EFFECT_FACADE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_FOCUS_PUNCH)
        // TODO: Focus Punch
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SMELLINGSALT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_CURE_STATUS(battlerAtk);

        CASE_AND_LABEL(EFFECT_FOLLOW_ME)
        return AI_SCORE_FOLLOW_ME;

        CASE_AND_LABEL(EFFECT_NATURE_POWER)
        // TODO: Nature Power
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_CHARGE)
        return AI_SCORE_SPDEF_UP(battlerAtk, 1) + AI_SCORE_CHARGE(battlerAtk);

        CASE_AND_LABEL(EFFECT_TAUNT)
        return AI_SCORE_TAUNT;

        CASE_AND_LABEL(EFFECT_HELPING_HAND)
        return AI_SCORE_HELPING_HAND;

        CASE_AND_LABEL(EFFECT_TRICK)
        return AI_SCORE_SWAP_ITEMS;

        CASE_AND_LABEL(EFFECT_ROLE_PLAY)
        // TODO: Role Play
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_WISH)
        // TODO: Wish
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_ASSIST)
        // TODO: Assist
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_INGRAIN)
        return AI_SCORE_INGRAIN;

        CASE_AND_LABEL(EFFECT_SUPERPOWER)
        AI_CALC_DAMAGE;
        score += AI_SCORE_ATTACK_UP(battlerAtk, -1);
        score += AI_SCORE_DEFENSE_UP(battlerAtk, -1);
        return score;

        CASE_AND_LABEL(EFFECT_MAGIC_COAT)
        // TODO: Magic Coat
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_RECYCLE)
        return AI_SCORE_RECYCLE;

        CASE_AND_LABEL(EFFECT_BRICK_BREAK)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_BREAK_SCREENS(battlerDef);

        CASE_AND_LABEL(EFFECT_YAWN)
        return AI_SCORE_DROWSY;

        CASE_AND_LABEL(EFFECT_KNOCK_OFF)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_LOSE_ITEM(battlerDef);

        CASE_AND_LABEL(EFFECT_ENDEAVOR)
        return AI_SCORE_ENDEAVOR;

        CASE_AND_LABEL(EFFECT_ERUPTION)
        // TODO: Eruption
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SKILL_SWAP)
        return AI_SCORE_REPLACE_ABILITY(battlerAtk, GetBattlerAbility(battlerDef)) + AI_SCORE_REPLACE_ABILITY(battlerDef, GetBattlerAbility(battlerAtk));

        CASE_AND_LABEL(EFFECT_IMPRISON)
        return AI_SCORE_IMPRISON;

        CASE_AND_LABEL(EFFECT_REFRESH)
        return AI_SCORE_CURE_STATUS_AND_HEAL(battlerAtk, 25);

        CASE_AND_LABEL(EFFECT_GRUDGE)
        // TODO: Grudge
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SNATCH)
        // TODO: Snatch
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_LOW_KICK)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SECRET_POWER)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RECOIL_33)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 33, FALSE);

        CASE_AND_LABEL(EFFECT_TEETER_DANCE)
        GOTO(EFFECT_CONFUSE);

        CASE_AND_LABEL(EFFECT_HIT_ESCAPE)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_SWITCH(battlerAtk);

        CASE_AND_LABEL(EFFECT_MUD_SPORT)
        // TODO: Mud Sport
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_POISON_FANG)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_TOXIC(battlerDef));

        CASE_AND_LABEL(EFFECT_WEATHER_BALL)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_OVERHEAT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_SPATK_UP(battlerAtk, -2);

        CASE_AND_LABEL(EFFECT_TICKLE)
        return AI_SCORE_ATTACK_UP(battlerDef, -1) + AI_SCORE_DEFENSE_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_COSMIC_POWER)
        return AI_SCORE_DEFENSE_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SKY_UPPERCUT)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_BULK_UP)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_DEFENSE_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_WATER_SPORT)
        // TODO: Water Sport
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_CALM_MIND)
        return AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_DRAGON_DANCE)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_CAMOUFLAGE)
        // TODO: Camouflage
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_PLEDGE)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_FLING)
        // TODO: Verify that fling doesn't need state set
        // TODO: Flinch canceller
        AI_CALC_DAMAGE;
        switch (gBattleMons[battlerAtk].item) {
            case ITEM_FLAME_ORB:
                score += AI_SCORE_BURN_MOVE(battlerDef);
                break;
            case ITEM_RAZOR_FANG:
            case ITEM_KINGS_ROCK:
                score += AI_SCORE_FLINCH(battlerDef);
                break;
            case ITEM_TOXIC_ORB:
                score += AI_SCORE_TOXIC(battlerDef);
                break;
            case ITEM_FROST_ORB:
                score += AI_SCORE_FROSTBITE_MOVE(battlerDef);
                break;
            case ITEM_LIGHT_BALL:
                score += AI_SCORE_PARALYSIS(battlerDef);
                break;
            case ITEM_POISON_BARB:
                score += AI_SCORE_POISON_MOVE(battlerDef);
                break;
        }
        return score + AI_SCORE_LOSE_ITEM(battlerAtk);

        CASE_AND_LABEL(EFFECT_NATURAL_GIFT)
        // TODO: Natural Gift
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_WAKE_UP_SLAP)
        AI_CALC_DAMAGE;
        if (gBattleMons[battlerDef].status1 & gBattleMoves[move].argument) score += AI_SCORE_CURE_STATUS(battlerDef);
        return score;

        CASE_AND_LABEL(EFFECT_WRING_OUT)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_HEX)
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_ASSURANCE)
        // TODO: Assurance
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_TRUMP_CARD)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_ACROBATICS)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_HEAT_CRASH)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_PUNISHMENT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_STORED_POWER)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_ELECTRO_BALL)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_GYRO_BALL)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_ECHOED_VOICE)
        // TODO: Handle multiple echoed voice
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_PAYBACK)
        // TODO: Payback
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_ROUND)
        // TODO: Handle multiple rounds
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_BRINE)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_VENOSHOCK)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_RETALIATE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_BULLDOZE)
        GOTO(EFFECT_SPEED_DOWN_HIT);

        CASE_AND_LABEL(EFFECT_FOUL_PLAY)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_PSYSHOCK)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_ROOST)
        // TODO: Roost flying removal
        return AI_SCORE_HEAL(battlerAtk, 50);

        CASE_AND_LABEL(EFFECT_GRAVITY)
        return AI_SCORE_GRAVITY;

        CASE_AND_LABEL(EFFECT_MIRACLE_EYE)
        // TODO: Miracle eye
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_TAILWIND)
        return AI_SCORE_TAILWIND;

        CASE_AND_LABEL(EFFECT_EMBARGO)
        return AI_SCORE_EMBARGO(battlerDef);

        CASE_AND_LABEL(EFFECT_AQUA_RING)
        return AI_SCORE_AQUA_RING;

        CASE_AND_LABEL(EFFECT_TRICK_ROOM)
        return AI_SCORE_TRICK_ROOM(TRICK_ROOM_DURATION);

        CASE_AND_LABEL(EFFECT_WONDER_ROOM)
        return AI_SCORE_WONDER_ROOM;

        CASE_AND_LABEL(EFFECT_MAGIC_ROOM)
        return AI_SCORE_MAGIC_ROOM;

        CASE_AND_LABEL(EFFECT_MAGNET_RISE)
        return AI_SCORE_MAGNET_RISE;

        CASE_AND_LABEL(EFFECT_TOXIC_SPIKES)
        return AI_SCORE_TOXIC_SPIKES(battlerDef);

        CASE_AND_LABEL(EFFECT_GASTRO_ACID)
        return AI_SCORE_SUPPRESS_ABILITY;

        CASE_AND_LABEL(EFFECT_STEALTH_ROCK)
        return AI_SCORE_STEALTH_ROCK(battlerDef, gBattleMoves[move].type);

        CASE_AND_LABEL(EFFECT_TELEKINESIS)
        return AI_SCORE_TELEKINESIS;

        CASE_AND_LABEL(EFFECT_POWER_SWAP)
        // TODO: Power Swap
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_GUARD_SWAP)
        // TODO: Guard Swap
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_HEART_SWAP)
        score = AI_SCORE_GET_STATS_OF(battlerAtk, battlerDef);
        score += AI_SCORE_GET_STATS_OF(battlerDef, battlerAtk);
        score += AI_SCORE_RESET_STAT_CHANGES(battlerAtk);
        score += AI_SCORE_RESET_STAT_CHANGES(battlerDef);
        return score;

        CASE_AND_LABEL(EFFECT_POWER_SPLIT)
        // TODO: Power Split
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_GUARD_SPLIT)
        // TODO: Guard Split
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_STICKY_WEB)
        return AI_SCORE_STICKY_WEB;

        CASE_AND_LABEL(EFFECT_METAL_BURST)
        return AI_SCORE_METAL_BURST;

        CASE_AND_LABEL(EFFECT_LUCKY_CHANT)
        return AI_SCORE_LUCKY_CHANT;

        CASE_AND_LABEL(EFFECT_SUCKER_PUNCH)
        // TODO: Sucker Punch legality check
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SPECIAL_DEFENSE_DOWN_HIT_2)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPDEF_UP(battlerDef, -2));

        CASE_AND_LABEL(EFFECT_SIMPLE_BEAM)
        return AI_SCORE_REPLACE_ABILITY(battlerDef, ABILITY_SIMPLE);

        CASE_AND_LABEL(EFFECT_ENTRAINMENT)
        return AI_SCORE_REPLACE_ABILITY(battlerDef, GetBattlerAbility(battlerAtk));

        CASE_AND_LABEL(EFFECT_HEAL_PULSE)
        return AI_SCORE_HEAL(battlerDef, 50);

        CASE_AND_LABEL(EFFECT_QUASH)
        return AI_SCORE_QUASH;

        CASE_AND_LABEL(EFFECT_ION_DELUGE)
        // TODO: Ion Deluge
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_FREEZE_DRY)
        GOTO(EFFECT_FROSTBITE_HIT);

        CASE_AND_LABEL(EFFECT_TOPSY_TURVY)
        return AI_SCORE_INVERT_STAT_CHANGES;

        CASE_AND_LABEL(EFFECT_MISTY_TERRAIN)
        return AI_SCORE_MISTY_TERRAIN;

        CASE_AND_LABEL(EFFECT_GRASSY_TERRAIN)
        return AI_SCORE_GRASSY_TERRAIN;

        CASE_AND_LABEL(EFFECT_ELECTRIC_TERRAIN)
        return AI_SCORE_ELECTRIC_TERRAIN;

        CASE_AND_LABEL(EFFECT_PSYCHIC_TERRAIN)
        return AI_SCORE_PSYCHIC_TERRAIN;

        CASE_AND_LABEL(EFFECT_ATTACK_ACCURACY_UP)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_ACC_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_ATTACK_SPATK_UP)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_SPATK_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_HURRICANE)
        GOTO(EFFECT_CONFUSE_HIT);

        CASE_AND_LABEL(EFFECT_TWO_TYPED_MOVE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_ME_FIRST)
        // TODO: Me First
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SPEED_UP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPEED_UP(battlerAtk, 1));

        CASE_AND_LABEL(EFFECT_QUIVER_DANCE)
        return AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_COIL)
        if ((BattlerHasAbility(battlerAtk, ABILITY_COIL_UP, FALSE) || BattlerHasAbility(battlerAtk, ABILITY_SIDEWINDER, FALSE)) &&
            !(gStatuses4[battlerAtk] & STATUS4_COILED))
            score += AI_SCORE_COILED_UP;
        return score + AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_DEFENSE_UP(battlerAtk, 1) + AI_SCORE_ACC_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_ELECTRIFY)
        // TODO: Electrify
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SCALD)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BURN_MOVE(battlerDef));

        CASE_AND_LABEL(EFFECT_REFLECT_TYPE)
        // TODO: Reflect Type
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SOAK)
        // TODO: Soak
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_GROWTH) {
            // int boost = SeesSunlight(battlerAtk, aiData) ? 2 : 1;
            return AI_SCORE_ATTACK_UP(battlerAtk, boost) + AI_SCORE_SPATK_UP(battlerAtk, boost);
        }

        CASE_AND_LABEL(EFFECT_CLOSE_COMBAT)
        AI_CALC_DAMAGE;
        return AI_SCORE_DEFENSE_UP(battlerAtk, -1) + AI_SCORE_SPDEF_UP(battlerAtk, -1);

        CASE_AND_LABEL(EFFECT_LAST_RESORT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RECOIL_33_STATUS)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 33, FALSE) + AI_SCORE_ARGUMENT_MOVE_EFFECT;

        CASE_AND_LABEL(EFFECT_FLINCH_STATUS)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_FLINCH(battlerDef) + AI_SCORE_ARGUMENT_MOVE_EFFECT);

        CASE_AND_LABEL(EFFECT_RECOIL_50)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 50, FALSE);

        CASE_AND_LABEL(EFFECT_SHELL_SMASH)
        score = AI_SCORE_ATTACK_UP(battlerAtk, 2);
        score += AI_SCORE_SPATK_UP(battlerAtk, 2);
        score += AI_SCORE_SPEED_UP(battlerAtk, 2);
        score += AI_SCORE_SPDEF_UP(battlerAtk, -1);
        score += AI_SCORE_DEFENSE_UP(battlerAtk, -1);
        return score;

        CASE_AND_LABEL(EFFECT_SHIFT_GEAR)
        return AI_SCORE_SPEED_UP(battlerAtk, 2) + AI_SCORE_ATTACK_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_DEFENSE_UP_3)
        return AI_SCORE_DEFENSE_UP(battlerAtk, 3);

        CASE_AND_LABEL(EFFECT_NOBLE_ROAR)
        return AI_SCORE_ATTACK_UP(battlerDef, -1) + AI_SCORE_DEFENSE_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_VENOM_DRENCH)
        if (!(gBattleMons[battlerDef].status1 & STATUS1_PSN_ANY)) return AI_SCORE_IMMUNE;
        return AI_SCORE_ATTACK_UP(battlerDef, -1) + AI_SCORE_DEFENSE_UP(battlerDef, -1) + AI_SCORE_SPEED_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_TOXIC_THREAD)
        return AI_SCORE_SPEED_UP(battlerDef, -3) + AI_SCORE_TOXIC(battlerDef);

        CASE_AND_LABEL(EFFECT_CLEAR_SMOG)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RESET_STAT_CHANGES(battlerDef);

        CASE_AND_LABEL(EFFECT_HIT_SWITCH_TARGET)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RANDOM_SWITCH(battlerDef);

        CASE_AND_LABEL(EFFECT_FINAL_GAMBIT)
        // TODO: Final Gambit
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_CHANGE_TYPE_ON_ITEM)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_AUTOTOMIZE)
        GOTO(EFFECT_SPEED_UP_2);

        CASE_AND_LABEL(EFFECT_COPYCAT)
        // TODO: Copycat
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_DEFOG)
        // TODO: Defog
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_HIT_ENEMY_HEAL_ALLY)
        if (AreSameSide(battlerAtk, battlerDef)) return AI_SCORE_HEAL(battlerDef, 50);
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SMACK_DOWN)
        // TODO: Smack Down
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SYNCHRONOISE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_PSYCHO_SHIFT) {
            int status = gBattleMons[battlerAtk].status1;
            if (status & STATUS1_SLEEP)
                score = AI_SCORE_SLEEP_MOVE(battlerDef);
            else if (status & STATUS1_BLEED)
                score = AI_SCORE_BLEED(battlerDef);
            else if (status & STATUS1_BURN)
                score = AI_SCORE_BURN_MOVE(battlerDef);
            else if (status & STATUS1_FROSTBITE)
                score = AI_SCORE_FROSTBITE_MOVE(battlerDef);
            else if (status & STATUS1_PARALYSIS)
                score = AI_SCORE_PARALYSIS(battlerDef);
            else if (status & STATUS1_TOXIC_POISON)
                score = AI_SCORE_TOXIC(battlerDef);
            else if (status & STATUS1_POISON)
                score = AI_SCORE_POISON_MOVE(battlerDef);
            else
                return AI_SCORE_IMMUNE;
        }
        // TODO: Make sure this is the right number
        if (score > AI_SCORE_IMMUNE) return score + AI_SCORE_CURE_STATUS(battlerAtk);
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_POWER_TRICK)
        // TODO: Power Trick
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_FLAME_BURST)
        // TODO: Flame Burst
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_AFTER_YOU)
        // TODO: After You
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_BESTOW)
        if (gBattleMons[battlerDef].item) return AI_SCORE_IMMUNE;
        return AI_SCORE_SWAP_ITEMS;

        CASE_AND_LABEL(EFFECT_ROTOTILLER)
        if (!IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS)) return AI_SCORE_IMMUNE;
        {
            // int boost = IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN) ? 2 : 1;
            return AI_SCORE_SPATK_UP(battlerAtk, boost);
        }

        CASE_AND_LABEL(EFFECT_FLOWER_SHIELD)
        if (!IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS)) return AI_SCORE_IMMUNE;
        {
            return AI_SCORE_DEFENSE_UP(battlerAtk, 1);
        }

        CASE_AND_LABEL(EFFECT_HIT_PREVENT_ESCAPE)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_TRAP(battlerDef);

        CASE_AND_LABEL(EFFECT_SPEED_SWAP)
        // TODO: Speed Swap
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_DEFENSE_UP2_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_DEFENSE_UP(battlerAtk, 2));

        CASE_AND_LABEL(EFFECT_REVELATION_DANCE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_AURORA_VEIL)
        if (!IsWeatherActive(WEATHER_HAIL_ANY)) return AI_SCORE_IMMUNE;
        return AI_SCORE_AURORA_VEIL(battlerAtk, SCREEN_DURATION_SHORT);

        CASE_AND_LABEL(EFFECT_THIRD_TYPE)
        // TODO: Trick or Treat fog
        return AI_SCORE_ADD_TYPE(battlerDef, gBattleMoves[move].argument);

        CASE_AND_LABEL(EFFECT_FEINT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_BREAK_PROTECT;

        CASE_AND_LABEL(EFFECT_SPARKLING_ARIA)
        if (!AreSameSide(battlerAtk, battlerDef)) {
            AI_CALC_DAMAGE;
        }
        if (gBattleMons[battlerDef].status1 & STATUS1_BURN) score += AI_SCORE_CURE_STATUS(battlerDef);
        return score;

        CASE_AND_LABEL(EFFECT_ACUPRESSURE)
        score = AI_SCORE_ATTACK_UP(battlerDef, 2);
        score += AI_SCORE_DEFENSE_UP(battlerDef, 2);
        score += AI_SCORE_SPEED_UP(battlerDef, 2);
        score += AI_SCORE_SPATK_UP(battlerDef, 2);
        score += AI_SCORE_SPDEF_UP(battlerDef, 2);
        return score / 5;

        CASE_AND_LABEL(EFFECT_AROMATIC_MIST)
        return AI_SCORE_DEFENSE_UP(battlerAtk, 2) + AI_SCORE_DEFENSE_UP(BATTLE_PARTNER(battlerAtk), 2);

        CASE_AND_LABEL(EFFECT_POWDER)
        return AI_SCORE_POWDER;

        CASE_AND_LABEL(EFFECT_SP_ATTACK_UP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPATK_UP(battlerAtk, 2));

        CASE_AND_LABEL(EFFECT_BELCH)
        // TODO: Check berry
        GOTO(EFFECT_BERRY_SMASH);

        CASE_AND_LABEL(EFFECT_PARTING_SHOT)
        return AI_SCORE_SWITCH(battlerAtk) + AI_SCORE_ATTACK_UP(battlerDef, -1) + AI_SCORE_SPATK_UP(battlerDef, -1);

        CASE_AND_LABEL(EFFECT_SPECTRAL_THIEF)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_GET_STATS_OF(battlerAtk, battlerDef) + AI_SCORE_RESET_STAT_CHANGES(battlerDef);

        CASE_AND_LABEL(EFFECT_V_CREATE)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_DEFENSE_UP(battlerAtk, -1) + AI_SCORE_SPDEF_UP(battlerAtk, -1) + AI_SCORE_SPEED_UP(battlerAtk, -1);

        CASE_AND_LABEL(EFFECT_MAT_BLOCK)
        GOTO(EFFECT_PROTECT);

        CASE_AND_LABEL(EFFECT_STOMPING_TANTRUM)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_CORE_ENFORCER)
        // TODO: Core Enforcer
        AI_CALC_DAMAGE;
        return score;

        CASE_AND_LABEL(EFFECT_INSTRUCT)
        // TODO: Instruct
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_THROAT_CHOP)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_THROAT_CHOP;

        CASE_AND_LABEL(EFFECT_LASER_FOCUS)
        // TODO: Laser Focus
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_MAGNETIC_FLUX)
        // if (IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) score += AI_SCORE_PARALYSIS(battlerDef);
        if (BattlerHasAbility(battlerAtk, ABILITY_PLUS, FALSE) || BattlerHasAbility(battlerAtk, ABILITY_MINUS, FALSE))
            score += AI_SCORE_DEFENSE_UP(battlerDef, 1) + AI_SCORE_SPDEF_UP(battlerDef, 1);
        return score;

        CASE_AND_LABEL(EFFECT_GEAR_UP)
        return AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_INCINERATE)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_BUG_BITE)
        AI_CALC_DAMAGE;
        if (ItemId_GetPocket(gBattleMons[battlerDef].item) == POCKET_BERRIES)
            score += AI_SCORE_EAT_BERRY(battlerDef);
        else
            score += AI_SCORE_LOSE_ITEM(battlerDef);
        return score;

        CASE_AND_LABEL(EFFECT_STRENGTH_SAP)
        return AI_SCORE_ATTACK_UP(battlerDef, -1) +
               AI_SCORE_HEAL_FIXED(CalculateStat(battlerAtk, STAT_ATK, 0, 0, TRUE, FALSE, IsUnaware(gBattlerAttacker), FALSE););

        CASE_AND_LABEL(EFFECT_MIND_BLOWN)
        // TODO: Score damage loss
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_PURIFY)
        if (!(gBattleMons[battlerDef].status1 & STATUS1_ANY)) return AI_SCORE_IMMUNE;
        return AI_SCORE_HEAL(battlerAtk, 50) + AI_SCORE_CURE_STATUS(battlerDef);

        CASE_AND_LABEL(EFFECT_BURN_UP)
        // TODO: Score losing type
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SHORE_UP)
        if (IsBattlerWeatherAffected(battlerAtk, WEATHER_SANDSTORM_ANY)) return AI_SCORE_HEAL(battlerAtk, 66);
        return AI_SCORE_HEAL(battlerAtk, 50);

        CASE_AND_LABEL(EFFECT_GEOMANCY)
        if (turn == 0) return AI_SCORE_IMMUNE;
        return AI_SCORE_SPATK_UP(battlerAtk, 2) + AI_SCORE_SPATK_UP(battlerAtk, 2) + AI_SCORE_SPEED_UP(battlerAtk, 2);

        CASE_AND_LABEL(EFFECT_FAIRY_LOCK)
        // TODO: Fairy Lock
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_ALLY_SWITCH)
        // TODO: Ally Switch
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_RELIC_SONG)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_ATTACKER_DEFENSE_DOWN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_DEFENSE_UP(battlerAtk, -1);

        CASE_AND_LABEL(EFFECT_BODY_PRESS)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_EERIE_SPELL)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_PP_DOWN(battlerDef, 6);

        CASE_AND_LABEL(EFFECT_JUNGLE_HEALING)
        return AI_SCORE_CURE_STATUS_AND_HEAL(battlerAtk, 25) + AI_SCORE_CURE_STATUS_AND_HEAL(BATTLE_PARTNER(battlerAtk), 25);

        CASE_AND_LABEL(EFFECT_COACHING)
        return AI_SCORE_DEFENSE_UP(battlerDef, 1) + AI_SCORE_ATTACK_UP(battlerDef, 1);

        CASE_AND_LABEL(EFFECT_LASH_OUT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_GRASSY_GLIDE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_REMOVE_TERRAIN)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_DYNAMAX_DOUBLE_DMG)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_DECORATE)
        if (!AreSameSide(battlerAtk, battlerDef)) GOTO(EFFECT_HIT);
        return AI_SCORE_ATTACK_UP(battlerDef, 2) + AI_SCORE_SPATK_UP(battlerDef, 2) + AI_SCORE_CRIT_UP(battlerDef, 2);

        CASE_AND_LABEL(EFFECT_SNIPE_SHOT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_TRIPLE_HIT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RECOIL_HP_25)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_LOSE_HP(battlerDef, 25);

        CASE_AND_LABEL(EFFECT_STUFF_CHEEKS)
        return AI_SCORE_DEFENSE_UP(battlerAtk, 2) + AI_SCORE_EAT_BERRY(battlerAtk);

        CASE_AND_LABEL(EFFECT_GRAV_APPLE)
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_EVASION_UP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_EVASION_UP(battlerAtk, 1));

        CASE_AND_LABEL(EFFECT_GLITZY_GLOW)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_LIGHTSCREEN;

        CASE_AND_LABEL(EFFECT_BADDY_BAD)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_REFLECT;

        CASE_AND_LABEL(EFFECT_SAPPY_SEED)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_LEECH_SEED;

        CASE_AND_LABEL(EFFECT_FREEZY_FROST)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_HAZE;

        CASE_AND_LABEL(EFFECT_SPARKLY_SWIRL)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_CURE_PARTY_STATUS(battlerAtk);

        CASE_AND_LABEL(EFFECT_PLASMA_FISTS)
        // TODO: Plasma Fists
        AI_CALC_DAMAGE;
        return score;

        CASE_AND_LABEL(EFFECT_HYPERSPACE_FURY)
        GOTO(EFFECT_FEINT);

        CASE_AND_LABEL(EFFECT_AURA_WHEEL)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_PHOTON_GEYSER)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_SHELL_SIDE_ARM)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_TERRAIN_PULSE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_JAW_LOCK)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_TRAP(battlerDef) + AI_SCORE_NO_ESCAPE(battlerAtk);

        CASE_AND_LABEL(EFFECT_NO_RETREAT)
        // TODO: Check that you can actually use no retreat
        score = AI_SCORE_NO_ESCAPE(battlerAtk);
        score += AI_SCORE_ATTACK_UP(battlerAtk, 1);
        score += AI_SCORE_DEFENSE_UP(battlerAtk, 1);
        score += AI_SCORE_SPEED_UP(battlerAtk, 1);
        score += AI_SCORE_SPATK_UP(battlerAtk, 1);
        score += AI_SCORE_SPDEF_UP(battlerAtk, 1);
        return score;

        CASE_AND_LABEL(EFFECT_TAR_SHOT)
        // TODO: Tar Shot
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_POLTERGEIST)
        if (!gBattleMons[battlerDef].item) return AI_SCORE_IMMUNE;
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_OCTOLOCK)
        // TODO: Octolock
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_CLANGOROUS_SOUL)
        score = AI_SCORE_LOSE_HP(battlerDef, 33);
        score += AI_SCORE_ATTACK_UP(battlerAtk, 1);
        score += AI_SCORE_DEFENSE_UP(battlerAtk, 1);
        score += AI_SCORE_SPEED_UP(battlerAtk, 1);
        score += AI_SCORE_SPATK_UP(battlerAtk, 1);
        score += AI_SCORE_SPDEF_UP(battlerAtk, 1);
        return score;

        CASE_AND_LABEL(EFFECT_BOLT_BEAK)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_RISING_VOLTAGE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_BEAK_BLAST)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_EXCALIBUR)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_EXPANDING_FORCE)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_STEEL_BEAM)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_LOSE_HP(battlerDef, 50);

        CASE_AND_LABEL(EFFECT_HOWL)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_ATTACK_UP(BATTLE_PARTNER(battlerAtk), 1);

        CASE_AND_LABEL(EFFECT_ATTRACT_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ATTRACT(battlerAtk, battlerDef));

        CASE_AND_LABEL(EFFECT_CURSE_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_CURSE(battlerDef));

        CASE_AND_LABEL(EFFECT_FLINCH_RECOIL_33)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 33, FALSE) + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_FLINCH(battlerDef));

        CASE_AND_LABEL(EFFECT_FLINCH_RECOIL_50)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 50, FALSE) + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_FLINCH(battlerDef));

        CASE_AND_LABEL(EFFECT_IGNORE_TYPE_IMMUNITY)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SE_AGAINST_TYPE_HIT)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_DOUBLE_DMG_IF_STATUS1)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_STEALTH_ROCK_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_STEALTH_ROCK(battlerDef, TYPE_ROCK));

        CASE_AND_LABEL(EFFECT_LEECH_SEED_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_LEECH_SEED);

        CASE_AND_LABEL(EFFECT_STICKY_WEB_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_STICKY_WEB);

        CASE_AND_LABEL(EFFECT_BLEED_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BLEED(battlerDef));

        CASE_AND_LABEL(EFFECT_SLEEP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SLEEP_MOVE(battlerDef));

        CASE_AND_LABEL(EFFECT_BLEED)
        return AI_SCORE_BLEED(battlerDef);

        CASE_AND_LABEL(EFFECT_FROSTBITE)
        return AI_SCORE_FROSTBITE_MOVE(battlerDef);

        CASE_AND_LABEL(EFFECT_FREEZE)
        return AI_SCORE_FROSTBITE_MOVE(battlerDef);

        CASE_AND_LABEL(EFFECT_INFERNAL_PARADE)
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_MISTY_TERRAIN_BOOST)
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_BERRY_SMASH)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_EAT_BERRY(battlerAtk);

        CASE_AND_LABEL(EFFECT_INVERSE_ROOM)
        return AI_SCORE_INVERSE_ROOM(INVERSE_ROOM_DURATION);

        CASE_AND_LABEL(EFFECT_DRAIN_BRAIN)
        return AI_SCORE_SPATK_UP(battlerDef, -1) +
               AI_SCORE_HEAL_FIXED(CalculateStat(battlerAtk, STAT_SPATK, 0, 0, TRUE, FALSE, IsUnaware(gBattlerAttacker), FALSE););

        CASE_AND_LABEL(EFFECT_TRIPLE_ARROWS)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_DEFENSE_UP(battlerDef, -1)) +
               AI_SCORE_ADJUST(GetFullChance(battlerAtk, move, moveEffect, 30, aiData), AI_SCORE_FLINCH(battlerDef));

        CASE_AND_LABEL(EFFECT_RECOIL_25_STATUS)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_RECOIL(battlerAtk, 25, FALSE) + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ARGUMENT_MOVE_EFFECT);

        CASE_AND_LABEL(EFFECT_WEATHER_BOOST)
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_MORTAL_SPIN)
        // TODO: Add hazard clearing
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_KARMA)
        return AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, -1);

        CASE_AND_LABEL(EFFECT_REMOVE_TERRAIN_NO_FAIL)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_REMOVE_TERRAIN + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ARGUMENT_MOVE_EFFECT);

        CASE_AND_LABEL(EFFECT_TEN_HITS)
        // TODO: Special handling
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_TWO_TURN_SECONDARY)
        if (turn == 0) return AI_SCORE_ARGUMENT_MOVE_EFFECT;
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_SPECIAL_ATTACK_UP_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPATK_UP(battlerAtk, 1));

        CASE_AND_LABEL(EFFECT_ARGUMENT_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ARGUMENT_MOVE_EFFECT);

        CASE_AND_LABEL(EFFECT_EVERY_OTHER_TURN)
        // TODO: Every other turn scoring
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_MISC_HIT)
        switch (gBattleMoves[move].argument) {
            case MISC_EFFECT_TRANSMUTE:
                // TODO: Handle item recovery
                GOTO(EFFECT_HIT);

            case MISC_EFFECT_DOUBLE_DAMAGE:
                // TODO: Handle 30% chance
                GOTO(EFFECT_HIT);
        }
        GOTO(EFFECT_HIT);

        CASE_AND_LABEL(EFFECT_FILLET_AWAY)
        score = AI_SCORE_LOSE_HP(battlerDef, 50);
        score += AI_SCORE_ATTACK_UP(battlerAtk, 2);
        score += AI_SCORE_SPEED_UP(battlerAtk, 2);
        score += AI_SCORE_SPATK_UP(battlerAtk, 2);
        return score;

        CASE_AND_LABEL(EFFECT_COURT_CHANGE)
        // TODO: Court Change
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_CHILLY_RECEPTION)
        return AI_SCORE_HAIL + AI_SCORE_SWITCH(battlerAtk);

        CASE_AND_LABEL(EFFECT_SHED_TAIL)
        return AI_SCORE_SUBSTITUTE + AI_SCORE_LOSE_HP(battlerDef, 50) + AI_SCORE_SWITCH(battlerAtk);

        CASE_AND_LABEL(EFFECT_GHASTLY_ECHO)
        AI_CALC_DAMAGE;
        // TODO: Make sure this makes sense
        return score + AI_SCORE_SWITCH(battlerAtk) + AI_SCORE_GHASTLY_ECHO;

        CASE_AND_LABEL(EFFECT_REVIVAL_BLESSING)
        return AI_SCORE_REVIVE(battlerAtk, 50);

        CASE_AND_LABEL(EFFECT_TIDY_UP)
        return AI_SCORE_CLEAR_HAZARDS(GetBattlerSide(battlerAtk)) + AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_ACID)
        GOTO(EFFECT_SPECIAL_DEFENSE_DOWN_HIT);

        CASE_AND_LABEL(EFFECT_SLUDGE)
        GOTO(EFFECT_POISON_HIT);

        CASE_AND_LABEL(EFFECT_POISON_GAS)
        GOTO(EFFECT_POISON_HIT);

        CASE_AND_LABEL(EFFECT_PARALYZE_IGNORE_TYPE)
        return AI_SCORE_PARALYSIS_IGNORE_TYPE;

        CASE_AND_LABEL(EFFECT_SUPER_FANG_HAZE)
        AI_CALC_DAMAGE;
        // TODO: Move haze out of this layer.
        return score + AI_SCORE_RESET_STAT_CHANGES(battlerDef);

        CASE_AND_LABEL(EFFECT_SPICY_EXTRACT)
        return AI_SCORE_CONFUSION(battlerDef) + AI_SCORE_ATTACK_UP(battlerAtk, 2) + AI_SCORE_ATTACK_UP(battlerDef, 2);

        CASE_AND_LABEL(EFFECT_CLEAR_WEATHER_AND_TERRAIN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_REMOVE_TERRAIN + AI_SCORE_REMOVE_WEATHER;

        CASE_AND_LABEL(EFFECT_MATCHA_GOTCHA)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ABSORB_MOVE(50) + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BURN_MOVE(battlerDef));

        CASE_AND_LABEL(EFFECT_DOODLE)
        return AI_SCORE_REPLACE_ABILITY(battlerAtk, GetBattlerAbility(battlerDef)) +
               AI_SCORE_REPLACE_ABILITY(BATTLE_PARTNER(battlerAtk), GetBattlerAbility(battlerDef));

        CASE_AND_LABEL(EFFECT_SPIKE_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SPIKES(battlerDef));

        CASE_AND_LABEL(EFFECT_VICTORY_DANCE)
        return AI_SCORE_SPEED_UP(battlerAtk, 1) + AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_DEFENSE_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_DRAGON_CHEER)
        return AI_SCORE_CRIT_UP(battlerAtk, 1 + IS_BATTLER_OF_TYPE(battlerAtk, TYPE_DRAGON)) +
               AI_SCORE_CRIT_UP(BATTLE_PARTNER(battlerAtk), 1 + IS_BATTLER_OF_TYPE(BATTLE_PARTNER(battlerAtk), TYPE_DRAGON));

        CASE_AND_LABEL(EFFECT_SHELTER)
        return AI_SCORE_DEFENSE_UP(battlerAtk, 2) + AI_SCORE_DEFENSE_UP(BATTLE_PARTNER(battlerAtk), 2);

        CASE_AND_LABEL(EFFECT_ARGUMENT_HIT_IF_STAT_UP)
        // TODO: EFFECT_ARGUMENT_HIT_IF_STAT_UP
        GOTO(EFFECT_ARGUMENT_HIT);

        CASE_AND_LABEL(EFFECT_UPPER_HAND)
        // TODO: Upper Hand
        return AI_SCORE_IMMUNE;

        CASE_AND_LABEL(EFFECT_ELECTRO_SHOT)
        // TODO: Electro Shot
        GOTO(EFFECT_TWO_TURN_SECONDARY);

        CASE_AND_LABEL(EFFECT_SHARPEN)
        // TODO: Sharpen
        return AI_SCORE_CRIT_UP(battlerAtk, 1) + AI_SCORE_STAT(battlerAtk, GetHighestAttackingStatId(battlerAtk, TRUE), 1);

        CASE_AND_LABEL(EFFECT_SCARY_FACE)
        return AI_SCORE_SPEED_UP(battlerDef, -2) + AI_SCORE_FEAR(battlerDef);

        CASE_AND_LABEL(EFFECT_SMOKESCREEN)
        return AI_SCORE_SMOKESCREEN;

        CASE_AND_LABEL(EFFECT_EERIE_FOG)
        return AI_SCORE_FOG;

        CASE_AND_LABEL(EFFECT_MYSTIC_DANCE)
        return AI_SCORE_SPEED_UP(battlerAtk, 1) + AI_SCORE_SPATK_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_SNAP_JAW)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ADJUST(50, AI_SCORE_SPEED_UP(battlerAtk, 1) + AI_SCORE_SPEED_UP(battlerDef, -1)));

        CASE_AND_LABEL(EFFECT_RIP_AND_TEAR)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BLEED(battlerDef) + AI_SCORE_SPEED_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_SKY_DROP)
        // TODO: Sky Drop
        AI_CALC_DAMAGE;
        return score;

        CASE_AND_LABEL(EFFECT_TERROR_CHARGE)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_BLEED(battlerDef) + AI_SCORE_FEAR(battlerDef));

        CASE_AND_LABEL(EFFECT_TAILWIND_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_TAILWIND);

        CASE_AND_LABEL(EFFECT_SANDSTORM_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_SANDSTORM);

        CASE_AND_LABEL(EFFECT_RAIN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_RAIN);

        CASE_AND_LABEL(EFFECT_FAIRY_TERRAIN_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_MISTY_TERRAIN);

        CASE_AND_LABEL(EFFECT_CREEPING_THORNS_HIT)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_STEALTH_ROCK(battlerDef, TYPE_GRASS));

        CASE_AND_LABEL(EFFECT_TAKE_HEART)
        return AI_SCORE_CURE_STATUS(battlerAtk) + AI_SCORE_SPATK_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_CLEAR_SKIES)
        return AI_SCORE_CLEAR_SKIES;

        CASE_AND_LABEL(EFFECT_SHOWTIME)
        if (gFieldStatuses & STATUS_FIELD_TRICK_ROOM) score += AI_SCORE_TRICK_ROOM(TRICK_ROOM_DURATION);
        if (gFieldStatuses & STATUS_FIELD_WONDER_ROOM) score += AI_SCORE_WONDER_ROOM;
        if (gFieldStatuses & STATUS_FIELD_INVERSE_ROOM) score += AI_SCORE_INVERSE_ROOM(INVERSE_ROOM_DURATION);
        if (!(gFieldStatuses & STATUS_FIELD_MAGIC_ROOM)) score += AI_SCORE_MAGIC_ROOM;
        return score + AI_SCORE_SWITCH(battlerAtk);

        CASE_AND_LABEL(EFFECT_TREPIDATION)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_TREPIDATION;

        CASE_AND_LABEL(EFFECT_CHIP_AWAY)
        AI_CALC_DAMAGE;
        return score + AI_SCORE_ADJUST(AI_GET_MOVE_EFFECT_CHANCE, AI_SCORE_ATTACK_UP(battlerDef, -1) + AI_SCORE_DEFENSE_UP(battlerDef, -1));

        CASE_AND_LABEL(EFFECT_MEDITATE)
        return AI_SCORE_ATTACK_UP(battlerAtk, 1) + AI_SCORE_SPDEF_UP(battlerAtk, 1);

        CASE_AND_LABEL(EFFECT_BARRIER)
        if (!IsTerrainActive(STATUS_FIELD_PSYCHIC_TERRAIN)) return AI_SCORE_IMMUNE;
        return AI_SCORE_LIGHTSCREEN + AI_SCORE_REFLECT;

        CASE_AND_LABEL(EFFECT_KINESIS)
        if (!gBattleMons[battlerDef].item) return AI_SCORE_IMMUNE;
        return AI_SCORE_FLINCH(battlerDef) + AI_SCORE_LOSE_ITEM(battlerDef);

        CASE_AND_LABEL(EFFECT_CORROSIVE_GAS)
        GOTO(EFFECT_KNOCK_OFF);

        CASE_AND_LABEL(EFFECT_CALTROPS)
        return AI_SCORE_CALTROPS;

        CASE_AND_LABEL(EFFECT_SWEET_KISS)
        return AI_SCORE_CONFUSION(battlerDef) + AI_SCORE_ATTRACT(battlerAtk, battlerDef);

        CASE_AND_LABEL(EFFECT_QUICK_GUARD)
        return AI_SCORE_QUICK_GUARD;

        CASE_AND_LABEL(EFFECT_TWO_TURN_RETALIATION)
        if (turn == 0) return AI_SCORE_CONTACT(AI_SCORE_ARGUMENT_MOVE_EFFECT);
        GOTO(EFFECT_ARGUMENT_HIT);
    }

    return AI_SCORE_IMMUNE;
}

int CheckImmunities(int battlerAtk, int battlerDef, int move, int moveType, int effectiveness, int* score, struct MoveState* moveState) {
    int statId, absorbing;
    AbilityEnum ability;
    if (effectiveness == -1) {
        effectiveness = CalcTypeEffectivenessMultiplier(move, moveType, battlerAtk, battlerDef, FALSE);
    }
    if (!effectiveness) {
        moveState->cancelled = TRUE;
        *score = 0;
        return TRUE;
    }
    if (TestImmunityAbilitiesOnly(battlerDef, battlerAtk, move, moveType)) {
        moveState->cancelled = TRUE;
        *score = 0;
        return TRUE;
    }
    if ((absorbing = TestAbsorbingAbilities(battlerDef, move, moveType, &ability, &statId))) {
        moveState->cancelled = TRUE;
        switch (absorbing) {
            case 1:
                *score = AI_SCORE_HEAL(battlerDef, 25);
                return TRUE;

            case 2:
                *score = AI_SCORE_STAT(battlerDef, statId, 1);
                return TRUE;

            case 3:
                *score = AI_SCORE_FLASH_FIRE;
                return TRUE;
        }
    }
    if (CheckPowder(battlerAtk, move)) {
        moveState->missesThisTurn = AI_MISSES_THIS_TURN;
        *score = AI_SCORE_LOSE_HP(battlerAtk, 25);
        return FALSE;
    }
    return FALSE;
}

const u16 sCritChance[] = {UQ_4_12(1.0 / 24), UQ_4_12(1.0 / 8), UQ_4_12(1.0 / 2), UQ_4_12(1)};

void CheckSingleHitKo(int baseDamage, int defenderHp, int multiplier, int procChance, int flatDamage, int setCrit, struct MoveState* moveState) {
    if (multiplier) ApplyModifier(multiplier, baseDamage);
    if (baseDamage + flatDamage > defenderHp) {
        int minDamage = moveState->noVariance ? baseDamage : ApplyModifier(UQ_4_12(.85), baseDamage);
        int koChance;
        if (minDamage + flatDamage >= defenderHp) {
            if (!setCrit && !moveState->overkillInHalves) moveState->overkillInHalves = minDamage * 2 / defenderHp - 2;
            koChance = 100;
        } else {
            // Clamp to 1/16 steps
            koChance = (16 * (defenderHp - minDamage) / (baseDamage - minDamage) + 1) * (100 / 16);
        }
        if (procChance) {
            koChance = ApplyModifier(procChance, koChance);
            moveState->koChance -= ApplyModifier(procChance, moveState->koChance);
        }
        moveState->koChance = 100 - (100 - moveState->koChance) * (100 - koChance) / 100;
        moveState->koChance |= setCrit;
    }
}

int CalculateKoChanceFine(
    int baseDamage, int bonusDamage, int critMultiplier, int doubleDamageChance, int battlerDef, int defenderHp, struct MoveState* moveState) {
    if (moveState->critChance <= 3 && baseDamage > defenderHp) {
        CheckSingleHitKo(baseDamage, defenderHp, 0, 0, bonusDamage, FALSE, moveState);
        if (moveState->koChance && !moveState->critChance) {
            int minDamage = moveState->noVariance ? baseDamage : ApplyModifier(UQ_4_12(.85), baseDamage);
            int overkillInHalves = minDamage * 2 / defenderHp - 2;
            moveState->overkillInHalves = min(overkillInHalves, 3);
            return AI_SCORE_DAMAGE(moveState->damage, battlerDef);
        }
    }
    if (moveState->critChance <= 3 && doubleDamageChance) {
        CheckSingleHitKo(baseDamage, defenderHp, UQ_4_12(2), doubleDamageChance, bonusDamage, FALSE, moveState);
    }
    if (moveState->critChance > 0) {
        CheckSingleHitKo(baseDamage, defenderHp, critMultiplier, sCritChance[moveState->critChance - 1], bonusDamage, !moveState->koChance, moveState);
        if (moveState->koChance >= 100) return AI_SCORE_DAMAGE(moveState->damage, battlerDef);
    }
    if (doubleDamageChance && moveState->critChance > 0) {
        CheckSingleHitKo(baseDamage,
                         defenderHp,
                         ApplyModifier(critMultiplier, UQ_4_12(2)),
                         ApplyModifier(sCritChance[moveState->critChance - 1], doubleDamageChance),
                         bonusDamage,
                         !moveState->koChance,
                         moveState);
    }
    return AI_SCORE_DAMAGE(moveState->damage, battlerDef);
}

int ScoreMoveDamage(int battlerAtk,
                    int battlerDef,
                    int move,
                    AiProcessingPhase phase,
                    struct MoveState* moveState,
                    struct MoveContainer* moveContainer,
                    struct AiData* aiData) {
    u16 parentalBondSpread[6];
    int hitCount = 1;
    int i;
    int damageShield = phase == AI_PHASE_DAMAGE_ROUGH ? aiData->battlerState[battlerDef].shield : 0;
    int scoreOther = 0;
    u8 moveType;
    int baseDamage = 0;
    int baseDamageAverage = 0;
    int defenderHp = gBattleMons[battlerDef].hp;
    u16 critMultiplier = UQ_4_12(1.5);
    int doubleDamageChance = 0;
    int isTripleKick;
    int requiredHits;

    GET_MOVE_TYPE(move, moveType)

    if (IS_MOVE_STATUS(move)) {
        CheckImmunities(battlerAtk, battlerDef, move, moveType, -2 + (gBattleMoves[move].effect == EFFECT_PARALYZE), &scoreOther, moveState);
        return scoreOther;
    }

    if (moveContainer->multihitType >= PARENTAL_BOND_START) {
        hitCount = GetParentalBondCount(battlerAtk, moveContainer->multihitType);
        if (hitCount == 1)
            moveContainer->multihitType = 0;
        else {
            for (i = 0; i < hitCount; i++) {
                parentalBondSpread[i] = GetParentalBondMultiplier(moveContainer->multihitType, i);
            }
        }
    }

    switch (gBattleMoves[move].effect) {
        case EFFECT_SUPER_FANG:
        case EFFECT_SUPER_FANG_HAZE:
            if (CheckImmunities(battlerAtk, battlerDef, move, moveType, -1, &scoreOther, moveState)) return scoreOther;
            moveContainer->fixedDamage = TRUE;
            for (i = 0; i < hitCount; i++) {
                if (damageShield) {
                    int damage = ApplyModifier(parentalBondSpread[i], defenderHp / 2);
                    moveState->damage += damage;
                    moveState->negatedDamage += damage;
                    if (moveState->damage >= damageShield) {
                        damageShield = 0;
                        moveState->breakShield = TRUE;
                    }
                } else {
                    baseDamage += ApplyModifier(parentalBondSpread[i], (defenderHp - baseDamage) / 2);
                }
            }
            moveState->damage += baseDamage;
            moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(hitCount));
            if (moveState->missesThisTurn) return scoreOther;
            return AI_SCORE_DAMAGE(moveState->damage, battlerDef);

        case EFFECT_LEVEL_DAMAGE:
            if (CheckImmunities(battlerAtk, battlerDef, move, moveType, -1, &scoreOther, moveState)) return scoreOther;
            moveContainer->fixedDamage = TRUE;
            baseDamage = gBattleMons[gBattlerAttacker].level;
            moveState->noVariance = TRUE;
            break;

        default:
            if (move == MOVE_SEISMIC_TOSS) {
                if (CheckImmunities(battlerAtk, battlerDef, move, moveType, -1, &scoreOther, moveState)) return scoreOther;
                moveContainer->fixedDamage = TRUE;
                baseDamage = gBattleMons[gBattlerAttacker].level;
                moveState->noVariance = TRUE;
                break;
            }
            baseDamage = CalcMoveDamageAi(move, battlerAtk, battlerDef, &moveType, moveContainer->fixedDamage, moveState);
            if (CheckImmunities(battlerAtk, battlerDef, move, moveType, -1, &scoreOther, moveState)) return scoreOther;
            break;
    }

    baseDamageAverage = baseDamage;

    if (!moveContainer->fixedDamage && moveState->critChance) {
        baseDamageAverage += ApplyModifier(ApplyModifier((critMultiplier - UQ_4_12(1.0)), sCritChance[moveState->critChance - 1]), baseDamageAverage);
    }

    if (!moveState->noVariance) {
        baseDamageAverage = ApplyModifier(UQ_4_12((1.0 + .85) / 2), baseDamageAverage);
    }

    if (gBattleMoves[move].effect == EFFECT_MISC_HIT && gBattleMoves[move].argument == MISC_EFFECT_DOUBLE_DAMAGE) {
        doubleDamageChance = UQ_4_12_PERCENT(gBattleMoves[move].secondaryEffectChance);
        baseDamageAverage += ApplyModifier(doubleDamageChance, baseDamageAverage);
    }

    if (moveContainer->multihitType >= PARENTAL_BOND_START) {
        int multi = parentalBondSpread[0];
        for (i = 1; i < hitCount; i++) {
            multi += parentalBondSpread[i];
        }
        moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(hitCount));
        moveState->damage = ApplyModifier(baseDamage, multi);
    } else {
        switch (moveContainer->multihitType) {
            case MULTIHIT_BEAT_UP:
                // TODO: Beat up
            case MULTIHIT_FIVE:
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(5));
                moveState->damage = baseDamageAverage * 5;
                hitCount = 5;
                break;

            case MULTIHIT_FOUR_OR_FIVE:
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(4.5));
                moveState->damage = baseDamageAverage * 9 / 5;
                hitCount = 5;
                break;

            case MULTIHIT_SINGLE:
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(1));
                moveState->damage = baseDamageAverage;
                hitCount = 1;
                break;

            case MULTIHIT_TEN:
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(10));
                moveState->damage = baseDamageAverage * 10;
                hitCount = 10;
                break;

            case MULTIHIT_TEN_CAN_MISS:
                if (moveState->accuracy >= 100) {
                    moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(10));
                    moveState->damage = baseDamageAverage * 10;
                } else {
                    moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(gTenHitsMultiplier[moveState->accuracy - 1]));
                    moveState->damage = ApplyModifier(gTenHitsMultiplier[moveState->accuracy - 1], baseDamageAverage);
                }
                hitCount = 10;
                break;

            case MULTIHIT_THREE:
                if (gBattleMoves[move].effect == EFFECT_TRIPLE_KICK) baseDamageAverage *= 2;
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(3));
                moveState->damage = baseDamageAverage * 3;
                hitCount = 3;
                break;

            case MULTIHIT_TRIPLE_KICK:
                if (moveState->accuracy >= 100) {
                    moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(3));
                    moveState->damage = baseDamageAverage * 6;
                } else {
                    moveState->multiHitExpect = gTripleKickHitExpected[moveState->accuracy - 1];
                    moveState->damage = ApplyModifier(gTripleKickMultiplier[moveState->accuracy - 1], baseDamageAverage);
                }
                hitCount = 3;
                break;

            case MULTIHIT_TWO:
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12(2));
                moveState->damage = baseDamageAverage * 2;
                hitCount = 2;
                break;

            case MULTIHIT_TWO_TO_FIVE:
                moveState->multiHitExpect = UQ_CLAMP_EXPECT(UQ_4_12((2.0 + 3.0) / 3.0 + (4.0 + 5.0) / 6.0));
                moveState->damage = ApplyModifier(UQ_4_12((2.0 + 3.0) / 3.0 + (4.0 + 5.0) / 6.0), baseDamageAverage);
                hitCount = 5;
                break;
        }
    }

    if (phase == AI_PHASE_DAMAGE_ROUGH || moveContainer->isTwoTurn) return AI_SCORE_DAMAGE(moveState->damage, battlerDef);

    if (moveState->missesThisTurn) return scoreOther;

    // Fail to break shield
    if (damageShield && damageShield > moveState->damage)
        return AI_SCORE_DAMAGE(moveState->damage, battlerDef);
    else if (damageShield)
        moveState->breakShield = TRUE;

    if (hitCount == 1) {
        if (damageShield) return AI_SCORE_BREAK_SUBSTITUTE;

        if (aiData->battlerState[battlerDef].sash) {
            moveState->breakShield = TRUE;
            return AI_SCORE_DAMAGE(max(defenderHp - 1, moveState->damage), battlerDef);
        }

        return CalculateKoChanceFine(baseDamage, 0, critMultiplier, doubleDamageChance, battlerDef, defenderHp, moveState);
    }

    if (moveContainer->multihitType > PARENTAL_BOND_START) {
        int bonusMultiplier = parentalBondSpread[1];
        int bonusDamage;
        for (i = 1; i < hitCount; i++) {
            bonusMultiplier += parentalBondSpread[i];
        }
        bonusDamage = ApplyModifier(bonusMultiplier, baseDamageAverage);
        baseDamage = ApplyModifier(parentalBondSpread[0], baseDamage);

        if (damageShield) {
            moveState->breakShield = TRUE;
            moveState->koChance = 100 * (baseDamage > damageShield && bonusDamage >= defenderHp);
            return AI_SCORE_BREAK_SUBSTITUTE + (moveState->koChance ? AI_SCORE_DAMAGE(bonusDamage, battlerDef) : 0);
        }

        return CalculateKoChanceFine(baseDamage, bonusDamage, critMultiplier, doubleDamageChance, battlerDef, defenderHp, moveState);
    }

    isTripleKick = gBattleMoves[move].effect == EFFECT_TRIPLE_KICK;
    if (isTripleKick) hitCount *= 2;

    requiredHits = 0;
    if (damageShield) {
        requiredHits += damageShield / baseDamageAverage + 1;
        if (requiredHits >= hitCount) return AI_SCORE_BREAK_SUBSTITUTE;
        scoreOther = AI_SCORE_BREAK_SUBSTITUTE;
    }

    if ((hitCount - requiredHits) * baseDamageAverage <= defenderHp)
        return scoreOther + AI_SCORE_DAMAGE((hitCount - requiredHits) * baseDamageAverage, battlerDef);

    switch (moveContainer->multihitType) {
        case MULTIHIT_THREE:
            if (isTripleKick) {
                if (requiredHits == 2)
                    requiredHits++;
                else if (requiredHits > 3)
                    return AI_SCORE_BREAK_SUBSTITUTE;
            }
            FALLTHROUGH
        case MULTIHIT_BEAT_UP:
            // TODO: Beat up
        case MULTIHIT_FIVE:
        case MULTIHIT_TEN:
        case MULTIHIT_TWO:
        HANDLE_KO_MULTIHIT:
            moveState->koChance = 100;
            i = (hitCount - requiredHits) * baseDamageAverage * 2 / defenderHp - 2;
            moveState->overkillInHalves = min(i, 3);
            break;

        case MULTIHIT_TWO_TO_FIVE:
            switch (requiredHits + defenderHp / baseDamageAverage + 1) {
                case 1:
                case 2:
                    moveState->koChance = 100;
                    moveState->overkillInHalves = 3;
                    break;

                case 3:
                    moveState->koChance = 66;
                    moveState->overkillInHalves = 1;
                    break;

                case 4:
                    moveState->koChance = 33;
                    break;

                case 5:
                    moveState->koChance = 16;
                    break;
            }
            break;

        case MULTIHIT_FOUR_OR_FIVE:
            switch (requiredHits + defenderHp / baseDamageAverage + 1) {
                case 1:
                case 2:
                    moveState->koChance = 100;
                    moveState->overkillInHalves = 3;
                    break;

                case 3:
                    moveState->koChance = 100;
                    moveState->overkillInHalves = 1;
                    break;

                case 4:
                    moveState->koChance = 100;
                    break;

                case 5:
                    moveState->koChance = 50;
                    break;
            }
            break;

        case MULTIHIT_TEN_CAN_MISS:
            if (moveState->accuracy >= 100) goto HANDLE_KO_MULTIHIT;
            {
                int hitsToKo = requiredHits + defenderHp / baseDamageAverage + 1;
                if (hitsToKo == 1)
                    moveState->koChance = 100;
                else if (hitsToKo == 2)
                    moveState->koChance = UQ_4_12_PERCENT(moveState->accuracy);
                else
                    moveState->koChance = ApplyModifier(100, gHitOdds[hitsToKo - 3][moveState->accuracy - 1]);
                i = hitCount * 2 / hitsToKo - 2;
                moveState->overkillInHalves = min(i, 3);
            }
            break;

        case MULTIHIT_TRIPLE_KICK:
            if (requiredHits == 2)
                requiredHits++;
            else if (requiredHits > 3)
                return AI_SCORE_BREAK_SUBSTITUTE;
            if (moveState->accuracy >= 100) goto HANDLE_KO_MULTIHIT;
            {
                int hitsToKo = requiredHits + defenderHp / baseDamageAverage + 1;
                if (hitsToKo == 1) {
                    moveState->koChance = 100;
                    moveState->overkillInHalves = 3;
                } else if (hitsToKo <= 3) {
                    moveState->koChance = moveState->accuracy;
                    moveState->overkillInHalves = 2 + (hitsToKo == 2);
                } else {
                    moveState->koChance = ApplyModifier(100, gHitOdds[0][moveState->accuracy - 1]);
                    moveState->overkillInHalves = 2 * hitCount / requiredHits - 2;
                }
            }
            break;
    }

    return scoreOther + AI_SCORE_DAMAGE((hitCount - requiredHits) * baseDamageAverage, battlerDef);
}