#pragma once

#include "behavior/constants.hh"
#include "behavior/implementation_interface.hh"
#include "behavior/ability/constants.hh"
#include "behavior/ability/template.hh"
#include "behavior/ability/impl_battle.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-function"

#define ENUM_OR(enumType) \
    inline enumType operator|(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }

ENUM_OR(InfiltrateType)
ENUM_OR(MoveEffectEnum)
ENUM_OR(NonStackingState)

template <typename As>
const As *dispatchTo(AbilityEnum id);

#define NO_ANNOUNCE 2

struct __EnumHack {
   public:
    operator int() const { return 0; }
    operator AccuracyPriority() const { return ACCURACY_NO_RESULT; }
    operator MultihitType() const { return MULTIHIT_SINGLE; }
};

#define CHECK(effect) \
    if (!(effect)) return __EnumHack();
#define CHECK_NOT(effect) \
    if (effect) return __EnumHack();

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

// int IsTargettedApplyOnFlagAppropriate(int contextBattler, int sourceBattler, int attacker, int target, AbilityApplyOnWithTarget flag) {
//     switch (flag) {
//         case APPLY_ON_ATTACKER_OR_TARGET:
//             return sourceBattler == attacker || sourceBattler == target;

//         case APPLY_ON_ATTACKER:
//             return sourceBattler == attacker;

//         case APPLY_ON_TARGET:
//             return sourceBattler == target;
//     }

//     return IsApplyOnFlagAppropriate(contextBattler, sourceBattler, (AbilityApplyOn)flag);
// }

// int IsApplyOnFlagAppropriate(int contextBattler, int sourceBattler, AbilityApplyOn flag) {
//     if (flag == APPLY_ON_SELF) return contextBattler == sourceBattler;
//     if (contextBattler == sourceBattler) return !(flag & APPLY_IGNORE_SELF);
//     if (GetBattlerSide(contextBattler) == GetBattlerSide(sourceBattler))
//         return flag & APPLY_ON_ALLY;
//     else
//         return flag & APPLY_ON_FOE;
//     return FALSE;
// }

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

template <Type BoostType>
struct AteAbility : extends OnMoveType, extends OnStab {
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_NORMAL)
        *ateBoost = TRUE;
        return BoostType + 1;
    }
    ON_STAB { return moveType == BoostType; }
};

template <Type BoostType>
struct SwarmLike : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == BoostType) {
            if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                MUL(1.5);
            else
                MUL(1.2);
        }
    }
};

// int DoesMoveMatchFlag(ON_MODIFY_MOVE_FLAGS_ARGS) {
//     switch (flag) {
//         case MOVE_FLAG_DANCE:
//             if (gBattleMoves[flag].flags & FLAG_DANCE) return TRUE;
//             break;
//         case MOVE_FLAG_KICK:
//             if (gBattleMoves[flag].flags & FLAG_STRIKER_BOOST) return TRUE;
//             break;
//         case MOVE_FLAG_MEGA_LAUNCHER:
//             if (gBattleMoves[flag].flags & FLAG_MEGA_LAUNCHER_BOOST) return TRUE;
//             break;
//         case MOVE_FLAG_PUNCH:
//             if (gBattleMoves[flag].flags & FLAG_IRON_FIST_BOOST) return TRUE;
//             break;
//         case MOVE_FLAG_SOUND:
//             if (gBattleMoves[flag].flags & FLAG_SOUND) return TRUE;
//             break;

//         default:
//             return FALSE;
//             break;
//     }

//     ON_ABILITY(battler, FALSE, gAbilities[ability].onModifyMoveFlags, if (gAbilities[ability].onModifyMoveFlags(DELEGATE_MODIFY_MOVE_FLAGS)) return TRUE)
//     return FALSE;
// }

template <>
struct AbilityImpl<ABILITY_NONE> : extends RandomizerBanned {};

template <int Stat>
struct RuinEffect : extends OnStat<ApplyOn::OTHER> {
    // static constexpr auto ruinAbilities = abilitiesAs<RuinEffect<Stat>>();
    ON_STAT {
        if (statId != Stat) return;
        if (*flags & Stat) return;
        ON_ABILITY(battler, FALSE, dispatchTo<RuinEffect<Stat>>(ability), return) *stat *= .75;
        *flags = *flags | static_cast<NonStackingState>(1 << Stat);
    }
};

template <>
struct AbilityImpl<ABILITY_TABLETS_OF_RUIN> : extends RuinEffect<STAT_ATK> {};
template <>
struct AbilityImpl<ABILITY_SWORD_OF_RUIN> : extends RuinEffect<STAT_DEF> {};
template <>
struct AbilityImpl<ABILITY_VESSEL_OF_RUIN> : extends RuinEffect<STAT_SPATK> {};
template <>
struct AbilityImpl<ABILITY_BEADS_OF_RUIN> : extends RuinEffect<STAT_SPDEF> {};

struct ToxicTerrainImmune {};
template <>
struct AbilityImpl<ABILITY_STENCH> : extends OnAttacker, extends ToxicTerrainImmune {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_HEAL> : extends ToxicTerrainImmune {};

template <>
struct AbilityImpl<ABILITY_DRIZZLE> : extends OnEntry {
    ON_ENTRY {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_DrizzleActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_SPEED_BOOST> : extends OnEndTurn {
    ON_END_TURN {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BATTLE_ARMOR> : extends Breakable, extends OnDefensiveMultiplier, extends OnCrit<ApplyOnTarget::TARGET> {
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
    ON_CRIT { return NEVER_CRIT; }
};

template <>
struct AbilityImpl<ABILITY_STURDY> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_DAMP> : extends OnEither {
    ON_EITHER {
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
};

struct RemovesStatusOnImmunity : extends OnStatusImmune<ApplyOn::SELF> {};

template <>
struct AbilityImpl<ABILITY_LIMBER> : extends RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_PARALYSIS) return TRUE; }
};

struct SandImmune {};
template <>
struct AbilityImpl<ABILITY_SAND_VEIL> : extends Breakable, extends SandImmune, extends OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_STATIC> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeParalyzed(battler, opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_PARALYSIS, battler, opponent);
        return TRUE;
    }
};

template <Type Absorbed>
struct AbsorbHeal : extends OnAbsorb {
    ON_ABSORB {
        CHECK(moveType == Absorbed)
        return ABSORB_RESULT_HEAL;
    }
};
template <>
struct AbilityImpl<ABILITY_VOLT_ABSORB> : extends AbsorbHeal<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_WATER_ABSORB> : extends AbsorbHeal<TYPE_WATER> {};

struct TauntImmune : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_OBLIVIOUS> : extends RemovesStatusOnImmunity, extends TauntImmune {
    ON_STATUS_IMMUNE { CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_CLOUD_NINE> : extends OnEntry {
    ON_ENTRY {
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_COMPOUND_EYES> : extends OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.3;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_INSOMNIA> : extends RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_SLEEP) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_COLOR_CHANGE> : extends OnBeforeAttack<ApplyOnTarget::TARGET> {
    ON_BEFORE_ATTACK {
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
    }
};

template <>
struct AbilityImpl<ABILITY_IMMUNITY> : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_POISON) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & (CHECK_STATUS1 & ~CHECK_SLEEP))
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLASH_FIRE> : extends OnAbsorb, extends OnOffensiveMultiplier<> {
    ON_ABSORB { CHECK(moveType == TYPE_FIRE) return ABSORB_RESULT_FLASH_FIRE; }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
    }
};

struct PowderImmune : extends Breakable {};
template <>
struct AbilityImpl<ABILITY_SHIELD_DUST> : extends PowderImmune {};

template <>
struct AbilityImpl<ABILITY_OWN_TEMPO> : extends RemovesStatusOnImmunity, extends TauntImmune {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_CONFUSION) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_SUCTION_CUPS> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_INTIMIDATE> : extends OnEntry {
    ON_ENTRY {
        u8 numAbility;

        for (numAbility = 0; numAbility < NUM_INTIMIDATE_CLONES; numAbility++) {
            if (gIntimidateCloneData[numAbility].ability == ability) break;
        }

        if (numAbility >= NUM_INTIMIDATE_CLONES) return FALSE;

        if (!gIntimidateCloneData[numAbility].numStatsLowered) return FALSE;

        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(gBattlerTarget) && !IsBattlerAlive(BATTLE_PARTNER(gBattlerTarget))) return FALSE;

        BattleScriptPushCursorAndCallback(BattleScript_IntimidateActivatedNew);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHADOW_TAG> : extends OnTrap {
    ON_TRAP { ON_ABILITY(switchingBattler, FALSE, dispatchTo<AbilityImpl<ABILITY_SHADOW_TAG>>(ability), return FALSE) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_ROUGH_SKIN> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasMagicGuard(attacker)) CHECK(IsMoveMakingContact(move, attacker)) gBattleMoveDamage = gBattleMons[attacker].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WONDER_GUARD> : extends Breakable, extends RandomizerBanned, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod < UQ_4_12(2.0)) *mod = 0;
    }
};

struct GroundImmune : extends Breakable {};
template <>
struct AbilityImpl<ABILITY_LEVITATE> : extends GroundImmune, extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING) MUL(1.25);
    }
};

template <>
struct AbilityImpl<ABILITY_EFFECT_SPORE> : extends PowderImmune, extends OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_CLEAR_BODY> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_NATURAL_CURE> : extends OnExit {
    ON_EXIT {
        CHECK(IsBattlerAlive(battler))
        CHECK(gBattleMons[battler].status1 & STATUS1_ANY)

            gActiveBattler = battler;
        gBattleMons[battler].status1 &= ~STATUS1_ANY;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
        MarkBattlerForControllerExec(battler);

        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptCall(BattleScript_NaturalCureExits);
        return TRUE;
    }
};

template <Type Absorbed, int Stat>
struct AbsorbStatUp : OnAbsorb {
    ON_ABSORB {
        CHECK(moveType == TYPE_ELECTRIC);
        int stat = Stat == STAT_HIGHEST_ATTACKING ? GetHighestAttackingStatId(battler, TRUE) : Stat;
        *statId = stat;
        return ABSORB_RESULT_STAT;
    }
};
template <Type Absorbed>
struct LightningRodClone : extends Redirects<Absorbed>, extends AbsorbStatUp<Absorbed, STAT_HIGHEST_ATTACKING> {};
template <>
struct AbilityImpl<ABILITY_LIGHTNING_ROD> : LightningRodClone<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_SERENE_GRACE> : extends OnModifyEffectChance<> {
    ON_MODIFY_EFFECT_CHANCE { *effectChance *= 2; }
};

template <>
struct AbilityImpl<ABILITY_SWIFT_SWIM> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CHLOROPHYLL> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_ILLUMINATE> : extends OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_TRACE> : extends RandomizerBanned, extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_HUGE_POWER> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_ATK) *stat *= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_POINT> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBePoisoned(battler, opponent, MOVE_NONE)) CHECK(IsMoveMakingContact(move, gBattlerAttacker)) CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_INNER_FOCUS> : extends TauntImmune, extends OnAccuracy<> {
    ON_ACCURACY { CHECK(move == MOVE_FOCUS_BLAST) return ACCURACY_ALWAYS_HITS; }
};

template <>
struct AbilityImpl<ABILITY_MAGMA_ARMOR> : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_FROSTBITE)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WATER_VEIL> : extends OnEntry, extends RemovesStatusOnImmunity {
    ON_ENTRY {
        CHECK_NOT(gStatuses3[battler] & STATUS3_AQUA_RING)

        gStatuses3[battler] |= STATUS3_AQUA_RING;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
        return TRUE;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_BURN)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MAGNET_PULL> : extends OnTrap {
    ON_TRAP { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); }
};

template <>
struct AbilityImpl<ABILITY_SOUNDPROOF> : extends OnImmune<> {
    ON_IMMUNE {
        CHECK(IsSoundMove(attacker, move))
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RAIN_DISH> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler)) CHECK(gVolatileStructs[battler].isFirstTurn != 2) CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_STREAM> : extends SandImmune, extends OnEntry {
    ON_ENTRY {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_SandstreamActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_PRESSURE> : extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_THICK_FAT> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_FLAME_BODY> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent)) CHECK(IsMoveMakingContact(move, gBattlerAttacker)) CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_KEEN_EYE> : extends OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_HYPER_CUTTER> : extends Breakable, extends OnCrit<> {
    ON_CRIT { CHECK(IsMoveMakingContact(move, battler)) return 1; }
};

template <>
struct AbilityImpl<ABILITY_PICKUP> : extends OnEntry {
    ON_ENTRY {
        int side = GetBattlerSide(battler);
        CHECK(gSideStatuses[side] & SIDE_STATUS_HAZARDS_ANY || gSideTimers[side].hotCoals || gSideTimers[side].caltrops)

        gSideStatuses[side] &= ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES | SIDE_STATUS_STICKY_WEB);
        gSideTimers[side].spikesAmount = 0;
        gSideTimers[side].toxicSpikesAmount = 0;
        gSideTimers[side].hotCoals = FALSE;
        gSideTimers[side].caltrops = FALSE;
        BattleScriptPushCursorAndCallback(BattleScript_PickUpActivate);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TRUANT> : extends OnEndTurn {
    ON_END_TURN {
        if (GetAbilityState(battler, ability))
            SetAbilityState(battler, ability, FALSE);
        else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
            SetAbilityState(battler, ability, TRUE);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_HUSTLE> : extends OnAccuracy<>, extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER { MUL(1.4); }
    ON_ACCURACY {
        CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_CUTE_CHARM> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker)) CHECK(CanInfatuate(battler, opponent)) CHECK(Random() % 100 < 50)

            AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PLUS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        int partner = BATTLE_PARTNER(battler);
        if (!IsBattlerAlive(partner)) return;
        if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
    }
};

template <>
struct AbilityImpl<ABILITY_MINUS> : extends AbilityImpl<ABILITY_PLUS> {};

struct StandardTransformation : extends FormChangeAbility, extends OnEntry, extends OnEndTurn {
    ON_ENTRY { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_END_TURN { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
};
struct WeatherTransformation : extends StandardTransformation, extends OnWeather {
    ON_WEATHER { return TryTransformAttacker(ability, battler, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_FORECAST> : extends WeatherTransformation, extends OnAttacker {
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_STICKY_HOLD> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_SHED_SKIN> : extends OnEndTurn {
    ON_END_TURN {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

struct NegateBurnAtkDrop {};
template <>
struct AbilityImpl<ABILITY_GUTS> : extends OnOffensiveMultiplier<>, extends NegateBurnAtkDrop {
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MARVEL_SCALE> : extends OnStat<> {
    ON_STAT {
        if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_OVERGROW> : extends SwarmLike<TYPE_GRASS> {};

template <>
struct AbilityImpl<ABILITY_BLAZE> : extends SwarmLike<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_TORRENT> : extends SwarmLike<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_SWARM> : extends SwarmLike<TYPE_BUG> {};

template <>
struct AbilityImpl<ABILITY_ROCK_HEAD> : extends RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_CONFUSION) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_DROUGHT> : extends OnEntry {
    ON_ENTRY {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_SUN, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_DroughtActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_ARENA_TRAP> : extends OnTrap {
    ON_TRAP { return IsBattlerGrounded(switchingBattler); }
};

template <>
struct AbilityImpl<ABILITY_VITAL_SPIRIT> : extends OnAttacker, extends RemovesStatusOnImmunity, extends TauntImmune {
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIGHTING) CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WHITE_SMOKE> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

        int side = GET_BATTLER_SIDE(battler);
        gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
        gSideTimers[side].started.smokescreen = TRUE;
        gSideTimers[side].smokescreenBattler = battler;
        return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
    }
};

template <>
struct AbilityImpl<ABILITY_FELINE_PROWESS> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPATK) *stat *= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_PURE_POWER> : extends AbilityImpl<ABILITY_FELINE_PROWESS> {};

template <>
struct AbilityImpl<ABILITY_SHELL_ARMOR> : extends AbilityImpl<ABILITY_BATTLE_ARMOR> {};

template <>
struct AbilityImpl<ABILITY_AIR_LOCK> : extends AbilityImpl<ABILITY_CLOUD_NINE> {};

template <>
struct AbilityImpl<ABILITY_TANGLED_FEET> : extends OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOTOR_DRIVE> : extends AbsorbStatUp<TYPE_ELECTRIC, STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_RIVALRY> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[battler].species, gBattleMons[battler].personality);
        if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[target].species, gBattleMons[target].personality))
            MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[attacker].species, gBattleMons[attacker].personality);
        if (genderAtk == MON_MALE)
            genderAtk = MON_FEMALE;
        else if (genderAtk == MON_FEMALE)
            genderAtk = MON_MALE;
        if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[battler].species, gBattleMons[battler].personality))
            MUL(.75);
    }
};

struct HailImmune {};
template <>
struct AbilityImpl<ABILITY_SNOW_CLOAK> : extends Breakable, extends HailImmune, extends OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANGER_POINT> : extends OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_UNBURDEN> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_HEATPROOF> : extends OnDefensiveMultiplier, extends NegateBurnAtkDrop {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRY_SKIN> : extends AbilityImpl<ABILITY_WATER_ABSORB>, extends AbilityImpl<ABILITY_RAIN_DISH>, extends OnDefensiveMultiplier {
    ON_END_TURN {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !HasMagicGuard(battler)) {
            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            BattleScriptPushCursorAndCallback(BattleScript_SolarPowerActivates);
            return TRUE;
        }

        return AbilityImpl<ABILITY_RAIN_DISH>::onEndTurn(DELEGATE_END_TURN);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(1.25);
    }
};

template <>
struct AbilityImpl<ABILITY_DOWNLOAD> : extends OnEntry {
    ON_ENTRY {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(battler)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(battler))

        int stat = GetHighestDefendingStatId(gBattlerTarget, TRUE) == STAT_DEF ? STAT_SPATK : STAT_ATK;
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_FIST> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsIronFistBoosted(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_ADAPTABILITY> {};

template <>
struct AbilityImpl<ABILITY_SKILL_LINK> {};

template <>
struct AbilityImpl<ABILITY_HYDRATION> : extends OnEndTurn {
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOLAR_POWER> : extends OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_QUICK_FEET> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_NORMALIZE> : extends OnOffensiveMultiplier<>, extends OnMoveType, extends OnTypeEffectiveness<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
    }
    ON_MOVE_TYPE { return TYPE_NORMAL + 1; }
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_NORMAL) CHECK(*mod) CHECK(*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SNIPER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (isCrit) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MAGIC_GUARD> {};

template <>
struct AbilityImpl<ABILITY_NO_GUARD> : extends OnAccuracy<ApplyOnTarget::ATTACKER_OR_TARGET> {
    ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
};

template <>
struct AbilityImpl<ABILITY_STALL> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
    }
};

template <>
struct AbilityImpl<ABILITY_TECHNICIAN> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (basePower <= 60) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_LEAF_GUARD> : extends OnEndTurn {
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOLD_BREAKER> : extends OnEntry, extends OnMoldBreaker {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); }
    ON_MOLD_BREAKER { return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_SUPER_LUCK> : extends OnCrit<> {
    ON_CRIT { return 1; }
};

template <>
struct AbilityImpl<ABILITY_AFTERMATH> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(HasMagicGuard(attacker)) CHECK(IsMoveMakingContact(move, attacker))

            gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = 1;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANTICIPATION> : extends Breakable, extends OnEntry, extends Persistent {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_FOREWARN> : extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_UNAWARE> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_TINTED_LENS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
    }
};

template <>
struct AbilityImpl<ABILITY_FILTER> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
    }
};

template <>
struct AbilityImpl<ABILITY_SLOW_START> : extends OnEntry, extends OnStat<> {
    ON_ENTRY {
        gVolatileStructs[battler].slowStartTimer = 5;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
    }
    ON_STAT {
        if (statId != STAT_ATK && statId != STAT_SPATK && statId != STAT_SPEED) return;
        if (gVolatileStructs[battler].slowStartTimer) *stat /= 2;
    }
};

struct HitsGhost : extends OnTypeEffectiveness<> {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
        CHECK(defType == TYPE_GHOST)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    }
};
template <>
struct AbilityImpl<ABILITY_SCRAPPY> : extends HitsGhost, extends TauntImmune {};

template <>
struct AbilityImpl<ABILITY_STORM_DRAIN> : extends LightningRodClone<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_ICE_BODY> : extends HailImmune, extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2) CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOLID_ROCK> : extends AbilityImpl<ABILITY_FILTER> {};

template <>
struct AbilityImpl<ABILITY_SNOW_WARNING> : extends HailImmune, extends OnEntry {
    ON_ENTRY {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_SnowWarningActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_HONEY_GATHER> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK(Random() % 2)

        gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
        BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FRISK> : extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_RECKLESS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_MULTITYPE> : extends FormChangeAbility {};

template <>
struct AbilityImpl<ABILITY_FLOWER_GIFT> : extends WeatherTransformation, extends Breakable, extends OnStat<ApplyOn::ALLY> {
    ON_STAT {
        if (statId != STAT_SPATK && statId != STAT_SPDEF) return;
        if (IsWeatherActive(WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_BAD_DREAMS> : extends OnEndTurn {
    ON_END_TURN {
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHEER_FORCE> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_CONTRARY> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_UNNERVE> : extends OnEntry {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); }
};

template <>
struct AbilityImpl<ABILITY_DEFEATIST> : extends OnStat<> {
    ON_STAT {
        if (statId != STAT_ATK && statId != STAT_SPATK) return;
        if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_CURSED_BODY> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(gVolatileStructs[attacker].disabledMove)
        CHECK(IsMoveMakingContact(move, attacker)) CHECK_NOT(IsAbilityStatusProtected(attacker, CHECK_RESTRICTING))
            CHECK(gBattleMons[attacker].pp[gChosenMovePos]) CHECK(Random() % 100 < 30)

                gVolatileStructs[attacker]
                    .disabledMove = gChosenMove;
        gVolatileStructs[attacker].disableTimer = 4;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove);
        BattleScriptCall(BattleScript_CursedBodyActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_HEALER> : extends OnEndTurn {
    ON_END_TURN {
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
    }
};

template <>
struct AbilityImpl<ABILITY_FRIEND_GUARD> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_WEAK_ARMOR> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(CanRaiseStat(battler, STAT_SPEED) || CanLowerStat(battler, STAT_DEF))

            if (gBattleMoves[move].effect == EFFECT_HIT_ESCAPE && CanBattlerSwitch(attacker)) gRoundStructs[battler]
                .disableEjectPack = TRUE;  // Set flag for target

        BattleScriptCall(BattleScript_WeakArmorActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LIGHT_METAL> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= 1.3;
    }
};

template <>
struct AbilityImpl<ABILITY_MULTISCALE> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (BATTLER_MAX_HP(battler)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_TOXIC_BOOST> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_FLARE_BOOST> : extends OnEntry, extends OnWeather, extends OnStat<>, extends NegateBurnAtkDrop {
    static int FlareBoostHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
        CHECK(CanBeBurned(battler))
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        InsertCorrectEndType(callType);
        gBattleMons[battler].status1 |= STATUS1_BURN;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
        MarkBattlerForControllerExec(battler);
        BattleScriptCall(BattleScript_FlareBoostRet);
        return TRUE;
    }

    ON_ENTRY { return FlareBoostHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_WEATHER { return FlareBoostHandler(ability, battler, ABILITY_BS_CALL); }
    ON_STAT {
        if (statId != STAT_SPATK) return;
        if (gBattleMons[battler].status1 & STATUS1_BURN) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_HARVEST> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK_NOT(gBattleStruct->changedItems[battler])
        CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES) CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

            BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TELEPATHY> : extends OnAfterTypeEffectiveness<ApplyOnTarget::ATTACKER_OR_TARGET> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_MOODY> : extends OnEndTurn {
    ON_END_TURN {
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
    }
};

template <>
struct AbilityImpl<ABILITY_OVERCOAT> : extends Breakable, extends SandImmune, extends HailImmune, extends PowderImmune, extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.8);
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_TOUCH> : extends AbilityImpl<ABILITY_POISON_POINT> {};

template <>
struct AbilityImpl<ABILITY_REGENERATOR> : extends OnExit {
    ON_EXIT {
        CHECK(IsBattlerAlive(battler)) CHECK_NOT(BATTLER_MAX_HP(battler)) BattleScriptCall(BattleScript_RegeneratorExits);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_BIG_PECKS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsMoveMakingContact(move, battler)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_RUSH> : extends OnStat<>, extends SandImmune {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_WONDER_SKIN> : extends AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_ANALYTIC> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_ILLUSION> : extends OnDefender, extends OnOffensiveMultiplier<> {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(gBattleStruct->illusion[battler].on)
        CHECK_NOT(gBattleStruct->illusion[battler].broken)

            BattleScriptCall(BattleScript_IllusionOff);
        return TRUE;
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleStruct->illusion[battler].on && !gBattleStruct->illusion[battler].broken) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_IMPOSTER> : extends OnEntry {
    ON_ENTRY {
        gBattlerTarget = BATTLE_OPPOSITE(battler);
        if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        CHECK(IsBattlerAlive(gBattlerTarget))
        CHECK_NOT(gBattleMons[gBattlerTarget].status2 & (STATUS2_TRANSFORMED | STATUS2_SUBSTITUTE))
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
        CHECK_NOT(gBattleStruct->illusion[gBattlerTarget].on)
        CHECK_NOT(gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE)

        BattleScriptPushCursorAndCallback(BattleScript_ImposterActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_INFILTRATOR> : extends OnInfiltrate {
    ON_INFILTRATE { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; }
};

template <>
struct AbilityImpl<ABILITY_MUMMY> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability))
        CHECK(IsMoveMakingContact(move, attacker)) CHECK_NOT(IsPersistentOrUnsuppressable(GetBattlerAbility(attacker)))
            CHECK_NOT(DoesBattlerHaveAbilityShield(attacker))

                UpdateAbilityStateIndicesForNewAbility(attacker, ability);
        ReplaceAbility(attacker, ability);
        BattleScriptCall(BattleScript_MummyActivates);
        return TRUE;
    }
};

template <int Stat>
struct MoxieClone : extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(HasAttackerFaintedTarget())
        int stat = Stat == STAT_HIGHEST_TOTAL ? GetHighestStatId(battler, FALSE) : Stat;
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))
        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOXIE> : extends MoxieClone<STAT_ATK> {};

template <>
struct AbilityImpl<ABILITY_JUSTIFIED> : extends AbsorbStatUp<TYPE_DARK, STAT_HIGHEST_ATTACKING> {};

template <>
struct AbilityImpl<ABILITY_RATTLED> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

            SetStatChanger(STAT_SPEED, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MAGIC_BOUNCE> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_SAP_SIPPER> : extends LightningRodClone<TYPE_GRASS> {};

template <>
struct AbilityImpl<ABILITY_PRANKSTER> : extends OnPriority {
    ON_PRIORITY { CHECK(IS_MOVE_STATUS(move)) return 1; }
};

template <>
struct AbilityImpl<ABILITY_SAND_FORCE> : extends SandImmune, extends OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_BARBS> : extends AbilityImpl<ABILITY_ROUGH_SKIN> {};

template <>
struct AbilityImpl<ABILITY_ZEN_MODE> : extends StandardTransformation {};

template <>
struct AbilityImpl<ABILITY_VICTORY_STAR> : extends OnAccuracy<ApplyOnTarget::ALLY> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <Type ExtraType>
struct AddsType : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, ExtraType))

        gBattleMons[battler].type3 = ExtraType;
        PREPARE_TYPE_BUFFER(gBattleTextBuff2, ExtraType);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TURBOBLAZE> : extends AbilityImpl<ABILITY_MOLD_BREAKER>, extends AddsType<TYPE_FIRE> {
    ON_ENTRY { return AddsType<TYPE_FIRE>::onEntry(DELEGATE_ENTRY); }
};

template <>
struct AbilityImpl<ABILITY_TERAVOLT> : extends AbilityImpl<ABILITY_MOLD_BREAKER>, extends AddsType<TYPE_ELECTRIC> {
    ON_ENTRY { return AddsType<TYPE_ELECTRIC>::onEntry(DELEGATE_ENTRY); }
};

template <>
struct AbilityImpl<ABILITY_AROMA_VEIL> : extends OnStatusImmune<ApplyOn::ALLY> {
    ON_STATUS_IMMUNE { CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_FLOWER_VEIL> : extends OnStatusImmune<ApplyOn::ALLY> {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_STATUS1) CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_CHEEK_POUCH> : extends RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_PROTEAN> : extends OnBeforeAttack<> {
    ON_BEFORE_ATTACK {
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, moveType)) CHECK(move != MOVE_STRUGGLE) SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        BattleScriptCall(BattleScript_ProteanActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FUR_COAT> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_BULLETPROOF> : extends OnImmune<> {
    ON_IMMUNE {
        CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STRONG_JAW> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_REFRIGERATE> : extends AteAbility<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_SWEET_VEIL> : extends OnStatusImmune<ApplyOn::ALLY> {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_SLEEP) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_STANCE_CHANGE> : extends FormChangeAbility, extends OnBeforeAttack<> {
    ON_BEFORE_ATTACK {
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
    }
};

template <Type GaleWingsType>
struct GaleWingsLike : extends OnPriority {
    ON_PRIORITY {
        CHECK(GetTypeBeforeUsingMove(move, battler) == GaleWingsType)
        CHECK(BATTLER_MAX_HP(battler))
        return 1;
    }
};
template <>
struct AbilityImpl<ABILITY_GALE_WINGS> : extends GaleWingsLike<TYPE_FLYING> {};

template <>
struct AbilityImpl<ABILITY_MEGA_LAUNCHER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_GRASS_PELT> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_TOUGH_CLAWS> : extends AbilityImpl<ABILITY_BIG_PECKS> {};

template <>
struct AbilityImpl<ABILITY_PIXILATE> : extends AteAbility<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_GOOEY> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
        CHECK(IsMoveMakingContact(move, attacker))

            BattleScriptCall(BattleScript_GooeyActivates);
        gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AERILATE> : extends AteAbility<TYPE_FLYING> {};

template <>
struct AbilityImpl<ABILITY_HYPER_AGGRESSIVE> : extends OnParentalBond {
    ON_PARENTAL_BOND { return PARENTAL_BOND_HYPER_AGGRESSIVE; }
};

struct IgnoresFortKnox {};
template <>
struct AbilityImpl<ABILITY_PARENTAL_BOND> : extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE>, extends IgnoresFortKnox {};

template <>
struct AbilityImpl<ABILITY_DARK_AURA> : extends OnEntry, extends OnOffensiveMultiplier<ApplyOn::ANY> {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA); }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType != TYPE_DARK) return;
        if (IsAbilityOnField(ABILITY_AURA_BREAK))
            MUL(.75);
        else
            MUL(1.33);
    }
};

template <>
struct AbilityImpl<ABILITY_FAIRY_AURA> : extends OnEntry, extends OnOffensiveMultiplier<ApplyOn::ANY> {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA); }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType != TYPE_FAIRY) return;
        if (IsAbilityOnField(ABILITY_AURA_BREAK))
            MUL(.75);
        else
            MUL(1.33);
    }
};

template <>
struct AbilityImpl<ABILITY_AURA_BREAK> : extends Breakable, extends OnEntry {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); }
};

template <>
struct AbilityImpl<ABILITY_PRIMORDIAL_SEA> : extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DESOLATE_LAND> : extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WEATHER_CONTROL> : extends OnImmune<> {
    ON_IMMUNE {
        CHECK(gBattleMoves[move].flags & FLAG_WEATHER_BASED)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DELTA_STREAM> : extends AbilityImpl<ABILITY_WEATHER_CONTROL>, extends OverrideBreakable, extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STAMINA> : extends OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_WIMP_OUT> : extends OnDefender {
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(TestSheerForceFlag(attacker, gCurrentMove))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER) CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
            CHECK(CountUsablePartyMons(battler));
        gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_EMERGENCY_EXIT> : extends AbilityImpl<ABILITY_WIMP_OUT> {};

template <>
struct AbilityImpl<ABILITY_WATER_COMPACTION> : extends OnDefensiveMultiplier, extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_WATER)
        CHECK(CanRaiseStat(battler, STAT_DEF))

            SetStatChanger(STAT_DEF, 2);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MERCILESS> : extends OnCrit<> {
    ON_CRIT {
        if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
        if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
        if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
        return 0;
    }
};

template <>
struct AbilityImpl<ABILITY_SHIELDS_DOWN> : extends StandardTransformation, extends OnAttacker, extends OnStatusImmune<>, extends OverrideBreakable {
    ON_ATTACKER {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
        CHECK(gBattleMoves[move].effect == EFFECT_SHELL_SMASH) CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

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
    }
    ON_STATUS_IMMUNE {
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
    }
};

template <>
struct AbilityImpl<ABILITY_STAKEOUT> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
    }
};

template <>
struct AbilityImpl<ABILITY_WATER_BUBBLE> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) MUL(2.0);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_BURN)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STEELWORKER> : extends Breakable, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends AteAbility<TYPE_STEEL> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_BERSERK> : extends OnDefender {
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker)) CHECK_NOT(GetAbilityState(battler, ability)) int stat = GetHighestAttackingStatId(battler, TRUE);
        CHECK(CanRaiseStat(battler, stat))

        SetAbilityState(battler, ability, TRUE);
        SetStatChanger(stat, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SLUSH_RUSH> : extends HailImmune, extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_LONG_REACH> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move) && !gBattleMoves[move].contact) MUL(1.2);
    }
};

template <Type BoostType>
struct LiquidVoiceClone : extends OnOffensiveMultiplier<>, extends OnMoveType {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsSoundMove(battler, move)) MUL(1.2);
    }
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_NORMAL)
        CHECK(gBattleMoves[move].flags & FLAG_SOUND)
        return BoostType + 1;
    }
};
template <>
struct AbilityImpl<ABILITY_LIQUID_VOICE> : extends LiquidVoiceClone<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_TRIAGE> : extends OnPriority {
    ON_PRIORITY { CHECK(IsHealingMoveEffect(gBattleMoves[move].effect)) return 3; }
};

template <>
struct AbilityImpl<ABILITY_GALVANIZE> : extends AteAbility<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_SURGE_SURFER> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_SCHOOLING> : extends StandardTransformation {
    ON_ENTRY { CHECK(gBattleMons[battler].level >= 20) return StandardTransformation::onEntry(DELEGATE_ENTRY); }
    ON_END_TURN {
        CHECK(gBattleMons[battler].level >= 20)
        return StandardTransformation::onEndTurn(DELEGATE_END_TURN);
    }
};

template <>
struct AbilityImpl<ABILITY_DISGUISE> : extends FormChangeAbility, extends OnEntry, extends OnDisguise, extends OnWeather {
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
    ON_ENTRY { return DisguiseReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_DISGUISE {
        switch (gBattleMons[battler].species) {
            case SPECIES_MIMIKYU:
                return SPECIES_MIMIKYU_BUSTED;
            case SPECIES_MIMIKYU_RAYQUAZA:
                return SPECIES_MIMIKYU_RAYQUAZA_BUSTED;

            default:
                return SPECIES_NONE;
        }
    }
    ON_WEATHER { return DisguiseReformHandler(ability, battler, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_BATTLE_BOND> : extends FormChangeAbility, extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
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
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_CONSTRUCT> : extends FormChangeAbility, extends OnEndTurn {
    ON_END_TURN {
        CHECK(gBattleMons[battler].species == SPECIES_ZYGARDE || gBattleMons[battler].species == SPECIES_ZYGARDE_10)
        CHECK(gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        gBattleStruct->changedSpecies[gBattlerPartyIndexes[battler]] = gBattleMons[battler].species;
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_ZYGARDE_COMPLETE);
        gBattleMons[battler].species = SPECIES_ZYGARDE_COMPLETE;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CORROSION> : extends OnTypeEffectiveness<>, extends OnCanStatusType {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_POISON) CHECK(defType == TYPE_STEEL) *mod = UQ_4_12(2.0);
        return TRUE;
    }
    ON_CAN_STATUS_TYPE {
        CHECK(status & CHECK_POISON)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_COMATOSE> : extends OnEntry, extends RemovesStatusOnImmunity, extends Unsuppressable {
    ON_ENTRY {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COMATOSE;
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceStatusAbility);
        return TRUE;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

static int blocksPriority(ON_IMMUNE_ARGS) {
    CHECK_NOT(gProcessingExtraAttacks)
    CHECK(GetBattlerSide(attacker) != GetBattlerSide(battler))
    CHECK(GetMovePriority(attacker, move, battler) > 0);
    *immunityScript = BattleScript_DazzlingProtected;
    return TRUE;
}
template <>
struct AbilityImpl<ABILITY_QUEENLY_MAJESTY> : extends OnImmune<ApplyOn::ALLY> {
    ON_IMMUNE { return blocksPriority(DELEGATE_IMMUNE); }
};

template <>
struct AbilityImpl<ABILITY_INNARDS_OUT> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(HasMagicGuard(attacker))

            gBattleMoveDamage = gTurnStructs[battler].dmg;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DANCER> : extends OnCopyMove {
    ON_COPY_MOVE { CHECK(IsDance(attacker, move)) return UseOutOfTurnAttack(battler, target, ability, move, 0); }
};

template <>
struct AbilityImpl<ABILITY_BATTERY> : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_FLUFFY> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(2.0);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DAZZLING> : extends AbilityImpl<ABILITY_QUEENLY_MAJESTY> {};

template <>
struct AbilityImpl<ABILITY_SOUL_HEART> : extends OnBattlerFaints<ApplyOnTarget::ANY> {
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TANGLING_HAIR> : extends AbilityImpl<ABILITY_GOOEY> {};

template <>
struct AbilityImpl<ABILITY_RECEIVER> : extends OnBattlerFaints<ApplyOnTarget::ALLY> {
    ON_BATTLER_FAINTS {
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
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_OF_ALCHEMY> : extends OnEntry, extends OnReactive, extends OnBattlerFaints<ApplyOnTarget::ANY> {
    ON_ENTRY {
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
    }
    ON_REACTIVE {
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
    }
    ON_BATTLER_FAINTS {
        int state = GetAbilityState(battler, ability);
        if (state & (3 << fainted)) SetAbilityState(battler, ability, state & ~(3 << fainted));
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_BEAST_BOOST> : extends MoxieClone<STAT_HIGHEST_TOTAL> {};

template <>
struct AbilityImpl<ABILITY_RKS_SYSTEM> : extends AbilityImpl<ABILITY_PROTEAN>, extends AbilityImpl<ABILITY_ADAPTABILITY>, extends FormChangeAbility {};

template <>
struct AbilityImpl<ABILITY_ELECTRIC_SURGE> : extends AllowTerrainIfAirborne<TERRAIN_ELECTRIC>, extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

        for (int i = 0; i < gBattlersCount; i++) {
            DisableSwitchInAbility(i, ABILITY_GENERATOR);
            DisableSwitchInAbility(i, ABILITY_ENERGIZED);
        }
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PSYCHIC_SURGE> : extends AllowTerrainIfAirborne<TERRAIN_PSYCHIC>, extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MISTY_SURGE> : extends AllowTerrainIfAirborne<TERRAIN_MISTY>, extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_GRASSY_SURGE> : extends AllowTerrainIfAirborne<TERRAIN_GRASSY>, extends OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHADOW_SHIELD> : extends AbilityImpl<ABILITY_MULTISCALE>, extends OverrideBreakable {};

template <>
struct AbilityImpl<ABILITY_PRISM_ARMOR> : extends AbilityImpl<ABILITY_FILTER>, extends OverrideBreakable {};

template <>
struct AbilityImpl<ABILITY_NEUROFORCE> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.35);
    }
};

template <int Stat>
struct RaiseStatOnEntry : OnEntry {
    ON_ENTRY {
        CHECK(CanRaiseStat(battler, Stat))

        SetStatChanger(Stat, 1);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
        return TRUE;
    }
};
template <>
struct AbilityImpl<ABILITY_INTREPID_SWORD> : extends RaiseStatOnEntry<STAT_ATK> {};

template <>
struct AbilityImpl<ABILITY_DAUNTLESS_SHIELD> : extends RaiseStatOnEntry<STAT_DEF> {};

template <>
struct AbilityImpl<ABILITY_LIBERO> : extends AbilityImpl<ABILITY_PROTEAN> {};

template <>
struct AbilityImpl<ABILITY_COTTON_DOWN> : extends OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit());
        gStackBattler1 = BATTLE_OPPOSITE(battler);
        CHECK(IsBattlerAlive(gStackBattler1) || IsBattlerAlive(BATTLE_PARTNER(gStackBattler1)))

        gEffectBattler = battler;
        gStackBattler1 = GetOppositeSide(battler);
        BattleScriptCall(BattleScript_CottonDownActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MIRROR_ARMOR> : extends Breakable {};

template <>
struct AbilityImpl<ABILITY_GULP_MISSILE> : extends FormChangeAbility, extends OnDefender, extends OnAttacker {
    ON_ATTACKER {
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
        CHECK(gBattleMons[battler].species == SPECIES_CRAMORANT)
        CHECK(((gCurrentMove == MOVE_SURF || gCurrentMove == MOVE_TRIPLE_DIVE) && TARGET_TURN_DAMAGED) || gStatuses3[battler] & STATUS3_UNDERWATER ||
              (gCurrentMove == MOVE_DIVE && gBattleScripting.acceleratedTwoTurn))

        SpeciesEnum newSpecies = gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 2 ? SPECIES_CRAMORANT_GORGING : SPECIES_CRAMORANT_GULPING;
        UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
        gBattleMons[battler].species = newSpecies;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    }
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        SpeciesEnum species = gBattleMons[battler].species;
        CHECK(species == SPECIES_CRAMORANT_GORGING || species == SPECIES_CRAMORANT_GULPING)
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_CRAMORANT);
        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        BattleScriptCall(species == SPECIES_CRAMORANT_GORGING ? BattleScript_GulpMissileGorging : BattleScript_GulpMissileGulping);
        gBattleMons[battler].species = SPECIES_CRAMORANT;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STEAM_ENGINE> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPEED))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

            SetStatChanger(STAT_SPEED, 12);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AMPLIFIER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsSoundMove(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PUNK_ROCK> : extends OnDefensiveMultiplier, extends AbilityImpl<ABILITY_AMPLIFIER> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IsSoundMove(attacker, move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_SPIT> : extends SandImmune, extends OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_ICE_SCALES> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_ICE_FACE> : extends FormChangeAbility, extends HailImmune, extends OnEntry, extends OnDisguise, extends OnWeather {
    static int IceFaceReformHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
        CHECK(gBattleMons[battler].species == SPECIES_EISCUE_NOICE_FACE)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

            InsertCorrectEndType(callType);
        UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_EISCUE);
        gBattleMons[battler].species = SPECIES_EISCUE;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    }
    ON_ENTRY { return IceFaceReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_DISGUISE { return gBattleMons[battler].species == SPECIES_EISCUE ? SPECIES_EISCUE_NOICE_FACE : SPECIES_NONE; }
    ON_WEATHER { return IceFaceReformHandler(ability, battler, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_POWER_SPOT> : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_OFFENSIVE_MULTIPLIER { MUL(1.3); }
};

template <>
struct AbilityImpl<ABILITY_MIMICRY> : extends OnEntry, extends OnTerrain {
    static int HandleMimicry(u8 battler, AbilityEnum ability, AbilityCallType endType) {
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

    ON_ENTRY {
        CHECK(IsBattlerAlive(battler))

        return HandleMimicry(battler, ability, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    }
    ON_TERRAIN {
        CHECK(IsBattlerAlive(battler))

        return HandleMimicry(battler, ability, ABILITY_BS_CALL);
    }
};

template <>
struct AbilityImpl<ABILITY_SCREEN_CLEANER> : extends OnEntry {
    ON_ENTRY {
        CHECK(TryRemoveScreens(battler))

        return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
    }
};

template <>
struct AbilityImpl<ABILITY_STEELY_SPIRIT> : extends OnOffensiveMultiplier<ApplyOn::ALLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_STEEL) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PERISH_BODY> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(IsBattlerAlive(attacker))
        CHECK(IsMoveMakingContact(move, attacker)) CHECK_NOT(gStatuses3[attacker] & STATUS3_PERISH_SONG)

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
    }
};

template <>
struct AbilityImpl<ABILITY_WANDERING_SPIRIT> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(GetBattlerAbility(battler) == ability)
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability)) CHECK(IsMoveMakingContact(move, attacker))
            CHECK_NOT(IsPersistentOrUnsuppressable(GetBattlerAbility(attacker))) CHECK_NOT(DoesBattlerHaveAbilityShield(attacker))

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
    }
};

template <>
struct AbilityImpl<ABILITY_GORILLA_TACTICS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_NEUTRALIZING_GAS> : extends Unsuppressable {};

template <>
struct AbilityImpl<ABILITY_PASTEL_VEIL> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_SAFEGUARD)

        int side = GetBattlerSide(battler);
        gSideTimers[side].started.safeguard = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_SAFEGUARD;
        gSideTimers[side].safeguardBattlerId = battler;
        gSideTimers[side].safeguardTimer = SCREEN_DURATION;
        BattleScriptPushCursorAndCallback(BattleScript_PastelVeilActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_HUNGER_SWITCH> : extends FormChangeAbility, extends OnEndTurn {
    ON_END_TURN {
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
    }
};

template <>
struct AbilityImpl<ABILITY_CURIOUS_MEDICINE> : extends OnEntry {
    ON_ENTRY {
        CHECK(IsDoubleBattle())
        CHECK(IsBattlerAlive(BATTLE_PARTNER(battler)))
        CHECK(TryResetBattlerStatChanges(BATTLE_PARTNER(battler), RESET_ALL_STATS))

            gEffectBattler = BATTLE_PARTNER(battler);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CURIOUS_MEDICINE;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TRANSISTOR> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRAGONS_MAW> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DRAGON) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_CHILLING_NEIGH> : extends AbilityImpl<ABILITY_MOXIE> {};

template <>
struct AbilityImpl<ABILITY_GRIM_NEIGH> : extends MoxieClone<STAT_SPATK> {};

template <AbilityEnum FaintAbility>
struct AsOne : extends Unsuppressable, extends RandomizerBanned, extends AbilityImpl<ABILITY_UNNERVE>, extends AbilityImpl<FaintAbility> {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE); }
    ON_BATTLER_FAINTS {
        CHECK(AbilityImpl<FaintAbility>::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_CHILLING_NEIGH;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_AS_ONE_ICE_RIDER> : extends AsOne<ABILITY_CHILLING_NEIGH> {};

template <>
struct AbilityImpl<ABILITY_AS_ONE_SHADOW_RIDER> : extends AsOne<ABILITY_GRIM_NEIGH> {};

template <>
struct AbilityImpl<ABILITY_CHLOROPLAST> {};

template <>
struct AbilityImpl<ABILITY_WHITEOUT> : extends HailImmune, extends OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_PYROMANCY> : extends OnModifyEffectChance<> {
    ON_MODIFY_EFFECT_CHANCE {
        if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
    }
};

template <>
struct AbilityImpl<ABILITY_KEEN_EDGE> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PRISM_SCALES> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.7);
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_FISTS> : extends AbilityImpl<ABILITY_IRON_FIST>, extends OnChooseDefensiveStat<> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK(IsIronFistBoosted(battler, move)) return STAT_SPDEF; }
};

template <>
struct AbilityImpl<ABILITY_SAND_SONG> : extends LiquidVoiceClone<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_RAMPAGE> : extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        SetAbilityState(battler, ability, TRUE);
        gVolatileStructs[battler].rechargeTimer = 0;
        gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_VENGEANCE> : extends SwarmLike<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_BLITZ_BOXER> : extends OnPriority {
    ON_PRIORITY {
        CHECK(IsIronFistBoosted(battler, move)) CHECK(BATTLER_MAX_HP(battler));
        return 1;
    }
};

template <>
struct AbilityImpl<ABILITY_ANTARCTIC_BIRD> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_IMMOLATE> : extends AteAbility<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_CRYSTALLIZE> : extends OnOffensiveMultiplier<>, extends OnMoveType {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ICE && gBattleStruct->ateBoost[battler]) MUL(1.1);
    }
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_ROCK)
        *ateBoost = TRUE;
        return TYPE_ICE + 1;
    }
};

template <>
struct AbilityImpl<ABILITY_ELECTROCYTES> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.25);
    }
};

template <>
struct AbilityImpl<ABILITY_AERODYNAMICS> : extends AbsorbStatUp<TYPE_FLYING, STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_CHRISTMAS_SPIRIT> : extends OnDefensiveMultiplier, extends HailImmune {
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_EXPLOIT_WEAKNESS> : extends OnOffensiveMultiplier<>, extends OnChooseDefensiveStat<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(target)) MUL(1.25);
    }
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(HasAnyStatusOrAbility(target))
        u32 def = CalculateStat(target, STAT_DEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        u32 spDef = CalculateStat(target, STAT_SPDEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        if (def < spDef)
            return STAT_DEF;
        else if (spDef < def)
            return STAT_SPDEF;
        else
            return 0;
    }
};

template <>
struct AbilityImpl<ABILITY_GROUND_SHOCK> : extends OnTypeEffectiveness<> {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_ELECTRIC) CHECK(defType == TYPE_GROUND) CHECK_NOT(*mod) *mod = UQ_4_12(.5);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANCIENT_IDOL> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; }
};

template <>
struct AbilityImpl<ABILITY_MYSTIC_POWER> : extends OnStab {
    ON_STAB { return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_PERFECTIONIST> : extends OnPriority, extends OnCrit<> {
    ON_PRIORITY {
        CHECK(gBattleMoves[move].power <= 25) CHECK(gBattleMoves[move].power);
        return 1;
    }
    ON_CRIT {
        CHECK(gBattleMoves[move].power <= 50)
        CHECK(gBattleMoves[move].power)
        return 1;
    }
};

template <>
struct AbilityImpl<ABILITY_GROWING_TOOTH> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

            gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_AttackBoostActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_INFLATABLE> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF)) CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
        BattleScriptCall(BattleScript_InflatableActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AURORA_BOREALIS> : extends HailImmune, extends OnStab {
    ON_STAB { return moveType == TYPE_ICE; }
};

template <>
struct AbilityImpl<ABILITY_AVENGER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_LETS_ROLL> : extends OnEntry {
    ON_ENTRY {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LOUD_BANG> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeConfused(target))
        CHECK(IsSoundMove(battler, move)) CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
    }
};

template <>
struct AbilityImpl<ABILITY_LEAD_COAT> : extends OnDefensiveMultiplier, extends OnStat<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.6);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .9;
    }
};

template <>
struct AbilityImpl<ABILITY_AMPHIBIOUS> : extends OnStab, extends OnStatusImmune<> {
    ON_STAB { return moveType == TYPE_WATER; }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_DRENCH)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_GROUNDED> : extends AddsType<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_EARTHBOUND> : extends SwarmLike<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_FIGHT_SPIRIT> : extends AteAbility<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_COIL_UP> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

        gStatuses4[battler] |= STATUS4_COILED;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FOSSILIZED> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MAGICAL_DUST> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_PSYCHIC))

            gBattleMons[attacker]
                .type3 = TYPE_PSYCHIC;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[attacker].type3);
        BattleScriptCall(BattleScript_AttackerBecameTheType);
        return TRUE;
    }
};

struct UseTurnAttackAsPursuit : OnPreemptAction {
    ON_PREEMPT_ACTION {
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
};

template <>
struct AbilityImpl<ABILITY_DREAMCATCHER> : extends OnOffensiveMultiplier<>, extends UseTurnAttackAsPursuit {
    ON_OFFENSIVE_MULTIPLIER {
        for (int i = 0; i < gBattlersCount; i++) {
            if (IsBattlerAlive(i) && gBattleMons[i].status1 & STATUS1_SLEEP) {
                FILTER_NOT(gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability && gQueuedExtraAttackData[0].target == i) MUL(2.0);
                return;
            }
        }
    }
    ON_PREEMPT_ACTION {
        CHECK(gBattleMons[turnBattler].status1 & STATUS1_SLEEP)
        return UseTurnAttackAsPursuit::onPreemptAction(DELEGATE_PREEMPT_ACTION);
    }
};

template <>
struct AbilityImpl<ABILITY_NOCTURNAL> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK) MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
    }
};

template <>
struct AbilityImpl<ABILITY_SELF_SUFFICIENT> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)

            gBattleMoveDamage = gBattleMons[battler].maxHP / 16;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TECTONIZE> : extends AteAbility<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_ICE_AGE> : extends AddsType<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_HALF_DRAKE> : extends AddsType<TYPE_DRAGON> {};

template <>
struct AbilityImpl<ABILITY_AQUATIC> : extends AddsType<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_LIQUIFIED> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) RESISTANCE(2);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRAGONFLY> : extends AbilityImpl<ABILITY_HALF_DRAKE>, extends GroundImmune {};

template <Type StrongVs>
struct TypeSlayer : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(target, StrongVs)) RESISTANCE(1.5);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(attacker, StrongVs)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRAGONSLAYER> : extends TypeSlayer<TYPE_DRAGON> {};

struct StealthRockImmune {};
template <>
struct AbilityImpl<ABILITY_MOUNTAINEER> : extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends StealthRockImmune {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_ROCK) *mod = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_HYDRATE> : extends AteAbility<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_METALLIC> : extends AddsType<TYPE_STEEL> {};

template <>
struct AbilityImpl<ABILITY_PERMAFROST> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
    }
};

template <>
struct AbilityImpl<ABILITY_PRIMAL_ARMOR> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_RAGING_BOXER> : extends OnParentalBond {
    ON_PARENTAL_BOND { CHECK(IsIronFistBoosted(battler, move)) return PARENTAL_BOND_PRIMAL_MAW; }
};

template <>
struct AbilityImpl<ABILITY_AIR_BLOWER> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND) int side = GetBattlerSide(battler);
        gSideTimers[side].started.tailwind = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_TAILWIND;
        gSideTimers[side].tailwindBattlerId = battler;
        gSideTimers[side].tailwindTimer = TAILWIND_DURATION_SHORT;

        DisableSwitchInAbility(battler, ABILITY_WIND_RIDER);
        DisableSwitchInAbility(BATTLE_PARTNER(battler), ABILITY_WIND_RIDER);

        BattleScriptPushCursorAndCallback(BattleScript_AirBlowerActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_JUGGERNAUT> : extends OnChooseOffensiveStat, extends RemovesStatusOnImmunity {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHORT_CIRCUIT> : extends SwarmLike<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_MAJESTIC_BIRD> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPATK) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_PHANTOM> : extends AddsType<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_INTOXICATE> : extends AteAbility<TYPE_POISON> {};

template <>
struct AbilityImpl<ABILITY_IMPENETRABLE> : extends AbilityImpl<ABILITY_MAGIC_GUARD> {};

template <>
struct AbilityImpl<ABILITY_HYPNOTIST> : extends OnAccuracy<> {
    ON_ACCURACY {
        CHECK(move == MOVE_HYPNOSIS);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_OVERWHELM> : extends OnTypeEffectiveness<>, extends TauntImmune {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SCARE> : extends AbilityImpl<ABILITY_INTIMIDATE> {};

template <>
struct AbilityImpl<ABILITY_MAJESTIC_MOTH> : extends OnEntry {
    ON_ENTRY {
        CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOUL_EATER> : extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK_NOT(BATTLER_MAX_HP(battler));
        CHECK(CanBattlerHeal(battler));
        BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOUL_LINKER> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER)) CHECK(move != MOVE_PAIN_SPLIT)

            BattleScriptCall(BattleScript_AttackerSoulLinker);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SWEET_DREAMS> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gBattleMons[battler].status1 & STATUS1_SLEEP || BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SweetDreamsActivates);
        return TRUE;
    }
};

struct ForcesMinRolls {};

template <>
struct AbilityImpl<ABILITY_BAD_LUCK> : extends Breakable, extends OnCrit<ApplyOnTarget::FOE>, extends OnModifyEffectChance<ApplyOn::FOE>, extends ForcesMinRolls {
    ON_CRIT { return NEVER_CRIT; }
    ON_MODIFY_EFFECT_CHANCE {
        if (*effectChance < 1) *effectChance = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_HAUNTED_SPIRIT> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_GHOST)) CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED) CHECK(IsMoveMakingContact(move, attacker))

            gBattleMons[attacker]
                .status2 |= STATUS2_CURSED;
        BattleScriptCall(BattleScript_HauntedSpiritActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ELECTRIC_BURST> : extends OnRecoil, extends OnOffensiveMultiplier<> {
    ON_RECOIL {
        CHECK(moveType == TYPE_ELECTRIC);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.35);
    }
};

template <>
struct AbilityImpl<ABILITY_RAW_WOOD> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SOLENOGLYPHS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

template <>
struct AbilityImpl<ABILITY_SPIDER_LAIR> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

        int side = GetOppositeSide(battler);
        gSideTimers[side].started.spiderWeb = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[side].stickyWebTimer = 5;
        BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FATAL_PRECISION> : extends OnAccuracy<>, extends OnCrit<> {
    ON_ACCURACY {
        CHECK_NOT(IS_MOVE_STATUS(move))
        CHECK(CalcTypeEffectivenessMultiplier(move, moveType, battler, target, TRUE) >= UQ_4_12(2.0)) return ACCURACY_HITS_IF_POSSIBLE;
    }
    ON_CRIT {
        CHECK(typeEffectiveness >= UQ_4_12(2.0))
        return ALWAYS_CRIT;
    }
};

template <>
struct AbilityImpl<ABILITY_SEAWEED> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS)) RESISTANCE(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_PSYCHIC_MIND> : extends SwarmLike<TYPE_PSYCHIC> {};

template <>
struct AbilityImpl<ABILITY_POISON_ABSORB> : extends Redirects<TYPE_POISON>, extends AbsorbHeal<TYPE_POISON>, extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2) CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SCAVENGER> : extends AbilityImpl<ABILITY_SOUL_EATER> {};

template <>
struct AbilityImpl<ABILITY_TWISTED_DIMENSION> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

        gFieldTimers.started.trickRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MULTI_HEADED> : extends OnParentalBond, extends IgnoresFortKnox {
    ON_PARENTAL_BOND {
        if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
        if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
        return MULTIHIT_SINGLE;
    }
};

template <>
struct AbilityImpl<ABILITY_NORTH_WIND> : extends HailImmune, extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_OVERCHARGE> : extends OnTypeEffectiveness<>, extends OnCanStatusType {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_ELECTRIC) CHECK(defType == TYPE_ELECTRIC) *mod = UQ_4_12(2.0);
        return TRUE;
    }
    ON_CAN_STATUS_TYPE {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VIOLENT_RUSH> : extends OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
    }
};

template <>
struct AbilityImpl<ABILITY_FLAMING_SOUL> : extends GaleWingsLike<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_SAGE_POWER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_BONE_ZONE> : extends OnAfterTypeEffectiveness<> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod >= UQ_4_12(1.0)) return;
        if (*mod == 0) {
            *mod = UQ_4_12(1.0);
            if (mod1) MulModifier(mod, mod1);
            if (mod2) MulModifier(mod, mod2);
            if (mod3) MulModifier(mod, mod3);
        }
        if (*mod < UQ_4_12(1.0)) MulModifier(mod, UQ_4_12(2.0));
    }
};

template <>
struct AbilityImpl<ABILITY_SPEED_FORCE> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
    }
};

template <>
struct AbilityImpl<ABILITY_SEA_GUARDIAN> : extends OnEntry {
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        SetStatChanger(stat, 1);
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOLTEN_DOWN> : extends OnTypeEffectiveness<> {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_FIRE) CHECK(defType == TYPE_ROCK) *mod = UQ_4_12(2.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLOCK> : extends SwarmLike<TYPE_FLYING> {};

template <>
struct AbilityImpl<ABILITY_FIELD_EXPLORER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_STRIKER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsStrikerBoosted(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_FROZEN_SOUL> : extends GaleWingsLike<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_PREDATOR> : extends AbilityImpl<ABILITY_SOUL_EATER> {};

template <>
struct AbilityImpl<ABILITY_LOOTER> : extends AbilityImpl<ABILITY_SOUL_EATER> {};

template <>
struct AbilityImpl<ABILITY_LUNAR_ECLIPSE> : extends AbilityImpl<ABILITY_HYPNOTIST>, extends OnStab {
    ON_STAB { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; }
};

template <>
struct AbilityImpl<ABILITY_SOLAR_FLARE> : extends AbilityImpl<ABILITY_IMMOLATE>, extends AbilityImpl<ABILITY_CHLOROPLAST> {};

template <>
struct AbilityImpl<ABILITY_POWER_CORE> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; }
};

template <>
struct AbilityImpl<ABILITY_SIGHTING_SYSTEM> : extends OnAccuracy<>, extends OnPriority {
    ON_ACCURACY { return ACCURACY_HITS_IF_POSSIBLE; }
    ON_PRIORITY {
        CHECK(gBattleMoves[move].accuracy)
        CHECK(gBattleMoves[move].accuracy < 80);
        return -3;
    }
};

template <>
struct AbilityImpl<ABILITY_BAD_COMPANY> : extends RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_OPPORTUNIST> : extends OnPriority {
    ON_PRIORITY { CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2) return 1; }
};

template <>
struct AbilityImpl<ABILITY_GIANT_WINGS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].airBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_MOMENTUM> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
    }
};

template <>
struct AbilityImpl<ABILITY_GRIP_PINCER> : extends OnAttacker, extends OnAccuracy<> {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(gBattlerTarget))
        CHECK(IsBattlerAlive(battler))
        CHECK(IsMoveMakingContact(move, battler)) CHECK_NOT(gBattleMons[target].status2 & STATUS2_WRAPPED) CHECK(Random() % 2)

            gBattleMons[target]
                .status2 |= STATUS2_WRAPPED;
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_GRIP_CLAW)
            gVolatileStructs[target].wrapTurns = 7;
        else
            gVolatileStructs[target].wrapTurns = (Random() % 2) + 4;

        gBattleStruct->wrappedMove[target] = gCurrentMove;
        gBattleStruct->wrappedBy[target] = battler;
        BattleScriptCall(BattleScript_GripPincerActivated);
        return TRUE;
    }
    ON_ACCURACY {
        CHECK(gBattleMons[target].status2 & STATUS2_WRAPPED)
        return ACCURACY_ALWAYS_HITS;
    }
};

template <>
struct AbilityImpl<ABILITY_BIG_LEAVES> : extends AbilityImpl<ABILITY_CHLOROPLAST>,
                                         extends Merged<ABILITY_SOLAR_POWER, ABILITY_CHLOROPHYLL>,
                                         extends Merged<ABILITY_HARVEST, ABILITY_LEAF_GUARD> {};

template <>
struct AbilityImpl<ABILITY_PRECISE_FIST> : extends OnCrit<>, extends OnModifyEffectChance<> {
    ON_CRIT { CHECK(IsIronFistBoosted(battler, move)) return 1; }
    ON_MODIFY_EFFECT_CHANCE {
        if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
    }
};

template <>
struct AbilityImpl<ABILITY_DEADEYE> : extends OnAccuracy<>, extends OnChooseDefensiveStat<> {
    ON_ACCURACY { CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].arrowBased) return ACCURACY_HITS_IF_POSSIBLE; }
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(gIsCriticalHit)
        u32 def = CalculateStat(target, STAT_DEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        u32 spDef = CalculateStat(target, STAT_SPDEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        if (def < spDef)
            return STAT_DEF;
        else if (spDef < def)
            return STAT_SPDEF;
        else
            return 0;
    }
};

template <>
struct AbilityImpl<ABILITY_ARTILLERY> : extends OnAccuracy<> {
    ON_ACCURACY { CHECK(IsMegaLauncherBoosted(battler, move)) return ACCURACY_HITS_IF_POSSIBLE; }
};

template <>
struct AbilityImpl<ABILITY_ICE_DEW> : extends LightningRodClone<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_SUN_WORSHIP> : extends OnEntry {
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_POLLINATE> : extends AteAbility<TYPE_BUG> {};

template <>
struct AbilityImpl<ABILITY_VOLCANO_RAGE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_COLD_REBOUND> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
        return FALSE;
    }
};

template <MoveEnum Move, int Power = 0>
struct SimpleEntryMove : extends OnEntry {
    ON_ENTRY { return UseEntryMove(battler, ability, Move, Power); }
};
template <>
struct AbilityImpl<ABILITY_LOW_BLOW> : extends SimpleEntryMove<MOVE_FEINT_ATTACK, 40> {};

template <>
struct AbilityImpl<ABILITY_SPECTRALIZE> : extends AteAbility<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_SPECTRAL_SHROUD> : extends AbilityImpl<ABILITY_SPECTRALIZE>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleStruct->ateBoost[battler]) CHECK(moveType == TYPE_GHOST) CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

template <>
struct AbilityImpl<ABILITY_DISCIPLINE> : extends RemovesStatusOnImmunity, extends TauntImmune {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_CONFUSION) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_THUNDERCALL> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
    }
};

template <>
struct AbilityImpl<ABILITY_MARINE_APEX> : extends AbilityImpl<ABILITY_INFILTRATOR>, extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MIGHTY_HORN> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hornBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_HARDENED_SHEATH> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].hornBased)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptCall(BattleScript_AttackBoostActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ARCTIC_FUR> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER { MUL(.65); }
};

template <>
struct AbilityImpl<ABILITY_LETHARGY> : extends OnEntry, extends OnOffensiveMultiplier<> {
    ON_ENTRY {
        TryResetBattlerStatChanges(battler, RESET_ALL_STATS);
        gVolatileStructs[battler].slowStartTimer = 5;
        BattleScriptPushCursorAndCallback(BattleScript_LethargyEnters);
        return TRUE;
    }
    ON_OFFENSIVE_MULTIPLIER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_BARRAGE> : extends AbilityImpl<ABILITY_MEGA_LAUNCHER>, extends AbilityImpl<ABILITY_SIGHTING_SYSTEM> {};

template <>
struct AbilityImpl<ABILITY_STEEL_BARREL> : extends AbilityImpl<ABILITY_ROCK_HEAD> {};

template <>
struct AbilityImpl<ABILITY_PYRO_SHELLS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(IsMegaLauncherBoosted(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_FUNGAL_INFECTION> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED) CHECK(IsMoveMakingContact(move, battler))

            gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PARRY> : extends OnDefender, extends OnDefensiveMultiplier, extends OverrideBreakable {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
        return FALSE;
    }
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
};

template <>
struct AbilityImpl<ABILITY_SCRAPYARD> : extends OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

            BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LOOSE_QUILLS> : extends AbilityImpl<ABILITY_SCRAPYARD> {};

template <>
struct AbilityImpl<ABILITY_TOXIC_DEBRIS> : extends OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

            BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ROUNDHOUSE> : extends OnAccuracy<>, extends OnChooseDefensiveStat<> {
    ON_ACCURACY { CHECK(IsStrikerBoosted(battler, move)) return ACCURACY_HITS_IF_POSSIBLE; }
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(IsStrikerBoosted(battler, move))
        u32 def = CalculateStat(target, STAT_DEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        u32 spDef = CalculateStat(target, STAT_SPDEF, 0, move, FALSE, ignoreDefensiveStatBoosts, battlerUnaware, FALSE);
        if (def < spDef)
            return STAT_DEF;
        else if (spDef < def)
            return STAT_SPDEF;
        else
            return 0;
    }
};

template <>
struct AbilityImpl<ABILITY_MINERALIZE> : extends AteAbility<TYPE_ROCK> {};

template <>
struct AbilityImpl<ABILITY_LOOSE_ROCKS> : extends OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

            BattleScriptCall(BattleScript_DefenderSetsStealthRock);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SPINNING_TOP> : extends OnAttacker {
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_RETRIBUTION_BLOW> : extends OnReactive {
    ON_REACTIVE {
        CHECK_NOT(gTurnStructs[battler].dancerUsedMove)
        CHECK(IsBattlerAlive(gBattlerAttacker))
        CHECK(gCurrentTurnActionNumber < gBattlersCount || gProcessingExtraAttacks)
            CHECK(gBattleStruct->statStageCheckState != STAT_STAGE_CHECK_NOT_NEEDED) for (int stat = STAT_ATK; stat < NUM_STATS; stat++) {
            if (gBattleStruct->statChangesToCheck[gBattlerAttacker][stat - 1] > 0) {
                UseOutOfTurnAttack(battler, gBattlerAttacker, ability, MOVE_HYPER_BEAM, 0);
                return FALSE;
            }
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_FEARMONGER> : extends AbilityImpl<ABILITY_INTIMIDATE>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(IsMoveMakingContact(move, battler)) CHECK(Random() % 100 < 10)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
};

template <>
struct AbilityImpl<ABILITY_TOXIC_SPILL> : extends OnEntry, extends OnEndTurn, extends OnExit {
    ON_ENTRY {
        CHECK_NOT(getMonotypeChampType() == TYPE_POISON) BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
        return TRUE;
    }
    ON_END_TURN {
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
            FILTER_NOT(HasMagicGuard(target))
            FILTER_NOT(BATTLER_HAS_ABILITY(battler, ABILITY_TOXIC_BOOST))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_ToxicWasteTurnDmg);
            any = TRUE;
        }
        return any;
    }
    ON_EXIT {
        CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
        BattleScriptCall(BattleScript_TheToxicWasHasDissapeared);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DESERT_CLOAK> : extends OnStatusImmune<ApplyOn::ALLY>, extends SandImmune {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_STATUS1) CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_DRACONIZE> : extends AteAbility<TYPE_DRAGON> {};

template <>
struct AbilityImpl<ABILITY_PRETTY_PRINCESS> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (!HasUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SELF_REPAIR> : extends AbilityImpl<ABILITY_SELF_SUFFICIENT>, extends AbilityImpl<ABILITY_NATURAL_CURE> {};

template <>
struct AbilityImpl<ABILITY_ELECTROMORPHOSIS> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ATOMIC_BURST> : extends AbilityImpl<ABILITY_ELECTROMORPHOSIS>, extends AbilityImpl<ABILITY_GALVANIZE> {};

template <int BoostType>
struct BoostedSwarmLike : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (move == BoostType) {
            if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                MUL(1.8);
            else
                MUL(1.3);
        }
    }
};
template <>
struct AbilityImpl<ABILITY_HELLBLAZE> : extends BoostedSwarmLike<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_RIPTIDE> : extends BoostedSwarmLike<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_FOREST_RAGE> : extends BoostedSwarmLike<TYPE_GRASS> {};

template <>
struct AbilityImpl<ABILITY_PRIMAL_MAW> : extends OnParentalBond {
    ON_PARENTAL_BOND { CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) return PARENTAL_BOND_PRIMAL_MAW; }
};

template <>
struct AbilityImpl<ABILITY_SWEEPING_EDGE> : extends OnAccuracy<> {
    ON_ACCURACY { CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) return ACCURACY_HITS_IF_POSSIBLE; }
};

template <>
struct AbilityImpl<ABILITY_GIFTED_MIND> : extends OnAccuracy<>, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    ON_ACCURACY { CHECK(IS_MOVE_STATUS(move)) return ACCURACY_HITS_IF_POSSIBLE; }
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_BUG || moveType == TYPE_GHOST || moveType == TYPE_DARK) *mod = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_HYDRO_CIRCUIT> : extends AbilityImpl<ABILITY_TRANSISTOR>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler)) CHECK(moveType == TYPE_WATER)

            gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_EQUINOX> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
        int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
        if (atk > spAtk)
            *atkStatToUse = STAT_ATK;
        else if (spAtk > atk)
            *atkStatToUse = STAT_SPATK;
    }
};

template <>
struct AbilityImpl<ABILITY_ABSORBANT> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED) CHECK(gBattleMoves[move].effect == EFFECT_ABSORB || gBattleMoves[move].effect == EFFECT_DREAM_EATER)

            gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CLUELESS> : extends AbilityImpl<ABILITY_CLOUD_NINE>, extends Unsuppressable {};

template <int N>
struct NoDamageHits : extends Persistent, extends OnEntry, extends Breakable {
    ON_ENTRY {
        int uses = N - GetSingleUseAbilityCounter(battler, ability);
        CHECK(uses)

        if (uses == 1)
            BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
        else if (N > 1 && uses > 1) {
            ConvertIntToDecimalStringN(gBattleTextBuff4, uses, STR_CONV_MODE_LEFT_ALIGN, 2);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerHasNoDamageHits);
        }
        return TRUE;
    }

    virtual int noDamageHits() { return N; }
};
template <>
struct AbilityImpl<ABILITY_CHEATING_DEATH> : extends NoDamageHits<2>, extends OverrideBreakable {};

template <>
struct AbilityImpl<ABILITY_CHEAP_TACTICS> : extends SimpleEntryMove<MOVE_SCRATCH> {};

template <>
struct AbilityImpl<ABILITY_COWARD> : extends OnEntry, extends Persistent {
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        SetSingleUseAbilityCounter(battler, ability, TRUE);
        gRoundStructs[battler].protectedThisTurn = TRUE;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VOLT_RUSH> : extends GaleWingsLike<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_DUNE_TERROR> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier, extends SandImmune {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GROUND) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
    }
};

template <>
struct AbilityImpl<ABILITY_INFERNAL_RAGE> : extends OnRecoil, extends OnOffensiveMultiplier<> {
    ON_RECOIL {
        CHECK(moveType == TYPE_FIRE);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) MUL(1.35);
    }
};

template <>
struct AbilityImpl<ABILITY_DUAL_WIELD> : extends OnParentalBond {
    ON_PARENTAL_BOND {
        CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return PARENTAL_BOND_DUAL_WIELD;
    }
};

template <>
struct AbilityImpl<ABILITY_ELEMENTAL_CHARGE> : extends OnAttacker {
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_AMBUSH> : extends OnCrit<> {
    ON_CRIT { CHECK(gVolatileStructs[battler].isFirstTurn) return ALWAYS_CRIT; }
};

template <>
struct AbilityImpl<ABILITY_ATLAS> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RADIANCE> : extends OnImmune<ApplyOn::ANY>, extends OnAccuracy<> {
    ON_IMMUNE {
        CHECK(moveType == TYPE_DARK);
        *immunityScript = BattleScript_RadianceProtected;
        return TRUE;
    }
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_JAWS_OF_CARNAGE> : extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler)) if (gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST) BattleScriptCall(BattleScript_HandleJawsOfCarnageEffect);
        else BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANGELS_WRATH>
    : extends OnAttacker, extends OnAccuracy<>, extends OnTypeEffectiveness<>, extends OnModifyEffectChance<>, extends OnCanStatusType {
    ON_ATTACKER {
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
    }
    ON_ACCURACY {
        switch (move) {
            case MOVE_TACKLE:
            case MOVE_POISON_STING:
            case MOVE_ELECTROWEB:
            case MOVE_BUG_BITE:
                return ACCURACY_HITS_IF_POSSIBLE;

            default:
                return ACCURACY_NO_RESULT;
        }
    }
    ON_TYPE_EFFECTIVENESS {
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
    }
    ON_MODIFY_EFFECT_CHANCE {
        if (move == MOVE_POISON_STING) *effectChance = 100;
    }
    ON_CAN_STATUS_TYPE {
        CHECK(status & CHECK_POISON)
        CHECK(move == MOVE_POISON_STING)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PRISMATIC_FUR>
    : extends AbilityImpl<ABILITY_COLOR_CHANGE>, extends AbilityImpl<ABILITY_PROTEAN>, extends OnDefensiveMultiplier, extends OverrideBreakable {
    ON_DEFENSIVE_MULTIPLIER { MUL(.5); }
    ON_BEFORE_ATTACK {
        if (battler == attacker)
            return AbilityImpl<ABILITY_PROTEAN>::onBeforeAttack(DELEGATE_BEFORE_ATTACK);
        else
            return AbilityImpl<ABILITY_COLOR_CHANGE>::onBeforeAttack(DELEGATE_BEFORE_ATTACK);
    }
    ApplyOnTarget onBeforeAttackFor() const override { return ApplyOnTarget::ATTACKER_OR_TARGET; }
};

template <>
struct AbilityImpl<ABILITY_SHOCKING_JAWS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
};

template <>
struct AbilityImpl<ABILITY_FAE_HUNTER> : extends TypeSlayer<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_GRAVITY_WELL> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_EVAPORATE> : extends OnAbsorb {
    ON_ABSORB { CHECK(moveType == TYPE_WATER) return ABSORB_RESULT_EVAPORATE; }
};

template <>
struct AbilityImpl<ABILITY_LUMBERJACK> : extends TypeSlayer<TYPE_GRASS> {};

struct AbsorbUp2 {};
template <>
struct AbilityImpl<ABILITY_WELL_BAKED_BODY> : extends AbsorbStatUp<TYPE_FIRE, STAT_DEF>, extends AbsorbUp2 {};

template <>
struct AbilityImpl<ABILITY_FURNACE> : extends OnEntry, extends OnDefender {
    ON_ENTRY {
        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        CHECK(gSideTimers[GetBattlerSide(battler)].stealthRockType == TYPE_ROCK)
        CHECK(IsBattlerAlive(battler))
        CHECK(ChangeStatBuffs(battler, 2, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 2);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ROCKY_PAYLOAD> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_EARTH_EATER> : extends AbsorbHeal<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_LINGERING_AROMA> : extends AbilityImpl<ABILITY_MUMMY> {};

template <>
struct AbilityImpl<ABILITY_FAIRY_TALE> : extends AddsType<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_RAGING_MOTH> : extends OnParentalBond {
    ON_PARENTAL_BOND { CHECK(moveType == TYPE_FIRE) return PARENTAL_BOND_DUAL_WIELD; }
};

template <>
struct AbilityImpl<ABILITY_ADRENALINE_RUSH> : extends MoxieClone<STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_ARCHMAGE> : extends RandomizerBanned, extends OnAttacker {
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_CRYOMANCY> : extends OnModifyEffectChance<> {
    ON_MODIFY_EFFECT_CHANCE {
        if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
    }
};

template <>
struct AbilityImpl<ABILITY_PHANTOM_PAIN> : extends OnTypeEffectiveness<> {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_GHOST) CHECK(defType == TYPE_NORMAL) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PURGATORY> : extends BoostedSwarmLike<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_EMANATE> : extends AteAbility<TYPE_PSYCHIC> {};

template <>
struct AbilityImpl<ABILITY_KUNOICHI_BLADE> : extends AbilityImpl<ABILITY_TECHNICIAN>, extends AbilityImpl<ABILITY_SKILL_LINK> {};

template <>
struct AbilityImpl<ABILITY_MONKEY_BUSINESS> : extends SimpleEntryMove<MOVE_TICKLE> {};

template <>
struct AbilityImpl<ABILITY_COMBAT_SPECIALIST> : extends Merged<ABILITY_IRON_FIST, ABILITY_STRIKER> {};

template <>
struct AbilityImpl<ABILITY_JUNGLES_GUARD> : extends AbilityImpl<ABILITY_FLOWER_VEIL> {};

template <>
struct AbilityImpl<ABILITY_HUNTERS_HORN> : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_MIGHTY_HORN> {};

template <>
struct AbilityImpl<ABILITY_PIXIE_POWER> : extends AbilityImpl<ABILITY_FAIRY_AURA>, extends OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_PLASMA_LAMP> : extends OnOffensiveMultiplier<>, extends OnAccuracy<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC) MUL(1.2);
    }
    ON_ACCURACY {
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC)
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_MAGMA_EATER> : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_MOLTEN_DOWN> {};

template <>
struct AbilityImpl<ABILITY_SUPER_HOT_GOO> : extends Merged<ABILITY_GOOEY, ABILITY_FLAME_BODY> {};

template <>
struct AbilityImpl<ABILITY_NIKA> : extends AbilityImpl<ABILITY_IRON_FIST> {};

template <>
struct AbilityImpl<ABILITY_ARCHER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].arrowBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPER_SLAMMER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hammerBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_INVERSE_ROOM> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

        gFieldTimers.started.inverseRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
        gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FROST_BURN> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
    }
};

template <>
struct AbilityImpl<ABILITY_ITCHY_DEFENSE> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_WRAPPED)

            gBattleMons[attacker]
                .status2 |= STATUS2_WRAPPED;
        if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_GRIP_CLAW)
            gVolatileStructs[attacker].wrapTurns = 7;
        else
            gVolatileStructs[attacker].wrapTurns = (Random() % 2) + 4;

        gBattleStruct->wrappedMove[attacker] = MOVE_INFESTATION;
        gBattleStruct->wrappedBy[attacker] = battler;

        BattleScriptCall(BattleScript_AttackerBecameInfested);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_GENERATOR> : extends OnEntry, extends OnTerrain, extends OnExit, extends Persistent {
    ON_ENTRY {
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
    }
    ON_TERRAIN {
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)
        CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))

        gStackBattler1 = battler;
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    }
    ON_EXIT {
        CHECK(gStatuses3[battler] & STATUS3_CHARGED_UP)
        SetSingleUseAbilityCounter(battler, ability, FALSE);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOON_SPIRIT> : extends OnStab {
    ON_STAB { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; }
};

template <>
struct AbilityImpl<ABILITY_DUST_CLOUD> : extends SimpleEntryMove<MOVE_SAND_ATTACK> {};

template <>
struct AbilityImpl<ABILITY_TIPPING_POINT> : extends OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_BERSERKER_RAGE> : extends AbilityImpl<ABILITY_TIPPING_POINT>, extends AbilityImpl<ABILITY_RAMPAGE> {};

template <>
struct AbilityImpl<ABILITY_TRICKSTER> : extends SimpleEntryMove<MOVE_DISABLE> {};

template <>
struct AbilityImpl<ABILITY_SAND_GUARD> : extends OnImmune<>, extends OnDefensiveMultiplier, extends SandImmune {
    ON_IMMUNE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
        return blocksPriority(DELEGATE_IMMUNE);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_NATURAL_RECOVERY> : extends Merged<ABILITY_NATURAL_CURE, ABILITY_REGENERATOR> {};

template <>
struct AbilityImpl<ABILITY_WIND_RIDER> : extends OnEntry, extends OnAbsorb {
    ON_ENTRY {
        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND)
        CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE)))

        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityHighestAttackingStatRaiseOnSwitchIn);
        return TRUE;
    }
    ON_ABSORB {
        CHECK(gBattleMoves[move].airBased)
        *statId = GetHighestAttackingStatId(battler, TRUE);
        return ABSORB_RESULT_STAT;
    }
};

template <>
struct AbilityImpl<ABILITY_SOOTHING_AROMA> : extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_PRIM_AND_PROPER> : extends AbilityImpl<ABILITY_WONDER_SKIN>, extends AbilityImpl<ABILITY_CUTE_CHARM> {};

template <>
struct AbilityImpl<ABILITY_SUPER_STRAIN> : extends OnRecoil, OnBattlerFaints<> {
    ON_RECOIL {
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_STRAIN;
        return max(damage / 4, 1);
    }
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, -1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS | MOVE_EFFECT_CERTAIN, NULL))
        BattleScriptCall(BattleScript_LowerStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ENLIGHTENED> : extends AbilityImpl<ABILITY_EMANATE>, extends AbilityImpl<ABILITY_INNER_FOCUS> {};

template <>
struct AbilityImpl<ABILITY_PEACEFUL_SLUMBER> : extends AbilityImpl<ABILITY_SWEET_DREAMS>, extends AbilityImpl<ABILITY_SELF_SUFFICIENT> {
    ON_END_TURN {
        if (!AbilityImpl<ABILITY_SWEET_DREAMS>::onEndTurn(DELEGATE_END_TURN)) return AbilityImpl<ABILITY_SELF_SUFFICIENT>::onEndTurn(DELEGATE_END_TURN);
        gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AFTERSHOCK> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
    }
};

template <>
struct AbilityImpl<ABILITY_FREEZING_POINT> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanGetFrostbite(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker)) CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRYO_PROFICIENCY> : extends AbilityImpl<ABILITY_FREEZING_POINT> {
    int CryoProficiencyHail(AbilityEnum ability, int battler, int attacker, MoveEnum move, int moveType) {
        CHECK(ShouldApplyOnHitAffect(battler)) CHECK_NOT(gBattleWeather & WEATHER_HAIL_ANY) if (gBattleWeather & WEATHER_PRIMAL_ANY) {
            BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
            return NO_ANNOUNCE;
        }
        else if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE)) {
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_CryoProficiencyActivates);
            return TRUE;
        }
        return FALSE;
    }
    ON_DEFENDER { return AbilityImpl<ABILITY_FREEZING_POINT>::onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(ability, battler, attacker, move, moveType); }
};

template <>
struct AbilityImpl<ABILITY_ARCANE_FORCE> : extends AbilityImpl<ABILITY_MYSTIC_POWER>, extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
    }
};

template <>
struct AbilityImpl<ABILITY_DOOMBRINGER> : extends SimpleEntryMove<MOVE_DOOM_DESIRE> {};

template <>
struct AbilityImpl<ABILITY_WISHMAKER> : extends OnEntry, extends Persistent {
    ON_ENTRY {
        int counter = GetSingleUseAbilityCounter(battler, ability);
        CHECK(counter < 3)
        CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

        SetSingleUseAbilityCounter(battler, ability, counter + 1);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_YUKI_ONNA> : extends AbilityImpl<ABILITY_INTIMIDATE>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanInfatuate(battler, target))
        CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPPRESS> : extends SimpleEntryMove<MOVE_TORMENT> {};

template <>
struct AbilityImpl<ABILITY_REFRIGERATOR> : extends AbilityImpl<ABILITY_FILTER>, extends AbilityImpl<ABILITY_ILLUMINATE> {};

template <>
struct AbilityImpl<ABILITY_HEAVEN_ASUNDER> : extends OnCrit<> {
    ON_CRIT {
        if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
        return 1;
    }
};

template <>
struct AbilityImpl<ABILITY_PURIFYING_WATERS> : extends AbilityImpl<ABILITY_WATER_VEIL>, extends AbilityImpl<ABILITY_HYDRATION> {};

template <>
struct AbilityImpl<ABILITY_SEABORNE> : extends AbilityImpl<ABILITY_DRIZZLE>, extends AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_HIGH_TIDE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_WATER)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_CHANGE_OF_HEART> : extends SimpleEntryMove<MOVE_HEART_SWAP> {};

template <>
struct AbilityImpl<ABILITY_MYSTIC_BLADES> : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends OnSwapSplit {
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL) CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return TRUE;
    }
};

struct NegateFrzSpatkDrop {};
template <>
struct AbilityImpl<ABILITY_DETERMINATION> : extends OnOffensiveMultiplier<>, extends NegateFrzSpatkDrop {
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_FERTILIZE> : extends AteAbility<TYPE_GRASS> {};

struct InfatuatesAny {};
template <>
struct AbilityImpl<ABILITY_PURE_LOVE> : extends OnDefender, extends OnAttacker, extends InfatuatesAny {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler)) CHECK(gBattleMons[target].status2 & STATUS2_INFATUATION)

            gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
    ON_DEFENDER { return AbilityImpl<ABILITY_CUTE_CHARM>::onEither(DELEGATE_DEFENDER); }
};

template <>
struct AbilityImpl<ABILITY_FIGHTER> : extends SwarmLike<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_TELEKINETIC> : extends SimpleEntryMove<MOVE_TELEKINESIS> {};

template <>
struct AbilityImpl<ABILITY_COMBUSTION> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_PONY_POWER> : extends Merged<ABILITY_KEEN_EDGE, ABILITY_MYSTIC_BLADES> {};

template <>
struct AbilityImpl<ABILITY_POWDER_BURST> : extends SimpleEntryMove<MOVE_POWDER> {};

template <>
struct AbilityImpl<ABILITY_RETRIEVER> : extends OnExit {
    ON_EXIT {
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
    }
};

template <>
struct AbilityImpl<ABILITY_MONSTER_MASH> : extends SimpleEntryMove<MOVE_TRICK_OR_TREAT> {};

template <>
struct AbilityImpl<ABILITY_TWO_STEP> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_SPITEFUL> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker)) CHECK(gBattleMons[attacker].pp[gChosenMovePos])

            BattleScriptCall(BattleScript_AbilitySpiteful);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FORTITUDE> : extends OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_DEVOURER> : extends AbilityImpl<ABILITY_PRIMAL_MAW>, extends AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_PHANTOM_THIEF> : extends SimpleEntryMove<MOVE_SPECTRAL_THIEF, 40> {};

template <>
struct AbilityImpl<ABILITY_EARLY_GRAVE> : extends GaleWingsLike<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_BASS_BOOSTED> : extends Merged<ABILITY_PUNK_ROCK, ABILITY_AMPLIFIER> {};

template <>
struct AbilityImpl<ABILITY_FLAMING_JAWS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_BURN);
    }
};

template <>
struct AbilityImpl<ABILITY_MONSTER_HUNTER> : extends TypeSlayer<TYPE_DARK> {};

template <>
struct AbilityImpl<ABILITY_CROWNED_SWORD> : extends AbilityImpl<ABILITY_INTREPID_SWORD>, extends AbilityImpl<ABILITY_ANGER_POINT> {};

template <>
struct AbilityImpl<ABILITY_CROWNED_SHIELD> : extends AbilityImpl<ABILITY_DAUNTLESS_SHIELD>, extends AbilityImpl<ABILITY_STAMINA> {};

template <>
struct AbilityImpl<ABILITY_BERSERK_DNA> : extends OnEntry {
    ON_ENTRY {
        CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE))) if (CanBeConfused(battler)) {
            gBattleMons[battler].status2 |= STATUS2_CONFUSION_TURN(3);
            BattleScriptPushCursorAndCallback(BattleScript_BerserkDNA);
        }
        else {
            BattleScriptPushCursorAndCallback(BattleScript_BerserkDNANoConfusion);
        }
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CROWNED_KING> : extends AbilityImpl<ABILITY_AS_ONE_ICE_RIDER>, extends AbilityImpl<ABILITY_AS_ONE_SHADOW_RIDER> {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); }
    ON_BATTLER_FAINTS {
        CHECK(AbilityImpl<ABILITY_CHILLING_NEIGH>::onBattlerFaints(DELEGATE_BATTLER_FAINTS) |
              AbilityImpl<ABILITY_GRIM_NEIGH>::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_CROWNED_KING;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_SNAP_TRAP_WHEN_HIT> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_PERMANENCE> : extends OnEntry {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); }
};

template <>
struct AbilityImpl<ABILITY_HUBRIS> : extends AbilityImpl<ABILITY_GRIM_NEIGH> {};

template <>
struct AbilityImpl<ABILITY_COSMIC_DAZE> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
    }
};

template <>
struct AbilityImpl<ABILITY_MINDS_EYE> : extends Breakable, extends HitsGhost {};

template <>
struct AbilityImpl<ABILITY_BLOOD_PRICE> : extends OnEndTurn, extends OnOffensiveMultiplier<> {
    ON_END_TURN {
        CHECK_NOT(IS_MOVE_STATUS(gLastResultingMoves[battler]))
        CHECK_NOT(HasMagicGuard(battler))
        CHECK(IsBattlerAlive(battler))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 10;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        BattleScriptPushCursorAndCallback(BattleScript_AbilitySelfDamage);
        return TRUE;
    }
    ON_OFFENSIVE_MULTIPLIER { MUL(1.3); }
};

template <>
struct AbilityImpl<ABILITY_SPIKE_ARMOR> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBleed(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker)) CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VOODOO_POWER> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IS_MOVE_SPECIAL(move))
        CHECK(CanBleed(attacker)) CHECK(Random() % 100 < 30)

            AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CHROME_COAT> : extends OnDefensiveMultiplier, extends OnStat<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.6);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .9;
    }
};

template <>
struct AbilityImpl<ABILITY_BANSHEE> : LiquidVoiceClone<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_WEB_SPINNER> : extends SimpleEntryMove<MOVE_STRING_SHOT> {};

template <>
struct AbilityImpl<ABILITY_SHOWDOWN_MODE> : extends OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
    }
};

template <>
struct AbilityImpl<ABILITY_SEED_SOWER> : extends OnDefender, extends AllowTerrainIfAirborne<TERRAIN_GRASSY> {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptCall(BattleScript_SeedSower);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AIRBORNE> : extends OnOffensiveMultiplier<ApplyOn::ALLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PARROTING> : extends AbilityImpl<ABILITY_SOUNDPROOF>, extends OnCopyMove {
    ON_COPY_MOVE { CHECK(IsSoundMove(attacker, move)) return UseOutOfTurnAttack(battler, target, ability, move, 0); }
};

template <>
struct AbilityImpl<ABILITY_SALT_CIRCLE> : extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_PURIFYING_SALT> : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GHOST) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

struct ParadoxBoostEffect : extends OnStat<>, extends OnEntry {
   public:
    static int handler(AbilityEnum ability, int battler, bool weatherState, int weatherMessage, AbilityCallType callType) {
        ParadoxBoost state = GetAbilityStateAs(battler, ability).paradoxBoost;

        if (state.source == PARADOX_BOOST_NOT_ACTIVE && weatherState) {
            InsertCorrectEndType(callType);
            ParadoxBoost boost = {.source = PARADOX_WEATHER_ACTIVE, .statId = GetHighestStatId(battler, TRUE)};
            SetAbilityStateAs(battler, ability, (AbilityStates){.paradoxBoost = boost});
            SetStatChanger(boost.statId, 0);
            gBattleCommunication[MULTISTRING_CHOOSER] = weatherMessage;
            BattleScriptCall(BattleScript_ParadoxBoostActivatesRet);
            return TRUE;
        }

        if (state.source == PARADOX_WEATHER_ACTIVE && !weatherState) {
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

    ON_STAT {
        ParadoxBoost boost = GetAbilityStateAs(battler, ability).paradoxBoost;
        if (!boost.source || boost.statId != statId) return;
        if (statId == STAT_SPEED)
            *stat *= 1.5;
        else
            *stat *= 1.3;
    }
};

template <>
struct AbilityImpl<ABILITY_PROTOSYNTHESIS> : extends ParadoxBoostEffect, extends OnWeather {
    ON_ENTRY { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_WEATHER { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_QUARK_DRIVE> : extends ParadoxBoostEffect, extends OnTerrain {
    ON_ENTRY {
        return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    }
    ON_TERRAIN { return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_WIND_POWER> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].airBased)
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_IMPULSE> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
    }
};

template <>
struct AbilityImpl<ABILITY_TERMINAL_VELOCITY> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
    }
};

template <>
struct AbilityImpl<ABILITY_ANGER_SHELL> : extends OnDefender {
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_AngerShell);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_EGOIST> : extends OnReactive {
    ON_REACTIVE {
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
    }
};

template <>
struct AbilityImpl<ABILITY_READIED_ACTION> : extends OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
    }
};

template <>
struct AbilityImpl<ABILITY_DARK_GALE_WINGS> : extends GaleWingsLike<TYPE_DARK> {};

template <>
struct AbilityImpl<ABILITY_GUILT_TRIP> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

            BattleScriptCall(BattleScript_GuiltTrip);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WATER_GALE_WINGS> : extends GaleWingsLike<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_ZERO_TO_HERO> : extends FormChangeAbility, extends OnEntry, extends OnExit {
    ON_ENTRY {
        CHECK(gBattleMons[battler].species == SPECIES_PALAFIN)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
        CHECK(GetSingleUseAbilityCounter(battler, ability))

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_PALAFIN_HERO);
        gBattleMons[battler].species = SPECIES_PALAFIN_HERO;
        BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
        return TRUE;
    }
    ON_EXIT {
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_COSTAR> : extends OnEntry {
    ON_ENTRY {
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
    }
};

template <>
struct AbilityImpl<ABILITY_COMMANDER> : extends FormChangeAbility, extends OnBattlerFaints<ApplyOnTarget::ALLY>, extends OnAccuracy<ApplyOnTarget::TARGET> {
    ON_BATTLER_FAINTS {
        CHECK(GetAbilityState(battler, ability))

        SetAbilityState(battler, ability, COMMANDER_NOT_ACTIVE);
        gStatuses3[battler] &= ~STATUS3_SEMI_INVULNERABLE;
        BattleScriptCall(BattleScript_CommanderEnds);
        return TRUE;
    }
    ON_ACCURACY {
        CHECK(GetAbilityState(target, ability))
        return ACCURACY_ALWAYS_MISSES;
    }
};

template <>
struct AbilityImpl<ABILITY_EJECT_PACK_ABILITY> : extends Persistent {};

template <>
struct AbilityImpl<ABILITY_VENGEFUL_SPIRIT> : extends AbilityImpl<ABILITY_HAUNTED_SPIRIT>, extends AbilityImpl<ABILITY_VENGEANCE> {};

template <>
struct AbilityImpl<ABILITY_CUD_CHEW> : extends OnEndTurn {
    ON_END_TURN {
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
    }
};

template <>
struct AbilityImpl<ABILITY_ARMOR_TAIL> : extends AbilityImpl<ABILITY_QUEENLY_MAJESTY> {};

template <>
struct AbilityImpl<ABILITY_MIND_CRUSH> : extends AbilityImpl<ABILITY_STRONG_JAW>, extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
    }
};

template <>
struct AbilityImpl<ABILITY_SUPREME_OVERLORD> : extends OnEntry, extends OnStat<> {
    ON_ENTRY {
        CHECK(gFaintedMonCount[GetBattlerSide(battler)])

        return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
    }
    ON_STAT {
        if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
    }
};

template <>
struct AbilityImpl<ABILITY_ILL_WILL> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker)) CHECK(gBattleMons[attacker].pp[gChosenMovePos]) CHECK_NOT(IsBattlerAlive(battler))

            gBattleMons[attacker]
                .pp[gChosenMovePos] = 0;
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove)
        gActiveBattler = attacker;
        BtlController_EmitSetMonData(0, gChosenMovePos + REQUEST_PPMOVE1_BATTLE, 0, 1, &gBattleMons[attacker].pp[gChosenMovePos]);
        BattleScriptCall(BattleScript_IllWillTakesPp);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FIRE_SCALES> : extends AbilityImpl<ABILITY_ICE_SCALES> {};

template <>
struct AbilityImpl<ABILITY_WATCH_YOUR_STEP> : extends OnEntry {
    ON_ENTRY {
        u8 targetSide = GetOppositeSide(battler);
        CHECK(gSideTimers[targetSide].spikesAmount < 3)

        gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RAPID_RESPONSE> : extends OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
    }
};

template <>
struct AbilityImpl<ABILITY_DOUBLE_IRON_BARBS> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasMagicGuard(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

            gBattleMoveDamage = gBattleMons[attacker].maxHP / 6;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_THERMAL_EXCHANGE> : extends OnDefender, extends RemovesStatusOnImmunity {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanRaiseStat(battler, STAT_ATK))

            SetStatChanger(STAT_ATK, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_BURN)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_GOOD_AS_GOLD> : extends OnImmune<> {
    ON_IMMUNE {
        CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHARING_IS_CARING> : extends OnReactive {
    ON_REACTIVE {
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
    }
};

template <>
struct AbilityImpl<ABILITY_PERMAFROST_CLONE> : extends AbilityImpl<ABILITY_PERMAFROST> {};

template <>
struct AbilityImpl<ABILITY_GALLANTRY> : extends NoDamageHits<1> {};

template <>
struct AbilityImpl<ABILITY_ORICHALCUM_PULSE> : extends AbilityImpl<ABILITY_DROUGHT>, extends OnStat<> {
    ON_STAT {
        if (statId != STAT_ATK) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
    }
};

template <>
struct AbilityImpl<ABILITY_SUN_BASKING> : extends OnImmune<>, extends OnDefensiveMultiplier {
    ON_IMMUNE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
        return blocksPriority(DELEGATE_IMMUNE);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_WINGED_KING> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
    }
};

template <>
struct AbilityImpl<ABILITY_HADRON_ENGINE> : extends AbilityImpl<ABILITY_ELECTRIC_SURGE>, extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_SERPENT> : extends AbilityImpl<ABILITY_WINGED_KING> {};

template <>
struct AbilityImpl<ABILITY_SWEEPING_EDGE_PLUS> : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends AbilityImpl<ABILITY_SWEEPING_EDGE> {};

template <>
struct AbilityImpl<ABILITY_CELESTIAL_BLESSING> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2) CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_MISTY_TERRAIN))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 12;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MINION_CONTROL> : extends OnParentalBond {
    ON_PARENTAL_BOND { return PARENTAL_BOND_MINION_CONTROL; }
};

template <>
struct AbilityImpl<ABILITY_MOLTEN_BLADES> : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_BURN);
    }
};

template <>
struct AbilityImpl<ABILITY_HAUNTING_FRENZY> : extends AbilityImpl<ABILITY_ADRENALINE_RUSH>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 20)

            return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    }
};

template <>
struct AbilityImpl<ABILITY_NOISE_CANCEL> : extends AbilityImpl<ABILITY_SOUNDPROOF> {
    ApplyOn onImmuneFor() const override { return ApplyOn::ALLY; }
};

template <>
struct AbilityImpl<ABILITY_RADIO_JAM> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeDisabled(target))
        CHECK(IsSoundMove(battler, move)) CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
    }
};

template <>
struct AbilityImpl<ABILITY_OLE> : extends OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        switch (GetBattlerBattleMoveTargetFlags(move, battler)) {
            case MOVE_TARGET_SELECTED:
            case MOVE_TARGET_USER_OR_SELECTED:
            case MOVE_TARGET_RANDOM:
                *accuracy *= .8;
                return ACCURACY_MULTIPLICATIVE;

            default:
                return ACCURACY_NO_RESULT;
        }
    }
};

template <>
struct AbilityImpl<ABILITY_MALICIOUS> : extends AbilityImpl<ABILITY_INTIMIDATE> {};

template <>
struct AbilityImpl<ABILITY_DEAD_POWER> : extends OnAttacker, extends OnStat<> {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gBattleMons[target].status2 & STATUS2_CURSED)
        CHECK(IsMoveMakingContact(move, battler)) CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_CURSE);
    }
    ON_STAT {
        if (statId == STAT_ATK) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_BRAWLING_WYVERN> : extends AbilityImpl<ABILITY_NO_GUARD>, extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_PUNCH) CHECK(IS_MOVE_TYPE(move, TYPE_DRAGON)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_JUNSHI_SANDA> : extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS {
        switch (flag) {
            case MOVE_FLAG_PUNCH:
                return gBattleMoves[move].flags & FLAG_STRIKER_BOOST;
            case MOVE_FLAG_KICK:
                return gBattleMoves[move].flags & FLAG_IRON_FIST_BOOST;
            default:
                return FALSE;
        }
    }
};

template <>
struct AbilityImpl<ABILITY_MYTHICAL_ARROWS> : extends AbilityImpl<ABILITY_ARCHER>, extends OnSwapSplit {
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL) CHECK(gBattleMoves[move].arrowBased);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LAWNMOWER> : extends OnEntry {
    ON_ENTRY {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLOURISH> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DESERT_SPIRIT> : extends AbilityImpl<ABILITY_SAND_STREAM>, extends OnAfterTypeEffectiveness<> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
            *mod = UQ_4_12(1.0);
        }
    }
};

template <>
struct AbilityImpl<ABILITY_CONTEMPT> : extends AbilityImpl<ABILITY_UNAWARE> {};

template <>
struct AbilityImpl<ABILITY_AERIALIST> : extends Merged<ABILITY_LEVITATE, ABILITY_FLOCK> {};

template <>
struct AbilityImpl<ABILITY_TERA_SHELL> : extends Breakable, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_TOXIC_CHAIN> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

template <>
struct AbilityImpl<ABILITY_PARASITIC_SPORES> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

        gVolatileStructs[battler].parasiticSpores = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
    }
};

template <MoveEffectEnum Effect>
struct PoisonPuppeteerLike : extends OnBattlerFaints<ApplyOnTarget::ANY>, extends SetStateOnEffect<Effect>, extends OnReactive {
   public:
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

    ON_BATTLER_FAINTS {
        int state = GetAbilityState(battler, ability);
        if (state & (1 << fainted)) SetAbilityState(battler, ability, state ^ (1 << fainted));
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_PUPPETEER> : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
    }
};

template <>
struct AbilityImpl<ABILITY_ENTRANCE> : extends PoisonPuppeteerLike<MOVE_EFFECT_CONFUSION> {
    ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); }
};

template <>
struct AbilityImpl<ABILITY_REJECTION> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gFieldTimers.quashTimer)

        gFieldTimers.quashTimer = QUASH_DURATION;
        gFieldTimers.started.quash = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
    }
};

template <>
struct AbilityImpl<ABILITY_APPLE_ENLIGHTENMENT> : extends AbilityImpl<ABILITY_FUR_COAT>, extends AbilityImpl<ABILITY_MAGIC_GUARD> {};

template <>
struct AbilityImpl<ABILITY_BALLOON_BOMBER> : extends Merged<ABILITY_AFTERMATH, ABILITY_INFLATABLE> {};

template <>
struct AbilityImpl<ABILITY_FLAMING_MAW> : extends AbilityImpl<ABILITY_FLAMING_JAWS>, extends AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_DEMOLITIONIST> : extends AbilityImpl<ABILITY_READIED_ACTION>, extends OnInfiltrate, extends OnAttacker {
    ON_INFILTRATE {
        if (gVolatileStructs[battler].readiedAction && !IS_MOVE_STATUS(move)) return INFILTRATE_BREAK_SCREENS;
        return INFILTRATE_NONE;
    }
    ON_ATTACKER {
        CHECK(DidMoveHit())
        CHECK(gVolatileStructs[battler].readiedAction)
        int opposingSide = GetBattlerSide(target);
        CHECK(gSideTimers[opposingSide].reflectTimer || gSideTimers[opposingSide].lightscreenTimer || gSideTimers[opposingSide].auroraVeilTimer)
        BattleScriptCall(BattleScript_AttackerShattersScreens);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ROCKHARD_WILL> : extends SwarmLike<TYPE_ROCK> {};
template <>
struct AbilityImpl<ABILITY_FRAGRANT_DAZE> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeConfused(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker)) CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_CONFUSION, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LOW_VISIBILITY> : extends OnEntry {
    ON_ENTRY {
        if (TryChangeBattleWeather(battler, ENUM_WEATHER_FOG, TRUE)) {
            BattleScriptPushCursorAndCallback(BattleScript_BadOmensActivates);
            return TRUE;
        } else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT) {
            BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
            return NO_ANNOUNCE;
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_OLD_MARINER> : extends AbilityImpl<ABILITY_SEAWEED>, extends AbilityImpl<ABILITY_AMPHIBIOUS> {};

template <>
struct AbilityImpl<ABILITY_ECTOPLASM> : extends OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_BEAUTIFUL_MUSIC> : extends OnAttacker, extends InfatuatesAny {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 2)
        CHECK(IsSoundMove(battler, move))

            return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    }
};

template <>
struct AbilityImpl<ABILITY_SNOW_SONG> : LiquidVoiceClone<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_GREATER_SPIRIT> : extends OnEntry {
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RESONANCE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move)) CHECK(Random() % 100 < 50)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_ETHEREAL_RUSH> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CUTE_ANTECEDENCE> : extends GaleWingsLike<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_RECURRING_NIGHTMARE> : extends OnRevive {
    ON_REVIVE { CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) return B_MSG_FADE_OUT; }
};

template <>
struct AbilityImpl<ABILITY_MENACING_SITUATION> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK_NOT(gVolatileStructs[opponent].fear) CHECK(Random() % 100 < 30)

            gStackBattler1 = battler;
        gStackBattler2 = opponent;
        BattleScriptCall(BattleScript_AbilitySetFear);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHINY_LIGHTNING> : extends OnAccuracy<> {
    ON_ACCURACY {
        if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_TERRIFY> : extends AbilityImpl<ABILITY_INTIMIDATE> {};

template <>
struct AbilityImpl<ABILITY_ICE_DOWNFALL> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_LAST_STAND> : extends Breakable, extends OnStat<> {
    ON_STAT {
        if (statId == STAT_DEF || statId == STAT_SPDEF)
            *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
    }
};

template <>
struct AbilityImpl<ABILITY_PYROCLASTIC_FLOW> : Merged<ABILITY_MOLTEN_DOWN, ABILITY_CORROSION> {};

template <>
struct AbilityImpl<ABILITY_BLOOD_BATH> : extends PoisonPuppeteerLike<MOVE_EFFECT_BLEED>, extends RemovesStatusOnImmunity {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_BLEED)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BATTLE_AURA> : extends OnCrit<ApplyOnTarget::ANY> {
    ON_CRIT { return 2; }
};

template <>
struct AbilityImpl<ABILITY_BLOODLUST> : extends AbilityImpl<ABILITY_BLOOD_BATH>, extends AbilityImpl<ABILITY_SOUL_EATER> {
    ON_BATTLER_FAINTS {
        int result = 0;
        if (battler == attacker) {
            result |= AbilityImpl<ABILITY_SOUL_EATER>::onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        return result | AbilityImpl<ABILITY_BLOOD_BATH>::onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    }
    ApplyOnTarget onBattlerFaintsFor() const override { return ApplyOnTarget::ANY; }
};

template <>
struct AbilityImpl<ABILITY_PIERCING_SOLO> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_RHYTHMIC> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); }
};

template <>
struct AbilityImpl<ABILITY_CHUNKY_BASS_LINE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(IsSoundMove(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
    }
};

template <>
struct AbilityImpl<ABILITY_DUAL_HAMMER> : extends OnParentalBond {
    ON_PARENTAL_BOND { CHECK(gBattleMoves[move].hammerBased) return PARENTAL_BOND_DUAL_WIELD; }
};

template <>
struct AbilityImpl<ABILITY_DENTING_BLOWS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(gBattleMoves[move].hammerBased)
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

            int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    }
};

template <>
struct AbilityImpl<ABILITY_ICE_COLD_HUNTER> : extends HailImmune, extends OnParentalBond {
    ON_PARENTAL_BOND { CHECK(moveType == TYPE_ICE) CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) return PARENTAL_BOND_ICE_COLD_HUNTER; }
};

template <>
struct AbilityImpl<ABILITY_SOUL_CRUSHER> : extends OnOffensiveMultiplier<>, extends OnChooseDefensiveStat<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hammerBased) MUL(1.1);
    }
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(gBattleMoves[move].hammerBased)
        return STAT_SPDEF;
    }
};

template <>
struct AbilityImpl<ABILITY_ARC_FLASH> : extends OnAttacker, extends OnDefender {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(CanBeBurned(attacker))
        CHECK(Random() % 2)

        AbilityStatusEffect(MOVE_EFFECT_BURN | MOVE_EFFECT_AFFECTS_USER);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_UNICORN> : extends AbilityImpl<ABILITY_MIGHTY_HORN>, extends AbilityImpl<ABILITY_PIXILATE> {};

template <>
struct AbilityImpl<ABILITY_ON_THE_PROWL> : extends OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
    }
};

template <>
struct AbilityImpl<ABILITY_PRETENTIOUS> : extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(gVolatileStructs[battler].critBoost < 3);
        gVolatileStructs[battler].critBoost++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
        BattleScriptCall(BattleScript_AbilityBoostsCrit);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VENOBLAZE_PINCERS> : extends OnAttacker, extends OnOffensiveMultiplier<> {
    ON_ATTACKER {
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
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_ETERNAL_BLESSING> : extends AbilityImpl<ABILITY_CELESTIAL_BLESSING>, extends AbilityImpl<ABILITY_REGENERATOR> {};

template <>
struct AbilityImpl<ABILITY_RIPEN> {};
template <>
struct AbilityImpl<ABILITY_SUGAR_RUSH> : extends AbilityImpl<ABILITY_UNBURDEN>, extends AbilityImpl<ABILITY_RIPEN> {};

template <>
struct AbilityImpl<ABILITY_PEACEFUL_REST> : extends OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2) CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WHITE_NOISE> : extends AbilityImpl<ABILITY_PEACEFUL_REST>, extends AbilityImpl<ABILITY_STATIC> {};

template <>
struct AbilityImpl<ABILITY_SMOKEY_MANEUVERS> : extends OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_METAL> : LiquidVoiceClone<TYPE_STEEL> {};

template <>
struct AbilityImpl<ABILITY_POWER_EDGE> : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends OnChooseDefensiveStat<> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) return STAT_SPDEF; }
};

template <>
struct AbilityImpl<ABILITY_SUPERCONDUCTOR> : extends OnOffensiveMultiplier<>, extends OnMoveType {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
    }
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_STEEL)
        *ateBoost = TRUE;
        return TYPE_ELECTRIC + 1;
    }
};

template <>
struct AbilityImpl<ABILITY_ULTRA_INSTINCT> : extends OnDefender, extends OnDefensiveMultiplier, extends OverrideBreakable {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
        return FALSE;
    }
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
};

template <>
struct AbilityImpl<ABILITY_UNLOCKED_POTENTIAL> : extends AbilityImpl<ABILITY_BERSERK>, extends AbilityImpl<ABILITY_INNER_FOCUS> {};

template <>
struct AbilityImpl<ABILITY_HIGHER_RANK> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_FUNERAL_PYRE> : extends OnEntry, extends OnEndTurn {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE); }
    ON_END_TURN {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GHOST) || IS_BATTLER_OF_TYPE(target, TYPE_DARK))
            FILTER_NOT(HasMagicGuard(target))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_FuneralPyreDamage);
            any = TRUE;
        }
        return any;
    }
};

template <>
struct AbilityImpl<ABILITY_FLAME_BUBBLE> : extends AbilityImpl<ABILITY_WATER_BUBBLE>, extends AbilityImpl<ABILITY_FLAMING_SOUL> {};

template <>
struct AbilityImpl<ABILITY_ELEMENTAL_VORTEX> : extends Merged<ABILITY_WATER_ABSORB, ABILITY_FLASH_FIRE> {};

template <>
struct AbilityImpl<ABILITY_SNOWY_WRATH> : extends AbilityImpl<ABILITY_SNOW_WARNING>, extends AbilityImpl<ABILITY_CRYOMANCY> {};

template <>
struct AbilityImpl<ABILITY_PATTERN_CHANGE> : extends AbilityImpl<ABILITY_SHED_SKIN>, extends AbilityImpl<ABILITY_PROTEAN> {};

template <>
struct AbilityImpl<ABILITY_NO_TURNING_BACK> : extends OnDefender {
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_NoTurningBack);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLAMMABLE_COAT> : extends FormChangeAbility, extends OnDefender, extends OnBeforeAttack<> {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler) || (gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE))
        CHECK(moveType == TYPE_FIRE)
        CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[battler].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_TargetFormChange);
        return TRUE;
    }
    ON_BEFORE_ATTACK {
        CHECK(moveType == TYPE_FIRE)
        CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
        CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

        UpdateAbilityStateIndicesForNewSpecies(gBattlerAttacker, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[gBattlerAttacker].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_AttackerFormChange);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DRACO_MORALE> : extends SimpleEntryMove<MOVE_DRAGON_CHEER> {};

template <>
struct AbilityImpl<ABILITY_BAD_OMEN> : extends OnDefensiveMultiplier, extends ForcesMinRolls {
    ON_DEFENSIVE_MULTIPLIER {
        if (isCrit) MUL(.25);
    }
};

template <>
struct AbilityImpl<ABILITY_MOSH_PIT> : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST)
            MUL(1.25);
        else
            MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_BLOOD_STAIN> : extends OnEither, extends OnEntry, extends Unsuppressable, extends RemovesStatusOnImmunity {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK_NOT(IsPersistentOrUnsuppressable(GetBattlerAbility(opponent))) CHECK_NOT(HasAbilityIgnoringSuppression(opponent, ability))
            CHECK_NOT(DoesBattlerHaveAbilityShield(opponent))

                UpdateAbilityStateIndicesForNewAbility(opponent, ability);
        ReplaceAbility(opponent, ability);
        gStackBattler1 = opponent;
        BattleScriptCall(BattleScript_BloodStainActivates);
        DisableSwitchInAbility(opponent, ability);
        return TRUE;
    }
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN); }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BLOOD_STIGMA> : extends OnOffensiveMultiplier<>, extends RemovesStatusOnImmunity, extends Unsuppressable {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SLIPSTREAM> : extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPEED] += 20; }
};

template <>
struct AbilityImpl<ABILITY_MAXIMUM_ACCELERATION> : extends AbilityImpl<ABILITY_SLIPSTREAM>, extends AbilityImpl<ABILITY_SPEED_BOOST> {};

template <>
struct AbilityImpl<ABILITY_SIDEWINDER> : extends AbilityImpl<ABILITY_COIL_UP>, extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
        gStatuses4[battler] |= STATUS4_COILED;
        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PETRIFY> : extends AbilityImpl<ABILITY_INTIMIDATE> {
    ON_ENTRY {
        int loweredStats = 0;
        int intimidated = AbilityImpl<ABILITY_INTIMIDATE>::onEntry(DELEGATE_ENTRY);
        for (int i = GetOppositeSide(battler); i < gBattlersCount; i += 2) {
            FILTER(IsBattlerAlive(i))
            loweredStats |= TryResetBattlerStatChanges(i, RESET_STAT_BUFFS);
        }

        if (loweredStats) {
            BattleScriptPushCursorAndCallback(BattleScript_Petrify);
        }
        return intimidated || loweredStats;
    }
};

template <>
struct AbilityImpl<ABILITY_FLUFFIEST> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(2.0);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_WAY_OF_PRECISION> : extends AbilityImpl<ABILITY_INNER_FOCUS>, extends AbilityImpl<ABILITY_PRECISE_FIST> {};

template <>
struct AbilityImpl<ABILITY_WAY_OF_SWIFTNESS> : extends AbilityImpl<ABILITY_PRETENTIOUS>, extends AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_ATOMIC_PUNCH> : extends AbilityImpl<ABILITY_IRON_FIST> {
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_IRON_FIST>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        if (moveType == TYPE_STEEL) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_GIANT> : extends AbilityImpl<ABILITY_HEATPROOF>, extends AbilityImpl<ABILITY_JUGGERNAUT> {};

template <>
struct AbilityImpl<ABILITY_MASTER_HAND> : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_MEGA_LAUNCHER> {};

template <>
struct AbilityImpl<ABILITY_UNSEEN_FIST> {};

template <>
struct AbilityImpl<ABILITY_FINAL_BLOW> : extends AbilityImpl<ABILITY_FATAL_PRECISION>, extends AbilityImpl<ABILITY_UNSEEN_FIST> {};

template <>
struct AbilityImpl<ABILITY_HOSPITALITY> : extends OnEntry {
    ON_ENTRY {
        gBattlerTarget = BATTLE_PARTNER(battler);
        CHECK(IsBattlerAlive(gBattlerTarget))
        CHECK_NOT(BATTLER_MAX_HP(gBattlerTarget))

        gBattleMoveDamage = -gBattleMons[gBattlerTarget].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptPushCursorAndCallback(BattleScript_Hospitality_AfterPopup);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BUTTER_UP> : extends Merged<ABILITY_HOSPITALITY, ABILITY_SOOTHING_AROMA> {};

template <>
struct AbilityImpl<ABILITY_VITALITY_STRIKE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler)) CHECK(IsIronFistBoosted(battler, move))

            gBattleMoveDamage = -gHpDealt / 10;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_HUGE_WINGS> : extends Merged<ABILITY_GIANT_WINGS, ABILITY_LEVITATE> {};

template <>
struct AbilityImpl<ABILITY_SWORD_OF_DAMNATION> : extends AbilityImpl<ABILITY_SWORD_OF_RUIN>, extends AbilityImpl<ABILITY_UNAWARE> {};

template <>
struct AbilityImpl<ABILITY_RESTRAINING_ORDER> : extends OnDefender {
    ON_DEFENDER {
        CHECK(GetAbilityState(battler, ability) == RESTRAINING_ORDER_NOT_TRIGGERED)
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER) CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
            CHECK(CountUsablePartyMons(battler))

                SetAbilityState(battler, ability, RESTRAINING_ORDER_ACTIVATING);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_ASSASSINS_TOOLS> : extends OnAttacker {
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_FROSTMAW> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanGetFrostbite(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
    }
};

template <>
struct AbilityImpl<ABILITY_PATCHWORK> : extends AbilityImpl<ABILITY_DISGUISE>, extends OnDefender {
    ON_DISGUISE {
        SpeciesEnum species = AbilityImpl<ABILITY_DISGUISE>::onDisguise(DELEGATE_DISGUISE);
        if (species && !testOnly) {
            SetOncePerTurnAbilityCounter(battler, ABILITY_PATCHWORK, gBattlerAttacker + 1);
        }
        return species;
    }
    ON_DEFENDER {
        int triggeringBattler = GetOncePerTurnAbilityCounter(battler, ability) - 1;
        CHECK(triggeringBattler == attacker)
        SetOncePerTurnAbilityCounter(battler, ability, 0);

        CHECK(IsBattlerAlive(attacker))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)

        AbilityStatusEffect(MOVE_EFFECT_CURSE | MOVE_EFFECT_AFFECTS_USER);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BLIND_RAGE> : extends AbilityImpl<ABILITY_MOLD_BREAKER>, extends AbilityImpl<ABILITY_SCRAPPY> {};

template <>
struct AbilityImpl<ABILITY_APEX_PREDATOR> : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_TOUGH_CLAWS> {};

template <>
struct AbilityImpl<ABILITY_DRAGONS_RITUAL> : extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
        BattleScriptCall(BattleScript_DragonsRitual);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PINNACLE_BLADE> : extends OnInfiltrate, extends OnAttacker {
    ON_INFILTRATE { return gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST ? INFILTRATE_BREAK_SCREENS | INFILTRATE_SUBSTITUTE : INFILTRATE_NONE; }
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_ENERGIZED> : extends AbilityImpl<ABILITY_GENERATOR>, extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(moveType == TYPE_ELECTRIC);
        SetOncePerTurnAbilityCounter(battler, ability, TRUE);
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_COLOR_SPECTRUM> : extends OnEndTurn, extends OnOffensiveMultiplier<> {
    ON_END_TURN {
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
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (StabMultiplierInHalves(battler, moveType, move) > 2) MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_STEEL_BEETLE> : extends AbilityImpl<ABILITY_RAGING_BOXER>, extends AbilityImpl<ABILITY_POLLINATE> {};

template <>
struct AbilityImpl<ABILITY_FROM_THE_SHADOWS> : extends OnAttacker {
    ON_ATTACKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_RAGE_POINT> : OnDefender, extends OnOffensiveMultiplier<>, extends NegateBurnAtkDrop, extends NegateFrzSpatkDrop {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gIsCriticalHit)
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK))

            BattleScriptCall(BattleScript_RagePointActivates);
        return TRUE;
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_HOT_COALS> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

        gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
    }
};

template <>
struct AbilityImpl<ABILITY_TERASTAL_TREASURE> : extends OnDefensiveMultiplier, extends OnStat<> {
    ON_DEFENSIVE_MULTIPLIER { MUL(.6); }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .8;
    }
};

template <>
struct AbilityImpl<ABILITY_SHOCKING_MAW> : extends AbilityImpl<ABILITY_SHOCKING_JAWS>, extends AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_GLEAM_EYES> : extends Merged<ABILITY_INTIMIDATE, ABILITY_FRISK> {};

template <>
struct AbilityImpl<ABILITY_ROUSED_FANGS> : extends AbilityImpl<ABILITY_STRONG_JAW>, extends AbilityImpl<ABILITY_MIND_CRUSH> {};

template <>
struct AbilityImpl<ABILITY_DREAM_STATE> : extends AbilityImpl<ABILITY_BATTLE_ARMOR> {};

template <>
struct AbilityImpl<ABILITY_DREAM_WHIMSY> : extends SimpleEntryMove<MOVE_YAWN> {};

template <>
struct AbilityImpl<ABILITY_LUNAR_AFFINITY> : extends OnCopyMove {
    ON_COPY_MOVE { CHECK(gBattleMoves[move].lunar) return UseOutOfTurnAttack(battler, target, ability, move, 0); }
};

template <>
struct AbilityImpl<ABILITY_FLAME_SHIELD> : extends AbilityImpl<ABILITY_FILTER> {};

template <>
struct AbilityImpl<ABILITY_AQUATIC_DWELLER> : extends AbilityImpl<ABILITY_AQUATIC>, extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_APPLE_PIE> : extends AbilityImpl<ABILITY_SELF_SUFFICIENT> {};

template <>
struct AbilityImpl<ABILITY_HOVER> : extends GroundImmune, AddsType<TYPE_PSYCHIC> {};

template <>
struct AbilityImpl<ABILITY_DEPRAVITY> : extends AbilityImpl<ABILITY_MERCILESS>, extends AbilityImpl<ABILITY_OVERCHARGE> {};

template <>
struct AbilityImpl<ABILITY_WILDFIRE> : extends SimpleEntryMove<MOVE_FIRE_SPIN> {};

template <>
struct AbilityImpl<ABILITY_JUMP_SCARE> : extends OnEntry, extends Persistent {
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
    }
};

template <>
struct AbilityImpl<ABILITY_TAR_TOSS> : extends SimpleEntryMove<MOVE_TAR_SHOT> {};

template <>
struct AbilityImpl<ABILITY_STUN_SHOCK> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 100 < 60) switch (Random() % 2) {
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
    }
};

template <>
struct AbilityImpl<ABILITY_RAGING_GODDESS> : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {};

template <>
struct AbilityImpl<ABILITY_WHIPLASH> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

            int affected = GetOncePerTurnAbilityCounter(battler, ability);
        CHECK_NOT(affected & (1 << target))

        SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
        return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPERSWEET_SYRUP> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_EMBARGO) CHECK(gBattleMons[attacker].item)

            gVolatileStructs[attacker]
                .embargoTimer = 2;
        gStatuses3[attacker] |= STATUS3_EMBARGO;
        gLastUsedItem = gBattleMons[attacker].item;
        BattleScriptCall(BattleScript_AnnounceAttackerItemDisabled);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TRASH_HEAP> : extends AbilityImpl<ABILITY_TOXIC_SPILL>, extends AbilityImpl<ABILITY_CORROSION> {};

template <>
struct AbilityImpl<ABILITY_SLUDGY_MIX> : extends AbilityImpl<ABILITY_INTOXICATE>, extends AbilityImpl<ABILITY_PUNK_ROCK> {};

template <>
struct AbilityImpl<ABILITY_OVERWATCH> : extends AbilityImpl<ABILITY_ON_THE_PROWL>, extends AbilityImpl<ABILITY_STAKEOUT> {};

template <>
struct AbilityImpl<ABILITY_WIND_RAGE> : extends AbilityImpl<ABILITY_GIANT_WINGS>, extends SimpleEntryMove<MOVE_DEFOG> {};

template <>
struct AbilityImpl<ABILITY_VICTORY_BOMB> : extends OnDefender, extends OnMoveType {
    ON_DEFENDER {
        CHECK_NOT(IsBattlerAlive(battler))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_EXPLOSION, 100);
        return FALSE;
    }
    ON_MOVE_TYPE {
        CHECK(gProcessingExtraAttacks)
        CHECK(gQueuedExtraAttackData[0].ability == ability)
        return TYPE_FIRE + 1;
    }
};

template <>
struct AbilityImpl<ABILITY_RAZOR_SHARP> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gIsCriticalHit)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_TO_THE_BONE> : extends AbilityImpl<ABILITY_RAZOR_SHARP>, extends AbilityImpl<ABILITY_SNIPER> {};

template <>
struct AbilityImpl<ABILITY_BLADE_DANCE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_APE_SHIFT> : extends FormChangeAbility, extends OnEntry, extends OnEndTurn, extends OnDefender, extends OnCrit<> {
    static int ApeShiftHandler(int battler, AbilityCallType callType) {
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

    ON_ENTRY { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_END_TURN { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_DEFENDER { return ApeShiftHandler(battler, ABILITY_BS_CALL); }
    ON_CRIT {
        CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
        return ALWAYS_CRIT;
    }
};

template <>
struct AbilityImpl<ABILITY_KNOW_YOUR_PLACE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gVolatileStructs[target].dazed)
        CHECK(IsMoveMakingContact(move, battler))

            gVolatileStructs[target]
                .dazed = 5;
        BattleScriptCall(BattleScript_TargetDazed);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DEEP_CUTS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_LIFE_STEAL> : extends OnEndTurn {
    ON_END_TURN {
        int any = FALSE;
        for (int target = GetOppositeSide(battler); target < gBattlersCount; target += 2) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(HasMagicGuard(target))

            gStackBattler1 = battler;
            gStackBattler2 = target;
            gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_IGNORE_DISGUISE;
            BattleScriptExecute(BattleScript_AbilityDrainsHp);
            any = TRUE;
        }
        return any;
    }
};

template <>
struct AbilityImpl<ABILITY_RUDE_AWAKENING> : extends RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_SLEEP) CHECK(GetAbilityState(battler, ability)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_TERAFORM_ZERO> : extends AbilityImpl<ABILITY_TERA_SHELL>, extends OnEntry {
    ON_ENTRY {
        CHECK(!GetSingleUseAbilityCounter(battler, ability));
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
        BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SET_ABLAZE> : extends PoisonPuppeteerLike<MOVE_EFFECT_BURN> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    }
};

template <>
struct AbilityImpl<ABILITY_BREAKWATER> : extends AbilityImpl<ABILITY_STALL>, extends AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_MAGICAL_FISTS> : extends AbilityImpl<ABILITY_IRON_FIST>, extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
    }
};

template <>
struct AbilityImpl<ABILITY_CUTTHROAT> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_BENDER> : extends AbilityImpl<ABILITY_SAND_STREAM>, extends AbilityImpl<ABILITY_SAND_FORCE> {};

template <>
struct AbilityImpl<ABILITY_SAND_PIT> : extends SimpleEntryMove<MOVE_SAND_TOMB, 20> {};

template <>
struct AbilityImpl<ABILITY_DESOLATE_SUN> : extends RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_DAYBREAK> : extends OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ENERGY_SIPHON> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))

            gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RESERVOIR> : extends Merged<ABILITY_WATER_ABSORB, ABILITY_STORM_DRAIN> {};

static int NeurotoxinCondition(int battler, int target) {
    return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
}
template <>
struct AbilityImpl<ABILITY_NEUROTOXIN> : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); }
};

template <>
struct AbilityImpl<ABILITY_ENERGIZED_HORNS> : extends AbilityImpl<ABILITY_MIGHTY_HORN> {
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL) CHECK(gBattleMoves[move].hornBased);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SPIDER_LAIR_UPGRADE> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

        int side = GetOppositeSide(battler);
        gSideTimers[side].started.spiderWeb = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[side].stickyWebTimer = 7;
        BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRUST_COAT> : extends AbilityImpl<ABILITY_BATTLE_ARMOR> {};

template <>
struct AbilityImpl<ABILITY_PUFFY> : extends AbilityImpl<ABILITY_FLUFFY> {};

template <>
struct AbilityImpl<ABILITY_BALLOON_BLITZ> : extends AbilityImpl<ABILITY_INFLATABLE>, extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {};

template <>
struct AbilityImpl<ABILITY_STRIKER_PIXILATE> : extends AbilityImpl<ABILITY_STRIKER>, extends AbilityImpl<ABILITY_PIXILATE> {};

// 2.6
template <>
struct AbilityImpl<ABILITY_DOOM_BLAST> : extends OnRecoil, extends OnOffensiveMultiplier<> {
    ON_RECOIL {
        CHECK(moveType == TYPE_DARK);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK) MUL(1.35);
    }
};

template <>
struct AbilityImpl<ABILITY_BRUTEFORCE> : extends AbilityImpl<ABILITY_RECKLESS>, extends AbilityImpl<ABILITY_ROCK_HEAD> {};

template <>
struct AbilityImpl<ABILITY_FARADAY_CAGE> : extends AbilityImpl<ABILITY_SHELL_ARMOR>, extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_ACIDIC_SLIME> : extends AbilityImpl<ABILITY_CORROSION>, extends OnStab {
    ON_STAB { return moveType == TYPE_WATER; }
};

template <>
struct AbilityImpl<ABILITY_ROSE_GARDEN> : extends OnEntry {
    ON_ENTRY {
        u8 targetSide = GetOppositeSide(battler);
        CHECK(gSideTimers[targetSide].toxicSpikesAmount < 2)

        gSideTimers[targetSide].toxicSpikesAmount = 2;
        gSideStatuses[targetSide] |= SIDE_STATUS_TOXIC_SPIKES;
        gBattlerTarget = targetSide;
        BattleScriptPushCursorAndCallback(BattleScript_RoseGarden);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_QIGONG> : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_FIGHT_SPIRIT>, extends OnAccuracy<> {
    ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
};

template <>
struct AbilityImpl<ABILITY_CONJOURER_OF_DECEIT> : extends AbilityImpl<ABILITY_MAGIC_GUARD>, extends AbilityImpl<ABILITY_MAGIC_BOUNCE> {};

template <>
struct AbilityImpl<ABILITY_DEEP_FREEZE> : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_ICE) MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SOUL_DEVOURER> : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_PHANTOM_PAIN> {};

template <>
struct AbilityImpl<ABILITY_CHAMPIONS_ENTRANCE> : extends Merged<ABILITY_INTIMIDATE, ABILITY_VIOLENT_RUSH> {};

template <>
struct AbilityImpl<ABILITY_PRESTO> : extends OnPriority {
    ON_PRIORITY { CHECK(BATTLER_MAX_HP(battler)) CHECK(IsSoundMove(battler, move)) return 1; }
};

template <>
struct AbilityImpl<ABILITY_SAMBA> : extends AbilityImpl<ABILITY_STRIKER>, extends AbilityImpl<ABILITY_DANCER> {};

template <>
struct AbilityImpl<ABILITY_GLADIATOR> : extends BoostedSwarmLike<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_FORSAKEN_HEART> : extends OnBattlerFaints<ApplyOnTarget::ANY> {
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RELENTLESS> : extends AbilityImpl<ABILITY_EXPLOIT_WEAKNESS>, extends AbilityImpl<ABILITY_MERCILESS> {};

template <>
struct AbilityImpl<ABILITY_SOOTHSAYER> : extends OnEntry, extends OnEndTurn, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends Persistent {
    ON_ENTRY {
        CHECK(!GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        SetAbilityState(battler, ability, 3);
        return SwitchInAnnounce(B_MSG_SWITCHIN_SOOTHSAYER);
    }
    ON_END_TURN {
        int counter = GetAbilityState(battler, ability);
        if (counter) SetAbilityState(battler, ability, counter - 1);
        return FALSE;
    }
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (!GetAbilityState(battler, ability)) return;
        if (*mod >= UQ_4_12(1.0)) *mod = UQ_4_12(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_CORRUPTED_MIND> : extends RandomizerBanned, extends OnTypeEffectiveness<>, extends OnModifyEffectChance<> {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_PSYCHIC) if (*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        return FALSE;
    }
    ON_MODIFY_EFFECT_CHANCE {
        int type;
        GET_MOVE_TYPE(move, type)
        if (type == TYPE_PSYCHIC) *effectChance *= 1.4;
    }
};

template <>
struct AbilityImpl<ABILITY_FLAME_COAT> : extends OnEntry, extends OnEndTurn {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FIRE_COAT); }
    ON_END_TURN {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(IS_BATTLER_OF_TYPE(target, TYPE_FIRE))
            FILTER_NOT(HasMagicGuard(target))
            FILTER_NOT(BATTLER_HAS_ABILITY(target, ABILITY_FLARE_BOOST))

            gStackBattler1 = target;
            BattleScriptExecute(BattleScript_FireCoatDamage);
            any = TRUE;
        }
        return any;
    }
};

template <>
struct AbilityImpl<ABILITY_UNOWN_POWER> : extends RandomizerBanned, extends OnStab, extends OnAfterTypeEffectiveness<> {
    ON_STAB { return TRUE; }
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPER_SCOPE> : extends AbilityImpl<ABILITY_MEGA_LAUNCHER>, extends AbilityImpl<ABILITY_ARTILLERY> {};

template <>
struct AbilityImpl<ABILITY_VENOM_CROWN> : extends AbilityImpl<ABILITY_POISON_POINT>, extends AbilityImpl<ABILITY_MIGHTY_HORN>, extends RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_BLIGHT_SCALE> : extends AbilityImpl<ABILITY_POISON_POINT>, extends AbilityImpl<ABILITY_MULTISCALE>, extends RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_GUNMAN> : extends AbilityImpl<ABILITY_MEGA_LAUNCHER>, extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_MEGA_LAUNCHER) CHECK(IS_MOVE_STATUS(move)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_CARETAKER> : extends OnEndTurn, extends AbilityImpl<ABILITY_FRIEND_GUARD> {
    ON_END_TURN {
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
    }
};

template <>
struct AbilityImpl<ABILITY_POSEIDONS_DOMINION> : extends SimpleEntryMove<MOVE_WHIRLPOOL> {};

template <>
struct AbilityImpl<ABILITY_DUAL_SHADOW> : extends AbilityImpl<ABILITY_HUNGER_SWITCH>, extends OnRecoil, extends OnOffensiveMultiplier<> {
    ON_RECOIL {
        CHECK(moveType == TYPE_ELECTRIC || moveType == TYPE_DARK);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 10, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC || moveType == TYPE_DARK) MUL(1.35);
    }
};

template <>
struct AbilityImpl<ABILITY_LULLABY> : extends OnAccuracy<> {
    ON_ACCURACY {
        CHECK(move == MOVE_SING);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRYO_ARCHITECT> : extends OnEndTurn, extends OnDefender {
    ON_END_TURN {
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
    }
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_GLACIAL_RAGE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_ICE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_IMMOVABLE_OBJECT> : extends AbilityImpl<ABILITY_MAGIC_GUARD>, extends AbilityImpl<ABILITY_STURDY> {};

template <>
struct AbilityImpl<ABILITY_FRENZIED_PHANTOM> : extends AbilityImpl<ABILITY_SHADOW_TAG>, extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {};

template <>
struct AbilityImpl<ABILITY_DNA_SCRAMBLE> : extends FormChangeAbility, extends OnBeforeAttack<> {
    ON_BEFORE_ATTACK {
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
    }
};

template <>
struct AbilityImpl<ABILITY_METALLIC_JAWS> : extends AbilityImpl<ABILITY_METALLIC>, extends AbilityImpl<ABILITY_PRIMAL_MAW> {};

template <>
struct AbilityImpl<ABILITY_CALCULATIVE> : extends Merged<ABILITY_ANALYTIC, ABILITY_NEUROFORCE> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT> : extends RaiseStatOnEntry<STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT_HEARTHFLAME> : extends AbilityImpl<ABILITY_INTREPID_SWORD> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT_CORNERSTONE> : extends AbilityImpl<ABILITY_DAUNTLESS_SHIELD> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT_WELLSPRING> : extends RaiseStatOnEntry<STAT_SPDEF> {};

template <>
struct AbilityImpl<ABILITY_ROCKHARD_SHAFT> : extends BoostedSwarmLike<TYPE_ROCK> {};

template <>
struct AbilityImpl<ABILITY_HUNTERS_MARK> : extends AbilityImpl<ABILITY_DEADEYE>, extends AbilityImpl<ABILITY_AMBUSH> {};

template <>
struct AbilityImpl<ABILITY_DEVIATE> : extends AteAbility<TYPE_DARK> {};

template <>
struct AbilityImpl<ABILITY_SUNS_BOUNTY> : extends Merged<ABILITY_HARVEST, ABILITY_LEAF_GUARD> {};

template <>
struct AbilityImpl<ABILITY_RITE_OF_SPRING> : extends Merged<ABILITY_SOLAR_POWER, ABILITY_CHLOROPHYLL> {};

template <>
struct AbilityImpl<ABILITY_HEADSTRONG> : extends RaiseStatOnEntry<STAT_SPDEF> {};

template <>
struct AbilityImpl<ABILITY_FIREFIGHTER> : extends TypeSlayer<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_SEPIA_LENS> : extends AbilityImpl<ABILITY_SAND_GUARD>, extends AbilityImpl<ABILITY_TINTED_LENS> {};

template <>
struct AbilityImpl<ABILITY_SUPER_SNIPER> : extends AbilityImpl<ABILITY_SNIPER>, extends UseTurnAttackAsPursuit {
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_SNIPER>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
            MUL(0.5);
        }
    }
};

template <>
struct AbilityImpl<ABILITY_WOODLAND_CURSE> : extends OnEither, extends SimpleEntryMove<MOVE_FORESTS_CURSE> {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(opponent, TYPE_GRASS))

            gBattleMons[opponent]
                .type3 = TYPE_GRASS;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[opponent].type3);
        gStackBattler1 = opponent;
        BattleScriptCall(BattleScript_StackBecameTheTypeFull);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MALODOR> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_GASTRO_ACID)

            gStatuses3[attacker] |= STATUS3_GASTRO_ACID;
        BattleScriptCall(BattleScript_StackAbilitySuppressedMessage);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BLUR> : extends OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK(IsMoveMakingContact(move, gBattlerAttacker)) return STAT_SPEED; }
};

template <>
struct AbilityImpl<ABILITY_ELUDE> : extends OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker)) return STAT_SPEED; }
};

template <>
struct AbilityImpl<ABILITY_DRAKE_OF_RAGE> : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_TINTED_LENS> {};

template <>
struct AbilityImpl<ABILITY_MIXED_MARTIAL_ARTS> : extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_PUNCH || flag == MOVE_FLAG_KICK) CHECK(gBattleMoves[move].type == TYPE_NORMAL) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_STRATEGIC_PAUSE> : extends AbilityImpl<ABILITY_ANALYTIC>, extends OnCrit<> {
    ON_CRIT { CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber) CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) return 2; }
};

template <>
struct AbilityImpl<ABILITY_OVERRULE> : extends OnAfterTypeEffectiveness<> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
    }
};

template <>
struct AbilityImpl<ABILITY_MENTAL_POLLUTION> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_MADNESS_ENHANCEMENT> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_TENTALOCK> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SERPENT_BIND> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SOUL_TAP> : extends OnEndTurn {
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) int any = FALSE;
        for (int target = GetOppositeSide(battler); target < gBattlersCount; target += 2) {
            FILTER(IsBattlerAlive(target))
            FILTER_NOT(HasMagicGuard(target))

            gStackBattler1 = battler;
            gStackBattler2 = target;
            gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_IGNORE_DISGUISE;
            BattleScriptExecute(BattleScript_AbilityDrainsHp);
            any = TRUE;
        }
        return any;
    }
};

template <>
struct AbilityImpl<ABILITY_SCARECROW> : extends AbilityImpl<ABILITY_INTIMIDATE>, extends AbilityImpl<ABILITY_BAD_LUCK> {};

template <>
struct AbilityImpl<ABILITY_OMINOUS_SHROUD> : extends AbilityImpl<ABILITY_PHANTOM>, extends AbilityImpl<ABILITY_SHADOW_SHIELD> {};

template <>
struct AbilityImpl<ABILITY_CHILLING_PRESENCE> : extends SimpleEntryMove<MOVE_ICY_WIND, 10> {};

template <>
struct AbilityImpl<ABILITY_FROSTBIND> : extends PoisonPuppeteerLike<MOVE_EFFECT_FROSTBITE> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) { return (int)CanGetFrostbite(battler); }, BattleScript_Frostbind);
    }
};

template <>
struct AbilityImpl<ABILITY_TENDER_AFFECTION> : extends AbilityImpl<ABILITY_CUTE_CHARM>, extends OnStab {
    ON_STAB { return moveType == TYPE_FAIRY; }
};

template <>
struct AbilityImpl<ABILITY_GLACIAL_GHOST> : extends AbilityImpl<ABILITY_SLUSH_RUSH>, extends AbilityImpl<ABILITY_SNOW_CLOAK> {};

template <>
struct AbilityImpl<ABILITY_WONDER_SCALE> : extends AbilityImpl<ABILITY_SHED_SKIN>, extends AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_OVERZEALOUS> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_STAINLESS_STEEL> : extends AteAbility<TYPE_STEEL>, extends AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_TEMPORAL_RUPTURE> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_GRASS_FLUTE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IsSoundMove(battler, move))
        CHECK_NOT(gVolatileStructs[target].fear)

            return AbilityStatusEffect(MOVE_EFFECT_FEAR);
    }
};

template <>
struct AbilityImpl<ABILITY_HEMOTOXIN> : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(
            ability,
            battler,
            [](int battler, int target) -> int { return !(gStatuses3[target] & STATUS3_GASTRO_ACID); },
            BattleScript_StackAbilitySuppressedMessage);
    }
};

template <>
struct AbilityImpl<ABILITY_HARUKAZE> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_TOXIC_SURGE> : extends OnEntry, extends AllowTerrainIfAirborne<TERRAIN_TOXIC> {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_QUILLS> : extends Merged<ABILITY_POISON_POINT, ABILITY_ROUGH_SKIN> {};

template <>
struct AbilityImpl<ABILITY_DRACONIC_MIGHT> : extends AbilityImpl<ABILITY_HALF_DRAKE>, extends AteAbility<TYPE_DRAGON> {};

template <>
struct AbilityImpl<ABILITY_ATLANTIC_RULER> : extends AbilityImpl<ABILITY_AQUATIC_DWELLER>, extends AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_BIOFILM> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CHOKEHOLD> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_GUARDIAN_COAT> : extends SandImmune, extends OnDefensiveMultiplier, extends PowderImmune, extends HailImmune {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.8);
    }
};

template <>
struct AbilityImpl<ABILITY_NEUTRALIZING_FOG> : extends SimpleEntryMove<MOVE_DEFOG> {};

template <>
struct AbilityImpl<ABILITY_FESTIVITIES> : extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS {
        switch (flag) {
            case MOVE_FLAG_DANCE:
                return gBattleMoves[move].flags & FLAG_SOUND;
            case MOVE_FLAG_SOUND:
                return gBattleMoves[move].flags & FLAG_DANCE;
            default:
                return FALSE;
        }
    }
};

template <>
struct AbilityImpl<ABILITY_FEY_FLIGHT> : extends AbilityImpl<ABILITY_FAIRY_TALE>, extends GroundImmune {};

template <>
struct AbilityImpl<ABILITY_BEST_OFFENSE> : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends AbilityImpl<ABILITY_MYSTIC_BLADES>, extends OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPDEF] += 20; }
};

template <>
struct AbilityImpl<ABILITY_IMPALER> : extends AbilityImpl<ABILITY_MIGHTY_HORN>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target)) CHECK(CanBleed(target)) CHECK(gBattleMoves[move].hornBased);
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_MAGUS_BLADES> : extends AbilityImpl<ABILITY_DUAL_WIELD>, extends AbilityImpl<ABILITY_BEST_OFFENSE> {};

template <>
struct AbilityImpl<ABILITY_LIGHTNING_BORN> : extends AddsType<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_SUPERHEAVY> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_WORLD_SERPENT> : extends AbilityImpl<ABILITY_GRIP_PINCER>, extends AbilityImpl<ABILITY_LONG_REACH> {};

template <>
struct AbilityImpl<ABILITY_LUCKY_WINGS> : extends AbilityImpl<ABILITY_GIANT_WINGS>, extends AbilityImpl<ABILITY_SERENE_GRACE> {};

template <>
struct AbilityImpl<ABILITY_KOMODO> : extends AbilityImpl<ABILITY_HALF_DRAKE>, extends AbilityImpl<ABILITY_TOXIC_CHAIN> {};

template <>
struct AbilityImpl<ABILITY_ENVENOM> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_POISON);
    }
};

template <>
struct AbilityImpl<ABILITY_PURPLE_HAZE> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
    }
};

template <>
struct AbilityImpl<ABILITY_GNASHING_CANNON> : extends Merged<ABILITY_MEGA_LAUNCHER, ABILITY_MIND_CRUSH> {};

template <>
struct AbilityImpl<ABILITY_HYPER_CLEANSE> : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_POISON) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOLTEN_COAT> : extends OnAttacker, extends AteAbility<TYPE_ROCK> {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanBeBurned(target)) CHECK(Random() % 2)

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ROYAL_DECREE> : extends AbilityImpl<ABILITY_QUEENLY_MAJESTY>, extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_GLARE, 0);
    }
};

template <>
struct AbilityImpl<ABILITY_TAG> : extends OnPreemptAction {
    ON_PREEMPT_ACTION {
        CHECK(gCurrentActionFuncId == B_ACTION_SWITCH)
        gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
            .ability = ability,
            .move = MOVE_PURSUIT,
            .movePower = 20,
            .attacker = battler,
            .target = turnBattler,
        };

        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SURPRISE> : extends OnPreemptAction {
    ON_PREEMPT_ACTION {
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
    }
};

template <>
struct AbilityImpl<ABILITY_BREEZY_NEIGH> : extends AbilityImpl<ABILITY_ADRENALINE_RUSH> {};

template <>
struct AbilityImpl<ABILITY_DREAMSCAPE> : extends AbilityImpl<ABILITY_COMATOSE>, extends AbilityImpl<ABILITY_DREAMCATCHER> {
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_DREAMCATCHER>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_HASTE_MAKES_WASTE> : extends AbilityImpl<ABILITY_ANALYTIC>, extends AbilityImpl<ABILITY_STALL> {};

template <>
struct AbilityImpl<ABILITY_HUNGRY_MAWS> : extends AbilityImpl<ABILITY_JAWS_OF_CARNAGE>, extends AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_THERMAL_SLIDE> : extends OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_THERMOMANCY> : extends Merged<ABILITY_CRYOMANCY, ABILITY_PYROMANCY> {};

template <>
struct AbilityImpl<ABILITY_CHUCKSTER> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_HEAT_SINK> : extends LightningRodClone<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_RELIC_STONE> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SUPERCELL> : extends Merged<ABILITY_ELECTRIC_SURGE, ABILITY_DRIZZLE> {};

template <>
struct AbilityImpl<ABILITY_LIGHTNING_ASPECT> : extends AbsorbStatUp<TYPE_ELECTRIC, STAT_HIGHEST_ATTACKING> {};

template <>
struct AbilityImpl<ABILITY_FIRE_ASPECT> : extends AbsorbHeal<TYPE_FIRE>, extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanBeBurned(target))

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BLISTERING_SUN> : extends Merged<ABILITY_DESOLATE_LAND, ABILITY_AIR_BLOWER> {};

template <>
struct AbilityImpl<ABILITY_AURORAS_GALE> : extends AbilityImpl<ABILITY_NORTH_WIND>, extends AbilityImpl<ABILITY_MAJESTIC_BIRD> {};

template <>
struct AbilityImpl<ABILITY_WINTER_THRONE> : extends OnEntry, extends OnEndTurn {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_WINTER_THRONE); }
    ON_END_TURN {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))

            if (IS_BATTLER_OF_TYPE(target, TYPE_ICE)) {
                FILTER_NOT(HasMagicGuard(target))
                gStackBattler1 = target;
                BattleScriptExecute(BattleScript_FuneralPyreDamage);
            } else {
                FILTER_NOT(BATTLER_MAX_HP(target))
                FILTER(CanBattlerHeal(target))
                gStackBattler1 = target;
                BattleScriptExecute(BattleScript_HealStack1HpOver8End3);
            }

            any = TRUE;
        }
        return any;
    }
};

template <>
struct AbilityImpl<ABILITY_ICE_PLUMES> : extends AbilityImpl<ABILITY_ICE_SCALES> {};

template <>
struct AbilityImpl<ABILITY_PROPELLER_TAIL> : extends AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_ENERGY_TAP> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))

            gBattleMoveDamage = -gHpDealt / 8;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOLTEN_CORE>
    : extends AbilityImpl<ABILITY_FURNACE>, extends AbsorbStatUp<TYPE_ROCK, STAT_SPEED>, extends AbsorbUp2, extends StealthRockImmune {
    ON_ENTRY {
        AbilityImpl<ABILITY_FURNACE>::onEntry(DELEGATE_ENTRY);

        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[GetBattlerSide(battler)] &= ~SIDE_STATUS_STEALTH_ROCK;
        return SwitchInAnnounce(B_MSG_SWITCHIN_MOLTEN_CORE);
    }
};

template <>
struct AbilityImpl<ABILITY_REVERBATE> : extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_SOUND) CHECK(gBattleMoves[move].type == TYPE_NORMAL) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_TAEKKYEON> : extends OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_DANCE) CHECK_NOT(IS_MOVE_STATUS(move)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_SLUDGE_SPIT> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_VENOM_BOLT, 35);
    }
};

template <>
struct AbilityImpl<ABILITY_SWAMP_THING> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].swampTimer)

        AbilityStatusEffectSafe(MOVE_EFFECT_SWAMP, battler, GetOppositeSide(battler));
        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FROSTY_PRESCENCE> : extends SimpleEntryMove<MOVE_MIST> {};

template <>
struct AbilityImpl<ABILITY_CHILLING_PELLETS> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_SPEAR, 13);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_PAINT_SHOT> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, moveType))
        CHECK(IsMegaLauncherBoosted(battler, move))

            gBattleMons[target]
                .type1 = moveType;
        gBattleMons[target].type2 = moveType;
        gBattleMons[target].type3 = TYPE_MYSTERY;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        gStackBattler1 = target;
        BattleScriptCall(BattleScript_StackBecameTheTypeFull);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STONECUTTER> : extends AbilityImpl<ABILITY_FOSSILIZED>, extends OnMoldBreaker {
    ON_MOLD_BREAKER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_EDGELORD> : extends AbilityImpl<ABILITY_CUTTHROAT>, extends OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CUTTHROAT;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WARMONGER> : extends OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK || moveType == TYPE_STEEL || moveType == TYPE_FIGHTING) MUL(1.30);
    }
};

template <>
struct AbilityImpl<ABILITY_LOCUST_SWARM> : extends StandardTransformation {};

template <>
struct AbilityImpl<ABILITY_REVELATION> : extends StandardTransformation {};

template <>
struct AbilityImpl<ABILITY_CURSE_OF_FAMINE> : extends OnEntry {
    ON_ENTRY {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_CurseOfFamine);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRYSTALLINE_ARMOR> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SOUL_HARVEST> : extends OnStat<>, extends Breakable {
    ON_STAT {
        if (statId != STAT_SPEED) *stat = *stat * (20 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 20;
    }
};

template <>
struct AbilityImpl<ABILITY_THICK_BLUBBER> : extends OnDefensiveMultiplier, extends OnStat<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.25);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .5;
    }
};

template <>
struct AbilityImpl<ABILITY_CRAVING> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_RAT_KING> : extends OnStat<ApplyOn::ALLY> {
    ON_STAT {
        const BaseStats *baseStats = &gBaseStats[gBattleMons[battler].species];
        int bst =
            baseStats->baseHP + baseStats->baseAttack + baseStats->baseDefense + baseStats->baseSpAttack + baseStats->baseSpDefense + baseStats->baseSpeed;
        if (bst >= 400) return;
        *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CRISPY_CREAM> : extends OnDefender {
    ON_DEFENDER {
        return Random() % 2 ? AbilityImpl<ABILITY_FLAME_BODY>::onEither(DELEGATE_DEFENDER) : AbilityImpl<ABILITY_FREEZING_POINT>::onEither(DELEGATE_DEFENDER);
    }
};

template <>
struct AbilityImpl<ABILITY_DEEP_FRIED> : extends OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].fireSeaTimer)

        AbilityStatusEffectSafe(MOVE_EFFECT_FIRE_SEA, battler, GetOppositeSide(battler));
        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FOOD_LOVERS> : extends AbilityImpl<ABILITY_HOSPITALITY>, extends AbilityImpl<ABILITY_FRIEND_GUARD> {};

template <>
struct AbilityImpl<ABILITY_LUNAR_WRATH> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_GHOST)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MOONGEIST_BEAM, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_SPYWARE> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_VIRUS> : extends OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(CanBePoisoned(battler, target, move))

            return AbilityStatusEffect(MOVE_EFFECT_POISON);
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_LEAK> : extends OnDefender, extends AllowTerrainIfAirborne<TERRAIN_ELECTRIC> {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
        BattleScriptCall(BattleScript_SurgeActivatesRet);
        return TRUE;
    }
    TerrainType allowTerrainIfAirborne() { return TERRAIN_ELECTRIC; }
};

template <>
struct AbilityImpl<ABILITY_BACKUP_POWER> : extends OnRevive {
    ON_REVIVE { CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) return B_MSG_BACKUP_POWER; }
};

template <>
struct AbilityImpl<ABILITY_SAND_FIEND> : extends AbilityImpl<ABILITY_SAND_GUARD>, extends AbilityImpl<ABILITY_SAND_FORCE> {};

template <>
struct AbilityImpl<ABILITY_MOUSTACHE> : extends Merged<ABILITY_TANGLING_HAIR, ABILITY_STAMINA> {};

template <>
struct AbilityImpl<ABILITY_DEPTH_EXPLORER> : extends AbilityImpl<ABILITY_FIELD_EXPLORER>, extends AbilityImpl<ABILITY_ILLUMINATE> {};

template <>
struct AbilityImpl<ABILITY_DUNE_VEIL> : extends AbilityImpl<ABILITY_SAND_GUARD>, extends AbilityImpl<ABILITY_SELF_SUFFICIENT> {};

template <>
struct AbilityImpl<ABILITY_STRONG_FOUNDATION> : extends OnDefensiveMultiplier {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_GROUND) RESISTANCE(.50);
    }
};

template <>
struct AbilityImpl<ABILITY_FOG_MACHINE> : extends OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler)) CHECK_NOT(gBattleWeather & WEATHER_FOG_ANY) if (gBattleWeather & WEATHER_PRIMAL_ANY) {
            BattleScriptCall(BattleScript_BlockedByPrimalWeatherRet);
            return NO_ANNOUNCE;
        }
        else if (TryChangeBattleWeather(battler, ENUM_WEATHER_FOG, TRUE)) {
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_FogStartsReturn);
            return TRUE;
        }
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_DROP_BLOCKS> : extends OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LASER_DRILL> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_LIGHT_SABER> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_LOOSE_THORNS> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_TURF_WAR> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_GREEDY> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_MUSICAL_NOTES> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_STRIKEOUT> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_HOME_RUN> : extends NotImplemented {};

template <>
struct AbilityImpl<ABILITY_BRUISER> : extends AddsType<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_LETS_DANCE> : extends SimpleEntryMove<MOVE_TEETER_DANCE> {};

template <>
struct AbilityImpl<ABILITY_MYCELIUM_MIGHT> : extends OnMoldBreaker {
    ON_MOLD_BREAKER { return IS_MOVE_STATUS(move); }
};

template <>
struct AbilityImpl<ABILITY_DEADLY_PRECISION> : extends OnMoldBreaker {
    ON_MOLD_BREAKER {
        gHitMarker |= HITMARKER_MOLD_BREAKER;
        SetTypeBeforeUsingMove(move, gActiveBattler);
        u8 moveType;
        GET_MOVE_TYPE(move, moveType)
        u16 typeEffectiveness;
        CalculateMoveDamageAndEffectiveness(gCurrentMove, gBattlerAttacker, gBattlerTarget, &moveType, &typeEffectiveness);
        gHitMarker &= ~HITMARKER_MOLD_BREAKER;
        return typeEffectiveness >= UQ_4_12(2.0);
    }
};

static const AbilityImpl<ABILITY_CHLOROPLAST> test = AbilityImpl<ABILITY_CHLOROPLAST>();
#define __ON_ABILITY(ABILITY) static const AbilityImpl<ABILITY> __instance##ABILITY = AbilityImpl<ABILITY>();
OVER_ALL_ABILITIES
#undef __ON_ABILITY

template <typename As>
inline const As *dispatchTo(AbilityEnum id) {
    switch (id) {
#define __ON_ABILITY(ABILITY)                                 \
    case ABILITY:                                             \
        if (std::is_assignable_v<As, AbilityImpl<ABILITY>>) { \
            return (const As *)&__instance##ABILITY;          \
        } else {                                              \
            return nullptr;                                   \
        }
        OVER_ALL_ABILITIES
#undef __ON_ABILITY

        default:
            break;
    }

    return nullptr;
}

#pragma GCC diagnostic pop
