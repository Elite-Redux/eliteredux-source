#pragma once

#include "behavior/constants.hh"
#include "behavior/implementation_interface.hh"
#include "behavior/ability/constants.hh"
#include "behavior/ability/template.hh"
#include <array>

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-function"

template <typename T>
class AbilityBehavior {
    typedef std::array<const Ability *, ABILITIES_COUNT> AbilityPtrArray;
    static constexpr AbilityPtrArray gAbilities = generate();

    template <typename T>
    consteval std::array<const T *, ABILITIES_COUNT> abilitiesAs() {
        std::array<const T *, ABILITIES_COUNT> arr{0};
        for (int i = 0; i < ABILITIES_COUNT i++) {
            arr[i] = dynamic_cast<const T *>(gAbilities[i]);
        }
        return arr;
    }

    typedef<typename T> const T *abilityAs(AbilityEnum id) { return dynamic_cast<const T *>(gAbilities[id]); }

    constexpr static Implementation impl = T();

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
    ENUM_OR(NonStackingState)

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
    class AteAbility : extends OnMoveType, extends OnStab {
        ON_MOVE_TYPE {
            CHECK(moveType == TYPE_NORMAL)
            *ateBoost = TRUE;
            return BoostType + 1;
        }
        ON_STAB { return moveType == AteType; }
    };

    template <Type BoostType>
    class SwarmLike : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (move == BoostType) {
                if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                    MUL(1.5);
                else
                    MUL(1.2);
            }
        }
    };

    int DoesMoveMatchFlag(ON_MODIFY_MOVE_FLAGS) {
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

    class None : extends RandomizerBanned {};

    class ToxicTerrainImmune : extends Ability {};
    class Stench : extends OnAttacker, extends ToxicTerrainImmune {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanMoveHaveExtraFlinchChance(move))
            CHECK(Random() % 100 < 10)

            return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
        }
    };

    class PoisonHeal : extends ToxicTerrainImmune {};

    class Drizzle : extends OnEntry {
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

    class SpeedBoost : extends OnEndTurn {
        ON_END_TURN {
            CHECK(gVolatileStructs[battler].isFirstTurn != 2)
            CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            gBattleScripting.battler = battler;
            return TRUE;
        }
    };

    class BattleArmor : extends Breakable, extends OnDefensiveMultiplier, extends OnCrit<ApplyOnTarget::TARGET> {
        ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
        ON_CRIT { return NEVER_CRIT; }
    };

    class Sturdy : extends Breakable {};

    class Damp : extends OnEither {
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

    class HalfRecoil : extends Ability {};
    class RemovesStatusOnImmunity : extends OnStatusImmune<ApplyOn::SELF> {};

    class Limber : extends RemovesStatusOnImmunity, extends HalfRecoil {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_PARALYSIS)
            return TRUE;
        }
    };

    class SandImmune : extends Ability {};
    class SandVeil : extends Breakable, extends SandImmune, extends OnAccuracy<ApplyOnTarget::TARGET> {
        ON_ACCURACY {
            CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
            *accuracy /= 1.25;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Static : extends OnEither {
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
    class AbsorbHeal : OnAbsorb {
        ON_ABSORB {
            CHECK(moveType == Absorbed)
            return ABSORB_RESULT_HEAL;
        }
    };
    class VoltAbsorb : extends AbsorbHeal<TYPE_ELECTRIC> {};

    class WaterAbsorb : extends AbsorbHeal<TYPE_WATER> {};

    class TauntImmune : extends Breakable;

    class Oblivious : extends RemovesStatusOnImmunity, extends TauntImmune {
        ON_STATUS_IMMUNE {
            CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING))
            return TRUE;
        }
    };

    class CloudNine : extends OnEntry {
        ON_ENTRY {
            BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
            return TRUE;
        }
    };

    class CompoundEyes : extends OnAccuracy<> {
        ON_ACCURACY {
            *accuracy *= 1.3;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Insomnia : extends RemovesStatusOnImmunity {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_SLEEP)
            return TRUE;
        }
    };

    class ColorChange : extends OnBeforeAttack<ApplyOnTarget::TARGET> {
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

    class Immunity : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        }
        ON_STATUS_IMMUNE {
            CHECK(status & (CHECK_STATUS1 & ~CHECK_SLEEP))
            return TRUE;
        }
    };

    class FlashFire : extends OnAbsorb, extends OnOffensiveMultiplier<> {
        ON_ABSORB {
            CHECK(moveType == TYPE_FIRE)
            return ABSORB_RESULT_FLASH_FIRE;
        }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
        }
    };

    class PowderImmune : extends Breakable {};
    class ShieldDust : extends PowderImmune {};

    class OwnTempo : extends RemovesStatusOnImmunity, extends TauntImmune {
        ABILITY_ON_STATUS_IMMUNE {
            CHECK(status & CHECK_CONFUSION)
            return TRUE;
        }
    };

    class SuctionCups : extends Breakable {};

    class Intimidate : extends OnEntry {
        ON_ENTRY {
            u8 numAbility;

            for (numAbility = 0; numAbility < NUM_INTIMIDATE_CLONES; numAbility++) {
                if (gIntimidateCloneData[numAbility].ability == abilityToCheck) break;
            }

            if (numAbility >= NUM_INTIMIDATE_CLONES) return FALSE;

            if (!gIntimidateCloneData[numAbility].numStatsLowered) return FALSE;

            gBattlerTarget = BATTLE_OPPOSITE(battler);
            if (!IsBattlerAlive(gBattlerTarget) && !IsBattlerAlive(BATTLE_PARTNER(gBattlerTarget))) return FALSE;

            BattleScriptPushCursorAndCallback(BattleScript_IntimidateActivatedNew);
            return TRUE;
        }
    };

    class ShadowTag : extends OnTrap {
        ABILITY_ON_TRAP {
            ON_ABILITY(switchingBattler, FALSE, gAbilities[ability].shadowTag, return FALSE)
            return TRUE;
        }
    };

    class RoughSkin : extends OnDefender {
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

    class WonderGuard : extends Breakable, extends RandomizerBanned, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (*mod < UQ_4_12(2.0)) *mod = 0;
        }
    };

    class GroundImmune : extends Breakable {};
    class Levitate : extends GroundImmune, extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FLYING) MUL(1.25);
        }
    };

    class EffectSpore : extends PowderImmune, extends OnDefender {
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

    class ClearBody : extends Breakable {};

    class NaturalCure : extends OnExit {
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

    template <Type Absorbed, int stat>
    class AbsorbStatUp : OnAbsorb {
        ON_ABSORB {
            CHECK(moveType == TYPE_ELECTRIC);
            int stat = stat == STAT_HIGHEST_ATTACKING ? GetHighestAttackingStatId(battler, TRUE) : stat;
            *statId = STAT_SPEED;
            return ABSORB_RESULT_STAT;
        }
    };
    template <Type Absorbed>
    class LightningRodClone : extends Redirects<Absorbed>, extends AbsorbStatUp<Absorbed, STAT_HIGHEST_ATTACKING> {};
    class LightningRod : LightningRodClone<TYPE_ELECTRIC> {};

    class SereneGrace : extends OnModifyEffectChance<> {
        ON_MODIFY_EFFECT_CHANCE { *effectChance *= 2; }
    };

    class SwiftSwim : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
        }
    };

    class Chlorophyll : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        }
    };

    class Illuminate : extends OnAccuracy<> {
        ON_ACCURACY {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Trace : extends RandomizerBanned, extends OnEntry {
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

    class HugePower : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_ATK) *stat *= 2;
        }
    };

    class PoisonPoint : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBePoisoned(battler, opponent, MOVE_NONE))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
            return TRUE;
        }
    };

    class InnerFocus : extends TauntImmune, extends OnAccuracy<> {
        ON_ACCURACY {
            CHECK(move == MOVE_FOCUS_BLAST)
            return ACCURACY_ALWAYS_HITS;
        }
    };

    class MagmaArmor : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
        }
        ABILITY_ON_STATUS_IMMUNE {
            CHECK(status & CHECK_FROSTBITE)
            return TRUE;
        }
    };

    class WaterVeil : extends OnEntry, extends RemovesStatusOnImmunity {
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

    class MagnetPull : extends OnTrap {
        ABILITY_ON_TRAP { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); }
    };

    class Soundproof : extends OnImmune<> {
        ON_IMMUNE {
            CHECK(IsSoundMove(attacker, move))
            CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class RainDish : extends OnEndTurn {
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

    class SandStream : extends SandImmune, extends OnEntry {
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

    class Pressure : extends OnEntry {
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

    class ThickFat : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
        }
    };

    class FlameBody : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBeBurned(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
            return TRUE;
        }
    };

    class KeenEye : extends OnAccuracy<> {
        ON_ACCURACY {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class HyperCutter : extends Breakable, extends OnCrit<> {
        ON_CRIT {
            CHECK(IsMoveMakingContact(move, battler))
            return 1;
        }
    };

    class Pickup : extends OnEntry {
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

    class Truant : extends OnEndTurn {
        ON_END_TURN {
            if (GetAbilityState(battler, ability))
                SetAbilityState(battler, ability, FALSE);
            else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
                SetAbilityState(battler, ability, TRUE);
            return FALSE;
        }
    };

    class Hustle : extends OnAccuracy<>, extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER { MUL(1.4); }
        ON_ACCURACY {
            CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class CuteCharm : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(CanInfatuate(battler, opponent))
            CHECK(Random() % 100 < 50)

            AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
            return TRUE;
        }
    };

    class Plus : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            int partner = BATTLE_PARTNER(battler);
            if (!IsBattlerAlive(partner)) return;
            if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
        }
    };

    class Minus : extends Plus {};

    class StandardTransformation : extends FormChange, extends OnEntry, extends OnEndTurn {
        ON_ENTRY { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        ON_END_TURN { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    };
    class WeatherTransformation : extends StandardTransformation, extends OnWeather {
        ON_WEATHER { return TryTransformAttacker(ability, battler, ABILITY_BS_CALL); }
    };

    class Forecast : extends WeatherTransformation, extends OnAttacker {
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

    class StickyHold : extends Breakable {};

    class ShedSkin : extends OnEndTurn {
        ON_END_TURN {
            CHECK(Random() % 100 < 30)

            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
    };

    class NegateBurnAtkDrop : extends Ability {};
    class Guts : extends OnOffensiveMultiplier<>, extends NegateBurnAtkDrop {
        ON_OFFENSIVE_MULTIPLIER {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        }
    };

    class MarvelScale : extends OnStat<> {
        ON_STAT {
            if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        }
    };

    class Overgrow : extends SwarmLike<TYPE_GRASS> {};

    class Blaze : extends SwarmLike<TYPE_FIRE> {};

    class Torrent : extends SwarmLike<TYPE_WATER> {};

    class Swarm : extends SwarmLike<TYPE_BUG> {};

    class NoRecoil : extends Ability {};
    class RockHead : extends RemovesStatusOnImmunity, extends NoRecoil {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_CONFUSION)
            return TRUE;
        }
    };

    class Drought : extends OnEntry {
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

    class ArenaTrap : extends OnTrap {
        ON_TRAP { return IsBattlerGrounded(switchingBattler); }
    };

    class VitalSpirit : extends OnAttacker, extends RemovesStatusOnImmunity, extends TauntImmune {
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

    class WhiteSmoke : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

            int side = GET_BATTLER_SIDE(battler);
            gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
            gSideTimers[side].started.smokescreen = TRUE;
            gSideTimers[side].smokescreenBattler = battler;
            return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
        }
    };

    class PurePower : extends HugePower {};

    class ShellArmor : extends BattleArmor {};

    class AirLock : extends CloudNine {};

    class TangledFeet : extends OnAccuracy<ApplyOnTarget::TARGET> {
        ON_ACCURACY {
            CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
            *accuracy /= 2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class MotorDrive : extends AbsorbStatUp<TYPE_ELECTRIC, STAT_SPEED> {};

    class Rivalry : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
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

    class HailImmune : extends Ability {};
    class SnowCloak : extends Breakable, extends HailImmune, extends OnAccuracy<ApplyOnTarget::TARGET> {
        ON_ACCURACY {
            CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
            *accuracy /= 1.25;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class AngerPoint : extends OnDefender {
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

    class Unburden : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
        }
    };

    class Heatproof : extends OnDefensiveMultiplier, extends NegateBurnAtkDrop {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        }
    };

    class DrySkin : extends WaterAbsorb, extends RainDish, extends OnDefensiveMultiplier {
        ON_END_TURN {
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !IsMagicGuardProtected(battler)) {
                gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
                if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
                BattleScriptPushCursorAndCallback(BattleScript_SolarPowerActivates);
                return TRUE;
            }

            return RainDish::onEndTurn(DELEGATE_END_TURN);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) RESISTANCE(1.25);
        }
    };

    class Download : extends OnEntry {
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

    class IronFist : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IsIronFistBoosted(battler, move)) MUL(1.3);
        }
    };

    class Adaptability : extends Ability {};

    class SkillLink : extends Ability {};

    class Hydration : extends OnEndTurn {
        ON_END_TURN {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
    };

    class SolarPower : extends OnStat<> {
        ON_STAT {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        }
    };

    class QuickFeet : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        }
    };

    class Normalize : extends OnOffensiveMultiplier<>, extends OnMoveType, extends OnTypeEffectiveness<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        }
        ON_MOVE_TYPE { return TYPE_NORMAL + 1; }
        ON_TYPE_EFFECTIVENESS {
            CHECK(moveType == TYPE_NORMAL) CHECK(*mod) CHECK(*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
            return TRUE;
        }
    };

    class Sniper : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (isCrit) MUL(1.5);
        }
    };

    class MagicGuard : extends Ability {};

    class NoGuard : extends OnAccuracy<ApplyOnTarget::ATTACKER_OR_TARGET> {
        ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
    };

    class Stall : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
        }
    };

    class Technician : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (basePower <= 60) MUL(1.5);
        }
    };

    class LeafGuard : extends OnEndTurn {
        ON_END_TURN {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
    };

    class MoldBreaker : extends OnEntry, extends OnMoldBreaker {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); }
        ON_MOLD_BREAKER { return TRUE; }
    };

    class SuperLuck : extends OnCrit<> {
        ON_CRIT { return 1; }
    };

    class Aftermath : extends OnDefender {
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

    class Anticipation : extends Breakable, extends OnEntry, extends Persistent {
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

    class Forewarn : extends OnEntry {
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

    class Unaware : extends Breakable {};

    class TintedLens : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
        }
    };

    class Filter : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        }
    };

    class SlowStart : extends OnEntry, extends OnStat<> {
        ON_ENTRY {
            gVolatileStructs[battler].slowStartTimer = 5;
            return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
        }
        ON_STAT {
            if (statId != STAT_ATK && statId != STAT_SPATK && statId != STAT_SPEED) return;
            if (gVolatileStructs[battler].slowStartTimer) *stat /= 2;
        }
    };

    class HitsGhost : extends OnTypeEffectiveness<> {
        ON_TYPE_EFFECTIVENESS {
            CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
            CHECK(defType == TYPE_GHOST)
            CHECK_NOT(*mod)
            *mod = UQ_4_12(1.0);
            return TRUE;
        }
    };
    class Scrappy : extends HitsGhost, extends TauntImmune {};

    class StormDrain : extends LightningRodClone<TYPE_WATER> {};

    class IceBody : extends HailImmune, extends OnEndTurn {
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

    class SolidRock : extends Filter {};

    class SnowWarning : extends HailImmune, extends OnEntry {
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

    class HoneyGather : extends OnEndTurn {
        ON_END_TURN {
            CHECK_NOT(gBattleMons[battler].item)
            CHECK(Random() % 2)

            gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
            BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
            return TRUE;
        }
    };

    class Frisk : extends OnEntry {
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

    class Reckless : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
        }
    };

    class Multitype : extends FormChange {};

    class FlowerGift : extends WeatherTransformation, extends Breakable, extends OnStat<ApplyOn::ALLY> {
        ON_STAT {
            if (statId != STAT_SPATK && statId != STAT_SPDEF) return;
            if (IsWeatherActive(WEATHER_SUN_ANY)) *stat *= 1.5;
        }
    };

    class BadDreams : extends OnEndTurn {
        ON_END_TURN {
            gBattleScripting.abilityPopupOverwrite = ability;
            BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
            return NO_ANNOUNCE;
        }
    };

    class SheerForce : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
        }
    };

    class Contrary : extends Breakable {};

    class Unnerve : extends OnEntry {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); }
    };

    class Defeatist : extends OnStat<> {
        ON_STAT {
            if (statId != STAT_ATK && statId != STAT_SPATK) return;
            if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
        }
    };

    class CursedBody : extends OnDefender {
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

    class Healer : extends OnEndTurn {
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

    class FriendGuard : extends Breakable {};

    class WeakArmor : extends OnDefender {
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

    class LightMetal : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED) *stat *= 1.3;
        }
    };

    class Multiscale : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (BATTLER_MAX_HP(battler)) MUL(.5);
        }
    };

    class ToxicBoost : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        }
    };

    class FlareBoost : extends OnEntry, extends OnWeather, extends OnStat<>, extends NegateBurnAtkDrop {
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

    class Harvest : extends OnEndTurn {
        ON_END_TURN {
            CHECK_NOT(gBattleMons[battler].item)
            CHECK_NOT(gBattleStruct->changedItems[battler])
            CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

            BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
            return TRUE;
        }
    };

    class Telepathy : extends OnAfterTypeEffectiveness<ApplyOnTarget::ATTACKER_OR_TARGET> {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
        }
    };

    class Moody : extends OnEndTurn {
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

    class Overcoat : extends Breakable, extends SandImmune, extends HailImmune, extends PowderImmune, extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move)) MUL(.8);
        }
    };

    class PoisonTouch : extends PoisonPoint {};

    class Regenerator : extends OnExit {
        ON_EXIT {
            CHECK(IsBattlerAlive(battler))
            CHECK_NOT(BATTLER_MAX_HP(battler))
            BattleScriptCall(BattleScript_RegeneratorExits);
            return FALSE;
        }
    };

    class BigPecks : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IsMoveMakingContact(move, battler)) MUL(1.3);
        }
    };

    class SandRush : extends OnStat<>, extends SandImmune {
        ON_STAT {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        }
    };

    class FortKnox : extends Ability {};

    class WonderSkin : extends FortKnox {};

    class Analytic : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
        }
    };

    class Illusion : extends OnDefender, extends OnOffensiveMultiplier<> {
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

    class Imposter : extends OnEntry {
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

    class Infiltrator : extends OnInfiltrate {
        ON_INFILTRATE { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; }
    };

    class Mummy : extends OnDefender {
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
    class MoxieClone : extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            CHECK(HasAttackerFaintedTarget())
            int stat = Stat == STAT_HIGHEST_TOTAL ? GetHighestStatId(battler, FALSE) : Stat;
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))
            BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
            return TRUE;
        }
    };

    class Moxie : extends MoxieClone<STAT_ATK> {};

    class Justified : extends AbsorbStatUp<TYPE_DARK, STAT_HIGHEST_ATTACKING> {};

    class Rattled : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
            CHECK(CanRaiseStat(battler, STAT_SPEED))

            SetStatChanger(STAT_SPEED, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
    };

    class MagicBounce : extends Breakable {};

    class SapSipper : extends LightningRodClone<TYPE_GRASS> {};

    class Prankster : extends OnPriority {
        ON_PRIORITY {
            CHECK(IS_MOVE_STATUS(move))
            return 1;
        }
    };

    class SandForce : extends SandImmune, extends OnStat<> {
        ON_STAT {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        }
    };

    class IronBarbs : extends RoughSkin {};

    class ZenMode : extends StandardTransformation {};

    class VictoryStar : extends OnAccuracy<ApplyOnTarget::ALLY> {
        ON_ACCURACY {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    template <Type ExtraType>
    class AddsType : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(IS_BATTLER_OF_TYPE(battler, ExtraType))

            gBattleMons[battler].type3 = ExtraType;
            PREPARE_TYPE_BUFFER(gBattleTextBuff2, ExtraType);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
            return TRUE;
        }
    };

    class Turboblaze : extends MoldBreaker, extends AddsType<TYPE_FIRE> {
        ON_ENTRY { return AddsType<TYPE_FIRE>::onEntry(DELEGATE_ENTRY); }
    };

    class Teravolt : extends MoldBreaker, extends AddsType<TYPE_ELECTRIC> {
        ON_ENTRY { return AddsType<TYPE_ELECTRIC>::onEntry(DELEGATE_ENTRY); }
    };

    class AromaVeil : extends OnStatusImmune<ApplyOn::ALLY> {
        ON_STATUS_IMMUNE {
            CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK))
            return TRUE;
        }
    };

    class FlowerVeil : extends OnStatusImmune<ApplyOn::ALLY> {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_STATUS1)
            CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
            return TRUE;
        }
    };

    class CheekPouch : extends RandomizerBanned {};

    class Protean : extends OnBeforeAttack<> {
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

    class FurCoat : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_PHYSICAL(move)) MUL(.5);
        }
    };

    class Bulletproof : extends OnImmune<> {
        ON_IMMUNE {
            CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
            CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class StrongJaw : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
        }
    };

    class Refrigerate : extends AteAbility<TYPE_ICE> {};

    class SweetVeil : extends OnStatusImmune<ApplyOn::ALLY> {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_SLEEP)
            return TRUE;
        }
    };

    class StanceChange : extends FormChange, extends OnBeforeAttack<> {
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
    class GaleWingsLike : extends OnPriority {
        ON_PRIORITY {
            CHECK(GetTypeBeforeUsingMove(move, battler) == GaleWingsType)
            CHECK(BATTLER_MAX_HP(battler))
            return 1;
        }
    };
    class GaleWings : extends GaleWingsLike<TYPE_FLYING> {};

    class MegaLauncher : extends OnOffensiveMultiplier<> {
        void ON_OFFENSIVE_MULTIPLIER {
            if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
        }
    };

    class GrassPelt : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
        }
    };

    class ToughClaws : extends BigPecks {};

    class Pixilate : extends AteAbility<TYPE_FAIRY> {};

    class Gooey : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
            CHECK(IsMoveMakingContact(move, attacker))

            BattleScriptCall(BattleScript_GooeyActivates);
            gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
            return TRUE;
        }
    };

    class Aerilate : extends AteAbility<TYPE_FLYING> {
        ATE_ABILITY(TYPE_FLYING),
    };

    class HyperAggressive : extends OnParentalBond {
        ON_PARENTAL_BOND { return PARENTAL_BOND_HYPER_AGGRESSIVE; }
    };

    class IgnoresFortKnox : extends Ability {};
    class ParentalBond : extends HyperAggressive, extends IgnoresFortKnox {};

    class DarkAura : extends OnEntry, extends OnOffensiveMultiplier<ApplyOn::ANY> {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA); }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType != TYPE_DARK) return;
            if (IsAbilityOnField(ABILITY_AURA_BREAK))
                MUL(.75);
            else
                MUL(1.33);
        }
    };

    class FairyAura : extends OnEntry, extends OnOffensiveMultiplier<ApplyOn::ANY> {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA); }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType != TYPE_FAIRY) return;
            if (IsAbilityOnField(ABILITY_AURA_BREAK))
                MUL(.75);
            else
                MUL(1.33);
        }
    };

    class AuraBreak : extends Breakable, extends OnEntry {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); }
    };

    class PrimordialSea : extends OnEntry {
        ON_ENTRY {
            CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

            BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
            return TRUE;
        }
    };

    class DesolateLand : extends OnEntry {
        ON_ENTRY {
            CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

            BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
            return TRUE;
        }
    };

    class WeatherControl : extends OnImmune<> {
        ON_IMMUNE {
            CHECK(gBattleMoves[move].flags & FLAG_WEATHER_BASED)
            CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER)
            *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class DeltaStream : extends WeatherControl, extends OverrideBreakable, extends OnEntry {
        ON_ENTRY {
            CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))

            BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
            return TRUE;
        }
    };

    class Stamina : extends OnDefender {
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

    class WimpOut : extends OnDefender {
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

    class EmergencyExit : extends WimpOut {};

    class WaterCompaction : extends OnDefensiveMultiplier, extends OnDefender {
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

    class Merciless : extends OnCrit<> {
        ON_CRIT {
            if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
            if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
            if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
            if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
            if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
            return 0;
        }
    };

    class ShieldsDown : extends StandardTransformation, extends OnAttacker, extends OnStatusImmune<>, extends OverrideBreakable {
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

    class Stakeout : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
        }
    };

    class WaterBubble : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
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

    class Steelworker : extends Breakable, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends AteAbility<TYPE_STEEL> {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
        }
    };

    class Berserk : extends OnDefender {
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

    class SlushRush : extends HailImmune, extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        }
    };

    class LongReach : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IS_MOVE_PHYSICAL(move) && !gBattleMoves[move].contact) MUL(1.2);
        }
    };

    template <Type BoostType>
    class LiquidVoiceClone : extends OnOffensiveMultiplier<>, extends OnMoveType {
        ON_OFFENSIVE_MULTIPLIER {
            if (IsSoundMove(battler, move)) MUL(1.2);
        }
        ON_MOVE_TYPE {
            CHECK(moveType == TYPE_NORMAL)
            CHECK(gBattleMoves[move].flags & FLAG_SOUND)
            return BoostType + 1;
        }
    };
    class LiquidVoice : extends LiquidVoiceClone<TYPE_WATER> {};

    class Triage : extends OnPriority {
        ON_PRIORITY {
            CHECK(IsHealingMoveEffect(gBattleMoves[move].effect))
            return 3;
        }
    };

    class Galvanize : extends AteAbility<TYPE_ELECTRIC> {};

    class SurgeSurfer : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
        }
    };

    class Schooling : extends StandardTransformation {
        ON_ENTRY {
            CHECK(gBattleMons[battler].level >= 20)
            return StandardTransformation::onEntry(DELEGATE_ENTRY);
        }
        ON_END_TURN {
            CHECK(gBattleMons[battler].level >= 20)
            return StandardTransformation::onEndTurn(DELEGATE_END_TURN);
        }
    };

    class Disguise : extends FormChange, extends OnEntry, extends OnDisguise, extends OnWeather {
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

    class BattleBond : extends FormChange, extends OnBattlerFaints<> {
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

    class PowerConstruct : extends FormChange, extends OnEndTurn {
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

    class Corrosion : extends OnTypeEffectiveness<>, extends OnCanStatusType {
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

    class Comatose : extends OnEntry, extends RemovesStatusOnImmunity, extends Unsuppressable {
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

    template <ApplyOn For = ApplyOn::SELF>
    class BlocksPriority : extends OnImmune<For> {
        ON_IMMUNE {
            CHECK_NOT(gProcessingExtraAttacks)
            CHECK(GetBattlerSide(attacker) != GetBattlerSide(battler))
            CHECK(GetMovePriority(attacker, move, battler) > 0);
            *immunityScript = BattleScript_DazzlingProtected;
            return TRUE;
        }
    };
    class QueenlyMajesty : extends BlocksPriority<ApplyOn::ALLY> {};

    class InnardsOut : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK_NOT(IsBattlerAlive(battler))
            CHECK_NOT(IsMagicGuardProtected(attacker))

            gBattleMoveDamage = gTurnStructs[battler].dmg;
            BattleScriptCall(BattleScript_AftermathDmg);
            return TRUE;
        }
    };

    class Dancer : extends OnCopyMove {
        ON_COPY_MOVE {
            CHECK(IsDance(attacker, move))
            return UseOutOfTurnAttack(battler, target, ability, move, 0);
        }
    };

    class Battery : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move)) MUL(1.3);
        }
    };

    class Fluffy : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        }
    };

    class Dazzling : extends QueenlyMajesty {};

    class SoulHeart : extends OnBattlerFaints<ApplyOnTarget::ANY> {
        ON_BATTLER_FAINTS {
            CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

            BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
            return TRUE;
        }
    };

    class TanglingHair : extends Gooey {};

    class Receiver : extends OnBattlerFaints<ApplyOnTarget::ALLY> {
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

    class PowerOfAlchemy : extends OnEntry, extends OnReactive, extends OnBattlerFaints<ApplyOnTarget::ANY> {
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

    class BeastBoost : extends MoxieClone<STAT_HIGHEST_TOTAL> {};

    class RksSystem : extends Protean, extends Adaptability, extends FormChange {};

    class ElectricSurge : extends AllowTerrainIfAirborne<TERRAIN_ELECTRIC>, extends OnEntry {
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

    class PsychicSurge : extends AllowTerrainIfAirborne<TERRAIN_PSYCHIC>, extends OnEntry {
        ON_ENTRY {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
    };

    class MistySurge : extends AllowTerrainIfAirborne<TERRAIN_MISTY>, extends OnEntry {
        ON_ENTRY {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
    };

    class GrassySurge : extends AllowTerrainIfAirborne<TERRAIN_GRASSY>, extends OnEntry {
        ON_ENTRY {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
    };

    class ShadowShield : extends Multiscale, extends OverrideBreakable {};

    class PrismArmor : extends Filter, extends OverrideBreakable {};

    class Neuroforce : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.35);
        }
    };

    template <int Stat>
    class RaiseStatOnEntry : OnEntry {
        ON_ENTRY {
            CHECK(CanRaiseStat(battler, Stat))

            SetStatChanger(Stat, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            return TRUE;
        }
    };
    class IntrepidSword : extends RaiseStatOnEntry<STAT_ATK> {};

    class DauntlessShield : extends RaiseStatOnEntry<STAT_DEF> {};

    class Libero : extends Protean {};

    class CottonDown : extends OnDefender {
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

    class MirrorArmor : extends Breakable {};

    class GulpMissile : extends FormChange, extends OnDefender, extends OnAttacker {
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

    class SteamEngine : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(CanRaiseStat(battler, STAT_SPEED))
            CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

            SetStatChanger(STAT_SPEED, 12);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
    };

    class Amplifier : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IsSoundMove(battler, move)) MUL(1.3);
        }
    };

    class PunkRock : extends OnDefensiveMultiplier, extends Amplifier {
        ON_DEFENSIVE_MULTIPLIER {
            if (IsSoundMove(attacker, move)) MUL(.5);
        }
    };

    class SandSpit : extends SandImmune, extends OnDefender {
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

    class IceScales : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move)) MUL(.5);
        }
    };

    class IceFace : extends FormChange, extends HailImmune, extends OnEntry, extends OnDisguise, extends OnWeather {
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

    class PowerSpot : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
        ON_OFFENSIVE_MULTIPLIER { MUL(1.3); }
    };

    class Mimicry : extends OnEntry, extends OnTerrain {
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

    class ScreenCleaner : extends OnEntry {
        ON_ENTRY {
            CHECK(TryRemoveScreens(battler))

            return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
        }
    };

    class SteelySpirit : extends OnOffensiveMultiplier<ApplyOn::ALLY> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_STEEL) MUL(1.3);
        }
    };

    class PerishBody : extends OnDefender {
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

    class WanderingSpirit : extends OnDefender {
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

    class GorillaTactics : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
        }
    };

    class NeutralizingGas : extends Unsuppressable {};

    class PastelVeil : extends OnEntry {
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

    class HungerSwitch : extends FormChange, extends OnEndTurn {
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

    class CuriousMedicine : extends OnEntry {
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

    class Transistor : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ELECTRIC) MUL(1.5);
        }
    };

    class DragonsMaw : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_DRAGON) MUL(1.5);
        }
    };

    class ChillingNeigh : extends Moxie {};

    class GrimNeigh : extends MoxieClone<STAT_SPATK> {};

    template <typename FaintAbility>
    class AsOne : extends Unsuppressable, extends RandomizerBanned, extends Unnerve, extends FaintAbility {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE); }
        ON_BATTLER_FAINTS {
            CHECK(FaintAbility::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
            gBattleScripting.abilityPopupOverwrite = ABILITY_CHILLING_NEIGH;
            BattleScriptCall(BattleScript_AbilityPopUpStack);
            return NO_ANNOUNCE;
        }
    };

    class AsOneIceRider : extends AsOne<ChillingNeigh> {};

    class AsOneShadowRider : extends AsOne<GrimNeigh> {};

    class Chloroplast : extends Ability {};

    class Whiteout : extends HailImmune, extends OnStat<> {
        ON_STAT {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        }
    };

    class Pyromancy : extends OnModifyEffectChance<> {
        ON_MODIFY_EFFECT_CHANCE {
            if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
        }
    };

    class KeenEdge : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
        }
    };

    class PrismScales : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move)) MUL(.7);
        }
    };

    class PowerFists : extends IronFist, extends OnChooseDefensiveStat<> {
        ON_CHOOSE_DEFENSIVE_STAT {
            CHECK(IsIronFistBoosted(battler, move))
            return STAT_SPDEF;
        }
    };

    class SandSong : extends LiquidVoiceClone<TYPE_GROUND> {};

    class Rampage : extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            SetAbilityState(battler, ability, TRUE);
            gVolatileStructs[battler].rechargeTimer = 0;
            gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
            return FALSE;
        }
    };

    class Vengeance : extends SwarmLike<TYPE_GHOST> {};

    class BlitzBoxer : extends OnPriority {
        ON_PRIORITY {
            CHECK(IsIronFistBoosted(battler, move))
            CHECK(BATTLER_MAX_HP(battler));
            return 1;
        }
    };

    class AntarcticBird : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
        }
    };

    class Immolate : extends AteAbility<TYPE_FIRE> {};

    class Crystallize : extends OnOffensiveMultiplier<>, extends OnMoveType {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ICE && gBattleStruct->ateBoost[battler]) MUL(1.1);
        }
        ON_MOVE_TYPE {
            CHECK(moveType == TYPE_ROCK)
            *ateBoost = TRUE;
            return TYPE_ICE + 1;
        }
    };

    class Electrocytes : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ELECTRIC) MUL(1.25);
        }
    };

    class Aerodynamics : extends AbsorbStatUp<TYPE_FLYING, STAT_SPEED> {};

    class ChristmasSpirit : extends OnDefensiveMultiplier, extends HailImmune {
        ON_DEFENSIVE_MULTIPLIER {
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
        }
    };

    class ExploitWeakness : extends OnOffensiveMultiplier<>, extends OnChooseDefensiveStat<> {
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

    class GroundShock : extends OnTypeEffectiveness<> {
        ON_TYPE_EFFECTIVENESS {
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(defType == TYPE_GROUND)
            CHECK_NOT(*mod)
            *mod = UQ_4_12(.5);
            return TRUE;
        }
    };

    class AncientIdol : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; }
    };

    class MysticPower : extends OnStab {
        ON_STAB { return TRUE; }
    };

    class Perfectionist : extends OnPriority, extends OnCrit<> {
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

    class GrowingTooth : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_AttackBoostActivates);
            return TRUE;
        }
    };

    class Inflatable : extends ON_DEFENDER {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF))
            CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
            BattleScriptCall(BattleScript_InflatableActivates);
            gBattleScripting.battler = battler;
            return TRUE;
        }
    };

    class AuroraBorealis : extends HailImmune, extends OnStab {
        ON_STAB { return moveType == TYPE_ICE; }
    };

    class Avenger : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
        }
    };

    class LetsRoll : extends OnEntry {
        ON_ENTRY {
            CHECK(CanRaiseStat(battler, STAT_DEF))

            SetStatChanger(STAT_DEF, 1);
            gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class LoudBang : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeConfused(target))
            CHECK(IsSoundMove(battler, move))
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
        }
    };

    class LeadCoat : extends OnDefensiveMultiplier, extends OnStat<> {
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_PHYSICAL(move)) MUL(.6);
        }
        ON_STAT {
            if (statId == STAT_SPEED) *stat *= .9;
        }
    };

    class Amphibious : extends OnStab, extends OnStatusImmune<> {
        ON_STAB { return moveType == TYPE_WATER; }
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_DRENCH)
            return TRUE;
        }
    };

    class Grounded : extends AddsType<TYPE_GROUND> {};

    class Earthbound : extends SwarmLike<TYPE_GROUND> {};

    class FightingSpirit : extends AteAbility<TYPE_FIGHTING> {};

    class FelineProwess : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPATK) *stat *= 2;
        }
    };

    class CoilUp : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

            gStatuses4[battler] |= STATUS4_COILED;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
            return TRUE;
        }
    };

    class Fossilized : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ROCK) MUL(1.2);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ROCK) RESISTANCE(.5);
        }
    };

    class MagicalDust : extends OnDefender {
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

    class Dreamcatcher : extends OnOffensiveMultiplier<>, extends OnPreemptAction {
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
            return UseTurnAttackAsPursuit(DELEGATE_PREEMPT_ACTION);
        }
    };

    class Nocturnal : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_DARK) MUL(1.25);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
        }
    };

    class SelfSufficient : extends OnEndTurn {
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

    class Tectonize : extends AteAbility<TYPE_GROUND> {};

    class IceAge : extends AddsType<TYPE_ICE> {};

    class HalfDrake : extends AddsType<TYPE_DRAGON> {};

    class Aquatic : extends AddsType<TYPE_WATER> {};

    class Liquified : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_WATER) RESISTANCE(2);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        }
    };

    class Dragonfly : extends HalfDrake, extends GroundImmune {};

    template <Type StrongVs>
    class TypeSlayer : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
        ON_OFFENSIVE_MULTIPLIER {
            if (IS_BATTLER_OF_TYPE(target, StrongVs)) RESISTANCE(1.5);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_BATTLER_OF_TYPE(attacker, StrongVs)) MUL(.5);
        }
    };

    class Dragonslayer : extends TypeSlayer<TYPE_DRAGON> {};

    class StealthRockImmune : extends Ability {};
    class Mountaineer : extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends StealthRockImmune {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (moveType == TYPE_ROCK) *mod = 0;
        }
    };

    class Hydrate : extends AteAbility<TYPE_WATER> {};

    class Metallic : extends AddsType<TYPE_STEEL> {};

    class Permafrost : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        }
    };

    class PrimalArmor : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
        }
    };

    class RagingBoxer : extends OnParentalBond {
        ON_PARENTAL_BOND {
            CHECK(IsIronFistBoosted(battler, move))
            return PARENTAL_BOND_PRIMAL_MAW;
        }
    };

    class AirBlower : extends OnEntry {
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

    class Juggernaut : extends OnChooseOffensiveStat, extends RemovesStatusOnImmunity {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
        }
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_PARALYSIS)
            return TRUE;
        }
    };

    class ShortCircuit : extends SwarmLike<TYPE_ELECTRIC> {};

    class MajesticBird : extends OnStat {
        ON_STAT {
            if (statId == STAT_SPATK) *stat *= 1.5;
        }
    };

    class Phantom : extends AddsType<TYPE_GHOST> {};

    class Intoxicate : extends AteAbility<TYPE_POISON> {};

    class Impenetrable : extends MagicGuard {};

    class Hypnotist : extends OnAccuracy<> {
        ON_ACCURACY {
            CHECK(move == MOVE_HYPNOSIS);
            *accuracy *= 1.5;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Overwhelm : extends OnTypeEffectiveness<>, extends TauntImmune {
        ON_TYPE_EFFECTIVENESS {
            CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
            return TRUE;
        }
    };

    class Scare : public Intimidate {};

    class MajesticMoth : extends OnEntry {
        ON_ENTRY {
            CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class SoulEater : extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            CHECK_NOT(BATTLER_MAX_HP(battler));
            CHECK(CanBattlerHeal(battler));
            BattleScriptCall(BattleScript_HandleSoulEaterEffect);
            return TRUE;
        }
    };

    class SoulLinker : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(IsBattlerAlive(battler))
            CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER))
            CHECK(move != MOVE_PAIN_SPLIT)

            BattleScriptCall(BattleScript_AttackerSoulLinker);
            return TRUE;
        }
    };

    class SweetDreams : extends OnEndTurn {
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

    class BadLuck : extends Breakable, extends OnCrit<ApplyOnTarget::FOE>, extends OnModifyEffectChance<ApplyOnTarget::FOE> {
        ON_CRIT { return NEVER_CRIT; }
        ON_MODIFY_EFFECT_CHANCE {
            if (*effectChance < 1) *effectChance = 0;
        }
    };

    class HauntedSpirit : extends OnDefender {
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

    class ElectricBurst : extends OnRecoil, extends OnOffensiveMultiplier<> {
        ON_RECOIL {
            CHECK(moveType == TYPE_ELECTRIC);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 20, 1);
        }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ELECTRIC) MUL(1.35);
        }
    };

    class RawWood : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_GRASS) MUL(1.2);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_GRASS) RESISTANCE(.5);
        }
    };

    class Solenoglyphs : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
        }
    };

    class SpiderLair : extends OnEntry {
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

    class FatalPrecision : extends OnAccuracy<>, extends OnCrit<> {
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

    class Seaweed : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(2);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS)) RESISTANCE(0.5);
        }
    };

    class PsychicMind : extends SwarmLike<TYPE_PSYCHIC> {};

    class PoisonAbsorb : extends Redirects<TYPE_POISON>, extends AbsorbHeal<TYPE_POISON>, extends OnEndTurn {
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

    class Scavenger : extends SoulEater {};

    class TwistedDimension : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

            gFieldTimers.started.trickRoom = TRUE;
            gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
            gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
            return TRUE;
        }
    };

    class MultiHeaded : extends OnParentalBond, extends IgnoresFortKnox {
        ON_PARENTAL_BOND {
            if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
            if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
            return MULTIHIT_SINGLE;
        }
    };

    class NorthWind : extends HailImmune, extends OnEntry {
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

    class Overcharge : extends OnTypeEffectiveness<>, extends OnCanStatusType {
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

    class ViolentRush : extends OnEntry {
        ON_ENTRY {
            gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
        }
    };

    class FlamingSoul : extends GaleWingsLike<TYPE_FIRE> {};

    class SagePower : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move)) MUL(1.5);
        }
    };

    class BoneZone : extends OnAfterTypeEffectiveness<> {
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

    class SpeedForce : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
        }
    };

    class SeaGuardian : extends OnEntry {
        ON_ENTRY {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

            int stat = GetHighestStatId(battler, TRUE);
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            SetStatChanger(stat, 1);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class MoltenDown : extends OnTypeEffectiveness<> {
        ON_TYPE_EFFECTIVENESS {
            CHECK(moveType == TYPE_FIRE)
            CHECK(defType == TYPE_ROCK)
            *mod = UQ_4_12(2.0);
            return TRUE;
        }
    };

    class Flock : extends SwarmLike<TYPE_FLYING> {};

    class FieldExplorer : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
        }
    };

    class Striker : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IsStrikerBoosted(battler, move)) MUL(1.3);
        }
    };

    class FrozenSoul : extends GaleWingsLike<TYPE_ICE> {};

    class Predator : extends SoulEater {};

    class Looter : extends SoulEater {};

    class LunarEclipse : extends Hypnotist, extends OnStab {
        ON_STAB { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; }
    };

    class SolarFlare : extends Immolate, extends Chloroplast {};

    class PowerCore : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; }
    };

    class SightingSystem : extends OnAccuracy<>, extends OnPriority {
        ON_ACCURACY { return ACCURACY_HITS_IF_POSSIBLE; }
        ON_PRIORITY {
            CHECK(gBattleMoves[move].accuracy)
            CHECK(gBattleMoves[move].accuracy < 80);
            return -3;
        }
    };

    class BadCompany : extends RandomizerBanned {};

    class Opportunist : extends OnPriority {
        ON_PRIORITY {
            CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2)
            return 1;
        }
    };

    class GiantWings : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].airBased) MUL(1.3);
        }
    };

    class Momentum : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
        }
    };

    class GripPincer : extends OnAttacker, extends OnAccuracy<> {
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

    class BigLeaves : extends Harvest, extends Merged<SolarPower, Chlorophyll>, extends Merged<Harvest, LeafGuard> {};

    class PreciseFist : extends OnCrit<>, extends OnModifyEffectChance<> {
        ON_CRIT {
            CHECK(IsIronFistBoosted(battler, move))
            return 1;
        }
        ON_MODIFY_EFFECT_CHANCE {
            if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
        }
    };

    class Deadeye : extends OnAccuracy<>, extends OnChooseDefensiveStat<> {
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

    class Artillery : extends OnAccuracy<> {
        ON_ACCURACY {
            CHECK(IsMegaLauncherBoosted(battler, move))
            return ACCURACY_HITS_IF_POSSIBLE;
        }
    };

    class IceDew : extends LightningRodClone<TYPE_ICE> {};

    class SunWorship : extends OnEntry {
        ON_ENTRY {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

            int stat = GetHighestStatId(battler, TRUE);
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class Pollinate : extends AteAbility<TYPE_BUG> {};

    class VolcanoRage : extends OnAttacker {
        ON_ATTACKER {
            CHECK(moveType == TYPE_FIRE)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
        }
    };

    class ColdRebound : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
            return FALSE;
        }
    };

    template <MoveEnum Move, int Power = 0>
    class SimpleEntryMove : extends OnEntry {
        ON_ENTRY { return UseEntryMove(battler, ability, Move, Power); }
    };
    class LowBlow : extends SimpleEntryMove<MOVE_FEINT_ATTACK, 40> {};

    class Spectralize : extends AteAbility<TYPE_GHOST> {};

    class SpectralShroud : extends Spectralize, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(gBattleStruct->ateBoost[battler])
            CHECK(moveType == TYPE_GHOST)
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
        }
    };

    class Discipline : extends RemovesStatusOnImmunity, extends TauntImmune {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_CONFUSION)
            return TRUE;
        }
    };

    class Thundercall : extends OnAttacker {
        ON_ATTACKER {
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
        }
    };

    class MarineApex : extends Infiltrator, extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
        }
    };

    class MightyHorn : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].hornBased) MUL(1.3);
        }
    };

    class HardenedSheath : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gBattleMoves[move].hornBased)
            CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptCall(BattleScript_AttackBoostActivates);
            gBattleScripting.battler = battler;
            return TRUE;
        }
    };

    class ArcticFur : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER { MUL(.65); }
    };

    class Lethargy : extends OnEntry, extends OnOffensiveMultiplier<> {
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

    class IronBarrage : extends MegaLauncher, extends SightingSystem {};

    class SteelBarrel : extends RockHead {};

    class PyroShells : extends OnAttacker {
        ON_ATTACKER {
            CHECK(IsMegaLauncherBoosted(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
        }
    };

    class FungalInfection : extends OnAttacker {
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

    class Parry : extends OnDefender, extends OnDefensiveMultiplier, extends OverrideBreakable {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
            return FALSE;
        }
        ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
    };

    class Scrapyard : extends OnDefender {
        ON_DEFENDER {
            CHECK(DidMoveHit())
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

            BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
            return TRUE;
        }
    };

    class LooseQuills : extends Scrapyard {};

    class ToxicDebris : extends OnDefender {
        ON_DEFENDER {
            CHECK(DidMoveHit())
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

            BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
            return TRUE;
        }
    };

    class Roundhouse : extends OnAccuracy<>, extends OnChooseDefensiveStat<> {
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

    class Mineralize : extends AteAbility<TYPE_ROCK> {};

    class LooseRocks : extends OnDefender {
        ON_DEFENDER {
            CHECK(DidMoveHit())
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

            BattleScriptCall(BattleScript_DefenderSetsStealthRock);
            return TRUE;
        }
    };

    class SpinningTop : extends OnAttacker {
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

    class RetributionBlow : extends OnReactive {
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

    class Fearmonger : extends Intimidate, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeParalyzed(battler, target))
            CHECK(IsMoveMakingContact(move, battler))
            CHECK(Random() % 100 < 10)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
        }
    };

    class ToxicSpill : extends OnEntry, extends OnEndTurn, extends OnExit {
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

    class DesertCloak : extends OnStatusImmune<ApplyOn::ALLY>, extends SandImmune {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_STATUS1)
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY))
            return TRUE;
        }
    };

    class Draconize : extends AteAbility<TYPE_DRAGON> {};

    class PrettyPrincess : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (!IsUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
        }
    };

    class SelfRepair : extends SelfSufficient, extends NaturalCure {};

    class Electromorphosis : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
            BattleScriptCall(BattleScript_ElectromorphosisActivates);
            return TRUE;
        }
    };

    class AtomicBurst : extends Electromorphosis, extends Galvanize {};

    template <int BoostType>
    class BoostedSwarmLike : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (move == BoostType) {
                if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                    MUL(1.8);
                else
                    MUL(1.3);
            }
        }
    };
    class Hellblaze : extends BoostedSwarmLike<TYPE_FIRE> {};

    class Riptide : extends BoostedSwarmLike<TYPE_WATER> {};

    class ForestRage : extends BoostedSwarmLike<TYPE_GRASS> {};

    class PrimalMaw : extends OnParentalBond {
        ON_PARENTAL_BOND {
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return PARENTAL_BOND_PRIMAL_MAW;
        }
    };

    class SweepingEdge : extends OnAccuracy<> {
        ON_ACCURACY {
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            return ACCURACY_HITS_IF_POSSIBLE;
        }
    };

    class GiftedMind : extends OnAccuracy<>, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
        ON_ACCURACY {
            CHECK(IS_MOVE_STATUS(move))
            return ACCURACY_HITS_IF_POSSIBLE;
        }
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (moveType == TYPE_BUG || moveType == TYPE_GHOST || moveType == TYPE_DARK) *mod = 0;
        }
    };

    class HydroCircuit : extends Transistor, extends OnAttacker {
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

    class Equinox : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT {
            int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
            int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
            if (atk > spAtk)
                *atkStatToUse = STAT_ATK;
            else if (spAtk > atk)
                *atkStatToUse = STAT_SPATK;
        }
    };

    class Absorbant : extends OnAttacker {
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

    class Clueless : extends CloudNine, extends Unsuppressable {};

    template <int N>
    class NoDamageHits : extends Persistent, extends OnEntry, extends Breakable {
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

        virtual int noDamageHits() override { return N; }
    };
    class CheatingDeath : extends NoDamageHits<2>, extends OverrideBreakable {};

    class CheapTactics : extends SimpleEntryMove<MOVE_SCRATCH> {};

    class Coward : extends OnEntry, extends Persistent {
        ON_ENTRY {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

            SetSingleUseAbilityCounter(battler, ability, TRUE);
            gRoundStructs[battler].protectedThisTurn = TRUE;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
            return TRUE;
        }
    };

    class VoltRush : extends GaleWingsLike<TYPE_ELECTRIC> {};

    class DuneTerror : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier, extends SandImmune {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_GROUND) MUL(1.2);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
        }
    };

    class InfernalRage : extends OnRecoil, extends OnOffensiveMultiplier<> {
        ON_RECOIL {
            CHECK(moveType == TYPE_FIRE);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 20, 1);
        }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) MUL(1.35);
        }
    };

    class DualWield : extends OnParentalBond {
        ON_PARENTAL_BOND {
            CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
            return PARENTAL_BOND_DUAL_WIELD;
        }
    };

    class ElementalCharge : extends OnAttacker {
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

    class Ambush : extends OnCrit {
        ON_CRIT {
            CHECK(gVolatileStructs[battler].isFirstTurn)
            return ALWAYS_CRIT;
        }
    };

    class Atlas : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

            gFieldTimers.started.gravity = TRUE;
            gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
            gFieldStatuses |= STATUS_FIELD_GRAVITY;
            BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
            return TRUE;
        }
    };

    class Radiance : extends OnImmune<ApplyOn::ANY>, extends OnAccuracy<> {
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

    class JawsOfCarnage : extends OnBattlerFaints<> {
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

    class AngelsWrath : extends OnAttacker, extends OnAccuracy<>, extends OnTypeEffectiveness<>, extends OnModifyEffectChance<>, extends OnCanStatusType {
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

    class PrismaticFur : extends ColorChange, extends Protean, extends OnDefensiveMultiplier, extends OverrideBreakable {
        ON_DEFENSIVE_MULTIPLIER { MUL(.5); }
        ON_BEFORE_ATTACK {
            if (battler == attacker)
                return Protean::onBeforeAttack(DELEGATE_ON_BEFORE_ATTACK);
            else
                return ColorChange::onBeforeAttack(DELEGATE_ON_BEFORE_ATTACK);
        }
        AbilityApplyOnWithTarget onBeforeAttackFor() override { return APPLY_ON_ATTACKER_OR_TARGET; }
    };

    class ShockingJaws : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeParalyzed(battler, target))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
        }
    };

    class FaeHunter : extends TypeSlayer<TYPE_FAIRY> {};

    class GravityWell : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

            gFieldTimers.started.gravity = TRUE;
            gFieldTimers.gravityTimer = GRAVITY_DURATION;
            gFieldStatuses |= STATUS_FIELD_GRAVITY;
            BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
            return TRUE;
        }
    };

    class Evaporate : extends OnAbsorb {
        ON_ABSORB {
            CHECK(moveType == TYPE_WATER)
            return ABSORB_RESULT_EVAPORATE;
        }
    };

    class Lumberjack : extends TypeSlayer<TYPE_GRASS> {};

    class AbsorbUp2 : extends Ability {};
    class WellBakedBody : extends AbsorbStatUp<TYPE_FIRE, STAT_DEF>, extends AbsorbUp2 {};

    class Furnace : extends OnEntry, extends OnDefender {
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

    class RockyPayload : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
        }
    };

    class EarthEater : extends AbsorbHeal<TYPE_GROUND> {};

    class LingeringAroma : extends Mummy {};

    class FairyTale : extends AddsType<TYPE_FAIRY> {};

    class RagingMoth : extends OnParentalBond {
        ON_PARENTAL_BOND {
            CHECK(moveType == TYPE_FIRE)
            return PARENTAL_BOND_DUAL_WIELD;
        }
    };

    class AdrenalineRush : extends MoxieClone<STAT_SPEED> {};

    class Archmage : extends RandomizerBanned, extends OnAttacker {
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

    class Cryomancy : extends OnModifyEffectChance<> {
        ON_MODIFY_EFFECT_CHANCE {
            if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
        }
    };

    class PhantomPain : extends OnTypeEffectiveness<> {
        ON_TYPE_EFFECTIVENESS {
            CHECK(moveType == TYPE_GHOST)
            CHECK(defType == TYPE_NORMAL)
            CHECK_NOT(*mod)
            *mod = UQ_4_12(1.0);
            return TRUE;
        }
    };

    class Purgatory : extends BoostedSwarmLike<TYPE_GHOST> {};

    class Emanate : extends AteAbility<TYPE_PSYCHIC> {};

    class KunoichiBlade : extends Technician, extends SkillLink {};

    class MonkeyBusiness : extends SimpleEntryMove<MOVE_TICKLE> {};

    class CombatSpecialist : extends Merged<IronFist, Striker> {};

    class JunglesGuard : extends FlowerVeil {};

    class HuntersHorn : extends SoulEater, extends MightyHorn {};

    class PixiePower : extends FairyAura, extends OnAccuracy<> {
        ON_ACCURACY {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class PlasmaLamp : extends OnOffensiveMultiplier<>, extends OnAccuracy<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC) MUL(1.2);
        }
        ON_ACCURACY {
            CHECK(moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC)
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class MagmaEater : extends SoulEater, extends MoltenDown {};

    class SuperHotGoo : extends Merged<Gooey, FlameBody> {};

    class Nika : extends IronFist {};

    class Archer : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].arrowBased) MUL(1.3);
        }
    };

    class SuperSlammer : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].hammerBased) MUL(1.3);
        }
    };

    class InverseRoom : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

            gFieldTimers.started.inverseRoom = TRUE;
            gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
            gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
            return TRUE;
        }
    };

    class FrostBurn : extends OnAttacker {
        ON_ATTACKER {
            CHECK(moveType == TYPE_FIRE)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
        }
    };

    class ItchyDefense : extends OnDefender {
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

    class Generator : extends OnEntry, extends OnTerrain, extends OnExit, extends Persistent {
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

    class MoonSpirit : extends OnStab {
        ON_STAB { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; }
    };

    class DustCloud : extends SimpleEntryMove<MOVE_SAND_ATTACK> {};

    class TippingPoint : extends OnDefender {
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

    class BerserkerRage : extends TippingPoint, extends Rampage {};

    class Trickster : extends SimpleEntryMove<MOVE_DISABLE> {};

    class SandGuard : extends OnImmune<>, extends OnDefensiveMultiplier, extends SandImmune {
        ON_IMMUNE {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
            return blockPriority(DELEGATE_IMMUNE);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
        }
    };

    class NaturalRecovery : extends Merged<NaturalCure, Regenerator> {};

    class WindRider : extends OnEntry, extends OnAbsorb {
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

    class SoothingAroma : extends OnEntry {
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

    class PrimAndProper : extends WonderSkin, extends CuteCharm {};

    class SuperStrain : extends OnRecoil, OnBattlerFaints<> {
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

    class Enlightened : extends Emanate, extends InnerFocus {};

    class PeacefulSlumber : extends SweetDreams, extends SelfSufficient {
        ON_END_TURN {
            if (!SweetDreams::onEndTurn(DELEGATE_END_TURN)) return SelfSufficient::onEndTurn(DELEGATE_END_TURN);
            gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
            return TRUE;
        }
    };

    class Aftershock : extends OnAttacker {
        ON_ATTACKER {
            CHECK(gBattleMoves[move].power)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
        }
    };

    class FreezingPoint : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanGetFrostbite(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
            return TRUE;
        }
    };

    class CryoProficiency : extends FreezingPoint {
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
        ON_DEFENDER { return FreezingPoint::onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(ability, battler, attacker, move, moveType); }
    };

    class ArcaneForce : extends MysticPower, extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
        }
    };

    class Doombringer : extends SimpleEntryMove<MOVE_DOOM_DESIRE> {};

    class Wishmaker : extends OnEntry, extends Persistent {
        ON_ENTRY {
            int counter = GetSingleUseAbilityCounter(battler, ability);
            CHECK(counter < 3)
            CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

            SetSingleUseAbilityCounter(battler, ability, counter + 1);
            return TRUE;
        }
    };

    class YukiOnna : public Intimidate, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanInfatuate(battler, target))
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
        }
    };

    class Suppress : extends SimpleEntryMove<MOVE_TORMENT> {};

    class Refrigerator : extends Filter, extends Illuminate {};

    class HeavenAsunder : extends OnCrit<> {
        ON_CRIT {
            if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
            return 1;
        }
    };

    class PurifyingWaters : extends WaterVeil, extends Hydration {};

    class Seaborne : extends Drizzle, extends SwiftSwim {};

    class HighTide : extends OnAttacker {
        ON_ATTACKER {
            CHECK(moveType == TYPE_WATER)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
        }
    };

    class ChangeOfHeart : extends SimpleEntryMove<MOVE_HEART_SWAP> {};

    class MysticBlades : extends KeenEdge, extends OnSwapSplit {
        ON_SWAP_SPLIT {
            CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
            return TRUE;
        }
    };

    class NegateFrzSpatkDrop : extends Ability {};
    class Determination : extends OnOffensiveMultiplier<>, extends NegateFrzSpatkDrop {
        ON_OFFENSIVE_MULTIPLIER {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
        }
    };

    class Fertilize : extends AteAbility<TYPE_GRASS> {};

    class CanInfatuateAny : extends Ability {};
    class PureLove : extends OnDefender, extends OnAttacker, extends CanInfatuateAny {
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
        ON_DEFENDER { return CuteCharm::onEither(DELEGATE_DEFENDER); }
    };

    class Fighter : extends SwarmLike<TYPE_FIGHTING> {};

    class Telekinetic : extends SimpleEntryMove<MOVE_TELEKINESIS> {};

    class Combustion : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) MUL(1.5);
        }
    };

    class PonyPower : extends Merged<KeenEdge, MysticBlades> {};

    class PowderBurst : SimpleEntryMove<MOVE_POWDER> {};

    class Retriever : extends OnExit {
        ON_EXIT {
            CHECK(IsBattlerAlive(battler))
            CHECK_NOT(gBattleMons[battler].item)

            u8 side = GetBattlerSide(gActiveBattler);
            u8 index = gBattlerPartyIndexes[gActiveBattler];
            u16 originalItem = gLastUsedItem =
                side == B_SIDE_PLAYER ? gBattleStruct->itemStolen[index].originalItem : gBattleStruct->opposingOriginalItems[index];

            CHECK(originalItem)

            gBattleStruct->usedHeldItems[index][side] = ITEM_NONE;

            UpdateBattlerItem(gActiveBattler, originalItem);

            BattleScriptCall(BattleScript_RetrieverExits);
            return TRUE;
        }
    };

    class MonsterMash : SimpleEntryMove<MOVE_TRICK_OR_TREAT> {};

    class TwoStep : extends OnAttacker {
        ON_ATTACKER {
            CHECK(IsDance(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
        }
    };

    class Spiteful : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(move != MOVE_STRUGGLE)
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK(gBattleMons[attacker].pp[gChosenMovePos])

            BattleScriptCall(BattleScript_AbilitySpiteful);
            return TRUE;
        }
    };

    class Fortitude : extends OnDefender {
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

    class Devourer : extends PrimalMaw, extends StrongJaw {};

    class PhantomThief : SimpleEntryMove<MOVE_SPECTRAL_THIEF, 40> {};

    class EarlyGrave : extends GaleWingsLike<TYPE_GHOST> {};

    class BassBoosted : extends Merged<PunkRock, Amplifier> {};

    class FlamingJaws : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeBurned(target))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_BURN);
        }
    };

    class MonsterHunter : extends TypeSlayer<TYPE_DARK> {};

    class CrownedSword : extends IntrepidSword, extends AngerPoint {};

    class CrownedShield : extends DauntlessShield, extends Stamina {};

    class BerserkDna : extends OnEntry {
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

    class CrownedKing : extends AsOneShadowRider, extends AsOneIceRider {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); }
        ON_BATTLER_FAINTS {
            CHECK(ChillingNeigh::onBattlerFaints(DELEGATE_BATTLER_FAINTS) | GrimNeigh::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
            gBattleScripting.abilityPopupOverwrite = ABILITY_CROWNED_KING;
            BattleScriptCall(BattleScript_AbilityPopUpStack);
            return NO_ANNOUNCE;
        }
    };

    class SnapTrapWhenHit : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
            return FALSE;
        }
    };

    class Permanence : extends OnEntry {
        ON_ENTRY { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); }
    };

    class Hubris : extends GrimNeigh {};

    class CosmicDaze : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
        }
    };

    class MindsEye : extends Breakable, extends OnTypeEffectiveness<> {
        ON_TYPE_EFFECTIVENESS { return Scrappy::hitGhost(DELEGATE_TYPE_EFFECTIVENESS); }
    };

    class BloodPrice : extends OnEndTurn, extends OnOffensiveMultiplier<> {
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

    class SpikeArmor : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBleed(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
            return TRUE;
        }
    };

    class VoodooPower : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IS_MOVE_SPECIAL(move))
            CHECK(CanBleed(attacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
            return TRUE;
        }
    };

    class ChromeCoat : extends OnDefensiveMultiplier, extends OnStat<> {
        ON_DEFENSIVE_MULTIPLIER {
            if (IS_MOVE_SPECIAL(move)) MUL(.6);
        }
        ON_STAT {
            if (statId == STAT_SPEED) *stat *= .9;
        }
    };

    class Banshee : LiquidVoiceClone<TYPE_GHOST> {};

    class WebSpinner : SimpleEntryMove<MOVE_STRING_SHOT> {};

    class ShowdownMode : extends OnEntry {
        ON_ENTRY {
            gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
        }
    };

    class SeedSower : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

            BattleScriptCall(BattleScript_SeedSower);
            return TRUE;
        }
        TerrainType allowTerrainIfAirborne() override { return TERRAIN_GRASSY; }
    };

    class Airborne : extends OnOffensiveMultiplier<ApplyOn::ALLY> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FLYING) MUL(1.3);
        }
    };

    class Parroting : extends Soundproof, extends OnCopyMove {
        ON_COPY_MOVE {
            CHECK(IsSoundMove(attacker, move))
            return UseOutOfTurnAttack(battler, target, ability, move, 0);
        }
    };

    class SaltCircle : extends OnEntry {
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

    class PurifyingSalt : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_GHOST) RESISTANCE(.5);
        }
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
    };

    class ParadoxBoostEffect : extends OnStat<>, extends OnEntry {
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

    class Protosynthesis : extends ParadoxBoostEffect, extends OnWeather {
        ON_ENTRY { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        ON_WEATHER { return handler(ability, battler, IsWeatherActive(WEATHER_SUN_ANY), B_MSG_PARADOX_BOOST_WEATHER, ABILITY_BS_CALL); }
    };

    class QuarkDrive : extends ParadoxBoostEffect, extends OnTerrain {
        ON_ENTRY {
            return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        }
        ON_WEATHER { return handler(ability, battler, IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN), B_MSG_PARADOX_BOOST_TERRAIN, ABILITY_BS_CALL); }
    };

    class WindPower : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gBattleMoves[move].airBased)
            CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
            BattleScriptCall(BattleScript_ElectromorphosisActivates);
            return TRUE;
        }
    };

    class Impulse : extends OnChooseDefensiveStat<> {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
        }
    };

    class TerminalVelocity : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
        }
    };

    class AngerShell : extends OnDefender {
        ON_DEFENDER {
            CHECK(CheckHalfHpAbility(battler, attacker))
            CHECK_NOT(GetAbilityState(battler, ability))
            CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_AngerShell);
            return TRUE;
        }
    };

    class Egoist : extends OnReactive {
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

    class ReadiedAction : extends OnEntry {
        ON_ENTRY {
            gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
        }
    };

    class DarkGaleWings : extends GaleWingsLike<TYPE_DARK> {};

    class GuiltTrip : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK_NOT(IsBattlerAlive(battler))
            CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

            BattleScriptCall(BattleScript_GuiltTrip);
            return TRUE;
        }
    };

    class WaterGaleWings : extends GaleWingsLike<TYPE_WATER> {};

    class ZeroToHero : extends FormChange, extends OnEntry, extends OnExit {
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

    class Costar : extends OnEntry {
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

    class Commander : extends FormChange, extends OnBattlerFaints<ApplyOnTarget::ALLY>, extends OnAccuracy<ApplyOnTarget::TARGET> {
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

    class EjectPackAbility : extends Persistent {};

    class VengefulSpirit : extends HauntedSpirit, extends Vengeance {};

    class CudChew : extends OnEndTurn {
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

    class ArmorTail : extends QueenlyMajesty {};

    class MindCrush : extends StrongJaw, extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
        }
    };

    class SupremeOverlord : extends OnEntry, extends OnStat<> {
        ON_ENTRY {
            CHECK(gFaintedMonCount[GetBattlerSide(battler)])

            return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
        }
        ON_STAT {
            if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
        }
    };

    class IllWill : extends OnDefender {
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

    class FireScales : extends IceScales {};

    class WatchYourStep : extends OnEntry {
        ON_ENTRY {
            u8 targetSide = GetOppositeSide(battler);
            CHECK(gSideTimers[targetSide].spikesAmount < 3)

            gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
            gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
            BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
            return TRUE;
        }
    };

    class RapidResponse : extends OnEntry {
        ON_ENTRY {
            gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
        }
    };

    class DoubleIronBarbs : extends OnDefender {
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

    class ThermalExchange : extends OnDefender, extends RemovesStatusOnImmunity {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(moveType == TYPE_FIRE)
            CHECK(CanRaiseStat(battler, STAT_ATK))

            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) {
            CHECK(status & CHECK_BURN)
            return TRUE;
        }
    };

    class GoodAsGold : extends OnImmune<> {
        ON_IMMUNE {
            CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
            *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class SharingIsCaring : extends OnReactive {
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

    template <int Stat>
    class RuinEffect : extends OnStat<ApplyOn::OTHER> {
        static constexpr auto ruinAbilities = abilitiesAs<RuinEffect<Stat>>();
        ON_STAT {
            if (statId != Stat) return;
            if (*flags & Stat) return;
            ON_ABILITY(battler, FALSE, ruinAbilities[ability], return) *stat *= .75;
            *flags |= static_cast<NonStackingState>(1 << Stat);
        }
    };

    class TabletsOfRuin : extends RuinEffect<STAT_ATK> {};
    class SwordOfRuin : extends RuinEffect<STAT_DEF> {};
    class VesselOfRuin : extends RuinEffect<STAT_SPATK> {};
    class BeadsOfRuin : extends RuinEffect<STAT_SPEED> {};

    class PermafrostClone : extends Permafrost {};

    class Gallantry : extends NoDamageHits<1> {};

    class OrichalcumPulse : extends Drought, extends OnStat<> {
        ON_STAT {
            if (statId != STAT_ATK) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
        }
    };

    class SunBasking : extends OnImmune<>, extends OnDefensiveMultiplier {
        ON_IMMUNE {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
            return blockPriority(DELEGATE_IMMUNE);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
        }
    };

    class WingedKing : extends Ability, extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
        }
    };

    class HadronEngine : extends ElectricSurge, extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
        }
    };

    class IronSerpent : extends WingedKing {};

    class SweepingEdgePlus : extends KeenEdge, extends SweepingEdge {};

    class CelestialBlessing : extends OnEndTurn {
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

    class MinionControl : extends OnParentalBond {
        ON_PARENTAL_BOND { return PARENTAL_BOND_MINION_CONTROL; }
    };

    class MoltenBlades : extends KeenEdge, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeBurned(target))
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_BURN);
        }
    };

    class HauntingFrenzy : extends AdrenalineRush, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanMoveHaveExtraFlinchChance(move))
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
        }
    };

    class NoiseCancel : extends Soundproof {
        AbilityApplyOn onImmuneFor() override { APPLY_ON_ALLY; }
    };

    class RadioJam : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeDisabled(target))
            CHECK(IsSoundMove(battler, move))
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
        }
    };

    class Ole : extends OnAccuracy<ApplyOnTarget::TARGET> {
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

    class Malicious : public Intimidate {};

    class DeadPower : extends OnAttacker, extends OnStat<> {
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

    class BrawlingWyvern : extends NoGuard, extends OnModifyMoveFlags {
        ON_MODIFY_MOVE_FLAGS {
            CHECK(flag == MOVE_FLAG_PUNCH)
            CHECK(IS_MOVE_TYPE(move, TYPE_DRAGON))
            return TRUE;
        }
    };

    class JunshiSanda : extends OnModifyMoveFlags {
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

    class MythicalArrows : extends Archer, extends OnSwapSplit {
        ON_SWAP_SPLIT {
            CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
            CHECK(gBattleMoves[move].arrowBased);
            return TRUE;
        }
    };

    class Lawnmower : extends OnEntry {
        ON_ENTRY {
            CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

            BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
            return TRUE;
        }
    };

    class Flourish : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
        }
    };

    class DesertSpirit : extends SandStream, extends OnAfterTypeEffectiveness<> {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
                *mod = UQ_4_12(1.0);
            }
        }
    };

    class Contempt : extends Unaware {};

    class Aerialist : extends Merged<Levitate, Flock> {};

    class TeraShell : extends Breakable, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET> {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
        }
    };

    class ToxicChain : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
        }
    };

    class ParasiticSpores : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

            gVolatileStructs[battler].parasiticSpores = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
        }
    };

    template <MoveEffectEnum Effect>
    class PoisonPuppeteerLike : extends OnBattlerFaints<ApplyOnTarget::ANY>, extends SetStateOnEffect<Effect>, extends OnReactive {
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

    class PoisonPuppeteer : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
        ON_REACTIVE {
            return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
        }
    };

    class Entrance : extends PoisonPuppeteerLike<MOVE_EFFECT_CONFUSION> {
        ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); }
    };

    class Rejection : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gFieldTimers.quashTimer)

            gFieldTimers.quashTimer = QUASH_DURATION;
            gFieldTimers.started.quash = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
        }
    };

    class AppleEnlightenment : extends FurCoat, extends MagicGuard {};

    class BalloonBomber : extends Merged<Aftermath, Inflatable> {};

    class FlamingMaw : extends FlamingJaws, extends StrongJaw {};

    class Demolitionist : extends ReadiedAction, extends OnInfiltrate, extends OnAttacker {
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

    class RockhardWill : extends SwarmLike<TYPE_ROCK> {};
    class FragrantDaze : extends OnEither {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBeConfused(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_CONFUSION, battler, opponent);
            return TRUE;
        }
    };

    class LowVisibility : extends OnEntry {
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

    class OldMariner : extends Seaweed, extends Amphibious {};

    class Ectoplasm : extends OnStat<> {
        ON_STAT {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        }
    };

    class BeautifulMusic : extends OnAttacker, extends CanInfatuateAny {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(Random() % 2)
            CHECK(IsSoundMove(battler, move))

            return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
        }
    };

    class SnowSong : LiquidVoiceClone<TYPE_ICE> {};

    class GreaterSpirit : extends OnEntry {
        ON_ENTRY {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

            int stat = GetHighestStatId(battler, TRUE);
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class Resonance : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(IsSoundMove(battler, move))
            CHECK(Random() % 100 < 50)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class EtherealRush : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        }
    };

    class CuteAntecedence : extends GaleWingsLike<TYPE_FAIRY> {};

    class RecurringNightmare : extends OnRevive {
        ON_REVIVE {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
            return B_MSG_FADE_OUT;
        }
    };

    class MenacingSituation : extends OnEither {
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

    class ShinyLightning : extends OnAccuracy<> {
        ON_ACCURACY {
            if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Terrify : public Intimidate {};

    class IceDownfall : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
            return FALSE;
        }
    };

    class LastStand : extends Breakable, extends OnStat {
        ON_STAT {
            if (statId == STAT_DEF || statId == STAT_SPDEF)
                *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
        }
    };

    class PyroclasticFlow : Merged<MoltenDown, Corrosion> {};

    class BloodBath : extends PoisonPuppeteerLike<MOVE_EFFECT_BLEED>, extends RemovesStatusOnImmunity {
        ON_REACTIVE {
            return PoisonPuppeteerClone(
                ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
        }
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_BLEED)
            return TRUE;
        }
    };

    class BattleAura : extends OnCrit<ApplyOnTarget::ANY> {
        ON_CRIT { return 2; }
    };

    class Bloodlust : extends BloodBath, extends SoulEater {
        ON_BATTLER_FAINTS {
            int result = 0;
            if (battler == attacker) {
                result |= SoulEater::onBattlerFaints(DELEGATE_BATTLER_FAINTS);
            }
            return result | BloodBath::onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        ApplyOnTarget onBattlerFaintsFor override { return ApplyOnTarget::ANY; }
    };

    class PiercingSolo : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(IsSoundMove(battler, move))

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class Rhythmic : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); }
    };

    class ChunkyBassLine : extends OnAttacker {
        ON_ATTACKER {
            CHECK(IsSoundMove(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
        }
    };

    class DualHammer : extends OnParentalBond {
        ON_PARENTAL_BOND {
            CHECK(gBattleMoves[move].hammerBased)
            return PARENTAL_BOND_DUAL_WIELD;
        }
    };

    class DentingBlows : extends OnAttacker {
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

    class IceColdHunter : extends HailImmune, extends OnParentalBond {
        ON_PARENTAL_BOND {
            CHECK(moveType == TYPE_ICE)
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
            return PARENTAL_BOND_ICE_COLD_HUNTER;
        }
    };

    class SoulCrusher : extends OnOffensiveMultiplier<>, extends OnChooseDefensiveStat<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].hammerBased) MUL(1.1);
        }
        ON_CHOOSE_DEFENSIVE_STAT {
            CHECK(gBattleMoves[move].hammerBased)
            return STAT_SPDEF;
        }
    };

    class ArcFlash : extends OnAttacker, extends OnDefender {
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

    class Unicorn : extends MightyHorn, Pixilate {};

    class OnTheProwl : extends OnEntry {
        ON_ENTRY {
            gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
        }
    };

    class Pretentious : extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            CHECK(gVolatileStructs[battler].critBoost < 3);
            gVolatileStructs[battler].critBoost++;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
            BattleScriptCall(BattleScript_AbilityBoostsCrit);
            return TRUE;
        }
    };

    class VenoblazePincers : extends OnAttacker, extends OnOffensiveMultiplier<> {
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

    class EternalBlessing : extends CelestialBlessing, extends Regenerator {};

    class Ripen : extends Ability {};
    class SugarRush : extends Unburden, extends Ripen {};

    class PeacefulRest : extends OnEndTurn {
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

    class WhiteNoise : extends PeacefulRest, extends Static {};

    class SmokeyManeuvers : extends OnAccuracy<ApplyOnTarget::TARGET> {
        ON_ACCURACY {
            CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
            *accuracy /= 1.25;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class PowerMetal : LiquidVoiceClone<TYPE_STEEL> {};

    class PowerEdge : extends KeenEdge, extends OnChooseDefensiveStat<> {
        ON_CHOOSE_DEFENSIVE_STAT {
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            return STAT_SPDEF;
        }
    };

    class Superconductor : extends OnOffensiveMultiplier<>, extends OnMoveType {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        }
        ON_MOVE_TYPE {
            CHECK(moveType == TYPE_STEEL)
            *ateBoost = TRUE;
            return TYPE_ELECTRIC + 1;
        }
    };

    class UltraInstinct : extends OnDefender, extends OnDefensiveMultiplier, extends OverrideBreakable {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
            return FALSE;
        }
        ON_DEFENSIVE_MULTIPLIER { MUL(.8); }
    };

    class UnlockedPotential : extends Berserk, extends InnerFocus {};

    class HigherRank : extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
        }
    };

    class FuneralPyre : extends OnEntry, extends OnEndTurn {
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

    class FlameBubble : extends WaterBubble, extends FlamingSoul {};

    class ElementalVortex : extends Merged<WaterAbsorb, FlashFire> {};

    class SnowyWrath : extends SnowWarning, extends Cryomancy {};

    class PatternChange : extends ShedSkin, extends Protean {};

    class NoTurningBack : extends OnDefender {
        ON_DEFENDER {
            CHECK(CheckHalfHpAbility(battler, attacker))
            CHECK_NOT(GetAbilityState(battler, ability))
            CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_NoTurningBack);
            return TRUE;
        }
    };

    class FlammableCoat : extends FormChange, extends OnDefender, extends OnBeforeAttack<> {
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

    class DracoMorale : SimpleEntryMove<MOVE_DRAGON_CHEER> {};

    class BadOmen : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (isCrit) MUL(.25);
        }
    };

    class MoshPit : extends OnOffensiveMultiplier<ApplyOn::ALLY_ONLY> {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST)
                MUL(1.25);
            else
                MUL(1.5);
        }
    };

    class BloodStain : extends OnEither, extends OnEntry, extends Unsuppressable, extends RemovesStatusOnImmunity {
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

    class BloodStigma : extends OnOffensiveMultiplier<>, extends RemovesStatusOnImmunity, extends Unsuppressable {
        ON_OFFENSIVE_MULTIPLIER {
            if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
    };

    class Slipstream : extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT { secondaryAtkStatToUse[STAT_SPEED] += 20; }
    };

    class MaximumAcceleration : extends Slipstream, extends SpeedBoost {};

    class Sidewinder : extends CoilUp, extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
            gStatuses4[battler] |= STATUS4_COILED;
            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
            return TRUE;
        }
    };

    class Petrify : public Intimidate {
        ON_ENTRY {
            int loweredStats = 0;
            int intimidated = Intimidate::onEntry(DELEGATE_ENTRY);
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

    class Fluffiest : extends OnDefensiveMultiplier {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        }
    };

    class WayOfPrecision : extends InnerFocus, extends InnerFocus {};

    class WayOfSwiftness : extends Pretentious, extends SwiftSwim {};

    class AtomicPunch : extends IronFist {
        ON_OFFENSIVE_MULTIPLIER {
            IronFist::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            if (moveType == TYPE_STEEL) MUL(1.3);
        }
    };

    class IronGiant : extends Heatproof, extends Juggernaut {};

    class MasterHand : extends Rampage, extends MegaLauncher {};

    class FinalBlow : extends FatalPrecision {};

    class Hospitality : extends OnEntry {
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

    class ButterUp : extends Merged<Hospitality, SoothingAroma> {};

    class VitalityStrike : extends OnAttacker {
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

    class HugeWings : extends Merged<GiantWings, Levitate> {};

    class SwordOfDamnation : extends SwordOfRuin, extends Unaware {};

    class RestrainingOrder : extends OnDefender {
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

    class AssassinsTools : extends OnAttacker {
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

    class Frostmaw : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanGetFrostbite(target))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
        }
    };

    class Patchwork : extends Disguise, extends OnDefender {
        ON_DISGUISE {
            SpeciesEnum species = Disguise::onDisguise(DELEGATE_DISGUISE);
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

    class BlindRage : extends MoldBreaker, extends Scrappy {};

    class ApexPredator : extends SoulEater, extends ToughClaws {};

    class DragonsRitual : extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
            BattleScriptCall(BattleScript_DragonsRitual);
            return TRUE;
        }
    };

    class PinnacleBlade : extends OnInfiltrate, extends OnAttacker {
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

    class Energized : extends Generator, extends OnBattlerFaints<> {
        ON_BATTLER_FAINTS {
            CHECK(moveType == TYPE_ELECTRIC);
            SetOncePerTurnAbilityCounter(battler, ability, TRUE);
            BattleScriptCall(BattleScript_GeneratorActivatesRet);
            return TRUE;
        }
    };

    class ColorSpectrum : extends OnEndTurn, extends OnOffensiveMultiplier<> {
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

    class SteelBeetle : extends RagingBoxer, extends Pollinate {};

    class FromTheShadows : extends OnAttacker {
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

    class RagePoint : OnDefender, extends OnOffensiveMultiplier<>, extends NegateBurnAtkDrop, extends NegateFrzSpatkDrop {
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

    class HotCoals : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

            gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
        }
    };

    class TerastalTreasure : extends OnDefensiveMultiplier, extends OnStat<> {
        ON_DEFENSIVE_MULTIPLIER { MUL(.6); }
        ON_STAT {
            if (statId == STAT_SPEED) *stat *= .8;
        }
    };

    class ShockingMaw : extends ShockingJaws, StrongJaw {};

    class GleamEyes : extends Merged<Intimidate, Frisk> {};

    class RousedFangs : extends StrongJaw, extends MindCrush {};

    class DreamState : extends BattleArmor {};

    class DreamWhimsy : SimpleEntryMove<MOVE_YAWN> {};

    class LunarAffinity : extends OnCopyMove {
        ON_COPY_MOVE {
            CHECK(gBattleMoves[move].lunar)
            return UseOutOfTurnAttack(battler, target, ability, move, 0);
        }
    };

    class FlameShield : extends Filter {};

    class AquaticDweller : extends Aquatic, extends OnOffensiveMultiplier<> {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_WATER) MUL(1.5);
        }
    };

    class ApplePie : extends SelfSufficient {};

    class Hover : extends GroundImmune, AddsType<TYPE_PSYCHIC> {};

    class Depravity : extends Merciless, extends Overcharge {};

    class Wildfire : SimpleEntryMove<MOVE_FIRE_SPIN> {};

    class JumpScare : extends OnEntry, extends Persistent {
        ON_ENTRY {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
            return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
        }
    };

    class TarToss : SimpleEntryMove<MOVE_TAR_SHOT> {};

    class StunShock : extends OnAttacker {
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

    class RagingGoddess : extends Rampage, extends HyperAggressive {};

    class Whiplash : extends OnAttacker {
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

    class SupersweetSyrup : extends OnDefender {
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

    class TrashHeap : extends ToxicSpill, extends Corrosion {};

    class SludgyMix : extends Intoxicate, extends PunkRock {};

    class Overwatch : extends OnTheProwl, extends Stakeout {};

    class WindRage : extends GiantWings, extends SimpleEntryMove<MOVE_DEFOG> {};

    class VictoryBomb : extends OnDefender, extends OnMoveType {
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

    class RazorSharp : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(gIsCriticalHit)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class ToTheBone : extends RazorSharp, extends Sniper {};

    class BladeDance : extends OnAttacker {
        ON_ATTACKER {
            CHECK(IsDance(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
        }
    };

    class ApeShift : extends FormChange, extends OnEntry, extends OnEndTurn, extends OnDefender, extends OnCrit<> {
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

    class KnowYourPlace : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK_NOT(gVolatileStructs[target].dazed)
            CHECK(IsMoveMakingContact(move, battler))

            gVolatileStructs[target].dazed = 5;
            BattleScriptCall(BattleScript_TargetDazed);
            return TRUE;
        }
    };

    class DeepCuts : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class LifeSteal : extends OnEndTurn {
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

    class RudeAwakening : extends RemovesStatusOnImmunity {
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_SLEEP)
            CHECK(GetAbilityState(battler, ability))
            return TRUE;
        }
    };

    class TeraformZero : extends TeraShell, extends OnEntry {
        ON_ENTRY {
            CHECK(!GetSingleUseAbilityCounter(battler, ability));
            SetSingleUseAbilityCounter(battler, ability, TRUE);
            CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
            BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
            return TRUE;
        }
    };

    class SetAblaze : extends PoisonPuppeteerLike<MOVE_EFFECT_BURN> {
        ON_REACTIVE {
            return PoisonPuppeteerClone(
                ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
        }
    };

    class Breakwater : extends Stall, extends SwiftSwim {};

    class MagicalFists : extends IronFist, extends OnChooseOffensiveStat {
        ON_CHOOSE_OFFENSIVE_STAT {
            if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
        }
    };

    class Cutthroat : extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

            gStatuses4[battler] |= STATUS4_CUTTHROAT;
            return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
        }
    };

    class SandBender : extends SandStream, extends SandForce {};

    class SandPit : SimpleEntryMove<MOVE_SAND_TOMB, 20> {};

    class DesolateSun : extends RandomizerBanned {};

    ON_EITHER(Daybreak) {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
    class Daybreak : extends Ability {
        ON_EITHER_ABILITY(Daybreak),
    };

    class EnergySiphon : extends OnAttacker {
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

    class Reservoir : extends Merged<WaterAbsorb, StormDrain> {};

    static int NeurotoxinCondition(int battler, int target) {
        return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
    }
    class Neurotoxin : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
        ON_REACTIVE { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); }
    };

    class EnergizedHorns : extends MightyHorn {
        int onSwapSplit(ON_SWAP_SPLIT) {
            CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
            CHECK(gBattleMoves[move].hornBased);
            return TRUE;
        }
    };

    class SpiderLairUpgrade : extends OnEntry {
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

    class CrustCoat : extends BattleArmor {};

    class Puffy : extends Fluffy {};

    class BalloonBlitz : extends Inflatable, extends HyperAggressive {};

    class StrikerPixilate : extends Striker, extends Pixilate {};

    // 2.6
    class DoomBlast : extends OnRecoil, extends OnOffensiveMultiplier<> {
        ON_RECOIL {
            CHECK(moveType == TYPE_DARK);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 20, 1);
        }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_DARK) MUL(1.35);
        }
    };

    class Bruteforce : extends Reckless, extends RockHead {};

    class FaradayCage : extends ShellArmor, extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
            return FALSE;
        }
    };

    class AcidicSlime : extends Corrosion, extends OnStab {
        ON_STAB { return moveType == TYPE_WATER; }
    };

    class RoseGarden : extends OnEntry {
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

    class Qigong : extends Rampage, extends FightingSpirit, extends OnAccuracy<> {
        ON_ACCURACY { return ACCURACY_ALWAYS_HITS; }
    };

    class ConjurerOfDeceit : extends MagicGuard, extends MagicBounce {};

    class DeepFreeze : extends OnOffensiveMultiplier<>, extends OnDefensiveMultiplier {
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) MUL(1.25);
        }
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        }
    };

    class SoulDevourer : extends SoulEater, extends PhantomPain {};

    class ChampionsEntrance : extends Merged<Intimidate, ViolentRush> {};

    class Presto : extends OnPriority {
        ON_PRIORITY {
            CHECK(BATTLER_MAX_HP(battler))
            CHECK(IsSoundMove(battler, move))
            return 1;
        }
    };

    class Samba : extends Striker, extends Dancer {};

    class Gladiator : extends BoostedSwarmLike<TYPE_FIGHTING> {};

    class ForsakenHeart : extends OnBattlerFaints<ApplyOnTarget::ANY> {
        ON_BATTLER_FAINTS {
            CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

            BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
            return TRUE;
        }
    };

    class Relentless : extends ExploitWeakness, extends Merciless {};

    class Soothsayer : extends OnEntry, extends OnEndTurn, extends OnAfterTypeEffectiveness<ApplyOnTarget::TARGET>, extends Persistent {
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

    class CorruptedMind : extends RandomizerBanned, extends OnTypeEffectiveness<>, extends OnModifyEffectChance<> {
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

    class FlameCoat : extends OnEntry, extends OnEndTurn {
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

    class UnownPower : extends RandomizerBanned, extends OnStab, extends OnAfterTypeEffectiveness<> {
        ON_STAB { return TRUE; }
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
        }
    };

    class SuperScope : extends MegaLauncher, extends Artillery {};

    class VenomCrown : extends PoisonPoint, extends MightyHorn, extends RandomizerBanned {};

    class BlightScale : extends PoisonPoint, extends Multiscale, extends RandomizerBanned {};

    class Gunman : extends MegaLauncher, extends OnModifyMoveFlags {
        ON_MODIFY_MOVE_FLAGS {
            CHECK(flag == MOVE_FLAG_MEGA_LAUNCHER)
            CHECK(IS_MOVE_STATUS(move))
            return TRUE;
        }
    };

    class Caretaker : extends OnEndTurn, extends FriendGuard {
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

    class PoseidonsDominion : SimpleEntryMove<MOVE_WHIRLPOOL> {};

    class DualShadow : extends HungerSwitch, extends OnRecoil, extends OnOffensiveMultiplier<> {
        ON_RECOIL {
            CHECK(moveType == TYPE_ELECTRIC || moveType == TYPE_DARK);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 10, 1);
        }
        ON_OFFENSIVE_MULTIPLIER {
            if (moveType == TYPE_ELECTRIC || moveType == TYPE_DARK) MUL(1.35);
        }
    };

    class Lullaby : extends OnAccuracy<> {
        ON_ACCURACY {
            CHECK(move == MOVE_SING);
            *accuracy *= 1.5;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class CryoArchitect : extends OnEndTurn, extends OnDefender {
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

    class GlacialRage : extends OnAttacker {
        ON_ATTACKER {
            CHECK(moveType == TYPE_ICE)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
        }
    };

    class ImmovableObject : extends MagicGuard, extends Sturdy {};

    class FrenziedPhantom : extends ShadowTag, extends HyperAggressive {};

    class DNAScramble : extends FormChange, extends OnBeforeAttack<> {
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

    class MetallicJaws : extends Metallic, extends PrimalMaw {};

    class Calculative : extends Merged<Analytic, Neuroforce> {};

    class EmbodyAspect : extends RaiseStatOnEntry<STAT_SPEED> {};

    class EmbodyAspectHearthflame : extends IntrepidSword {};

    class EmbodyAspectCornerstone : extends DauntlessShield {};

    class EmbodyAspectWellspring : extends RaiseStatOnEntry<STAT_SPDEF> {};

    class RockhardShaft : extends BoostedSwarmLike<TYPE_ROCK> {};

    class HuntersMark : extends Deadeye, extends Ambush {};

    class Deviate : extends AteAbility<TYPE_DARK> {};

    class SunsBounty : extends Merged<Harvest, LeafGuard> {};

    class RiteOfSpring : extends Merged<SolarPower, Chlorophyll> {};

    class Headstrong : extends RaiseStatOnEntry<STAT_SPDEF> {};

    class Firefighter : extends TypeSlayer<TYPE_FIRE> {};

    class SepiaLens : extends SandGuard, extends TintedLens {};

    class SuperSniper : extends Sniper, extends OnPreemptAction {
        ON_OFFENSIVE_MULTIPLIER {
            Sniper::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
                MUL(0.5);
            }
        }
        ON_PREEMPT_ACTION { UseTurnAttackAsPursuit(DELEGATE_PREEMPT_ACTION); }
    };

    class WoodlandCurse : extends OnEither, extends SimpleEntryMove<MOVE_FORESTS_CURSE> {
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

    class Malodor : extends OnDefender {
        ON_DEFENDER {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK_NOT(gStatuses3[attacker] & STATUS3_GASTRO_ACID)

            gStatuses3[attacker] |= STATUS3_GASTRO_ACID;
            BattleScriptCall(BattleScript_StackAbilitySuppressedMessage);
            return TRUE;
        }
    };

    class Blur : extends OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
        ON_CHOOSE_DEFENSIVE_STAT {
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            return STAT_SPEED;
        }
    };

    class Elude : extends OnChooseDefensiveStat<ApplyOnTarget::TARGET> {
        ON_CHOOSE_DEFENSIVE_STAT {
            CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker))
            return STAT_SPEED;
        }
    };

    class DrakeOfRage : extends Rampage, extends TintedLens {};

    class MixedMartialArts : extends OnModifyMoveFlags {
        ON_MODIFY_MOVE_FLAGS {
            CHECK(flag == MOVE_FLAG_PUNCH || flag == MOVE_FLAG_KICK)
            CHECK(gBattleMoves[move].type == TYPE_NORMAL)
            return TRUE;
        }
    };

    class StrategicPause : extends Analytic, extends OnCrit<> {
        ON_CRIT {
            CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber)
            CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT)
            return 2;
        }
    };

    class Overrule : extends extends OnAfterTypeEffectiveness<> {
        ON_AFTER_TYPE_EFFECTIVENESS {
            if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        }
    };

    class MentalPollution : extends NotImplemented {};

    class MadnessEnhancement : extends NotImplemented {};

    class Tentalock : extends NotImplemented {};

    class SerpentBind : extends NotImplemented {};

    class SoulTap : extends OnEndTurn {
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

    class Scarecrow : extends Intimidate, extends BadLuck {};

    class OminousShroud : extends Phantom, extends ShadowShield {};

    class ChillingPresence : SimpleEntryMove<MOVE_ICY_WIND, 10> {};

    class Frostbind : extends PoisonPuppeteerLike<MOVE_EFFECT_FROSTBITE> {
        ON_REACTIVE override {
            return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) { return (int)CanGetFrostbite(battler); }, BattleScript_Frostbind);
        }
    };

    class TenderAffection : extends CuteCharm {
        ON_STAB override { return moveType == TYPE_FAIRY; }
    };

    class GlacialGhost : extends SlushRush, extends SnowCloak {};

    class WonderScale : extends ShedSkin, extends FortKnox {};

    class Overzealous : extends NotImplemented {};

    class StainlessSteel : extends AteAbility<TYPE_STEEL>, extends FortKnox {};

    class TemporalRupture : extends NotImplemented {};

    class GrassFlute : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(IsSoundMove(battler, move))
            CHECK_NOT(gVolatileStructs[target].fear)

            return AbilityStatusEffect(MOVE_EFFECT_FEAR);
        }
    };

    class Hemotoxin : extends PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
        ON_REACTIVE {
            return PoisonPuppeteerClone(
                ability,
                battler,
                [](int battler, int target) -> int { return !(gStatuses3[target] & STATUS3_GASTRO_ACID); },
                BattleScript_StackAbilitySuppressedMessage);
        }
    };

    class Harukaze : extends NotImplemented {};

    class ToxicSurge : extends OnEntry, extends AllowTerrainIfAirborne<TERRAIN_TOXIC> {
        ON_ENTRY {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
    };

    class PoisonQuills : extends Merged<PoisonPoint, RoughSkin> {};

    class DraconicMight : extends HalfDrake, extends AteAbility<TYPE_DRAGON> {};

    class AtlanticRuler : extends AquaticDweller, extends SwiftSwim {};

    class Biofilm : extends OnStat<> {
        ON_STAT {
            if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
        }
    };

    class Chokehold : extends NotImplemented {};

    class GuardianCoat : extends SandImmune, extends OnDefensiveMultiplier, extends PowderImmune, extends HailImmune {
        ON_DEFENSIVE_MULTIPLIER override {
            if (IS_MOVE_PHYSICAL(move)) MUL(.8);
        }
    };

    class NeutralizingFog : SimpleEntryMove<MOVE_DEFOG> {};

    class Festivities : extends OnModifyMoveFlags {
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

    class FeyFlight : extends FairyTale, extends GroundImmune {};

    class BestOffense : extends KeenEdge, extends MysticBlades {
        ON_CHOOSE_OFFENSIVE_STAT override { secondaryAtkStatToUse[STAT_SPDEF] += 20; }
    };

    class Impaler : extends MightyHorn, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(gBattleMoves[move].hornBased);
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class MagusBlades : extends DualWield, extends BestOffense {};

    class LightningBorn : extends AddsType<TYPE_ELECTRIC> {};

    class Superheavy : extends NotImplemented {};

    class WorldSerpent : extends GripPincer, extends LongReach {};

    class LuckyWings : extends GiantWings, extends SereneGrace {};

    class Komodo : extends HalfDrake, extends ToxicChain {};

    class Envenom : extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_POISON);
        }
    };

    class PurpleHaze : extends OnAttacker {
        ON_ATTACKER {
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
        }
    };

    class GnashingCannon : extends Merged<MegaLauncher, MindCrush> {};

    class HyperCleanse : extends OnDefensiveMultiplier, extends RemovesStatusOnImmunity {
        ON_DEFENSIVE_MULTIPLIER {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        }
        ON_STATUS_IMMUNE {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
    };

    class MoltenCoat : extends OnAttacker, extends AteAbility<TYPE_ROCK> {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(moveType == TYPE_ROCK)
            CHECK(CanBeBurned(target))
            CHECK(Random() % 2)

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
            return TRUE;
        }
    };

    class RoyalDecree : extends QueenlyMajesty, extends OnEntry {
        ON_ENTRY {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
            return UseEntryMove(battler, ability, MOVE_GLARE, 0);
        }
    };

    class Tag : extends OnPreemptAction {
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

    class Surprise : extends OnPreemptAction {
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

    class BreezyNeigh : extends AdrenalineRush {};

    class Dreamscape : extends Comatose, extends Dreamcatcher {
        ON_OFFENSIVE_MULTIPLIER {
            Dreamcatcher::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MUL(1.2);
        }
    };

    class HasteMakesWaste : extends Analytic, extends Stall {};

    class HungryMaws : extends JawsOfCarnage, extends StrongJaw {};

    class ThermalSlide : extends OnStat {
        ON_STAT {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
        }
    };

    class Thermomancy : extends Merged<Cryomancy, Pyromancy> {};

    class Chuckster : extends NotImplemented {};

    class HeatSink : extends LightningRodClone<TYPE_FIRE> {};

    class RelicStone : extends NotImplemented {};

    class Supercell : extends Merged<ElectricSurge, Drizzle> {};

    class LightningAspect : extends AbsorbStatUp<TYPE_ELECTRIC, STAT_HIGHEST_ATTACKING> {};

    class FireAspect : extends AbsorbHeal<TYPE_FIRE>, extends OnAttacker {
        ON_ATTACKER {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(moveType == TYPE_FIRE)
            CHECK(CanBeBurned(target))

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
            return TRUE;
        }
    };

    class BlisteringSun : extends Merged<DesolateLand, AirBlower> {};

    class AurorasGale : extends NorthWind, extends MajesticBird {};

    class WinterThrone : extends OnEntry, extends OnEndTurn {
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

    class IcePlumes : extends IceScales {};

    class PropellerTail : extends SwiftSwim {};

    class EnergyTap : extends OnAttacker {
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

    class MoltenCore : extends Furnace, extends AbsorbStatUp<TYPE_ROCK, STAT_SPEED>, extends AbsorbUp2, extends StealthRockImmune {
        ON_ENTRY {
            Furnace::onEntry(DELEGATE_ENTRY);

            CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
            gSideStatuses[GetBattlerSide(battler)] &= ~SIDE_STATUS_STEALTH_ROCK;
            return SwitchInAnnounce(B_MSG_SWITCHIN_MOLTEN_CORE);
        }
    };

    class Reverberate : extends Ability {
        ON_MODIFY_MOVE_FLAGS override {
            CHECK(flag == MOVE_FLAG_SOUND)
            CHECK(gBattleMoves[move].type == TYPE_NORMAL)
            return TRUE;
        }
    };

    class Taekkyeon : extends Ability {
        ON_MODIFY_MOVE_FLAGS override {
            CHECK(flag == MOVE_FLAG_DANCE)
            CHECK_NOT(IS_MOVE_STATUS(move))
            return TRUE;
        }
    };

    class SludgeSpit : extends Ability {
        ON_ATTACKER override {
            CHECK(gBattleMoves[move].power)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_VENOM_BOLT, 35);
        }
    };

    class SwampThing : extends Ability {
        ON_ENTRY override {
            CHECK_NOT(gSideTimers[GetOppositeSide(battler)].swampTimer)

            AbilityStatusEffectSafe(MOVE_EFFECT_SWAMP, battler, GetOppositeSide(battler));
            InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
            return TRUE;
        }
    };

    class FrostyPrescence : SimpleEntryMove<MOVE_MIST> {};

    class ChillingPellets : extends Ability {
        ON_DEFENDER override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_SPEAR, 13);
            return FALSE;
        }
    };

    class PaintShot : extends Ability {
        ON_ATTACKER override {
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

    class Stonecutter : extends Fossilized {
        ON_MOLD_BREAKER override {
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

    class Edgelord : extends Cutthroat {
        ON_BATTLER_FAINTS override {
            CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

            gStatuses4[battler] |= STATUS4_CUTTHROAT;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CUTTHROAT;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Warmonger : extends Ability {
        ON_OFFENSIVE_MULTIPLIER override {
            if (moveType == TYPE_ROCK || moveType == TYPE_STEEL || moveType == TYPE_FIGHTING) MUL(1.30);
        }
    };

    class LocustSwarm : extends StandardTransformation {};

    class Revelation : extends StandardTransformation {};

    class CurseOfFamine : extends Ability {
        ON_ENTRY override {
            CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

            BattleScriptPushCursorAndCallback(BattleScript_CurseOfFamine);
            return TRUE;
        }
    };

    class CrystallineArmor : extends Ability {
        bool breakable() override { return true; }
    };

    class SoulHarvest : extends Ability {
        ON_STAT override {
            if (statId != STAT_SPEED) *stat = *stat * (20 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 20;
        }
        bool breakable() override { return true; }
    };

    class ThickBlubber : extends Ability {
        ON_DEFENSIVE_MULTIPLIER override {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.25);
        }
        ON_STAT override {
            if (statId == STAT_SPEED) *stat *= .5;
        }
    };

    class Craving : extends Ability {
        bool breakable() override { return true; }
    };

    class RatKing : extends Ability {
        ON_STAT override {
            const BaseStats *baseStats = &gBaseStats[gBattleMons[battler].species];
            int bst =
                baseStats->baseHP + baseStats->baseAttack + baseStats->baseDefense + baseStats->baseSpAttack + baseStats->baseSpDefense + baseStats->baseSpeed;
            if (bst >= 400) return;
            *stat *= 1.5;
        }
        AbilityApplyOn onStatFor() override { APPLY_ON_ALLY; }
    };

    class CrispyCream : extends Ability {
        ON_DEFENDER override { return Random() % 2 ? FlameBody::onEither(DELEGATE_DEFENDER) : FreezingPoint::onEither(DELEGATE_DEFENDER); }
    };

    class DeepFried : extends Ability {
        ON_ENTRY override {
            CHECK_NOT(gSideTimers[GetOppositeSide(battler)].fireSeaTimer)

            AbilityStatusEffectSafe(MOVE_EFFECT_FIRE_SEA, battler, GetOppositeSide(battler));
            InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
            return TRUE;
        }
    };

    class FoodLovers : extends Hospitality, extends FriendGuard {};

    class LunarWrath : extends Ability {
        ON_ATTACKER override {
            CHECK(moveType == TYPE_GHOST)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_MOONGEIST_BEAM, 50);
        }
    };

    class Spyware : extends Ability {
        bool breakable() override { return true; }
    };

    class Virus : extends Ability {
        ON_ATTACKER override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(CanBePoisoned(battler, target, move))

            return AbilityStatusEffect(MOVE_EFFECT_POISON);
        }
    };

    class PowerLeak : extends Ability {
        ON_DEFENDER override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
            BattleScriptCall(BattleScript_SurgeActivatesRet);
            return TRUE;
        }
        TerrainType allowTerrainIfAirborne() override { return TERRAIN_ELECTRIC; }
    };

    class BackupPower : extends Ability {
        ON_REVIVE override {
            CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))
            return B_MSG_BACKUP_POWER;
        }
        bool persistent() override { return true; }
    };

    class SandFiend : extends SandGuard, extends SandForce {};

    class Moustache : extends Merged<TanglingHair, Stamina> {};

    class DepthExplorer : extends FieldExplorer, extends Illuminate {};

    class DuneVeil : extends SandGuard, extends SelfSufficient {};

    class StrongFoundation : extends Ability {
        ON_DEFENSIVE_MULTIPLIER override {
            if (moveType == TYPE_WATER || moveType == TYPE_GROUND) RESISTANCE(.50);
        }
    };

    class FogMachine : extends Ability {
        ON_DEFENDER override {
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

    class DropBlocks : extends Ability {
        ON_DEFENDER override {
            CHECK(DidMoveHit())
            CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

            BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
            return TRUE;
        }
    };

    class LaserDrill : extends Ability {
        bool breakable() override { return true; }
    };

    class LightSaber : extends Ability {
        bool breakable() override { return true; }
    };

    class LooseThorns : extends Ability {
        bool breakable() override { return true; }
    };

    class TurfWar : extends Ability {
        bool breakable() override { return true; }
    };

    class Greedy : extends Ability {
        bool breakable() override { return true; }
    };

    class MusicalNotes : extends Ability {
        bool breakable() override { return true; }
    };

    class Strikeout : extends Ability {
        bool breakable() override { return true; }
    };

    class HomeRun : extends Ability {
        bool breakable() override { return true; }
    };

    class Bruiser : extends AddsType<TYPE_FIGHTING> {};

    class LetsDance : SimpleEntryMove<MOVE_TEETER_DANCE> {};

    class MyceliumMight : extends Ability {
        ON_MOLD_BREAKER override { return IS_MOVE_STATUS(move); }
    };

    class DeadlyPrecision : extends Ability {
        ON_MOLD_BREAKER override {
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

    typedef struct AbilityKVPair {
        u16 key;
        const *Ability ability;
    } AbilityKVPair;

    template <AbilityEnum Key, typename T>
    consteval AbilityKVPair pair() {
        return (AbilityKVPair) {
            key = Key;
            ability = (T[])(T());
        };
    }

    constexpr AbilityKVPair sAbilities[] = {
        pair<ABILITY_NONE, None>(),
        pair<ABILITY_STENCH, Stench>(),
        pair<ABILITY_DRIZZLE, Drizzle>(),
        pair<ABILITY_SPEED_BOOST, SpeedBoost>(),
        pair<ABILITY_BATTLE_ARMOR, BattleArmor>(),
        pair<ABILITY_STURDY, Sturdy>(),
        pair<ABILITY_DAMP, Damp>(),
        pair<ABILITY_LIMBER, Limber>(),
        pair<ABILITY_SAND_VEIL, SandVeil>(),
        pair<ABILITY_STATIC, Static>(),
        pair<ABILITY_VOLT_ABSORB, VoltAbsorb>(),
        pair<ABILITY_WATER_ABSORB, WaterAbsorb>(),
        pair<ABILITY_OBLIVIOUS, Oblivious>(),
        pair<ABILITY_CLOUD_NINE, CloudNine>(),
        pair<ABILITY_COMPOUND_EYES, CompoundEyes>(),
        pair<ABILITY_INSOMNIA, Insomnia>(),
        pair<ABILITY_COLOR_CHANGE, ColorChange>(),
        pair<ABILITY_IMMUNITY, Immunity>(),
        pair<ABILITY_FLASH_FIRE, FlashFire>(),
        pair<ABILITY_SHIELD_DUST, ShieldDust>(),
        pair<ABILITY_OWN_TEMPO, OwnTempo>(),
        pair<ABILITY_SUCTION_CUPS, SuctionCups>(),
        pair<ABILITY_INTIMIDATE, Intimidate>(),
        pair<ABILITY_SHADOW_TAG, ShadowTag>(),
        pair<ABILITY_ROUGH_SKIN, RoughSkin>(),
        pair<ABILITY_WONDER_GUARD, WonderGuard>(),
        pair<ABILITY_LEVITATE, Levitate>(),
        pair<ABILITY_EFFECT_SPORE, EffectSpore>(),
        pair<ABILITY_CLEAR_BODY, ClearBody>(),
        pair<ABILITY_NATURAL_CURE, NaturalCure>(),
        pair<ABILITY_LIGHTNING_ROD, LightningRod>(),
        pair<ABILITY_SERENE_GRACE, SereneGrace>(),
        pair<ABILITY_SWIFT_SWIM, SwiftSwim>(),
        pair<ABILITY_CHLOROPHYLL, Chlorophyll>(),
        pair<ABILITY_ILLUMINATE, Illuminate>(),
        pair<ABILITY_TRACE, Trace>(),
        pair<ABILITY_HUGE_POWER, HugePower>(),
        pair<ABILITY_POISON_POINT, PoisonPoint>(),
        pair<ABILITY_INNER_FOCUS, InnerFocus>(),
        pair<ABILITY_MAGMA_ARMOR, MagmaArmor>(),
        pair<ABILITY_WATER_VEIL, WaterVeil>(),
        pair<ABILITY_MAGNET_PULL, MagnetPull>(),
        pair<ABILITY_SOUNDPROOF, Soundproof>(),
        pair<ABILITY_RAIN_DISH, RainDish>(),
        pair<ABILITY_SAND_STREAM, SandStream>(),
        pair<ABILITY_PRESSURE, Pressure>(),
        pair<ABILITY_THICK_FAT, ThickFat>(),
        pair<ABILITY_FLAME_BODY, FlameBody>(),
        pair<ABILITY_KEEN_EYE, KeenEye>(),
        pair<ABILITY_HYPER_CUTTER, HyperCutter>(),
        pair<ABILITY_PICKUP, Pickup>(),
        pair<ABILITY_TRUANT, Truant>(),
        pair<ABILITY_HUSTLE, Hustle>(),
        pair<ABILITY_CUTE_CHARM, CuteCharm>(),
        pair<ABILITY_PLUS, Plus>(),
        pair<ABILITY_MINUS, Minus>(),
        pair<ABILITY_FORECAST, Forecast>(),
        pair<ABILITY_STICKY_HOLD, StickyHold>(),
        pair<ABILITY_SHED_SKIN, ShedSkin>(),
        pair<ABILITY_GUTS, Guts>(),
        pair<ABILITY_MARVEL_SCALE, MarvelScale>(),
        pair<ABILITY_OVERGROW, Overgrow>(),
        pair<ABILITY_BLAZE, Blaze>(),
        pair<ABILITY_TORRENT, Torrent>(),
        pair<ABILITY_SWARM, Swarm>(),
        pair<ABILITY_ROCK_HEAD, RockHead>(),
        pair<ABILITY_DROUGHT, Drought>(),
        pair<ABILITY_ARENA_TRAP, ArenaTrap>(),
        pair<ABILITY_VITAL_SPIRIT, VitalSpirit>(),
        pair<ABILITY_WHITE_SMOKE, WhiteSmoke>(),
        pair<ABILITY_PURE_POWER, PurePower>(),
        pair<ABILITY_SHELL_ARMOR, ShellArmor>(),
        pair<ABILITY_AIR_LOCK, AirLock>(),
        pair<ABILITY_TANGLED_FEET, TangledFeet>(),
        pair<ABILITY_MOTOR_DRIVE, MotorDrive>(),
        pair<ABILITY_RIVALRY, Rivalry>(),
        pair<ABILITY_SNOW_CLOAK, SnowCloak>(),
        pair<ABILITY_ANGER_POINT, AngerPoint>(),
        pair<ABILITY_UNBURDEN, Unburden>(),
        pair<ABILITY_HEATPROOF, Heatproof>(),
        pair<ABILITY_DRY_SKIN, DrySkin>(),
        pair<ABILITY_DOWNLOAD, Download>(),
        pair<ABILITY_IRON_FIST, IronFist>(),
        pair<ABILITY_ADAPTABILITY, Adaptability>(),
        pair<ABILITY_SKILL_LINK, SkillLink>(),
        pair<ABILITY_HYDRATION, Hydration>(),
        pair<ABILITY_SOLAR_POWER, SolarPower>(),
        pair<ABILITY_QUICK_FEET, QuickFeet>(),
        pair<ABILITY_NORMALIZE, Normalize>(),
        pair<ABILITY_SNIPER, Sniper>(),
        pair<ABILITY_MAGIC_GUARD, MagicGuard>(),
        pair<ABILITY_NO_GUARD, NoGuard>(),
        pair<ABILITY_STALL, Stall>(),
        pair<ABILITY_TECHNICIAN, Technician>(),
        pair<ABILITY_LEAF_GUARD, LeafGuard>(),
        pair<ABILITY_MOLD_BREAKER, MoldBreaker>(),
        pair<ABILITY_SUPER_LUCK, SuperLuck>(),
        pair<ABILITY_AFTERMATH, Aftermath>(),
        pair<ABILITY_ANTICIPATION, Anticipation>(),
        pair<ABILITY_FOREWARN, Forewarn>(),
        pair<ABILITY_UNAWARE, Unaware>(),
        pair<ABILITY_TINTED_LENS, TintedLens>(),
        pair<ABILITY_FILTER, Filter>(),
        pair<ABILITY_SLOW_START, SlowStart>(),
        pair<ABILITY_SCRAPPY, Scrappy>(),
        pair<ABILITY_STORM_DRAIN, StormDrain>(),
        pair<ABILITY_ICE_BODY, IceBody>(),
        pair<ABILITY_SOLID_ROCK, SolidRock>(),
        pair<ABILITY_SNOW_WARNING, SnowWarning>(),
        pair<ABILITY_HONEY_GATHER, HoneyGather>(),
        pair<ABILITY_FRISK, Frisk>(),
        pair<ABILITY_RECKLESS, Reckless>(),
        pair<ABILITY_MULTITYPE, Multitype>(),
        pair<ABILITY_FLOWER_GIFT, FlowerGift>(),
        pair<ABILITY_BAD_DREAMS, BadDreams>(),
        pair<ABILITY_SHEER_FORCE, SheerForce>(),
        pair<ABILITY_CONTRARY, Contrary>(),
        pair<ABILITY_UNNERVE, Unnerve>(),
        pair<ABILITY_DEFEATIST, Defeatist>(),
        pair<ABILITY_CURSED_BODY, CursedBody>(),
        pair<ABILITY_HEALER, Healer>(),
        pair<ABILITY_FRIEND_GUARD, FriendGuard>(),
        pair<ABILITY_WEAK_ARMOR, WeakArmor>(),
        pair<ABILITY_LIGHT_METAL, LightMetal>(),
        pair<ABILITY_MULTISCALE, Multiscale>(),
        pair<ABILITY_TOXIC_BOOST, ToxicBoost>(),
        pair<ABILITY_FLARE_BOOST, FlareBoost>(),
        pair<ABILITY_HARVEST, Harvest>(),
        pair<ABILITY_TELEPATHY, Telepathy>(),
        pair<ABILITY_MOODY, Moody>(),
        pair<ABILITY_OVERCOAT, Overcoat>(),
        pair<ABILITY_POISON_TOUCH, PoisonTouch>(),
        pair<ABILITY_REGENERATOR, Regenerator>(),
        pair<ABILITY_BIG_PECKS, BigPecks>(),
        pair<ABILITY_SAND_RUSH, SandRush>(),
        pair<ABILITY_WONDER_SKIN, WonderSkin>(),
        pair<ABILITY_ANALYTIC, Analytic>(),
        pair<ABILITY_ILLUSION, Illusion>(),
        pair<ABILITY_IMPOSTER, Imposter>(),
        pair<ABILITY_INFILTRATOR, Infiltrator>(),
        pair<ABILITY_MUMMY, Mummy>(),
        pair<ABILITY_MOXIE, Moxie>(),
        pair<ABILITY_JUSTIFIED, Justified>(),
        pair<ABILITY_RATTLED, Rattled>(),
        pair<ABILITY_MAGIC_BOUNCE, MagicBounce>(),
        pair<ABILITY_SAP_SIPPER, SapSipper>(),
        pair<ABILITY_PRANKSTER, Prankster>(),
        pair<ABILITY_SAND_FORCE, SandForce>(),
        pair<ABILITY_IRON_BARBS, IronBarbs>(),
        pair<ABILITY_ZEN_MODE, ZenMode>(),
        pair<ABILITY_VICTORY_STAR, VictoryStar>(),
        pair<ABILITY_TURBOBLAZE, Turboblaze>(),
        pair<ABILITY_TERAVOLT, Teravolt>(),
        pair<ABILITY_AROMA_VEIL, AromaVeil>(),
        pair<ABILITY_FLOWER_VEIL, FlowerVeil>(),
        pair<ABILITY_CHEEK_POUCH, CheekPouch>(),
        pair<ABILITY_PROTEAN, Protean>(),
        pair<ABILITY_FUR_COAT, FurCoat>(),
        pair<ABILITY_BULLETPROOF, Bulletproof>(),
        pair<ABILITY_STRONG_JAW, StrongJaw>(),
        pair<ABILITY_REFRIGERATE, Refrigerate>(),
        pair<ABILITY_SWEET_VEIL, SweetVeil>(),
        pair<ABILITY_STANCE_CHANGE, StanceChange>(),
        pair<ABILITY_GALE_WINGS, GaleWings>(),
        pair<ABILITY_MEGA_LAUNCHER, MegaLauncher>(),
        pair<ABILITY_GRASS_PELT, GrassPelt>(),
        pair<ABILITY_TOUGH_CLAWS, ToughClaws>(),
        pair<ABILITY_PIXILATE, Pixilate>(),
        pair<ABILITY_GOOEY, Gooey>(),
        pair<ABILITY_AERILATE, Aerilate>(),
        pair<ABILITY_PARENTAL_BOND, ParentalBond>(),
        pair<ABILITY_DARK_AURA, DarkAura>(),
        pair<ABILITY_FAIRY_AURA, FairyAura>(),
        pair<ABILITY_AURA_BREAK, AuraBreak>(),
        pair<ABILITY_PRIMORDIAL_SEA, PrimordialSea>(),
        pair<ABILITY_DESOLATE_LAND, DesolateLand>(),
        pair<ABILITY_DELTA_STREAM, DeltaStream>(),
        pair<ABILITY_STAMINA, Stamina>(),
        pair<ABILITY_WIMP_OUT, WimpOut>(),
        pair<ABILITY_EMERGENCY_EXIT, EmergencyExit>(),
        pair<ABILITY_WATER_COMPACTION, WaterCompaction>(),
        pair<ABILITY_MERCILESS, Merciless>(),
        pair<ABILITY_SHIELDS_DOWN, ShieldsDown>(),
        pair<ABILITY_STAKEOUT, Stakeout>(),
        pair<ABILITY_WATER_BUBBLE, WaterBubble>(),
        pair<ABILITY_STEELWORKER, Steelworker>(),
        pair<ABILITY_BERSERK, Berserk>(),
        pair<ABILITY_SLUSH_RUSH, SlushRush>(),
        pair<ABILITY_LONG_REACH, LongReach>(),
        pair<ABILITY_LIQUID_VOICE, LiquidVoice>(),
        pair<ABILITY_TRIAGE, Triage>(),
        pair<ABILITY_GALVANIZE, Galvanize>(),
        pair<ABILITY_SURGE_SURFER, SurgeSurfer>(),
        pair<ABILITY_SCHOOLING, Schooling>(),
        pair<ABILITY_DISGUISE, Disguise>(),
        pair<ABILITY_BATTLE_BOND, BattleBond>(),
        pair<ABILITY_POWER_CONSTRUCT, PowerConstruct>(),
        pair<ABILITY_CORROSION, Corrosion>(),
        pair<ABILITY_COMATOSE, Comatose>(),
        pair<ABILITY_QUEENLY_MAJESTY, QueenlyMajesty>(),
        pair<ABILITY_INNARDS_OUT, InnardsOut>(),
        pair<ABILITY_DANCER, Dancer>(),
        pair<ABILITY_BATTERY, Battery>(),
        pair<ABILITY_FLUFFY, Fluffy>(),
        pair<ABILITY_DAZZLING, Dazzling>(),
        pair<ABILITY_SOUL_HEART, SoulHeart>(),
        pair<ABILITY_TANGLING_HAIR, TanglingHair>(),
        pair<ABILITY_RECEIVER, Receiver>(),
        pair<ABILITY_POWER_OF_ALCHEMY, PowerOfAlchemy>(),
        pair<ABILITY_BEAST_BOOST, BeastBoost>(),
        pair<ABILITY_RKS_SYSTEM, RksSystem>(),
        pair<ABILITY_ELECTRIC_SURGE, ElectricSurge>(),
        pair<ABILITY_PSYCHIC_SURGE, PsychicSurge>(),
        pair<ABILITY_MISTY_SURGE, MistySurge>(),
        pair<ABILITY_GRASSY_SURGE, GrassySurge>(),
        pair<ABILITY_SHADOW_SHIELD, ShadowShield>(),
        pair<ABILITY_PRISM_ARMOR, PrismArmor>(),
        pair<ABILITY_NEUROFORCE, Neuroforce>(),
        pair<ABILITY_INTREPID_SWORD, IntrepidSword>(),
        pair<ABILITY_DAUNTLESS_SHIELD, DauntlessShield>(),
        pair<ABILITY_LIBERO, Libero>(),
        pair<ABILITY_COTTON_DOWN, CottonDown>(),
        pair<ABILITY_PROPELLER_TAIL, PropellerTail>(),
        pair<ABILITY_MIRROR_ARMOR, MirrorArmor>(),
        pair<ABILITY_GULP_MISSILE, GulpMissile>(),
        pair<ABILITY_STEAM_ENGINE, SteamEngine>(),
        pair<ABILITY_PUNK_ROCK, PunkRock>(),
        pair<ABILITY_SAND_SPIT, SandSpit>(),
        pair<ABILITY_ICE_SCALES, IceScales>(),
        pair<ABILITY_ICE_FACE, IceFace>(),
        pair<ABILITY_POWER_SPOT, PowerSpot>(),
        pair<ABILITY_MIMICRY, Mimicry>(),
        pair<ABILITY_SCREEN_CLEANER, ScreenCleaner>(),
        pair<ABILITY_STEELY_SPIRIT, SteelySpirit>(),
        pair<ABILITY_PERISH_BODY, PerishBody>(),
        pair<ABILITY_WANDERING_SPIRIT, WanderingSpirit>(),
        pair<ABILITY_GORILLA_TACTICS, GorillaTactics>(),
        pair<ABILITY_NEUTRALIZING_GAS, NeutralizingGas>(),
        pair<ABILITY_PASTEL_VEIL, PastelVeil>(),
        pair<ABILITY_HUNGER_SWITCH, HungerSwitch>(),
        pair<ABILITY_CURIOUS_MEDICINE, CuriousMedicine>(),
        pair<ABILITY_TRANSISTOR, Transistor>(),
        pair<ABILITY_DRAGONS_MAW, DragonsMaw>(),
        pair<ABILITY_CHILLING_NEIGH, ChillingNeigh>(),
        pair<ABILITY_GRIM_NEIGH, GrimNeigh>(),
        pair<ABILITY_AS_ONE_ICE_RIDER, AsOneIceRider>(),
        pair<ABILITY_AS_ONE_SHADOW_RIDER, AsOneShadowRider>(),
        pair<ABILITY_CHLOROPLAST, Chloroplast>(),
        pair<ABILITY_WHITEOUT, Whiteout>(),
        pair<ABILITY_PYROMANCY, Pyromancy>(),
        pair<ABILITY_KEEN_EDGE, KeenEdge>(),
        pair<ABILITY_PRISM_SCALES, PrismScales>(),
        pair<ABILITY_POWER_FISTS, PowerFists>(),
        pair<ABILITY_SAND_SONG, SandSong>(),
        pair<ABILITY_RAMPAGE, Rampage>(),
        pair<ABILITY_VENGEANCE, Vengeance>(),
        pair<ABILITY_BLITZ_BOXER, BlitzBoxer>(),
        pair<ABILITY_ANTARCTIC_BIRD, AntarcticBird>(),
        pair<ABILITY_IMMOLATE, Immolate>(),
        pair<ABILITY_CRYSTALLIZE, Crystallize>(),
        pair<ABILITY_ELECTROCYTES, Electrocytes>(),
        pair<ABILITY_AERODYNAMICS, Aerodynamics>(),
        pair<ABILITY_CHRISTMAS_SPIRIT, ChristmasSpirit>(),
        pair<ABILITY_EXPLOIT_WEAKNESS, ExploitWeakness>(),
        pair<ABILITY_GROUND_SHOCK, GroundShock>(),
        pair<ABILITY_ANCIENT_IDOL, AncientIdol>(),
        pair<ABILITY_MYSTIC_POWER, MysticPower>(),
        pair<ABILITY_PERFECTIONIST, Perfectionist>(),
        pair<ABILITY_GROWING_TOOTH, GrowingTooth>(),
        pair<ABILITY_INFLATABLE, Inflatable>(),
        pair<ABILITY_AURORA_BOREALIS, AuroraBorealis>(),
        pair<ABILITY_AVENGER, Avenger>(),
        pair<ABILITY_LETS_ROLL, LetsRoll>(),
        pair<ABILITY_LOUD_BANG, LoudBang>(),
        pair<ABILITY_LEAD_COAT, LeadCoat>(),
        pair<ABILITY_AMPHIBIOUS, Amphibious>(),
        pair<ABILITY_GROUNDED, Grounded>(),
        pair<ABILITY_EARTHBOUND, Earthbound>(),
        pair<ABILITY_FIGHT_SPIRIT, FightingSpirit>(),
        pair<ABILITY_FELINE_PROWESS, FelineProwess>(),
        pair<ABILITY_COIL_UP, CoilUp>(),
        pair<ABILITY_FOSSILIZED, Fossilized>(),
        pair<ABILITY_MAGICAL_DUST, MagicalDust>(),
        pair<ABILITY_DREAMCATCHER, Dreamcatcher>(),
        pair<ABILITY_NOCTURNAL, Nocturnal>(),
        pair<ABILITY_SELF_SUFFICIENT, SelfSufficient>(),
        pair<ABILITY_TECTONIZE, Tectonize>(),
        pair<ABILITY_ICE_AGE, IceAge>(),
        pair<ABILITY_HALF_DRAKE, HalfDrake>(),
        pair<ABILITY_LIQUIFIED, Liquified>(),
        pair<ABILITY_DRAGONFLY, Dragonfly>(),
        pair<ABILITY_DRAGONSLAYER, Dragonslayer>(),
        pair<ABILITY_MOUNTAINEER, Mountaineer>(),
        pair<ABILITY_HYDRATE, Hydrate>(),
        pair<ABILITY_METALLIC, Metallic>(),
        pair<ABILITY_PERMAFROST, Permafrost>(),
        pair<ABILITY_PRIMAL_ARMOR, PrimalArmor>(),
        pair<ABILITY_RAGING_BOXER, RagingBoxer>(),
        pair<ABILITY_AIR_BLOWER, AirBlower>(),
        pair<ABILITY_JUGGERNAUT, Juggernaut>(),
        pair<ABILITY_SHORT_CIRCUIT, ShortCircuit>(),
        pair<ABILITY_MAJESTIC_BIRD, MajesticBird>(),
        pair<ABILITY_PHANTOM, Phantom>(),
        pair<ABILITY_INTOXICATE, Intoxicate>(),
        pair<ABILITY_IMPENETRABLE, Impenetrable>(),
        pair<ABILITY_HYPNOTIST, Hypnotist>(),
        pair<ABILITY_OVERWHELM, Overwhelm>(),
        pair<ABILITY_SCARE, Scare>(),
        pair<ABILITY_MAJESTIC_MOTH, MajesticMoth>(),
        pair<ABILITY_SOUL_EATER, SoulEater>(),
        pair<ABILITY_SOUL_LINKER, SoulLinker>(),
        pair<ABILITY_SWEET_DREAMS, SweetDreams>(),
        pair<ABILITY_BAD_LUCK, BadLuck>(),
        pair<ABILITY_HAUNTED_SPIRIT, HauntedSpirit>(),
        pair<ABILITY_ELECTRIC_BURST, ElectricBurst>(),
        pair<ABILITY_RAW_WOOD, RawWood>(),
        pair<ABILITY_SOLENOGLYPHS, Solenoglyphs>(),
        pair<ABILITY_SPIDER_LAIR, SpiderLair>(),
        pair<ABILITY_FATAL_PRECISION, FatalPrecision>(),
        pair<ABILITY_FORT_KNOX, FortKnox>(),
        pair<ABILITY_SEAWEED, Seaweed>(),
        pair<ABILITY_PSYCHIC_MIND, PsychicMind>(),
        pair<ABILITY_POISON_ABSORB, PoisonAbsorb>(),
        pair<ABILITY_SCAVENGER, Scavenger>(),
        pair<ABILITY_TWISTED_DIMENSION, TwistedDimension>(),
        pair<ABILITY_MULTI_HEADED, MultiHeaded>(),
        pair<ABILITY_NORTH_WIND, NorthWind>(),
        pair<ABILITY_OVERCHARGE, Overcharge>(),
        pair<ABILITY_VIOLENT_RUSH, ViolentRush>(),
        pair<ABILITY_FLAMING_SOUL, FlamingSoul>(),
        pair<ABILITY_SAGE_POWER, SagePower>(),
        pair<ABILITY_BONE_ZONE, BoneZone>(),
        pair<ABILITY_WEATHER_CONTROL, WeatherControl>(),
        pair<ABILITY_SPEED_FORCE, SpeedForce>(),
        pair<ABILITY_SEA_GUARDIAN, SeaGuardian>(),
        pair<ABILITY_MOLTEN_DOWN, MoltenDown>(),
        pair<ABILITY_HYPER_AGGRESSIVE, HyperAggressive>(),
        pair<ABILITY_FLOCK, Flock>(),
        pair<ABILITY_FIELD_EXPLORER, FieldExplorer>(),
        pair<ABILITY_STRIKER, Striker>(),
        pair<ABILITY_FROZEN_SOUL, FrozenSoul>(),
        pair<ABILITY_PREDATOR, Predator>(),
        pair<ABILITY_LOOTER, Looter>(),
        pair<ABILITY_LUNAR_ECLIPSE, LunarEclipse>(),
        pair<ABILITY_SOLAR_FLARE, SolarFlare>(),
        pair<ABILITY_POWER_CORE, PowerCore>(),
        pair<ABILITY_SIGHTING_SYSTEM, SightingSystem>(),
        pair<ABILITY_BAD_COMPANY, BadCompany>(),
        pair<ABILITY_OPPORTUNIST, Opportunist>(),
        pair<ABILITY_GIANT_WINGS, GiantWings>(),
        pair<ABILITY_MOMENTUM, Momentum>(),
        pair<ABILITY_GRIP_PINCER, GripPincer>(),
        pair<ABILITY_BIG_LEAVES, BigLeaves>(),
        pair<ABILITY_PRECISE_FIST, PreciseFist>(),
        pair<ABILITY_DEADEYE, Deadeye>(),
        pair<ABILITY_ARTILLERY, Artillery>(),
        pair<ABILITY_AMPLIFIER, Amplifier>(),
        pair<ABILITY_ICE_DEW, IceDew>(),
        pair<ABILITY_SUN_WORSHIP, SunWorship>(),
        pair<ABILITY_POLLINATE, Pollinate>(),
        pair<ABILITY_VOLCANO_RAGE, VolcanoRage>(),
        pair<ABILITY_COLD_REBOUND, ColdRebound>(),
        pair<ABILITY_LOW_BLOW, LowBlow>(),
        pair<ABILITY_SPECTRAL_SHROUD, SpectralShroud>(),
        pair<ABILITY_DISCIPLINE, Discipline>(),
        pair<ABILITY_THUNDERCALL, Thundercall>(),
        pair<ABILITY_MARINE_APEX, MarineApex>(),
        pair<ABILITY_MIGHTY_HORN, MightyHorn>(),
        pair<ABILITY_HARDENED_SHEATH, HardenedSheath>(),
        pair<ABILITY_ARCTIC_FUR, ArcticFur>(),
        pair<ABILITY_SPECTRALIZE, Spectralize>(),
        pair<ABILITY_LETHARGY, Lethargy>(),
        pair<ABILITY_IRON_BARRAGE, IronBarrage>(),
        pair<ABILITY_STEEL_BARREL, SteelBarrel>(),
        pair<ABILITY_PYRO_SHELLS, PyroShells>(),
        pair<ABILITY_FUNGAL_INFECTION, FungalInfection>(),
        pair<ABILITY_PARRY, Parry>(),
        pair<ABILITY_SCRAPYARD, Scrapyard>(),
        pair<ABILITY_LOOSE_QUILLS, LooseQuills>(),
        pair<ABILITY_TOXIC_DEBRIS, ToxicDebris>(),
        pair<ABILITY_ROUNDHOUSE, Roundhouse>(),
        pair<ABILITY_MINERALIZE, Mineralize>(),
        pair<ABILITY_LOOSE_ROCKS, LooseRocks>(),
        pair<ABILITY_SPINNING_TOP, SpinningTop>(),
        pair<ABILITY_RETRIBUTION_BLOW, RetributionBlow>(),
        pair<ABILITY_FEARMONGER, Fearmonger>(),
        pair<ABILITY_TOXIC_SPILL, ToxicSpill>(),
        pair<ABILITY_DESERT_CLOAK, DesertCloak>(),
        pair<ABILITY_DRACONIZE, Draconize>(),
        pair<ABILITY_PRETTY_PRINCESS, PrettyPrincess>(),
        pair<ABILITY_SELF_REPAIR, SelfRepair>(),
        pair<ABILITY_ATOMIC_BURST, AtomicBurst>(),
        pair<ABILITY_HELLBLAZE, Hellblaze>(),
        pair<ABILITY_RIPTIDE, Riptide>(),
        pair<ABILITY_FOREST_RAGE, ForestRage>(),
        pair<ABILITY_PRIMAL_MAW, PrimalMaw>(),
        pair<ABILITY_SWEEPING_EDGE, SweepingEdge>(),
        pair<ABILITY_GIFTED_MIND, GiftedMind>(),
        pair<ABILITY_HYDRO_CIRCUIT, HydroCircuit>(),
        pair<ABILITY_EQUINOX, Equinox>(),
        pair<ABILITY_ABSORBANT, Absorbant>(),
        pair<ABILITY_CLUELESS, Clueless>(),
        pair<ABILITY_CHEATING_DEATH, CheatingDeath>(),
        pair<ABILITY_CHEAP_TACTICS, CheapTactics>(),
        pair<ABILITY_COWARD, Coward>(),
        pair<ABILITY_VOLT_RUSH, VoltRush>(),
        pair<ABILITY_DUNE_TERROR, DuneTerror>(),
        pair<ABILITY_INFERNAL_RAGE, InfernalRage>(),
        pair<ABILITY_DUAL_WIELD, DualWield>(),
        pair<ABILITY_ELEMENTAL_CHARGE, ElementalCharge>(),
        pair<ABILITY_AMBUSH, Ambush>(),
        pair<ABILITY_ATLAS, Atlas>(),
        pair<ABILITY_RADIANCE, Radiance>(),
        pair<ABILITY_JAWS_OF_CARNAGE, JawsOfCarnage>(),
        pair<ABILITY_ANGELS_WRATH, AngelsWrath>(),
        pair<ABILITY_PRISMATIC_FUR, PrismaticFur>(),
        pair<ABILITY_SHOCKING_JAWS, ShockingJaws>(),
        pair<ABILITY_FAE_HUNTER, FaeHunter>(),
        pair<ABILITY_GRAVITY_WELL, GravityWell>(),
        pair<ABILITY_EVAPORATE, Evaporate>(),
        pair<ABILITY_LUMBERJACK, Lumberjack>(),
        pair<ABILITY_WELL_BAKED_BODY, WellBakedBody>(),
        pair<ABILITY_FURNACE, Furnace>(),
        pair<ABILITY_ELECTROMORPHOSIS, Electromorphosis>(),
        pair<ABILITY_ROCKY_PAYLOAD, RockyPayload>(),
        pair<ABILITY_EARTH_EATER, EarthEater>(),
        pair<ABILITY_LINGERING_AROMA, LingeringAroma>(),
        pair<ABILITY_FAIRY_TALE, FairyTale>(),
        pair<ABILITY_RAGING_MOTH, RagingMoth>(),
        pair<ABILITY_ADRENALINE_RUSH, AdrenalineRush>(),
        pair<ABILITY_ARCHMAGE, Archmage>(),
        pair<ABILITY_CRYOMANCY, Cryomancy>(),
        pair<ABILITY_PHANTOM_PAIN, PhantomPain>(),
        pair<ABILITY_PURGATORY, Purgatory>(),
        pair<ABILITY_EMANATE, Emanate>(),
        pair<ABILITY_KUNOICHI_BLADE, KunoichiBlade>(),
        pair<ABILITY_MONKEY_BUSINESS, MonkeyBusiness>(),
        pair<ABILITY_COMBAT_SPECIALIST, CombatSpecialist>(),
        pair<ABILITY_JUNGLES_GUARD, JunglesGuard>(),
        pair<ABILITY_HUNTERS_HORN, HuntersHorn>(),
        pair<ABILITY_PIXIE_POWER, PixiePower>(),
        pair<ABILITY_PLASMA_LAMP, PlasmaLamp>(),
        pair<ABILITY_MAGMA_EATER, MagmaEater>(),
        pair<ABILITY_SUPER_HOT_GOO, SuperHotGoo>(),
        pair<ABILITY_NIKA, Nika>(),
        pair<ABILITY_ARCHER, Archer>(),
        pair<ABILITY_SUPER_SLAMMER, SuperSlammer>(),
        pair<ABILITY_INVERSE_ROOM, InverseRoom>(),
        pair<ABILITY_FROST_BURN, FrostBurn>(),
        pair<ABILITY_ITCHY_DEFENSE, ItchyDefense>(),
        pair<ABILITY_GENERATOR, Generator>(),
        pair<ABILITY_MOON_SPIRIT, MoonSpirit>(),
        pair<ABILITY_DUST_CLOUD, DustCloud>(),
        pair<ABILITY_BERSERKER_RAGE, BerserkerRage>(),
        pair<ABILITY_TRICKSTER, Trickster>(),
        pair<ABILITY_SAND_GUARD, SandGuard>(),
        pair<ABILITY_NATURAL_RECOVERY, NaturalRecovery>(),
        pair<ABILITY_WIND_RIDER, WindRider>(),
        pair<ABILITY_SOOTHING_AROMA, SoothingAroma>(),
        pair<ABILITY_PRIM_AND_PROPER, PrimAndProper>(),
        pair<ABILITY_SUPER_STRAIN, SuperStrain>(),
        pair<ABILITY_TIPPING_POINT, TippingPoint>(),
        pair<ABILITY_ENLIGHTENED, Enlightened>(),
        pair<ABILITY_PEACEFUL_SLUMBER, PeacefulSlumber>(),
        pair<ABILITY_AFTERSHOCK, Aftershock>(),
        pair<ABILITY_FREEZING_POINT, FreezingPoint>(),
        pair<ABILITY_CRYO_PROFICIENCY, CryoProficiency>(),
        pair<ABILITY_ARCANE_FORCE, ArcaneForce>(),
        pair<ABILITY_DOOMBRINGER, Doombringer>(),
        pair<ABILITY_WISHMAKER, Wishmaker>(),
        pair<ABILITY_YUKI_ONNA, YukiOnna>(),
        pair<ABILITY_SUPPRESS, Suppress>(),
        pair<ABILITY_REFRIGERATOR, Refrigerator>(),
        pair<ABILITY_HEAVEN_ASUNDER, HeavenAsunder>(),
        pair<ABILITY_PURIFYING_WATERS, PurifyingWaters>(),
        pair<ABILITY_SEABORNE, Seaborne>(),
        pair<ABILITY_HIGH_TIDE, HighTide>(),
        pair<ABILITY_CHANGE_OF_HEART, ChangeOfHeart>(),
        pair<ABILITY_MYSTIC_BLADES, MysticBlades>(),
        pair<ABILITY_DETERMINATION, Determination>(),
        pair<ABILITY_FERTILIZE, Fertilize>(),
        pair<ABILITY_PURE_LOVE, PureLove>(),
        pair<ABILITY_FIGHTER, Fighter>(),
        pair<ABILITY_TELEKINETIC, Telekinetic>(),
        pair<ABILITY_COMBUSTION, Combustion>(),
        pair<ABILITY_PONY_POWER, PonyPower>(),
        pair<ABILITY_POWDER_BURST, PowderBurst>(),
        pair<ABILITY_RETRIEVER, Retriever>(),
        pair<ABILITY_MONSTER_MASH, MonsterMash>(),
        pair<ABILITY_TWO_STEP, TwoStep>(),
        pair<ABILITY_SPITEFUL, Spiteful>(),
        pair<ABILITY_FORTITUDE, Fortitude>(),
        pair<ABILITY_DEVOURER, Devourer>(),
        pair<ABILITY_PHANTOM_THIEF, PhantomThief>(),
        pair<ABILITY_EARLY_GRAVE, EarlyGrave>(),
        pair<ABILITY_BASS_BOOSTED, BassBoosted>(),
        pair<ABILITY_FLAMING_JAWS, FlamingJaws>(),
        pair<ABILITY_MONSTER_HUNTER, MonsterHunter>(),
        pair<ABILITY_CROWNED_SWORD, CrownedSword>(),
        pair<ABILITY_CROWNED_SHIELD, CrownedShield>(),
        pair<ABILITY_BERSERK_DNA, BerserkDna>(),
        pair<ABILITY_CROWNED_KING, CrownedKing>(),
        pair<ABILITY_SNAP_TRAP_WHEN_HIT, SnapTrapWhenHit>(),
        pair<ABILITY_PERMANENCE, Permanence>(),
        pair<ABILITY_HUBRIS, Hubris>(),
        pair<ABILITY_COSMIC_DAZE, CosmicDaze>(),
        pair<ABILITY_MINDS_EYE, MindsEye>(),
        pair<ABILITY_BLOOD_PRICE, BloodPrice>(),
        pair<ABILITY_SPIKE_ARMOR, SpikeArmor>(),
        pair<ABILITY_VOODOO_POWER, VoodooPower>(),
        pair<ABILITY_CHROME_COAT, ChromeCoat>(),
        pair<ABILITY_BANSHEE, Banshee>(),
        pair<ABILITY_WEB_SPINNER, WebSpinner>(),
        pair<ABILITY_SHOWDOWN_MODE, ShowdownMode>(),
        pair<ABILITY_SEED_SOWER, SeedSower>(),
        pair<ABILITY_AIRBORNE, Airborne>(),
        pair<ABILITY_PARROTING, Parroting>(),
        pair<ABILITY_SALT_CIRCLE, SaltCircle>(),
        pair<ABILITY_PURIFYING_SALT, PurifyingSalt>(),
        pair<ABILITY_PROTOSYNTHESIS, Protosynthesis>(),
        pair<ABILITY_QUARK_DRIVE, QuarkDrive>(),
        pair<ABILITY_WIND_POWER, WindPower>(),
        pair<ABILITY_IMPULSE, Impulse>(),
        pair<ABILITY_TERMINAL_VELOCITY, TerminalVelocity>(),
        pair<ABILITY_ANGER_SHELL, AngerShell>(),
        pair<ABILITY_EGOIST, Egoist>(),
        pair<ABILITY_READIED_ACTION, ReadiedAction>(),
        pair<ABILITY_DARK_GALE_WINGS, DarkGaleWings>(),
        pair<ABILITY_GUILT_TRIP, GuiltTrip>(),
        pair<ABILITY_WATER_GALE_WINGS, WaterGaleWings>(),
        pair<ABILITY_ZERO_TO_HERO, ZeroToHero>(),
        pair<ABILITY_COSTAR, Costar>(),
        pair<ABILITY_COMMANDER, Commander>(),
        pair<ABILITY_EJECT_PACK_ABILITY, EjectPackAbility>(),
        pair<ABILITY_VENGEFUL_SPIRIT, VengefulSpirit>(),
        pair<ABILITY_CUD_CHEW, CudChew>(),
        pair<ABILITY_ARMOR_TAIL, ArmorTail>(),
        pair<ABILITY_MIND_CRUSH, MindCrush>(),
        pair<ABILITY_SUPREME_OVERLORD, SupremeOverlord>(),
        pair<ABILITY_ILL_WILL, IllWill>(),
        pair<ABILITY_FIRE_SCALES, FireScales>(),
        pair<ABILITY_WATCH_YOUR_STEP, WatchYourStep>(),
        pair<ABILITY_RAPID_RESPONSE, RapidResponse>(),
        pair<ABILITY_DOUBLE_IRON_BARBS, DoubleIronBarbs>(),
        pair<ABILITY_THERMAL_EXCHANGE, ThermalExchange>(),
        pair<ABILITY_GOOD_AS_GOLD, GoodAsGold>(),
        pair<ABILITY_SHARING_IS_CARING, SharingIsCaring>(),
        pair<ABILITY_TABLETS_OF_RUIN, TabletsOfRuin>(),
        pair<ABILITY_SWORD_OF_RUIN, SwordOfRuin>(),
        pair<ABILITY_VESSEL_OF_RUIN, VesselOfRuin>(),
        pair<ABILITY_BEADS_OF_RUIN, BeadsOfRuin>(),
        pair<ABILITY_PERMAFROST_CLONE, PermafrostClone>(),
        pair<ABILITY_GALLANTRY, Gallantry>(),
        pair<ABILITY_ORICHALCUM_PULSE, OrichalcumPulse>(),
        pair<ABILITY_SUN_BASKING, SunBasking>(),
        pair<ABILITY_WINGED_KING, WingedKing>(),
        pair<ABILITY_HADRON_ENGINE, HadronEngine>(),
        pair<ABILITY_IRON_SERPENT, IronSerpent>(),
        pair<ABILITY_SWEEPING_EDGE_PLUS, SweepingEdgePlus>(),
        pair<ABILITY_CELESTIAL_BLESSING, CelestialBlessing>(),
        pair<ABILITY_MINION_CONTROL, MinionControl>(),
        pair<ABILITY_MOLTEN_BLADES, MoltenBlades>(),
        pair<ABILITY_HAUNTING_FRENZY, HauntingFrenzy>(),
        pair<ABILITY_NOISE_CANCEL, NoiseCancel>(),
        pair<ABILITY_RADIO_JAM, RadioJam>(),
        pair<ABILITY_OLE, Ole>(),
        pair<ABILITY_MALICIOUS, Malicious>(),
        pair<ABILITY_DEAD_POWER, DeadPower>(),
        pair<ABILITY_BRAWLING_WYVERN, BrawlingWyvern>(),
        pair<ABILITY_MYTHICAL_ARROWS, MythicalArrows>(),
        pair<ABILITY_LAWNMOWER, Lawnmower>(),
        pair<ABILITY_FLOURISH, Flourish>(),
        pair<ABILITY_DESERT_SPIRIT, DesertSpirit>(),
        pair<ABILITY_CONTEMPT, Contempt>(),
        pair<ABILITY_AERIALIST, Aerialist>(),
        pair<ABILITY_TERA_SHELL, TeraShell>(),
        pair<ABILITY_TOXIC_CHAIN, ToxicChain>(),
        pair<ABILITY_PARASITIC_SPORES, ParasiticSpores>(),
        pair<ABILITY_POISON_PUPPETEER, PoisonPuppeteer>(),
        pair<ABILITY_ENTRANCE, Entrance>(),
        pair<ABILITY_REJECTION, Rejection>(),
        pair<ABILITY_APPLE_ENLIGHTENMENT, AppleEnlightenment>(),
        pair<ABILITY_BALLOON_BOMBER, BalloonBomber>(),
        pair<ABILITY_FLAMING_MAW, FlamingMaw>(),
        pair<ABILITY_DEMOLITIONIST, Demolitionist>(),
        pair<ABILITY_ROCKHARD_WILL, RockhardWill>(),
        pair<ABILITY_FRAGRANT_DAZE, FragrantDaze>(),
        pair<ABILITY_LOW_VISIBILITY, LowVisibility>(),
        pair<ABILITY_OLD_MARINER, OldMariner>(),
        pair<ABILITY_ECTOPLASM, Ectoplasm>(),
        pair<ABILITY_BEAUTIFUL_MUSIC, BeautifulMusic>(),
        pair<ABILITY_SNOW_SONG, SnowSong>(),
        pair<ABILITY_GREATER_SPIRIT, GreaterSpirit>(),
        pair<ABILITY_RESONANCE, Resonance>(),
        pair<ABILITY_ETHEREAL_RUSH, EtherealRush>(),
        pair<ABILITY_CUTE_ANTECEDENCE, CuteAntecedence>(),
        pair<ABILITY_RECURRING_NIGHTMARE, RecurringNightmare>(),
        pair<ABILITY_MENACING_SITUATION, MenacingSituation>(),
        pair<ABILITY_SHINY_LIGHTNING, ShinyLightning>(),
        pair<ABILITY_TERRIFY, Terrify>(),
        pair<ABILITY_ICE_DOWNFALL, IceDownfall>(),
        pair<ABILITY_LAST_STAND, LastStand>(),
        pair<ABILITY_PYROCLASTIC_FLOW, PyroclasticFlow>(),
        pair<ABILITY_BLOOD_BATH, BloodBath>(),
        pair<ABILITY_BATTLE_AURA, BattleAura>(),
        pair<ABILITY_BLOODLUST, Bloodlust>(),
        pair<ABILITY_PIERCING_SOLO, PiercingSolo>(),
        pair<ABILITY_RHYTHMIC, Rhythmic>(),
        pair<ABILITY_CHUNKY_BASS_LINE, ChunkyBassLine>(),
        pair<ABILITY_DUAL_HAMMER, DualHammer>(),
        pair<ABILITY_DENTING_BLOWS, DentingBlows>(),
        pair<ABILITY_ICE_COLD_HUNTER, IceColdHunter>(),
        pair<ABILITY_SOUL_CRUSHER, SoulCrusher>(),
        pair<ABILITY_ARC_FLASH, ArcFlash>(),
        pair<ABILITY_UNICORN, Unicorn>(),
        pair<ABILITY_ON_THE_PROWL, OnTheProwl>(),
        pair<ABILITY_PRETENTIOUS, Pretentious>(),
        pair<ABILITY_VENOBLAZE_PINCERS, VenoblazePincers>(),
        pair<ABILITY_ETERNAL_BLESSING, EternalBlessing>(),
        pair<ABILITY_SUGAR_RUSH, SugarRush>(),
        pair<ABILITY_PEACEFUL_REST, PeacefulRest>(),
        pair<ABILITY_WHITE_NOISE, WhiteNoise>(),
        pair<ABILITY_SMOKEY_MANEUVERS, SmokeyManeuvers>(),
        pair<ABILITY_POWER_METAL, PowerMetal>(),
        pair<ABILITY_POWER_EDGE, PowerEdge>(),
        pair<ABILITY_SUPERCONDUCTOR, Superconductor>(),
        pair<ABILITY_ULTRA_INSTINCT, UltraInstinct>(),
        pair<ABILITY_UNLOCKED_POTENTIAL, UnlockedPotential>(),
        pair<ABILITY_HIGHER_RANK, HigherRank>(),
        pair<ABILITY_FUNERAL_PYRE, FuneralPyre>(),
        pair<ABILITY_FLAME_BUBBLE, FlameBubble>(),
        pair<ABILITY_ELEMENTAL_VORTEX, ElementalVortex>(),
        pair<ABILITY_SNOWY_WRATH, SnowyWrath>(),
        pair<ABILITY_PATTERN_CHANGE, PatternChange>(),
        pair<ABILITY_NO_TURNING_BACK, NoTurningBack>(),
        pair<ABILITY_FLAMMABLE_COAT, FlammableCoat>(),
        pair<ABILITY_DRACO_MORALE, DracoMorale>(),
        pair<ABILITY_BAD_OMEN, BadOmen>(),
        pair<ABILITY_MOSH_PIT, MoshPit>(),
        pair<ABILITY_BLOOD_STAIN, BloodStain>(),
        pair<ABILITY_BLOOD_STIGMA, BloodStigma>(),
        pair<ABILITY_MAXIMUM_ACCELERATION, MaximumAcceleration>(),
        pair<ABILITY_SIDEWINDER, Sidewinder>(),
        pair<ABILITY_PETRIFY, Petrify>(),
        pair<ABILITY_FLUFFIEST, Fluffiest>(),
        pair<ABILITY_WAY_OF_PRECISION, WayOfPrecision>(),
        pair<ABILITY_WAY_OF_SWIFTNESS, WayOfSwiftness>(),
        pair<ABILITY_ATOMIC_PUNCH, AtomicPunch>(),
        pair<ABILITY_IRON_GIANT, IronGiant>(),
        pair<ABILITY_MASTER_HAND, MasterHand>(),
        pair<ABILITY_FINAL_BLOW, FinalBlow>(),
        pair<ABILITY_HOSPITALITY, Hospitality>(),
        pair<ABILITY_BUTTER_UP, ButterUp>(),
        pair<ABILITY_VITALITY_STRIKE, VitalityStrike>(),
        pair<ABILITY_HUGE_WINGS, HugeWings>(),
        pair<ABILITY_SWORD_OF_DAMNATION, SwordOfDamnation>(),
        pair<ABILITY_RESTRAINING_ORDER, RestrainingOrder>(),
        pair<ABILITY_ASSASSINS_TOOLS, AssassinsTools>(),
        pair<ABILITY_FROSTMAW, Frostmaw>(),
        pair<ABILITY_PATCHWORK, Patchwork>(),
        pair<ABILITY_BLIND_RAGE, BlindRage>(),
        pair<ABILITY_SLIPSTREAM, Slipstream>(),
        pair<ABILITY_APEX_PREDATOR, ApexPredator>(),
        pair<ABILITY_DRAGONS_RITUAL, DragonsRitual>(),
        pair<ABILITY_PINNACLE_BLADE, PinnacleBlade>(),
        pair<ABILITY_ENERGIZED, Energized>(),
        pair<ABILITY_COLOR_SPECTRUM, ColorSpectrum>(),
        pair<ABILITY_STEEL_BEETLE, SteelBeetle>(),
        pair<ABILITY_FROM_THE_SHADOWS, FromTheShadows>(),
        pair<ABILITY_RAGE_POINT, RagePoint>(),
        pair<ABILITY_HOT_COALS, HotCoals>(),
        pair<ABILITY_TERASTAL_TREASURE, TerastalTreasure>(),
        pair<ABILITY_SHOCKING_MAW, ShockingMaw>(),
        pair<ABILITY_GLEAM_EYES, GleamEyes>(),
        pair<ABILITY_ROUSED_FANGS, RousedFangs>(),
        pair<ABILITY_DREAM_STATE, DreamState>(),
        pair<ABILITY_DREAM_WHIMSY, DreamWhimsy>(),
        pair<ABILITY_LUNAR_AFFINITY, LunarAffinity>(),
        pair<ABILITY_FLAME_SHIELD, FlameShield>(),
        pair<ABILITY_AQUATIC_DWELLER, AquaticDweller>(),
        pair<ABILITY_APPLE_PIE, ApplePie>(),
        pair<ABILITY_HOVER, Hover>(),
        pair<ABILITY_DEPRAVITY, Depravity>(),
        pair<ABILITY_WILDFIRE, Wildfire>(),
        pair<ABILITY_JUMP_SCARE, JumpScare>(),
        pair<ABILITY_TAR_TOSS, TarToss>(),
        pair<ABILITY_STUN_SHOCK, StunShock>(),
        pair<ABILITY_RAGING_GODDESS, RagingGoddess>(),
        pair<ABILITY_WHIPLASH, Whiplash>(),
        pair<ABILITY_SUPERSWEET_SYRUP, SupersweetSyrup>(),
        pair<ABILITY_TRASH_HEAP, TrashHeap>(),
        pair<ABILITY_SLUDGY_MIX, SludgyMix>(),
        pair<ABILITY_OVERWATCH, Overwatch>(),
        pair<ABILITY_WIND_RAGE, WindRage>(),
        pair<ABILITY_VICTORY_BOMB, VictoryBomb>(),
        pair<ABILITY_RAZOR_SHARP, RazorSharp>(),
        pair<ABILITY_TO_THE_BONE, ToTheBone>(),
        pair<ABILITY_BLADE_DANCE, BladeDance>(),
        pair<ABILITY_APE_SHIFT, ApeShift>(),
        pair<ABILITY_KNOW_YOUR_PLACE, KnowYourPlace>(),
        pair<ABILITY_DEEP_CUTS, DeepCuts>(),
        pair<ABILITY_LIFE_STEAL, LifeSteal>(),
        pair<ABILITY_RUDE_AWAKENING, RudeAwakening>(),
        pair<ABILITY_TERAFORM_ZERO, TeraformZero>(),
        pair<ABILITY_SET_ABLAZE, SetAblaze>(),
        pair<ABILITY_BREAKWATER, Breakwater>(),
        pair<ABILITY_MAGICAL_FISTS, MagicalFists>(),
        pair<ABILITY_CUTTHROAT, Cutthroat>(),
        pair<ABILITY_SAND_BENDER, SandBender>(),
        pair<ABILITY_SAND_PIT, SandPit>(),
        pair<ABILITY_DESOLATE_SUN, DesolateSun>(),
        pair<ABILITY_DAYBREAK, Daybreak>(),
        pair<ABILITY_ENERGY_SIPHON, EnergySiphon>(),
        pair<ABILITY_RESERVOIR, Reservoir>(),
        pair<ABILITY_NEUROTOXIN, Neurotoxin>(),
        pair<ABILITY_ENERGIZED_HORNS, EnergizedHorns>(),
        pair<ABILITY_SPIDER_LAIR_UPGRADE, SpiderLairUpgrade>(),
        pair<ABILITY_CRUST_COAT, CrustCoat>(),
        pair<ABILITY_PUFFY, Puffy>(),
        pair<ABILITY_BALLOON_BLITZ, BalloonBlitz>(),
        pair<ABILITY_STRIKER_PIXILATE, StrikerPixilate>(),
        pair<ABILITY_DOOM_BLAST, DoomBlast>(),
        pair<ABILITY_BRUTEFORCE, Bruteforce>(),
        pair<ABILITY_FARADAY_CAGE, FaradayCage>(),
        pair<ABILITY_ACIDIC_SLIME, AcidicSlime>(),
        pair<ABILITY_ROSE_GARDEN, RoseGarden>(),
        pair<ABILITY_QIGONG, Qigong>(),
        pair<ABILITY_CONJOURER_OF_DECEIT, ConjurerOfDeceit>(),
        pair<ABILITY_DEEP_FREEZE, DeepFreeze>(),
        pair<ABILITY_SOUL_DEVOURER, SoulDevourer>(),
        pair<ABILITY_CHAMPIONS_ENTRANCE, ChampionsEntrance>(),
        pair<ABILITY_PRESTO, Presto>(),
        pair<ABILITY_SAMBA, Samba>(),
        pair<ABILITY_GLADIATOR, Gladiator>(),
        pair<ABILITY_FORSAKEN_HEART, ForsakenHeart>(),
        pair<ABILITY_RELENTLESS, Relentless>(),
        pair<ABILITY_SOOTHSAYER, Soothsayer>(),
        pair<ABILITY_CORRUPTED_MIND, CorruptedMind>(),
        pair<ABILITY_FLAME_COAT, FlameCoat>(),
        pair<ABILITY_UNOWN_POWER, UnownPower>(),
        pair<ABILITY_SUPER_SCOPE, SuperScope>(),
        pair<ABILITY_VENOM_CROWN, VenomCrown>(),
        pair<ABILITY_BLIGHT_SCALE, BlightScale>(),
        pair<ABILITY_GUNMAN, Gunman>(),
        pair<ABILITY_HUNTERS_MARK, HuntersMark>(),
        pair<ABILITY_CARETAKER, Caretaker>(),
        pair<ABILITY_POSEIDONS_DOMINION, PoseidonsDominion>(),
        pair<ABILITY_DUAL_SHADOW, DualShadow>(),
        pair<ABILITY_LULLABY, Lullaby>(),
        pair<ABILITY_CRYO_ARCHITECT, CryoArchitect>(),
        pair<ABILITY_GLACIAL_RAGE, GlacialRage>(),
        pair<ABILITY_IMMOVABLE_OBJECT, ImmovableObject>(),
        pair<ABILITY_FRENZIED_PHANTOM, FrenziedPhantom>(),
        pair<ABILITY_DNA_SCRAMBLE, DNAScramble>(),
        pair<ABILITY_AQUATIC, Aquatic>(),
        pair<ABILITY_METALLIC_JAWS, MetallicJaws>(),
        pair<ABILITY_CALCULATIVE, Calculative>(),
        pair<ABILITY_EMBODY_ASPECT, EmbodyAspect>(),
        pair<ABILITY_EMBODY_ASPECT_HEARTHFLAME, EmbodyAspect>(),
        pair<ABILITY_EMBODY_ASPECT_CORNERSTONE, EmbodyAspectCornerstone>(),
        pair<ABILITY_EMBODY_ASPECT_WELLSPRING, EmbodyAspectWellspring>(),
        pair<ABILITY_ROCKHARD_SHAFT, RockhardShaft>(),
        pair<ABILITY_DEVIATE, Deviate>(),
        pair<ABILITY_SUNS_BOUNTY, SunsBounty>(),
        pair<ABILITY_RITE_OF_SPRING, RiteOfSpring>(),
        pair<ABILITY_HEADSTRONG, Headstrong>(),
        pair<ABILITY_FIREFIGHTER, Firefighter>(),
        pair<ABILITY_SEPIA_LENS, SepiaLens>(),
        pair<ABILITY_SUPER_SNIPER, SuperSniper>(),
        pair<ABILITY_WOODLAND_CURSE, WoodlandCurse>(),
        pair<ABILITY_MALODOR, Malodor>(),
        pair<ABILITY_BLUR, Blur>(),
        pair<ABILITY_ELUDE, Elude>(),
        pair<ABILITY_DRAKE_OF_RAGE, DrakeOfRage>(),
        pair<ABILITY_MIXED_MARTIAL_ARTS, MixedMartialArts>(),
        pair<ABILITY_STRATEGIC_PAUSE, StrategicPause>(),
        pair<ABILITY_OVERRULE, Overrule>(),
        pair<ABILITY_MENTAL_POLLUTION, MentalPollution>(),
        pair<ABILITY_MADNESS_ENHANCEMENT, MadnessEnhancement>(),
        pair<ABILITY_TENTALOCK, Tentalock>(),
        pair<ABILITY_SERPENT_BIND, SerpentBind>(),
        pair<ABILITY_SOUL_TAP, SoulTap>(),
        pair<ABILITY_SCARECROW, Scarecrow>(),
        pair<ABILITY_OMINOUS_SHROUD, OminousShroud>(),
        pair<ABILITY_CHILLING_PRESENCE, ChillingPresence>(),
        pair<ABILITY_FROSTBIND, Frostbind>(),
        pair<ABILITY_GLACIAL_GHOST, GlacialGhost>(),
        pair<ABILITY_TENDER_AFFECTION, TenderAffection>(),
        pair<ABILITY_WONDER_SCALE, WonderScale>(),
        pair<ABILITY_OVERZEALOUS, Overzealous>(),
        pair<ABILITY_STAINLESS_STEEL, StainlessSteel>(),
        pair<ABILITY_TEMPORAL_RUPTURE, TemporalRupture>(),
        pair<ABILITY_GRASS_FLUTE, GrassFlute>(),
        pair<ABILITY_HEMOTOXIN, Hemotoxin>(),
        pair<ABILITY_HARUKAZE, Harukaze>(),
        pair<ABILITY_TOXIC_SURGE, ToxicSurge>(),
        pair<ABILITY_ATLANTIC_RULER, AtlanticRuler>(),
        pair<ABILITY_BIOFILM, Biofilm>(),
        pair<ABILITY_CHOKEHOLD, Chokehold>(),
        pair<ABILITY_GUARDIAN_COAT, GuardianCoat>(),
        pair<ABILITY_NEUTRALIZING_FOG, NeutralizingFog>(),
        pair<ABILITY_POISON_QUILLS, PoisonQuills>(),
        pair<ABILITY_DRACONIC_MIGHT, DraconicMight>(),
        pair<ABILITY_FESTIVITIES, Festivities>(),
        pair<ABILITY_FEY_FLIGHT, FeyFlight>(),
        pair<ABILITY_BEST_OFFENSE, BestOffense>(),
        pair<ABILITY_IMPALER, Impaler>(),
        pair<ABILITY_MAGUS_BLADES, MagusBlades>(),
        pair<ABILITY_LIGHTNING_BORN, LightningBorn>(),
        pair<ABILITY_SUPERHEAVY, Superheavy>(),
        pair<ABILITY_WORLD_SERPENT, WorldSerpent>(),
        pair<ABILITY_LUCKY_WINGS, LuckyWings>(),
        pair<ABILITY_KOMODO, Komodo>(),
        pair<ABILITY_ENVENOM, Envenom>(),
        pair<ABILITY_PURPLE_HAZE, PurpleHaze>(),
        pair<ABILITY_GNASHING_CANNON, GnashingCannon>(),
        pair<ABILITY_HYPER_CLEANSE, HyperCleanse>(),
        pair<ABILITY_MOLTEN_COAT, MoltenCoat>(),
        pair<ABILITY_ROYAL_DECREE, RoyalDecree>(),
        pair<ABILITY_BREEZY_NEIGH, BreezyNeigh>(),
        pair<ABILITY_DREAMSCAPE, Dreamscape>(),
        pair<ABILITY_HASTE_MAKES_WASTE, HasteMakesWaste>(),
        pair<ABILITY_HUNGRY_MAWS, HungryMaws>(),
        pair<ABILITY_THERMAL_SLIDE, ThermalSlide>(),
        pair<ABILITY_THERMOMANCY, Thermomancy>(),
        pair<ABILITY_CHUCKSTER, Chuckster>(),
        pair<ABILITY_HEAT_SINK, HeatSink>(),
        pair<ABILITY_RELIC_STONE, RelicStone>(),
        pair<ABILITY_SUPERCELL, Supercell>(),
        pair<ABILITY_LIGHTNING_ASPECT, LightningAspect>(),
        pair<ABILITY_POISON_HEAL, PoisonHeal>(),
        pair<ABILITY_ENERGY_TAP, EnergyTap>(),
        pair<ABILITY_JUNSHI_SANDA, JunshiSanda>(),
        pair<ABILITY_REVERBATE, Reverberate>(),
        pair<ABILITY_TAEKKYEON, Taekkyeon>(),
        pair<ABILITY_SLUDGE_SPIT, SludgeSpit>(),
        pair<ABILITY_SWAMP_THING, SwampThing>(),
        pair<ABILITY_FROSTY_PRESCENCE, FrostyPrescence>(),
        pair<ABILITY_CHILLING_PELLETS, ChillingPellets>(),
        pair<ABILITY_PAINT_SHOT, PaintShot>(),
        pair<ABILITY_STONECUTTER, Stonecutter>(),
        pair<ABILITY_EDGELORD, Edgelord>(),
        pair<ABILITY_WARMONGER, Warmonger>(),
        pair<ABILITY_LOCUST_SWARM, LocustSwarm>(),
        pair<ABILITY_REVELATION, Revelation>(),
        pair<ABILITY_CURSE_OF_FAMINE, CurseOfFamine>(),
        pair<ABILITY_CRYSTALLINE_ARMOR, CrystallineArmor>(),
        pair<ABILITY_SOUL_HARVEST, SoulHarvest>(),
        pair<ABILITY_THICK_BLUBBER, ThickBlubber>(),
        pair<ABILITY_CRAVING, Craving>(),
        pair<ABILITY_RAT_KING, RatKing>(),
        pair<ABILITY_CRISPY_CREAM, CrispyCream>(),
        pair<ABILITY_DEEP_FRIED, DeepFried>(),
        pair<ABILITY_FOOD_LOVERS, FoodLovers>(),
        pair<ABILITY_LUNAR_WRATH, LunarWrath>(),
        pair<ABILITY_SPYWARE, Spyware>(),
        pair<ABILITY_VIRUS, Virus>(),
        pair<ABILITY_POWER_LEAK, PowerLeak>(),
        pair<ABILITY_BACKUP_POWER, BackupPower>(),
        pair<ABILITY_SAND_FIEND, SandFiend>(),
        pair<ABILITY_MOUSTACHE, Moustache>(),
        pair<ABILITY_DEPTH_EXPLORER, DepthExplorer>(),
        pair<ABILITY_DUNE_VEIL, DuneVeil>(),
        pair<ABILITY_STRONG_FOUNDATION, StrongFoundation>(),
        pair<ABILITY_FOG_MACHINE, FogMachine>(),
        pair<ABILITY_DROP_BLOCKS, DropBlocks>(),
        pair<ABILITY_LASER_DRILL, LaserDrill>(),
        pair<ABILITY_LIGHT_SABER, LightSaber>(),
        pair<ABILITY_LOOSE_THORNS, LooseThorns>(),
        pair<ABILITY_TURF_WAR, TurfWar>(),
        pair<ABILITY_GREEDY, Greedy>(),
        pair<ABILITY_MUSICAL_NOTES, MusicalNotes>(),
        pair<ABILITY_STRIKEOUT, Strikeout>(),
        pair<ABILITY_HOME_RUN, HomeRun>(),
        pair<ABILITY_BRUISER, Bruiser>(),
        pair<ABILITY_LETS_DANCE, LetsDance>(),
        pair<ABILITY_MYCELIUM_MIGHT, MyceliumMight>(),
        pair<ABILITY_DEADLY_PRECISION, DeadlyPrecision>(),
        pair<ABILITY_RIPEN, Ripen>(),
    };

    consteval AbilityPtrArray generate() {
        AbilityPtrArray arr{0};
        for (auto pair : sAbilities) {
            arr[pair.key] = pair.ability;
        }
        return arr;
    }

   public:
    ~ability_behavior();
};

template <typename T>
class AbilityEngine {
   private:
    constexpr static Implementation impl = T();

   public:
    ~AbilityEngine();
};

#pragma GCC diagnostic pop
