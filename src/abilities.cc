
#include "abilities.hh"

extern "C" {
#include "generated/constants/abilities.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "generated/constants/battle_move_effects.h"
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
ENUM_OR(MoveEffectEnum)

#define CHECK(effect) \
    if (!(effect)) return __EnumHack();
#define CHECK_NOT(effect) \
    if (effect) return __EnumHack();

#define __COMBINE(val1, val2) val1##val2
#define COMBINE(val1, val2) __COMBINE(val1, val2)

#define ON_ENTRY AbilityEnum ability, int battler
#define DELEGATE_ENTRY ability, battler
#define ON_ABSORB int battler, MoveEnum move, int moveType, int *statId
#define DELEGATE_ABSORB battler, move, moveType, statId
#define ON_IMMUNE int battler, int attacker, MoveEnum move, int moveType, const u8 **immunityScript
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
#define ON_INFILTRATE int battler, MoveEnum move
#define DELEGATE_INFILTRATE battler, move
#define ON_DISGUISE int battler, int testOnly
#define DELEGATE_DISGUISE battler, testOnly
#define ON_WEATHER AbilityEnum ability, int battler
#define DELEGATE_WEATHER ability, battler
#define ON_TERRAIN AbilityEnum ability, int battler
#define DELEGATE_TERRAIN ability, battler
#define ON_END_TURN AbilityEnum ability, int battler
#define DELEGATE_END_TURN ability, battler
#define ON_ATTACKER AbilityEnum ability, int battler, int target, MoveEnum move, int moveType
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
#define ON_DEFENDER AbilityEnum ability, int battler, int attacker, MoveEnum move, int moveType
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
#define ON_EITHER(name) static int name##OnEither(AbilityEnum ability, int battler, int opponent, MoveEnum move, int moveType)
#define ON_EITHER_ABILITY(name) .onAttacker = name##OnEither, .onDefender = name##OnEither
#define ON_RECOIL int damage, int battler, int moveType
#define DELEGATE_RECOIL damage, battler, moveType
#define ON_REACTIVE AbilityEnum ability, int battler, AbilityCallType callType
#define DELEGATE_REACTIVE ability, battler
#define ON_BATTLER_FAINTS AbilityEnum ability, int battler, int attacker, int fainted, MoveEnum move, int moveType
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
#define ON_PARENTAL_BOND int battler, MoveEnum move, int moveType
#define DELEGATE_PARENTAL_BOND battler, move, moveType
#define ON_STAT AbilityEnum ability, int battler, int statId, u32 *stat, NonStackingState *flags
#define DELEGATE_STAT ability, battler, statId, stat, flags
#define ON_OFFENSIVE_MULTIPLIER                                                                                                                             \
    int battler, AbilityEnum ability, int target, MoveEnum move, int moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16 *resistance, \
        u16 *modifier
#define DELEGATE_OFFENSIVE_MULTIPLIER battler, ability, target, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier
#define ON_DEFENSIVE_MULTIPLIER \
    int battler, int attacker, MoveEnum move, int moveType, int typeEffectivenessModifier, int isCrit, u16 *resistance, u16 *modifier
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
#define ON_ACCURACY AbilityEnum ability, int battler, int target, MoveEnum move, int moveType, int *accuracy
#define DELEGATE_ACCURACY ability, battler, target, move, moveType, accuracy
#define ON_SWAP_SPLIT int battler, MoveEnum move
#define DELEGATE_SWAP_SPLIT battler, move
#define ON_CHOOSE_OFFENSIVE_STAT \
    int battler, MoveEnum move, int ignoreOffensiveStatDrops, int targetUnaware, u8 *atkStatToUse, u8 secondaryAtkStatToUse[NUM_STATS]
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
#define ON_CHOOSE_DEFENSIVE_STAT int battler, int target, MoveEnum move, int ignoreDefensiveStatBoosts, int battlerUnaware
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
#define ON_STAB int moveType
#define DELEGATE_STAB moveType
#define ON_PRIORITY int battler, int target, MoveEnum move
#define DELEGATE_PRIORITY battler, target, move
#define ON_MOVE_TYPE AbilityEnum ability, MoveEnum move, int moveType, u8 *ateBoost
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
#define ON_EXIT AbilityEnum ability, int battler
#define DELEGATE_EXIT ability, battler
#define ON_CRIT int battler, int target, MoveEnum move, u16 typeEffectiveness
#define DELEGATE_CRIT battler, target, move, typeEffectiveness
#define ON_TYPE_EFFECTIVENESS int defType, MoveEnum move, int moveType, u16 *mod
#define DELEGATE_TYPE_EFFECTIVENESS defType, move, moveType, mod
#define ON_COPY_MOVE AbilityEnum ability, int battler, int attacker, int target, MoveEnum move
#define DELEGATE_COPY_MOVE ability, battler, attacker, target, move
#define ON_AFTER_TYPE_EFFECTIVENESS int battler, AbilityEnum ability, int target, MoveEnum move, int moveType, u16 *mod, u16 mod1, u16 mod2, u16 mod3
#define DELEGATE_AFTER_TYPE_EFFECTIVENESS battler, target, move, moveType, mod, mod1, mod2, mod3
#define ON_MODIFY_EFFECT_CHANCE int battler, MoveEnum move, MoveEffectEnum moveEffect, int *effectChance
#define DELEGATE_MODIFY_EFFECT_CHANCE battler, move, moveEffect, effectChance
#define ABILITY_ON_CAN_STATUS_TYPE int battler, MoveEnum move, StatusCheckEnum status
#define DELEGATE_ON_CAN_STATUS_TYPE battler, move, status
#define ABILITY_ON_STATUS_IMMUNE int battler, int target, AbilityEnum ability, StatusCheckEnum status
#define DELEGATE_ON_STATUS_IMMUNE int battler, target, ability, status
#define ABILITY_ON_TRAP int switchingBattler
#define DELEGATE_ON_TRAP switchingBattler
#define ABILITY_ON_BEFORE_ATTACK int battler, int attacker, AbilityEnum ability, MoveEnum move, int moveType
#define DELEGATE_ON_BEFORE_ATTACK battler, attacker, ability, move, moveType
#define ON_PREEMPT_ACTION u8 battler, AbilityEnum ability, u8 turnBattler
#define DELEGATE_PREEMPT_ACTION battler, ability, turnBattler

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
static int AdjustFollowupMoveTarget(int battler, int *target, MoveEnum move, FollowupType type) {
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

static int TryTransformAttacker(AbilityEnum ability, int battler, AbilityCallType callType) {
    CHECK(ShouldChangeFormHpBased(battler))
    CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

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

static int AbilityStatusEffect(MoveEffectEnum effect) {
    gBattleScripting.moveEffect = effect;
    BattleScriptCall(BattleScript_AbilityStatusEffect);
    gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
    return TRUE;
}

static int AbilityStatusEffectDirect(MoveEffectEnum effect) {
    gBattleScripting.moveEffect = effect;
    gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
    SetMoveEffect(FALSE, FALSE);
    return FALSE;
}

static int AbilityStatusEffectSafe(MoveEffectEnum effect, int attacker, int target) {
    gBattleScripting.moveEffect = effect;
    gStackBattler1 = attacker;
    gStackBattler2 = target;
    BattleScriptCall(BattleScript_AbilityStatusEffectSafe);
    gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
    return TRUE;
}

static int PoisonPuppeteerClone(AbilityEnum ability, int battler, int (*predicate)(int battler, int target), const u8 *callback) {
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

#define ATE_ABILITY(type)                    \
    .onMoveType = +[](ON_MOVE_TYPE) -> int { \
        CHECK(moveType == TYPE_NORMAL)       \
        *ateBoost = TRUE;                    \
        return type + 1;                     \
    },                                       \
    .onStab = +[](ON_STAB) -> int { return moveType == type; }

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
constexpr Ability None = {
    .randomizerBanned = TRUE,
};

constexpr Ability Stench = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    },
    .toxicTerrainImmune = TRUE,
};

constexpr Ability PoisonHeal = {
    .toxicTerrainImmune = TRUE,
};

constexpr Ability Drizzle = {
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

constexpr Ability SpeedBoost = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

constexpr Ability BattleArmor = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.8); },
    .onCrit = +[](ON_CRIT) { return NEVER_CRIT; },
    .onCritFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability Sturdy = {
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
constexpr Ability Damp = {
    ON_EITHER_ABILITY(Damp),
};

constexpr Ability Limber = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
    .breakable = TRUE,
    .halfRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability SandVeil = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

ON_EITHER(Static) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeParalyzed(battler, opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_PARALYSIS, battler, opponent);
    return TRUE;
}
constexpr Ability Static = {
    ON_EITHER_ABILITY(Static),
};

constexpr Ability VoltAbsorb = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

constexpr Ability WaterAbsorb = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

constexpr Ability Oblivious = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING))
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability CloudNine = {
    .onEntry = +[](ON_ENTRY) -> int {
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
        return TRUE;
    },
};

constexpr Ability CompoundEyes = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.3;
        return ACCURACY_MULTIPLICATIVE;
    },
};

constexpr Ability Insomnia = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability ColorChange = {
    .onBeforeAttack = +[](ABILITY_ON_BEFORE_ATTACK) -> int {
        CHECK(battler != attacker)
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))

        u32 bestType = gBattleMons[gBattlerTarget].type1;
        u16 bestModifier = GetTypeModifier(moveType, bestType, attacker, battler);

        for (int currentType = TYPE_NORMAL; currentType < NUMBER_OF_MON_TYPES; ++currentType) {
            u16 currentModifier = GetTypeModifier(moveType, currentType, attacker, battler);
            if (currentModifier < bestModifier) {
                bestModifier = currentModifier;
                bestType = currentType;
            }
            if (bestModifier == UQ_4_12(0.0)) break;
        }

        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, bestType))

        SET_BATTLER_TYPE(battler, bestType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, bestType);
        BattleScriptCall(BattleScript_ColorChangeActivates);
        return TRUE;
    },
    .onBeforeAttackFor = APPLY_ON_TARGET,
};

constexpr Ability Immunity = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_POISON)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability FlashFire = {
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

constexpr Ability ShieldDust = {
    .breakable = TRUE,
    .powderImmune = TRUE,
};

constexpr Ability OwnTempo = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability SuctionCups = {
    .breakable = TRUE,
};

constexpr Ability Intimidate = {
    .onEntry = UseIntimidateClone,
};

constexpr Ability ShadowTag = {
    .onTrap = +[](ABILITY_ON_TRAP) -> int {
        ON_ABILITY(switchingBattler, FALSE, gAbilities[ability].shadowTag, return FALSE)
        return TRUE;
    },
    .shadowTag = TRUE,
};

constexpr Ability RoughSkin = {
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

constexpr Ability WonderGuard = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod < UQ_4_12(2.0)) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability Levitate = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING) MUL(1.25);
        },
    .breakable = TRUE,
    .levitate = TRUE,
};

constexpr Ability EffectSpore = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPowderImmune(attacker, FALSE))
        CHECK(Random() % 100 < 30)

        switch (Random() % 3) {
            case 0:
                CHECK(CanBePoisoned(battler, attacker, MOVE_NONE))

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
    .breakable = TRUE,
    .powderImmune = TRUE,
};

constexpr Ability ClearBody = {
    .breakable = TRUE,
};

constexpr Ability NaturalCure = {
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

constexpr Ability LightningRod = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_ELECTRIC,
    .breakable = TRUE,
};

constexpr Ability SereneGrace = {
    .onModifyEffectChance = +[](ON_MODIFY_EFFECT_CHANCE) { *effectChance *= 2; },
};

constexpr Ability SwiftSwim = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
        },
};

constexpr Ability Chlorophyll = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        },
};

constexpr Ability Illuminate = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

constexpr Ability Trace = {
    .onEntry = +[](ON_ENTRY) -> int {
        int target = BATTLE_OPPOSITE(battler);
        auto newAbility = GetBattlerAbility(target);
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

constexpr Ability HugePower = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK) *stat *= 2;
        },
};

ON_EITHER(PoisonPoint) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBePoisoned(battler, opponent, MOVE_NONE))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
    return TRUE;
}
constexpr Ability PoisonPoint = {
    ON_EITHER_ABILITY(PoisonPoint),
};

constexpr Ability InnerFocus = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_FOCUS_BLAST)
        return ACCURACY_ALWAYS_HITS;
    },
    .breakable = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability MagmaArmor = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
        },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_FROSTBITE)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability WaterVeil = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses3[battler] & STATUS3_AQUA_RING)

        gStatuses3[battler] |= STATUS3_AQUA_RING;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
        return TRUE;
    },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BURN)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability MagnetPull = {
    .onTrap = +[](ABILITY_ON_TRAP) -> int { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); },
};

constexpr Ability Soundproof = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsSoundMove(attacker, move))
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

constexpr Ability RainDish = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
};

constexpr Ability SandStream = {
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

constexpr Ability Pressure = {
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

constexpr Ability ThickFat = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

ON_EITHER(FlameBody) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeBurned(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
    return TRUE;
}
constexpr Ability FlameBody = {
    ON_EITHER_ABILITY(FlameBody),
};

constexpr Ability KeenEye = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .breakable = TRUE,
};

constexpr Ability HyperCutter = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(IsMoveMakingContact(move, battler))
        return 1;
    },
    .breakable = TRUE,
};

constexpr Ability Pickup = {
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

constexpr Ability Truant = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (GetAbilityState(battler, ability))
            SetAbilityState(battler, ability, FALSE);
        else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
            SetAbilityState(battler, ability, TRUE);
        return FALSE;
    },
};

constexpr Ability Hustle = {
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MUL(1.4); },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
        return ACCURACY_MULTIPLICATIVE;
    },
};

ON_EITHER(CuteCharm) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(CanInfatuate(battler, opponent))
    CHECK(Random() % 100 < 50)

    AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
    return TRUE;
}
constexpr Ability CuteCharm = {
    ON_EITHER_ABILITY(CuteCharm),
};

constexpr Ability Plus = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            int partner = BATTLE_PARTNER(battler);
            if (!IsBattlerAlive(partner)) return;
            if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
        },
};

constexpr Ability Minus = {
    .onOffensiveMultiplier = Plus.onOffensiveMultiplier,
};

constexpr Ability Forecast = {
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

constexpr Ability StickyHold = {
    .breakable = TRUE,
};

constexpr Ability ShedSkin = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

constexpr Ability Guts = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
    .negatesBurnAtkDrop = TRUE,
};

constexpr Ability MarvelScale = {
    .onStat =
        +[](ON_STAT) {
            if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        },
    .breakable = TRUE,
};

constexpr Ability Overgrow = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GRASS),
};

constexpr Ability Blaze = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIRE),
};

constexpr Ability Torrent = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_WATER),
};

constexpr Ability Swarm = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_BUG),
};

constexpr Ability RockHead = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    },
    .breakable = TRUE,
    .noRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability Drought = {
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

constexpr Ability ArenaTrap = {
    .onTrap = +[](ABILITY_ON_TRAP) -> int { return IsBattlerGrounded(switchingBattler); },
};

constexpr Ability VitalSpirit = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIGHTING)
        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability WhiteSmoke = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

        int side = GET_BATTLER_SIDE(battler);
        gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
        gSideTimers[side].started.smokescreen = TRUE;
        gSideTimers[side].smokescreenBattler = battler;
        return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
    },
};

constexpr Ability PurePower = {
    .onStat = HugePower.onStat,
};

constexpr Ability ShellArmor = {
    .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier,
    .onCrit = BattleArmor.onCrit,
    .onCritFor = BattleArmor.onCritFor,
    .breakable = TRUE,
};

constexpr Ability AirLock = {
    .onEntry = CloudNine.onEntry,
};

constexpr Ability TangledFeet = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability MotorDrive = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

constexpr Ability Rivalry = {
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

constexpr Ability SnowCloak = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .hailImmune = TRUE,
};

constexpr Ability AngerPoint = {
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

constexpr Ability Unburden = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
        },
};

constexpr Ability Heatproof = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        },
    .breakable = TRUE,
    .negatesBurnAtkDrop = TRUE,
};

constexpr Ability DrySkin = {
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

constexpr Ability Download = {
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

constexpr Ability IronFist = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsIronFistBoosted(battler, move)) MUL(1.3);
        },
};

constexpr Ability Adaptability = {
    .adaptability = TRUE,
};

constexpr Ability SkillLink = {
    .skillLink = TRUE,
};

constexpr Ability Hydration = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

constexpr Ability SolarPower = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        },
};

constexpr Ability QuickFeet = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        },
};

constexpr Ability Normalize = {
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

constexpr Ability Sniper = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (isCrit) MUL(1.5);
        },
};

constexpr Ability MagicGuard = {
    .magicGuard = TRUE,
};

constexpr Ability NoGuard = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_ALWAYS_HITS; },
    .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
};

constexpr Ability Stall = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
        },
    .breakable = TRUE,
};

constexpr Ability Technician = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (basePower <= 60) MUL(1.5);
        },
};

constexpr Ability LeafGuard = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

constexpr Ability MoldBreaker = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); },
};

constexpr Ability SuperLuck = {
    .onCrit = +[](ON_CRIT) -> int { return 1; },
};

constexpr Ability Aftermath = {
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

constexpr Ability Anticipation = {
    .onEntry = +[](ON_ENTRY) -> int {
        int side = GetBattlerSide(battler);
        int any = FALSE;

        for (int i = 0; i < gBattlersCount; i++) {
            if (IsBattlerAlive(i) && side != GetBattlerSide(i)) {
                for (int j = 0; j < MAX_MON_MOVES; j++) {
                    MoveEnum move = gBattleMons[i].moves[j];
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

constexpr Ability Forewarn = {
    .onEntry = +[](ON_ENTRY) -> int {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(gBattlerTarget) || gWishFutureKnock.futureSightCounter[gBattlerTarget]) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(gBattlerTarget))
        CHECK_NOT(gWishFutureKnock.futureSightCounter[gBattlerTarget])

        gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] |= SIDE_STATUS_FUTUREATTACK;
        gWishFutureKnock.futureSightMove[gBattlerTarget] = MOVE_FUTURE_SIGHT;
        gWishFutureKnock.futureSightPower[gBattlerTarget] = 80;
        gWishFutureKnock.futureSightAttacker[gBattlerTarget] = battler;
        gWishFutureKnock.futureSightCounter[gBattlerTarget] = 3;

        BattleScriptPushCursorAndCallback(BattleScript_ForewarnReworkActivates);
        return TRUE;
    },
};

constexpr Ability Unaware = {
    .breakable = TRUE,
    .unaware = TRUE,
};

constexpr Ability TintedLens = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
        },
};

constexpr Ability Filter = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        },
    .breakable = TRUE,
};

constexpr Ability SlowStart = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].slowStartTimer = 5;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
    },
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK && statId != STAT_SPATK && statId != STAT_SPEED) return;
            if (gVolatileStructs[battler].slowStartTimer) *stat /= 2;
        },
};

constexpr Ability Scrappy = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
        CHECK(defType == TYPE_GHOST)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    },
    .tauntImmune = TRUE,
};

constexpr Ability StormDrain = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_WATER,
    .breakable = TRUE,
};

constexpr Ability IceBody = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
    .hailImmune = TRUE,
};

constexpr Ability SolidRock = {
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability SnowWarning = {
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

constexpr Ability HoneyGather = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK(Random() % 2)

        gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
        BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
        return TRUE;
    },
};

constexpr Ability Frisk = {
    .onEntry = +[](ON_ENTRY) -> int {
        int any = FALSE;
        for (int i = GetOppositeSide(battler); i < gBattlersCount; i += 2) {
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

constexpr Ability Reckless = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
        },
};

constexpr Ability Multitype = {
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability FlowerGift = {
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

constexpr Ability BadDreams = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
        return NO_ANNOUNCE;
    },
};

constexpr Ability SheerForce = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
        },
};

constexpr Ability Contrary = {
    .breakable = TRUE,
};

constexpr Ability Unnerve = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); },
    .unnerve = TRUE,
};

constexpr Ability Defeatist = {
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK && statId != STAT_SPATK) return;
            if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
        },
};

constexpr Ability CursedBody = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(gVolatileStructs[attacker].disabledMove)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsAbilityStatusProtected(attacker, CHECK_RESTRICTING))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])
        CHECK(Random() % 100 < 30)

        gVolatileStructs[attacker].disabledMove = gChosenMove;
        gVolatileStructs[attacker].disableTimer = 4;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove);
        BattleScriptCall(BattleScript_CursedBodyActivates);
        return TRUE;
    },
};

constexpr Ability Healer = {
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

constexpr Ability FriendGuard = {
    .breakable = TRUE,
};

constexpr Ability WeakArmor = {
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

constexpr Ability LightMetal = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= 1.3;
        },
};

constexpr Ability Multiscale = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (BATTLER_MAX_HP(battler)) MUL(.5);
        },
    .breakable = TRUE,
};

constexpr Ability ToxicBoost = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

int FlareBoostHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
    CHECK(CanBeBurned(battler))
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

    InsertCorrectEndType(callType);
    gBattleMons[battler].status1 |= STATUS1_BURN;
    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
    MarkBattlerForControllerExec(battler);
    BattleScriptCall(BattleScript_FlareBoostRet);
    return TRUE;
}

constexpr Ability FlareBoost = {
    .onEntry = +[](ON_ENTRY) -> int { return FlareBoostHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onWeather = +[](ON_WEATHER) -> int { return FlareBoostHandler(ability, battler, ABILITY_BS_CALL); },
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_SPATK) return;
            if (gBattleMons[battler].status1 & STATUS1_BURN) *stat *= 1.5;
        },
    .negatesBurnAtkDrop = TRUE,
};

constexpr Ability Harvest = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK_NOT(gBattleStruct->changedItems[battler])
        CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

        BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
        return TRUE;
    },
};

constexpr Ability Telepathy = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_ATTACKER_OR_TARGET,
    .breakable = TRUE,
};

constexpr Ability Moody = {
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

constexpr Ability Overcoat = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.8);
        },
    .breakable = TRUE,
    .powderImmune = TRUE,
    .sandImmune = TRUE,
    .hailImmune = TRUE,
};

constexpr Ability PoisonTouch = {
    .onAttacker = PoisonPoint.onAttacker,
    .onDefender = PoisonPoint.onDefender,
};

constexpr Ability Regenerator = {
    .onExit = +[](ON_EXIT) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        BattleScriptCall(BattleScript_RegeneratorExits);
        return FALSE;
    },
};

constexpr Ability BigPecks = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsMoveMakingContact(move, battler)) MUL(1.3);
        },
};

constexpr Ability SandRush = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        },
    .sandImmune = TRUE,
};

constexpr Ability WonderSkin = {
    .fortKnox = TRUE,
};

constexpr Ability Analytic = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
        },
};

constexpr Ability Illusion = {
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

constexpr Ability Imposter = {
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

constexpr Ability Infiltrator = {
    .onInfiltrate = +[](ON_INFILTRATE) -> InfiltrateType { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; },
};

constexpr Ability Mummy = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(attacker)))
        CHECK_NOT(DoesBattlerHaveAbilityShield(attacker))

        UpdateAbilityStateIndicesForNewAbility(attacker, ability);
        ReplaceAbility(attacker, ability);
        BattleScriptCall(BattleScript_MummyActivates);
        return TRUE;
    },
};

constexpr Ability Moxie = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_ATK); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Justified = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_DARK);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
};

constexpr Ability Rattled = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

constexpr Ability MagicBounce = {
    .breakable = TRUE,
    .magicBounce = TRUE,
};

constexpr Ability SapSipper = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_GRASS);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_GRASS,
    .breakable = TRUE,
};

constexpr Ability Prankster = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IS_MOVE_STATUS(move))
        return 1;
    },
};

constexpr Ability SandForce = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        },
    .sandImmune = TRUE,
};

constexpr Ability IronBarbs = {
    .onDefender = RoughSkin.onDefender,
};

constexpr Ability ZenMode = {
    .onEntry = Forecast.onEntry,
    .onEndTurn = Forecast.onEndTurn,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability VictoryStar = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_ALLY,
};

constexpr Ability Turboblaze = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FIRE); },
};

constexpr Ability Teravolt = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ELECTRIC); },
};

constexpr Ability AromaVeil = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK))
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

constexpr Ability FlowerVeil = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

constexpr Ability CheekPouch = {
    .randomizerBanned = TRUE,
};

constexpr Ability Protean = {
    .onBeforeAttack = +[](ABILITY_ON_BEFORE_ATTACK) -> int {
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, moveType))
        CHECK(move != MOVE_STRUGGLE)
        SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        BattleScriptCall(BattleScript_ProteanActivates);
        return TRUE;
    },
};

constexpr Ability FurCoat = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

constexpr Ability Bulletproof = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
};

constexpr Ability StrongJaw = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
        },
};

constexpr Ability Refrigerate = {
    ATE_ABILITY(TYPE_ICE),
};

constexpr Ability SweetVeil = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

constexpr Ability StanceChange = {
    .onBeforeAttack = +[](ABILITY_ON_BEFORE_ATTACK) -> int {
        SpeciesEnum newSpecies = SPECIES_NONE;
        switch (gBattleMons[battler].species) {
            default:
                return FALSE;
            case SPECIES_AEGISLASH:  // Shield -> Blade
                if (gBattleMoves[move].power > 0) newSpecies = SPECIES_AEGISLASH_BLADE;
                break;
            case SPECIES_AEGISLASH_BLADE:  // Blade -> Shield
                if (move == MOVE_KINGS_SHIELD) newSpecies = SPECIES_AEGISLASH;
                break;
            case SPECIES_AEGISLASH_BLADE_REDUX:  // Special -> Physical
                if (gBattleMoves[move].split == SPLIT_PHYSICAL && !gBattleMoves[move].arrowBased) newSpecies = SPECIES_AEGISLASH_REDUX;
                break;
            case SPECIES_AEGISLASH_BLADE_REDUX_MEGA:  // Special -> Physical
                if (gBattleMoves[move].split == SPLIT_PHYSICAL && !gBattleMoves[move].arrowBased) newSpecies = SPECIES_AEGISLASH_REDUX_MEGA;
                break;
            case SPECIES_AEGISLASH_REDUX:  // Physical -> Special
                if (gBattleMoves[move].split == SPLIT_SPECIAL || gBattleMoves[move].arrowBased) newSpecies = SPECIES_AEGISLASH_BLADE_REDUX;
                break;
            case SPECIES_AEGISLASH_REDUX_MEGA:  // Physical -> Special
                if (gBattleMoves[move].split == SPLIT_SPECIAL || gBattleMoves[move].arrowBased) newSpecies = SPECIES_AEGISLASH_BLADE_REDUX_MEGA;
                break;
        }
        CHECK(newSpecies)

        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability GaleWings = {
    .onPriority = GALE_WINGS_CLONE(TYPE_FLYING),
};

constexpr Ability MegaLauncher = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
        },
    .megaLauncherBoost = TRUE,
};

constexpr Ability GrassPelt = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
        },
    .breakable = TRUE,
};

constexpr Ability ToughClaws = {
    .onOffensiveMultiplier = BigPecks.onOffensiveMultiplier,
};

constexpr Ability Pixilate = {
    ATE_ABILITY(TYPE_FAIRY),
};

constexpr Ability Gooey = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
        CHECK(IsMoveMakingContact(move, attacker))

        BattleScriptCall(BattleScript_GooeyActivates);
        gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
        return TRUE;
    },
};

constexpr Ability Aerilate = {
    ATE_ABILITY(TYPE_FLYING),
};

constexpr Ability ParentalBond = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType { return PARENTAL_BOND_HYPER_AGGRESSIVE; },
    .resistsFortKnox = TRUE,
};

constexpr Ability DarkAura = {
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

constexpr Ability FairyAura = {
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

constexpr Ability AuraBreak = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); },
    .breakable = TRUE,
};

constexpr Ability PrimordialSea = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
        return TRUE;
    },
};

constexpr Ability DesolateLand = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
        return TRUE;
    },
};

constexpr Ability DeltaStream = {
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

constexpr Ability Stamina = {
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

constexpr Ability WimpOut = {
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

constexpr Ability EmergencyExit = {
    .onDefender = WimpOut.onDefender,
};

constexpr Ability WaterCompaction = {
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

constexpr Ability Merciless = {
    .onCrit = +[](ON_CRIT) -> int {
        if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
        if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
        if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
        return 0;
    },
};

constexpr Ability ShieldsDown = {
    .onEntry = Forecast.onEntry,
    .onEndTurn = Forecast.onEndTurn,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
        CHECK(gBattleMoves[move].effect == EFFECT_SHELL_SMASH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        int i;
        for (i = 0; i < ARRAY_COUNT(gHpTransformations); i++) {
            if (gHpTransformations[i].ability == ability && gBattleMons[battler].species == gHpTransformations[i].highHpSpecies) break;
        }

        if (i < ARRAY_COUNT(gHpTransformations)) {
            UpdateAbilityStateIndicesForNewSpecies(battler, gHpTransformations[i].lowHpSpecies);
            SetAbilityState(battler, ability, TRUE);
            gBattleMons[battler].species = gHpTransformations[i].lowHpSpecies;
            BattleScriptCall(BattleScript_AttackerFormChange);
            return TRUE;
        }
        return FALSE;
    },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        switch (gBattleMons[battler].species) {
            case SPECIES_MINIOR:
            case SPECIES_MINIOR_METEOR_ORANGE:
            case SPECIES_MINIOR_METEOR_YELLOW:
            case SPECIES_MINIOR_METEOR_GREEN:
            case SPECIES_MINIOR_METEOR_BLUE:
            case SPECIES_MINIOR_METEOR_INDIGO:
            case SPECIES_MINIOR_METEOR_VIOLET:
                return TRUE;

            default:
                return FALSE;
        }
    },
    .unsuppressable = TRUE,
};

constexpr Ability Stakeout = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
        },
};

constexpr Ability WaterBubble = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) MUL(2.0);
        },
    .onDefensiveMultiplier = Heatproof.onDefensiveMultiplier,
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BURN)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability Steelworker = {
    ATE_ABILITY(TYPE_STEEL),
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability Berserk = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        int stat = GetHighestAttackingStatId(battler, TRUE);
        CHECK(CanRaiseStat(battler, stat))

        SetAbilityState(battler, ability, TRUE);
        SetStatChanger(stat, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

constexpr Ability SlushRush = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
    .hailImmune = TRUE,
};

constexpr Ability LongReach = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move) && !gBattleMoves[move].contact) MUL(1.2);
        },
};

constexpr Ability LiquidVoice = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsSoundMove(battler, move)) MUL(1.2);
        },
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        return TYPE_WATER + 1;
    },
};

constexpr Ability Triage = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IsHealingMoveEffect(gBattleMoves[move].effect))
        return 3;
    },
};

constexpr Ability Galvanize = {
    ATE_ABILITY(TYPE_ELECTRIC),
};

constexpr Ability SurgeSurfer = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
        },
};

constexpr Ability Schooling = {
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

static int DisguiseReformHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
    SpeciesEnum newSpecies;
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
    CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

    InsertCorrectEndType(callType);
    UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
    gBattleMons[battler].species = newSpecies;
    BattleScriptCall(BattleScript_AttackerFormChange);
    return TRUE;
}
constexpr Ability Disguise = {
    .onEntry = +[](ON_ENTRY) -> int { return DisguiseReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDisguise = +[](ON_DISGUISE) -> SpeciesEnum {
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

constexpr Ability BattleBond = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        SpeciesEnum newSpecies = SPECIES_NONE;
        switch (gBattleMons[battler].species) {
            case SPECIES_GRENINJA_BATTLE_BOND:
                newSpecies = SPECIES_GRENINJA_ASH;
                break;

            case SPECIES_CHESNAUGHT_BATTLE_BOND:
                newSpecies = SPECIES_CHESNAUGHT_CLEMONT;
                break;

            case SPECIES_DELPHOX_BATTLE_BOND:
                newSpecies = SPECIES_DELPHOX_SERENA;
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

constexpr Ability PowerConstruct = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_ZYGARDE || gBattleMons[battler].species == SPECIES_ZYGARDE_10)
        CHECK(gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = gBattleMons[battler].species;
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_ZYGARDE_COMPLETE);
        gBattleMons[battler].species = SPECIES_ZYGARDE_COMPLETE;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability Corrosion = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_POISON)
        CHECK(defType == TYPE_STEEL)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
    .onCanStatusType = +[](ABILITY_ON_CAN_STATUS_TYPE) -> int {
        CHECK(status & CHECK_POISON)
        return TRUE;
    },
};

constexpr Ability Comatose = {
    .onEntry = +[](ON_ENTRY) -> int {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COMATOSE;
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceStatusAbility);
        return TRUE;
    },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability QueenlyMajesty = {
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

constexpr Ability InnardsOut = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))

        gBattleMoveDamage = gTurnStructs[battler].dmg;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    },
};

constexpr Ability Dancer = {
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(IsDance(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
};

constexpr Ability Battery = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
};

constexpr Ability Fluffy = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

constexpr Ability Dazzling = {
    .onImmune = QueenlyMajesty.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

constexpr Ability SoulHeart = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
};

constexpr Ability TanglingHair = {
    .onDefender = Gooey.onDefender,
};

constexpr Ability Receiver = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        AbilityEnum allyAbility = GetBattlerAbility(fainted);
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

constexpr Ability PowerOfAlchemy = {
    .onEntry = +[](ON_ENTRY) -> int {
        int any = FALSE;
        for (int i = GetOppositeSide(battler); i < gBattlersCount; i += 2) {
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

constexpr Ability BeastBoost = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, GetHighestStatId(battler, FALSE)); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability RksSystem = {
    .onBeforeAttack = Protean.onBeforeAttack,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .adaptability = TRUE,
};

constexpr Ability ElectricSurge = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

        for (int i = 0; i < gBattlersCount; i++) {
            DisableSwitchInAbility(i, ABILITY_GENERATOR);
            DisableSwitchInAbility(i, ABILITY_ENERGIZED);
        }
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
};

constexpr Ability PsychicSurge = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_PSYCHIC,
};

constexpr Ability MistySurge = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_MISTY,
};

constexpr Ability GrassySurge = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_GRASSY,
};

constexpr Ability ShadowShield = {
    .onDefensiveMultiplier = Multiscale.onDefensiveMultiplier,
};

constexpr Ability PrismArmor = {
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
};

constexpr Ability Neuroforce = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.35);
        },
};

constexpr Ability IntrepidSword = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_ATK))

        SetStatChanger(STAT_ATK, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

constexpr Ability DauntlessShield = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

constexpr Ability Libero = {
    .onBeforeAttack = Protean.onBeforeAttack,
};

constexpr Ability CottonDown = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit());
        gStackBattler1 = BATTLE_OPPOSITE(battler);
        CHECK(IsBattlerAlive(gStackBattler1) || IsBattlerAlive(BATTLE_PARTNER(gStackBattler1)))

        gEffectBattler = battler;
        gStackBattler1 = GetOppositeSide(battler);
        BattleScriptCall(BattleScript_CottonDownActivates);
        return TRUE;
    },
};

constexpr Ability MirrorArmor = {
    .breakable = TRUE,
};

constexpr Ability GulpMissile = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
        CHECK(gBattleMons[battler].species == SPECIES_CRAMORANT)
        CHECK(((gCurrentMove == MOVE_SURF || gCurrentMove == MOVE_TRIPLE_DIVE) && TARGET_TURN_DAMAGED) || gStatuses3[battler] & STATUS3_UNDERWATER ||
              (gCurrentMove == MOVE_DIVE && gBattleScripting.acceleratedTwoTurn))

        SpeciesEnum newSpecies = gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2 ? SPECIES_CRAMORANT_GORGING : SPECIES_CRAMORANT_GULPING;
        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        SpeciesEnum species = gBattleMons[battler].species;
        CHECK(species == SPECIES_CRAMORANT_GORGING || species == SPECIES_CRAMORANT_GULPING)
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_CRAMORANT);
        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        BattleScriptCall(species == SPECIES_CRAMORANT_GORGING ? BattleScript_GulpMissileGorging : BattleScript_GulpMissileGulping);
        gBattleMons[battler].species = SPECIES_CRAMORANT;
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability SteamEngine = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPEED))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

        SetStatChanger(STAT_SPEED, 12);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

constexpr Ability PunkRock = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsSoundMove(battler, move)) MUL(1.3);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsSoundMove(attacker, move)) MUL(.5);
        },
    .breakable = TRUE,
};

constexpr Ability SandSpit = {
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
    .sandImmune = TRUE,
};

constexpr Ability IceScales = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

int IceFaceReformHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
    CHECK(gBattleMons[battler].species == SPECIES_EISCUE_NOICE_FACE)
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
    CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

    InsertCorrectEndType(callType);
    UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_EISCUE);
    gBattleMons[battler].species = SPECIES_EISCUE;
    BattleScriptCall(BattleScript_AttackerFormChange);
    return TRUE;
}
constexpr Ability IceFace = {
    .onEntry = +[](ON_ENTRY) -> int { return IceFaceReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDisguise = +[](ON_DISGUISE) -> SpeciesEnum { return gBattleMons[battler].species == SPECIES_EISCUE ? SPECIES_EISCUE_NOICE_FACE : SPECIES_NONE; },
    .onWeather = +[](ON_WEATHER) -> int { return IceFaceReformHandler(ability, battler, ABILITY_BS_CALL); },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .hailImmune = TRUE,
};

constexpr Ability PowerSpot = {
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MUL(1.3); },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
};

int HandleMimicry(u8 battler, AbilityEnum ability, AbilityCallType endType) {
    u32 moveType = 0;

    switch (gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) {
        case STATUS_FIELD_ELECTRIC_TERRAIN:
            moveType = TYPE_ELECTRIC;
            break;
        case STATUS_FIELD_MISTY_TERRAIN:
            moveType = TYPE_FAIRY;
            break;
        case STATUS_FIELD_GRASSY_TERRAIN:
            moveType = TYPE_GRASS;
            break;
        case STATUS_FIELD_PSYCHIC_TERRAIN:
            moveType = TYPE_PSYCHIC;
            break;
        default:
            moveType = 0;
            break;
    }

    gStackBattler1 = battler;

    if (!moveType) {
        MimicryState state = GetAbilityStateAs(battler, ability).mimicryState;
        if (state.active) {
            SetAbilityState(battler, ability, 0);
            gBattleMons[battler].type1 = state.type1;
            gBattleMons[battler].type2 = state.type2;
            InsertCorrectEndType(endType);
            BattleScriptCall(BattleScript_MimicryEnds);
            return TRUE;
        }
    } else {
        if (!IS_BATTLER_OF_TYPE(battler, moveType)) {
            MimicryState state = GetAbilityStateAs(battler, ability).mimicryState;
            if (!state.active) {
                SetAbilityStateAs(battler,
                                  ability,
                                  (AbilityStates){
                                      .mimicryState =
                                          {
                                              .type1 = gBattleMons[battler].type1,
                                              .type2 = gBattleMons[battler].type2,
                                              .active = TRUE,
                                          },
                                  });
            }
            SET_BATTLER_TYPE(battler, moveType);
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, moveType);
            InsertCorrectEndType(endType);
            BattleScriptCall(BattleScript_MimicryActivates);
            return TRUE;
        }
    }

    return FALSE;
}
constexpr Ability Mimicry = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerAlive(battler))

        return HandleMimicry(battler, ability, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    },
    .onTerrain = +[](ON_TERRAIN) -> int {
        CHECK(IsBattlerAlive(battler))

        return HandleMimicry(battler, ability, ABILITY_BS_CALL);
    },
};

constexpr Ability ScreenCleaner = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryRemoveScreens(battler))

        return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
    },
};

constexpr Ability SteelySpirit = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_STEEL) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY,
};

constexpr Ability PerishBody = {
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

constexpr Ability WanderingSpirit = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(GetBattlerAbility(battler) == ability)
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability))
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

constexpr Ability GorillaTactics = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

constexpr Ability NeutralizingGas = {
    .unsuppressable = TRUE,
};

constexpr Ability PastelVeil = {
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

constexpr Ability HungerSwitch = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        SpeciesEnum newSpecies;
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
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

constexpr Ability CuriousMedicine = {
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

constexpr Ability Transistor = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.5);
        },
};

constexpr Ability DragonsMaw = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_DRAGON) MUL(1.5);
        },
};

constexpr Ability ChillingNeigh = {
    .onBattlerFaints = Moxie.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability GrimNeigh = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_SPATK); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability AsOneIceRider = {
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
    .unnerve = TRUE,
};

constexpr Ability AsOneShadowRider = {
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
    .unnerve = TRUE,
};

constexpr Ability Chloroplast = {
    .chloroplast = TRUE,
};

constexpr Ability Whiteout = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
    .hailImmune = TRUE,
};

constexpr Ability Pyromancy = {
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
        },
};

constexpr Ability KeenEdge = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
        },
};

constexpr Ability PrismScales = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.7);
        },
    .breakable = TRUE,
};

constexpr Ability PowerFists = {
    .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(IsIronFistBoosted(battler, move))
        return STAT_SPDEF;
    },
};

constexpr Ability SandSong = {
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_GROUND + 1;
    },
};

constexpr Ability Rampage = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        SetAbilityState(battler, ability, TRUE);
        gVolatileStructs[battler].rechargeTimer = 0;
        gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
        return FALSE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Vengeance = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GHOST),
};

constexpr Ability BlitzBoxer = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IsIronFistBoosted(battler, move))
        CHECK(BATTLER_MAX_HP(battler));
        return 1;
    },
};

constexpr Ability AntarcticBird = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
        },
};

constexpr Ability Immolate = {
    ATE_ABILITY(TYPE_FIRE),
};

constexpr Ability Crystallize = {
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

constexpr Ability Electrocytes = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.25);
        },
};

constexpr Ability Aerodynamics = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FLYING);
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

constexpr Ability ChristmasSpirit = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
        },
    .breakable = TRUE,
    .hailImmune = TRUE,
};

constexpr Ability ExploitWeakness = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(target)) MUL(1.25);
        },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(HasAnyStatusOrAbility(target))
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

constexpr Ability GroundShock = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_GROUND)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(.5);
        return TRUE;
    },
};

constexpr Ability AncientIdol = {
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; },
};

constexpr Ability MysticPower = {
    .onStab = +[](ON_STAB) -> int { return TRUE; },
};

constexpr Ability Perfectionist = {
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

constexpr Ability GrowingTooth = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_AttackBoostActivates);
        return TRUE;
    },
};

constexpr Ability Inflatable = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
        BattleScriptCall(BattleScript_InflatableActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

constexpr Ability AuroraBorealis = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_ICE; },
    .hailImmune = TRUE,
};

constexpr Ability Avenger = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
        },
};

constexpr Ability LetsRoll = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
        return TRUE;
    },
};

constexpr Ability LoudBang = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeConfused(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
    },
};

constexpr Ability LeadCoat = {
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

constexpr Ability Amphibious = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_DRENCH)
        return TRUE;
    },
};

constexpr Ability Grounded = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GROUND); },
};

constexpr Ability Earthbound = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GROUND),
};

constexpr Ability FightingSpirit = {
    ATE_ABILITY(TYPE_FIGHTING),
};

constexpr Ability FelineProwess = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK) *stat *= 2;
        },
};

constexpr Ability CoilUp = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

        gStatuses4[battler] |= STATUS4_COILED;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
        return TRUE;
    },
};

constexpr Ability Fossilized = {
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

constexpr Ability MagicalDust = {
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

constexpr Ability Dreamcatcher = {
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

constexpr Ability Nocturnal = {
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

constexpr Ability SelfSufficient = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)

        gBattleMoveDamage = gBattleMons[battler].maxHP / 16;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    },
};

constexpr Ability Tectonize = {
    ATE_ABILITY(TYPE_GROUND),
};

constexpr Ability IceAge = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ICE); },
};

constexpr Ability HalfDrake = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_DRAGON); },
};

constexpr Ability Aquatic = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_WATER); },
};

constexpr Ability Liquified = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) RESISTANCE(2);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

constexpr Ability Dragonfly = {
    .onEntry = HalfDrake.onEntry,
    .breakable = TRUE,
    .levitate = TRUE,
};

constexpr Ability Dragonslayer = {
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

constexpr Ability Mountaineer = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (moveType == TYPE_ROCK) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .stealthRockImmune = TRUE,
};

constexpr Ability Hydrate = {
    ATE_ABILITY(TYPE_WATER),
};

constexpr Ability Metallic = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_STEEL); },
};

constexpr Ability Permafrost = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        },
    .breakable = TRUE,
};

constexpr Ability PrimalArmor = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
        },
    .breakable = TRUE,
};

constexpr Ability RagingBoxer = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(IsIronFistBoosted(battler, move))
        return PARENTAL_BOND_PRIMAL_MAW;
    },
};

constexpr Ability AirBlower = {
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

constexpr Ability Juggernaut = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
        },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability ShortCircuit = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_ELECTRIC),
};

constexpr Ability MajesticBird = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK) *stat *= 1.5;
        },
};

constexpr Ability Phantom = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GHOST); },
};

constexpr Ability Intoxicate = {
    ATE_ABILITY(TYPE_POISON),
};

constexpr Ability Impenetrable = {
    .magicGuard = TRUE,
};

constexpr Ability Hypnotist = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_HYPNOSIS);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    },
};

constexpr Ability Overwhelm = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
        return TRUE;
    },
    .tauntImmune = TRUE,
};

constexpr Ability Scare = {
    .onEntry = UseIntimidateClone,
};

constexpr Ability MajesticMoth = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

constexpr Ability SoulEater = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler));
        CHECK(CanBattlerHeal(battler));
        BattleScriptCall(BattleScript_HandleSoulEaterEffect);
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
constexpr Ability SoulLinker = {
    ON_EITHER_ABILITY(SoulLinker),
};

constexpr Ability SweetDreams = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gBattleMons[battler].status1 & STATUS1_SLEEP || BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SweetDreamsActivates);
        return TRUE;
    },
};

constexpr Ability BadLuck = {
    .onCrit = +[](ON_CRIT) -> int { return NEVER_CRIT; },
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (*effectChance < 1) *effectChance = 0;
        },
    .onCritFor = APPLY_ON_FOE,
    .onModifyEffectChanceFor = APPLY_ON_FOE,
    .breakable = TRUE,
    .foesMinRoll = TRUE,
};

constexpr Ability HauntedSpirit = {
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

constexpr Ability ElectricBurst = {
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

constexpr Ability RawWood = {
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

constexpr Ability Solenoglyphs = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
};

constexpr Ability SpiderLair = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

        int side = GetOppositeSide(battler);
        gSideTimers[side].started.spiderWeb = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[side].stickyWebTimer = 5;
        BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
        return TRUE;
    },
};

constexpr Ability FatalPrecision = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK_NOT(IS_MOVE_STATUS(move))
        CHECK(CalcTypeEffectivenessMultiplier(move, moveType, battler, target, TRUE) >= UQ_4_12(2.0))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(typeEffectiveness >= UQ_4_12(2.0))
        return ALWAYS_CRIT;
    },
};

constexpr Ability FortKnox = {
    .fortKnox = TRUE,
};

constexpr Ability Seaweed = {
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

constexpr Ability PsychicMind = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_PSYCHIC),
};

constexpr Ability PoisonAbsorb = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_POISON)
        return ABSORB_RESULT_HEAL;
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
    .redirectType = TYPE_POISON,
    .breakable = TRUE,
};

constexpr Ability Scavenger = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability TwistedDimension = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

        gFieldTimers.started.trickRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
        return TRUE;
    },
};

constexpr Ability MultiHeaded = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
        if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
        return MULTIHIT_SINGLE;
    },
    .resistsFortKnox = TRUE,
};

constexpr Ability NorthWind = {
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
    .hailImmune = TRUE,
};

constexpr Ability Overcharge = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_ELECTRIC)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
    .onCanStatusType = +[](ABILITY_ON_CAN_STATUS_TYPE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
};

constexpr Ability ViolentRush = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
    },
};

constexpr Ability FlamingSoul = {
    .onPriority = GALE_WINGS_CLONE(TYPE_FIRE),
};

constexpr Ability SagePower = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(1.5);
        },
};

constexpr Ability BoneZone = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod >= UQ_4_12(1.0)) return;
            if (*mod == 0) {
                *mod = UQ_4_12(1.0);
                if (mod1) MulModifier(mod, mod1);
                if (mod2) MulModifier(mod, mod2);
                if (mod3) MulModifier(mod, mod3);
            }
            if (*mod < UQ_4_12(1.0)) MulModifier(mod, UQ_4_12(2.0));
        },
};

constexpr Ability WeatherControl = {
    .onImmune = DeltaStream.onImmune,
    .breakable = TRUE,
};

constexpr Ability SpeedForce = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
        },
};

constexpr Ability SeaGuardian = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        SetStatChanger(stat, 1);
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

constexpr Ability MoltenDown = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(defType == TYPE_ROCK)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
};

constexpr Ability HyperAggressive = {
    .onParentalBond = ParentalBond.onParentalBond,
};

constexpr Ability Flock = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FLYING),
};

constexpr Ability FieldExplorer = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
        },
};

constexpr Ability Striker = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsStrikerBoosted(battler, move)) MUL(1.3);
        },
};

constexpr Ability FrozenSoul = {
    .onPriority = GALE_WINGS_CLONE(TYPE_ICE),
};

constexpr Ability Predator = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Looter = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability LunarEclipse = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; },
    .onAccuracy = Hypnotist.onAccuracy,
};

constexpr Ability SolarFlare = {
    .onOffensiveMultiplier = Immolate.onOffensiveMultiplier,
    .onMoveType = Immolate.onMoveType,
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FIRE; },
    .chloroplast = TRUE,
};

constexpr Ability PowerCore = {
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; },
};

constexpr Ability SightingSystem = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_HITS_IF_POSSIBLE; },
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMoves[move].accuracy)
        CHECK(gBattleMoves[move].accuracy < 80);
        return -3;
    },
};

constexpr Ability BadCompany = {
    .randomizerBanned = TRUE,
};

constexpr Ability Opportunist = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2)
        return 1;
    },
};

constexpr Ability GiantWings = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].airBased) MUL(1.3);
        },
};

constexpr Ability Momentum = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
        },
};

constexpr Ability GripPincer = {
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

constexpr Ability BigLeaves = {
    .onEndTurn = +[](ON_END_TURN) -> int { return Harvest.onEndTurn(DELEGATE_END_TURN) | LeafGuard.onEndTurn(DELEGATE_END_TURN); },
    .onStat =
        +[](ON_STAT) {
            SolarPower.onStat(DELEGATE_STAT);
            Chlorophyll.onStat(DELEGATE_STAT);
        },
    .chloroplast = TRUE,
};

constexpr Ability PreciseFist = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(IsIronFistBoosted(battler, move))
        return 1;
    },
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
        },
};

constexpr Ability Deadeye = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].arrowBased)
        return ACCURACY_HITS_IF_POSSIBLE;
    },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(gIsCriticalHit)
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

constexpr Ability Artillery = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsMegaLauncherBoosted(battler, move))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

constexpr Ability Amplifier = {
    .onOffensiveMultiplier = PunkRock.onOffensiveMultiplier,
};

constexpr Ability IceDew = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ICE);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_ICE,
    .breakable = TRUE,
};

constexpr Ability SunWorship = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

constexpr Ability Pollinate = {
    ATE_ABILITY(TYPE_BUG),
};

constexpr Ability VolcanoRage = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
    },
};

constexpr Ability ColdRebound = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
        return FALSE;
    },
};

constexpr Ability LowBlow = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FEINT_ATTACK, 40); },
};

constexpr Ability Spectralize = {
    ATE_ABILITY(TYPE_GHOST),
};

constexpr Ability SpectralShroud = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleStruct->ateBoost[battler])
        CHECK(moveType == TYPE_GHOST)
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
    .onOffensiveMultiplier = Spectralize.onOffensiveMultiplier,
    .onMoveType = Spectralize.onMoveType,
};

constexpr Ability Discipline = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability Thundercall = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
    },
};

constexpr Ability MarineApex = {
    .onInfiltrate = Infiltrator.onInfiltrate,
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
        },
};

constexpr Ability MightyHorn = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hornBased) MUL(1.3);
        },
};

constexpr Ability HardenedSheath = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].hornBased)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptCall(BattleScript_AttackBoostActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

constexpr Ability ArcticFur = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.65); },
    .breakable = TRUE,
};

constexpr Ability Lethargy = {
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

constexpr Ability IronBarrage = {
    .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
    .onAccuracy = SightingSystem.onAccuracy,
    .onPriority = SightingSystem.onPriority,
    .megaLauncherBoost = TRUE,
};

constexpr Ability SteelBarrel = {
    .onStatusImmune = RockHead.onStatusImmune,
    .noRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability PyroShells = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsMegaLauncherBoosted(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
    },
};

constexpr Ability FungalInfection = {
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

constexpr Ability Parry = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
        return FALSE;
    },
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.8); },
};

constexpr Ability Scrapyard = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    },
};

constexpr Ability LooseQuills = {
    .onDefender = Scrapyard.onDefender,
};

constexpr Ability ToxicDebris = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

        BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
        return TRUE;
    },
};

constexpr Ability Roundhouse = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsStrikerBoosted(battler, move))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(IsStrikerBoosted(battler, move))
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

constexpr Ability Mineralize = {
    ATE_ABILITY(TYPE_ROCK),
};

constexpr Ability LooseRocks = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

        BattleScriptCall(BattleScript_DefenderSetsStealthRock);
        return TRUE;
    },
};

constexpr Ability SpinningTop = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_FIGHTING)
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))

        int any = FALSE;
        if (gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_HAZARDS_ANY || gSideTimers[GetBattlerSide(battler)].hotCoals ||
            gSideTimers[GetBattlerSide(battler)].caltrops) {
            gSideStatuses[GetBattlerSide(battler)] &=
                ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES_DAMAGED | SIDE_STATUS_STICKY_WEB);
            gSideTimers[GetBattlerSide(battler)].hotCoals = FALSE;
            gSideTimers[GetBattlerSide(battler)].caltrops = FALSE;
            BattleScriptCall(BattleScript_AnnounceRemovedHazards);
            gBattleScripting.battler = battler;
            any = TRUE;
        }

        if (ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL)) {
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_AttackBoostActivates);
            any = TRUE;
        }

        return any;
    },
};

constexpr Ability RetributionBlow = {
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

constexpr Ability Fearmonger = {
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
};

constexpr Ability ToxicSpill = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
        return TRUE;
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (ability) {
            CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
            AbilityEnum sourceAbilities[] = {ABILITY_TOXIC_SPILL, ABILITY_TRASH_HEAP};
            for (auto sourceAbility : sourceAbilities) {
                int source = IsAbilityOnField(sourceAbility);
                FILTER(source)
                CHECK(sourceAbility == ability)
                CHECK(source - 1 == battler)
                break;
            }
        }

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))

            if (BATTLER_HAS_ABILITY(target, ABILITY_POISON_HEAL)) {
                FILTER_NOT(BATTLER_MAX_HP(target))
                FILTER(CanBattlerHeal(target))
                gStackBattler1 = target;
                BattleScriptExecute(BattleScript_ToxicWasteHeal);
                any = TRUE;
                continue;
            }

            FILTER_NOT(IS_BATTLER_OF_TYPE(target, TYPE_POISON))
            FILTER_NOT(IsMagicGuardProtected(target))
            FILTER_NOT(BATTLER_HAS_ABILITY(battler, ABILITY_TOXIC_BOOST))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_ToxicWasteTurnDmg);
            any = TRUE;
        }
        return any;
    },
    .onExit = +[](ON_EXIT) -> int {
        CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
        BattleScriptCall(BattleScript_TheToxicWasHasDissapeared);
        return TRUE;
    },
};

constexpr Ability DesertCloak = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY))
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

constexpr Ability Draconize = {
    ATE_ABILITY(TYPE_DRAGON),
};

constexpr Ability PrettyPrincess = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (!IsUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
        },
};

constexpr Ability SelfRepair = {
    .onEndTurn = SelfSufficient.onEndTurn,
    .onExit = NaturalCure.onExit,
};

constexpr Ability Electromorphosis = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    },
};

constexpr Ability AtomicBurst = {
    .onDefender = Electromorphosis.onDefender,
    ATE_ABILITY(TYPE_ELECTRIC),
};

constexpr Ability Hellblaze = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIRE),
};

constexpr Ability Riptide = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_WATER),
};

constexpr Ability ForestRage = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GRASS),
};

constexpr Ability PrimalMaw = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        return PARENTAL_BOND_PRIMAL_MAW;
    },
};

constexpr Ability SweepingEdge = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

constexpr Ability GiftedMind = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IS_MOVE_STATUS(move))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (moveType == TYPE_BUG || moveType == TYPE_GHOST || moveType == TYPE_DARK) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability HydroCircuit = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(moveType == TYPE_WATER)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
    .onOffensiveMultiplier = Transistor.onOffensiveMultiplier,
};

constexpr Ability Equinox = {
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

constexpr Ability Absorbant = {
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

constexpr Ability Clueless = {
    .onEntry = CloudNine.onEntry,
    .unsuppressable = TRUE,
};

constexpr Ability CheatingDeath = {
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

constexpr Ability CheapTactics = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SCRATCH, 0); },
};

constexpr Ability Coward = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        SetSingleUseAbilityCounter(battler, ability, TRUE);
        gRoundStructs[battler].protectedThisTurn = TRUE;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
        return TRUE;
    },
    .persistent = TRUE,
};

constexpr Ability VoltRush = {
    .onPriority = GALE_WINGS_CLONE(TYPE_ELECTRIC),
};

constexpr Ability DuneTerror = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GROUND) MUL(1.2);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
        },
    .breakable = TRUE,
    .sandImmune = TRUE,
};

constexpr Ability InfernalRage = {
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

constexpr Ability DualWield = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

constexpr Ability ElementalCharge = {
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

constexpr Ability Ambush = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn)
        return ALWAYS_CRIT;
    },
};

constexpr Ability Atlas = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    },
};

constexpr Ability Radiance = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(moveType == TYPE_DARK);
        *immunityScript = BattleScript_RadianceProtected;
        return TRUE;
    },
    .onAccuracy = Illuminate.onAccuracy,
    .onImmuneFor = APPLY_ON_ANY,
    .breakable = TRUE,
};

constexpr Ability JawsOfCarnage = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        if (gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST)
            BattleScriptCall(BattleScript_HandleJawsOfCarnageEffect);
        else
            BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability AngelsWrath = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        switch (move) {
            case MOVE_TACKLE: {
                CHECK(ShouldApplyOnHitAffect(target))
                CHECK(!IsAbilityStatusProtected(target, CHECK_RESTRICTING))
                CHECK(!gVolatileStructs[target].encoreTimer || !gVolatileStructs[target].disableTimer)

                if (!gVolatileStructs[target].encoreTimer) {
                    gVolatileStructs[target].encoreTimer = 2;
                    gVolatileStructs[target].encoredMove = gBattleMons[target].moves[0];
                }

                if (!gVolatileStructs[target].disableTimer) {
                    gVolatileStructs[target].disableTimer = gVolatileStructs[target].disableTimerStartValue = 2;
                    gVolatileStructs[target].disabledMove = gBattleMons[target].moves[0];
                }

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
                CHECK(CanBattlerHeal(battler))

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
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (move == MOVE_POISON_STING) *effectChance = 100;
        },
    .onCanStatusType = +[](ABILITY_ON_CAN_STATUS_TYPE) -> int {
        CHECK(status & CHECK_POISON)
        CHECK(move == MOVE_POISON_STING)
        return TRUE;
    },
};

constexpr Ability PrismaticFur = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.5); },
    .onBeforeAttack = +[](ABILITY_ON_BEFORE_ATTACK) -> int {
        if (battler == attacker && Protean.onBeforeAttack(DELEGATE_ON_BEFORE_ATTACK)) return TRUE;
        return ColorChange.onBeforeAttack(DELEGATE_ON_BEFORE_ATTACK);
    },
    .onBeforeAttackFor = APPLY_ON_ATTACKER_OR_TARGET,
};

constexpr Ability ShockingJaws = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
};

constexpr Ability FaeHunter = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_FAIRY)) RESISTANCE(1.5);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_FAIRY)) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

constexpr Ability GravityWell = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    },
};

constexpr Ability Evaporate = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_EVAPORATE;
    },
    .breakable = TRUE,
};

constexpr Ability Lumberjack = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) RESISTANCE(1.5);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_GRASS)) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

constexpr Ability WellBakedBody = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE);
        *statId = STAT_DEF;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
    .absorbUp2 = TRUE,
};

constexpr Ability Furnace = {
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

constexpr Ability RockyPayload = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
        },
};

constexpr Ability EarthEater = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_GROUND)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

constexpr Ability LingeringAroma = {
    .onDefender = Mummy.onDefender,
};

constexpr Ability FairyTale = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FAIRY); },
};

constexpr Ability RagingMoth = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(moveType == TYPE_FIRE)
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

constexpr Ability AdrenalineRush = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_SPEED); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Archmage = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DidMoveHit())
        CHECK_NOT(IS_MOVE_STATUS(move))
        CHECK(Random() % 100 < 30)

        switch (moveType) {
            case TYPE_POISON:
                CHECK(IsBattlerAlive(target))
                CHECK(CanBePoisoned(battler, target, MOVE_NONE))

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
                CHECK_NOT(IsAbilityStatusProtected(target, CHECK_RESTRICTING))
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

constexpr Ability Cryomancy = {
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
        },
};

constexpr Ability PhantomPain = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_GHOST)
        CHECK(defType == TYPE_NORMAL)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    },
};

constexpr Ability Purgatory = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GHOST),
};

constexpr Ability Emanate = {
    ATE_ABILITY(TYPE_PSYCHIC),
};

constexpr Ability KunoichiBlade = {
    .onOffensiveMultiplier = Technician.onOffensiveMultiplier,
    .skillLink = TRUE,
};

constexpr Ability MonkeyBusiness = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TICKLE, 0); },
};

constexpr Ability CombatSpecialist = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            IronFist.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Striker.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

constexpr Ability JunglesGuard = {
    .onStatusImmune = FlowerVeil.onStatusImmune,
    .onStatusImmuneFor = FlowerVeil.onStatusImmuneFor,
    .breakable = TRUE,
};

constexpr Ability HuntersHorn = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability PixiePower = {
    .onEntry = FairyAura.onEntry,
    .onOffensiveMultiplier = FairyAura.onOffensiveMultiplier,
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

constexpr Ability PlasmaLamp = {
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

constexpr Ability MagmaEater = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onTypeEffectiveness = MoltenDown.onTypeEffectiveness,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability SuperHotGoo = {
    .onAttacker = FlameBody.onAttacker,
    .onDefender = +[](ON_DEFENDER) -> int { return Gooey.onDefender(DELEGATE_DEFENDER) | FlameBody.onDefender(DELEGATE_DEFENDER); },
};

constexpr Ability Nika = {
    .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
};

constexpr Ability Archer = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].arrowBased) MUL(1.3);
        },
};

constexpr Ability SuperSlammer = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hammerBased) MUL(1.3);
        },
};

constexpr Ability InverseRoom = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

        gFieldTimers.started.inverseRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
        gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
        return TRUE;
    },
};

constexpr Ability FrostBurn = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
    },
};

constexpr Ability ItchyDefense = {
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

constexpr Ability Generator = {
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
    .persistent = TRUE,
};

constexpr Ability MoonSpirit = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; },
};

constexpr Ability DustCloud = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SAND_ATTACK, 0); },
};

constexpr Ability TippingPoint = {
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

constexpr Ability BerserkerRage = {
    .onDefender = TippingPoint.onDefender,
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Trickster = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DISABLE, 0); },
};

constexpr Ability SandGuard = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
        return QueenlyMajesty.onImmune(DELEGATE_IMMUNE);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
        },
    .breakable = TRUE,
    .sandImmune = TRUE,
};

constexpr Ability NaturalRecovery = {
    .onExit = +[](ON_EXIT) -> int { return NaturalCure.onExit(DELEGATE_EXIT) | Regenerator.onExit(DELEGATE_EXIT); },
};

constexpr Ability WindRider = {
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

constexpr Ability SoothingAroma = {
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

constexpr Ability PrimAndProper = {
    .onDefender = CuteCharm.onDefender,
    .fortKnox = TRUE,
};

constexpr Ability SuperStrain = {
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

constexpr Ability Enlightened = {
    .onOffensiveMultiplier = Emanate.onOffensiveMultiplier,
    .onMoveType = Emanate.onMoveType,
    .onAccuracy = InnerFocus.onAccuracy,
    .breakable = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability PeacefulSlumber = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (!SweetDreams.onEndTurn(DELEGATE_END_TURN)) return SelfSufficient.onEndTurn(DELEGATE_END_TURN);
        gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
        return TRUE;
    },
};

constexpr Ability Aftershock = {
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
constexpr Ability FreezingPoint = {
    ON_EITHER_ABILITY(FreezingPoint),
};

static int CryoProficiencyHail(AbilityEnum ability, int battler, int attacker, MoveEnum move, int moveType) {
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
constexpr Ability CryoProficiency = {
    .onAttacker = FreezingPoint.onAttacker,
    .onDefender =
        +[](ON_DEFENDER) -> int { return FreezingPoint.onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(ability, battler, attacker, move, moveType); },
    .hailImmune = TRUE,
};

constexpr Ability ArcaneForce = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
        },
    .onStab = MysticPower.onStab,
};

constexpr Ability Doombringer = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DOOM_DESIRE, 0); },
};

constexpr Ability Wishmaker = {
    .onEntry = +[](ON_ENTRY) -> int {
        int counter = GetSingleUseAbilityCounter(battler, ability);
        CHECK(counter < 3)
        CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

        SetSingleUseAbilityCounter(battler, ability, counter + 1);
        return TRUE;
    },
    .persistent = TRUE,
};

constexpr Ability YukiOnna = {
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanInfatuate(battler, target))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    },
};

constexpr Ability Suppress = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TORMENT, 0); },
};

constexpr Ability Refrigerator = {
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
    .onAccuracy = Illuminate.onAccuracy,
};

constexpr Ability HeavenAsunder = {
    .onCrit =
        +[](ON_CRIT) {
            if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
            return 1;
        },
};

constexpr Ability PurifyingWaters = {
    .onEntry = WaterVeil.onEntry,
    .onEndTurn = Hydration.onEndTurn,
    .onStatusImmune = WaterVeil.onStatusImmune,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability Seaborne = {
    .onEntry = Drizzle.onEntry,
    .onStat = SwiftSwim.onStat,
};

constexpr Ability HighTide = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_WATER)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
    },
};

constexpr Ability ChangeOfHeart = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_HEART_SWAP, 0); },
};

constexpr Ability MysticBlades = {
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return TRUE;
    },
};

constexpr Ability Determination = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
        },
    .negatesFrzSpatkDrop = TRUE,
};

constexpr Ability Fertilize = {
    ATE_ABILITY(TYPE_GRASS),
};

constexpr Ability PureLove = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gBattleMons[target].status2 & STATUS2_INFATUATION)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
    .onDefender = CuteCharm.onDefender,
    .canInfatuateAny = TRUE,
};

constexpr Ability Fighter = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIGHTING),
};

constexpr Ability Telekinetic = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TELEKINESIS, 0); },
};

constexpr Ability Combustion = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) MUL(1.5);
        },
};

constexpr Ability PonyPower = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            KeenEdge.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MysticBlades.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onSwapSplit = MysticBlades.onSwapSplit,
};

constexpr Ability PowderBurst = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_POWDER, 0); },
};

constexpr Ability Retriever = {
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

constexpr Ability MonsterMash = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TRICK_OR_TREAT, 0); },
};

constexpr Ability TwoStep = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
    },
};

constexpr Ability Spiteful = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])

        BattleScriptCall(BattleScript_AbilitySpiteful);
        return TRUE;
    },
};

constexpr Ability Fortitude = {
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

constexpr Ability Devourer = {
    .onParentalBond = PrimalMaw.onParentalBond,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
};

constexpr Ability PhantomThief = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SPECTRAL_THIEF, 40); },
};

constexpr Ability EarlyGrave = {
    .onPriority = GALE_WINGS_CLONE(TYPE_GHOST),
};

constexpr Ability BassBoosted = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Amplifier.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            PunkRock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onDefensiveMultiplier = PunkRock.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability FlamingJaws = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
};

constexpr Ability MonsterHunter = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_DARK)) RESISTANCE(1.5);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_DARK)) MUL(.5);
        },
    .breakable = TRUE,
};

constexpr Ability CrownedSword = {
    .onEntry = IntrepidSword.onEntry,
    .onDefender = AngerPoint.onDefender,
};

constexpr Ability CrownedShield = {
    .onEntry = DauntlessShield.onEntry,
    .onDefender = Stamina.onDefender,
};

constexpr Ability BerserkDna = {
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

constexpr Ability CrownedKing = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        return AsOneShadowRider.onBattlerFaints(DELEGATE_BATTLER_FAINTS) | AsOneIceRider.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .unnerve = TRUE,
};

constexpr Ability SnapTrapWhenHit = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
        return FALSE;
    },
};

constexpr Ability Permanence = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); },
};

constexpr Ability Hubris = {
    .onBattlerFaints = GrimNeigh.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability CosmicDaze = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
        },
};

constexpr Ability MindsEye = {
    .onTypeEffectiveness = Scrappy.onTypeEffectiveness,
    .breakable = TRUE,
};

constexpr Ability BloodPrice = {
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
constexpr Ability SpikeArmor = {
    ON_EITHER_ABILITY(SpikeArmor),
};

constexpr Ability VoodooPower = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IS_MOVE_SPECIAL(move))
        CHECK(CanBleed(attacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
        return TRUE;
    },
};

constexpr Ability ChromeCoat = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.6);
        },
    .onStat = LeadCoat.onStat,
    .breakable = TRUE,
};

constexpr Ability Banshee = {
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_GHOST + 1;
    },
};

constexpr Ability WebSpinner = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_STRING_SHOT, 0); },
};

constexpr Ability ShowdownMode = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
    },
};

constexpr Ability SeedSower = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptCall(BattleScript_SeedSower);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_GRASSY,
};

constexpr Ability Airborne = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY,
};

constexpr Ability Parroting = {
    .onImmune = Soundproof.onImmune,
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(IsSoundMove(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

constexpr Ability SaltCircle = {
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

constexpr Ability PurifyingSalt = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GHOST) RESISTANCE(.5);
        },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

int ProtosynthesisHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
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
constexpr Ability Protosynthesis = {
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

int QuarkDriveHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
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
constexpr Ability QuarkDrive = {
    .onEntry = +[](ON_ENTRY) -> int { return QuarkDriveHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onTerrain = +[](ON_TERRAIN) -> int { return QuarkDriveHandler(ability, battler, ABILITY_BS_CALL); },
    .onStat = Protosynthesis.onStat,
};

constexpr Ability WindPower = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].airBased)
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    },
};

constexpr Ability Impulse = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
        },
};

constexpr Ability TerminalVelocity = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
        },
};

constexpr Ability AngerShell = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_AngerShell);
        return TRUE;
    },
};

constexpr Ability Egoist = {
    .onReactive = +[](ON_REACTIVE) -> int {
        CHECK(gBattleStruct->statStageCheckState != STAT_STAGE_CHECK_NOT_NEEDED)
        for (int opponent = GetOppositeSide(battler); opponent < gBattlersCount; opponent += 2) {
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

constexpr Ability ReadiedAction = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
    },
};

constexpr Ability DarkGaleWings = {
    .onPriority = GALE_WINGS_CLONE(TYPE_DARK),
};

constexpr Ability GuiltTrip = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

        BattleScriptCall(BattleScript_GuiltTrip);
        return TRUE;
    },
};

constexpr Ability WaterGaleWings = {
    .onPriority = GALE_WINGS_CLONE(TYPE_WATER),
};

constexpr Ability ZeroToHero = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_PALAFIN)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
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

constexpr Ability Costar = {
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

constexpr Ability Commander = {
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

constexpr Ability EjectPackAbility = {
    .persistent = TRUE,
};

constexpr Ability VengefulSpirit = {
    .onDefender = HauntedSpirit.onDefender,
    .onOffensiveMultiplier = Vengeance.onOffensiveMultiplier,
};

constexpr Ability CudChew = {
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

constexpr Ability ArmorTail = {
    .onImmune = QueenlyMajesty.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

constexpr Ability MindCrush = {
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
        },
};

constexpr Ability SupremeOverlord = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFaintedMonCount[GetBattlerSide(battler)])

        return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
    },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
        },
};

constexpr Ability IllWill = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])
        CHECK_NOT(IsBattlerAlive(battler))

        gBattleMons[attacker].pp[gChosenMovePos] = 0;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove)
        gActiveBattler = attacker;
        BtlController_EmitSetMonData(0, gChosenMovePos + REQUEST_PPMOVE1_BATTLE, 0, 1, &gBattleMons[attacker].pp[gChosenMovePos]);
        BattleScriptCall(BattleScript_IllWillTakesPp);
        return TRUE;
    },
};

constexpr Ability FireScales = {
    .onDefensiveMultiplier = IceScales.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability WatchYourStep = {
    .onEntry = +[](ON_ENTRY) -> int {
        u8 targetSide = GetOppositeSide(battler);
        CHECK(gSideTimers[targetSide].spikesAmount < 3)

        gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
        return TRUE;
    },
};

constexpr Ability RapidResponse = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
    },
};

constexpr Ability DoubleIronBarbs = {
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

constexpr Ability ThermalExchange = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanRaiseStat(battler, STAT_ATK))

        SetStatChanger(STAT_ATK, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BURN)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability GoodAsGold = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
};

constexpr Ability SharingIsCaring = {
    .onReactive = +[](ON_REACTIVE) -> int {
        switch (gBattleStruct->statStageCheckState) {
            default:
                return FALSE;

            case STAT_STAGE_CHECK_IN_PROGRESS:
                SetAbilityStateAs(battler, ability, (AbilityStates){.statCopyState = (StatCopyState){.inProgress = TRUE}});
                return FALSE;

            case STAT_STAGE_CHECK_NEEDED:
                InsertCorrectEndType(callType);
                BattleScriptCall(BattleScript_PerformCopyStatEffects);
                gBattleStruct->statStageCheckState = STAT_STAGE_CHECK_IN_PROGRESS;
                SetAbilityStateAs(battler, ability, (AbilityStates){.statCopyState = (StatCopyState){.inProgress = TRUE}});
                return TRUE;
        }
    },
};

constexpr Ability TabletsOfRuin = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_ATK, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_ATK,
};

constexpr Ability SwordOfRuin = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_DEF, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
};

constexpr Ability VesselOfRuin = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_SPATK, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_SPATK,
};

constexpr Ability BeadsOfRuin = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_DEF, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
};

constexpr Ability PermafrostClone = {
    .onDefensiveMultiplier = Permafrost.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability Gallantry = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
        return TRUE;
    },
    .noDamageHits = 1,
    .breakable = TRUE,
    .persistent = TRUE,
};

constexpr Ability OrichalcumPulse = {
    .onEntry = Drought.onEntry,
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
        },
};

constexpr Ability SunBasking = {
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

constexpr Ability WingedKing = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
        },
};

constexpr Ability HadronEngine = {
    .onEntry = ElectricSurge.onEntry,
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
        },
    .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
};

constexpr Ability IronSerpent = {
    .onOffensiveMultiplier = WingedKing.onOffensiveMultiplier,
};

constexpr Ability SweepingEdgePlus = {
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    .onAccuracy = SweepingEdge.onAccuracy,
};

constexpr Ability CelestialBlessing = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_MISTY_TERRAIN))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 12;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    },
};

constexpr Ability MinionControl = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType { return PARENTAL_BOND_MINION_CONTROL; },
};

constexpr Ability MoltenBlades = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
};

constexpr Ability HauntingFrenzy = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    },
    .onBattlerFaints = AdrenalineRush.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability NoiseCancel = {
    .onImmune = Soundproof.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

constexpr Ability RadioJam = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeDisabled(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
    },
};

constexpr Ability Ole = {
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

constexpr Ability Malicious = {
    .onEntry = UseIntimidateClone,
};

constexpr Ability DeadPower = {
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

constexpr Ability BrawlingWyvern = {
    .onAccuracy = NoGuard.onAccuracy,
    .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
};

constexpr Ability MythicalArrows = {
    .onOffensiveMultiplier = Archer.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].arrowBased);
        return TRUE;
    },
};

constexpr Ability Lawnmower = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
        return TRUE;
    },
};

constexpr Ability Flourish = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
        },
};

constexpr Ability DesertSpirit = {
    .onEntry = SandStream.onEntry,
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
                *mod = UQ_4_12(1.0);
            }
        },
    .sandImmune = TRUE,
};

constexpr Ability Contempt = {
    .unaware = TRUE,
};

constexpr Ability Aerialist = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Levitate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Flock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .breakable = TRUE,
    .levitate = TRUE,
};

constexpr Ability TeraShell = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability ToxicChain = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
};

constexpr Ability ParasiticSpores = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

        gVolatileStructs[battler].parasiticSpores = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
    },
};

constexpr Ability PoisonPuppeteer = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
    },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int state = GetAbilityState(battler, ability);
        if (state & (1 << fainted)) SetAbilityState(battler, ability, state ^ (1 << fainted));
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_POISON,
};

constexpr Ability Entrance = {
    .onReactive = +[](ON_REACTIVE) -> int { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_CONFUSION,
};

constexpr Ability Rejection = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldTimers.quashTimer)

        gFieldTimers.quashTimer = QUASH_DURATION;
        gFieldTimers.started.quash = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
    },
};

constexpr Ability AppleEnlightenment = {
    .onDefensiveMultiplier = FurCoat.onDefensiveMultiplier,
    .breakable = TRUE,
    .magicGuard = TRUE,
};

constexpr Ability BalloonBomber = {
    .onDefender = +[](ON_DEFENDER) -> int { return Aftermath.onDefender(DELEGATE_DEFENDER) || Inflatable.onDefender(DELEGATE_DEFENDER); },
};

constexpr Ability FlamingMaw = {
    .onAttacker = FlamingJaws.onAttacker,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
};

constexpr Ability Demolitionist = {
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

constexpr Ability RockhardWill = {
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
constexpr Ability FragrantDaze = {
    ON_EITHER_ABILITY(FragrantDaze),
};

constexpr Ability LowVisibility = {
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

constexpr Ability OldMariner = {
    .onOffensiveMultiplier = Seaweed.onOffensiveMultiplier,
    .onDefensiveMultiplier = Seaweed.onDefensiveMultiplier,
    .onStab = Amphibious.onStab,
    .breakable = TRUE,
};

constexpr Ability Ectoplasm = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        },
};

constexpr Ability BeautifulMusic = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 2)
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    },
    .canInfatuateAny = TRUE,
};

constexpr Ability SnowSong = {
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_ICE + 1;
    },
};

constexpr Ability GreaterSpirit = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

constexpr Ability Resonance = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 50)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

constexpr Ability EtherealRush = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        },
};

constexpr Ability CuteAntecedence = {
    .onPriority = GALE_WINGS_CLONE(TYPE_FAIRY),
};

constexpr Ability RecurringNightmare = {
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
constexpr Ability MenacingSituation = {
    ON_EITHER_ABILITY(MenacingSituation),
};

constexpr Ability ShinyLightning = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

constexpr Ability Terrify = {
    .onEntry = UseIntimidateClone,
};

constexpr Ability IceDownfall = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
        return FALSE;
    },
};

constexpr Ability LastStand = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF || statId == STAT_SPDEF)
                *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
        },
    .breakable = TRUE,
};

constexpr Ability PyroclasticFlow = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        return MoltenDown.onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS) || Corrosion.onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS);
    },
    .onCanStatusType = Corrosion.onCanStatusType,
};

constexpr Ability BloodBath = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BLEED)
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_BLEED,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability BattleAura = {
    .onCrit = +[](ON_CRIT) -> int { return 2; },
    .onCritFor = APPLY_ON_ANY,
};

constexpr Ability Bloodlust = {
    .onReactive = BloodBath.onReactive,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int result = 0;
        if (battler == attacker) {
            result |= SoulEater.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        return result | BloodBath.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    },
    .onStatusImmune = BloodBath.onStatusImmune,
    .onBattlerFaintsFor = APPLY_ON_ANY,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability PiercingSolo = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

constexpr Ability Rhythmic = {
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); },
};

constexpr Ability ChunkyBassLine = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsSoundMove(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
    },
};

constexpr Ability DualHammer = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].hammerBased)
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

constexpr Ability DentingBlows = {
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

constexpr Ability IceColdHunter = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(moveType == TYPE_ICE)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
        return PARENTAL_BOND_ICE_COLD_HUNTER;
    },
    .hailImmune = TRUE,
};

constexpr Ability SoulCrusher = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hammerBased) MUL(1.1);
        },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(gBattleMoves[move].hammerBased)
        return STAT_SPDEF;
    },
};

constexpr Ability ArcFlash = {
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

constexpr Ability Unicorn = {
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    ATE_ABILITY(TYPE_FAIRY),
};

constexpr Ability OnTheProwl = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
    },
};

constexpr Ability Pretentious = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(gVolatileStructs[battler].critBoost < 3);
        gVolatileStructs[battler].critBoost++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
        BattleScriptCall(BattleScript_AbilityBoostsCrit);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability VenoblazePincers = {
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
                CHECK(CanBePoisoned(battler, target, MOVE_NONE))
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

constexpr Ability EternalBlessing = {
    .onEndTurn = CelestialBlessing.onEndTurn,
    .onExit = Regenerator.onExit,
    .persistent = TRUE,
};

constexpr Ability SugarRush = {.onStat = Unburden.onStat};

constexpr Ability PeacefulRest = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    },
};

constexpr Ability WhiteNoise = {
    .onEndTurn = PeacefulRest.onEndTurn,
    .onAttacker = Static.onAttacker,
    .onDefender = Static.onDefender,
};

constexpr Ability SmokeyManeuvers = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability PowerMetal = {
    .onOffensiveMultiplier = LiquidVoice.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_STEEL + 1;
    },
};

constexpr Ability PowerEdge = {
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
};

constexpr Ability Superconductor = {
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

constexpr Ability UltraInstinct = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
        return FALSE;
    },
    .onDefensiveMultiplier = Parry.onDefensiveMultiplier,
};

constexpr Ability UnlockedPotential = {
    .onDefender = Berserk.onDefender,
    .onAccuracy = InnerFocus.onAccuracy,
    .tauntImmune = TRUE,
};

constexpr Ability HigherRank = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
        },
};

constexpr Ability FuneralPyre = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE); },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GHOST) || IS_BATTLER_OF_TYPE(target, TYPE_DARK))
            FILTER_NOT(IsMagicGuardProtected(target))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_FuneralPyreDamage);
            any = TRUE;
        }
        return any;
    },
};

constexpr Ability FlameBubble = {
    .onOffensiveMultiplier = WaterBubble.onOffensiveMultiplier,
    .onDefensiveMultiplier = WaterBubble.onDefensiveMultiplier,
    .onPriority = FlamingSoul.onPriority,
    .onStatusImmune = WaterBubble.onStatusImmune,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability ElementalVortex = {
    .onAbsorb = +[](ON_ABSORB) -> int { return WaterAbsorb.onAbsorb(DELEGATE_ABSORB) || FlashFire.onAbsorb(DELEGATE_ABSORB); },
    .onOffensiveMultiplier = FlashFire.onOffensiveMultiplier,
};

constexpr Ability SnowyWrath = {
    .onEntry = SnowWarning.onEntry,
    .onModifyEffectChance = Cryomancy.onModifyEffectChance,
};

constexpr Ability PatternChange = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
    .onBeforeAttack = Protean.onBeforeAttack,
};

constexpr Ability NoTurningBack = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_NoTurningBack);
        return TRUE;
    },
};

constexpr Ability FlammableCoat = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler) || (gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE))
        CHECK(moveType == TYPE_FIRE)
        CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[battler].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_TargetFormChange);
        return TRUE;
    },
    .onBeforeAttack = +[](ABILITY_ON_BEFORE_ATTACK) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        UpdateAbilityStateIndicesForNewSpecies(gBattlerAttacker, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[gBattlerAttacker].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability DracoMorale = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DRAGON_CHEER, 0); },
};

constexpr Ability BadOmen = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (isCrit) MUL(.25);
        },
    .breakable = TRUE,
};

constexpr Ability MoshPit = {
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
    CHECK_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(opponent)))
    CHECK_NOT(HasAbilityIgnoringSuppression(opponent, ability))
    CHECK_NOT(DoesBattlerHaveAbilityShield(opponent))

    UpdateAbilityStateIndicesForNewAbility(opponent, ability);
    ReplaceAbility(opponent, ability);
    gStackBattler1 = opponent;
    BattleScriptCall(BattleScript_BloodStainActivates);
    DisableSwitchInAbility(opponent, ability);
    return TRUE;
}
constexpr Ability BloodStain = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN); },
    ON_EITHER_ABILITY(BloodStain),
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability BloodStigma = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
        },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability Slipstream = {
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[STAT_SPEED] += 20; },
};

constexpr Ability MaximumAcceleration = {
    .onEndTurn = SpeedBoost.onEndTurn,
    .onChooseOffensiveStat = Slipstream.onChooseOffensiveStat,
};

constexpr Ability Sidewinder = {
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

constexpr Ability Petrify = {
    .onEntry = +[](ON_ENTRY) -> int {
        int loweredStats = 0;
        int intimidated = UseIntimidateClone(ability, battler);
        for (int i = GetOppositeSide(battler); i < gBattlersCount; i += 2) {
            FILTER(IsBattlerAlive(i))
            loweredStats |= TryResetBattlerStatChanges(i, RESET_STAT_BUFFS);
        }

        if (loweredStats) {
            BattleScriptPushCursorAndCallback(BattleScript_Petrify);
        }
        return intimidated || loweredStats;
    },
};

constexpr Ability Fluffiest = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

constexpr Ability WayOfPrecision = {
    .onAccuracy = InnerFocus.onAccuracy,
    .onCrit = PreciseFist.onCrit,
    .onModifyEffectChance = PreciseFist.onModifyEffectChance,
    .breakable = TRUE,
    .tauntImmune = TRUE,
};

constexpr Ability WayOfSwiftness = {
    .onBattlerFaints = Pretentious.onBattlerFaints,
    .onStat = SwiftSwim.onStat,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability AtomicPunch = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            IronFist.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            SteelySpirit.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

constexpr Ability IronGiant = {
    .onDefensiveMultiplier = Heatproof.onDefensiveMultiplier,
    .onChooseOffensiveStat = Juggernaut.onChooseOffensiveStat,
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
    .breakable = TRUE,
    .negatesBurnAtkDrop = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability MasterHand = {
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .megaLauncherBoost = TRUE,
};

constexpr Ability FinalBlow = {
    .onAccuracy = FatalPrecision.onAccuracy,
    .onCrit = FatalPrecision.onCrit,
};

constexpr Ability Hospitality = {
    .onEntry = +[](ON_ENTRY) -> int {
        gBattlerTarget = BATTLE_PARTNER(battler);
        CHECK(IsBattlerAlive(gBattlerTarget))
        CHECK_NOT(BATTLER_MAX_HP(gBattlerTarget))

        gBattleMoveDamage = -gBattleMons[gBattlerTarget].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptPushCursorAndCallback(BattleScript_Hospitality_AfterPopup);
        return TRUE;
    },
};

constexpr Ability ButterUp = {
    .onEntry = +[](ON_ENTRY) -> int { return Hospitality.onEntry(DELEGATE_ENTRY) | SoothingAroma.onEntry(DELEGATE_ENTRY); },
};

constexpr Ability VitalityStrike = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(IsIronFistBoosted(battler, move))

        gBattleMoveDamage = -gHpDealt / 10;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

constexpr Ability HugeWings = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            GiantWings.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Levitate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .breakable = TRUE,
    .levitate = TRUE,
};

constexpr Ability SwordOfDamnation = {
    .onStat = SwordOfRuin.onStat,
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
    .unaware = TRUE,
};

constexpr Ability RestrainingOrder = {
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

constexpr Ability AssassinsTools = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IsMoveMakingContact(move, battler))

        switch (Random() % 3) {
            case 0:
                CHECK(CanBePoisoned(battler, target, MOVE_NONE));
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

constexpr Ability Frostmaw = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanGetFrostbite(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
    },
};

constexpr Ability Patchwork = {
    .onEntry = Disguise.onEntry,
    .onDisguise = +[](ON_DISGUISE) -> SpeciesEnum {
        SpeciesEnum species = Disguise.onDisguise(DELEGATE_DISGUISE);
        if (species && !testOnly) {
            SetOncePerTurnAbilityCounter(battler, ABILITY_PATCHWORK, gBattlerAttacker + 1);
        }
        return species;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        int triggeringBattler = GetOncePerTurnAbilityCounter(battler, ability) - 1;
        CHECK(triggeringBattler == attacker)
        SetOncePerTurnAbilityCounter(battler, ability, 0);

        CHECK(IsBattlerAlive(attacker))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)

        AbilityStatusEffect(MOVE_EFFECT_CURSE | MOVE_EFFECT_AFFECTS_USER);
        return TRUE;
    },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability BlindRage = {
    .onEntry = MoldBreaker.onEntry,
    .onTypeEffectiveness = Scrappy.onTypeEffectiveness,
    .tauntImmune = TRUE,
};

constexpr Ability ApexPredator = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onOffensiveMultiplier = ToughClaws.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability DragonsRitual = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
        BattleScriptCall(BattleScript_DragonsRitual);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability PinnacleBlade = {
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

constexpr Ability Energized = {
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

constexpr Ability ColorSpectrum = {
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

constexpr Ability SteelBeetle = {
    .onParentalBond = RagingBoxer.onParentalBond,
    .onOffensiveMultiplier = Pollinate.onOffensiveMultiplier,
    .onMoveType = Pollinate.onMoveType,
};

constexpr Ability FromTheShadows = {
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

constexpr Ability RagePoint = {
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
    .negatesBurnAtkDrop = TRUE,
    .negatesFrzSpatkDrop = TRUE,
};

constexpr Ability HotCoals = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

        gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
    },
};

constexpr Ability TerastalTreasure = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.6); },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= .8;
        },
    .breakable = TRUE,
};

constexpr Ability ShockingMaw = {
    .onAttacker = ShockingJaws.onAttacker,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
};

constexpr Ability GleamEyes = {
    .onEntry = +[](ON_ENTRY) -> int { return UseIntimidateClone(ability, battler) | Frisk.onEntry(DELEGATE_ENTRY); },
};

constexpr Ability RousedFangs = {
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    .onChooseOffensiveStat = MindCrush.onChooseOffensiveStat,
};

constexpr Ability DreamState = {
    .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier,
    .onCrit = BattleArmor.onCrit,
    .onCritFor = BattleArmor.onCritFor,
    .breakable = TRUE,
};

constexpr Ability DreamWhimsy = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_YAWN, 0); },
};

constexpr Ability LunarAffinity = {
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(gBattleMoves[move].lunar)
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
};

constexpr Ability FlameShield = {
    .onDefensiveMultiplier = Filter.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability AquaticDweller = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) MUL(1.5);
        },
};

constexpr Ability ApplePie = {
    .onEndTurn = SelfSufficient.onEndTurn,
};

constexpr Ability Hover = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_PSYCHIC); },
    .breakable = TRUE,
    .levitate = TRUE,
};

constexpr Ability Depravity = {
    .onCrit = Merciless.onCrit,
    .onTypeEffectiveness = Overcharge.onTypeEffectiveness,
    .onCanStatusType = Overcharge.onCanStatusType,
};

constexpr Ability Wildfire = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FIRE_SPIN, 0); },
};

constexpr Ability JumpScare = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
    },
    .persistent = TRUE,
};

constexpr Ability TarToss = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TAR_SHOT, 0); },
};

constexpr Ability StunShock = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target)) CHECK(Random() % 100 < 60) switch (Random() % 2) {
            case 0:
                CHECK(CanBePoisoned(battler, target, MOVE_NONE));
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

constexpr Ability RagingGoddess = {
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onParentalBond = ParentalBond.onParentalBond,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Whiplash = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
};

constexpr Ability SupersweetSyrup = {
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

constexpr Ability TrashHeap = {
    .onEntry = ToxicSpill.onEntry,
    .onEndTurn = ToxicSpill.onEndTurn,
    .onExit = ToxicSpill.onExit,
    .onTypeEffectiveness = Corrosion.onTypeEffectiveness,
    .onCanStatusType = Corrosion.onCanStatusType,
};

constexpr Ability SludgyMix = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Intoxicate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            PunkRock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onMoveType = Intoxicate.onMoveType,
};

constexpr Ability Overwatch = {
    .onEntry = OnTheProwl.onEntry,
    .onOffensiveMultiplier = Stakeout.onOffensiveMultiplier,
};

constexpr Ability WindRage = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); },
    .onOffensiveMultiplier = GiantWings.onOffensiveMultiplier,
};

constexpr Ability VictoryBomb = {
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

constexpr Ability RazorSharp = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gIsCriticalHit)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

constexpr Ability ToTheBone = {
    .onAttacker = RazorSharp.onAttacker,
    .onOffensiveMultiplier = Sniper.onOffensiveMultiplier,
};

constexpr Ability BladeDance = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
    },
};

int ApeShiftHandler(int battler, AbilityCallType callType) {
    CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
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
constexpr Ability ApeShift = {
    .onEntry = +[](ON_ENTRY) -> int { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onEndTurn = +[](ON_END_TURN) -> int { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDefender = +[](ON_DEFENDER) -> int { return ApeShiftHandler(battler, ABILITY_BS_CALL); },
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
        return ALWAYS_CRIT;
    },
    .randomizerBanned = TRUE,
};

constexpr Ability KnowYourPlace = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gVolatileStructs[target].dazed)
        CHECK(IsMoveMakingContact(move, battler))

        gVolatileStructs[target].dazed = 5;
        BattleScriptCall(BattleScript_TargetDazed);
        return TRUE;
    },
};

constexpr Ability DeepCuts = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

constexpr Ability LifeSteal = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        int any = FALSE;
        for (int target = GetOppositeSide(battler); target < gBattlersCount; target += 2) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IsMagicGuardProtected(target))

            gStackBattler1 = battler;
            gStackBattler2 = target;
            gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_IGNORE_DISGUISE;
            BattleScriptExecute(BattleScript_AbilityDrainsHp);
            any = TRUE;
        }
        return any;
    },
};

constexpr Ability RudeAwakening = {
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        CHECK(GetAbilityState(battler, ability))
        return TRUE;
    },
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability TeraformZero = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(!GetSingleUseAbilityCounter(battler, ability));
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
        BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
        return TRUE;
    },
    .onAfterTypeEffectiveness = TeraShell.onAfterTypeEffectiveness,
    .onAfterTypeEffectivenessFor = TeraShell.onAfterTypeEffectivenessFor,
    .breakable = TRUE,
};

constexpr Ability SetAblaze = {
    .onReactive = BloodBath.onReactive,
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_BURN,
};

constexpr Ability Breakwater = {
    .onDefensiveMultiplier = Stall.onDefensiveMultiplier,
    .onStat = SwiftSwim.onStat,
    .breakable = TRUE,
};

constexpr Ability MagicalFists = {
    .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
        },
};

constexpr Ability Cutthroat = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
    },
};

constexpr Ability SandBender = {
    .onEntry = SandStream.onEntry,
    .onStat = SandForce.onStat,
    .sandImmune = TRUE,
};

constexpr Ability SandPit = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SAND_TOMB, 20); },
};

constexpr Ability DesolateSun = {
    .randomizerBanned = TRUE,
};

ON_EITHER(Daybreak) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(CanBeBurned(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))

    AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
    return TRUE;
}
constexpr Ability Daybreak = {
    ON_EITHER_ABILITY(Daybreak),
};

constexpr Ability EnergySiphon = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

constexpr Ability Reservoir = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT | ABSORB_RESULT_HEAL;
    },
    .redirectType = TYPE_WATER,
    .breakable = TRUE,
};

static int NeurotoxinCondition(int battler, int target) {
    return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
}
constexpr Ability Neurotoxin = {
    .onReactive = +[](ON_REACTIVE) -> int { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_POISON,
};

constexpr Ability EnergizedHorns = {
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].hornBased);
        return TRUE;
    },
};

constexpr Ability SpiderLairUpgrade = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

        int side = GetOppositeSide(battler);
        gSideTimers[side].started.spiderWeb = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[side].stickyWebTimer = 7;
        BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
        return TRUE;
    },
};

constexpr Ability CrustCoat = {
    .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier,
    .onCrit = BattleArmor.onCrit,
    .onCritFor = BattleArmor.onCritFor,
    .breakable = TRUE,
};

constexpr Ability Puffy = {
    .onDefensiveMultiplier = Fluffy.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability BalloonBlitz = {
    .onDefender = Inflatable.onDefender,
    .onParentalBond = ParentalBond.onParentalBond,
};

constexpr Ability StrikerPixilate = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Striker.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Pixilate.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onMoveType = Pixilate.onMoveType,
};

// 2.6
constexpr Ability DoomBlast = {
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

constexpr Ability Bruteforce = {
    .onOffensiveMultiplier = Reckless.onOffensiveMultiplier,
    .onStatusImmune = RockHead.onStatusImmune,
    .noRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability FaradayCage = {
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

constexpr Ability AcidicSlime = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
    .onTypeEffectiveness = Corrosion.onTypeEffectiveness,
    .onCanStatusType = Corrosion.onCanStatusType,
};

constexpr Ability RoseGarden = {
    .onEntry = +[](ON_ENTRY) -> int {
        u8 targetSide = GetOppositeSide(battler);
        CHECK(gSideTimers[targetSide].toxicSpikesAmount < 2)

        gSideTimers[targetSide].toxicSpikesAmount = 2;
        gSideStatuses[targetSide] |= SIDE_STATUS_TOXIC_SPIKES;
        gBattlerTarget = targetSide;
        BattleScriptPushCursorAndCallback(BattleScript_RoseGarden);
        return TRUE;
    },
};

constexpr Ability Qigong = {
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onOffensiveMultiplier = FightingSpirit.onOffensiveMultiplier,
    .onMoveType = FightingSpirit.onMoveType,
    .onAccuracy = +[](ON_ACCURACY) { return ACCURACY_ALWAYS_HITS; },
    .onBattlerFaintsFor = Rampage.onBattlerFaintsFor,
};

constexpr Ability ConjurerOfDeceit = {
    .breakable = TRUE,
    .magicGuard = TRUE,
    .magicBounce = TRUE,
};

constexpr Ability DeepFreeze = {
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

constexpr Ability SoulDevourer = {
    .onBattlerFaints = SoulEater.onBattlerFaints,
    .onTypeEffectiveness = PhantomPain.onTypeEffectiveness,
    .onBattlerFaintsFor = SoulEater.onBattlerFaintsFor,
};

constexpr Ability ChampionsEntrance = {
    .onEntry = +[](ON_ENTRY) -> int { return Intimidate.onEntry(DELEGATE_ENTRY) | ViolentRush.onEntry(DELEGATE_ENTRY); },
};

constexpr Ability Presto = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(BATTLER_MAX_HP(battler))
        CHECK(IsSoundMove(battler, move))
        return 1;
    },
};

constexpr Ability Samba = {
    .onOffensiveMultiplier = Striker.onOffensiveMultiplier,
    .onCopyMove = Dancer.onCopyMove,
};

constexpr Ability Gladiator = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIGHTING),
};

constexpr Ability ForsakenHeart = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
};

constexpr Ability Relentless = {
    .onOffensiveMultiplier = ExploitWeakness.onOffensiveMultiplier,
    .onChooseDefensiveStat = ExploitWeakness.onChooseDefensiveStat,
    .onCrit = Merciless.onCrit,
};

constexpr Ability Soothsayer = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(!GetSingleUseAbilityCounter(battler, ability))
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        SetAbilityState(battler, ability, 3);
        return SwitchInAnnounce(B_MSG_SWITCHIN_SOOTHSAYER);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        int counter = GetAbilityState(battler, ability);
        if (counter) SetAbilityState(battler, ability, counter - 1);
        return FALSE;
    },
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (!GetAbilityState(battler, ability)) return;
            if (*mod >= UQ_4_12(1.0)) *mod = UQ_4_12(0.5);
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .persistent = TRUE,
};

constexpr Ability CorruptedMind = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_PSYCHIC)
        if (*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        return FALSE;
    },
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            int type;
            GET_MOVE_TYPE(move, type)
            if (type == TYPE_PSYCHIC) *effectChance *= 1.4;
        },
    .randomizerBanned = TRUE,
};

constexpr Ability FlameCoat = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_FIRE_COAT); },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IS_BATTLER_OF_TYPE(target, TYPE_FIRE))
            FILTER_NOT(IsMagicGuardProtected(target))
            FILTER_NOT(BATTLER_HAS_ABILITY(target, ABILITY_FLARE_BOOST))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_FireCoatDamage);
            any = TRUE;
        }
        return any;
    },
};

constexpr Ability UnownPower = {
    .onStab = +[](ON_STAB) -> int { return TRUE; },
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
        },
    .randomizerBanned = TRUE,
};

constexpr Ability SuperScope = {
    .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
    .onAccuracy = Artillery.onAccuracy,
    .megaLauncherBoost = TRUE,
};

constexpr Ability VenomCrown = {
    ON_EITHER_ABILITY(PoisonPoint),
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    .randomizerBanned = TRUE,
};

constexpr Ability BlightScale = {
    ON_EITHER_ABILITY(PoisonPoint),
    .onDefensiveMultiplier = Multiscale.onDefensiveMultiplier,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability Gunman = {
    .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
    .megaLauncherBoost = TRUE,
};

constexpr Ability Caretaker = {
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

constexpr Ability PoseidonsDominion = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_WHIRLPOOL, 0); },
};

constexpr Ability DualShadow = {
    .onEndTurn = HungerSwitch.onEndTurn,
    .onRecoil = +[](ON_RECOIL) -> int {
        CHECK(moveType == TYPE_ELECTRIC || moveType == TYPE_DARK);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 10, 1);
    },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC || moveType == TYPE_DARK) MUL(1.35);
        },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability Lullaby = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_SING);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    },
};

constexpr Ability CryoArchitect = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        int abilityState = GetAbilityState(battler, ability);
        CHECK(abilityState)

        int activate = abilityState & 1;
        SetAbilityState(battler, ability, abilityState >> 1);

        CHECK(activate)
        CHECK(CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_LESS_THAN))

        SetStatChanger(STAT_DEF, 1);
        gBattleScripting.battler = battler;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_WATER || moveType == TYPE_ICE)

        int any = FALSE;

        if (CompareStat(battler, STAT_DEF, MAX_STAT_STAGE, CMP_LESS_THAN)) {
            if (moveType == TYPE_WATER) {
                int abilityState = GetAbilityState(battler, ability);
                abilityState |= 1 << 1;
                SetAbilityState(battler, ability, abilityState);
            } else {
                SetStatChanger(STAT_DEF, 1);
                BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
                any = TRUE;
            }
        }

        if (CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN)) {
            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            any = TRUE;
        }
        return any;
    },
};

constexpr Ability GlacialRage = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_ICE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
    },
};

constexpr Ability ImmovableObject = {
    .magicGuard = TRUE,
};

constexpr Ability FrenziedPhantom = {
    .onParentalBond = ParentalBond.onParentalBond,
    .onTrap = ShadowTag.onTrap,
    .shadowTag = TRUE,
};

constexpr Ability DNAScramble = {
    .onBeforeAttack = +[](ABILITY_ON_BEFORE_ATTACK) -> int {
        SpeciesEnum newSpecies = SPECIES_NONE;
        switch (gBattleMons[battler].species) {
            default:
                return FALSE;
            case SPECIES_DEOXYS:
                if (gBattleMoves[move].power > 0)
                    newSpecies = SPECIES_DEOXYS_ATTACK;
                else if (move == MOVE_RECOVER)
                    newSpecies = SPECIES_DEOXYS_DEFENSE;
                else if (gBattleMoves[move].split == SPLIT_STATUS)
                    newSpecies = SPECIES_DEOXYS_SPEED;
                break;
            case SPECIES_DEOXYS_ATTACK:
                if (move == MOVE_RECOVER)
                    newSpecies = SPECIES_DEOXYS_DEFENSE;
                else if (gBattleMoves[move].split == SPLIT_STATUS)
                    newSpecies = SPECIES_DEOXYS_SPEED;
                break;
            case SPECIES_DEOXYS_DEFENSE:
                if (gBattleMoves[move].power > 0)
                    newSpecies = SPECIES_DEOXYS_ATTACK;
                else if (move != MOVE_RECOVER && gBattleMoves[move].split == SPLIT_STATUS)
                    newSpecies = SPECIES_DEOXYS_SPEED;
                break;
            case SPECIES_DEOXYS_SPEED:
                if (gBattleMoves[move].power > 0)
                    newSpecies = SPECIES_DEOXYS_ATTACK;
                else if (move == MOVE_RECOVER)
                    newSpecies = SPECIES_DEOXYS_DEFENSE;
                break;
        }
        CHECK(newSpecies)

        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

constexpr Ability MetallicJaws = {
    .onEntry = Metallic.onEntry,
    .onParentalBond = PrimalMaw.onParentalBond,
};

constexpr Ability Calculative = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Analytic.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Neuroforce.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

constexpr Ability EmbodyAspect = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

constexpr Ability EmbodyAspectHearthflame = {
    .onEntry = IntrepidSword.onEntry,
};

constexpr Ability EmbodyAspectCornerstone = {
    .onEntry = DauntlessShield.onEntry,
};

constexpr Ability EmbodyAspectWellspring = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPDEF))

        SetStatChanger(STAT_SPDEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

constexpr Ability RockhardShaft = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_ROCK),
};

constexpr Ability HuntersMark = {
    .onAccuracy = Deadeye.onAccuracy,
    .onChooseDefensiveStat = Deadeye.onChooseDefensiveStat,
    .onCrit = Ambush.onCrit,
};

constexpr Ability Deviate = {
    ATE_ABILITY(TYPE_DARK),
};

constexpr Ability SunsBounty = {
    .onEndTurn = +[](ON_END_TURN) -> int { return Harvest.onEndTurn(DELEGATE_END_TURN) | LeafGuard.onEndTurn(DELEGATE_END_TURN); },
};

constexpr Ability RiteOfSpring = {
    .onStat =
        +[](ON_STAT) {
            SolarPower.onStat(DELEGATE_STAT);
            Chlorophyll.onStat(DELEGATE_STAT);
        },
};

constexpr Ability Headstrong = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPDEF))

        SetStatChanger(STAT_SPDEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
    .breakable = TRUE,
};

constexpr Ability Firefighter = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(1.5);
        },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_FIRE)) MUL(.5);
        },
    .breakable = TRUE,
};

constexpr Ability SepiaLens = {
    .onImmune = SandGuard.onImmune,
    .onOffensiveMultiplier = TintedLens.onOffensiveMultiplier,
    .onDefensiveMultiplier = SandGuard.onDefensiveMultiplier,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

constexpr Ability SuperSniper = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Sniper.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
                MUL(0.5);
            }
        },
    .onPreemptAction = +[](ON_PREEMPT_ACTION) -> int {
        CHECK(gCurrentActionFuncId == B_ACTION_SWITCH)
        CHECK(gActionsByTurnOrder[GetBattlerTurnOrderNum(battler)] == B_ACTION_USE_MOVE)

        MoveEnum move = GetChosenMove(battler);
        int targetFlag = GetBattlerBattleMoveTargetFlags(move, battler);

        switch (targetFlag) {
            case MOVE_TARGET_SELECTED:
                CHECK(gBattleStruct->moveTarget[battler] == turnBattler)
                break;

            case MOVE_TARGET_BOTH:
            case MOVE_TARGET_FOES_AND_ALLY:
                break;

            case MOVE_TARGET_RANDOM:
            default:
                return FALSE;
        }

        gQueuedExtraAttackData[++gQueuedAttackCount] = (ExtraAttackActionStruct){
            .ability = ability,
            .move = move,
            .attacker = battler,
            .target = turnBattler,
            .movePos = gBattleStruct->chosenMovePositions[battler],
        };
        gActionsByTurnOrder[GetBattlerTurnOrderNum(battler)] = B_ACTION_FINISHED;
        return TRUE;
    },
};

ON_EITHER(WoodlandCurse) {
    CHECK(ShouldApplyOnHitAffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK_NOT(IS_BATTLER_OF_TYPE(opponent, TYPE_GRASS))

    gBattleMons[opponent].type3 = TYPE_GRASS;
    PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[opponent].type3);
    gStackBattler1 = opponent;
    BattleScriptCall(BattleScript_StackBecameTheTypeFull);
    return TRUE;
}
constexpr Ability WoodlandCurse = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FORESTS_CURSE, 0); },
    ON_EITHER_ABILITY(WoodlandCurse),
};

constexpr Ability Malodor = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_GASTRO_ACID)

        gStatuses3[attacker] |= STATUS3_GASTRO_ACID;
        BattleScriptCall(BattleScript_StackAbilitySuppressedMessage);
        return TRUE;
    },
};

constexpr Ability Blur = {
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        return STAT_SPEED;
    },
    .onChooseDefensiveStatFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability Elude = {
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker))
        return STAT_SPEED;
    },
    .onChooseDefensiveStatFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

constexpr Ability DrakeOfRage = {
    .onBattlerFaints = Rampage.onBattlerFaints,
    .onOffensiveMultiplier = TintedLens.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability MixedMartialArts = {
    .breakable = TRUE,
};

constexpr Ability StrategicPause = {
    .onOffensiveMultiplier = Analytic.onOffensiveMultiplier,
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber)
        CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT)
        return 2;
    },
};

constexpr Ability Overrule = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        },
};

constexpr Ability MentalPollution = {
    .breakable = TRUE,
};

constexpr Ability MadnessEnhancement = {
    .breakable = TRUE,
};

constexpr Ability Tentalock = {
    .breakable = TRUE,
};

constexpr Ability SerpentBind = {
    .breakable = TRUE,
};

constexpr Ability SoulTap = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
        int any = FALSE;
        for (int target = GetOppositeSide(battler); target < gBattlersCount; target += 2) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IsMagicGuardProtected(target))

            gStackBattler1 = battler;
            gStackBattler2 = target;
            gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_IGNORE_DISGUISE;
            BattleScriptExecute(BattleScript_AbilityDrainsHp);
            any = TRUE;
        }
        return any;
    },
};

constexpr Ability Scarecrow = {
    .onEntry = UseIntimidateClone,
    .onCrit = BadLuck.onCrit,
    .onModifyEffectChance = BadLuck.onModifyEffectChance,
    .onCritFor = BadLuck.onCritFor,
    .onModifyEffectChanceFor = BadLuck.onModifyEffectChanceFor,
    .breakable = TRUE,
};

constexpr Ability OminousShroud = {
    .onEntry = Phantom.onEntry,
    .onDefensiveMultiplier = ShadowShield.onDefensiveMultiplier,
};

constexpr Ability ChillingPresence = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_ICY_WIND, 10); },
};

constexpr Ability Frostbind = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) { return (int)CanGetFrostbite(battler); }, BattleScript_Frostbind);
    },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_FROSTBITE,
};

constexpr Ability TenderAffection = {
    ON_EITHER_ABILITY(CuteCharm),
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FAIRY; },
};

constexpr Ability GlacialGhost = {
    .onStat = SlushRush.onStat,
    .onAccuracy = SnowCloak.onAccuracy,
    .onAccuracyFor = SnowCloak.onAccuracyFor,
    .breakable = TRUE,
    .hailImmune = TRUE,
};

constexpr Ability WonderScale = {
    .onEndTurn = ShedSkin.onEndTurn,
    .fortKnox = TRUE,
};

constexpr Ability Overzealous = {
    .breakable = TRUE,
};

constexpr Ability StainlessSteel = {
    ATE_ABILITY(TYPE_STEEL),
    .fortKnox = TRUE,
};

constexpr Ability TemporalRupture = {
    .breakable = TRUE,
};

constexpr Ability GrassFlute = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IsSoundMove(battler, move))
        CHECK_NOT(gVolatileStructs[target].fear)

        return AbilityStatusEffect(MOVE_EFFECT_FEAR);
    },
};

constexpr Ability Hemotoxin = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(
            ability,
            battler,
            [](int battler, int target) -> int { return !(gStatuses3[target] & STATUS3_GASTRO_ACID); },
            BattleScript_StackAbilitySuppressedMessage);
    },
    .onBattlerFaints = PoisonPuppeteer.onBattlerFaints,
    .onBattlerFaintsFor = PoisonPuppeteer.onBattlerFaintsFor,
    .setStateOnEffect = MOVE_EFFECT_POISON,
};

constexpr Ability Harukaze = {
    .breakable = TRUE,
};

constexpr Ability ToxicSurge = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_TOXIC,
};

constexpr Ability PoisonQuills = {
    .onAttacker = PoisonPoint.onAttacker,
    .onDefender = +[](ON_DEFENDER) -> int { return RoughSkin.onDefender(DELEGATE_DEFENDER) | PoisonPoint.onDefender(DELEGATE_DEFENDER); },
};

constexpr Ability DraconicMight = {
    .onEntry = HalfDrake.onEntry,
    ATE_ABILITY(TYPE_DRAGON),
};

constexpr Ability AtlanticRuler = {
    .onOffensiveMultiplier = AquaticDweller.onOffensiveMultiplier,
    .onStat = SwiftSwim.onStat,
    .breakable = TRUE,
};

constexpr Ability Biofilm = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
        },
    .breakable = TRUE,
};

constexpr Ability Chokehold = {
    .breakable = TRUE,
};

constexpr Ability GuardianCoat = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(.8);
        },
    .breakable = TRUE,
    .powderImmune = TRUE,
    .sandImmune = TRUE,
    .hailImmune = TRUE,
};

constexpr Ability NeutralizingFog = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); },
};

constexpr Ability Festivities = {
    .breakable = TRUE,
};

constexpr Ability FeyFlight = {
    .onEntry = FairyTale.onEntry,
    .breakable = TRUE,
    .levitate = TRUE,
};

constexpr Ability BestOffense = {
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    .onSwapSplit = MysticBlades.onSwapSplit,
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[STAT_SPDEF] += 20; },
};

constexpr Ability Impaler = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].hornBased);
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
    .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
};

constexpr Ability MagusBlades = {
    .onParentalBond = DualWield.onParentalBond,
    .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    .onSwapSplit = MysticBlades.onSwapSplit,
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[STAT_SPDEF] += 20; },
};

constexpr Ability LightningBorn = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ELECTRIC); },
};

constexpr Ability Superheavy = {
    .breakable = TRUE,
};

constexpr Ability WorldSerpent = {
    .onAttacker = GripPincer.onAttacker,
    .onOffensiveMultiplier = LongReach.onOffensiveMultiplier,
    .onAccuracy = GripPincer.onAccuracy,
};

constexpr Ability LuckyWings = {
    .onOffensiveMultiplier = GiantWings.onOffensiveMultiplier,
    .onModifyEffectChance = SereneGrace.onModifyEffectChance,
};

constexpr Ability Komodo = {
    .onEntry = HalfDrake.onEntry,
    .onAttacker = ToxicChain.onAttacker,
};

constexpr Ability Envenom = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_POISON);
    },
};

constexpr Ability PurpleHaze = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
    },
};

constexpr Ability GnashingCannon = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            MegaLauncher.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MindCrush.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onChooseOffensiveStat = MindCrush.onChooseOffensiveStat,
};

constexpr Ability HyperCleanse = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        },
    .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability MoltenCoat = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanBeBurned(target))
        CHECK(Random() % 2)

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    },
    ATE_ABILITY(TYPE_ROCK),
};

constexpr Ability RoyalDecree = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_GLARE, 0);
    },
    .onImmune = QueenlyMajesty.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

constexpr Ability Tag = {
    .onPreemptAction = +[](ON_PREEMPT_ACTION) -> int {
        CHECK(gCurrentActionFuncId == B_ACTION_SWITCH)
        gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
            .ability = ability,
            .move = MOVE_PURSUIT,
            .movePower = 20,
            .attacker = battler,
            .target = turnBattler,
        };

        return TRUE;
    },
};

constexpr Ability Surprise = {
    .onPreemptAction = +[](ON_PREEMPT_ACTION) -> int {
        CHECK(gCurrentActionFuncId == B_ACTION_USE_MOVE)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        MoveEnum move = GetChosenMove(turnBattler);
        int targetFlag = GetBattlerBattleMoveTargetFlags(move, turnBattler);

        switch (targetFlag) {
            case MOVE_TARGET_BOTH:
            case MOVE_TARGET_RANDOM:
            case MOVE_TARGET_FOES_AND_ALLY:
                break;

            case MOVE_TARGET_SELECTED:
                CHECK(GetBattlerSide(gBattleStruct->moveTarget[turnBattler]) == GetBattlerSide(battler))
                break;

            default:
                return FALSE;
        }
        gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
            .ability = ability,
            .move = MOVE_ASTONISH,
            .attacker = battler,
            .target = turnBattler,
        };

        return TRUE;
    },
};

constexpr Ability BreezyNeigh = {
    .onBattlerFaints = AdrenalineRush.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability Dreamscape = {
    .onEntry = Comatose.onEntry,
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Dreamcatcher.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MUL(1.2);
        },
    .onStatusImmune = Comatose.onStatusImmune,
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

constexpr Ability HasteMakesWaste = {
    .onOffensiveMultiplier = Analytic.onOffensiveMultiplier,
    .onDefensiveMultiplier = Stall.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability HungryMaws = {
    .onBattlerFaints = JawsOfCarnage.onBattlerFaints,
    .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

constexpr Ability ThermalSlide = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
};

constexpr Ability Thermomancy = {
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            Cryomancy.onModifyEffectChance(DELEGATE_MODIFY_EFFECT_CHANCE);
            Pyromancy.onModifyEffectChance(DELEGATE_MODIFY_EFFECT_CHANCE);
        },
};

constexpr Ability Chuckster = {
    .breakable = TRUE,
};

constexpr Ability HeatSink = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_FIRE,
    .breakable = TRUE,
};

constexpr Ability RelicStone = {
    .breakable = TRUE,
};

constexpr Ability Supercell = {
    .onEntry = +[](ON_ENTRY) -> int { return ElectricSurge.onEntry(DELEGATE_ENTRY) | Drizzle.onEntry(DELEGATE_ENTRY); },
    .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
};

constexpr Ability LightningAspect = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

constexpr Ability FireAspect = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

constexpr Ability BlisteringSun = {
    .onEntry = +[](ON_ENTRY) -> int { return DesolateLand.onEntry(DELEGATE_ENTRY) | AirBlower.onEntry(DELEGATE_ENTRY); },
};

constexpr Ability AurorasGale = {
    .onEntry = NorthWind.onEntry,
    .onStat = MajesticBird.onStat,
    .hailImmune = TRUE,
};

constexpr Ability WinterThrone = {
    .breakable = TRUE,
};

constexpr Ability IcePlumes = {
    .onDefensiveMultiplier = IceScales.onDefensiveMultiplier,
    .breakable = TRUE,
};

constexpr Ability PropellerTail = {
    .onStat = SwiftSwim.onStat,
};

constexpr Ability EnergyTap = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))

        gBattleMoveDamage = -gHpDealt / 8;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

constexpr Ability MoltenCore = {
    .onEntry = +[](ON_ENTRY) -> int {
        Furnace.onEntry(DELEGATE_ENTRY);

        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[GetBattlerSide(battler)] &= ~SIDE_STATUS_STEALTH_ROCK;
        return SwitchInAnnounce(B_MSG_SWITCHIN_MOLTEN_CORE);
    },
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ROCK)
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
    .absorbUp2 = TRUE,
    .stealthRockImmune = TRUE,
};

typedef struct AbilityKVPair {
    u16 key;
    Ability ability;
} AbilityKVPair;

constexpr AbilityKVPair sAbilities[] = {
    {ABILITY_NONE, None},
    {ABILITY_STENCH, Stench},
    {ABILITY_DRIZZLE, Drizzle},
    {ABILITY_SPEED_BOOST, SpeedBoost},
    {ABILITY_BATTLE_ARMOR, BattleArmor},
    {ABILITY_STURDY, Sturdy},
    {ABILITY_DAMP, Damp},
    {ABILITY_LIMBER, Limber},
    {ABILITY_SAND_VEIL, SandVeil},
    {ABILITY_STATIC, Static},
    {ABILITY_VOLT_ABSORB, VoltAbsorb},
    {ABILITY_WATER_ABSORB, WaterAbsorb},
    {ABILITY_OBLIVIOUS, Oblivious},
    {ABILITY_CLOUD_NINE, CloudNine},
    {ABILITY_COMPOUND_EYES, CompoundEyes},
    {ABILITY_INSOMNIA, Insomnia},
    {ABILITY_COLOR_CHANGE, ColorChange},
    {ABILITY_IMMUNITY, Immunity},
    {ABILITY_FLASH_FIRE, FlashFire},
    {ABILITY_SHIELD_DUST, ShieldDust},
    {ABILITY_OWN_TEMPO, OwnTempo},
    {ABILITY_SUCTION_CUPS, SuctionCups},
    {ABILITY_INTIMIDATE, Intimidate},
    {ABILITY_SHADOW_TAG, ShadowTag},
    {ABILITY_ROUGH_SKIN, RoughSkin},
    {ABILITY_WONDER_GUARD, WonderGuard},
    {ABILITY_LEVITATE, Levitate},
    {ABILITY_EFFECT_SPORE, EffectSpore},
    {ABILITY_CLEAR_BODY, ClearBody},
    {ABILITY_NATURAL_CURE, NaturalCure},
    {ABILITY_LIGHTNING_ROD, LightningRod},
    {ABILITY_SERENE_GRACE, SereneGrace},
    {ABILITY_SWIFT_SWIM, SwiftSwim},
    {ABILITY_CHLOROPHYLL, Chlorophyll},
    {ABILITY_ILLUMINATE, Illuminate},
    {ABILITY_TRACE, Trace},
    {ABILITY_HUGE_POWER, HugePower},
    {ABILITY_POISON_POINT, PoisonPoint},
    {ABILITY_INNER_FOCUS, InnerFocus},
    {ABILITY_MAGMA_ARMOR, MagmaArmor},
    {ABILITY_WATER_VEIL, WaterVeil},
    {ABILITY_MAGNET_PULL, MagnetPull},
    {ABILITY_SOUNDPROOF, Soundproof},
    {ABILITY_RAIN_DISH, RainDish},
    {ABILITY_SAND_STREAM, SandStream},
    {ABILITY_PRESSURE, Pressure},
    {ABILITY_THICK_FAT, ThickFat},
    {ABILITY_FLAME_BODY, FlameBody},
    {ABILITY_KEEN_EYE, KeenEye},
    {ABILITY_HYPER_CUTTER, HyperCutter},
    {ABILITY_PICKUP, Pickup},
    {ABILITY_TRUANT, Truant},
    {ABILITY_HUSTLE, Hustle},
    {ABILITY_CUTE_CHARM, CuteCharm},
    {ABILITY_PLUS, Plus},
    {ABILITY_MINUS, Minus},
    {ABILITY_FORECAST, Forecast},
    {ABILITY_STICKY_HOLD, StickyHold},
    {ABILITY_SHED_SKIN, ShedSkin},
    {ABILITY_GUTS, Guts},
    {ABILITY_MARVEL_SCALE, MarvelScale},
    {ABILITY_OVERGROW, Overgrow},
    {ABILITY_BLAZE, Blaze},
    {ABILITY_TORRENT, Torrent},
    {ABILITY_SWARM, Swarm},
    {ABILITY_ROCK_HEAD, RockHead},
    {ABILITY_DROUGHT, Drought},
    {ABILITY_ARENA_TRAP, ArenaTrap},
    {ABILITY_VITAL_SPIRIT, VitalSpirit},
    {ABILITY_WHITE_SMOKE, WhiteSmoke},
    {ABILITY_PURE_POWER, PurePower},
    {ABILITY_SHELL_ARMOR, ShellArmor},
    {ABILITY_AIR_LOCK, AirLock},
    {ABILITY_TANGLED_FEET, TangledFeet},
    {ABILITY_MOTOR_DRIVE, MotorDrive},
    {ABILITY_RIVALRY, Rivalry},
    {ABILITY_SNOW_CLOAK, SnowCloak},
    {ABILITY_ANGER_POINT, AngerPoint},
    {ABILITY_UNBURDEN, Unburden},
    {ABILITY_HEATPROOF, Heatproof},
    {ABILITY_DRY_SKIN, DrySkin},
    {ABILITY_DOWNLOAD, Download},
    {ABILITY_IRON_FIST, IronFist},
    {ABILITY_ADAPTABILITY, Adaptability},
    {ABILITY_SKILL_LINK, SkillLink},
    {ABILITY_HYDRATION, Hydration},
    {ABILITY_SOLAR_POWER, SolarPower},
    {ABILITY_QUICK_FEET, QuickFeet},
    {ABILITY_NORMALIZE, Normalize},
    {ABILITY_SNIPER, Sniper},
    {ABILITY_MAGIC_GUARD, MagicGuard},
    {ABILITY_NO_GUARD, NoGuard},
    {ABILITY_STALL, Stall},
    {ABILITY_TECHNICIAN, Technician},
    {ABILITY_LEAF_GUARD, LeafGuard},
    {ABILITY_MOLD_BREAKER, MoldBreaker},
    {ABILITY_SUPER_LUCK, SuperLuck},
    {ABILITY_AFTERMATH, Aftermath},
    {ABILITY_ANTICIPATION, Anticipation},
    {ABILITY_FOREWARN, Forewarn},
    {ABILITY_UNAWARE, Unaware},
    {ABILITY_TINTED_LENS, TintedLens},
    {ABILITY_FILTER, Filter},
    {ABILITY_SLOW_START, SlowStart},
    {ABILITY_SCRAPPY, Scrappy},
    {ABILITY_STORM_DRAIN, StormDrain},
    {ABILITY_ICE_BODY, IceBody},
    {ABILITY_SOLID_ROCK, SolidRock},
    {ABILITY_SNOW_WARNING, SnowWarning},
    {ABILITY_HONEY_GATHER, HoneyGather},
    {ABILITY_FRISK, Frisk},
    {ABILITY_RECKLESS, Reckless},
    {ABILITY_MULTITYPE, Multitype},
    {ABILITY_FLOWER_GIFT, FlowerGift},
    {ABILITY_BAD_DREAMS, BadDreams},
    {ABILITY_SHEER_FORCE, SheerForce},
    {ABILITY_CONTRARY, Contrary},
    {ABILITY_UNNERVE, Unnerve},
    {ABILITY_DEFEATIST, Defeatist},
    {ABILITY_CURSED_BODY, CursedBody},
    {ABILITY_HEALER, Healer},
    {ABILITY_FRIEND_GUARD, FriendGuard},
    {ABILITY_WEAK_ARMOR, WeakArmor},
    {ABILITY_LIGHT_METAL, LightMetal},
    {ABILITY_MULTISCALE, Multiscale},
    {ABILITY_TOXIC_BOOST, ToxicBoost},
    {ABILITY_FLARE_BOOST, FlareBoost},
    {ABILITY_HARVEST, Harvest},
    {ABILITY_TELEPATHY, Telepathy},
    {ABILITY_MOODY, Moody},
    {ABILITY_OVERCOAT, Overcoat},
    {ABILITY_POISON_TOUCH, PoisonTouch},
    {ABILITY_REGENERATOR, Regenerator},
    {ABILITY_BIG_PECKS, BigPecks},
    {ABILITY_SAND_RUSH, SandRush},
    {ABILITY_WONDER_SKIN, WonderSkin},
    {ABILITY_ANALYTIC, Analytic},
    {ABILITY_ILLUSION, Illusion},
    {ABILITY_IMPOSTER, Imposter},
    {ABILITY_INFILTRATOR, Infiltrator},
    {ABILITY_MUMMY, Mummy},
    {ABILITY_MOXIE, Moxie},
    {ABILITY_JUSTIFIED, Justified},
    {ABILITY_RATTLED, Rattled},
    {ABILITY_MAGIC_BOUNCE, MagicBounce},
    {ABILITY_SAP_SIPPER, SapSipper},
    {ABILITY_PRANKSTER, Prankster},
    {ABILITY_SAND_FORCE, SandForce},
    {ABILITY_IRON_BARBS, IronBarbs},
    {ABILITY_ZEN_MODE, ZenMode},
    {ABILITY_VICTORY_STAR, VictoryStar},
    {ABILITY_TURBOBLAZE, Turboblaze},
    {ABILITY_TERAVOLT, Teravolt},
    {ABILITY_AROMA_VEIL, AromaVeil},
    {ABILITY_FLOWER_VEIL, FlowerVeil},
    {ABILITY_CHEEK_POUCH, CheekPouch},
    {ABILITY_PROTEAN, Protean},
    {ABILITY_FUR_COAT, FurCoat},
    {ABILITY_BULLETPROOF, Bulletproof},
    {ABILITY_STRONG_JAW, StrongJaw},
    {ABILITY_REFRIGERATE, Refrigerate},
    {ABILITY_SWEET_VEIL, SweetVeil},
    {ABILITY_STANCE_CHANGE, StanceChange},
    {ABILITY_GALE_WINGS, GaleWings},
    {ABILITY_MEGA_LAUNCHER, MegaLauncher},
    {ABILITY_GRASS_PELT, GrassPelt},
    {ABILITY_TOUGH_CLAWS, ToughClaws},
    {ABILITY_PIXILATE, Pixilate},
    {ABILITY_GOOEY, Gooey},
    {ABILITY_AERILATE, Aerilate},
    {ABILITY_PARENTAL_BOND, ParentalBond},
    {ABILITY_DARK_AURA, DarkAura},
    {ABILITY_FAIRY_AURA, FairyAura},
    {ABILITY_AURA_BREAK, AuraBreak},
    {ABILITY_PRIMORDIAL_SEA, PrimordialSea},
    {ABILITY_DESOLATE_LAND, DesolateLand},
    {ABILITY_DELTA_STREAM, DeltaStream},
    {ABILITY_STAMINA, Stamina},
    {ABILITY_WIMP_OUT, WimpOut},
    {ABILITY_EMERGENCY_EXIT, EmergencyExit},
    {ABILITY_WATER_COMPACTION, WaterCompaction},
    {ABILITY_MERCILESS, Merciless},
    {ABILITY_SHIELDS_DOWN, ShieldsDown},
    {ABILITY_STAKEOUT, Stakeout},
    {ABILITY_WATER_BUBBLE, WaterBubble},
    {ABILITY_STEELWORKER, Steelworker},
    {ABILITY_BERSERK, Berserk},
    {ABILITY_SLUSH_RUSH, SlushRush},
    {ABILITY_LONG_REACH, LongReach},
    {ABILITY_LIQUID_VOICE, LiquidVoice},
    {ABILITY_TRIAGE, Triage},
    {ABILITY_GALVANIZE, Galvanize},
    {ABILITY_SURGE_SURFER, SurgeSurfer},
    {ABILITY_SCHOOLING, Schooling},
    {ABILITY_DISGUISE, Disguise},
    {ABILITY_BATTLE_BOND, BattleBond},
    {ABILITY_POWER_CONSTRUCT, PowerConstruct},
    {ABILITY_CORROSION, Corrosion},
    {ABILITY_COMATOSE, Comatose},
    {ABILITY_QUEENLY_MAJESTY, QueenlyMajesty},
    {ABILITY_INNARDS_OUT, InnardsOut},
    {ABILITY_DANCER, Dancer},
    {ABILITY_BATTERY, Battery},
    {ABILITY_FLUFFY, Fluffy},
    {ABILITY_DAZZLING, Dazzling},
    {ABILITY_SOUL_HEART, SoulHeart},
    {ABILITY_TANGLING_HAIR, TanglingHair},
    {ABILITY_RECEIVER, Receiver},
    {ABILITY_POWER_OF_ALCHEMY, PowerOfAlchemy},
    {ABILITY_BEAST_BOOST, BeastBoost},
    {ABILITY_RKS_SYSTEM, RksSystem},
    {ABILITY_ELECTRIC_SURGE, ElectricSurge},
    {ABILITY_PSYCHIC_SURGE, PsychicSurge},
    {ABILITY_MISTY_SURGE, MistySurge},
    {ABILITY_GRASSY_SURGE, GrassySurge},
    {ABILITY_SHADOW_SHIELD, ShadowShield},
    {ABILITY_PRISM_ARMOR, PrismArmor},
    {ABILITY_NEUROFORCE, Neuroforce},
    {ABILITY_INTREPID_SWORD, IntrepidSword},
    {ABILITY_DAUNTLESS_SHIELD, DauntlessShield},
    {ABILITY_LIBERO, Libero},
    {ABILITY_COTTON_DOWN, CottonDown},
    {ABILITY_PROPELLER_TAIL, PropellerTail},
    {ABILITY_MIRROR_ARMOR, MirrorArmor},
    {ABILITY_GULP_MISSILE, GulpMissile},
    {ABILITY_STEAM_ENGINE, SteamEngine},
    {ABILITY_PUNK_ROCK, PunkRock},
    {ABILITY_SAND_SPIT, SandSpit},
    {ABILITY_ICE_SCALES, IceScales},
    {ABILITY_ICE_FACE, IceFace},
    {ABILITY_POWER_SPOT, PowerSpot},
    {ABILITY_MIMICRY, Mimicry},
    {ABILITY_SCREEN_CLEANER, ScreenCleaner},
    {ABILITY_STEELY_SPIRIT, SteelySpirit},
    {ABILITY_PERISH_BODY, PerishBody},
    {ABILITY_WANDERING_SPIRIT, WanderingSpirit},
    {ABILITY_GORILLA_TACTICS, GorillaTactics},
    {ABILITY_NEUTRALIZING_GAS, NeutralizingGas},
    {ABILITY_PASTEL_VEIL, PastelVeil},
    {ABILITY_HUNGER_SWITCH, HungerSwitch},
    {ABILITY_CURIOUS_MEDICINE, CuriousMedicine},
    {ABILITY_TRANSISTOR, Transistor},
    {ABILITY_DRAGONS_MAW, DragonsMaw},
    {ABILITY_CHILLING_NEIGH, ChillingNeigh},
    {ABILITY_GRIM_NEIGH, GrimNeigh},
    {ABILITY_AS_ONE_ICE_RIDER, AsOneIceRider},
    {ABILITY_AS_ONE_SHADOW_RIDER, AsOneShadowRider},
    {ABILITY_CHLOROPLAST, Chloroplast},
    {ABILITY_WHITEOUT, Whiteout},
    {ABILITY_PYROMANCY, Pyromancy},
    {ABILITY_KEEN_EDGE, KeenEdge},
    {ABILITY_PRISM_SCALES, PrismScales},
    {ABILITY_POWER_FISTS, PowerFists},
    {ABILITY_SAND_SONG, SandSong},
    {ABILITY_RAMPAGE, Rampage},
    {ABILITY_VENGEANCE, Vengeance},
    {ABILITY_BLITZ_BOXER, BlitzBoxer},
    {ABILITY_ANTARCTIC_BIRD, AntarcticBird},
    {ABILITY_IMMOLATE, Immolate},
    {ABILITY_CRYSTALLIZE, Crystallize},
    {ABILITY_ELECTROCYTES, Electrocytes},
    {ABILITY_AERODYNAMICS, Aerodynamics},
    {ABILITY_CHRISTMAS_SPIRIT, ChristmasSpirit},
    {ABILITY_EXPLOIT_WEAKNESS, ExploitWeakness},
    {ABILITY_GROUND_SHOCK, GroundShock},
    {ABILITY_ANCIENT_IDOL, AncientIdol},
    {ABILITY_MYSTIC_POWER, MysticPower},
    {ABILITY_PERFECTIONIST, Perfectionist},
    {ABILITY_GROWING_TOOTH, GrowingTooth},
    {ABILITY_INFLATABLE, Inflatable},
    {ABILITY_AURORA_BOREALIS, AuroraBorealis},
    {ABILITY_AVENGER, Avenger},
    {ABILITY_LETS_ROLL, LetsRoll},
    {ABILITY_LOUD_BANG, LoudBang},
    {ABILITY_LEAD_COAT, LeadCoat},
    {ABILITY_AMPHIBIOUS, Amphibious},
    {ABILITY_GROUNDED, Grounded},
    {ABILITY_EARTHBOUND, Earthbound},
    {ABILITY_FIGHT_SPIRIT, FightingSpirit},
    {ABILITY_FELINE_PROWESS, FelineProwess},
    {ABILITY_COIL_UP, CoilUp},
    {ABILITY_FOSSILIZED, Fossilized},
    {ABILITY_MAGICAL_DUST, MagicalDust},
    {ABILITY_DREAMCATCHER, Dreamcatcher},
    {ABILITY_NOCTURNAL, Nocturnal},
    {ABILITY_SELF_SUFFICIENT, SelfSufficient},
    {ABILITY_TECTONIZE, Tectonize},
    {ABILITY_ICE_AGE, IceAge},
    {ABILITY_HALF_DRAKE, HalfDrake},
    {ABILITY_LIQUIFIED, Liquified},
    {ABILITY_DRAGONFLY, Dragonfly},
    {ABILITY_DRAGONSLAYER, Dragonslayer},
    {ABILITY_MOUNTAINEER, Mountaineer},
    {ABILITY_HYDRATE, Hydrate},
    {ABILITY_METALLIC, Metallic},
    {ABILITY_PERMAFROST, Permafrost},
    {ABILITY_PRIMAL_ARMOR, PrimalArmor},
    {ABILITY_RAGING_BOXER, RagingBoxer},
    {ABILITY_AIR_BLOWER, AirBlower},
    {ABILITY_JUGGERNAUT, Juggernaut},
    {ABILITY_SHORT_CIRCUIT, ShortCircuit},
    {ABILITY_MAJESTIC_BIRD, MajesticBird},
    {ABILITY_PHANTOM, Phantom},
    {ABILITY_INTOXICATE, Intoxicate},
    {ABILITY_IMPENETRABLE, Impenetrable},
    {ABILITY_HYPNOTIST, Hypnotist},
    {ABILITY_OVERWHELM, Overwhelm},
    {ABILITY_SCARE, Scare},
    {ABILITY_MAJESTIC_MOTH, MajesticMoth},
    {ABILITY_SOUL_EATER, SoulEater},
    {ABILITY_SOUL_LINKER, SoulLinker},
    {ABILITY_SWEET_DREAMS, SweetDreams},
    {ABILITY_BAD_LUCK, BadLuck},
    {ABILITY_HAUNTED_SPIRIT, HauntedSpirit},
    {ABILITY_ELECTRIC_BURST, ElectricBurst},
    {ABILITY_RAW_WOOD, RawWood},
    {ABILITY_SOLENOGLYPHS, Solenoglyphs},
    {ABILITY_SPIDER_LAIR, SpiderLair},
    {ABILITY_FATAL_PRECISION, FatalPrecision},
    {ABILITY_FORT_KNOX, FortKnox},
    {ABILITY_SEAWEED, Seaweed},
    {ABILITY_PSYCHIC_MIND, PsychicMind},
    {ABILITY_POISON_ABSORB, PoisonAbsorb},
    {ABILITY_SCAVENGER, Scavenger},
    {ABILITY_TWISTED_DIMENSION, TwistedDimension},
    {ABILITY_MULTI_HEADED, MultiHeaded},
    {ABILITY_NORTH_WIND, NorthWind},
    {ABILITY_OVERCHARGE, Overcharge},
    {ABILITY_VIOLENT_RUSH, ViolentRush},
    {ABILITY_FLAMING_SOUL, FlamingSoul},
    {ABILITY_SAGE_POWER, SagePower},
    {ABILITY_BONE_ZONE, BoneZone},
    {ABILITY_WEATHER_CONTROL, WeatherControl},
    {ABILITY_SPEED_FORCE, SpeedForce},
    {ABILITY_SEA_GUARDIAN, SeaGuardian},
    {ABILITY_MOLTEN_DOWN, MoltenDown},
    {ABILITY_HYPER_AGGRESSIVE, HyperAggressive},
    {ABILITY_FLOCK, Flock},
    {ABILITY_FIELD_EXPLORER, FieldExplorer},
    {ABILITY_STRIKER, Striker},
    {ABILITY_FROZEN_SOUL, FrozenSoul},
    {ABILITY_PREDATOR, Predator},
    {ABILITY_LOOTER, Looter},
    {ABILITY_LUNAR_ECLIPSE, LunarEclipse},
    {ABILITY_SOLAR_FLARE, SolarFlare},
    {ABILITY_POWER_CORE, PowerCore},
    {ABILITY_SIGHTING_SYSTEM, SightingSystem},
    {ABILITY_BAD_COMPANY, BadCompany},
    {ABILITY_OPPORTUNIST, Opportunist},
    {ABILITY_GIANT_WINGS, GiantWings},
    {ABILITY_MOMENTUM, Momentum},
    {ABILITY_GRIP_PINCER, GripPincer},
    {ABILITY_BIG_LEAVES, BigLeaves},
    {ABILITY_PRECISE_FIST, PreciseFist},
    {ABILITY_DEADEYE, Deadeye},
    {ABILITY_ARTILLERY, Artillery},
    {ABILITY_AMPLIFIER, Amplifier},
    {ABILITY_ICE_DEW, IceDew},
    {ABILITY_SUN_WORSHIP, SunWorship},
    {ABILITY_POLLINATE, Pollinate},
    {ABILITY_VOLCANO_RAGE, VolcanoRage},
    {ABILITY_COLD_REBOUND, ColdRebound},
    {ABILITY_LOW_BLOW, LowBlow},
    {ABILITY_SPECTRAL_SHROUD, SpectralShroud},
    {ABILITY_DISCIPLINE, Discipline},
    {ABILITY_THUNDERCALL, Thundercall},
    {ABILITY_MARINE_APEX, MarineApex},
    {ABILITY_MIGHTY_HORN, MightyHorn},
    {ABILITY_HARDENED_SHEATH, HardenedSheath},
    {ABILITY_ARCTIC_FUR, ArcticFur},
    {ABILITY_SPECTRALIZE, Spectralize},
    {ABILITY_LETHARGY, Lethargy},
    {ABILITY_IRON_BARRAGE, IronBarrage},
    {ABILITY_STEEL_BARREL, SteelBarrel},
    {ABILITY_PYRO_SHELLS, PyroShells},
    {ABILITY_FUNGAL_INFECTION, FungalInfection},
    {ABILITY_PARRY, Parry},
    {ABILITY_SCRAPYARD, Scrapyard},
    {ABILITY_LOOSE_QUILLS, LooseQuills},
    {ABILITY_TOXIC_DEBRIS, ToxicDebris},
    {ABILITY_ROUNDHOUSE, Roundhouse},
    {ABILITY_MINERALIZE, Mineralize},
    {ABILITY_LOOSE_ROCKS, LooseRocks},
    {ABILITY_SPINNING_TOP, SpinningTop},
    {ABILITY_RETRIBUTION_BLOW, RetributionBlow},
    {ABILITY_FEARMONGER, Fearmonger},
    {ABILITY_TOXIC_SPILL, ToxicSpill},
    {ABILITY_DESERT_CLOAK, DesertCloak},
    {ABILITY_DRACONIZE, Draconize},
    {ABILITY_PRETTY_PRINCESS, PrettyPrincess},
    {ABILITY_SELF_REPAIR, SelfRepair},
    {ABILITY_ATOMIC_BURST, AtomicBurst},
    {ABILITY_HELLBLAZE, Hellblaze},
    {ABILITY_RIPTIDE, Riptide},
    {ABILITY_FOREST_RAGE, ForestRage},
    {ABILITY_PRIMAL_MAW, PrimalMaw},
    {ABILITY_SWEEPING_EDGE, SweepingEdge},
    {ABILITY_GIFTED_MIND, GiftedMind},
    {ABILITY_HYDRO_CIRCUIT, HydroCircuit},
    {ABILITY_EQUINOX, Equinox},
    {ABILITY_ABSORBANT, Absorbant},
    {ABILITY_CLUELESS, Clueless},
    {ABILITY_CHEATING_DEATH, CheatingDeath},
    {ABILITY_CHEAP_TACTICS, CheapTactics},
    {ABILITY_COWARD, Coward},
    {ABILITY_VOLT_RUSH, VoltRush},
    {ABILITY_DUNE_TERROR, DuneTerror},
    {ABILITY_INFERNAL_RAGE, InfernalRage},
    {ABILITY_DUAL_WIELD, DualWield},
    {ABILITY_ELEMENTAL_CHARGE, ElementalCharge},
    {ABILITY_AMBUSH, Ambush},
    {ABILITY_ATLAS, Atlas},
    {ABILITY_RADIANCE, Radiance},
    {ABILITY_JAWS_OF_CARNAGE, JawsOfCarnage},
    {ABILITY_ANGELS_WRATH, AngelsWrath},
    {ABILITY_PRISMATIC_FUR, PrismaticFur},
    {ABILITY_SHOCKING_JAWS, ShockingJaws},
    {ABILITY_FAE_HUNTER, FaeHunter},
    {ABILITY_GRAVITY_WELL, GravityWell},
    {ABILITY_EVAPORATE, Evaporate},
    {ABILITY_LUMBERJACK, Lumberjack},
    {ABILITY_WELL_BAKED_BODY, WellBakedBody},
    {ABILITY_FURNACE, Furnace},
    {ABILITY_ELECTROMORPHOSIS, Electromorphosis},
    {ABILITY_ROCKY_PAYLOAD, RockyPayload},
    {ABILITY_EARTH_EATER, EarthEater},
    {ABILITY_LINGERING_AROMA, LingeringAroma},
    {ABILITY_FAIRY_TALE, FairyTale},
    {ABILITY_RAGING_MOTH, RagingMoth},
    {ABILITY_ADRENALINE_RUSH, AdrenalineRush},
    {ABILITY_ARCHMAGE, Archmage},
    {ABILITY_CRYOMANCY, Cryomancy},
    {ABILITY_PHANTOM_PAIN, PhantomPain},
    {ABILITY_PURGATORY, Purgatory},
    {ABILITY_EMANATE, Emanate},
    {ABILITY_KUNOICHI_BLADE, KunoichiBlade},
    {ABILITY_MONKEY_BUSINESS, MonkeyBusiness},
    {ABILITY_COMBAT_SPECIALIST, CombatSpecialist},
    {ABILITY_JUNGLES_GUARD, JunglesGuard},
    {ABILITY_HUNTERS_HORN, HuntersHorn},
    {ABILITY_PIXIE_POWER, PixiePower},
    {ABILITY_PLASMA_LAMP, PlasmaLamp},
    {ABILITY_MAGMA_EATER, MagmaEater},
    {ABILITY_SUPER_HOT_GOO, SuperHotGoo},
    {ABILITY_NIKA, Nika},
    {ABILITY_ARCHER, Archer},
    {ABILITY_SUPER_SLAMMER, SuperSlammer},
    {ABILITY_INVERSE_ROOM, InverseRoom},
    {ABILITY_FROST_BURN, FrostBurn},
    {ABILITY_ITCHY_DEFENSE, ItchyDefense},
    {ABILITY_GENERATOR, Generator},
    {ABILITY_MOON_SPIRIT, MoonSpirit},
    {ABILITY_DUST_CLOUD, DustCloud},
    {ABILITY_BERSERKER_RAGE, BerserkerRage},
    {ABILITY_TRICKSTER, Trickster},
    {ABILITY_SAND_GUARD, SandGuard},
    {ABILITY_NATURAL_RECOVERY, NaturalRecovery},
    {ABILITY_WIND_RIDER, WindRider},
    {ABILITY_SOOTHING_AROMA, SoothingAroma},
    {ABILITY_PRIM_AND_PROPER, PrimAndProper},
    {ABILITY_SUPER_STRAIN, SuperStrain},
    {ABILITY_TIPPING_POINT, TippingPoint},
    {ABILITY_ENLIGHTENED, Enlightened},
    {ABILITY_PEACEFUL_SLUMBER, PeacefulSlumber},
    {ABILITY_AFTERSHOCK, Aftershock},
    {ABILITY_FREEZING_POINT, FreezingPoint},
    {ABILITY_CRYO_PROFICIENCY, CryoProficiency},
    {ABILITY_ARCANE_FORCE, ArcaneForce},
    {ABILITY_DOOMBRINGER, Doombringer},
    {ABILITY_WISHMAKER, Wishmaker},
    {ABILITY_YUKI_ONNA, YukiOnna},
    {ABILITY_SUPPRESS, Suppress},
    {ABILITY_REFRIGERATOR, Refrigerator},
    {ABILITY_HEAVEN_ASUNDER, HeavenAsunder},
    {ABILITY_PURIFYING_WATERS, PurifyingWaters},
    {ABILITY_SEABORNE, Seaborne},
    {ABILITY_HIGH_TIDE, HighTide},
    {ABILITY_CHANGE_OF_HEART, ChangeOfHeart},
    {ABILITY_MYSTIC_BLADES, MysticBlades},
    {ABILITY_DETERMINATION, Determination},
    {ABILITY_FERTILIZE, Fertilize},
    {ABILITY_PURE_LOVE, PureLove},
    {ABILITY_FIGHTER, Fighter},
    {ABILITY_TELEKINETIC, Telekinetic},
    {ABILITY_COMBUSTION, Combustion},
    {ABILITY_PONY_POWER, PonyPower},
    {ABILITY_POWDER_BURST, PowderBurst},
    {ABILITY_RETRIEVER, Retriever},
    {ABILITY_MONSTER_MASH, MonsterMash},
    {ABILITY_TWO_STEP, TwoStep},
    {ABILITY_SPITEFUL, Spiteful},
    {ABILITY_FORTITUDE, Fortitude},
    {ABILITY_DEVOURER, Devourer},
    {ABILITY_PHANTOM_THIEF, PhantomThief},
    {ABILITY_EARLY_GRAVE, EarlyGrave},
    {ABILITY_BASS_BOOSTED, BassBoosted},
    {ABILITY_FLAMING_JAWS, FlamingJaws},
    {ABILITY_MONSTER_HUNTER, MonsterHunter},
    {ABILITY_CROWNED_SWORD, CrownedSword},
    {ABILITY_CROWNED_SHIELD, CrownedShield},
    {ABILITY_BERSERK_DNA, BerserkDna},
    {ABILITY_CROWNED_KING, CrownedKing},
    {ABILITY_SNAP_TRAP_WHEN_HIT, SnapTrapWhenHit},
    {ABILITY_PERMANENCE, Permanence},
    {ABILITY_HUBRIS, Hubris},
    {ABILITY_COSMIC_DAZE, CosmicDaze},
    {ABILITY_MINDS_EYE, MindsEye},
    {ABILITY_BLOOD_PRICE, BloodPrice},
    {ABILITY_SPIKE_ARMOR, SpikeArmor},
    {ABILITY_VOODOO_POWER, VoodooPower},
    {ABILITY_CHROME_COAT, ChromeCoat},
    {ABILITY_BANSHEE, Banshee},
    {ABILITY_WEB_SPINNER, WebSpinner},
    {ABILITY_SHOWDOWN_MODE, ShowdownMode},
    {ABILITY_SEED_SOWER, SeedSower},
    {ABILITY_AIRBORNE, Airborne},
    {ABILITY_PARROTING, Parroting},
    {ABILITY_SALT_CIRCLE, SaltCircle},
    {ABILITY_PURIFYING_SALT, PurifyingSalt},
    {ABILITY_PROTOSYNTHESIS, Protosynthesis},
    {ABILITY_QUARK_DRIVE, QuarkDrive},
    {ABILITY_WIND_POWER, WindPower},
    {ABILITY_IMPULSE, Impulse},
    {ABILITY_TERMINAL_VELOCITY, TerminalVelocity},
    {ABILITY_ANGER_SHELL, AngerShell},
    {ABILITY_EGOIST, Egoist},
    {ABILITY_READIED_ACTION, ReadiedAction},
    {ABILITY_DARK_GALE_WINGS, DarkGaleWings},
    {ABILITY_GUILT_TRIP, GuiltTrip},
    {ABILITY_WATER_GALE_WINGS, WaterGaleWings},
    {ABILITY_ZERO_TO_HERO, ZeroToHero},
    {ABILITY_COSTAR, Costar},
    {ABILITY_COMMANDER, Commander},
    {ABILITY_EJECT_PACK_ABILITY, EjectPackAbility},
    {ABILITY_VENGEFUL_SPIRIT, VengefulSpirit},
    {ABILITY_CUD_CHEW, CudChew},
    {ABILITY_ARMOR_TAIL, ArmorTail},
    {ABILITY_MIND_CRUSH, MindCrush},
    {ABILITY_SUPREME_OVERLORD, SupremeOverlord},
    {ABILITY_ILL_WILL, IllWill},
    {ABILITY_FIRE_SCALES, FireScales},
    {ABILITY_WATCH_YOUR_STEP, WatchYourStep},
    {ABILITY_RAPID_RESPONSE, RapidResponse},
    {ABILITY_DOUBLE_IRON_BARBS, DoubleIronBarbs},
    {ABILITY_THERMAL_EXCHANGE, ThermalExchange},
    {ABILITY_GOOD_AS_GOLD, GoodAsGold},
    {ABILITY_SHARING_IS_CARING, SharingIsCaring},
    {ABILITY_TABLETS_OF_RUIN, TabletsOfRuin},
    {ABILITY_SWORD_OF_RUIN, SwordOfRuin},
    {ABILITY_VESSEL_OF_RUIN, VesselOfRuin},
    {ABILITY_BEADS_OF_RUIN, BeadsOfRuin},
    {ABILITY_PERMAFROST_CLONE, PermafrostClone},
    {ABILITY_GALLANTRY, Gallantry},
    {ABILITY_ORICHALCUM_PULSE, OrichalcumPulse},
    {ABILITY_SUN_BASKING, SunBasking},
    {ABILITY_WINGED_KING, WingedKing},
    {ABILITY_HADRON_ENGINE, HadronEngine},
    {ABILITY_IRON_SERPENT, IronSerpent},
    {ABILITY_SWEEPING_EDGE_PLUS, SweepingEdgePlus},
    {ABILITY_CELESTIAL_BLESSING, CelestialBlessing},
    {ABILITY_MINION_CONTROL, MinionControl},
    {ABILITY_MOLTEN_BLADES, MoltenBlades},
    {ABILITY_HAUNTING_FRENZY, HauntingFrenzy},
    {ABILITY_NOISE_CANCEL, NoiseCancel},
    {ABILITY_RADIO_JAM, RadioJam},
    {ABILITY_OLE, Ole},
    {ABILITY_MALICIOUS, Malicious},
    {ABILITY_DEAD_POWER, DeadPower},
    {ABILITY_BRAWLING_WYVERN, BrawlingWyvern},
    {ABILITY_MYTHICAL_ARROWS, MythicalArrows},
    {ABILITY_LAWNMOWER, Lawnmower},
    {ABILITY_FLOURISH, Flourish},
    {ABILITY_DESERT_SPIRIT, DesertSpirit},
    {ABILITY_CONTEMPT, Contempt},
    {ABILITY_AERIALIST, Aerialist},
    {ABILITY_TERA_SHELL, TeraShell},
    {ABILITY_TOXIC_CHAIN, ToxicChain},
    {ABILITY_PARASITIC_SPORES, ParasiticSpores},
    {ABILITY_POISON_PUPPETEER, PoisonPuppeteer},
    {ABILITY_ENTRANCE, Entrance},
    {ABILITY_REJECTION, Rejection},
    {ABILITY_APPLE_ENLIGHTENMENT, AppleEnlightenment},
    {ABILITY_BALLOON_BOMBER, BalloonBomber},
    {ABILITY_FLAMING_MAW, FlamingMaw},
    {ABILITY_DEMOLITIONIST, Demolitionist},
    {ABILITY_ROCKHARD_WILL, RockhardWill},
    {ABILITY_FRAGRANT_DAZE, FragrantDaze},
    {ABILITY_LOW_VISIBILITY, LowVisibility},
    {ABILITY_OLD_MARINER, OldMariner},
    {ABILITY_ECTOPLASM, Ectoplasm},
    {ABILITY_BEAUTIFUL_MUSIC, BeautifulMusic},
    {ABILITY_SNOW_SONG, SnowSong},
    {ABILITY_GREATER_SPIRIT, GreaterSpirit},
    {ABILITY_RESONANCE, Resonance},
    {ABILITY_ETHEREAL_RUSH, EtherealRush},
    {ABILITY_CUTE_ANTECEDENCE, CuteAntecedence},
    {ABILITY_RECURRING_NIGHTMARE, RecurringNightmare},
    {ABILITY_MENACING_SITUATION, MenacingSituation},
    {ABILITY_SHINY_LIGHTNING, ShinyLightning},
    {ABILITY_TERRIFY, Terrify},
    {ABILITY_ICE_DOWNFALL, IceDownfall},
    {ABILITY_LAST_STAND, LastStand},
    {ABILITY_PYROCLASTIC_FLOW, PyroclasticFlow},
    {ABILITY_BLOOD_BATH, BloodBath},
    {ABILITY_BATTLE_AURA, BattleAura},
    {ABILITY_BLOODLUST, Bloodlust},
    {ABILITY_PIERCING_SOLO, PiercingSolo},
    {ABILITY_RHYTHMIC, Rhythmic},
    {ABILITY_CHUNKY_BASS_LINE, ChunkyBassLine},
    {ABILITY_DUAL_HAMMER, DualHammer},
    {ABILITY_DENTING_BLOWS, DentingBlows},
    {ABILITY_ICE_COLD_HUNTER, IceColdHunter},
    {ABILITY_SOUL_CRUSHER, SoulCrusher},
    {ABILITY_ARC_FLASH, ArcFlash},
    {ABILITY_UNICORN, Unicorn},
    {ABILITY_ON_THE_PROWL, OnTheProwl},
    {ABILITY_PRETENTIOUS, Pretentious},
    {ABILITY_VENOBLAZE_PINCERS, VenoblazePincers},
    {ABILITY_ETERNAL_BLESSING, EternalBlessing},
    {ABILITY_SUGAR_RUSH, SugarRush},
    {ABILITY_PEACEFUL_REST, PeacefulRest},
    {ABILITY_WHITE_NOISE, WhiteNoise},
    {ABILITY_SMOKEY_MANEUVERS, SmokeyManeuvers},
    {ABILITY_POWER_METAL, PowerMetal},
    {ABILITY_POWER_EDGE, PowerEdge},
    {ABILITY_SUPERCONDUCTOR, Superconductor},
    {ABILITY_ULTRA_INSTINCT, UltraInstinct},
    {ABILITY_UNLOCKED_POTENTIAL, UnlockedPotential},
    {ABILITY_HIGHER_RANK, HigherRank},
    {ABILITY_FUNERAL_PYRE, FuneralPyre},
    {ABILITY_FLAME_BUBBLE, FlameBubble},
    {ABILITY_ELEMENTAL_VORTEX, ElementalVortex},
    {ABILITY_SNOWY_WRATH, SnowyWrath},
    {ABILITY_PATTERN_CHANGE, PatternChange},
    {ABILITY_NO_TURNING_BACK, NoTurningBack},
    {ABILITY_FLAMMABLE_COAT, FlammableCoat},
    {ABILITY_DRACO_MORALE, DracoMorale},
    {ABILITY_BAD_OMEN, BadOmen},
    {ABILITY_MOSH_PIT, MoshPit},
    {ABILITY_BLOOD_STAIN, BloodStain},
    {ABILITY_BLOOD_STIGMA, BloodStigma},
    {ABILITY_MAXIMUM_ACCELERATION, MaximumAcceleration},
    {ABILITY_SIDEWINDER, Sidewinder},
    {ABILITY_PETRIFY, Petrify},
    {ABILITY_FLUFFIEST, Fluffiest},
    {ABILITY_WAY_OF_PRECISION, WayOfPrecision},
    {ABILITY_WAY_OF_SWIFTNESS, WayOfSwiftness},
    {ABILITY_ATOMIC_PUNCH, AtomicPunch},
    {ABILITY_IRON_GIANT, IronGiant},
    {ABILITY_MASTER_HAND, MasterHand},
    {ABILITY_FINAL_BLOW, FinalBlow},
    {ABILITY_HOSPITALITY, Hospitality},
    {ABILITY_BUTTER_UP, ButterUp},
    {ABILITY_VITALITY_STRIKE, VitalityStrike},
    {ABILITY_HUGE_WINGS, HugeWings},
    {ABILITY_SWORD_OF_DAMNATION, SwordOfDamnation},
    {ABILITY_RESTRAINING_ORDER, RestrainingOrder},
    {ABILITY_ASSASSINS_TOOLS, AssassinsTools},
    {ABILITY_FROSTMAW, Frostmaw},
    {ABILITY_PATCHWORK, Patchwork},
    {ABILITY_BLIND_RAGE, BlindRage},
    {ABILITY_SLIPSTREAM, Slipstream},
    {ABILITY_APEX_PREDATOR, ApexPredator},
    {ABILITY_DRAGONS_RITUAL, DragonsRitual},
    {ABILITY_PINNACLE_BLADE, PinnacleBlade},
    {ABILITY_ENERGIZED, Energized},
    {ABILITY_COLOR_SPECTRUM, ColorSpectrum},
    {ABILITY_STEEL_BEETLE, SteelBeetle},
    {ABILITY_FROM_THE_SHADOWS, FromTheShadows},
    {ABILITY_RAGE_POINT, RagePoint},
    {ABILITY_HOT_COALS, HotCoals},
    {ABILITY_TERASTAL_TREASURE, TerastalTreasure},
    {ABILITY_SHOCKING_MAW, ShockingMaw},
    {ABILITY_GLEAM_EYES, GleamEyes},
    {ABILITY_ROUSED_FANGS, RousedFangs},
    {ABILITY_DREAM_STATE, DreamState},
    {ABILITY_DREAM_WHIMSY, DreamWhimsy},
    {ABILITY_LUNAR_AFFINITY, LunarAffinity},
    {ABILITY_FLAME_SHIELD, FlameShield},
    {ABILITY_AQUATIC_DWELLER, AquaticDweller},
    {ABILITY_APPLE_PIE, ApplePie},
    {ABILITY_HOVER, Hover},
    {ABILITY_DEPRAVITY, Depravity},
    {ABILITY_WILDFIRE, Wildfire},
    {ABILITY_JUMP_SCARE, JumpScare},
    {ABILITY_TAR_TOSS, TarToss},
    {ABILITY_STUN_SHOCK, StunShock},
    {ABILITY_RAGING_GODDESS, RagingGoddess},
    {ABILITY_WHIPLASH, Whiplash},
    {ABILITY_SUPERSWEET_SYRUP, SupersweetSyrup},
    {ABILITY_TRASH_HEAP, TrashHeap},
    {ABILITY_SLUDGY_MIX, SludgyMix},
    {ABILITY_OVERWATCH, Overwatch},
    {ABILITY_WIND_RAGE, WindRage},
    {ABILITY_VICTORY_BOMB, VictoryBomb},
    {ABILITY_RAZOR_SHARP, RazorSharp},
    {ABILITY_TO_THE_BONE, ToTheBone},
    {ABILITY_BLADE_DANCE, BladeDance},
    {ABILITY_APE_SHIFT, ApeShift},
    {ABILITY_KNOW_YOUR_PLACE, KnowYourPlace},
    {ABILITY_DEEP_CUTS, DeepCuts},
    {ABILITY_LIFE_STEAL, LifeSteal},
    {ABILITY_RUDE_AWAKENING, RudeAwakening},
    {ABILITY_TERAFORM_ZERO, TeraformZero},
    {ABILITY_SET_ABLAZE, SetAblaze},
    {ABILITY_BREAKWATER, Breakwater},
    {ABILITY_MAGICAL_FISTS, MagicalFists},
    {ABILITY_CUTTHROAT, Cutthroat},
    {ABILITY_SAND_BENDER, SandBender},
    {ABILITY_SAND_PIT, SandPit},
    {ABILITY_DESOLATE_SUN, DesolateSun},
    {ABILITY_DAYBREAK, Daybreak},
    {ABILITY_ENERGY_SIPHON, EnergySiphon},
    {ABILITY_RESERVOIR, Reservoir},
    {ABILITY_NEUROTOXIN, Neurotoxin},
    {ABILITY_ENERGIZED_HORNS, EnergizedHorns},
    {ABILITY_SPIDER_LAIR_UPGRADE, SpiderLairUpgrade},
    {ABILITY_CRUST_COAT, CrustCoat},
    {ABILITY_PUFFY, Puffy},
    {ABILITY_BALLOON_BLITZ, BalloonBlitz},
    {ABILITY_STRIKER_PIXILATE, StrikerPixilate},
    {ABILITY_DOOM_BLAST, DoomBlast},
    {ABILITY_BRUTEFORCE, Bruteforce},
    {ABILITY_FARADAY_CAGE, FaradayCage},
    {ABILITY_ACIDIC_SLIME, AcidicSlime},
    {ABILITY_ROSE_GARDEN, RoseGarden},
    {ABILITY_QIGONG, Qigong},
    {ABILITY_CONJOURER_OF_DECEIT, ConjurerOfDeceit},
    {ABILITY_DEEP_FREEZE, DeepFreeze},
    {ABILITY_SOUL_DEVOURER, SoulDevourer},
    {ABILITY_CHAMPIONS_ENTRANCE, ChampionsEntrance},
    {ABILITY_PRESTO, Presto},
    {ABILITY_SAMBA, Samba},
    {ABILITY_GLADIATOR, Gladiator},
    {ABILITY_FORSAKEN_HEART, ForsakenHeart},
    {ABILITY_RELENTLESS, Relentless},
    {ABILITY_SOOTHSAYER, Soothsayer},
    {ABILITY_CORRUPTED_MIND, CorruptedMind},
    {ABILITY_FLAME_COAT, FlameCoat},
    {ABILITY_UNOWN_POWER, UnownPower},
    {ABILITY_SUPER_SCOPE, SuperScope},
    {ABILITY_VENOM_CROWN, VenomCrown},
    {ABILITY_BLIGHT_SCALE, BlightScale},
    {ABILITY_GUNMAN, Gunman},
    {ABILITY_HUNTERS_MARK, HuntersMark},
    {ABILITY_CARETAKER, Caretaker},
    {ABILITY_POSEIDONS_DOMINION, PoseidonsDominion},
    {ABILITY_DUAL_SHADOW, DualShadow},
    {ABILITY_LULLABY, Lullaby},
    {ABILITY_CRYO_ARCHITECT, CryoArchitect},
    {ABILITY_GLACIAL_RAGE, GlacialRage},
    {ABILITY_IMMOVABLE_OBJECT, ImmovableObject},
    {ABILITY_FRENZIED_PHANTOM, FrenziedPhantom},
    {ABILITY_DNA_SCRAMBLE, DNAScramble},
    {ABILITY_AQUATIC, Aquatic},
    {ABILITY_METALLIC_JAWS, MetallicJaws},
    {ABILITY_CALCULATIVE, Calculative},
    {ABILITY_EMBODY_ASPECT, EmbodyAspect},
    {ABILITY_EMBODY_ASPECT_HEARTHFLAME, EmbodyAspect},
    {ABILITY_EMBODY_ASPECT_CORNERSTONE, EmbodyAspectCornerstone},
    {ABILITY_EMBODY_ASPECT_WELLSPRING, EmbodyAspectWellspring},
    {ABILITY_ROCKHARD_SHAFT, RockhardShaft},
    {ABILITY_DEVIATE, Deviate},
    {ABILITY_SUNS_BOUNTY, SunsBounty},
    {ABILITY_RITE_OF_SPRING, RiteOfSpring},
    {ABILITY_HEADSTRONG, Headstrong},
    {ABILITY_FIREFIGHTER, Firefighter},
    {ABILITY_SEPIA_LENS, SepiaLens},
    {ABILITY_SUPER_SNIPER, SuperSniper},
    {ABILITY_WOODLAND_CURSE, WoodlandCurse},
    {ABILITY_MALODOR, Malodor},
    {ABILITY_BLUR, Blur},
    {ABILITY_ELUDE, Elude},
    {ABILITY_DRAKE_OF_RAGE, DrakeOfRage},
    {ABILITY_MIXED_MARTIAL_ARTS, MixedMartialArts},
    {ABILITY_STRATEGIC_PAUSE, StrategicPause},
    {ABILITY_OVERRULE, Overrule},
    {ABILITY_MENTAL_POLLUTION, MentalPollution},
    {ABILITY_MADNESS_ENHANCEMENT, MadnessEnhancement},
    {ABILITY_TENTALOCK, Tentalock},
    {ABILITY_SERPENT_BIND, SerpentBind},
    {ABILITY_SOUL_TAP, SoulTap},
    {ABILITY_SCARECROW, Scarecrow},
    {ABILITY_OMINOUS_SHROUD, OminousShroud},
    {ABILITY_CHILLING_PRESENCE, ChillingPresence},
    {ABILITY_FROSTBIND, Frostbind},
    {ABILITY_GLACIAL_GHOST, GlacialGhost},
    {ABILITY_TENDER_AFFECTION, TenderAffection},
    {ABILITY_WONDER_SCALE, WonderScale},
    {ABILITY_OVERZEALOUS, Overzealous},
    {ABILITY_STAINLESS_STEEL, StainlessSteel},
    {ABILITY_TEMPORAL_RUPTURE, TemporalRupture},
    {ABILITY_GRASS_FLUTE, GrassFlute},
    {ABILITY_HEMOTOXIN, Hemotoxin},
    {ABILITY_HARUKAZE, Harukaze},
    {ABILITY_TOXIC_SURGE, ToxicSurge},
    {ABILITY_ATLANTIC_RULER, AtlanticRuler},
    {ABILITY_BIOFILM, Biofilm},
    {ABILITY_CHOKEHOLD, Chokehold},
    {ABILITY_GUARDIAN_COAT, GuardianCoat},
    {ABILITY_NEUTRALIZING_FOG, NeutralizingFog},
    {ABILITY_POISON_QUILLS, PoisonQuills},
    {ABILITY_DRACONIC_MIGHT, DraconicMight},
    {ABILITY_FESTIVITIES, Festivities},
    {ABILITY_FEY_FLIGHT, FeyFlight},
    {ABILITY_BEST_OFFENSE, BestOffense},
    {ABILITY_IMPALER, Impaler},
    {ABILITY_MAGUS_BLADES, MagusBlades},
    {ABILITY_LIGHTNING_BORN, LightningBorn},
    {ABILITY_SUPERHEAVY, Superheavy},
    {ABILITY_WORLD_SERPENT, WorldSerpent},
    {ABILITY_LUCKY_WINGS, LuckyWings},
    {ABILITY_KOMODO, Komodo},
    {ABILITY_ENVENOM, Envenom},
    {ABILITY_PURPLE_HAZE, PurpleHaze},
    {ABILITY_GNASHING_CANNON, GnashingCannon},
    {ABILITY_HYPER_CLEANSE, HyperCleanse},
    {ABILITY_MOLTEN_COAT, MoltenCoat},
    {ABILITY_ROYAL_DECREE, RoyalDecree},
    {ABILITY_BREEZY_NEIGH, BreezyNeigh},
    {ABILITY_DREAMSCAPE, Dreamscape},
    {ABILITY_HASTE_MAKES_WASTE, HasteMakesWaste},
    {ABILITY_HUNGRY_MAWS, HungryMaws},
    {ABILITY_THERMAL_SLIDE, ThermalSlide},
    {ABILITY_THERMOMANCY, Thermomancy},
    {ABILITY_CHUCKSTER, Chuckster},
    {ABILITY_HEAT_SINK, HeatSink},
    {ABILITY_RELIC_STONE, RelicStone},
    {ABILITY_SUPERCELL, Supercell},
    {ABILITY_LIGHTNING_ASPECT, LightningAspect},
    {ABILITY_POISON_HEAL, PoisonHeal},
    {ABILITY_ENERGY_TAP, EnergyTap},
};

template <int N>
consteval AbilitiesWrapper mergeArrays(AbilitiesWrapper wrapper, const AbilityKVPair second[N]) {
    AbilitiesWrapper newWrapper = wrapper;
    bool assigned[ABILITIES_COUNT] = {0};
    for (int i = 0; i < N; i++) {
        auto val = second[i];
        if (assigned[val.key]) {
            return (AbilitiesWrapper){0};
        }
        assigned[val.key] = true;
#define __OVERWRITE_ARRAY_VAL(field) .field = val.ability.field ? val.ability.field : wrapper.abilities[val.key].field

        newWrapper.abilities[val.key] = (Ability){
            __OVERWRITE_ARRAY_VAL(name),
            __OVERWRITE_ARRAY_VAL(description),
            __OVERWRITE_ARRAY_VAL(onEntry),
            __OVERWRITE_ARRAY_VAL(onAbsorb),
            __OVERWRITE_ARRAY_VAL(onImmune),
            __OVERWRITE_ARRAY_VAL(onInfiltrate),
            __OVERWRITE_ARRAY_VAL(onDisguise),
            __OVERWRITE_ARRAY_VAL(onWeather),
            __OVERWRITE_ARRAY_VAL(onTerrain),
            __OVERWRITE_ARRAY_VAL(onEndTurn),
            __OVERWRITE_ARRAY_VAL(onAttacker),
            __OVERWRITE_ARRAY_VAL(onDefender),
            __OVERWRITE_ARRAY_VAL(onRecoil),
            __OVERWRITE_ARRAY_VAL(onReactive),
            __OVERWRITE_ARRAY_VAL(onBattlerFaints),
            __OVERWRITE_ARRAY_VAL(onParentalBond),
            __OVERWRITE_ARRAY_VAL(onOffensiveMultiplier),
            __OVERWRITE_ARRAY_VAL(onDefensiveMultiplier),
            __OVERWRITE_ARRAY_VAL(onMoveType),
            __OVERWRITE_ARRAY_VAL(onStab),
            __OVERWRITE_ARRAY_VAL(onStat),
            __OVERWRITE_ARRAY_VAL(onAccuracy),
            __OVERWRITE_ARRAY_VAL(onSwapSplit),
            __OVERWRITE_ARRAY_VAL(onChooseOffensiveStat),
            __OVERWRITE_ARRAY_VAL(onChooseDefensiveStat),
            __OVERWRITE_ARRAY_VAL(onPriority),
            __OVERWRITE_ARRAY_VAL(onExit),
            __OVERWRITE_ARRAY_VAL(onCrit),
            __OVERWRITE_ARRAY_VAL(onTypeEffectiveness),
            __OVERWRITE_ARRAY_VAL(onCopyMove),
            __OVERWRITE_ARRAY_VAL(onAfterTypeEffectiveness),
            __OVERWRITE_ARRAY_VAL(onModifyEffectChance),
            __OVERWRITE_ARRAY_VAL(onCanStatusType),
            __OVERWRITE_ARRAY_VAL(onStatusImmune),
            __OVERWRITE_ARRAY_VAL(onTrap),
            __OVERWRITE_ARRAY_VAL(onBeforeAttack),
            __OVERWRITE_ARRAY_VAL(onImmuneFor),
            __OVERWRITE_ARRAY_VAL(onBattlerFaintsFor),
            __OVERWRITE_ARRAY_VAL(onOffensiveMultiplierFor),
            __OVERWRITE_ARRAY_VAL(onStatFor),
            __OVERWRITE_ARRAY_VAL(onAccuracyFor),
            __OVERWRITE_ARRAY_VAL(onChooseDefensiveStatFor),
            __OVERWRITE_ARRAY_VAL(onCritFor),
            __OVERWRITE_ARRAY_VAL(onAfterTypeEffectivenessFor),
            __OVERWRITE_ARRAY_VAL(onModifyEffectChanceFor),
            __OVERWRITE_ARRAY_VAL(onStatusImmuneFor),
            __OVERWRITE_ARRAY_VAL(onBeforeAttackFor),
            __OVERWRITE_ARRAY_VAL(setStateOnEffect),
            __OVERWRITE_ARRAY_VAL(allowTerrainIfAirborne),
            __OVERWRITE_ARRAY_VAL(redirectType),
            __OVERWRITE_ARRAY_VAL(ruinStat),
            __OVERWRITE_ARRAY_VAL(noDamageHits),
            __OVERWRITE_ARRAY_VAL(breakable),
            __OVERWRITE_ARRAY_VAL(unsuppressable),
            __OVERWRITE_ARRAY_VAL(persistent),
            __OVERWRITE_ARRAY_VAL(randomizerBanned),
            __OVERWRITE_ARRAY_VAL(unaware),
            __OVERWRITE_ARRAY_VAL(absorbUp2),
            __OVERWRITE_ARRAY_VAL(isSoundproof),
            __OVERWRITE_ARRAY_VAL(magicGuard),
            __OVERWRITE_ARRAY_VAL(noRecoil),
            __OVERWRITE_ARRAY_VAL(halfRecoil),
            __OVERWRITE_ARRAY_VAL(chloroplast),
            __OVERWRITE_ARRAY_VAL(skillLink),
            __OVERWRITE_ARRAY_VAL(resistsFortKnox),
            __OVERWRITE_ARRAY_VAL(fortKnox),
            __OVERWRITE_ARRAY_VAL(adaptability),
            __OVERWRITE_ARRAY_VAL(magicBounce),
            __OVERWRITE_ARRAY_VAL(levitate),
            __OVERWRITE_ARRAY_VAL(megaLauncherBoost),
            __OVERWRITE_ARRAY_VAL(unnerve),
            __OVERWRITE_ARRAY_VAL(negatesBurnAtkDrop),
            __OVERWRITE_ARRAY_VAL(negatesFrzSpatkDrop),
            __OVERWRITE_ARRAY_VAL(canInfatuateAny),
            __OVERWRITE_ARRAY_VAL(removesStatusOnImmunity),
            __OVERWRITE_ARRAY_VAL(tauntImmune),
            __OVERWRITE_ARRAY_VAL(shadowTag),
            __OVERWRITE_ARRAY_VAL(foesMinRoll),
            __OVERWRITE_ARRAY_VAL(powderImmune),
            __OVERWRITE_ARRAY_VAL(sandImmune),
            __OVERWRITE_ARRAY_VAL(hailImmune),
            __OVERWRITE_ARRAY_VAL(toxicTerrainImmune),
            __OVERWRITE_ARRAY_VAL(stealthRockImmune),
        };
    }
    return newWrapper;
}

#include "generated/data/abilities/ability_text.hh"

const AbilitiesWrapper gAbilitiesWrapper =
    mergeArrays<ARRAY_COUNT(sAbilities)>(mergeArrays<ARRAY_COUNT(sAbilityText)>((AbilitiesWrapper){0}, sAbilityText), sAbilities);

#pragma GCC diagnostic pop