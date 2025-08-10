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
    operator AbilityEnum() const { return ABILITY_NONE; }
    operator bool() const { return false; }
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
struct AteAbility : is OnMoveType, is OnStab {
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_NORMAL)
        *ateBoost = TRUE;
        return BoostType + 1;
    }
    ON_STAB { return moveType == BoostType; }
};

template <Type BoostType>
struct SwarmLike : is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_NONE> : is RandomizerBanned {};

template <int Stat>
struct RuinEffect : is OnStat<ApplyOn::OTHER> {
    // static constexpr auto ruinAbilities = abilitiesAs<RuinEffect<Stat>>();
    ON_STAT {
        if (statId != Stat) return;
        if (*flags & Stat) return;
        ON_ABILITY(battler, FALSE, dispatchTo<RuinEffect<Stat>>(ability), return) *stat *= .75;
        *flags = *flags | static_cast<NonStackingState>(1 << Stat);
    }
};

template <>
struct AbilityImpl<ABILITY_TABLETS_OF_RUIN> : is RuinEffect<STAT_ATK> {};
template <>
struct AbilityImpl<ABILITY_SWORD_OF_RUIN> : is RuinEffect<STAT_DEF> {};
template <>
struct AbilityImpl<ABILITY_VESSEL_OF_RUIN> : is RuinEffect<STAT_SPATK> {};
template <>
struct AbilityImpl<ABILITY_BEADS_OF_RUIN> : is RuinEffect<STAT_SPDEF> {};

struct ToxicTerrainImmune {};
template <>
struct AbilityImpl<ABILITY_STENCH> : is OnAttacker, is ToxicTerrainImmune {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_HEAL> : is ToxicTerrainImmune {};

template <>
struct AbilityImpl<ABILITY_DRIZZLE> : is OnEntry {
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
struct AbilityImpl<ABILITY_SPEED_BOOST> : is OnEndTurn {
    ON_END_TURN {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BATTLE_ARMOR> : is Breakable, is OnDefensiveMultiplier<>, is OnCrit<ApplyOnTarget::TARGET> {
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
    ON_CRIT { return NEVER_CRIT; }
};

template <>
struct AbilityImpl<ABILITY_STURDY> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_DAMP> : is OnEither {
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

struct RemovesStatusOnImmunity : is OnStatusImmune<ApplyOn::SELF> {};

template <int Stat>
struct BlocksStatDrops : is Breakable {};

struct BlocksSelfStatDrops {};

template <>
struct AbilityImpl<ABILITY_LIMBER> : is RemovesStatusOnImmunity, is BlocksSelfStatDrops {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_PARALYSIS) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_LUCKY_HALO> : is BlocksSelfStatDrops {};

struct SandImmune {};
template <>
struct AbilityImpl<ABILITY_SAND_VEIL> : is Breakable, is SandImmune, is OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_STATIC> : is OnEither {
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
struct AbsorbHeal : is OnAbsorb {
    ON_ABSORB {
        CHECK(moveType == Absorbed)
        return ABSORB_RESULT_HEAL;
    }
};
template <>
struct AbilityImpl<ABILITY_VOLT_ABSORB> : is AbsorbHeal<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_WATER_ABSORB> : is AbsorbHeal<TYPE_WATER> {};

struct TauntImmune : is Breakable {};

template <>
struct AbilityImpl<ABILITY_OBLIVIOUS> : is RemovesStatusOnImmunity, is TauntImmune {
    ON_STATUS_IMMUNE { CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_CLOUD_NINE> : is OnEntry {
    ON_ENTRY {
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_COMPOUND_EYES> : is OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.3;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_INSOMNIA> : is RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_SLEEP) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_COLOR_CHANGE> : is OnBeforeAttack<ApplyOnTarget::TARGET> {
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
struct AbilityImpl<ABILITY_IMMUNITY> : is OnDefensiveMultiplier<>, is RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_POISON) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & (CHECK_STATUS1 & ~CHECK_SLEEP))
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLASH_FIRE> : is OnAbsorb, is OnOffensiveMultiplier<> {
    ON_ABSORB { CHECK(moveType == TYPE_FIRE) return ABSORB_RESULT_FLASH_FIRE; }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
    }
};

struct PowderImmune : is Breakable {};
template <>
struct AbilityImpl<ABILITY_SHIELD_DUST> : is PowderImmune {};

template <>
struct AbilityImpl<ABILITY_OWN_TEMPO> : is RemovesStatusOnImmunity, is TauntImmune {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_CONFUSION) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_SUCTION_CUPS> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_INTIMIDATE> : is OnEntry {
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
struct AbilityImpl<ABILITY_SHADOW_TAG> : is OnTrap {
    ON_TRAP { ON_ABILITY(switchingBattler, FALSE, dispatchTo<AbilityImpl<ABILITY_SHADOW_TAG>>(ability), return FALSE) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_ROUGH_SKIN> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasMagicGuard(attacker)) CHECK(IsMoveMakingContact(move, attacker)) gBattleMoveDamage = gBattleMons[attacker].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    }
};

struct RolePlayBanned {};
struct SkillSwapBanned : is RolePlayBanned {};

template <>
struct AbilityImpl<ABILITY_WONDER_GUARD> : is Breakable, is RandomizerBanned, is OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, is SkillSwapBanned {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod < UQ_4_12(2.0)) *mod = 0;
    }
};

struct GroundImmune : is Breakable {};
template <>
struct AbilityImpl<ABILITY_LEVITATE> : is GroundImmune, is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING) MUL(1.25);
    }
};

template <>
struct AbilityImpl<ABILITY_EFFECT_SPORE> : is PowderImmune, is OnDefender {
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
struct AbilityImpl<ABILITY_CLEAR_BODY> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_FULL_METAL_BODY> : is AbilityImpl<ABILITY_CLEAR_BODY> {};

template <>
struct AbilityImpl<ABILITY_NATURAL_CURE> : is OnExit {
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
struct AbsorbStatUp : is OnAbsorb {
    ON_ABSORB {
        CHECK(moveType == TYPE_ELECTRIC);
        int stat = Stat == STAT_HIGHEST_ATTACKING ? GetHighestAttackingStatId(battler, TRUE) : Stat;
        *statId = stat;
        return ABSORB_RESULT_STAT;
    }
};
template <Type Absorbed>
struct LightningRodClone : is Redirects<Absorbed>, is AbsorbStatUp<Absorbed, STAT_HIGHEST_ATTACKING> {};
template <>
struct AbilityImpl<ABILITY_LIGHTNING_ROD> : LightningRodClone<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_SERENE_GRACE> : is OnModifyEffectChance<> {
    ON_MODIFY_EFFECT_CHANCE { *effectChance *= 2; }
};

template <>
struct AbilityImpl<ABILITY_SWIFT_SWIM> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CHLOROPHYLL> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_ILLUMINATE> : is OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_TRACE> : is RandomizerBanned, is OnEntry, is RolePlayBanned {
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
struct AbilityImpl<ABILITY_HUGE_POWER> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_ATK) *stat *= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_POINT> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBePoisoned(battler, opponent, MOVE_NONE))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_INNER_FOCUS> : is TauntImmune, is OnAccuracy<> {
    ON_ACCURACY { CHECK(move == MOVE_FOCUS_BLAST) return ACCURACY_ALWAYS_HITS; }
};

template <>
struct AbilityImpl<ABILITY_MAGMA_ARMOR> : is OnDefensiveMultiplier<>, is RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_FROSTBITE)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WATER_VEIL> : is OnEntry, is RemovesStatusOnImmunity {
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
struct AbilityImpl<ABILITY_MAGNET_PULL> : is OnTrap {
    ON_TRAP { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); }
};

template <>
struct AbilityImpl<ABILITY_SOUNDPROOF> : is OnImmune<> {
    ON_IMMUNE {
        CHECK(IsSoundMove(attacker, move))
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RAIN_DISH> : is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_STREAM> : is SandImmune, is OnEntry {
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
struct AbilityImpl<ABILITY_PRESSURE> : is OnEntry {
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
struct AbilityImpl<ABILITY_THICK_FAT> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_FLAME_BODY> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_KEEN_EYE> : is OnAccuracy<>, is BlocksStatDrops<STAT_ACC> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_HYPER_CUTTER> : is BlocksStatDrops<STAT_ATK>, is BlocksStatDrops<STAT_SPATK>, is OnCrit<> {
    ON_CRIT { CHECK(IsMoveMakingContact(move, battler)) return 1; }
};

template <>
struct AbilityImpl<ABILITY_PICKUP> : is OnEntry {
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
struct AbilityImpl<ABILITY_TRUANT> : is OnEndTurn {
    ON_END_TURN {
        if (GetAbilityState(battler, ability))
            SetAbilityState(battler, ability, FALSE);
        else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
            SetAbilityState(battler, ability, TRUE);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_HUSTLE> : is OnAccuracy<>, is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER { MUL(1.4); }
    ON_ACCURACY {
        CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_CUTE_CHARM> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(CanInfatuate(battler, opponent))
        CHECK(Random() % 100 < 50)

        AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PLUS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        int partner = BATTLE_PARTNER(battler);
        if (!IsBattlerAlive(partner)) return;
        if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
    }
};

template <>
struct AbilityImpl<ABILITY_MINUS> : is AbilityImpl<ABILITY_PLUS> {};

struct StandardTransformation : is FormChangeAbility, is OnEntry, is OnEndTurn {
    ON_ENTRY { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_END_TURN { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
};
struct WeatherTransformation : is StandardTransformation, is OnWeather {
    ON_WEATHER { return TryTransformAttacker(ability, battler, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_FORECAST> : is WeatherTransformation, is OnAttacker {
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
struct AbilityImpl<ABILITY_STICKY_HOLD> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_SHED_SKIN> : is OnEndTurn {
    ON_END_TURN {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

struct NegateBurnAtkDrop {};
template <>
struct AbilityImpl<ABILITY_GUTS> : is OnOffensiveMultiplier<>, is NegateBurnAtkDrop {
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MARVEL_SCALE> : is OnStat<> {
    ON_STAT {
        if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_OVERGROW> : is SwarmLike<TYPE_GRASS> {};

template <>
struct AbilityImpl<ABILITY_BLAZE> : is SwarmLike<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_TORRENT> : is SwarmLike<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_SWARM> : is SwarmLike<TYPE_BUG> {};

template <>
struct AbilityImpl<ABILITY_ROCK_HEAD> : is RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_CONFUSION) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_DROUGHT> : is OnEntry {
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
struct AbilityImpl<ABILITY_ARENA_TRAP> : is OnTrap {
    ON_TRAP { return IsBattlerGrounded(switchingBattler); }
};

template <>
struct AbilityImpl<ABILITY_VITAL_SPIRIT> : is OnAttacker, is RemovesStatusOnImmunity, is TauntImmune {
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
struct AbilityImpl<ABILITY_WHITE_SMOKE> : is OnEntry {
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
struct AbilityImpl<ABILITY_FELINE_PROWESS> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPATK) *stat *= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_PURE_POWER> : is AbilityImpl<ABILITY_FELINE_PROWESS> {};

template <>
struct AbilityImpl<ABILITY_SHELL_ARMOR> : is AbilityImpl<ABILITY_BATTLE_ARMOR> {};

template <>
struct AbilityImpl<ABILITY_AIR_LOCK> : is AbilityImpl<ABILITY_CLOUD_NINE> {};

template <>
struct AbilityImpl<ABILITY_TANGLED_FEET> : is OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOTOR_DRIVE> : is AbsorbStatUp<TYPE_ELECTRIC, STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_RIVALRY> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_SNOW_CLOAK> : is Breakable, is HailImmune, is OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANGER_POINT> : is OnDefender {
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
struct AbilityImpl<ABILITY_UNBURDEN> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && GetUnburdenState(battler)) *stat *= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_HEATPROOF> : is OnDefensiveMultiplier<>, is NegateBurnAtkDrop {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRY_SKIN> : is AbilityImpl<ABILITY_WATER_ABSORB>, is AbilityImpl<ABILITY_RAIN_DISH>, is OnDefensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_DOWNLOAD> : is OnEntry {
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
struct AbilityImpl<ABILITY_IRON_FIST> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsIronFistBoosted(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_ADAPTABILITY> {};

template <>
struct AbilityImpl<ABILITY_SKILL_LINK> {};

template <>
struct AbilityImpl<ABILITY_HYDRATION> : is OnEndTurn {
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOLAR_POWER> : is OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_QUICK_FEET> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_NORMALIZE> : is OnOffensiveMultiplier<>, is OnMoveType, is OnTypeEffectiveness {
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
struct AbilityImpl<ABILITY_SNIPER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (isCrit) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MAGIC_GUARD> {};

template <>
struct AbilityImpl<ABILITY_NO_GUARD> : is OnAccuracy<ApplyOnTarget::ATTACKER_OR_TARGET> {
    ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
};

template <>
struct AbilityImpl<ABILITY_STALL> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
    }
};

template <>
struct AbilityImpl<ABILITY_TECHNICIAN> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (basePower <= 60) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_LEAF_GUARD> : is OnEndTurn {
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOLD_BREAKER> : is OnEntry, is OnMoldBreaker {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); }
    ON_MOLD_BREAKER { return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_SUPER_LUCK> : is OnCrit<> {
    ON_CRIT { return 1; }
};

template <>
struct AbilityImpl<ABILITY_AFTERMATH> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(HasMagicGuard(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = 1;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANTICIPATION> : is Breakable, is OnEntry, is Persistent {
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
struct AbilityImpl<ABILITY_FOREWARN> : is OnEntry {
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
struct AbilityImpl<ABILITY_UNAWARE> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_TINTED_LENS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
    }
};

template <>
struct AbilityImpl<ABILITY_FILTER> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
    }
};

template <>
struct AbilityImpl<ABILITY_SLOW_START> : is OnEntry, is OnStat<> {
    ON_ENTRY {
        gVolatileStructs[battler].slowStartTimer = 5;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
    }
    ON_STAT {
        if (statId != STAT_ATK && statId != STAT_SPATK && statId != STAT_SPEED) return;
        if (gVolatileStructs[battler].slowStartTimer) *stat /= 2;
    }
};

struct HitsGhost : is OnTypeEffectiveness {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
        CHECK(defType == TYPE_GHOST)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    }
};
template <>
struct AbilityImpl<ABILITY_SCRAPPY> : is HitsGhost, is TauntImmune {};

template <>
struct AbilityImpl<ABILITY_STORM_DRAIN> : is LightningRodClone<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_ICE_BODY> : is HailImmune, is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOLID_ROCK> : is AbilityImpl<ABILITY_FILTER> {};

template <>
struct AbilityImpl<ABILITY_SNOW_WARNING> : is HailImmune, is OnEntry {
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
struct AbilityImpl<ABILITY_HONEY_GATHER> : is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK(Random() % 2)

        gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
        BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FRISK> : is OnEntry {
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
struct AbilityImpl<ABILITY_RECKLESS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_MULTITYPE> : is FormChangeAbility {};

template <>
struct AbilityImpl<ABILITY_FLOWER_GIFT> : is WeatherTransformation, is Breakable, is OnStat<ApplyOn::ALLY> {
    ON_STAT {
        if (statId != STAT_SPATK && statId != STAT_SPDEF) return;
        if (IsWeatherActive(WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_BAD_DREAMS> : is OnEndTurn {
    ON_END_TURN {
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHEER_FORCE> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_CONTRARY> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_UNNERVE> : is OnEntry {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); }
};

template <>
struct AbilityImpl<ABILITY_DEFEATIST> : is OnStat<> {
    ON_STAT {
        if (statId != STAT_ATK && statId != STAT_SPATK) return;
        if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_CURSED_BODY> : is OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_HEALER> : is OnEndTurn {
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
struct AbilityImpl<ABILITY_FRIEND_GUARD> : is OnDefensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_DEFENSIVE_MULTIPLIER {
        MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_WEAK_ARMOR> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(IS_MOVE_PHYSICAL(move))
        CHECK(CanRaiseStat(battler, STAT_SPEED) || CanLowerStat(battler, STAT_DEF))

        if (gBattleMoves[move].effect == EFFECT_HIT_ESCAPE && CanBattlerSwitch(attacker))
            gRoundStructs[battler].disableEjectPack = TRUE;  // Set flag for target

        BattleScriptCall(BattleScript_WeakArmorActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LIGHT_METAL> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= 1.3;
    }
};

template <>
struct AbilityImpl<ABILITY_MULTISCALE> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (BATTLER_MAX_HP(battler)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_TOXIC_BOOST> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_FLARE_BOOST> : is OnEntry, is OnWeather, is OnStat<>, is NegateBurnAtkDrop {
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
struct AbilityImpl<ABILITY_HARVEST> : is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK_NOT(gBattleStruct->changedItems[battler])
        CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

        BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TELEPATHY> : is OnAfterTypeEffectiveness<ApplyOnTarget::ATTACKER_OR_TARGET> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_MOODY> : is OnEndTurn {
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
struct AbilityImpl<ABILITY_OVERCOAT> : is Breakable, is SandImmune, is HailImmune, is PowderImmune, is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.8);
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_TOUCH> : is AbilityImpl<ABILITY_POISON_POINT> {};

template <>
struct AbilityImpl<ABILITY_REGENERATOR> : is OnExit {
    ON_EXIT {
        CHECK(IsBattlerAlive(battler)) CHECK_NOT(BATTLER_MAX_HP(battler)) BattleScriptCall(BattleScript_RegeneratorExits);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_BIG_PECKS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsMoveMakingContact(move, battler)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_RUSH> : is OnStat<>, is SandImmune {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_WONDER_SKIN> : is AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_ANALYTIC> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_ILLUSION> : is OnDefender, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_IMPOSTER> : is OnEntry {
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
struct AbilityImpl<ABILITY_INFILTRATOR> : is OnInfiltrate {
    ON_INFILTRATE { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; }
};

template <>
struct AbilityImpl<ABILITY_MUMMY> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPersistentOrUnsuppressable(GetBattlerAbility(attacker)))
        CHECK_NOT(DoesBattlerHaveAbilityShield(attacker))

        UpdateAbilityStateIndicesForNewAbility(attacker, ability);
        ReplaceAbility(attacker, ability);
        BattleScriptCall(BattleScript_MummyActivates);
        return TRUE;
    }
};

template <int Stat>
struct MoxieClone : is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(HasAttackerFaintedTarget())
        int stat = Stat == STAT_HIGHEST_TOTAL ? GetHighestStatId(battler, FALSE) : Stat;
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))
        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOXIE> : is MoxieClone<STAT_ATK> {};

template <>
struct AbilityImpl<ABILITY_JUSTIFIED> : is AbsorbStatUp<TYPE_DARK, STAT_HIGHEST_ATTACKING> {};

template <>
struct AbilityImpl<ABILITY_RATTLED> : is OnDefender {
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
struct AbilityImpl<ABILITY_MAGIC_BOUNCE> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_SAP_SIPPER> : is LightningRodClone<TYPE_GRASS> {};

template <>
struct AbilityImpl<ABILITY_PRANKSTER> : is OnPriority {
    ON_PRIORITY { CHECK(IS_MOVE_STATUS(move)) return 1; }
};

template <>
struct AbilityImpl<ABILITY_SAND_FORCE> : is SandImmune, is OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_BARBS> : is AbilityImpl<ABILITY_ROUGH_SKIN> {};

template <>
struct AbilityImpl<ABILITY_ZEN_MODE> : is StandardTransformation {};

template <>
struct AbilityImpl<ABILITY_VICTORY_STAR> : is OnAccuracy<ApplyOnTarget::ALLY> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <Type ExtraType>
struct AddsType : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, ExtraType))

        gBattleMons[battler].type3 = ExtraType;
        PREPARE_TYPE_BUFFER(gBattleTextBuff2, ExtraType);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TURBOBLAZE> : is AbilityImpl<ABILITY_MOLD_BREAKER>, is AddsType<TYPE_FIRE> {
    ON_ENTRY { return AddsType<TYPE_FIRE>::onEntry(DELEGATE_ENTRY); }
};

template <>
struct AbilityImpl<ABILITY_TERAVOLT> : is AbilityImpl<ABILITY_MOLD_BREAKER>, is AddsType<TYPE_ELECTRIC> {
    ON_ENTRY { return AddsType<TYPE_ELECTRIC>::onEntry(DELEGATE_ENTRY); }
};

template <>
struct AbilityImpl<ABILITY_AROMA_VEIL> : is OnStatusImmune<ApplyOn::ALLY> {
    ON_STATUS_IMMUNE { CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_FLOWER_VEIL> : is OnStatusImmune<ApplyOn::ALLY> {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_STATUS1) CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_CHEEK_POUCH> : is RandomizerBanned {};

struct AlwaysStab {};

template <>
struct AbilityImpl<ABILITY_PROTEAN> : is OnBeforeAttack<>, is AlwaysStab {
    ON_BEFORE_ATTACK {
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, moveType)) CHECK(move != MOVE_STRUGGLE) SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        BattleScriptCall(BattleScript_ProteanActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FUR_COAT> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_BULLETPROOF> : is OnImmune<> {
    ON_IMMUNE {
        CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STRONG_JAW> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_REFRIGERATE> : is AteAbility<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_SWEET_VEIL> : is OnStatusImmune<ApplyOn::ALLY> {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_SLEEP) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_STANCE_CHANGE> : is FormChangeAbility, is OnBeforeAttack<> {
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
struct GaleWingsLike : is OnPriority {
    ON_PRIORITY {
        CHECK(GetTypeBeforeUsingMove(move, battler) == GaleWingsType)
        CHECK(BATTLER_MAX_HP(battler))
        return 1;
    }
};
template <>
struct AbilityImpl<ABILITY_GALE_WINGS> : is GaleWingsLike<TYPE_FLYING> {};

template <>
struct AbilityImpl<ABILITY_MEGA_LAUNCHER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_GRASS_PELT> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_TOUGH_CLAWS> : is AbilityImpl<ABILITY_BIG_PECKS> {};

template <>
struct AbilityImpl<ABILITY_PIXILATE> : is AteAbility<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_GOOEY> : is OnDefender {
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
struct AbilityImpl<ABILITY_AERILATE> : is AteAbility<TYPE_FLYING> {};

template <>
struct AbilityImpl<ABILITY_HYPER_AGGRESSIVE> : is OnParentalBond {
    ON_PARENTAL_BOND { return PARENTAL_BOND_HYPER_AGGRESSIVE; }
};

struct IgnoresFortKnox {};
template <>
struct AbilityImpl<ABILITY_PARENTAL_BOND> : is AbilityImpl<ABILITY_HYPER_AGGRESSIVE>, is IgnoresFortKnox {};

template <>
struct AbilityImpl<ABILITY_DARK_AURA> : is OnEntry, is OnOffensiveMultiplier<ApplyOn::ANY> {
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
struct AbilityImpl<ABILITY_FAIRY_AURA> : is OnEntry, is OnOffensiveMultiplier<ApplyOn::ANY> {
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
struct AbilityImpl<ABILITY_AURA_BREAK> : is Breakable, is OnEntry {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); }
};

template <>
struct AbilityImpl<ABILITY_PRIMORDIAL_SEA> : is OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DESOLATE_LAND> : is OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WEATHER_CONTROL> : is OnImmune<> {
    ON_IMMUNE {
        CHECK(gBattleMoves[move].flags & FLAG_WEATHER_BASED)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DELTA_STREAM> : is AbilityImpl<ABILITY_WEATHER_CONTROL>, is OverrideBreakable, is OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STAMINA> : is OnDefender {
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
struct AbilityImpl<ABILITY_WIMP_OUT> : is OnDefender {
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(TestSheerForceFlag(attacker, gCurrentMove))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA) CHECK(CountUsablePartyMons(battler));
        gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_EMERGENCY_EXIT> : is AbilityImpl<ABILITY_WIMP_OUT> {};

template <>
struct AbilityImpl<ABILITY_WATER_COMPACTION> : is OnDefensiveMultiplier<>, is OnDefender {
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
struct AbilityImpl<ABILITY_MERCILESS> : is OnCrit<> {
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
struct AbilityImpl<ABILITY_SHIELDS_DOWN> : is StandardTransformation, is OnAttacker, is OnStatusImmune<>, is OverrideBreakable {
    ON_ATTACKER {
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
struct AbilityImpl<ABILITY_STAKEOUT> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
    }
};

template <>
struct AbilityImpl<ABILITY_WATER_BUBBLE> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<>, is RemovesStatusOnImmunity {
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
struct AbilityImpl<ABILITY_STEELWORKER> : is Breakable, is OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, is AteAbility<TYPE_STEEL> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
    }
};

template <>
struct AbilityImpl<ABILITY_BERSERK> : is OnDefender {
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
struct AbilityImpl<ABILITY_SLUSH_RUSH> : is HailImmune, is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_LONG_REACH> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move) && !gBattleMoves[move].contact) MUL(1.2);
    }
};

template <Type BoostType>
struct LiquidVoiceClone : is OnOffensiveMultiplier<>, is OnMoveType {
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
struct AbilityImpl<ABILITY_LIQUID_VOICE> : is LiquidVoiceClone<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_TRIAGE> : is OnPriority {
    ON_PRIORITY { CHECK(IsHealingMoveEffect(gBattleMoves[move].effect)) return 3; }
};

template <>
struct AbilityImpl<ABILITY_GALVANIZE> : is AteAbility<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_SURGE_SURFER> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_SCHOOLING> : is StandardTransformation {
    ON_ENTRY { CHECK(gBattleMons[battler].level >= 20) return StandardTransformation::onEntry(DELEGATE_ENTRY); }
    ON_END_TURN {
        CHECK(gBattleMons[battler].level >= 20)
        return StandardTransformation::onEndTurn(DELEGATE_END_TURN);
    }
};

template <>
struct AbilityImpl<ABILITY_DISGUISE> : is FormChangeAbility, is OnEntry, is OnDisguise, is OnWeather {
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
struct AbilityImpl<ABILITY_BATTLE_BOND> : is FormChangeAbility, is OnBattlerFaints<> {
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
struct AbilityImpl<ABILITY_POWER_CONSTRUCT> : is FormChangeAbility, is OnEndTurn {
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
struct AbilityImpl<ABILITY_CORROSION> : is OnTypeEffectiveness, is OnCanStatusType {
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
struct AbilityImpl<ABILITY_COMATOSE> : is OnEntry, is RemovesStatusOnImmunity, is Unsuppressable {
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
struct AbilityImpl<ABILITY_QUEENLY_MAJESTY> : is OnImmune<ApplyOn::ALLY> {
    ON_IMMUNE { return blocksPriority(DELEGATE_IMMUNE); }
};

template <>
struct AbilityImpl<ABILITY_INNARDS_OUT> : is OnDefender {
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
struct AbilityImpl<ABILITY_DANCER> : is OnCopyMove {
    ON_COPY_MOVE { CHECK(IsDance(attacker, move)) return UseOutOfTurnAttack(battler, target, ability, move, 0); }
};

template <>
struct AbilityImpl<ABILITY_BATTERY> : is OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_FLUFFY> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(2.0);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DAZZLING> : is AbilityImpl<ABILITY_QUEENLY_MAJESTY> {};

template <>
struct AbilityImpl<ABILITY_SOUL_HEART> : is OnBattlerFaints<ApplyOnTarget::ANY> {
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TANGLING_HAIR> : is AbilityImpl<ABILITY_GOOEY> {};

template <>
struct AbilityImpl<ABILITY_RECEIVER> : is OnBattlerFaints<ApplyOnTarget::ALLY_IS_TARGET>, is RolePlayBanned {
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
struct AbilityImpl<ABILITY_POWER_OF_ALCHEMY> : is OnEntry, is OnReactive, is OnBattlerFaints<ApplyOnTarget::ANY> {
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
struct AbilityImpl<ABILITY_BEAST_BOOST> : is MoxieClone<STAT_HIGHEST_TOTAL> {};

template <>
struct AbilityImpl<ABILITY_RKS_SYSTEM> : is AbilityImpl<ABILITY_PROTEAN>, is AbilityImpl<ABILITY_ADAPTABILITY>, is FormChangeAbility {};

template <>
struct AbilityImpl<ABILITY_ELECTRIC_SURGE> : is AllowTerrainIfAirborne<TERRAIN_ELECTRIC>, is OnEntry {
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
struct AbilityImpl<ABILITY_PSYCHIC_SURGE> : is AllowTerrainIfAirborne<TERRAIN_PSYCHIC>, is OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MISTY_SURGE> : is AllowTerrainIfAirborne<TERRAIN_MISTY>, is OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_GRASSY_SURGE> : is AllowTerrainIfAirborne<TERRAIN_GRASSY>, is OnEntry {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHADOW_SHIELD> : is AbilityImpl<ABILITY_MULTISCALE>, is OverrideBreakable {};

template <>
struct AbilityImpl<ABILITY_PRISM_ARMOR> : is AbilityImpl<ABILITY_FILTER>, is OverrideBreakable {};

template <>
struct AbilityImpl<ABILITY_NEUROFORCE> : is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_INTREPID_SWORD> : is RaiseStatOnEntry<STAT_ATK> {};

template <>
struct AbilityImpl<ABILITY_DAUNTLESS_SHIELD> : is RaiseStatOnEntry<STAT_DEF> {};

template <>
struct AbilityImpl<ABILITY_LIBERO> : is AbilityImpl<ABILITY_PROTEAN> {};

template <>
struct AbilityImpl<ABILITY_COTTON_DOWN> : is OnDefender {
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
struct AbilityImpl<ABILITY_MIRROR_ARMOR> : is Breakable {};

template <>
struct AbilityImpl<ABILITY_GULP_MISSILE> : is FormChangeAbility, is OnDefender, is OnAttacker {
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
struct AbilityImpl<ABILITY_STEAM_ENGINE> : is OnDefender {
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
struct AbilityImpl<ABILITY_AMPLIFIER> : is OnOffensiveMultiplier<>, is OnMakeSpread {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsSoundMove(battler, move)) MUL(1.3);
    }
    ON_MAKE_SPREAD { return IsSoundMove(battler, move); }
};

template <>
struct AbilityImpl<ABILITY_PUNK_ROCK> : is OnDefensiveMultiplier<>, is AbilityImpl<ABILITY_AMPLIFIER> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IsSoundMove(attacker, move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_SPIT> : is SandImmune, is OnDefender {
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
struct AbilityImpl<ABILITY_ICE_SCALES> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_ICE_FACE> : is FormChangeAbility, is HailImmune, is OnEntry, is OnDisguise, is OnWeather {
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
struct AbilityImpl<ABILITY_POWER_SPOT> : is OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_OFFENSIVE_MULTIPLIER { MUL(1.3); }
};

template <>
struct AbilityImpl<ABILITY_MIMICRY> : is OnEntry, is OnTerrain {
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
struct AbilityImpl<ABILITY_SCREEN_CLEANER> : is OnEntry {
    ON_ENTRY {
        CHECK(TryRemoveScreens(battler))

        return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
    }
};

template <>
struct AbilityImpl<ABILITY_STEELY_SPIRIT> : is OnOffensiveMultiplier<ApplyOn::ALLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_STEEL) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PERISH_BODY> : is OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_WANDERING_SPIRIT> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(GetBattlerAbility(battler) == ability)
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPersistentOrUnsuppressable(GetBattlerAbility(attacker)))
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
    }
};

template <>
struct AbilityImpl<ABILITY_GORILLA_TACTICS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_NEUTRALIZING_GAS> : is Unsuppressable {};

template <>
struct AbilityImpl<ABILITY_PASTEL_VEIL> : is OnEntry {
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
struct AbilityImpl<ABILITY_HUNGER_SWITCH> : is FormChangeAbility, is OnEndTurn {
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
struct AbilityImpl<ABILITY_CURIOUS_MEDICINE> : is OnEntry {
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
struct AbilityImpl<ABILITY_TRANSISTOR> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRAGONS_MAW> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DRAGON) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_CHILLING_NEIGH> : is AbilityImpl<ABILITY_MOXIE> {};

template <>
struct AbilityImpl<ABILITY_GRIM_NEIGH> : is MoxieClone<STAT_SPATK> {};

template <AbilityEnum FaintAbility>
struct AsOne : is Unsuppressable, is RandomizerBanned, is AbilityImpl<ABILITY_UNNERVE>, is AbilityImpl<FaintAbility> {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE); }
    ON_BATTLER_FAINTS {
        CHECK(AbilityImpl<FaintAbility>::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_CHILLING_NEIGH;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    }
};

template <>
struct AbilityImpl<ABILITY_AS_ONE_ICE_RIDER> : is AsOne<ABILITY_CHILLING_NEIGH> {};

template <>
struct AbilityImpl<ABILITY_AS_ONE_SHADOW_RIDER> : is AsOne<ABILITY_GRIM_NEIGH> {};

template <>
struct AbilityImpl<ABILITY_CHLOROPLAST> {};

template <>
struct AbilityImpl<ABILITY_WHITEOUT> : is HailImmune, is OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_PYROMANCY> : is OnModifyEffectChance<> {
    ON_MODIFY_EFFECT_CHANCE {
        if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
    }
};

template <>
struct AbilityImpl<ABILITY_KEEN_EDGE> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PRISM_SCALES> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.7);
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_FISTS> : is AbilityImpl<ABILITY_IRON_FIST>, is OnChooseDefensiveStat<> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK(IsIronFistBoosted(battler, move)) return STAT_SPDEF; }
};

template <>
struct AbilityImpl<ABILITY_SAND_SONG> : is LiquidVoiceClone<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_RAMPAGE> : is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        SetAbilityState(battler, ability, TRUE);
        gVolatileStructs[battler].rechargeTimer = 0;
        gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_VENGEANCE> : is SwarmLike<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_BLITZ_BOXER> : is OnPriority {
    ON_PRIORITY {
        CHECK(IsIronFistBoosted(battler, move)) CHECK(BATTLER_MAX_HP(battler));
        return 1;
    }
};

template <>
struct AbilityImpl<ABILITY_ANTARCTIC_BIRD> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_IMMOLATE> : is AteAbility<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_CRYSTALLIZE> : is OnOffensiveMultiplier<>, is OnMoveType {
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
struct AbilityImpl<ABILITY_ELECTROCYTES> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.25);
    }
};

template <>
struct AbilityImpl<ABILITY_AERODYNAMICS> : is AbsorbStatUp<TYPE_FLYING, STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_CHRISTMAS_SPIRIT> : is OnDefensiveMultiplier<>, is HailImmune {
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_EXPLOIT_WEAKNESS> : is OnOffensiveMultiplier<>, is OnChooseDefensiveStat<> {
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
struct AbilityImpl<ABILITY_GROUND_SHOCK> : is OnTypeEffectiveness {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_ELECTRIC) CHECK(defType == TYPE_GROUND) CHECK_NOT(*mod) *mod = UQ_4_12(.5);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANCIENT_IDOL> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; }
};

template <>
struct AbilityImpl<ABILITY_MYSTIC_POWER> : is OnStab, is AlwaysStab {
    ON_STAB { return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_PERFECTIONIST> : is OnPriority, is OnCrit<> {
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
struct AbilityImpl<ABILITY_GROWING_TOOTH> : is OnAttacker {
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
struct AbilityImpl<ABILITY_INFLATABLE> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF)) CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
        BattleScriptCall(BattleScript_InflatableActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AURORA_BOREALIS> : is HailImmune, is OnStab {
    ON_STAB { return moveType == TYPE_ICE; }
};

template <>
struct AbilityImpl<ABILITY_AVENGER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_LETS_ROLL> : is OnEntry {
    ON_ENTRY {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LOUD_BANG> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeConfused(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
    }
};

template <>
struct AbilityImpl<ABILITY_LEAD_COAT> : is OnDefensiveMultiplier<>, is OnStat<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.6);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .9;
    }
};

template <>
struct AbilityImpl<ABILITY_AMPHIBIOUS> : is OnStab, is OnStatusImmune<> {
    ON_STAB { return moveType == TYPE_WATER; }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_DRENCH)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_GROUNDED> : is AddsType<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_EARTHBOUND> : is SwarmLike<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_FIGHT_SPIRIT> : is AteAbility<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_COIL_UP> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

        gStatuses4[battler] |= STATUS4_COILED;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FOSSILIZED> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MAGICAL_DUST> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_PSYCHIC))

        gBattleMons[attacker].type3 = TYPE_PSYCHIC;
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
struct AbilityImpl<ABILITY_DREAMCATCHER> : is OnOffensiveMultiplier<>, is UseTurnAttackAsPursuit {
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
struct AbilityImpl<ABILITY_NOCTURNAL> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK) MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
    }
};

template <>
struct AbilityImpl<ABILITY_SELF_SUFFICIENT> : is OnEndTurn {
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
struct AbilityImpl<ABILITY_TECTONIZE> : is AteAbility<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_ICE_AGE> : is AddsType<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_HALF_DRAKE> : is AddsType<TYPE_DRAGON> {};

template <>
struct AbilityImpl<ABILITY_AQUATIC> : is AddsType<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_LIQUIFIED> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) RESISTANCE(2);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRAGONFLY> : is AbilityImpl<ABILITY_HALF_DRAKE>, is GroundImmune {};

template <Type StrongVs>
struct TypeSlayer : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(target, StrongVs)) RESISTANCE(1.5);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(attacker, StrongVs)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DRAGONSLAYER> : is TypeSlayer<TYPE_DRAGON> {};

struct StealthRockImmune {};
template <>
struct AbilityImpl<ABILITY_MOUNTAINEER> : is OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, is StealthRockImmune {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_ROCK) *mod = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_HYDRATE> : is AteAbility<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_METALLIC> : is AddsType<TYPE_STEEL> {};

template <>
struct AbilityImpl<ABILITY_PERMAFROST> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
    }
};

template <>
struct AbilityImpl<ABILITY_PRIMAL_ARMOR> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_RAGING_BOXER> : is OnParentalBond {
    ON_PARENTAL_BOND { CHECK(IsIronFistBoosted(battler, move)) return PARENTAL_BOND_PRIMAL_MAW; }
};

template <>
struct AbilityImpl<ABILITY_AIR_BLOWER> : is OnEntry {
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
struct AbilityImpl<ABILITY_JUGGERNAUT> : is OnChooseOffensiveStat, is RemovesStatusOnImmunity {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHORT_CIRCUIT> : is SwarmLike<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_MAJESTIC_BIRD> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPATK) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_PHANTOM> : is AddsType<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_INTOXICATE> : is AteAbility<TYPE_POISON> {};

template <>
struct AbilityImpl<ABILITY_IMPENETRABLE> : is AbilityImpl<ABILITY_MAGIC_GUARD> {};

template <>
struct AbilityImpl<ABILITY_HYPNOTIST> : is OnAccuracy<> {
    ON_ACCURACY {
        CHECK(move == MOVE_HYPNOSIS);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_OVERWHELM> : is OnTypeEffectiveness, is TauntImmune {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SCARE> : is AbilityImpl<ABILITY_INTIMIDATE> {};

template <>
struct AbilityImpl<ABILITY_MAJESTIC_MOTH> : is OnEntry {
    ON_ENTRY {
        CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOUL_EATER> : is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK_NOT(BATTLER_MAX_HP(battler));
        CHECK(CanBattlerHeal(battler));
        BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SOUL_LINKER> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER))
        CHECK(move != MOVE_PAIN_SPLIT)

        BattleScriptCall(BattleScript_AttackerSoulLinker);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SWEET_DREAMS> : is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gBattleMons[battler].status1 & STATUS1_SLEEP || HasComatose(battler))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SweetDreamsActivates);
        return TRUE;
    }
};

struct ForcesMinRolls {};

template <>
struct AbilityImpl<ABILITY_BAD_LUCK> : is Breakable, is OnCrit<ApplyOnTarget::FOE>, is OnModifyEffectChance<ApplyOn::FOE>, is ForcesMinRolls {
    ON_CRIT { return NEVER_CRIT; }
    ON_MODIFY_EFFECT_CHANCE {
        if (*effectChance < 1) *effectChance = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_HAUNTED_SPIRIT> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_GHOST))
        CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMons[attacker].status2 |= STATUS2_CURSED;
        BattleScriptCall(BattleScript_HauntedSpiritActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ELECTRIC_BURST> : is OnRecoil, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_RAW_WOOD> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SOLENOGLYPHS> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

template <>
struct AbilityImpl<ABILITY_SPIDER_LAIR> : is OnEntry {
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
struct AbilityImpl<ABILITY_FATAL_PRECISION> : is OnAccuracy<>, is OnCrit<> {
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
struct AbilityImpl<ABILITY_SEAWEED> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS)) RESISTANCE(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_PSYCHIC_MIND> : is SwarmLike<TYPE_PSYCHIC> {};

template <>
struct AbilityImpl<ABILITY_POISON_ABSORB> : is Redirects<TYPE_POISON>, is AbsorbHeal<TYPE_POISON>, is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SCAVENGER> : is AbilityImpl<ABILITY_SOUL_EATER> {};

template <>
struct AbilityImpl<ABILITY_TWISTED_DIMENSION> : is OnEntry {
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
struct AbilityImpl<ABILITY_MULTI_HEADED> : is OnParentalBond, is IgnoresFortKnox {
    ON_PARENTAL_BOND {
        if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
        if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
        return MULTIHIT_SINGLE;
    }
};

template <>
struct AbilityImpl<ABILITY_NORTH_WIND> : is HailImmune, is OnEntry {
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
struct AbilityImpl<ABILITY_OVERCHARGE> : is OnTypeEffectiveness, is OnCanStatusType {
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
struct AbilityImpl<ABILITY_VIOLENT_RUSH> : is OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
    }
};

template <>
struct AbilityImpl<ABILITY_FLAMING_SOUL> : is GaleWingsLike<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_SAGE_POWER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_BONE_ZONE> : is OnAfterTypeEffectiveness<> {
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
struct AbilityImpl<ABILITY_SPEED_FORCE> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
    }
};

template <>
struct AbilityImpl<ABILITY_SEA_GUARDIAN> : is OnEntry {
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
struct AbilityImpl<ABILITY_MOLTEN_DOWN> : is OnTypeEffectiveness {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_FIRE) CHECK(defType == TYPE_ROCK) *mod = UQ_4_12(2.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLOCK> : is SwarmLike<TYPE_FLYING> {};

template <>
struct AbilityImpl<ABILITY_FIELD_EXPLORER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_STRIKER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IsStrikerBoosted(battler, move)) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_FROZEN_SOUL> : is GaleWingsLike<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_PREDATOR> : is AbilityImpl<ABILITY_SOUL_EATER> {};

template <>
struct AbilityImpl<ABILITY_LOOTER> : is AbilityImpl<ABILITY_SOUL_EATER> {};

template <>
struct AbilityImpl<ABILITY_LUNAR_ECLIPSE> : is AbilityImpl<ABILITY_HYPNOTIST>, is OnStab {
    ON_STAB { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; }
};

template <>
struct AbilityImpl<ABILITY_SOLAR_FLARE> : is AbilityImpl<ABILITY_IMMOLATE>, is AbilityImpl<ABILITY_CHLOROPLAST> {};

template <>
struct AbilityImpl<ABILITY_POWER_CORE> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; }
};

template <>
struct AbilityImpl<ABILITY_SIGHTING_SYSTEM> : is OnAccuracy<>, is OnPriority {
    ON_ACCURACY { return ACCURACY_HITS_IF_POSSIBLE; }
    ON_PRIORITY {
        CHECK(gBattleMoves[move].accuracy)
        CHECK(gBattleMoves[move].accuracy < 80);
        return -3;
    }
};

template <>
struct AbilityImpl<ABILITY_BAD_COMPANY> : is RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_OPPORTUNIST> : is OnPriority {
    ON_PRIORITY { CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2) return 1; }
};

template <>
struct AbilityImpl<ABILITY_GIANT_WINGS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].airBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_MOMENTUM> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
    }
};

template <>
struct AbilityImpl<ABILITY_GRIP_PINCER> : is OnAttacker, is OnAccuracy<> {
    ON_ATTACKER {
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
    }
    ON_ACCURACY {
        CHECK(gBattleMons[target].status2 & STATUS2_WRAPPED)
        return ACCURACY_ALWAYS_HITS;
    }
};

template <>
struct AbilityImpl<ABILITY_BIG_LEAVES>
    : is AbilityImpl<ABILITY_CHLOROPLAST>, is Merged<ABILITY_SOLAR_POWER, ABILITY_CHLOROPHYLL>, is Merged<ABILITY_HARVEST, ABILITY_LEAF_GUARD> {};

template <>
struct AbilityImpl<ABILITY_PRECISE_FIST> : is OnCrit<>, is OnModifyEffectChance<> {
    ON_CRIT { CHECK(IsIronFistBoosted(battler, move)) return 1; }
    ON_MODIFY_EFFECT_CHANCE {
        if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
    }
};

template <>
struct AbilityImpl<ABILITY_DEADEYE> : is OnAccuracy<>, is OnChooseDefensiveStat<> {
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
struct AbilityImpl<ABILITY_ARTILLERY> : is OnAccuracy<>, is OnMakeSpread {
    ON_ACCURACY { CHECK(IsMegaLauncherBoosted(battler, move)) return ACCURACY_HITS_IF_POSSIBLE; }
    ON_MAKE_SPREAD { return IsMegaLauncherBoosted(battler, move); }
};

template <>
struct AbilityImpl<ABILITY_ICE_DEW> : is LightningRodClone<TYPE_ICE> {};

template <>
struct AbilityImpl<ABILITY_SUN_WORSHIP> : is OnEntry {
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_POLLINATE> : is AteAbility<TYPE_BUG> {};

template <>
struct AbilityImpl<ABILITY_VOLCANO_RAGE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_COLD_REBOUND> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
        return FALSE;
    }
};

template <MoveEnum Move, int Power = 0>
struct SimpleEntryMove : is OnEntry {
    ON_ENTRY { return UseEntryMove(battler, ability, Move, Power); }
};
template <>
struct AbilityImpl<ABILITY_LOW_BLOW> : is SimpleEntryMove<MOVE_FEINT_ATTACK, 40> {};

template <>
struct AbilityImpl<ABILITY_SPECTRALIZE> : is AteAbility<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_SPECTRAL_SHROUD> : is AbilityImpl<ABILITY_SPECTRALIZE>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleStruct->ateBoost[battler])
        CHECK(moveType == TYPE_GHOST)
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

template <>
struct AbilityImpl<ABILITY_DISCIPLINE> : is RemovesStatusOnImmunity, is TauntImmune {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_CONFUSION) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_THUNDERCALL> : is OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
    }
};

template <>
struct AbilityImpl<ABILITY_MARINE_APEX> : is AbilityImpl<ABILITY_INFILTRATOR>, is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_MIGHTY_HORN> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hornBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_HARDENED_SHEATH> : is OnAttacker {
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
struct AbilityImpl<ABILITY_ARCTIC_FUR> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER { MUL(.65); }
};

template <>
struct AbilityImpl<ABILITY_LETHARGY> : is OnEntry, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_IRON_BARRAGE> : is AbilityImpl<ABILITY_MEGA_LAUNCHER>, is AbilityImpl<ABILITY_SIGHTING_SYSTEM> {};

template <>
struct AbilityImpl<ABILITY_STEEL_BARREL> : is AbilityImpl<ABILITY_ROCK_HEAD> {};

template <>
struct AbilityImpl<ABILITY_PYRO_SHELLS> : is OnAttacker {
    ON_ATTACKER {
        CHECK(IsMegaLauncherBoosted(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_FUNGAL_INFECTION> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED)
        CHECK(IsMoveMakingContact(move, battler))

        gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PARRY> : is OnDefender, is OnDefensiveMultiplier<>, is OverrideBreakable {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
        return FALSE;
    }
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
};

template <>
struct AbilityImpl<ABILITY_SCRAPYARD> : is OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LOOSE_QUILLS> : is AbilityImpl<ABILITY_SCRAPYARD> {};

template <>
struct AbilityImpl<ABILITY_TOXIC_DEBRIS> : is OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

        BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ROUNDHOUSE> : is OnAccuracy<>, is OnChooseDefensiveStat<> {
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
struct AbilityImpl<ABILITY_MINERALIZE> : is AteAbility<TYPE_ROCK> {};

template <>
struct AbilityImpl<ABILITY_LOOSE_ROCKS> : is OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

        BattleScriptCall(BattleScript_DefenderSetsStealthRock);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SPINNING_TOP> : is OnAttacker {
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
struct AbilityImpl<ABILITY_RETRIBUTION_BLOW> : is OnReactive {
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
struct AbilityImpl<ABILITY_FEARMONGER> : is AbilityImpl<ABILITY_INTIMIDATE>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
};

template <>
struct AbilityImpl<ABILITY_TOXIC_SPILL> : is OnEntry, is OnEndTurn, is OnExit {
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
struct AbilityImpl<ABILITY_DESERT_CLOAK> : is OnStatusImmune<ApplyOn::ALLY>, is SandImmune {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_STATUS1) CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_DRACONIZE> : is AteAbility<TYPE_DRAGON> {};

template <>
struct AbilityImpl<ABILITY_PRETTY_PRINCESS> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (!HasUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SELF_REPAIR> : is AbilityImpl<ABILITY_SELF_SUFFICIENT>, is AbilityImpl<ABILITY_NATURAL_CURE> {};

template <>
struct AbilityImpl<ABILITY_ELECTROMORPHOSIS> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ATOMIC_BURST> : is AbilityImpl<ABILITY_ELECTROMORPHOSIS>, is AbilityImpl<ABILITY_GALVANIZE> {};

template <int BoostType>
struct BoostedSwarmLike : is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_HELLBLAZE> : is BoostedSwarmLike<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_RIPTIDE> : is BoostedSwarmLike<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_FOREST_RAGE> : is BoostedSwarmLike<TYPE_GRASS> {};

template <>
struct AbilityImpl<ABILITY_PRIMAL_MAW> : is OnParentalBond {
    ON_PARENTAL_BOND { CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) return PARENTAL_BOND_PRIMAL_MAW; }
};

template <>
struct AbilityImpl<ABILITY_SWEEPING_EDGE> : is OnAccuracy<>, is OnMakeSpread {
    ON_ACCURACY { CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) return ACCURACY_HITS_IF_POSSIBLE; }
    ON_MAKE_SPREAD { return gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST; }
};

template <>
struct AbilityImpl<ABILITY_GIFTED_MIND> : is OnAccuracy<>, is OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    ON_ACCURACY { CHECK(IS_MOVE_STATUS(move)) return ACCURACY_HITS_IF_POSSIBLE; }
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_BUG || moveType == TYPE_GHOST || moveType == TYPE_DARK) *mod = 0;
    }
};

template <>
struct AbilityImpl<ABILITY_HYDRO_CIRCUIT> : is AbilityImpl<ABILITY_TRANSISTOR>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(moveType == TYPE_WATER)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_EQUINOX> : is OnChooseOffensiveStat {
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
struct AbilityImpl<ABILITY_ABSORBANT> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        CHECK_NOT(gStatuses3[target] & STATUS3_LEECHSEED)
        CHECK(gBattleMoves[move].effect == EFFECT_ABSORB || gBattleMoves[move].effect == EFFECT_DREAM_EATER)

        gStatuses3[target] |= battler;
        gStatuses3[target] |= STATUS3_LEECHSEED;
        BattleScriptCall(BattleScript_AbsorbantActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CLUELESS> : is AbilityImpl<ABILITY_CLOUD_NINE>, is Unsuppressable {};

template <int N>
struct NoDamageHits : is Persistent, is OnEntry, is Breakable {
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
struct AbilityImpl<ABILITY_CHEATING_DEATH> : is NoDamageHits<2>, is OverrideBreakable {};

template <>
struct AbilityImpl<ABILITY_CHEAP_TACTICS> : is SimpleEntryMove<MOVE_SCRATCH> {};

template <>
struct AbilityImpl<ABILITY_COWARD> : is OnEntry, is Persistent {
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        SetSingleUseAbilityCounter(battler, ability, TRUE);
        gRoundStructs[battler].protectedThisTurn = TRUE;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VOLT_RUSH> : is GaleWingsLike<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_DUNE_TERROR> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<>, is SandImmune {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GROUND) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
    }
};

template <>
struct AbilityImpl<ABILITY_INFERNAL_RAGE> : is OnRecoil, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_DUAL_WIELD> : is OnParentalBond {
    ON_PARENTAL_BOND {
        CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return PARENTAL_BOND_DUAL_WIELD;
    }
};

template <>
struct AbilityImpl<ABILITY_ELEMENTAL_CHARGE> : is OnAttacker {
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
struct AbilityImpl<ABILITY_AMBUSH> : is OnCrit<> {
    ON_CRIT { CHECK(gVolatileStructs[battler].isFirstTurn) return ALWAYS_CRIT; }
};

template <>
struct AbilityImpl<ABILITY_ATLAS> : is OnEntry {
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
struct AbilityImpl<ABILITY_RADIANCE> : is OnImmune<ApplyOn::ANY>, is OnAccuracy<> {
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
struct AbilityImpl<ABILITY_JAWS_OF_CARNAGE> : is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler)) if (gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST) BattleScriptCall(BattleScript_HandleJawsOfCarnageEffect);
        else BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ANGELS_WRATH> : is OnAttacker, is OnAccuracy<>, is OnTypeEffectiveness, is OnModifyEffectChance<>, is OnCanStatusType {
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
    : is AbilityImpl<ABILITY_COLOR_CHANGE>, is AbilityImpl<ABILITY_PROTEAN>, is OnDefensiveMultiplier<>, is OverrideBreakable {
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
struct AbilityImpl<ABILITY_SHOCKING_JAWS> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
};

template <>
struct AbilityImpl<ABILITY_FAE_HUNTER> : is TypeSlayer<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_GRAVITY_WELL> : is OnEntry {
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
struct AbilityImpl<ABILITY_EVAPORATE> : is OnAbsorb {
    ON_ABSORB { CHECK(moveType == TYPE_WATER) return ABSORB_RESULT_EVAPORATE; }
};

template <>
struct AbilityImpl<ABILITY_LUMBERJACK> : is TypeSlayer<TYPE_GRASS> {};

struct AbsorbUp2 {};
template <>
struct AbilityImpl<ABILITY_WELL_BAKED_BODY> : is AbsorbStatUp<TYPE_FIRE, STAT_DEF>, is AbsorbUp2 {};

template <>
struct AbilityImpl<ABILITY_FURNACE> : is OnEntry, is OnDefender {
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
struct AbilityImpl<ABILITY_ROCKY_PAYLOAD> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_EARTH_EATER> : is AbsorbHeal<TYPE_GROUND> {};

template <>
struct AbilityImpl<ABILITY_LINGERING_AROMA> : is AbilityImpl<ABILITY_MUMMY> {};

template <>
struct AbilityImpl<ABILITY_FAIRY_TALE> : is AddsType<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_RAGING_MOTH> : is OnParentalBond {
    ON_PARENTAL_BOND { CHECK(moveType == TYPE_FIRE) return PARENTAL_BOND_DUAL_WIELD; }
};

template <>
struct AbilityImpl<ABILITY_ADRENALINE_RUSH> : is MoxieClone<STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_ARCHMAGE> : is RandomizerBanned, is OnAttacker {
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
struct AbilityImpl<ABILITY_CRYOMANCY> : is OnModifyEffectChance<> {
    ON_MODIFY_EFFECT_CHANCE {
        if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
    }
};

template <>
struct AbilityImpl<ABILITY_PHANTOM_PAIN> : is OnTypeEffectiveness {
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_GHOST) CHECK(defType == TYPE_NORMAL) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PURGATORY> : is BoostedSwarmLike<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_EMANATE> : is AteAbility<TYPE_PSYCHIC> {};

template <>
struct AbilityImpl<ABILITY_KUNOICHI_BLADE> : is AbilityImpl<ABILITY_TECHNICIAN>, is AbilityImpl<ABILITY_SKILL_LINK> {};

template <>
struct AbilityImpl<ABILITY_MONKEY_BUSINESS> : is SimpleEntryMove<MOVE_TICKLE> {};

template <>
struct AbilityImpl<ABILITY_COMBAT_SPECIALIST> : is Merged<ABILITY_IRON_FIST, ABILITY_STRIKER> {};

template <>
struct AbilityImpl<ABILITY_JUNGLES_GUARD> : is AbilityImpl<ABILITY_FLOWER_VEIL> {};

template <>
struct AbilityImpl<ABILITY_HUNTERS_HORN> : is AbilityImpl<ABILITY_SOUL_EATER>, is AbilityImpl<ABILITY_MIGHTY_HORN> {};

template <>
struct AbilityImpl<ABILITY_PIXIE_POWER> : is AbilityImpl<ABILITY_FAIRY_AURA>, is OnAccuracy<> {
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_PLASMA_LAMP> : is OnOffensiveMultiplier<>, is OnAccuracy<> {
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
struct AbilityImpl<ABILITY_MAGMA_EATER> : is AbilityImpl<ABILITY_SOUL_EATER>, is AbilityImpl<ABILITY_MOLTEN_DOWN> {};

template <>
struct AbilityImpl<ABILITY_SUPER_HOT_GOO> : is Merged<ABILITY_GOOEY, ABILITY_FLAME_BODY> {};

template <>
struct AbilityImpl<ABILITY_NIKA> : is AbilityImpl<ABILITY_IRON_FIST> {};

template <>
struct AbilityImpl<ABILITY_ARCHER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].arrowBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPER_SLAMMER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hammerBased) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_INVERSE_ROOM> : is OnEntry {
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
struct AbilityImpl<ABILITY_FROST_BURN> : is OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
    }
};

template <>
struct AbilityImpl<ABILITY_ITCHY_DEFENSE> : is OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_GENERATOR> : is OnEntry, is OnTerrain, is OnExit, is Persistent {
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
struct AbilityImpl<ABILITY_MOON_SPIRIT> : is OnStab {
    ON_STAB { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; }
};

template <>
struct AbilityImpl<ABILITY_DUST_CLOUD> : is SimpleEntryMove<MOVE_SAND_ATTACK> {};

template <>
struct AbilityImpl<ABILITY_TIPPING_POINT> : is OnDefender {
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
struct AbilityImpl<ABILITY_BERSERKER_RAGE> : is AbilityImpl<ABILITY_TIPPING_POINT>, is AbilityImpl<ABILITY_RAMPAGE> {};

template <>
struct AbilityImpl<ABILITY_TRICKSTER> : is SimpleEntryMove<MOVE_DISABLE> {};

template <>
struct AbilityImpl<ABILITY_SAND_GUARD> : is OnImmune<>, is OnDefensiveMultiplier<>, is SandImmune {
    ON_IMMUNE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
        return blocksPriority(DELEGATE_IMMUNE);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_NATURAL_RECOVERY> : is Merged<ABILITY_NATURAL_CURE, ABILITY_REGENERATOR> {};

template <>
struct AbilityImpl<ABILITY_WIND_RIDER> : is OnEntry, is OnAbsorb {
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
struct AbilityImpl<ABILITY_SOOTHING_AROMA> : is OnEntry {
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
struct AbilityImpl<ABILITY_PRIM_AND_PROPER> : is AbilityImpl<ABILITY_WONDER_SKIN>, is AbilityImpl<ABILITY_CUTE_CHARM> {};

template <>
struct AbilityImpl<ABILITY_SUPER_STRAIN> : is OnRecoil, OnBattlerFaints<> {
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
struct AbilityImpl<ABILITY_ENLIGHTENED> : is AbilityImpl<ABILITY_EMANATE>, is AbilityImpl<ABILITY_INNER_FOCUS> {};

template <>
struct AbilityImpl<ABILITY_PEACEFUL_SLUMBER> : is AbilityImpl<ABILITY_SWEET_DREAMS>, is AbilityImpl<ABILITY_SELF_SUFFICIENT> {
    ON_END_TURN {
        if (!AbilityImpl<ABILITY_SWEET_DREAMS>::onEndTurn(DELEGATE_END_TURN)) return AbilityImpl<ABILITY_SELF_SUFFICIENT>::onEndTurn(DELEGATE_END_TURN);
        gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AFTERSHOCK> : is OnAttacker {
    ON_ATTACKER {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
    }
};

template <>
struct AbilityImpl<ABILITY_FREEZING_POINT> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanGetFrostbite(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRYO_PROFICIENCY> : is AbilityImpl<ABILITY_FREEZING_POINT> {
    static int CryoProficiencyHail(AbilityEnum ability, int battler, int attacker, MoveEnum move, int moveType) {
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
struct AbilityImpl<ABILITY_ARCANE_FORCE> : is AbilityImpl<ABILITY_MYSTIC_POWER>, is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
    }
};

template <>
struct AbilityImpl<ABILITY_DOOMBRINGER> : is SimpleEntryMove<MOVE_DOOM_DESIRE> {};

template <>
struct AbilityImpl<ABILITY_WISHMAKER> : is OnEntry, is Persistent {
    ON_ENTRY {
        int counter = GetSingleUseAbilityCounter(battler, ability);
        CHECK(counter < 3)
        CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

        SetSingleUseAbilityCounter(battler, ability, counter + 1);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_YUKI_ONNA> : is AbilityImpl<ABILITY_INTIMIDATE>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanInfatuate(battler, target))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPPRESS> : is SimpleEntryMove<MOVE_TORMENT> {};

template <>
struct AbilityImpl<ABILITY_REFRIGERATOR> : is AbilityImpl<ABILITY_FILTER>, is AbilityImpl<ABILITY_ILLUMINATE> {};

template <>
struct AbilityImpl<ABILITY_HEAVEN_ASUNDER> : is OnCrit<> {
    ON_CRIT {
        if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
        return 1;
    }
};

template <>
struct AbilityImpl<ABILITY_PURIFYING_WATERS> : is AbilityImpl<ABILITY_WATER_VEIL>, is AbilityImpl<ABILITY_HYDRATION> {};

template <>
struct AbilityImpl<ABILITY_SEABORNE> : is AbilityImpl<ABILITY_DRIZZLE>, is AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_HIGH_TIDE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_WATER)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_CHANGE_OF_HEART> : is SimpleEntryMove<MOVE_HEART_SWAP> {};

template <>
struct AbilityImpl<ABILITY_MYSTIC_BLADES> : is AbilityImpl<ABILITY_KEEN_EDGE>, is OnSwapSplit {
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL) CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return TRUE;
    }
};

struct NegateFrzSpatkDrop {};
template <>
struct AbilityImpl<ABILITY_DETERMINATION> : is OnOffensiveMultiplier<>, is NegateFrzSpatkDrop {
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_FERTILIZE> : is AteAbility<TYPE_GRASS> {};

struct InfatuatesAny {};
template <>
struct AbilityImpl<ABILITY_PURE_LOVE> : is OnDefender, is OnAttacker, is InfatuatesAny {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gBattleMons[target].status2 & STATUS2_INFATUATION)

        gBattleMoveDamage = -gHpDealt / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
    ON_DEFENDER { return AbilityImpl<ABILITY_CUTE_CHARM>::onEither(DELEGATE_DEFENDER); }
};

template <>
struct AbilityImpl<ABILITY_FIGHTER> : is SwarmLike<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_TELEKINETIC> : is SimpleEntryMove<MOVE_TELEKINESIS> {};

template <>
struct AbilityImpl<ABILITY_COMBUSTION> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_PONY_POWER> : is Merged<ABILITY_KEEN_EDGE, ABILITY_MYSTIC_BLADES> {};

template <>
struct AbilityImpl<ABILITY_POWDER_BURST> : is SimpleEntryMove<MOVE_POWDER> {};

template <>
struct AbilityImpl<ABILITY_RETRIEVER> : is OnExit {
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
struct AbilityImpl<ABILITY_MONSTER_MASH> : is SimpleEntryMove<MOVE_TRICK_OR_TREAT> {};

template <>
struct AbilityImpl<ABILITY_TWO_STEP> : is OnAttacker {
    ON_ATTACKER {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_SPITEFUL> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])

        BattleScriptCall(BattleScript_AbilitySpiteful);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FORTITUDE> : is OnDefender {
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
struct AbilityImpl<ABILITY_DEVOURER> : is AbilityImpl<ABILITY_PRIMAL_MAW>, is AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_PHANTOM_THIEF> : is SimpleEntryMove<MOVE_SPECTRAL_THIEF, 40> {};

template <>
struct AbilityImpl<ABILITY_EARLY_GRAVE> : is GaleWingsLike<TYPE_GHOST> {};

template <>
struct AbilityImpl<ABILITY_BASS_BOOSTED> : is Merged<ABILITY_PUNK_ROCK, ABILITY_AMPLIFIER> {};

template <>
struct AbilityImpl<ABILITY_FLAMING_JAWS> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    }
};

template <>
struct AbilityImpl<ABILITY_MONSTER_HUNTER> : is TypeSlayer<TYPE_DARK> {};

template <>
struct AbilityImpl<ABILITY_CROWNED_SWORD> : is AbilityImpl<ABILITY_INTREPID_SWORD>, is AbilityImpl<ABILITY_ANGER_POINT> {};

template <>
struct AbilityImpl<ABILITY_CROWNED_SHIELD> : is AbilityImpl<ABILITY_DAUNTLESS_SHIELD>, is AbilityImpl<ABILITY_STAMINA> {};

template <>
struct AbilityImpl<ABILITY_BERSERK_DNA> : is OnEntry {
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
struct AbilityImpl<ABILITY_CROWNED_KING> : is AbilityImpl<ABILITY_AS_ONE_ICE_RIDER>, is AbilityImpl<ABILITY_AS_ONE_SHADOW_RIDER> {
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
struct AbilityImpl<ABILITY_SNAP_TRAP_WHEN_HIT> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_PERMANENCE> : is OnEntry {
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); }
};

template <>
struct AbilityImpl<ABILITY_HUBRIS> : is AbilityImpl<ABILITY_GRIM_NEIGH> {};

template <>
struct AbilityImpl<ABILITY_COSMIC_DAZE> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
    }
};

template <>
struct AbilityImpl<ABILITY_MINDS_EYE> : is BlocksStatDrops<STAT_ACC>, is HitsGhost {};

template <>
struct AbilityImpl<ABILITY_BLOOD_PRICE> : is OnEndTurn, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_SPIKE_ARMOR> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBleed(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VOODOO_POWER> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IS_MOVE_SPECIAL(move))
        CHECK(CanBleed(attacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CHROME_COAT> : is OnDefensiveMultiplier<>, is OnStat<> {
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
struct AbilityImpl<ABILITY_WEB_SPINNER> : is SimpleEntryMove<MOVE_STRING_SHOT> {};

template <>
struct AbilityImpl<ABILITY_SHOWDOWN_MODE> : is OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
    }
};

template <>
struct AbilityImpl<ABILITY_SEED_SOWER> : is OnDefender, is AllowTerrainIfAirborne<TERRAIN_GRASSY> {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptCall(BattleScript_SeedSower);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_AIRBORNE> : is OnOffensiveMultiplier<ApplyOn::ALLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_PARROTING> : is AbilityImpl<ABILITY_SOUNDPROOF>, is OnCopyMove {
    ON_COPY_MOVE { CHECK(IsSoundMove(attacker, move)) return UseOutOfTurnAttack(battler, target, ability, move, 0); }
};

template <>
struct AbilityImpl<ABILITY_SALT_CIRCLE> : is OnEntry {
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
struct AbilityImpl<ABILITY_PURIFYING_SALT> : is OnDefensiveMultiplier<>, is RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GHOST) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

struct ParadoxBoostEffect : is OnStat<>, is OnEntry {
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
struct AbilityImpl<ABILITY_PROTOSYNTHESIS> : is ParadoxBoostEffect, is OnWeather {
    ON_ENTRY { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_WEATHER { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_QUARK_DRIVE> : is ParadoxBoostEffect, is OnTerrain {
    ON_ENTRY {
        return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    }
    ON_TERRAIN { return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_CALL); }
};

template <>
struct AbilityImpl<ABILITY_WIND_POWER> : is OnDefender {
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
struct AbilityImpl<ABILITY_IMPULSE> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
    }
};

template <>
struct AbilityImpl<ABILITY_TERMINAL_VELOCITY> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
    }
};

template <>
struct AbilityImpl<ABILITY_ANGER_SHELL> : is OnDefender {
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
struct AbilityImpl<ABILITY_EGOIST> : is OnReactive {
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
struct AbilityImpl<ABILITY_READIED_ACTION> : is OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
    }
};

template <>
struct AbilityImpl<ABILITY_DARK_GALE_WINGS> : is GaleWingsLike<TYPE_DARK> {};

template <>
struct AbilityImpl<ABILITY_GUILT_TRIP> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

        BattleScriptCall(BattleScript_GuiltTrip);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WATER_GALE_WINGS> : is GaleWingsLike<TYPE_WATER> {};

template <>
struct AbilityImpl<ABILITY_ZERO_TO_HERO> : is FormChangeAbility, is OnEntry, is OnExit {
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
struct AbilityImpl<ABILITY_COSTAR> : is OnEntry {
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
struct AbilityImpl<ABILITY_COMMANDER> : is FormChangeAbility, is OnBattlerFaints<ApplyOnTarget::ALLY_IS_TARGET>, is OnAccuracy<ApplyOnTarget::TARGET> {
    ON_BATTLER_FAINTS {
        CHECK(GetAbilityState(battler, ability))

        SetAbilityState(battler, ability, 0);
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
struct AbilityImpl<ABILITY_EJECT_PACK_ABILITY> : is Persistent {};

template <>
struct AbilityImpl<ABILITY_VENGEFUL_SPIRIT> : is AbilityImpl<ABILITY_HAUNTED_SPIRIT>, is AbilityImpl<ABILITY_VENGEANCE> {};

template <>
struct AbilityImpl<ABILITY_CUD_CHEW> : is OnEndTurn {
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
struct AbilityImpl<ABILITY_ARMOR_TAIL> : is AbilityImpl<ABILITY_QUEENLY_MAJESTY> {};

template <>
struct AbilityImpl<ABILITY_MIND_CRUSH> : is AbilityImpl<ABILITY_STRONG_JAW>, is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
    }
};

template <>
struct AbilityImpl<ABILITY_SUPREME_OVERLORD> : is OnEntry, is OnStat<> {
    ON_ENTRY {
        CHECK(gFaintedMonCount[GetBattlerSide(battler)])

        return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
    }
    ON_STAT {
        if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
    }
};

template <>
struct AbilityImpl<ABILITY_ILL_WILL> : is OnDefender {
    ON_DEFENDER {
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
    }
};

template <>
struct AbilityImpl<ABILITY_FIRE_SCALES> : is AbilityImpl<ABILITY_ICE_SCALES> {};

template <>
struct AbilityImpl<ABILITY_WATCH_YOUR_STEP> : is OnEntry {
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
struct AbilityImpl<ABILITY_RAPID_RESPONSE> : is OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
    }
};

template <>
struct AbilityImpl<ABILITY_DOUBLE_IRON_BARBS> : is OnDefender {
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
struct AbilityImpl<ABILITY_THERMAL_EXCHANGE> : is OnDefender, is RemovesStatusOnImmunity {
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
struct AbilityImpl<ABILITY_GOOD_AS_GOLD> : is OnImmune<> {
    ON_IMMUNE {
        CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHARING_IS_CARING> : is OnReactive {
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
struct AbilityImpl<ABILITY_PERMAFROST_CLONE> : is AbilityImpl<ABILITY_PERMAFROST> {};

template <>
struct AbilityImpl<ABILITY_GALLANTRY> : is NoDamageHits<1> {};

template <>
struct AbilityImpl<ABILITY_ORICHALCUM_PULSE> : is AbilityImpl<ABILITY_DROUGHT>, is OnStat<> {
    ON_STAT {
        if (statId != STAT_ATK) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
    }
};

template <>
struct AbilityImpl<ABILITY_SUN_BASKING> : is OnImmune<>, is OnDefensiveMultiplier<> {
    ON_IMMUNE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
        return blocksPriority(DELEGATE_IMMUNE);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_WINGED_KING> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
    }
};

template <>
struct AbilityImpl<ABILITY_HADRON_ENGINE> : is AbilityImpl<ABILITY_ELECTRIC_SURGE>, is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_SERPENT> : is AbilityImpl<ABILITY_WINGED_KING> {};

template <>
struct AbilityImpl<ABILITY_SWEEPING_EDGE_PLUS> : is AbilityImpl<ABILITY_KEEN_EDGE>, is AbilityImpl<ABILITY_SWEEPING_EDGE> {};

template <>
struct AbilityImpl<ABILITY_CELESTIAL_BLESSING> : is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerTerrainAffected(battler, STATUS_FIELD_MISTY_TERRAIN))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 12;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_SelfSufficientActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MINION_CONTROL> : is OnParentalBond {
    ON_PARENTAL_BOND { return PARENTAL_BOND_MINION_CONTROL; }
};

template <>
struct AbilityImpl<ABILITY_MOLTEN_BLADES> : is AbilityImpl<ABILITY_KEEN_EDGE>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    }
};

template <>
struct AbilityImpl<ABILITY_HAUNTING_FRENZY> : is AbilityImpl<ABILITY_ADRENALINE_RUSH>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    }
};

template <>
struct AbilityImpl<ABILITY_NOISE_CANCEL> : is AbilityImpl<ABILITY_SOUNDPROOF> {
    ApplyOn onImmuneFor() const override { return ApplyOn::ALLY; }
};

template <>
struct AbilityImpl<ABILITY_RADIO_JAM> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeDisabled(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
    }
};

template <>
struct AbilityImpl<ABILITY_OLE> : is OnAccuracy<ApplyOnTarget::TARGET> {
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
struct AbilityImpl<ABILITY_MALICIOUS> : is AbilityImpl<ABILITY_INTIMIDATE> {};

template <>
struct AbilityImpl<ABILITY_DEAD_POWER> : is OnAttacker, is OnStat<> {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gBattleMons[target].status2 & STATUS2_CURSED)
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_CURSE);
    }
    ON_STAT {
        if (statId == STAT_ATK) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_BRAWLING_WYVERN> : is AbilityImpl<ABILITY_NO_GUARD>, is OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_PUNCH) CHECK(IS_MOVE_TYPE(move, TYPE_DRAGON)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_JUNSHI_SANDA> : is OnModifyMoveFlags {
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
struct AbilityImpl<ABILITY_MYTHICAL_ARROWS> : is AbilityImpl<ABILITY_ARCHER>, is OnSwapSplit {
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL) CHECK(gBattleMoves[move].arrowBased);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LAWNMOWER> : is OnEntry {
    ON_ENTRY {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FLOURISH> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_DESERT_SPIRIT> : is AbilityImpl<ABILITY_SAND_STREAM>, is OnAfterTypeEffectiveness<> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
            *mod = UQ_4_12(1.0);
        }
    }
};

template <>
struct AbilityImpl<ABILITY_CONTEMPT> : is AbilityImpl<ABILITY_UNAWARE> {};

template <>
struct AbilityImpl<ABILITY_AERIALIST> : is Merged<ABILITY_LEVITATE, ABILITY_FLOCK> {};

template <>
struct AbilityImpl<ABILITY_TERA_SHELL> : is Breakable, is OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_TOXIC_CHAIN> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

template <>
struct AbilityImpl<ABILITY_PARASITIC_SPORES> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

        gVolatileStructs[battler].parasiticSpores = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
    }
};

template <MoveEffectEnum Effect>
struct PoisonPuppeteerLike : is OnBattlerFaints<ApplyOnTarget::ANY>, is SetStateOnEffect<Effect>, is OnReactive {
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
struct AbilityImpl<ABILITY_POISON_PUPPETEER> : is PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
    }
};

template <>
struct AbilityImpl<ABILITY_ENTRANCE> : is PoisonPuppeteerLike<MOVE_EFFECT_CONFUSION> {
    ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); }
};

template <>
struct AbilityImpl<ABILITY_REJECTION> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gFieldTimers.quashTimer)

        gFieldTimers.quashTimer = QUASH_DURATION;
        gFieldTimers.started.quash = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
    }
};

template <>
struct AbilityImpl<ABILITY_APPLE_ENLIGHTENMENT> : is AbilityImpl<ABILITY_FUR_COAT>, is AbilityImpl<ABILITY_MAGIC_GUARD> {};

template <>
struct AbilityImpl<ABILITY_BALLOON_BOMBER> : is Merged<ABILITY_AFTERMATH, ABILITY_INFLATABLE> {};

template <>
struct AbilityImpl<ABILITY_FLAMING_MAW> : is AbilityImpl<ABILITY_FLAMING_JAWS>, is AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_DEMOLITIONIST> : is AbilityImpl<ABILITY_READIED_ACTION>, is OnInfiltrate, is OnAttacker {
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
struct AbilityImpl<ABILITY_ROCKHARD_WILL> : is SwarmLike<TYPE_ROCK> {};
template <>
struct AbilityImpl<ABILITY_FRAGRANT_DAZE> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeConfused(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_CONFUSION, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LOW_VISIBILITY> : is OnEntry {
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
struct AbilityImpl<ABILITY_OLD_MARINER> : is AbilityImpl<ABILITY_SEAWEED>, is AbilityImpl<ABILITY_AMPHIBIOUS> {};

template <>
struct AbilityImpl<ABILITY_ECTOPLASM> : is OnStat<> {
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_BEAUTIFUL_MUSIC> : is OnAttacker, is InfatuatesAny {
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
struct AbilityImpl<ABILITY_GREATER_SPIRIT> : is OnEntry {
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RESONANCE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 50)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_ETHEREAL_RUSH> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CUTE_ANTECEDENCE> : is GaleWingsLike<TYPE_FAIRY> {};

template <>
struct AbilityImpl<ABILITY_RECURRING_NIGHTMARE> : is OnRevive {
    ON_REVIVE { CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) return B_MSG_FADE_OUT; }
};

template <>
struct AbilityImpl<ABILITY_MENACING_SITUATION> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK_NOT(gVolatileStructs[opponent].fear)
        CHECK(Random() % 100 < 30)

        gStackBattler1 = battler;
        gStackBattler2 = opponent;
        BattleScriptCall(BattleScript_AbilitySetFear);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SHINY_LIGHTNING> : is OnAccuracy<> {
    ON_ACCURACY {
        if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_TERRIFY> : is AbilityImpl<ABILITY_INTIMIDATE> {};

template <>
struct AbilityImpl<ABILITY_ICE_DOWNFALL> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_LAST_STAND> : is Breakable, is OnStat<> {
    ON_STAT {
        if (statId == STAT_DEF || statId == STAT_SPDEF)
            *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
    }
};

template <>
struct AbilityImpl<ABILITY_PYROCLASTIC_FLOW> : Merged<ABILITY_MOLTEN_DOWN, ABILITY_CORROSION> {};

template <>
struct AbilityImpl<ABILITY_BLOOD_BATH> : is PoisonPuppeteerLike<MOVE_EFFECT_BLEED>, is RemovesStatusOnImmunity {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_BLEED)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BATTLE_AURA> : is OnCrit<ApplyOnTarget::ANY> {
    ON_CRIT { return 2; }
};

template <>
struct AbilityImpl<ABILITY_BLOODLUST> : is AbilityImpl<ABILITY_BLOOD_BATH>, is AbilityImpl<ABILITY_SOUL_EATER> {
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
struct AbilityImpl<ABILITY_PIERCING_SOLO> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_RHYTHMIC> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); }
};

template <>
struct AbilityImpl<ABILITY_CHUNKY_BASS_LINE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(IsSoundMove(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
    }
};

template <>
struct AbilityImpl<ABILITY_DUAL_HAMMER> : is OnParentalBond {
    ON_PARENTAL_BOND { CHECK(gBattleMoves[move].hammerBased) return PARENTAL_BOND_DUAL_WIELD; }
};

template <>
struct AbilityImpl<ABILITY_DENTING_BLOWS> : is OnAttacker {
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
struct AbilityImpl<ABILITY_ICE_COLD_HUNTER> : is HailImmune, is OnParentalBond {
    ON_PARENTAL_BOND { CHECK(moveType == TYPE_ICE) CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) return PARENTAL_BOND_ICE_COLD_HUNTER; }
};

template <>
struct AbilityImpl<ABILITY_SOUL_CRUSHER> : is OnOffensiveMultiplier<>, is OnChooseDefensiveStat<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hammerBased) MUL(1.1);
    }
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(gBattleMoves[move].hammerBased)
        return STAT_SPDEF;
    }
};

template <>
struct AbilityImpl<ABILITY_ARC_FLASH> : is OnAttacker, is OnDefender {
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
struct AbilityImpl<ABILITY_UNICORN> : is AbilityImpl<ABILITY_MIGHTY_HORN>, is AbilityImpl<ABILITY_PIXILATE> {};

template <>
struct AbilityImpl<ABILITY_ON_THE_PROWL> : is OnEntry {
    ON_ENTRY {
        gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
    }
};

template <>
struct AbilityImpl<ABILITY_PRETENTIOUS> : is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(gVolatileStructs[battler].critBoost < 3);
        gVolatileStructs[battler].critBoost++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
        BattleScriptCall(BattleScript_AbilityBoostsCrit);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_VENOBLAZE_PINCERS> : is OnAttacker, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_ETERNAL_BLESSING> : is AbilityImpl<ABILITY_CELESTIAL_BLESSING>, is AbilityImpl<ABILITY_REGENERATOR> {};

template <>
struct AbilityImpl<ABILITY_RIPEN> {};
template <>
struct AbilityImpl<ABILITY_SUGAR_RUSH> : is AbilityImpl<ABILITY_UNBURDEN>, is AbilityImpl<ABILITY_RIPEN> {};

template <>
struct AbilityImpl<ABILITY_PEACEFUL_REST> : is OnEndTurn {
    ON_END_TURN {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        BattleScriptPushCursorAndCallback(BattleScript_RainDishActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WHITE_NOISE> : is AbilityImpl<ABILITY_PEACEFUL_REST>, is AbilityImpl<ABILITY_STATIC> {};

template <>
struct AbilityImpl<ABILITY_SMOKEY_MANEUVERS> : is OnAccuracy<ApplyOnTarget::TARGET> {
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_METAL> : LiquidVoiceClone<TYPE_STEEL> {};

template <>
struct AbilityImpl<ABILITY_POWER_EDGE> : is AbilityImpl<ABILITY_KEEN_EDGE>, is OnChooseDefensiveStat<> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) return STAT_SPDEF; }
};

template <>
struct AbilityImpl<ABILITY_SUPERCONDUCTOR> : is OnOffensiveMultiplier<>, is OnMoveType {
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
struct AbilityImpl<ABILITY_ULTRA_INSTINCT> : is OnDefender, is OnDefensiveMultiplier<>, is OverrideBreakable {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
        return FALSE;
    }
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
};

template <>
struct AbilityImpl<ABILITY_UNLOCKED_POTENTIAL> : is AbilityImpl<ABILITY_BERSERK>, is AbilityImpl<ABILITY_INNER_FOCUS> {};

template <>
struct AbilityImpl<ABILITY_HIGHER_RANK> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_FUNERAL_PYRE> : is OnEntry, is OnEndTurn {
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
struct AbilityImpl<ABILITY_FLAME_BUBBLE> : is AbilityImpl<ABILITY_WATER_BUBBLE>, is AbilityImpl<ABILITY_FLAMING_SOUL> {};

template <>
struct AbilityImpl<ABILITY_ELEMENTAL_VORTEX> : is Merged<ABILITY_WATER_ABSORB, ABILITY_FLASH_FIRE> {};

template <>
struct AbilityImpl<ABILITY_SNOWY_WRATH> : is AbilityImpl<ABILITY_SNOW_WARNING>, is AbilityImpl<ABILITY_CRYOMANCY> {};

template <>
struct AbilityImpl<ABILITY_PATTERN_CHANGE> : is AbilityImpl<ABILITY_SHED_SKIN>, is AbilityImpl<ABILITY_PROTEAN> {};

template <>
struct AbilityImpl<ABILITY_NO_TURNING_BACK> : is OnDefender {
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
struct AbilityImpl<ABILITY_FLAMMABLE_COAT> : is FormChangeAbility, is OnDefender, is OnBeforeAttack<> {
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
struct AbilityImpl<ABILITY_DRACO_MORALE> : is SimpleEntryMove<MOVE_DRAGON_CHEER> {};

template <>
struct AbilityImpl<ABILITY_BAD_OMEN> : is OnDefensiveMultiplier<>, is ForcesMinRolls {
    ON_DEFENSIVE_MULTIPLIER {
        if (isCrit) MUL(.25);
    }
};

template <>
struct AbilityImpl<ABILITY_MOSH_PIT> : is OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST)
            MUL(1.25);
        else
            MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_BLOOD_STAIN> : is OnEither, is OnEntry, is Unsuppressable, is RemovesStatusOnImmunity {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK_NOT(IsPersistentOrUnsuppressable(GetBattlerAbility(opponent)))
        CHECK_NOT(HasAbilityIgnoringSuppression(opponent, ability))
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
struct AbilityImpl<ABILITY_BLOOD_STIGMA> : is OnOffensiveMultiplier<>, is RemovesStatusOnImmunity, is Unsuppressable {
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SLIPSTREAM> : is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPEED] += 20; }
};

template <>
struct AbilityImpl<ABILITY_MAXIMUM_ACCELERATION> : is AbilityImpl<ABILITY_SLIPSTREAM>, is AbilityImpl<ABILITY_SPEED_BOOST> {};

template <>
struct AbilityImpl<ABILITY_SIDEWINDER> : is AbilityImpl<ABILITY_COIL_UP>, is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
        gStatuses4[battler] |= STATUS4_COILED;
        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PETRIFY> : is AbilityImpl<ABILITY_INTIMIDATE> {
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
struct AbilityImpl<ABILITY_FLUFFIEST> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(2.0);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

template <>
struct AbilityImpl<ABILITY_WAY_OF_PRECISION> : is AbilityImpl<ABILITY_INNER_FOCUS>, is AbilityImpl<ABILITY_PRECISE_FIST> {};

template <>
struct AbilityImpl<ABILITY_WAY_OF_SWIFTNESS> : is AbilityImpl<ABILITY_PRETENTIOUS>, is AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_ATOMIC_PUNCH> : is AbilityImpl<ABILITY_IRON_FIST> {
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_IRON_FIST>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        if (moveType == TYPE_STEEL) MUL(1.3);
    }
};

template <>
struct AbilityImpl<ABILITY_IRON_GIANT> : is AbilityImpl<ABILITY_HEATPROOF>, is AbilityImpl<ABILITY_JUGGERNAUT> {};

template <>
struct AbilityImpl<ABILITY_MASTER_HAND> : is AbilityImpl<ABILITY_RAMPAGE>, is AbilityImpl<ABILITY_MEGA_LAUNCHER> {};

template <>
struct AbilityImpl<ABILITY_UNSEEN_FIST> {};

template <>
struct AbilityImpl<ABILITY_FINAL_BLOW> : is AbilityImpl<ABILITY_FATAL_PRECISION>, is AbilityImpl<ABILITY_UNSEEN_FIST> {};

template <>
struct AbilityImpl<ABILITY_HOSPITALITY> : is OnEntry {
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
struct AbilityImpl<ABILITY_BUTTER_UP> : is Merged<ABILITY_HOSPITALITY, ABILITY_SOOTHING_AROMA> {};

template <>
struct AbilityImpl<ABILITY_VITALITY_STRIKE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        CHECK(IsIronFistBoosted(battler, move))

        gBattleMoveDamage = -gHpDealt / 10;
        if (!gBattleMoveDamage) gBattleMoveDamage = -1;
        BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_HUGE_WINGS> : is Merged<ABILITY_GIANT_WINGS, ABILITY_LEVITATE> {};

template <>
struct AbilityImpl<ABILITY_SWORD_OF_DAMNATION> : is AbilityImpl<ABILITY_SWORD_OF_RUIN>, is AbilityImpl<ABILITY_UNAWARE> {};

template <>
struct AbilityImpl<ABILITY_RESTRAINING_ORDER> : is OnDefender {
    ON_DEFENDER {
        CHECK(GetAbilityState(battler, ability) == RESTRAINING_ORDER_NOT_TRIGGERED)
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
        CHECK(CountUsablePartyMons(battler))

        SetAbilityState(battler, ability, RESTRAINING_ORDER_ACTIVATING);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_ASSASSINS_TOOLS> : is OnAttacker {
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
struct AbilityImpl<ABILITY_FROSTMAW> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanGetFrostbite(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
    }
};

template <>
struct AbilityImpl<ABILITY_PATCHWORK> : is AbilityImpl<ABILITY_DISGUISE>, is OnDefender {
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
struct AbilityImpl<ABILITY_BLIND_RAGE> : is AbilityImpl<ABILITY_MOLD_BREAKER>, is AbilityImpl<ABILITY_SCRAPPY> {};

template <>
struct AbilityImpl<ABILITY_APEX_PREDATOR> : is AbilityImpl<ABILITY_SOUL_EATER>, is AbilityImpl<ABILITY_TOUGH_CLAWS> {};

template <>
struct AbilityImpl<ABILITY_DRAGONS_RITUAL> : is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
        BattleScriptCall(BattleScript_DragonsRitual);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_PINNACLE_BLADE> : is OnInfiltrate, is OnAttacker {
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
struct AbilityImpl<ABILITY_ENERGIZED> : is AbilityImpl<ABILITY_GENERATOR>, is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK(moveType == TYPE_ELECTRIC);
        SetOncePerTurnAbilityCounter(battler, ability, TRUE);
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_COLOR_SPECTRUM> : is OnEndTurn, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_STEEL_BEETLE> : is AbilityImpl<ABILITY_RAGING_BOXER>, is AbilityImpl<ABILITY_POLLINATE> {};

template <>
struct AbilityImpl<ABILITY_FROM_THE_SHADOWS> : is OnAttacker {
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
struct AbilityImpl<ABILITY_RAGE_POINT> : OnDefender, is OnOffensiveMultiplier<>, is NegateBurnAtkDrop, is NegateFrzSpatkDrop {
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
struct AbilityImpl<ABILITY_HOT_COALS> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

        gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
    }
};

template <>
struct AbilityImpl<ABILITY_TERASTAL_TREASURE> : is OnDefensiveMultiplier<>, is OnStat<> {
    ON_DEFENSIVE_MULTIPLIER { MUL(.6); }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .8;
    }
};

template <>
struct AbilityImpl<ABILITY_SHOCKING_MAW> : is AbilityImpl<ABILITY_SHOCKING_JAWS>, is AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_GLEAM_EYES> : is Merged<ABILITY_INTIMIDATE, ABILITY_FRISK> {};

template <>
struct AbilityImpl<ABILITY_ROUSED_FANGS> : is AbilityImpl<ABILITY_STRONG_JAW>, is AbilityImpl<ABILITY_MIND_CRUSH> {};

template <>
struct AbilityImpl<ABILITY_DREAM_STATE> : is AbilityImpl<ABILITY_BATTLE_ARMOR> {};

template <>
struct AbilityImpl<ABILITY_DREAM_WHIMSY> : is SimpleEntryMove<MOVE_YAWN> {};

template <>
struct AbilityImpl<ABILITY_LUNAR_AFFINITY> : is OnCopyMove {
    ON_COPY_MOVE { CHECK(gBattleMoves[move].lunar) return UseOutOfTurnAttack(battler, target, ability, move, 0); }
};

template <>
struct AbilityImpl<ABILITY_FLAME_SHIELD> : is AbilityImpl<ABILITY_FILTER> {};

template <>
struct AbilityImpl<ABILITY_AQUATIC_DWELLER> : is AbilityImpl<ABILITY_AQUATIC>, is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) MUL(1.5);
    }
};

template <>
struct AbilityImpl<ABILITY_APPLE_PIE> : is AbilityImpl<ABILITY_SELF_SUFFICIENT> {};

template <>
struct AbilityImpl<ABILITY_HOVER> : is GroundImmune, AddsType<TYPE_PSYCHIC> {};

template <>
struct AbilityImpl<ABILITY_DEPRAVITY> : is AbilityImpl<ABILITY_MERCILESS>, is AbilityImpl<ABILITY_OVERCHARGE> {};

template <>
struct AbilityImpl<ABILITY_WILDFIRE> : is SimpleEntryMove<MOVE_FIRE_SPIN> {};

template <>
struct AbilityImpl<ABILITY_JUMP_SCARE> : is OnEntry, is Persistent {
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
    }
};

template <>
struct AbilityImpl<ABILITY_TAR_TOSS> : is SimpleEntryMove<MOVE_TAR_SHOT> {};

template <>
struct AbilityImpl<ABILITY_STUN_SHOCK> : is OnAttacker {
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
struct AbilityImpl<ABILITY_RAGING_GODDESS> : is AbilityImpl<ABILITY_RAMPAGE>, is AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {};

template <>
struct AbilityImpl<ABILITY_WHIPLASH> : is OnAttacker {
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
struct AbilityImpl<ABILITY_SUPERSWEET_SYRUP> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_EMBARGO)
        CHECK(gBattleMons[attacker].item)

        gVolatileStructs[attacker].embargoTimer = 2;
        gStatuses3[attacker] |= STATUS3_EMBARGO;
        gLastUsedItem = gBattleMons[attacker].item;
        BattleScriptCall(BattleScript_AnnounceAttackerItemDisabled);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_TRASH_HEAP> : is AbilityImpl<ABILITY_TOXIC_SPILL>, is AbilityImpl<ABILITY_CORROSION> {};

template <>
struct AbilityImpl<ABILITY_SLUDGY_MIX> : is AbilityImpl<ABILITY_INTOXICATE>, is AbilityImpl<ABILITY_PUNK_ROCK> {};

template <>
struct AbilityImpl<ABILITY_OVERWATCH> : is AbilityImpl<ABILITY_ON_THE_PROWL>, is AbilityImpl<ABILITY_STAKEOUT> {};

template <>
struct AbilityImpl<ABILITY_WIND_RAGE> : is AbilityImpl<ABILITY_GIANT_WINGS>, is SimpleEntryMove<MOVE_DEFOG> {};

template <>
struct AbilityImpl<ABILITY_VICTORY_BOMB> : is OnDefender, is OnMoveType {
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
struct AbilityImpl<ABILITY_RAZOR_SHARP> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gIsCriticalHit)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_TO_THE_BONE> : is AbilityImpl<ABILITY_RAZOR_SHARP>, is AbilityImpl<ABILITY_SNIPER> {};

template <>
struct AbilityImpl<ABILITY_BLADE_DANCE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_APE_SHIFT> : is FormChangeAbility, is OnEntry, is OnEndTurn, is OnDefender, is OnCrit<> {
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
struct AbilityImpl<ABILITY_KNOW_YOUR_PLACE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gVolatileStructs[target].dazed)
        CHECK(IsMoveMakingContact(move, battler))

        gVolatileStructs[target].dazed = 5;
        BattleScriptCall(BattleScript_TargetDazed);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_DEEP_CUTS> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_LIFE_STEAL> : is OnEndTurn {
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
struct AbilityImpl<ABILITY_RUDE_AWAKENING> : is RemovesStatusOnImmunity {
    ON_STATUS_IMMUNE { CHECK(status & CHECK_SLEEP) CHECK(GetAbilityState(battler, ability)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_TERAFORM_ZERO> : is AbilityImpl<ABILITY_TERA_SHELL>, is OnEntry {
    ON_ENTRY {
        CHECK(!GetSingleUseAbilityCounter(battler, ability));
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
        BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SET_ABLAZE> : is PoisonPuppeteerLike<MOVE_EFFECT_BURN> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    }
};

template <>
struct AbilityImpl<ABILITY_BREAKWATER> : is AbilityImpl<ABILITY_STALL>, is AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_MAGICAL_FISTS> : is AbilityImpl<ABILITY_IRON_FIST>, is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT {
        if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
    }
};

template <>
struct AbilityImpl<ABILITY_CUTTHROAT> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
    }
};

template <>
struct AbilityImpl<ABILITY_SAND_BENDER> : is AbilityImpl<ABILITY_SAND_STREAM>, is AbilityImpl<ABILITY_SAND_FORCE> {};

template <>
struct AbilityImpl<ABILITY_SAND_PIT> : is SimpleEntryMove<MOVE_SAND_TOMB, 20> {};

template <>
struct AbilityImpl<ABILITY_DESOLATE_SUN> : is RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_DAYBREAK> : is OnEither {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ENERGY_SIPHON> : is OnAttacker {
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
struct AbilityImpl<ABILITY_RESERVOIR> : is Merged<ABILITY_WATER_ABSORB, ABILITY_STORM_DRAIN> {};

static int NeurotoxinCondition(int battler, int target) {
    return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
}
template <>
struct AbilityImpl<ABILITY_NEUROTOXIN> : is PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); }
};

template <>
struct AbilityImpl<ABILITY_ENERGIZED_HORNS> : is AbilityImpl<ABILITY_MIGHTY_HORN> {
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL) CHECK(gBattleMoves[move].hornBased);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_SPIDER_LAIR_UPGRADE> : is OnEntry {
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
struct AbilityImpl<ABILITY_CRUST_COAT> : is AbilityImpl<ABILITY_BATTLE_ARMOR> {};

template <>
struct AbilityImpl<ABILITY_PUFFY> : is AbilityImpl<ABILITY_FLUFFY> {};

template <>
struct AbilityImpl<ABILITY_BALLOON_BLITZ> : is AbilityImpl<ABILITY_INFLATABLE>, is AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {};

template <>
struct AbilityImpl<ABILITY_STRIKER_PIXILATE> : is AbilityImpl<ABILITY_STRIKER>, is AbilityImpl<ABILITY_PIXILATE> {};

// 2.6
template <>
struct AbilityImpl<ABILITY_DOOM_BLAST> : is OnRecoil, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_BRUTEFORCE> : is AbilityImpl<ABILITY_RECKLESS>, is AbilityImpl<ABILITY_ROCK_HEAD> {};

template <>
struct AbilityImpl<ABILITY_FARADAY_CAGE> : is AbilityImpl<ABILITY_SHELL_ARMOR>, is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_ACIDIC_SLIME> : is AbilityImpl<ABILITY_CORROSION>, is OnStab {
    ON_STAB { return moveType == TYPE_WATER; }
};

template <>
struct AbilityImpl<ABILITY_ROSE_GARDEN> : is OnEntry {
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
struct AbilityImpl<ABILITY_QIGONG> : is AbilityImpl<ABILITY_RAMPAGE>, is AbilityImpl<ABILITY_FIGHT_SPIRIT>, is OnAccuracy<> {
    ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
};

template <>
struct AbilityImpl<ABILITY_CONJOURER_OF_DECEIT> : is AbilityImpl<ABILITY_MAGIC_GUARD>, is AbilityImpl<ABILITY_MAGIC_BOUNCE> {};

template <>
struct AbilityImpl<ABILITY_DEEP_FREEZE> : is OnOffensiveMultiplier<>, is OnDefensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_ICE) MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
};

template <>
struct AbilityImpl<ABILITY_SOUL_DEVOURER> : is AbilityImpl<ABILITY_SOUL_EATER>, is AbilityImpl<ABILITY_PHANTOM_PAIN> {};

template <>
struct AbilityImpl<ABILITY_CHAMPIONS_ENTRANCE> : is Merged<ABILITY_INTIMIDATE, ABILITY_VIOLENT_RUSH> {};

template <>
struct AbilityImpl<ABILITY_PRESTO> : is OnPriority {
    ON_PRIORITY { CHECK(BATTLER_MAX_HP(battler)) CHECK(IsSoundMove(battler, move)) return 1; }
};

template <>
struct AbilityImpl<ABILITY_SAMBA> : is AbilityImpl<ABILITY_STRIKER>, is AbilityImpl<ABILITY_DANCER> {};

template <>
struct AbilityImpl<ABILITY_GLADIATOR> : is BoostedSwarmLike<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_FORSAKEN_HEART> : is OnBattlerFaints<ApplyOnTarget::ANY> {
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_RELENTLESS> : is AbilityImpl<ABILITY_EXPLOIT_WEAKNESS>, is AbilityImpl<ABILITY_MERCILESS> {};

template <>
struct AbilityImpl<ABILITY_SOOTHSAYER> : is OnEntry, is OnEndTurn, is OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, is Persistent {
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
struct AbilityImpl<ABILITY_CORRUPTED_MIND> : is RandomizerBanned, is OnTypeEffectiveness, is OnModifyEffectChance<> {
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
struct AbilityImpl<ABILITY_FLAME_COAT> : is OnEntry, is OnEndTurn {
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
struct AbilityImpl<ABILITY_UNOWN_POWER> : is RandomizerBanned, is AbilityImpl<ABILITY_MYSTIC_POWER>, is OnAfterTypeEffectiveness<> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
    }
};

template <>
struct AbilityImpl<ABILITY_SUPER_SCOPE> : is AbilityImpl<ABILITY_MEGA_LAUNCHER>, is AbilityImpl<ABILITY_ARTILLERY> {};

template <>
struct AbilityImpl<ABILITY_VENOM_CROWN> : is AbilityImpl<ABILITY_POISON_POINT>, is AbilityImpl<ABILITY_MIGHTY_HORN>, is RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_BLIGHT_SCALE> : is AbilityImpl<ABILITY_POISON_POINT>, is AbilityImpl<ABILITY_MULTISCALE>, is RandomizerBanned {};

template <>
struct AbilityImpl<ABILITY_GUNMAN> : is AbilityImpl<ABILITY_MEGA_LAUNCHER>, is OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_MEGA_LAUNCHER) CHECK(IS_MOVE_STATUS(move)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_CARETAKER> : is OnEndTurn, is AbilityImpl<ABILITY_FRIEND_GUARD> {
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
struct AbilityImpl<ABILITY_POSEIDONS_DOMINION> : is SimpleEntryMove<MOVE_WHIRLPOOL> {};

template <>
struct AbilityImpl<ABILITY_DUAL_SHADOW> : is AbilityImpl<ABILITY_HUNGER_SWITCH>, is OnRecoil, is OnOffensiveMultiplier<> {
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
struct AbilityImpl<ABILITY_LULLABY> : is OnAccuracy<> {
    ON_ACCURACY {
        CHECK(move == MOVE_SING);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRYO_ARCHITECT> : is OnEndTurn, is OnDefender {
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
struct AbilityImpl<ABILITY_GLACIAL_RAGE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_ICE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_IMMOVABLE_OBJECT> : is AbilityImpl<ABILITY_MAGIC_GUARD>, is AbilityImpl<ABILITY_STURDY> {};

template <>
struct AbilityImpl<ABILITY_FRENZIED_PHANTOM> : is AbilityImpl<ABILITY_SHADOW_TAG>, is AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {};

template <>
struct AbilityImpl<ABILITY_DNA_SCRAMBLE> : is FormChangeAbility, is OnBeforeAttack<> {
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
struct AbilityImpl<ABILITY_METALLIC_JAWS> : is AbilityImpl<ABILITY_METALLIC>, is AbilityImpl<ABILITY_PRIMAL_MAW> {};

template <>
struct AbilityImpl<ABILITY_CALCULATIVE> : is Merged<ABILITY_ANALYTIC, ABILITY_NEUROFORCE> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT> : is RaiseStatOnEntry<STAT_SPEED> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT_HEARTHFLAME> : is AbilityImpl<ABILITY_INTREPID_SWORD> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT_CORNERSTONE> : is AbilityImpl<ABILITY_DAUNTLESS_SHIELD> {};

template <>
struct AbilityImpl<ABILITY_EMBODY_ASPECT_WELLSPRING> : is RaiseStatOnEntry<STAT_SPDEF> {};

template <>
struct AbilityImpl<ABILITY_ROCKHARD_SHAFT> : is BoostedSwarmLike<TYPE_ROCK> {};

template <>
struct AbilityImpl<ABILITY_HUNTERS_MARK> : is AbilityImpl<ABILITY_DEADEYE>, is AbilityImpl<ABILITY_AMBUSH> {};

template <>
struct AbilityImpl<ABILITY_DEVIATE> : is AteAbility<TYPE_DARK> {};

template <>
struct AbilityImpl<ABILITY_SUNS_BOUNTY> : is Merged<ABILITY_HARVEST, ABILITY_LEAF_GUARD> {};

template <>
struct AbilityImpl<ABILITY_RITE_OF_SPRING> : is Merged<ABILITY_SOLAR_POWER, ABILITY_CHLOROPHYLL> {};

template <>
struct AbilityImpl<ABILITY_HEADSTRONG> : is RaiseStatOnEntry<STAT_SPDEF> {};

template <>
struct AbilityImpl<ABILITY_FIREFIGHTER> : is TypeSlayer<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_SEPIA_LENS> : is AbilityImpl<ABILITY_SAND_GUARD>, is AbilityImpl<ABILITY_TINTED_LENS> {};

template <>
struct AbilityImpl<ABILITY_SUPER_SNIPER> : is AbilityImpl<ABILITY_SNIPER>, is UseTurnAttackAsPursuit {
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_SNIPER>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
            MUL(0.5);
        }
    }
};

template <>
struct AbilityImpl<ABILITY_WOODLAND_CURSE> : is OnEither, is SimpleEntryMove<MOVE_FORESTS_CURSE> {
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK_NOT(IS_BATTLER_OF_TYPE(opponent, TYPE_GRASS))

        gBattleMons[opponent].type3 = TYPE_GRASS;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[opponent].type3);
        gStackBattler1 = opponent;
        BattleScriptCall(BattleScript_StackBecameTheTypeFull);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MALODOR> : is OnDefender {
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
struct AbilityImpl<ABILITY_BLUR> : is OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK(IsMoveMakingContact(move, gBattlerAttacker)) return STAT_SPEED; }
};

template <>
struct AbilityImpl<ABILITY_ELUDE> : is OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
    ON_CHOOSE_DEFENSIVE_STAT { CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker)) return STAT_SPEED; }
};

template <>
struct AbilityImpl<ABILITY_DRAKE_OF_RAGE> : is AbilityImpl<ABILITY_RAMPAGE>, is AbilityImpl<ABILITY_TINTED_LENS> {};

template <>
struct AbilityImpl<ABILITY_MIXED_MARTIAL_ARTS> : is OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_PUNCH || flag == MOVE_FLAG_KICK) CHECK(gBattleMoves[move].type == TYPE_NORMAL) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_STRATEGIC_PAUSE> : is AbilityImpl<ABILITY_ANALYTIC>, is OnCrit<> {
    ON_CRIT { CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber) CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) return 2; }
};

template <>
struct AbilityImpl<ABILITY_OVERRULE> : is OnAfterTypeEffectiveness<> {
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
    }
};

template <>
struct AbilityImpl<ABILITY_MENTAL_POLLUTION> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_MADNESS_ENHANCEMENT> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_TENTALOCK> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SERPENT_BIND> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SOUL_TAP> : is OnEndTurn {
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
struct AbilityImpl<ABILITY_SCARECROW> : is AbilityImpl<ABILITY_INTIMIDATE>, is AbilityImpl<ABILITY_BAD_LUCK> {};

template <>
struct AbilityImpl<ABILITY_OMINOUS_SHROUD> : is AbilityImpl<ABILITY_PHANTOM>, is AbilityImpl<ABILITY_SHADOW_SHIELD> {};

template <>
struct AbilityImpl<ABILITY_CHILLING_PRESENCE> : is SimpleEntryMove<MOVE_ICY_WIND, 10> {};

template <>
struct AbilityImpl<ABILITY_FROSTBIND> : is PoisonPuppeteerLike<MOVE_EFFECT_FROSTBITE> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) { return (int)CanGetFrostbite(battler); }, BattleScript_Frostbind);
    }
};

template <>
struct AbilityImpl<ABILITY_TENDER_AFFECTION> : is AbilityImpl<ABILITY_CUTE_CHARM>, is OnStab {
    ON_STAB { return moveType == TYPE_FAIRY; }
};

template <>
struct AbilityImpl<ABILITY_GLACIAL_GHOST> : is AbilityImpl<ABILITY_SLUSH_RUSH>, is AbilityImpl<ABILITY_SNOW_CLOAK> {};

template <>
struct AbilityImpl<ABILITY_WONDER_SCALE> : is AbilityImpl<ABILITY_SHED_SKIN>, is AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_OVERZEALOUS> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_STAINLESS_STEEL> : is AteAbility<TYPE_STEEL>, is AbilityImpl<ABILITY_FORT_KNOX> {};

template <>
struct AbilityImpl<ABILITY_TEMPORAL_RUPTURE> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_GRASS_FLUTE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IsSoundMove(battler, move))
        CHECK_NOT(gVolatileStructs[target].fear)

        return AbilityStatusEffect(MOVE_EFFECT_FEAR);
    }
};

template <>
struct AbilityImpl<ABILITY_HEMOTOXIN> : is PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    ON_REACTIVE {
        return PoisonPuppeteerClone(
            ability,
            battler,
            [](int battler, int target) -> int { return !(gStatuses3[target] & STATUS3_GASTRO_ACID); },
            BattleScript_StackAbilitySuppressedMessage);
    }
};

template <>
struct AbilityImpl<ABILITY_HARUKAZE> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_TOXIC_SURGE> : is OnEntry, is AllowTerrainIfAirborne<TERRAIN_TOXIC> {
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_POISON_QUILLS> : is Merged<ABILITY_POISON_POINT, ABILITY_ROUGH_SKIN> {};

template <>
struct AbilityImpl<ABILITY_DRACONIC_MIGHT> : is AbilityImpl<ABILITY_HALF_DRAKE>, is AteAbility<TYPE_DRAGON> {};

template <>
struct AbilityImpl<ABILITY_ATLANTIC_RULER> : is AbilityImpl<ABILITY_AQUATIC_DWELLER>, is AbilityImpl<ABILITY_SWIFT_SWIM> {};

template <>
struct AbilityImpl<ABILITY_BIOFILM> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CHOKEHOLD> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_GUARDIAN_COAT> : is SandImmune, is OnDefensiveMultiplier<>, is PowderImmune, is HailImmune {
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.8);
    }
};

template <>
struct AbilityImpl<ABILITY_NEUTRALIZING_FOG> : is SimpleEntryMove<MOVE_DEFOG> {};

template <>
struct AbilityImpl<ABILITY_FESTIVITIES> : is OnModifyMoveFlags {
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
struct AbilityImpl<ABILITY_FEY_FLIGHT> : is AbilityImpl<ABILITY_FAIRY_TALE>, is GroundImmune {};

template <>
struct AbilityImpl<ABILITY_BEST_OFFENSE> : is AbilityImpl<ABILITY_KEEN_EDGE>, is AbilityImpl<ABILITY_MYSTIC_BLADES>, is OnChooseOffensiveStat {
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPDEF] += 20; }
};

template <>
struct AbilityImpl<ABILITY_IMPALER> : is AbilityImpl<ABILITY_MIGHTY_HORN>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target)) CHECK(CanBleed(target)) CHECK(gBattleMoves[move].hornBased);
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

template <>
struct AbilityImpl<ABILITY_MAGUS_BLADES> : is AbilityImpl<ABILITY_DUAL_WIELD>, is AbilityImpl<ABILITY_BEST_OFFENSE> {};

template <>
struct AbilityImpl<ABILITY_LIGHTNING_BORN> : is AddsType<TYPE_ELECTRIC> {};

template <>
struct AbilityImpl<ABILITY_SUPERHEAVY> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_WORLD_SERPENT> : is AbilityImpl<ABILITY_GRIP_PINCER>, is AbilityImpl<ABILITY_LONG_REACH> {};

template <>
struct AbilityImpl<ABILITY_LUCKY_WINGS> : is AbilityImpl<ABILITY_GIANT_WINGS>, is AbilityImpl<ABILITY_SERENE_GRACE> {};

template <>
struct AbilityImpl<ABILITY_KOMODO> : is AbilityImpl<ABILITY_HALF_DRAKE>, is AbilityImpl<ABILITY_TOXIC_CHAIN> {};

template <>
struct AbilityImpl<ABILITY_ENVENOM> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_POISON);
    }
};

template <>
struct AbilityImpl<ABILITY_PURPLE_HAZE> : is OnAttacker {
    ON_ATTACKER {
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
    }
};

template <>
struct AbilityImpl<ABILITY_GNASHING_CANNON> : is Merged<ABILITY_MEGA_LAUNCHER, ABILITY_MIND_CRUSH> {};

template <>
struct AbilityImpl<ABILITY_HYPER_CLEANSE> : is OnDefensiveMultiplier<>, is RemovesStatusOnImmunity {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_POISON) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_MOLTEN_COAT> : is OnAttacker, is AteAbility<TYPE_ROCK> {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanBeBurned(target))
        CHECK(Random() % 2)

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_ROYAL_DECREE> : is AbilityImpl<ABILITY_QUEENLY_MAJESTY>, is OnEntry {
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_GLARE, 0);
    }
};

template <>
struct AbilityImpl<ABILITY_TAG> : is OnPreemptAction {
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
struct AbilityImpl<ABILITY_SURPRISE> : is OnPreemptAction {
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
struct AbilityImpl<ABILITY_BREEZY_NEIGH> : is AbilityImpl<ABILITY_ADRENALINE_RUSH> {};

template <>
struct AbilityImpl<ABILITY_DREAMSCAPE> : is AbilityImpl<ABILITY_COMATOSE>, is AbilityImpl<ABILITY_DREAMCATCHER> {
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_DREAMCATCHER>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        MUL(1.2);
    }
};

template <>
struct AbilityImpl<ABILITY_HASTE_MAKES_WASTE> : is AbilityImpl<ABILITY_ANALYTIC>, is AbilityImpl<ABILITY_STALL> {};

template <>
struct AbilityImpl<ABILITY_HUNGRY_MAWS> : is AbilityImpl<ABILITY_JAWS_OF_CARNAGE>, is AbilityImpl<ABILITY_STRONG_JAW> {};

template <>
struct AbilityImpl<ABILITY_THERMAL_SLIDE> : is OnStat<> {
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_THERMOMANCY> : is Merged<ABILITY_CRYOMANCY, ABILITY_PYROMANCY> {};

template <>
struct AbilityImpl<ABILITY_CHUCKSTER> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_HEAT_SINK> : is LightningRodClone<TYPE_FIRE> {};

template <>
struct AbilityImpl<ABILITY_RELIC_STONE> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_SUPERCELL> : is Merged<ABILITY_ELECTRIC_SURGE, ABILITY_DRIZZLE> {};

template <>
struct AbilityImpl<ABILITY_LIGHTNING_ASPECT> : is AbsorbStatUp<TYPE_ELECTRIC, STAT_HIGHEST_ATTACKING> {};

template <>
struct AbilityImpl<ABILITY_FIRE_ASPECT> : is AbsorbHeal<TYPE_FIRE>, is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanBeBurned(target))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_BLISTERING_SUN> : is Merged<ABILITY_DESOLATE_LAND, ABILITY_AIR_BLOWER> {};

template <>
struct AbilityImpl<ABILITY_AURORAS_GALE> : is AbilityImpl<ABILITY_NORTH_WIND>, is AbilityImpl<ABILITY_MAJESTIC_BIRD> {};

template <>
struct AbilityImpl<ABILITY_WINTER_THRONE> : is OnEntry, is OnEndTurn {
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
struct AbilityImpl<ABILITY_ICE_PLUMES> : is AbilityImpl<ABILITY_ICE_SCALES> {};

template <>
struct AbilityImpl<ABILITY_STALWART> {};

template <>
struct AbilityImpl<ABILITY_PROPELLER_TAIL> : is AbilityImpl<ABILITY_SWIFT_SWIM>, is AbilityImpl<ABILITY_STALWART> {};

template <>
struct AbilityImpl<ABILITY_ENERGY_TAP> : is OnAttacker {
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
struct AbilityImpl<ABILITY_MOLTEN_CORE> : is AbilityImpl<ABILITY_FURNACE>, is AbsorbStatUp<TYPE_ROCK, STAT_SPEED>, is AbsorbUp2, is StealthRockImmune {
    ON_ENTRY {
        AbilityImpl<ABILITY_FURNACE>::onEntry(DELEGATE_ENTRY);

        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[GetBattlerSide(battler)] &= ~SIDE_STATUS_STEALTH_ROCK;
        return SwitchInAnnounce(B_MSG_SWITCHIN_MOLTEN_CORE);
    }
};

template <>
struct AbilityImpl<ABILITY_REVERBATE> : is OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_SOUND) CHECK(gBattleMoves[move].type == TYPE_NORMAL) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_TAEKKYEON> : is OnModifyMoveFlags {
    ON_MODIFY_MOVE_FLAGS { CHECK(flag == MOVE_FLAG_DANCE) CHECK_NOT(IS_MOVE_STATUS(move)) return TRUE; }
};

template <>
struct AbilityImpl<ABILITY_SLUDGE_SPIT> : is OnAttacker {
    ON_ATTACKER {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_VENOM_BOLT, 35);
    }
};

template <>
struct AbilityImpl<ABILITY_SWAMP_THING> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].swampTimer)

        AbilityStatusEffectSafe(MOVE_EFFECT_SWAMP, battler, GetOppositeSide(battler));
        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FROSTY_PRESCENCE> : is SimpleEntryMove<MOVE_MIST> {};

template <>
struct AbilityImpl<ABILITY_CHILLING_PELLETS> : is OnDefender {
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_SPEAR, 13);
        return FALSE;
    }
};

template <>
struct AbilityImpl<ABILITY_PAINT_SHOT> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(IS_BATTLER_OF_TYPE(target, moveType))
        CHECK(IsMegaLauncherBoosted(battler, move))

        gBattleMons[target].type1 = moveType;
        gBattleMons[target].type2 = moveType;
        gBattleMons[target].type3 = TYPE_MYSTERY;
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        gStackBattler1 = target;
        BattleScriptCall(BattleScript_StackBecameTheTypeFull);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_STONECUTTER> : is AbilityImpl<ABILITY_FOSSILIZED>, is OnMoldBreaker {
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
struct AbilityImpl<ABILITY_EDGELORD> : is AbilityImpl<ABILITY_CUTTHROAT>, is OnBattlerFaints<> {
    ON_BATTLER_FAINTS {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CUTTHROAT;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_WARMONGER> : is OnOffensiveMultiplier<> {
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK || moveType == TYPE_STEEL || moveType == TYPE_FIGHTING) MUL(1.30);
    }
};

template <>
struct AbilityImpl<ABILITY_LOCUST_SWARM> : is StandardTransformation {};

template <>
struct AbilityImpl<ABILITY_REVELATION> : is StandardTransformation {};

template <>
struct AbilityImpl<ABILITY_CURSE_OF_FAMINE> : is OnEntry {
    ON_ENTRY {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_CurseOfFamine);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_CRYSTALLINE_ARMOR> : is AbilityImpl<ABILITY_MIRROR_ARMOR>, is OnCrit<ApplyOnTarget::TARGET> {
    ON_CRIT { return NEVER_CRIT; }
};

template <>
struct AbilityImpl<ABILITY_SOUL_HARVEST> : is OnStat<>, is Breakable {
    ON_STAT {
        if (statId != STAT_SPEED) *stat = *stat * (20 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 20;
    }
};

template <>
struct AbilityImpl<ABILITY_THICK_BLUBBER> : is OnDefensiveMultiplier<>, is OnStat<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.25);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .5;
    }
};

template <>
struct AbilityImpl<ABILITY_CRAVING> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_RAT_KING> : is OnStat<ApplyOn::ALLY> {
    ON_STAT {
        const BaseStats *baseStats = &gBaseStats[gBattleMons[battler].species];
        int bst =
            baseStats->baseHP + baseStats->baseAttack + baseStats->baseDefense + baseStats->baseSpAttack + baseStats->baseSpDefense + baseStats->baseSpeed;
        if (bst >= 400) return;
        *stat *= 1.5;
    }
};

template <>
struct AbilityImpl<ABILITY_CRISPY_CREAM> : is OnDefender {
    ON_DEFENDER {
        return Random() % 2 ? AbilityImpl<ABILITY_FLAME_BODY>::onEither(DELEGATE_DEFENDER) : AbilityImpl<ABILITY_FREEZING_POINT>::onEither(DELEGATE_DEFENDER);
    }
};

template <>
struct AbilityImpl<ABILITY_DEEP_FRIED> : is OnEntry {
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].fireSeaTimer)

        AbilityStatusEffectSafe(MOVE_EFFECT_FIRE_SEA, battler, GetOppositeSide(battler));
        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_FOOD_LOVERS> : is AbilityImpl<ABILITY_HOSPITALITY>, is AbilityImpl<ABILITY_FRIEND_GUARD> {};

template <>
struct AbilityImpl<ABILITY_LUNAR_WRATH> : is OnAttacker {
    ON_ATTACKER {
        CHECK(moveType == TYPE_GHOST)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MOONGEIST_BEAM, 50);
    }
};

template <>
struct AbilityImpl<ABILITY_SPYWARE> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_VIRUS> : is OnAttacker {
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(CanBePoisoned(battler, target, move))

        return AbilityStatusEffect(MOVE_EFFECT_POISON);
    }
};

template <>
struct AbilityImpl<ABILITY_POWER_LEAK> : is OnDefender, is AllowTerrainIfAirborne<TERRAIN_ELECTRIC> {
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
struct AbilityImpl<ABILITY_BACKUP_POWER> : is OnRevive {
    ON_REVIVE { CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) return B_MSG_BACKUP_POWER; }
};

template <>
struct AbilityImpl<ABILITY_SAND_FIEND> : is AbilityImpl<ABILITY_SAND_GUARD>, is AbilityImpl<ABILITY_SAND_FORCE> {};

template <>
struct AbilityImpl<ABILITY_MOUSTACHE> : is Merged<ABILITY_TANGLING_HAIR, ABILITY_STAMINA> {};

template <>
struct AbilityImpl<ABILITY_DEPTH_EXPLORER> : is AbilityImpl<ABILITY_FIELD_EXPLORER>, is AbilityImpl<ABILITY_ILLUMINATE> {};

template <>
struct AbilityImpl<ABILITY_DUNE_VEIL> : is AbilityImpl<ABILITY_SAND_GUARD>, is AbilityImpl<ABILITY_SELF_SUFFICIENT> {};

template <>
struct AbilityImpl<ABILITY_STRONG_FOUNDATION> : is OnDefensiveMultiplier<> {
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_GROUND) RESISTANCE(.50);
    }
};

template <>
struct AbilityImpl<ABILITY_FOG_MACHINE> : is OnDefender {
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
struct AbilityImpl<ABILITY_DROP_BLOCKS> : is OnDefender {
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    }
};

template <>
struct AbilityImpl<ABILITY_LASER_DRILL> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_LIGHT_SABER> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_LOOSE_THORNS> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_TURF_WAR> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_GREEDY> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_MUSICAL_NOTES> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_STRIKEOUT> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_HOME_RUN> : is NotImplemented {};

template <>
struct AbilityImpl<ABILITY_BRUISER> : is AddsType<TYPE_FIGHTING> {};

template <>
struct AbilityImpl<ABILITY_LETS_DANCE> : is SimpleEntryMove<MOVE_TEETER_DANCE> {};

template <>
struct AbilityImpl<ABILITY_MYCELIUM_MIGHT> : is OnMoldBreaker {
    ON_MOLD_BREAKER { return IS_MOVE_STATUS(move); }
};

template <>
struct AbilityImpl<ABILITY_DEADLY_PRECISION> : is OnMoldBreaker {
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
