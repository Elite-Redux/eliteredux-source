#include "abilities.hh"

#undef __SIZE_TYPE__
#define __SIZE_TYPE__ uint32_t
#include <cstddef>
#include <array>

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

#define NO_ANNOUNCE 2

static int max(int a, int b) { return a > b ? a : b; }

static int min(int a, int b) { return a < b ? a : b; }

class __EnumHack {
   public:
    operator int() const { return 0; }
    operator u32() const { return 0; }
    operator AccuracyPriority() const { return ACCURACY_NO_RESULT; }
    operator MultihitType() const { return MULTIHIT_SINGLE; }
};

#define ENUM_OR(enumType) \
    inline constexpr enumType operator|(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }

ENUM_OR(InfiltrateType)
ENUM_OR(MoveEffectEnum)
ENUM_OR(TerrainType)
ENUM_OR(NonStackingState)

#define CHECK(effect) \
    if (!(effect)) return __EnumHack();
#define CHECK_NOT(effect) \
    if (effect) return __EnumHack();

#define __COMBINE(val1, val2) val1##val2
#define COMBINE(val1, val2) __COMBINE(val1, val2)

#define opt [[maybe_unused]]

#define ON_ENTRY opt opt AbilityEnum ability, opt int battler
#define DELEGATE_ENTRY ability, battler
#define ON_ABSORB opt int battler, opt MoveEnum move, opt Type moveType, opt int *statId
#define DELEGATE_ABSORB battler, move, moveType, statId
#define ON_IMMUNE opt int battler, opt int attacker, opt MoveEnum move, opt Type moveType, opt const u8 **immunityScript
#define DELEGATE_IMMUNE battler, attacker, move, moveType, immunityScript
#define ON_INFILTRATE opt int battler, opt MoveEnum move, opt Type moveType
#define DELEGATE_INFILTRATE battler, move, moveType
#define ON_DISGUISE opt int battler, opt int testOnly
#define DELEGATE_DISGUISE battler, testOnly
#define ON_WEATHER opt AbilityEnum ability, opt int battler
#define DELEGATE_WEATHER ability, battler
#define ON_TERRAIN opt AbilityEnum ability, opt int battler
#define DELEGATE_TERRAIN ability, battler
#define ON_END_TURN opt AbilityEnum ability, opt u8 battler
#define DELEGATE_END_TURN ability, battler
#define ON_ATTACKER opt AbilityEnum ability, opt int battler, opt int target, opt MoveEnum move, opt Type moveType
#define DELEGATE_ATTACKER ability, battler, target, move, moveType
#define ON_DEFENDER opt AbilityEnum ability, opt int battler, opt int attacker, opt MoveEnum move, opt Type moveType
#define DELEGATE_DEFENDER ability, battler, attacker, move, moveType
#define ON_EITHER(name) static int name##OnEither(opt AbilityEnum ability, opt int battler, opt int opponent, opt MoveEnum move, opt Type moveType)
#define ON_EITHER_ABILITY(name) .onAttacker = name##OnEither, .onDefender = name##OnEither
#define ON_RECOIL opt int damage, opt int battler, opt Type moveType
#define DELEGATE_RECOIL damage, battler, moveType
#define ON_REACTIVE opt AbilityEnum ability, opt int battler, opt AbilityCallType callType
#define DELEGATE_REACTIVE ability, battler
#define ON_BATTLER_FAINTS opt AbilityEnum ability, opt int battler, opt int attacker, opt int fainted, opt MoveEnum move, opt Type moveType
#define DELEGATE_BATTLER_FAINTS ability, battler, attacker, fainted, move, moveType
#define ON_PARENTAL_BOND opt int battler, opt MoveEnum move, opt Type moveType
#define DELEGATE_PARENTAL_BOND battler, move, moveType
#define ON_STAT opt AbilityEnum ability, opt int battler, opt int statId, opt u32 *stat, opt NonStackingState *flags
#define DELEGATE_STAT ability, battler, statId, stat, flags
#define ON_OFFENSIVE_MULTIPLIER                                                                                                                             \
    opt int battler, opt AbilityEnum ability, opt int target, opt MoveEnum move, opt Type moveType, opt int basePower, opt int typeEffectivenessMultiplier, \
        opt int isCrit, opt u16 *resistance, u16 *modifier
#define DELEGATE_OFFENSIVE_MULTIPLIER battler, ability, target, move, moveType, basePower, typeEffectivenessMultiplier, isCrit, resistance, modifier
#define ON_DEFENSIVE_MULTIPLIER                                                                                                                          \
    opt int battler, opt AbilityEnum ability, opt int attacker, opt MoveEnum move, opt Type moveType, opt int typeEffectivenessModifier, opt int isCrit, \
        opt u16 *resistance, opt u16 *modifier
#define DELEGATE_DEFENSIVE_MULTIPLIER battler, attacker, move, moveType, typeEffectivenessModifier, isCrit, resistance, modifier
#define ON_ACCURACY opt AbilityEnum ability, opt int battler, opt int target, opt MoveEnum move, opt Type moveType, opt int *accuracy
#define DELEGATE_ACCURACY ability, battler, target, move, moveType, accuracy
#define ON_SWAP_SPLIT opt int battler, opt MoveEnum move, opt Type moveType
#define DELEGATE_SWAP_SPLIT battler, move, type
#define ON_CHOOSE_OFFENSIVE_STAT \
    opt int battler, opt MoveEnum move, opt int ignoreOffensiveStatDrops, opt int targetUnaware, opt u8 *atkStatToUse, opt u8 secondaryAtkStatToUse[NUM_STATS]
#define DELEGATE_CHOOSE_OFFENSIVE_STAT battler, move, ignoreOffensiveStatDrops, targetUnaware, atkStatToUse, secondaryAtkStatToUse
#define ON_CHOOSE_DEFENSIVE_STAT opt int battler, opt int target, opt MoveEnum move, opt int ignoreDefensiveStatBoosts, opt int battlerUnaware
#define DELEGATE_CHOOSE_DEFENSIVE_STAT battler, target, move, ignoreDefensiveStatBoosts, battlerUnaware
#define ON_STAB opt Type moveType
#define DELEGATE_STAB moveType
#define ON_PRIORITY opt int battler, opt int target, opt MoveEnum move
#define DELEGATE_PRIORITY battler, target, move
#define ON_MOVE_TYPE opt AbilityEnum ability, opt MoveEnum move, opt Type moveType, opt u8 *ateBoost
#define DELEGATE_MOVE_TYPE ability, move, moveType, ateBoost
#define ON_EXIT opt AbilityEnum ability, opt int battler
#define DELEGATE_EXIT ability, battler
#define ON_CRIT opt int battler, opt int target, opt MoveEnum move, opt u16 typeEffectiveness
#define DELEGATE_CRIT battler, target, move, typeEffectiveness
#define ON_TYPE_EFFECTIVENESS opt int defType, opt MoveEnum move, opt Type moveType, opt u16 *mod
#define DELEGATE_TYPE_EFFECTIVENESS defType, move, moveType, mod
#define ON_COPY_MOVE opt AbilityEnum ability, opt int battler, opt int attacker, opt int target, opt MoveEnum move
#define DELEGATE_COPY_MOVE ability, battler, attacker, target, move
#define ON_AFTER_TYPE_EFFECTIVENESS \
    opt int battler, opt AbilityEnum ability, opt int target, opt MoveEnum move, opt Type moveType, opt u16 *mod, opt u16 mod1, opt u16 mod2, opt u16 mod3
#define DELEGATE_AFTER_TYPE_EFFECTIVENESS battler, target, move, moveType, mod, mod1, mod2, mod3
#define ON_MODIFY_EFFECT_CHANCE opt int battler, opt MoveEnum move, opt MoveEffectEnum moveEffect, opt int *effectChance
#define DELEGATE_MODIFY_EFFECT_CHANCE battler, move, moveEffect, effectChance
#define ON_CAN_STATUS_TYPE opt int battler, opt MoveEnum move, opt StatusCheckEnum status
#define DELEGATE_CAN_STATUS_TYPE battler, move, status
#define ON_STATUS_IMMUNE opt int battler, opt int target, opt AbilityEnum ability, opt StatusCheckEnum status
#define DELEGATE_STATUS_IMMUNE int battler, target, ability, status
#define ON_TRAP opt int switchingBattler
#define DELEGATE_TRAP switchingBattler
#define ON_BEFORE_ATTACK opt int battler, opt int attacker, opt AbilityEnum ability, opt MoveEnum move, opt Type moveType
#define DELEGATE_BEFORE_ATTACK battler, attacker, ability, move, moveType
#define ON_PREEMPT_ACTION opt u8 battler, opt AbilityEnum ability, opt u8 turnBattler
#define DELEGATE_PREEMPT_ACTION battler, ability, turnBattler
#define ON_MODIFY_MOVE_FLAGS opt int battler, opt MoveEnum move, opt Type moveType, opt MoveFlag flag
#define DELEGATE_MODIFY_MOVE_FLAGS battler, move, moveType, flag
#define ON_MOLD_BREAKER opt int battler, opt MoveEnum move
#define DELEGATE_MOLD_BREAKER battler, move
#define ON_REVIVE opt int battler
#define DELEGATE_REVIVE battler
#define ON_STAT_LOWERED opt int battler
#define DELEGATE_STAT_LOWERED battler

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

template <typename AbilityPredicate>
static inline bool BattlerHasAbility(int battler, int checkMoldBreaker, AbilityPredicate abilityPredicate) {
    for (int j = 0; j < GetNumPossibleAbilitiesForBattler(); j++) {
        if (abilityPredicate(GetAbilityAtIndex(battler, j, checkMoldBreaker))) return true;
    }
    return false;
}

template <typename AbilityPredicate, typename BattlerPredicate>
static inline bool IsAbilityOnField(int breakable, AbilityPredicate abilityPredicate, BattlerPredicate battlerPredicate) {
    for (int i = 0; i < gBattlersCount; i++) {
        if (!battlerPredicate(i)) continue;
        if (BattlerHasAbility(i, breakable, abilityPredicate)) return true;
    }
    return false;
}

template <typename AbilityPredicate>
static inline bool IsAbilityOnField(int breakable, AbilityPredicate abilityPredicate) {
    return IsAbilityOnField(breakable, abilityPredicate, +[](opt int battler) -> bool { return true; });
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
static int AdjustFollowupMoveTarget(int battler, int* target, MoveEnum move, FollowupType type) {
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

static int CheckAbilityWasAnnouncedBy(int announcer, AbilityEnum ability) {
    return BattlerHasAbility(announcer, ability, FALSE) && !CheckAndSetSwitchInAbility(announcer, ability);
}

static int CheckAbilityWasAnnounced(int battler, AbilityEnum ability) {
    for (int other = 0; other < gBattlersCount; other++) {
        FILTER(other != battler)
        if (CheckAbilityWasAnnouncedBy(other, ability)) return TRUE;
    }
    return FALSE;
}

static int SwitchInAnnounce(int message) {
    gBattleCommunication[MULTISTRING_CHOOSER] = message;
    BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
    return TRUE;
}

static int TryTransformAttacker(int battler, AbilityCallType callType) {
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

static int PoisonPuppeteerClone(AbilityEnum ability, int battler, int (*predicate)(int battler, int target), const u8* callback) {
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

static void RuinEffect(int ruinStat, int battler, int statId, u32* stat, NonStackingState* flags) {
    if (statId != ruinStat) return;
    if (*flags & NON_STACKING_RUIN) return;
    if (BattlerHasAbility(battler, FALSE, [statId](AbilityEnum ability) -> int { return gAbilities[ability].ruinStat == statId; })) return;
    *stat *= .75;
    *flags = *flags | NON_STACKING_RUIN;
}

int DoesMoveMatchFlag(ON_MODIFY_MOVE_FLAGS) {
    switch (flag) {
        case MOVE_FLAG_DANCE:
            if (gBattleMoves[move].flags & FLAG_DANCE) return TRUE;
            break;
        case MOVE_FLAG_KICK:
            if (gBattleMoves[move].flags & FLAG_STRIKER_BOOST) return TRUE;
            break;
        case MOVE_FLAG_MEGA_LAUNCHER:
            if (gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST) return TRUE;
            break;
        case MOVE_FLAG_PUNCH:
            if (gBattleMoves[move].flags & FLAG_IRON_FIST_BOOST) return TRUE;
            break;
        case MOVE_FLAG_SOUND:
            if (gBattleMoves[move].flags & FLAG_SOUND) return TRUE;
            break;
        case MOVE_FLAG_KEEN_EDGE:
            if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) return TRUE;
            break;

        default:
            return FALSE;
            break;
    }

    return BattlerHasAbility(battler, FALSE, [&](int ability) -> int {
        CHECK(gAbilities[ability].onModifyMoveFlags)
        return (gAbilities[ability].onModifyMoveFlags(DELEGATE_MODIFY_MOVE_FLAGS));
    });
}

static int UseTurnAttackAsPursuit(ON_PREEMPT_ACTION) {
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
}

template <AbilityEnum Id>
constexpr Ability Impl = {0};

template <>
constexpr Ability Impl<ABILITY_NONE> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STENCH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    },
    .toxicTerrainImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_POISON_HEAL> = {
    .toxicTerrainImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DRIZZLE> = {
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

template <>
constexpr Ability Impl<ABILITY_SPEED_BOOST> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_COOL_EXIT> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)

        switch (GetAbilityState(battler, ability)) {
            case 0:
                SetAbilityState(battler, ability, 1);
                break;

            case 1:
                gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
                    .ability = ability,
                    .move = MOVE_CHILLY_RECEPTION,
                    .attacker = battler,
                    .target = battler,
                };
                SetAbilityState(battler, ability, 2);
                break;
        }

        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_BATTLE_ARMOR> = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.8); },
    .onCrit = +[](ON_CRIT) { return NEVER_CRIT; },
    .onCritFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STURDY> = {
    .breakable = TRUE,
};

ON_EITHER(Damp) {
    CHECK(ShouldApplyOnHitEffect(opponent))
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
template <>
constexpr Ability Impl<ABILITY_DAMP> = {
    ON_EITHER_ABILITY(Damp),
};

template <>
constexpr Ability Impl<ABILITY_LIMBER> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
    .breakable = TRUE,
    .halfRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SAND_VEIL> = {
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
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanBeParalyzed(battler, opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_PARALYSIS, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_STATIC> = {
    ON_EITHER_ABILITY(Static),
};

template <>
constexpr Ability Impl<ABILITY_VOLT_ABSORB> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WATER_ABSORB> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_OBLIVIOUS> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING))
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CLOUD_NINE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_COMPOUND_EYES> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.3;
        return ACCURACY_MULTIPLICATIVE;
    },
};

template <>
constexpr Ability Impl<ABILITY_INSOMNIA> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_COLOR_CHANGE> = {
    .onBeforeAttack = +[](ON_BEFORE_ATTACK) -> int {
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

template <>
constexpr Ability Impl<ABILITY_IMMUNITY> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & (CHECK_STATUS1 & ~CHECK_SLEEP))
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FLASH_FIRE> = {
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

template <>
constexpr Ability Impl<ABILITY_SHIELD_DUST> = {
    .breakable = TRUE,
    .powderImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_OWN_TEMPO> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUCTION_CUPS> = {
    .breakable = TRUE,
    .suctionCups = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GUARD_DOG> = {
    .breakable = TRUE,
    .suctionCups = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_INTIMIDATE> = {
    .onEntry = UseIntimidateClone,
};

template <>
constexpr Ability Impl<ABILITY_SHADOW_TAG> = {
    .onTrap =
        +[](ON_TRAP) -> int { return BattlerHasAbility(switchingBattler, FALSE, [](AbilityEnum ability) -> int { return gAbilities[ability].shadowTag; }); },
    .shadowTag = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ROUGH_SKIN> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        gBattleMoveDamage = gBattleMons[attacker].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_WONDER_GUARD> = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod < UQ_4_12(2.0)) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LEVITATE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING) MUL(1.25);
        },
    .breakable = TRUE,
    .levitate = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_EFFECT_SPORE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_CLEAR_BODY> = {
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_NATURAL_CURE> = {
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

template <>
constexpr Ability Impl<ABILITY_LIGHTNING_ROD> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_ELECTRIC,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SERENE_GRACE> = {
    .onModifyEffectChance = +[](ON_MODIFY_EFFECT_CHANCE) { *effectChance *= 2; },
};

template <>
constexpr Ability Impl<ABILITY_SWIFT_SWIM> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_CHLOROPHYLL> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_ILLUMINATE> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

template <>
constexpr Ability Impl<ABILITY_TRACE> = {
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

template <>
constexpr Ability Impl<ABILITY_HUGE_POWER> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK) *stat *= 2;
        },
};

ON_EITHER(PoisonPoint) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanBePoisoned(battler, opponent, MOVE_NONE))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_POISON_POINT> = {
    ON_EITHER_ABILITY(PoisonPoint),
};

template <>
constexpr Ability Impl<ABILITY_INNER_FOCUS> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_FOCUS_BLAST)
        return ACCURACY_ALWAYS_HITS;
    },
    .breakable = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MAGMA_ARMOR> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
        },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_FROSTBITE)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WATER_VEIL> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses3[battler] & STATUS3_AQUA_RING)

        gStatuses3[battler] |= STATUS3_AQUA_RING;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
        return TRUE;
    },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BURN)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MAGNET_PULL> = {
    .onTrap = +[](ON_TRAP) -> int { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); },
};

template <>
constexpr Ability Impl<ABILITY_SOUNDPROOF> = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsSoundMove(attacker, move))
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER)* immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RAIN_DISH> = {
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

template <>
constexpr Ability Impl<ABILITY_SAND_STREAM> = {
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

template <>
constexpr Ability Impl<ABILITY_PRESSURE> = {
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

template <>
constexpr Ability Impl<ABILITY_THICK_FAT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
        },
    .breakable = TRUE,
};

ON_EITHER(FlameBody) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanBeBurned(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_FLAME_BODY> = {
    ON_EITHER_ABILITY(FlameBody),
};

template <>
constexpr Ability Impl<ABILITY_KEEN_EYE> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HYPER_CUTTER> = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(IsMoveMakingContact(move, battler))
        return 1;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PICKUP> = {
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

template <>
constexpr Ability Impl<ABILITY_TRUANT> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (GetAbilityState(battler, ability))
            SetAbilityState(battler, ability, FALSE);
        else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
            SetAbilityState(battler, ability, TRUE);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_HUSTLE> = {
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MUL(1.4); },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK_NOT(IS_MOVE_STATUS(move))* accuracy *= .9;
        return ACCURACY_MULTIPLICATIVE;
    },
};

ON_EITHER(CuteCharm) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(CanInfatuate(battler, opponent))
    CHECK(Random() % 100 < 50)

    AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_CUTE_CHARM> = {
    ON_EITHER_ABILITY(CuteCharm),
};

template <>
constexpr Ability Impl<ABILITY_PLUS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            int partner = BATTLE_PARTNER(battler);
            if (!IsBattlerAlive(partner)) return;
            if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE) ||
                BattlerHasAbility(partner, ABILITY_POLARITY, FALSE))
                MUL(2.0);
        },
};

template <>
constexpr Ability Impl<ABILITY_MINUS> = {
    .onOffensiveMultiplier = Impl<ABILITY_PLUS>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_POLARITY> = {
    .onOffensiveMultiplier = Impl<ABILITY_PLUS>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_FORECAST> = {
    .onEntry = +[](ON_ENTRY) -> int { return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onWeather = +[](ON_WEATHER) -> int { return TryTransformAttacker(battler, ABILITY_BS_CALL); },
    .onEndTurn = +[](ON_END_TURN) -> int { return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
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

template <>
constexpr Ability Impl<ABILITY_STICKY_HOLD> = {
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SHED_SKIN> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_GUTS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
    .negatesBurnAtkDrop = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MARVEL_SCALE> = {
    .onStat =
        +[](ON_STAT) {
            if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_OVERGROW> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GRASS),
};

template <>
constexpr Ability Impl<ABILITY_BLAZE> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIRE),
};

template <>
constexpr Ability Impl<ABILITY_TORRENT> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_WATER),
};

template <>
constexpr Ability Impl<ABILITY_SWARM> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_BUG),
};

template <>
constexpr Ability Impl<ABILITY_ROCK_HEAD> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    },
    .breakable = TRUE,
    .noRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DROUGHT> = {
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

template <>
constexpr Ability Impl<ABILITY_ARENA_TRAP> = {
    .onTrap = +[](ON_TRAP) -> int { return IsBattlerGrounded(switchingBattler); },
};

template <>
constexpr Ability Impl<ABILITY_VITAL_SPIRIT> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIGHTING)
        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WHITE_SMOKE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

        int side = GET_BATTLER_SIDE(battler);
        gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
        gSideTimers[side].started.smokescreen = TRUE;
        gSideTimers[side].smokescreenBattler = battler;
        return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
    },
};

template <>
constexpr Ability Impl<ABILITY_SHELL_ARMOR> = {
    .onDefensiveMultiplier = Impl<ABILITY_BATTLE_ARMOR>.onDefensiveMultiplier,
    .onCrit = Impl<ABILITY_BATTLE_ARMOR>.onCrit,
    .onCritFor = Impl<ABILITY_BATTLE_ARMOR>.onCritFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_AIR_LOCK> = {
    .onEntry = Impl<ABILITY_CLOUD_NINE>.onEntry,
};

template <>
constexpr Ability Impl<ABILITY_TANGLED_FEET> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MOTOR_DRIVE> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RIVALRY> = {
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

template <>
constexpr Ability Impl<ABILITY_SNOW_CLOAK> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ANGER_POINT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_UNBURDEN> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
        },
};

template <>
constexpr Ability Impl<ABILITY_HEATPROOF> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        },
    .breakable = TRUE,
    .negatesBurnAtkDrop = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DRY_SKIN> = {
    .onAbsorb = Impl<ABILITY_WATER_ABSORB>.onAbsorb,
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !IsMagicGuardProtected(battler)) {
            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            BattleScriptPushCursorAndCallback(BattleScript_SolarPowerActivates);
            return TRUE;
        }

        return Impl<ABILITY_RAIN_DISH>.onEndTurn(DELEGATE_END_TURN);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(1.25);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DOWNLOAD> = {
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

template <>
constexpr Ability Impl<ABILITY_IRON_FIST> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsIronFistBoosted(battler, move)) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_ADAPTABILITY> = {
    .adaptability = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SKILL_LINK> = {
    .skillLink = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HYDRATION> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SOLAR_POWER> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_RAGING_STORM> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_QUICK_FEET> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_NORMALIZE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        },
    .onMoveType = +[](ON_MOVE_TYPE) -> int { return TYPE_NORMAL + 1; },
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_NORMAL) CHECK(*mod) CHECK(*mod < UQ_4_12(1.0))* mod = UQ_4_12(1.0);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SNIPER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (isCrit) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_MAGIC_GUARD> = {
    .magicGuard = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_NO_GUARD> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_ALWAYS_HITS; },
    .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
};

template <>
constexpr Ability Impl<ABILITY_STALL> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TECHNICIAN> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (basePower <= 60) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_LEAF_GUARD> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_MOLD_BREAKER> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); },
    .onMoldBreaker = +[](ON_MOLD_BREAKER) -> int { return TRUE; },
};

template <>
constexpr Ability Impl<ABILITY_SUPER_LUCK> = {
    .onCrit = +[](ON_CRIT) -> int { return 1; },
};

template <>
constexpr Ability Impl<ABILITY_AFTERMATH> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = 1;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ANTICIPATION> = {
    .onEntry = +[](ON_ENTRY) -> int {
        int side = GetBattlerSide(battler);
        int any = FALSE;

        for (int i = 0; i < gBattlersCount; i++) {
            if (IsBattlerAlive(i) && side != GetBattlerSide(i)) {
                for (int j = 0; j < MAX_MON_MOVES; j++) {
                    MoveEnum move = gBattleMons[i].moves[j];
                    Type moveType = gBattleMoves[move].type;
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

template <>
constexpr Ability Impl<ABILITY_FOREWARN> = {
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

template <>
constexpr Ability Impl<ABILITY_UNAWARE> = {
    .breakable = TRUE,
    .unaware = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TINTED_LENS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
        },
};

template <>
constexpr Ability Impl<ABILITY_FILTER> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SLOW_START> = {
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

template <>
constexpr Ability Impl<ABILITY_SCRAPPY> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
        CHECK(defType == TYPE_GHOST)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    },
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STORM_DRAIN> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_WATER,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ICE_BODY> = {
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

template <>
constexpr Ability Impl<ABILITY_SOLID_ROCK> = {
    .onDefensiveMultiplier = Impl<ABILITY_FILTER>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SNOW_WARNING> = {
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

template <>
constexpr Ability Impl<ABILITY_HONEY_GATHER> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK(Random() % 2)

        gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
        BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FRISK> = {
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

template <>
constexpr Ability Impl<ABILITY_RECKLESS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
        },
};

template <>
constexpr Ability Impl<ABILITY_MULTITYPE> = {
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FLOWER_GIFT> = {
    .onEntry = Impl<ABILITY_FORECAST>.onEntry,
    .onWeather = Impl<ABILITY_FORECAST>.onWeather,
    .onEndTurn = Impl<ABILITY_FORECAST>.onEndTurn,
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

template <>
constexpr Ability Impl<ABILITY_BAD_DREAMS> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
        return NO_ANNOUNCE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SHEER_FORCE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_CONTRARY> = {
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_UNNERVE> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); },
    .unnerve = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DEFEATIST> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK && statId != STAT_SPATK) return;
            if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
        },
};

template <>
constexpr Ability Impl<ABILITY_CURSED_BODY> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_HEALER> = {
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

template <>
constexpr Ability Impl<ABILITY_FRIEND_GUARD> = {
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WEAK_ARMOR> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(CanRaiseStat(battler, STAT_SPEED) || CanLowerStat(battler, STAT_DEF))

        if (gBattleMoves[move].effect == EFFECT_HIT_ESCAPE && CanBattlerSwitch(attacker))
            gRoundStructs[battler].disableEjectPack = TRUE;  // Set flag for target

        BattleScriptCall(BattleScript_WeakArmorActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LIGHT_METAL> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= 1.3;
        },
};

template <>
constexpr Ability Impl<ABILITY_MULTISCALE> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (BATTLER_MAX_HP(battler)) MUL(.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TOXIC_BOOST> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

int FlareBoostHandler(int battler, AbilityCallType callType) {
    CHECK(CanBeBurned(battler))
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

    InsertCorrectEndType(callType);
    gBattleMons[battler].status1 |= STATUS1_BURN;
    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
    MarkBattlerForControllerExec(battler);
    BattleScriptCall(BattleScript_FlareBoostRet);
    return TRUE;
}

template <>
constexpr Ability Impl<ABILITY_FLARE_BOOST> = {
    .onEntry = +[](ON_ENTRY) -> int { return FlareBoostHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onWeather = +[](ON_WEATHER) -> int { return FlareBoostHandler(battler, ABILITY_BS_CALL); },
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_SPATK) return;
            if (gBattleMons[battler].status1 & STATUS1_BURN) *stat *= 1.5;
        },
    .negatesBurnAtkDrop = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HARVEST> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK_NOT(gBattleStruct->changedItems[battler])
        CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

        BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_TELEPATHY> = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_ATTACKER_OR_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MOODY> = {
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

template <>
constexpr Ability Impl<ABILITY_OVERCOAT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.8);
        },
    .breakable = TRUE,
    .powderImmune = TRUE,
    .sandImmune = TRUE,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_POISON_TOUCH> = {
    .onAttacker = Impl<ABILITY_POISON_POINT>.onAttacker,
    .onDefender = Impl<ABILITY_POISON_POINT>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_REGENERATOR> = {
    .onExit = +[](ON_EXIT) -> int {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        BattleScriptCall(BattleScript_RegeneratorExits);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_BIG_PECKS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsMoveMakingContact(move, battler)) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_SAND_RUSH> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        },
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WONDER_SKIN> = {
    .fortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ANALYTIC> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_ILLUSION> = {
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

template <>
constexpr Ability Impl<ABILITY_IMPOSTER> = {
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

template <>
constexpr Ability Impl<ABILITY_INFILTRATOR> = {
    .onInfiltrate = +[](ON_INFILTRATE) -> InfiltrateType { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; },
};

template <>
constexpr Ability Impl<ABILITY_MUMMY> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_MOXIE> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_ATK); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_JUSTIFIED> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_DARK);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
};

template <>
constexpr Ability Impl<ABILITY_RATTLED> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
    .steadfast = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MAGIC_BOUNCE> = {
    .breakable = TRUE,
    .magicBounce = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SAP_SIPPER> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_GRASS);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_GRASS,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PRANKSTER> = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IS_MOVE_STATUS(move))
        return 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_SAND_FORCE> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        },
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_IRON_BARBS> = {
    .onDefender = Impl<ABILITY_ROUGH_SKIN>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_ZEN_MODE> = {
    .onEntry = Impl<ABILITY_FORECAST>.onEntry,
    .onEndTurn = Impl<ABILITY_FORECAST>.onEndTurn,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_VICTORY_STAR> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_TURBOBLAZE> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FIRE); },
    .onMoldBreaker = Impl<ABILITY_MOLD_BREAKER>.onMoldBreaker,
    .addsType = TYPE_FIRE,
};

template <>
constexpr Ability Impl<ABILITY_TERAVOLT> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ELECTRIC); },
    .onMoldBreaker = Impl<ABILITY_MOLD_BREAKER>.onMoldBreaker,
    .addsType = TYPE_ELECTRIC,
};

template <>
constexpr Ability Impl<ABILITY_AROMA_VEIL> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK))
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FLOWER_VEIL> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CHEEK_POUCH> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PROTEAN> = {
    .onBeforeAttack = +[](ON_BEFORE_ATTACK) -> int {
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, moveType))
        CHECK(move != MOVE_STRUGGLE)
        SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        BattleScriptCall(BattleScript_ProteanActivates);
        return TRUE;
    },
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FUR_COAT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BULLETPROOF> = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER)* immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STRONG_JAW> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_REFRIGERATE> = {
    ATE_ABILITY(TYPE_ICE),
};

template <>
constexpr Ability Impl<ABILITY_SWEET_VEIL> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STANCE_CHANGE> = {
    .onBeforeAttack = +[](ON_BEFORE_ATTACK) -> int {
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

template <>
constexpr Ability Impl<ABILITY_GALE_WINGS> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_FLYING),
};

template <>
constexpr Ability Impl<ABILITY_MEGA_LAUNCHER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
        },
    .megaLauncherBoost = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GRASS_PELT> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_FLOWER_NECKLACE> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_TOUGH_CLAWS> = {
    .onOffensiveMultiplier = Impl<ABILITY_BIG_PECKS>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_PIXILATE> = {
    ATE_ABILITY(TYPE_FAIRY),
};

template <>
constexpr Ability Impl<ABILITY_GOOEY> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
        CHECK(IsMoveMakingContact(move, attacker))

        BattleScriptCall(BattleScript_GooeyActivates);
        gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_AERILATE> = {
    ATE_ABILITY(TYPE_FLYING),
};

template <>
constexpr Ability Impl<ABILITY_PARENTAL_BOND> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType { return PARENTAL_BOND_HYPER_AGGRESSIVE; },
    .resistsFortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DARK_AURA> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA); },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType != TYPE_DARK) return;

            if (IsAbilityOnField(FALSE, [](AbilityEnum ability) -> int { return gAbilities[ability].auraBreak; }))
                MUL(.75);
            else
                MUL(1.33);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

template <>
constexpr Ability Impl<ABILITY_FAIRY_AURA> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA); },
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType != TYPE_FAIRY) return;
            if (IsAbilityOnField(FALSE, [](AbilityEnum ability) -> int { return gAbilities[ability].auraBreak; }))
                MUL(.75);
            else
                MUL(1.33);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

template <>
constexpr Ability Impl<ABILITY_AURA_BREAK> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); },
    .breakable = TRUE,
    .auraBreak = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PRIMORDIAL_SEA> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DESOLATE_LAND> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DELTA_STREAM> = {
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

template <>
constexpr Ability Impl<ABILITY_STAMINA> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

u32 CanBattlerBeForceSwitched(int battler) {
    CHECK(IsBattlerAlive(battler))
    CHECK(CanBattlerSwitch(battler))
    CHECK(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
    CHECK(CountUsablePartyMons(battler))
    return TRUE;
}

template <>
constexpr Ability Impl<ABILITY_WIMP_OUT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(TestSheerForceFlag(attacker, gCurrentMove))
        CHECK(CanBattlerBeForceSwitched(battler))

        gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_EMERGENCY_EXIT> = {
    .onDefender = Impl<ABILITY_WIMP_OUT>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_WATER_COMPACTION> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_MERCILESS> = {
    .onCrit = +[](ON_CRIT) -> int {
        if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
        if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
        if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
        return 0;
    },
};

template <>
constexpr Ability Impl<ABILITY_SHIELDS_DOWN> = {
    .onEntry = Impl<ABILITY_FORECAST>.onEntry,
    .onEndTurn = Impl<ABILITY_FORECAST>.onEndTurn,
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
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
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

template <>
constexpr Ability Impl<ABILITY_STAKEOUT> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
        },
};

template <>
constexpr Ability Impl<ABILITY_WATER_BUBBLE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) MUL(2.0);
        },
    .onDefensiveMultiplier = Impl<ABILITY_HEATPROOF>.onDefensiveMultiplier,
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BURN)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STEELWORKER> = {
    ATE_ABILITY(TYPE_STEEL),
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BERSERK> = {
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

template <>
constexpr Ability Impl<ABILITY_SLUSH_RUSH> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LONG_REACH> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move) && !gBattleMoves[move].contact) MUL(1.2);
        },
};

template <>
constexpr Ability Impl<ABILITY_LIQUID_VOICE> = {
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

template <>
constexpr Ability Impl<ABILITY_TRIAGE> = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IsHealingMoveEffect(gBattleMoves[move].effect))
        return 3;
    },
};

template <>
constexpr Ability Impl<ABILITY_GALVANIZE> = {
    ATE_ABILITY(TYPE_ELECTRIC),
};

template <>
constexpr Ability Impl<ABILITY_SURGE_SURFER> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_SCHOOLING> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gBattleMons[battler].level >= 20)
        return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(gBattleMons[battler].level >= 20)
        return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

static int DisguiseReformHandler(int battler, AbilityCallType callType) {
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
template <>
constexpr Ability Impl<ABILITY_DISGUISE> = {
    .onEntry = +[](ON_ENTRY) -> int { return DisguiseReformHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
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
    .onWeather = +[](ON_WEATHER) -> int { return DisguiseReformHandler(battler, ABILITY_BS_CALL); },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BATTLE_BOND> = {
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

template <>
constexpr Ability Impl<ABILITY_POWER_CONSTRUCT> = {
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

template <>
constexpr Ability Impl<ABILITY_CORROSION> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_POISON)
        CHECK(defType == TYPE_STEEL)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
    .onCanStatusType = +[](ON_CAN_STATUS_TYPE) -> int {
        CHECK(status & CHECK_POISON)
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_COMATOSE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COMATOSE;
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceStatusAbility);
        return TRUE;
    },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .alwaysSleeping = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_QUEENLY_MAJESTY> = {
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

template <>
constexpr Ability Impl<ABILITY_INNARDS_OUT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))

        gBattleMoveDamage = gTurnStructs[battler].dmg;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DANCER> = {
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(IsDance(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
};

template <>
constexpr Ability Impl<ABILITY_BATTERY> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
};

template <>
constexpr Ability Impl<ABILITY_FLUFFY> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DAZZLING> = {
    .onImmune = Impl<ABILITY_QUEENLY_MAJESTY>.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SOUL_HEART> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
};

template <>
constexpr Ability Impl<ABILITY_TANGLING_HAIR> = {
    .onDefender = Impl<ABILITY_GOOEY>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_RECEIVER> = {
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

template <>
constexpr Ability Impl<ABILITY_POWER_OF_ALCHEMY> = {
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

template <>
constexpr Ability Impl<ABILITY_BEAST_BOOST> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, GetHighestStatId(battler, FALSE)); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_RKS_SYSTEM> = {
    .onBeforeAttack = Impl<ABILITY_PROTEAN>.onBeforeAttack,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .adaptability = TRUE,
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ELECTRIC_SURGE> = {
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

template <>
constexpr Ability Impl<ABILITY_PSYCHIC_SURGE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_PSYCHIC,
};

template <>
constexpr Ability Impl<ABILITY_MISTY_SURGE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_MISTY,
};

template <>
constexpr Ability Impl<ABILITY_GRASSY_SURGE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_GRASSY,
};

template <>
constexpr Ability Impl<ABILITY_SHADOW_SHIELD> = {
    .onDefensiveMultiplier = Impl<ABILITY_MULTISCALE>.onDefensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_PRISM_ARMOR> = {
    .onDefensiveMultiplier = Impl<ABILITY_FILTER>.onDefensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_NEUROFORCE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.35);
        },
};

template <>
constexpr Ability Impl<ABILITY_INTREPID_SWORD> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_ATK))

        SetStatChanger(STAT_ATK, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DAUNTLESS_SHIELD> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LIBERO> = {
    .onBeforeAttack = Impl<ABILITY_PROTEAN>.onBeforeAttack,
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_COTTON_DOWN> = {
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

template <>
constexpr Ability Impl<ABILITY_MIRROR_ARMOR> = {
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GULP_MISSILE> = {
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
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_STEAM_ENGINE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPEED))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

        SetStatChanger(STAT_SPEED, 12);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_PUNK_ROCK> = {
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

template <>
constexpr Ability Impl<ABILITY_SAND_SPIT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler)) CHECK_NOT(gBattleWeather & WEATHER_SANDSTORM_ANY) if (gBattleWeather & WEATHER_PRIMAL_ANY) {
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

template <>
constexpr Ability Impl<ABILITY_ICE_SCALES> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

int IceFaceReformHandler(int battler, AbilityCallType callType) {
    CHECK(gBattleMons[battler].species == SPECIES_EISCUE_NOICE_FACE)
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
    CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

    InsertCorrectEndType(callType);
    UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_EISCUE);
    gBattleMons[battler].species = SPECIES_EISCUE;
    BattleScriptCall(BattleScript_AttackerFormChange);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_ICE_FACE> = {
    .onEntry = +[](ON_ENTRY) -> int { return IceFaceReformHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDisguise = +[](ON_DISGUISE) -> SpeciesEnum { return gBattleMons[battler].species == SPECIES_EISCUE ? SPECIES_EISCUE_NOICE_FACE : SPECIES_NONE; },
    .onWeather = +[](ON_WEATHER) -> int { return IceFaceReformHandler(battler, ABILITY_BS_CALL); },
    .breakable = TRUE,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_POWER_SPOT> = {
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
template <>
constexpr Ability Impl<ABILITY_MIMICRY> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerAlive(battler))

        return HandleMimicry(battler, ability, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    },
    .onTerrain = +[](ON_TERRAIN) -> int {
        CHECK(IsBattlerAlive(battler))

        return HandleMimicry(battler, ability, ABILITY_BS_CALL);
    },
};

template <>
constexpr Ability Impl<ABILITY_SCREEN_CLEANER> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryRemoveScreens(battler))

        return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
    },
};

template <>
constexpr Ability Impl<ABILITY_STEELY_SPIRIT> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_STEEL) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_PERISH_BODY> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_WANDERING_SPIRIT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_GORILLA_TACTICS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_NEUTRALIZING_GAS> = {
    .unsuppressable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PASTEL_VEIL> = {
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

template <>
constexpr Ability Impl<ABILITY_HUNGER_SWITCH> = {
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

template <>
constexpr Ability Impl<ABILITY_CURIOUS_MEDICINE> = {
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

template <>
constexpr Ability Impl<ABILITY_TRANSISTOR> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_DRAGONS_MAW> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_DRAGON) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_CHILLING_NEIGH> = {
    .onBattlerFaints = Impl<ABILITY_MOXIE>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_GRIM_NEIGH> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_SPATK); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_AS_ONE_ICE_RIDER> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE); },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(Impl<ABILITY_CHILLING_NEIGH>.onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_CHILLING_NEIGH;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .unnerve = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_AS_ONE_SHADOW_RIDER> = {
    .onEntry = Impl<ABILITY_AS_ONE_ICE_RIDER>.onEntry,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(Impl<ABILITY_GRIM_NEIGH>.onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_GRIM_NEIGH;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .unnerve = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CHLOROPLAST> = {
    .chloroplast = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WHITEOUT> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PYROMANCY> = {
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
        },
};

template <>
constexpr Ability Impl<ABILITY_KEEN_EDGE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE)) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_PRISM_SCALES> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.7);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_POWER_FISTS> = {
    .onOffensiveMultiplier = Impl<ABILITY_IRON_FIST>.onOffensiveMultiplier,
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(IsIronFistBoosted(battler, move))
        return STAT_SPDEF;
    },
};

template <>
constexpr Ability Impl<ABILITY_SAND_SONG> = {
    .onOffensiveMultiplier = Impl<ABILITY_LIQUID_VOICE>.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_GROUND + 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_RAMPAGE> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        SetAbilityState(battler, ability, TRUE);
        gVolatileStructs[battler].rechargeTimer = 0;
        gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
        return FALSE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_VENGEANCE> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GHOST),
};

template <>
constexpr Ability Impl<ABILITY_BLITZ_BOXER> = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(IsIronFistBoosted(battler, move))
        CHECK(BATTLER_MAX_HP(battler));
        return 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_ANTARCTIC_BIRD> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_IMMOLATE> = {
    ATE_ABILITY(TYPE_FIRE),
};

template <>
constexpr Ability Impl<ABILITY_CRYSTALLIZE> = {
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

template <>
constexpr Ability Impl<ABILITY_ELECTROCYTES> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ELECTRIC) MUL(1.25);
        },
};

template <>
constexpr Ability Impl<ABILITY_AERODYNAMICS> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FLYING);
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CHRISTMAS_SPIRIT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
        },
    .breakable = TRUE,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_EXPLOIT_WEAKNESS> = {
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

template <>
constexpr Ability Impl<ABILITY_GROUND_SHOCK> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_GROUND)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(.5);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ANCIENT_IDOL> = {
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; },
};

template <>
constexpr Ability Impl<ABILITY_MYSTIC_POWER> = {
    .onStab = +[](ON_STAB) -> int { return TRUE; },
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PERFECTIONIST> = {
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

template <>
constexpr Ability Impl<ABILITY_GROWING_TOOTH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_AttackBoostActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_INFLATABLE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
        BattleScriptCall(BattleScript_InflatableActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_AURORA_BOREALIS> = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_ICE; },
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_AVENGER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_LETS_ROLL> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LOUD_BANG> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeConfused(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
    },
};

template <>
constexpr Ability Impl<ABILITY_LEAD_COAT> = {
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

template <>
constexpr Ability Impl<ABILITY_AMPHIBIOUS> = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_DRENCH)
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_GROUNDED> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GROUND); },
    .addsType = TYPE_GROUND,
};

template <>
constexpr Ability Impl<ABILITY_EARTHBOUND> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_GROUND),
};

template <>
constexpr Ability Impl<ABILITY_FIGHT_SPIRIT> = {
    ATE_ABILITY(TYPE_FIGHTING),
};

template <>
constexpr Ability Impl<ABILITY_FELINE_PROWESS> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK) *stat *= 2;
        },
};

template <>
constexpr Ability Impl<ABILITY_PURE_POWER> = {
    .onStat = Impl<ABILITY_FELINE_PROWESS>.onStat,
};

template <>
constexpr Ability Impl<ABILITY_COIL_UP> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

        gStatuses4[battler] |= STATUS4_COILED;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
        return TRUE;
    },
    .coilUp = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FOSSILIZED> = {
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

template <>
constexpr Ability Impl<ABILITY_MAGICAL_DUST> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_PSYCHIC))

        gBattleMons[attacker].type3 = TYPE_PSYCHIC;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[attacker].type3);
        BattleScriptCall(BattleScript_AttackerBecameTheType);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DREAMCATCHER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            for (int i = 0; i < gBattlersCount; i++) {
                if (IsBattlerAlive(i) && gBattleMons[i].status1 & STATUS1_SLEEP) {
                    FILTER_NOT(gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability && gQueuedExtraAttackData[0].target == i)
                    MUL(2.0);
                    return;
                }
            }
        },
    .onPreemptAction = +[](ON_PREEMPT_ACTION) -> int {
        CHECK(gBattleMons[turnBattler].status1 & STATUS1_SLEEP)
        return UseTurnAttackAsPursuit(DELEGATE_PREEMPT_ACTION);
    },
};

template <>
constexpr Ability Impl<ABILITY_NOCTURNAL> = {
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

template <>
constexpr Ability Impl<ABILITY_SELF_SUFFICIENT> = {
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

template <>
constexpr Ability Impl<ABILITY_TECTONIZE> = {
    ATE_ABILITY(TYPE_GROUND),
};

template <>
constexpr Ability Impl<ABILITY_ICE_AGE> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ICE); },
    .addsType = TYPE_ICE,
};

template <>
constexpr Ability Impl<ABILITY_HALF_DRAKE> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_DRAGON); },
    .addsType = TYPE_DRAGON,
};

template <>
constexpr Ability Impl<ABILITY_AQUATIC> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_WATER); },
    .addsType = TYPE_WATER,
};

template <>
constexpr Ability Impl<ABILITY_LIQUIFIED> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) RESISTANCE(2);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DRAGONFLY> = {
    .onEntry = Impl<ABILITY_HALF_DRAKE>.onEntry,
    .breakable = TRUE,
    .levitate = TRUE,
    .addsType = TYPE_DRAGON,
};

template <>
constexpr Ability Impl<ABILITY_DRAGONSLAYER> = {
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

template <>
constexpr Ability Impl<ABILITY_MOUNTAINEER> = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (moveType == TYPE_ROCK) *mod = 0;
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .stealthRockImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HYDRATE> = {
    ATE_ABILITY(TYPE_WATER),
};

template <>
constexpr Ability Impl<ABILITY_METALLIC> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_STEEL); },
    .addsType = TYPE_STEEL,
};

template <>
constexpr Ability Impl<ABILITY_PERMAFROST> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PRIMAL_ARMOR> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RAGING_BOXER> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(IsIronFistBoosted(battler, move))
        return PARENTAL_BOND_PRIMAL_MAW;
    },
};

template <>
constexpr Ability Impl<ABILITY_AIR_BLOWER> = {
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

template <>
constexpr Ability Impl<ABILITY_JUGGERNAUT> = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
        },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SHORT_CIRCUIT> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_ELECTRIC),
};

template <>
constexpr Ability Impl<ABILITY_MAJESTIC_BIRD> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_PHANTOM> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GHOST); },
    .addsType = TYPE_GHOST,
};

template <>
constexpr Ability Impl<ABILITY_INTOXICATE> = {
    ATE_ABILITY(TYPE_POISON),
};

template <>
constexpr Ability Impl<ABILITY_IMPENETRABLE> = {
    .magicGuard = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HYPNOTIST> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_HYPNOSIS);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    },
};

template <>
constexpr Ability Impl<ABILITY_OVERWHELM> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod)* mod = UQ_4_12(1.0);
        return TRUE;
    },
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SCARE> = {
    .onEntry = UseIntimidateClone,
};

template <>
constexpr Ability Impl<ABILITY_MAJESTIC_MOTH> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SOUL_EATER> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK_NOT(BATTLER_MAX_HP(battler));
        CHECK(CanBattlerHeal(battler));
        BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

ON_EITHER(SoulLinker) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(IsBattlerAlive(battler))
    CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER))
    CHECK(move != MOVE_PAIN_SPLIT)

    BattleScriptCall(BattleScript_AttackerSoulLinker);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_SOUL_LINKER> = {
    ON_EITHER_ABILITY(SoulLinker),
};

template <>
constexpr Ability Impl<ABILITY_SWEET_DREAMS> = {
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

template <>
constexpr Ability Impl<ABILITY_BAD_LUCK> = {
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

template <>
constexpr Ability Impl<ABILITY_HAUNTED_SPIRIT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_GHOST))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMons[attacker].status2 |= STATUS2_CURSED;
        BattleScriptCall(BattleScript_HauntedSpiritActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ELECTRIC_BURST> = {
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

template <>
constexpr Ability Impl<ABILITY_RAW_WOOD> = {
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

template <>
constexpr Ability Impl<ABILITY_SOLENOGLYPHS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
};

template <>
constexpr Ability Impl<ABILITY_SPIDER_LAIR> = {
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

template <>
constexpr Ability Impl<ABILITY_FATAL_PRECISION> = {
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

template <>
constexpr Ability Impl<ABILITY_FORT_KNOX> = {
    .fortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SEAWEED> = {
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

template <>
constexpr Ability Impl<ABILITY_PSYCHIC_MIND> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_PSYCHIC),
};

template <>
constexpr Ability Impl<ABILITY_POISON_ABSORB> = {
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

template <>
constexpr Ability Impl<ABILITY_SCAVENGER> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_TWISTED_DIMENSION> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

        gFieldTimers.started.trickRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_MULTI_HEADED> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
        if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
        return MULTIHIT_SINGLE;
    },
    .resistsFortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_NORTH_WIND> = {
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

template <>
constexpr Ability Impl<ABILITY_OVERCHARGE> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_ELECTRIC)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
    .onCanStatusType = +[](ON_CAN_STATUS_TYPE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_VIOLENT_RUSH> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
    },
};

template <>
constexpr Ability Impl<ABILITY_FLAMING_SOUL> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_FIRE),
};

template <>
constexpr Ability Impl<ABILITY_SAGE_POWER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_BONE_ZONE> = {
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

template <>
constexpr Ability Impl<ABILITY_WEATHER_CONTROL> = {
    .onImmune = Impl<ABILITY_DELTA_STREAM>.onImmune,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SPEED_FORCE> = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
        },
};

template <>
constexpr Ability Impl<ABILITY_SEA_GUARDIAN> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        SetStatChanger(stat, 1);
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_MOLTEN_DOWN> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(defType == TYPE_ROCK)
        *mod = UQ_4_12(2.0);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_HYPER_AGGRESSIVE> = {
    .onParentalBond = Impl<ABILITY_PARENTAL_BOND>.onParentalBond,
};

template <>
constexpr Ability Impl<ABILITY_FLOCK> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FLYING),
};

template <>
constexpr Ability Impl<ABILITY_FIELD_EXPLORER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_STRIKER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsStrikerBoosted(battler, move)) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_FROZEN_SOUL> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_ICE),
};

template <>
constexpr Ability Impl<ABILITY_PREDATOR> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_LOOTER> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_LUNAR_ECLIPSE> = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; },
    .onAccuracy = Impl<ABILITY_HYPNOTIST>.onAccuracy,
};

template <>
constexpr Ability Impl<ABILITY_SOLAR_FLARE> = {
    .onOffensiveMultiplier = Impl<ABILITY_IMMOLATE>.onOffensiveMultiplier,
    .onMoveType = Impl<ABILITY_IMMOLATE>.onMoveType,
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FIRE; },
    .chloroplast = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_POWER_CORE> = {
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; },
};

template <>
constexpr Ability Impl<ABILITY_SIGHTING_SYSTEM> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority { return ACCURACY_HITS_IF_POSSIBLE; },
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMoves[move].accuracy)
        CHECK(gBattleMoves[move].accuracy < 80);
        return -3;
    },
};

template <>
constexpr Ability Impl<ABILITY_BAD_COMPANY> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_OPPORTUNIST> = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2)
        return 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_GIANT_WINGS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].airBased) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_MOMENTUM> = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
        },
};

template <>
constexpr Ability Impl<ABILITY_GRIP_PINCER> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(gBattlerTarget))
        CHECK(IsBattlerAlive(battler))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK_NOT(gBattleMons[target].status2 & STATUS2_WRAPPED)
        CHECK(Random() % 2)

        SetOnMoveEffectReactionFlags(battler, target, MOVE_EFFECT_WRAP);
        gBattleMons[target].status2 |= STATUS2_WRAPPED;
        gVolatileStructs[target].wrapTurns = WrapDuration(battler);

        gBattleStruct->wrappedMove[target] = move;
        gBattleStruct->wrappedBy[target] = battler;
        BattleScriptCall(BattleScript_GripPincerActivated);
        return TRUE;
    },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(gBattleMons[target].status2 & STATUS2_WRAPPED)
        return ACCURACY_ALWAYS_HITS;
    },
};

template <>
constexpr Ability Impl<ABILITY_BIG_LEAVES> = {
    .onEndTurn = +[](ON_END_TURN) -> int { return Impl<ABILITY_HARVEST>.onEndTurn(DELEGATE_END_TURN) | Impl<ABILITY_LEAF_GUARD>.onEndTurn(DELEGATE_END_TURN); },
    .onStat =
        +[](ON_STAT) {
            Impl<ABILITY_SOLAR_POWER>.onStat(DELEGATE_STAT);
            Impl<ABILITY_CHLOROPHYLL>.onStat(DELEGATE_STAT);
        },
    .chloroplast = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PRECISE_FIST> = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(IsIronFistBoosted(battler, move))
        return 1;
    },
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
        },
};

template <>
constexpr Ability Impl<ABILITY_DEADEYE> = {
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

template <>
constexpr Ability Impl<ABILITY_ARTILLERY> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsMegaLauncherBoosted(battler, move))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

template <>
constexpr Ability Impl<ABILITY_AMPLIFIER> = {
    .onOffensiveMultiplier = Impl<ABILITY_PUNK_ROCK>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_ICE_DEW> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ICE);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_ICE,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUN_WORSHIP> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_POLLINATE> = {
    ATE_ABILITY(TYPE_BUG),
};

template <>
constexpr Ability Impl<ABILITY_VOLCANO_RAGE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_COLD_REBOUND> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LOW_BLOW> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FEINT_ATTACK, 40); },
};

template <>
constexpr Ability Impl<ABILITY_SPECTRALIZE> = {
    ATE_ABILITY(TYPE_GHOST),
};

template <>
constexpr Ability Impl<ABILITY_SPECTRAL_SHROUD> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleStruct->ateBoost[battler])
        CHECK(moveType == TYPE_GHOST)
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
    .onOffensiveMultiplier = Impl<ABILITY_SPECTRALIZE>.onOffensiveMultiplier,
    .onMoveType = Impl<ABILITY_SPECTRALIZE>.onMoveType,
};

template <>
constexpr Ability Impl<ABILITY_DISCIPLINE> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_THUNDERCALL> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
    },
};

template <>
constexpr Ability Impl<ABILITY_MARINE_APEX> = {
    .onInfiltrate = Impl<ABILITY_INFILTRATOR>.onInfiltrate,
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_MIGHTY_HORN> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hornBased) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_HARDENED_SHEATH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(gBattleMoves[move].hornBased)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptCall(BattleScript_AttackBoostActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ARCTIC_FUR> = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.65); },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LETHARGY> = {
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

template <>
constexpr Ability Impl<ABILITY_IRON_BARRAGE> = {
    .onOffensiveMultiplier = Impl<ABILITY_MEGA_LAUNCHER>.onOffensiveMultiplier,
    .onAccuracy = Impl<ABILITY_SIGHTING_SYSTEM>.onAccuracy,
    .onPriority = Impl<ABILITY_SIGHTING_SYSTEM>.onPriority,
    .megaLauncherBoost = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STEEL_BARREL> = {
    .onStatusImmune = Impl<ABILITY_ROCK_HEAD>.onStatusImmune,
    .noRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PYRO_SHELLS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsMegaLauncherBoosted(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_FUNGAL_INFECTION> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED)
        CHECK(IsMoveMakingContact(move, battler))

        gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_PARRY> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
        return FALSE;
    },
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.8); },
};

template <>
constexpr Ability Impl<ABILITY_SCRAPYARD> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LOOSE_QUILLS> = {
    .onDefender = Impl<ABILITY_SCRAPYARD>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_TOXIC_DEBRIS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

        BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ROUNDHOUSE> = {
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

template <>
constexpr Ability Impl<ABILITY_MINERALIZE> = {
    ATE_ABILITY(TYPE_ROCK),
};

template <>
constexpr Ability Impl<ABILITY_LOOSE_ROCKS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

        BattleScriptCall(BattleScript_DefenderSetsStealthRock);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SPINNING_TOP> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_RETRIBUTION_BLOW> = {
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

template <>
constexpr Ability Impl<ABILITY_FEARMONGER> = {
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
};

template <>
constexpr Ability Impl<ABILITY_FIRES_WRATH> = {
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeBurned(target))
        CHECK_NOT(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
};

template <>
constexpr Ability Impl<ABILITY_TOXIC_SPILL> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(getMonotypeChampType() == TYPE_POISON)

        CHECK_NOT(CheckAbilityWasAnnounced(battler, ABILITY_TOXIC_SPILL))
        CHECK_NOT(CheckAbilityWasAnnounced(battler, ABILITY_TRASH_HEAP))

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

template <>
constexpr Ability Impl<ABILITY_DESERT_CLOAK> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY))
        return TRUE;
    },
    .onStatusImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DRACONIZE> = {
    ATE_ABILITY(TYPE_DRAGON),
};

template <>
constexpr Ability Impl<ABILITY_PRETTY_PRINCESS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (!IsUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_SELF_REPAIR> = {
    .onEndTurn = Impl<ABILITY_SELF_SUFFICIENT>.onEndTurn,
    .onExit = Impl<ABILITY_NATURAL_CURE>.onExit,
};

template <>
constexpr Ability Impl<ABILITY_ELECTROMORPHOSIS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ATOMIC_BURST> = {
    .onDefender = Impl<ABILITY_ELECTROMORPHOSIS>.onDefender,
    ATE_ABILITY(TYPE_ELECTRIC),
};

template <>
constexpr Ability Impl<ABILITY_HELLBLAZE> = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIRE),
};

template <>
constexpr Ability Impl<ABILITY_RIPTIDE> = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_WATER),
};

template <>
constexpr Ability Impl<ABILITY_FOREST_RAGE> = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GRASS),
};

template <>
constexpr Ability Impl<ABILITY_PRIMAL_MAW> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        return PARENTAL_BOND_PRIMAL_MAW;
    },
};

template <>
constexpr Ability Impl<ABILITY_SWEEPING_EDGE> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE))
        return ACCURACY_HITS_IF_POSSIBLE;
    },
};

template <>
constexpr Ability Impl<ABILITY_GIFTED_MIND> = {
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

template <>
constexpr Ability Impl<ABILITY_HYDRO_CIRCUIT> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(moveType == TYPE_WATER)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
    .onOffensiveMultiplier = Impl<ABILITY_TRANSISTOR>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_EQUINOX> = {
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

template <>
constexpr Ability Impl<ABILITY_ABSORBANT> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED)
        CHECK(gBattleMoves[move].effect == EFFECT_ABSORB || gBattleMoves[move].effect == EFFECT_DREAM_EATER)

        gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_CLUELESS> = {
    .onEntry = Impl<ABILITY_CLOUD_NINE>.onEntry,
    .unsuppressable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CHEATING_DEATH> = {
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

template <>
constexpr Ability Impl<ABILITY_CHEAP_TACTICS> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SCRATCH, 0); },
};

template <>
constexpr Ability Impl<ABILITY_COWARD> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        SetSingleUseAbilityCounter(battler, ability, TRUE);
        gRoundStructs[battler].protectedThisTurn = TRUE;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
        return TRUE;
    },
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_VOLT_RUSH> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_ELECTRIC),
};

template <>
constexpr Ability Impl<ABILITY_DUNE_TERROR> = {
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

template <>
constexpr Ability Impl<ABILITY_INFERNAL_RAGE> = {
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

template <>
constexpr Ability Impl<ABILITY_DUAL_WIELD> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(IsMegaLauncherBoosted(battler, move) || DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE));
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

template <>
constexpr Ability Impl<ABILITY_ELEMENTAL_CHARGE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
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

template <>
constexpr Ability Impl<ABILITY_AMBUSH> = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gVolatileStructs[battler].isFirstTurn)
        return ALWAYS_CRIT;
    },
};

template <>
constexpr Ability Impl<ABILITY_ATLAS> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_RADIANCE> = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(moveType == TYPE_DARK);
        *immunityScript = BattleScript_RadianceProtected;
        return TRUE;
    },
    .onAccuracy = Impl<ABILITY_ILLUMINATE>.onAccuracy,
    .onImmuneFor = APPLY_ON_ANY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_JAWS_OF_CARNAGE> = {
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

template <>
constexpr Ability Impl<ABILITY_ANGELS_WRATH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        switch (move) {
            case MOVE_TACKLE: {
                CHECK(ShouldApplyOnHitEffect(target))
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
                CHECK(ShouldApplyOnHitEffect(target))
                CHECK_NOT(gBattleMons[target].status2 & STATUS2_ESCAPE_PREVENTION)
                CHECK_NOT(gBattleMons[target].statStages[STAT_SPEED] == MIN_STAT_STAGE)

                gBattleMons[target].statStages[STAT_SPEED] = MIN_STAT_STAGE;
                gBattleMons[target].status2 |= (STATUS2_ESCAPE_PREVENTION);
                BattleScriptCall(BattleScript_AngelsWrath_Effect_Electroweb);
                return TRUE;
            }

            case MOVE_BUG_BITE: {
                CHECK(ShouldApplyOnHitEffect(battler))
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
    .onCanStatusType = +[](ON_CAN_STATUS_TYPE) -> int {
        CHECK(status & CHECK_POISON)
        CHECK(move == MOVE_POISON_STING)
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_PRISMATIC_FUR> = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.5); },
    .onBeforeAttack = +[](ON_BEFORE_ATTACK) -> int {
        if (battler == attacker && Impl<ABILITY_PROTEAN>.onBeforeAttack(DELEGATE_BEFORE_ATTACK)) return TRUE;
        return Impl<ABILITY_COLOR_CHANGE>.onBeforeAttack(DELEGATE_BEFORE_ATTACK);
    },
    .onBeforeAttackFor = APPLY_ON_ATTACKER_OR_TARGET,
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SHOCKING_JAWS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
};

template <>
constexpr Ability Impl<ABILITY_FAE_HUNTER> = {
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

template <>
constexpr Ability Impl<ABILITY_GRAVITY_WELL> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_EVAPORATE> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_EVAPORATE;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LUMBERJACK> = {
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

template <>
constexpr Ability Impl<ABILITY_WELL_BAKED_BODY> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE);
        *statId = STAT_DEF;
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
    .absorbUp2 = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FURNACE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        CHECK(gSideTimers[GetBattlerSide(battler)].stealthRockType == TYPE_ROCK)
        CHECK(IsBattlerAlive(battler))
        CHECK(ChangeStatBuffs(battler, 2, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 2);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ROCKY_PAYLOAD> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_EARTH_EATER> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_GROUND)
        return ABSORB_RESULT_HEAL;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LINGERING_AROMA> = {
    .onDefender = Impl<ABILITY_MUMMY>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_FAIRY_TALE> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FAIRY); },
    .addsType = TYPE_FAIRY,
};

template <>
constexpr Ability Impl<ABILITY_RAGING_MOTH> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(moveType == TYPE_FIRE)
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

template <>
constexpr Ability Impl<ABILITY_ADRENALINE_RUSH> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int { return MoxieClone(battler, STAT_SPEED); },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_ARCHMAGE> = {
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

template <>
constexpr Ability Impl<ABILITY_CRYOMANCY> = {
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
        },
};

template <>
constexpr Ability Impl<ABILITY_PHANTOM_PAIN> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        CHECK(moveType == TYPE_GHOST)
        CHECK(defType == TYPE_NORMAL)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_PURGATORY> = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GHOST),
};

template <>
constexpr Ability Impl<ABILITY_EMANATE> = {
    ATE_ABILITY(TYPE_PSYCHIC),
};

template <>
constexpr Ability Impl<ABILITY_KUNOICHI_BLADE> = {
    .onOffensiveMultiplier = Impl<ABILITY_TECHNICIAN>.onOffensiveMultiplier,
    .skillLink = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MONKEY_BUSINESS> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TICKLE, 0); },
};

template <>
constexpr Ability Impl<ABILITY_COMBAT_SPECIALIST> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_IRON_FIST>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_STRIKER>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

template <>
constexpr Ability Impl<ABILITY_JUNGLES_GUARD> = {
    .onStatusImmune = Impl<ABILITY_FLOWER_VEIL>.onStatusImmune,
    .onStatusImmuneFor = Impl<ABILITY_FLOWER_VEIL>.onStatusImmuneFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HUNTERS_HORN> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onOffensiveMultiplier = Impl<ABILITY_MIGHTY_HORN>.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_PIXIE_POWER> = {
    .onEntry = Impl<ABILITY_FAIRY_AURA>.onEntry,
    .onOffensiveMultiplier = Impl<ABILITY_FAIRY_AURA>.onOffensiveMultiplier,
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onOffensiveMultiplierFor = APPLY_ON_ANY,
};

template <>
constexpr Ability Impl<ABILITY_PLASMA_LAMP> = {
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

template <>
constexpr Ability Impl<ABILITY_MAGMA_EATER> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onTypeEffectiveness = Impl<ABILITY_MOLTEN_DOWN>.onTypeEffectiveness,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_SUPER_HOT_GOO> = {
    .onAttacker = Impl<ABILITY_FLAME_BODY>.onAttacker,
    .onDefender =
        +[](ON_DEFENDER) -> int { return Impl<ABILITY_GOOEY>.onDefender(DELEGATE_DEFENDER) | Impl<ABILITY_FLAME_BODY>.onDefender(DELEGATE_DEFENDER); },
};

template <>
constexpr Ability Impl<ABILITY_NIKA> = {
    .onOffensiveMultiplier = Impl<ABILITY_IRON_FIST>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_ARCHER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].arrowBased) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_SUPER_SLAMMER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hammerBased) MUL(1.3);
        },
};

template <>
constexpr Ability Impl<ABILITY_INVERSE_ROOM> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

        gFieldTimers.started.inverseRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
        gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FROST_BURN> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
    },
};

template <>
constexpr Ability Impl<ABILITY_ITCHY_DEFENSE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_WRAPPED)

        SetOnMoveEffectReactionFlags(battler, attacker, MOVE_EFFECT_WRAP);
        gBattleMons[attacker].status2 |= STATUS2_WRAPPED;
        gVolatileStructs[attacker].wrapTurns = WrapDuration(battler);

        gBattleStruct->wrappedMove[attacker] = MOVE_INFESTATION;
        gBattleStruct->wrappedBy[attacker] = battler;

        BattleScriptCall(BattleScript_AttackerBecameInfested);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_GENERATOR> = {
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

template <>
constexpr Ability Impl<ABILITY_MOON_SPIRIT> = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; },
};

template <>
constexpr Ability Impl<ABILITY_DUST_CLOUD> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SAND_ATTACK, 0); },
};

template <>
constexpr Ability Impl<ABILITY_TIPPING_POINT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_BERSERKER_RAGE> = {
    .onDefender = Impl<ABILITY_TIPPING_POINT>.onDefender,
    .onBattlerFaints = Impl<ABILITY_RAMPAGE>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_TRICKSTER> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DISABLE, 0); },
};

template <>
constexpr Ability Impl<ABILITY_SAND_GUARD> = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
        return Impl<ABILITY_QUEENLY_MAJESTY>.onImmune(DELEGATE_IMMUNE);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
        },
    .breakable = TRUE,
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_NATURAL_RECOVERY> = {
    .onExit = +[](ON_EXIT) -> int { return Impl<ABILITY_NATURAL_CURE>.onExit(DELEGATE_EXIT) | Impl<ABILITY_REGENERATOR>.onExit(DELEGATE_EXIT); },
};

template <>
constexpr Ability Impl<ABILITY_WIND_RIDER> = {
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

template <>
constexpr Ability Impl<ABILITY_SOOTHING_AROMA> = {
    .onEntry = +[](ON_ENTRY) -> int {
        int anyStatus = FALSE;
        struct Pokemon* party;

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

template <>
constexpr Ability Impl<ABILITY_PRIM_AND_PROPER> = {
    .onDefender = Impl<ABILITY_CUTE_CHARM>.onDefender,
    .fortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUPER_STRAIN> = {
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

template <>
constexpr Ability Impl<ABILITY_ENLIGHTENED> = {
    .onOffensiveMultiplier = Impl<ABILITY_EMANATE>.onOffensiveMultiplier,
    .onMoveType = Impl<ABILITY_EMANATE>.onMoveType,
    .onAccuracy = Impl<ABILITY_INNER_FOCUS>.onAccuracy,
    .breakable = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PEACEFUL_SLUMBER> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        if (!Impl<ABILITY_SWEET_DREAMS>.onEndTurn(DELEGATE_END_TURN)) return Impl<ABILITY_SELF_SUFFICIENT>.onEndTurn(DELEGATE_END_TURN);
        gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_AFTERSHOCK> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
    },
};

ON_EITHER(FreezingPoint) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanGetFrostbite(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_FREEZING_POINT> = {
    ON_EITHER_ABILITY(FreezingPoint),
};

static int CryoProficiencyHail(int battler) {
    CHECK(ShouldApplyOnHitEffect(battler))
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
template <>
constexpr Ability Impl<ABILITY_CRYO_PROFICIENCY> = {
    .onAttacker = Impl<ABILITY_FREEZING_POINT>.onAttacker,
    .onDefender = +[](ON_DEFENDER) -> int { return Impl<ABILITY_FREEZING_POINT>.onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(battler); },
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ARCANE_FORCE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
        },
    .onStab = Impl<ABILITY_MYSTIC_POWER>.onStab,
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DOOMBRINGER> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DOOM_DESIRE, 0); },
};

template <>
constexpr Ability Impl<ABILITY_WISHMAKER> = {
    .onEntry = +[](ON_ENTRY) -> int {
        int counter = GetSingleUseAbilityCounter(battler, ability);
        CHECK(counter < 3)
        CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

        SetSingleUseAbilityCounter(battler, ability, counter + 1);
        return TRUE;
    },
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_YUKI_ONNA> = {
    .onEntry = UseIntimidateClone,
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanInfatuate(battler, target))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    },
};

template <>
constexpr Ability Impl<ABILITY_SUPPRESS> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TORMENT, 0); },
};

template <>
constexpr Ability Impl<ABILITY_REFRIGERATOR> = {
    .onDefensiveMultiplier = Impl<ABILITY_FILTER>.onDefensiveMultiplier,
    .onAccuracy = Impl<ABILITY_ILLUMINATE>.onAccuracy,
};

template <>
constexpr Ability Impl<ABILITY_HEAVEN_ASUNDER> = {
    .onCrit =
        +[](ON_CRIT) {
            if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
            return 1;
        },
};

template <>
constexpr Ability Impl<ABILITY_PURIFYING_WATERS> = {
    .onEntry = Impl<ABILITY_WATER_VEIL>.onEntry,
    .onEndTurn = Impl<ABILITY_HYDRATION>.onEndTurn,
    .onStatusImmune = Impl<ABILITY_WATER_VEIL>.onStatusImmune,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SEABORNE> = {
    .onEntry = Impl<ABILITY_DRIZZLE>.onEntry,
    .onStat = Impl<ABILITY_SWIFT_SWIM>.onStat,
};

template <>
constexpr Ability Impl<ABILITY_HIGH_TIDE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_WATER)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_CHANGE_OF_HEART> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_HEART_SWAP, 0); },
};

template <>
constexpr Ability Impl<ABILITY_MYSTIC_BLADES> = {
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE));
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DETERMINATION> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
        },
    .negatesFrzSpatkDrop = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FERTILIZE> = {
    ATE_ABILITY(TYPE_GRASS),
};

template <>
constexpr Ability Impl<ABILITY_PURE_LOVE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gBattleMons[target].status2 & STATUS2_INFATUATION)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
    .onDefender = Impl<ABILITY_CUTE_CHARM>.onDefender,
    .canInfatuateAny = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FIGHTER> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIGHTING),
};

template <>
constexpr Ability Impl<ABILITY_TELEKINETIC> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TELEKINESIS, 0); },
};

template <>
constexpr Ability Impl<ABILITY_COMBUSTION> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_PONY_POWER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_MYSTIC_BLADES>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onSwapSplit = Impl<ABILITY_MYSTIC_BLADES>.onSwapSplit,
};

template <>
constexpr Ability Impl<ABILITY_POWDER_BURST> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_POWDER, 0); },
};

template <>
constexpr Ability Impl<ABILITY_RETRIEVER> = {
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

template <>
constexpr Ability Impl<ABILITY_MONSTER_MASH> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TRICK_OR_TREAT, 0); },
};

template <>
constexpr Ability Impl<ABILITY_TWO_STEP> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_SPITEFUL> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])

        BattleScriptCall(BattleScript_AbilitySpiteful);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FORTITUDE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_DEVOURER> = {
    .onParentalBond = Impl<ABILITY_PRIMAL_MAW>.onParentalBond,
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_PHANTOM_THIEF> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SPECTRAL_THIEF, 40); },
};

template <>
constexpr Ability Impl<ABILITY_EARLY_GRAVE> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_GHOST),
};

template <>
constexpr Ability Impl<ABILITY_BASS_BOOSTED> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_AMPLIFIER>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_PUNK_ROCK>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onDefensiveMultiplier = Impl<ABILITY_PUNK_ROCK>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FLAMING_JAWS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
};

template <>
constexpr Ability Impl<ABILITY_MONSTER_HUNTER> = {
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

template <>
constexpr Ability Impl<ABILITY_CROWNED_SWORD> = {
    .onEntry = Impl<ABILITY_INTREPID_SWORD>.onEntry,
    .onDefender = Impl<ABILITY_ANGER_POINT>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_CROWNED_SHIELD> = {
    .onEntry = Impl<ABILITY_DAUNTLESS_SHIELD>.onEntry,
    .onDefender = Impl<ABILITY_STAMINA>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_BERSERK_DNA> = {
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

template <>
constexpr Ability Impl<ABILITY_CROWNED_KING> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        return Impl<ABILITY_AS_ONE_SHADOW_RIDER>.onBattlerFaints(DELEGATE_BATTLER_FAINTS) |
               Impl<ABILITY_AS_ONE_ICE_RIDER>.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .unnerve = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SNAP_TRAP_WHEN_HIT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_PERMANENCE> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); },
};

template <>
constexpr Ability Impl<ABILITY_HUBRIS> = {
    .onBattlerFaints = Impl<ABILITY_GRIM_NEIGH>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_COSMIC_DAZE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
        },
};

template <>
constexpr Ability Impl<ABILITY_COSMIC_DUST> = {
    .onOffensiveMultiplier = Impl<ABILITY_COSMIC_DAZE>.onOffensiveMultiplier,
    .magicGuard = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MINDS_EYE> = {
    .onTypeEffectiveness = Impl<ABILITY_SCRAPPY>.onTypeEffectiveness,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BLOOD_PRICE> = {
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
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanBleed(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_SPIKE_ARMOR> = {
    ON_EITHER_ABILITY(SpikeArmor),
};

template <>
constexpr Ability Impl<ABILITY_VOODOO_POWER> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IS_MOVE_SPECIAL(move))
        CHECK(CanBleed(attacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_CHROME_COAT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_SPECIAL(move)) MUL(.6);
        },
    .onStat = Impl<ABILITY_LEAD_COAT>.onStat,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BANSHEE> = {
    .onOffensiveMultiplier = Impl<ABILITY_LIQUID_VOICE>.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_GHOST + 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_WEB_SPINNER> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_STRING_SHOT, 0); },
};

template <>
constexpr Ability Impl<ABILITY_SHOWDOWN_MODE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
    },
};

template <>
constexpr Ability Impl<ABILITY_SEED_SOWER> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptCall(BattleScript_SeedSower);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_GRASSY,
};

template <>
constexpr Ability Impl<ABILITY_AIRBORNE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FLYING) MUL(1.3);
        },
    .onOffensiveMultiplierFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_PARROTING> = {
    .onImmune = Impl<ABILITY_SOUNDPROOF>.onImmune,
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(IsSoundMove(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SALT_CIRCLE> = {
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

template <>
constexpr Ability Impl<ABILITY_PURIFYING_SALT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GHOST) RESISTANCE(.5);
        },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
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
template <>
constexpr Ability Impl<ABILITY_PROTOSYNTHESIS> = {
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
template <>
constexpr Ability Impl<ABILITY_QUARK_DRIVE> = {
    .onEntry = +[](ON_ENTRY) -> int { return QuarkDriveHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onTerrain = +[](ON_TERRAIN) -> int { return QuarkDriveHandler(ability, battler, ABILITY_BS_CALL); },
    .onStat = Impl<ABILITY_PROTOSYNTHESIS>.onStat,
};

template <>
constexpr Ability Impl<ABILITY_WIND_POWER> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(gBattleMoves[move].airBased)
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_IMPULSE> = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
        },
};

template <>
constexpr Ability Impl<ABILITY_TERMINAL_VELOCITY> = {
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
        },
};

template <>
constexpr Ability Impl<ABILITY_ANGER_SHELL> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_AngerShell);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_EGOIST> = {
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

template <>
constexpr Ability Impl<ABILITY_READIED_ACTION> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
    },
};

template <>
constexpr Ability Impl<ABILITY_DARK_GALE_WINGS> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_DARK),
};

template <>
constexpr Ability Impl<ABILITY_GUILT_TRIP> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

        BattleScriptCall(BattleScript_GuiltTrip);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_WATER_GALE_WINGS> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_WATER),
};

template <>
constexpr Ability Impl<ABILITY_ZERO_TO_HERO> = {
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

template <>
constexpr Ability Impl<ABILITY_COSTAR> = {
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

template <>
constexpr Ability Impl<ABILITY_COMMANDER> = {
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

template <>
constexpr Ability Impl<ABILITY_EJECT_PACK_ABILITY> = {
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_VENGEFUL_SPIRIT> = {
    .onDefender = Impl<ABILITY_HAUNTED_SPIRIT>.onDefender,
    .onOffensiveMultiplier = Impl<ABILITY_VENGEANCE>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_CUD_CHEW> = {
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

template <>
constexpr Ability Impl<ABILITY_ARMOR_TAIL> = {
    .onImmune = Impl<ABILITY_QUEENLY_MAJESTY>.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MIND_CRUSH> = {
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
        },
};

template <>
constexpr Ability Impl<ABILITY_SUPREME_OVERLORD> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFaintedMonCount[GetBattlerSide(battler)])

        return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
    },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
        },
};

template <>
constexpr Ability Impl<ABILITY_ILL_WILL> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_FIRE_SCALES> = {
    .onDefensiveMultiplier = Impl<ABILITY_ICE_SCALES>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WATCH_YOUR_STEP> = {
    .onEntry = +[](ON_ENTRY) -> int {
        u8 targetSide = GetOppositeSide(battler);
        CHECK(gSideTimers[targetSide].spikesAmount < 3)

        gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_RAPID_RESPONSE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
    },
};

template <>
constexpr Ability Impl<ABILITY_DOUBLE_IRON_BARBS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 6;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_THERMAL_EXCHANGE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanRaiseStat(battler, STAT_ATK))

        SetStatChanger(STAT_ATK, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BURN)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GOOD_AS_GOLD> = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SHARING_IS_CARING> = {
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

template <>
constexpr Ability Impl<ABILITY_TABLETS_OF_RUIN> = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_ATK, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_ATK,
};

template <>
constexpr Ability Impl<ABILITY_SWORD_OF_RUIN> = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_DEF, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
};

template <>
constexpr Ability Impl<ABILITY_VESSEL_OF_RUIN> = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_SPATK, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_SPATK,
};

template <>
constexpr Ability Impl<ABILITY_BEADS_OF_RUIN> = {
    .onStat = +[](ON_STAT) { RuinEffect(STAT_DEF, battler, statId, stat, flags); },
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
};

template <>
constexpr Ability Impl<ABILITY_PERMAFROST_CLONE> = {
    .onDefensiveMultiplier = Impl<ABILITY_PERMAFROST>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GALLANTRY> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
        return TRUE;
    },
    .noDamageHits = 1,
    .breakable = TRUE,
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ORICHALCUM_PULSE> = {
    .onEntry = Impl<ABILITY_DROUGHT>.onEntry,
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_ATK) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
        },
};

template <>
constexpr Ability Impl<ABILITY_SUN_BASKING> = {
    .onImmune = +[](ON_IMMUNE) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
        return Impl<ABILITY_QUEENLY_MAJESTY>.onImmune(DELEGATE_IMMUNE);
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WINGED_KING> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
        },
};

template <>
constexpr Ability Impl<ABILITY_HADRON_ENGINE> = {
    .onEntry = Impl<ABILITY_ELECTRIC_SURGE>.onEntry,
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
        },
    .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
};

template <>
constexpr Ability Impl<ABILITY_IRON_SERPENT> = {
    .onOffensiveMultiplier = Impl<ABILITY_WINGED_KING>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_SWEEPING_EDGE_PLUS> = {
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
    .onAccuracy = Impl<ABILITY_SWEEPING_EDGE>.onAccuracy,
};

template <>
constexpr Ability Impl<ABILITY_CELESTIAL_BLESSING> = {
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

template <>
constexpr Ability Impl<ABILITY_MINION_CONTROL> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType { return PARENTAL_BOND_MINION_CONTROL; },
};

template <>
constexpr Ability Impl<ABILITY_MOLTEN_BLADES> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeBurned(target))
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_HAUNTING_FRENZY> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    },
    .onBattlerFaints = Impl<ABILITY_ADRENALINE_RUSH>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_NOISE_CANCEL> = {
    .onImmune = Impl<ABILITY_SOUNDPROOF>.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
    .isSoundproof = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RADIO_JAM> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeDisabled(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
    },
};

template <>
constexpr Ability Impl<ABILITY_OLE> = {
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

template <>
constexpr Ability Impl<ABILITY_MALICIOUS> = {
    .onEntry = UseIntimidateClone,
};

template <>
constexpr Ability Impl<ABILITY_DEAD_POWER> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
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

template <>
constexpr Ability Impl<ABILITY_BRAWLING_WYVERN> = {
    .onAccuracy = Impl<ABILITY_NO_GUARD>.onAccuracy,
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_PUNCH)
        CHECK(IS_MOVE_TYPE(move, TYPE_DRAGON))
        return TRUE;
    },
    .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
};

template <>
constexpr Ability Impl<ABILITY_JUNSHI_SANDA> = {
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        switch (flag) {
            case MOVE_FLAG_PUNCH:
                return gBattleMoves[move].flags & FLAG_STRIKER_BOOST;
            case MOVE_FLAG_KICK:
                return gBattleMoves[move].flags & FLAG_IRON_FIST_BOOST;
            default:
                return FALSE;
        }
    },
};

template <>
constexpr Ability Impl<ABILITY_MYTHICAL_ARROWS> = {
    .onOffensiveMultiplier = Impl<ABILITY_ARCHER>.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].arrowBased);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LAWNMOWER> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FLOURISH> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_DESERT_SPIRIT> = {
    .onEntry = Impl<ABILITY_SAND_STREAM>.onEntry,
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
                *mod = UQ_4_12(1.0);
            }
        },
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_AERIALIST> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_LEVITATE>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_FLOCK>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .breakable = TRUE,
    .levitate = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TERA_SHELL> = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TOXIC_CHAIN> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    },
};

template <>
constexpr Ability Impl<ABILITY_PARASITIC_SPORES> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

        gVolatileStructs[battler].parasiticSpores = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
    },
};

template <>
constexpr Ability Impl<ABILITY_POISON_PUPPETEER> = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](opt int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
    },
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int state = GetAbilityState(battler, ability);
        if (state & (1 << fainted)) SetAbilityState(battler, ability, state ^ (1 << fainted));
        return NO_ANNOUNCE;
    },
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_POISON,
};

template <>
constexpr Ability Impl<ABILITY_ENTRANCE> = {
    .onReactive = +[](ON_REACTIVE) -> int { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_CONFUSION,
};

template <>
constexpr Ability Impl<ABILITY_REJECTION> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gFieldTimers.quashTimer)

        gFieldTimers.quashTimer = QUASH_DURATION;
        gFieldTimers.started.quash = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
    },
};

template <>
constexpr Ability Impl<ABILITY_APPLE_ENLIGHTENMENT> = {
    .onDefensiveMultiplier = Impl<ABILITY_FUR_COAT>.onDefensiveMultiplier,
    .breakable = TRUE,
    .magicGuard = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BALLOON_BOMBER> = {
    .onDefender =
        +[](ON_DEFENDER) -> int { return Impl<ABILITY_AFTERMATH>.onDefender(DELEGATE_DEFENDER) || Impl<ABILITY_INFLATABLE>.onDefender(DELEGATE_DEFENDER); },
};

template <>
constexpr Ability Impl<ABILITY_FLAMING_MAW> = {
    .onAttacker = Impl<ABILITY_FLAMING_JAWS>.onAttacker,
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_DEMOLITIONIST> = {
    .onEntry = Impl<ABILITY_READIED_ACTION>.onEntry,
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

template <>
constexpr Ability Impl<ABILITY_ROCKHARD_WILL> = {
    .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_ROCK),
};

ON_EITHER(FragrantDaze) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanBeConfused(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 30)

    AbilityStatusEffectSafe(MOVE_EFFECT_CONFUSION, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_FRAGRANT_DAZE> = {
    ON_EITHER_ABILITY(FragrantDaze),
};

template <>
constexpr Ability Impl<ABILITY_LOW_VISIBILITY> = {
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

template <>
constexpr Ability Impl<ABILITY_OLD_MARINER> = {
    .onOffensiveMultiplier = Impl<ABILITY_SEAWEED>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_SEAWEED>.onDefensiveMultiplier,
    .onStab = Impl<ABILITY_AMPHIBIOUS>.onStab,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ECTOPLASM> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_BEAUTIFUL_MUSIC> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(Random() % 2)
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    },
    .canInfatuateAny = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SNOW_SONG> = {
    .onOffensiveMultiplier = Impl<ABILITY_LIQUID_VOICE>.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_ICE + 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_GREATER_SPIRIT> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_RESONANCE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 50)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

template <>
constexpr Ability Impl<ABILITY_ETHEREAL_RUSH> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_CUTE_ANTECEDENCE> = {
    .onPriority = GALE_WINGS_CLONE(TYPE_FAIRY),
};

template <>
constexpr Ability Impl<ABILITY_RECURRING_NIGHTMARE> = {
    .onRevive = +[](ON_REVIVE) -> int {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
        return B_MSG_FADE_OUT;
    },
    .persistent = TRUE,
};

ON_EITHER(MenacingSituation) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK_NOT(gVolatileStructs[opponent].fear)
    CHECK(Random() % 100 < 30)

    gStackBattler1 = battler;
    gStackBattler2 = opponent;
    BattleScriptCall(BattleScript_AbilitySetFear);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_MENACING_SITUATION> = {
    ON_EITHER_ABILITY(MenacingSituation),
};

template <>
constexpr Ability Impl<ABILITY_SHINY_LIGHTNING> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    },
};

template <>
constexpr Ability Impl<ABILITY_TERRIFY> = {
    .onEntry = UseIntimidateClone,
};

template <>
constexpr Ability Impl<ABILITY_ICE_DOWNFALL> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LAST_STAND> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_DEF || statId == STAT_SPDEF)
                *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PYROCLASTIC_FLOW> = {
    .onTypeEffectiveness = +[](ON_TYPE_EFFECTIVENESS) -> int {
        return Impl<ABILITY_MOLTEN_DOWN>.onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS) ||
               Impl<ABILITY_CORROSION>.onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS);
    },
    .onCanStatusType = Impl<ABILITY_CORROSION>.onCanStatusType,
};

template <>
constexpr Ability Impl<ABILITY_BLOOD_BATH> = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(
            ability, battler, +[](opt int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_BLEED)
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_BLEED,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BATTLE_AURA> = {
    .onCrit = +[](ON_CRIT) -> int { return 2; },
    .onCritFor = APPLY_ON_ANY,
};

template <>
constexpr Ability Impl<ABILITY_BLOODLUST> = {
    .onReactive = Impl<ABILITY_BLOOD_BATH>.onReactive,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        int result = 0;
        if (battler == attacker) {
            result |= Impl<ABILITY_SOUL_EATER>.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        return result | Impl<ABILITY_BLOOD_BATH>.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    },
    .onStatusImmune = Impl<ABILITY_BLOOD_BATH>.onStatusImmune,
    .onBattlerFaintsFor = APPLY_ON_ANY,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PIERCING_SOLO> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

template <>
constexpr Ability Impl<ABILITY_RHYTHMIC> = {
    .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); },
};

template <>
constexpr Ability Impl<ABILITY_CHUNKY_BASS_LINE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(IsSoundMove(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
    },
};

template <>
constexpr Ability Impl<ABILITY_DUAL_HAMMER> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(gBattleMoves[move].hammerBased)
        return PARENTAL_BOND_DUAL_WIELD;
    },
};

template <>
constexpr Ability Impl<ABILITY_DENTING_BLOWS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(gBattleMoves[move].hammerBased)
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
};

template <>
constexpr Ability Impl<ABILITY_ICE_COLD_HUNTER> = {
    .onParentalBond = +[](ON_PARENTAL_BOND) -> MultihitType {
        CHECK(moveType == TYPE_ICE)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
        return PARENTAL_BOND_ICE_COLD_HUNTER;
    },
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SOUL_CRUSHER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMoves[move].hammerBased) MUL(1.1);
        },
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(gBattleMoves[move].hammerBased)
        return STAT_SPDEF;
    },
};

template <>
constexpr Ability Impl<ABILITY_ARC_FLASH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    },
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(CanBeBurned(attacker))
        CHECK(Random() % 2)

        AbilityStatusEffect(MOVE_EFFECT_BURN | MOVE_EFFECT_AFFECTS_USER);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_UNICORN> = {
    .onOffensiveMultiplier = Impl<ABILITY_MIGHTY_HORN>.onOffensiveMultiplier,
    ATE_ABILITY(TYPE_FAIRY),
};

template <>
constexpr Ability Impl<ABILITY_ON_THE_PROWL> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
    },
};

template <>
constexpr Ability Impl<ABILITY_PRETENTIOUS> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(gVolatileStructs[battler].critBoost < 3);
        gVolatileStructs[battler].critBoost++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
        BattleScriptCall(BattleScript_AbilityBoostsCrit);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_VENOBLAZE_PINCERS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
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

template <>
constexpr Ability Impl<ABILITY_ETERNAL_BLESSING> = {
    .onEndTurn = Impl<ABILITY_CELESTIAL_BLESSING>.onEndTurn,
    .onExit = Impl<ABILITY_REGENERATOR>.onExit,
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUGAR_RUSH> = {
    .onStat = Impl<ABILITY_UNBURDEN>.onStat,
};

template <>
constexpr Ability Impl<ABILITY_PEACEFUL_REST> = {
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

template <>
constexpr Ability Impl<ABILITY_WHITE_NOISE> = {
    .onEndTurn = Impl<ABILITY_PEACEFUL_REST>.onEndTurn,
    .onAttacker = Impl<ABILITY_STATIC>.onAttacker,
    .onDefender = Impl<ABILITY_STATIC>.onDefender,
};

template <>
constexpr Ability Impl<ABILITY_SMOKEY_MANEUVERS> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_POWER_METAL> = {
    .onOffensiveMultiplier = Impl<ABILITY_LIQUID_VOICE>.onOffensiveMultiplier,
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND);
        return TYPE_STEEL + 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_POWER_EDGE> = {
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_SUPERCONDUCTOR> = {
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

template <>
constexpr Ability Impl<ABILITY_ULTRA_INSTINCT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
        return FALSE;
    },
    .onDefensiveMultiplier = Impl<ABILITY_PARRY>.onDefensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_UNLOCKED_POTENTIAL> = {
    .onDefender = Impl<ABILITY_BERSERK>.onDefender,
    .onAccuracy = Impl<ABILITY_INNER_FOCUS>.onAccuracy,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HIGHER_RANK> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
        },
};

template <>
constexpr Ability Impl<ABILITY_FUNERAL_PYRE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(CheckAbilityWasAnnounced(battler, ABILITY_FUNERAL_PYRE))

        return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE);
    },
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

template <>
constexpr Ability Impl<ABILITY_FLAME_BUBBLE> = {
    .onOffensiveMultiplier = Impl<ABILITY_WATER_BUBBLE>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_WATER_BUBBLE>.onDefensiveMultiplier,
    .onPriority = Impl<ABILITY_FLAMING_SOUL>.onPriority,
    .onStatusImmune = Impl<ABILITY_WATER_BUBBLE>.onStatusImmune,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ELEMENTAL_VORTEX> = {
    .onAbsorb = +[](ON_ABSORB) -> int { return Impl<ABILITY_WATER_ABSORB>.onAbsorb(DELEGATE_ABSORB) || Impl<ABILITY_FLASH_FIRE>.onAbsorb(DELEGATE_ABSORB); },
    .onOffensiveMultiplier = Impl<ABILITY_FLASH_FIRE>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_SNOWY_WRATH> = {
    .onEntry = Impl<ABILITY_SNOW_WARNING>.onEntry,
    .onModifyEffectChance = Impl<ABILITY_CRYOMANCY>.onModifyEffectChance,
};

template <>
constexpr Ability Impl<ABILITY_PATTERN_CHANGE> = {
    .onEndTurn = Impl<ABILITY_SHED_SKIN>.onEndTurn,
    .onBeforeAttack = Impl<ABILITY_PROTEAN>.onBeforeAttack,
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_NO_TURNING_BACK> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_NoTurningBack);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FLAMMABLE_COAT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler) || (gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE))
        CHECK(moveType == TYPE_FIRE)
        CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[battler].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_TargetFormChange);
        return TRUE;
    },
    .onBeforeAttack = +[](ON_BEFORE_ATTACK) -> int {
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

template <>
constexpr Ability Impl<ABILITY_DRACO_MORALE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DRAGON_CHEER, 0); },
};

template <>
constexpr Ability Impl<ABILITY_BAD_OMEN> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (isCrit) MUL(.25);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MOSH_PIT> = {
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
    CHECK(ShouldApplyOnHitEffect(opponent))
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
template <>
constexpr Ability Impl<ABILITY_BLOOD_STAIN> = {
    .onEntry = +[](ON_ENTRY) -> int { return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN); },
    ON_EITHER_ABILITY(BloodStain),
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BLOOD_STIGMA> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
        },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SLIPSTREAM> = {
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[STAT_SPEED] += 20; },
};

template <>
constexpr Ability Impl<ABILITY_MAXIMUM_ACCELERATION> = {
    .onEndTurn = Impl<ABILITY_SPEED_BOOST>.onEndTurn,
    .onChooseOffensiveStat = Impl<ABILITY_SLIPSTREAM>.onChooseOffensiveStat,
};

template <>
constexpr Ability Impl<ABILITY_SIDEWINDER> = {
    .onEntry = Impl<ABILITY_COIL_UP>.onEntry,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
        gStatuses4[battler] |= STATUS4_COILED;
        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .coilUp = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PETRIFY> = {
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

template <>
constexpr Ability Impl<ABILITY_FLUFFIEST> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WAY_OF_PRECISION> = {
    .onAccuracy = Impl<ABILITY_INNER_FOCUS>.onAccuracy,
    .onCrit = Impl<ABILITY_PRECISE_FIST>.onCrit,
    .onModifyEffectChance = Impl<ABILITY_PRECISE_FIST>.onModifyEffectChance,
    .breakable = TRUE,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WAY_OF_SWIFTNESS> = {
    .onBattlerFaints = Impl<ABILITY_PRETENTIOUS>.onBattlerFaints,
    .onStat = Impl<ABILITY_SWIFT_SWIM>.onStat,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_ATOMIC_PUNCH> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_IRON_FIST>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_STEELY_SPIRIT>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

template <>
constexpr Ability Impl<ABILITY_IRON_GIANT> = {
    .onDefensiveMultiplier = Impl<ABILITY_HEATPROOF>.onDefensiveMultiplier,
    .onChooseOffensiveStat = Impl<ABILITY_JUGGERNAUT>.onChooseOffensiveStat,
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    },
    .breakable = TRUE,
    .negatesBurnAtkDrop = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MASTER_HAND> = {
    .onBattlerFaints = Impl<ABILITY_RAMPAGE>.onBattlerFaints,
    .onOffensiveMultiplier = Impl<ABILITY_MEGA_LAUNCHER>.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .megaLauncherBoost = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FINAL_BLOW> = {
    .onAccuracy = Impl<ABILITY_FATAL_PRECISION>.onAccuracy,
    .onCrit = Impl<ABILITY_FATAL_PRECISION>.onCrit,
};

template <>
constexpr Ability Impl<ABILITY_HOSPITALITY> = {
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

template <>
constexpr Ability Impl<ABILITY_BUTTER_UP> = {
    .onEntry = +[](ON_ENTRY) -> int { return Impl<ABILITY_HOSPITALITY>.onEntry(DELEGATE_ENTRY) | Impl<ABILITY_SOOTHING_AROMA>.onEntry(DELEGATE_ENTRY); },
};

template <>
constexpr Ability Impl<ABILITY_VITALITY_STRIKE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(IsIronFistBoosted(battler, move))

        gBattleMoveDamage = -gHpDealt / 10;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_HUGE_WINGS> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_GIANT_WINGS>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_LEVITATE>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .breakable = TRUE,
    .levitate = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SWORD_OF_DAMNATION> = {
    .onStat = Impl<ABILITY_SWORD_OF_RUIN>.onStat,
    .onStatFor = APPLY_ON_OTHER,
    .ruinStat = STAT_DEF,
    .unaware = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RESTRAINING_ORDER> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(GetAbilityState(battler, ability) == RESTRAINING_ORDER_NOT_TRIGGERED)
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(CanBattlerBeForceSwitched(attacker))

        SetAbilityState(battler, ability, RESTRAINING_ORDER_ACTIVATING);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ASSASSINS_TOOLS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
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

template <>
constexpr Ability Impl<ABILITY_FROSTMAW> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanGetFrostbite(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
    },
};

template <>
constexpr Ability Impl<ABILITY_PATCHWORK> = {
    .onEntry = Impl<ABILITY_DISGUISE>.onEntry,
    .onDisguise = +[](ON_DISGUISE) -> SpeciesEnum {
        SpeciesEnum species = Impl<ABILITY_DISGUISE>.onDisguise(DELEGATE_DISGUISE);
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

template <>
constexpr Ability Impl<ABILITY_BLIND_RAGE> = {
    .onEntry = Impl<ABILITY_MOLD_BREAKER>.onEntry,
    .onTypeEffectiveness = Impl<ABILITY_SCRAPPY>.onTypeEffectiveness,
    .onMoldBreaker = Impl<ABILITY_MOLD_BREAKER>.onMoldBreaker,
    .tauntImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_APEX_PREDATOR> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onOffensiveMultiplier = Impl<ABILITY_TOUGH_CLAWS>.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_DRAGONS_RITUAL> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
        BattleScriptCall(BattleScript_DragonsRitual);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_PINNACLE_BLADE> = {
    .onInfiltrate = +[](ON_INFILTRATE) -> InfiltrateType {
        return DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE) ? INFILTRATE_BREAK_SCREENS | INFILTRATE_SUBSTITUTE : INFILTRATE_NONE;
    },
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DidMoveHit())
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE))

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

template <>
constexpr Ability Impl<ABILITY_ENERGIZED> = {
    .onEntry = Impl<ABILITY_GENERATOR>.onEntry,
    .onTerrain = Impl<ABILITY_GENERATOR>.onTerrain,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(moveType == TYPE_ELECTRIC);
        SetOncePerTurnAbilityCounter(battler, ability, TRUE);
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    },
    .onExit = Impl<ABILITY_GENERATOR>.onExit,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_COLOR_SPECTRUM> = {
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

template <>
constexpr Ability Impl<ABILITY_STEEL_BEETLE> = {
    .onParentalBond = Impl<ABILITY_RAGING_BOXER>.onParentalBond,
    .onOffensiveMultiplier = Impl<ABILITY_POLLINATE>.onOffensiveMultiplier,
    .onMoveType = Impl<ABILITY_POLLINATE>.onMoveType,
};

template <>
constexpr Ability Impl<ABILITY_FROM_THE_SHADOWS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
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

template <>
constexpr Ability Impl<ABILITY_RAGE_POINT> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_HOT_COALS> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

        gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
    },
};

template <>
constexpr Ability Impl<ABILITY_TERASTAL_TREASURE> = {
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.6); },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= .8;
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SHOCKING_MAW> = {
    .onAttacker = Impl<ABILITY_SHOCKING_JAWS>.onAttacker,
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_GLEAM_EYES> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseIntimidateClone(ability, battler) | Impl<ABILITY_FRISK>.onEntry(DELEGATE_ENTRY); },
};

template <>
constexpr Ability Impl<ABILITY_ROUSED_FANGS> = {
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
    .onChooseOffensiveStat = Impl<ABILITY_MIND_CRUSH>.onChooseOffensiveStat,
};

template <>
constexpr Ability Impl<ABILITY_DREAM_STATE> = {
    .onDefensiveMultiplier = Impl<ABILITY_BATTLE_ARMOR>.onDefensiveMultiplier,
    .onCrit = Impl<ABILITY_BATTLE_ARMOR>.onCrit,
    .onCritFor = Impl<ABILITY_BATTLE_ARMOR>.onCritFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DREAM_WHIMSY> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_YAWN, 0); },
};

template <>
constexpr Ability Impl<ABILITY_LUNAR_AFFINITY> = {
    .onCopyMove = +[](ON_COPY_MOVE) -> int {
        CHECK(gBattleMoves[move].lunar)
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    },
};

template <>
constexpr Ability Impl<ABILITY_FLAME_SHIELD> = {
    .onDefensiveMultiplier = Impl<ABILITY_FILTER>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_AQUATIC_DWELLER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_APPLE_PIE> = {
    .onEndTurn = Impl<ABILITY_SELF_SUFFICIENT>.onEndTurn,
};

template <>
constexpr Ability Impl<ABILITY_HOVER> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_PSYCHIC); },
    .breakable = TRUE,
    .levitate = TRUE,
    .addsType = TYPE_PSYCHIC,
};

template <>
constexpr Ability Impl<ABILITY_DEPRAVITY> = {
    .onCrit = Impl<ABILITY_MERCILESS>.onCrit,
    .onTypeEffectiveness = Impl<ABILITY_OVERCHARGE>.onTypeEffectiveness,
    .onCanStatusType = Impl<ABILITY_OVERCHARGE>.onCanStatusType,
};

template <>
constexpr Ability Impl<ABILITY_WILDFIRE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FIRE_SPIN, 0); },
};

template <>
constexpr Ability Impl<ABILITY_JUMP_SCARE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
    },
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TAR_TOSS> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TAR_SHOT, 0); },
};

template <>
constexpr Ability Impl<ABILITY_STUN_SHOCK> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target)) CHECK(Random() % 100 < 60) switch (Random() % 2) {
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

template <>
constexpr Ability Impl<ABILITY_RAGING_GODDESS> = {
    .onBattlerFaints = Impl<ABILITY_RAMPAGE>.onBattlerFaints,
    .onParentalBond = Impl<ABILITY_PARENTAL_BOND>.onParentalBond,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_WHIPLASH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
};

template <>
constexpr Ability Impl<ABILITY_SUPERSWEET_SYRUP> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
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

template <>
constexpr Ability Impl<ABILITY_TRASH_HEAP> = {
    .onEntry = Impl<ABILITY_TOXIC_SPILL>.onEntry,
    .onEndTurn = Impl<ABILITY_TOXIC_SPILL>.onEndTurn,
    .onExit = Impl<ABILITY_TOXIC_SPILL>.onExit,
    .onTypeEffectiveness = Impl<ABILITY_CORROSION>.onTypeEffectiveness,
    .onCanStatusType = Impl<ABILITY_CORROSION>.onCanStatusType,
};

template <>
constexpr Ability Impl<ABILITY_SLUDGY_MIX> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_INTOXICATE>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_PUNK_ROCK>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onMoveType = Impl<ABILITY_INTOXICATE>.onMoveType,
};

template <>
constexpr Ability Impl<ABILITY_OVERWATCH> = {
    .onEntry = Impl<ABILITY_ON_THE_PROWL>.onEntry,
    .onOffensiveMultiplier = Impl<ABILITY_STAKEOUT>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_WIND_RAGE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); },
    .onOffensiveMultiplier = Impl<ABILITY_GIANT_WINGS>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_VICTORY_BOMB> = {
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

template <>
constexpr Ability Impl<ABILITY_RAZOR_SHARP> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBleed(target))
        CHECK(gIsCriticalHit)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

template <>
constexpr Ability Impl<ABILITY_TO_THE_BONE> = {
    .onAttacker = Impl<ABILITY_RAZOR_SHARP>.onAttacker,
    .onOffensiveMultiplier = Impl<ABILITY_SNIPER>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_BLADE_DANCE> = {
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
template <>
constexpr Ability Impl<ABILITY_APE_SHIFT> = {
    .onEntry = +[](ON_ENTRY) -> int { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onEndTurn = +[](ON_END_TURN) -> int { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onDefender = +[](ON_DEFENDER) -> int { return ApeShiftHandler(battler, ABILITY_BS_CALL); },
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
        return ALWAYS_CRIT;
    },
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_KNOW_YOUR_PLACE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK_NOT(gVolatileStructs[target].dazed)
        CHECK(IsMoveMakingContact(move, battler))

        gVolatileStructs[target].dazed = 5;
        BattleScriptCall(BattleScript_TargetDazed);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DEEP_CUTS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBleed(target))
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

template <>
constexpr Ability Impl<ABILITY_LIFE_STEAL> = {
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

template <>
constexpr Ability Impl<ABILITY_RUDE_AWAKENING> = {
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_SLEEP)
        CHECK(GetAbilityState(battler, ability))
        return TRUE;
    },
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TERAFORM_ZERO> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(!GetSingleUseAbilityCounter(battler, ability));
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
        BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
        return TRUE;
    },
    .onAfterTypeEffectiveness = Impl<ABILITY_TERA_SHELL>.onAfterTypeEffectiveness,
    .onAfterTypeEffectivenessFor = Impl<ABILITY_TERA_SHELL>.onAfterTypeEffectivenessFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SET_ABLAZE> = {
    .onReactive = Impl<ABILITY_BLOOD_BATH>.onReactive,
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_BURN,
};

template <>
constexpr Ability Impl<ABILITY_BREAKWATER> = {
    .onDefensiveMultiplier = Impl<ABILITY_STALL>.onDefensiveMultiplier,
    .onStat = Impl<ABILITY_SWIFT_SWIM>.onStat,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MAGICAL_FISTS> = {
    .onOffensiveMultiplier = Impl<ABILITY_IRON_FIST>.onOffensiveMultiplier,
    .onChooseOffensiveStat =
        +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
        },
};

template <>
constexpr Ability Impl<ABILITY_CUTTHROAT> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
    },
    .cutthroat = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SAND_BENDER> = {
    .onEntry = Impl<ABILITY_SAND_STREAM>.onEntry,
    .onStat = Impl<ABILITY_SAND_FORCE>.onStat,
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SAND_PIT> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SAND_TOMB, 20); },
};

template <>
constexpr Ability Impl<ABILITY_DESOLATE_SUN> = {
    .randomizerBanned = TRUE,
};

ON_EITHER(Daybreak) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanBeBurned(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))

    AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_DAYBREAK> = {
    ON_EITHER_ABILITY(Daybreak),
};

template <>
constexpr Ability Impl<ABILITY_ENERGY_SIPHON> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_RESERVOIR> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_WATER);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT | ABSORB_RESULT_HEAL;
    },
    .redirectType = TYPE_WATER,
    .breakable = TRUE,
};

static int NeurotoxinCondition(opt int battler, int target) {
    return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
}
template <>
constexpr Ability Impl<ABILITY_NEUROTOXIN> = {
    .onReactive = +[](ON_REACTIVE) -> int { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_POISON,
};

template <>
constexpr Ability Impl<ABILITY_ENERGIZED_HORNS> = {
    .onOffensiveMultiplier = Impl<ABILITY_MIGHTY_HORN>.onOffensiveMultiplier,
    .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].hornBased);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SPIDER_LAIR_UPGRADE> = {
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

template <>
constexpr Ability Impl<ABILITY_CRUST_COAT> = {
    .onDefensiveMultiplier = Impl<ABILITY_BATTLE_ARMOR>.onDefensiveMultiplier,
    .onCrit = Impl<ABILITY_BATTLE_ARMOR>.onCrit,
    .onCritFor = Impl<ABILITY_BATTLE_ARMOR>.onCritFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PUFFY> = {
    .onDefensiveMultiplier = Impl<ABILITY_FLUFFY>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BALLOON_BLITZ> = {
    .onDefender = Impl<ABILITY_INFLATABLE>.onDefender,
    .onParentalBond = Impl<ABILITY_PARENTAL_BOND>.onParentalBond,
};

template <>
constexpr Ability Impl<ABILITY_STRIKER_PIXILATE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_STRIKER>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_PIXILATE>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onMoveType = Impl<ABILITY_PIXILATE>.onMoveType,
};

// 2.6
template <>
constexpr Ability Impl<ABILITY_DOOM_BLAST> = {
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

template <>
constexpr Ability Impl<ABILITY_BRUTEFORCE> = {
    .onOffensiveMultiplier = Impl<ABILITY_RECKLESS>.onOffensiveMultiplier,
    .onStatusImmune = Impl<ABILITY_ROCK_HEAD>.onStatusImmune,
    .noRecoil = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FARADAY_CAGE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
        return FALSE;
    },
    .onDefensiveMultiplier = Impl<ABILITY_SHELL_ARMOR>.onDefensiveMultiplier,
    .onCrit = Impl<ABILITY_SHELL_ARMOR>.onCrit,
    .onCritFor = Impl<ABILITY_SHELL_ARMOR>.onCritFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ACIDIC_SLIME> = {
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
    .onTypeEffectiveness = Impl<ABILITY_CORROSION>.onTypeEffectiveness,
    .onCanStatusType = Impl<ABILITY_CORROSION>.onCanStatusType,
};

template <>
constexpr Ability Impl<ABILITY_ROSE_GARDEN> = {
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

template <>
constexpr Ability Impl<ABILITY_QIGONG> = {
    .onBattlerFaints = Impl<ABILITY_RAMPAGE>.onBattlerFaints,
    .onOffensiveMultiplier = Impl<ABILITY_FIGHT_SPIRIT>.onOffensiveMultiplier,
    .onMoveType = Impl<ABILITY_FIGHT_SPIRIT>.onMoveType,
    .onAccuracy = +[](ON_ACCURACY) { return ACCURACY_ALWAYS_HITS; },
    .onBattlerFaintsFor = Impl<ABILITY_RAMPAGE>.onBattlerFaintsFor,
};

template <>
constexpr Ability Impl<ABILITY_CONJOURER_OF_DECEIT> = {
    .breakable = TRUE,
    .magicGuard = TRUE,
    .magicBounce = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DEEP_FREEZE> = {
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

template <>
constexpr Ability Impl<ABILITY_SOUL_DEVOURER> = {
    .onBattlerFaints = Impl<ABILITY_SOUL_EATER>.onBattlerFaints,
    .onTypeEffectiveness = Impl<ABILITY_PHANTOM_PAIN>.onTypeEffectiveness,
    .onBattlerFaintsFor = Impl<ABILITY_SOUL_EATER>.onBattlerFaintsFor,
};

template <>
constexpr Ability Impl<ABILITY_CHAMPIONS_ENTRANCE> = {
    .onEntry = +[](ON_ENTRY) -> int { return Impl<ABILITY_INTIMIDATE>.onEntry(DELEGATE_ENTRY) | Impl<ABILITY_VIOLENT_RUSH>.onEntry(DELEGATE_ENTRY); },
};

template <>
constexpr Ability Impl<ABILITY_PRESTO> = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(BATTLER_MAX_HP(battler))
        CHECK(IsSoundMove(battler, move))
        return 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_SAMBA> = {
    .onOffensiveMultiplier = Impl<ABILITY_STRIKER>.onOffensiveMultiplier,
    .onCopyMove = Impl<ABILITY_DANCER>.onCopyMove,
};

template <>
constexpr Ability Impl<ABILITY_GLADIATOR> = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIGHTING),
};

template <>
constexpr Ability Impl<ABILITY_FORSAKEN_HEART> = {
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ANY,
};

template <>
constexpr Ability Impl<ABILITY_RELENTLESS> = {
    .onOffensiveMultiplier = Impl<ABILITY_EXPLOIT_WEAKNESS>.onOffensiveMultiplier,
    .onChooseDefensiveStat = Impl<ABILITY_EXPLOIT_WEAKNESS>.onChooseDefensiveStat,
    .onCrit = Impl<ABILITY_MERCILESS>.onCrit,
};

template <>
constexpr Ability Impl<ABILITY_SOOTHSAYER> = {
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

template <>
constexpr Ability Impl<ABILITY_CORRUPTED_MIND> = {
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

template <>
constexpr Ability Impl<ABILITY_FLAME_COAT> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(CheckAbilityWasAnnounced(battler, ABILITY_FLAME_COAT))
        return SwitchInAnnounce(B_MSG_SWITCHIN_FIRE_COAT);
    },
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

template <>
constexpr Ability Impl<ABILITY_UNOWN_POWER> = {
    .onStab = +[](ON_STAB) -> int { return TRUE; },
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
        },
    .randomizerBanned = TRUE,
    .omniStab = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUPER_SCOPE> = {
    .onOffensiveMultiplier = Impl<ABILITY_MEGA_LAUNCHER>.onOffensiveMultiplier,
    .onAccuracy = Impl<ABILITY_ARTILLERY>.onAccuracy,
    .megaLauncherBoost = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_VENOM_CROWN> = {
    ON_EITHER_ABILITY(PoisonPoint),
    .onOffensiveMultiplier = Impl<ABILITY_MIGHTY_HORN>.onOffensiveMultiplier,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BLIGHT_SCALE> = {
    ON_EITHER_ABILITY(PoisonPoint),
    .onDefensiveMultiplier = Impl<ABILITY_MULTISCALE>.onDefensiveMultiplier,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GUNMAN> = {
    .onOffensiveMultiplier = Impl<ABILITY_MEGA_LAUNCHER>.onOffensiveMultiplier,
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_MEGA_LAUNCHER)
        CHECK(IS_MOVE_STATUS(move))
        return TRUE;
    },
    .megaLauncherBoost = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CARETAKER> = {
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

template <>
constexpr Ability Impl<ABILITY_POSEIDONS_DOMINION> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_WHIRLPOOL, 0); },
};

template <>
constexpr Ability Impl<ABILITY_DUAL_SHADOW> = {
    .onEndTurn = Impl<ABILITY_HUNGER_SWITCH>.onEndTurn,
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

template <>
constexpr Ability Impl<ABILITY_LULLABY> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(move == MOVE_SING);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    },
};

template <>
constexpr Ability Impl<ABILITY_CRYO_ARCHITECT> = {
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
        CHECK(ShouldApplyOnHitEffect(battler))
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

template <>
constexpr Ability Impl<ABILITY_GLACIAL_RAGE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_ICE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_IMMOVABLE_OBJECT> = {
    .magicGuard = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FRENZIED_PHANTOM> = {
    .onParentalBond = Impl<ABILITY_PARENTAL_BOND>.onParentalBond,
    .onTrap = Impl<ABILITY_SHADOW_TAG>.onTrap,
    .shadowTag = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DNA_SCRAMBLE> = {
    .onBeforeAttack = +[](ON_BEFORE_ATTACK) -> int {
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

template <>
constexpr Ability Impl<ABILITY_METALLIC_JAWS> = {
    .onEntry = Impl<ABILITY_METALLIC>.onEntry,
    .onParentalBond = Impl<ABILITY_PRIMAL_MAW>.onParentalBond,
    .addsType = TYPE_STEEL,
};

template <>
constexpr Ability Impl<ABILITY_CALCULATIVE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_ANALYTIC>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_NEUROFORCE>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
};

template <>
constexpr Ability Impl<ABILITY_EMBODY_ASPECT> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_EMBODY_ASPECT_HEARTHFLAME> = {
    .onEntry = Impl<ABILITY_INTREPID_SWORD>.onEntry,
};

template <>
constexpr Ability Impl<ABILITY_EMBODY_ASPECT_CORNERSTONE> = {
    .onEntry = Impl<ABILITY_DAUNTLESS_SHIELD>.onEntry,
};

template <>
constexpr Ability Impl<ABILITY_EMBODY_ASPECT_WELLSPRING> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPDEF))

        SetStatChanger(STAT_SPDEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_ROCKHARD_SHAFT> = {
    .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_ROCK),
};

template <>
constexpr Ability Impl<ABILITY_HUNTERS_MARK> = {
    .onAccuracy = Impl<ABILITY_DEADEYE>.onAccuracy,
    .onChooseDefensiveStat = Impl<ABILITY_DEADEYE>.onChooseDefensiveStat,
    .onCrit = Impl<ABILITY_AMBUSH>.onCrit,
};

template <>
constexpr Ability Impl<ABILITY_DEVIATE> = {
    ATE_ABILITY(TYPE_DARK),
};

template <>
constexpr Ability Impl<ABILITY_SUNS_BOUNTY> = {
    .onEndTurn = +[](ON_END_TURN) -> int { return Impl<ABILITY_HARVEST>.onEndTurn(DELEGATE_END_TURN) | Impl<ABILITY_LEAF_GUARD>.onEndTurn(DELEGATE_END_TURN); },
};

template <>
constexpr Ability Impl<ABILITY_RITE_OF_SPRING> = {
    .onStat =
        +[](ON_STAT) {
            Impl<ABILITY_SOLAR_POWER>.onStat(DELEGATE_STAT);
            Impl<ABILITY_CHLOROPHYLL>.onStat(DELEGATE_STAT);
        },
};

template <>
constexpr Ability Impl<ABILITY_HEADSTRONG> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPDEF))

        SetStatChanger(STAT_SPDEF, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FIREFIGHTER> = {
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

template <>
constexpr Ability Impl<ABILITY_SEPIA_LENS> = {
    .onImmune = Impl<ABILITY_SAND_GUARD>.onImmune,
    .onOffensiveMultiplier = Impl<ABILITY_TINTED_LENS>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_SAND_GUARD>.onDefensiveMultiplier,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUPER_SNIPER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_SNIPER>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
                MUL(0.5);
            }
        },
    .onPreemptAction = UseTurnAttackAsPursuit,
};

ON_EITHER(WoodlandCurse) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK_NOT(IS_BATTLER_OF_TYPE(opponent, TYPE_GRASS))

    gBattleMons[opponent].type3 = TYPE_GRASS;
    PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[opponent].type3);
    gStackBattler1 = opponent;
    BattleScriptCall(BattleScript_StackBecameTheTypeFull);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_WOODLAND_CURSE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_FORESTS_CURSE, 0); },
    ON_EITHER_ABILITY(WoodlandCurse),
};

template <>
constexpr Ability Impl<ABILITY_MALODOR> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_GASTRO_ACID)

        gStatuses3[attacker] |= STATUS3_GASTRO_ACID;
        BattleScriptCall(BattleScript_StackAbilitySuppressedMessage);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_BLUR> = {
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        return STAT_SPEED;
    },
    .onChooseDefensiveStatFor = APPLY_ON_TARGET,
};

template <>
constexpr Ability Impl<ABILITY_ELUDE> = {
    .onChooseDefensiveStat = +[](ON_CHOOSE_DEFENSIVE_STAT) -> int {
        CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker))
        return STAT_SPEED;
    },
    .onChooseDefensiveStatFor = APPLY_ON_TARGET,
};

template <>
constexpr Ability Impl<ABILITY_DRAKE_OF_RAGE> = {
    .onBattlerFaints = Impl<ABILITY_RAMPAGE>.onBattlerFaints,
    .onOffensiveMultiplier = Impl<ABILITY_TINTED_LENS>.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_MIXED_MARTIAL_ARTS> = {
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_PUNCH || flag == MOVE_FLAG_KICK)
        CHECK(gBattleMoves[move].type == TYPE_NORMAL)
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_STRATEGIC_PAUSE> = {
    .onOffensiveMultiplier = Impl<ABILITY_ANALYTIC>.onOffensiveMultiplier,
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber)
        CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT)
        return 2;
    },
};

template <>
constexpr Ability Impl<ABILITY_OVERRULE> = {
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        },
};

template <>
constexpr Ability Impl<ABILITY_MENTAL_POLLUTION> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MADNESS_ENHANCEMENT> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SOUL_TAP> = {
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

template <>
constexpr Ability Impl<ABILITY_SCARECROW> = {
    .onEntry = UseIntimidateClone,
    .onCrit = Impl<ABILITY_BAD_LUCK>.onCrit,
    .onModifyEffectChance = Impl<ABILITY_BAD_LUCK>.onModifyEffectChance,
    .onCritFor = Impl<ABILITY_BAD_LUCK>.onCritFor,
    .onModifyEffectChanceFor = Impl<ABILITY_BAD_LUCK>.onModifyEffectChanceFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_OMINOUS_SHROUD> = {
    .onEntry = Impl<ABILITY_PHANTOM>.onEntry,
    .onDefensiveMultiplier = Impl<ABILITY_SHADOW_SHIELD>.onDefensiveMultiplier,
    .addsType = TYPE_GHOST,
};

template <>
constexpr Ability Impl<ABILITY_CHILLING_PRESENCE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_ICY_WIND, 10); },
};

template <>
constexpr Ability Impl<ABILITY_FROSTBIND> = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](opt int battler, int target) { return (int)CanBeDisabled(target); }, BattleScript_Frostbind);
    },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_FROSTBITE,
};

template <>
constexpr Ability Impl<ABILITY_TENDER_AFFECTION> = {
    ON_EITHER_ABILITY(CuteCharm),
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FAIRY; },
};

template <>
constexpr Ability Impl<ABILITY_GLACIAL_GHOST> = {
    .onStat = Impl<ABILITY_SLUSH_RUSH>.onStat,
    .onAccuracy = Impl<ABILITY_SNOW_CLOAK>.onAccuracy,
    .onAccuracyFor = Impl<ABILITY_SNOW_CLOAK>.onAccuracyFor,
    .breakable = TRUE,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WONDER_SCALE> = {
    .onEndTurn = Impl<ABILITY_SHED_SKIN>.onEndTurn,
    .fortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_OVERZEALOUS> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STAINLESS_STEEL> = {
    ATE_ABILITY(TYPE_STEEL),
    .onAfterTypeEffectiveness = Impl<ABILITY_STEELWORKER>.onAfterTypeEffectiveness,
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
    .breakable = TRUE,
    .fortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TEMPORAL_RUPTURE> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GRASS_FLUTE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(IsSoundMove(battler, move))
        CHECK_NOT(gVolatileStructs[target].fear)

        return AbilityStatusEffect(MOVE_EFFECT_FEAR);
    },
};

template <>
constexpr Ability Impl<ABILITY_HEMOTOXIN> = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(
            ability,
            battler,
            [](opt int battler, int target) -> int {
                CHECK_NOT(gStatuses3[target] & STATUS3_GASTRO_ACID);
                gStatuses3[target] |= STATUS3_GASTRO_ACID;
                return TRUE;
            },
            BattleScript_StackAbilitySuppressedMessage);
    },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onBattlerFaintsFor = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaintsFor,
    .setStateOnEffect = MOVE_EFFECT_POISON,
};

template <>
constexpr Ability Impl<ABILITY_HARUKAZE> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TOXIC_SURGE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_TOXIC,
};

template <>
constexpr Ability Impl<ABILITY_POISON_QUILLS> = {
    .onAttacker = Impl<ABILITY_POISON_POINT>.onAttacker,
    .onDefender =
        +[](ON_DEFENDER) -> int { return Impl<ABILITY_ROUGH_SKIN>.onDefender(DELEGATE_DEFENDER) | Impl<ABILITY_POISON_POINT>.onDefender(DELEGATE_DEFENDER); },
};

template <>
constexpr Ability Impl<ABILITY_DRACONIC_MIGHT> = {
    .onEntry = Impl<ABILITY_HALF_DRAKE>.onEntry,
    ATE_ABILITY(TYPE_DRAGON),
    .addsType = TYPE_DRAGON,
};

template <>
constexpr Ability Impl<ABILITY_ATLANTIC_RULER> = {
    .onOffensiveMultiplier = Impl<ABILITY_AQUATIC_DWELLER>.onOffensiveMultiplier,
    .onStat = Impl<ABILITY_SWIFT_SWIM>.onStat,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BIOFILM> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_GUARDIAN_COAT> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (IS_MOVE_PHYSICAL(move)) MUL(.8);
        },
    .breakable = TRUE,
    .powderImmune = TRUE,
    .sandImmune = TRUE,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_NEUTRALIZING_FOG> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); },
};

template <>
constexpr Ability Impl<ABILITY_FESTIVITIES> = {
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        switch (flag) {
            case MOVE_FLAG_DANCE:
                return gBattleMoves[move].flags & FLAG_SOUND;
            case MOVE_FLAG_SOUND:
                return gBattleMoves[move].flags & FLAG_DANCE;
            default:
                return FALSE;
        }
    },
};

template <>
constexpr Ability Impl<ABILITY_FEY_FLIGHT> = {
    .onEntry = Impl<ABILITY_FAIRY_TALE>.onEntry,
    .breakable = TRUE,
    .levitate = TRUE,
    .addsType = TYPE_FAIRY,
};

template <>
constexpr Ability Impl<ABILITY_BEST_OFFENSE> = {
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
    .onSwapSplit = Impl<ABILITY_MYSTIC_BLADES>.onSwapSplit,
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[STAT_SPDEF] += 20; },
};

template <>
constexpr Ability Impl<ABILITY_IMPALER> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].hornBased);
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
    .onOffensiveMultiplier = Impl<ABILITY_MIGHTY_HORN>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_MAGUS_BLADES> = {
    .onParentalBond = Impl<ABILITY_DUAL_WIELD>.onParentalBond,
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
    .onSwapSplit = Impl<ABILITY_MYSTIC_BLADES>.onSwapSplit,
    .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) { secondaryAtkStatToUse[STAT_SPDEF] += 20; },
};

template <>
constexpr Ability Impl<ABILITY_LIGHTNING_BORN> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ELECTRIC); },
    .addsType = TYPE_ELECTRIC,
};

template <>
constexpr Ability Impl<ABILITY_WORLD_SERPENT> = {
    .onAttacker = Impl<ABILITY_GRIP_PINCER>.onAttacker,
    .onOffensiveMultiplier = Impl<ABILITY_LONG_REACH>.onOffensiveMultiplier,
    .onAccuracy = Impl<ABILITY_GRIP_PINCER>.onAccuracy,
};

template <>
constexpr Ability Impl<ABILITY_LUCKY_WINGS> = {
    .onOffensiveMultiplier = Impl<ABILITY_GIANT_WINGS>.onOffensiveMultiplier,
    .onModifyEffectChance = Impl<ABILITY_SERENE_GRACE>.onModifyEffectChance,
};

template <>
constexpr Ability Impl<ABILITY_KOMODO> = {
    .onEntry = Impl<ABILITY_HALF_DRAKE>.onEntry,
    .onAttacker = Impl<ABILITY_TOXIC_CHAIN>.onAttacker,
    .addsType = TYPE_DRAGON,
};

template <>
constexpr Ability Impl<ABILITY_ENVENOM> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_POISON);
    },
};

template <>
constexpr Ability Impl<ABILITY_PURPLE_HAZE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
    },
};

template <>
constexpr Ability Impl<ABILITY_GNASHING_CANNON> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_MEGA_LAUNCHER>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Impl<ABILITY_MIND_CRUSH>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        },
    .onChooseOffensiveStat = Impl<ABILITY_MIND_CRUSH>.onChooseOffensiveStat,
};

template <>
constexpr Ability Impl<ABILITY_HYPER_CLEANSE> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        },
    .onStatusImmune = +[](ON_STATUS_IMMUNE) -> int {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    },
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MOLTEN_COAT> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanBeBurned(target))
        CHECK(Random() % 2)

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    },
    ATE_ABILITY(TYPE_ROCK),
};

template <>
constexpr Ability Impl<ABILITY_ROYAL_DECREE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_GLARE, 0);
    },
    .onImmune = Impl<ABILITY_QUEENLY_MAJESTY>.onImmune,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TAG> = {
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

template <>
constexpr Ability Impl<ABILITY_SURPRISE> = {
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

template <>
constexpr Ability Impl<ABILITY_BREEZY_NEIGH> = {
    .onBattlerFaints = Impl<ABILITY_ADRENALINE_RUSH>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_DREAMSCAPE> = {
    .onEntry = Impl<ABILITY_COMATOSE>.onEntry,
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            Impl<ABILITY_DREAMCATCHER>.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MUL(1.2);
        },
    .onStatusImmune = Impl<ABILITY_COMATOSE>.onStatusImmune,
    .onPreemptAction = Impl<ABILITY_DREAMCATCHER>.onPreemptAction,
    .unsuppressable = TRUE,
    .removesStatusOnImmunity = TRUE,
    .alwaysSleeping = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HASTE_MAKES_WASTE> = {
    .onOffensiveMultiplier = Impl<ABILITY_ANALYTIC>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_STALL>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HUNGRY_MAWS> = {
    .onBattlerFaints = Impl<ABILITY_JAWS_OF_CARNAGE>.onBattlerFaints,
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
};

template <>
constexpr Ability Impl<ABILITY_THERMAL_SLIDE> = {
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
        },
};

template <>
constexpr Ability Impl<ABILITY_THERMOMANCY> = {
    .onModifyEffectChance =
        +[](ON_MODIFY_EFFECT_CHANCE) {
            Impl<ABILITY_CRYOMANCY>.onModifyEffectChance(DELEGATE_MODIFY_EFFECT_CHANCE);
            Impl<ABILITY_PYROMANCY>.onModifyEffectChance(DELEGATE_MODIFY_EFFECT_CHANCE);
        },
};

template <>
constexpr Ability Impl<ABILITY_CHUCKSTER> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(GetAbilityState(battler, ability) == RESTRAINING_ORDER_NOT_TRIGGERED)
        CHECK(DidMoveHit())

        SetAbilityState(battler, ability, CanBattlerBeForceSwitched(attacker) ? RESTRAINING_ORDER_ACTIVATING : RESTRAINING_ORDER_DONE);
        return FALSE;
    },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (!GetAbilityState(battler, ability)) {
                MUL(.5);
            }
        },
    .redCardEffect = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HEAT_SINK> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE);
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .redirectType = TYPE_FIRE,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RELIC_STONE> = {
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUPERCELL> = {
    .onEntry = +[](ON_ENTRY) -> int { return Impl<ABILITY_ELECTRIC_SURGE>.onEntry(DELEGATE_ENTRY) | Impl<ABILITY_DRIZZLE>.onEntry(DELEGATE_ENTRY); },
    .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
};

template <>
constexpr Ability Impl<ABILITY_LIGHTNING_ASPECT> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_ELECTRIC)
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FIRE_ASPECT> = {
    .onAbsorb = +[](ON_ABSORB) -> int {
        CHECK(moveType == TYPE_FIRE)
        return ABSORB_RESULT_HEAL;
    },
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanBeBurned(target))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BLISTERING_SUN> = {
    .onEntry = +[](ON_ENTRY) -> int { return Impl<ABILITY_DESOLATE_LAND>.onEntry(DELEGATE_ENTRY) | Impl<ABILITY_AIR_BLOWER>.onEntry(DELEGATE_ENTRY); },
};

template <>
constexpr Ability Impl<ABILITY_AURORAS_GALE> = {
    .onEntry = Impl<ABILITY_NORTH_WIND>.onEntry,
    .onStat = Impl<ABILITY_MAJESTIC_BIRD>.onStat,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WINTER_THRONE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(CheckAbilityWasAnnounced(battler, ABILITY_WINTER_THRONE)) return SwitchInAnnounce(B_MSG_SWITCHIN_WINTER_THRONE);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))

            if (IS_BATTLER_OF_TYPE(target, TYPE_ICE)) {
                FILTER_NOT(IsMagicGuardProtected(target))
                gStackBattler1 = target;
                BattleScriptExecute(BattleScript_WinterThroneDamage);
            } else {
                FILTER_NOT(BATTLER_MAX_HP(target))
                FILTER(CanBattlerHeal(target))
                gStackBattler1 = target;
                BattleScriptExecute(BattleScript_HealStack1HpOver8End3);
            }

            any = TRUE;
        }
        return any;
    },
};

template <>
constexpr Ability Impl<ABILITY_CHRISTMAS_NIGHTMARE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(IsWeatherActive(WEATHER_HAIL_ANY))

        CHECK_NOT(CheckAbilityWasAnnouncedBy(BATTLE_PARTNER(battler), ABILITY_FLAME_COAT))

        return SwitchInAnnounce(B_MSG_SWITCHIN_CHRISTMAS_NIGHTMARE);
    },
    .onWeather = +[](ON_WEATHER) -> int {
        CHECK(IsWeatherActive(WEATHER_HAIL_ANY))
        CHECK(IsAbilityOnSide(GetBattlerSide(battler), ability) - 1 == battler)

        DisableSwitchInAbility(battler, ability);
        DisableSwitchInAbility(BATTLE_PARTNER(battler), ability);

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CHRISTMAS_NIGHTMARE;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
        return TRUE;
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        CHECK(IsAbilityOnSide(GetBattlerSide(battler), ability) - 1 == battler)

        int any = FALSE;
        for (int target = GetOppositeSide(battler); target < gBattlersCount; target += 2) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IsHailImmune(target))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_WinterThroneDamage);

            any = TRUE;
        }
        return any;
    },
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ICE_PLUMES> = {
    .onDefensiveMultiplier = Impl<ABILITY_ICE_SCALES>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_PROPELLER_TAIL> = {
    .onStat = Impl<ABILITY_SWIFT_SWIM>.onStat,
};

template <>
constexpr Ability Impl<ABILITY_ENERGY_TAP> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))

        gBattleMoveDamage = -gHpDealt / 8;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_MOLTEN_CORE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        Impl<ABILITY_FURNACE>.onEntry(DELEGATE_ENTRY);

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

template <>
constexpr Ability Impl<ABILITY_REVERBATE> = {
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_SOUND)
        CHECK(gBattleMoves[move].type == TYPE_NORMAL)
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_TAEKKYEON> = {
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_DANCE)
        CHECK_NOT(IS_MOVE_STATUS(move))
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_SLUDGE_SPIT> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_VENOM_BOLT, 35);
    },
};

template <>
constexpr Ability Impl<ABILITY_SWAMP_THING> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].swampTimer)

        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        AbilityStatusEffectSafe(MOVE_EFFECT_SWAMP, battler, GetOppositeSide(battler));
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FROSTY_PRESCENCE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_MIST, 0); },
};

template <>
constexpr Ability Impl<ABILITY_CHILLING_PELLETS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_SPEAR, 13);
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_PAINT_SHOT> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, moveType))
        CHECK(IsMegaLauncherBoosted(battler, move))

        gBattleMons[target].type1 = moveType;
        gBattleMons[target].type2 = moveType;
        gBattleMons[target].type3 = TYPE_MYSTERY;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        gStackBattler1 = target;
        BattleScriptCall(BattleScript_StackBecameTheTypeFull);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_STONECUTTER> = {
    .onOffensiveMultiplier = Impl<ABILITY_FOSSILIZED>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_FOSSILIZED>.onDefensiveMultiplier,
    .onMoldBreaker = +[](ON_MOLD_BREAKER) -> int {
        gHitMarker |= HITMARKER_MOLD_BREAKER;
        SetTypeBeforeUsingMove(move, gActiveBattler);
        u8 moveType;
        GET_MOVE_TYPE(move, moveType)
        if (gBattleMoves[move].type2) {
            u16 typeEffectiveness;
            CalculateMoveDamageAndEffectiveness(gCurrentMove, gBattlerAttacker, gBattlerTarget, &moveType, &typeEffectiveness);
        }
        gHitMarker &= ~HITMARKER_MOLD_BREAKER;
        return moveType == TYPE_ROCK;
    },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_EDGELORD> = {
    .onEntry = Impl<ABILITY_CUTTHROAT>.onEntry,
    .onBattlerFaints = +[](ON_BATTLER_FAINTS) -> int {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CUTTHROAT;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
        return TRUE;
    },
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .cutthroat = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WARMONGER> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ROCK || moveType == TYPE_STEEL || moveType == TYPE_FIGHTING) MUL(1.30);
        },
};

template <>
constexpr Ability Impl<ABILITY_LOCUST_SWARM> = {
    .onEntry = +[](ON_ENTRY) -> int { return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onEndTurn = +[](ON_END_TURN) -> int { return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_REVELATION> = {
    .onEntry = +[](ON_ENTRY) -> int { return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .onEndTurn = +[](ON_END_TURN) -> int { return TryTransformAttacker(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); },
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CURSE_OF_FAMINE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_CurseOfFamine);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_CRYSTALLINE_ARMOR> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SOUL_HARVEST> = {
    .onStat =
        +[](ON_STAT) {
            if (statId != STAT_SPEED) *stat = *stat * (20 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 20;
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_THICK_BLUBBER> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.25);
        },
    .onStat =
        +[](ON_STAT) {
            if (statId == STAT_SPEED) *stat *= .5;
        },
};

template <>
constexpr Ability Impl<ABILITY_CRAVING> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RAT_KING> = {
    .onStat =
        +[](ON_STAT) {
            const BaseStats* baseStats = &gBaseStats[gBattleMons[battler].species];
            int bst =
                baseStats->baseHP + baseStats->baseAttack + baseStats->baseDefense + baseStats->baseSpAttack + baseStats->baseSpDefense + baseStats->baseSpeed;
            if (bst >= 400) return;
            *stat *= 1.5;
        },
    .onStatFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_CRISPY_CREAM> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        return Random() % 2 ? Impl<ABILITY_FLAME_BODY>.onDefender(DELEGATE_DEFENDER) : Impl<ABILITY_FREEZING_POINT>.onDefender(DELEGATE_DEFENDER);
    },
};

template <>
constexpr Ability Impl<ABILITY_DEEP_FRIED> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].fireSeaTimer)

        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        AbilityStatusEffectSafe(MOVE_EFFECT_FIRE_SEA, battler, GetOppositeSide(battler));
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_FOOD_LOVERS> = {
    .onEntry = Impl<ABILITY_HOSPITALITY>.onEntry,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LUNAR_WRATH> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(moveType == TYPE_GHOST)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MOONGEIST_BEAM, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_SPYWARE> = {
    .onEntry = +[](ON_ENTRY) -> int {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(battler)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(battler))

        int stat;
        switch (GetHighestStatId(gBattlerTarget, TRUE)) {
            case STAT_SPEED:
                stat = STAT_SPEED;
                break;
            case STAT_ATK:
                stat = STAT_DEF;
                break;
            case STAT_DEF:
                stat = STAT_ATK;
                break;
            case STAT_SPATK:
                stat = STAT_SPDEF;
                break;
            case STAT_SPDEF:
                stat = STAT_SPATK;
                break;
            default:
                stat = STAT_SPEED;
                break;
        }

        CHECK(ChangeStatBuffs(battler, 2, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_VIRUS> = {.onAttacker = +[](ON_ATTACKER) -> int {
    CHECK(ShouldApplyOnHitEffect(target))
    CHECK(moveType == TYPE_ELECTRIC)
    CHECK(CanBePoisoned(battler, target, move))

    return AbilityStatusEffect(MOVE_EFFECT_POISON);
}};

template <>
constexpr Ability Impl<ABILITY_POWER_LEAK> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
        BattleScriptCall(BattleScript_SurgeActivatesRet);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
};

template <>
constexpr Ability Impl<ABILITY_BACKUP_POWER> = {
    .onRevive = +[](ON_REVIVE) -> int {
        CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))
        return B_MSG_BACKUP_POWER;
    },
    .persistent = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SAND_FIEND> = {
    .onImmune = Impl<ABILITY_SAND_GUARD>.onImmune,
    .onDefensiveMultiplier = Impl<ABILITY_SAND_GUARD>.onDefensiveMultiplier,
    .onStat = Impl<ABILITY_SAND_FORCE>.onStat,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_MOUSTACHE> = {
    .onDefender =
        +[](ON_DEFENDER) -> int { return Impl<ABILITY_TANGLING_HAIR>.onDefender(DELEGATE_DEFENDER) | Impl<ABILITY_STAMINA>.onDefender(DELEGATE_DEFENDER); },
};

template <>
constexpr Ability Impl<ABILITY_DEPTH_EXPLORER> = {
    .onOffensiveMultiplier = Impl<ABILITY_FIELD_EXPLORER>.onOffensiveMultiplier,
    .onAccuracy = Impl<ABILITY_ILLUMINATE>.onAccuracy,
};

template <>
constexpr Ability Impl<ABILITY_DUNE_VEIL> = {
    .onImmune = Impl<ABILITY_SAND_GUARD>.onImmune,
    .onEndTurn = Impl<ABILITY_SELF_SUFFICIENT>.onEndTurn,
    .onDefensiveMultiplier = Impl<ABILITY_SAND_GUARD>.onDefensiveMultiplier,
    .breakable = TRUE,
    .sandImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STRONG_FOUNDATION> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_WATER || moveType == TYPE_GROUND) RESISTANCE(.50);
        },
    .suctionCups = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FOG_MACHINE> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(ShouldApplyOnHitEffect(battler)) CHECK_NOT(gBattleWeather & WEATHER_FOG_ANY) if (gBattleWeather & WEATHER_PRIMAL_ANY) {
            BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
            return NO_ANNOUNCE;
        }
        else if (TryChangeBattleWeather(battler, ENUM_WEATHER_FOG, TRUE)) {
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_FogStartsReturn);
            return TRUE;
        }
        return FALSE;
    },
};

template <>
constexpr Ability Impl<ABILITY_DROP_BLOCKS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_LASER_DRILL> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].hornBased)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    },
};

template <>
constexpr Ability Impl<ABILITY_LIGHT_SABER> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FIRE); },
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE))
        CHECK(Random() % 2)
        switch (Random() % 4) {
            case 0:
                CHECK(CanBeBurned(target))
                return AbilityStatusEffect(MOVE_EFFECT_BURN);

            case 1:
                CHECK(CanBeParalyzed(battler, target))
                return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);

            default:
                return FALSE;
        }
    },
    .addsType = TYPE_FIRE,
};

template <>
constexpr Ability Impl<ABILITY_LOOSE_THORNS> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

        BattleScriptCall(BattleScript_DefenderSetsCreepingThorns);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_TURF_WAR> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GREEDY> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_STRIKEOUT> = {
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BRUISER> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_FIGHTING); },
    .addsType = TYPE_FIGHTING,
};

template <>
constexpr Ability Impl<ABILITY_LETS_DANCE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_TEETER_DANCE, 0); },
};

template <>
constexpr Ability Impl<ABILITY_MYCELIUM_MIGHT> = {
    .onMoldBreaker = +[](ON_MOLD_BREAKER) -> int { return IS_MOVE_STATUS(move); },
};

template <>
constexpr Ability Impl<ABILITY_I_AM_STEVE> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_NO_RETREAT, 0); },
    .randomizerBanned = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_DEADLY_PRECISION> = {
    .onMoldBreaker = +[](ON_MOLD_BREAKER) -> int {
        gHitMarker |= HITMARKER_MOLD_BREAKER;
        SetTypeBeforeUsingMove(move, gActiveBattler);
        u8 moveType;
        GET_MOVE_TYPE(move, moveType)
        u16 typeEffectiveness;
        CalculateMoveDamageAndEffectiveness(gCurrentMove, gBattlerAttacker, gBattlerTarget, &moveType, &typeEffectiveness);
        gHitMarker &= ~HITMARKER_MOLD_BREAKER;
        return typeEffectiveness >= UQ_4_12(2.0);
    },
};

template <>
constexpr Ability Impl<ABILITY_ROCKY_EXTERIOR> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_ROCK); },
    .addsType = TYPE_ROCK,
};

template <>
constexpr Ability Impl<ABILITY_DRAGONFRUIT> = {
    .onEntry = Impl<ABILITY_HALF_DRAKE>.onEntry,
    .onDefender = Impl<ABILITY_ROUGH_SKIN>.onDefender,
    .addsType = TYPE_DRAGON,
};

template <>
constexpr Ability Impl<ABILITY_LEAD_CLAWS> = {
    .onOffensiveMultiplier = Impl<ABILITY_BIG_PECKS>.onOffensiveMultiplier,
    ATE_ABILITY(TYPE_ROCK),
};

template <>
constexpr Ability Impl<ABILITY_CHAINSAW> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KEEN_EDGE))
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
};

template <>
constexpr Ability Impl<ABILITY_GALEFORCE_WINGS> = {
    .onPriority = +[](ON_PRIORITY) -> int {
        CHECK(GetTypeBeforeUsingMove(move, battler) == TYPE_FLYING)
        return 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_EMPRESS> = {
    .onImmune = Impl<ABILITY_QUEENLY_MAJESTY>.onImmune,
    .onOffensiveMultiplier = Impl<ABILITY_RIVALRY>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_RIVALRY>.onDefensiveMultiplier,
    .onImmuneFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

ON_EITHER(HypnoticTouch) {
    CHECK(ShouldApplyOnHitEffect(opponent))
    CHECK(CanSleep(opponent))
    CHECK(IsMoveMakingContact(move, gBattlerAttacker))
    CHECK(Random() % 100 < 20)

    AbilityStatusEffectSafe(MOVE_EFFECT_SLEEP, battler, opponent);
    return TRUE;
}
template <>
constexpr Ability Impl<ABILITY_HYPNOTIC_TOUCH> = {
    ON_EITHER_ABILITY(HypnoticTouch),
};

template <>
constexpr Ability Impl<ABILITY_SUNDAE> = {
    .onEntry = Impl<ABILITY_SNOW_WARNING>.onEntry,
    .onEndTurn = Impl<ABILITY_ICE_BODY>.onEndTurn,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HYDRA> = {
    .onBattlerFaints = Impl<ABILITY_BEAST_BOOST>.onBattlerFaints,
    .onParentalBond = Impl<ABILITY_MULTI_HEADED>.onParentalBond,
    .onBattlerFaintsFor = APPLY_ON_ATTACKER,
    .resistsFortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WINGS_OF_PESTILENCE> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))

        int any = FALSE;

        if (Random() % 100 < 10 && !(gBattleMons[target].status2 & STATUS2_CURSED)) {
            gBattleMons[target].status2 |= STATUS2_CURSED;
            BattleScriptCall(BattleScript_MoveEffectCurse);
            any = TRUE;
        }

        if (Random() % 100 < 20 && CanBleed(target)) {
            any |= AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }

        return any;
    },
};

template <>
constexpr Ability Impl<ABILITY_ZEN_GARDEN> = {
    .onEntry = +[](ON_ENTRY) -> int {
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_SEEDS) {
            switch (GetBattlerHoldEffectParam(battler)) {
                case HOLD_EFFECT_PARAM_GRASSY_TERRAIN:
                    return Impl<ABILITY_GRASSY_SURGE>.onEntry(DELEGATE_ENTRY);

                case HOLD_EFFECT_PARAM_PSYCHIC_TERRAIN:
                    return Impl<ABILITY_PSYCHIC_SURGE>.onEntry(DELEGATE_ENTRY);
            }
        }

        switch (Random() % 2) {
            case 0:
                return Impl<ABILITY_GRASSY_SURGE>.onEntry(DELEGATE_ENTRY);

            case 1:
                return Impl<ABILITY_PSYCHIC_SURGE>.onEntry(DELEGATE_ENTRY);

            default:
                return FALSE;
        }
    },
    .allowTerrainIfAirborne = TERRAIN_GRASSY | TERRAIN_PSYCHIC,
};

template <>
constexpr Ability Impl<ABILITY_SHARP_TALONS> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_KICK))
        CHECK(CanBleed(target))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    },
};

template <>
constexpr Ability Impl<ABILITY_MASSIVE_PELT> = {
    .onDefender = Impl<ABILITY_TANGLING_HAIR>.onDefender,
    .onDefensiveMultiplier = Impl<ABILITY_FLUFFY>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ECHOLOCATION> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) MUL(1.2);
        },
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
        return ACCURACY_ALWAYS_HITS;
    },
};

template <>
constexpr Ability Impl<ABILITY_BARK_SKIN> = {
    .onEntry = +[](ON_ENTRY) -> int { return AddBattlerType(battler, TYPE_GHOST); },
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) {
                MUL(.7);
            } else {
                MUL(.85);
            }
        },
    .addsType = TYPE_GHOST,
};

template <>
constexpr Ability Impl<ABILITY_SAP_TRAP> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        int any = FALSE;

        int target = BATTLE_OPPOSITE(battler);
        if (CanLowerStat(target, STAT_SPEED)) {
            InsertCorrectEndType(ABILITY_BS_EXECUTE);
            any = TRUE;
            AbilityStatusEffectSafe(MOVE_EFFECT_SPD_MINUS_1, battler, target);
        }

        target = BATTLE_PARTNER(target);
        if (CanLowerStat(target, STAT_SPEED)) {
            if (!any) InsertCorrectEndType(ABILITY_BS_EXECUTE);
            any = TRUE;
            AbilityStatusEffectSafe(MOVE_EFFECT_SPD_MINUS_1, battler, target);
        }

        return any;
    },
    .onTrap = +[](ON_TRAP) -> int { return gBattleMons[switchingBattler].statStages[STAT_SPEED] <= 3; },
};

template <>
constexpr Ability Impl<ABILITY_DEVIOUS_PRESENT> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_ICE || gBattleMoves[move].throwingBased) MUL(1.5);
        },
};

template <>
constexpr Ability Impl<ABILITY_COSMIC_WINGS> = {
    .onMoveType = +[](ON_MOVE_TYPE) -> int {
        CHECK(moveType == TYPE_FLYING)
        return TYPE_FAIRY + 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_MACH_3> = {
    .onChooseOffensiveStat = Impl<ABILITY_SLIPSTREAM>.onChooseOffensiveStat,
    .onMoldBreaker = Impl<ABILITY_DEADLY_PRECISION>.onMoldBreaker,
};

template <>
constexpr Ability Impl<ABILITY_FOGGY_EYE> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (moveType == TYPE_GHOST && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) MUL(1.5);
        },
    .onAfterTypeEffectiveness =
        +[](ON_AFTER_TYPE_EFFECTIVENESS) {
            if (moveType != TYPE_GHOST) return;
            if (!IsBattlerWeatherAffected(target, WEATHER_FOG_ANY)) return;
            if (*mod > UQ_4_12(0.5)) *mod = UQ_4_12(0.5);
        },
    .onAfterTypeEffectivenessFor = APPLY_ON_TARGET,
};

template <>
constexpr Ability Impl<ABILITY_CHANDELIER> = {
    .onAccuracy = Impl<ABILITY_ILLUMINATE>.onAccuracy,
    .onModifyEffectChance = Impl<ABILITY_PYROMANCY>.onModifyEffectChance,
};

template <>
constexpr Ability Impl<ABILITY_ANGELIC_WINGS> = {
    .onOffensiveMultiplier = Impl<ABILITY_HUGE_WINGS>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_PRISM_SCALES>.onDefensiveMultiplier,
    .breakable = TRUE,
    .levitate = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WITCH_BROOM> = {
    .onEntry = Impl<ABILITY_HOVER>.onEntry,
    .onParentalBond = Impl<ABILITY_HYPER_AGGRESSIVE>.onParentalBond,
    .breakable = TRUE,
    .levitate = TRUE,
    .addsType = TYPE_PSYCHIC,
};

template <>
constexpr Ability Impl<ABILITY_RAIN_SHROUD> = {
    .onAccuracy = +[](ON_ACCURACY) -> AccuracyPriority {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_RAIN_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    },
    .onAccuracyFor = APPLY_ON_TARGET,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CHESTNUT_SHIELD> = {
    .onImmune = Impl<ABILITY_BULLETPROOF>.onImmune,
    .breakable = TRUE,
    .magicGuard = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BRAIN_MASS> = {
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (BATTLER_MAX_HP(battler)) MUL(.5);
        },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TUMMYACHE> = {
    .onDefensiveMultiplier = Impl<ABILITY_THICK_FAT>.onDefensiveMultiplier,
    .onTypeEffectiveness = Impl<ABILITY_CORROSION>.onTypeEffectiveness,
    .onCanStatusType = Impl<ABILITY_CORROSION>.onCanStatusType,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUMO_GUARD> = {
    .onDefensiveMultiplier = Impl<ABILITY_THICK_FAT>.onDefensiveMultiplier,
    .onChooseOffensiveStat = Impl<ABILITY_JUGGERNAUT>.onChooseOffensiveStat,
    .onStatusImmune = Impl<ABILITY_JUGGERNAUT>.onStatusImmune,
    .breakable = TRUE,
    .removesStatusOnImmunity = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_ICE_PICK> = {
    .onOffensiveMultiplier = Impl<ABILITY_TOUGH_CLAWS>.onOffensiveMultiplier,
    .onStat = Impl<ABILITY_SLUSH_RUSH>.onStat,
    .hailImmune = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HAMMER_FIST> = {
    .onOffensiveMultiplier =
        +[](ON_OFFENSIVE_MULTIPLIER) {
            if (DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_PUNCH) || gBattleMoves[move].hammerBased) MUL(1.25);
        },
};

template <>
constexpr Ability Impl<ABILITY_TOXIC_SHELL> = {
    .onAttacker = Impl<ABILITY_POISON_POINT>.onAttacker,
    .onDefender = Impl<ABILITY_POISON_POINT>.onDefender,
    .onDefensiveMultiplier = Impl<ABILITY_SHELL_ARMOR>.onDefensiveMultiplier,
    .onCrit = Impl<ABILITY_SHELL_ARMOR>.onCrit,
    .onCritFor = Impl<ABILITY_SHELL_ARMOR>.onCritFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HAND_BARNACLES> = {
    .onParentalBond = Impl<ABILITY_MULTI_HEADED>.onParentalBond,
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_WATER; },
    .resistsFortKnox = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_LEPIDOPTERAN> = {
    .onOffensiveMultiplier = Impl<ABILITY_SWARM>.onOffensiveMultiplier,
    .breakable = TRUE,
    .unaware = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BREAK_IT_DOWN> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_DANCE))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_RAPID_SPIN, 20);
    },
};

template <>
constexpr Ability Impl<ABILITY_BACKSTREET_BOY> = {
    .onOffensiveMultiplier = Impl<ABILITY_STRIKER>.onOffensiveMultiplier,
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        switch (flag) {
            case MOVE_FLAG_KICK:
                CHECK(gBattleMoves[move].flags & FLAG_DANCE)
                return TRUE;

            case MOVE_FLAG_DANCE:
                CHECK(gBattleMoves[move].flags & FLAG_STRIKER_BOOST)
                return TRUE;

            default:
                return FALSE;
        }
    },
};

template <>
constexpr Ability Impl<ABILITY_BACKFLIP> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(DoesMoveMatchFlag(battler, move, moveType, MOVE_FLAG_DANCE))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_CHIP_AWAY, 50);
    },
};

template <>
constexpr Ability Impl<ABILITY_CRUSHING_JAW> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    },
    .onOffensiveMultiplier = Impl<ABILITY_STRONG_JAW>.onOffensiveMultiplier,
};

template <>
constexpr Ability Impl<ABILITY_CRYOSTASIS> = {
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(
            ability,
            battler,
            +[](opt int battler, int target) -> int {
                gBattleMons[target].status2 |= STATUS2_FLINCHED;
                return FALSE;
            },
            nullptr);
    },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onModifyEffectChance = Impl<ABILITY_CRYOMANCY>.onModifyEffectChance,
    .onBattlerFaintsFor = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaintsFor,
    .setStateOnEffect = MOVE_EFFECT_FROSTBITE,
};

template <>
constexpr Ability Impl<ABILITY_MUCUS_MEMBRANE> = {
    .onDefender = Impl<ABILITY_GOOEY>.onDefender,
    .onDefensiveMultiplier = +[](ON_DEFENSIVE_MULTIPLIER) { MUL(.7); },
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_VOLTRON> = {
    .onOffensiveMultiplier = Impl<ABILITY_MIGHTY_HORN>.onOffensiveMultiplier,
    .onDefensiveMultiplier = Impl<ABILITY_BATTLE_ARMOR>.onDefensiveMultiplier,
    .onCrit = Impl<ABILITY_BATTLE_ARMOR>.onCrit,
    .onCritFor = Impl<ABILITY_BATTLE_ARMOR>.onCritFor,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_BRAIN_OVERLOAD> = {
    .onDefender = +[](ON_DEFENDER) -> int {
        CHECK(DidMoveHit())
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
        BattleScriptCall(BattleScript_SurgeActivatesRet);
        return TRUE;
    },
    .allowTerrainIfAirborne = TERRAIN_PSYCHIC,
};

template <>
constexpr Ability Impl<ABILITY_ETERNAL_FLOWER> = {
    .onStat =
        +[](ON_STAT) {
            if (BattlerHasAbility(battler, ABILITY_ETERNAL_FLOWER, FALSE)) return;
            if (!GetBaseSpeciesFromMega(gBattleMons[battler].species)) return;
            if (*flags & NON_STACKING_ETERNAL_FLOWER) return;

            *stat *= .8;
            *flags = *flags | NON_STACKING_ETERNAL_FLOWER;
        },
    .onStatFor = APPLY_ON_OTHER,
};

template <>
constexpr Ability Impl<ABILITY_CURLIPEDE> = {
    .onEntry = +[](ON_ENTRY) -> int { return Impl<ABILITY_LETS_ROLL>.onEntry(DELEGATE_ENTRY) | Impl<ABILITY_COIL_UP>.onEntry(DELEGATE_ENTRY); },
    .coilUp = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FLAWLESS_PRECISION> = {
    .onAccuracy = Impl<ABILITY_FATAL_PRECISION>.onAccuracy,
    .onCrit = Impl<ABILITY_FATAL_PRECISION>.onCrit,
    .onMoldBreaker = Impl<ABILITY_DEADLY_PRECISION>.onMoldBreaker,
};

template <>
constexpr Ability Impl<ABILITY_MASHED_POTATO> = {
    .onEntry = +[](ON_ENTRY) -> int { return UseEntryMove(battler, ability, MOVE_SYRUP_BOMB, 0); },
};

template <>
constexpr Ability Impl<ABILITY_NIHIL_BLASTER> = {
    .onEntry = Impl<ABILITY_AURA_BREAK>.onEntry,
    .onOffensiveMultiplier = Impl<ABILITY_MEGA_LAUNCHER>.onOffensiveMultiplier,
    .breakable = TRUE,
    .megaLauncherBoost = TRUE,
    .auraBreak = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_GIANT_SHURIKEN> = {
    .onCrit = +[](ON_CRIT) -> int {
        CHECK(move == MOVE_WATER_SHURIKEN);
        return 1;
    },
};

template <>
constexpr Ability Impl<ABILITY_CHESTNUT_AXE> = {
    .onOffensiveMultiplier = Impl<ABILITY_KEEN_EDGE>.onOffensiveMultiplier,
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_KEEN_EDGE)
        return moveType == TYPE_GRASS;
    },
};

template <>
constexpr Ability Impl<ABILITY_WRESTLE_SHOWMAN> = {.onAttacker = +[](ON_ATTACKER) -> int {
    CHECK(move == MOVE_FLYING_PRESS)
    CHECK(ShouldApplyOnHitEffect(target))
    CHECK(IsBattlerAlive(target))
    CHECK(!IsAbilityStatusProtected(target, CHECK_RESTRICTING))
    CHECK(!gVolatileStructs[target].tauntTimer)
    BattleScriptCall(BattleScript_WrestleShowman_Effect_FlyingPress);
    return TRUE;
}};

template <>
constexpr Ability Impl<ABILITY_OVERCAST> = {
    .onEntry = +[](ON_ENTRY) -> int {
        int any = FALSE;
        if (!(gSideStatuses[battler] & SIDE_STATUS_MIST)) {
            InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerSetsMist);
            any = TRUE;
        }
        return any | Impl<ABILITY_LOW_VISIBILITY>.onEntry(DELEGATE_ENTRY);
    },
};

template <>
constexpr Ability Impl<ABILITY_STEADFAST> = {
    .steadfast = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SUPERHEAVY> = {
    .suctionCups = TRUE,
    .steadfast = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_WATERBORNE> = {
    .onEntry = Impl<ABILITY_AQUATIC>.onEntry,
    .adaptability = TRUE,
    .addsType = Impl<ABILITY_AQUATIC>.addsType,
};

template <>
constexpr Ability Impl<ABILITY_DEFIANT> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        CHECK(CanRaiseStat(battler, STAT_ATK))
        SetStatChanger(STAT_ATK, 2);
        BattleScriptCall(BattleScript_StackBattlerStatUp);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_COMPETITIVE> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPATK))
        SetStatChanger(STAT_SPATK, 2);
        BattleScriptCall(BattleScript_StackBattlerStatUp);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_CONTEMPT> = {
    .onStatLowered = Impl<ABILITY_DEFIANT>.onStatLowered,
    .unaware = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_RUN_AWAY> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        CHECK(CanRaiseStat(battler, STAT_SPEED))
        SetStatChanger(STAT_SPEED, 2);
        BattleScriptCall(BattleScript_StackBattlerStatUp);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_KINGS_WRATH> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        int any = FALSE;
        if (CanRaiseStat(battler, STAT_DEF)) {
            SetStatChanger(STAT_DEF, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        if (CanRaiseStat(battler, STAT_ATK)) {
            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        return any;
    },
    .onStatLoweredFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_QUEENS_MOURNING> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        int any = FALSE;
        if (CanRaiseStat(battler, STAT_SPDEF)) {
            SetStatChanger(STAT_SPDEF, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        if (CanRaiseStat(battler, STAT_SPATK)) {
            SetStatChanger(STAT_SPATK, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        return any;
    },
    .onStatLoweredFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_EMPERORS_WRATH> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        int any = FALSE;
        if (CanRaiseStat(battler, STAT_SPDEF)) {
            SetStatChanger(STAT_SPDEF, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        if (CanRaiseStat(battler, STAT_DEF)) {
            SetStatChanger(STAT_DEF, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        if (CanRaiseStat(battler, STAT_SPATK)) {
            SetStatChanger(STAT_SPATK, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        if (CanRaiseStat(battler, STAT_ATK)) {
            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        return any;
    },
    .onStatLoweredFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_LUCHA_LIBRE> = {
    .onImmune = Impl<ABILITY_DAZZLING>.onImmune,
    .onStatLowered = Impl<ABILITY_DEFIANT>.onStatLowered,
    .onImmuneFor = APPLY_ON_ALLY,
    .onStatLoweredFor = APPLY_ON_ALLY,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_FIRE_RULER> = {
    .onAttacker = Impl<ABILITY_FLAME_BODY>.onAttacker,
    .onDefender = Impl<ABILITY_FLAME_BODY>.onDefender,
    .onStatLowered = Impl<ABILITY_KINGS_WRATH>.onStatLowered,
    .onStatLoweredFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_NARCISSIST> = {
    .onStatLowered = +[](ON_STAT_LOWERED) -> int {
        int any = FALSE;
        if (CanRaiseStat(battler, STAT_SPATK)) {
            SetStatChanger(STAT_SPATK, 2);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        if (CanRaiseStat(battler, STAT_ATK)) {
            SetStatChanger(STAT_ATK, 2);
            BattleScriptCall(BattleScript_StackBattlerStatUp);
            any = TRUE;
        }
        return any;
    },
    .onStatLoweredFor = APPLY_ON_ALLY,
};

template <>
constexpr Ability Impl<ABILITY_STORM_CLOUD> = {
    .onEntry = Impl<ABILITY_DRIZZLE>.onEntry,
    .onStab = +[](ON_STAB) -> int { return moveType == TYPE_ELECTRIC; },
};

template <>
constexpr Ability Impl<ABILITY_TASTE_THE_RAINBOW> = {
    .onEntry = +[](ON_ENTRY) -> int {
        CHECK_NOT(gSideTimers[GetBattlerSide(battler)].rainbowTimer)

        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);

        return AbilityStatusEffect(MOVE_EFFECT_RAINBOW | MOVE_EFFECT_AFFECTS_USER);
    },
};

template <>
constexpr Ability Impl<ABILITY_RAINBOW_SCALES> = {
    .onEntry = Impl<ABILITY_TASTE_THE_RAINBOW>.onEntry,
    .onDefensiveMultiplier = Impl<ABILITY_FIRE_SCALES>.onDefensiveMultiplier,
    .breakable = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_HOME_RUN> = {
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(gIsCriticalHit)
        CHECK(ShouldApplyOnHitEffect(battler))

        u8 secondaryStat[6] = {0};
        u32 temp = 0;

        u32 stats[5][2] = {
            {STAT_ATK, CalculateStat(battler, STAT_ATK, secondaryStat, move, FALSE, FALSE, FALSE, FALSE)},
            {STAT_DEF, CalculateStat(battler, STAT_DEF, secondaryStat, move, FALSE, FALSE, FALSE, FALSE)},
            {STAT_SPATK, CalculateStat(battler, STAT_SPATK, secondaryStat, move, FALSE, FALSE, FALSE, FALSE)},
            {STAT_SPDEF, CalculateStat(battler, STAT_SPDEF, secondaryStat, move, FALSE, FALSE, FALSE, FALSE)},
            {STAT_SPEED, CalculateStat(battler, STAT_SPEED, secondaryStat, move, FALSE, FALSE, FALSE, FALSE)},
        };

        // This is just a partial bubble sort
        for (int maxValueIndex : {4, 3}) {
            for (int i = 0; i < maxValueIndex; i++) {
                if (stats[i][1] < stats[maxValueIndex][1]) continue;
                if (stats[i][1] == stats[maxValueIndex][1] && Random() % 2) continue;
                SWAP(stats[i][0], stats[maxValueIndex][0], temp)
                SWAP(stats[i][1], stats[maxValueIndex][1], temp)
            }
        }

        int statsToBoost[3] = {0};
        int pos = 0;

        // Sort stats by boost order. Read LIFO so in reverse order.
        for (int statId : {STAT_SPEED, STAT_SPDEF, STAT_DEF, STAT_SPATK, STAT_ATK}) {
            for (int i = 0; i < 3; i++) {
                if (stats[i][0] == statId) {
                    statsToBoost[pos++] = statId;
                    break;
                }
            }
            if (pos >= 3) break;
        }

        int any = FALSE;

        for (int stat : statsToBoost) {
            FILTER(CanRaiseStat(battler, stat))
            any = TRUE;
            AbilityStatusEffect(static_cast<MoveEffectEnum>(static_cast<int>(MOVE_EFFECT_ATK_PLUS_1) + (stat - 1)));
        }

        return any;
    },
};

template <>
constexpr Ability Impl<ABILITY_MUSICAL_NOTES> = {
    .onModifyMoveFlags = +[](ON_MODIFY_MOVE_FLAGS) -> int {
        CHECK(flag == MOVE_FLAG_SOUND)
        return IS_MOVE_STATUS(move);
    },
};

template <>
constexpr Ability Impl<ABILITY_GRAPPLER> = {
    .grappler = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_TANGLED_TAILS> = {
    .onAttacker = Impl<ABILITY_KNOW_YOUR_PLACE>.onAttacker,
    .grappler = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_SERPENT_BIND> = {
    .onEndTurn = +[](ON_END_TURN) -> int {
        int any = FALSE;
        for (int i = gBattlersCount - 1; i >= 0; i--) {
            FILTER(gBattleMons[i].status2 & STATUS2_WRAPPED)
            FILTER(gBattleStruct->wrappedBy[i] == battler)
            FILTER(StatLowerableOrMirrorArmor(i, STAT_SPEED))

            if (!any) {
                InsertCorrectEndType(ABILITY_BS_EXECUTE);
                any = TRUE;
            }

            AbilityStatusEffectSafe(MOVE_EFFECT_SPD_MINUS_1, battler, i);
        }

        return any;
    },
    .onAttacker = +[](ON_ATTACKER) -> int {
        CHECK(ShouldApplyOnHitEffect(gBattlerTarget))
        CHECK(IsBattlerAlive(battler))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK_NOT(gBattleMons[target].status2 & STATUS2_WRAPPED)
        CHECK(Random() % 2)

        SetOnMoveEffectReactionFlags(battler, target, MOVE_EFFECT_WRAP);
        gBattleMons[target].status2 |= STATUS2_WRAPPED;
        gVolatileStructs[target].wrapTurns = WrapDuration(battler);

        gBattleStruct->wrappedMove[target] = move;
        gBattleStruct->wrappedBy[target] = battler;
        BattleScriptCall(BattleScript_GripPincerActivated);
        return TRUE;
    },
};

template <>
constexpr Ability Impl<ABILITY_TENTALOCK> = {
    .onEndTurn = Impl<ABILITY_SERPENT_BIND>.onEndTurn,
    .onAttacker = Impl<ABILITY_SERPENT_BIND>.onAttacker,
    .grappler = TRUE,
};

template <>
constexpr Ability Impl<ABILITY_CHOKEHOLD> = {
    .onEndTurn = Impl<ABILITY_SERPENT_BIND>.onEndTurn,
    .onReactive = +[](ON_REACTIVE) -> int {
        return PoisonPuppeteerClone(ability, battler, +[](opt int battler, int target) { return (int)CanBeParalyzed(battler, target); }, BattleScript_Chokehold);
    },
    .onBattlerFaints = Impl<ABILITY_POISON_PUPPETEER>.onBattlerFaints,
    .onBattlerFaintsFor = APPLY_ON_OTHER,
    .setStateOnEffect = MOVE_EFFECT_WRAP,
};

#include "generated/data/abilities/ability_text.hh"

template <AbilityEnum Id>
constexpr Ability mergeAbility() {
    Ability impl = Impl<Id>;
    impl.name = AbilityStrings<Id>.name;
    impl.description = AbilityStrings<Id>.description;
    impl.expandedDescription = AbilityStrings<Id>.expandedDescription;
    return impl;
}

#define FOR_EACH_ABILITY_FUNCTION(ability) mergeAbility<ability>(),
const Ability gAbilities[] = {FOR_EACH_ABILITY};
