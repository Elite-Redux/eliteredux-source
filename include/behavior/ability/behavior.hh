#pragma once

#include "behavior/constants.hh"
#include "behavior/implementation_interface.hh"
#include "behavior/ability/constants.hh"
#include "behavior/ability/template.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-function"

#define ENUM_OR(enumType) \
    inline enumType operator|(enumType a, enumType b) { return static_cast<enumType>(static_cast<int>(a) | static_cast<int>(b)); }

ENUM_OR(InfiltrateType)
ENUM_OR(MoveEffectEnum)
ENUM_OR(NonStackingState)

template <typename As>
const As *dispatchTo(AbilityEnum id);

#define ABILITY(ID) \
    template <>     \
    struct AbilityImpl<ID>
#define INSTANCE(ID) static const AbilityImpl<ID> instance

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

int DoesMoveMatchFlag(ON_MODIFY_MOVE_FLAGS_ARGS) {
    switch (flag) {
        case MOVE_FLAG_DANCE:
            if (gBattleMoves[flag].flags & FLAG_DANCE) return TRUE;
            break;
        case MOVE_FLAG_KICK:
            if (gBattleMoves[flag].flags & FLAG_STRIKER_BOOST) return TRUE;
            break;
        case MOVE_FLAG_MEGA_LAUNCHER:
            if (gBattleMoves[flag].flags & FLAG_MEGA_LAUNCHER_BOOST) return TRUE;
            break;
        case MOVE_FLAG_PUNCH:
            if (gBattleMoves[flag].flags & FLAG_IRON_FIST_BOOST) return TRUE;
            break;
        case MOVE_FLAG_SOUND:
            if (gBattleMoves[flag].flags & FLAG_SOUND) return TRUE;
            break;

        default:
            return FALSE;
            break;
    }

    ON_ABILITY(battler, FALSE, gAbilities[ability].onModifyMoveFlags, if (gAbilities[ability].onModifyMoveFlags(DELEGATE_MODIFY_MOVE_FLAGS)) return TRUE)
    return FALSE;
}

ABILITY(ABILITY_NONE) : extends RandomizerBanned { INSTANCE(ABILITY_NONE); };

template <int Stat>
struct RuinEffect : extends OnStat<ApplyOn::OTHER> {
    // static constexpr auto ruinAbilities = abilitiesAs<RuinEffect<Stat>>();
    ON_STAT {
        if (statId != Stat) return;
        if (*flags & Stat) return;
        ON_ABILITY(battler, FALSE, dispatchTo<RuinEffect<Stat>>(ability), return) *stat *= .75;
        *flags |= static_cast<NonStackingState>(1 << Stat);
    }
};

ABILITY(ABILITY_TABLETS_OF_RUIN) : extends RuinEffect<STAT_ATK> { INSTANCE(ABILITY_TABLETS_OF_RUIN); };
ABILITY(ABILITY_SWORD_OF_RUIN) : extends RuinEffect<STAT_DEF> { INSTANCE(ABILITY_SWORD_OF_RUIN); };
ABILITY(ABILITY_VESSEL_OF_RUIN) : extends RuinEffect<STAT_SPATK> { INSTANCE(ABILITY_VESSEL_OF_RUIN); };
ABILITY(ABILITY_BEADS_OF_RUIN) : extends RuinEffect<STAT_SPDEF> { INSTANCE(ABILITY_BEADS_OF_RUIN); };

struct ToxicTerrainImmune {};
ABILITY(ABILITY_STENCH) : extends OnAttacker, extends ToxicTerrainImmune {
    INSTANCE(ABILITY_STENCH);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    }
};

ABILITY(ABILITY_POISON_HEAL) : extends ToxicTerrainImmune { INSTANCE(ABILITY_POISON_HEAL); };

ABILITY(ABILITY_DRIZZLE) : extends OnEntry {
    INSTANCE(ABILITY_DRIZZLE);
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

ABILITY(ABILITY_SPEED_BOOST) : extends OnEndTurn {
    INSTANCE(ABILITY_SPEED_BOOST);
    ON_END_TURN {
        CHECK(gVolatileStructs[battler].isFirstTurn != 2)
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

ABILITY(ABILITY_BATTLE_ARMOR) : extends Breakable, extends OnDefensiveMultiplier, extends OnCrit<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_BATTLE_ARMOR);
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
    ON_CRIT { return NEVER_CRIT; }
};

ABILITY(ABILITY_STURDY) : extends Breakable { INSTANCE(ABILITY_STURDY); };

ABILITY(ABILITY_DAMP) : extends OnEither {
    INSTANCE(ABILITY_DAMP);
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

struct HalfRecoil {};
struct RemovesStatusOnImmunity : extends OnStatusImmune<ApplyOn::SELF> {};

ABILITY(ABILITY_LIMBER) : extends RemovesStatusOnImmunity, extends HalfRecoil {
    INSTANCE(ABILITY_LIMBER);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    }
};

struct SandImmune {};
ABILITY(ABILITY_SAND_VEIL) : extends Breakable, extends SandImmune, extends OnAccuracy<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_SAND_VEIL);
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_STATIC) : extends OnEither {
    INSTANCE(ABILITY_STATIC);
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
ABILITY(ABILITY_VOLT_ABSORB) : extends AbsorbHeal<TYPE_ELECTRIC> { INSTANCE(ABILITY_VOLT_ABSORB); };

ABILITY(ABILITY_WATER_ABSORB) : extends AbsorbHeal<TYPE_WATER> { INSTANCE(ABILITY_WATER_ABSORB); };

struct TauntImmune : extends Breakable {};

ABILITY(ABILITY_OBLIVIOUS) : extends RemovesStatusOnImmunity, extends TauntImmune {
    INSTANCE(ABILITY_OBLIVIOUS);
    ON_STATUS_IMMUNE {
        CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING))
        return TRUE;
    }
};

ABILITY(ABILITY_CLOUD_NINE) : extends OnEntry {
    INSTANCE(ABILITY_CLOUD_NINE);
    ON_ENTRY {
        BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
        return TRUE;
    }
};

ABILITY(ABILITY_COMPOUND_EYES) : extends OnAccuracy<> {
    INSTANCE(ABILITY_COMPOUND_EYES);
    ON_ACCURACY {
        *accuracy *= 1.3;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_INSOMNIA) : extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_INSOMNIA);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    }
};

ABILITY(ABILITY_COLOR_CHANGE) : extends OnBeforeAttack<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_COLOR_CHANGE);
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

ABILITY(ABILITY_IMMUNITY) : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_IMMUNITY);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_POISON) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & (CHECK_STATUS1 & ~CHECK_SLEEP))
        return TRUE;
    }
};

ABILITY(ABILITY_FLASH_FIRE) : extends OnAbsorb, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_FLASH_FIRE);
    ON_ABSORB {
        CHECK(moveType == TYPE_FIRE)
        return ABSORB_RESULT_FLASH_FIRE;
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
    }
};

struct PowderImmune : extends Breakable {};
ABILITY(ABILITY_SHIELD_DUST) : extends PowderImmune { INSTANCE(ABILITY_SHIELD_DUST); };

ABILITY(ABILITY_OWN_TEMPO) : extends RemovesStatusOnImmunity, extends TauntImmune {
    INSTANCE(ABILITY_OWN_TEMPO);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    }
};

ABILITY(ABILITY_SUCTION_CUPS) : extends Breakable { INSTANCE(ABILITY_SUCTION_CUPS); };

ABILITY(ABILITY_INTIMIDATE) : extends OnEntry {
    INSTANCE(ABILITY_INTIMIDATE);
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

ABILITY(ABILITY_SHADOW_TAG) : extends OnTrap {
    INSTANCE(ABILITY_SHADOW_TAG);
    ON_TRAP {
        ON_ABILITY(switchingBattler, FALSE, gAbilities[ability].shadowTag, return FALSE)
        return TRUE;
    }
};

ABILITY(ABILITY_ROUGH_SKIN) : extends OnDefender {
    INSTANCE(ABILITY_ROUGH_SKIN);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        gBattleMoveDamage = gBattleMons[attacker].maxHP / 8;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_WONDER_GUARD) : extends Breakable, extends RandomizerBanned, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_WONDER_GUARD);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod < UQ_4_12(2.0)) *mod = 0;
    }
};

struct GroundImmune : extends Breakable {};
ABILITY(ABILITY_LEVITATE) : extends GroundImmune, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_LEVITATE);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING) MUL(1.25);
    }
};

ABILITY(ABILITY_EFFECT_SPORE) : extends PowderImmune, extends OnDefender {
    INSTANCE(ABILITY_EFFECT_SPORE);
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

ABILITY(ABILITY_CLEAR_BODY) : extends Breakable { INSTANCE(ABILITY_CLEAR_BODY); };

ABILITY(ABILITY_NATURAL_CURE) : extends OnExit {
    INSTANCE(ABILITY_NATURAL_CURE);
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
        *statId = STAT_SPEED;
        return ABSORB_RESULT_STAT;
    }
};
template <Type Absorbed>
struct LightningRodClone : extends Redirects<Absorbed>, extends AbsorbStatUp<Absorbed, STAT_HIGHEST_ATTACKING> {};
ABILITY(ABILITY_LIGHTNING_ROD) : LightningRodClone<TYPE_ELECTRIC> { INSTANCE(ABILITY_LIGHTNING_ROD); };

ABILITY(ABILITY_SERENE_GRACE) : extends OnModifyEffectChance<> {
    INSTANCE(ABILITY_SERENE_GRACE);
    ON_MODIFY_EFFECT_CHANCE { *effectChance *= 2; }
};

ABILITY(ABILITY_SWIFT_SWIM) : extends OnStat<> {
    INSTANCE(ABILITY_SWIFT_SWIM);
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_CHLOROPHYLL) : extends OnStat<> {
    INSTANCE(ABILITY_CHLOROPHYLL);
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_ILLUMINATE) : extends OnAccuracy<> {
    INSTANCE(ABILITY_ILLUMINATE);
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_TRACE) : extends RandomizerBanned, extends OnEntry {
    INSTANCE(ABILITY_TRACE);
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

ABILITY(ABILITY_HUGE_POWER) : extends OnStat<> {
    INSTANCE(ABILITY_HUGE_POWER);
    ON_STAT {
        if (statId == STAT_ATK) *stat *= 2;
    }
};

ABILITY(ABILITY_POISON_POINT) : extends OnEither {
    INSTANCE(ABILITY_POISON_POINT);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBePoisoned(battler, opponent, MOVE_NONE))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_INNER_FOCUS) : extends TauntImmune, extends OnAccuracy<> {
    INSTANCE(ABILITY_INNER_FOCUS);
    ON_ACCURACY {
        CHECK(move == MOVE_FOCUS_BLAST)
        return ACCURACY_ALWAYS_HITS;
    }
};

ABILITY(ABILITY_MAGMA_ARMOR) : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_MAGMA_ARMOR);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_FROSTBITE)
        return TRUE;
    }
};

ABILITY(ABILITY_WATER_VEIL) : extends OnEntry, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_WATER_VEIL);
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

ABILITY(ABILITY_MAGNET_PULL) : extends OnTrap {
    INSTANCE(ABILITY_MAGNET_PULL);
    ON_TRAP { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); }
};

ABILITY(ABILITY_SOUNDPROOF) : extends OnImmune<> {
    INSTANCE(ABILITY_SOUNDPROOF);
    ON_IMMUNE {
        CHECK(IsSoundMove(attacker, move))
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

ABILITY(ABILITY_RAIN_DISH) : extends OnEndTurn {
    INSTANCE(ABILITY_RAIN_DISH);
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

ABILITY(ABILITY_SAND_STREAM) : extends SandImmune, extends OnEntry {
    INSTANCE(ABILITY_SAND_STREAM);
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

ABILITY(ABILITY_PRESSURE) : extends OnEntry {
    INSTANCE(ABILITY_PRESSURE);
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

ABILITY(ABILITY_THICK_FAT) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_THICK_FAT);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
    }
};

ABILITY(ABILITY_FLAME_BODY) : extends OnEither {
    INSTANCE(ABILITY_FLAME_BODY);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_KEEN_EYE) : extends OnAccuracy<> {
    INSTANCE(ABILITY_KEEN_EYE);
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_HYPER_CUTTER) : extends Breakable, extends OnCrit<> {
    INSTANCE(ABILITY_HYPER_CUTTER);
    ON_CRIT {
        CHECK(IsMoveMakingContact(move, battler))
        return 1;
    }
};

ABILITY(ABILITY_PICKUP) : extends OnEntry {
    INSTANCE(ABILITY_PICKUP);
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

ABILITY(ABILITY_TRUANT) : extends OnEndTurn {
    INSTANCE(ABILITY_TRUANT);
    ON_END_TURN {
        if (GetAbilityState(battler, ability))
            SetAbilityState(battler, ability, FALSE);
        else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
            SetAbilityState(battler, ability, TRUE);
        return FALSE;
    }
};

ABILITY(ABILITY_HUSTLE) : extends OnAccuracy<>, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_HUSTLE);
    ON_OFFENSIVE_MULTIPLIER { MUL(1.4); }
    ON_ACCURACY {
        CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_CUTE_CHARM) : extends OnEither {
    INSTANCE(ABILITY_CUTE_CHARM);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(CanInfatuate(battler, opponent))
        CHECK(Random() % 100 < 50)

        AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_PLUS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_PLUS);
    ON_OFFENSIVE_MULTIPLIER {
        int partner = BATTLE_PARTNER(battler);
        if (!IsBattlerAlive(partner)) return;
        if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
    }
};

ABILITY(ABILITY_MINUS) : extends AbilityImpl<ABILITY_PLUS> { INSTANCE(ABILITY_MINUS); };

struct StandardTransformation : extends FormChangeAbility, extends OnEntry, extends OnEndTurn {
    ON_ENTRY { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_END_TURN { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
};
struct WeatherTransformation : extends StandardTransformation, extends OnWeather {
    ON_WEATHER { return TryTransformAttacker(ability, battler, ABILITY_BS_CALL); }
};

ABILITY(ABILITY_FORECAST) : extends WeatherTransformation, extends OnAttacker {
    INSTANCE(ABILITY_FORECAST);
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

ABILITY(ABILITY_STICKY_HOLD) : extends Breakable { INSTANCE(ABILITY_STICKY_HOLD); };

ABILITY(ABILITY_SHED_SKIN) : extends OnEndTurn {
    INSTANCE(ABILITY_SHED_SKIN);
    ON_END_TURN {
        CHECK(Random() % 100 < 30)

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

struct NegateBurnAtkDrop {};
ABILITY(ABILITY_GUTS) : extends OnOffensiveMultiplier<>, extends NegateBurnAtkDrop {
    INSTANCE(ABILITY_GUTS);
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

ABILITY(ABILITY_MARVEL_SCALE) : extends OnStat<> {
    INSTANCE(ABILITY_MARVEL_SCALE);
    ON_STAT {
        if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_OVERGROW) : extends SwarmLike<TYPE_GRASS> { INSTANCE(ABILITY_OVERGROW); };

ABILITY(ABILITY_BLAZE) : extends SwarmLike<TYPE_FIRE> { INSTANCE(ABILITY_BLAZE); };

ABILITY(ABILITY_TORRENT) : extends SwarmLike<TYPE_WATER> { INSTANCE(ABILITY_TORRENT); };

ABILITY(ABILITY_SWARM) : extends SwarmLike<TYPE_BUG> { INSTANCE(ABILITY_SWARM); };

struct NoRecoil {};
ABILITY(ABILITY_ROCK_HEAD) : extends RemovesStatusOnImmunity, extends NoRecoil {
    INSTANCE(ABILITY_ROCK_HEAD);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    }
};

ABILITY(ABILITY_DROUGHT) : extends OnEntry {
    INSTANCE(ABILITY_DROUGHT);
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

ABILITY(ABILITY_ARENA_TRAP) : extends OnTrap {
    INSTANCE(ABILITY_ARENA_TRAP);
    ON_TRAP { return IsBattlerGrounded(switchingBattler); }
};

ABILITY(ABILITY_VITAL_SPIRIT) : extends OnAttacker, extends RemovesStatusOnImmunity, extends TauntImmune {
    INSTANCE(ABILITY_VITAL_SPIRIT);
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIGHTING)
        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    }
};

ABILITY(ABILITY_WHITE_SMOKE) : extends OnEntry {
    INSTANCE(ABILITY_WHITE_SMOKE);
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

        int side = GET_BATTLER_SIDE(battler);
        gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
        gSideTimers[side].started.smokescreen = TRUE;
        gSideTimers[side].smokescreenBattler = battler;
        return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
    }
};

ABILITY(ABILITY_FELINE_PROWESS) : extends OnStat<> {
    INSTANCE(ABILITY_FELINE_PROWESS);
    ON_STAT {
        if (statId == STAT_SPATK) *stat *= 2;
    }
};

ABILITY(ABILITY_PURE_POWER) : extends AbilityImpl<ABILITY_FELINE_PROWESS> { INSTANCE(ABILITY_PURE_POWER); };

ABILITY(ABILITY_SHELL_ARMOR) : extends AbilityImpl<ABILITY_BATTLE_ARMOR> { INSTANCE(ABILITY_SHELL_ARMOR); };

ABILITY(ABILITY_AIR_LOCK) : extends AbilityImpl<ABILITY_CLOUD_NINE> { INSTANCE(ABILITY_AIR_LOCK); };

ABILITY(ABILITY_TANGLED_FEET) : extends OnAccuracy<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_TANGLED_FEET);
    ON_ACCURACY {
        CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
        *accuracy /= 2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_MOTOR_DRIVE) : extends AbsorbStatUp<TYPE_ELECTRIC, STAT_SPEED> { INSTANCE(ABILITY_MOTOR_DRIVE); };

ABILITY(ABILITY_RIVALRY) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_RIVALRY);
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
ABILITY(ABILITY_SNOW_CLOAK) : extends Breakable, extends HailImmune, extends OnAccuracy<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_SNOW_CLOAK);
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_ANGER_POINT) : extends OnDefender {
    INSTANCE(ABILITY_ANGER_POINT);
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

ABILITY(ABILITY_UNBURDEN) : extends OnStat<> {
    INSTANCE(ABILITY_UNBURDEN);
    ON_STAT {
        if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
    }
};

ABILITY(ABILITY_HEATPROOF) : extends OnDefensiveMultiplier, extends NegateBurnAtkDrop {
    INSTANCE(ABILITY_HEATPROOF);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
};

ABILITY(ABILITY_DRY_SKIN) : extends AbilityImpl<ABILITY_WATER_ABSORB>, extends AbilityImpl<ABILITY_RAIN_DISH>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_DRY_SKIN);
    ON_END_TURN {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !IsMagicGuardProtected(battler)) {
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

ABILITY(ABILITY_DOWNLOAD) : extends OnEntry {
    INSTANCE(ABILITY_DOWNLOAD);
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

ABILITY(ABILITY_IRON_FIST) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_IRON_FIST);
    ON_OFFENSIVE_MULTIPLIER {
        if (IsIronFistBoosted(battler, move)) MUL(1.3);
    }
};

ABILITY(ABILITY_ADAPTABILITY) { INSTANCE(ABILITY_ADAPTABILITY); };

ABILITY(ABILITY_SKILL_LINK) { INSTANCE(ABILITY_SKILL_LINK); };

ABILITY(ABILITY_HYDRATION) : extends OnEndTurn {
    INSTANCE(ABILITY_HYDRATION);
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

ABILITY(ABILITY_SOLAR_POWER) : extends OnStat<> {
    INSTANCE(ABILITY_SOLAR_POWER);
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_QUICK_FEET) : extends OnStat<> {
    INSTANCE(ABILITY_QUICK_FEET);
    ON_STAT {
        if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_NORMALIZE) : extends OnOffensiveMultiplier<>, extends OnMoveType, extends OnTypeEffectiveness<> {
    INSTANCE(ABILITY_NORMALIZE);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
    }
    ON_MOVE_TYPE { return TYPE_NORMAL + 1; }
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_NORMAL) CHECK(*mod) CHECK(*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

ABILITY(ABILITY_SNIPER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_SNIPER);
    ON_OFFENSIVE_MULTIPLIER {
        if (isCrit) MUL(1.5);
    }
};

ABILITY(ABILITY_MAGIC_GUARD) { INSTANCE(ABILITY_MAGIC_GUARD); };

ABILITY(ABILITY_NO_GUARD) : extends OnAccuracy<ApplyOnTarget::ATTACKER_OR_TARGET> {
    INSTANCE(ABILITY_NO_GUARD);
    ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
};

ABILITY(ABILITY_STALL) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_STALL);
    ON_DEFENSIVE_MULTIPLIER {
        if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
    }
};

ABILITY(ABILITY_TECHNICIAN) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_TECHNICIAN);
    ON_OFFENSIVE_MULTIPLIER {
        if (basePower <= 60) MUL(1.5);
    }
};

ABILITY(ABILITY_LEAF_GUARD) : extends OnEndTurn {
    INSTANCE(ABILITY_LEAF_GUARD);
    ON_END_TURN {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        CHECK(AbilityHealMonStatus(battler, ability));
        return TRUE;
    }
};

ABILITY(ABILITY_MOLD_BREAKER) : extends OnEntry, extends OnMoldBreaker {
    INSTANCE(ABILITY_MOLD_BREAKER);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); }
    ON_MOLD_BREAKER { return TRUE; }
};

ABILITY(ABILITY_SUPER_LUCK) : extends OnCrit<> {
    INSTANCE(ABILITY_SUPER_LUCK);
    ON_CRIT { return 1; }
};

ABILITY(ABILITY_AFTERMATH) : extends OnDefender {
    INSTANCE(ABILITY_AFTERMATH);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 4;
        if (!gBattleMoveDamage) gBattleMoveDamage = 1;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    }
};

ABILITY(ABILITY_ANTICIPATION) : extends Breakable, extends OnEntry, extends Persistent {
    INSTANCE(ABILITY_ANTICIPATION);
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

ABILITY(ABILITY_FOREWARN) : extends OnEntry {
    INSTANCE(ABILITY_FOREWARN);
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

ABILITY(ABILITY_UNAWARE) : extends Breakable { INSTANCE(ABILITY_UNAWARE); };

ABILITY(ABILITY_TINTED_LENS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_TINTED_LENS);
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
    }
};

ABILITY(ABILITY_FILTER) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_FILTER);
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
    }
};

ABILITY(ABILITY_SLOW_START) : extends OnEntry, extends OnStat<> {
    INSTANCE(ABILITY_SLOW_START);
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
ABILITY(ABILITY_SCRAPPY) : extends HitsGhost, extends TauntImmune { INSTANCE(ABILITY_SCRAPPY); };

ABILITY(ABILITY_STORM_DRAIN) : extends LightningRodClone<TYPE_WATER> { INSTANCE(ABILITY_STORM_DRAIN); };

ABILITY(ABILITY_ICE_BODY) : extends HailImmune, extends OnEndTurn {
    INSTANCE(ABILITY_ICE_BODY);
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

ABILITY(ABILITY_SOLID_ROCK) : extends AbilityImpl<ABILITY_FILTER> { INSTANCE(ABILITY_SOLID_ROCK); };

ABILITY(ABILITY_SNOW_WARNING) : extends HailImmune, extends OnEntry {
    INSTANCE(ABILITY_SNOW_WARNING);
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

ABILITY(ABILITY_HONEY_GATHER) : extends OnEndTurn {
    INSTANCE(ABILITY_HONEY_GATHER);
    ON_END_TURN {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK(Random() % 2)

        gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
        BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_FRISK) : extends OnEntry {
    INSTANCE(ABILITY_FRISK);
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

ABILITY(ABILITY_RECKLESS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_RECKLESS);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
    }
};

ABILITY(ABILITY_MULTITYPE) : extends FormChangeAbility { INSTANCE(ABILITY_MULTITYPE); };

ABILITY(ABILITY_FLOWER_GIFT) : extends WeatherTransformation, extends Breakable, extends OnStat<ApplyOn::ALLY> {
    INSTANCE(ABILITY_FLOWER_GIFT);
    ON_STAT {
        if (statId != STAT_SPATK && statId != STAT_SPDEF) return;
        if (IsWeatherActive(WEATHER_SUN_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_BAD_DREAMS) : extends OnEndTurn {
    INSTANCE(ABILITY_BAD_DREAMS);
    ON_END_TURN {
        gBattleScripting.abilityPopupOverwrite = ability;
        BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
        return NO_ANNOUNCE;
    }
};

ABILITY(ABILITY_SHEER_FORCE) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_SHEER_FORCE);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
    }
};

ABILITY(ABILITY_CONTRARY) : extends Breakable { INSTANCE(ABILITY_CONTRARY); };

ABILITY(ABILITY_UNNERVE) : extends OnEntry {
    INSTANCE(ABILITY_UNNERVE);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); }
};

ABILITY(ABILITY_DEFEATIST) : extends OnStat<> {
    INSTANCE(ABILITY_DEFEATIST);
    ON_STAT {
        if (statId != STAT_ATK && statId != STAT_SPATK) return;
        if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
    }
};

ABILITY(ABILITY_CURSED_BODY) : extends OnDefender {
    INSTANCE(ABILITY_CURSED_BODY);
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

ABILITY(ABILITY_HEALER) : extends OnEndTurn {
    INSTANCE(ABILITY_HEALER);
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

ABILITY(ABILITY_FRIEND_GUARD) : extends Breakable { INSTANCE(ABILITY_FRIEND_GUARD); };

ABILITY(ABILITY_WEAK_ARMOR) : extends OnDefender {
    INSTANCE(ABILITY_WEAK_ARMOR);
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

ABILITY(ABILITY_LIGHT_METAL) : extends OnStat<> {
    INSTANCE(ABILITY_LIGHT_METAL);
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= 1.3;
    }
};

ABILITY(ABILITY_MULTISCALE) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_MULTISCALE);
    ON_DEFENSIVE_MULTIPLIER {
        if (BATTLER_MAX_HP(battler)) MUL(.5);
    }
};

ABILITY(ABILITY_TOXIC_BOOST) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_TOXIC_BOOST);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

ABILITY(ABILITY_FLARE_BOOST) : extends OnEntry, extends OnWeather, extends OnStat<>, extends NegateBurnAtkDrop {
    INSTANCE(ABILITY_FLARE_BOOST);
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

ABILITY(ABILITY_HARVEST) : extends OnEndTurn {
    INSTANCE(ABILITY_HARVEST);
    ON_END_TURN {
        CHECK_NOT(gBattleMons[battler].item)
        CHECK_NOT(gBattleStruct->changedItems[battler])
        CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

        BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_TELEPATHY) : extends OnAfterTypeEffectiveness<ApplyOnTarget::ATTACKER_OR_TARGET> {
    INSTANCE(ABILITY_TELEPATHY);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
    }
};

ABILITY(ABILITY_MOODY) : extends OnEndTurn {
    INSTANCE(ABILITY_MOODY);
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

ABILITY(ABILITY_OVERCOAT) : extends Breakable, extends SandImmune, extends HailImmune, extends PowderImmune, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_OVERCOAT);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.8);
    }
};

ABILITY(ABILITY_POISON_TOUCH) : extends AbilityImpl<ABILITY_POISON_POINT> { INSTANCE(ABILITY_POISON_TOUCH); };

ABILITY(ABILITY_REGENERATOR) : extends OnExit {
    INSTANCE(ABILITY_REGENERATOR);
    ON_EXIT {
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(BATTLER_MAX_HP(battler))
        BattleScriptCall(BattleScript_RegeneratorExits);
        return FALSE;
    }
};

ABILITY(ABILITY_BIG_PECKS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_BIG_PECKS);
    ON_OFFENSIVE_MULTIPLIER {
        if (IsMoveMakingContact(move, battler)) MUL(1.3);
    }
};

ABILITY(ABILITY_SAND_RUSH) : extends OnStat<>, extends SandImmune {
    INSTANCE(ABILITY_SAND_RUSH);
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_FORT_KNOX) { INSTANCE(ABILITY_FORT_KNOX); };

ABILITY(ABILITY_WONDER_SKIN) : extends AbilityImpl<ABILITY_FORT_KNOX> { INSTANCE(ABILITY_WONDER_SKIN); };

ABILITY(ABILITY_ANALYTIC) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ANALYTIC);
    ON_OFFENSIVE_MULTIPLIER {
        if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
    }
};

ABILITY(ABILITY_ILLUSION) : extends OnDefender, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ILLUSION);
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

ABILITY(ABILITY_IMPOSTER) : extends OnEntry {
    INSTANCE(ABILITY_IMPOSTER);
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

ABILITY(ABILITY_INFILTRATOR) : extends OnInfiltrate {
    INSTANCE(ABILITY_INFILTRATOR);
    ON_INFILTRATE { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; }
};

ABILITY(ABILITY_MUMMY) : extends OnDefender {
    INSTANCE(ABILITY_MUMMY);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(HasAbilityIgnoringSuppression(attacker, ability))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(IsPersistentOrUnsuppressableAbility(GetBattlerAbility(attacker)))
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

ABILITY(ABILITY_MOXIE) : extends MoxieClone<STAT_ATK> { INSTANCE(ABILITY_MOXIE); };

ABILITY(ABILITY_JUSTIFIED) : extends AbsorbStatUp<TYPE_DARK, STAT_HIGHEST_ATTACKING> { INSTANCE(ABILITY_JUSTIFIED); };

ABILITY(ABILITY_RATTLED) : extends OnDefender {
    INSTANCE(ABILITY_RATTLED);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
        CHECK(CanRaiseStat(battler, STAT_SPEED))

        SetStatChanger(STAT_SPEED, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

ABILITY(ABILITY_MAGIC_BOUNCE) : extends Breakable { INSTANCE(ABILITY_MAGIC_BOUNCE); };

ABILITY(ABILITY_SAP_SIPPER) : extends LightningRodClone<TYPE_GRASS> { INSTANCE(ABILITY_SAP_SIPPER); };

ABILITY(ABILITY_PRANKSTER) : extends OnPriority {
    INSTANCE(ABILITY_PRANKSTER);
    ON_PRIORITY {
        CHECK(IS_MOVE_STATUS(move))
        return 1;
    }
};

ABILITY(ABILITY_SAND_FORCE) : extends SandImmune, extends OnStat<> {
    INSTANCE(ABILITY_SAND_FORCE);
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_IRON_BARBS) : extends AbilityImpl<ABILITY_ROUGH_SKIN> { INSTANCE(ABILITY_IRON_BARBS); };

ABILITY(ABILITY_ZEN_MODE) : extends StandardTransformation { INSTANCE(ABILITY_ZEN_MODE); };

ABILITY(ABILITY_VICTORY_STAR) : extends OnAccuracy<ApplyOnTarget::ALLY> {
    INSTANCE(ABILITY_VICTORY_STAR);
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

ABILITY(ABILITY_TURBOBLAZE) : extends AbilityImpl<ABILITY_MOLD_BREAKER>, extends AddsType<TYPE_FIRE> {
    INSTANCE(ABILITY_TURBOBLAZE);
    ON_ENTRY { return AddsType<TYPE_FIRE>::onEntry(DELEGATE_ENTRY); }
};

ABILITY(ABILITY_TERAVOLT) : extends AbilityImpl<ABILITY_MOLD_BREAKER>, extends AddsType<TYPE_ELECTRIC> {
    INSTANCE(ABILITY_TERAVOLT);
    ON_ENTRY { return AddsType<TYPE_ELECTRIC>::onEntry(DELEGATE_ENTRY); }
};

ABILITY(ABILITY_AROMA_VEIL) : extends OnStatusImmune<ApplyOn::ALLY> {
    INSTANCE(ABILITY_AROMA_VEIL);
    ON_STATUS_IMMUNE {
        CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK))
        return TRUE;
    }
};

ABILITY(ABILITY_FLOWER_VEIL) : extends OnStatusImmune<ApplyOn::ALLY> {
    INSTANCE(ABILITY_FLOWER_VEIL);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
        return TRUE;
    }
};

ABILITY(ABILITY_CHEEK_POUCH) : extends RandomizerBanned { INSTANCE(ABILITY_CHEEK_POUCH); };

ABILITY(ABILITY_PROTEAN) : extends OnBeforeAttack<> {
    INSTANCE(ABILITY_PROTEAN);
    ON_BEFORE_ATTACK {
        CHECK(CheckAndSetOncePerTurnAbility(battler, ability))
        CHECK_NOT(IS_BATTLER_OF_TYPE(battler, moveType))
        CHECK(move != MOVE_STRUGGLE)
        SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        BattleScriptCall(BattleScript_ProteanActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_FUR_COAT) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_FUR_COAT);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.5);
    }
};

ABILITY(ABILITY_BULLETPROOF) : extends OnImmune<> {
    INSTANCE(ABILITY_BULLETPROOF);
    ON_IMMUNE {
        CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

ABILITY(ABILITY_STRONG_JAW) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_STRONG_JAW);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
    }
};

ABILITY(ABILITY_REFRIGERATE) : extends AteAbility<TYPE_ICE> { INSTANCE(ABILITY_REFRIGERATE); };

ABILITY(ABILITY_SWEET_VEIL) : extends OnStatusImmune<ApplyOn::ALLY> {
    INSTANCE(ABILITY_SWEET_VEIL);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_SLEEP)
        return TRUE;
    }
};

ABILITY(ABILITY_STANCE_CHANGE) : extends FormChangeAbility, extends OnBeforeAttack<> {
    INSTANCE(ABILITY_STANCE_CHANGE);
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
ABILITY(ABILITY_GALE_WINGS) : extends GaleWingsLike<TYPE_FLYING> { INSTANCE(ABILITY_GALE_WINGS); };

ABILITY(ABILITY_MEGA_LAUNCHER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_MEGA_LAUNCHER);
    ON_OFFENSIVE_MULTIPLIER {
        if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
    }
};

ABILITY(ABILITY_GRASS_PELT) : extends OnStat<> {
    INSTANCE(ABILITY_GRASS_PELT);
    ON_STAT {
        if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_TOUGH_CLAWS) : extends AbilityImpl<ABILITY_BIG_PECKS> { INSTANCE(ABILITY_TOUGH_CLAWS); };

ABILITY(ABILITY_PIXILATE) : extends AteAbility<TYPE_FAIRY> { INSTANCE(ABILITY_PIXILATE); };

ABILITY(ABILITY_GOOEY) : extends OnDefender {
    INSTANCE(ABILITY_GOOEY);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
        CHECK(IsMoveMakingContact(move, attacker))

        BattleScriptCall(BattleScript_GooeyActivates);
        gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
        return TRUE;
    }
};

ABILITY(ABILITY_AERILATE) : extends AteAbility<TYPE_FLYING> { INSTANCE(ABILITY_AERILATE); };

ABILITY(ABILITY_HYPER_AGGRESSIVE) : extends OnParentalBond {
    INSTANCE(ABILITY_HYPER_AGGRESSIVE);
    ON_PARENTAL_BOND { return PARENTAL_BOND_HYPER_AGGRESSIVE; }
};

struct IgnoresFortKnox {};
ABILITY(ABILITY_PARENTAL_BOND) : extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE>, extends IgnoresFortKnox { INSTANCE(ABILITY_PARENTAL_BOND); };

ABILITY(ABILITY_DARK_AURA) : extends OnEntry, extends OnOffensiveMultiplier<ApplyOn::ANY> {
    INSTANCE(ABILITY_DARK_AURA);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA); }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType != TYPE_DARK) return;
        if (IsAbilityOnField(ABILITY_AURA_BREAK))
            MUL(.75);
        else
            MUL(1.33);
    }
};

ABILITY(ABILITY_FAIRY_AURA) : extends OnEntry, extends OnOffensiveMultiplier<ApplyOn::ANY> {
    INSTANCE(ABILITY_FAIRY_AURA);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA); }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType != TYPE_FAIRY) return;
        if (IsAbilityOnField(ABILITY_AURA_BREAK))
            MUL(.75);
        else
            MUL(1.33);
    }
};

ABILITY(ABILITY_AURA_BREAK) : extends Breakable, extends OnEntry {
    INSTANCE(ABILITY_AURA_BREAK);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); }
};

ABILITY(ABILITY_PRIMORDIAL_SEA) : extends OnEntry {
    INSTANCE(ABILITY_PRIMORDIAL_SEA);
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_DESOLATE_LAND) : extends OnEntry {
    INSTANCE(ABILITY_DESOLATE_LAND);
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_WEATHER_CONTROL) : extends OnImmune<> {
    INSTANCE(ABILITY_WEATHER_CONTROL);
    ON_IMMUNE {
        CHECK(gBattleMoves[move].flags & FLAG_WEATHER_BASED)
        CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER)
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

ABILITY(ABILITY_DELTA_STREAM) : extends AbilityImpl<ABILITY_WEATHER_CONTROL>, extends OverrideBreakable, extends OnEntry {
    INSTANCE(ABILITY_DELTA_STREAM);
    ON_ENTRY {
        CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))

        BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_STAMINA) : extends OnDefender {
    INSTANCE(ABILITY_STAMINA);
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

ABILITY(ABILITY_WIMP_OUT) : extends OnDefender {
    INSTANCE(ABILITY_WIMP_OUT);
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(TestSheerForceFlag(attacker, gCurrentMove))
        CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
        CHECK(CountUsablePartyMons(battler));
        gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
        return FALSE;
    }
};

ABILITY(ABILITY_EMERGENCY_EXIT) : extends AbilityImpl<ABILITY_WIMP_OUT> { INSTANCE(ABILITY_EMERGENCY_EXIT); };

ABILITY(ABILITY_WATER_COMPACTION) : extends OnDefensiveMultiplier, extends OnDefender {
    INSTANCE(ABILITY_WATER_COMPACTION);
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

ABILITY(ABILITY_MERCILESS) : extends OnCrit<> {
    INSTANCE(ABILITY_MERCILESS);
    ON_CRIT {
        if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
        if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
        if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
        if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
        return 0;
    }
};

ABILITY(ABILITY_SHIELDS_DOWN) : extends StandardTransformation, extends OnAttacker, extends OnStatusImmune<>, extends OverrideBreakable {
    INSTANCE(ABILITY_SHIELDS_DOWN);
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

ABILITY(ABILITY_STAKEOUT) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_STAKEOUT);
    ON_OFFENSIVE_MULTIPLIER {
        if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
    }
};

ABILITY(ABILITY_WATER_BUBBLE) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_WATER_BUBBLE);
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

ABILITY(ABILITY_STEELWORKER) : extends Breakable, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends AteAbility<TYPE_STEEL> {
    INSTANCE(ABILITY_STEELWORKER);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
    }
};

ABILITY(ABILITY_BERSERK) : extends OnDefender {
    INSTANCE(ABILITY_BERSERK);
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        int stat = GetHighestAttackingStatId(battler, TRUE);
        CHECK(CanRaiseStat(battler, stat))

        SetAbilityState(battler, ability, TRUE);
        SetStatChanger(stat, 1);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

ABILITY(ABILITY_SLUSH_RUSH) : extends HailImmune, extends OnStat<> {
    INSTANCE(ABILITY_SLUSH_RUSH);
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_LONG_REACH) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_LONG_REACH);
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
ABILITY(ABILITY_LIQUID_VOICE) : extends LiquidVoiceClone<TYPE_WATER> { INSTANCE(ABILITY_LIQUID_VOICE); };

ABILITY(ABILITY_TRIAGE) : extends OnPriority {
    INSTANCE(ABILITY_TRIAGE);
    ON_PRIORITY {
        CHECK(IsHealingMoveEffect(gBattleMoves[move].effect))
        return 3;
    }
};

ABILITY(ABILITY_GALVANIZE) : extends AteAbility<TYPE_ELECTRIC> { INSTANCE(ABILITY_GALVANIZE); };

ABILITY(ABILITY_SURGE_SURFER) : extends OnStat<> {
    INSTANCE(ABILITY_SURGE_SURFER);
    ON_STAT {
        if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_SCHOOLING) : extends StandardTransformation {
    INSTANCE(ABILITY_SCHOOLING);
    ON_ENTRY {
        CHECK(gBattleMons[battler].level >= 20)
        return StandardTransformation::onEntry(DELEGATE_ENTRY);
    }
    ON_END_TURN {
        CHECK(gBattleMons[battler].level >= 20)
        return StandardTransformation::onEndTurn(DELEGATE_END_TURN);
    }
};

ABILITY(ABILITY_DISGUISE) : extends FormChangeAbility, extends OnEntry, extends OnDisguise, extends OnWeather {
    INSTANCE(ABILITY_DISGUISE);
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

ABILITY(ABILITY_BATTLE_BOND) : extends FormChangeAbility, extends OnBattlerFaints<> {
    INSTANCE(ABILITY_BATTLE_BOND);
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

ABILITY(ABILITY_POWER_CONSTRUCT) : extends FormChangeAbility, extends OnEndTurn {
    INSTANCE(ABILITY_POWER_CONSTRUCT);
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

ABILITY(ABILITY_CORROSION) : extends OnTypeEffectiveness<>, extends OnCanStatusType {
    INSTANCE(ABILITY_CORROSION);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_POISON)
        CHECK(defType == TYPE_STEEL)
        *mod = UQ_4_12(2.0);
        return TRUE;
    }
    ON_CAN_STATUS_TYPE {
        CHECK(status & CHECK_POISON)
        return TRUE;
    }
};

ABILITY(ABILITY_COMATOSE) : extends OnEntry, extends RemovesStatusOnImmunity, extends Unsuppressable {
    INSTANCE(ABILITY_COMATOSE);
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
ABILITY(ABILITY_QUEENLY_MAJESTY) : extends OnImmune<ApplyOn::ALLY> {
    INSTANCE(ABILITY_QUEENLY_MAJESTY);
    ON_IMMUNE { return blocksPriority(DELEGATE_IMMUNE); }
};

ABILITY(ABILITY_INNARDS_OUT) : extends OnDefender {
    INSTANCE(ABILITY_INNARDS_OUT);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK_NOT(IsMagicGuardProtected(attacker))

        gBattleMoveDamage = gTurnStructs[battler].dmg;
        BattleScriptCall(BattleScript_AftermathDmg);
        return TRUE;
    }
};

ABILITY(ABILITY_DANCER) : extends OnCopyMove {
    INSTANCE(ABILITY_DANCER);
    ON_COPY_MOVE {
        CHECK(IsDance(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    }
};

ABILITY(ABILITY_BATTERY) : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    INSTANCE(ABILITY_BATTERY);
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(1.3);
    }
};

ABILITY(ABILITY_FLUFFY) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_FLUFFY);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(2.0);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

ABILITY(ABILITY_DAZZLING) : extends AbilityImpl<ABILITY_QUEENLY_MAJESTY> { INSTANCE(ABILITY_DAZZLING); };

ABILITY(ABILITY_SOUL_HEART) : extends OnBattlerFaints<ApplyOnTarget::ANY> {
    INSTANCE(ABILITY_SOUL_HEART);
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

ABILITY(ABILITY_TANGLING_HAIR) : extends AbilityImpl<ABILITY_GOOEY> { INSTANCE(ABILITY_TANGLING_HAIR); };

ABILITY(ABILITY_RECEIVER) : extends OnBattlerFaints<ApplyOnTarget::ALLY> {
    INSTANCE(ABILITY_RECEIVER);
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

ABILITY(ABILITY_POWER_OF_ALCHEMY) : extends OnEntry, extends OnReactive, extends OnBattlerFaints<ApplyOnTarget::ANY> {
    INSTANCE(ABILITY_POWER_OF_ALCHEMY);
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

ABILITY(ABILITY_BEAST_BOOST) : extends MoxieClone<STAT_HIGHEST_TOTAL> { INSTANCE(ABILITY_BEAST_BOOST); };

ABILITY(ABILITY_RKS_SYSTEM) : extends AbilityImpl<ABILITY_PROTEAN>, extends AbilityImpl<ABILITY_ADAPTABILITY>, extends FormChangeAbility {
    INSTANCE(ABILITY_RKS_SYSTEM);
};

ABILITY(ABILITY_ELECTRIC_SURGE) : extends AllowTerrainIfAirborne<TERRAIN_ELECTRIC>, extends OnEntry {
    INSTANCE(ABILITY_ELECTRIC_SURGE);
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

ABILITY(ABILITY_PSYCHIC_SURGE) : extends AllowTerrainIfAirborne<TERRAIN_PSYCHIC>, extends OnEntry {
    INSTANCE(ABILITY_PSYCHIC_SURGE);
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_MISTY_SURGE) : extends AllowTerrainIfAirborne<TERRAIN_MISTY>, extends OnEntry {
    INSTANCE(ABILITY_MISTY_SURGE);
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_GRASSY_SURGE) : extends AllowTerrainIfAirborne<TERRAIN_GRASSY>, extends OnEntry {
    INSTANCE(ABILITY_GRASSY_SURGE);
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_SHADOW_SHIELD) : extends AbilityImpl<ABILITY_MULTISCALE>, extends OverrideBreakable { INSTANCE(ABILITY_SHADOW_SHIELD); };

ABILITY(ABILITY_PRISM_ARMOR) : extends AbilityImpl<ABILITY_FILTER>, extends OverrideBreakable { INSTANCE(ABILITY_PRISM_ARMOR); };

ABILITY(ABILITY_NEUROFORCE) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_NEUROFORCE);
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
ABILITY(ABILITY_INTREPID_SWORD) : extends RaiseStatOnEntry<STAT_ATK> { INSTANCE(ABILITY_INTREPID_SWORD); };

ABILITY(ABILITY_DAUNTLESS_SHIELD) : extends RaiseStatOnEntry<STAT_DEF> { INSTANCE(ABILITY_DAUNTLESS_SHIELD); };

ABILITY(ABILITY_LIBERO) : extends AbilityImpl<ABILITY_PROTEAN> { INSTANCE(ABILITY_LIBERO); };

ABILITY(ABILITY_COTTON_DOWN) : extends OnDefender {
    INSTANCE(ABILITY_COTTON_DOWN);
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

ABILITY(ABILITY_MIRROR_ARMOR) : extends Breakable { INSTANCE(ABILITY_MIRROR_ARMOR); };

ABILITY(ABILITY_GULP_MISSILE) : extends FormChangeAbility, extends OnDefender, extends OnAttacker {
    INSTANCE(ABILITY_GULP_MISSILE);
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

ABILITY(ABILITY_STEAM_ENGINE) : extends OnDefender {
    INSTANCE(ABILITY_STEAM_ENGINE);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_SPEED))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

        SetStatChanger(STAT_SPEED, 12);
        BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
        return TRUE;
    }
};

ABILITY(ABILITY_AMPLIFIER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_AMPLIFIER);
    ON_OFFENSIVE_MULTIPLIER {
        if (IsSoundMove(battler, move)) MUL(1.3);
    }
};

ABILITY(ABILITY_PUNK_ROCK) : extends OnDefensiveMultiplier, extends AbilityImpl<ABILITY_AMPLIFIER> {
    INSTANCE(ABILITY_PUNK_ROCK);
    ON_DEFENSIVE_MULTIPLIER {
        if (IsSoundMove(attacker, move)) MUL(.5);
    }
};

ABILITY(ABILITY_SAND_SPIT) : extends SandImmune, extends OnDefender {
    INSTANCE(ABILITY_SAND_SPIT);
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

ABILITY(ABILITY_ICE_SCALES) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_ICE_SCALES);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.5);
    }
};

ABILITY(ABILITY_ICE_FACE) : extends FormChangeAbility, extends HailImmune, extends OnEntry, extends OnDisguise, extends OnWeather {
    INSTANCE(ABILITY_ICE_FACE);
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

ABILITY(ABILITY_POWER_SPOT) : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    INSTANCE(ABILITY_POWER_SPOT);
    ON_OFFENSIVE_MULTIPLIER { MUL(1.3); }
};

ABILITY(ABILITY_MIMICRY) : extends OnEntry, extends OnTerrain {
    INSTANCE(ABILITY_MIMICRY);
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

ABILITY(ABILITY_SCREEN_CLEANER) : extends OnEntry {
    INSTANCE(ABILITY_SCREEN_CLEANER);
    ON_ENTRY {
        CHECK(TryRemoveScreens(battler))

        return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
    }
};

ABILITY(ABILITY_STEELY_SPIRIT) : extends OnOffensiveMultiplier<ApplyOn::ALLY> {
    INSTANCE(ABILITY_STEELY_SPIRIT);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_STEEL) MUL(1.3);
    }
};

ABILITY(ABILITY_PERISH_BODY) : extends OnDefender {
    INSTANCE(ABILITY_PERISH_BODY);
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

ABILITY(ABILITY_WANDERING_SPIRIT) : extends OnDefender {
    INSTANCE(ABILITY_WANDERING_SPIRIT);
    ON_DEFENDER {
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
    }
};

ABILITY(ABILITY_GORILLA_TACTICS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_GORILLA_TACTICS);
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
    }
};

ABILITY(ABILITY_NEUTRALIZING_GAS) : extends Unsuppressable { INSTANCE(ABILITY_NEUTRALIZING_GAS); };

ABILITY(ABILITY_PASTEL_VEIL) : extends OnEntry {
    INSTANCE(ABILITY_PASTEL_VEIL);
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

ABILITY(ABILITY_HUNGER_SWITCH) : extends FormChangeAbility, extends OnEndTurn {
    INSTANCE(ABILITY_HUNGER_SWITCH);
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

ABILITY(ABILITY_CURIOUS_MEDICINE) : extends OnEntry {
    INSTANCE(ABILITY_CURIOUS_MEDICINE);
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

ABILITY(ABILITY_TRANSISTOR) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_TRANSISTOR);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.5);
    }
};

ABILITY(ABILITY_DRAGONS_MAW) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_DRAGONS_MAW);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DRAGON) MUL(1.5);
    }
};

ABILITY(ABILITY_CHILLING_NEIGH) : extends AbilityImpl<ABILITY_MOXIE> { INSTANCE(ABILITY_CHILLING_NEIGH); };

ABILITY(ABILITY_GRIM_NEIGH) : extends MoxieClone<STAT_SPATK> { INSTANCE(ABILITY_GRIM_NEIGH); };

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

ABILITY(ABILITY_AS_ONE_ICE_RIDER) : extends AsOne<ABILITY_CHILLING_NEIGH> { INSTANCE(ABILITY_AS_ONE_ICE_RIDER); };

ABILITY(ABILITY_AS_ONE_SHADOW_RIDER) : extends AsOne<ABILITY_GRIM_NEIGH> { INSTANCE(ABILITY_AS_ONE_SHADOW_RIDER); };

ABILITY(ABILITY_CHLOROPLAST) { INSTANCE(ABILITY_CHLOROPLAST); };

ABILITY(ABILITY_WHITEOUT) : extends HailImmune, extends OnStat<> {
    INSTANCE(ABILITY_WHITEOUT);
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_PYROMANCY) : extends OnModifyEffectChance<> {
    INSTANCE(ABILITY_PYROMANCY);
    ON_MODIFY_EFFECT_CHANCE {
        if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
    }
};

ABILITY(ABILITY_KEEN_EDGE) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_KEEN_EDGE);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
    }
};

ABILITY(ABILITY_PRISM_SCALES) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_PRISM_SCALES);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.7);
    }
};

ABILITY(ABILITY_POWER_FISTS) : extends AbilityImpl<ABILITY_IRON_FIST>, extends OnChooseDefensiveStat<> {
    INSTANCE(ABILITY_POWER_FISTS);
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(IsIronFistBoosted(battler, move))
        return STAT_SPDEF;
    }
};

ABILITY(ABILITY_SAND_SONG) : extends LiquidVoiceClone<TYPE_GROUND> { INSTANCE(ABILITY_SAND_SONG); };

ABILITY(ABILITY_RAMPAGE) : extends OnBattlerFaints<> {
    INSTANCE(ABILITY_RAMPAGE);
    ON_BATTLER_FAINTS {
        SetAbilityState(battler, ability, TRUE);
        gVolatileStructs[battler].rechargeTimer = 0;
        gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
        return FALSE;
    }
};

ABILITY(ABILITY_VENGEANCE) : extends SwarmLike<TYPE_GHOST> { INSTANCE(ABILITY_VENGEANCE); };

ABILITY(ABILITY_BLITZ_BOXER) : extends OnPriority {
    INSTANCE(ABILITY_BLITZ_BOXER);
    ON_PRIORITY {
        CHECK(IsIronFistBoosted(battler, move))
        CHECK(BATTLER_MAX_HP(battler));
        return 1;
    }
};

ABILITY(ABILITY_ANTARCTIC_BIRD) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ANTARCTIC_BIRD);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
    }
};

ABILITY(ABILITY_IMMOLATE) : extends AteAbility<TYPE_FIRE> { INSTANCE(ABILITY_IMMOLATE); };

ABILITY(ABILITY_CRYSTALLIZE) : extends OnOffensiveMultiplier<>, extends OnMoveType {
    INSTANCE(ABILITY_CRYSTALLIZE);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ICE && gBattleStruct->ateBoost[battler]) MUL(1.1);
    }
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_ROCK)
        *ateBoost = TRUE;
        return TYPE_ICE + 1;
    }
};

ABILITY(ABILITY_ELECTROCYTES) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ELECTROCYTES);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.25);
    }
};

ABILITY(ABILITY_AERODYNAMICS) : extends AbsorbStatUp<TYPE_FLYING, STAT_SPEED> { INSTANCE(ABILITY_AERODYNAMICS); };

ABILITY(ABILITY_CHRISTMAS_SPIRIT) : extends OnDefensiveMultiplier, extends HailImmune {
    INSTANCE(ABILITY_CHRISTMAS_SPIRIT);
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
    }
};

ABILITY(ABILITY_EXPLOIT_WEAKNESS) : extends OnOffensiveMultiplier<>, extends OnChooseDefensiveStat<> {
    INSTANCE(ABILITY_EXPLOIT_WEAKNESS);
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

ABILITY(ABILITY_GROUND_SHOCK) : extends OnTypeEffectiveness<> {
    INSTANCE(ABILITY_GROUND_SHOCK);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_GROUND)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(.5);
        return TRUE;
    }
};

ABILITY(ABILITY_ANCIENT_IDOL) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_ANCIENT_IDOL);
    ON_CHOOSE_OFFENSIVE_STAT { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; }
};

ABILITY(ABILITY_MYSTIC_POWER) : extends OnStab {
    INSTANCE(ABILITY_MYSTIC_POWER);
    ON_STAB { return TRUE; }
};

ABILITY(ABILITY_PERFECTIONIST) : extends OnPriority, extends OnCrit<> {
    INSTANCE(ABILITY_PERFECTIONIST);
    ON_PRIORITY {
        CHECK(gBattleMoves[move].power <= 25)
        CHECK(gBattleMoves[move].power);
        return 1;
    }
    ON_CRIT {
        CHECK(gBattleMoves[move].power <= 50)
        CHECK(gBattleMoves[move].power)
        return 1;
    }
};

ABILITY(ABILITY_GROWING_TOOTH) : extends OnAttacker {
    INSTANCE(ABILITY_GROWING_TOOTH);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        gBattleScripting.battler = battler;
        BattleScriptCall(BattleScript_AttackBoostActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_INFLATABLE) : extends OnDefender {
    INSTANCE(ABILITY_INFLATABLE);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF))
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
        BattleScriptCall(BattleScript_InflatableActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

ABILITY(ABILITY_AURORA_BOREALIS) : extends HailImmune, extends OnStab {
    INSTANCE(ABILITY_AURORA_BOREALIS);
    ON_STAB { return moveType == TYPE_ICE; }
};

ABILITY(ABILITY_AVENGER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_AVENGER);
    ON_OFFENSIVE_MULTIPLIER {
        if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
    }
};

ABILITY(ABILITY_LETS_ROLL) : extends OnEntry {
    INSTANCE(ABILITY_LETS_ROLL);
    ON_ENTRY {
        CHECK(CanRaiseStat(battler, STAT_DEF))

        SetStatChanger(STAT_DEF, 1);
        gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
        return TRUE;
    }
};

ABILITY(ABILITY_LOUD_BANG) : extends OnAttacker {
    INSTANCE(ABILITY_LOUD_BANG);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeConfused(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
    }
};

ABILITY(ABILITY_LEAD_COAT) : extends OnDefensiveMultiplier, extends OnStat<> {
    INSTANCE(ABILITY_LEAD_COAT);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.6);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .9;
    }
};

ABILITY(ABILITY_AMPHIBIOUS) : extends OnStab, extends OnStatusImmune<> {
    INSTANCE(ABILITY_AMPHIBIOUS);
    ON_STAB { return moveType == TYPE_WATER; }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_DRENCH)
        return TRUE;
    }
};

ABILITY(ABILITY_GROUNDED) : extends AddsType<TYPE_GROUND> { INSTANCE(ABILITY_GROUNDED); };

ABILITY(ABILITY_EARTHBOUND) : extends SwarmLike<TYPE_GROUND> { INSTANCE(ABILITY_EARTHBOUND); };

ABILITY(ABILITY_FIGHT_SPIRIT) : extends AteAbility<TYPE_FIGHTING> { INSTANCE(ABILITY_FIGHT_SPIRIT); };

ABILITY(ABILITY_COIL_UP) : extends OnEntry {
    INSTANCE(ABILITY_COIL_UP);
    ON_ENTRY {
        CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

        gStatuses4[battler] |= STATUS4_COILED;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
        return TRUE;
    }
};

ABILITY(ABILITY_FOSSILIZED) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_FOSSILIZED);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK) RESISTANCE(.5);
    }
};

ABILITY(ABILITY_MAGICAL_DUST) : extends OnDefender {
    INSTANCE(ABILITY_MAGICAL_DUST);
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

ABILITY(ABILITY_DREAMCATCHER) : extends OnOffensiveMultiplier<>, extends UseTurnAttackAsPursuit {
    INSTANCE(ABILITY_DREAMCATCHER);
    ON_OFFENSIVE_MULTIPLIER {
        for (int i = 0; i < gBattlersCount; i++) {
            if (IsBattlerAlive(i) && gBattleMons[i].status1 & STATUS1_SLEEP) {
                FILTER_NOT(gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability && gQueuedExtraAttackData[0].target == i)
                MUL(2.0);
                return;
            }
        }
    }
    ON_PREEMPT_ACTION {
        CHECK(gBattleMons[turnBattler].status1 & STATUS1_SLEEP)
        return UseTurnAttackAsPursuit::onPreemptAction(DELEGATE_PREEMPT_ACTION);
    }
};

ABILITY(ABILITY_NOCTURNAL) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_NOCTURNAL);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK) MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
    }
};

ABILITY(ABILITY_SELF_SUFFICIENT) : extends OnEndTurn {
    INSTANCE(ABILITY_SELF_SUFFICIENT);
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

ABILITY(ABILITY_TECTONIZE) : extends AteAbility<TYPE_GROUND> { INSTANCE(ABILITY_TECTONIZE); };

ABILITY(ABILITY_ICE_AGE) : extends AddsType<TYPE_ICE> { INSTANCE(ABILITY_ICE_AGE); };

ABILITY(ABILITY_HALF_DRAKE) : extends AddsType<TYPE_DRAGON> { INSTANCE(ABILITY_HALF_DRAKE); };

ABILITY(ABILITY_AQUATIC) : extends AddsType<TYPE_WATER> { INSTANCE(ABILITY_AQUATIC); };

ABILITY(ABILITY_LIQUIFIED) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_LIQUIFIED);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) RESISTANCE(2);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

ABILITY(ABILITY_DRAGONFLY) : extends AbilityImpl<ABILITY_HALF_DRAKE>, extends GroundImmune { INSTANCE(ABILITY_DRAGONFLY); };

template <Type StrongVs>
struct TypeSlayer : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(target, StrongVs)) RESISTANCE(1.5);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(attacker, StrongVs)) MUL(.5);
    }
};

ABILITY(ABILITY_DRAGONSLAYER) : extends TypeSlayer<TYPE_DRAGON> { INSTANCE(ABILITY_DRAGONSLAYER); };

struct StealthRockImmune {};
ABILITY(ABILITY_MOUNTAINEER) : extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends StealthRockImmune {
    INSTANCE(ABILITY_MOUNTAINEER);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_ROCK) *mod = 0;
    }
};

ABILITY(ABILITY_HYDRATE) : extends AteAbility<TYPE_WATER> { INSTANCE(ABILITY_HYDRATE); };

ABILITY(ABILITY_METALLIC) : extends AddsType<TYPE_STEEL> { INSTANCE(ABILITY_METALLIC); };

ABILITY(ABILITY_PERMAFROST) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_PERMAFROST);
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
    }
};

ABILITY(ABILITY_PRIMAL_ARMOR) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_PRIMAL_ARMOR);
    ON_DEFENSIVE_MULTIPLIER {
        if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
    }
};

ABILITY(ABILITY_RAGING_BOXER) : extends OnParentalBond {
    INSTANCE(ABILITY_RAGING_BOXER);
    ON_PARENTAL_BOND {
        CHECK(IsIronFistBoosted(battler, move))
        return PARENTAL_BOND_PRIMAL_MAW;
    }
};

ABILITY(ABILITY_AIR_BLOWER) : extends OnEntry {
    INSTANCE(ABILITY_AIR_BLOWER);
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

ABILITY(ABILITY_JUGGERNAUT) : extends OnChooseOffensiveStat, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_JUGGERNAUT);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    }
};

ABILITY(ABILITY_SHORT_CIRCUIT) : extends SwarmLike<TYPE_ELECTRIC> { INSTANCE(ABILITY_SHORT_CIRCUIT); };

ABILITY(ABILITY_MAJESTIC_BIRD) : extends OnStat<> {
    INSTANCE(ABILITY_MAJESTIC_BIRD);
    ON_STAT {
        if (statId == STAT_SPATK) *stat *= 1.5;
    }
};

ABILITY(ABILITY_PHANTOM) : extends AddsType<TYPE_GHOST> { INSTANCE(ABILITY_PHANTOM); };

ABILITY(ABILITY_INTOXICATE) : extends AteAbility<TYPE_POISON> { INSTANCE(ABILITY_INTOXICATE); };

ABILITY(ABILITY_IMPENETRABLE) : extends AbilityImpl<ABILITY_MAGIC_GUARD> { INSTANCE(ABILITY_IMPENETRABLE); };

ABILITY(ABILITY_HYPNOTIST) : extends OnAccuracy<> {
    INSTANCE(ABILITY_HYPNOTIST);
    ON_ACCURACY {
        CHECK(move == MOVE_HYPNOSIS);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_OVERWHELM) : extends OnTypeEffectiveness<>, extends TauntImmune {
    INSTANCE(ABILITY_OVERWHELM);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

ABILITY(ABILITY_SCARE) : extends AbilityImpl<ABILITY_INTIMIDATE> { INSTANCE(ABILITY_SCARE); };

ABILITY(ABILITY_MAJESTIC_MOTH) : extends OnEntry {
    INSTANCE(ABILITY_MAJESTIC_MOTH);
    ON_ENTRY {
        CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

ABILITY(ABILITY_SOUL_EATER) : extends OnBattlerFaints<> {
    INSTANCE(ABILITY_SOUL_EATER);
    ON_BATTLER_FAINTS {
        CHECK_NOT(BATTLER_MAX_HP(battler));
        CHECK(CanBattlerHeal(battler));
        BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    }
};

ABILITY(ABILITY_SOUL_LINKER) : extends OnEither {
    INSTANCE(ABILITY_SOUL_LINKER);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(IsBattlerAlive(battler))
        CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER))
        CHECK(move != MOVE_PAIN_SPLIT)

        BattleScriptCall(BattleScript_AttackerSoulLinker);
        return TRUE;
    }
};

ABILITY(ABILITY_SWEET_DREAMS) : extends OnEndTurn {
    INSTANCE(ABILITY_SWEET_DREAMS);
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

ABILITY(ABILITY_BAD_LUCK) : extends Breakable, extends OnCrit<ApplyOnTarget::FOE>, extends OnModifyEffectChance<ApplyOn::FOE> {
    INSTANCE(ABILITY_BAD_LUCK);
    ON_CRIT { return NEVER_CRIT; }
    ON_MODIFY_EFFECT_CHANCE {
        if (*effectChance < 1) *effectChance = 0;
    }
};

ABILITY(ABILITY_HAUNTED_SPIRIT) : extends OnDefender {
    INSTANCE(ABILITY_HAUNTED_SPIRIT);
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

ABILITY(ABILITY_ELECTRIC_BURST) : extends OnRecoil, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ELECTRIC_BURST);
    ON_RECOIL {
        CHECK(moveType == TYPE_ELECTRIC);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC) MUL(1.35);
    }
};

ABILITY(ABILITY_RAW_WOOD) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_RAW_WOOD);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS) RESISTANCE(.5);
    }
};

ABILITY(ABILITY_SOLENOGLYPHS) : extends OnAttacker {
    INSTANCE(ABILITY_SOLENOGLYPHS);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

ABILITY(ABILITY_SPIDER_LAIR) : extends OnEntry {
    INSTANCE(ABILITY_SPIDER_LAIR);
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

ABILITY(ABILITY_FATAL_PRECISION) : extends OnAccuracy<>, extends OnCrit<> {
    INSTANCE(ABILITY_FATAL_PRECISION);
    ON_ACCURACY {
        CHECK_NOT(IS_MOVE_STATUS(move))
        CHECK(CalcTypeEffectivenessMultiplier(move, moveType, battler, target, TRUE) >= UQ_4_12(2.0))
        return ACCURACY_HITS_IF_POSSIBLE;
    }
    ON_CRIT {
        CHECK(typeEffectiveness >= UQ_4_12(2.0))
        return ALWAYS_CRIT;
    }
};

ABILITY(ABILITY_SEAWEED) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_SEAWEED);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS)) RESISTANCE(0.5);
    }
};

ABILITY(ABILITY_PSYCHIC_MIND) : extends SwarmLike<TYPE_PSYCHIC> { INSTANCE(ABILITY_PSYCHIC_MIND); };

ABILITY(ABILITY_POISON_ABSORB) : extends Redirects<TYPE_POISON>, extends AbsorbHeal<TYPE_POISON>, extends OnEndTurn {
    INSTANCE(ABILITY_POISON_ABSORB);
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

ABILITY(ABILITY_SCAVENGER) : extends AbilityImpl<ABILITY_SOUL_EATER> { INSTANCE(ABILITY_SCAVENGER); };

ABILITY(ABILITY_TWISTED_DIMENSION) : extends OnEntry {
    INSTANCE(ABILITY_TWISTED_DIMENSION);
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

        gFieldTimers.started.trickRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
        return TRUE;
    }
};

ABILITY(ABILITY_MULTI_HEADED) : extends OnParentalBond, extends IgnoresFortKnox {
    INSTANCE(ABILITY_MULTI_HEADED);
    ON_PARENTAL_BOND {
        if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
        if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
        return MULTIHIT_SINGLE;
    }
};

ABILITY(ABILITY_NORTH_WIND) : extends HailImmune, extends OnEntry {
    INSTANCE(ABILITY_NORTH_WIND);
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

ABILITY(ABILITY_OVERCHARGE) : extends OnTypeEffectiveness<>, extends OnCanStatusType {
    INSTANCE(ABILITY_OVERCHARGE);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(defType == TYPE_ELECTRIC)
        *mod = UQ_4_12(2.0);
        return TRUE;
    }
    ON_CAN_STATUS_TYPE {
        CHECK(status & CHECK_PARALYSIS)
        return TRUE;
    }
};

ABILITY(ABILITY_VIOLENT_RUSH) : extends OnEntry {
    INSTANCE(ABILITY_VIOLENT_RUSH);
    ON_ENTRY {
        gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
    }
};

ABILITY(ABILITY_FLAMING_SOUL) : extends GaleWingsLike<TYPE_FIRE> { INSTANCE(ABILITY_FLAMING_SOUL); };

ABILITY(ABILITY_SAGE_POWER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_SAGE_POWER);
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(1.5);
    }
};

ABILITY(ABILITY_BONE_ZONE) : extends OnAfterTypeEffectiveness<> {
    INSTANCE(ABILITY_BONE_ZONE);
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

ABILITY(ABILITY_SPEED_FORCE) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_SPEED_FORCE);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
    }
};

ABILITY(ABILITY_SEA_GUARDIAN) : extends OnEntry {
    INSTANCE(ABILITY_SEA_GUARDIAN);
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        SetStatChanger(stat, 1);
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

ABILITY(ABILITY_MOLTEN_DOWN) : extends OnTypeEffectiveness<> {
    INSTANCE(ABILITY_MOLTEN_DOWN);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_FIRE)
        CHECK(defType == TYPE_ROCK)
        *mod = UQ_4_12(2.0);
        return TRUE;
    }
};

ABILITY(ABILITY_FLOCK) : extends SwarmLike<TYPE_FLYING> { INSTANCE(ABILITY_FLOCK); };

ABILITY(ABILITY_FIELD_EXPLORER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_FIELD_EXPLORER);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
    }
};

ABILITY(ABILITY_STRIKER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_STRIKER);
    ON_OFFENSIVE_MULTIPLIER {
        if (IsStrikerBoosted(battler, move)) MUL(1.3);
    }
};

ABILITY(ABILITY_FROZEN_SOUL) : extends GaleWingsLike<TYPE_ICE> { INSTANCE(ABILITY_FROZEN_SOUL); };

ABILITY(ABILITY_PREDATOR) : extends AbilityImpl<ABILITY_SOUL_EATER> { INSTANCE(ABILITY_PREDATOR); };

ABILITY(ABILITY_LOOTER) : extends AbilityImpl<ABILITY_SOUL_EATER> { INSTANCE(ABILITY_LOOTER); };

ABILITY(ABILITY_LUNAR_ECLIPSE) : extends AbilityImpl<ABILITY_HYPNOTIST>, extends OnStab {
    INSTANCE(ABILITY_LUNAR_ECLIPSE);
    ON_STAB { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; }
};

ABILITY(ABILITY_SOLAR_FLARE) : extends AbilityImpl<ABILITY_IMMOLATE>, extends AbilityImpl<ABILITY_CHLOROPLAST> { INSTANCE(ABILITY_SOLAR_FLARE); };

ABILITY(ABILITY_POWER_CORE) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_POWER_CORE);
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; }
};

ABILITY(ABILITY_SIGHTING_SYSTEM) : extends OnAccuracy<>, extends OnPriority {
    INSTANCE(ABILITY_SIGHTING_SYSTEM);
    ON_ACCURACY { return ACCURACY_HITS_IF_POSSIBLE; }
    ON_PRIORITY {
        CHECK(gBattleMoves[move].accuracy)
        CHECK(gBattleMoves[move].accuracy < 80);
        return -3;
    }
};

ABILITY(ABILITY_BAD_COMPANY) : extends RandomizerBanned { INSTANCE(ABILITY_BAD_COMPANY); };

ABILITY(ABILITY_OPPORTUNIST) : extends OnPriority {
    INSTANCE(ABILITY_OPPORTUNIST);
    ON_PRIORITY {
        CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2)
        return 1;
    }
};

ABILITY(ABILITY_GIANT_WINGS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_GIANT_WINGS);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].airBased) MUL(1.3);
    }
};

ABILITY(ABILITY_MOMENTUM) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_MOMENTUM);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
    }
};

ABILITY(ABILITY_GRIP_PINCER) : extends OnAttacker, extends OnAccuracy<> {
    INSTANCE(ABILITY_GRIP_PINCER);
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

ABILITY(ABILITY_BIG_LEAVES) : extends AbilityImpl<ABILITY_HARVEST>,
                   extends Merged<ABILITY_SOLAR_POWER, ABILITY_CHLOROPHYLL>,
                   extends Merged<ABILITY_HARVEST, ABILITY_LEAF_GUARD> { INSTANCE(ABILITY_BIG_LEAVES);};

ABILITY(ABILITY_PRECISE_FIST) : extends OnCrit<>, extends OnModifyEffectChance<> {
    INSTANCE(ABILITY_PRECISE_FIST);
    ON_CRIT {
        CHECK(IsIronFistBoosted(battler, move))
        return 1;
    }
    ON_MODIFY_EFFECT_CHANCE {
        if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
    }
};

ABILITY(ABILITY_DEADEYE) : extends OnAccuracy<>, extends OnChooseDefensiveStat<> {
    INSTANCE(ABILITY_DEADEYE);
    ON_ACCURACY {
        CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].arrowBased)
        return ACCURACY_HITS_IF_POSSIBLE;
    }
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

ABILITY(ABILITY_ARTILLERY) : extends OnAccuracy<> {
    INSTANCE(ABILITY_ARTILLERY);
    ON_ACCURACY {
        CHECK(IsMegaLauncherBoosted(battler, move))
        return ACCURACY_HITS_IF_POSSIBLE;
    }
};

ABILITY(ABILITY_ICE_DEW) : extends LightningRodClone<TYPE_ICE> { INSTANCE(ABILITY_ICE_DEW); };

ABILITY(ABILITY_SUN_WORSHIP) : extends OnEntry {
    INSTANCE(ABILITY_SUN_WORSHIP);
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

ABILITY(ABILITY_POLLINATE) : extends AteAbility<TYPE_BUG> { INSTANCE(ABILITY_POLLINATE); };

ABILITY(ABILITY_VOLCANO_RAGE) : extends OnAttacker {
    INSTANCE(ABILITY_VOLCANO_RAGE);
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
    }
};

ABILITY(ABILITY_COLD_REBOUND) : extends OnDefender {
    INSTANCE(ABILITY_COLD_REBOUND);
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
ABILITY(ABILITY_LOW_BLOW) : extends SimpleEntryMove<MOVE_FEINT_ATTACK, 40> { INSTANCE(ABILITY_LOW_BLOW); };

ABILITY(ABILITY_SPECTRALIZE) : extends AteAbility<TYPE_GHOST> { INSTANCE(ABILITY_SPECTRALIZE); };

ABILITY(ABILITY_SPECTRAL_SHROUD) : extends AbilityImpl<ABILITY_SPECTRALIZE>, extends OnAttacker {
    INSTANCE(ABILITY_SPECTRAL_SHROUD);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(gBattleStruct->ateBoost[battler])
        CHECK(moveType == TYPE_GHOST)
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

ABILITY(ABILITY_DISCIPLINE) : extends RemovesStatusOnImmunity, extends TauntImmune {
    INSTANCE(ABILITY_DISCIPLINE);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_CONFUSION)
        return TRUE;
    }
};

ABILITY(ABILITY_THUNDERCALL) : extends OnAttacker {
    INSTANCE(ABILITY_THUNDERCALL);
    ON_ATTACKER {
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
    }
};

ABILITY(ABILITY_MARINE_APEX) : extends AbilityImpl<ABILITY_INFILTRATOR>, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_MARINE_APEX);
    ON_OFFENSIVE_MULTIPLIER {
        if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
    }
};

ABILITY(ABILITY_MIGHTY_HORN) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_MIGHTY_HORN);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hornBased) MUL(1.3);
    }
};

ABILITY(ABILITY_HARDENED_SHEATH) : extends OnAttacker {
    INSTANCE(ABILITY_HARDENED_SHEATH);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].hornBased)
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

        BattleScriptCall(BattleScript_AttackBoostActivates);
        gBattleScripting.battler = battler;
        return TRUE;
    }
};

ABILITY(ABILITY_ARCTIC_FUR) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_ARCTIC_FUR);
    ON_DEFENSIVE_MULTIPLIER { MUL(.65); }
};

ABILITY(ABILITY_LETHARGY) : extends OnEntry, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_LETHARGY);
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

ABILITY(ABILITY_IRON_BARRAGE) : extends AbilityImpl<ABILITY_MEGA_LAUNCHER>, extends AbilityImpl<ABILITY_SIGHTING_SYSTEM> { INSTANCE(ABILITY_IRON_BARRAGE); };

ABILITY(ABILITY_STEEL_BARREL) : extends AbilityImpl<ABILITY_ROCK_HEAD> { INSTANCE(ABILITY_STEEL_BARREL); };

ABILITY(ABILITY_PYRO_SHELLS) : extends OnAttacker {
    INSTANCE(ABILITY_PYRO_SHELLS);
    ON_ATTACKER {
        CHECK(IsMegaLauncherBoosted(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
    }
};

ABILITY(ABILITY_FUNGAL_INFECTION) : extends OnAttacker {
    INSTANCE(ABILITY_FUNGAL_INFECTION);
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

ABILITY(ABILITY_PARRY) : extends OnDefender, extends OnDefensiveMultiplier, extends OverrideBreakable {
    INSTANCE(ABILITY_PARRY);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
        return FALSE;
    }
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
};

ABILITY(ABILITY_SCRAPYARD) : extends OnDefender {
    INSTANCE(ABILITY_SCRAPYARD);
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    }
};

ABILITY(ABILITY_LOOSE_QUILLS) : extends AbilityImpl<ABILITY_SCRAPYARD> { INSTANCE(ABILITY_LOOSE_QUILLS); };

ABILITY(ABILITY_TOXIC_DEBRIS) : extends OnDefender {
    INSTANCE(ABILITY_TOXIC_DEBRIS);
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

        BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
        return TRUE;
    }
};

ABILITY(ABILITY_ROUNDHOUSE) : extends OnAccuracy<>, extends OnChooseDefensiveStat<> {
    INSTANCE(ABILITY_ROUNDHOUSE);
    ON_ACCURACY {
        CHECK(IsStrikerBoosted(battler, move))
        return ACCURACY_HITS_IF_POSSIBLE;
    }
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

ABILITY(ABILITY_MINERALIZE) : extends AteAbility<TYPE_ROCK> { INSTANCE(ABILITY_MINERALIZE); };

ABILITY(ABILITY_LOOSE_ROCKS) : extends OnDefender {
    INSTANCE(ABILITY_LOOSE_ROCKS);
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

        BattleScriptCall(BattleScript_DefenderSetsStealthRock);
        return TRUE;
    }
};

ABILITY(ABILITY_SPINNING_TOP) : extends OnAttacker {
    INSTANCE(ABILITY_SPINNING_TOP);
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

ABILITY(ABILITY_RETRIBUTION_BLOW) : extends OnReactive {
    INSTANCE(ABILITY_RETRIBUTION_BLOW);
    ON_REACTIVE {
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
    }
};

ABILITY(ABILITY_FEARMONGER) : extends AbilityImpl<ABILITY_INTIMIDATE>, extends OnAttacker {
    INSTANCE(ABILITY_FEARMONGER);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(IsMoveMakingContact(move, battler))
        CHECK(Random() % 100 < 10)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
};

ABILITY(ABILITY_TOXIC_SPILL) : extends OnEntry, extends OnEndTurn, extends OnExit {
    INSTANCE(ABILITY_TOXIC_SPILL);
    ON_ENTRY {
        CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
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
            FILTER_NOT(IsMagicGuardProtected(target))
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

ABILITY(ABILITY_DESERT_CLOAK) : extends OnStatusImmune<ApplyOn::ALLY>, extends SandImmune {
    INSTANCE(ABILITY_DESERT_CLOAK);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY))
        return TRUE;
    }
};

ABILITY(ABILITY_DRACONIZE) : extends AteAbility<TYPE_DRAGON> { INSTANCE(ABILITY_DRACONIZE); };

ABILITY(ABILITY_PRETTY_PRINCESS) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_PRETTY_PRINCESS);
    ON_OFFENSIVE_MULTIPLIER {
        if (!IsUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
    }
};

ABILITY(ABILITY_SELF_REPAIR) : extends AbilityImpl<ABILITY_SELF_SUFFICIENT>, extends AbilityImpl<ABILITY_NATURAL_CURE> { INSTANCE(ABILITY_SELF_REPAIR); };

ABILITY(ABILITY_ELECTROMORPHOSIS) : extends OnDefender {
    INSTANCE(ABILITY_ELECTROMORPHOSIS);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_ATOMIC_BURST) : extends AbilityImpl<ABILITY_ELECTROMORPHOSIS>, extends AbilityImpl<ABILITY_GALVANIZE> { INSTANCE(ABILITY_ATOMIC_BURST); };

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
ABILITY(ABILITY_HELLBLAZE) : extends BoostedSwarmLike<TYPE_FIRE> { INSTANCE(ABILITY_HELLBLAZE); };

ABILITY(ABILITY_RIPTIDE) : extends BoostedSwarmLike<TYPE_WATER> { INSTANCE(ABILITY_RIPTIDE); };

ABILITY(ABILITY_FOREST_RAGE) : extends BoostedSwarmLike<TYPE_GRASS> { INSTANCE(ABILITY_FOREST_RAGE); };

ABILITY(ABILITY_PRIMAL_MAW) : extends OnParentalBond {
    INSTANCE(ABILITY_PRIMAL_MAW);
    ON_PARENTAL_BOND {
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        return PARENTAL_BOND_PRIMAL_MAW;
    }
};

ABILITY(ABILITY_SWEEPING_EDGE) : extends OnAccuracy<> {
    INSTANCE(ABILITY_SWEEPING_EDGE);
    ON_ACCURACY {
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        return ACCURACY_HITS_IF_POSSIBLE;
    }
};

ABILITY(ABILITY_GIFTED_MIND) : extends OnAccuracy<>, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_GIFTED_MIND);
    ON_ACCURACY {
        CHECK(IS_MOVE_STATUS(move))
        return ACCURACY_HITS_IF_POSSIBLE;
    }
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (moveType == TYPE_BUG || moveType == TYPE_GHOST || moveType == TYPE_DARK) *mod = 0;
    }
};

ABILITY(ABILITY_HYDRO_CIRCUIT) : extends AbilityImpl<ABILITY_TRANSISTOR>, extends OnAttacker {
    INSTANCE(ABILITY_HYDRO_CIRCUIT);
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

ABILITY(ABILITY_EQUINOX) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_EQUINOX);
    ON_CHOOSE_OFFENSIVE_STAT {
        int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
        int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
        if (atk > spAtk)
            *atkStatToUse = STAT_ATK;
        else if (spAtk > atk)
            *atkStatToUse = STAT_SPATK;
    }
};

ABILITY(ABILITY_ABSORBANT) : extends OnAttacker {
    INSTANCE(ABILITY_ABSORBANT);
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

ABILITY(ABILITY_CLUELESS) : extends AbilityImpl<ABILITY_CLOUD_NINE>, extends Unsuppressable { INSTANCE(ABILITY_CLUELESS); };

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
ABILITY(ABILITY_CHEATING_DEATH) : extends NoDamageHits<2>, extends OverrideBreakable { INSTANCE(ABILITY_CHEATING_DEATH); };

ABILITY(ABILITY_CHEAP_TACTICS) : extends SimpleEntryMove<MOVE_SCRATCH> { INSTANCE(ABILITY_CHEAP_TACTICS); };

ABILITY(ABILITY_COWARD) : extends OnEntry, extends Persistent {
    INSTANCE(ABILITY_COWARD);
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

        SetSingleUseAbilityCounter(battler, ability, TRUE);
        gRoundStructs[battler].protectedThisTurn = TRUE;
        BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
        return TRUE;
    }
};

ABILITY(ABILITY_VOLT_RUSH) : extends GaleWingsLike<TYPE_ELECTRIC> { INSTANCE(ABILITY_VOLT_RUSH); };

ABILITY(ABILITY_DUNE_TERROR) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier, extends SandImmune {
    INSTANCE(ABILITY_DUNE_TERROR);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GROUND) MUL(1.2);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
    }
};

ABILITY(ABILITY_INFERNAL_RAGE) : extends OnRecoil, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_INFERNAL_RAGE);
    ON_RECOIL {
        CHECK(moveType == TYPE_FIRE);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) MUL(1.35);
    }
};

ABILITY(ABILITY_DUAL_WIELD) : extends OnParentalBond {
    INSTANCE(ABILITY_DUAL_WIELD);
    ON_PARENTAL_BOND {
        CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return PARENTAL_BOND_DUAL_WIELD;
    }
};

ABILITY(ABILITY_ELEMENTAL_CHARGE) : extends OnAttacker {
    INSTANCE(ABILITY_ELEMENTAL_CHARGE);
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

ABILITY(ABILITY_AMBUSH) : extends OnCrit<> {
    INSTANCE(ABILITY_AMBUSH);
    ON_CRIT {
        CHECK(gVolatileStructs[battler].isFirstTurn)
        return ALWAYS_CRIT;
    }
};

ABILITY(ABILITY_ATLAS) : extends OnEntry {
    INSTANCE(ABILITY_ATLAS);
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    }
};

ABILITY(ABILITY_RADIANCE) : extends OnImmune<ApplyOn::ANY>, extends OnAccuracy<> {
    INSTANCE(ABILITY_RADIANCE);
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

ABILITY(ABILITY_JAWS_OF_CARNAGE) : extends OnBattlerFaints<> {
    INSTANCE(ABILITY_JAWS_OF_CARNAGE);
    ON_BATTLER_FAINTS {
        CHECK_NOT(BATTLER_MAX_HP(battler))
        CHECK(CanBattlerHeal(battler))
        if (gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST)
            BattleScriptCall(BattleScript_HandleJawsOfCarnageEffect);
        else
            BattleScriptCall(BattleScript_HandleSoulEaterEffect);
        return TRUE;
    }
};

ABILITY(ABILITY_ANGELS_WRATH)
    : extends OnAttacker, extends OnAccuracy<>, extends OnTypeEffectiveness<>, extends OnModifyEffectChance<>, extends OnCanStatusType {
    INSTANCE(ABILITY_ANGELS_WRATH);
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

ABILITY(ABILITY_PRISMATIC_FUR)
    : extends AbilityImpl<ABILITY_COLOR_CHANGE>, extends AbilityImpl<ABILITY_PROTEAN>, extends OnDefensiveMultiplier, extends OverrideBreakable {
    INSTANCE(ABILITY_PRISMATIC_FUR);
    ON_DEFENSIVE_MULTIPLIER { MUL(.5); }
    ON_BEFORE_ATTACK {
        if (battler == attacker)
            return AbilityImpl<ABILITY_PROTEAN>::onBeforeAttack(DELEGATE_BEFORE_ATTACK);
        else
            return AbilityImpl<ABILITY_COLOR_CHANGE>::onBeforeAttack(DELEGATE_BEFORE_ATTACK);
    }
    ApplyOnTarget onBeforeAttackFor() override { return ApplyOnTarget::ATTACKER_OR_TARGET; }
};

ABILITY(ABILITY_SHOCKING_JAWS) : extends OnAttacker {
    INSTANCE(ABILITY_SHOCKING_JAWS);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeParalyzed(battler, target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
    }
};

ABILITY(ABILITY_FAE_HUNTER) : extends TypeSlayer<TYPE_FAIRY> { INSTANCE(ABILITY_FAE_HUNTER); };

ABILITY(ABILITY_GRAVITY_WELL) : extends OnEntry {
    INSTANCE(ABILITY_GRAVITY_WELL);
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION;
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
        return TRUE;
    }
};

ABILITY(ABILITY_EVAPORATE) : extends OnAbsorb {
    INSTANCE(ABILITY_EVAPORATE);
    ON_ABSORB {
        CHECK(moveType == TYPE_WATER)
        return ABSORB_RESULT_EVAPORATE;
    }
};

ABILITY(ABILITY_LUMBERJACK) : extends TypeSlayer<TYPE_GRASS> { INSTANCE(ABILITY_LUMBERJACK); };

struct AbsorbUp2 {};
ABILITY(ABILITY_WELL_BAKED_BODY) : extends AbsorbStatUp<TYPE_FIRE, STAT_DEF>, extends AbsorbUp2 { INSTANCE(ABILITY_WELL_BAKED_BODY); };

ABILITY(ABILITY_FURNACE) : extends OnEntry, extends OnDefender {
    INSTANCE(ABILITY_FURNACE);
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

ABILITY(ABILITY_ROCKY_PAYLOAD) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ROCKY_PAYLOAD);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
    }
};

ABILITY(ABILITY_EARTH_EATER) : extends AbsorbHeal<TYPE_GROUND> { INSTANCE(ABILITY_EARTH_EATER); };

ABILITY(ABILITY_LINGERING_AROMA) : extends AbilityImpl<ABILITY_MUMMY> { INSTANCE(ABILITY_LINGERING_AROMA); };

ABILITY(ABILITY_FAIRY_TALE) : extends AddsType<TYPE_FAIRY> { INSTANCE(ABILITY_FAIRY_TALE); };

ABILITY(ABILITY_RAGING_MOTH) : extends OnParentalBond {
    INSTANCE(ABILITY_RAGING_MOTH);
    ON_PARENTAL_BOND {
        CHECK(moveType == TYPE_FIRE)
        return PARENTAL_BOND_DUAL_WIELD;
    }
};

ABILITY(ABILITY_ADRENALINE_RUSH) : extends MoxieClone<STAT_SPEED> { INSTANCE(ABILITY_ADRENALINE_RUSH); };

ABILITY(ABILITY_ARCHMAGE) : extends RandomizerBanned, extends OnAttacker {
    INSTANCE(ABILITY_ARCHMAGE);
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

ABILITY(ABILITY_CRYOMANCY) : extends OnModifyEffectChance<> {
    INSTANCE(ABILITY_CRYOMANCY);
    ON_MODIFY_EFFECT_CHANCE {
        if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
    }
};

ABILITY(ABILITY_PHANTOM_PAIN) : extends OnTypeEffectiveness<> {
    INSTANCE(ABILITY_PHANTOM_PAIN);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_GHOST)
        CHECK(defType == TYPE_NORMAL)
        CHECK_NOT(*mod)
        *mod = UQ_4_12(1.0);
        return TRUE;
    }
};

ABILITY(ABILITY_PURGATORY) : extends BoostedSwarmLike<TYPE_GHOST> { INSTANCE(ABILITY_PURGATORY); };

ABILITY(ABILITY_EMANATE) : extends AteAbility<TYPE_PSYCHIC> { INSTANCE(ABILITY_EMANATE); };

ABILITY(ABILITY_KUNOICHI_BLADE) : extends AbilityImpl<ABILITY_TECHNICIAN>, extends AbilityImpl<ABILITY_SKILL_LINK> { INSTANCE(ABILITY_KUNOICHI_BLADE); };

ABILITY(ABILITY_MONKEY_BUSINESS) : extends SimpleEntryMove<MOVE_TICKLE> { INSTANCE(ABILITY_MONKEY_BUSINESS); };

ABILITY(ABILITY_COMBAT_SPECIALIST) : extends Merged<ABILITY_IRON_FIST, ABILITY_STRIKER> { INSTANCE(ABILITY_COMBAT_SPECIALIST); };

ABILITY(ABILITY_JUNGLES_GUARD) : extends AbilityImpl<ABILITY_FLOWER_VEIL> { INSTANCE(ABILITY_JUNGLES_GUARD); };

ABILITY(ABILITY_HUNTERS_HORN) : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_MIGHTY_HORN> { INSTANCE(ABILITY_HUNTERS_HORN); };

ABILITY(ABILITY_PIXIE_POWER) : extends AbilityImpl<ABILITY_FAIRY_AURA>, extends OnAccuracy<> {
    INSTANCE(ABILITY_PIXIE_POWER);
    ON_ACCURACY {
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_PLASMA_LAMP) : extends OnOffensiveMultiplier<>, extends OnAccuracy<> {
    INSTANCE(ABILITY_PLASMA_LAMP);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC) MUL(1.2);
    }
    ON_ACCURACY {
        CHECK(moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC)
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_MAGMA_EATER) : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_MOLTEN_DOWN> { INSTANCE(ABILITY_MAGMA_EATER); };

ABILITY(ABILITY_SUPER_HOT_GOO) : extends Merged<ABILITY_GOOEY, ABILITY_FLAME_BODY> { INSTANCE(ABILITY_SUPER_HOT_GOO); };

ABILITY(ABILITY_NIKA) : extends AbilityImpl<ABILITY_IRON_FIST> { INSTANCE(ABILITY_NIKA); };

ABILITY(ABILITY_ARCHER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ARCHER);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].arrowBased) MUL(1.3);
    }
};

ABILITY(ABILITY_SUPER_SLAMMER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_SUPER_SLAMMER);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hammerBased) MUL(1.3);
    }
};

ABILITY(ABILITY_INVERSE_ROOM) : extends OnEntry {
    INSTANCE(ABILITY_INVERSE_ROOM);
    ON_ENTRY {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

        gFieldTimers.started.inverseRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
        gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
        BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
        return TRUE;
    }
};

ABILITY(ABILITY_FROST_BURN) : extends OnAttacker {
    INSTANCE(ABILITY_FROST_BURN);
    ON_ATTACKER {
        CHECK(moveType == TYPE_FIRE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
    }
};

ABILITY(ABILITY_ITCHY_DEFENSE) : extends OnDefender {
    INSTANCE(ABILITY_ITCHY_DEFENSE);
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

ABILITY(ABILITY_GENERATOR) : extends OnEntry, extends OnTerrain, extends OnExit, extends Persistent {
    INSTANCE(ABILITY_GENERATOR);
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

ABILITY(ABILITY_MOON_SPIRIT) : extends OnStab {
    INSTANCE(ABILITY_MOON_SPIRIT);
    ON_STAB { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; }
};

ABILITY(ABILITY_DUST_CLOUD) : extends SimpleEntryMove<MOVE_SAND_ATTACK> { INSTANCE(ABILITY_DUST_CLOUD); };

ABILITY(ABILITY_TIPPING_POINT) : extends OnDefender {
    INSTANCE(ABILITY_TIPPING_POINT);
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

ABILITY(ABILITY_BERSERKER_RAGE) : extends AbilityImpl<ABILITY_TIPPING_POINT>, extends AbilityImpl<ABILITY_RAMPAGE> { INSTANCE(ABILITY_BERSERKER_RAGE); };

ABILITY(ABILITY_TRICKSTER) : extends SimpleEntryMove<MOVE_DISABLE> { INSTANCE(ABILITY_TRICKSTER); };

ABILITY(ABILITY_SAND_GUARD) : extends OnImmune<>, extends OnDefensiveMultiplier, extends SandImmune {
    INSTANCE(ABILITY_SAND_GUARD);
    ON_IMMUNE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
        return blocksPriority(DELEGATE_IMMUNE);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
    }
};

ABILITY(ABILITY_NATURAL_RECOVERY) : extends Merged<ABILITY_NATURAL_CURE, ABILITY_REGENERATOR> { INSTANCE(ABILITY_NATURAL_RECOVERY); };

ABILITY(ABILITY_WIND_RIDER) : extends OnEntry, extends OnAbsorb {
    INSTANCE(ABILITY_WIND_RIDER);
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

ABILITY(ABILITY_SOOTHING_AROMA) : extends OnEntry {
    INSTANCE(ABILITY_SOOTHING_AROMA);
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

ABILITY(ABILITY_PRIM_AND_PROPER) : extends AbilityImpl<ABILITY_WONDER_SKIN>, extends AbilityImpl<ABILITY_CUTE_CHARM> { INSTANCE(ABILITY_PRIM_AND_PROPER); };

ABILITY(ABILITY_SUPER_STRAIN) : extends OnRecoil, OnBattlerFaints<> {
    INSTANCE(ABILITY_SUPER_STRAIN);
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

ABILITY(ABILITY_ENLIGHTENED) : extends AbilityImpl<ABILITY_EMANATE>, extends AbilityImpl<ABILITY_INNER_FOCUS> { INSTANCE(ABILITY_ENLIGHTENED); };

ABILITY(ABILITY_PEACEFUL_SLUMBER) : extends AbilityImpl<ABILITY_SWEET_DREAMS>, extends AbilityImpl<ABILITY_SELF_SUFFICIENT> {
    INSTANCE(ABILITY_PEACEFUL_SLUMBER);
    ON_END_TURN {
        if (!AbilityImpl<ABILITY_SWEET_DREAMS>::onEndTurn(DELEGATE_END_TURN)) return AbilityImpl<ABILITY_SELF_SUFFICIENT>::onEndTurn(DELEGATE_END_TURN);
        gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
        return TRUE;
    }
};

ABILITY(ABILITY_AFTERSHOCK) : extends OnAttacker {
    INSTANCE(ABILITY_AFTERSHOCK);
    ON_ATTACKER {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
    }
};

ABILITY(ABILITY_FREEZING_POINT) : extends OnEither {
    INSTANCE(ABILITY_FREEZING_POINT);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanGetFrostbite(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_CRYO_PROFICIENCY) : extends AbilityImpl<ABILITY_FREEZING_POINT> {
    INSTANCE(ABILITY_CRYO_PROFICIENCY);
    int CryoProficiencyHail(AbilityEnum ability, int battler, int attacker, MoveEnum move, int moveType) {
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
    ON_DEFENDER { return AbilityImpl<ABILITY_FREEZING_POINT>::onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(ability, battler, attacker, move, moveType); }
};

ABILITY(ABILITY_ARCANE_FORCE) : extends AbilityImpl<ABILITY_MYSTIC_POWER>, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_ARCANE_FORCE);
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
    }
};

ABILITY(ABILITY_DOOMBRINGER) : extends SimpleEntryMove<MOVE_DOOM_DESIRE> { INSTANCE(ABILITY_DOOMBRINGER); };

ABILITY(ABILITY_WISHMAKER) : extends OnEntry, extends Persistent {
    INSTANCE(ABILITY_WISHMAKER);
    ON_ENTRY {
        int counter = GetSingleUseAbilityCounter(battler, ability);
        CHECK(counter < 3)
        CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

        SetSingleUseAbilityCounter(battler, ability, counter + 1);
        return TRUE;
    }
};

ABILITY(ABILITY_YUKI_ONNA) : extends AbilityImpl<ABILITY_INTIMIDATE>, extends OnAttacker {
    INSTANCE(ABILITY_YUKI_ONNA);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanInfatuate(battler, target))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    }
};

ABILITY(ABILITY_SUPPRESS) : extends SimpleEntryMove<MOVE_TORMENT> { INSTANCE(ABILITY_SUPPRESS); };

ABILITY(ABILITY_REFRIGERATOR) : extends AbilityImpl<ABILITY_FILTER>, extends AbilityImpl<ABILITY_ILLUMINATE> { INSTANCE(ABILITY_REFRIGERATOR); };

ABILITY(ABILITY_HEAVEN_ASUNDER) : extends OnCrit<> {
    INSTANCE(ABILITY_HEAVEN_ASUNDER);
    ON_CRIT {
        if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
        return 1;
    }
};

ABILITY(ABILITY_PURIFYING_WATERS) : extends AbilityImpl<ABILITY_WATER_VEIL>, extends AbilityImpl<ABILITY_HYDRATION> { INSTANCE(ABILITY_PURIFYING_WATERS); };

ABILITY(ABILITY_SEABORNE) : extends AbilityImpl<ABILITY_DRIZZLE>, extends AbilityImpl<ABILITY_SWIFT_SWIM> { INSTANCE(ABILITY_SEABORNE); };

ABILITY(ABILITY_HIGH_TIDE) : extends OnAttacker {
    INSTANCE(ABILITY_HIGH_TIDE);
    ON_ATTACKER {
        CHECK(moveType == TYPE_WATER)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
    }
};

ABILITY(ABILITY_CHANGE_OF_HEART) : extends SimpleEntryMove<MOVE_HEART_SWAP> { INSTANCE(ABILITY_CHANGE_OF_HEART); };

ABILITY(ABILITY_MYSTIC_BLADES) : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends OnSwapSplit {
    INSTANCE(ABILITY_MYSTIC_BLADES);
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
        return TRUE;
    }
};

struct NegateFrzSpatkDrop {};
ABILITY(ABILITY_DETERMINATION) : extends OnOffensiveMultiplier<>, extends NegateFrzSpatkDrop {
    INSTANCE(ABILITY_DETERMINATION);
    ON_OFFENSIVE_MULTIPLIER {
        if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
    }
};

ABILITY(ABILITY_FERTILIZE) : extends AteAbility<TYPE_GRASS> { INSTANCE(ABILITY_FERTILIZE); };

struct CanInfatuateAny {};
ABILITY(ABILITY_PURE_LOVE) : extends OnDefender, extends OnAttacker, extends CanInfatuateAny {
    INSTANCE(ABILITY_PURE_LOVE);
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

ABILITY(ABILITY_FIGHTER) : extends SwarmLike<TYPE_FIGHTING> { INSTANCE(ABILITY_FIGHTER); };

ABILITY(ABILITY_TELEKINETIC) : extends SimpleEntryMove<MOVE_TELEKINESIS> { INSTANCE(ABILITY_TELEKINETIC); };

ABILITY(ABILITY_COMBUSTION) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_COMBUSTION);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) MUL(1.5);
    }
};

ABILITY(ABILITY_PONY_POWER) : extends Merged<ABILITY_KEEN_EDGE, ABILITY_MYSTIC_BLADES> { INSTANCE(ABILITY_PONY_POWER); };

ABILITY(ABILITY_POWDER_BURST) : extends SimpleEntryMove<MOVE_POWDER> { INSTANCE(ABILITY_POWDER_BURST); };

ABILITY(ABILITY_RETRIEVER) : extends OnExit {
    INSTANCE(ABILITY_RETRIEVER);
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

ABILITY(ABILITY_MONSTER_MASH) : extends SimpleEntryMove<MOVE_TRICK_OR_TREAT> { INSTANCE(ABILITY_MONSTER_MASH); };

ABILITY(ABILITY_TWO_STEP) : extends OnAttacker {
    INSTANCE(ABILITY_TWO_STEP);
    ON_ATTACKER {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
    }
};

ABILITY(ABILITY_SPITEFUL) : extends OnDefender {
    INSTANCE(ABILITY_SPITEFUL);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(move != MOVE_STRUGGLE)
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK(gBattleMons[attacker].pp[gChosenMovePos])

        BattleScriptCall(BattleScript_AbilitySpiteful);
        return TRUE;
    }
};

ABILITY(ABILITY_FORTITUDE) : extends OnDefender {
    INSTANCE(ABILITY_FORTITUDE);
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

ABILITY(ABILITY_DEVOURER) : extends AbilityImpl<ABILITY_PRIMAL_MAW>, extends AbilityImpl<ABILITY_STRONG_JAW> { INSTANCE(ABILITY_DEVOURER); };

ABILITY(ABILITY_PHANTOM_THIEF) : extends SimpleEntryMove<MOVE_SPECTRAL_THIEF, 40> { INSTANCE(ABILITY_PHANTOM_THIEF); };

ABILITY(ABILITY_EARLY_GRAVE) : extends GaleWingsLike<TYPE_GHOST> { INSTANCE(ABILITY_EARLY_GRAVE); };

ABILITY(ABILITY_BASS_BOOSTED) : extends Merged<ABILITY_PUNK_ROCK, ABILITY_AMPLIFIER> { INSTANCE(ABILITY_BASS_BOOSTED); };

ABILITY(ABILITY_FLAMING_JAWS) : extends OnAttacker {
    INSTANCE(ABILITY_FLAMING_JAWS);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    }
};

ABILITY(ABILITY_MONSTER_HUNTER) : extends TypeSlayer<TYPE_DARK> { INSTANCE(ABILITY_MONSTER_HUNTER); };

ABILITY(ABILITY_CROWNED_SWORD) : extends AbilityImpl<ABILITY_INTREPID_SWORD>, extends AbilityImpl<ABILITY_ANGER_POINT> { INSTANCE(ABILITY_CROWNED_SWORD); };

ABILITY(ABILITY_CROWNED_SHIELD) : extends AbilityImpl<ABILITY_DAUNTLESS_SHIELD>, extends AbilityImpl<ABILITY_STAMINA> { INSTANCE(ABILITY_CROWNED_SHIELD); };

ABILITY(ABILITY_BERSERK_DNA) : extends OnEntry {
    INSTANCE(ABILITY_BERSERK_DNA);
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

ABILITY(ABILITY_CROWNED_KING) : extends AbilityImpl<ABILITY_AS_ONE_ICE_RIDER>, extends AbilityImpl<ABILITY_AS_ONE_SHADOW_RIDER> {
    INSTANCE(ABILITY_CROWNED_KING);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); }
    ON_BATTLER_FAINTS {
        CHECK(AbilityImpl<ABILITY_CHILLING_NEIGH>::onBattlerFaints(DELEGATE_BATTLER_FAINTS) |
              AbilityImpl<ABILITY_GRIM_NEIGH>::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
        gBattleScripting.abilityPopupOverwrite = ABILITY_CROWNED_KING;
        BattleScriptCall(BattleScript_AbilityPopUpStack);
        return NO_ANNOUNCE;
    }
};

ABILITY(ABILITY_SNAP_TRAP_WHEN_HIT) : extends OnDefender {
    INSTANCE(ABILITY_SNAP_TRAP_WHEN_HIT);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
        return FALSE;
    }
};

ABILITY(ABILITY_PERMANENCE) : extends OnEntry {
    INSTANCE(ABILITY_PERMANENCE);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); }
};

ABILITY(ABILITY_HUBRIS) : extends AbilityImpl<ABILITY_GRIM_NEIGH> { INSTANCE(ABILITY_HUBRIS); };

ABILITY(ABILITY_COSMIC_DAZE) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_COSMIC_DAZE);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
    }
};

ABILITY(ABILITY_MINDS_EYE) : extends Breakable, extends HitsGhost { INSTANCE(ABILITY_MINDS_EYE); };

ABILITY(ABILITY_BLOOD_PRICE) : extends OnEndTurn, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_BLOOD_PRICE);
    ON_END_TURN {
        CHECK_NOT(IS_MOVE_STATUS(gLastResultingMoves[battler]))
        CHECK_NOT(IsMagicGuardProtected(battler))
        CHECK(IsBattlerAlive(battler))

        gBattleMoveDamage = gBattleMons[battler].maxHP / 10;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        BattleScriptPushCursorAndCallback(BattleScript_AbilitySelfDamage);
        return TRUE;
    }
    ON_OFFENSIVE_MULTIPLIER { MUL(1.3); }
};

ABILITY(ABILITY_SPIKE_ARMOR) : extends OnEither {
    INSTANCE(ABILITY_SPIKE_ARMOR);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBleed(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_VOODOO_POWER) : extends OnDefender {
    INSTANCE(ABILITY_VOODOO_POWER);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IS_MOVE_SPECIAL(move))
        CHECK(CanBleed(attacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
        return TRUE;
    }
};

ABILITY(ABILITY_CHROME_COAT) : extends OnDefensiveMultiplier, extends OnStat<> {
    INSTANCE(ABILITY_CHROME_COAT);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_SPECIAL(move)) MUL(.6);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .9;
    }
};

ABILITY(ABILITY_BANSHEE) : LiquidVoiceClone<TYPE_GHOST> { INSTANCE(ABILITY_BANSHEE); };

ABILITY(ABILITY_WEB_SPINNER) : extends SimpleEntryMove<MOVE_STRING_SHOT> { INSTANCE(ABILITY_WEB_SPINNER); };

ABILITY(ABILITY_SHOWDOWN_MODE) : extends OnEntry {
    INSTANCE(ABILITY_SHOWDOWN_MODE);
    ON_ENTRY {
        gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
    }
};

ABILITY(ABILITY_SEED_SOWER) : extends OnDefender, extends AllowTerrainIfAirborne<TERRAIN_GRASSY> {
    INSTANCE(ABILITY_SEED_SOWER);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

        BattleScriptCall(BattleScript_SeedSower);
        return TRUE;
    }
};

ABILITY(ABILITY_AIRBORNE) : extends OnOffensiveMultiplier<ApplyOn::ALLY> {
    INSTANCE(ABILITY_AIRBORNE);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FLYING) MUL(1.3);
    }
};

ABILITY(ABILITY_PARROTING) : extends AbilityImpl<ABILITY_SOUNDPROOF>, extends OnCopyMove {
    INSTANCE(ABILITY_PARROTING);
    ON_COPY_MOVE {
        CHECK(IsSoundMove(attacker, move))
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    }
};

ABILITY(ABILITY_SALT_CIRCLE) : extends OnEntry {
    INSTANCE(ABILITY_SALT_CIRCLE);
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

ABILITY(ABILITY_PURIFYING_SALT) : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_PURIFYING_SALT);
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

ABILITY(ABILITY_PROTOSYNTHESIS) : extends ParadoxBoostEffect, extends OnWeather {
    INSTANCE(ABILITY_PROTOSYNTHESIS);
    ON_ENTRY { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    ON_WEATHER { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_CALL); }
};

ABILITY(ABILITY_QUARK_DRIVE) : extends ParadoxBoostEffect, extends OnTerrain {
    INSTANCE(ABILITY_QUARK_DRIVE);
    ON_ENTRY {
        return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
    }
    ON_TERRAIN { return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_CALL); }
};

ABILITY(ABILITY_WIND_POWER) : extends OnDefender {
    INSTANCE(ABILITY_WIND_POWER);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(gBattleMoves[move].airBased)
        CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

        gStatuses3[battler] |= STATUS3_CHARGED_UP;
        BattleScriptCall(BattleScript_ElectromorphosisActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_IMPULSE) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_IMPULSE);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
    }
};

ABILITY(ABILITY_TERMINAL_VELOCITY) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_TERMINAL_VELOCITY);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
    }
};

ABILITY(ABILITY_ANGER_SHELL) : extends OnDefender {
    INSTANCE(ABILITY_ANGER_SHELL);
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_AngerShell);
        return TRUE;
    }
};

ABILITY(ABILITY_EGOIST) : extends OnReactive {
    INSTANCE(ABILITY_EGOIST);
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

ABILITY(ABILITY_READIED_ACTION) : extends OnEntry {
    INSTANCE(ABILITY_READIED_ACTION);
    ON_ENTRY {
        gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
    }
};

ABILITY(ABILITY_DARK_GALE_WINGS) : extends GaleWingsLike<TYPE_DARK> { INSTANCE(ABILITY_DARK_GALE_WINGS); };

ABILITY(ABILITY_GUILT_TRIP) : extends OnDefender {
    INSTANCE(ABILITY_GUILT_TRIP);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsBattlerAlive(battler))
        CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

        BattleScriptCall(BattleScript_GuiltTrip);
        return TRUE;
    }
};

ABILITY(ABILITY_WATER_GALE_WINGS) : extends GaleWingsLike<TYPE_WATER> { INSTANCE(ABILITY_WATER_GALE_WINGS); };

ABILITY(ABILITY_ZERO_TO_HERO) : extends FormChangeAbility, extends OnEntry, extends OnExit {
    INSTANCE(ABILITY_ZERO_TO_HERO);
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

ABILITY(ABILITY_COSTAR) : extends OnEntry {
    INSTANCE(ABILITY_COSTAR);
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

ABILITY(ABILITY_COMMANDER) : extends FormChangeAbility, extends OnBattlerFaints<ApplyOnTarget::ALLY>, extends OnAccuracy<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_COMMANDER);
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

ABILITY(ABILITY_EJECT_PACK_ABILITY) : extends Persistent { INSTANCE(ABILITY_EJECT_PACK_ABILITY); };

ABILITY(ABILITY_VENGEFUL_SPIRIT) : extends AbilityImpl<ABILITY_HAUNTED_SPIRIT>, extends AbilityImpl<ABILITY_VENGEANCE> { INSTANCE(ABILITY_VENGEFUL_SPIRIT); };

ABILITY(ABILITY_CUD_CHEW) : extends OnEndTurn {
    INSTANCE(ABILITY_CUD_CHEW);
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

ABILITY(ABILITY_ARMOR_TAIL) : extends AbilityImpl<ABILITY_QUEENLY_MAJESTY> { INSTANCE(ABILITY_ARMOR_TAIL); };

ABILITY(ABILITY_MIND_CRUSH) : extends AbilityImpl<ABILITY_STRONG_JAW>, extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_MIND_CRUSH);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
    }
};

ABILITY(ABILITY_SUPREME_OVERLORD) : extends OnEntry, extends OnStat<> {
    INSTANCE(ABILITY_SUPREME_OVERLORD);
    ON_ENTRY {
        CHECK(gFaintedMonCount[GetBattlerSide(battler)])

        return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
    }
    ON_STAT {
        if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
    }
};

ABILITY(ABILITY_ILL_WILL) : extends OnDefender {
    INSTANCE(ABILITY_ILL_WILL);
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

ABILITY(ABILITY_FIRE_SCALES) : extends AbilityImpl<ABILITY_ICE_SCALES> { INSTANCE(ABILITY_FIRE_SCALES); };

ABILITY(ABILITY_WATCH_YOUR_STEP) : extends OnEntry {
    INSTANCE(ABILITY_WATCH_YOUR_STEP);
    ON_ENTRY {
        u8 targetSide = GetOppositeSide(battler);
        CHECK(gSideTimers[targetSide].spikesAmount < 3)

        gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
        return TRUE;
    }
};

ABILITY(ABILITY_RAPID_RESPONSE) : extends OnEntry {
    INSTANCE(ABILITY_RAPID_RESPONSE);
    ON_ENTRY {
        gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
    }
};

ABILITY(ABILITY_DOUBLE_IRON_BARBS) : extends OnDefender {
    INSTANCE(ABILITY_DOUBLE_IRON_BARBS);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK_NOT(IsMagicGuardProtected(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        gBattleMoveDamage = gBattleMons[attacker].maxHP / 6;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, ability);
        BattleScriptCall(BattleScript_IronBarbsActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_THERMAL_EXCHANGE) : extends OnDefender, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_THERMAL_EXCHANGE);
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

ABILITY(ABILITY_GOOD_AS_GOLD) : extends OnImmune<> {
    INSTANCE(ABILITY_GOOD_AS_GOLD);
    ON_IMMUNE {
        CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
        *immunityScript = BattleScript_SoundproofProtected;
        return TRUE;
    }
};

ABILITY(ABILITY_SHARING_IS_CARING) : extends OnReactive {
    INSTANCE(ABILITY_SHARING_IS_CARING);
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

ABILITY(ABILITY_PERMAFROST_CLONE) : extends AbilityImpl<ABILITY_PERMAFROST> { INSTANCE(ABILITY_PERMAFROST_CLONE); };

ABILITY(ABILITY_GALLANTRY) : extends NoDamageHits<1> { INSTANCE(ABILITY_GALLANTRY); };

ABILITY(ABILITY_ORICHALCUM_PULSE) : extends AbilityImpl<ABILITY_DROUGHT>, extends OnStat<> {
    INSTANCE(ABILITY_ORICHALCUM_PULSE);
    ON_STAT {
        if (statId != STAT_ATK) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
    }
};

ABILITY(ABILITY_SUN_BASKING) : extends OnImmune<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_SUN_BASKING);
    ON_IMMUNE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
        return blocksPriority(DELEGATE_IMMUNE);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
    }
};

ABILITY(ABILITY_WINGED_KING) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_WINGED_KING);
    ON_OFFENSIVE_MULTIPLIER {
        if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
    }
};

ABILITY(ABILITY_HADRON_ENGINE) : extends AbilityImpl<ABILITY_ELECTRIC_SURGE>, extends OnStat<> {
    INSTANCE(ABILITY_HADRON_ENGINE);
    ON_STAT {
        if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
    }
};

ABILITY(ABILITY_IRON_SERPENT) : extends AbilityImpl<ABILITY_WINGED_KING> { INSTANCE(ABILITY_IRON_SERPENT); };

ABILITY(ABILITY_SWEEPING_EDGE_PLUS) : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends AbilityImpl<ABILITY_SWEEPING_EDGE> {
    INSTANCE(ABILITY_SWEEPING_EDGE_PLUS);
};

ABILITY(ABILITY_CELESTIAL_BLESSING) : extends OnEndTurn {
    INSTANCE(ABILITY_CELESTIAL_BLESSING);
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

ABILITY(ABILITY_MINION_CONTROL) : extends OnParentalBond {
    INSTANCE(ABILITY_MINION_CONTROL);
    ON_PARENTAL_BOND { return PARENTAL_BOND_MINION_CONTROL; }
};

ABILITY(ABILITY_MOLTEN_BLADES) : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends OnAttacker {
    INSTANCE(ABILITY_MOLTEN_BLADES);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeBurned(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_BURN);
    }
};

ABILITY(ABILITY_HAUNTING_FRENZY) : extends AbilityImpl<ABILITY_ADRENALINE_RUSH>, extends OnAttacker {
    INSTANCE(ABILITY_HAUNTING_FRENZY);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanMoveHaveExtraFlinchChance(move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
    }
};

ABILITY(ABILITY_NOISE_CANCEL) : extends AbilityImpl<ABILITY_SOUNDPROOF> {
    INSTANCE(ABILITY_NOISE_CANCEL);
    ApplyOn onImmuneFor() override { return ApplyOn::ALLY; }
};

ABILITY(ABILITY_RADIO_JAM) : extends OnAttacker {
    INSTANCE(ABILITY_RADIO_JAM);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBeDisabled(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 20)

        return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
    }
};

ABILITY(ABILITY_OLE) : extends OnAccuracy<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_OLE);
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

ABILITY(ABILITY_MALICIOUS) : extends AbilityImpl<ABILITY_INTIMIDATE> { INSTANCE(ABILITY_MALICIOUS); };

ABILITY(ABILITY_DEAD_POWER) : extends OnAttacker, extends OnStat<> {
    INSTANCE(ABILITY_DEAD_POWER);
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

ABILITY(ABILITY_BRAWLING_WYVERN) : extends AbilityImpl<ABILITY_NO_GUARD>, extends OnModifyMoveFlags {
    INSTANCE(ABILITY_BRAWLING_WYVERN);
    ON_MODIFY_MOVE_FLAGS {
        CHECK(flag == MOVE_FLAG_PUNCH)
        CHECK(IS_MOVE_TYPE(move, TYPE_DRAGON))
        return TRUE;
    }
};

ABILITY(ABILITY_JUNSHI_SANDA) : extends OnModifyMoveFlags {
    INSTANCE(ABILITY_JUNSHI_SANDA);
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

ABILITY(ABILITY_MYTHICAL_ARROWS) : extends AbilityImpl<ABILITY_ARCHER>, extends OnSwapSplit {
    INSTANCE(ABILITY_MYTHICAL_ARROWS);
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].arrowBased);
        return TRUE;
    }
};

ABILITY(ABILITY_LAWNMOWER) : extends OnEntry {
    INSTANCE(ABILITY_LAWNMOWER);
    ON_ENTRY {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
        return TRUE;
    }
};

ABILITY(ABILITY_FLOURISH) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_FLOURISH);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
    }
};

ABILITY(ABILITY_DESERT_SPIRIT) : extends AbilityImpl<ABILITY_SAND_STREAM>, extends OnAfterTypeEffectiveness<> {
    INSTANCE(ABILITY_DESERT_SPIRIT);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
            *mod = UQ_4_12(1.0);
        }
    }
};

ABILITY(ABILITY_CONTEMPT) : extends AbilityImpl<ABILITY_UNAWARE> { INSTANCE(ABILITY_CONTEMPT); };

ABILITY(ABILITY_AERIALIST) : extends Merged<ABILITY_LEVITATE, ABILITY_FLOCK> { INSTANCE(ABILITY_AERIALIST); };

ABILITY(ABILITY_TERA_SHELL) : extends Breakable, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_TERA_SHELL);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
    }
};

ABILITY(ABILITY_TOXIC_CHAIN) : extends OnAttacker {
    INSTANCE(ABILITY_TOXIC_CHAIN);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
    }
};

ABILITY(ABILITY_PARASITIC_SPORES) : extends OnEntry {
    INSTANCE(ABILITY_PARASITIC_SPORES);
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

ABILITY(ABILITY_POISON_PUPPETEER) : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    INSTANCE(ABILITY_POISON_PUPPETEER);
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
    }
};

ABILITY(ABILITY_ENTRANCE) : extends PoisonPuppeteerLike<MOVE_EFFECT_CONFUSION> {
    INSTANCE(ABILITY_ENTRANCE);
    ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); }
};

ABILITY(ABILITY_REJECTION) : extends OnEntry {
    INSTANCE(ABILITY_REJECTION);
    ON_ENTRY {
        CHECK_NOT(gFieldTimers.quashTimer)

        gFieldTimers.quashTimer = QUASH_DURATION;
        gFieldTimers.started.quash = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
    }
};

ABILITY(ABILITY_APPLE_ENLIGHTENMENT) : extends AbilityImpl<ABILITY_FUR_COAT>, extends AbilityImpl<ABILITY_MAGIC_GUARD> {
    INSTANCE(ABILITY_APPLE_ENLIGHTENMENT);
};

ABILITY(ABILITY_BALLOON_BOMBER) : extends Merged<ABILITY_AFTERMATH, ABILITY_INFLATABLE> { INSTANCE(ABILITY_BALLOON_BOMBER); };

ABILITY(ABILITY_FLAMING_MAW) : extends AbilityImpl<ABILITY_FLAMING_JAWS>, extends AbilityImpl<ABILITY_STRONG_JAW> { INSTANCE(ABILITY_FLAMING_MAW); };

ABILITY(ABILITY_DEMOLITIONIST) : extends AbilityImpl<ABILITY_READIED_ACTION>, extends OnInfiltrate, extends OnAttacker {
    INSTANCE(ABILITY_DEMOLITIONIST);
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

ABILITY(ABILITY_ROCKHARD_WILL) : extends SwarmLike<TYPE_ROCK> { INSTANCE(ABILITY_ROCKHARD_WILL); };
ABILITY(ABILITY_FRAGRANT_DAZE) : extends OnEither {
    INSTANCE(ABILITY_FRAGRANT_DAZE);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeConfused(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_CONFUSION, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_LOW_VISIBILITY) : extends OnEntry {
    INSTANCE(ABILITY_LOW_VISIBILITY);
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

ABILITY(ABILITY_OLD_MARINER) : extends AbilityImpl<ABILITY_SEAWEED>, extends AbilityImpl<ABILITY_AMPHIBIOUS> { INSTANCE(ABILITY_OLD_MARINER); };

ABILITY(ABILITY_ECTOPLASM) : extends OnStat<> {
    INSTANCE(ABILITY_ECTOPLASM);
    ON_STAT {
        if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
        if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_BEAUTIFUL_MUSIC) : extends OnAttacker, extends CanInfatuateAny {
    INSTANCE(ABILITY_BEAUTIFUL_MUSIC);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(Random() % 2)
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
    }
};

ABILITY(ABILITY_SNOW_SONG) : LiquidVoiceClone<TYPE_ICE> { INSTANCE(ABILITY_SNOW_SONG); };

ABILITY(ABILITY_GREATER_SPIRIT) : extends OnEntry {
    INSTANCE(ABILITY_GREATER_SPIRIT);
    ON_ENTRY {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

        int stat = GetHighestStatId(battler, TRUE);
        CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
        BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
        return TRUE;
    }
};

ABILITY(ABILITY_RESONANCE) : extends OnAttacker {
    INSTANCE(ABILITY_RESONANCE);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))
        CHECK(Random() % 100 < 50)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

ABILITY(ABILITY_ETHEREAL_RUSH) : extends OnStat<> {
    INSTANCE(ABILITY_ETHEREAL_RUSH);
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_CUTE_ANTECEDENCE) : extends GaleWingsLike<TYPE_FAIRY> { INSTANCE(ABILITY_CUTE_ANTECEDENCE); };

ABILITY(ABILITY_RECURRING_NIGHTMARE) : extends OnRevive {
    INSTANCE(ABILITY_RECURRING_NIGHTMARE);
    ON_REVIVE {
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
        return B_MSG_FADE_OUT;
    }
};

ABILITY(ABILITY_MENACING_SITUATION) : extends OnEither {
    INSTANCE(ABILITY_MENACING_SITUATION);
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

ABILITY(ABILITY_SHINY_LIGHTNING) : extends OnAccuracy<> {
    INSTANCE(ABILITY_SHINY_LIGHTNING);
    ON_ACCURACY {
        if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
        *accuracy *= 1.2;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_TERRIFY) : extends AbilityImpl<ABILITY_INTIMIDATE> { INSTANCE(ABILITY_TERRIFY); };

ABILITY(ABILITY_ICE_DOWNFALL) : extends OnDefender {
    INSTANCE(ABILITY_ICE_DOWNFALL);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
        return FALSE;
    }
};

ABILITY(ABILITY_LAST_STAND) : extends Breakable, extends OnStat<> {
    INSTANCE(ABILITY_LAST_STAND);
    ON_STAT {
        if (statId == STAT_DEF || statId == STAT_SPDEF)
            *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
    }
};

ABILITY(ABILITY_PYROCLASTIC_FLOW) : Merged<ABILITY_MOLTEN_DOWN, ABILITY_CORROSION> { INSTANCE(ABILITY_PYROCLASTIC_FLOW); };

ABILITY(ABILITY_BLOOD_BATH) : extends PoisonPuppeteerLike<MOVE_EFFECT_BLEED>, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_BLOOD_BATH);
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_BLEED)
        return TRUE;
    }
};

ABILITY(ABILITY_BATTLE_AURA) : extends OnCrit<ApplyOnTarget::ANY> {
    INSTANCE(ABILITY_BATTLE_AURA);
    ON_CRIT { return 2; }
};

ABILITY(ABILITY_BLOODLUST) : extends AbilityImpl<ABILITY_BLOOD_BATH>, extends AbilityImpl<ABILITY_SOUL_EATER> {
    INSTANCE(ABILITY_BLOODLUST);
    ON_BATTLER_FAINTS {
        int result = 0;
        if (battler == attacker) {
            result |= AbilityImpl<ABILITY_SOUL_EATER>::onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        return result | AbilityImpl<ABILITY_BLOOD_BATH>::onBattlerFaints(DELEGATE_BATTLER_FAINTS);
    }
    ApplyOnTarget onBattlerFaintsFor() override { return ApplyOnTarget::ANY; }
};

ABILITY(ABILITY_PIERCING_SOLO) : extends OnAttacker {
    INSTANCE(ABILITY_PIERCING_SOLO);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(IsSoundMove(battler, move))

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

ABILITY(ABILITY_RHYTHMIC) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_RHYTHMIC);
    ON_OFFENSIVE_MULTIPLIER { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); }
};

ABILITY(ABILITY_CHUNKY_BASS_LINE) : extends OnAttacker {
    INSTANCE(ABILITY_CHUNKY_BASS_LINE);
    ON_ATTACKER {
        CHECK(IsSoundMove(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
    }
};

ABILITY(ABILITY_DUAL_HAMMER) : extends OnParentalBond {
    INSTANCE(ABILITY_DUAL_HAMMER);
    ON_PARENTAL_BOND {
        CHECK(gBattleMoves[move].hammerBased)
        return PARENTAL_BOND_DUAL_WIELD;
    }
};

ABILITY(ABILITY_DENTING_BLOWS) : extends OnAttacker {
    INSTANCE(ABILITY_DENTING_BLOWS);
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

ABILITY(ABILITY_ICE_COLD_HUNTER) : extends HailImmune, extends OnParentalBond {
    INSTANCE(ABILITY_ICE_COLD_HUNTER);
    ON_PARENTAL_BOND {
        CHECK(moveType == TYPE_ICE)
        CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
        return PARENTAL_BOND_ICE_COLD_HUNTER;
    }
};

ABILITY(ABILITY_SOUL_CRUSHER) : extends OnOffensiveMultiplier<>, extends OnChooseDefensiveStat<> {
    INSTANCE(ABILITY_SOUL_CRUSHER);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].hammerBased) MUL(1.1);
    }
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(gBattleMoves[move].hammerBased)
        return STAT_SPDEF;
    }
};

ABILITY(ABILITY_ARC_FLASH) : extends OnAttacker, extends OnDefender {
    INSTANCE(ABILITY_ARC_FLASH);
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

ABILITY(ABILITY_UNICORN) : extends AbilityImpl<ABILITY_MIGHTY_HORN>, extends AbilityImpl<ABILITY_PIXILATE> { INSTANCE(ABILITY_UNICORN); };

ABILITY(ABILITY_ON_THE_PROWL) : extends OnEntry {
    INSTANCE(ABILITY_ON_THE_PROWL);
    ON_ENTRY {
        gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
    }
};

ABILITY(ABILITY_PRETENTIOUS) : extends OnBattlerFaints<> {
    INSTANCE(ABILITY_PRETENTIOUS);
    ON_BATTLER_FAINTS {
        CHECK(gVolatileStructs[battler].critBoost < 3);
        gVolatileStructs[battler].critBoost++;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
        BattleScriptCall(BattleScript_AbilityBoostsCrit);
        return TRUE;
    }
};

ABILITY(ABILITY_VENOBLAZE_PINCERS) : extends OnAttacker, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_VENOBLAZE_PINCERS);
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

ABILITY(ABILITY_ETERNAL_BLESSING) : extends AbilityImpl<ABILITY_CELESTIAL_BLESSING>, extends AbilityImpl<ABILITY_REGENERATOR> {
    INSTANCE(ABILITY_ETERNAL_BLESSING);
};

ABILITY(ABILITY_RIPEN) { INSTANCE(ABILITY_RIPEN); };
ABILITY(ABILITY_SUGAR_RUSH) : extends AbilityImpl<ABILITY_UNBURDEN>, extends AbilityImpl<ABILITY_RIPEN> { INSTANCE(ABILITY_SUGAR_RUSH); };

ABILITY(ABILITY_PEACEFUL_REST) : extends OnEndTurn {
    INSTANCE(ABILITY_PEACEFUL_REST);
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

ABILITY(ABILITY_WHITE_NOISE) : extends AbilityImpl<ABILITY_PEACEFUL_REST>, extends AbilityImpl<ABILITY_STATIC> { INSTANCE(ABILITY_WHITE_NOISE); };

ABILITY(ABILITY_SMOKEY_MANEUVERS) : extends OnAccuracy<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_SMOKEY_MANEUVERS);
    ON_ACCURACY {
        CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
        *accuracy /= 1.25;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_POWER_METAL) : LiquidVoiceClone<TYPE_STEEL> { INSTANCE(ABILITY_POWER_METAL); };

ABILITY(ABILITY_POWER_EDGE) : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends OnChooseDefensiveStat<> {
    INSTANCE(ABILITY_POWER_EDGE);
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        return STAT_SPDEF;
    }
};

ABILITY(ABILITY_SUPERCONDUCTOR) : extends OnOffensiveMultiplier<>, extends OnMoveType {
    INSTANCE(ABILITY_SUPERCONDUCTOR);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
    }
    ON_MOVE_TYPE {
        CHECK(moveType == TYPE_STEEL)
        *ateBoost = TRUE;
        return TYPE_ELECTRIC + 1;
    }
};

ABILITY(ABILITY_ULTRA_INSTINCT) : extends OnDefender, extends OnDefensiveMultiplier, extends OverrideBreakable {
    INSTANCE(ABILITY_ULTRA_INSTINCT);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
        return FALSE;
    }
    ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
};

ABILITY(ABILITY_UNLOCKED_POTENTIAL) : extends AbilityImpl<ABILITY_BERSERK>, extends AbilityImpl<ABILITY_INNER_FOCUS> { INSTANCE(ABILITY_UNLOCKED_POTENTIAL); };

ABILITY(ABILITY_HIGHER_RANK) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_HIGHER_RANK);
    ON_OFFENSIVE_MULTIPLIER {
        if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
    }
};

ABILITY(ABILITY_FUNERAL_PYRE) : extends OnEntry, extends OnEndTurn {
    INSTANCE(ABILITY_FUNERAL_PYRE);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE); }
    ON_END_TURN {
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
    }
};

ABILITY(ABILITY_FLAME_BUBBLE) : extends AbilityImpl<ABILITY_WATER_BUBBLE>, extends AbilityImpl<ABILITY_FLAMING_SOUL> { INSTANCE(ABILITY_FLAME_BUBBLE); };

ABILITY(ABILITY_ELEMENTAL_VORTEX) : extends Merged<ABILITY_WATER_ABSORB, ABILITY_FLASH_FIRE> { INSTANCE(ABILITY_ELEMENTAL_VORTEX); };

ABILITY(ABILITY_SNOWY_WRATH) : extends AbilityImpl<ABILITY_SNOW_WARNING>, extends AbilityImpl<ABILITY_CRYOMANCY> { INSTANCE(ABILITY_SNOWY_WRATH); };

ABILITY(ABILITY_PATTERN_CHANGE) : extends AbilityImpl<ABILITY_SHED_SKIN>, extends AbilityImpl<ABILITY_PROTEAN> { INSTANCE(ABILITY_PATTERN_CHANGE); };

ABILITY(ABILITY_NO_TURNING_BACK) : extends OnDefender {
    INSTANCE(ABILITY_NO_TURNING_BACK);
    ON_DEFENDER {
        CHECK(CheckHalfHpAbility(battler, attacker))
        CHECK_NOT(GetAbilityState(battler, ability))
        CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_NoTurningBack);
        return TRUE;
    }
};

ABILITY(ABILITY_FLAMMABLE_COAT) : extends FormChangeAbility, extends OnDefender, extends OnBeforeAttack<> {
    INSTANCE(ABILITY_FLAMMABLE_COAT);
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

ABILITY(ABILITY_DRACO_MORALE) : extends SimpleEntryMove<MOVE_DRAGON_CHEER> { INSTANCE(ABILITY_DRACO_MORALE); };

ABILITY(ABILITY_BAD_OMEN) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_BAD_OMEN);
    ON_DEFENSIVE_MULTIPLIER {
        if (isCrit) MUL(.25);
    }
};

ABILITY(ABILITY_MOSH_PIT) : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
    INSTANCE(ABILITY_MOSH_PIT);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST)
            MUL(1.25);
        else
            MUL(1.5);
    }
};

ABILITY(ABILITY_BLOOD_STAIN) : extends OnEither, extends OnEntry, extends Unsuppressable, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_BLOOD_STAIN);
    ON_EITHER {
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
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN); }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

ABILITY(ABILITY_BLOOD_STIGMA) : extends OnOffensiveMultiplier<>, extends RemovesStatusOnImmunity, extends Unsuppressable {
    INSTANCE(ABILITY_BLOOD_STIGMA);
    ON_OFFENSIVE_MULTIPLIER {
        if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

ABILITY(ABILITY_SLIPSTREAM) : extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_SLIPSTREAM);
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPEED] += 20; }
};

ABILITY(ABILITY_MAXIMUM_ACCELERATION) : extends AbilityImpl<ABILITY_SLIPSTREAM>, extends AbilityImpl<ABILITY_SPEED_BOOST> {
    INSTANCE(ABILITY_MAXIMUM_ACCELERATION);
};

ABILITY(ABILITY_SIDEWINDER) : extends AbilityImpl<ABILITY_COIL_UP>, extends OnBattlerFaints<> {
    INSTANCE(ABILITY_SIDEWINDER);
    ON_BATTLER_FAINTS {
        CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
        gStatuses4[battler] |= STATUS4_COILED;
        SetAbilityState(battler, ability, TRUE);
        BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
        return TRUE;
    }
};

ABILITY(ABILITY_PETRIFY) : extends AbilityImpl<ABILITY_INTIMIDATE> {
    INSTANCE(ABILITY_PETRIFY);
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

ABILITY(ABILITY_FLUFFIEST) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_FLUFFIEST);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(2.0);
        if (IsMoveMakingContact(move, attacker)) MUL(0.5);
    }
};

ABILITY(ABILITY_WAY_OF_PRECISION) : extends AbilityImpl<ABILITY_INNER_FOCUS>, extends AbilityImpl<ABILITY_PRECISE_FIST> { INSTANCE(ABILITY_WAY_OF_PRECISION); };

ABILITY(ABILITY_WAY_OF_SWIFTNESS) : extends AbilityImpl<ABILITY_PRETENTIOUS>, extends AbilityImpl<ABILITY_SWIFT_SWIM> { INSTANCE(ABILITY_WAY_OF_SWIFTNESS); };

ABILITY(ABILITY_ATOMIC_PUNCH) : extends AbilityImpl<ABILITY_IRON_FIST> {
    INSTANCE(ABILITY_ATOMIC_PUNCH);
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_IRON_FIST>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        if (moveType == TYPE_STEEL) MUL(1.3);
    }
};

ABILITY(ABILITY_IRON_GIANT) : extends AbilityImpl<ABILITY_HEATPROOF>, extends AbilityImpl<ABILITY_JUGGERNAUT> { INSTANCE(ABILITY_IRON_GIANT); };

ABILITY(ABILITY_MASTER_HAND) : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_MEGA_LAUNCHER> { INSTANCE(ABILITY_MASTER_HAND); };

ABILITY(ABILITY_FINAL_BLOW) : extends AbilityImpl<ABILITY_FATAL_PRECISION> { INSTANCE(ABILITY_FINAL_BLOW); };

ABILITY(ABILITY_HOSPITALITY) : extends OnEntry {
    INSTANCE(ABILITY_HOSPITALITY);
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

ABILITY(ABILITY_BUTTER_UP) : extends Merged<ABILITY_HOSPITALITY, ABILITY_SOOTHING_AROMA> { INSTANCE(ABILITY_BUTTER_UP); };

ABILITY(ABILITY_VITALITY_STRIKE) : extends OnAttacker {
    INSTANCE(ABILITY_VITALITY_STRIKE);
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

ABILITY(ABILITY_HUGE_WINGS) : extends Merged<ABILITY_GIANT_WINGS, ABILITY_LEVITATE> { INSTANCE(ABILITY_HUGE_WINGS); };

ABILITY(ABILITY_SWORD_OF_DAMNATION) : extends AbilityImpl<ABILITY_SWORD_OF_RUIN>, extends AbilityImpl<ABILITY_UNAWARE> {
    INSTANCE(ABILITY_SWORD_OF_DAMNATION);
};

ABILITY(ABILITY_RESTRAINING_ORDER) : extends OnDefender {
    INSTANCE(ABILITY_RESTRAINING_ORDER);
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

ABILITY(ABILITY_ASSASSINS_TOOLS) : extends OnAttacker {
    INSTANCE(ABILITY_ASSASSINS_TOOLS);
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

ABILITY(ABILITY_FROSTMAW) : extends OnAttacker {
    INSTANCE(ABILITY_FROSTMAW);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanGetFrostbite(target))
        CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
    }
};

ABILITY(ABILITY_PATCHWORK) : extends AbilityImpl<ABILITY_DISGUISE>, extends OnDefender {
    INSTANCE(ABILITY_PATCHWORK);
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

ABILITY(ABILITY_BLIND_RAGE) : extends AbilityImpl<ABILITY_MOLD_BREAKER>, extends AbilityImpl<ABILITY_SCRAPPY> { INSTANCE(ABILITY_BLIND_RAGE); };

ABILITY(ABILITY_APEX_PREDATOR) : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_TOUGH_CLAWS> { INSTANCE(ABILITY_APEX_PREDATOR); };

ABILITY(ABILITY_DRAGONS_RITUAL) : extends OnBattlerFaints<> {
    INSTANCE(ABILITY_DRAGONS_RITUAL);
    ON_BATTLER_FAINTS {
        CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
        BattleScriptCall(BattleScript_DragonsRitual);
        return TRUE;
    }
};

ABILITY(ABILITY_PINNACLE_BLADE) : extends OnInfiltrate, extends OnAttacker {
    INSTANCE(ABILITY_PINNACLE_BLADE);
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

ABILITY(ABILITY_ENERGIZED) : extends AbilityImpl<ABILITY_GENERATOR>, extends OnBattlerFaints<> {
    INSTANCE(ABILITY_ENERGIZED);
    ON_BATTLER_FAINTS {
        CHECK(moveType == TYPE_ELECTRIC);
        SetOncePerTurnAbilityCounter(battler, ability, TRUE);
        BattleScriptCall(BattleScript_GeneratorActivatesRet);
        return TRUE;
    }
};

ABILITY(ABILITY_COLOR_SPECTRUM) : extends OnEndTurn, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_COLOR_SPECTRUM);
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

ABILITY(ABILITY_STEEL_BEETLE) : extends AbilityImpl<ABILITY_RAGING_BOXER>, extends AbilityImpl<ABILITY_POLLINATE> { INSTANCE(ABILITY_STEEL_BEETLE); };

ABILITY(ABILITY_FROM_THE_SHADOWS) : extends OnAttacker {
    INSTANCE(ABILITY_FROM_THE_SHADOWS);
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

ABILITY(ABILITY_RAGE_POINT) : OnDefender, extends OnOffensiveMultiplier<>, extends NegateBurnAtkDrop, extends NegateFrzSpatkDrop {
    INSTANCE(ABILITY_RAGE_POINT);
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

ABILITY(ABILITY_HOT_COALS) : extends OnEntry {
    INSTANCE(ABILITY_HOT_COALS);
    ON_ENTRY {
        CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

        gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
        return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
    }
};

ABILITY(ABILITY_TERASTAL_TREASURE) : extends OnDefensiveMultiplier, extends OnStat<> {
    INSTANCE(ABILITY_TERASTAL_TREASURE);
    ON_DEFENSIVE_MULTIPLIER { MUL(.6); }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .8;
    }
};

ABILITY(ABILITY_SHOCKING_MAW) : extends AbilityImpl<ABILITY_SHOCKING_JAWS>, extends AbilityImpl<ABILITY_STRONG_JAW> { INSTANCE(ABILITY_SHOCKING_MAW); };

ABILITY(ABILITY_GLEAM_EYES) : extends Merged<ABILITY_INTIMIDATE, ABILITY_FRISK> { INSTANCE(ABILITY_GLEAM_EYES); };

ABILITY(ABILITY_ROUSED_FANGS) : extends AbilityImpl<ABILITY_STRONG_JAW>, extends AbilityImpl<ABILITY_MIND_CRUSH> { INSTANCE(ABILITY_ROUSED_FANGS); };

ABILITY(ABILITY_DREAM_STATE) : extends AbilityImpl<ABILITY_BATTLE_ARMOR> { INSTANCE(ABILITY_DREAM_STATE); };

ABILITY(ABILITY_DREAM_WHIMSY) : extends SimpleEntryMove<MOVE_YAWN> { INSTANCE(ABILITY_DREAM_WHIMSY); };

ABILITY(ABILITY_LUNAR_AFFINITY) : extends OnCopyMove {
    INSTANCE(ABILITY_LUNAR_AFFINITY);
    ON_COPY_MOVE {
        CHECK(gBattleMoves[move].lunar)
        return UseOutOfTurnAttack(battler, target, ability, move, 0);
    }
};

ABILITY(ABILITY_FLAME_SHIELD) : extends AbilityImpl<ABILITY_FILTER> { INSTANCE(ABILITY_FLAME_SHIELD); };

ABILITY(ABILITY_AQUATIC_DWELLER) : extends AbilityImpl<ABILITY_AQUATIC>, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_AQUATIC_DWELLER);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER) MUL(1.5);
    }
};

ABILITY(ABILITY_APPLE_PIE) : extends AbilityImpl<ABILITY_SELF_SUFFICIENT> { INSTANCE(ABILITY_APPLE_PIE); };

ABILITY(ABILITY_HOVER) : extends GroundImmune, AddsType<TYPE_PSYCHIC> { INSTANCE(ABILITY_HOVER); };

ABILITY(ABILITY_DEPRAVITY) : extends AbilityImpl<ABILITY_MERCILESS>, extends AbilityImpl<ABILITY_OVERCHARGE> { INSTANCE(ABILITY_DEPRAVITY); };

ABILITY(ABILITY_WILDFIRE) : extends SimpleEntryMove<MOVE_FIRE_SPIN> { INSTANCE(ABILITY_WILDFIRE); };

ABILITY(ABILITY_JUMP_SCARE) : extends OnEntry, extends Persistent {
    INSTANCE(ABILITY_JUMP_SCARE);
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
    }
};

ABILITY(ABILITY_TAR_TOSS) : extends SimpleEntryMove<MOVE_TAR_SHOT> { INSTANCE(ABILITY_TAR_TOSS); };

ABILITY(ABILITY_STUN_SHOCK) : extends OnAttacker {
    INSTANCE(ABILITY_STUN_SHOCK);
    ON_ATTACKER {
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
    }
};

ABILITY(ABILITY_RAGING_GODDESS) : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE> { INSTANCE(ABILITY_RAGING_GODDESS); };

ABILITY(ABILITY_WHIPLASH) : extends OnAttacker {
    INSTANCE(ABILITY_WHIPLASH);
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

ABILITY(ABILITY_SUPERSWEET_SYRUP) : extends OnDefender {
    INSTANCE(ABILITY_SUPERSWEET_SYRUP);
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

ABILITY(ABILITY_TRASH_HEAP) : extends AbilityImpl<ABILITY_TOXIC_SPILL>, extends AbilityImpl<ABILITY_CORROSION> { INSTANCE(ABILITY_TRASH_HEAP); };

ABILITY(ABILITY_SLUDGY_MIX) : extends AbilityImpl<ABILITY_INTOXICATE>, extends AbilityImpl<ABILITY_PUNK_ROCK> { INSTANCE(ABILITY_SLUDGY_MIX); };

ABILITY(ABILITY_OVERWATCH) : extends AbilityImpl<ABILITY_ON_THE_PROWL>, extends AbilityImpl<ABILITY_STAKEOUT> { INSTANCE(ABILITY_OVERWATCH); };

ABILITY(ABILITY_WIND_RAGE) : extends AbilityImpl<ABILITY_GIANT_WINGS>, extends SimpleEntryMove<MOVE_DEFOG> { INSTANCE(ABILITY_WIND_RAGE); };

ABILITY(ABILITY_VICTORY_BOMB) : extends OnDefender, extends OnMoveType {
    INSTANCE(ABILITY_VICTORY_BOMB);
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

ABILITY(ABILITY_RAZOR_SHARP) : extends OnAttacker {
    INSTANCE(ABILITY_RAZOR_SHARP);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gIsCriticalHit)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

ABILITY(ABILITY_TO_THE_BONE) : extends AbilityImpl<ABILITY_RAZOR_SHARP>, extends AbilityImpl<ABILITY_SNIPER> { INSTANCE(ABILITY_TO_THE_BONE); };

ABILITY(ABILITY_BLADE_DANCE) : extends OnAttacker {
    INSTANCE(ABILITY_BLADE_DANCE);
    ON_ATTACKER {
        CHECK(IsDance(battler, move))
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
    }
};

ABILITY(ABILITY_APE_SHIFT) : extends FormChangeAbility, extends OnEntry, extends OnEndTurn, extends OnDefender, extends OnCrit<> {
    INSTANCE(ABILITY_APE_SHIFT);
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

ABILITY(ABILITY_KNOW_YOUR_PLACE) : extends OnAttacker {
    INSTANCE(ABILITY_KNOW_YOUR_PLACE);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK_NOT(gVolatileStructs[target].dazed)
        CHECK(IsMoveMakingContact(move, battler))

        gVolatileStructs[target].dazed = 5;
        BattleScriptCall(BattleScript_TargetDazed);
        return TRUE;
    }
};

ABILITY(ABILITY_DEEP_CUTS) : extends OnAttacker {
    INSTANCE(ABILITY_DEEP_CUTS);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
        CHECK(Random() % 2)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

ABILITY(ABILITY_LIFE_STEAL) : extends OnEndTurn {
    INSTANCE(ABILITY_LIFE_STEAL);
    ON_END_TURN {
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
    }
};

ABILITY(ABILITY_RUDE_AWAKENING) : extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_RUDE_AWAKENING);
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_SLEEP)
        CHECK(GetAbilityState(battler, ability))
        return TRUE;
    }
};

ABILITY(ABILITY_TERAFORM_ZERO) : extends AbilityImpl<ABILITY_TERA_SHELL>, extends OnEntry {
    INSTANCE(ABILITY_TERAFORM_ZERO);
    ON_ENTRY {
        CHECK(!GetSingleUseAbilityCounter(battler, ability));
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
        BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
        return TRUE;
    }
};

ABILITY(ABILITY_SET_ABLAZE) : extends PoisonPuppeteerLike<MOVE_EFFECT_BURN> {
    INSTANCE(ABILITY_SET_ABLAZE);
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
    }
};

ABILITY(ABILITY_BREAKWATER) : extends AbilityImpl<ABILITY_STALL>, extends AbilityImpl<ABILITY_SWIFT_SWIM> { INSTANCE(ABILITY_BREAKWATER); };

ABILITY(ABILITY_MAGICAL_FISTS) : extends AbilityImpl<ABILITY_IRON_FIST>, extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_MAGICAL_FISTS);
    ON_CHOOSE_OFFENSIVE_STAT {
        if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
    }
};

ABILITY(ABILITY_CUTTHROAT) : extends OnEntry {
    INSTANCE(ABILITY_CUTTHROAT);
    ON_ENTRY {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
    }
};

ABILITY(ABILITY_SAND_BENDER) : extends AbilityImpl<ABILITY_SAND_STREAM>, extends AbilityImpl<ABILITY_SAND_FORCE> { INSTANCE(ABILITY_SAND_BENDER); };

ABILITY(ABILITY_SAND_PIT) : extends SimpleEntryMove<MOVE_SAND_TOMB, 20> { INSTANCE(ABILITY_SAND_PIT); };

ABILITY(ABILITY_DESOLATE_SUN) : extends RandomizerBanned { INSTANCE(ABILITY_DESOLATE_SUN); };

ABILITY(ABILITY_DAYBREAK) : extends OnEither {
    INSTANCE(ABILITY_DAYBREAK);
    ON_EITHER {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
};

ABILITY(ABILITY_ENERGY_SIPHON) : extends OnAttacker {
    INSTANCE(ABILITY_ENERGY_SIPHON);
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

ABILITY(ABILITY_RESERVOIR) : extends Merged<ABILITY_WATER_ABSORB, ABILITY_STORM_DRAIN> { INSTANCE(ABILITY_RESERVOIR); };

static int NeurotoxinCondition(int battler, int target) {
    return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
}
ABILITY(ABILITY_NEUROTOXIN) : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    INSTANCE(ABILITY_NEUROTOXIN);
    ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); }
};

ABILITY(ABILITY_ENERGIZED_HORNS) : extends AbilityImpl<ABILITY_MIGHTY_HORN> {
    INSTANCE(ABILITY_ENERGIZED_HORNS);
    ON_SWAP_SPLIT {
        CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
        CHECK(gBattleMoves[move].hornBased);
        return TRUE;
    }
};

ABILITY(ABILITY_SPIDER_LAIR_UPGRADE) : extends OnEntry {
    INSTANCE(ABILITY_SPIDER_LAIR_UPGRADE);
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

ABILITY(ABILITY_CRUST_COAT) : extends AbilityImpl<ABILITY_BATTLE_ARMOR> { INSTANCE(ABILITY_CRUST_COAT); };

ABILITY(ABILITY_PUFFY) : extends AbilityImpl<ABILITY_FLUFFY> { INSTANCE(ABILITY_PUFFY); };

ABILITY(ABILITY_BALLOON_BLITZ) : extends AbilityImpl<ABILITY_INFLATABLE>, extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE> { INSTANCE(ABILITY_BALLOON_BLITZ); };

ABILITY(ABILITY_STRIKER_PIXILATE) : extends AbilityImpl<ABILITY_STRIKER>, extends AbilityImpl<ABILITY_PIXILATE> { INSTANCE(ABILITY_STRIKER_PIXILATE); };

// 2.6
ABILITY(ABILITY_DOOM_BLAST) : extends OnRecoil, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_DOOM_BLAST);
    ON_RECOIL {
        CHECK(moveType == TYPE_DARK);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 20, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_DARK) MUL(1.35);
    }
};

ABILITY(ABILITY_BRUTEFORCE) : extends AbilityImpl<ABILITY_RECKLESS>, extends AbilityImpl<ABILITY_ROCK_HEAD> { INSTANCE(ABILITY_BRUTEFORCE); };

ABILITY(ABILITY_FARADAY_CAGE) : extends AbilityImpl<ABILITY_SHELL_ARMOR>, extends OnDefender {
    INSTANCE(ABILITY_FARADAY_CAGE);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
        return FALSE;
    }
};

ABILITY(ABILITY_ACIDIC_SLIME) : extends AbilityImpl<ABILITY_CORROSION>, extends OnStab {
    INSTANCE(ABILITY_ACIDIC_SLIME);
    ON_STAB { return moveType == TYPE_WATER; }
};

ABILITY(ABILITY_ROSE_GARDEN) : extends OnEntry {
    INSTANCE(ABILITY_ROSE_GARDEN);
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

ABILITY(ABILITY_QIGONG) : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_FIGHT_SPIRIT>, extends OnAccuracy<> {
    INSTANCE(ABILITY_QIGONG);
    ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
};

ABILITY(ABILITY_CONJOURER_OF_DECEIT) : extends AbilityImpl<ABILITY_MAGIC_GUARD>, extends AbilityImpl<ABILITY_MAGIC_BOUNCE> {
    INSTANCE(ABILITY_CONJOURER_OF_DECEIT);
};

ABILITY(ABILITY_DEEP_FREEZE) : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_DEEP_FREEZE);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_ICE) MUL(1.25);
    }
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE) RESISTANCE(.5);
    }
};

ABILITY(ABILITY_SOUL_DEVOURER) : extends AbilityImpl<ABILITY_SOUL_EATER>, extends AbilityImpl<ABILITY_PHANTOM_PAIN> { INSTANCE(ABILITY_SOUL_DEVOURER); };

ABILITY(ABILITY_CHAMPIONS_ENTRANCE) : extends Merged<ABILITY_INTIMIDATE, ABILITY_VIOLENT_RUSH> { INSTANCE(ABILITY_CHAMPIONS_ENTRANCE); };

ABILITY(ABILITY_PRESTO) : extends OnPriority {
    INSTANCE(ABILITY_PRESTO);
    ON_PRIORITY {
        CHECK(BATTLER_MAX_HP(battler))
        CHECK(IsSoundMove(battler, move))
        return 1;
    }
};

ABILITY(ABILITY_SAMBA) : extends AbilityImpl<ABILITY_STRIKER>, extends AbilityImpl<ABILITY_DANCER> { INSTANCE(ABILITY_SAMBA); };

ABILITY(ABILITY_GLADIATOR) : extends BoostedSwarmLike<TYPE_FIGHTING> { INSTANCE(ABILITY_GLADIATOR); };

ABILITY(ABILITY_FORSAKEN_HEART) : extends OnBattlerFaints<ApplyOnTarget::ANY> {
    INSTANCE(ABILITY_FORSAKEN_HEART);
    ON_BATTLER_FAINTS {
        CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

        BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
        return TRUE;
    }
};

ABILITY(ABILITY_RELENTLESS) : extends AbilityImpl<ABILITY_EXPLOIT_WEAKNESS>, extends AbilityImpl<ABILITY_MERCILESS> { INSTANCE(ABILITY_RELENTLESS); };

ABILITY(ABILITY_SOOTHSAYER) : extends OnEntry, extends OnEndTurn, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends Persistent {
    INSTANCE(ABILITY_SOOTHSAYER);
    ON_ENTRY {
        CHECK(!GetSingleUseAbilityCounter(battler, ability))
        SetSingleUseAbilityCounter(battler, ability, TRUE);
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

ABILITY(ABILITY_CORRUPTED_MIND) : extends RandomizerBanned, extends OnTypeEffectiveness<>, extends OnModifyEffectChance<> {
    INSTANCE(ABILITY_CORRUPTED_MIND);
    ON_TYPE_EFFECTIVENESS {
        CHECK(moveType == TYPE_PSYCHIC)
        if (*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        return FALSE;
    }
    ON_MODIFY_EFFECT_CHANCE {
        int type;
        GET_MOVE_TYPE(move, type)
        if (type == TYPE_PSYCHIC) *effectChance *= 1.4;
    }
};

ABILITY(ABILITY_FLAME_COAT) : extends OnEntry, extends OnEndTurn {
    INSTANCE(ABILITY_FLAME_COAT);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FIRE_COAT); }
    ON_END_TURN {
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
    }
};

ABILITY(ABILITY_UNOWN_POWER) : extends RandomizerBanned, extends OnStab, extends OnAfterTypeEffectiveness<> {
    INSTANCE(ABILITY_UNOWN_POWER);
    ON_STAB { return TRUE; }
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
    }
};

ABILITY(ABILITY_SUPER_SCOPE) : extends AbilityImpl<ABILITY_MEGA_LAUNCHER>, extends AbilityImpl<ABILITY_ARTILLERY> { INSTANCE(ABILITY_SUPER_SCOPE); };

ABILITY(ABILITY_VENOM_CROWN) : extends AbilityImpl<ABILITY_POISON_POINT>, extends AbilityImpl<ABILITY_MIGHTY_HORN>, extends RandomizerBanned {
    INSTANCE(ABILITY_VENOM_CROWN);
};

ABILITY(ABILITY_BLIGHT_SCALE) : extends AbilityImpl<ABILITY_POISON_POINT>, extends AbilityImpl<ABILITY_MULTISCALE>, extends RandomizerBanned {
    INSTANCE(ABILITY_BLIGHT_SCALE);
};

ABILITY(ABILITY_GUNMAN) : extends AbilityImpl<ABILITY_MEGA_LAUNCHER>, extends OnModifyMoveFlags {
    INSTANCE(ABILITY_GUNMAN);
    ON_MODIFY_MOVE_FLAGS {
        CHECK(flag == MOVE_FLAG_MEGA_LAUNCHER)
        CHECK(IS_MOVE_STATUS(move))
        return TRUE;
    }
};

ABILITY(ABILITY_CARETAKER) : extends OnEndTurn, extends AbilityImpl<ABILITY_FRIEND_GUARD> {
    INSTANCE(ABILITY_CARETAKER);
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

ABILITY(ABILITY_POSEIDONS_DOMINION) : extends SimpleEntryMove<MOVE_WHIRLPOOL> { INSTANCE(ABILITY_POSEIDONS_DOMINION); };

ABILITY(ABILITY_DUAL_SHADOW) : extends AbilityImpl<ABILITY_HUNGER_SWITCH>, extends OnRecoil, extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_DUAL_SHADOW);
    ON_RECOIL {
        CHECK(moveType == TYPE_ELECTRIC || moveType == TYPE_DARK);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
        return max(damage / 10, 1);
    }
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ELECTRIC || moveType == TYPE_DARK) MUL(1.35);
    }
};

ABILITY(ABILITY_LULLABY) : extends OnAccuracy<> {
    INSTANCE(ABILITY_LULLABY);
    ON_ACCURACY {
        CHECK(move == MOVE_SING);
        *accuracy *= 1.5;
        return ACCURACY_MULTIPLICATIVE;
    }
};

ABILITY(ABILITY_CRYO_ARCHITECT) : extends OnEndTurn, extends OnDefender {
    INSTANCE(ABILITY_CRYO_ARCHITECT);
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

ABILITY(ABILITY_GLACIAL_RAGE) : extends OnAttacker {
    INSTANCE(ABILITY_GLACIAL_RAGE);
    ON_ATTACKER {
        CHECK(moveType == TYPE_ICE)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
    }
};

ABILITY(ABILITY_IMMOVABLE_OBJECT) : extends AbilityImpl<ABILITY_MAGIC_GUARD>, extends AbilityImpl<ABILITY_STURDY> { INSTANCE(ABILITY_IMMOVABLE_OBJECT); };

ABILITY(ABILITY_FRENZIED_PHANTOM) : extends AbilityImpl<ABILITY_SHADOW_TAG>, extends AbilityImpl<ABILITY_HYPER_AGGRESSIVE> {
    INSTANCE(ABILITY_FRENZIED_PHANTOM);
};

struct DNAScramble : extends FormChangeAbility, extends OnBeforeAttack<> {
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

ABILITY(ABILITY_METALLIC_JAWS) : extends AbilityImpl<ABILITY_METALLIC>, extends AbilityImpl<ABILITY_PRIMAL_MAW> { INSTANCE(ABILITY_METALLIC_JAWS); };

ABILITY(ABILITY_CALCULATIVE) : extends Merged<ABILITY_ANALYTIC, ABILITY_NEUROFORCE> { INSTANCE(ABILITY_CALCULATIVE); };

ABILITY(ABILITY_EMBODY_ASPECT) : extends RaiseStatOnEntry<STAT_SPEED> { INSTANCE(ABILITY_EMBODY_ASPECT); };

ABILITY(ABILITY_EMBODY_ASPECT_HEARTHFLAME) : extends AbilityImpl<ABILITY_INTREPID_SWORD> { INSTANCE(ABILITY_EMBODY_ASPECT_HEARTHFLAME); };

ABILITY(ABILITY_EMBODY_ASPECT_CORNERSTONE) : extends AbilityImpl<ABILITY_DAUNTLESS_SHIELD> { INSTANCE(ABILITY_EMBODY_ASPECT_CORNERSTONE); };

ABILITY(ABILITY_EMBODY_ASPECT_WELLSPRING) : extends RaiseStatOnEntry<STAT_SPDEF> { INSTANCE(ABILITY_EMBODY_ASPECT_WELLSPRING); };

ABILITY(ABILITY_ROCKHARD_SHAFT) : extends BoostedSwarmLike<TYPE_ROCK> { INSTANCE(ABILITY_ROCKHARD_SHAFT); };

ABILITY(ABILITY_HUNTERS_MARK) : extends AbilityImpl<ABILITY_DEADEYE>, extends AbilityImpl<ABILITY_AMBUSH> { INSTANCE(ABILITY_HUNTERS_MARK); };

ABILITY(ABILITY_DEVIATE) : extends AteAbility<TYPE_DARK> { INSTANCE(ABILITY_DEVIATE); };

ABILITY(ABILITY_SUNS_BOUNTY) : extends Merged<ABILITY_HARVEST, ABILITY_LEAF_GUARD> { INSTANCE(ABILITY_SUNS_BOUNTY); };

ABILITY(ABILITY_RITE_OF_SPRING) : extends Merged<ABILITY_SOLAR_POWER, ABILITY_CHLOROPHYLL> { INSTANCE(ABILITY_RITE_OF_SPRING); };

ABILITY(ABILITY_HEADSTRONG) : extends RaiseStatOnEntry<STAT_SPDEF> { INSTANCE(ABILITY_HEADSTRONG); };

ABILITY(ABILITY_FIREFIGHTER) : extends TypeSlayer<TYPE_FIRE> { INSTANCE(ABILITY_FIREFIGHTER); };

ABILITY(ABILITY_SEPIA_LENS) : extends AbilityImpl<ABILITY_SAND_GUARD>, extends AbilityImpl<ABILITY_TINTED_LENS> { INSTANCE(ABILITY_SEPIA_LENS); };

ABILITY(ABILITY_SUPER_SNIPER) : extends AbilityImpl<ABILITY_SNIPER>, extends UseTurnAttackAsPursuit {
    INSTANCE(ABILITY_SUPER_SNIPER);
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_SNIPER>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
            MUL(0.5);
        }
    }
};

ABILITY(ABILITY_WOODLAND_CURSE) : extends OnEither, extends SimpleEntryMove<MOVE_FORESTS_CURSE> {
    INSTANCE(ABILITY_WOODLAND_CURSE);
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

ABILITY(ABILITY_MALODOR) : extends OnDefender {
    INSTANCE(ABILITY_MALODOR);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))
        CHECK_NOT(gStatuses3[attacker] & STATUS3_GASTRO_ACID)

        gStatuses3[attacker] |= STATUS3_GASTRO_ACID;
        BattleScriptCall(BattleScript_StackAbilitySuppressedMessage);
        return TRUE;
    }
};

ABILITY(ABILITY_BLUR) : extends OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_BLUR);
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        return STAT_SPEED;
    }
};

ABILITY(ABILITY_ELUDE) : extends OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
    INSTANCE(ABILITY_ELUDE);
    ON_CHOOSE_DEFENSIVE_STAT {
        CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker))
        return STAT_SPEED;
    }
};

ABILITY(ABILITY_DRAKE_OF_RAGE) : extends AbilityImpl<ABILITY_RAMPAGE>, extends AbilityImpl<ABILITY_TINTED_LENS> { INSTANCE(ABILITY_DRAKE_OF_RAGE); };

ABILITY(ABILITY_MIXED_MARTIAL_ARTS) : extends OnModifyMoveFlags {
    INSTANCE(ABILITY_MIXED_MARTIAL_ARTS);
    ON_MODIFY_MOVE_FLAGS {
        CHECK(flag == MOVE_FLAG_PUNCH || flag == MOVE_FLAG_KICK)
        CHECK(gBattleMoves[move].type == TYPE_NORMAL)
        return TRUE;
    }
};

ABILITY(ABILITY_STRATEGIC_PAUSE) : extends AbilityImpl<ABILITY_ANALYTIC>, extends OnCrit<> {
    INSTANCE(ABILITY_STRATEGIC_PAUSE);
    ON_CRIT {
        CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber)
        CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT)
        return 2;
    }
};

ABILITY(ABILITY_OVERRULE) : extends OnAfterTypeEffectiveness<> {
    INSTANCE(ABILITY_OVERRULE);
    ON_AFTER_TYPE_EFFECTIVENESS {
        if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
    }
};

ABILITY(ABILITY_MENTAL_POLLUTION) : extends NotImplemented { INSTANCE(ABILITY_MENTAL_POLLUTION); };

ABILITY(ABILITY_MADNESS_ENHANCEMENT) : extends NotImplemented { INSTANCE(ABILITY_MADNESS_ENHANCEMENT); };

ABILITY(ABILITY_TENTALOCK) : extends NotImplemented { INSTANCE(ABILITY_TENTALOCK); };

ABILITY(ABILITY_SERPENT_BIND) : extends NotImplemented { INSTANCE(ABILITY_SERPENT_BIND); };

ABILITY(ABILITY_SOUL_TAP) : extends OnEndTurn {
    INSTANCE(ABILITY_SOUL_TAP);
    ON_END_TURN {
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
    }
};

ABILITY(ABILITY_SCARECROW) : extends AbilityImpl<ABILITY_INTIMIDATE>, extends AbilityImpl<ABILITY_BAD_LUCK> { INSTANCE(ABILITY_SCARECROW); };

ABILITY(ABILITY_OMINOUS_SHROUD) : extends AbilityImpl<ABILITY_PHANTOM>, extends AbilityImpl<ABILITY_SHADOW_SHIELD> { INSTANCE(ABILITY_OMINOUS_SHROUD); };

ABILITY(ABILITY_CHILLING_PRESENCE) : extends SimpleEntryMove<MOVE_ICY_WIND, 10> { INSTANCE(ABILITY_CHILLING_PRESENCE); };

ABILITY(ABILITY_FROSTBIND) : extends PoisonPuppeteerLike<MOVE_EFFECT_FROSTBITE> {
    INSTANCE(ABILITY_FROSTBIND);
    ON_REACTIVE {
        return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) { return (int)CanGetFrostbite(battler); }, BattleScript_Frostbind);
    }
};

ABILITY(ABILITY_TENDER_AFFECTION) : extends AbilityImpl<ABILITY_CUTE_CHARM>, extends OnStab {
    INSTANCE(ABILITY_TENDER_AFFECTION);
    ON_STAB { return moveType == TYPE_FAIRY; }
};

ABILITY(ABILITY_GLACIAL_GHOST) : extends AbilityImpl<ABILITY_SLUSH_RUSH>, extends AbilityImpl<ABILITY_SNOW_CLOAK> { INSTANCE(ABILITY_GLACIAL_GHOST); };

ABILITY(ABILITY_WONDER_SCALE) : extends AbilityImpl<ABILITY_SHED_SKIN>, extends AbilityImpl<ABILITY_FORT_KNOX> { INSTANCE(ABILITY_WONDER_SCALE); };

ABILITY(ABILITY_OVERZEALOUS) : extends NotImplemented { INSTANCE(ABILITY_OVERZEALOUS); };

ABILITY(ABILITY_STAINLESS_STEEL) : extends AteAbility<TYPE_STEEL>, extends AbilityImpl<ABILITY_FORT_KNOX> { INSTANCE(ABILITY_STAINLESS_STEEL); };

ABILITY(ABILITY_TEMPORAL_RUPTURE) : extends NotImplemented { INSTANCE(ABILITY_TEMPORAL_RUPTURE); };

ABILITY(ABILITY_GRASS_FLUTE) : extends OnAttacker {
    INSTANCE(ABILITY_GRASS_FLUTE);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(IsSoundMove(battler, move))
        CHECK_NOT(gVolatileStructs[target].fear)

        return AbilityStatusEffect(MOVE_EFFECT_FEAR);
    }
};

ABILITY(ABILITY_HEMOTOXIN) : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
    INSTANCE(ABILITY_HEMOTOXIN);
    ON_REACTIVE {
        return PoisonPuppeteerClone(
            ability,
            battler,
            [](int battler, int target) -> int { return !(gStatuses3[target] & STATUS3_GASTRO_ACID); },
            BattleScript_StackAbilitySuppressedMessage);
    }
};

ABILITY(ABILITY_HARUKAZE) : extends NotImplemented { INSTANCE(ABILITY_HARUKAZE); };

ABILITY(ABILITY_TOXIC_SURGE) : extends OnEntry, extends AllowTerrainIfAirborne<TERRAIN_TOXIC> {
    INSTANCE(ABILITY_TOXIC_SURGE);
    ON_ENTRY {
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
        BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
        return TRUE;
    }
};

ABILITY(ABILITY_POISON_QUILLS) : extends Merged<ABILITY_POISON_POINT, ABILITY_ROUGH_SKIN> { INSTANCE(ABILITY_POISON_QUILLS); };

ABILITY(ABILITY_DRACONIC_MIGHT) : extends AbilityImpl<ABILITY_HALF_DRAKE>, extends AteAbility<TYPE_DRAGON> { INSTANCE(ABILITY_DRACONIC_MIGHT); };

ABILITY(ABILITY_ATLANTIC_RULER) : extends AbilityImpl<ABILITY_AQUATIC_DWELLER>, extends AbilityImpl<ABILITY_SWIFT_SWIM> { INSTANCE(ABILITY_ATLANTIC_RULER); };

ABILITY(ABILITY_BIOFILM) : extends OnStat<> {
    INSTANCE(ABILITY_BIOFILM);
    ON_STAT {
        if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_CHOKEHOLD) : extends NotImplemented { INSTANCE(ABILITY_CHOKEHOLD); };

ABILITY(ABILITY_GUARDIAN_COAT) : extends SandImmune, extends OnDefensiveMultiplier, extends PowderImmune, extends HailImmune {
    INSTANCE(ABILITY_GUARDIAN_COAT);
    ON_DEFENSIVE_MULTIPLIER {
        if (IS_MOVE_PHYSICAL(move)) MUL(.8);
    }
};

ABILITY(ABILITY_NEUTRALIZING_FOG) : extends SimpleEntryMove<MOVE_DEFOG> { INSTANCE(ABILITY_NEUTRALIZING_FOG); };

ABILITY(ABILITY_FESTIVITIES) : extends OnModifyMoveFlags {
    INSTANCE(ABILITY_FESTIVITIES);
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

ABILITY(ABILITY_FEY_FLIGHT) : extends AbilityImpl<ABILITY_FAIRY_TALE>, extends GroundImmune { INSTANCE(ABILITY_FEY_FLIGHT); };

ABILITY(ABILITY_BEST_OFFENSE) : extends AbilityImpl<ABILITY_KEEN_EDGE>, extends AbilityImpl<ABILITY_MYSTIC_BLADES>, extends OnChooseOffensiveStat {
    INSTANCE(ABILITY_BEST_OFFENSE);
    ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPDEF] += 20; }
};

ABILITY(ABILITY_IMPALER) : extends AbilityImpl<ABILITY_MIGHTY_HORN>, extends OnAttacker {
    INSTANCE(ABILITY_IMPALER);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBleed(target))
        CHECK(gBattleMoves[move].hornBased);
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_BLEED);
    }
};

ABILITY(ABILITY_MAGUS_BLADES) : extends AbilityImpl<ABILITY_DUAL_WIELD>, extends AbilityImpl<ABILITY_BEST_OFFENSE> { INSTANCE(ABILITY_MAGUS_BLADES); };

ABILITY(ABILITY_LIGHTNING_BORN) : extends AddsType<TYPE_ELECTRIC> { INSTANCE(ABILITY_LIGHTNING_BORN); };

ABILITY(ABILITY_SUPERHEAVY) : extends NotImplemented { INSTANCE(ABILITY_SUPERHEAVY); };

ABILITY(ABILITY_WORLD_SERPENT) : extends AbilityImpl<ABILITY_GRIP_PINCER>, extends AbilityImpl<ABILITY_LONG_REACH> { INSTANCE(ABILITY_WORLD_SERPENT); };

ABILITY(ABILITY_LUCKY_WINGS) : extends AbilityImpl<ABILITY_GIANT_WINGS>, extends AbilityImpl<ABILITY_SERENE_GRACE> { INSTANCE(ABILITY_LUCKY_WINGS); };

ABILITY(ABILITY_KOMODO) : extends AbilityImpl<ABILITY_HALF_DRAKE>, extends AbilityImpl<ABILITY_TOXIC_CHAIN> { INSTANCE(ABILITY_KOMODO); };

ABILITY(ABILITY_ENVENOM) : extends OnAttacker {
    INSTANCE(ABILITY_ENVENOM);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(CanBePoisoned(battler, target, MOVE_NONE))
        CHECK(Random() % 100 < 30)

        return AbilityStatusEffect(MOVE_EFFECT_POISON);
    }
};

ABILITY(ABILITY_PURPLE_HAZE) : extends OnAttacker {
    INSTANCE(ABILITY_PURPLE_HAZE);
    ON_ATTACKER {
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
    }
};

ABILITY(ABILITY_GNASHING_CANNON) : extends Merged<ABILITY_MEGA_LAUNCHER, ABILITY_MIND_CRUSH> { INSTANCE(ABILITY_GNASHING_CANNON); };

ABILITY(ABILITY_HYPER_CLEANSE) : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
    INSTANCE(ABILITY_HYPER_CLEANSE);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_POISON) RESISTANCE(.5);
    }
    ON_STATUS_IMMUNE {
        CHECK(status & CHECK_STATUS1)
        return TRUE;
    }
};

ABILITY(ABILITY_MOLTEN_COAT) : extends OnAttacker, extends AteAbility<TYPE_ROCK> {
    INSTANCE(ABILITY_MOLTEN_COAT);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ROCK)
        CHECK(CanBeBurned(target))
        CHECK(Random() % 2)

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    }
};

ABILITY(ABILITY_ROYAL_DECREE) : extends AbilityImpl<ABILITY_QUEENLY_MAJESTY>, extends OnEntry {
    INSTANCE(ABILITY_ROYAL_DECREE);
    ON_ENTRY {
        CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
        return UseEntryMove(battler, ability, MOVE_GLARE, 0);
    }
};

ABILITY(ABILITY_TAG) : extends OnPreemptAction {
    INSTANCE(ABILITY_TAG);
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

ABILITY(ABILITY_SURPRISE) : extends OnPreemptAction {
    INSTANCE(ABILITY_SURPRISE);
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

ABILITY(ABILITY_BREEZY_NEIGH) : extends AbilityImpl<ABILITY_ADRENALINE_RUSH> { INSTANCE(ABILITY_BREEZY_NEIGH); };

ABILITY(ABILITY_DREAMSCAPE) : extends AbilityImpl<ABILITY_COMATOSE>, extends AbilityImpl<ABILITY_DREAMCATCHER> {
    INSTANCE(ABILITY_DREAMSCAPE);
    ON_OFFENSIVE_MULTIPLIER {
        AbilityImpl<ABILITY_DREAMCATCHER>::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        MUL(1.2);
    }
};

ABILITY(ABILITY_HASTE_MAKES_WASTE) : extends AbilityImpl<ABILITY_ANALYTIC>, extends AbilityImpl<ABILITY_STALL> { INSTANCE(ABILITY_HASTE_MAKES_WASTE); };

ABILITY(ABILITY_HUNGRY_MAWS) : extends AbilityImpl<ABILITY_JAWS_OF_CARNAGE>, extends AbilityImpl<ABILITY_STRONG_JAW> { INSTANCE(ABILITY_HUNGRY_MAWS); };

ABILITY(ABILITY_THERMAL_SLIDE) : extends OnStat<> {
    INSTANCE(ABILITY_THERMAL_SLIDE);
    ON_STAT {
        if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
    }
};

ABILITY(ABILITY_THERMOMANCY) : extends Merged<ABILITY_CRYOMANCY, ABILITY_PYROMANCY> { INSTANCE(ABILITY_THERMOMANCY); };

ABILITY(ABILITY_CHUCKSTER) : extends NotImplemented { INSTANCE(ABILITY_CHUCKSTER); };

ABILITY(ABILITY_HEAT_SINK) : extends LightningRodClone<TYPE_FIRE> { INSTANCE(ABILITY_HEAT_SINK); };

ABILITY(ABILITY_RELIC_STONE) : extends NotImplemented { INSTANCE(ABILITY_RELIC_STONE); };

ABILITY(ABILITY_SUPERCELL) : extends Merged<ABILITY_ELECTRIC_SURGE, ABILITY_DRIZZLE> { INSTANCE(ABILITY_SUPERCELL); };

ABILITY(ABILITY_LIGHTNING_ASPECT) : extends AbsorbStatUp<TYPE_ELECTRIC, STAT_HIGHEST_ATTACKING> { INSTANCE(ABILITY_LIGHTNING_ASPECT); };

ABILITY(ABILITY_FIRE_ASPECT) : extends AbsorbHeal<TYPE_FIRE>, extends OnAttacker {
    INSTANCE(ABILITY_FIRE_ASPECT);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_FIRE)
        CHECK(CanBeBurned(target))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
        return TRUE;
    }
};

ABILITY(ABILITY_BLISTERING_SUN) : extends Merged<ABILITY_DESOLATE_LAND, ABILITY_AIR_BLOWER> { INSTANCE(ABILITY_BLISTERING_SUN); };

ABILITY(ABILITY_AURORAS_GALE) : extends AbilityImpl<ABILITY_NORTH_WIND>, extends AbilityImpl<ABILITY_MAJESTIC_BIRD> { INSTANCE(ABILITY_AURORAS_GALE); };

ABILITY(ABILITY_WINTER_THRONE) : extends OnEntry, extends OnEndTurn {
    INSTANCE(ABILITY_WINTER_THRONE);
    ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_WINTER_THRONE); }
    ON_END_TURN {
        CHECK(IsAbilityOnField(ability) - 1 == battler)

        int any = FALSE;
        for (int target = 0; target < gBattlersCount; target++) {
            FILTER(IsBattlerAlive(target))

            if (IS_BATTLER_OF_TYPE(target, TYPE_ICE)) {
                FILTER_NOT(IsMagicGuardProtected(target))
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

ABILITY(ABILITY_ICE_PLUMES) : extends AbilityImpl<ABILITY_ICE_SCALES> { INSTANCE(ABILITY_ICE_PLUMES); };

ABILITY(ABILITY_PROPELLER_TAIL) : extends AbilityImpl<ABILITY_SWIFT_SWIM> { INSTANCE(ABILITY_PROPELLER_TAIL); };

ABILITY(ABILITY_ENERGY_TAP) : extends OnAttacker {
    INSTANCE(ABILITY_ENERGY_TAP);
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

ABILITY(ABILITY_MOLTEN_CORE) : extends AbilityImpl<ABILITY_FURNACE>, extends AbsorbStatUp<TYPE_ROCK, STAT_SPEED>, extends AbsorbUp2, extends StealthRockImmune {
    INSTANCE(ABILITY_MOLTEN_CORE);
    ON_ENTRY {
        AbilityImpl<ABILITY_FURNACE>::onEntry(DELEGATE_ENTRY);

        CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[GetBattlerSide(battler)] &= ~SIDE_STATUS_STEALTH_ROCK;
        return SwitchInAnnounce(B_MSG_SWITCHIN_MOLTEN_CORE);
    }
};

ABILITY(ABILITY_REVERBATE) : extends OnModifyMoveFlags {
    INSTANCE(ABILITY_REVERBATE);
    ON_MODIFY_MOVE_FLAGS {
        CHECK(flag == MOVE_FLAG_SOUND)
        CHECK(gBattleMoves[move].type == TYPE_NORMAL)
        return TRUE;
    }
};

ABILITY(ABILITY_TAEKKYEON) : extends OnModifyMoveFlags {
    INSTANCE(ABILITY_TAEKKYEON);
    ON_MODIFY_MOVE_FLAGS {
        CHECK(flag == MOVE_FLAG_DANCE)
        CHECK_NOT(IS_MOVE_STATUS(move))
        return TRUE;
    }
};

ABILITY(ABILITY_SLUDGE_SPIT) : extends OnAttacker {
    INSTANCE(ABILITY_SLUDGE_SPIT);
    ON_ATTACKER {
        CHECK(gBattleMoves[move].power)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_VENOM_BOLT, 35);
    }
};

ABILITY(ABILITY_SWAMP_THING) : extends OnEntry {
    INSTANCE(ABILITY_SWAMP_THING);
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].swampTimer)

        AbilityStatusEffectSafe(MOVE_EFFECT_SWAMP, battler, GetOppositeSide(battler));
        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        return TRUE;
    }
};

ABILITY(ABILITY_FROSTY_PRESCENCE) : extends SimpleEntryMove<MOVE_MIST> { INSTANCE(ABILITY_FROSTY_PRESCENCE); };

ABILITY(ABILITY_CHILLING_PELLETS) : extends OnDefender {
    INSTANCE(ABILITY_CHILLING_PELLETS);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(attacker))
        CHECK(IsMoveMakingContact(move, attacker))

        UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_SPEAR, 13);
        return FALSE;
    }
};

ABILITY(ABILITY_PAINT_SHOT) : extends OnAttacker {
    INSTANCE(ABILITY_PAINT_SHOT);
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

ABILITY(ABILITY_STONECUTTER) : extends AbilityImpl<ABILITY_FOSSILIZED>, extends OnMoldBreaker {
    INSTANCE(ABILITY_STONECUTTER);
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

ABILITY(ABILITY_EDGELORD) : extends AbilityImpl<ABILITY_CUTTHROAT>, extends OnBattlerFaints<> {
    INSTANCE(ABILITY_EDGELORD);
    ON_BATTLER_FAINTS {
        CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

        gStatuses4[battler] |= STATUS4_CUTTHROAT;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CUTTHROAT;
        BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
        return TRUE;
    }
};

ABILITY(ABILITY_WARMONGER) : extends OnOffensiveMultiplier<> {
    INSTANCE(ABILITY_WARMONGER);
    ON_OFFENSIVE_MULTIPLIER {
        if (moveType == TYPE_ROCK || moveType == TYPE_STEEL || moveType == TYPE_FIGHTING) MUL(1.30);
    }
};

ABILITY(ABILITY_LOCUST_SWARM) : extends StandardTransformation { INSTANCE(ABILITY_LOCUST_SWARM); };

ABILITY(ABILITY_REVELATION) : extends StandardTransformation { INSTANCE(ABILITY_REVELATION); };

ABILITY(ABILITY_CURSE_OF_FAMINE) : extends OnEntry {
    INSTANCE(ABILITY_CURSE_OF_FAMINE);
    ON_ENTRY {
        CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

        BattleScriptPushCursorAndCallback(BattleScript_CurseOfFamine);
        return TRUE;
    }
};

ABILITY(ABILITY_CRYSTALLINE_ARMOR) : extends NotImplemented { INSTANCE(ABILITY_CRYSTALLINE_ARMOR); };

ABILITY(ABILITY_SOUL_HARVEST) : extends OnStat<>, extends Breakable {
    INSTANCE(ABILITY_SOUL_HARVEST);
    ON_STAT {
        if (statId != STAT_SPEED) *stat = *stat * (20 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 20;
    }
};

ABILITY(ABILITY_THICK_BLUBBER) : extends OnDefensiveMultiplier, extends OnStat<> {
    INSTANCE(ABILITY_THICK_BLUBBER);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.25);
    }
    ON_STAT {
        if (statId == STAT_SPEED) *stat *= .5;
    }
};

ABILITY(ABILITY_CRAVING) : extends NotImplemented { INSTANCE(ABILITY_CRAVING); };

ABILITY(ABILITY_RAT_KING) : extends OnStat<ApplyOn::ALLY> {
    INSTANCE(ABILITY_RAT_KING);
    ON_STAT {
        const BaseStats *baseStats = &gBaseStats[gBattleMons[battler].species];
        int bst =
            baseStats->baseHP + baseStats->baseAttack + baseStats->baseDefense + baseStats->baseSpAttack + baseStats->baseSpDefense + baseStats->baseSpeed;
        if (bst >= 400) return;
        *stat *= 1.5;
    }
};

ABILITY(ABILITY_CRISPY_CREAM) : extends OnDefender {
    INSTANCE(ABILITY_CRISPY_CREAM);
    ON_DEFENDER {
        return Random() % 2 ? AbilityImpl<ABILITY_FLAME_BODY>::onEither(DELEGATE_DEFENDER) : AbilityImpl<ABILITY_FREEZING_POINT>::onEither(DELEGATE_DEFENDER);
    }
};

ABILITY(ABILITY_DEEP_FRIED) : extends OnEntry {
    INSTANCE(ABILITY_DEEP_FRIED);
    ON_ENTRY {
        CHECK_NOT(gSideTimers[GetOppositeSide(battler)].fireSeaTimer)

        AbilityStatusEffectSafe(MOVE_EFFECT_FIRE_SEA, battler, GetOppositeSide(battler));
        InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        return TRUE;
    }
};

ABILITY(ABILITY_FOOD_LOVERS) : extends AbilityImpl<ABILITY_HOSPITALITY>, extends AbilityImpl<ABILITY_FRIEND_GUARD> { INSTANCE(ABILITY_FOOD_LOVERS); };

ABILITY(ABILITY_LUNAR_WRATH) : extends OnAttacker {
    INSTANCE(ABILITY_LUNAR_WRATH);
    ON_ATTACKER {
        CHECK(moveType == TYPE_GHOST)
        CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

        return UseAttackerFollowUpMove(battler, target, ability, MOVE_MOONGEIST_BEAM, 50);
    }
};

ABILITY(ABILITY_SPYWARE) : extends NotImplemented { INSTANCE(ABILITY_SPYWARE); };

ABILITY(ABILITY_VIRUS) : extends OnAttacker {
    INSTANCE(ABILITY_VIRUS);
    ON_ATTACKER {
        CHECK(ShouldApplyOnHitAffect(target))
        CHECK(moveType == TYPE_ELECTRIC)
        CHECK(CanBePoisoned(battler, target, move))

        return AbilityStatusEffect(MOVE_EFFECT_POISON);
    }
};

ABILITY(ABILITY_POWER_LEAK) : extends OnDefender, extends AllowTerrainIfAirborne<TERRAIN_ELECTRIC> {
    INSTANCE(ABILITY_POWER_LEAK);
    ON_DEFENDER {
        CHECK(ShouldApplyOnHitAffect(battler))
        CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
        BattleScriptCall(BattleScript_SurgeActivatesRet);
        return TRUE;
    }
    TerrainType allowTerrainIfAirborne() { return TERRAIN_ELECTRIC; }
};

ABILITY(ABILITY_BACKUP_POWER) : extends OnRevive {
    INSTANCE(ABILITY_BACKUP_POWER);
    ON_REVIVE {
        CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))
        return B_MSG_BACKUP_POWER;
    }
};

ABILITY(ABILITY_SAND_FIEND) : extends AbilityImpl<ABILITY_SAND_GUARD>, extends AbilityImpl<ABILITY_SAND_FORCE> { INSTANCE(ABILITY_SAND_FIEND); };

ABILITY(ABILITY_MOUSTACHE) : extends Merged<ABILITY_TANGLING_HAIR, ABILITY_STAMINA> { INSTANCE(ABILITY_MOUSTACHE); };

ABILITY(ABILITY_DEPTH_EXPLORER) : extends AbilityImpl<ABILITY_FIELD_EXPLORER>, extends AbilityImpl<ABILITY_ILLUMINATE> { INSTANCE(ABILITY_DEPTH_EXPLORER); };

ABILITY(ABILITY_DUNE_VEIL) : extends AbilityImpl<ABILITY_SAND_GUARD>, extends AbilityImpl<ABILITY_SELF_SUFFICIENT> { INSTANCE(ABILITY_DUNE_VEIL); };

ABILITY(ABILITY_STRONG_FOUNDATION) : extends OnDefensiveMultiplier {
    INSTANCE(ABILITY_STRONG_FOUNDATION);
    ON_DEFENSIVE_MULTIPLIER {
        if (moveType == TYPE_WATER || moveType == TYPE_GROUND) RESISTANCE(.50);
    }
};

ABILITY(ABILITY_FOG_MACHINE) : extends OnDefender {
    INSTANCE(ABILITY_FOG_MACHINE);
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

ABILITY(ABILITY_DROP_BLOCKS) : extends OnDefender {
    INSTANCE(ABILITY_DROP_BLOCKS);
    ON_DEFENDER {
        CHECK(DidMoveHit())
        CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

        BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
        return TRUE;
    }
};

ABILITY(ABILITY_LASER_DRILL) : extends NotImplemented { INSTANCE(ABILITY_LASER_DRILL); };

ABILITY(ABILITY_LIGHT_SABER) : extends NotImplemented { INSTANCE(ABILITY_LIGHT_SABER); };

ABILITY(ABILITY_LOOSE_THORNS) : extends NotImplemented { INSTANCE(ABILITY_LOOSE_THORNS); };

ABILITY(ABILITY_TURF_WAR) : extends NotImplemented { INSTANCE(ABILITY_TURF_WAR); };

ABILITY(ABILITY_GREEDY) : extends NotImplemented { INSTANCE(ABILITY_GREEDY); };

ABILITY(ABILITY_MUSICAL_NOTES) : extends NotImplemented { INSTANCE(ABILITY_MUSICAL_NOTES); };

ABILITY(ABILITY_STRIKEOUT) : extends NotImplemented { INSTANCE(ABILITY_STRIKEOUT); };

ABILITY(ABILITY_HOME_RUN) : extends NotImplemented { INSTANCE(ABILITY_HOME_RUN); };

ABILITY(ABILITY_BRUISER) : extends AddsType<TYPE_FIGHTING> { INSTANCE(ABILITY_BRUISER); };

ABILITY(ABILITY_LETS_DANCE) : extends SimpleEntryMove<MOVE_TEETER_DANCE> { INSTANCE(ABILITY_LETS_DANCE); };

ABILITY(ABILITY_MYCELIUM_MIGHT) : extends OnMoldBreaker {
    INSTANCE(ABILITY_MYCELIUM_MIGHT);
    ON_MOLD_BREAKER { return IS_MOVE_STATUS(move); }
};

ABILITY(ABILITY_DEADLY_PRECISION) : extends OnMoldBreaker {
    INSTANCE(ABILITY_DEADLY_PRECISION);
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

template <typename As>
inline const As *dispatchTo(AbilityEnum id) {
    switch (id) {
#define __ON_ABILITY(ABILITY) \
    case ABILITY:             \
        return dynamic_cast<const As *>(AbilityImpl<ABILITY>::instance);
        OVER_ALL_ABILITIES
#undef __ON_ABILITY

        default:
            break;
    }

    return nullptr;
}

template <typename T>
struct AbilityEngine {
   private:
    constexpr static Implementation impl = T();

   public:
    ~AbilityEngine();
};

#pragma GCC diagnostic pop
