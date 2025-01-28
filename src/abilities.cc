
#include "abilities.hh"

extern "C" {
#include "constants/abilities.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_string_ids.h"
#include "constants/hold_effects.h"
#include "constants/item.h"
#include "constants/items.h"
#include "global.h"
#include "item.h"
#include "mgba_printf/mgba.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
}

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-function"

#define NO_ANNOUNCE 2

class __EnumHack {
   public:
    operator int() const { return 0; }
    operator AccuracyPriority() const { return ACCURACY_NO_RESULT; }
    operator MultihitType() const { return MULTIHIT_SINGLE; }
};

#define ENUM_OR(enumType) \
    inline enumType operator|(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }

ENUM_OR(InfiltrateType)

#define CHECK(effect) \
    if (!(effect)) return __EnumHack();
#define CHECK_NOT(effect) \
    if (effect) return __EnumHack();

#define __COMBINE(val1, val2) val1##val2
#define COMBINE(val1, val2) __COMBINE(val1, val2)

#define ON_ENTRY int ability, int battler
#define DELEGATE_ENTRY ability, battler
#define ON_ABSORB int battler, int move, int moveType, int *statId
#define DELEGATE_ABSORB battler, move, moveType, statId
#define ON_IMMUNE int battler, int attacker, int move, int moveType, const u8 **immunityScript
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
#define ON_INFILTRATE int battler, int move
#define DELEGATE_INFILTRATE battler, move
#define ON_DISGUISE int battler, int testOnly
#define DELEGATE_DISGUISE battler, testOnly
#define ON_WEATHER int ability, int battler
#define DELEGATE_WEATHER ability, battler
#define ON_TERRAIN int ability, int battler
#define DELEGATE_TERRAIN ability, battler
#define ON_END_TURN int ability, int battler
#define DELEGATE_END_TURN ability, battler
#define ON_ATTACKER int ability, int battler, int target, int move, int moveType
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
#define ON_DEFENDER int ability, int battler, int attacker, int move, int moveType
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
#define ON_EITHER(name) static int name##OnEither(int ability, int battler, int opponent, int move, int moveType)
#define ON_EITHER_ABILITY(name) .onAttacker = name##OnEither, .onDefender = name##OnEither
#define ON_RECOIL int damage, int battler, int moveType
#define DELEGATE_RECOIL damage, battler, moveType
#define ON_REACTIVE int ability, int battler, AbilityCallType callType
#define DELEGATE_REACTIVE ability, battler
#define ON_BATTLER_FAINTS int ability, int battler, int attacker, int fainted, int move, int moveType
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
#define ON_PARENTAL_BOND int battler, int move, int moveType
#define DELEGATE_PARENTAL_BOND battler, move, moveType
#define ON_STAT int ability, int battler, int statId, u32 *stat, NonStackingState *flags
#define DELEGATE_STAT ability, battler, statId, stat, flags
#define ON_OFFENSIVE_MULTIPLIER \
    int battler, int target, int move, int moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16 *resistance, u16 *modifier
#define DELEGATE_OFFENSIVE_MULTIPLIER battler, target, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier
#define ON_DEFENSIVE_MULTIPLIER int battler, int attacker, int move, int moveType, int typeEffectivenessModifier, int isCrit, u16 *resistance, u16 *modifier
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
#define ON_ACCURACY int ability, int battler, int target, int move, int moveType, int *accuracy
#define DELEGATE_ACCURACY ability, battler, target, move, moveType, accuracy
#define ON_SWAP_SPLIT int battler, int move
#define DELEGATE_SWAP_SPLIT battler, move
#define ON_CHOOSE_OFFENSIVE_STAT int battler, int move, int ignoreOffensiveStatDrops, int targetUnaware, u8 *atkStatToUse, u8 *secondaryAtkStatToUse
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
#define ON_CHOOSE_DEFENSIVE_STAT int battler, int target, int move, int ignoreDefensiveStatBoosts, int battlerUnaware
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
#define ON_STAB int moveType
#define DELEGATE_STAB moveType
#define ON_PRIORITY int battler, int target, int move
#define DELEGATE_PRIORITY battler, target, move
#define ON_MOVE_TYPE int ability, int move, int moveType, u8 *ateBoost
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
#define ON_EXIT int ability, int battler
#define DELEGATE_EXIT ability, battler
#define ON_CRIT int battler, int target, int move
#define DELEGATE_CRIT battler, target, move
#define ON_TYPE_EFFECTIVENESS int defType, int move, int moveType, u16 *mod
#define DELEGATE_TYPE_EFFECTIVENESS defType, move, moveType, mod
#define ON_COPY_MOVE int ability, int battler, int attacker, int target, int move
#define DELEGATE_COPY_MOVE ability, battler, attacker, target, move

#define GALE_WINGS_CLONE(type)                               \
    +[](ON_PRIORITY) -> int {                                \
        CHECK(GetTypeBeforeUsingMove(move, battler) == type) \
        CHECK(BATTLER_MAX_HP(battler))                       \
        return 1;                                            \
    }

#define MUL(val) MUL_MODIFIER(modifier, val)
#define RESISTANCE(val)                \
    {                                  \
        MUL_MODIFIER(resistance, val); \
        MUL_MODIFIER(modifier, val);   \
    }
static void InsertCorrectEndType(AbilityCallType type) {
    switch (type) {
        case ABILITY_BS_EXECUTE:
            BattleScriptExecute(BattleScript_End2);
            return;

        case ABILITY_BS_PUSH_CURSOR_AND_CALLBACK:
            BattleScriptPushCursorAndCallback(BattleScript_End3);
            return;
    }
}

int IsTargettedApplyOnFlagAppropriate(int contextBattler, int sourceBattler, int attacker, int target, AbilityApplyOnWithTarget flag) {
    switch (flag) {
        case APPLY_ON_ATTACKER_OR_TARGET:
            return sourceBattler == attacker || sourceBattler == target;

        case APPLY_ON_ATTACKER:
            return sourceBattler == attacker;

        case APPLY_ON_TARGET:
            return sourceBattler == target;
    }

    return IsApplyOnFlagAppropriate(contextBattler, sourceBattler, (AbilityApplyOn)flag);
}

int IsApplyOnFlagAppropriate(int contextBattler, int sourceBattler, AbilityApplyOn flag) {
    if (flag == APPLY_ON_SELF) return contextBattler == sourceBattler;
    if (contextBattler == sourceBattler) return !(flag & APPLY_IGNORE_SELF);
    if (GetBattlerSide(contextBattler) == GetBattlerSide(sourceBattler))
        return flag & APPLY_ON_ALLY;
    else
        return flag & APPLY_ON_FOE;
    return FALSE;
}

typedef enum {
    FOLLOWUP_STANDARD,
    FOLLOWUP_ALLOW_FAILED,
    FOLLOWUP_ALLOW_SELF,
} FollowupType;
static int AdjustFollowupMoveTarget(int battler, int *target, int move, FollowupType type) {
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT && type != FOLLOWUP_ALLOW_FAILED) return FALSE;

    switch (GetBattlerBattleMoveTargetFlags(move, battler)) {
        case MOVE_TARGET_BOTH:
        case MOVE_TARGET_FOES_AND_ALLY:
            *target = GetMoveTarget(MOVE_POUND, MOVE_TARGET_SELECTED + 1);
            return IsBattlerAlive(*target);

        default:
            if (*target == battler || *target == BATTLE_PARTNER(battler)) {
                if (type == FOLLOWUP_ALLOW_SELF)
                    *target = GetMoveTarget(MOVE_POUND, MOVE_TARGET_SELECTED + 1);
                else
                    return FALSE;
            }
            return battler != *target && IsBattlerAlive(*target);
    }
}

static int SwitchInAnnounce(int message) {
    gBattleCommunication[MULTISTRING_CHOOSER] = message;
    BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
    return TRUE;
}

static int TryTransformAttacker(int ability, int battler, AbilityCallType callType) {
    CHECK(ShouldChangeFormHpBased(battler))
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

    InsertCorrectEndType(callType);
    BattleScriptCall(BattleScript_AttackerFormChange);
    return TRUE;
}

static int AddBattlerType(int battler, int type) {
    CHECK_NOT(IS_BATTLER_OF_TYPE(battler, type))

    gBattleMons[battler].type3 = type;
    PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
    BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
    return TRUE;
}

static int AbilityStatusEffect(int effect) {
    gBattleScripting.moveEffect = effect;
    BattleScriptCall(BattleScript_AbilityStatusEffect);
    gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
    return TRUE;
}

static int AbilityStatusEffectDirect(int effect) {
    gBattleScripting.moveEffect = effect;
    gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
    SetMoveEffect(FALSE, FALSE);
    return FALSE;
}

static int AbilityStatusEffectSafe(int effect, int attacker, int target) {
    gBattleScripting.moveEffect = effect;
    gStackBattler1 = attacker;
    gStackBattler2 = target;
    BattleScriptCall(BattleScript_AbilityStatusEffectSafe);
    gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
    return TRUE;
}

static int PoisonPuppeteerClone(int ability, int battler, int (*predicate)(int battler, int target), const u8 *callback) {
    int flag = GetAbilityState(battler, ability);
    if (!flag) return FALSE;
    int any = FALSE;
    int realAttacker = gBattlerAttacker;
    gBattlerAttacker = battler;
    SetAbilityState(battler, ability, 0);

    for (int target = 0; target < gBattlersCount; target++) {
        FILTER(flag & (1 << target))
        FILTER(IsBattlerAlive(target))
        FILTER(predicate(battler, target))

        gStackBattler1 = gBattlerAttacker;
        gStackBattler2 = gBattlerTarget;
        BattleScriptCall(callback);
        any = TRUE;
    }
    gBattlerAttacker = realAttacker;

    CHECK(any)

    gStackBattler1 = battler;
    gBattleScripting.abilityPopupOverwrite = ability;
    BattleScriptCall(BattleScript_AbilityPopUpStack);
    return TRUE;
}

static int MoxieClone(int battler, int stat) {
    CHECK(HasAttackerFaintedTarget())
    CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))
    BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
    return TRUE;
}

#define ATE_ABILITY(type)                                                       \
    .onOffensiveMultiplier =                                                    \
        +[](ON_OFFENSIVE_MULTIPLIER) {                                          \
            if (moveType == type && gBattleStruct->ateBoost[battler]) MUL(1.1); \
        },                                                                      \
    .onMoveType = +[](ON_MOVE_TYPE) -> int {                                    \
        CHECK(moveType == TYPE_NORMAL)                                          \
        *ateBoost = TRUE;                                                       \
        return type + 1;                                                        \
    }

#define SWARM_MULTIPLIER(type)                                               \
    +[](ON_OFFENSIVE_MULTIPLIER) {                                           \
        if (moveType == type) {                                              \
            if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3)) \
                MUL(1.5);                                                    \
            else                                                             \
                MUL(1.2);                                                    \
        }                                                                    \
    }

#define BOOSTED_SWARM_MULTIPLIER(type)                                       \
    +[](ON_OFFENSIVE_MULTIPLIER) {                                           \
        if (moveType == type) {                                              \
            if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3)) \
                MUL(1.8);                                                    \
            else                                                             \
                MUL(1.3);                                                    \
        }                                                                    \
    }

static void RuinEffect(int ruinStat, int battler, int statId, u32 *stat, NonStackingState *flags) {
    if (statId != ruinStat) return;
    if (*flags & NON_STACKING_RUIN) return;
    ON_ABILITY(battler, FALSE, gAbilities[ability].ruinStat == statId, return) *stat *= .75;
    *flags = static_cast<NonStackingState>(static_cast<int>(*flags) | static_cast<int>(NON_STACKING_RUIN));
}

#define CONTEXT None
static const Ability None = {
    .name = $("-------"),
    .description = $("Empty ability slot."),
    .randomizerBanned = TRUE,
};

static const Ability Stench = {
    .name = $("Stench"),
    .description = $("Attacks have a 10% chance to\n"
                     "cause enemy to flinch."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    },
};

static const Ability Drizzle = {
    .name = $("Drizzle"),
    .description = $("Summons rain on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_DrizzleActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    },
};

static const Ability SpeedBoost = {
    .name = $("Speed Boost"),
    .description = $("Raises own Speed by one stage\n"
                     "after every turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_SpeedBoostActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

static const Ability BattleArmor = {
    .name = $("Battle Armor"),
    .description = $("Immune to critical hits. Takes\n"
                     "20% less damage from all attacks."),
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.8); },
    .onCrit = +[](ON_CRIT) { return NEVER_CRIT; },
    .onCritFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

static const Ability Sturdy = {
    .name = $("Sturdy"),
    .description = $("At full HP, cannot be KO in one\n"
                     "hit, stays at 1 HP instead."),
    .breakable = TRUE,
};

ON_EITHER(Damp) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK_NOT(IS_BATTLER_OF_TYPE(opponent, TYPE_WATER))

    gBattleMons[opponent].type1 = TYPE_WATER;
    gBattleMons[opponent].type2 = TYPE_WATER;
    gBattleMons[opponent].type3 = TYPE_MYSTERY;
    PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_WATER);
    gStackBattler1 = opponent;
    BattleScriptCall(BattleScript_StackBecameTheTypeFull);
    return TRUE;
}
static const Ability Damp = {
    .name = $("Damp"),
    .description = $("Makes foe Water-type on contact.\n"
                     "Also works on offense."),
    ON_EITHER_ABILITY(Damp),
};

static const Ability Limber = {
    .name = $("Limber"),
    .description = $("Immune to paralysis.\n"
                     "Takes 50% less recoil damage."),
    .breakable = TRUE,
    .halfRecoil = TRUE,
};

static const Ability SandVeil = {
    .name = $("Sand Veil"),
    .description = $("Evasion is boosted by 1.25x\n"
                     "while a sandstorm is active."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

ON_EITHER(Static) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeParalyzed(battler, opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_PARALYSIS, battler, opponent);
    return TRUE;
}
static const Ability Static = {
    .name = $("Static"),
    .description = $("30% chance to paralyze on\n"
                     "contact. Also works on offense."),
    ON_EITHER_ABILITY(Static),
};

static const Ability VoltAbsorb = {
    .name = $("Volt Absorb"),
    .description = $("Heals 25% of max HP when hit\n"
                     "by an Electric-type move."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

static const Ability WaterAbsorb = {
    .name = $("Water Absorb"),
    .description = $("Heals 25% of max HP when hit\n"
                     "by a Water-type move."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

static const Ability Oblivious = {
    .name = $("Oblivious"),
    .description = $("Immune to infatuation, Scare,\n"
                     "Intimidate and Taunt."),
    .breakable = TRUE,
};

static const Ability CloudNine = {
    .name = $("Cloud Nine"),
    .description = $("Clears weather and prevents\n"
                     "its effects."),
    .onEntry = +[](ON_ENTRY) -> int {
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
        return TRUE;
    },
};

static const Ability CompoundEyes = {
    .name = $("Compound Eyes"),
    .description = $("Grants a 1.3x accuracy boost."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.3;
        return ACCURACY_MULTIPLICATIVE;
    },
};

static const Ability Insomnia = {
    .name = $("Insomnia"),
    .description = $("Cannot fall asleep.\n"
                     "Rest fails if used."),
    .breakable = TRUE,
};

static const Ability ColorChange = {
    .name = $("Color Change"),
    .description = $("Changes type to a resist or an\n"
                     "immunity before getting hit."),
    .colorChange = TRUE,
};

static const Ability Immunity = {
    .name = $("Immunity"),
    .description = $("Cannot be poisoned. Halves\n"
                     "damage taken from Poison moves."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability FlashFire = {
    .name = $("Flash Fire"),
    .description = $("Powers up Fire-type moves by\n"
                     "1.5x if hit by a Fire-type move."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE)
        return ABSORB_RESULT_FLASH_FIRE;
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
        },
    .breakable = TRUE,
};

static const Ability ShieldDust = {
    .name = $("Shield Dust"),
    .description = $("Immune to added move effects and\n"
                     "all entry hazards."),
    .breakable = TRUE,
};

static const Ability OwnTempo = {
    .name = $("Own Tempo"),
    .description = $("Immune to confusion, Intimidate\n"
                     "and Scare."),
    .breakable = TRUE,
};

static const Ability SuctionCups = {
    .name = $("Suction Cups"),
    .description = $("Cannot be forced to switch out\n"
                     "by an enemy's move."),
    .breakable = TRUE,
};

static const Ability Intimidate = {
    .name = $("Intimidate"),
    .description = $("Lowers foes' Atk by one stage on\n"
                     "entry."),
    .onEntry = UseIntimidateClone,
};

static const Ability ShadowTag = {
    .name = $("Shadow Tag"),
    .description = $("Opponents can't be switched out.\n"
                     "Ghosts aren't affected."),
};

static const Ability RoughSkin = {
    .name = $("Rough Skin"),
    .description = $("Enemies lose 1/8 of max HP if\n"
                     "they use a contact move."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        gBattleMoveDamage = gBattleMons[attacker].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    },
};

static const Ability WonderGuard = {
    .name = $("Wonder Guard"),
    .description = $("Is only hit by Super-effective\n"
                     "attacks or indirect damage."),
    .breakable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability Levitate = {
    .name = $("Levitate"),
    .description = $("Immune to Ground-type moves.\n"
                     "Ups own Flying moves by 1.25x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING) MUL(1.25);
        },
    .breakable = TRUE,
};

static const Ability EffectSpore = {
    .name = $("Effect Spore"),
    .description = $("30% chance to inflict SLP, PARA\n"
                     "or PSN if hit by a contact move."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_GRASS))
        CHECK_NOT(BATTLER_HAS_ABILITY(attacker, ABILITY_OVERCOAT))
        CHECK_NOT(BATTLER_HAS_ABILITY(attacker, ABILITY_EFFECT_SPORE))
        CHECK(GetBattlerHoldEffect(attacker, TRUE) != HOLD_EFFECT_SAFETY_GOGGLES)
        CHECK(Random() % 100 < 30)

        switch (Random() % 3) {
            case 0:
                CHECK(CanBePoisoned(battler, attacker))

                AbilityStatusEffect(MOVE_EFFECT_POISON | MOVE_EFFECT_AFFECTS_USER);
                return TRUE;

            case 1:
                CHECK(CanBeParalyzed(battler, attacker))

                AbilityStatusEffect(MOVE_EFFECT_PARALYSIS | MOVE_EFFECT_AFFECTS_USER);
                return TRUE;

            case 2:
                CHECK(CanSleep(attacker))

                AbilityStatusEffect(MOVE_EFFECT_SLEEP | MOVE_EFFECT_AFFECTS_USER);
                return TRUE;
        }
        return FALSE;
    },
};

static const Ability Synchronize = {
    .name = $("Synchronize"),
    .description = $("Enemies inflicting status on\n"
                     "this Pokémon get same status."),
};

static const Ability ClearBody = {
    .name = $("Clear Body"),
    .description = $("Immune to stat drops."),
    .breakable = TRUE,
};

static const Ability NaturalCure = {
    .name = $("Natural Cure"),
    .description = $("Heals status condition upon\n"
                     "switching out."),
    .onExit = +[](ON_EXIT) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK(gBattleMons[battler].status1 & STATUS1_ANY)

        gActiveBattler = battler;
        gBattleMons[battler].status1 &= ~STATUS1_ANY;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
        MarkBattlerForControllerExec(battler);

        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptCall(BattleScript_NaturalCureExits);
        return TRUE;
    },
};

static const Ability LightningRod = {
    .name = $("Lightning Rod"),
    .description = $("Redirects Electric moves.\n"
                     "Absorbs them, ups highest Atk."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_ELECTRIC,
    .breakable = TRUE,
};

static const Ability SereneGrace = {
    .name = $("Serene Grace"),
    .description = $("Doubles chance of secondary\n"
                     "effects on its own moves."),
};

static const Ability SwiftSwim = {
    .name = $("Swift Swim"),
    .description = $("This Pokémon's Speed gets a\n"
                     "1.5x boost if rain is active."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
        },
};

static const Ability Chlorophyll = {
    .name = $("Chlorophyll"),
    .description = $("This Pokémon's Speed gets a\n"
                     "1.5x boost if sun is active."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        },
};

static const Ability Illuminate = {
    .name = $("Illuminate"),
    .description = $("Grants a 1.2x accuracy boost."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

static const Ability Trace = {
    .name = $("Trace"),
    .description = $("Copies the foe's ability.\n"
                     "Does not copy innates."),
    .onEntry = +[](ON_ENTRY) -> int {
        int target = BATTLE_OPPOSITE(battler);
        int newAbility = GetBattlerAbility(target);
        if (!IsBattlerAlive(target) || IsRolePlayBannedAbility(newAbility)) {
            target = BATTLE_PARTNER(target);
            CHECK(IsBattlerAlive(target))
            newAbility = GetBattlerAbility(target);
            CHECK_NOT(IsRolePlayBannedAbility(newAbility))
        }

        CHECK_NOT(HasAbilityIgnoringSuppression(battler, newAbility))

        int index = GetAbilityIndex(battler, ability, FALSE);
        CHECK(index < TOTAL_ABILITY_COUNT)

        gBattleMons[battler].abilities[index] = newAbility;
        gVolatileStructs[battler].switchInAbilityDone[index] = FALSE;

        gStackBattler1 = battler;
        gStackBattler2 = target;
        gBattleScripting.abilityPopupOverwrite = newAbility;
        BattleScriptPushCursorAndCallback(BattleScript_TraceActivatesEnd3);
        return TRUE;
    },
    .randomizerBanned = TRUE,
};

static const Ability HugePower = {
    .name = $("Huge Power"),
    .description = $("Doubles own Attack stat.\n"
                     "Boosts raw stat, not base stat."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK) *stat *= 2;
        },
};

ON_EITHER(PoisonPoint) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBePoisoned(battler, opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
    return TRUE;
}
static const Ability PoisonPoint = {
    .name = $("Poison Point"),
    .description = $("30% chance to poison on contact.\n"
                     "Also works on offense."),
    ON_EITHER_ABILITY(PoisonPoint),
};

static const Ability InnerFocus = {
    .name = $("Inner Focus"),
    .description = $("Blocks flinch, Intimidate, Scare.\n"
                     "Focus Blast never misses."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_FOCUS_BLAST)
        return ACCURACY_ALWAYS_HITS;
    },
    .breakable = TRUE,
};

static const Ability MagmaArmor = {
    .name = $("Magma Armor"),
    .description = $("Frostbite-immune. Takes 30% less\n"
                     "dmg from Water/Ice-type moves."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
        },
    .breakable = TRUE,
};

static const Ability WaterVeil = {
    .name = $("Water Veil"),
    .description = $("Burn-immune.\n"
                     "Casts Aqua Ring on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses3[battler] & STATUS3_AQUA_RING)

        gStatuses3[battler] |= STATUS3_AQUA_RING;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability MagnetPull = {
    .name = $("Magnet Pull"),
    .description = $("Traps opposing Steel-types.\n"
                     "Ghosts aren't affected."),
};

static const Ability Soundproof = {
    .name = $("Soundproof"),
    .description = $("Immune to sound-based moves."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

static const Ability RainDish = {
    .name = $("Rain Dish"),
    .description = $("Heals 1/8 of max HP every turn\n"
                     "if rain is active."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
};

static const Ability SandStream = {
    .name = $("Sand Stream"),
    .description = $("Summons a sandstorm on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_SandstreamActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    },
};

static const Ability Pressure = {
    .name = $("Pressure"),
    .description = $("Doubles foe's PP usage.\n"
                     "Clears stat buffs on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        int loweredStats = 0;
        for (int i = 0; i < gBattlersCount; i++) {
            if (!IsBattlerAlive(i)) continue;
            loweredStats |= TryResetBattlerStatChanges(i, i == battler ? RESET_STAT_DROPS : RESET_STAT_BUFFS);
        }

        if (loweredStats) {
            BattleScriptPushCursorAndCallback(BattleScript_PressureRemoveStats);
        }

        SwitchInAnnounce(B_MSG_SWITCHIN_PRESSURE);

        return TRUE;
    },
};

static const Ability ThickFat = {
    .name = $("Thick Fat"),
    .description = $("Takes 1/2 damage from Fire-type\n"
                     "and Ice-type attacks."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability EarlyBird = {
    .name = $("Early Bird"),
    .description = $("Awakens twice as fast from sleep."),
};

ON_EITHER(FlameBody) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeBurned(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
    return TRUE;
}
static const Ability FlameBody = {
    .name = $("Flame Body"),
    .description = $("30% chance to burn on contact.\n"
                     "Also works on offense."),
    ON_EITHER_ABILITY(FlameBody),
};

static const Ability RunAway = {
    .name = $("Run Away"),
    .description = $("Guarantees fleeing. Raises Speed\n"
                     "if stats lowered by an enemy."),
};

static const Ability KeenEye = {
    .name = $("Keen Eye"),
    .description = $("Immune to accuracy drops.\n"
                     "Grants a 1.2x accuracy boost."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .breakable = TRUE,
};

static const Ability HyperCutter = {
    .name = $("Hyper Cutter"),
    .description = $("Enemies can't lower Atk/Sp. Atk.\n"
                     "Crit rate of contact moves: +1."),
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(IsMoveMakingContact(move, battler))
        return 1;
    },
    .breakable = TRUE,
};

static const Ability Pickup = {
    .name = $("Pickup"),
    .description = $("Removes all hazards on entry.\n"
                     "Not immune to hazards."),
    .onEntry = +[](ON_ENTRY) -> int {
        int side = GetBattlerSide(battler);
        CHECK(gSideStatuses[side] & SIDE_STATUS_HAZARDS_ANY || gSideTimers[side].hotCoals || gSideTimers[side].caltrops)

        gSideStatuses[side] &= ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES | SIDE_STATUS_STICKY_WEB);
        gSideTimers[side].spikesAmount = 0;
        gSideTimers[side].toxicSpikesAmount = 0;
        gSideTimers[side].hotCoals = FALSE;
        gSideTimers[side].caltrops = FALSE;
        BattleScriptPushCursorAndCallback(BattleScript_PickUpActivate);
        return TRUE;
    },
};

static const Ability Truant = {
    .name = $("Truant"),
    .description = $("Can only attack every other turn.\n"
                     "Can use status moves every turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (GetAbilityState(battler, ability))
            SetAbilityState(battler, ability, FALSE);
        else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
            SetAbilityState(battler, ability, TRUE);
        return FALSE;
    },
};

static const Ability Hustle = {
    .name = $("Hustle"),
    .description = $("0.9x accuracy.\n"
                     "Boosts damage by 1.4x."),
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MUL(1.4); },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
        return ACCURACY_MULTIPLICATIVE;
    },
};

static const Ability CuteCharm = {
    .name = $("Cute Charm"),
    .description = $("30% chance to charm attacker on\n"
                     "contact, which halves its power."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(CanInfatuate(battler, attacker))
        CHECK(Random() % 100 < 30)

        gBattleMons[attacker].status2 |= STATUS2_INFATUATED_WITH(battler);
        BattleScriptCall(BattleScript_CuteCharmActivates);
        return TRUE;
    },
};

static const Ability Plus = {
    .name = $("Plus"),
    .description = $("Deals double damage if an ally\n"
                     "Pokémon has Minus or Plus."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            int partner = BATTLE_PARTNER(battler);
            if (!IsBattlerAlive(partner)) return;
            if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
        },
};

static const Ability Minus = {
    .name = $("Minus"),
    .description = $("Deals double damage if an ally\n"
                     "Pokémon has Minus or Plus."),
    .onOffensiveMultiplier = Plus.onOffensiveMultiplier,
};

static const Ability Forecast = {
    .name = $("Forecast"),
    .description = $("Changes form with the weather.\n"
                     "Weather setting triggers attack."),
    .onEntry = +[](ON_ENTRY) -> int { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onWeather = +[](ON_WEATHER) -> int { return TryTransformAttacker(ability, battler, ABILITY_BS_CALL); },
    .onEndTurn = +[](ON_END_TURN) -> int { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onAttacker = +[](ON_ATTACKER) -> int {
        switch (move) {
            case MOVE_SUNNY_DAY:
            case MOVE_RAIN_DANCE:
            case MOVE_SANDSTORM:
            case MOVE_HAIL:
            case MOVE_EERIE_FOG:
                break;

            default:
                return FALSE;
        }
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_FAILED))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_WEATHER_BALL, 0);
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability StickyHold = {
    .name = $("Sticky Hold"),
    .description = $("Can't lose its item."),
    .breakable = TRUE,
};

static const Ability ShedSkin = {
    .name = $("Shed Skin"),
    .description = $("30% chance to heal its status\n"
                     "condition at the end of a turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

static const Ability Guts = {
    .name = $("Guts"),
    .description = $("Ups Atk by 1.5x if suffering\n"
                     "from a status condition."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

static const Ability MarvelScale = {
    .name = $("Marvel Scale"),
    .description = $("Ups Def by 1.5x if suffering\n"
                     "from a status condition."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        },
    .breakable = TRUE,
};

static const Ability LiquidOoze = {
    .name = $("Liquid Ooze"),
    .description = $("Draining causes harm to enemies\n"
                     "instead of healing them."),
};

static const Ability Overgrow = {
    .name = $("Overgrow"),
    .description = $("Boosts Grass-type moves by 1.2x,\n"
                     "or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GRASS),
};

static const Ability Blaze = {
    .name = $("Blaze"),
    .description = $("Boosts Fire-type moves by 1.2x,\n"
                     "or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIRE),
};

static const Ability Torrent = {
    .name = $("Torrent"),
    .description = $("Boosts Water-type moves by 1.2x,\n"
                     "or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_WATER),
};

static const Ability Swarm = {
    .name = $("Swarm"),
    .description = $("Boosts Bug-type moves by 1.2x,\n"
                     "or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_BUG),
};

static const Ability RockHead = {
    .name = $("Rock Head"),
    .description = $("Immune to recoil damage, but not\n"
                     "immune to Explosion/crash dmg."),
    .noRecoil = TRUE,
};

static const Ability Drought = {
    .name = $("Drought"),
    .description = $("Summons sun on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_SUN, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_DroughtActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    },
};

static const Ability ArenaTrap = {
    .name = $("Arena Trap"),
    .description = $("Enemies can't flee. Ghosts and\n"
                     "ungrounded Pokémon are immune."),
};

static const Ability VitalSpirit = {
    .name = $("Vital Spirit"),
    .description = $("Can't fall asleep. Heals status\n"
                     "after using Fighting-type moves."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIGHTING)
        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability WhiteSmoke = {
    .name = $("White Smoke"),
    .description = $("Sets Smokescreen for 3 turns\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

        int side = GET_BATTLER_SIDE(battler);
        gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
        gSideTimers[side].started.smokescreen = TRUE;
        gSideTimers[side].smokescreenBattler = battler;
        return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
    },
};

static const Ability PurePower = {
    .name = $("Pure Power"),
    .description = $("Doubles own Attack stat.\n"
                     "Boosts raw stat, not base stat."),
    .onStat = HugePower.onStat,
};

static const Ability ShellArmor = {
    .name = $("Shell Armor"),
    .description = $("Immune to critical hits. Takes\n"
                     "20% less damage from all attacks."),
    .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier,
    .onCrit = BattleArmor.onCrit,
    .onCritFor = BattleArmor.onCritFor,
    .breakable = TRUE,
};

static const Ability AirLock = {
    .name = $("Air Lock"),
    .description = $("Clears weather and prevents\n"
                     "its effects."),
    .onEntry = CloudNine.onEntry,
};

static const Ability TangledFeet = {
    .name = $("Tangled Feet"),
    .description = $("Doubles Evasion when confused."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

static const Ability MotorDrive = {
    .name = $("Motor Drive"),
    .description = $("Boosts Speed instead of being\n"
                     "hit by Electric-type moves."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

static const Ability Rivalry = {
    .name = $("Rivalry"),
    .description = $("Deals 1.25x to same gender.\n"
                     "Takes .75x from opposite gender."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[battler].species, gBattleMons[battler].personality);
            if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[target].species, gBattleMons[target].personality))
                MUL(1.25);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[attacker].species, gBattleMons[attacker].personality);
            if (genderAtk == MON_MALE)
                genderAtk = MON_FEMALE;
            else if (genderAtk == MON_FEMALE)
                genderAtk = MON_MALE;
            if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[battler].species, gBattleMons[battler].personality))
                MUL(.75);
        },
    .breakable = TRUE,
};

static const Ability Steadfast = {
    .name = $("Steadfast"),
    .description = $("Raises Speed by one stage if\n"
                     "this Pokémon flinches."),
};

static const Ability SnowCloak = {
    .name = $("Snow Cloak"),
    .description = $("Evasion is boosted by 1.25x\n"
                     "under hail."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

static const Ability Gluttony = {
    .name = $("Gluttony"),
    .description = $("Eats berries early. Berries also\n"
                     "restore 1/3 of max HP."),
};

static const Ability AngerPoint = {
    .name = $("Anger Point"),
    .description = $("Getting hit raises Atk by +1.\n"
                     "Critical hits maximize Attack."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_ATK))

        if (gIsCriticalHit) {
            SetStatChanger(STAT_ATK, 12);
            BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
        } else {
            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        }
        return TRUE;
    },
};

static const Ability Unburden = {
    .name = $("Unburden"),
    .description = $("Consuming its held item doubles\n"
                     "Speed until switched out."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
        },
};

static const Ability Heatproof = {
    .name = $("Heatproof"),
    .description = $("Halves damage taken from Fire-\n"
                     "type moves. Takes no burn damage."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability Simple = {
    .name = $("Simple"),
    .description = $("Doubles all stat changes on\n"
                     "this Pokémon."),
};

static const Ability DrySkin = {
    .name = $("Dry Skin"),
    .description = $("Water/Rain heals.\n"
                     "Fire/Sun hurts."),
    .onAbsorb = WaterAbsorb.onAbsorb,
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !IsMagicGuardProtected(battler)) {
            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            BattleScriptPushCursorAndCallback(BattleScript_SolarPowerActivates);
            return TRUE;
        }

        return RainDish.onEndTurn(DELEGATE_END_TURN);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(1.25);
        },
    .breakable = TRUE,
};

static const Ability Download = {
    .name = $("Download"),
    .description = $("Raises Atk/Sp. Atk by one stage\n"
                     "depending on opponent."),
    .onEntry = +[](ON_ENTRY) -> int {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(battler)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(battler))

        int stat = GetHighestDefendingStatId(gBattlerTarget, TRUE) == STAT_DEF ? STAT_SPATK : STAT_ATK;
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

static const Ability IronFist = {
    .name = $("Iron Fist"),
    .description = $("Boosts the power of punching\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_IRON_FIST(battler, move)) MUL(1.3);
        },
};

static const Ability PoisonHeal = {
    .name = $("Poison Heal"),
    .description = $("Restores 1/8 of max HP after\n"
                     "each turn if poisoned."),
};

static const Ability Adaptability = {
    .name = $("Adaptability"),
    .description = $("Increases STAB from 1.5x to 2x."),
    .adaptability = TRUE,
};

static const Ability SkillLink = {
    .name = $("Skill Link"),
    .description = $("Multi-hit moves always hit the\n"
                     "maximum number of times."),
    .skillLink = TRUE,
};

static const Ability Hydration = {
    .name = $("Hydration"),
    .description = $("Cures own status at the end of\n"
                     "every turn in rain."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

static const Ability SolarPower = {
    .name = $("Solar Power"),
    .description = $("Ups highest attacking stat\n"
                     "by 1.5x in sun."),
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        },
};

static const Ability QuickFeet = {
    .name = $("Quick Feet"),
    .description = $("Ups Speed by 1.5x if suffering\n"
                     "from a status condition."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        },
};

static const Ability Normalize = {
    .name = $("Normalize"),
    .description = $("Its moves become Normal-type,\n"
                     "get 1.1x boost, ignore resists."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        },
    .onMoveType = +[](ON_MOVE_TYPE) -> int { return TYPE_NORMAL + 1; },
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_NORMAL) CHECK(*mod) CHECK(*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        return TRUE;
    },
};

static const Ability Sniper = {
    .name = $("Sniper"),
    .description = $("Critical hits have a 2.25x dmg\n"
                     "multiplier instead of 1.5x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (isCrit) MUL(1.5);
        },
};

static const Ability MagicGuard = {
    .name = $("Magic Guard"),
    .description = $("Only damaged by attacks."),
    .magicGuard = TRUE,
};

static const Ability NoGuard = {
    .name = $("No Guard"),
    .description = $("Attacks used by and on this\n"
                     "Pokémon bypass accuracy checks."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_ALWAYS_HITS; },
    .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
};

static const Ability Stall = {
    .name = $("Stall"),
    .description = $("Takes 30% less damage if it\n"
                     "hasn't moved yet."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
        },
    .breakable = TRUE,
};

static const Ability Technician = {
    .name = $("Technician"),
    .description = $("Moves with 60 BP or less get\n"
                     "a 1.5x boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (basePower <= 60) MUL(1.5);
        },
};

static const Ability LeafGuard = {
    .name = $("Leaf Guard"),
    .description = $("Immune to status conditions if\n"
                     "sun is active."),
    .breakable = TRUE,
};

static const Ability Klutz = {
    .name = $("Klutz"),
    .description = $("Own held item has no effect.\n"
                     "Mega Stones are unaffected."),
};

static const Ability MoldBreaker = {
    .name = $("Mold Breaker"),
    .description = $("Moves hit through abilities.\n"
                     "Also affects innates."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); },
};

static const Ability SuperLuck = {
    .name = $("Super Luck"),
    .description = $("Raises critical-hit ratio of own\n"
                     "moves by +1."),
    .onCrit = +[](ON_CRIT) -> int { return 1; },
};

static const Ability Aftermath = {
    .name = $("Aftermath"),
    .description = $("If faints by a contact move,\n"
                     "attacker takes 25% of max HP."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = 1;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    },
};

static const Ability Anticipation = {
    .name = $("Anticipation"),
    .description = $("Senses Super-effective moves.\n"
                     "Blocks one Super-effective hit."),
    .onEntry = +[](ON_ENTRY) -> int {
        int side = GetBattlerSide(battler);
        int any = FALSE;

        for (int i = 0; i < gBattlersCount; i++) {
            if (IsBattlerAlive(i) && side != GetBattlerSide(i)) {
                for (int j = 0; j < MAX_MON_MOVES; j++) {
                    int move = gBattleMons[i].moves[j];
                    int moveType = gBattleMoves[move].type;
                    if (CalcTypeEffectivenessMultiplier(move, moveType, i, battler, FALSE) >= UQ_4_12(2.0)) {
                        any = TRUE;
                        break;
                    }
                }
            }
        }

        CHECK(any)

        return SwitchInAnnounce(B_MSG_SWITCHIN_ANTICIPATION);
    },
    .breakable = TRUE,
    .persistent = TRUE,
};

static const Ability Forewarn = {
    .name = $("Forewarn"),
    .description = $("Casts a 50 BP Future Sight on\n"
                     "entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(gBattlerTarget) || gWishFutureKnock.futureSightCounter[gBattlerTarget]) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(gBattlerTarget))
        CHECK_NOT(gWishFutureKnock.futureSightCounter[gBattlerTarget])

        gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] |= SIDE_STATUS_FUTUREATTACK;
        gWishFutureKnock.futureSightMove[gBattlerTarget] = MOVE_FUTURE_SIGHT;
        gWishFutureKnock.futureSightPower[gBattlerTarget] = 50;
        gWishFutureKnock.futureSightAttacker[gBattlerTarget] = battler;
        gWishFutureKnock.futureSightCounter[gBattlerTarget] = 3;

        BattleScriptPushCursorAndCallback(BattleScript_ForewarnReworkActivates);
        return TRUE;
    },
};

static const Ability Unaware = {
    .name = $("Unaware"),
    .description = $("Ignores foes' stat changes, both\n"
                     "positive and negative ones."),
    .breakable = TRUE,
    .unaware = TRUE,
};

static const Ability TintedLens = {
    .name = $("Tinted Lens"),
    .description = $("Attacks deal double damage if\n"
                     "resisted."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
        },
};

static const Ability Filter = {
    .name = $("Filter"),
    .description = $("Takes 35% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        },
    .breakable = TRUE,
};

static const Ability SlowStart = {
    .name = $("Slow Start"),
    .description = $("Halves Attack and Speed during\n"
                     "the first 5 turns out."),
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].slowStartTimer = 5;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
    },
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK && statId != STAT_SPEED) return;
            if (gVolatileStructs[battler].slowStartTimer) *stat /= 2;
        },
};

static const Ability Scrappy = {
    .name = $("Scrappy"),
    .description = $("Normal/Fighting can hit Ghosts.\n"
                     "Immune to Intimidate/Scare."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
        CHECK(defType == TYPE_GHOST)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    },
};

static const Ability StormDrain = {
    .name = $("Storm Drain"),
    .description = $("Redirects Water moves.\n"
                     "Absorbs them, ups highest Atk."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_WATER,
    .breakable = TRUE,
};

static const Ability IceBody = {
    .name = $("Ice Body"),
    .description = $("Heals 1/8 of max HP every turn\n"
                     "in hail."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
};

static const Ability SolidRock = {
    .name = $("Solid Rock"),
    .description = $("Takes 35% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability SnowWarning = {
    .name = $("Snow Warning"),
    .description = $("Summons hail on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_SnowWarningActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    },
};

static const Ability HoneyGather = {
    .name = $("Honey Gather"),
    .description = $("Has a 50% chance to find Honey\n"
                     "each turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK(Random() % 2)

        gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
        BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
        return TRUE;
    },
};

static const Ability Frisk = {
    .name = $("Frisk"),
    .description = $("Checks foes' item and disables\n"
                     "their items for two turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        int any = FALSE;
        for (int i = GetBattlerSide(BATTLE_OPPOSITE(battler)); i < gBattlersCount; i += 2) {
            FILTER(IsBattlerAlive(i))
            FILTER(gBattleMons[i].item)
            any = TRUE;
            break;
        }

        CHECK(any)
        BattleScriptPushCursorAndCallback(BattleScript_FriskActivates);
        return TRUE;
    },
};

static const Ability Reckless = {
    .name = $("Reckless"),
    .description = $("Moves causing recoil damage\n"
                     "deal 1.2x more damage."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
        },
};

static const Ability Multitype = {
    .name = $("Multitype"),
    .description = $("Held Plate item decides holder's\n"
                     "type."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability FlowerGift = {
    .name = $("Flower Gift"),
    .description = $("Increases the party's SpAtk\n"
                     "and SpDef by 1.5x in Sun."),
    .onEntry = Forecast.onEntry,
    .onWeather = Forecast.onWeather,
    .onEndTurn = Forecast.onEndTurn,
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_SPATK && statId != STAT_SPDEF) return;
            if (IsWeatherActive(WEATHER_SUN_ANY)) *stat *= 1.5;
        },
    .onStatFor = APPLY_ON_ALLY,
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability BadDreams = {
    .name = $("Bad Dreams"),
    .description = $("Sleeping Pokémon lose 1/4 of max\n"
                     "HP at the end of each turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
        return NO_ANNOUNCE;
    },
};

static const Ability Pickpocket = {
    .name = $("Pickpocket"),
    .description = $("Steals the foe's held item on\n"
                     "contact."),
};

static const Ability SheerForce = {
    .name = $("Sheer Force"),
    .description = $("Exchanges added effects on its\n"
                     "moves for 1.3x more power."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
        },
};

static const Ability Contrary = {
    .name = $("Contrary"),
    .description = $("Stat raises turn into stat drops\n"
                     "for this Pokémon and vice versa."),
    .breakable = TRUE,
};

static const Ability Unnerve = {
    .name = $("Unnerve"),
    .description = $("Foes can't eat Berries as long\n"
                     "as this Pokémon is in battle."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); },
};

static const Ability Defiant = {
    .name = $("Defiant"),
    .description = $("Raises Attack by two stages if\n"
                     "stats are lowered by an enemy."),
};

static const Ability Defeatist = {
    .name = $("Defeatist"),
    .description = $("Halves Atk and Sp. Atk stats if\n"
                     "user is below 1/3 of max HP."),
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK && statId != STAT_SPATK) return;
            if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
        },
};

static const Ability CursedBody = {
    .name = $("Cursed Body"),
    .description = $("30% chance to disable moves\n"
                     "if enemy makes contact."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(gVolatileStructs[attacker].disabledMove)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsAbilityOnSide(attacker, ABILITY_AROMA_VEIL))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])
        CHECK(Random() % 100 < 30)

        gVolatileStructs[attacker].disabledMove = gChosenMove;
        gVolatileStructs[attacker].disableTimer = 4;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove);
        BattleScriptCall(BattleScript_CursedBodyActivates);
        return TRUE;
    },
};

static const Ability Healer = {
    .name = $("Healer"),
    .description = $("30% chance to heal user or ally's\n"
                     "status at the end of each turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(Random() % 100 < 30)

        if (IsBattlerAlive(BATTLE_PARTNER(battler)) && gBattleMons[BATTLE_PARTNER(battler)].status1 & STATUS1_ANY) {
            gEffectBattler = battler;
            gBattleScripting.battler = BATTLE_PARTNER(battler);
            BattleScriptPushCursorAndCallback(BattleScript_HealerActivates);
            return TRUE;
        } else if (IsBattlerAlive(battler) && gBattleMons[battler].status1 & STATUS1_ANY) {
            if (AbilityHealMonStatus(battler, ability)) return TRUE;
        }
        return FALSE;
    },
};

static const Ability FriendGuard = {
    .name = $("Friend Guard"),
    .description = $("Reduces damage that ally takes\n"
                     "by 50% in double battles."),
    .breakable = TRUE,
};

static const Ability WeakArmor = {
    .name = $("Weak Armor"),
    .description = $("If hit by a contact attack:\n"
                     "-1 Defense and +2 Speed."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(CanRaiseStat(battler, STAT_SPEED) || CanLowerStat(battler, STAT_DEF))

        if (gBattleMoves[move].effect == EFFECT_HIT_ESCAPE && CanBattlerSwitch(attacker))
            gRoundStructs[battler].disableEjectPack = TRUE;  // Set flag for target

        BattleScriptCall(BattleScript_WeakArmorActivates);
        return TRUE;
    },
};

static const Ability HeavyMetal = {
    .name = $("Heavy Metal"),
    .description = $("Doubles this Pokémon's weight."),
};

static const Ability LightMetal = {
    .name = $("Light Metal"),
    .description = $("Boosts Speed by 1.3x and halves\n"
                     "this Pokémon's weight."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= 1.3;
        },
};

static const Ability Multiscale = {
    .name = $("Multiscale"),
    .description = $("At full HP, halves damage taken\n"
                     "from attacks"),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (BATTLER_MAX_HP(battler)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability ToxicBoost = {
    .name = $("Toxic Boost"),
    .description = $("Ups Atk by 1.5x if poisoned.\n"
                     "Immune to Poison status damage."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

int FlareBoostHandler(int ability, int battler, AbilityCallType callType) {
    CHECK(CanBeBurned(battler))
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

    InsertCorrectEndType(callType);
    gBattleMons[battler].status1 |= STATUS1_BURN;
    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
    MarkBattlerForControllerExec(battler);
    BattleScriptCall(BattleScript_FlareBoostRet);
    return TRUE;
}

static const Ability FlareBoost = {
    .name = $("Flare Boost"),
    .description = $("Ups Sp. Atk by 1.5x if burned.\n"
                     "Ignites in fog."),
    .onEntry = +[](ON_ENTRY) -> int { return FlareBoostHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onWeather = +[](ON_WEATHER) -> int { return FlareBoostHandler(ability, battler, ABILITY_BS_CALL); },
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_SPATK) return;
            if (gBattleMons[battler].status1 & STATUS1_BURN) *stat *= 1.5;
        },
};

static const Ability Harvest = {
    .name = $("Harvest"),
    .description = $("50% chance to recycle a used\n"
                     "Berry every turn, 100% in sun."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK_NOT(gBattleStruct->changedItems[battler])
        CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

        BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
        return TRUE;
    },
};

static const Ability Telepathy = {
    .name = $("Telepathy"),
    .description = $("Can't be damaged by ally attacks."),
    .breakable = TRUE,
};

static const Ability Moody = {
    .name = $("Moody"),
    .description = $("Lowers a random stat by -1 and\n"
                     "raises another by +2 every turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2);
        int validToRaise = 0, validToLower = 0;

        int i;
        for (i = STAT_ATK; i < NUM_STATS; i++) {
            if (CanLowerStat(battler, i)) validToLower |= 1 << i;
            if (CanRaiseStat(battler, i)) validToRaise |= 1 << i;
        }

        CHECK(validToLower || validToRaise)

        if (validToRaise) {
            do {
                i = (Random() % NUM_STATS - STAT_ATK) + STAT_ATK;
            } while (!(validToRaise & (1 << i)));
            SetStatChanger(i, 2);
            validToLower &= ~(1 << i);
        }
        if (validToLower) {
            do {
                i = (Random() % NUM_STATS - STAT_ATK) + STAT_ATK;
            } while (!(validToLower & (1 << i)));
            SET_STATCHANGER2(gBattleScripting.savedStatChanger, i, 1, TRUE);
        }
        BattleScriptPushCursorAndCallback(BattleScript_MoodyActivates);
        return TRUE;
    },
};

static const Ability Overcoat = {
    .name = $("Overcoat"),
    .description = $("Blocks weather dmg, powder moves.\n"
                     "20% Special damage reduction."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.8);
        },
    .breakable = TRUE,
};

static const Ability PoisonTouch = {
    .name = $("Poison Touch"),
    .description = $("30% chance to poison on contact.\n"
                     "Also works on offense."),
    .onAttacker = PoisonPoint.onAttacker,
    .onDefender = PoisonPoint.onDefender,
};

static const Ability Regenerator = {
    .name = $("Regenerator"),
    .description = $("Heals 1/3 of max HP upon\n"
                     "switching out."),
    .onExit = +[](ON_EXIT) -> int {
        CHECK(IsBattlerAlive(battler)) CHECK(BATTLER_MAX_HP(battler));
        BattleScriptCall(BattleScript_RegeneratorExits);
        return FALSE;
    },
};

static const Ability BigPecks = {
    .name = $("Big Pecks"),
    .description = $("Boosts the power of contact\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsMoveMakingContact(move, battler)) MUL(1.3);
        },
};

static const Ability SandRush = {
    .name = $("Sand Rush"),
    .description = $("This Pokémon's Speed gets a\n"
                     "1.5x boost in a sandstorm."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        },
};

static const Ability WonderSkin = {
    .name = $("Wonder Skin"),
    .description = $("Opposing status moves have\n"
                     "their accuracy halved."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IS_MOVE_STATUS(move));
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

static const Ability Analytic = {
    .name = $("Analytic"),
    .description = $("Attacks get a 1.3x power boost\n"
                     "if it moves last."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && move != MOVE_FUTURE_SIGHT && move != MOVE_DOOM_DESIRE) MUL(1.3);
        },
};

static const Ability Illusion = {
    .name = $("Illusion"),
    .description = $("Appears as last party slot and\n"
                     "boosts power by 1.3x until hit."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(gBattleStruct->illusion[battler].on)
        CHECK_NOT(gBattleStruct->illusion[battler].broken)

        BattleScriptCall(BattleScript_IllusionOff);
        return TRUE;
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleStruct->illusion[battler].on && !gBattleStruct->illusion[battler].broken) MUL(1.3);
        },
};

static const Ability Imposter = {
    .name = $("Imposter"),
    .description = $("Transforms into the foe on\n"
                     "entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(gBattlerTarget))
        CHECK_NOT(gBattleMons[gBattlerTarget].status2 & (STATUS2_TRANSFORMED | STATUS2_SUBSTITUTE))
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
        CHECK_NOT(gBattleStruct->illusion[gBattlerTarget].on)
        CHECK_NOT(gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE)

        BattleScriptPushCursorAndCallback(BattleScript_ImposterActivates);
        return TRUE;
    },
};

static const Ability Infiltrator = {
    .name = $("Infiltrator"),
    .description = $("Own moves bypass Substitutes\n"
                     "and damage reduction screens."),
    .onInfiltrate = +[](ON_INFILTRATE) -> InfiltrateType { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; },
};

static const Ability Mummy = {
    .name = $("Mummy"),
    .description = $("If hit, makes the attacker's ability\n"
                     "Mummy."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(BattlerHasAbility(attacker, ability, FALSE))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(attacker)))
        CHECK_NOT(DoesBattlerHaveAbilityShield(attacker))

        UpdateAbilityStateIndicesForNewAbility(attacker, ability);
        ReplaceAbility(attacker, ability);
        BattleScriptCall(BattleScript_MummyActivates);
        return TRUE;
    },
};

static const Ability Moxie = {
    .name = $("Moxie"),
    .description = $("Dealing a KO raises Attack by\n"
                     "one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_ATK); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Justified = {
    .name = $("Justified"),
    .description = $("Boosts Attack instead of being\n"
                     "hit by Dark-type moves."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_DARK);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
};

static const Ability Rattled = {
    .name = $("Rattled"),
    .description = $("If hit by Bug, Dark or Ghost\n"
                     "move, or flinches: +1 Speed."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

static const Ability MagicBounce = {
    .name = $("Magic Bounce"),
    .description = $("Bounces back the effect of\n"
                     "status moves to their user."),
    .breakable = TRUE,
    .magicBounce = TRUE,
};

static const Ability SapSipper = {
    .name = $("Sap Sipper"),
    .description = $("Boosts highest Atk instead of\n"
                     "being hit by Grass-type moves."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_GRASS);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

static const Ability Prankster = {
    .name = $("Prankster"),
    .description = $("Status moves have +1 priority\n"
                     "but fail on opposing Dark-types."),
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IS_MOVE_STATUS(move))
        return 1;
    },
};

static const Ability SandForce = {
    .name = $("Sand Force"),
    .description = $("Ups highest attacking stat\n"
                     "by 1.5x in sand."),
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        },
};

static const Ability IronBarbs = {
    .name = $("Iron Barbs"),
    .description = $("Enemies lose 1/8 of max HP if\n"
                     "they use a contact move."),
    .onDefender = RoughSkin.onDefender,
};

static const Ability ZenMode = {
    .name = $("Zen Mode"),
    .description = $("Transforms into Zen Mode on\n"
                     "entry until end of battle."),
    .onEntry = Forecast.onEntry,
    .onEndTurn = Forecast.onEndTurn,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability VictoryStar = {
    .name = $("Victory Star"),
    .description = $("Gives 1.2x accuracy boost to\n"
                     "its own and its allies' moves."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_ALLY,
};

static const Ability Turboblaze = {
    .name = $("Turboblaze"),
    .description = $("Moves hit through abilities.\n"
                     "Adds Fire type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FIRE); },
};

static const Ability Teravolt = {
    .name = $("Teravolt"),
    .description = $("Moves hit through abilities.\n"
                     "Adds Electric type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ELECTRIC); },
};

static const Ability AromaVeil = {
    .name = $("Aroma Veil"),
    .description = $("Immune to Encore, Attract, Taunt,\n"
                     "Torment, Disable, Heal Block."),
    .breakable = TRUE,
};

static const Ability FlowerVeil = {
    .name = $("Flower Veil"),
    .description = $("Grass-types on this Pokémon's\n"
                     "side are immune to stat drops."),
    .breakable = TRUE,
};

static const Ability CheekPouch = {
    .name = $("Cheek Pouch"),
    .description = $("This ability has no effect."),
    .randomizerBanned = TRUE,
};

static const Ability Protean = {
    .name = $("Protean"),
    .description = $("Changes type depending on the\n"
                     "move it's about to use."),
    .protean = TRUE,
};

static const Ability FurCoat = {
    .name = $("Fur Coat"),
    .description = $("Halves damage taken by Physical\n"
                     "moves. Does NOT double Defense."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability Magician = {
    .name = $("Magician"),
    .description = $("Steals the foe's held item after\n"
                     "using a non-contact move."),
};

static const Ability Bulletproof = {
    .name = $("Bulletproof"),
    .description = $("Immune to projectile, ball, or\n"
                     "bomb-based moves."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability Competitive = {
    .name = $("Competitive"),
    .description = $("Raises Sp. Atk by two stages if\n"
                     "stats are lowered by an enemy."),
};

static const Ability StrongJaw = {
    .name = $("Strong Jaw"),
    .description = $("Boosts the power of bite/fang\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
        },
};

static const Ability Refrigerate = {
    .name = $("Refrigerate"),
    .description = $("Normal-type moves become Ice-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_ICE),
};

static const Ability SweetVeil = {
    .name = $("Sweet Veil"),
    .description = $("This Pokémon and its ally are\n"
                     "immune to sleep."),
    .breakable = TRUE,
};

static const Ability StanceChange = {
    .name = $("Stance Change"),
    .description = $("Turns into Blade or Shield form\n"
                     "depending on move used."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability GaleWings = {
    .name = $("Gale Wings"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's Flying-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_FLYING),
};

static const Ability MegaLauncher = {
    .name = $("Mega Launcher"),
    .description = $("Boosts Beam/Pump/Cannon/Shot/\n"
                     "Gun/Pulse, etc. moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST) MUL(1.3);
        },
};

static const Ability GrassPelt = {
    .name = $("Grass Pelt"),
    .description = $("This Pokémon's Defense gets a\n"
                     "1.5x boost in Grassy Terrain."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
        },
    .breakable = TRUE,
};

static const Ability Symbiosis = {
    .name = $("Symbiosis"),
    .description = $("Passes own item to its ally if\n"
                     "said ally consumes its item."),
};

static const Ability ToughClaws = {
    .name = $("Tough Claws"),
    .description = $("Boosts the power of contact\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier = BigPecks.onOffensiveMultiplier,
};

static const Ability Pixilate = {
    .name = $("Pixilate"),
    .description = $("Normal-type moves become Fairy-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_FAIRY),
};

static const Ability Gooey = {
    .name = $("Gooey"),
    .description = $("Lowers Speed of enemies that\n"
                     "make contact with this Pokémon."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
        CHECK(IsMoveMakingContact(move, attacker))

        BattleScriptCall(BattleScript_GooeyActivates);
        gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
        return TRUE;
    },
};

static const Ability Aerilate = {
    .name = $("Aerilate"),
    .description = $("Normal-type moves become Flying-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_FLYING),
};

static const Ability ParentalBond = {
    .name = $("Parental Bond"),
    .description = $("Moves hit twice. 1st hit at 100%\n"
                     "power, 2nd hit at 25%."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType { return PARENTAL_BOND_HYPER_AGGRESSIVE; },
    .resistsFortKnox = TRUE,
};

static const Ability DarkAura = {
    .name = $("Dark Aura"),
    .description = $("Boosts Dark moves by 1.33x for\n"
                     "all while this Pokémon is out."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA); },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType != TYPE_DARK) return;
            if (IsAbilityOnField(ABILITY_AURA_BREAK))
                MUL(.75);
            else
                MUL(1.33);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

static const Ability FairyAura = {
    .name = $("Fairy Aura"),
    .description = $("Boosts Fairy moves by 1.33x for\n"
                     "all while this Pokémon is out."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA); },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType != TYPE_FAIRY) return;
            if (IsAbilityOnField(ABILITY_AURA_BREAK))
                MUL(.75);
            else
                MUL(1.33);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

static const Ability AuraBreak = {
    .name = $("Aura Break"),
    .description = $("Cancels aura abilities and makes\n"
                     "them 25% weaker instead."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); },
    .breakable = TRUE,
};

static const Ability PrimordialSea = {
    .name = $("Primordial Sea"),
    .description = $("Heavy Rain until switched out.\n"
                     "Fire-type moves are unusable."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
        return TRUE;
    },
};

static const Ability DesolateLand = {
    .name = $("Desolate Land"),
    .description = $("Intense Sun until switched out.\n"
                     "Water-type moves are unusable."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
        return TRUE;
    },
};

static const Ability DeltaStream = {
    .name = $("Delta Stream"),
    .description = $("Strong Winds until switched out.\n"
                     "Weather-based moves not usable."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
        return TRUE;
    },
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_WEATHER_BASED)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER)
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
};

static const Ability Stamina = {
    .name = $("Stamina"),
    .description = $("Getting hit raises Def by +1.\n"
                     "Critical hits maximize Defense."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF))

        if (gIsCriticalHit) {
            SetStatChanger(STAT_DEF, 12);
            BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
        } else {
            SetStatChanger(STAT_DEF, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        }
        return TRUE;
    },
};

static const Ability WimpOut = {
    .name = $("Wimp Out"),
    .description = $("At 1/2 of max HP or below,\n"
                     "instantly switches out."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(TestSheerForceFlag(attacker, gCurrentMove))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
        CHECK(CountUsablePartyMons(battler));
        gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
        return FALSE;
    },
};

static const Ability EmergencyExit = {
    .name = $("Emergency Exit"),
    .description = $("At 1/2 of max HP or below,\n"
                     "instantly switches out."),
    .onDefender = WimpOut.onDefender,
};

static const Ability WaterCompaction = {
    .name = $("Water Compaction"),
    .description = $("Takes 1/2 dmg from Water-type\n"
                     "moves. +2 Def when hit by those."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_WATER)
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 2);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability Merciless = {
    .name = $("Merciless"),
    .description = $("100% crit if targetting slowed,\n"
                     "poisoned, paralyzed, or bleeding foes."),
    .onCrit = +[](ON_CRIT) -> int {
        if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
        if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
        if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
        return 0;
    },
};

static const Ability ShieldsDown = {
    .name = $("Shields Down"),
    .description = $("At 1/2 of max HP or below,\n"
                     "transforms into Core form."),
    .onEntry = Forecast.onEntry,
    .onEndTurn = Forecast.onEndTurn,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
        CHECK(gBattleMoves[move].effect == EFFECT_SHELL_SMASH)
        CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

        int i;
        for (i = 0; i < ARRAY_COUNT(gHpTransformations); i++) {
            if (gHpTransformations[i][0] == ability && gBattleMons[battler].species == gHpTransformations[i][1]) break;
        }

        if (i < ARRAY_COUNT(gHpTransformations)) {
            UpdateAbilityStateIndicesForNewSpecies(battler, gHpTransformations[i][2]);
            SetAbilityState(battler, ability, TRUE);
            gBattleMons[battler].species = gHpTransformations[i][2];
            BattleScriptCall(BattleScript_AttackerFormChange);
            return TRUE;
        }
        return FALSE;
    },
    .unsuppressable = TRUE,
};

static const Ability Stakeout = {
    .name = $("Stakeout"),
    .description = $("Deals double damage to opponents\n"
                     "being switched in."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
        },
};

static const Ability WaterBubble = {
    .name = $("Water Bubble"),
    .description = $("Halves Fire dmg taken, no burns,\n"
                     "doubles power of its Water moves."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) MUL(2.0);
        },
    .onDefensiveMultiplier = Heatproof.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability Steelworker = {
    .name = $("Steelworker"),
    .description = $("Boosts the power of Steel-type\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_STEEL) MUL(1.3);
        },
};

static const Ability Berserk = {
    .name = $("Berserk"),
    .description = $("Boosts Sp. Atk by one stage when\n"
                     "at 1/2 of max HP or lower."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK(CanRaiseStat(battler, STAT_SPATK))

        SetAbilityState(battler, ability, TRUE);
        SetStatChanger(STAT_SPATK, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

static const Ability SlushRush = {
    .name = $("Slush Rush"),
    .description = $("This Pokémon's Speed gets a\n"
                     "1.5x boost in hail."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
};

static const Ability LongReach = {
    .name = $("Long Reach"),
    .description = $("Doesn't make contact. Boosts\n"
                     "Phys. non-contact moves by 1.2x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move) && !(gBattleMoves[move].flags & FLAG_MAKES_CONTACT)) MUL(1.2);
        },
};

static const Ability LiquidVoice = {
    .name = $("Liquid Voice"),
    .description = $("Sound moves get a 1.2x boost\n"
                     "and become Water if Normal."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(1.2);
        },
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        return TYPE_WATER + 1;
    },
};

static const Ability Triage = {
    .name = $("Triage"),
    .description = $("Moves that have a healing effect\n"
                     "gain +3 priority."),
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IsHealingMoveEffect(gBattleMoves[move].effect))
        return 3;
    },
};

static const Ability Galvanize = {
    .name = $("Galvanize"),
    .description = $("Normal-type moves become Elec.-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_ELECTRIC),
};

static const Ability SurgeSurfer = {
    .name = $("Surge Surfer"),
    .description = $("If Electric Terrain is active,\n"
                     "gets a 1.5x Speed boost."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsWeatherActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
        },
};

static const Ability Schooling = {
    .name = $("Schooling"),
    .description = $("If Lv. 20 or more: changes into\n"
                     "School form until 1/4 HP or less."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gBattleMons[battler].level >= 20)
        return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gBattleMons[battler].level >= 20)
        return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static int DisguiseReformHandler(int ability, int battler, AbilityCallType callType) {
    int newSpecies;
    switch (gBattleMons[battler].species) {
        case SPECIES_MIMIKYU_BUSTED:
            newSpecies = SPECIES_MIMIKYU;
            break;
        case SPECIES_MIMIKYU_RAYQUAZA_BUSTED:
            newSpecies = SPECIES_MIMIKYU_RAYQUAZA;
            break;

        default:
            return FALSE;
    }
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

    InsertCorrectEndType(callType);
    UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
    gBattleMons[battler].species = newSpecies;
    BattleScriptCall(BattleScript_AttackerFormChange);
    return TRUE;
}
static const Ability Disguise = {
    .name = $("Disguise"),
    .description = $("Protects once against an attack.\n"
                     "Restores protection in fog."),
    .onEntry = +[](ON_ENTRY) -> int { return DisguiseReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDisguise = +[](ON_DISGUISE) -> int {
        switch (gBattleMons[battler].species) {
            case SPECIES_MIMIKYU:
                return SPECIES_MIMIKYU_BUSTED;
            case SPECIES_MIMIKYU_RAYQUAZA:
                return SPECIES_MIMIKYU_RAYQUAZA_BUSTED;

            default:
                return SPECIES_NONE;
        }
    },
    .onWeather = +[](ON_WEATHER) -> int { return DisguiseReformHandler(ability, battler, ABILITY_BS_CALL); },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability BattleBond = {
    .name = $("Battle Bond"),
    .description = $("Transforms into Battle Bond form\n"
                     "after dealing a KO."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int newSpecies = 0;
        switch (gBattleMons[battler].species) {
            case SPECIES_GRENINJA_BATTLE_BOND:
                newSpecies = SPECIES_GRENINJA_ASH;
                break;

            case SPECIES_DARMANITAN_REDUX_BOND:
                newSpecies = SPECIES_DARMANITAN_REDUX_BLUNDER;
                break;
        }

        CHECK(newSpecies)

        PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[battler].species);
        gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = gBattleMons[battler].species;
        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptCall(BattleScript_BattleBondActivatesOnMoveEndAttacker);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability PowerConstruct = {
    .name = $("Power Construct"),
    .description = $("At 1/2 of max HP or below,\n"
                     "transforms into Complete form."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_ZYGARDE || gBattleMons[battler].species == SPECIES_ZYGARDE_10)
        CHECK(gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2)
        CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

        gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = gBattleMons[battler].species;
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_ZYGARDE_COMPLETE);
        gBattleMons[battler].species = SPECIES_ZYGARDE_COMPLETE;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability Corrosion = {
    .name = $("Corrosion"),
    .description = $("Steel-types take Supereffective\n"
                     "from Poison. Can poison any type."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_POISON)
        CHECK(defType == TYPE_STEEL)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
};

static const Ability Comatose = {
    .name = $("Comatose"),
    .description = $("Can move, but is always asleep.\n"
                     "Immune to status conditions."),
    .onEntry = +[](ON_ENTRY) -> int {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COMATOSE;
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceStatusAbility);
        return TRUE;
    },
    .unsuppressable = TRUE,
};

static const Ability QueenlyMajesty = {
    .name = $("Queenly Majesty"),
    .description = $("Protects itself and ally from\n"
                     "priority moves."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK_NOT(gProcessingExtraAttacks)
        CHECK(GetBattlerSide(attacker) != GetBattlerSide(battler))
        CHECK(GetMovePriority(attacker, move, battler) > 0);
        *immunityScript = BattleScript_DazzlingProtected;
        return TRUE;
    },
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

static const Ability InnardsOut = {
    .name = $("Innards Out"),
    .description = $("If KO'd, deals as much damage as\n"
                     "what the fatal attack dealt."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))

        gBattleMoveDamage = gTurnStructs[battler].dmg;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    },
};

static const Ability Dancer = {
    .name = $("Dancer"),
    .description = $("Copies dance moves used by\n"
                     "others."),
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(IsDance(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
};

static const Ability Battery = {
    .name = $("Battery"),
    .description = $("Grants a 1.3x power boost to\n"
                     "ally's Special attacks."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
};

static const Ability Fluffy = {
    .name = $("Fluffy"),
    .description = $("Takes 1/2 dmg from contact moves\n"
                     "but Fire moves hurt it 2x more."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

static const Ability Dazzling = {
    .name = $("Dazzling"),
    .description = $("Protects itself and ally from\n"
                     "priority moves."),
    .onImmune = QueenlyMajesty.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

static const Ability SoulHeart = {
    .name = $("Soul-Heart"),
    .description = $("KOs dealt anywhere on the field\n"
                     "raise Sp. Atk by one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
};

static const Ability TanglingHair = {
    .name = $("Tangling Hair"),
    .description = $("Lowers Speed of enemies that\n"
                     "make contact with this Pokémon."),
    .onDefender = Gooey.onDefender,
};

static const Ability Receiver = {
    .name = $("Receiver"),
    .description = $("In Double Battles, copies its\n"
                     "fainting partner's ability."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int allyAbility = GetBattlerAbility(fainted);
        CHECK_NOT(IsRolePlayBannedAbility(allyAbility))
        CHECK_NOT(HasAbilityIgnoringSuppression(battler, allyAbility))
        int index = GetAbilityIndex(battler, ability, FALSE);
        CHECK(index < TOTAL_ABILITY_COUNT)

        gBattleMons[battler].abilities[index] = allyAbility;
        gVolatileStructs[battler].switchInAbilityDone[index] = FALSE;

        gBattleScripting.abilityPopupOverwrite = allyAbility;
        BattleScriptCall(BattleScript_ReceiverActivates);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ALLY,
};

static const Ability PowerOfAlchemy = {
    .name = $("Power of Alchemy"),
    .description = $("Transmutes berries on entry.\n"
                     "Transmutes items when lost."),
    .onEntry = +[](ON_ENTRY) -> int {
        int any = FALSE;
        for (int i = GetBattlerSide(BATTLE_OPPOSITE(battler)); i < gBattlersCount; i += 2) {
            FILTER(IsBattlerAlive(i))
            FILTER(ItemId_GetPocket(GetBattlerHoldEffect(i, FALSE)) == POCKET_BERRIES)
            any = TRUE;
            UpdateBattlerItem(i, ITEM_BLACK_SLUDGE);
            BattleScriptPushCursorAndCallback(BattleScript_End3);
            BattleScriptCall(BattleScript_PowerOfAlchemySludgeNoPopup);
        }
        CHECK(any)
        return TRUE;
    },
    .onReactive = +[](ON_REACTIVE) -> int {
        int any = FALSE;
        int state = GetAbilityState(battler, ability);
        CHECK(state)

        for (int target = 0; target < gBattlersCount; target++) {
            int item = state & 3;
            state = state >> 2;
            FILTER(item)
            FILTER_NOT(gBattleMons[target].item)
            gStackBattler1 = battler;
            gStackBattler2 = target;
            if (!any) {
                InsertCorrectEndType(callType);
                any = TRUE;
            }
            if (item == 1) {
                UpdateBattlerItem(target, ITEM_BLACK_SLUDGE);
                BattleScriptCall(BattleScript_PowerOfAlchemySludge);
            } else {
                UpdateBattlerItem(target, ITEM_BIG_NUGGET);
                BattleScriptCall(BattleScript_PowerOfAlchemyGold);
            }
        }
        SetAbilityState(battler, ABILITY_POWER_OF_ALCHEMY, 0);
        return any;
    },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int state = GetAbilityState(battler, ability);
        if (state & (3 << fainted)) SetAbilityState(battler, ability, state & ~(3 << fainted));
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_OTHER,
};

static const Ability BeastBoost = {
    .name = $("Beast Boost"),
    .description = $("Dealing a KO raises highest\n"
                     "calculated stat by one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, GetHighestStatId(battler, FALSE)); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability RksSystem = {
    .name = $("RKS System"),
    .description = $("Held Memory determines its type.\n"
                     "Also has Protean + Adaptability."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .protean = TRUE,
    .adaptability = TRUE,
};

static const Ability ElectricSurge = {
    .name = $("Electro Surge"),
    .description = $("Casts Electric Terrain on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

        for (int i = 0; i < gBattlersCount; i++) {
            DisableSwitchInAbility(i, ABILITY_GENERATOR);
            DisableSwitchInAbility(i, ABILITY_ENERGIZED);
        }
        BattleScriptPushCursorAndCallback(BattleScript_ElectricSurgeActivates);
        return TRUE;
    },
};

static const Ability PsychicSurge = {
    .name = $("Psychic Surge"),
    .description = $("Casts Psychic Terrain on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptPushCursorAndCallback(BattleScript_PsychicSurgeActivates);
        return TRUE;
    },
};

static const Ability MistySurge = {
    .name = $("Misty Surge"),
    .description = $("Casts Misty Terrain on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptPushCursorAndCallback(BattleScript_MistySurgeActivates);
        return TRUE;
    },
};

static const Ability GrassySurge = {
    .name = $("Grassy Surge"),
    .description = $("Casts Grassy Terrain on entry.\n"
                     "Lasts 8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptPushCursorAndCallback(BattleScript_GrassySurgeActivates);
        return TRUE;
    },
};

static const Ability FullMetalBody = {
    .name = $("Full Metal Body"),
    .description = $("Immune to stat drops."),
};

static const Ability ShadowShield = {
    .name = $("Shadow Shield"),
    .description = $("At full HP, halves damage taken\n"
                     "from attacks"),
    .onDefensiveMultiplier = Multiscale.onDefensiveMultiplier,
};

static const Ability PrismArmor = {
    .name = $("Prism Armor"),
    .description = $("Takes 35% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
};

static const Ability Neuroforce = {
    .name = $("Neuroforce"),
    .description = $("Grants an additional 1.25x boost\n"
                     "to Super-effective moves."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.25);
        },
};

static const Ability IntrepidSword = {
    .name = $("Intrepid Sword"),
    .description = $("On entry, raises Attack by one\n"
                     "stage."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_ATK))

        SetStatChanger(STAT_ATK, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

static const Ability DauntlessShield = {
    .name = $("Dauntless Shield"),
    .description = $("On entry, raises Defense by one\n"
                     "stage."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

static const Ability Libero = {
    .name = $("Libero"),
    .description = $("Before using a move, changes its\n"
                     "type to the move's type."),
    .protean = TRUE,
};

static const Ability BallFetch = {
    .name = $("Ball Fetch"),
    .description = $("No effect in battle."),
};

static const Ability CottonDown = {
    .name = $("Cotton Down"),
    .description = $("Lowers the Speed of all foes\n"
                     "by one stage when hit."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit());
        gStackBattler1 = BATTLE_OPPOSITE(battler);
        CHECK(IsBattlerAlive(gStackBattler1) || IsBattlerAlive(BATTLE_PARTNER(gStackBattler1)))

        gEffectBattler = battler;
        gStackBattler1 = BATTLE_OPPOSITE(GetBattlerSide(battler));
        BattleScriptCall(BattleScript_CottonDownActivates);
        return TRUE;
    },
};

static const Ability PropellerTail = {
    .name = $("Propeller Tail"),
    .description = $("Isn't affected by target\n"
                     "redirection."),
};

static const Ability MirrorArmor = {
    .name = $("Mirror Armor"),
    .description = $("Bounces back any stat drops\n"
                     "inflicted by an enemy."),
    .breakable = TRUE,
};

static const Ability GulpMissile = {
    .name = $("Gulp Missile"),
    .description = $("Gulps a prey after Dive/Surf.\n"
                     "If hit, shoots prey at enemy."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)
        CHECK(gBattleMons[battler].species == SPECIES_CRAMORANT)
        CHECK(((gCurrentMove == MOVE_SURF || gCurrentMove == MOVE_TRIPLE_DIVE) && TARGET_TURN_DAMAGED) || gStatuses3[battler] & STATUS3_UNDERWATER ||
              (gCurrentMove == MOVE_DIVE && gBattleScripting.acceleratedTwoTurn))

        u16 newSpecies = gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2 ? SPECIES_CRAMORANT_GORGING : SPECIES_CRAMORANT_GULPING;
        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        int species = gBattleMons[battler].species;
        CHECK(species == SPECIES_CRAMORANT_GORGING || species == SPECIES_CRAMORANT_GULPING)
        gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = species;
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_CRAMORANT);
        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        BattleScriptCall(species == SPECIES_CRAMORANT_GORGING ? BattleScript_GulpMissileGorging : BattleScript_GulpMissileGulping);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability Stalwart = {
    .name = $("Stalwart"),
    .description = $("Isn't affected by target\n"
                     "redirection."),
};

static const Ability SteamEngine = {
    .name = $("Steam Engine"),
    .description = $("Maximizes Speed if hit by a\n"
                     "Fire-type or Water-type attack."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPEED))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

        SetStatChanger(STAT_SPEED, 12);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

static const Ability PunkRock = {
    .name = $("Punk Rock"),
    .description = $("Sound moves deal 1.3x more dmg.\n"
                     "Takes -50% dmg from sound moves."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(1.3);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_SOUND) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability SandSpit = {
    .name = $("Sand Spit"),
    .description = $("If hit, summons a sandstorm that\n"
                     "lasts 8 turns."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler)) CHECK_NOT(gBattleWeather & WEATHER_SANDSTORM_ANY) if (gBattleWeather & WEATHER_PRIMAL_ANY) {
            BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
            return NO_ANNOUNCE;
        }
        else if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, TRUE)) {
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_SandSpitActivates);
            return TRUE;
        }
        return FALSE;
    },
};

static const Ability IceScales = {
    .name = $("Ice Scales"),
    .description = $("Halves damage taken by Special\n"
                     "moves. Does NOT double SpDef."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability Ripen = {
    .name = $("Ripen"),
    .description = $("Doubles resistance, healing and\n"
                     "stat raises provided by Berries."),
};

int IceFaceReformHandler(int ability, int battler, AbilityCallType callType) {
    CHECK(gBattleMons[battler].species == SPECIES_EISCUE_NOICE_FACE)
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

    InsertCorrectEndType(callType);
    UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_EISCUE);
    gBattleMons[battler].species = SPECIES_EISCUE;
    BattleScriptCall(BattleScript_AttackerFormChange);
    return TRUE;
}
static const Ability IceFace = {
    .name = $("Ice Face"),
    .description = $("Protects once against an attack.\n"
                     "Restores protection under hail."),
    .onEntry = +[](ON_ENTRY) -> int { return IceFaceReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDisguise = +[](ON_DISGUISE) -> int { return gBattleMons[battler].species == SPECIES_EISCUE ? SPECIES_EISCUE_NOICE_FACE : SPECIES_NONE; },
    .onWeather = +[](ON_WEATHER) -> int { return IceFaceReformHandler(ability, battler, ABILITY_BS_CALL); },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability PowerSpot = {
    .name = $("Power Spot"),
    .description = $("Grants a 1.3x boost to ally's\n"
                     "attacks."),
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MUL(1.3); },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
};

static const Ability Mimicry = {
    .name = $("Mimicry"),
    .description = $("Changes type depending on active\n"
                     "Terrain."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        TryToApplyMimicry(battler, FALSE);
        return TRUE;
    },
};

static const Ability ScreenCleaner = {
    .name = $("Screen Cleaner"),
    .description = $("Clears screens and Aurora Veil\n"
                     "from both sides on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryRemoveScreens(battler))

        return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
    },
};

static const Ability SteelySpirit = {
    .name = $("Steely Spirit"),
    .description = $("Boosts own & ally's Steel-type\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_STEEL) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY,
};

static const Ability PerishBody = {
    .name = $("Perish Body"),
    .description = $("If hit, casts Perish Song."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(IsBattlerAlive(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_PERISH_SONG)

        if (!(gStatuses3[battler] & STATUS3_PERISH_SONG)) {
            gStatuses3[battler] |= STATUS3_PERISH_SONG;
            gVolatileStructs[battler].perishSongTimer = 3;
            gVolatileStructs[battler].perishSongTimerStartValue = 3;
        }
        gStatuses3[attacker] |= STATUS3_PERISH_SONG;
        gVolatileStructs[attacker].perishSongTimer = 3;
        gVolatileStructs[attacker].perishSongTimerStartValue = 3;
        BattleScriptCall(BattleScript_PerishBodyActivates);
        return TRUE;
    },
};

static const Ability WanderingSpirit = {
    .name = $("WandrngSprit"),
    .description = $("Trades ability with attacker on\n"
                     "contact."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(GetBattlerAbility(battler) == ability)
        CHECK_NOT(BattlerHasAbility(attacker, ability, FALSE))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(attacker)))
        CHECK_NOT(DoesBattlerHaveAbilityShield(attacker))

        UpdateAbilityStateIndicesForNewAbility(attacker, GetBattlerAbility(attacker));
        UpdateAbilityStateIndicesForNewAbility(battler, ability);
        ReplaceAbility(battler, GetBattlerAbility(attacker));
        ReplaceAbility(attacker, ability);

        BattleScriptCall(BattleScript_WanderingSpiritActivates);

        gBattleScripting.abilityPopupOverwrite = GetBattlerAbility(attacker);
        gStackBattler1 = battler;
        BattleScriptCall(BattleScript_WanderingSpiritSwap);

        gBattleScripting.abilityPopupOverwrite = GetBattlerAbility(battler);
        gStackBattler1 = attacker;
        BattleScriptCall(BattleScript_WanderingSpiritSwap);
        return NO_ANNOUNCE;
    },
};

static const Ability GorillaTactics = {
    .name = $("Gorilla Tactics"),
    .description = $("Raises own Atk by 1.5x, but can\n"
                     "only use the first chosen move."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

static const Ability NeutralizingGas = {
    .name = $("Neutralizing Gas"),
    .description = $("All abilities are nullified."),
    .unsuppressable = TRUE,
};

static const Ability PastelVeil = {
    .name = $("Pastel Veil"),
    .description = $("Casts Safeguard on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_SAFEGUARD)

        int side = GetBattlerSide(battler);
        gSideTimers[side].started.safeguard = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_SAFEGUARD;
        gSideTimers[side].safeguardBattlerId = battler;
        gSideTimers[side].safeguardTimer = SCREEN_DURATION;
        BattleScriptPushCursorAndCallback(BattleScript_PastelVeilActivated);
        return TRUE;
    },
};

static const Ability HungerSwitch = {
    .name = $("HungerSwitch"),
    .description = $("Changes between Full and Hangry\n"
                     "forms after each turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED) int newSpecies;
        switch (gBattleMons[battler].species) {
            case SPECIES_MORPEKO:
                newSpecies = SPECIES_MORPEKO_HANGRY;
                break;
            case SPECIES_MORPEKO_HANGRY:
                newSpecies = SPECIES_MORPEKO;
                break;
            case SPECIES_MORPEKYLL:
                newSpecies = SPECIES_MORPEKYLL_HANGRY;
                break;
            case SPECIES_MORPEKYLL_HANGRY:
                newSpecies = SPECIES_MORPEKYLL;
                break;

            default:
                return FALSE;
        }

        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability QuickDraw = {
    .name = $("Quick Draw"),
    .description = $("30% chance to move first."),
};

static const Ability UnseenFist = {
    .name = $("Unseen Fist"),
    .description = $("Its contact moves hit enemies,\n"
                     "even if they protect themselves."),
};

static const Ability CuriousMedicine = {
    .name = $("CuriusMedicn"),
    .description = $("Resets its ally's stat changes\n"
                     "on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsDoubleBattle())
        CHECK(IsBattlerAlive(BATTLE_PARTNER(battler)))
        CHECK(TryResetBattlerStatChanges(BATTLE_PARTNER(battler), RESET_ALL_STATS))

        gEffectBattler = BATTLE_PARTNER(battler);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CURIOUS_MEDICINE;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
        return TRUE;
    },
};

static const Ability Transistor = {
    .name = $("Transistor"),
    .description = $("Boosts the power of Electric-\n"
                     "type moves by 1.5x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.5);
        },
};

static const Ability DragonsMaw = {
    .name = $("Dragon's Maw"),
    .description = $("Boosts the power of Dragon-type\n"
                     "moves by 1.5x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_DRAGON) MUL(1.5);
        },
};

static const Ability ChillingNeigh = {
    .name = $("ChillngNeigh"),
    .description = $("KOs raise Attack by one stage."),
    .onBattlerFaints = Moxie.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability GrimNeigh = {
    .name = $("Grim Neigh"),
    .description = $("KOs raise Sp. Atk by one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_SPATK); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability AsOneIceRider = {
    .name = $("As One"),
    .description = $("Unnerve + Chilling Neigh."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE); },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChillingNeigh.onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_CHILLING_NEIGH;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability AsOneShadowRider = {
    .name = $("As One"),
    .description = $("Unnerve + Grim Neigh."),
    .onEntry = AsOneIceRider.onEntry,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(GrimNeigh.onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_GRIM_NEIGH;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability Chloroplast = {
    .name = $("Chloroplast"),
    .description = $("Weather Ball, Solar Beam/Blade,\n"
                     "Growth act as if used in sun."),
    .chloroplast = TRUE,
};

static const Ability Whiteout = {
    .name = $("Whiteout"),
    .description = $("Ups highest attacking stat\n"
                     "by 1.5x in hail."),
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
};

static const Ability Pyromancy = {
    .name = $("Pyromancy"),
    .description = $("Moves inflict burn 5x as often."),
};

static const Ability KeenEdge = {
    .name = $("Keen Edge"),
    .description = $("Boosts the power of slashing\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
        },
};

static const Ability PrismScales = {
    .name = $("Prism Scales"),
    .description = $("Takes 30% less damage from\n"
                     "Special attacks."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.7);
        },
    .breakable = TRUE,
};

static const Ability PowerFists = {
    .name = $("Power Fists"),
    .description = $("Iron Fist moves target Special\n"
                     "Defense and get a 1.3x boost."),
    .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(IS_IRON_FIST(battler, move))
        return STAT_SPDEF;
    },
};

static const Ability SandSong = {
    .name = $("Sand Song"),
    .description = $("Sound moves get a 1.2x boost\n"
                     "and become Ground if Normal."),
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_GROUND + 1;
    },
};

static const Ability Rampage = {
    .name = $("Rampage"),
    .description = $("No recharge after a KO, if it\n"
                     "usually would need to recharge."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        SetAbilityState(battler, ability, TRUE);
        gVolatileStructs[battler].rechargeTimer = 0;
        gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
        return FALSE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Vengeance = {
    .name = $("Vengeance"),
    .description = $("Boosts Ghost-type moves by 1.2x,\n"
                     "or 1.5x when below 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GHOST),
};

static const Ability BlitzBoxer = {
    .name = $("Blitz Boxer"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's punching moves."),
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IS_IRON_FIST(battler, move))
        CHECK(BATTLER_MAX_HP(battler));
        return 1;
    },
};

static const Ability AntarcticBird = {
    .name = $("Antarctic Bird"),
    .description = $("Ice-type and Flying-type moves\n"
                     "get a 1.3x power boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
        },
};

static const Ability Immolate = {
    .name = $("Immolate"),
    .description = $("Normal-type moves become Fire-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_FIRE),
};

static const Ability Crystallize = {
    .name = $("Crystallize"),
    .description = $("Rock-type moves become Ice-type\n"
                     "moves and get a 1.1x boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ICE && gBattleStruct->ateBoost[battler]) MUL(1.1);
        },
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_ROCK)
        *ateBoost = TRUE;
        return TYPE_ICE + 1;
    },
};

static const Ability Electrocytes = {
    .name = $("Electrocytes"),
    .description = $("Boosts the power of Electric-\n"
                     "type moves by 1.25x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.25);
        },
};

static const Ability Aerodynamics = {
    .name = $("Aerodynamics"),
    .description = $("Boosts Speed instead of being\n"
                     "hit by Flying-type moves."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FLYING);
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

static const Ability ChristmasSpirit = {
    .name = $("Christmas Spirit"),
    .description = $("Takes 50% less damage if hail is\n"
                     "active."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability ExploitWeakness = {
    .name = $("Exploit Weakness"),
    .description = $("Moves are 1.25x stronger on foes\n"
                     "affected by a status condition."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(target)) MUL(1.25);
        },
};

static const Ability GroundShock = {
    .name = $("Ground Shock"),
    .description = $("Target Grounds aren't immune to\n"
                     "Electric but resist it instead."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_GROUND)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(.5);
        return TRUE;
    },
};

static const Ability AncientIdol = {
    .name = $("Ancient Idol"),
    .description = $("Uses Def and Sp. Def instead of\n"
                     "Atk and Sp. Atk when attacking."),
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; },
};

static const Ability MysticPower = {
    .name = $("Mystic Power"),
    .description = $("All moves gain the 1.5x power\n"
                     "boost from STAB."),
    .onStab = +[](ON_STAB) -> int { return TRUE; },
};

static const Ability Perfectionist = {
    .name = $("Perfectionist"),
    .description = $("Move BP < 51 BP: +1 to crit rate.\n"
                     "Move BP < 26 BP: +1 priority too."),
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMoves[move].power <= 25)
        CHECK(gBattleMoves[move].power);
        return 1;
    },
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gBattleMoves[move].power <= 50)
        CHECK(gBattleMoves[move].power)
        return 1;
    },
};

static const Ability GrowingTooth = {
    .name = $("Growing Tooth"),
    .description = $("Raises Attack by one stage after\n"
                     "using a biting move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_AttackBoostActivates);
        return TRUE;
    },
};

static const Ability Inflatable = {
    .name = $("Inflatable"),
    .description = $("Ups Def and Sp. Def by one stage\n"
                     "if hit by Flying or Fire moves."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
        BattleScriptCall(BattleScript_InflatableActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

static const Ability AuroraBorealis = {
    .name = $("Aurora Borealis"),
    .description = $("Boosts the power of Ice-type\n"
                     "moves by 1.5x (due to STAB)."),
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_ICE; },
};

static const Ability Avenger = {
    .name = $("Avenger"),
    .description = $("If a party Pokémon fainted last\n"
                     "turn, next move gets 1.5x boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
        },
};

static const Ability LetsRoll = {
    .name = $("Let's Roll"),
    .description = $("Casts Defense Curl on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
        return TRUE;
    },
};

static const Ability Aquatic = {
    .name = $("Aquatic"),
    .description = $("Adds Water type to itself."),
};

static const Ability LoudBang = {
    .name = $("Loud Bang"),
    .description = $("Sound-based moves have 50%\n"
                     "chance to confuse the foe."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeConfused(target))
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
    },
};

static const Ability LeadCoat = {
    .name = $("Lead Coat"),
    .description = $("Takes 40% less from Phys. moves.\n"
                     "This Pokémon's Speed is 0.9x."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(.6);
        },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= .9;
        },
    .breakable = TRUE,
};

static const Ability Amphibious = {
    .name = $("Amphibious"),
    .description = $("Boosts the power of Water-type\n"
                     "moves by 1.5x (due to STAB)."),
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
};

static const Ability Grounded = {
    .name = $("Grounded"),
    .description = $("Adds Ground type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GROUND); },
};

static const Ability Earthbound = {
    .name = $("Earthbound"),
    .description = $("Boosts Ground-type moves by\n"
                     "1.2x, or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GROUND),
};

static const Ability FightingSpirit = {
    .name = $("Fighting Spirit"),
    .description = $("Normal-type moves become Fight.-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_FIGHTING),
};

static const Ability FelineProwess = {
    .name = $("Feline Prowess"),
    .description = $("Doubles own Sp. Atk stat.\n"
                     "Boosts raw stat, not base stat."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK) *stat *= 2;
        },
};

static const Ability CoilUp = {
    .name = $("Coil Up"),
    .description = $("On entry, gives +1 priority once\n"
                     "to the first biting move used."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

        gStatuses4[battler] |= STATUS4_COILED;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
        return TRUE;
    },
};

static const Ability Fossilized = {
    .name = $("Fossilized"),
    .description = $("Halves dmg taken by Rock moves.\n"
                     "Boosts own Rock moves by 1.2x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ROCK) MUL(1.2);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ROCK) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability MagicalDust = {
    .name = $("Magical Dust"),
    .description = $("If hit by a contact move, gives\n"
                     "Psychic type to the attacker."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_PSYCHIC))

        gBattleMons[attacker].type3 = TYPE_PSYCHIC;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[attacker].type3);
        BattleScriptCall(BattleScript_AttackerBecameTheType);
        return TRUE;
    },
};

static const Ability Dreamcatcher = {
    .name = $("Dreamcatcher"),
    .description = $("Doubles move power if anyone on\n"
                     "the field is asleep."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            for (int i = 0; i < gBattlersCount; i++) {
                if (IsBattlerAlive(i) && gBattleMons[i].status1 & STATUS1_SLEEP) {
                    MUL(2.0);
                    return;
                }
            }
        },
};

static const Ability Nocturnal = {
    .name = $("Nocturnal"),
    .description = $("Boosts own Dark moves by 1.25x.\n"
                     "Takes -25% dmg from Dark/Fairy."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_DARK) MUL(1.25);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
        },
    .breakable = TRUE,
};

static const Ability SelfSufficient = {
    .name = $("Self Sufficient"),
    .description = $("Recovers 1/16 of max HP at the\n"
                     "end of each turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)

        gBattleMoveDamage = gBattleMons[battler].maxHP / 16;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    },
};

static const Ability Tectonize = {
    .name = $("Tectonize"),
    .description = $("Normal-type moves become Ground-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_GROUND),
};

static const Ability IceAge = {
    .name = $("Ice Age"),
    .description = $("Adds Ice type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ICE); },
};

static const Ability HalfDrake = {
    .name = $("Half Drake"),
    .description = $("Adds Dragon type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_DRAGON); },
};

static const Ability Liquified = {
    .name = $("Liquified"),
    .description = $("Takes 1/2 dmg from contact moves\n"
                     "but Water moves hurt it 2x more."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) RESISTANCE(2);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

static const Ability Dragonfly = {
    .name = $("Dragonfly"),
    .description = $("Adds Dragon type to itself.\n"
                     "Avoids Ground attacks."),
    .onEntry = HalfDrake.onEntry,
    .breakable = TRUE,
};

static const Ability Dragonslayer = {
    .name = $("Dragonslayer"),
    .description = $("Deals 1.5x damage to Dragons.\n"
                     "Takes .5x damage from Dragons."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_DRAGON)) RESISTANCE(1.5);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_DRAGON)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability Mountaineer = {
    .name = $("Mountaineer"),
    .description = $("Immune to Rock-type attacks and\n"
                     "Stealth Rock damage."),
    .breakable = TRUE,
};

static const Ability Hydrate = {
    .name = $("Hydrate"),
    .description = $("Normal-type moves become Water-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_WATER),
};

static const Ability Metallic = {
    .name = $("Metallic"),
    .description = $("Adds Steel type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_STEEL); },
};

static const Ability Permafrost = {
    .name = $("Permafrost"),
    .description = $("Takes 25% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.75);
        },
    .breakable = TRUE,
};

static const Ability PrimalArmor = {
    .name = $("Primal Armor"),
    .description = $("Takes 50% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability RagingBoxer = {
    .name = $("Raging Boxer"),
    .description = $("Punching moves hit twice. 1st hit\n"
                     "at 100% power, 2nd hit at 40%."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(IS_IRON_FIST(battler, move))
        return PARENTAL_BOND_PRIMAL_MAW;
    },
};

static const Ability AirBlower = {
    .name = $("Air Blower"),
    .description = $("Casts a 3-turn Tailwind on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND) int side = GetBattlerSide(battler);
        gSideTimers[side].started.tailwind = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_TAILWIND;
        gSideTimers[side].tailwindBattlerId = battler;
        gSideTimers[side].tailwindTimer = TAILWIND_DURATION_SHORT;

        DisableSwitchInAbility(battler, ABILITY_WIND_RIDER);
        DisableSwitchInAbility(BATTLE_PARTNER(battler), ABILITY_WIND_RIDER);

        BattleScriptPushCursorAndCallback(BattleScript_AirBlowerActivated);
        return TRUE;
    },
};

static const Ability Juggernaut = {
    .name = $("Juggernaut"),
    .description = $("Paralysis-immune. Uses 20% of its\n"
                     "Def when using a contact move."),
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_MAKES_CONTACT) *secondaryAtkStatToUse = STAT_DEF;
        },
    .breakable = TRUE,
};

static const Ability ShortCircuit = {
    .name = $("Short Circuit"),
    .description = $("Boosts Elec.-type moves by 1.2x,\n"
                     "or 1.5x when below 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_ELECTRIC),
};

static const Ability MajesticBird = {
    .name = $("Majestic Bird"),
    .description = $("Boosts own Sp. Atk by 1.5x.\n"
                     "Boosts raw stat, not base stat."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK) *stat *= 1.5;
        },
};

static const Ability Phantom = {
    .name = $("Phantom"),
    .description = $("Adds Ghost type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GHOST); },
};

static const Ability Intoxicate = {
    .name = $("Intoxicate"),
    .description = $("Normal-type moves become Poison-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_POISON),
};

static const Ability Impenetrable = {
    .name = $("Impenetrable"),
    .description = $("Only damaged by attacks."),
    .magicGuard = TRUE,
};

static const Ability Hypnotist = {
    .name = $("Hypnotist"),
    .description = $("Hypnosis accuracy is 90% when\n"
                     "used by this Pokémon."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_HYPNOSIS);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    },
};

static const Ability Overwhelm = {
    .name = $("Overwhelm"),
    .description = $("Hits Fairies with Dragon moves.\n"
                     "Immune to Intimidate and Scare."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = 1.0;
        return TRUE;
    },
};

static const Ability Scare = {
    .name = $("Scare"),
    .description = $("Lowers foes' Sp. Atk by one\n"
                     "stage on entry."),
    .onEntry = UseIntimidateClone,
};

static const Ability MajesticMoth = {
    .name = $("Majestic Moth"),
    .description = $("On entry, raises highest\n"
                     "calculated stat by one stage."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

static const Ability SoulEater = {
    .name = $("Soul Eater"),
    .description = $("Dealing a KO heals 1/4 of this\n"
                     "Pokémon's max HP."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler)) CHECK_NOT(BATTLER_HEALING_BLOCKED(battler)) BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

ON_EITHER(SoulLinker) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(IsBattlerAlive(battler))
    CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER))
    CHECK(move != MOVE_PAIN_SPLIT)

    BattleScriptCall(BattleScript_AttackerSoulLinker);
    return TRUE;
}
static const Ability SoulLinker = {
    .name = $("Soul Linker"),
    .description = $("Enemies take all the damage they\n"
                     "deal, same for this Pokémon."),
    ON_EITHER_ABILITY(SoulLinker),
};

static const Ability SweetDreams = {
    .name = $("Sweet Dreams"),
    .description = $("Heals 1/8 of max HP every turn\n"
                     "if asleep. Immune to Bad Dreams."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gBattleMons[battler].status1 & STATUS1_SLEEP || BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SweetDreamsActivates);
        return TRUE;
    },
};

static const Ability BadLuck = {
    .name = $("Bad Luck"),
    .description = $("Foes hit the lowest damage roll,\n"
                     "have 5% less acc. and can't crit."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= .95;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onCrit = +[](ON_CRIT) -> int { return NEVER_CRIT; },
    .onAccuracyFor = APPLY_ON_FOE,
    .onCritFor = APPLY_ON_FOE,
    .breakable = TRUE,
};

static const Ability HauntedSpirit = {
    .name = $("Haunted Spirit"),
    .description = $("When this Pokémon is KO'd, casts\n"
                     "a Curse on the attacker."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_GHOST))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMons[attacker].status2 |= STATUS2_CURSED;
        BattleScriptCall(BattleScript_HauntedSpiritActivated);
        return TRUE;
    },
};

static const Ability ElectricBurst = {
    .name = $("Electric Burst"),
    .description = $("Boosts own Elec. moves by 1.35x,\n"
                     "takes 10% of dmg dealt as recoil."),
    .onRecoil = +[](ON_RECOIL) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.35);
        },
};

static const Ability RawWood = {
    .name = $("Raw Wood"),
    .description = $("Halves dmg taken by Grass moves.\n"
                     "Boosts own Grass moves by 1.2x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GRASS) MUL(1.2);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GRASS) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability Solenoglyphs = {
    .name = $("Solenoglyphs"),
    .description = $("Biting moves have a 50% chance to\n"
                     "badly poison the target."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
};

static const Ability SpiderLair = {
    .name = $("Spider Lair"),
    .description = $("Casts Sticky Web on entry.\n"
                     "Lasts 5 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

        int side = BATTLE_OPPOSITE(battler);
        gSideTimers[side].started.spiderWeb = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[side].stickyWebTimer = 5;
        BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
        return TRUE;
    },
};

static const Ability FatalPrecision = {
    .name = $("Fatal Precision"),
    .description = $("Super-effective moves never miss\n"
                     "and get a 1.2x boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.2);
        },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK_NOT(IS_MOVE_STATUS(move))
        CHECK(CalcTypeEffectivenessMultiplier(move, moveType, battler, target, TRUE) >= UQ_4_12(2.0))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

static const Ability FortKnox = {
    .name = $("Fort Knox"),
    .description = $("Blocks most damage boosting\n"
                     "and multihit abilities."),
    .fortKnox = TRUE,
};

static const Ability Seaweed = {
    .name = $("Seaweed"),
    .description = $("Takes 1/2 dmg from Fire if Grass,\n"
                     "doubles Grass dmg on Fire-types."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(2);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS)) RESISTANCE(0.5);
        },
    .breakable = TRUE,
};

static const Ability PsychicMind = {
    .name = $("Psychic Mind"),
    .description = $("Boosts Psychic-type moves by\n"
                     "1.2x, or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_PSYCHIC),
};

static const Ability PoisonAbsorb = {
    .name = $("Poison Absorb"),
    .description = $("Heals 25% of max HP when hit\n"
                     "by a Poison-type move."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_POISON)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

static const Ability Scavenger = {
    .name = $("Scavenger"),
    .description = $("Dealing a KO heals 1/4 of this\n"
                     "Pokémon's max HP."),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability TwistedDimension = {
    .name = $("Twist. Dimension"),
    .description = $("Sets up Trick Room on\n"
                     "entry, lasts 3 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

        gFieldTimers.started.trickRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
        return TRUE;
    },
};

static const Ability MultiHeaded = {
    .name = $("Multi Headed"),
    .description = $("Hits as many times,\n"
                     "as it has heads."),
    .onParentalBond =
        +[](int battler, int move, int moveType) {
            if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
            if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
            return MULTIHIT_SINGLE;
        },
    .resistsFortKnox = TRUE,
};

static const Ability NorthWind = {
    .name = $("North Wind"),
    .description = $("3 turns Aurora Veil on entry.\n"
                     "Immune to Hail damage."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_AURORA_VEIL)

        int side = GetBattlerSide(battler);
        gSideTimers[side].started.auroraVeil = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_AURORA_VEIL;
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_LIGHT_CLAY)
            gSideTimers[side].auroraVeilTimer = SCREEN_DURATION;
        else
            gSideTimers[side].auroraVeilTimer = SCREEN_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_NorthWindActivated);

        return TRUE;
    },
};

static const Ability Overcharge = {
    .name = $("Overcharge"),
    .description = $("Electric is super effective vs\n"
                     "Electric. Can paralyze Electric."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_ELECTRIC)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
};

static const Ability ViolentRush = {
    .name = $("Violent Rush"),
    .description = $("Boosts Speed by 50% + Attack\n"
                     "by 20% on first turn."),
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
    },
};

static const Ability FlamingSoul = {
    .name = $("Flaming Soul"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's Fire-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_FIRE),
};

static const Ability SagePower = {
    .name = $("Sage Power"),
    .description = $("Ups Special Attack by 50%\n"
                     "and locks move."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(1.5);
        },
};

static const Ability BoneZone = {
    .name = $("Bone Zone"),
    .description = $("Bone moves ignore immunities and\n"
                     "deal 2x on not very effective."),
};

static const Ability WeatherControl = {
    .name = $("Weather Control"),
    .description = $("Negates all weather based\n"
                     "moves from enemies."),
    .onImmune = DeltaStream.onImmune,
    .breakable = TRUE,
};

static const Ability SpeedForce = {
    .name = $("Speed Force"),
    .description = $("Contact moves use 20% of its\n"
                     "Speed stat additionally."),
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_MAKES_CONTACT) *secondaryAtkStatToUse = STAT_SPEED;
        },
};

static const Ability SeaGuardian = {
    .name = $("Sea Guardian"),
    .description = $("Ups highest stat by +1\n"
                     "on entry when it rains."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        SetStatChanger(stat, 1);
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

static const Ability MoltenDown = {
    .name = $("Molten Down"),
    .description = $("Fire-type is super effective\n"
                     "against Rock-type."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(defType == TYPE_ROCK)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
};

static const Ability HyperAggressive = {
    .name = $("Hyper Aggressive"),
    .description = $("Moves hit twice.\n"
                     "Second hit does 25% damage."),
    .onParentalBond = ParentalBond.onParentalBond,
};

static const Ability Flock = {
    .name = $("Flock"),
    .description = $("Boosts Flying-type moves by 1.2x,\n"
                     "or 1.5x when below 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FLYING),
};

static const Ability FieldExplorer = {
    .name = $("Field Explorer"),
    .description = $("Boosts field moves by 50%.\n"
                     "Cut, Surf, Strength etc."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
        },
};

static const Ability Striker = {
    .name = $("Striker"),
    .description = $("Boosts the power of kicking\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_STRIKER_BOOST) MUL(1.3);
        },
};

static const Ability FrozenSoul = {
    .name = $("Frozen Soul"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's Ice-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_ICE),
};

static const Ability Predator = {
    .name = $("Predator"),
    .description = $("Dealing a KO heals 1/4 of this\n"
                     "Pokémon's max HP."),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Looter = {
    .name = $("Looter"),
    .description = $("Dealing a KO heals 1/4 of this\n"
                     "Pokémon's max HP."),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability LunarEclipse = {
    .name = $("Lunar Eclipse"),
    .description = $("Fairy & Dark gains STAB.\n"
                     "Hypnosis has 1.5x accuracy."),
    .onAccuracy = Hypnotist.onAccuracy,
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; },
};

static const Ability SolarFlare = {
    .name = $("Solar Flare"),
    .description = $("Chloroplast + Immolate.\n"
                     "Fire moves gain STAB."),
    .onOffensiveMultiplier = Immolate.onOffensiveMultiplier,
    .onMoveType = Immolate.onMoveType,
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FIRE; },
    .chloroplast = TRUE,
};

static const Ability PowerCore = {
    .name = $("Power Core"),
    .description = $("The Pokémon uses +20% of its\n"
                     "Defense or SpDef during moves."),
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { *secondaryAtkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; },
};

static const Ability SightingSystem = {
    .name = $("Sighting System"),
    .description = $("Moves always hit. Moves last\n"
                     "for moves less than 80% accuracy."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_HITS_IF_POSSIBLE; },
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMoves[move].accuracy)
        CHECK(gBattleMoves[move].accuracy < 80);
        return -3;
    },
};

static const Ability BadCompany = {
    .name = $("Bad Company"),
    .description = $("Not implemented right now.\n"
                     "Has no effect."),
    .randomizerBanned = TRUE,
};

static const Ability Opportunist = {
    .name = $("Opportunist"),
    .description = $("If target has less than 1/2 HP,\n"
                     "single-target moves get +1 prio."),
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2)
        return 1;
    },
};

static const Ability GiantWings = {
    .name = $("Giant Wings"),
    .description = $("Boosts the power of wing, wind\n"
                     "or air-based moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].airBased) MUL(1.3);
        },
};

static const Ability Momentum = {
    .name = $("Momentum"),
    .description = $("Contact moves use the Speed stat\n"
                     "for damage calculation."),
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_MAKES_CONTACT) *atkStatToUse = STAT_SPEED;
        },
};

static const Ability GripPincer = {
    .name = $("Grip Pincer"),
    .description = $("50% chance to trap. Then ignores\n"
                     "Defense & accuracy checks."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(gBattlerTarget))
        CHECK(IsBattlerAlive(battler))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK_NOT(gBattleMons[target].status2 & STATUS2_WRAPPED)
        CHECK(Random() % 2)

        gBattleMons[target].status2 |= STATUS2_WRAPPED;
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_GRIP_CLAW)
            gVolatileStructs[target].wrapTurns = 7;
        else
            gVolatileStructs[target].wrapTurns = (Random() % 2) + 4;

        gBattleStruct->wrappedMove[target] = gCurrentMove;
        gBattleStruct->wrappedBy[target] = battler;
        BattleScriptCall(BattleScript_GripPincerActivated);
        return TRUE;
    },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMons[target].status2 & STATUS2_WRAPPED)
        return ACCURACY_ALWAYS_HITS;
    },
};

static const Ability BigLeaves = {
    .name = $("Big Leaves"),
    .description = $("Chloroplast + Chlorophyll + Leaf\n"
                     "Guard + Harvest + Solar Power."),
    .onEndTurn = Harvest.onEndTurn,
    .onStat =
        +[](ON_STAT) {
            SolarPower.onStat(DELEGATE_STAT);
            Chlorophyll.onStat(DELEGATE_STAT);
        },
    .breakable = TRUE,
    .chloroplast = TRUE,
};

static const Ability PreciseFist = {
    .name = $("Precise Fist"),
    .description = $("Punching moves get +1 crit\n"
                     "and 5x effect chance."),
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(IS_IRON_FIST(battler, move))
        return 1;
    },
};

static const Ability Deadeye = {
    .name = $("Deadeye"),
    .description = $("Never misses. Arrow and cannon\n"
                     "moves hit weakest defense."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_HITS_IF_POSSIBLE; },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST || gBattleMoves[move].arrowBased)
        u32 def = CalculateStat(target, STAT_DEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        u32 spDef = CalculateStat(target, STAT_SPDEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        if (def < spDef)
            return STAT_DEF;
        else if (spDef < def)
            return STAT_SPDEF;
        else
            return 0;
    },
};

static const Ability Artillery = {
    .name = $("Artillery"),
    .description = $("Mega Launcher moves always hit.\n"
                     "Single-target now hits both foes."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST)
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

static const Ability Amplifier = {
    .name = $("Amplifier"),
    .description = $("Ups sound moves by 30% and\n"
                     "makes them hit both foes."),
    .onOffensiveMultiplier = PunkRock.onOffensiveMultiplier,
};

static const Ability IceDew = {
    .name = $("Ice Dew"),
    .description = $("Boosts highest Atk instead of\n"
                     "being hit by Ice-type moves."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ICE);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

static const Ability SunWorship = {
    .name = $("Sun Worship"),
    .description = $("Ups highest stat by +1\n"
                     "on entry when sunny."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

static const Ability Pollinate = {
    .name = $("Pollinate"),
    .description = $("Normal-type moves become Bug-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_BUG),
};

static const Ability VolcanoRage = {
    .name = $("Volcano Rage"),
    .description = $("Triggers 50 BP Eruption after\n"
                     "using a Fire-type move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
    },
};

static const Ability ColdRebound = {
    .name = $("Cold Rebound"),
    .description = $("Attacks with Icy Wind\n"
                     "when hit by a contact move."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
        return FALSE;
    },
};

static const Ability LowBlow = {
    .name = $("Low Blow"),
    .description = $("Attacks with 40BP Feint\n"
                     "Attack on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FEINT_ATTACK, 40); },
};

static const Ability Nosferatu = {
    .name = $("Nosferatu"),
    .description = $("Contact moves do +20% damage\n"
                     "and heal 1/2 of damage dealt."),
};

static const Ability Spectralize = {
    .name = $("Spectralize"),
    .description = $("Normal-type moves become Ghost-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_GHOST),
};

static const Ability SpectralShroud = {
    .name = $("Spectral Shroud"),
    .description = $("Spectralize + 30% chance\n"
                     "to badly poison the foe."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target))
        CHECK(gBattleStruct->ateBoost[battler])
        CHECK(moveType == TYPE_GHOST)
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
    .onOffensiveMultiplier = Spectralize.onOffensiveMultiplier,
    .onMoveType = Spectralize.onMoveType,
};

static const Ability Discipline = {
    .name = $("Discipline"),
    .description = $("Rampage moves no longer trap you.\n"
                     "Can't be confused or intimidated."),
};

static const Ability Thundercall = {
    .name = $("Thundercall"),
    .description = $("Triggers Smite at 20% power\n"
                     "when using an Electric move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
    },
};

static const Ability MarineApex = {
    .name = $("Marine Apex"),
    .description = $("50% more damage to Water-\n"
                     "types + Infiltrator."),
    .onInfiltrate = Infiltrator.onInfiltrate,
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
        },
};

static const Ability MightyHorn = {
    .name = $("Mighty Horn"),
    .description = $("Boosts the power of horn and\n"
                     "drill-based by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hornBased) MUL(1.3);
        },
};

static const Ability HardenedSheath = {
    .name = $("Hardened Sheath"),
    .description = $("Ups Attack by +1\n"
                     "when using horn moves."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].hornBased)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptCall(BattleScript_AttackBoostActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

static const Ability ArcticFur = {
    .name = $("Arctic Fur"),
    .description = $("Weakens incoming physical\n"
                     "and special moves by 35%."),
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.65); },
    .breakable = TRUE,
};

static const Ability Lethargy = {
    .name = $("Lethargy"),
    .description = $("Damage drops 20% each turn to 20%.\n"
                     "Resets on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        TryResetBattlerStatChanges(battler, RESET_ALL_STATS);
        gVolatileStructs[battler].slowStartTimer = 5;
        BattleScriptPushCursorAndCallback(BattleScript_LethargyEnters);
        return TRUE;
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            switch (gVolatileStructs[battler].slowStartTimer) {
                case 0:
                case 1:
                    MUL(.2);
                    return;

                case 2:
                    MUL(.4);
                    return;

                case 3:
                    MUL(.6);
                    return;

                case 4:
                    MUL(.8);
                    return;
            }
        },
};

static const Ability IronBarrage = {
    .name = $("Iron Barrage"),
    .description = $("Mega Launcher + Sighting System."),
    .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
    .onAccuracy = SightingSystem.onAccuracy,
    .onPriority = SightingSystem.onPriority,
};

static const Ability SteelBarrel = {
    .name = $("Steel Barrel"),
    .description = $("Immune to recoil damage, but not\n"
                     "immune to Explosion/crash dmg."),
    .noRecoil = TRUE,
};

static const Ability PyroShells = {
    .name = $("Pyro Shells"),
    .description = $("Triggers 50 BP Outburst after\n"
                     "using a Mega Launcher move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
    },
};

static const Ability FungalInfection = {
    .name = $("Fungal Infection"),
    .description = $("Contact moves inflict\n"
                     "Leech Seed on the target."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED)
        CHECK(IsMoveMakingContact(move, battler))

        gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    },
};

static const Ability Parry = {
    .name = $("Parry"),
    .description = $("Counters contact with Mach\n"
                     "Punch. Takes 20% less damage."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
        return FALSE;
    },
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.8); },
};

static const Ability Scrapyard = {
    .name = $("Scrapyard"),
    .description = $("Sets a layer of Spikes when hit\n"
                     "(contact move)."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    },
};

static const Ability LooseQuills = {
    .name = $("Loose Quills"),
    .description = $("Sets a layer of Spikes when hit\n"
                     "(contact move)."),
    .onDefender = Scrapyard.onDefender,
};

static const Ability ToxicDebris = {
    .name = $("Toxic Debris"),
    .description = $("Sets a layer of Toxic Spikes\n"
                     "when hit by contact moves."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

        BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
        return TRUE;
    },
};

static const Ability Roundhouse = {
    .name = $("Roundhouse"),
    .description = $("Kicks always hit.\n"
                     "Damages foes' weaker defenses."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMoves[move].flags & FLAG_STRIKER_BOOST)
        return ACCURACY_HITS_IF_POSSIBLE;
    },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_STRIKER_BOOST)
        u32 def = CalculateStat(target, STAT_DEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        u32 spDef = CalculateStat(target, STAT_SPDEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        if (def < spDef)
            return STAT_DEF;
        else if (spDef < def)
            return STAT_SPDEF;
        else
            return 0;
    },
};

static const Ability Mineralize = {
    .name = $("Mineralize"),
    .description = $("Normal-type moves become Rock-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_ROCK),
};

static const Ability LooseRocks = {
    .name = $("Loose Rocks"),
    .description = $("Deploys Stealth Rocks\n"
                     "when hit by contact."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

        BattleScriptCall(BattleScript_DefenderSetsStealthRock);
        return TRUE;
    },
};

static const Ability SpinningTop = {
    .name = $("Spinning Top"),
    .description = $("Fighting moves up speed +1\n"
                     "and clear hazards."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_FIGHTING)
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))

        int any = FALSE;
        if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_HAZARDS_ANY || gSideTimers[GetBattlerSide(battler)].hotCoals ||
            gSideTimers[GetBattlerSide(battler)].caltrops) {
            gSideStatuses[GetBattlerSide(battler)] &=
                ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES_DAMAGED | SIDE_STATUS_STICKY_WEB);
            gSideTimers[GetBattlerSide(battler)].hotCoals = TRUE;
            gSideTimers[GetBattlerSide(battler)].caltrops = TRUE;
            BattleScriptCall(BattleScript_AnnounceRemovedHazards);
            gBattleScripting.battler = battler;
            any = TRUE;
        }

        if (ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL)) {
            BattleScriptCall(BattleScript_AnnounceAbilitySpeedBoost);
            gBattleScripting.battler = battler;
            any = TRUE;
        }

        return any;
    },
};

static const Ability RetributionBlow = {
    .name = $("Retribution Blow"),
    .description = $("Uses Hyper Beam if any foe\n"
                     "uses an stat boosting move."),
    .onReactive = +[](ON_REACTIVE) -> int {
        CHECK_NOT(gTurnStructs[battler].dancerUsedMove)
        CHECK(IsBattlerAlive(gBattlerAttacker))
        CHECK(gCurrentTurnActionNumber < gBattlersCount || gProcessingExtraAttacks)
        CHECK(gBattleStruct->statStageCheckState != STAT_STAGE_CHECK_NOT_NEEDED)
        for (int stat = STAT_ATK; stat < NUM_STATS; stat++) {
            if (gBattleStruct->statChangesToCheck[gBattlerAttacker][stat - 1] > 0) {
                UseOutOfTurnAttack(battler, gBattlerAttacker, ability, MOVE_HYPER_BEAM, 0);
                return FALSE;
            }
        }
        return FALSE;
    },
};

static const Ability Fearmonger = {
    .name = $("Fearmonger"),
    .description = $("Intimidate + Scare; 10%\n"
                     "para chance on contact moves."),
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
};

static const Ability KingsWrath = {
    .name = $("King's Wrath"),
    .description = $("Lowering any stats on its\n"
                     "side raises Atk and Def."),
};

static const Ability QueensMourning = {
    .name = $("Queen's Mourning"),
    .description = $("Lowering any stats on its\n"
                     "side raises SpAtk and SpDef."),
};

static const Ability ToxicSpill = {
    .name = $("Toxic Spill"),
    .description = $("Non-Poison-types take 1/8 dmg\n"
                     "every turn when on field."),
    .onEntry = +[](ON_ENTRY) -> int {
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
        return TRUE;
    },
    .onExit = +[](ON_EXIT) -> int {
        BattleScriptCall(BattleScript_TheToxicWasHasDissapeared);
        return TRUE;
    },
};

static const Ability DesertCloak = {
    .name = $("Desert Cloak"),
    .description = $("Protects its side from status\n"
                     "and secondary effects in sand."),
    .breakable = TRUE,
};

static const Ability Draconize = {
    .name = $("Draconize"),
    .description = $("Normal-type moves become Dragon-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_DRAGON),
};

static const Ability PrettyPrincess = {
    .name = $("Pretty Princess"),
    .description = $("Does 50% more damage if the\n"
                     "target has any lowered stat."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (!IsUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
        },
};

static const Ability SelfRepair = {
    .name = $("Self Repair"),
    .description = $("Self Sufficient + Natural Cure."),
    .onEndTurn = SelfSufficient.onEndTurn,
    .onExit = NaturalCure.onExit,
};

static const Ability AtomicBurst = {
    .name = $("Atomic Burst"),
    .description = $("When hit super-effectively,\n"
                     "triggers 50 BP Hyper Beam."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE)

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_HYPER_BEAM, 50);
        return FALSE;
    },
};

static const Ability Hellblaze = {
    .name = $("Hellblaze"),
    .description = $("Boosts Fire-type moves by 1.3x,\n"
                     "or 1.8x when below 1/3 HP."),
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIRE),
};

static const Ability Riptide = {
    .name = $("Riptide"),
    .description = $("Boosts Water-type moves by 1.3x,\n"
                     "or 1.8x when below 1/3 HP."),
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_WATER),
};

static const Ability ForestRage = {
    .name = $("Forest Rage"),
    .description = $("Boosts Grass-type moves by 1.3x,\n"
                     "or 1.8x when below 1/3 HP."),
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GRASS),
};

static const Ability PrimalMaw = {
    .name = $("Primal Maw"),
    .description = $("Biting moves hit twice.\n"
                     "2nd hit does 0.4x damage."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        return PARENTAL_BOND_PRIMAL_MAW;
    },
};

static const Ability SweepingEdge = {
    .name = $("Sweeping Edge"),
    .description = $("Keen Edge moves always hit.\n"
                     "Single-target now hits both foes."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

static const Ability GiftedMind = {
    .name = $("Gifted Mind"),
    .description = $("Nulls Psychic weakness;\n"
                     "status moves always hit."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IS_MOVE_STATUS(move))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
    .breakable = TRUE,
};

static const Ability HydroCircuit = {
    .name = $("Hydro Circuit"),
    .description = $("Electric moves +50%;\n"
                     "Water moves siphon 25% damage."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(moveType == TYPE_WATER)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
    .onOffensiveMultiplier = Transistor.onOffensiveMultiplier,
};

static const Ability Equinox = {
    .name = $("Equinox"),
    .description = $("Boosts Atk or SpAtk to\n"
                     "match the higher value."),
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
            int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
            if (atk > spAtk)
                *atkStatToUse = STAT_ATK;
            else if (spAtk > atk)
                *atkStatToUse = STAT_SPATK;
        },
};

static const Ability Absorbant = {
    .name = $("Absorbant"),
    .description = $("Drain moves recover +50%\n"
                     "HP & apply Leech Seed."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED)
        CHECK(gBattleMoves[move].effect == EFFECT_ABSORB || gBattleMoves[move].effect == EFFECT_DREAM_EATER)

        gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    },
};

static const Ability Clueless = {
    .name = $("Clueless"),
    .description = $("Negates Weather, Rooms\n"
                     "and Terrains."),
    .onEntry = CloudNine.onEntry,
    .unsuppressable = TRUE,
};

static const Ability CheatingDeath = {
    .name = $("Cheating Death"),
    .description = $("Gets no damage for\n"
                     "the first two hits."),
    .onEntry = +[](ON_ENTRY) -> int {
        int uses = 2 - GetSingleUseAbilityCounter(battler, ability);
        CHECK(uses)

        if (uses == 1)
            BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
        else if (uses > 1) {
            ConvertIntToDecimalStringN(gBattleTextBuff4, uses, STR_CONV_MODE_LEFT_ALIGN, 2);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerHasNoDamageHits);
        }
        return TRUE;
    },
    .noDamageHits = 2,
    .persistent = TRUE,
};

static const Ability CheapTactics = {
    .name = $("Cheap Tactics"),
    .description = $("Attacks with Scratch\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SCRATCH, 0); },
};

static const Ability Coward = {
    .name = $("Coward"),
    .description = $("Sets up Protect on switch-in.\n"
                     "Only works once."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        SetSingleUseAbilityCounter(battler, ability, TRUE);
        gRoundStructs[battler].protectedThisTurn = TRUE;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
        return TRUE;
    },
    .persistent = TRUE,
};

static const Ability VoltRush = {
    .name = $("Volt Rush"),
    .description = $("At full HP, gives +1 priority to\n"
                     "its Electric-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_ELECTRIC),
};

static const Ability DuneTerror = {
    .name = $("Dune Terror"),
    .description = $("Sand reduces damage by 35%.\n"
                     "Boosts Ground moves by 20%."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GROUND) MUL(1.2);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
        },
    .breakable = TRUE,
};

static const Ability InfernalRage = {
    .name = $("Infernal Rage"),
    .description = $("Fire-type moves are boosted\n"
                     "by 35% with 5% recoil."),
    .onRecoil = +[](ON_RECOIL) -> int {
        CHECK(moveType == TYPE_FIRE);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) MUL(1.35);
        },
};

static const Ability DualWield = {
    .name = $("Dual Wield"),
    .description = $("Mega Launcher and Keen Edge\n"
                     "moves hit twice for 70% damage."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

static const Ability ElementalCharge = {
    .name = $("Elemental Charge"),
    .description = $("20% chance to BRN/FRZ/PARA\n"
                     "with respective types."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 100 < 20)

        switch (moveType) {
            case TYPE_ELECTRIC:
                CHECK(CanBeParalyzed(battler, target))

                AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
                return TRUE;

            case TYPE_FIRE:
                CHECK(CanBeBurned(target))

                AbilityStatusEffect(MOVE_EFFECT_BURN);
                return TRUE;

            case TYPE_ICE:
                CHECK(CanGetFrostbite(target))

                AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
                return TRUE;
        }
        return FALSE;
    },
};

static const Ability Ambush = {
    .name = $("Ambush"),
    .description = $("Guaranteed critical hit\n"
                     "on first turn."),
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn)
        return ALWAYS_CRIT;
    },
};

static const Ability Atlas = {
    .name = $("Atlas"),
    .description = $("Sets Gravity on entry for\n"
                     "8 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    },
};

static const Ability Radiance = {
    .name = $("Radiance"),
    .description = $("+20% accuracy; Dark moves\n"
                     "fail when user is present."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(moveType == TYPE_DARK);
        *immunityScript = BattleScript_RadianceProtected;
        return TRUE;
    },
    .onAccuracy = Illuminate.onAccuracy,
    .onImmuneFor = APPLY_ON_ANY,
    .breakable = TRUE,
};

static const Ability JawsOfCarnage = {
    .name = $("Jaws of Carnage"),
    .description = $("Devours 1/2 of the foe\n"
                     "when defeating it."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        if (gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST)
            BattleScriptCall(BattleScript_HandleJawsOfCarnageEffect);
        else
            BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability AngelsWrath = {
    .name = $("Angel's Wrath"),
    .description = $("Drastically alters all\n"
                     "of the users moves."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        switch (move) {
            case MOVE_TACKLE: {
                CHECK(ShouldApplyOnHitAffect(target))
                CHECK(gVolatileStructs[target].encoreTimer)
                CHECK(gVolatileStructs[target].disableTimer)

                gVolatileStructs[target].encoreTimer = 2;
                gVolatileStructs[target].encoredMove = gBattleMons[target].moves[0];

                gVolatileStructs[target].disableTimer = gVolatileStructs[target].disableTimerStartValue = 2;
                gVolatileStructs[target].disabledMove = gBattleMons[target].moves[0];

                BattleScriptCall(BattleScript_AngelsWrath_Effect_Tackle);
                return TRUE;
            }

            case MOVE_STRING_SHOT: {
                CHECK(WasMoveSuccessful())

                int side = GetBattlerSide(target);
                if (gSideStatuses[side] & SIDE_STATUS_STEALTH_ROCK && gSideStatuses[side] & SIDE_STATUS_TOXIC_SPIKES &&
                    gSideStatuses[side] & SIDE_STATUS_SPIKES && gSideStatuses[side] & SIDE_STATUS_STICKY_WEB)
                    break;

                gSideStatuses[side] |= (SIDE_STATUS_STEALTH_ROCK);
                gSideTimers[side].stealthRockType = TYPE_ROCK;

                gSideStatuses[side] |= (SIDE_STATUS_TOXIC_SPIKES);
                gSideTimers[side].toxicSpikesAmount++;
                if (gSideTimers[side].toxicSpikesAmount > 2) gSideTimers[side].toxicSpikesAmount = 2;

                gSideStatuses[side] |= (SIDE_STATUS_SPIKES);
                gSideTimers[side].spikesAmount++;
                if (gSideTimers[side].spikesAmount > 3) gSideTimers[side].spikesAmount = 3;

                gSideStatuses[side] |= (SIDE_STATUS_STICKY_WEB);

                BattleScriptCall(BattleScript_AngelsWrath_Effect_String_Shot);
                return TRUE;
            }

            case MOVE_HARDEN: {
                CHECK_NOT(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)

                {
                    int activated = FALSE;
                    for (int i = 1; i < NUM_STATS; i++) {
                        if (i == STAT_DEF) continue;
                        activated |= ChangeStatBuffs(battler, 1, i, MOVE_EFFECT_AFFECTS_USER, NULL);
                    }

                    if (activated) {
                        BattleScriptCall(BattleScript_AngelsWrath_Effect_Harden);
                        return TRUE;
                    }
                }
                break;
            }

            case MOVE_IRON_DEFENSE: {
                CHECK_NOT(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)

                gRoundStructs[battler].angelsWrathProtected = TRUE;
                BattleScriptCall(BattleScript_AngelsWrath_Effect_Iron_Defense);
                return TRUE;
            }

            case MOVE_ELECTROWEB: {
                CHECK(ShouldApplyOnHitAffect(target))
                CHECK_NOT(gBattleMons[target].status2 & STATUS2_ESCAPE_PREVENTION)
                CHECK_NOT(gBattleMons[target].statStages[STAT_SPEED] == MIN_STAT_STAGE)

                gBattleMons[target].statStages[STAT_SPEED] = MIN_STAT_STAGE;
                gBattleMons[target].status2 |= (STATUS2_ESCAPE_PREVENTION);
                BattleScriptCall(BattleScript_AngelsWrath_Effect_Electroweb);
                return TRUE;
            }

            case MOVE_BUG_BITE: {
                CHECK(ShouldApplyOnHitAffect(battler))
                CHECK_NOT(BATTLER_MAX_HP(battler))
                CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))

                gBattleMoveDamage = -gHpDealt;
                if (!gBattleMoveDamage) gBattleMoveDamage = -1;
                BattleScriptCall(BattleScript_AngelsWrath_Effect_Bug_Bite_2);
                return TRUE;
            }
        }
        return FALSE;
    },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        switch (move) {
            case MOVE_TACKLE:
            case MOVE_POISON_STING:
            case MOVE_ELECTROWEB:
            case MOVE_BUG_BITE:
                return ACCURACY_HITS_IF_POSSIBLE;

            default:
                return ACCURACY_NO_RESULT;
        }
    },
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        if (move == MOVE_POISON_STING) {
            CHECK(defType == TYPE_STEEL)
            *mod = UQ_4_12(2.0);
            return TRUE;
        }

        if (move == MOVE_ELECTROWEB) {
            CHECK(defType == TYPE_GROUND)
            *mod = UQ_4_12(2.0);
            return TRUE;
        }
        return FALSE;
    },
};

static const Ability PrismaticFur = {
    .name = $("Prismatic Fur"),
    .description = $("Color Change + Protean +\n"
                     "Fur Coat + Ice Scales."),
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.5); },
    .protean = TRUE,
    .colorChange = TRUE,
};

static const Ability ShockingJaws = {
    .name = $("Shocking Jaws"),
    .description = $("Biting moves have 50% chance\n"
                     "to paralyze the target."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
};

static const Ability FaeHunter = {
    .name = $("Fae Hunter"),
    .description = $("Does 50% more damage to\n"
                     "Fairy-types."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_FAIRY)) RESISTANCE(1.5);
        },
};

static const Ability GravityWell = {
    .name = $("Gravity Well"),
    .description = $("Sets Gravity on entry for\n"
                     "5 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    },
};

static const Ability Evaporate = {
    .name = $("Evaporate"),
    .description = $("Takes no damage and sets Mist\n"
                     "if hit by water."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_EVAPORATE;
    },
    .breakable = TRUE,
};

static const Ability Lumberjack = {
    .name = $("Lumberjack"),
    .description = $("1.5x damage to Grass types."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) RESISTANCE(1.5);
        },
};

static const Ability WellBakedBody = {
    .name = $("Well Baked Body"),
    .description = $("Boosts Defense sharply instead\n"
                     "of being hit by Fire-type moves."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE);
        *statId = STAT_DEF;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
    .absorbUp2 = TRUE,
};

static const Ability Furnace = {
    .name = $("Furnace"),
    .description = $("User gains +2 Speed when\n"
                     "when hit by rocks."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        CHECK(gSideTimers[GetBattlerSide(battler)].stealthRockType == TYPE_ROCK)
        CHECK(IsBattlerAlive(battler))
        CHECK(ChangeStatBuffs(battler, 2, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 2);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

static const Ability Electromorphosis = {
    .name = $("Electromorphosis"),
    .description = $("Charges up when getting hit."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    },
};

static const Ability RockyPayload = {
    .name = $("Rocky Payload"),
    .description = $("Boosts the power of Rock-type\n"
                     "and throwing moves by 1.5x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
        },
};

static const Ability EarthEater = {
    .name = $("Earth Eater"),
    .description = $("Heals 25% of max HP when hit\n"
                     "by a Ground move."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_GROUND)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

static const Ability LingeringAroma = {
    .name = $("Lingering Aroma"),
    .description = $("If hit, makes the attacker's ability\n"
                     "Lingering Aroma."),
    .onDefender = Mummy.onDefender,
};

static const Ability FairyTale = {
    .name = $("Fairy Tale"),
    .description = $("Adds Fairy type to itself."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FAIRY); },
};

static const Ability RagingMoth = {
    .name = $("Raging Moth"),
    .description = $("Fire moves hits twice,\n"
                     "both hits at 70% power."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(moveType == TYPE_FIRE)
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

static const Ability AdrenalineRush = {
    .name = $("Adrenaline Rush"),
    .description = $("KOs raise Speed by one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_SPEED); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Archmage = {
    .name = $("Archmage"),
    .description = $("30% chance of adding a type\n"
                     "related effect to each move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DidMoveHit())
        CHECK_NOT(IS_MOVE_STATUS(move))
        CHECK(Random() % 100 < 30)

        switch (moveType) {
            case TYPE_POISON:
                CHECK(IsBattlerAlive(target))
                CHECK(CanBePoisoned(battler, target))

                AbilityStatusEffect(MOVE_EFFECT_TOXIC);
                return TRUE;

            case TYPE_ICE:
                CHECK(IsBattlerAlive(target))
                CHECK(CanGetFrostbite(target))

                AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
                return TRUE;

            case TYPE_WATER:
                CHECK(IsBattlerAlive(target))
                CHECK(CanBeConfused(target))

                AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
                return TRUE;

            case TYPE_FIRE:
                CHECK(IsBattlerAlive(target))
                CHECK(CanBeBurned(target))

                AbilityStatusEffect(MOVE_EFFECT_BURN);
                ;
                return TRUE;

            case TYPE_ELECTRIC:
                CHECK(IsBattlerAlive(target))
                CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

                BattleScriptCall(BattleScript_Archmage_Effect_Type_Electric);
                return TRUE;

            case TYPE_PSYCHIC:
                CHECK(IsBattlerAlive(target))
                CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

                BattleScriptCall(BattleScript_Archmage_Effect_Type_Psychic);
                return TRUE;

            case TYPE_FAIRY:
                CHECK(IsBattlerAlive(target))
                CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

                BattleScriptCall(BattleScript_Archmage_Effect_Type_Fairy);
                return TRUE;

            case TYPE_GRASS:
                CHECK(IsBattlerAlive(target))
                CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

                BattleScriptCall(BattleScript_Archmage_Effect_Type_Grass);
                return TRUE;

            case TYPE_NORMAL:
                CHECK(IsBattlerAlive(target))
                CHECK_NOT(gVolatileStructs[target].encoreTimer)
                CHECK_NOT(IsAbilityOnSide(target, ABILITY_AROMA_VEIL))
                CHECK_NOT(BATTLER_HAS_ABILITY(target, ABILITY_OBLIVIOUS))
                CHECK(SetEncore(target))

                BattleScriptCall(BattleScript_Archmage_Effect_Type_Normal);
                return TRUE;

            case TYPE_ROCK:
                CHECK_NOT(gSideStatuses[GetBattlerSide(target)] & SIDE_STATUS_STEALTH_ROCK)

                gSideStatuses[GetBattlerSide(target)] |= (SIDE_STATUS_STEALTH_ROCK);
                gSideTimers[GetBattlerSide(target)].stealthRockType = TYPE_ROCK;
                BattleScriptCall(BattleScript_Archmage_Effect_Type_Rock);
                return TRUE;

            case TYPE_GHOST:
                CHECK(IsBattlerAlive(target))
                CHECK(CanBeDisabled(target))

                AbilityStatusEffect(MOVE_EFFECT_DISABLE);
                return TRUE;

            case TYPE_DARK:
                CHECK(IsBattlerAlive(target))
                CHECK(CanBleed(target))

                AbilityStatusEffect(MOVE_EFFECT_BLEED);
                return TRUE;

            case TYPE_FIGHTING:
                CHECK(IsBattlerAlive(target))
                CHECK(CanRaiseStat(battler, STAT_SPATK))

                AbilityStatusEffect(MOVE_EFFECT_SP_ATK_PLUS_1 | MOVE_EFFECT_AFFECTS_USER);
                return TRUE;

            case TYPE_FLYING:
                CHECK(IsBattlerAlive(target))
                CHECK(CanRaiseStat(battler, STAT_SPEED))

                AbilityStatusEffect(MOVE_EFFECT_SPD_PLUS_1 | MOVE_EFFECT_AFFECTS_USER);
                return TRUE;

            case TYPE_BUG:
                // TODO: Set sticky web
                break;

            case TYPE_DRAGON:
                CHECK(IsBattlerAlive(target))
                CHECK(StatLowerableOrMirrorArmor(target, STAT_ATK))

                AbilityStatusEffect(MOVE_EFFECT_ATK_MINUS_1);
                return TRUE;

            case TYPE_GROUND:
                CHECK(IsBattlerAlive(target))
                CHECK_NOT(gBattleMons[target].status2 & STATUS2_ESCAPE_PREVENTION)

                AbilityStatusEffect(MOVE_EFFECT_PREVENT_ESCAPE);
                return TRUE;

            case TYPE_STEEL:
                CHECK(IsBattlerAlive(target))
                CHECK(CanRaiseStat(battler, STAT_DEF))

                AbilityStatusEffect(MOVE_EFFECT_DEF_PLUS_1 | MOVE_EFFECT_AFFECTS_USER);
                return TRUE;
        }
        return FALSE;
    },
    .randomizerBanned = TRUE,
};

static const Ability Cryomancy = {
    .name = $("Cryomancy"),
    .description = $("Moves inflict frostbite\n"
                     "5x as often."),
};

static const Ability PhantomPain = {
    .name = $("Phantom Pain"),
    .description = $("Ghost type moves can hit normal\n"
                     "type pokemon for neutral damage."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_GHOST)
        CHECK(defType == TYPE_NORMAL)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    },
};

static const Ability Purgatory = {
    .name = $("Purgatory"),
    .description = $("Boosts Ghost-type moves by 1.3x,\n"
                     "or 1.8x when below 1/3 HP."),
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GHOST),
};

static const Ability Emanate = {
    .name = $("Emanate"),
    .description = $("Normal-type moves become Psy.-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_PSYCHIC),
};

static const Ability KunoichiBlade = {
    .name = $("Kunoichi's Blade"),
    .description = $("Technician + Skill Link."),
    .onOffensiveMultiplier = Technician.onOffensiveMultiplier,
    .skillLink = TRUE,
};

static const Ability MonkeyBusiness = {
    .name = $("Monkey Business"),
    .description = $("Uses Tickle on entry."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TICKLE, 0); },
};

static const Ability CombatSpecialist = {
    .name = $("Combat Specialist"),
    .description = $("Boosts the power of punching and\n"
                     "kicking moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            IronFist.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Striker.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

static const Ability JunglesGuard = {
    .name = $("Jungle's Guard"),
    .description = $("Grass-types on user side: immune\n"
                     "to status/stat drops from enemy."),
    .breakable = TRUE,
};

static const Ability HuntersHorn = {
    .name = $("Hunter's Horn"),
    .description = $("Boost horn moves and heals\n"
                     "1/4 HP when defeating an enemy."),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability PixiePower = {
    .name = $("Pixie Power"),
    .description = $("1.2x accuracy. Boosts Fairy\n"
                     "moves by 1.33x for all."),
    .onEntry = FairyAura.onEntry,
    .onOffensiveMultiplier = FairyAura.onOffensiveMultiplier,
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

static const Ability PlasmaLamp = {
    .name = $("Plasma Lamp"),
    .description = $("Boost accuracy & power of Fire\n"
                     "& Electric type moves by 1.2x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC) MUL(1.2);
        },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC)
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

static const Ability MagmaEater = {
    .name = $("Magma Eater"),
    .description = $("Predator + Molten Down."),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onTypeEffectiveness = MoltenDown.onTypeEffectiveness,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability SuperHotGoo = {
    .name = $("Super Hot Goo"),
    .description = $("Inflicts burn and lowers\n"
                     "Speed on contact."),
    .onAttacker = FlameBody.onAttacker,
    .onDefender = +[](ON_DEFENDER) -> int { return Gooey.onDefender(DELEGATE_DEFENDER) | FlameBody.onDefender(DELEGATE_DEFENDER); },
};

static const Ability Nika = {
    .name = $("Nika"),
    .description = $("Iron fist + Water moves\n"
                     "function normally under sun."),
    .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
};

static const Ability Archer = {
    .name = $("Archer"),
    .description = $("Boosts the power of arrow moves\n"
                     "by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].arrowBased) MUL(1.3);
        },
};

static const Ability ColdPlasma = {
    .name = $("Cold Plasma"),
    .description = $("Electric type moves now\n"
                     "inflict burn instead of paralysis."),
};

static const Ability SuperSlammer = {
    .name = $("Super Slammer"),
    .description = $("Boosts the power of hammer and\n"
                     "slamming moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hammerBased) MUL(1.3);
        },
};

static const Ability InverseRoom = {
    .name = $("Inversion"),
    .description = $("Sets up Inverse Room on\n"
                     "entry, lasts 3 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

        gFieldTimers.started.inverseRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
        gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
        return TRUE;
    },
};

static const Ability Accelerate = {
    .name = $("Accelerate"),
    .description = $("Moves that need a charge turn\n"
                     "are now used instantly."),
};

static const Ability FrostBurn = {
    .name = $("Frost Burn"),
    .description = $("Triggers 40BP Ice Beam after\n"
                     "using a Fire-type move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
    },
};

static const Ability ItchyDefense = {
    .name = $("Itchy Defense"),
    .description = $("Causes infestation when\n"
                     "hit by a contact move."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_WRAPPED)

        gBattleMons[attacker].status2 |= STATUS2_WRAPPED;
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_GRIP_CLAW)
            gVolatileStructs[attacker].wrapTurns = 7;
        else
            gVolatileStructs[attacker].wrapTurns = (Random() % 2) + 4;

        gBattleStruct->wrappedMove[attacker] = MOVE_INFESTATION;
        gBattleStruct->wrappedBy[attacker] = battler;

        BattleScriptCall(BattleScript_AttackerBecameInfested);
        return TRUE;
    },
};

static const Ability Generator = {
    .name = $("Generator"),
    .description = $("Charges up once on entry or\n"
                     "when electric terrain is active."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        int any = FALSE;
        if (IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) {
            any = TRUE;
        } else if (!GetSingleUseAbilityCounter(battler, ability)) {
            SetSingleUseAbilityCounter(battler, ability, TRUE);
            any = TRUE;
        }

        CHECK(any)

        gStackBattler1 = battler;
        BattleScriptPushCursorAndCallback(BattleScript_GeneratorActivates);
        return TRUE;
    },
    .onTerrain = +[](ON_TERRAIN) -> int {
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)
        CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))

        gStackBattler1 = battler;
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    },
    .onExit = +[](ON_EXIT) -> int {
        CHECK(gStatuses3[battler] & STATUS3_CHARGED_UP)
        SetSingleUseAbilityCounter(battler, ability, FALSE);
        return FALSE;
    },
};

static const Ability MoonSpirit = {
    .name = $("Moon Spirit"),
    .description = $("Fairy & Dark gains STAB.\n"
                     "Moonlight recovers 75% HP."),
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; },
};

static const Ability DustCloud = {
    .name = $("Dust Cloud"),
    .description = $("Attacks with Sand Attack\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SAND_ATTACK, 0); },
};

static const Ability BerserkerRage = {
    .name = $("Berserker Rage"),
    .description = $("Berserk + Rampage."),
    .onDefender = Berserk.onDefender,
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Trickster = {
    .name = $("Trickster"),
    .description = $("Uses Disable\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DISABLE, 0); },
};

static const Ability SandGuard = {
    .name = $("Sand Guard"),
    .description = $("Blocks priority and reduces\n"
                     "special damage by 1/2 in sand."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
        return QueenlyMajesty.onImmune(DELEGATE_IMMUNE);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability NaturalRecovery = {
    .name = $("Natural Recovery"),
    .description = $("Natural Cure + Regenerator."),
    .onExit = +[](ON_EXIT) -> int { return NaturalCure.onExit(DELEGATE_EXIT) | Regenerator.onExit(DELEGATE_EXIT); },
};

static const Ability WindRider = {
    .name = $("Wind Rider"),
    .description = $("Increases attack in tailwind or\n"
                     "when hit by wind move."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND)
        CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE)))

        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityHighestAttackingStatRaiseOnSwitchIn);
        return TRUE;
    },
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(gBattleMoves[move].airBased)
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

static const Ability SoothingAroma = {
    .name = $("Soothing Aroma"),
    .description = $("Cures party status on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        int anyStatus = FALSE;
        struct Pokemon *party;

        if (GetBattlerSide(battler) == B_SIDE_PLAYER)
            party = gPlayerParty;
        else
            party = gEnemyParty;

        for (int i = 0; i < PARTY_SIZE; i++) {
            u32 status1 = GetMonData(&party[i], MON_DATA_STATUS);
            if (status1 & STATUS1_ANY) {
                anyStatus = TRUE;
                break;
            }
        }

        CHECK(anyStatus)

        BattleScriptPushCursorAndCallback(BattleScript_EffectSoothingAroma);
        return TRUE;
    },
};

static const Ability PrimAndProper = {
    .name = $("Prim and Proper"),
    .description = $("Wonder Skin + Cute Charm."),
    .onDefender = CuteCharm.onDefender,
    .onAccuracy = WonderSkin.onAccuracy,
    .onAccuracyFor = APPLY_ON_TARGET,
};

static const Ability SuperStrain = {
    .name = $("Super Strain"),
    .description = $("KOs lower Attack by +1.\n"
                     "Take 25% recoil damage."),
    .onRecoil = +[](ON_RECOIL) -> int {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_STRAIN;
        return max(damage / 4, 1);
    },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, -1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS | MOVE_EFFECT_CERTAIN, NULL))
        BattleScriptCall(BattleScript_LowerStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability TippingPoint = {
    .name = $("Tipping Point"),
    .description = $("Getting hit raises SpAtk.\n"
                     "Critical hits maximize SpAtk."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPATK))

        if (gIsCriticalHit) {
            SetStatChanger(STAT_SPATK, 12);
            BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
        } else {
            SetStatChanger(STAT_SPATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        }
        return TRUE;
    },
};

static const Ability Enlightened = {
    .name = $("Enlightened"),
    .description = $("Emanate + Inner Focus."),
    .onOffensiveMultiplier = Emanate.onOffensiveMultiplier,
    .onMoveType = Emanate.onMoveType,
    .onAccuracy = InnerFocus.onAccuracy,
    .breakable = TRUE,
};

static const Ability PeacefulSlumber = {
    .name = $("Peaceful Slumber"),
    .description = $("Sweet Dreams + Self Sufficient."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (!SweetDreams.onEndTurn(DELEGATE_END_TURN)) return SelfSufficient.onEndTurn(DELEGATE_END_TURN);
        gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
        return TRUE;
    },
};

static const Ability Aftershock = {
    .name = $("Aftershock"),
    .description = $("Triggers Magnitude 4-7 after\n"
                     "using a damaging move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
    },
};

ON_EITHER(FreezingPoint) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanGetFrostbite(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
    return TRUE;
}
static const Ability FreezingPoint = {
    .name = $("Freezing Point"),
    .description = $("30% chance to get frostbitten\n"
                     "on contact."),
    ON_EITHER_ABILITY(FreezingPoint),
};

static int CryoProficiencyHail(int ability, int battler, int attacker, int move, int moveType) {
    CHECK(ShouldApplyOnHitAffect(battler))
    CHECK_NOT(gBattleWeather & WEATHER_HAIL_ANY)
    if (gBattleWeather & WEATHER_PRIMAL_ANY) {
        BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
        return NO_ANNOUNCE;
    } else if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE)) {
        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_CryoProficiencyActivates);
        return TRUE;
    }
    return FALSE;
}
static const Ability CryoProficiency = {
    .name = $("Cryo Proficiency"),
    .description = $("Triggers hail when hit. 30%\n"
                     "chance to frostbite on contact."),
    .onAttacker = FreezingPoint.onAttacker,
    .onDefender =
        +[](ON_DEFENDER) -> int { return FreezingPoint.onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(ability, battler, attacker, move, moveType); },
};

static const Ability ArcaneForce = {
    .name = $("Arcane Force"),
    .description = $("All moves gain STAB.\n"
                     "Ups “supereffective” by 10%."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
        },
    .onStab = MysticPower.onStab,
};

static const Ability Doombringer = {
    .name = $("Doombringer"),
    .description = $("Uses Doom Desire\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DOOM_DESIRE, 0); },
};

static const Ability Wishmaker = {
    .name = $("Wishmaker"),
    .description = $("Uses Wish on switch-in.\n"
                     "Three uses per battle."),
    .onEntry = +[](ON_ENTRY) -> int {
        int counter = GetSingleUseAbilityCounter(battler, ability);
        CHECK(counter < 3)
        CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

        SetSingleUseAbilityCounter(battler, ability, counter + 1);
        return TRUE;
    },
    .persistent = TRUE,
};

static const Ability YukiOnna = {
    .name = $("Yuki Onna"),
    .description = $("Scare + Intimidate.\n"
                     "10% chance to infatuate on hit."),
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanInfatuate(battler, target))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    },
};

static const Ability Suppress = {
    .name = $("Suppress"),
    .description = $("Casts Torment on entry."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TORMENT, 0); },
};

static const Ability Refrigerator = {
    .name = $("Refrigerator"),
    .description = $("Filter + Illuminate."),
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
    .onAccuracy = Illuminate.onAccuracy,
};

static const Ability HeavenAsunder = {
    .name = $("Heaven Asunder"),
    .description = $("Spacial Rend always crits.\n"
                     "Ups crit level by +1."),
    .onCrit =
        +[](ON_CRIT) {
            if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
            return 1;
        },
};

static const Ability PurifyingWaters = {
    .name = $("Purifying Waters"),
    .description = $("Hydration + Water Veil."),
    .onEntry = WaterVeil.onEntry,
    .onEndTurn = Hydration.onEndTurn,
};

static const Ability Seaborne = {
    .name = $("Seaborne"),
    .description = $("Drizzle + Swift Swim."),
    .onEntry = Drizzle.onEntry,
    .onStat = SwiftSwim.onStat,
};

static const Ability HighTide = {
    .name = $("High Tide"),
    .description = $("Triggers 50 BP Surf after\n"
                     "using a Water-type move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_WATER)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
    },
};

static const Ability ChangeOfHeart = {
    .name = $("Change of Heart"),
    .description = $("Uses Heart Swap\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_HEART_SWAP, 0); },
};

static const Ability MysticBlades = {
    .name = $("Mystic Blades"),
    .description = $("Keen edge moves become special\n"
                     "and deal 30% more damage."),
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return TRUE;
    },
};

static const Ability Determination = {
    .name = $("Determination"),
    .description = $("Ups Special Attack by 50%\n"
                     "if suffering."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
        },
};

static const Ability Fertilize = {
    .name = $("Fertilize"),
    .description = $("Normal-type moves become Grass-\n"
                     "type moves and get a 1.1x boost."),
    ATE_ABILITY(TYPE_GRASS),
};

static const Ability PureLove = {
    .name = $("Pure Love"),
    .description = $("Infatuates on contact.\n"
                     "Heal 25% damage vs infatuated."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gBattleMons[target].status2 & STATUS2_INFATUATION)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
    .onDefender = CuteCharm.onDefender,
};

static const Ability Fighter = {
    .name = $("Fighter"),
    .description = $("Boosts Fight.-type moves by 1.2x,\n"
                     "or 1.5x when below 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIGHTING),
};

static const Ability MyceliumMight = {
    .name = $("Mycelium Might"),
    .description = $("Status moves ignore immunities\n"
                     "but go last."),
};

static const Ability Telekinetic = {
    .name = $("Telekinetic"),
    .description = $("Casts Telekinesis on entry."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TELEKINESIS, 0); },
};

static const Ability Combustion = {
    .name = $("Combustion"),
    .description = $("Boosts the power of Fire-type\n"
                     "moves by 1.5x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) MUL(1.5);
        },
};

static const Ability PonyPower = {
    .name = $("Blade's Essence"),
    .description = $("Keen Edge + Mystic Blades."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            KeenEdge.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MysticBlades.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onSwapSplit = MysticBlades.onSwapSplit,
};

static const Ability PowderBurst = {
    .name = $("Powder Burst"),
    .description = $("Casts Powder on entry."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_POWDER, 0); },
};

static const Ability Retriever = {
    .name = $("Retriever"),
    .description = $("Retrieves item on switch-out."),
    .onExit = +[](ON_EXIT) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(gBattleMons[battler].item)

        u8 side = GetBattlerSide(gActiveBattler);
        u8 index = gBattlerPartyIndexes[gActiveBattler];
        u16 originalItem = gLastUsedItem = side == B_SIDE_PLAYER ? gBattleStruct->itemStolen[index].originalItem : gBattleStruct->opposingOriginalItems[index];

        CHECK(originalItem)

        gBattleStruct->usedHeldItems[index][side] = ITEM_NONE;

        UpdateBattlerItem(gActiveBattler, originalItem);

        BattleScriptCall(BattleScript_RetrieverExits);
        return TRUE;
    },
};

static const Ability MonsterMash = {
    .name = $("Monster Mash"),
    .description = $("Casts Trick-or-Treat on entry."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TRICK_OR_TREAT, 0); },
};

static const Ability TwoStep = {
    .name = $("Two Step"),
    .description = $("Triggers 50BP Revelation Dance\n"
                     "after using a Dance move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
    },
};

static const Ability Spiteful = {
    .name = $("Spiteful"),
    .description = $("Reduces attacker's PP\n"
                     "on contact."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])

        BattleScriptCall(BattleScript_AbilitySpiteful);
        return TRUE;
    },
};

static const Ability Fortitude = {
    .name = $("Fortitude"),
    .description = $("Boosts SpDef +1 when hit.\n"
                     "Maxes SpDef on crit."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPDEF))

        if (gIsCriticalHit) {
            SetStatChanger(STAT_SPDEF, 12);
            BattleScriptCall(BattleScript_TargetsStatWasMaxedOut);
        } else {
            SetStatChanger(STAT_SPDEF, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        }
        return TRUE;
    },
};

static const Ability Devourer = {
    .name = $("Devourer"),
    .description = $("Strong Jaw + Primal Maw."),
    .onParentalBond = PrimalMaw.onParentalBond,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
};

static const Ability PhantomThief = {
    .name = $("Phantom Thief"),
    .description = $("Attacks with 40BP Spectral Thief\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SPECTRAL_THIEF, 40); },
};

static const Ability EarlyGrave = {
    .name = $("Early Grave"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's Ghost-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_GHOST),
};

static const Ability Grappler = {
    .name = $("Grappler"),
    .description = $("Trapping moves last 6 turns.\n"
                     "Trapping deals 1/6 HP."),
};

static const Ability BassBoosted = {
    .name = $("Bass Boosted"),
    .description = $("Amplifier + Punk Rock."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Amplifier.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            PunkRock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onDefensiveMultiplier = PunkRock.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability FlamingJaws = {
    .name = $("Flaming Jaws"),
    .description = $("Biting moves have 50% chance\n"
                     "to burn the target."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
};

static const Ability MonsterHunter = {
    .name = $("Monster Hunter"),
    .description = $("Deals 1.5x more damage to\n"
                     "Dark-types."),
};

static const Ability CrownedSword = {
    .name = $("Crowned Sword"),
    .description = $("Intrepid Sword + Anger Point."),
    .onEntry = IntrepidSword.onEntry,
    .onDefender = AngerPoint.onDefender,
};

static const Ability CrownedShield = {
    .name = $("Crowned Shield"),
    .description = $("Dauntless Shield + Stamina."),
    .onEntry = DauntlessShield.onEntry,
    .onDefender = Stamina.onDefender,
};

static const Ability BerserkDna = {
    .name = $("Berserk DNA"),
    .description = $("Sharply ups highest attacking stat\n"
                     "but confuses on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE))) if (CanBeConfused(battler)) {
            gBattleMons[battler].status2 |= STATUS2_CONFUSION_TURN(3);
            BattleScriptPushCursorAndCallback(BattleScript_BerserkDNA);
        }
        else {
            BattleScriptPushCursorAndCallback(BattleScript_BerserkDNANoConfusion);
        }
        return TRUE;
    },
};

static const Ability CrownedKing = {
    .name = $("Crowned King"),
    .description = $("Unnerve + Grim Neigh +\n"
                     "Chilling Neigh."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        return AsOneShadowRider.onBattlerFaints(DELEGATE_BATTLER_FAINTS) | AsOneIceRider.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability SnapTrapWhenHit = {
    .name = $("Clap Trap"),
    .description = $("Counters contact with\n"
                     "50BP Snap Trap."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
        return FALSE;
    },
};

static const Ability Permanence = {
    .name = $("Permanence"),
    .description = $("Foes can't heal in any way."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); },
};

static const Ability Hubris = {
    .name = $("Hubris"),
    .description = $("KOs raise SpAtk by one stage."),
    .onBattlerFaints = GrimNeigh.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability CosmicDaze = {
    .name = $("Cosmic Daze"),
    .description = $("2x damage vs confused. Enemies\n"
                     "take 2x confusion damage."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
        },
};

static const Ability MindsEye = {
    .name = $("Mind's Eye"),
    .description = $("Hits Ghost-type Pokémon.\n"
                     "Accuracy can't be lowered."),
    .onTypeEffectiveness = Scrappy.onTypeEffectiveness,
};

static const Ability BloodPrice = {
    .name = $("Blood Price"),
    .description = $("Does 30% more damage but\n"
                     "lose 10% HP when attacking."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(IS_MOVE_STATUS(gLastResultingMoves[battler]))
        CHECK_NOT(IsMagicGuardProtected(battler))
        CHECK(IsBattlerAlive(battler))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 10;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        BattleScriptPushCursorAndCallback(BattleScript_AbilitySelfDamage);
        return TRUE;
    },
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MUL(1.3); },
};

ON_EITHER(SpikeArmor) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBleed(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
    return TRUE;
}
static const Ability SpikeArmor = {
    .name = $("Spike Armor"),
    .description = $("30% chance to bleed\n"
                     "on contact or offense."),
    ON_EITHER_ABILITY(SpikeArmor),
};

static const Ability VoodooPower = {
    .name = $("Voodoo Power"),
    .description = $("30% chance to bleed when\n"
                     "hit by special attacks."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IS_MOVE_SPECIAL(move))
        CHECK(CanBleed(attacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
        return TRUE;
    },
};

static const Ability ChromeCoat = {
    .name = $("Chrome Coat"),
    .description = $("Reduces special damage taken by\n"
                     "40%, but decreases Speed by 10%."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.6);
        },
    .onStat = LeadCoat.onStat,
    .breakable = TRUE,
};

static const Ability Banshee = {
    .name = $("Banshee"),
    .description = $("Sound moves get a 1.2x boost\n"
                     "and become Ghost if Normal."),
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_GHOST + 1;
    },
};

static const Ability WebSpinner = {
    .name = $("Web Spinner"),
    .description = $("Uses String Shot\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_STRING_SHOT, 0); },
};

static const Ability ShowdownMode = {
    .name = $("Showdown Mode"),
    .description = $("Ambush + Violent Rush."),
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
    },
};

static const Ability SeedSower = {
    .name = $("Seed Sower"),
    .description = $("Sets Grassy Terrain when hit.\n"
                     "Heals party status when it does."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptCall(BattleScript_SeedSower);
        return TRUE;
    },
};

static const Ability Airborne = {
    .name = $("Airborne"),
    .description = $("Boosts own & ally's Flying-type\n"
                     "moves by 1.3x."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY,
};

static const Ability Parroting = {
    .name = $("Parroting"),
    .description = $("Copies sound moves used by\n"
                     "others. Immune to sound."),
    .onImmune = Soundproof.onImmune,
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

static const Ability SaltCircle = {
    .name = $("Salt Circle"),
    .description = $("Prevents opposing pokemon\n"
                     "from fleeing on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        int anyBlocked = FALSE;
        gBattlerTarget = BATTLE_OPPOSITE(battler);

        if (IsBattlerAlive(gBattlerTarget) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION)) {
            gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
            gVolatileStructs[gBattlerTarget].battlerPreventingEscape = battler;
            anyBlocked = TRUE;
        }

        gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        if (IsBattlerAlive(gBattlerTarget) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION)) {
            gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
            gVolatileStructs[gBattlerTarget].battlerPreventingEscape = battler;
            anyBlocked = TRUE;
        }

        CHECK(anyBlocked)
        return SwitchInAnnounce(B_MSG_SWITCHIN_SALT_CIRCLE);
    },
};

static const Ability PurifyingSalt = {
    .name = $("Purifying Salt"),
    .description = $("Immune to status conditions.\n"
                     "Take 1/2 damage from Ghost."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GHOST) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

int ProtosynthesisHandler(int ability, int battler, AbilityCallType callType) {
    ParadoxBoost state = GetAbilityStateAs(battler, ability).paradoxBoost;

    if (state.source == PARADOX_BOOST_NOT_ACTIVE && IsWeatherActive(WEATHER_SUN_ANY)) {
        InsertCorrectEndType(callType);
        ParadoxBoost boost = {.source = PARADOX_WEATHER_ACTIVE, .statId = GetHighestStatId(battler, TRUE)};
        SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_WEATHER;
        BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
        return TRUE;
    }

    if (state.source == PARADOX_WEATHER_ACTIVE && !IsWeatherActive(WEATHER_SUN_ANY)) {
        InsertCorrectEndType(callType);
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY) {
            // Push this first so it resolves last
            ParadoxBoost boost = {.source = PARADOX_BOOSTER_ENERGY, .statId = GetHighestStatId(battler, TRUE)};
            SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
            RemoveItem(battler);
            SetStatChanger(boost.statId, 0);
            BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
        } else
            SetAbilityState(battler, ability, 0);
        BattleScriptCall(BattleScript_ParadoxBoostEnds);
        return TRUE;
    }

    if (state.source == PARADOX_BOOST_NOT_ACTIVE && GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY) {
        InsertCorrectEndType(callType);
        ParadoxBoost boost = {.source = PARADOX_BOOSTER_ENERGY, .statId = GetHighestStatId(battler, TRUE)};
        SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
        RemoveItem(battler);
        BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
        return TRUE;
    }
    return FALSE;
}
static const Ability Protosynthesis = {
    .name = $("Protosynthesis"),
    .description = $("Boosts highest stat in Sun\n"
                     "or with Booster Energy."),
    .onEntry = +[](ON_ENTRY) -> int { return ProtosynthesisHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onWeather = +[](ON_WEATHER) -> int { return ProtosynthesisHandler(ability, battler, ABILITY_BS_CALL); },
    .onStat =
        +[](ON_STAT) {
            ParadoxBoost boost = GetAbilityStateAs(battler, ability).paradoxBoost;
            if (!boost.source || boost.statId != statId) return;
            if (statId == STAT_SPEED)
                *stat *= 1.5;
            else
                *stat *= 1.3;
        },
};

int QuarkDriveHandler(int ability, int battler, AbilityCallType callType) {
    ParadoxBoost state = GetAbilityStateAs(battler, ability).paradoxBoost;

    if (state.source == PARADOX_BOOST_NOT_ACTIVE && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) {
        InsertCorrectEndType(callType);
        ParadoxBoost boost = {.source = PARADOX_WEATHER_ACTIVE, .statId = GetHighestStatId(battler, TRUE)};
        SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_TERRAIN;
        BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
        return TRUE;
    }

    if (state.source == PARADOX_WEATHER_ACTIVE && !IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) {
        InsertCorrectEndType(callType);
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY) {
            // Push this first so it resolves last
            ParadoxBoost boost = {.source = PARADOX_BOOSTER_ENERGY, .statId = GetHighestStatId(battler, TRUE)};
            SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
            RemoveItem(battler);
            SetStatChanger(boost.statId, 0);
            BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
        } else
            SetAbilityState(battler, ability, 0);
        BattleScriptCall(BattleScript_ParadoxBoostEnds);
        return TRUE;
    }

    if (state.source == PARADOX_BOOST_NOT_ACTIVE && GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY) {
        InsertCorrectEndType(callType);
        ParadoxBoost boost = {.source = PARADOX_BOOSTER_ENERGY, .statId = GetHighestStatId(battler, TRUE)};
        SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
        RemoveItem(battler);
        BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
        return TRUE;
    }
    return FALSE;
}
static const Ability QuarkDrive = {
    .name = $("Quark Drive"),
    .description = $("Boosts highest stat in Electric\n"
                     "Terrain or with Booster Energy."),
    .onEntry = +[](ON_ENTRY) -> int { return QuarkDriveHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onTerrain = +[](ON_TERRAIN) -> int { return QuarkDriveHandler(ability, battler, ABILITY_BS_CALL); },
    .onStat = Protosynthesis.onStat,
};

static const Ability WindPower = {
    .name = $("Wind Power"),
    .description = $("Charges up when hit by wind\n"
                     "moves or Tailwind starts."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].airBased)
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    },
};

static const Ability Impulse = {
    .name = $("Impulse"),
    .description = $("Non-contact moves use the\n"
                     "Speed stat for damage."),
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (!(gBattleMoves[move].flags & FLAG_MAKES_CONTACT)) *atkStatToUse = STAT_SPEED;
        },
};

static const Ability TerminalVelocity = {
    .name = $("Terminal Velocity"),
    .description = $("Special moves use 20% of its\n"
                     "Speed stat additionally."),
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (IS_MOVE_SPECIAL(move)) *secondaryAtkStatToUse = STAT_SPEED;
        },
};

static const Ability GuardDog = {
    .name = $("Guard Dog"),
    .description = $("Can't be forced out.\n"
                     "Inverts Intimidate effects."),
};

static const Ability AngerShell = {
    .name = $("Anger Shell"),
    .description = $("Applies Shell Smash when\n"
                     "reduced below 1/2 HP."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_AngerShell);
        return TRUE;
    },
};

static const Ability Egoist = {
    .name = $("Egoist"),
    .description = $("Raises its own stats when\n"
                     "foes raise theirs."),
    .onReactive = +[](ON_REACTIVE) -> int {
        CHECK(gBattleStruct->statStageCheckState != STAT_STAGE_CHECK_NOT_NEEDED)
        for (int opponent = BATTLE_OPPOSITE(GetBattlerSide(battler)); opponent < gBattlersCount; opponent += 2) {
            for (int stat = STAT_ATK; stat < ARRAY_COUNT(gBattleStruct->statChangesToCheck[opponent]); stat++) {
                if (gBattleStruct->statChangesToCheck[opponent][stat - 1] > 0) {
                    if (gBattleStruct->statStageCheckState == STAT_STAGE_CHECK_NEEDED) {
                        gBattleStruct->statStageCheckState = STAT_STAGE_CHECK_IN_PROGRESS;
                        InsertCorrectEndType(callType);
                        BattleScriptCall(BattleScript_PerformCopyStatEffects);
                    }
                    SetAbilityStateAs(battler, ability, (AbilityStates){.statCopyState = (StatCopyState){.inProgress = TRUE}});
                    return TRUE;
                }
            }
        }
        return FALSE;
    },
};

static const Ability Subdue = {
    .name = $("Subdue"),
    .description = $("Doubles the power of\n"
                     "stat dropping moves."),
};

static const Ability ReadiedAction = {
    .name = $("Readied Action"),
    .description = $("Doubles attack on\n"
                     "first turn."),
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
    },
};

static const Ability DarkGaleWings = {
    .name = $("Stygian Rush"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's Dark-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_DARK),
};

static const Ability GuiltTrip = {
    .name = $("Guilt Trip"),
    .description = $("Sharply lowers attacker's Attack\n"
                     "and SpAtk when fainting."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

        BattleScriptCall(BattleScript_GuiltTrip);
        return TRUE;
    },
};

static const Ability WaterGaleWings = {
    .name = $("Tidal Rush"),
    .description = $("At full HP, gives +1 priority to\n"
                     "this Pokémon's Water-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_WATER),
};

static const Ability ZeroToHero = {
    .name = $("Zero To Hero"),
    .description = $("Changes forms after\n"
                     "switching out."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_PALAFIN)
        CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)
        CHECK(GetSingleUseAbilityCounter(battler, ability))

        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_PALAFIN_HERO);
        gBattleMons[battler].species = SPECIES_PALAFIN_HERO;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
        return TRUE;
    },
    .onExit = +[](ON_EXIT) -> int {
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        return FALSE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability Costar = {
    .name = $("Costar"),
    .description = $("Copies its ally's stat changes\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerAlive(BATTLE_PARTNER(battler)))

        int anyChanged = FALSE;
        for (int i = STAT_ATK; i < NUM_BATTLE_STATS; i++) {
            if (gBattleMons[battler].statStages[i] != gBattleMons[BATTLE_PARTNER(battler)].statStages[i]) {
                gBattleMons[battler].statStages[i] = gBattleMons[BATTLE_PARTNER(battler)].statStages[i];
                anyChanged = TRUE;
            }
        }

        CHECK(anyChanged)
        return SwitchInAnnounce(B_MSG_SWITCHIN_COSTAR);
    },
};

static const Ability Commander = {
    .name = $("Commander"),
    .description = $("Hops inside an allied Dondozo.\n"
                     "Boosts its ally but can't act."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(GetAbilityState(battler, ability))

        SetAbilityState(battler, ability, COMMANDER_NOT_ACTIVE);
        gStatuses3[battler] &= ~STATUS3_SEMI_INVULNERABLE;
        BattleScriptCall(BattleScript_CommanderEnds);
        return TRUE;
    },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(GetAbilityState(target, ability))
        return ACCURACY_ALWAYS_MISSES;
    },
    .onBattlerFaintsFor = APPLY_ON_ALLY,
    .onAccuracyFor = APPLY_ON_TARGET,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability EjectPackAbility = {
    .name = $("Tactical Retreat"),
    .description = $("Flees when stats are lowered."),
    .persistent = TRUE,
};

static const Ability VengefulSpirit = {
    .name = $("Vengeful Spirit"),
    .description = $("Haunted Spirit + Vengeance."),
    .onDefender = HauntedSpirit.onDefender,
    .onOffensiveMultiplier = Vengeance.onOffensiveMultiplier,
};

static const Ability CudChew = {
    .name = $("Cud Chew"),
    .description = $("Eats berries again at the\n"
                     "end of the next turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CudChewState state = GetAbilityStateAs(battler, ability).cudChewState;
        if (state.setThisTurn) {
            SetAbilityStateAs(battler, ability, (AbilityStates){.cudChewState = {.itemId = state.itemId}});
        } else if (state.itemId) {
            // attacker temporarily gains their item
            gBattleStruct->changedItems[battler] = gBattleMons[battler].item;
            gBattleMons[battler].item = state.itemId;

            SetAbilityStateAs(battler, ability, (AbilityStates){.cudChewState = {.activating = TRUE}});

            BattleScriptPushCursorAndCallback(BattleScript_CudChew);
            return TRUE;
        }
        return FALSE;
    },
};

static const Ability ArmorTail = {
    .name = $("Armor Tail"),
    .description = $("Protects itself and ally from\n"
                     "priority moves."),
    .onImmune = QueenlyMajesty.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

static const Ability MindCrush = {
    .name = $("Mind Crunch"),
    .description = $("Biting moves use SpAtk and\n"
                     "deal 30% more damage."),
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
        },
};

static const Ability SupremeOverlord = {
    .name = $("Supreme Overlord"),
    .description = $("Each fainted ally increases\n"
                     "Attack and SpAtk by 10%."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFaintedMonCount[GetBattlerSide(battler)])

        return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
    },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
        },
};

static const Ability IllWill = {
    .name = $("Ill Will"),
    .description = $("Deletes the PP of the move\n"
                     "that faints this Pokemon."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])
        CHECK_NOT(IsBattlerAlive(battler))

        gBattleMons[attacker].pp[gChosenMovePos] = 0;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove)
        gActiveBattler = battler;
        BtlController_EmitSetMonData(0, gChosenMovePos + REQUEST_PPMOVE1_BATTLE, 0, 1, &gBattleMons[battler].pp[gChosenMovePos]);
        BattleScriptCall(BattleScript_IllWillTakesPp);
        return TRUE;
    },
};

static const Ability FireScales = {
    .name = $("Fire Scales"),
    .description = $("Halves damage taken by Special\n"
                     "moves. Does NOT double SpDef."),
    .onDefensiveMultiplier = IceScales.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability WatchYourStep = {
    .name = $("Watch Your Step"),
    .description = $("Spreads two layers of\n"
                     "Spikes on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        u8 targetSide = GetBattlerSide(BATTLE_OPPOSITE(battler));
        CHECK(gSideTimers[targetSide].spikesAmount < 3)

        gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
        return TRUE;
    },
};

static const Ability RapidResponse = {
    .name = $("Rapid Response"),
    .description = $("Boosts Speed by 50% + SpAtk\n"
                     "by 20% on first turn."),
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
    },
};

static const Ability DoubleIronBarbs = {
    .name = $("Sharp Edges"),
    .description = $("1/6 HP damage when touched."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 6;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    },
};

static const Ability ThermalExchange = {
    .name = $("Thermal Exchange"),
    .description = $("Ups Attack when hit by Fire.\n"
                     "Immune to burn."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanRaiseStat(battler, STAT_ATK))

        SetStatChanger(STAT_ATK, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability GoodAsGold = {
    .name = $("Good As Gold"),
    .description = $("Immune to all Status moves,\n"
                     "unless whole field is affected."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability SharingIsCaring = {
    .name = $("Sharing Is Caring"),
    .description = $("Stat changes are shared\n"
                     "between all battlers."),
    .onReactive = +[](ON_REACTIVE) -> int {
        CHECK(gBattleStruct->statStageCheckState != STAT_STAGE_CHECK_NOT_NEEDED)
        CHECK(IsAbilityOnField(ability) - 1 == battler)
        if (gBattleStruct->statStageCheckState == STAT_STAGE_CHECK_NEEDED) {
            InsertCorrectEndType(callType);
            BattleScriptCall(BattleScript_PerformCopyStatEffects);
        }
        SetAbilityStateAs(battler, ability, (AbilityStates){.statCopyState = (StatCopyState){.inProgress = TRUE}});
        return TRUE;
    },
};

static const Ability TabletsOfRuin = {
    .name = $("Tablets Of Ruin"),
    .description = $("Lowers the Attack of\n"
                     "other Pokemon by 25%."),
    .onStat = +[](ON_STAT) { RuinEffect(STAT_ATK, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_ATK,
};

static const Ability SwordOfRuin = {
    .name = $("Sword Of Ruin"),
    .description = $("Lowers the Defense of\n"
                     "other Pokemon by 25%."),
    .onStat = +[](ON_STAT) { RuinEffect(STAT_DEF, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
};

static const Ability VesselOfRuin = {
    .name = $("Vessel Of Ruin"),
    .description = $("Lowers the Special Attack of\n"
                     "other Pokemon by 25%."),
    .onStat = +[](ON_STAT) { RuinEffect(STAT_SPATK, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_SPATK,
};

static const Ability BeadsOfRuin = {
    .name = $("Beads Of Ruin"),
    .description = $("Lowers the Special Defense\n"
                     "of other Pokemon by 25%."),
    .onStat = +[](ON_STAT) { RuinEffect(STAT_DEF, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
};

static const Ability PermafrostClone = {
    .name = $("Thick Skin"),
    .description = $("Takes 25% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier = Permafrost.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability Gallantry = {
    .name = $("Gallantry"),
    .description = $("Gets no damage for\n"
                     "first hit."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
        return TRUE;
    },
    .noDamageHits = 1,
    .breakable = TRUE,
    .persistent = TRUE,
};

static const Ability OrichalcumPulse = {
    .name = $("Orichalcum Pulse"),
    .description = $("Summons sun on entry.\n"
                     "Raises Atk by 1.33x in sun."),
    .onEntry = Drought.onEntry,
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
        },
};

static const Ability SunBasking = {
    .name = $("Sun Basking"),
    .description = $("Blocks priority and reduces\n"
                     "physical damage by 1/2 in sun."),
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
        return QueenlyMajesty.onImmune(DELEGATE_IMMUNE);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

static const Ability WingedKing = {
    .name = $("Winged King"),
    .description = $("Ups “supereffective” by 33%."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
        },
};

static const Ability HadronEngine = {
    .name = $("Hadron Engine"),
    .description = $("Field becomes Electric.\n"
                     "+33% SpAtk in Electric Terrain."),
    .onEntry = ElectricSurge.onEntry,
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
        },
};

static const Ability IronSerpent = {
    .name = $("Iron Serpent"),
    .description = $("Ups “supereffective” by 33%."),
    .onOffensiveMultiplier = WingedKing.onOffensiveMultiplier,
};

static const Ability WeatherDoubleBoost = {
    .name = $("Catastrophe"),
    .description = $("Sun boosts Water.\n"
                     "Rain boosts Fire."),
};

static const Ability SweepingEdgePlus = {
    .name = $("Blademaster"),
    .description = $("Sweeping Edge + Keen Edge."),
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    .onAccuracy = SweepingEdge.onAccuracy,
};

static const Ability CelestialBlessing = {
    .name = $("Celestial Blessing"),
    .description = $("Recovers 1/12 of its health each\n"
                     "turn under Misty Terrain."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_MISTY_TERRAIN))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 12;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    },
};

static const Ability MinionControl = {
    .name = $("Minion Control"),
    .description = $("Moves hit an extra time for\n"
                     "each healthy party member."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType { return PARENTAL_BOND_MINION_CONTROL; },
};

static const Ability MoltenBlades = {
    .name = $("Molten Blades"),
    .description = $("Keen Edge + Keen Edge moves\n"
                     "have a 20% chance to burn."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
};

static const Ability HauntingFrenzy = {
    .name = $("Haunting Frenzy"),
    .description = $("20% chance to flinch the\n"
                     "opponent. +1 speed on kill."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    },
    .onBattlerFaints = AdrenalineRush.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability NoiseCancel = {
    .name = $("Noise Cancel"),
    .description = $("Protects the party from sound-\n"
                     "based moves."),
    .onImmune = Soundproof.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

static const Ability RadioJam = {
    .name = $("Radio Jam"),
    .description = $("Sound-based moves have a 20%\n"
                     "chance to inflict disable."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeDisabled(target))
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
    },
};

static const Ability Ole = {
    .name = $("Olé!"),
    .description = $("20% chance to evade single-\n"
                     "target moves."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        switch (GetBattlerBattleMoveTargetFlags(move, battler)) {
            case MOVE_TARGET_SELECTED:
            case MOVE_TARGET_USER_OR_SELECTED:
            case MOVE_TARGET_RANDOM:
                *accuracy *= .8;
                return ACCURACY_MULTIPLICATIVE;

            default:
                return ACCURACY_NO_RESULT;
        }
    },
    .onAccuracyFor = APPLY_ON_TARGET,
};

static const Ability Malicious = {
    .name = $("Malicious"),
    .description = $("Lowers the foe's highest\n"
                     "Attack and Defense stat."),
    .onEntry = UseIntimidateClone,
};

static const Ability DeadPower = {
    .name = $("Dead Power"),
    .description = $("1.5x Attack boost. 20% chance\n"
                     "to curse on contact moves."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gBattleMons[target].status2 & STATUS2_CURSED)
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_CURSE);
    },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK) *stat *= 1.5;
        },
};

static const Ability BrawlingWyvern = {
    .name = $("Brawling Wyvern"),
    .description = $("No guard + Dragon type\n"
                     "moves become punching moves."),
    .onAccuracy = NoGuard.onAccuracy,
    .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
};

static const Ability MythicalArrows = {
    .name = $("Mythical Arrows"),
    .description = $("Arrow moves become special\n"
                     "and deal 30% more damage."),
    .onOffensiveMultiplier = Archer.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].arrowBased);
        return TRUE;
    },
};

static const Ability Lawnmower = {
    .name = $("Lawnmower"),
    .description = $("Removes terrain on switch-in.\n"
                     "Stat up if terrain removed."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
        return TRUE;
    },
};

static const Ability Flourish = {
    .name = $("Flourish"),
    .description = $("Boosts Grass moves by 50% in\n"
                     "grassy terrain."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
        },
};

static const Ability DesertSpirit = {
    .name = $("Desert Spirit"),
    .description = $("Summons sand on entry. Ground\n"
                     "moves hit airborne in sand."),
    .onEntry = SandStream.onEntry,
};

static const Ability Contempt = {
    .name = $("Contempt"),
    .description = $("Ignores opposing stat changes.\n"
                     "Boosts Attack when stat lowered."),
    .unaware = TRUE,
};

static const Ability Aerialist = {
    .name = $("Aerialist"),
    .description = $("Levitate + Flock."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Levitate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Flock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .breakable = TRUE,
};

static const Ability TeraShell = {
    .name = $("Tera Shell"),
    .description = $("All hits will be not very effective\n"
                     "while at full HP."),
    .breakable = TRUE,
};

static const Ability ToxicChain = {
    .name = $("Toxic Chain"),
    .description = $("Moves have a 30% chance to\n"
                     "badly poison the foe."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
};

static const Ability ParasiticSpores = {
    .name = $("Parasitic Spores"),
    .description = $("Deals 1/8 HP damage to non-\n"
                     "Ghost. Spreads on contact."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

        gVolatileStructs[battler].parasiticSpores = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
    },
};

static const Ability PoisonPuppeteer = {
    .name = $("Poison Puppeteer"),
    .description = $("Poison also inflicts confusion."),
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
    },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int state = GetAbilityState(battler, ability);
        if (state & (1 << fainted)) SetAbilityState(battler, ability, state ^ (1 << fainted));
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_OTHER,
};

static const Ability Entrance = {
    .name = $("Entrance"),
    .description = $("Confusion also inflicts\n"
                     "infatuation."),
    .onReactive = +[](ON_REACTIVE) -> int { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
};

static const Ability Rejection = {
    .name = $("Rejection"),
    .description = $("Applies Quash on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldTimers.quashTimer)

        gFieldTimers.quashTimer = QUASH_DURATION;
        gFieldTimers.started.quash = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
    },
};

static const Ability AppleEnlightenment = {
    .name = $("Apple Enlightenment"),
    .description = $("Fur coat + Magic Guard."),
    .onDefensiveMultiplier = FurCoat.onDefensiveMultiplier,
    .breakable = TRUE,
    .magicGuard = TRUE,
};

static const Ability BalloonBomber = {
    .name = $("Balloon Bomb"),
    .description = $("Aftermath + Inflatable"),
    .onDefender = +[](ON_DEFENDER) -> int { return Aftermath.onDefender(DELEGATE_DEFENDER) || Inflatable.onDefender(DELEGATE_DEFENDER); },
};

static const Ability FlamingMaw = {
    .name = $("Flaming Maw"),
    .description = $("Strong Jaw + Flaming Jaws"),
    .onAttacker = FlamingJaws.onAttacker,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
};

static const Ability Demolitionist = {
    .name = $("Demolitionist"),
    .description = $("Readied Action + Ignores Protect\n"
                     "+ screens break on readied turn"),
    .onEntry = ReadiedAction.onEntry,
    .onInfiltrate = +[](ON_INFILTRATE) -> InfiltrateType {
        if (gVolatileStructs[battler].readiedAction && !IS_MOVE_STATUS(move)) return INFILTRATE_BREAK_SCREENS;
        return INFILTRATE_NONE;
    },
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DidMoveHit())
        CHECK(gVolatileStructs[battler].readiedAction)
        int opposingSide = GetBattlerSide(target);
        CHECK(gSideTimers[opposingSide].reflectTimer || gSideTimers[opposingSide].lightscreenTimer || gSideTimers[opposingSide].auroraVeilTimer)
        BattleScriptCall(BattleScript_AttackerShattersScreens);
        return TRUE;
    },
};

static const Ability RockhardWill = {
    .name = $("Rockhard Will"),
    .description = $("Boosts Rock-type moves by 1.2x,\n"
                     "or 1.5x when under 1/3 HP."),
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_ROCK),
};

ON_EITHER(FragrantDaze) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeConfused(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_CONFUSION, battler, opponent);
    return TRUE;
}
static const Ability FragrantDaze = {
    .name = $("Fragrant Daze"),
    .description = $("30% chance to confuse on contact."),
    ON_EITHER_ABILITY(FragrantDaze),
};

static const Ability LowVisibility = {
    .name = $("Low Visibility"),
    .description = $("Summons Eerie Fog on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_FOG, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_BadOmensActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    },
};

static const Ability OldMariner = {
    .name = $("Old Mariner"),
    .description = $("Seaweed + Water STAB."),
    .onOffensiveMultiplier = Seaweed.onOffensiveMultiplier,
    .onDefensiveMultiplier = Seaweed.onDefensiveMultiplier,
    .onStab = Amphibious.onStab,
    .breakable = TRUE,
};

static const Ability Ectoplasm = {
    .name = $("Ectoplasm"),
    .description = $("Ups highest attacking stat\n"
                     "by 1.5x in fog."),
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        },
};

static const Ability BeautifulMusic = {
    .name = $("Beautiful Music"),
    .description = $("Sound-based moves have 50% chance\n"
                     "to infatuate the foe."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 2)
        CHECK(CanInfatuate(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    },
};

static const Ability Surprise = {
    .name = $("Surprise!"),
    .description = $("Astonishes enemy priority users\n"
                     "in fog."),
};

static const Ability SnowSong = {
    .name = $("Snow Song"),
    .description = $("Sound moves get a 1.2x boost\n"
                     "and become Ice if Normal."),
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_ICE + 1;
    },
};

static const Ability GreaterSpirit = {
    .name = $("Greater Spirit"),
    .description = $("Ups highest stat by +1\n"
                     "on entry in fog."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

static const Ability Resonance = {
    .name = $("Resonance"),
    .description = $("Sound moves cause the target to\n"
                     "bleed."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

static const Ability EtherealRush = {
    .name = $("Ethereal Rush"),
    .description = $("This Pokémon's Speed gets a\n"
                     "1.5x boost in fog."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        },
};

static const Ability CuteAntecedence = {
    .name = $("Cute Antecedence"),
    .description = $("At full HP, gives +1 priority to\n"
                     "its Fairy-type moves."),
    .onPriority = GALE_WINGS_CLONE(TYPE_FAIRY),
};

static const Ability RecurringNightmare = {
    .name = $("Shallow Grave"),
    .description = $("Revives at 25% HP once after\n"
                     "fainting in fog."),
    .persistent = TRUE,
};

ON_EITHER(MenacingSituation) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK_NOT(gVolatileStructs[opponent].fear)
    CHECK(Random() % 100 < 30)

    gStackBattler1 = battler;
    gStackBattler2 = opponent;
    BattleScriptCall(BattleScript_AbilitySetFear);
    return TRUE;
}
static const Ability MenacingSituation = {
    .name = $("Menacing Situation"),
    .description = $("20% chance to Fear on contact.\n"
                     "Also works on offense."),
    ON_EITHER_ABILITY(MenacingSituation),
};

static const Ability ShinyLightning = {
    .name = $("Shiny Lightning"),
    .description = $("Grants a 1.2x accuracy boost.\n"
                     "Thunder never misses."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

static const Ability Terrify = {
    .name = $("Terrify"),
    .description = $("Lowers foes' Sp. Atk by two\n"
                     "stages on entry."),
    .onEntry = UseIntimidateClone,
};

static const Ability IceDownfall = {
    .name = $("Ice Downfall"),
    .description = $("Counters contact with\n"
                     "60BP Icicle Crash."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
        return FALSE;
    },
};

static const Ability LastStand = {
    .name = $("Last Stand"),
    .description = $("Def and SpDef increase as\n"
                     "HP drops. Max 1.6x."),
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF || statId == STAT_SPDEF)
                *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
        },
    .breakable = TRUE,
};

static const Ability PyroclasticFlow = {
    .name = $("Pyroclastic Flow"),
    .description = $("Molten Down + Corrosion."),
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        return MoltenDown.onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS) || Corrosion.onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS);
    },
};

static const Ability BloodBath = {
    .name = $("Blood Bath"),
    .description = $("Immune to bleed. Inflict fear\n"
                     "when inflicting bleed."),
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .breakable = TRUE,
};

static const Ability BattleAura = {
    .name = $("Battle Aura"),
    .description = $("Sharply increases the critical\n"
                     "hit rate for all while on the field."),
    .onCrit = +[](ON_CRIT) -> int { return 2; },
    .onCritFor = APPLY_ON_ANY,
};

static const Ability Bloodlust = {
    .name = $("Bloodlust"),
    .description = $("Blood Bath + Soul Eater."),
    .onReactive = BloodBath.onReactive,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int result = 0;
        if (battler == attacker) {
            result |= SoulEater.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        return result | BloodBath.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
    .breakable = TRUE,
};

static const Ability PiercingSolo = {
    .name = $("Piercing Solo"),
    .description = $("Sound moves have a 30%\n"
                     "chance to cause bleeding."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

static const Ability Rhythmic = {
    .name = $("Rhythmic"),
    .description = $("Deals 10% more damage for\n"
                     "each repeated move use."),
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); },
};

static const Ability ChunkyBassLine = {
    .name = $("Chunky Bass Line"),
    .description = $("Triggers a 40BP Earthquake\n"
                     "after using a sound move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
    },
};

static const Ability DualHammer = {
    .name = $("Jackhammer"),
    .description = $("Super Slammer moves hit twice\n"
                     "for 70% damage."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].hammerBased)
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

static const Ability DentingBlows = {
    .name = $("Denting Blows"),
    .description = $("Hammer moves lower Defense."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(gBattleMoves[move].hammerBased)
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
};

static const Ability IceColdHunter = {
    .name = $("Ice Cold Hunter"),
    .description = $("Ice-type moves hit twice in hail."),
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(moveType == TYPE_ICE)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
        return PARENTAL_BOND_ICE_COLD_HUNTER;
    },
};

static const Ability SoulCrusher = {
    .name = $("Soul Crusher"),
    .description = $("Hammer moves hit SpDef\n"
                     "and get a 1.1x power boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hammerBased) MUL(1.1);
        },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(gBattleMoves[move].hammerBased)
        return STAT_SPDEF;
    },
};

static const Ability ArcFlash = {
    .name = $("Arc Flash"),
    .description = $("50% chance to burn when hit or\n"
                     "paralyze when dealing damage."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(CanBeBurned(attacker))
        CHECK(Random() % 2)

        AbilityStatusEffect(MOVE_EFFECT_BURN | MOVE_EFFECT_AFFECTS_USER);
        return TRUE;
    },
};

static const Ability Unicorn = {
    .name = $("Unicorn"),
    .description = $("Mighty Horn + Dazzling."),
    .onImmune = QueenlyMajesty.onImmune,
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    .onImmuneFor = APPLY_ON_ALLY,
};

static const Ability OnTheProwl = {
    .name = $("On the Prowl"),
    .description = $("+1 priority for the first turn.\n"
                     "Negative priority becomes +0."),
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
    },
};

static const Ability Pretentious = {
    .name = $("Pretentious"),
    .description = $("Dealing a KO raises Crit by\n"
                     "one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(gVolatileStructs[battler].critBoost < 3);
        gVolatileStructs[battler].critBoost++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
        BattleScriptCall(BattleScript_AbilityBoostsCrit);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability VenoblazePincers = {
    .name = $("Venoblaze Pincers"),
    .description = $("1.2x boost to physical moves and\n"
                     "20% chance to Burn or Poison."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(Random() % 100 < 20)

        switch (Random() % 2) {
            case 0:
                CHECK(CanBeBurned(target));
                AbilityStatusEffect(MOVE_EFFECT_BURN);
                return TRUE;

            case 1:
                CHECK(CanBePoisoned(battler, target))
                AbilityStatusEffect(MOVE_EFFECT_TOXIC);
                return TRUE;
        }
        return FALSE;
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.2);
        },
};

static const Ability EternalBlessing = {
    .name = $("Eternal Blessing"),
    .description = $("Celestial Blessing + Regenerator."),
    .onEndTurn = CelestialBlessing.onEndTurn,
    .onExit = Regenerator.onExit,
    .persistent = TRUE,
};

static const Ability SugarRush = {
    .name = $("Sugar Rush"),
    .description = $("Gluttony + eats foe's berry when\n"
                     "hitting with contact move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler)) CHECK(IsMoveMakingContact(move, battler)) CHECK(EatTargetBerry(battler, target));
        return TRUE;
    },
};

static const Ability PeacefulRest = {
    .name = $("Rest in Peace"),
    .description = $("Heals 1/8 of max HP every turn\n"
                     "in fog."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
};

static const Ability WhiteNoise = {
    .name = $("White Noise"),
    .description = $("Static + Rest in Peace."),
    .onEndTurn = PeacefulRest.onEndTurn,
    .onAttacker = Static.onAttacker,
    .onDefender = Static.onDefender,
};

static const Ability SmokeyManeuvers = {
    .name = $("Smokey Maneuvers"),
    .description = $("Evasion is boosted by 1.25x\n"
                     "in fog."),
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

static const Ability Tag = {
    .name = $("Tag"),
    .description = $("Attacks switching opponents\n"
                     "with a 20BP Pursuit."),
};

static const Ability PowerMetal = {
    .name = $("Power Metal"),
    .description = $("Sound moves get a 1.2x boost\n"
                     "and become Steel if Normal."),
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_STEEL + 1;
    },
};

static const Ability PowerEdge = {
    .name = $("Power Edge"),
    .description = $("Keen Edge moves target Special\n"
                     "Defense and get a 1.3x boost."),
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
};

static const Ability Superconductor = {
    .name = $("Superconductor"),
    .description = $("Steel-type moves become Electric\n"
                     "-type moves and get a 1.1x boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        },
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_STEEL)
        *ateBoost = TRUE;
        return TYPE_ELECTRIC + 1;
    },
};

static const Ability UltraInstinct = {
    .name = $("Ultra Instinct"),
    .description = $("Counters contact with Vacuum\n"
                     "Wave. Takes 20% less damage."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
        return FALSE;
    },
    .onDefensiveMultiplier = Parry.onDefensiveMultiplier,
};

static const Ability UnlockedPotential = {
    .name = $("Unlocked Potential"),
    .description = $("Inner Focus + Berserk."),
    .onDefender = Berserk.onDefender,
    .onAccuracy = InnerFocus.onAccuracy,
};

static const Ability HigherRank = {
    .name = $("Higher Rank"),
    .description = $("Priority moves get a 1.2x boost."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
        },
};

static const Ability FuneralPyre = {
    .name = $("Funeral Pyre"),
    .description = $("Non-Ghost and Dark-types\n"
                     "take 1/4 damage every turn."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE); },
};

static const Ability FlameBubble = {
    .name = $("Flame Bubble"),
    .description = $("Water Bubble + Flaming Soul."),
    .onOffensiveMultiplier = WaterBubble.onOffensiveMultiplier,
    .onDefensiveMultiplier = WaterBubble.onDefensiveMultiplier,
    .onPriority = FlamingSoul.onPriority,
};

static const Ability ElementalVortex = {
    .name = $("Elemental Vortex"),
    .description = $("Flash Fire + Water Absorb."),
    .onAbsorb = +[](ON_ABSORB) -> int { return WaterAbsorb.onAbsorb(DELEGATE_ABSORB) || FlashFire.onAbsorb(DELEGATE_ABSORB); },
    .onOffensiveMultiplier = FlashFire.onOffensiveMultiplier,
};

static const Ability SnowyWrath = {
    .name = $("Snowy Wrath"),
    .description = $("Snow Warning + Cryomancy."),
    .onEntry = SnowWarning.onEntry,
};

static const Ability PatternChange = {
    .name = $("Pattern Change"),
    .description = $("Changes type depending on the\n"
                     "move it's about to use."),
    .protean = TRUE,
};

static const Ability NoTurningBack = {
    .name = $("No Turning Back"),
    .description = $("Boosts all stats but can't retreat\n"
                     "when below 1/2 max HP."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_NoTurningBack);
        return TRUE;
    },
};

static const Ability FlammableCoat = {
    .name = $("Flammable Coat"),
    .description = $("Changes forms when using or\n"
                     "hit by a Fire-type move."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_FIRE)
        CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[battler].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_TargetFormChange);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability DracoMorale = {
    .name = $("Draco Morale"),
    .description = $("Uses Dragon Cheer\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DRAGON_CHEER, 0); },
};

static const Ability BadOmen = {
    .name = $("Bad Omen"),
    .description = $("Foes min roll and may miss.\n"
                     "Takes 1/4 damage from crits."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (isCrit) MUL(.25);
        },
    .onAccuracy = BadLuck.onAccuracy,
    .onAccuracyFor = APPLY_ON_FOE,
    .breakable = TRUE,
};

static const Ability MoshPit = {
    .name = $("Mosh Pit"),
    .description = $("Ally's attacks get a 1.25x boost.\n"
                     "1.5x if attack causes recoil."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST)
                MUL(1.25);
            else
                MUL(1.5);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
};

ON_EITHER(BloodStain) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_BLOOD_STAIN))
    CHECK_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(opponent)))
    CHECK_NOT(BattlerHasAbility(battler, ability, FALSE))
    CHECK_NOT(DoesBattlerHaveAbilityShield(opponent))

    UpdateAbilityStateIndicesForNewAbility(opponent, ability);
    ReplaceAbility(opponent, ability);
    gStackBattler1 = opponent;
    BattleScriptCall(BattleScript_BloodStainActivates);
    DisableSwitchInAbility(opponent, ability);
    return TRUE;
}
static const Ability BloodStain = {
    .name = $("Blood Stain"),
    .description = $("Bleeds if not immune. Can't get\n"
                     "other status. Spreads on contact."),
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN); },
    ON_EITHER_ABILITY(BloodStain),
    .unsuppressable = TRUE,
};

static const Ability BloodStigma = {
    .name = $("Blood Stigma"),
    .description = $("Immune to status. Gets a 50%\n"
                     "boost vs bleeding foes."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(1.5);
        },
    .unsuppressable = TRUE,
};

static const Ability MaximumAcceleration = {
    .name = $("Max Acceleration"),
    .description = $("Moves use the Speed stat\n"
                     "for damage calculations."),
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { *atkStatToUse = STAT_SPEED; },
};

static const Ability Sidewinder = {
    .name = $("Sidewinder"),
    .description = $("First biting move each entry gets\n"
                     "+1 priority. Resets on KO."),
    .onEntry = CoilUp.onEntry,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
        gStatuses4[battler] |= STATUS4_COILED;
        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Petrify = {
    .name = $("Petrify"),
    .description = $("Clears stat buffs then lowers\n"
                     "speed by one stage on entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        int loweredStats = 0;
        int intimidated = UseIntimidateClone(battler, ability);
        for (int i = BATTLE_OPPOSITE(GET_BATTLER_SIDE(battler)); i < gBattlersCount; i += 2) {
            if (!IsBattlerAlive(i)) continue;
            loweredStats |= TryResetBattlerStatChanges(i, RESET_STAT_BUFFS);
        }

        if (loweredStats) {
            BattleScriptPushCursorAndCallback(BattleScript_Petrify);
        }
        return intimidated || loweredStats;
    },
};

static const Ability Fluffiest = {
    .name = $("Fluffiest"),
    .description = $("Quarters contact damage taken.\n"
                     "4x weak to fire."),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

static const Ability WayOfPrecision = {
    .name = $("Way of Precision"),
    .description = $("Inner Focus + Precise Fist."),
    .onAccuracy = InnerFocus.onAccuracy,
    .onCrit = PreciseFist.onCrit,
    .breakable = TRUE,
};

static const Ability WayOfSwiftness = {
    .name = $("Way of Swiftness"),
    .description = $("Pretentious + Swift Swim."),
    .onBattlerFaints = Pretentious.onBattlerFaints,
    .onStat = SwiftSwim.onStat,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability AtomicPunch = {
    .name = $("Atomic Punch"),
    .description = $("Iron Fist + Steelworker."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            IronFist.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Steelworker.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

static const Ability IronGiant = {
    .name = $("Iron Giant"),
    .description = $("Heatproof + Juggernaut."),
    .onDefensiveMultiplier = Heatproof.onDefensiveMultiplier,
    .onChooseOffensiveStat = Juggernaut.onChooseOffensiveStat,
    .breakable = TRUE,
};

static const Ability MasterHand = {
    .name = $("Master Hand"),
    .description = $("Mega Launcher + Rampage."),
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability FinalBlow = {
    .name = $("Final Blow"),
    .description = $("Unseen Fist + Fatal Precision."),
    .onOffensiveMultiplier = FatalPrecision.onOffensiveMultiplier,
    .onAccuracy = FatalPrecision.onAccuracy,
};

static const Ability Hospitality = {
    .name = $("Hospitality"),
    .description = $("Heals partner for 25% of its max\n"
                     "HP on switch-in."),
};

static const Ability ButterUp = {
    .name = $("Butter Up"),
    .description = $("Hospitality + Soothing Aroma"),
    .onEntry = +[](ON_ENTRY) -> int { return Hospitality.onEntry(DELEGATE_ENTRY) | SoothingAroma.onEntry(DELEGATE_ENTRY); },
};

static const Ability VitalityStrike = {
    .name = $("Vitality Strike"),
    .description = $("Heals for 10% of the damage\n"
                     "dealt by punching moves."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))
        CHECK(IS_IRON_FIST(battler, move))

        gBattleMoveDamage = -gHpDealt / 10;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

static const Ability HugeWings = {
    .name = $("Huge Wings"),
    .description = $("Giant Wings + Levitate."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            GiantWings.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Levitate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .breakable = TRUE,
};

static const Ability SwordOfDamnation = {
    .name = $("Sword of Damnation"),
    .description = $("Unaware + Sword of Ruin."),
    .onStat = SwordOfRuin.onStat,
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
    .unaware = TRUE,
};

static const Ability RestrainingOrder = {
    .name = $("Restraining Order"),
    .description = $("Forces the attacker when hit\n"
                     "once each switch-in."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(GetAbilityState(battler, ability) == RESTRAINING_ORDER_NOT_TRIGGERED)
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
        CHECK(CountUsablePartyMons(battler))

        SetAbilityState(battler, ability, RESTRAINING_ORDER_ACTIVATING);
        return FALSE;
    },
};

static const Ability AssassinsTools = {
    .name = $("Assassin's Tools"),
    .description = $("Contact moves have a 30%\n"
                     "chance to PSN, PRLZ, or BLD."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IsMoveMakingContact(move, battler))

        switch (Random() % 3) {
            case 0:
                CHECK(CanBePoisoned(battler, target));
                AbilityStatusEffect(MOVE_EFFECT_POISON);
                return TRUE;

            case 1:
                CHECK(CanBeParalyzed(battler, target))
                AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
                return TRUE;

            case 2:
                CHECK(CanBleed(target))
                AbilityStatusEffect(MOVE_EFFECT_BLEED);
                return TRUE;
        }
        return FALSE;
    },
};

static const Ability Frostmaw = {
    .name = $("Frostmaw"),
    .description = $("Biting moves have a 50% chance\n"
                     "to inflict frostbite."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanGetFrostbite(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
    },
};

static const Ability Patchwork = {
    .name = $("Patchwork"),
    .description = $("Disguise + curses the opponent\n"
                     "when its Disguise breaks."),
    .onEntry = Disguise.onEntry,
    .onDisguise = +[](ON_DISGUISE) -> int {
        int species = Disguise.onDisguise(DELEGATE_DISGUISE);
        if (species && !testOnly) {
            SetOncePerTurnAbilityCounter(battler, ABILITY_PATCHWORK, gBattlerAttacker);
        }
        return species;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(GetSingleUseAbilityCounter(battler, ability))
        SetSingleUseAbilityCounter(battler, ability, 0);

        CHECK(IsBattlerAlive(attacker))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)

        AbilityStatusEffect(MOVE_EFFECT_CURSE | MOVE_EFFECT_AFFECTS_USER);
        return TRUE;
    },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static const Ability BlindRage = {
    .name = $("Blind Rage"),
    .description = $("Scrappy + Mold Breaker."),
    .onEntry = MoldBreaker.onEntry,
    .onTypeEffectiveness = Scrappy.onTypeEffectiveness,
};

static const Ability Slipstream = {
    .name = $("Slipstream"),
    .description = $("Moves use 20% of its Speed\n"
                     "stat additionally."),
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { *secondaryAtkStatToUse = STAT_SPEED; },
};

static const Ability ApexPredator = {
    .name = $("Apex Predator"),
    .description = $("Tough Claws + Predator."),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onOffensiveMultiplier = ToughClaws.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability DragonsRitual = {
    .name = $("Dragon's Ritual"),
    .description = $("Dealing a KO raises Attack and\n"
                     "Speed by one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
        BattleScriptCall(BattleScript_DragonsRitual);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability PinnacleBlade = {
    .name = $("Pinnacle Blade"),
    .description = $("Slashing moves always hit and\n"
                     "break protection and barriers."),
    .onInfiltrate = +[](ON_INFILTRATE) -> InfiltrateType {
        return gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST ? INFILTRATE_BREAK_SCREENS | INFILTRATE_SUBSTITUTE : INFILTRATE_NONE;
    },
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DidMoveHit())
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)

        int shouldApply = FALSE;
        int opposingSide = GetBattlerSide(target);

        if (gVolatileStructs[target].substituteHP) {
            gVolatileStructs[target].substituteHP = 0;
            BattleScriptCall(BattleScript_AttackerDestroysSubstitute);
            shouldApply = TRUE;
        }

        if (IsBattlerAlive(target)) {
            if (gSideTimers[opposingSide].reflectTimer || gSideTimers[opposingSide].lightscreenTimer || gSideTimers[opposingSide].auroraVeilTimer) {
                BattleScriptCall(BattleScript_AttackerShattersScreens);
                shouldApply = TRUE;
            }

            if (IS_BATTLER_PROTECTED(target)) {
                AbilityStatusEffectDirect(MOVE_EFFECT_FEINT);
                shouldApply = TRUE;
            }
        }

        return shouldApply;
    },
};

static const Ability Energized = {
    .name = $("Energized"),
    .description = $("Generator + charges up on KO\n"
                     "with an Electric-type move."),
    .onEntry = Generator.onEntry,
    .onTerrain = Generator.onTerrain,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        SetOncePerTurnAbilityCounter(battler, ability, TRUE);
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    },
    .onExit = Generator.onExit,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .persistent = TRUE,
};

static const Ability ColorSpectrum = {
    .name = $("Color Spectrum"),
    .description = $("Same-type attacks get a 1.2x\n"
                     "boost. Changes type each turn."),
    .onEndTurn = +[](ON_END_TURN) -> int {
        int newType;
        do {
            newType = Random() % NUMBER_OF_MON_TYPES;
        } while (newType == TYPE_MYSTERY || newType == TYPE_STELLAR || IS_BATTLER_OF_TYPE(battler, newType));

        gBattleMons[battler].type1 = newType;
        gBattleMons[battler].type2 = newType;
        gBattleMons[battler].type3 = TYPE_MYSTERY;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, newType);
        BattleScriptPushCursorAndCallback(BattleScript_AttackerBecameTheTypeFullEnd3);
        return TRUE;
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (StabMultiplierInHalves(battler, moveType, move) > 2) MUL(1.2);
        },
};

static const Ability SteelBeetle = {
    .name = $("Steel Beetle"),
    .description = $("Raging Boxer + Pollinate."),
    .onParentalBond = RagingBoxer.onParentalBond,
    .onOffensiveMultiplier = Pollinate.onOffensiveMultiplier,
    .onMoveType = Pollinate.onMoveType,
};

static const Ability FromTheShadows = {
    .name = $("From the Shadows"),
    .description = $("Attacks trap and have a 20%\n"
                     "flinch chance when moving first."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(GetBattlerTurnOrderNum(target) >= gCurrentTurnActionNumber)

        if (CanMoveHaveExtraFlinchChance(move) && Random() % 100 < 20) {
            AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
        }

        CHECK_NOT(gBattleMons[target].status2 & STATUS2_ESCAPE_PREVENTION)
        gBattleMons[target].status2 |= STATUS2_ESCAPE_PREVENTION;
        gVolatileStructs[target].battlerPreventingEscape = battler;
        BattleScriptCall(BattleScript_AnnounceTargetTrapped);
        return TRUE;
    },
};

static const Ability RagePoint = {
    .name = $("Rage Point"),
    .description = $("Gets a 1.5x boost while statused.\n"
                     "Raises offenses when crit."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gIsCriticalHit)
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK))

        BattleScriptCall(BattleScript_RagePointActivates);
        return TRUE;
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler)) MUL(1.5);
        },
};

static const Ability HotCoals = {
    .name = $("Hot Coals"),
    .description = $("Sets a trap that burns the next\n"
                     "foe that switches in."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

        gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
    },
};

static const Ability TerastalTreasure = {
    .name = $("Terastal Treasure"),
    .description = $("Reduces damage taken by 40%,\n"
                     "but lowers speed by 20%."),
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.6); },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= .8;
        },
    .breakable = TRUE,
};

static const Ability ShockingMaw = {
    .name = $("Shocking Maw"),
    .description = $("Strong Jaw + Bite moves have\n"
                     "50% paralysis chance."),
    .onAttacker = ShockingJaws.onAttacker,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
};

static const Ability GleamEyes = {
    .name = $("Gleam Eyes"),
    .description = $("Frisk + Scare."),
    .onEntry = +[](ON_ENTRY) -> int { return UseIntimidateClone(battler, ability) | Frisk.onEntry(DELEGATE_ENTRY); },
};

static const Ability RousedFangs = {
    .name = $("Megabite"),
    .description = $("Biting moves use SpAtk and\n"
                     "deal 30% more damage."),
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    .onChooseOffensiveStat = MindCrush.onChooseOffensiveStat,
};

static const Ability DreamState = {
    .name = $("Dream State"),
    .description = $("Immune to critical hits. Takes\n"
                     "20% less damage from all attacks."),
    .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier,
    .onCrit = BattleArmor.onCrit,
    .onCritFor = BattleArmor.onCritFor,
    .breakable = TRUE,
};

static const Ability DreamWhimsy = {
    .name = $("Dream Whimsy"),
    .description = $("Uses Yawn on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_YAWN, 0); },
};

static const Ability LunarAffinity = {
    .name = $("Lunar Affinity"),
    .description = $("Copies lunar moves used by\n"
                     "others."),
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(gBattleMoves[move].lunar)
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
};

static const Ability FlameShield = {
    .name = $("Flame Shield"),
    .description = $("Takes 35% less damage from\n"
                     "Super-effective moves."),
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability AquaticDweller = {
    .name = $("Aquatic Dweller"),
    .description = $("Boosts the power of Water-type\n"
                     "moves by 1.5x."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_WATER); },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) MUL(1.5);
        },
};

static const Ability ApplePie = {
    .name = $("Apple Pie"),
    .description = $("Self Sufficient + Ripen."),
    .onEndTurn = SelfSufficient.onEndTurn,
};

static const Ability Hover = {
    .name = $("Hover"),
    .description = $("Adds Psychic type to itself.\n"
                     "Avoids Ground attacks."),
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_PSYCHIC); },
    .breakable = TRUE,
};

static const Ability Depravity = {
    .name = $("Depravity"),
    .description = $("Merciless + Overcharge."),
    .onCrit = Merciless.onCrit,
    .onTypeEffectiveness = Overcharge.onTypeEffectiveness,
};

static const Ability Wildfire = {
    .name = $("Wildfire"),
    .description = $("Attacks with 20BP Fire Spin\n"
                     "when hit by a contact move."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_FIRE_SPIN, 20);
        return FALSE;
    },
};

static const Ability JumpScare = {
    .name = $("Jumpscare"),
    .description = $("Attacks with Astonish on first\n"
                     "switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
    },
};

static const Ability TarToss = {
    .name = $("Tar Toss"),
    .description = $("Uses Tar Shot on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TAR_SHOT, 0); },
};

static const Ability StunShock = {
    .name = $("Stun Shock"),
    .description = $("Attacks have a 60% chance to\n"
                     "Paralyze or Poison."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target)) CHECK(Random() % 100 < 60) switch (Random() % 2) {
            case 0:
                CHECK(CanBePoisoned(battler, target));
                AbilityStatusEffect(MOVE_EFFECT_POISON);
                return TRUE;

            case 1:
                CHECK(CanBeParalyzed(battler, target))
                AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
                return TRUE;
        }
        return FALSE;
    },
};

static const Ability RagingGoddess = {
    .name = $("Raging Goddess"),
    .description = $("Rampage + Hyper Aggressive."),
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onParentalBond = ParentalBond.onParentalBond,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

static const Ability Whiplash = {
    .name = $("Whiplash"),
    .description = $("Physical attacks have a 50%\n"
                     "chance to lower Defense."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 2)
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
};

static const Ability SupersweetSyrup = {
    .name = $("Supersweet Syrup"),
    .description = $("Can't lose its item. Disables foe's\n"
                     "item for 2 turns on contact."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_EMBARGO)
        CHECK(gBattleMons[attacker].item)

        gVolatileStructs[attacker].embargoTimer = 2;
        gStatuses3[attacker] |= STATUS3_EMBARGO;
        gLastUsedItem = gBattleMons[attacker].item;
        BattleScriptCall(BattleScript_AnnounceAttackerItemDisabled);
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability LuckyHalo = {
    .name = $("Lucky Halo"),
    .description = $("Negates self stat drops. Survives\n"
                     "the first hit that would KO it."),
};

static const Ability TrashHeap = {
    .name = $("Trash Heap"),
    .description = $("Corrosion + Toxic Spill."),
    .onEntry = ToxicSpill.onEntry,
    .onExit = ToxicSpill.onExit,
    .onTypeEffectiveness = Corrosion.onTypeEffectiveness,
};

static const Ability SludgyMix = {
    .name = $("Sludgy Mix"),
    .description = $("Intoxicate + Punk Rock."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Intoxicate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            PunkRock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onMoveType = Intoxicate.onMoveType,
};

static const Ability Overwatch = {
    .name = $("Overwatch"),
    .description = $("On the Prowl + Stakeout."),
    .onEntry = OnTheProwl.onEntry,
    .onOffensiveMultiplier = Stakeout.onOffensiveMultiplier,
};

static const Ability WindRage = {
    .name = $("Wind Rage"),
    .description = $("Uses Defog on switch-in. Air-\n"
                     "based moves get a 1.3x boost."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); },
    .onOffensiveMultiplier = GiantWings.onOffensiveMultiplier,
};

static const Ability VictoryBomb = {
    .name = $("Victory Bomb"),
    .description = $("Attacks with a 100BP Fire-type\n"
                     "Explosion on fainting."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK_NOT(IsBattlerAlive(battler))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_EXPLOSION, 100);
        return FALSE;
    },
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(gProcessingExtraAttacks)
        CHECK(gQueuedExtraAttackData[0].ability == ability)
        return TYPE_FIRE + 1;
    },
};

static const Ability RazorSharp = {
    .name = $("Razor Sharp"),
    .description = $("Critical hits also inflict bleeding."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gIsCriticalHit)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

static const Ability ToTheBone = {
    .name = $("To The Bone"),
    .description = $("Critical hits get a 1.5x boost and\n"
                     "inflict bleeding."),
    .onAttacker = RazorSharp.onAttacker,
    .onOffensiveMultiplier = Sniper.onOffensiveMultiplier,
};

static const Ability BladeDance = {
    .name = $("Blade Dance"),
    .description = $("Triggers 50 BP Leaf Blade after\n"
                     "using a dance move."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
    },
};

static const Ability Taekkyeon = {
    .name = $("Taekkyeon"),
    .description = $("All attacks are dances."),
};

int ApeShiftHandler(int battler, AbilityCallType callType) {
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)
    CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA || gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
    CHECK(ShouldChangeFormHpBased(battler))

    InsertCorrectEndType(callType);

    gStackBattler1 = battler;
    if (gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT) {
        BattleScriptCall(BattleScript_ApeShift);
    }
    BattleScriptCall(BattleScript_StackBattlerFormChange);
    return TRUE;
}
static const Ability ApeShift = {
    .name = $("Ape Shift"),
    .description = $("Transforms when below 50% HP,\n"
                     "curing status and always critting."),
    .onEntry = +[](ON_ENTRY) -> int { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onEndTurn = +[](ON_END_TURN) -> int { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDefender = +[](ON_DEFENDER) -> int { return ApeShiftHandler(battler, ABILITY_BS_CALL); },
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
        return ALWAYS_CRIT;
    },
    .randomizerBanned = TRUE,
};

static const Ability KnowYourPlace = {
    .name = $("Know Your Place"),
    .description = $("Contact attacks make foes move\n"
                     "last for 5 turns."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gVolatileStructs[target].dazed)
        CHECK(IsMoveMakingContact(move, battler))

        gVolatileStructs[target].dazed = 5;
        BattleScriptCall(BattleScript_TargetDazed);
        return TRUE;
    },
};

static const Ability DeepCuts = {
    .name = $("Deep Cuts"),
    .description = $("Slashing moves have a 50%\n"
                     "chance to inflict bleeding."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

static const Ability LifeSteal = {
    .name = $("Life Steal"),
    .description = $("Steals 1/10 HP from foes each\n"
                     "turn."),
};

static const Ability RudeAwakening = {
    .name = $("Rude Awakening"),
    .description = $("Raises all stats becomes immune\n"
                     "to sleep after waking up."),
};

static const Ability TeraformZero = {
    .name = $("Teraform Zero"),
    .description = $("Tera Shell + clears weather and\n"
                     "terrain on first entry."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(!GetSingleUseAbilityCounter(battler, ability));
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
        BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
        return TRUE;
    },
    .breakable = TRUE,
};

static const Ability SetAblaze = {
    .name = $("Set Ablaze"),
    .description = $("Inflicting burn also inflicts fear."),
    .onReactive = BloodBath.onReactive,
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
};

static const Ability Breakwater = {
    .name = $("Breakwater"),
    .description = $("Swift Swim + Stall."),
    .onDefensiveMultiplier = Stall.onDefensiveMultiplier,
    .onStat = SwiftSwim.onStat,
    .breakable = TRUE,
};

static const Ability MagicalFists = {
    .name = $("Magical Fists"),
    .description = $("Punching moves use Special\n"
                     "Attack and get a 1.3x boost."),
    .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (IS_IRON_FIST(battler, move)) *atkStatToUse = STAT_SPATK;
        },
};

static const Ability Cutthroat = {
    .name = $("Cutthroat"),
    .description = $("The first slicing move used on\n"
                     "each entry in gets +1 priority."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
    },
};

static const Ability SandBender = {
    .name = $("Sand Bender"),
    .description = $("Sand Stream + Sand Force."),
    .onEntry = SandStream.onEntry,
    .onStat = SandForce.onStat,
};

static const Ability SandPit = {
    .name = $("Sand Pit"),
    .description = $("Attacks with 20BP Sand Tomb\n"
                     "on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SAND_TOMB, 20); },
};

static const Ability DesolateSun = {
    .name = $("Desolate Sun"),
    .description = $("Desolate Land + Earth Eater."),
    .randomizerBanned = TRUE,
};

ON_EITHER(Daybreak) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeBurned(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))

    AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
    return TRUE;
}
static const Ability Daybreak = {
    .name = $("Daybreak"),
    .description = $("Burns the foe on contact.\n"
                     "Also works on offense."),
    ON_EITHER_ABILITY(Daybreak),
};

static const Ability EnergySiphon = {
    .name = $("Energy Siphon"),
    .description = $("Heals the user for 1/4\n"
                     "of the damage they deal."),
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK_NOT(BATTLER_HEALING_BLOCKED(battler))

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

static const Ability Reservoir = {
    .name = $("Reservoir"),
    .description = $("Water Absorb + Storm Drain."),
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT | ABSORB_RESULT_HEAL;
    },
    .redirectType = TYPE_WATER,
    .breakable = TRUE,
};

static int NeurotoxinCondition(int battler, int target) {
    return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_DEF) || CanLowerStat(target, STAT_SPEED);
}
static const Ability Neurotoxin = {
    .name = $("Neurotoxin"),
    .description = $("Inflicting poison also lowers\n"
                     "Attack, Defense, and Speed."),
    .onReactive = +[](ON_REACTIVE) -> int { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
};

static const Ability EnergizedHorns = {
    .name = $("Energy Horns"),
    .description = $("Mighty horn moves become special\n"
                     "and deal 30% more damage."),
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].hornBased);
        return TRUE;
    },
};

static const Ability SpiderLairUpgrade = {
    .name = $("Rising Dough"),
    .description = $("Casts Sticky Web on entry.\n"
                     "Lasts 7 turns."),
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

        int side = BATTLE_OPPOSITE(battler);
        gSideTimers[side].started.spiderWeb = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[side].stickyWebTimer = 7;
        BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
        return TRUE;
    },
};

static const Ability CrustCoat = {
    .name = $("Crust Coat"),
    .description = $("Immune to critical hits. Takes\n"
                     "20% less damage from all attacks."),
    .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier,
    .onCrit = BattleArmor.onCrit,
    .onCritFor = BattleArmor.onCritFor,
    .breakable = TRUE,
};

static const Ability Puffy = {
    .name = $("Puffy"),
    .description = $("Takes 1/2 dmg from contact moves\n"
                     "but Fire moves hurt it 2x more."),
    .onDefensiveMultiplier = Fluffy.onDefensiveMultiplier,
    .breakable = TRUE,
};

static const Ability BalloonBlitz = {
    .name = $("Balloon Blitz"),
    .description = $("Inflatable + Hyper Aggressive."),
    .onDefender = Inflatable.onDefender,
    .onParentalBond = ParentalBond.onParentalBond,
};

static const Ability StrikerPixilate = {
    .name = $("Twinkle Toes"),
    .description = $("Boosts the power of kicking\n"
                     "moves by 1.3x + Pixilate."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Striker.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Pixilate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onMoveType = Pixilate.onMoveType,
};

// 2.6
static const Ability DoomBlast = {
    .name = $("Doom Blast"),
    .description = $("Boosts own Dark moves by 1.35x,\n"
                     "takes 10% of dmg dealt as recoil."),
    .onRecoil = +[](ON_RECOIL) -> int {
        CHECK(moveType == TYPE_DARK);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_DARK) MUL(1.35);
        },
};

static const Ability Bruteforce = {
    .name = $("Brute Force"),
    .description = $("Rock Head + Reckless"),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
        },
    .noRecoil = TRUE,
};

static const Ability FaradayCage = {
    .name = $("Faraday Cage"),
    .description = $("Shell Armor + 50BP Thunder\n"
                     "Cage when hit by contact."),
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
        return FALSE;
    },
    .onDefensiveMultiplier = ShellArmor.onDefensiveMultiplier,
    .onCrit = ShellArmor.onCrit,
    .onCritFor = ShellArmor.onCritFor,
    .breakable = TRUE,
};

static const Ability AcidicSlime = {
    .name = $("Acidic Slime"),
    .description = $("Corrosion + Poison STAB."),
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
    .onTypeEffectiveness = Corrosion.onTypeEffectiveness,
};

static const Ability RoseGarden = {
    .name = $("Rose Garden"),
    .description = $("Spreads two layers of\n"
                     "Toxic Spikes on switch-in."),
    .onEntry = +[](ON_ENTRY) -> int {
        u8 targetSide = GetBattlerSide(BATTLE_OPPOSITE(battler));
        CHECK(gSideTimers[targetSide].toxicSpikesAmount < 2)

        gSideTimers[targetSide].toxicSpikesAmount = 2;
        gSideStatuses[targetSide] |= SIDE_STATUS_TOXIC_SPIKES;
        // TODO: Fix display message
        BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
        return TRUE;
    },
};

static const Ability Qigong = {
    .name = $("Qigong"),
    .description = $("Always hits. Fighting Spirit\n"
                     "+ Rampage."),
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onOffensiveMultiplier = FightingSpirit.onOffensiveMultiplier,
    .onMoveType = FightingSpirit.onMoveType,
    .onAccuracy = +[](ON_ACCURACY) { return ACCURACY_ALWAYS_HITS; },
    .onBattlerFaintsFor = Rampage.onBattlerFaintsFor,
};

static const Ability ConjurerOfDeceit = {
    .name = $("Conjurer Of Deceit"),
    .description = $("Magic Guard + Magic Bounce"),
    .breakable = TRUE,
    .magicGuard = TRUE,
    .magicBounce = TRUE,
};

static const Ability DeepFreeze = {
    .name = $("Deep Freeze"),
    .description = $("Boosts Water and Ice by 1.25x.\n"
                     "Halves Fire damage taken."),
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) MUL(1.25);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

static const Ability SoulDevourer = {
    .name = $("Soul Devourer"),
    .description = $("Soul Eater + Phantom Pain"),
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onTypeEffectiveness = PhantomPain.onTypeEffectiveness,
    .onBattlerFaintsFor = SoulEater.onBattlerFaintsFor,
};

static const Ability ChampionsEntrance = {
    .name = $("Champion's Entrance"),
    .description = $("Intimidate + Violent Rush"),
    .onEntry = +[](ON_ENTRY) -> int { return Intimidate.onEntry(DELEGATE_ENTRY) | ViolentRush.onEntry(DELEGATE_ENTRY); },
};

static const Ability Presto = {
    .name = $("Presto"),
    .description = $("Sound moves get +1 priority\n"
                     "at full HP."),
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(BATTLER_MAX_HP(battler))
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        return 1;
    },
};

static const Ability Samba = {
    .name = $("Samba"),
    .description = $("Striker + Dancer"),
    .onOffensiveMultiplier = Striker.onOffensiveMultiplier,
    .onCopyMove = Dancer.onCopyMove,
};

static const Ability JunshiSanda = {
    .name = $("JunshiSanda"),
    .description = $("Placeholder"),
    .randomizerBanned = TRUE,
};

static const Ability Gladiator = {
    .name = $("Gladiator"),
    .description = $("Boosts Fighting-type moves by 1.3x,\n"
                     "or 1.8x when below 1/3 HP."),
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIGHTING),
};

static const Ability ForsakenHeart = {
    .name = $("Forsaken Heart"),
    .description = $("KOs dealt anywhere on the field\n"
                     "raise Attack by one stage."),
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
};

static const Ability Relentless = {
    .name = $("Relentless"),
    .description = $("Exploit Weakness + Merciless"),
    .onOffensiveMultiplier = ExploitWeakness.onOffensiveMultiplier,
    .onCrit = Merciless.onCrit,
};

static const Ability Soothsayer = {
    .name = $("Soothsayer"),
    .description = $("Placeholder"),
    .randomizerBanned = TRUE,
};

static const Ability CorruptedMind = {
    .name = $("Corrupted Mind"),
    .description = $("Placeholder"),
    .randomizerBanned = TRUE,
};

static const Ability FlameCoat = {
    .name = $("Flame Coat"),
    .description = $("Placeholder"),
    .randomizerBanned = TRUE,
};

static const Ability UnownPower = {
    .name = $("Unown Power"),
    .description = $("Placeholder"),
    .randomizerBanned = TRUE,
};

const Ability gAbilities[] = {
    [ABILITY_NONE] = None,
    [ABILITY_STENCH] = Stench,
    [ABILITY_DRIZZLE] = Drizzle,
    [ABILITY_SPEED_BOOST] = SpeedBoost,
    [ABILITY_BATTLE_ARMOR] = BattleArmor,
    [ABILITY_STURDY] = Sturdy,
    [ABILITY_DAMP] = Damp,
    [ABILITY_LIMBER] = Limber,
    [ABILITY_SAND_VEIL] = SandVeil,
    [ABILITY_STATIC] = Static,
    [ABILITY_VOLT_ABSORB] = VoltAbsorb,
    [ABILITY_WATER_ABSORB] = WaterAbsorb,
    [ABILITY_OBLIVIOUS] = Oblivious,
    [ABILITY_CLOUD_NINE] = CloudNine,
    [ABILITY_COMPOUND_EYES] = CompoundEyes,
    [ABILITY_INSOMNIA] = Insomnia,
    [ABILITY_COLOR_CHANGE] = ColorChange,
    [ABILITY_IMMUNITY] = Immunity,
    [ABILITY_FLASH_FIRE] = FlashFire,
    [ABILITY_SHIELD_DUST] = ShieldDust,
    [ABILITY_OWN_TEMPO] = OwnTempo,
    [ABILITY_SUCTION_CUPS] = SuctionCups,
    [ABILITY_INTIMIDATE] = Intimidate,
    [ABILITY_SHADOW_TAG] = ShadowTag,
    [ABILITY_ROUGH_SKIN] = RoughSkin,
    [ABILITY_WONDER_GUARD] = WonderGuard,
    [ABILITY_LEVITATE] = Levitate,
    [ABILITY_EFFECT_SPORE] = EffectSpore,
    [ABILITY_SYNCHRONIZE] = Synchronize,
    [ABILITY_CLEAR_BODY] = ClearBody,
    [ABILITY_NATURAL_CURE] = NaturalCure,
    [ABILITY_LIGHTNING_ROD] = LightningRod,
    [ABILITY_SERENE_GRACE] = SereneGrace,
    [ABILITY_SWIFT_SWIM] = SwiftSwim,
    [ABILITY_CHLOROPHYLL] = Chlorophyll,
    [ABILITY_ILLUMINATE] = Illuminate,
    [ABILITY_TRACE] = Trace,
    [ABILITY_HUGE_POWER] = HugePower,
    [ABILITY_POISON_POINT] = PoisonPoint,
    [ABILITY_INNER_FOCUS] = InnerFocus,
    [ABILITY_MAGMA_ARMOR] = MagmaArmor,
    [ABILITY_WATER_VEIL] = WaterVeil,
    [ABILITY_MAGNET_PULL] = MagnetPull,
    [ABILITY_SOUNDPROOF] = Soundproof,
    [ABILITY_RAIN_DISH] = RainDish,
    [ABILITY_SAND_STREAM] = SandStream,
    [ABILITY_PRESSURE] = Pressure,
    [ABILITY_THICK_FAT] = ThickFat,
    [ABILITY_EARLY_BIRD] = EarlyBird,
    [ABILITY_FLAME_BODY] = FlameBody,
    [ABILITY_RUN_AWAY] = RunAway,
    [ABILITY_KEEN_EYE] = KeenEye,
    [ABILITY_HYPER_CUTTER] = HyperCutter,
    [ABILITY_PICKUP] = Pickup,
    [ABILITY_TRUANT] = Truant,
    [ABILITY_HUSTLE] = Hustle,
    [ABILITY_CUTE_CHARM] = CuteCharm,
    [ABILITY_PLUS] = Plus,
    [ABILITY_MINUS] = Minus,
    [ABILITY_FORECAST] = Forecast,
    [ABILITY_STICKY_HOLD] = StickyHold,
    [ABILITY_SHED_SKIN] = ShedSkin,
    [ABILITY_GUTS] = Guts,
    [ABILITY_MARVEL_SCALE] = MarvelScale,
    [ABILITY_LIQUID_OOZE] = LiquidOoze,
    [ABILITY_OVERGROW] = Overgrow,
    [ABILITY_BLAZE] = Blaze,
    [ABILITY_TORRENT] = Torrent,
    [ABILITY_SWARM] = Swarm,
    [ABILITY_ROCK_HEAD] = RockHead,
    [ABILITY_DROUGHT] = Drought,
    [ABILITY_ARENA_TRAP] = ArenaTrap,
    [ABILITY_VITAL_SPIRIT] = VitalSpirit,
    [ABILITY_WHITE_SMOKE] = WhiteSmoke,
    [ABILITY_PURE_POWER] = PurePower,
    [ABILITY_SHELL_ARMOR] = ShellArmor,
    [ABILITY_AIR_LOCK] = AirLock,
    [ABILITY_TANGLED_FEET] = TangledFeet,
    [ABILITY_MOTOR_DRIVE] = MotorDrive,
    [ABILITY_RIVALRY] = Rivalry,
    [ABILITY_STEADFAST] = Steadfast,
    [ABILITY_SNOW_CLOAK] = SnowCloak,
    [ABILITY_GLUTTONY] = Gluttony,
    [ABILITY_ANGER_POINT] = AngerPoint,
    [ABILITY_UNBURDEN] = Unburden,
    [ABILITY_HEATPROOF] = Heatproof,
    [ABILITY_SIMPLE] = Simple,
    [ABILITY_DRY_SKIN] = DrySkin,
    [ABILITY_DOWNLOAD] = Download,
    [ABILITY_IRON_FIST] = IronFist,
    [ABILITY_POISON_HEAL] = PoisonHeal,
    [ABILITY_ADAPTABILITY] = Adaptability,
    [ABILITY_SKILL_LINK] = SkillLink,
    [ABILITY_HYDRATION] = Hydration,
    [ABILITY_SOLAR_POWER] = SolarPower,
    [ABILITY_QUICK_FEET] = QuickFeet,
    [ABILITY_NORMALIZE] = Normalize,
    [ABILITY_SNIPER] = Sniper,
    [ABILITY_MAGIC_GUARD] = MagicGuard,
    [ABILITY_NO_GUARD] = NoGuard,
    [ABILITY_STALL] = Stall,
    [ABILITY_TECHNICIAN] = Technician,
    [ABILITY_LEAF_GUARD] = LeafGuard,
    [ABILITY_KLUTZ] = Klutz,
    [ABILITY_MOLD_BREAKER] = MoldBreaker,
    [ABILITY_SUPER_LUCK] = SuperLuck,
    [ABILITY_AFTERMATH] = Aftermath,
    [ABILITY_ANTICIPATION] = Anticipation,
    [ABILITY_FOREWARN] = Forewarn,
    [ABILITY_UNAWARE] = Unaware,
    [ABILITY_TINTED_LENS] = TintedLens,
    [ABILITY_FILTER] = Filter,
    [ABILITY_SLOW_START] = SlowStart,
    [ABILITY_SCRAPPY] = Scrappy,
    [ABILITY_STORM_DRAIN] = StormDrain,
    [ABILITY_ICE_BODY] = IceBody,
    [ABILITY_SOLID_ROCK] = SolidRock,
    [ABILITY_SNOW_WARNING] = SnowWarning,
    [ABILITY_HONEY_GATHER] = HoneyGather,
    [ABILITY_FRISK] = Frisk,
    [ABILITY_RECKLESS] = Reckless,
    [ABILITY_MULTITYPE] = Multitype,
    [ABILITY_FLOWER_GIFT] = FlowerGift,
    [ABILITY_BAD_DREAMS] = BadDreams,
    [ABILITY_PICKPOCKET] = Pickpocket,
    [ABILITY_SHEER_FORCE] = SheerForce,
    [ABILITY_CONTRARY] = Contrary,
    [ABILITY_UNNERVE] = Unnerve,
    [ABILITY_DEFIANT] = Defiant,
    [ABILITY_DEFEATIST] = Defeatist,
    [ABILITY_CURSED_BODY] = CursedBody,
    [ABILITY_HEALER] = Healer,
    [ABILITY_FRIEND_GUARD] = FriendGuard,
    [ABILITY_WEAK_ARMOR] = WeakArmor,
    [ABILITY_HEAVY_METAL] = HeavyMetal,
    [ABILITY_LIGHT_METAL] = LightMetal,
    [ABILITY_MULTISCALE] = Multiscale,
    [ABILITY_TOXIC_BOOST] = ToxicBoost,
    [ABILITY_FLARE_BOOST] = FlareBoost,
    [ABILITY_HARVEST] = Harvest,
    [ABILITY_TELEPATHY] = Telepathy,
    [ABILITY_MOODY] = Moody,
    [ABILITY_OVERCOAT] = Overcoat,
    [ABILITY_POISON_TOUCH] = PoisonTouch,
    [ABILITY_REGENERATOR] = Regenerator,
    [ABILITY_BIG_PECKS] = BigPecks,
    [ABILITY_SAND_RUSH] = SandRush,
    [ABILITY_WONDER_SKIN] = WonderSkin,
    [ABILITY_ANALYTIC] = Analytic,
    [ABILITY_ILLUSION] = Illusion,
    [ABILITY_IMPOSTER] = Imposter,
    [ABILITY_INFILTRATOR] = Infiltrator,
    [ABILITY_MUMMY] = Mummy,
    [ABILITY_MOXIE] = Moxie,
    [ABILITY_JUSTIFIED] = Justified,
    [ABILITY_RATTLED] = Rattled,
    [ABILITY_MAGIC_BOUNCE] = MagicBounce,
    [ABILITY_SAP_SIPPER] = SapSipper,
    [ABILITY_PRANKSTER] = Prankster,
    [ABILITY_SAND_FORCE] = SandForce,
    [ABILITY_IRON_BARBS] = IronBarbs,
    [ABILITY_ZEN_MODE] = ZenMode,
    [ABILITY_VICTORY_STAR] = VictoryStar,
    [ABILITY_TURBOBLAZE] = Turboblaze,
    [ABILITY_TERAVOLT] = Teravolt,
    [ABILITY_AROMA_VEIL] = AromaVeil,
    [ABILITY_FLOWER_VEIL] = FlowerVeil,
    [ABILITY_CHEEK_POUCH] = CheekPouch,
    [ABILITY_PROTEAN] = Protean,
    [ABILITY_FUR_COAT] = FurCoat,
    [ABILITY_MAGICIAN] = Magician,
    [ABILITY_BULLETPROOF] = Bulletproof,
    [ABILITY_COMPETITIVE] = Competitive,
    [ABILITY_STRONG_JAW] = StrongJaw,
    [ABILITY_REFRIGERATE] = Refrigerate,
    [ABILITY_SWEET_VEIL] = SweetVeil,
    [ABILITY_STANCE_CHANGE] = StanceChange,
    [ABILITY_GALE_WINGS] = GaleWings,
    [ABILITY_MEGA_LAUNCHER] = MegaLauncher,
    [ABILITY_GRASS_PELT] = GrassPelt,
    [ABILITY_SYMBIOSIS] = Symbiosis,
    [ABILITY_TOUGH_CLAWS] = ToughClaws,
    [ABILITY_PIXILATE] = Pixilate,
    [ABILITY_GOOEY] = Gooey,
    [ABILITY_AERILATE] = Aerilate,
    [ABILITY_PARENTAL_BOND] = ParentalBond,
    [ABILITY_DARK_AURA] = DarkAura,
    [ABILITY_FAIRY_AURA] = FairyAura,
    [ABILITY_AURA_BREAK] = AuraBreak,
    [ABILITY_PRIMORDIAL_SEA] = PrimordialSea,
    [ABILITY_DESOLATE_LAND] = DesolateLand,
    [ABILITY_DELTA_STREAM] = DeltaStream,
    [ABILITY_STAMINA] = Stamina,
    [ABILITY_WIMP_OUT] = WimpOut,
    [ABILITY_EMERGENCY_EXIT] = EmergencyExit,
    [ABILITY_WATER_COMPACTION] = WaterCompaction,
    [ABILITY_MERCILESS] = Merciless,
    [ABILITY_SHIELDS_DOWN] = ShieldsDown,
    [ABILITY_STAKEOUT] = Stakeout,
    [ABILITY_WATER_BUBBLE] = WaterBubble,
    [ABILITY_STEELWORKER] = Steelworker,
    [ABILITY_BERSERK] = Berserk,
    [ABILITY_SLUSH_RUSH] = SlushRush,
    [ABILITY_LONG_REACH] = LongReach,
    [ABILITY_LIQUID_VOICE] = LiquidVoice,
    [ABILITY_TRIAGE] = Triage,
    [ABILITY_GALVANIZE] = Galvanize,
    [ABILITY_SURGE_SURFER] = SurgeSurfer,
    [ABILITY_SCHOOLING] = Schooling,
    [ABILITY_DISGUISE] = Disguise,
    [ABILITY_BATTLE_BOND] = BattleBond,
    [ABILITY_POWER_CONSTRUCT] = PowerConstruct,
    [ABILITY_CORROSION] = Corrosion,
    [ABILITY_COMATOSE] = Comatose,
    [ABILITY_QUEENLY_MAJESTY] = QueenlyMajesty,
    [ABILITY_INNARDS_OUT] = InnardsOut,
    [ABILITY_DANCER] = Dancer,
    [ABILITY_BATTERY] = Battery,
    [ABILITY_FLUFFY] = Fluffy,
    [ABILITY_DAZZLING] = Dazzling,
    [ABILITY_SOUL_HEART] = SoulHeart,
    [ABILITY_TANGLING_HAIR] = TanglingHair,
    [ABILITY_RECEIVER] = Receiver,
    [ABILITY_POWER_OF_ALCHEMY] = PowerOfAlchemy,
    [ABILITY_BEAST_BOOST] = BeastBoost,
    [ABILITY_RKS_SYSTEM] = RksSystem,
    [ABILITY_ELECTRIC_SURGE] = ElectricSurge,
    [ABILITY_PSYCHIC_SURGE] = PsychicSurge,
    [ABILITY_MISTY_SURGE] = MistySurge,
    [ABILITY_GRASSY_SURGE] = GrassySurge,
    [ABILITY_FULL_METAL_BODY] = FullMetalBody,
    [ABILITY_SHADOW_SHIELD] = ShadowShield,
    [ABILITY_PRISM_ARMOR] = PrismArmor,
    [ABILITY_NEUROFORCE] = Neuroforce,
    [ABILITY_INTREPID_SWORD] = IntrepidSword,
    [ABILITY_DAUNTLESS_SHIELD] = DauntlessShield,
    [ABILITY_LIBERO] = Libero,
    [ABILITY_BALL_FETCH] = BallFetch,
    [ABILITY_COTTON_DOWN] = CottonDown,
    [ABILITY_PROPELLER_TAIL] = PropellerTail,
    [ABILITY_MIRROR_ARMOR] = MirrorArmor,
    [ABILITY_GULP_MISSILE] = GulpMissile,
    [ABILITY_STALWART] = Stalwart,
    [ABILITY_STEAM_ENGINE] = SteamEngine,
    [ABILITY_PUNK_ROCK] = PunkRock,
    [ABILITY_SAND_SPIT] = SandSpit,
    [ABILITY_ICE_SCALES] = IceScales,
    [ABILITY_RIPEN] = Ripen,
    [ABILITY_ICE_FACE] = IceFace,
    [ABILITY_POWER_SPOT] = PowerSpot,
    [ABILITY_MIMICRY] = Mimicry,
    [ABILITY_SCREEN_CLEANER] = ScreenCleaner,
    [ABILITY_STEELY_SPIRIT] = SteelySpirit,
    [ABILITY_PERISH_BODY] = PerishBody,
    [ABILITY_WANDERING_SPIRIT] = WanderingSpirit,
    [ABILITY_GORILLA_TACTICS] = GorillaTactics,
    [ABILITY_NEUTRALIZING_GAS] = NeutralizingGas,
    [ABILITY_PASTEL_VEIL] = PastelVeil,
    [ABILITY_HUNGER_SWITCH] = HungerSwitch,
    [ABILITY_QUICK_DRAW] = QuickDraw,
    [ABILITY_UNSEEN_FIST] = UnseenFist,
    [ABILITY_CURIOUS_MEDICINE] = CuriousMedicine,
    [ABILITY_TRANSISTOR] = Transistor,
    [ABILITY_DRAGONS_MAW] = DragonsMaw,
    [ABILITY_CHILLING_NEIGH] = ChillingNeigh,
    [ABILITY_GRIM_NEIGH] = GrimNeigh,
    [ABILITY_AS_ONE_ICE_RIDER] = AsOneIceRider,
    [ABILITY_AS_ONE_SHADOW_RIDER] = AsOneShadowRider,
    [ABILITY_CHLOROPLAST] = Chloroplast,
    [ABILITY_WHITEOUT] = Whiteout,
    [ABILITY_PYROMANCY] = Pyromancy,
    [ABILITY_KEEN_EDGE] = KeenEdge,
    [ABILITY_PRISM_SCALES] = PrismScales,
    [ABILITY_POWER_FISTS] = PowerFists,
    [ABILITY_SAND_SONG] = SandSong,
    [ABILITY_RAMPAGE] = Rampage,
    [ABILITY_VENGEANCE] = Vengeance,
    [ABILITY_BLITZ_BOXER] = BlitzBoxer,
    [ABILITY_ANTARCTIC_BIRD] = AntarcticBird,
    [ABILITY_IMMOLATE] = Immolate,
    [ABILITY_CRYSTALLIZE] = Crystallize,
    [ABILITY_ELECTROCYTES] = Electrocytes,
    [ABILITY_AERODYNAMICS] = Aerodynamics,
    [ABILITY_CHRISTMAS_SPIRIT] = ChristmasSpirit,
    [ABILITY_EXPLOIT_WEAKNESS] = ExploitWeakness,
    [ABILITY_GROUND_SHOCK] = GroundShock,
    [ABILITY_ANCIENT_IDOL] = AncientIdol,
    [ABILITY_MYSTIC_POWER] = MysticPower,
    [ABILITY_PERFECTIONIST] = Perfectionist,
    [ABILITY_GROWING_TOOTH] = GrowingTooth,
    [ABILITY_INFLATABLE] = Inflatable,
    [ABILITY_AURORA_BOREALIS] = AuroraBorealis,
    [ABILITY_AVENGER] = Avenger,
    [ABILITY_LETS_ROLL] = LetsRoll,
    [ABILITY_AQUATIC] = Aquatic,
    [ABILITY_LOUD_BANG] = LoudBang,
    [ABILITY_LEAD_COAT] = LeadCoat,
    [ABILITY_AMPHIBIOUS] = Amphibious,
    [ABILITY_GROUNDED] = Grounded,
    [ABILITY_EARTHBOUND] = Earthbound,
    [ABILITY_FIGHT_SPIRIT] = FightingSpirit,
    [ABILITY_FELINE_PROWESS] = FelineProwess,
    [ABILITY_COIL_UP] = CoilUp,
    [ABILITY_FOSSILIZED] = Fossilized,
    [ABILITY_MAGICAL_DUST] = MagicalDust,
    [ABILITY_DREAMCATCHER] = Dreamcatcher,
    [ABILITY_NOCTURNAL] = Nocturnal,
    [ABILITY_SELF_SUFFICIENT] = SelfSufficient,
    [ABILITY_TECTONIZE] = Tectonize,
    [ABILITY_ICE_AGE] = IceAge,
    [ABILITY_HALF_DRAKE] = HalfDrake,
    [ABILITY_LIQUIFIED] = Liquified,
    [ABILITY_DRAGONFLY] = Dragonfly,
    [ABILITY_DRAGONSLAYER] = Dragonslayer,
    [ABILITY_MOUNTAINEER] = Mountaineer,
    [ABILITY_HYDRATE] = Hydrate,
    [ABILITY_METALLIC] = Metallic,
    [ABILITY_PERMAFROST] = Permafrost,
    [ABILITY_PRIMAL_ARMOR] = PrimalArmor,
    [ABILITY_RAGING_BOXER] = RagingBoxer,
    [ABILITY_AIR_BLOWER] = AirBlower,
    [ABILITY_JUGGERNAUT] = Juggernaut,
    [ABILITY_SHORT_CIRCUIT] = ShortCircuit,
    [ABILITY_MAJESTIC_BIRD] = MajesticBird,
    [ABILITY_PHANTOM] = Phantom,
    [ABILITY_INTOXICATE] = Intoxicate,
    [ABILITY_IMPENETRABLE] = Impenetrable,
    [ABILITY_HYPNOTIST] = Hypnotist,
    [ABILITY_OVERWHELM] = Overwhelm,
    [ABILITY_SCARE] = Scare,
    [ABILITY_MAJESTIC_MOTH] = MajesticMoth,
    [ABILITY_SOUL_EATER] = SoulEater,
    [ABILITY_SOUL_LINKER] = SoulLinker,
    [ABILITY_SWEET_DREAMS] = SweetDreams,
    [ABILITY_BAD_LUCK] = BadLuck,
    [ABILITY_HAUNTED_SPIRIT] = HauntedSpirit,
    [ABILITY_ELECTRIC_BURST] = ElectricBurst,
    [ABILITY_RAW_WOOD] = RawWood,
    [ABILITY_SOLENOGLYPHS] = Solenoglyphs,
    [ABILITY_SPIDER_LAIR] = SpiderLair,
    [ABILITY_FATAL_PRECISION] = FatalPrecision,
    [ABILITY_FORT_KNOX] = FortKnox,
    [ABILITY_SEAWEED] = Seaweed,
    [ABILITY_PSYCHIC_MIND] = PsychicMind,
    [ABILITY_POISON_ABSORB] = PoisonAbsorb,
    [ABILITY_SCAVENGER] = Scavenger,
    [ABILITY_TWISTED_DIMENSION] = TwistedDimension,
    [ABILITY_MULTI_HEADED] = MultiHeaded,
    [ABILITY_NORTH_WIND] = NorthWind,
    [ABILITY_OVERCHARGE] = Overcharge,
    [ABILITY_VIOLENT_RUSH] = ViolentRush,
    [ABILITY_FLAMING_SOUL] = FlamingSoul,
    [ABILITY_SAGE_POWER] = SagePower,
    [ABILITY_BONE_ZONE] = BoneZone,
    [ABILITY_WEATHER_CONTROL] = WeatherControl,
    [ABILITY_SPEED_FORCE] = SpeedForce,
    [ABILITY_SEA_GUARDIAN] = SeaGuardian,
    [ABILITY_MOLTEN_DOWN] = MoltenDown,
    [ABILITY_HYPER_AGGRESSIVE] = HyperAggressive,
    [ABILITY_FLOCK] = Flock,
    [ABILITY_FIELD_EXPLORER] = FieldExplorer,
    [ABILITY_STRIKER] = Striker,
    [ABILITY_FROZEN_SOUL] = FrozenSoul,
    [ABILITY_PREDATOR] = Predator,
    [ABILITY_LOOTER] = Looter,
    [ABILITY_LUNAR_ECLIPSE] = LunarEclipse,
    [ABILITY_SOLAR_FLARE] = SolarFlare,
    [ABILITY_POWER_CORE] = PowerCore,
    [ABILITY_SIGHTING_SYSTEM] = SightingSystem,
    [ABILITY_BAD_COMPANY] = BadCompany,
    [ABILITY_OPPORTUNIST] = Opportunist,
    [ABILITY_GIANT_WINGS] = GiantWings,
    [ABILITY_MOMENTUM] = Momentum,
    [ABILITY_GRIP_PINCER] = GripPincer,
    [ABILITY_BIG_LEAVES] = BigLeaves,
    [ABILITY_PRECISE_FIST] = PreciseFist,
    [ABILITY_DEADEYE] = Deadeye,
    [ABILITY_ARTILLERY] = Artillery,
    [ABILITY_AMPLIFIER] = Amplifier,
    [ABILITY_ICE_DEW] = IceDew,
    [ABILITY_SUN_WORSHIP] = SunWorship,
    [ABILITY_POLLINATE] = Pollinate,
    [ABILITY_VOLCANO_RAGE] = VolcanoRage,
    [ABILITY_COLD_REBOUND] = ColdRebound,
    [ABILITY_LOW_BLOW] = LowBlow,
    [ABILITY_NOSFERATU] = Nosferatu,
    [ABILITY_SPECTRAL_SHROUD] = SpectralShroud,
    [ABILITY_DISCIPLINE] = Discipline,
    [ABILITY_THUNDERCALL] = Thundercall,
    [ABILITY_MARINE_APEX] = MarineApex,
    [ABILITY_MIGHTY_HORN] = MightyHorn,
    [ABILITY_HARDENED_SHEATH] = HardenedSheath,
    [ABILITY_ARCTIC_FUR] = ArcticFur,
    [ABILITY_SPECTRALIZE] = Spectralize,
    [ABILITY_LETHARGY] = Lethargy,
    [ABILITY_IRON_BARRAGE] = IronBarrage,
    [ABILITY_STEEL_BARREL] = SteelBarrel,
    [ABILITY_PYRO_SHELLS] = PyroShells,
    [ABILITY_FUNGAL_INFECTION] = FungalInfection,
    [ABILITY_PARRY] = Parry,
    [ABILITY_SCRAPYARD] = Scrapyard,
    [ABILITY_LOOSE_QUILLS] = LooseQuills,
    [ABILITY_TOXIC_DEBRIS] = ToxicDebris,
    [ABILITY_ROUNDHOUSE] = Roundhouse,
    [ABILITY_MINERALIZE] = Mineralize,
    [ABILITY_LOOSE_ROCKS] = LooseRocks,
    [ABILITY_SPINNING_TOP] = SpinningTop,
    [ABILITY_RETRIBUTION_BLOW] = RetributionBlow,
    [ABILITY_FEARMONGER] = Fearmonger,
    [ABILITY_KINGS_WRATH] = KingsWrath,
    [ABILITY_QUEENS_MOURNING] = QueensMourning,
    [ABILITY_TOXIC_SPILL] = ToxicSpill,
    [ABILITY_DESERT_CLOAK] = DesertCloak,
    [ABILITY_DRACONIZE] = Draconize,
    [ABILITY_PRETTY_PRINCESS] = PrettyPrincess,
    [ABILITY_SELF_REPAIR] = SelfRepair,
    [ABILITY_ATOMIC_BURST] = AtomicBurst,
    [ABILITY_HELLBLAZE] = Hellblaze,
    [ABILITY_RIPTIDE] = Riptide,
    [ABILITY_FOREST_RAGE] = ForestRage,
    [ABILITY_PRIMAL_MAW] = PrimalMaw,
    [ABILITY_SWEEPING_EDGE] = SweepingEdge,
    [ABILITY_GIFTED_MIND] = GiftedMind,
    [ABILITY_HYDRO_CIRCUIT] = HydroCircuit,
    [ABILITY_EQUINOX] = Equinox,
    [ABILITY_ABSORBANT] = Absorbant,
    [ABILITY_CLUELESS] = Clueless,
    [ABILITY_CHEATING_DEATH] = CheatingDeath,
    [ABILITY_CHEAP_TACTICS] = CheapTactics,
    [ABILITY_COWARD] = Coward,
    [ABILITY_VOLT_RUSH] = VoltRush,
    [ABILITY_DUNE_TERROR] = DuneTerror,
    [ABILITY_INFERNAL_RAGE] = InfernalRage,
    [ABILITY_DUAL_WIELD] = DualWield,
    [ABILITY_ELEMENTAL_CHARGE] = ElementalCharge,
    [ABILITY_AMBUSH] = Ambush,
    [ABILITY_ATLAS] = Atlas,
    [ABILITY_RADIANCE] = Radiance,
    [ABILITY_JAWS_OF_CARNAGE] = JawsOfCarnage,
    [ABILITY_ANGELS_WRATH] = AngelsWrath,
    [ABILITY_PRISMATIC_FUR] = PrismaticFur,
    [ABILITY_SHOCKING_JAWS] = ShockingJaws,
    [ABILITY_FAE_HUNTER] = FaeHunter,
    [ABILITY_GRAVITY_WELL] = GravityWell,
    [ABILITY_EVAPORATE] = Evaporate,
    [ABILITY_LUMBERJACK] = Lumberjack,
    [ABILITY_WELL_BAKED_BODY] = WellBakedBody,
    [ABILITY_FURNACE] = Furnace,
    [ABILITY_ELECTROMORPHOSIS] = Electromorphosis,
    [ABILITY_ROCKY_PAYLOAD] = RockyPayload,
    [ABILITY_EARTH_EATER] = EarthEater,
    [ABILITY_LINGERING_AROMA] = LingeringAroma,
    [ABILITY_FAIRY_TALE] = FairyTale,
    [ABILITY_RAGING_MOTH] = RagingMoth,
    [ABILITY_ADRENALINE_RUSH] = AdrenalineRush,
    [ABILITY_ARCHMAGE] = Archmage,
    [ABILITY_CRYOMANCY] = Cryomancy,
    [ABILITY_PHANTOM_PAIN] = PhantomPain,
    [ABILITY_PURGATORY] = Purgatory,
    [ABILITY_EMANATE] = Emanate,
    [ABILITY_KUNOICHI_BLADE] = KunoichiBlade,
    [ABILITY_MONKEY_BUSINESS] = MonkeyBusiness,
    [ABILITY_COMBAT_SPECIALIST] = CombatSpecialist,
    [ABILITY_JUNGLES_GUARD] = JunglesGuard,
    [ABILITY_HUNTERS_HORN] = HuntersHorn,
    [ABILITY_PIXIE_POWER] = PixiePower,
    [ABILITY_PLASMA_LAMP] = PlasmaLamp,
    [ABILITY_MAGMA_EATER] = MagmaEater,
    [ABILITY_SUPER_HOT_GOO] = SuperHotGoo,
    [ABILITY_NIKA] = Nika,
    [ABILITY_ARCHER] = Archer,
    [ABILITY_COLD_PLASMA] = ColdPlasma,
    [ABILITY_SUPER_SLAMMER] = SuperSlammer,
    [ABILITY_INVERSE_ROOM] = InverseRoom,
    [ABILITY_ACCELERATE] = Accelerate,
    [ABILITY_FROST_BURN] = FrostBurn,
    [ABILITY_ITCHY_DEFENSE] = ItchyDefense,
    [ABILITY_GENERATOR] = Generator,
    [ABILITY_MOON_SPIRIT] = MoonSpirit,
    [ABILITY_DUST_CLOUD] = DustCloud,
    [ABILITY_BERSERKER_RAGE] = BerserkerRage,
    [ABILITY_TRICKSTER] = Trickster,
    [ABILITY_SAND_GUARD] = SandGuard,
    [ABILITY_NATURAL_RECOVERY] = NaturalRecovery,
    [ABILITY_WIND_RIDER] = WindRider,
    [ABILITY_SOOTHING_AROMA] = SoothingAroma,
    [ABILITY_PRIM_AND_PROPER] = PrimAndProper,
    [ABILITY_SUPER_STRAIN] = SuperStrain,
    [ABILITY_TIPPING_POINT] = TippingPoint,
    [ABILITY_ENLIGHTENED] = Enlightened,
    [ABILITY_PEACEFUL_SLUMBER] = PeacefulSlumber,
    [ABILITY_AFTERSHOCK] = Aftershock,
    [ABILITY_FREEZING_POINT] = FreezingPoint,
    [ABILITY_CRYO_PROFICIENCY] = CryoProficiency,
    [ABILITY_ARCANE_FORCE] = ArcaneForce,
    [ABILITY_DOOMBRINGER] = Doombringer,
    [ABILITY_WISHMAKER] = Wishmaker,
    [ABILITY_YUKI_ONNA] = YukiOnna,
    [ABILITY_SUPPRESS] = Suppress,
    [ABILITY_REFRIGERATOR] = Refrigerator,
    [ABILITY_HEAVEN_ASUNDER] = HeavenAsunder,
    [ABILITY_PURIFYING_WATERS] = PurifyingWaters,
    [ABILITY_SEABORNE] = Seaborne,
    [ABILITY_HIGH_TIDE] = HighTide,
    [ABILITY_CHANGE_OF_HEART] = ChangeOfHeart,
    [ABILITY_MYSTIC_BLADES] = MysticBlades,
    [ABILITY_DETERMINATION] = Determination,
    [ABILITY_FERTILIZE] = Fertilize,
    [ABILITY_PURE_LOVE] = PureLove,
    [ABILITY_FIGHTER] = Fighter,
    [ABILITY_MYCELIUM_MIGHT] = MyceliumMight,
    [ABILITY_TELEKINETIC] = Telekinetic,
    [ABILITY_COMBUSTION] = Combustion,
    [ABILITY_PONY_POWER] = PonyPower,
    [ABILITY_POWDER_BURST] = PowderBurst,
    [ABILITY_RETRIEVER] = Retriever,
    [ABILITY_MONSTER_MASH] = MonsterMash,
    [ABILITY_TWO_STEP] = TwoStep,
    [ABILITY_SPITEFUL] = Spiteful,
    [ABILITY_FORTITUDE] = Fortitude,
    [ABILITY_DEVOURER] = Devourer,
    [ABILITY_PHANTOM_THIEF] = PhantomThief,
    [ABILITY_EARLY_GRAVE] = EarlyGrave,
    [ABILITY_GRAPPLER] = Grappler,
    [ABILITY_BASS_BOOSTED] = BassBoosted,
    [ABILITY_FLAMING_JAWS] = FlamingJaws,
    [ABILITY_MONSTER_HUNTER] = MonsterHunter,
    [ABILITY_CROWNED_SWORD] = CrownedSword,
    [ABILITY_CROWNED_SHIELD] = CrownedShield,
    [ABILITY_BERSERK_DNA] = BerserkDna,
    [ABILITY_CROWNED_KING] = CrownedKing,
    [ABILITY_SNAP_TRAP_WHEN_HIT] = SnapTrapWhenHit,
    [ABILITY_PERMANENCE] = Permanence,
    [ABILITY_HUBRIS] = Hubris,
    [ABILITY_COSMIC_DAZE] = CosmicDaze,
    [ABILITY_MINDS_EYE] = MindsEye,
    [ABILITY_BLOOD_PRICE] = BloodPrice,
    [ABILITY_SPIKE_ARMOR] = SpikeArmor,
    [ABILITY_VOODOO_POWER] = VoodooPower,
    [ABILITY_CHROME_COAT] = ChromeCoat,
    [ABILITY_BANSHEE] = Banshee,
    [ABILITY_WEB_SPINNER] = WebSpinner,
    [ABILITY_SHOWDOWN_MODE] = ShowdownMode,
    [ABILITY_SEED_SOWER] = SeedSower,
    [ABILITY_AIRBORNE] = Airborne,
    [ABILITY_PARROTING] = Parroting,
    [ABILITY_SALT_CIRCLE] = SaltCircle,
    [ABILITY_PURIFYING_SALT] = PurifyingSalt,
    [ABILITY_PROTOSYNTHESIS] = Protosynthesis,
    [ABILITY_QUARK_DRIVE] = QuarkDrive,
    [ABILITY_WIND_POWER] = WindPower,
    [ABILITY_IMPULSE] = Impulse,
    [ABILITY_TERMINAL_VELOCITY] = TerminalVelocity,
    [ABILITY_GUARD_DOG] = GuardDog,
    [ABILITY_ANGER_SHELL] = AngerShell,
    [ABILITY_EGOIST] = Egoist,
    [ABILITY_SUBDUE] = Subdue,
    [ABILITY_READIED_ACTION] = ReadiedAction,
    [ABILITY_DARK_GALE_WINGS] = DarkGaleWings,
    [ABILITY_GUILT_TRIP] = GuiltTrip,
    [ABILITY_WATER_GALE_WINGS] = WaterGaleWings,
    [ABILITY_ZERO_TO_HERO] = ZeroToHero,
    [ABILITY_COSTAR] = Costar,
    [ABILITY_COMMANDER] = Commander,
    [ABILITY_EJECT_PACK_ABILITY] = EjectPackAbility,
    [ABILITY_VENGEFUL_SPIRIT] = VengefulSpirit,
    [ABILITY_CUD_CHEW] = CudChew,
    [ABILITY_ARMOR_TAIL] = ArmorTail,
    [ABILITY_MIND_CRUSH] = MindCrush,
    [ABILITY_SUPREME_OVERLORD] = SupremeOverlord,
    [ABILITY_ILL_WILL] = IllWill,
    [ABILITY_FIRE_SCALES] = FireScales,
    [ABILITY_WATCH_YOUR_STEP] = WatchYourStep,
    [ABILITY_RAPID_RESPONSE] = RapidResponse,
    [ABILITY_DOUBLE_IRON_BARBS] = DoubleIronBarbs,
    [ABILITY_THERMAL_EXCHANGE] = ThermalExchange,
    [ABILITY_GOOD_AS_GOLD] = GoodAsGold,
    [ABILITY_SHARING_IS_CARING] = SharingIsCaring,
    [ABILITY_TABLETS_OF_RUIN] = TabletsOfRuin,
    [ABILITY_SWORD_OF_RUIN] = SwordOfRuin,
    [ABILITY_VESSEL_OF_RUIN] = VesselOfRuin,
    [ABILITY_BEADS_OF_RUIN] = BeadsOfRuin,
    [ABILITY_PERMAFROST_CLONE] = PermafrostClone,
    [ABILITY_GALLANTRY] = Gallantry,
    [ABILITY_ORICHALCUM_PULSE] = OrichalcumPulse,
    [ABILITY_SUN_BASKING] = SunBasking,
    [ABILITY_WINGED_KING] = WingedKing,
    [ABILITY_HADRON_ENGINE] = HadronEngine,
    [ABILITY_IRON_SERPENT] = IronSerpent,
    [ABILITY_WEATHER_DOUBLE_BOOST] = WeatherDoubleBoost,
    [ABILITY_SWEEPING_EDGE_PLUS] = SweepingEdgePlus,
    [ABILITY_CELESTIAL_BLESSING] = CelestialBlessing,
    [ABILITY_MINION_CONTROL] = MinionControl,
    [ABILITY_MOLTEN_BLADES] = MoltenBlades,
    [ABILITY_HAUNTING_FRENZY] = HauntingFrenzy,
    [ABILITY_NOISE_CANCEL] = NoiseCancel,
    [ABILITY_RADIO_JAM] = RadioJam,
    [ABILITY_OLE] = Ole,
    [ABILITY_MALICIOUS] = Malicious,
    [ABILITY_DEAD_POWER] = DeadPower,
    [ABILITY_BRAWLING_WYVERN] = BrawlingWyvern,
    [ABILITY_MYTHICAL_ARROWS] = MythicalArrows,
    [ABILITY_LAWNMOWER] = Lawnmower,
    [ABILITY_FLOURISH] = Flourish,
    [ABILITY_DESERT_SPIRIT] = DesertSpirit,
    [ABILITY_CONTEMPT] = Contempt,
    [ABILITY_AERIALIST] = Aerialist,
    [ABILITY_TERA_SHELL] = TeraShell,
    [ABILITY_TOXIC_CHAIN] = ToxicChain,
    [ABILITY_PARASITIC_SPORES] = ParasiticSpores,
    [ABILITY_POISON_PUPPETEER] = PoisonPuppeteer,
    [ABILITY_ENTRANCE] = Entrance,
    [ABILITY_REJECTION] = Rejection,
    [ABILITY_APPLE_ENLIGHTENMENT] = AppleEnlightenment,
    [ABILITY_BALLOON_BOMBER] = BalloonBomber,
    [ABILITY_FLAMING_MAW] = FlamingMaw,
    [ABILITY_DEMOLITIONIST] = Demolitionist,
    [ABILITY_ROCKHARD_WILL] = RockhardWill,
    [ABILITY_FRAGRANT_DAZE] = FragrantDaze,
    [ABILITY_LOW_VISIBILITY] = LowVisibility,
    [ABILITY_OLD_MARINER] = OldMariner,
    [ABILITY_ECTOPLASM] = Ectoplasm,
    [ABILITY_BEAUTIFUL_MUSIC] = BeautifulMusic,
    [ABILITY_SURPRISE] = Surprise,
    [ABILITY_SNOW_SONG] = SnowSong,
    [ABILITY_GREATER_SPIRIT] = GreaterSpirit,
    [ABILITY_RESONANCE] = Resonance,
    [ABILITY_ETHEREAL_RUSH] = EtherealRush,
    [ABILITY_CUTE_ANTECEDENCE] = CuteAntecedence,
    [ABILITY_RECURRING_NIGHTMARE] = RecurringNightmare,
    [ABILITY_MENACING_SITUATION] = MenacingSituation,
    [ABILITY_SHINY_LIGHTNING] = ShinyLightning,
    [ABILITY_TERRIFY] = Terrify,
    [ABILITY_ICE_DOWNFALL] = IceDownfall,
    [ABILITY_LAST_STAND] = LastStand,
    [ABILITY_PYROCLASTIC_FLOW] = PyroclasticFlow,
    [ABILITY_BLOOD_BATH] = BloodBath,
    [ABILITY_BATTLE_AURA] = BattleAura,
    [ABILITY_BLOODLUST] = Bloodlust,
    [ABILITY_PIERCING_SOLO] = PiercingSolo,
    [ABILITY_RHYTHMIC] = Rhythmic,
    [ABILITY_CHUNKY_BASS_LINE] = ChunkyBassLine,
    [ABILITY_DUAL_HAMMER] = DualHammer,
    [ABILITY_DENTING_BLOWS] = DentingBlows,
    [ABILITY_ICE_COLD_HUNTER] = IceColdHunter,
    [ABILITY_SOUL_CRUSHER] = SoulCrusher,
    [ABILITY_ARC_FLASH] = ArcFlash,
    [ABILITY_UNICORN] = Unicorn,
    [ABILITY_ON_THE_PROWL] = OnTheProwl,
    [ABILITY_PRETENTIOUS] = Pretentious,
    [ABILITY_VENOBLAZE_PINCERS] = VenoblazePincers,
    [ABILITY_ETERNAL_BLESSING] = EternalBlessing,
    [ABILITY_SUGAR_RUSH] = SugarRush,
    [ABILITY_PEACEFUL_REST] = PeacefulRest,
    [ABILITY_WHITE_NOISE] = WhiteNoise,
    [ABILITY_SMOKEY_MANEUVERS] = SmokeyManeuvers,
    [ABILITY_TAG] = Tag,
    [ABILITY_POWER_METAL] = PowerMetal,
    [ABILITY_POWER_EDGE] = PowerEdge,
    [ABILITY_SUPERCONDUCTOR] = Superconductor,
    [ABILITY_ULTRA_INSTINCT] = UltraInstinct,
    [ABILITY_UNLOCKED_POTENTIAL] = UnlockedPotential,
    [ABILITY_HIGHER_RANK] = HigherRank,
    [ABILITY_FUNERAL_PYRE] = FuneralPyre,
    [ABILITY_FLAME_BUBBLE] = FlameBubble,
    [ABILITY_ELEMENTAL_VORTEX] = ElementalVortex,
    [ABILITY_SNOWY_WRATH] = SnowyWrath,
    [ABILITY_PATTERN_CHANGE] = PatternChange,
    [ABILITY_NO_TURNING_BACK] = NoTurningBack,
    [ABILITY_FLAMMABLE_COAT] = FlammableCoat,
    [ABILITY_DRACO_MORALE] = DracoMorale,
    [ABILITY_BAD_OMEN] = BadOmen,
    [ABILITY_MOSH_PIT] = MoshPit,
    [ABILITY_BLOOD_STAIN] = BloodStain,
    [ABILITY_BLOOD_STIGMA] = BloodStigma,
    [ABILITY_MAXIMUM_ACCELERATION] = MaximumAcceleration,
    [ABILITY_SIDEWINDER] = Sidewinder,
    [ABILITY_PETRIFY] = Petrify,
    [ABILITY_FLUFFIEST] = Fluffiest,
    [ABILITY_WAY_OF_PRECISION] = WayOfPrecision,
    [ABILITY_WAY_OF_SWIFTNESS] = WayOfSwiftness,
    [ABILITY_ATOMIC_PUNCH] = AtomicPunch,
    [ABILITY_IRON_GIANT] = IronGiant,
    [ABILITY_MASTER_HAND] = MasterHand,
    [ABILITY_FINAL_BLOW] = FinalBlow,
    [ABILITY_HOSPITALITY] = Hospitality,
    [ABILITY_BUTTER_UP] = ButterUp,
    [ABILITY_VITALITY_STRIKE] = VitalityStrike,
    [ABILITY_HUGE_WINGS] = HugeWings,
    [ABILITY_SWORD_OF_DAMNATION] = SwordOfDamnation,
    [ABILITY_RESTRAINING_ORDER] = RestrainingOrder,
    [ABILITY_ASSASSINS_TOOLS] = AssassinsTools,
    [ABILITY_FROSTMAW] = Frostmaw,
    [ABILITY_PATCHWORK] = Patchwork,
    [ABILITY_BLIND_RAGE] = BlindRage,
    [ABILITY_SLIPSTREAM] = Slipstream,
    [ABILITY_APEX_PREDATOR] = ApexPredator,
    [ABILITY_DRAGONS_RITUAL] = DragonsRitual,
    [ABILITY_PINNACLE_BLADE] = PinnacleBlade,
    [ABILITY_ENERGIZED] = Energized,
    [ABILITY_COLOR_SPECTRUM] = ColorSpectrum,
    [ABILITY_STEEL_BEETLE] = SteelBeetle,
    [ABILITY_FROM_THE_SHADOWS] = FromTheShadows,
    [ABILITY_RAGE_POINT] = RagePoint,
    [ABILITY_HOT_COALS] = HotCoals,
    [ABILITY_TERASTAL_TREASURE] = TerastalTreasure,
    [ABILITY_SHOCKING_MAW] = ShockingMaw,
    [ABILITY_GLEAM_EYES] = GleamEyes,
    [ABILITY_ROUSED_FANGS] = RousedFangs,
    [ABILITY_DREAM_STATE] = DreamState,
    [ABILITY_DREAM_WHIMSY] = DreamWhimsy,
    [ABILITY_LUNAR_AFFINITY] = LunarAffinity,
    [ABILITY_FLAME_SHIELD] = FlameShield,
    [ABILITY_AQUATIC_DWELLER] = AquaticDweller,
    [ABILITY_APPLE_PIE] = ApplePie,
    [ABILITY_HOVER] = Hover,
    [ABILITY_DEPRAVITY] = Depravity,
    [ABILITY_WILDFIRE] = Wildfire,
    [ABILITY_JUMP_SCARE] = JumpScare,
    [ABILITY_TAR_TOSS] = TarToss,
    [ABILITY_STUN_SHOCK] = StunShock,
    [ABILITY_RAGING_GODDESS] = RagingGoddess,
    [ABILITY_WHIPLASH] = Whiplash,
    [ABILITY_SUPERSWEET_SYRUP] = SupersweetSyrup,
    [ABILITY_LUCKY_HALO] = LuckyHalo,
    [ABILITY_TRASH_HEAP] = TrashHeap,
    [ABILITY_SLUDGY_MIX] = SludgyMix,
    [ABILITY_OVERWATCH] = Overwatch,
    [ABILITY_WIND_RAGE] = WindRage,
    [ABILITY_VICTORY_BOMB] = VictoryBomb,
    [ABILITY_RAZOR_SHARP] = RazorSharp,
    [ABILITY_TO_THE_BONE] = ToTheBone,
    [ABILITY_BLADE_DANCE] = BladeDance,
    [ABILITY_TAEKKYEON] = Taekkyeon,
    [ABILITY_APE_SHIFT] = ApeShift,
    [ABILITY_KNOW_YOUR_PLACE] = KnowYourPlace,
    [ABILITY_DEEP_CUTS] = DeepCuts,
    [ABILITY_LIFE_STEAL] = LifeSteal,
    [ABILITY_RUDE_AWAKENING] = RudeAwakening,
    [ABILITY_TERAFORM_ZERO] = TeraformZero,
    [ABILITY_SET_ABLAZE] = SetAblaze,
    [ABILITY_BREAKWATER] = Breakwater,
    [ABILITY_MAGICAL_FISTS] = MagicalFists,
    [ABILITY_CUTTHROAT] = Cutthroat,
    [ABILITY_SAND_BENDER] = SandBender,
    [ABILITY_SAND_PIT] = SandPit,
    [ABILITY_DESOLATE_SUN] = DesolateSun,
    [ABILITY_DAYBREAK] = Daybreak,
    [ABILITY_ENERGY_SIPHON] = EnergySiphon,
    [ABILITY_RESERVOIR] = Reservoir,
    [ABILITY_NEUROTOXIN] = Neurotoxin,
    [ABILITY_ENERGIZED_HORNS] = EnergizedHorns,
    [ABILITY_SPIDER_LAIR_UPGRADE] = SpiderLairUpgrade,
    [ABILITY_CRUST_COAT] = CrustCoat,
    [ABILITY_PUFFY] = Puffy,
    [ABILITY_BALLOON_BLITZ] = BalloonBlitz,
    [ABILITY_STRIKER_PIXILATE] = StrikerPixilate,
    [ABILITY_DOOM_BLAST] = DoomBlast,
    [ABILITY_BRUTEFORCE] = Bruteforce,
    [ABILITY_FARADAY_CAGE] = FaradayCage,
    [ABILITY_ACIDIC_SLIME] = AcidicSlime,
    [ABILITY_ROSE_GARDEN] = RoseGarden,
    [ABILITY_QIGONG] = Qigong,
    [ABILITY_CONJOURER_OF_DECEIT] = ConjurerOfDeceit,
    [ABILITY_DEEP_FREEZE] = DeepFreeze,
    [ABILITY_SOUL_DEVOURER] = SoulDevourer,
    [ABILITY_CHAMPIONS_ENTRANCE] = ChampionsEntrance,
    [ABILITY_PRESTO] = Presto,
    [ABILITY_SAMBA] = Samba,
    [ABILITY_JUNSHI_SANDA] = JunshiSanda,
    [ABILITY_GLADIATOR] = Gladiator,
    [ABILITY_FORSAKEN_HEART] = ForsakenHeart,
    [ABILITY_RELENTLESS] = Relentless,
    [ABILITY_SOOTHSAYER] = Soothsayer,
    [ABILITY_CORRUPTED_MIND] = CorruptedMind,
    [ABILITY_FLAME_COAT] = FlameCoat,
    [ABILITY_UNOWN_POWER] = UnownPower,
};

#pragma GCC diagnostic pop