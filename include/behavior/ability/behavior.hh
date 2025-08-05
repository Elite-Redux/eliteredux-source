#pragma once

#include "behavior/constants.hh"
#include "behavior/implementation_interface.hh"
#include "behavior/ability/constants.hh"

template <typename T>
class AbilityBehavior {
   private:
    constexpr static Implementation impl = T();

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-function"

    class Breakable : public virtual Ability {
        virtual breakable() override { return true; }
    };
    class RandomizerBanned : public virtual Ability {
        virtual randomizerBanned() override { return true; }
    };
    class Unsuppressable : public virtual Ability {
        virtual unsuppressable() override { return true; }
    };
    class Persistent : public virtual Ability {
        virtual persistent() override { return true; }
    };
    class FormChange : public virtual RandomizerBanned, public virtual Unsuppressable {};

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

    template <Type GaleWingsType>
    class GaleWingsLike : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(GetTypeBeforeUsingMove(move, battler) == GaleWingsType)
            CHECK(BATTLER_MAX_HP(battler))
            return 1;
        }
    }
#define MUL(val) MUL_MODIFIER(modifier, val)
#define RESISTANCE(val)                \
    {                                  \
        MUL_MODIFIER(resistance, val); \
        MUL_MODIFIER(modifier, val);   \
    }
    static void
    InsertCorrectEndType(AbilityCallType type) {
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

    template <Type BoostType>
    class AteAbility : public virtual Ability {
        int onMoveType(ON_MOVE_TYPE) override {
            CHECK(moveType == TYPE_NORMAL)
            *ateBoost = TRUE;
            return BoostType + 1;
        }
        int onStab(ON_STAB) override { return moveType == AteType; }
    };

    template <Type BoostType>
    class SwarmLike : public virtual Ability {
        int onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (move == BoostType) {
                if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                    MUL(1.5);
                else
                    MUL(1.2);
            }
        }
    };

    template <int BoostType>
    class BoostedSwarmLike : public virtual Ability {
        int onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (move == BoostType) {
                if (gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / 3))
                    MUL(1.8);
                else
                    MUL(1.3);
            }
        }
    };

    static void RuinEffect(int ruinStat, int battler, int statId, u32 *stat, NonStackingState *flags) {
        if (statId != ruinStat) return;
        if (*flags & NON_STACKING_RUIN) return;
        ON_ABILITY(battler, FALSE, gAbilities[ability].ruinStat == statId, return) *stat *= .75;
        *flags = static_cast<NonStackingState>(static_cast<int>(*flags) | static_cast<int>(NON_STACKING_RUIN));
    }

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

    class None : public virtual RandomizerBanned {};

    class Stench : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanMoveHaveExtraFlinchChance(move))
            CHECK(Random() % 100 < 10)

            return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
        }
        bool toxicTerrainImmune() override { return true; }
    };

    class PoisonHeal : public virtual Ability {
        bool toxicTerrainImmune() override { return true; }
    };

    class Drizzle : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class SpeedBoost : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK(gVolatileStructs[battler].isFirstTurn != 2)
            CHECK(ChangeStatBuffs(battler, 1, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            gBattleScripting.battler = battler;
            return TRUE;
        }
    };

    class BattleArmor : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override { MUL(.8); }
        int onCrit override { return NEVER_CRIT; }
        AbilityApplyOnWithTarget onCritFor() override { return APPLY_ON_TARGET; }
    };

    class Sturdy : public virtual Breakable {};

    class Damp : public virtual Ability {
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

    class Limber : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_PARALYSIS)
            return TRUE;
        }
        bool halfRecoil() { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class SandImmune : public virtual Ability {
        AbilityApplyOnWithTarget sandImmune() override { return true; }
    };
    class SandVeil : public virtual Breakable, public virtual SandImmune {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IsBattlerWeatherAffected(target, WEATHER_SANDSTORM_ANY));
            *accuracy /= 1.25;
            return ACCURACY_MULTIPLICATIVE;
        }
        AbilityApplyOnWithTarget onAccuracyFor() override { return APPLY_ON_TARGET; }
    };

    class Static : public virtual Ability {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBeParalyzed(battler, opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_PARALYSIS, battler, opponent);
            return TRUE;
        }
    };

    class VoltAbsorb : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_ELECTRIC)
            return ABSORB_RESULT_HEAL;
        }
    };

    class WaterAbsorb : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_WATER)
            return ABSORB_RESULT_HEAL;
        }
    };

    class Oblivious : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING))
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
        bool tauntImmune() { return true; }
    };

    class CloudNine : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
            return TRUE;
        }
    };

    class CompoundEyes : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            *accuracy *= 1.3;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Insomnia : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_SLEEP)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class ColorChange : public virtual Ability {
        int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) override {
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
        AbilityApplyOnWithTarget onBeforeAttackFor() override { return onBeforeAttackFor; }
    };

    class Immunity : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & (CHECK_STATUS1 & ~CHECK_SLEEP))
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class FlashFire : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_FIRE)
            return ABSORB_RESULT_FLASH_FIRE;
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE && gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE) MUL(1.5);
        }
    };

    class ShieldDust : public virtual Breakable {
        bool powderImmune() { return true; }
    };

    class OwnTempo : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_CONFUSION)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
        bool tauntImmune() { return true; }
    };

    class SuctionCups : public virtual Breakable {};

    class Intimidate : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class ShadowTag : public virtual Ability {
        int onTrap(ABILITY_ON_TRAP) override {
            ON_ABILITY(switchingBattler, FALSE, gAbilities[ability].shadowTag, return FALSE)
            return TRUE;
        }
        bool shadowTag() override { return true; }
    };

    class RoughSkin : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class WonderGuard : public virtual Breakable, public virtual RandomizerBanned {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (*mod < UQ_4_12(2.0)) *mod = 0;
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return onAfterTypeEffectivenessFor; }
    };

    class GroundImmune : public virtual Breakable {
        bool levitate() { return true; }
    };
    class Levitate : public virtual GroundImmune {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FLYING) MUL(1.25);
        }
    };

    class EffectSpore : public virtual Breakable {
        int onDefender(ON_DEFENDER) override {
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
        bool powderImmune() { return true; }
    };

    class ClearBody : public virtual Breakable {};

    class NaturalCure : public virtual Ability {
        int onExit(ON_EXIT) override {
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

    template <Type Redirect>
    class Redirects : public virtual Breakable {
        Type redirectType() { return Redirect; }
    };

    class LightningRod : public virtual Redirects<TYPE_ELECTRIC> {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_ELECTRIC);
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class SereneGrace : public virtual Ability {
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override { *effectChance *= 2; }
    };

    class SwiftSwim : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY)) *stat *= 1.5;
        }
    };

    class Chlorophyll : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        }
    };

    class Illuminate : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Trace : public virtual RandomizerBanned {
        int onEntry(ON_ENTRY) override {
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

    class HugePower : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_ATK) *stat *= 2;
        }
    };

    class PoisonPoint : public virtual Ability {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBePoisoned(battler, opponent, MOVE_NONE))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_POISON, battler, opponent);
            return TRUE;
        }
    };

    class InnerFocus : public virtual Breakable {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(move == MOVE_FOCUS_BLAST)
            return ACCURACY_ALWAYS_HITS;
        }
        bool tauntImmune() { return true; }
    };

    class MagmaArmor : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) RESISTANCE(.7);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_FROSTBITE)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class WaterVeil : public virtual Breakable {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gStatuses3[battler] & STATUS3_AQUA_RING)

            gStatuses3[battler] |= STATUS3_AQUA_RING;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
            return TRUE;
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_BURN)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class MagnetPull : public virtual Ability {
        int onTrap(ABILITY_ON_TRAP) override { return IS_BATTLER_OF_TYPE(switchingBattler, TYPE_STEEL); }
    };

    class Soundproof : public virtual Breakable {
        int onImmune(ON_IMMUNE) override {
            CHECK(IsSoundMove(attacker, move))
            CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
        bool isSoundproof() { return true; }
    };

    class RainDish : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class SandStream : public virtual SandImmune {
        int onEntry(ON_ENTRY) override {
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

    class Pressure : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class ThickFat : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.5);
        }
    };

    class FlameBody : public virtual Ability {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(CanBeBurned(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
            return TRUE;
        }
    };

    class KeenEye : public virtual Breakable {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class HyperCutter : public virtual Breakable {
        int onCrit(ON_CRIT) override {
            CHECK(IsMoveMakingContact(move, battler))
            return 1;
        }
    };

    class Pickup : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class Truant : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            if (GetAbilityState(battler, ability))
                SetAbilityState(battler, ability, FALSE);
            else if (gChosenMoveByBattler[battler] && !IS_MOVE_STATUS(gChosenMoveByBattler[battler]))
                SetAbilityState(battler, ability, TRUE);
            return FALSE;
        }
    };

    class Hustle : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override { MUL(1.4); }
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK_NOT(IS_MOVE_STATUS(move)) *accuracy *= .9;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class CuteCharm : public virtual Ability {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            CHECK(CanInfatuate(battler, opponent))
            CHECK(Random() % 100 < 50)

            AbilityStatusEffectSafe(MOVE_EFFECT_ATTRACT, battler, opponent);
            return TRUE;
        }
    };

    class Plus : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            int partner = BATTLE_PARTNER(battler);
            if (!IsBattlerAlive(partner)) return;
            if (BattlerHasAbility(partner, ABILITY_PLUS, FALSE) || BattlerHasAbility(partner, ABILITY_MINUS, FALSE)) MUL(2.0);
        }
    };

    class Minus : public virtual Plus {};

    class StandardTransformation : public virtual FormChange {
        int onEntry(ON_ENTRY) override { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        int onEndTurn(ON_END_TURN) override { return TryTransformAttacker(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
    };
    class WeatherTransformation : public virtual StandardTransformation {
        int onWeather(ON_WEATHER) override { return TryTransformAttacker(ability, battler, ABILITY_BS_CALL); }
    };
    class Forecast : public virtual WeatherTransformation {
        int onAttacker(ON_ATTACKER) override {
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

    class StickyHold : public virtual Breakable {};

    class ShedSkin : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK(Random() % 100 < 30)

            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
    };

    class Guts : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        }
        bool negatesBurnAtkDrop() override { return true; }
    };

    class MarvelScale : public virtual Ability {
        void onStat(ON_STAT) override {
            if ((statId == STAT_DEF || statId == STAT_SPDEF) && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        }
    };

    class Overgrow : public virtual SwarmLike<TYPE_GRASS> {};

    class Blaze : public virtual SwarmLike<TYPE_FIRE> {};

    class Torrent : public virtual SwarmLike<TYPE_WATER> {};

    class Swarm : public virtual SwarmLike<TYPE_BUG> {};

    class RockHead : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_CONFUSION)
            return TRUE;
        }
        bool noRecoil() { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class Drought : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class ArenaTrap : public virtual Ability {
        int onTrap(ABILITY_ON_TRAP) override { return IsBattlerGrounded(switchingBattler); }
    };

    class VitalSpirit : public virtual Breakable {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_FIGHTING)
            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_SLEEP)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
        bool tauntImmune() { return true; }
    };

    class WhiteSmoke : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)

            int side = GET_BATTLER_SIDE(battler);
            gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
            gSideTimers[side].started.smokescreen = TRUE;
            gSideTimers[side].smokescreenBattler = battler;
            return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
        }
    };

    class PurePower : public virtual HugePower {};

    class ShellArmor : public virtual BattleArmor {};

    class AirLock : public virtual CloudNine {};

    class TangledFeet : public virtual Breakable {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(gBattleMons[target].status2 & STATUS2_CONFUSION);
            *accuracy /= 2;
            return ACCURACY_MULTIPLICATIVE;
        }
        AbilityApplyOnWithTarget onAccuracyFor() override { return APPLY_ON_TARGET; }
    };

    class MotorDrive : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_ELECTRIC);
            *statId = STAT_SPEED;
            return ABSORB_RESULT_STAT;
        }
    };

    class Rivalry : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[battler].species, gBattleMons[battler].personality);
            if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[target].species, gBattleMons[target].personality))
                MUL(1.25);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            int genderAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[attacker].species, gBattleMons[attacker].personality);
            if (genderAtk == MON_MALE)
                genderAtk = MON_FEMALE;
            else if (genderAtk == MON_FEMALE)
                genderAtk = MON_MALE;
            if (genderAtk != MON_GENDERLESS && genderAtk == GetGenderFromSpeciesAndPersonality(gBattleMons[battler].species, gBattleMons[battler].personality))
                MUL(.75);
        }
    };

    class HailImmune : public virtual Ability {
        AbilityApplyOnWithTarget hailImmune() override { return true; }
    } class SnowCloak : public virtual Breakable, public virtual HailImmune {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IsBattlerWeatherAffected(target, WEATHER_HAIL_ANY));
            *accuracy /= 1.25;
            return ACCURACY_MULTIPLICATIVE;
        }
        AbilityApplyOnWithTarget onAccuracyFor() override { return APPLY_ON_TARGET; }
    };

    class AngerPoint : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class Unburden : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && GetAbilityState(battler, ability)) *stat *= 2;
        }
    };

    class Heatproof : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        }
        bool negatesBurnAtkDrop() { return true; }
    };

    class DrySkin : public virtual WaterAbsorb, public virtual RainDish {
        int onEndTurn(ON_END_TURN) override {
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && !IsMagicGuardProtected(battler)) {
                gBattleMoveDamage = gBattleMons[battler].maxHP / 8;
                if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
                BattleScriptPushCursorAndCallback(BattleScript_SolarPowerActivates);
                return TRUE;
            }

            return RainDish::onEndTurn(DELEGATE_END_TURN);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) RESISTANCE(1.25);
        }
    };

    class Download : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gBattlerTarget = BATTLE_OPPOSITE(battler);
            if (!IsBattlerAlive(battler)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
            CHECK(IsBattlerAlive(battler))

            int stat = GetHighestDefendingStatId(gBattlerTarget, TRUE) == STAT_DEF ? STAT_SPATK : STAT_ATK;
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class IronFist : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IsIronFistBoosted(battler, move)) MUL(1.3);
        }
    };

    class Adaptability : public virtual Ability {
        bool adaptability() override { return true; }
    };

    class SkillLink : public virtual Ability {
        bool skillLink() override { return true; }
    };

    class Hydration : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
    };

    class SolarPower : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat *= 1.5;
        }
    };

    class QuickFeet : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && HasAnyStatusOrAbility(battler)) *stat *= 1.5;
        }
    };

    class Normalize : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        }
        int onMoveType(ON_MOVE_TYPE) override { return TYPE_NORMAL + 1; }
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_NORMAL) CHECK(*mod) CHECK(*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
            return TRUE;
        }
    };

    class Sniper : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (isCrit) MUL(1.5);
        }
    };

    class MagicGuard : public virtual Ability {
        bool magicGuard() override { return true; }
    };

    class NoGuard : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override { return ACCURACY_ALWAYS_HITS; }
        AbilityApplyOnWithTarget onAccuracyFor() override { return APPLY_ON_ATTACKER_OR_TARGET; }
    };

    class Stall : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (gCurrentTurnActionNumber < GetBattlerTurnOrderNum(battler)) MUL(.7);
        }
    };

    class Technician : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (basePower <= 60) MUL(1.5);
        }
    };

    class LeafGuard : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

            CHECK(AbilityHealMonStatus(battler, ability));
            return TRUE;
        }
    };

    class MoldBreaker : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER); }
        int onMoldBreaker(ON_MOLD_BREAKER) override { return TRUE; }
    };

    class SuperLuck : public virtual Ability {
        int onCrit(ON_CRIT) override { return 1; }
    };

    class Aftermath : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class Anticipation : public virtual Breakable {
        int onEntry(ON_ENTRY) override {
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
        bool persistent() { return true; }
    };

    class Forewarn : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class Unaware : public virtual Breakable {
        bool unaware() { return true; }
    };

    class TintedLens : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessMultiplier <= UQ_4_12(.5)) RESISTANCE(2);
        }
    };

    class Filter : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        }
    };

    class SlowStart : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gVolatileStructs[battler].slowStartTimer = 5;
            return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
        }
        void onStat(ON_STAT) override {
            if (statId != STAT_ATK && statId != STAT_SPATK && statId != STAT_SPEED) return;
            if (gVolatileStructs[battler].slowStartTimer) *stat /= 2;
        }
    };

    class Scrappy : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING)
            CHECK(defType == TYPE_GHOST)
            CHECK_NOT(*mod)
            *mod = UQ_4_12(1.0);
            return TRUE;
        }
        bool tauntImmune() override { return true; }
    };

    class StormDrain : public virtual Ability<TYPE_WATER> {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_WATER);
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class IceBody : public virtual HailImmune {
        int onEndTurn(ON_END_TURN) override {
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

    class SolidRock : public virtual Filter {};

    class SnowWarning : public virtual HailImmune {
        int onEntry(ON_ENTRY) override {
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

    class HoneyGather : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK_NOT(gBattleMons[battler].item)
            CHECK(Random() % 2)

            gBattleMons[battler].item = gLastUsedItem = ITEM_HONEY;
            BattleScriptPushCursorAndCallback(BattleScript_HoneyGatherActivates);
            return TRUE;
        }
    };

    class Frisk : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class Reckless : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST) MUL(1.2);
        }
    };

    class Multitype : public virtual FormChange {};

    class FlowerGift : public virtual WeatherTransformation, public virtual Breakable {
        void onStat(ON_STAT) override {
            if (statId != STAT_SPATK && statId != STAT_SPDEF) return;
            if (IsWeatherActive(WEATHER_SUN_ANY)) *stat *= 1.5;
        }
        AbilityApplyOn onStatFor() override { return APPLY_ON_ALLY; }
    };

    class BadDreams : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            gBattleScripting.abilityPopupOverwrite = ability;
            BattleScriptPushCursorAndCallback(BattleScript_BadDreamsActivates);
            return NO_ANNOUNCE;
        }
    };

    class SheerForce : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].flags & FLAG_SHEER_FORCE_BOOST) MUL(1.3);
        }
    };

    class Contrary : public virtual Breakable {};

    class Unnerve : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE); }
        bool unnerve() override { return true; }
    };

    class Defeatist : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId != STAT_ATK && statId != STAT_SPATK) return;
            if (gBattleMons[battler].hp <= gBattleMons[battler].maxHP / 3) *stat /= 2;
        }
    };

    class CursedBody : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class Healer : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class FriendGuard : public virtual Breakable {};

    class WeakArmor : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(IS_MOVE_PHYSICAL(move))
            CHECK(CanRaiseStat(battler, STAT_SPEED) || CanLowerStat(battler, STAT_DEF))

            if (gBattleMoves[move].effect == EFFECT_HIT_ESCAPE && CanBattlerSwitch(attacker))
                gRoundStructs[battler].disableEjectPack = TRUE;  // Set flag for target

            BattleScriptCall(BattleScript_WeakArmorActivates);
            return TRUE;
        }
    };

    class LightMetal : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED) *stat *= 1.3;
        }
    };

    class Multiscale : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (BATTLER_MAX_HP(battler)) MUL(.5);
        }
    };

    class ToxicBoost : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMons[battler].status1 & STATUS1_PSN_ANY && IS_MOVE_PHYSICAL(move)) MUL(1.5);
        }
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

    class FlareBoost : public virtual Ability {
        int onEntry(ON_ENTRY) override { return FlareBoostHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        int onWeather(ON_WEATHER) override { return FlareBoostHandler(ability, battler, ABILITY_BS_CALL); }
        void onStat(ON_STAT) override {
            if (statId != STAT_SPATK) return;
            if (gBattleMons[battler].status1 & STATUS1_BURN) *stat *= 1.5;
        }
        bool negatesBurnAtkDrop() override { return true; }
    };

    class Harvest : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK_NOT(gBattleMons[battler].item)
            CHECK_NOT(gBattleStruct->changedItems[battler])
            CHECK(ItemId_GetPocket(GetUsedHeldItem(battler)) == POCKET_BERRIES)
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) || Random() % 2)

            BattleScriptPushCursorAndCallback(BattleScript_HarvestActivates);
            return TRUE;
        }
    };

    class Telepathy : public virtual Breakable {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (target == BATTLE_PARTNER(battler) && gBattleMoves[move].power) *mod = 0;
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return APPLY_ON_ATTACKER_OR_TARGET; }
    };

    class Moody : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class Overcoat : public virtual Breakable, public virtual SandImmune, public virtual HailImmune {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move)) MUL(.8);
        }
        bool powderImmune() { return true; }
    };

    class PoisonTouch : public virtual PoisonPoint {};

    class Regenerator : public virtual Ability {
        int onExit(ON_EXIT) override {
            CHECK(IsBattlerAlive(battler))
            CHECK_NOT(BATTLER_MAX_HP(battler))
            BattleScriptCall(BattleScript_RegeneratorExits);
            return FALSE;
        }
    };

    class BigPecks : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IsMoveMakingContact(move, battler)) MUL(1.3);
        }
    };

    class SandRush : public virtual Ability, public virtual SandImmune {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        }
    };

    class WonderSkin : public virtual Ability {
        bool fortKnox() override { return true; }
    };

    class Analytic : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber && gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT) MUL(1.3);
        }
    };

    class Illusion : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(DidMoveHit())
            CHECK(gBattleStruct->illusion[battler].on)
            CHECK_NOT(gBattleStruct->illusion[battler].broken)

            BattleScriptCall(BattleScript_IllusionOff);
            return TRUE;
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleStruct->illusion[battler].on && !gBattleStruct->illusion[battler].broken) MUL(1.3);
        }
    };

    class Imposter : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class Infiltrator : public virtual Ability {
        InfiltrateType onInfiltrate(ON_INFILTRATE) override { return INFILTRATE_SCREENS | INFILTRATE_SUBSTITUTE; }
    };

    class Mummy : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class Moxie : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override { return MoxieClone(battler, STAT_ATK); }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Justified : public virtual Ability {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_DARK);
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class Rattled : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(moveType == TYPE_DARK || moveType == TYPE_BUG || moveType == TYPE_GHOST)
            CHECK(CanRaiseStat(battler, STAT_SPEED))

            SetStatChanger(STAT_SPEED, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
    };

    class MagicBounce : public virtual Breakable {
        bool magicBounce() { return true; }
    };

    class SapSipper : public virtual Redirects<TYPE_GRASS> {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_GRASS);
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class Prankster : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(IS_MOVE_STATUS(move))
            return 1;
        }
    };

    class SandForce : public virtual SandImmune {
        void onStat(ON_STAT) override {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) *stat *= 1.5;
        }
    };

    class IronBarbs : public virtual RoughSkin {};

    class ZenMode : public virtual StandardTransformation {};

    class VictoryStar : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
        AbilityApplyOnWithTarget onAccuracyFor() override { return APPLY_ON_ALLY; }
    };

    class Turboblaze : public virtual MoldBreaker {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_FIRE); }
    };

    class Teravolt : public virtual MoldBreaker {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_ELECTRIC); }
    };

    class AromaVeil : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & (CHECK_INFATUATE | CHECK_RESTRICTING | CHECK_HEAL_BLOCK))
            return TRUE;
        }
        AbilityApplyOn onStatusImmuneFor() { return APPLY_ON_ALLY; }
    };

    class FlowerVeil : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            CHECK(IS_BATTLER_OF_TYPE(target, TYPE_GRASS))
            return TRUE;
        }
        AbilityApplyOn onStatusImmuneFor() override { return APPLY_ON_ALLY; }
    };

    class CheekPouch : public virtual RandomizerBanned {};

    class Protean : public virtual Ability {
        int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) override {
            CHECK(CheckAndSetOncePerTurnAbility(battler, ability))
            CHECK_NOT(IS_BATTLER_OF_TYPE(battler, moveType))
            CHECK(move != MOVE_STRUGGLE)
            SET_BATTLER_TYPE(gBattlerAttacker, moveType);
            PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
            BattleScriptCall(BattleScript_ProteanActivates);
            return TRUE;
        }
    };

    class FurCoat : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_PHYSICAL(move)) MUL(.5);
        }
    };

    class Bulletproof : public virtual Breakable {
        int onImmune(ON_IMMUNE) override {
            CHECK(gBattleMoves[move].flags & FLAG_BALLISTIC)
            CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER) *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class StrongJaw : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) MUL(1.3);
        }
    };

    class Refrigerate : public virtual Ability {
        ATE_ABILITY(TYPE_ICE),
    };

    class SweetVeil : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_SLEEP)
            return TRUE;
        }
        AbilityApplyOn onStatusImmuneFor override { return APPLY_ON_ALLY; }
        .breakable = TRUE,
    };

    class StanceChange : public virtual FormChange {
        int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) override {
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

    class GaleWings : public virtual GaleWingsLike<TYPE_FLYING> {};

    class MegaLauncher : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IsMegaLauncherBoosted(battler, move)) MUL(1.3);
        }
        bool megaLauncherBoost() override { return true; }
    };

    class GrassPelt : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_DEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) *stat *= 1.5;
        }
    };

    class ToughClaws : public virtual Ability {
        .onOffensiveMultiplier = BigPecks.onOffensiveMultiplier,
    };

    class Pixilate : public virtual Ability {
        ATE_ABILITY(TYPE_FAIRY),
    };

    class Gooey : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(StatLowerableOrMirrorArmor(attacker, STAT_SPEED))
            CHECK(IsMoveMakingContact(move, attacker))

            BattleScriptCall(BattleScript_GooeyActivates);
            gHitMarker |= HITMARKER_IGNORE_SAFEGUARD;
            return TRUE;
        }
    };

    class Aerilate : public virtual Ability {
        ATE_ABILITY(TYPE_FLYING),
    };

    class HyperAggressive : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override { return PARENTAL_BOND_HYPER_AGGRESSIVE; }
    };

    class ParentalBond : public virtual HyperAggressive {
        bool resistsFortKnox() override { return true; }
    };

    class DarkAura : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA); }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType != TYPE_DARK) return;
            if (IsAbilityOnField(ABILITY_AURA_BREAK))
                MUL(.75);
            else
                MUL(1.33);
        }
        .onOffensiveMultiplierFor = APPLY_ON_ANY,
    };

    class FairyAura : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA); }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType != TYPE_FAIRY) return;
            if (IsAbilityOnField(ABILITY_AURA_BREAK))
                MUL(.75);
            else
                MUL(1.33);
        }
        .onOffensiveMultiplierFor = APPLY_ON_ANY,
    };

    class AuraBreak : public virtual Breakable {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK); }
    };

    class PrimordialSea : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))

            BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
            return TRUE;
        }
    };

    class DesolateLand : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))

            BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
            return TRUE;
        }
    };

    class WeatherControl : public virtual Breakable {
        int onImmune(ON_IMMUNE) override {
            CHECK(gBattleMoves[move].flags & FLAG_WEATHER_BASED)
            CHECK_NOT(GetBattlerBattleMoveTargetFlags(move, attacker) & MOVE_TARGET_USER)
            *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class DeltaStream : public virtual WeatherControl {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))

            BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
            return TRUE;
        }
        bool breakable() override { return false; }
    };

    class Stamina : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class WimpOut : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(CheckHalfHpAbility(battler, attacker))
            CHECK_NOT(TestSheerForceFlag(attacker, gCurrentMove))
            CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
            CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
            CHECK(CountUsablePartyMons(battler));
            gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_EMERGENCY_EXIT;
            return FALSE;
        }
    };

    class EmergencyExit : public virtual WimpOut {};

    class WaterCompaction : public virtual Breakable {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(moveType == TYPE_WATER)
            CHECK(CanRaiseStat(battler, STAT_DEF))

            SetStatChanger(STAT_DEF, 2);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER) RESISTANCE(.5);
        }
    };

    class Merciless : public virtual Ability {
        int onCrit(ON_CRIT) override {
            if (gBattleMons[target].status1 & STATUS1_PSN_ANY) return ALWAYS_CRIT;
            if (gBattleMons[target].status1 & STATUS1_PARALYSIS) return ALWAYS_CRIT;
            if (gBattleMons[target].status1 & STATUS1_BLEED) return ALWAYS_CRIT;
            if (gBattleMons[target].statStages[STAT_SPEED] < DEFAULT_STAT_STAGE) return ALWAYS_CRIT;
            if (GetBattlerHoldEffect(target, TRUE) == HOLD_EFFECT_IRON_BALL) return ALWAYS_CRIT;
            return 0;
        }
    };

    class ShieldsDown : public virtual StandardTransformation {
        int onAttacker(ON_ATTACKER) override {
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
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
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
        bool unsuppressable() override { return true; }
    };

    class Stakeout : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gVolatileStructs[target].isFirstTurn == 2) MUL(2.0);
        }
    };

    class WaterBubble : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER) MUL(2.0);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_BURN)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class Steelworker : public virtual Breakable {
        ATE_ABILITY(TYPE_STEEL), void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (moveType == TYPE_DARK || moveType == TYPE_GHOST) *mod /= 2;
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return onAfterTypeEffectivenessFor; }
    };

    class Berserk : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class SlushRush : public virtual HailImmune {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        }
    };

    class LongReach : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_PHYSICAL(move) && !gBattleMoves[move].contact) MUL(1.2);
        }
    };

    template <Type BoostType>
    class LiquidVoiceClone : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IsSoundMove(battler, move)) MUL(1.2);
        }
        int onMoveType(ON_MOVE_TYPE) override {
            CHECK(moveType == TYPE_NORMAL)
            CHECK(gBattleMoves[move].flags & FLAG_SOUND)
            return BoostType + 1;
        }
    };
    class LiquidVoice : public virtual LiquidVoiceClone<TYPE_WATER> {};

    class Triage : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(IsHealingMoveEffect(gBattleMoves[move].effect))
            return 3;
        }
    };

    class Galvanize : public virtual Ability {
        ATE_ABILITY(TYPE_ELECTRIC),
    };

    class SurgeSurfer : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN)) *stat *= 1.5;
        }
    };

    class Schooling : public virtual StandardTransformation {
        int onEntry(ON_ENTRY) override {
            CHECK(gBattleMons[battler].level >= 20)
            return StandardTransformation::onEntry(DELEGATE_ENTRY);
        }
        int onEndTurn(ON_END_TURN) override {
            CHECK(gBattleMons[battler].level >= 20)
            return StandardTransformation::onEndTurn(DELEGATE_END_TURN);
        }
    };

    class Disguise : public virtual FormChange {
        int DisguiseReformHandler(AbilityEnum ability, int battler, AbilityCallType callType) {
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
        int onEntry(ON_ENTRY) override { return DisguiseReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        SpeciesEnum onDisguise(ON_DISGUISE) override {
            switch (gBattleMons[battler].species) {
                case SPECIES_MIMIKYU:
                    return SPECIES_MIMIKYU_BUSTED;
                case SPECIES_MIMIKYU_RAYQUAZA:
                    return SPECIES_MIMIKYU_RAYQUAZA_BUSTED;

                default:
                    return SPECIES_NONE;
            }
        }
        int onWeather(ON_WEATHER) override { return DisguiseReformHandler(ability, battler, ABILITY_BS_CALL); }
    };

    class BattleBond : public virtual FormChange {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
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
        AbilityApplyOnWithTarget onBattlerFaintsFor() override { return APPLY_ON_ATTACKER; }
    };

    class PowerConstruct : public virtual FormChange {
        int onEndTurn(ON_END_TURN) override {
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

    class Corrosion : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_POISON)
            CHECK(defType == TYPE_STEEL)
            *mod = UQ_4_12(2.0);
            return TRUE;
        }
        int onCanStatusType(ABILITY_ON_CAN_STATUS_TYPE) override {
            CHECK(status & CHECK_POISON)
            return TRUE;
        }
    };

    class Comatose : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COMATOSE;
            BattleScriptPushCursorAndCallback(BattleScript_AnnounceStatusAbility);
            return TRUE;
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
        bool unsuppressable() override { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class QueenlyMajesty : public virtual Breakable {
        int onImmune(ON_IMMUNE) override {
            CHECK_NOT(gProcessingExtraAttacks)
            CHECK(GetBattlerSide(attacker) != GetBattlerSide(battler))
            CHECK(GetMovePriority(attacker, move, battler) > 0);
            *immunityScript = BattleScript_DazzlingProtected;
            return TRUE;
        }
        AbilityApplyOn onImmuneFor() override { return APPLY_ON_ALLY; }
    };

    class InnardsOut : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK_NOT(IsBattlerAlive(battler))
            CHECK_NOT(IsMagicGuardProtected(attacker))

            gBattleMoveDamage = gTurnStructs[battler].dmg;
            BattleScriptCall(BattleScript_AftermathDmg);
            return TRUE;
        }
    };

    class Dancer : public virtual Ability {
        int onCopyMove(ON_COPY_MOVE) override {
            CHECK(IsDance(attacker, move))
            return UseOutOfTurnAttack(battler, target, ability, move, 0);
        }
    };

    class Battery : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move)) MUL(1.3);
        }
        .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
    };

    class Fluffy : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        }
    };

    class Dazzling : public virtual QueenlyMajesty {};

    class SoulHeart : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(ChangeStatBuffs(battler, 1, STAT_SPATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

            BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ANY; }
    };

    class TanglingHair : public virtual Gooey {};

    class Receiver : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
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
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ALLY; }
    };

    class PowerOfAlchemy : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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
        int onReactive(ON_REACTIVE) override {
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
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            int state = GetAbilityState(battler, ability);
            if (state & (3 << fainted)) SetAbilityState(battler, ability, state & ~(3 << fainted));
            return NO_ANNOUNCE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_OTHER; }
    };

    class BeastBoost : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override { return MoxieClone(battler, GetHighestStatId(battler, FALSE)); }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class RksSystem : public virtual Protean, public virtual Adaptability, public virtual FormChange {};

    class ElectricSurge : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

            for (int i = 0; i < gBattlersCount; i++) {
                DisableSwitchInAbility(i, ABILITY_GENERATOR);
                DisableSwitchInAbility(i, ABILITY_ENERGIZED);
            }
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
    };

    class PsychicSurge : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESPSYCHIC;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_PSYCHIC,
    };

    class MistySurge : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESMISTY;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_MISTY,
    };

    class GrassySurge : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESGRASSY;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_GRASSY,
    };

    class ShadowShield : public virtual Multiscale {
        bool breakable() override { return false; }
    };

    class PrismArmor : public virtual Filter {
        bool breakable() override { return false; }
    };

    class Neuroforce : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.35);
        }
    };

    class IntrepidSword : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(CanRaiseStat(battler, STAT_ATK))

            SetStatChanger(STAT_ATK, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class DauntlessShield : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(CanRaiseStat(battler, STAT_DEF))

            SetStatChanger(STAT_DEF, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class Libero : public virtual Protean {};

    class CottonDown : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(DidMoveHit());
            gStackBattler1 = BATTLE_OPPOSITE(battler);
            CHECK(IsBattlerAlive(gStackBattler1) || IsBattlerAlive(BATTLE_PARTNER(gStackBattler1)))

            gEffectBattler = battler;
            gStackBattler1 = GetOppositeSide(battler);
            BattleScriptCall(BattleScript_CottonDownActivates);
            return TRUE;
        }
    };

    class MirrorArmor : public virtual Breakable {};

    class GulpMissile : public virtual FormChange {
        int onAttacker(ON_ATTACKER) override {
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
        int onDefender(ON_DEFENDER) override {
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

    class SteamEngine : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(CanRaiseStat(battler, STAT_SPEED))
            CHECK(moveType == TYPE_FIRE || moveType == TYPE_WATER)

            SetStatChanger(STAT_SPEED, 12);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
    };

    class PunkRock : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IsSoundMove(battler, move)) MUL(1.3);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IsSoundMove(attacker, move)) MUL(.5);
        }
    };

    class SandSpit : public virtual SandImmune {
        int onDefender(ON_DEFENDER) override {
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

    class IceScales : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move)) MUL(.5);
        }
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
    class IceFace : public virtual FormChange, public virtual HailImmune {
        int onEntry(ON_ENTRY) override { return IceFaceReformHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        SpeciesEnum onDisguise(ON_DISGUISE) override { return gBattleMons[battler].species == SPECIES_EISCUE ? SPECIES_EISCUE_NOICE_FACE : SPECIES_NONE; }
        int onWeather(ON_WEATHER) override { return IceFaceReformHandler(ability, battler, ABILITY_BS_CALL); }
    };

    class PowerSpot : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override { MUL(1.3); }
        .onOffensiveMultiplierFor = APPLY_ON_ALLY_ONLY,
    };

    class Mimicry : public virtual Ability {
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

        int onEntry(ON_ENTRY) override {
            CHECK(IsBattlerAlive(battler))

            return HandleMimicry(battler, ability, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
        }
        int onTerrain(ON_TERRAIN) override {
            CHECK(IsBattlerAlive(battler))

            return HandleMimicry(battler, ability, ABILITY_BS_CALL);
        }
    };

    class ScreenCleaner : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryRemoveScreens(battler))

            return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
        }
    };

    class SteelySpirit : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_STEEL) MUL(1.3);
        }
        .onOffensiveMultiplierFor = APPLY_ON_ALLY,
    };

    class PerishBody : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class WanderingSpirit : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class GorillaTactics : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.5);
        }
    };

    class NeutralizingGas : public virtual Ability {
        bool unsuppressable() override { return true; }
    };

    class PastelVeil : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class HungerSwitch : public virtual FormChange {
        int onEndTurn(ON_END_TURN) override {
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

    class CuriousMedicine : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(IsDoubleBattle())
            CHECK(IsBattlerAlive(BATTLE_PARTNER(battler)))
            CHECK(TryResetBattlerStatChanges(BATTLE_PARTNER(battler), RESET_ALL_STATS))

            gEffectBattler = BATTLE_PARTNER(battler);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CURIOUS_MEDICINE;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
            return TRUE;
        }
    };

    class Transistor : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ELECTRIC) MUL(1.5);
        }
    };

    class DragonsMaw : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_DRAGON) MUL(1.5);
        }
    };

    class ChillingNeigh : public virtual Moxie {};

    class GrimNeigh : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override { return MoxieClone(battler, STAT_SPATK); }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class AsOneIceRider : public virtual Unsuppressable, public virtual RandomizerBanned, public virtual Unnerve, public virtual ChillingNeigh {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE); }
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(ChillingNeigh::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
            gBattleScripting.abilityPopupOverwrite = ABILITY_CHILLING_NEIGH;
            BattleScriptCall(BattleScript_AbilityPopUpStack);
            return NO_ANNOUNCE;
        }
    };

    class AsOneShadowRider : public virtual AsOneIceRider, public virtual GrimNeigh {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(GrimNeigh::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
            gBattleScripting.abilityPopupOverwrite = ABILITY_GRIM_NEIGH;
            BattleScriptCall(BattleScript_AbilityPopUpStack);
            return NO_ANNOUNCE;
        }
    };

    class Chloroplast : public virtual Ability {
        bool chloroplast() override { return true; }
    };

    class Whiteout : public virtual HailImmune {
        void onStat(ON_STAT) override {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) *stat *= 1.5;
        }
    };

    class Pyromancy : public virtual Ability {
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            if (moveEffect == MOVE_EFFECT_BURN) *effectChance *= 5;
        }
    };

    class KeenEdge : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST) MUL(1.3);
        }
    };

    class PrismScales : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move)) MUL(.7);
        }
    };

    class PowerFists : public virtual IronFist {
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
            CHECK(IsIronFistBoosted(battler, move))
            return STAT_SPDEF;
        }
        ,
    };

    class SandSong : public virtual LiquidVoiceClone<TYPE_GROUND> {};

    class Rampage : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            SetAbilityState(battler, ability, TRUE);
            gVolatileStructs[battler].rechargeTimer = 0;
            gBattleMons[battler].status2 &= ~(STATUS2_RECHARGE);
            return FALSE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Vengeance : public virtual SwarmLike<TYPE_GHOST> {};

    class BlitzBoxer : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(IsIronFistBoosted(battler, move))
            CHECK(BATTLER_MAX_HP(battler));
            return 1;
        }
    };

    class AntarcticBird : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FLYING || moveType == TYPE_ICE) MUL(1.3);
        }
    };

    class Immolate : public virtual Ability {
        ATE_ABILITY(TYPE_FIRE),
    };

    class Crystallize : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ICE && gBattleStruct->ateBoost[battler]) MUL(1.1);
        }
        int onMoveType(ON_MOVE_TYPE) override {
            CHECK(moveType == TYPE_ROCK)
            *ateBoost = TRUE;
            return TYPE_ICE + 1;
        }
    };

    class Electrocytes : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ELECTRIC) MUL(1.25);
        }
    };

    class Aerodynamics : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_FLYING);
            *statId = STAT_SPEED;
            return ABSORB_RESULT_STAT;
        }
    };

    class ChristmasSpirit : public virtual Breakable, public virtual HailImmune {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY)) MUL(.5);
        }
    };

    class ExploitWeakness : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (HasAnyStatusOrAbility(target)) MUL(1.25);
        }
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
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

    class GroundShock : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(defType == TYPE_GROUND)
            CHECK_NOT(*mod)
            *mod = UQ_4_12(.5);
            return TRUE;
        }
    };

    class AncientIdol : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override { *atkStatToUse = IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF; }
    };

    class MysticPower : public virtual Ability {
        int onStab(ON_STAB) override { return TRUE; }
    };

    class Perfectionist : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(gBattleMoves[move].power <= 25)
            CHECK(gBattleMoves[move].power);
            return 1;
        }
        int onCrit(ON_CRIT) override {
            CHECK(gBattleMoves[move].power <= 50)
            CHECK(gBattleMoves[move].power)
            return 1;
        }
    };

    class GrowingTooth : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_AttackBoostActivates);
            return TRUE;
        }
    };

    class Inflatable : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(CanRaiseStat(battler, STAT_DEF) || CanRaiseStat(battler, STAT_SPDEF))
            CHECK(moveType == TYPE_FIRE || moveType == TYPE_FLYING);
            BattleScriptCall(BattleScript_InflatableActivates);
            gBattleScripting.battler = battler;
            return TRUE;
        }
    };

    class AuroraBorealis : public virtual HailImmune {
        int onStab(ON_STAB) override { return moveType == TYPE_ICE; }
    };

    class Avenger : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gSideTimers[GET_BATTLER_SIDE(battler)].retaliateTimer) MUL(1.5);
        }
    };

    class LetsRoll : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(CanRaiseStat(battler, STAT_DEF))

            SetStatChanger(STAT_DEF, 1);
            gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class LoudBang : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeConfused(target))
            CHECK(IsSoundMove(battler, move))
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_CONFUSION);
        }
    };

    class LeadCoat : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_PHYSICAL(move)) MUL(.6);
        }
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED) *stat *= .9;
        }
    };

    class Amphibious : public virtual Ability {
        int onStab(ON_STAB) override { return moveType == TYPE_WATER; }
        .onStatusImmune = +[](ABILITY_ON_STATUS_IMMUNE) -> int {
            CHECK(status & CHECK_DRENCH)
            return TRUE;
        },
    };

    class Grounded : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_GROUND); }
    };

    class Earthbound : public virtual SwarmLike<TYPE_GROUND> {};

    class FightingSpirit : public virtual AteAbility<TYPE_FIGHTING> {
        ATE_ABILITY(TYPE_FIGHTING),
    };

    class FelineProwess : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPATK) *stat *= 2;
        }
    };

    class CoilUp : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

            gStatuses4[battler] |= STATUS4_COILED;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
            return TRUE;
        }
    };

    class Fossilized : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ROCK) MUL(1.2);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ROCK) RESISTANCE(.5);
        }
    };

    class MagicalDust : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK_NOT(IS_BATTLER_OF_TYPE(attacker, TYPE_PSYCHIC))

            gBattleMons[attacker].type3 = TYPE_PSYCHIC;
            PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMons[attacker].type3);
            BattleScriptCall(BattleScript_AttackerBecameTheType);
            return TRUE;
        }
    };

    class Dreamcatcher : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            for (int i = 0; i < gBattlersCount; i++) {
                if (IsBattlerAlive(i) && gBattleMons[i].status1 & STATUS1_SLEEP) {
                    FILTER_NOT(gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability && gQueuedExtraAttackData[0].target == i)
                    MUL(2.0);
                    return;
                }
            }
        }
        int onPreemptAction(ON_PREEMPT_ACTION) override {
            CHECK(gBattleMons[turnBattler].status1 & STATUS1_SLEEP)
            return UseTurnAttackAsPursuit(DELEGATE_PREEMPT_ACTION);
        }
    };

    class Nocturnal : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_DARK) MUL(1.25);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_DARK || moveType == TYPE_FAIRY) RESISTANCE(.75);
        }
    };

    class SelfSufficient : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class Tectonize : public virtual Ability {
        ATE_ABILITY(TYPE_GROUND),
    };

    class IceAge : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_ICE); }
    };

    class HalfDrake : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_DRAGON); }
    };

    class Aquatic : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_WATER); }
    };

    class Liquified : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER) RESISTANCE(2);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        }
    };

    class Dragonfly : public virtual HalfDrake, public virtual GroundImmune {};

    class Dragonslayer : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(target, TYPE_DRAGON)) RESISTANCE(1.5);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_DRAGON)) MUL(.5);
        }
    };

    class Mountaineer : public virtual Breakable {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (moveType == TYPE_ROCK) *mod = 0;
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return onAfterTypeEffectivenessFor; }
        AbilityApplyOnWithTarget stealthRockImmune() override { return true; }
    };

    class Hydrate : public virtual Ability {
        ATE_ABILITY(TYPE_WATER),
    };

    class Metallic : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_STEEL); }
    };

    class Permafrost : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.65);
        }
    };

    class PrimalArmor : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessModifier >= UQ_4_12(2.0)) MUL(.5);
        }
    };

    class RagingBoxer : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            CHECK(IsIronFistBoosted(battler, move))
            return PARENTAL_BOND_PRIMAL_MAW;
        }
    };

    class AirBlower : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class Juggernaut : public virtual Breakable {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_DEF] += 20;
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_PARALYSIS)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class ShortCircuit : public virtual SwarmLike<TYPE_ELECTRIC> {};

    class MajesticBird : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPATK) *stat *= 1.5;
        }
    };

    class Phantom : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_GHOST); }
    };

    class Intoxicate : public virtual Ability {
        ATE_ABILITY(TYPE_POISON),
    };

    class Impenetrable : public virtual Ability {
        bool magicGuard() override { return true; }
    };

    class Hypnotist : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(move == MOVE_HYPNOSIS);
            *accuracy *= 1.5;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Overwhelm : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_DRAGON) CHECK(defType == TYPE_FAIRY) CHECK_NOT(*mod) *mod = UQ_4_12(1.0);
            return TRUE;
        }
        bool tauntImmune() override { return true; }
    };

    class Scare : public Intimidate {};

    class MajesticMoth : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class SoulEater : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK_NOT(BATTLER_MAX_HP(battler));
            CHECK(CanBattlerHeal(battler));
            BattleScriptCall(BattleScript_HandleSoulEaterEffect);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class SoulLinker : public virtual Ability {
        ON_EITHER {
            CHECK(ShouldApplyOnHitAffect(opponent))
            CHECK(IsBattlerAlive(battler))
            CHECK_NOT(BATTLER_HAS_ABILITY(opponent, ABILITY_SOUL_LINKER))
            CHECK(move != MOVE_PAIN_SPLIT)

            BattleScriptCall(BattleScript_AttackerSoulLinker);
            return TRUE;
        }
    };

    class SweetDreams : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class BadLuck : public virtual Breakable {
        int onCrit(ON_CRIT) override { return NEVER_CRIT; }
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            if (*effectChance < 1) *effectChance = 0;
        }
        AbilityApplyOnWithTarget onCritFor() override { return APPLY_ON_FOE; }
        AbilityApplyOn onModifyEffectChanceFor() override { return APPLY_ON_FOE; }
    };

    class HauntedSpirit : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class ElectricBurst : public virtual Ability {
        int onRecoil(ON_RECOIL) override {
            CHECK(moveType == TYPE_ELECTRIC);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 20, 1);
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ELECTRIC) MUL(1.35);
        }
    };

    class RawWood : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_GRASS) MUL(1.2);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_GRASS) RESISTANCE(.5);
        }
    };

    class Solenoglyphs : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
        }
    };

    class SpiderLair : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

            int side = GetOppositeSide(battler);
            gSideTimers[side].started.spiderWeb = TRUE;
            gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
            gSideTimers[side].stickyWebTimer = 5;
            BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
            return TRUE;
        }
    };

    class FatalPrecision : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK_NOT(IS_MOVE_STATUS(move))
            CHECK(CalcTypeEffectivenessMultiplier(move, moveType, battler, target, TRUE) >= UQ_4_12(2.0))
            return ACCURACY_HITS_IF_POSSIBLE;
        }
        int onCrit(ON_CRIT) override {
            CHECK(typeEffectiveness >= UQ_4_12(2.0))
            return ALWAYS_CRIT;
        }
    };

    class FortKnox : public virtual Ability {
        bool fortKnox() override { return true; }
    };

    class Seaweed : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_GRASS && IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(2);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS)) RESISTANCE(0.5);
        }
    };

    class PsychicMind : public virtual SwarmLike<TYPE_PSYCHIC> {};

    class PoisonAbsorb : public virtual Redirects<TYPE_POISON> {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_POISON)
            return ABSORB_RESULT_HEAL;
        }
        int onEndTurn(ON_END_TURN) override {
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

    class Scavenger : public virtual SoulEater {};

    class TwistedDimension : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)

            gFieldTimers.started.trickRoom = TRUE;
            gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
            gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
            return TRUE;
        }
    };

    class MultiHeaded : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            if (gBaseStats[gBattleMons[battler].species].flags & F_TWO_HEADED) return PARENTAL_BOND_HYPER_AGGRESSIVE;
            if (gBaseStats[gBattleMons[battler].species].flags & F_THREE_HEADED) return PARENTAL_BOND_THREE_HEADED;
            return MULTIHIT_SINGLE;
        }
        bool resistsFortKnox() override { return true; }
    };

    class NorthWind : public virtual HailImmune {
        int onEntry(ON_ENTRY) override {
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

    class Overcharge : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(defType == TYPE_ELECTRIC)
            *mod = UQ_4_12(2.0);
            return TRUE;
        }
        int onCanStatusType(ABILITY_ON_CAN_STATUS_TYPE) override {
            CHECK(status & CHECK_PARALYSIS)
            return TRUE;
        }
    };

    class ViolentRush : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
        }
    };

    class FlamingSoul : public virtual GaleWingsLike<TYPE_FIRE> {};

    class SagePower : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move)) MUL(1.5);
        }
    };

    class BoneZone : public virtual Ability {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
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

    class SpeedForce : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            if (gBattleMoves[move].contact) secondaryAtkStatToUse[STAT_SPEED] += 20;
        }
    };

    class SeaGuardian : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

            int stat = GetHighestStatId(battler, TRUE);
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            SetStatChanger(stat, 1);
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class MoltenDown : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_FIRE)
            CHECK(defType == TYPE_ROCK)
            *mod = UQ_4_12(2.0);
            return TRUE;
        }
    };

    class Flock : public virtual Ability {
        .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FLYING),
    };

    class FieldExplorer : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].flags & FLAG_FIELD_BASED) MUL(1.5);
        }
    };

    class Striker : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IsStrikerBoosted(battler, move)) MUL(1.3);
        }
    };

    class FrozenSoul : public virtual GaleWingsLike<TYPE_ICE> {};

    class Predator : public virtual Ability {
        .onBattlerFaints = SoulEater.onBattlerFaints, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Looter : public virtual Ability {
        .onBattlerFaints = SoulEater.onBattlerFaints, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class LunarEclipse : public virtual Ability {
        int onStab(ON_STAB) override { return moveType == TYPE_DARK || moveType == TYPE_FAIRY; }
        .onAccuracy = Hypnotist.onAccuracy,
    };

    class SolarFlare : public virtual Ability {
        .onOffensiveMultiplier = Immolate.onOffensiveMultiplier, .onMoveType = Immolate.onMoveType, int onStab(ON_STAB) override {
            return moveType == TYPE_FIRE;
        }
        bool chloroplast() override { return true; }
    };

    class PowerCore : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override { secondaryAtkStatToUse[IS_MOVE_PHYSICAL(move) ? STAT_DEF : STAT_SPDEF] += 20; }
    };

    class SightingSystem : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override { return ACCURACY_HITS_IF_POSSIBLE; }
        .onPriority = +[](ON_PRIORITY) -> int {
            CHECK(gBattleMoves[move].accuracy)
            CHECK(gBattleMoves[move].accuracy < 80);
            return -3;
        },
    };

    class BadCompany : public virtual RandomizerBanned {};

    class Opportunist : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(gBattleMons[target].hp <= gBattleMons[target].maxHP / 2)
            return 1;
        }
    };

    class GiantWings : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].airBased) MUL(1.3);
        }
    };

    class Momentum : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            if (gBattleMoves[move].contact) *atkStatToUse = STAT_SPEED;
        }
    };

    class GripPincer : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(gBattleMons[target].status2 & STATUS2_WRAPPED)
            return ACCURACY_ALWAYS_HITS;
        }
    };

    class BigLeaves : public virtual Ability {
        int onEndTurn(ON_END_TURN) override { return Harvest.onEndTurn(DELEGATE_END_TURN) | LeafGuard.onEndTurn(DELEGATE_END_TURN); }
        void onStat(ON_STAT) override {
            SolarPower.onStat(DELEGATE_STAT);
            Chlorophyll.onStat(DELEGATE_STAT);
        }
        bool chloroplast() override { return true; }
    };

    class PreciseFist : public virtual Ability {
        int onCrit(ON_CRIT) override {
            CHECK(IsIronFistBoosted(battler, move))
            return 1;
        }
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            if (IsIronFistBoosted(battler, move)) *effectChance *= 5;
        }
    };

    class Deadeye : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].arrowBased)
            return ACCURACY_HITS_IF_POSSIBLE;
        }
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
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

    class Artillery : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IsMegaLauncherBoosted(battler, move))
            return ACCURACY_HITS_IF_POSSIBLE;
        }
    };

    class Amplifier : public virtual Ability {
        .onOffensiveMultiplier = PunkRock.onOffensiveMultiplier,
    };

    class IceDew : public virtual Redirects<TYPE_ICE> {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_ICE);
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class SunWorship : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

            int stat = GetHighestStatId(battler, TRUE);
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class Pollinate : public virtual Ability {
        ATE_ABILITY(TYPE_BUG),
    };

    class VolcanoRage : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_FIRE)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_ERUPTION, 50);
        }
    };

    class ColdRebound : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICY_WIND, 0);
            return FALSE;
        }
    };

    class LowBlow : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_FEINT_ATTACK, 40); }
    };

    class Spectralize : public virtual Ability {
        ATE_ABILITY(TYPE_GHOST)
    };

    class SpectralShroud : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(gBattleStruct->ateBoost[battler])
            CHECK(moveType == TYPE_GHOST)
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
        }
        .onOffensiveMultiplier = Spectralize.onOffensiveMultiplier, .onMoveType = Spectralize.onMoveType,
    };

    class Discipline : public virtual Breakable {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_CONFUSION)
            return TRUE;
        }
        bool removesStatusOnImmunity() override { return true; }
        bool tauntImmune() override { return true; }
    };

    class Thundercall : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_SMITE, .2 * gBattleMoves[MOVE_SMITE].power);
        }
    };

    class MarineApex : public virtual Ability {
        .onInfiltrate = Infiltrator.onInfiltrate, .onOffensiveMultiplier = +[](ON_OFFENSIVE_MULTIPLIER) {
            if (IS_BATTLER_OF_TYPE(target, TYPE_WATER)) RESISTANCE(1.5);
        },
    };

    class MightyHorn : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].hornBased) MUL(1.3);
        }
    };

    class HardenedSheath : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gBattleMoves[move].hornBased)
            CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptCall(BattleScript_AttackBoostActivates);
            gBattleScripting.battler = battler;
            return TRUE;
        }
    };

    class ArcticFur : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override { MUL(.65); }
    };

    class Lethargy : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            TryResetBattlerStatChanges(battler, RESET_ALL_STATS);
            gVolatileStructs[battler].slowStartTimer = 5;
            BattleScriptPushCursorAndCallback(BattleScript_LethargyEnters);
            return TRUE;
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
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

    class IronBarrage : public virtual Ability {
        .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier, .onAccuracy = SightingSystem.onAccuracy, .onPriority = SightingSystem.onPriority,
        bool megaLauncherBoost() override {
            return true;
        }
    };

    class SteelBarrel : public virtual Ability {
        .onStatusImmune = RockHead.onStatusImmune, .noRecoil = TRUE, .removesStatusOnImmunity = TRUE,
    };

    class PyroShells : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(IsMegaLauncherBoosted(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_OUTBURST, 50);
        }
    };

    class FungalInfection : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class Parry : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_MACH_PUNCH, 0);
            return FALSE;
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override { MUL(.8); }
    };

    class Scrapyard : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(DidMoveHit())
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

            BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
            return TRUE;
        }
    };

    class LooseQuills : public virtual Ability {
        .onDefender = Scrapyard.onDefender,
    };

    class ToxicDebris : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(DidMoveHit())
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].toxicSpikesAmount < 2)

            BattleScriptCall(BattleScript_DefenderSetsToxicSpikeLayer);
            return TRUE;
        }
    };

    class Roundhouse : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IsStrikerBoosted(battler, move))
            return ACCURACY_HITS_IF_POSSIBLE;
        }
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
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

    class Mineralize : public virtual Ability {
        ATE_ABILITY(TYPE_ROCK),
    };

    class LooseRocks : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(DidMoveHit())
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STEALTH_ROCK)

            BattleScriptCall(BattleScript_DefenderSetsStealthRock);
            return TRUE;
        }
    };

    class SpinningTop : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class RetributionBlow : public virtual Ability {
        int onReactive(ON_REACTIVE) override {
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

    class Fearmonger : public Intimidate {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeParalyzed(battler, target))
            CHECK(IsMoveMakingContact(move, battler))
            CHECK(Random() % 100 < 10)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
        }
    };

    class ToxicSpill : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
            return TRUE;
        }
        int onEndTurn(ON_END_TURN) override {
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
        int onExit(ON_EXIT) override {
            CHECK_NOT(getMonotypeChampType() == TYPE_POISON)
            BattleScriptCall(BattleScript_TheToxicWasHasDissapeared);
            return TRUE;
        }
    };

    class DesertCloak : public virtual Breakable, public virtual SandImmune {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY))
            return TRUE;
        }
        AbilityApplyOn onStatusImmuneFor override { return APPLY_ON_ALLY; }
    };

    class Draconize : public virtual Ability {
        ATE_ABILITY(TYPE_DRAGON),
    };

    class PrettyPrincess : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (!IsUnaware(battler) && HasAnyLoweredStat(target)) MUL(1.5);
        }
    };

    class SelfRepair : public virtual Ability {
        .onEndTurn = SelfSufficient.onEndTurn, .onExit = NaturalCure.onExit,
    };

    class Electromorphosis : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
            BattleScriptCall(BattleScript_ElectromorphosisActivates);
            return TRUE;
        }
    };

    class AtomicBurst : public virtual Ability {
        .onDefender = Electromorphosis.onDefender, ATE_ABILITY(TYPE_ELECTRIC),
    };

    class Hellblaze : public virtual Ability {
        .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_FIRE),
    };

    class Riptide : public virtual Ability {
        .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_WATER),
    };

    class ForestRage : public virtual Ability {
        .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GRASS),
    };

    class PrimalMaw : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            return PARENTAL_BOND_PRIMAL_MAW;
        }
    };

    class SweepingEdge : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            return ACCURACY_HITS_IF_POSSIBLE;
        }
    };

    class GiftedMind : public virtual Breakable {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IS_MOVE_STATUS(move))
            return ACCURACY_HITS_IF_POSSIBLE;
        }
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (moveType == TYPE_BUG || moveType == TYPE_GHOST || moveType == TYPE_DARK) *mod = 0;
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return onAfterTypeEffectivenessFor; }
    };

    class HydroCircuit : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK_NOT(BATTLER_MAX_HP(battler))
            CHECK(CanBattlerHeal(battler))
            CHECK(moveType == TYPE_WATER)

            gBattleMoveDamage = -gHpDealt / 4;
            if (!gBattleMoveDamage) gBattleMoveDamage = -1;
            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        }
        .onOffensiveMultiplier = Transistor.onOffensiveMultiplier,
    };

    class Equinox : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            int atk = CalculateStat(battler, STAT_ATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
            int spAtk = CalculateStat(battler, STAT_SPATK, 0, move, TRUE, ignoreOffensiveStatDrops, targetUnaware, FALSE);
            if (atk > spAtk)
                *atkStatToUse = STAT_ATK;
            else if (spAtk > atk)
                *atkStatToUse = STAT_SPATK;
        }
    };

    class Absorbant : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class Clueless : public virtual Ability {
        .onEntry = CloudNine.onEntry, .unsuppressable = TRUE,
    };

    class CheatingDeath : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            int uses = 2 - GetSingleUseAbilityCounter(battler, ability);
            CHECK(uses)

            if (uses == 1)
                BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
            else if (uses > 1) {
                ConvertIntToDecimalStringN(gBattleTextBuff4, uses, STR_CONV_MODE_LEFT_ALIGN, 2);
                BattleScriptPushCursorAndCallback(BattleScript_BattlerHasNoDamageHits);
            }
            return TRUE;
        }
        .noDamageHits = 2, .persistent = TRUE,
    };

    class CheapTactics : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_SCRATCH, 0); }
    };

    class Coward : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

            SetSingleUseAbilityCounter(battler, ability, TRUE);
            gRoundStructs[battler].protectedThisTurn = TRUE;
            BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
            return TRUE;
        }
        bool persistent() override { return true; }
    };

    class VoltRush : public virtual GaleWingsLike<TYPE_ELECTRIC> {};

    class DuneTerror : public virtual Breakable, public virtual SandImmune {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_GROUND) MUL(1.2);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) MUL(.65);
        }
    };

    class InfernalRage : public virtual Ability {
        int onRecoil(ON_RECOIL) override {
            CHECK(moveType == TYPE_FIRE);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 20, 1);
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) MUL(1.35);
        }
    };

    class DualWield : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            CHECK(IsMegaLauncherBoosted(battler, move) || gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
            return PARENTAL_BOND_DUAL_WIELD;
        }
    };

    class ElementalCharge : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class Ambush : public virtual Ability {
        int onCrit(ON_CRIT) override {
            CHECK(gVolatileStructs[battler].isFirstTurn)
            return ALWAYS_CRIT;
        }
    };

    class Atlas : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

            gFieldTimers.started.gravity = TRUE;
            gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
            gFieldStatuses |= STATUS_FIELD_GRAVITY;
            BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
            return TRUE;
        }
    };

    class Radiance : public virtual Ability {
        int onImmune(ON_IMMUNE) override {
            CHECK(moveType == TYPE_DARK);
            *immunityScript = BattleScript_RadianceProtected;
            return TRUE;
        }
        .onAccuracy = Illuminate.onAccuracy, .onImmuneFor = APPLY_ON_ANY, .breakable = TRUE,
    };

    class JawsOfCarnage : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK_NOT(BATTLER_MAX_HP(battler))
            CHECK(CanBattlerHeal(battler))
            if (gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST)
                BattleScriptCall(BattleScript_HandleJawsOfCarnageEffect);
            else
                BattleScriptCall(BattleScript_HandleSoulEaterEffect);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class AngelsWrath : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
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
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
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
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            if (move == MOVE_POISON_STING) *effectChance = 100;
        }
        int onCanStatusType(ABILITY_ON_CAN_STATUS_TYPE) override {
            CHECK(status & CHECK_POISON)
            CHECK(move == MOVE_POISON_STING)
            return TRUE;
        }
    };

    class PrismaticFur : public virtual Ability {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override { MUL(.5); }
        int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) override {
            if (battler == attacker && Protean.onBeforeAttack(DELEGATE_ON_BEFORE_ATTACK)) return TRUE;
            return ColorChange.onBeforeAttack(DELEGATE_ON_BEFORE_ATTACK);
        }
        .onBeforeAttackFor = APPLY_ON_ATTACKER_OR_TARGET,
    };

    class ShockingJaws : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeParalyzed(battler, target))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
        }
    };

    class FaeHunter : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(target, TYPE_FAIRY)) RESISTANCE(1.5);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_FAIRY)) RESISTANCE(.5);
        }
    };

    class GravityWell : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)

            gFieldTimers.started.gravity = TRUE;
            gFieldTimers.gravityTimer = GRAVITY_DURATION;
            gFieldStatuses |= STATUS_FIELD_GRAVITY;
            BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
            return TRUE;
        }
    };

    class Evaporate : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_WATER)
            return ABSORB_RESULT_EVAPORATE;
        }
    };

    class Lumberjack : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(target, TYPE_GRASS)) RESISTANCE(1.5);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_GRASS)) RESISTANCE(.5);
        }
    };

    class WellBakedBody : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_FIRE);
            *statId = STAT_DEF;
            return ABSORB_RESULT_STAT;
        }
        bool absorbUp2() { return true; }
    };

    class Furnace : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
            CHECK(gSideTimers[GetBattlerSide(battler)].stealthRockType == TYPE_ROCK)
            CHECK(IsBattlerAlive(battler))
            CHECK(ChangeStatBuffs(battler, 2, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(moveType == TYPE_ROCK)
            CHECK(CanRaiseStat(battler, STAT_SPEED))

            SetStatChanger(STAT_SPEED, 2);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
    };

    class RockyPayload : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ROCK || gBattleMoves[move].throwingBased) MUL(1.5);
        }
    };

    class EarthEater : public virtual Breakable {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_GROUND)
            return ABSORB_RESULT_HEAL;
        }
    };

    class LingeringAroma : public virtual Ability {
        .onDefender = Mummy.onDefender,
    };

    class FairyTale : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_FAIRY); }
    };

    class RagingMoth : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            CHECK(moveType == TYPE_FIRE)
            return PARENTAL_BOND_DUAL_WIELD;
        }
    };

    class AdrenalineRush : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override { return MoxieClone(battler, STAT_SPEED); }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Archmage : public virtual RandomizerBanned {
        int onAttacker(ON_ATTACKER) override {
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

    class Cryomancy : public virtual Ability {
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            if (moveEffect == MOVE_EFFECT_FROSTBITE) *effectChance *= 5;
        }
    };

    class PhantomPain : public virtual Ability {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_GHOST)
            CHECK(defType == TYPE_NORMAL)
            CHECK_NOT(*mod)
            *mod = UQ_4_12(1.0);
            return TRUE;
        }
    };

    class Purgatory : public virtual Ability {
        .onOffensiveMultiplier = BOOSTED_SWARM_MULTIPLIER(TYPE_GHOST),
    };

    class Emanate : public virtual Ability {
        ATE_ABILITY(TYPE_PSYCHIC),
    };

    class KunoichiBlade : public virtual Ability {
        .onOffensiveMultiplier = Technician.onOffensiveMultiplier, .skillLink = TRUE,
    };

    class MonkeyBusiness : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_TICKLE, 0); }
    };

    class CombatSpecialist : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            IronFist.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Striker.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    class JunglesGuard : public virtual Ability {
        .onStatusImmune = FlowerVeil.onStatusImmune, .onStatusImmuneFor = FlowerVeil.onStatusImmuneFor, .breakable = TRUE,
    };

    class HuntersHorn : public virtual Ability {
        .onBattlerFaints = SoulEater.onBattlerFaints, .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
        AbilityApplyOnWithTarget onBattlerFaintsFor override {
            return APPLY_ON_ATTACKER;
        }
    };

    class PixiePower : public virtual Ability {
        .onEntry = FairyAura.onEntry, .onOffensiveMultiplier = FairyAura.onOffensiveMultiplier, AccuracyPriority onAccuracy(ON_ACCURACY) override {
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
        .onOffensiveMultiplierFor = APPLY_ON_ANY,
    };

    class PlasmaLamp : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC) MUL(1.2);
        }
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(moveType == TYPE_FIRE || moveType == TYPE_ELECTRIC)
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class MagmaEater : public virtual Ability {
        .onBattlerFaints = SoulEater.onBattlerFaints, .onTypeEffectiveness = MoltenDown.onTypeEffectiveness,
        AbilityApplyOnWithTarget onBattlerFaintsFor override {
            return APPLY_ON_ATTACKER;
        }
    };

    class SuperHotGoo : public virtual Ability {
        .onAttacker = FlameBody.onAttacker, int onDefender(ON_DEFENDER) override {
            return Gooey.onDefender(DELEGATE_DEFENDER) | FlameBody.onDefender(DELEGATE_DEFENDER);
        }
    };

    class Nika : public virtual Ability {
        .onOffensiveMultiplier = IronFist.onOffensiveMultiplier,
    };

    class Archer : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].arrowBased) MUL(1.3);
        }
    };

    class SuperSlammer : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].hammerBased) MUL(1.3);
        }
    };

    class InverseRoom : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)

            gFieldTimers.started.inverseRoom = TRUE;
            gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
            gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
            BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
            return TRUE;
        }
    };

    class FrostBurn : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_FIRE)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_ICE_BEAM, 40);
        }
    };

    class ItchyDefense : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class Generator : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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
        int onTerrain(ON_TERRAIN) override {
            CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)
            CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))

            gStackBattler1 = battler;
            BattleScriptCall(BattleScript_GeneratorActivatesRet);
            return TRUE;
        }
        int onExit(ON_EXIT) override {
            CHECK(gStatuses3[battler] & STATUS3_CHARGED_UP)
            SetSingleUseAbilityCounter(battler, ability, FALSE);
            return FALSE;
        }
        bool persistent() override { return true; }
    };

    class MoonSpirit : public virtual Ability {
        int onStab(ON_STAB) override { return moveType == TYPE_FAIRY || moveType == TYPE_DARK; }
    };

    class DustCloud : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_SAND_ATTACK, 0); }
    };

    class TippingPoint : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class BerserkerRage : public virtual Ability {
        .onDefender = TippingPoint.onDefender, .onBattlerFaints = Rampage.onBattlerFaints, AbilityApplyOnWithTarget onBattlerFaintsFor override {
            return APPLY_ON_ATTACKER;
        }
    };

    class Trickster : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_DISABLE, 0); }
    };

    class SandGuard : public virtual Breakable, public virtual SandImmune {
        int onImmune(ON_IMMUNE) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY));
            return QueenlyMajesty.onImmune(DELEGATE_IMMUNE);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move) && IsBattlerWeatherAffected(attacker, WEATHER_SANDSTORM_ANY)) MUL(.5);
        }
    };

    class NaturalRecovery : public virtual Ability {
        int onExit(ON_EXIT) override { return NaturalCure.onExit(DELEGATE_EXIT) | Regenerator.onExit(DELEGATE_EXIT); }
    };

    class WindRider : public virtual Breakable {
        int onEntry(ON_ENTRY) override {
            CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND)
            CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE)))

            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityHighestAttackingStatRaiseOnSwitchIn);
            return TRUE;
        }
        int onAbsorb(ON_ABSORB) override {
            CHECK(gBattleMoves[move].airBased)
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class SoothingAroma : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class PrimAndProper : public virtual Ability {
        .onDefender = CuteCharm.onDefender, .fortKnox = TRUE,
    };

    class SuperStrain : public virtual Ability {
        int onRecoil(ON_RECOIL) override {
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_STRAIN;
            return max(damage / 4, 1);
        }
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(ChangeStatBuffs(battler, -1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS | MOVE_EFFECT_CERTAIN, NULL))
            BattleScriptCall(BattleScript_LowerStatOnFaintingTarget);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Enlightened : public virtual Ability {
        .onOffensiveMultiplier = Emanate.onOffensiveMultiplier, .onMoveType = Emanate.onMoveType, .onAccuracy = InnerFocus.onAccuracy, .breakable = TRUE,
        bool tauntImmune() override {
            return true;
        }
    };

    class PeacefulSlumber : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            if (!SweetDreams.onEndTurn(DELEGATE_END_TURN)) return SelfSufficient.onEndTurn(DELEGATE_END_TURN);
            gBattleMoveDamage -= gBattleMons[battler].maxHP / 16;
            return TRUE;
        }
    };

    class Aftershock : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(gBattleMoves[move].power)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_MAGNITUDE, 65);
        }
    };

    ON_EITHER(FreezingPoint) {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanGetFrostbite(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_FROSTBITE, battler, opponent);
        return TRUE;
    }
    class FreezingPoint : public virtual Ability {
        ON_EITHER_ABILITY(FreezingPoint),
    };

    class CryoProficiency : public virtual FreezingPoint, public virtaul CryoProficiency {
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
        int onDefender(ON_DEFENDER) override {
            return FreezingPoint::onDefender(DELEGATE_DEFENDER) | CryoProficiencyHail(ability, battler, attacker, move, moveType);
        }
    };

    class ArcaneForce : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.1);
        }
        .onStab = MysticPower.onStab,
    };

    class Doombringer : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_DOOM_DESIRE, 0); }
    };

    class Wishmaker : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            int counter = GetSingleUseAbilityCounter(battler, ability);
            CHECK(counter < 3)
            CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))

            SetSingleUseAbilityCounter(battler, ability, counter + 1);
            return TRUE;
        }
        bool persistent() override { return true; }
    };

    class YukiOnna : public Intimidate {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanInfatuate(battler, target))
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
        }
    };

    class Suppress : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_TORMENT, 0); }
    };

    class Refrigerator : public virtual Ability {
        .onDefensiveMultiplier = Filter.onDefensiveMultiplier, .onAccuracy = Illuminate.onAccuracy,
    };

    class HeavenAsunder : public virtual Ability {
        void onCrit(ON_CRIT) override {
            if (move == MOVE_SPACIAL_REND) return ALWAYS_CRIT;
            return 1;
        }
    };

    class PurifyingWaters : public virtual Ability {
        .onEntry = WaterVeil.onEntry, .onEndTurn = Hydration.onEndTurn, .onStatusImmune = WaterVeil.onStatusImmune, .breakable = TRUE,
        bool removesStatusOnImmunity() override {
            return true;
        }
    };

    class Seaborne : public virtual Ability {
        .onEntry = Drizzle.onEntry, .onStat = SwiftSwim.onStat,
    };

    class HighTide : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_WATER)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_SURF, 50);
        }
    };

    class ChangeOfHeart : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_HEART_SWAP, 0); }
    };

    class MysticBlades : public virtual Ability {
        .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier, .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
            CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST);
            return TRUE;
        },
    };

    class Determination : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (HasAnyStatusOrAbility(battler) && IS_MOVE_SPECIAL(move)) MUL(1.5);
        }
        bool negatesFrzSpatkDrop() override { return true; }
    };

    class Fertilize : public virtual Ability {
        ATE_ABILITY(TYPE_GRASS),
    };

    class PureLove : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK_NOT(BATTLER_MAX_HP(battler))
            CHECK(CanBattlerHeal(battler))
            CHECK(gBattleMons[target].status2 & STATUS2_INFATUATION)

            gBattleMoveDamage = -gHpDealt / 4;
            if (!gBattleMoveDamage) gBattleMoveDamage = -1;
            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        }
        .onDefender = CuteCharm.onDefender, .canInfatuateAny = TRUE,
    };

    class Fighter : public virtual Ability {
        .onOffensiveMultiplier = SWARM_MULTIPLIER(TYPE_FIGHTING),
    };

    class Telekinetic : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_TELEKINESIS, 0); }
    };

    class Combustion : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) MUL(1.5);
        }
    };

    class PonyPower : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            KeenEdge.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MysticBlades.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
        .onSwapSplit = MysticBlades.onSwapSplit,
    };

    class PowderBurst : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_POWDER, 0); }
    };

    class Retriever : public virtual Ability {
        int onExit(ON_EXIT) override {
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

    class MonsterMash : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_TRICK_OR_TREAT, 0); }
    };

    class TwoStep : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(IsDance(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_REVELATION_DANCE, 50);
        }
    };

    class Spiteful : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(move != MOVE_STRUGGLE)
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK(gBattleMons[attacker].pp[gChosenMovePos])

            BattleScriptCall(BattleScript_AbilitySpiteful);
            return TRUE;
        }
    };

    class Fortitude : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class Devourer : public virtual PrimalMaw, public virtual StrongJaw {};

    class PhantomThief : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_SPECTRAL_THIEF, 40); }
    };

    class EarlyGrave : public virtual GaleWingsLike<TYPE_GHOST> {};

    class BassBoosted : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Amplifier.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            PunkRock.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
        .onDefensiveMultiplier = PunkRock.onDefensiveMultiplier, .breakable = TRUE,
    };

    class FlamingJaws : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeBurned(target))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_BURN);
        }
    };

    class MonsterHunter : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(target, TYPE_DARK)) RESISTANCE(1.5);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_DARK)) MUL(.5);
        }
    };

    class CrownedSword : public virtual Ability {
        .onEntry = IntrepidSword.onEntry, .onDefender = AngerPoint.onDefender,
    };

    class CrownedShield : public virtual Ability {
        .onEntry = DauntlessShield.onEntry, .onDefender = Stamina.onDefender,
    };

    class BerserkDna : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class CrownedKing : public virtual AsOneShadowRider, public virtual AsOneIceRider {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING); }
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(ChillingNeigh::onBattlerFaints(DELEGATE_BATTLER_FAINTS) | GrimNeigh::onBattlerFaints(DELEGATE_BATTLER_FAINTS))
            gBattleScripting.abilityPopupOverwrite = ABILITY_CROWNED_KING;
            BattleScriptCall(BattleScript_AbilityPopUpStack);
            return NO_ANNOUNCE;
        }
    };

    class SnapTrapWhenHit : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_SNAP_TRAP, 50);
            return FALSE;
        }
    };

    class Permanence : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE); }
    };

    class Hubris : public virtual Ability {
        .onBattlerFaints = GrimNeigh.onBattlerFaints, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class CosmicDaze : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMons[target].status2 & STATUS2_CONFUSION) MUL(2);
        }
    };

    class MindsEye : public virtual Ability {
        .onTypeEffectiveness = Scrappy.onTypeEffectiveness, .breakable = TRUE,
    };

    class BloodPrice : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
            CHECK_NOT(IS_MOVE_STATUS(gLastResultingMoves[battler]))
            CHECK_NOT(IsMagicGuardProtected(battler))
            CHECK(IsBattlerAlive(battler))

            gBattleMoveDamage = gBattleMons[battler].maxHP / 10;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            BattleScriptPushCursorAndCallback(BattleScript_AbilitySelfDamage);
            return TRUE;
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override { MUL(1.3); }
    };

    ON_EITHER(SpikeArmor) {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBleed(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))
        CHECK(Random() % 100 < 30)

        AbilityStatusEffectSafe(MOVE_EFFECT_BLEED, battler, opponent);
        return TRUE;
    }
    class SpikeArmor : public virtual Ability {
        ON_EITHER_ABILITY(SpikeArmor),
    };

    class VoodooPower : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IS_MOVE_SPECIAL(move))
            CHECK(CanBleed(attacker))
            CHECK(Random() % 100 < 30)

            AbilityStatusEffect(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_BLEED);
            return TRUE;
        }
    };

    class ChromeCoat : public virtual Ability {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_SPECIAL(move)) MUL(.6);
        }
        .onStat = LeadCoat.onStat, .breakable = TRUE,
    };

    class Banshee : LiquidVoiceClone<TYPE_GHOST> {};

    class WebSpinner : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_STRING_SHOT, 0); }
    };

    class ShowdownMode : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
        }
    };

    class SeedSower : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))

            BattleScriptCall(BattleScript_SeedSower);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_GRASSY,
    };

    class Airborne : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FLYING) MUL(1.3);
        }
        .onOffensiveMultiplierFor = APPLY_ON_ALLY,
    };

    class Parroting : public virtual Breakable {
        .onImmune = Soundproof.onImmune, int onCopyMove(ON_COPY_MOVE) override {
            CHECK(IsSoundMove(attacker, move))
            return UseOutOfTurnAttack(battler, target, ability, move, 0);
        }
        bool isSoundproof() { return true; }
    };

    class SaltCircle : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class PurifyingSalt : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_GHOST) RESISTANCE(.5);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
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
    class Protosynthesis : public virtual Ability {
        int onEntry(ON_ENTRY) override { return ProtosynthesisHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        int onWeather(ON_WEATHER) override { return ProtosynthesisHandler(ability, battler, ABILITY_BS_CALL); }
        .onStat = +[](ON_STAT) {
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
    class QuarkDrive : public virtual Ability {
        int onEntry(ON_ENTRY) override { return QuarkDriveHandler(ability, battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        int onTerrain(ON_TERRAIN) override { return QuarkDriveHandler(ability, battler, ABILITY_BS_CALL); }
        .onStat = Protosynthesis.onStat,
    };

    class WindPower : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gBattleMoves[move].airBased)
            CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

            gStatuses3[battler] |= STATUS3_CHARGED_UP;
            BattleScriptCall(BattleScript_ElectromorphosisActivates);
            return TRUE;
        }
    };

    class Impulse : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            if (!(gBattleMoves[move].contact)) *atkStatToUse = STAT_SPEED;
        }
    };

    class TerminalVelocity : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            if (IS_MOVE_SPECIAL(move)) secondaryAtkStatToUse[STAT_SPEED] += 20;
        }
    };

    class AngerShell : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(CheckHalfHpAbility(battler, attacker))
            CHECK_NOT(GetAbilityState(battler, ability))
            CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK) || CanRaiseStat(battler, STAT_SPEED))

            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_AngerShell);
            return TRUE;
        }
    };

    class Egoist : public virtual Ability {
        int onReactive(ON_REACTIVE) override {
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

    class ReadiedAction : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
        }
    };

    class DarkGaleWings : public virtual GaleWingsLike<TYPE_DARK> {};

    class GuiltTrip : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK_NOT(IsBattlerAlive(battler))
            CHECK(CanLowerStat(attacker, STAT_ATK) || CanLowerStat(attacker, STAT_SPATK))

            BattleScriptCall(BattleScript_GuiltTrip);
            return TRUE;
        }
    };

    class WaterGaleWings : public virtual GaleWingsLike<TYPE_WATER> {};

    class ZeroToHero : public virtual FormChange {
        int onEntry(ON_ENTRY) override {
            CHECK(gBattleMons[battler].species == SPECIES_PALAFIN)
            CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
            CHECK(GetSingleUseAbilityCounter(battler, ability))

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_PALAFIN_HERO);
            gBattleMons[battler].species = SPECIES_PALAFIN_HERO;
            BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
            return TRUE;
        }
        int onExit(ON_EXIT) override {
            SetSingleUseAbilityCounter(battler, ability, TRUE);
            return FALSE;
        }
    };

    class Costar : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class Commander : public virtual FormChange {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(GetAbilityState(battler, ability))

            SetAbilityState(battler, ability, COMMANDER_NOT_ACTIVE);
            gStatuses3[battler] &= ~STATUS3_SEMI_INVULNERABLE;
            BattleScriptCall(BattleScript_CommanderEnds);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor() override { return APPLY_ON_ALLY; }
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(GetAbilityState(target, ability))
            return ACCURACY_ALWAYS_MISSES;
        }
        AbilityApplyOnWithTarget onAccuracyFor() override { return APPLY_ON_TARGET; }
    };

    class EjectPackAbility : public virtual Ability {
        bool persistent() override { return true; }
    };

    class VengefulSpirit : public virtual Ability {
        .onDefender = HauntedSpirit.onDefender, .onOffensiveMultiplier = Vengeance.onOffensiveMultiplier,
    };

    class CudChew : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class ArmorTail : public virtual Ability {
        .onImmune = QueenlyMajesty.onImmune, .onImmuneFor = APPLY_ON_ALLY, .breakable = TRUE,
    };

    class MindCrush : public virtual Ability {
        .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier, .onChooseOffensiveStat = +[](ON_CHOOSE_OFFENSIVE_STAT) {
            if (gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST) *atkStatToUse = STAT_SPATK;
        },
    };

    class SupremeOverlord : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(gFaintedMonCount[GetBattlerSide(battler)])

            return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
        }
        void onStat(ON_STAT) override {
            if (statId == STAT_ATK || statId == STAT_SPATK) *stat = *stat * (10 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 10;
        }
    };

    class IllWill : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class FireScales : public virtual Ability {
        .onDefensiveMultiplier = IceScales.onDefensiveMultiplier, .breakable = TRUE,
    };

    class WatchYourStep : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            u8 targetSide = GetOppositeSide(battler);
            CHECK(gSideTimers[targetSide].spikesAmount < 3)

            gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
            gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
            BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
            return TRUE;
        }
    };

    class RapidResponse : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
        }
    };

    class DoubleIronBarbs : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class ThermalExchange : public virtual Breakable {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(moveType == TYPE_FIRE)
            CHECK(CanRaiseStat(battler, STAT_ATK))

            SetStatChanger(STAT_ATK, 1);
            BattleScriptCall(BattleScript_TargetAbilityStatRaiseOnMoveEnd);
            return TRUE;
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_BURN)
            return TRUE;
        }
        bool removesStatusOnImmunity() { return true; }
    };

    class GoodAsGold : public virtual Breakable {
        int onImmune(ON_IMMUNE) override {
            CHECK(battler != attacker) CHECK(IS_MOVE_STATUS(move));
            *immunityScript = BattleScript_SoundproofProtected;
            return TRUE;
        }
    };

    class SharingIsCaring : public virtual Ability {
        int onReactive(ON_REACTIVE) override {
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

    class TabletsOfRuin : public virtual Ability {
        void onStat(ON_STAT) override { RuinEffect(STAT_ATK, battler, statId, stat, flags); }
        .onStatFor = APPLY_ON_OTHER, .ruinStat = STAT_ATK,
    };

    class SwordOfRuin : public virtual Ability {
        void onStat(ON_STAT) override { RuinEffect(STAT_DEF, battler, statId, stat, flags); }
        .onStatFor = APPLY_ON_OTHER, .ruinStat = STAT_DEF,
    };

    class VesselOfRuin : public virtual Ability {
        void onStat(ON_STAT) override { RuinEffect(STAT_SPATK, battler, statId, stat, flags); }
        .onStatFor = APPLY_ON_OTHER, .ruinStat = STAT_SPATK,
    };

    class BeadsOfRuin : public virtual Ability {
        void onStat(ON_STAT) override { RuinEffect(STAT_DEF, battler, statId, stat, flags); }
        .onStatFor = APPLY_ON_OTHER, .ruinStat = STAT_DEF,
    };

    class PermafrostClone : public virtual Ability {
        .onDefensiveMultiplier = Permafrost.onDefensiveMultiplier, .breakable = TRUE,
    };

    class Gallantry : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

            BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
            return TRUE;
        }
        .noDamageHits = 1, .breakable = TRUE, .persistent = TRUE,
    };

    class OrichalcumPulse : public virtual Ability {
        .onEntry = Drought.onEntry, .onStat = +[](ON_STAT) {
            if (statId != STAT_ATK) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) *stat = *stat * 4 / 3;
        },
    };

    class SunBasking : public virtual Breakable {
        int onImmune(ON_IMMUNE) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY));
            return QueenlyMajesty.onImmune(DELEGATE_IMMUNE);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY) && IS_MOVE_PHYSICAL(move)) MUL(.5);
        }
    };

    class WingedKing : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (typeEffectivenessMultiplier >= UQ_4_12(2.0)) MUL(1.33);
        }
    };

    class HadronEngine : public virtual Ability {
        .onEntry = ElectricSurge.onEntry, void onStat(ON_STAT) override {
            if (statId == STAT_SPATK && IsBattlerTerrainAffected(battler, STATUS_FIELD_ELECTRIC_TERRAIN)) *stat = *stat * 4 / 3;
        }
        .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
    };

    class IronSerpent : public virtual Ability {
        .onOffensiveMultiplier = WingedKing.onOffensiveMultiplier,
    };

    class SweepingEdgePlus : public virtual Ability {
        .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier, .onAccuracy = SweepingEdge.onAccuracy,
    };

    class CelestialBlessing : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class MinionControl : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override { return PARENTAL_BOND_MINION_CONTROL; }
    };

    class MoltenBlades : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeBurned(target))
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_BURN);
        }
        .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    };

    class HauntingFrenzy : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanMoveHaveExtraFlinchChance(move))
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffectDirect(MOVE_EFFECT_FLINCH);
        }
        .onBattlerFaints = AdrenalineRush.onBattlerFaints, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class NoiseCancel : public virtual Ability {
        .onImmune = Soundproof.onImmune, .onImmuneFor = APPLY_ON_ALLY, .breakable = TRUE, .isSoundproof = TRUE,
    };

    class RadioJam : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeDisabled(target))
            CHECK(IsSoundMove(battler, move))
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_DISABLE);
        }
    };

    class Ole : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
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
        AbilityApplyOnWithTarget onAccuracyFor() override { return onAccuracyFor; }
    };

    class Malicious : public Intimidate {};

    class DeadPower : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK_NOT(gBattleMons[target].status2 & STATUS2_CURSED)
            CHECK(IsMoveMakingContact(move, battler))
            CHECK(Random() % 100 < 20)

            return AbilityStatusEffect(MOVE_EFFECT_CURSE);
        }
        void onStat(ON_STAT) override {
            if (statId == STAT_ATK) *stat *= 1.5;
        }
    };

    class BrawlingWyvern : public virtual Ability {
        .onAccuracy = NoGuard.onAccuracy, int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
            CHECK(flag == MOVE_FLAG_PUNCH)
            CHECK(IS_MOVE_TYPE(move, TYPE_DRAGON))
            return TRUE;
        }
        .onAccuracyFor = APPLY_ON_ATTACKER_OR_TARGET,
    };

    class JunshiSanda : public virtual Ability {
        int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
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

    class MythicalArrows : public virtual Ability {
        .onOffensiveMultiplier = Archer.onOffensiveMultiplier, .onSwapSplit = +[](ON_SWAP_SPLIT) -> int {
            CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
            CHECK(gBattleMoves[move].arrowBased);
            return TRUE;
        },
    };

    class Lawnmower : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

            BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
            return TRUE;
        }
    };

    class Flourish : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_GRASS && IsBattlerTerrainAffected(battler, STATUS_FIELD_GRASSY_TERRAIN)) MUL(1.5);
        }
    };

    class DesertSpirit : public virtual SandStream {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (*mod == 0 && !IsBattlerGrounded(target) && moveType == TYPE_GROUND && IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY)) {
                *mod = UQ_4_12(1.0);
            }
        }
    };

    class Contempt : public virtual Ability {
        bool unaware() override { return true; }
    };

    class Aerialist : public virtual Levitate, public virtual Flock {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Levitate::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Flock::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    class TeraShell : public virtual Breakable {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (*mod >= UQ_4_12(1.0) && BATTLER_MAX_HP(battler)) *mod = UQ_4_12(0.5);
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return onAfterTypeEffectivenessFor; }
    };

    class ToxicChain : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_TOXIC);
        }
    };

    class ParasiticSpores : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gVolatileStructs[battler].parasiticSpores)

            gVolatileStructs[battler].parasiticSpores = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
        }
    };

    template <MoveEffectEnum Effect>
    class PoisonPuppeteerLike : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            int state = GetAbilityState(battler, ability);
            if (state & (1 << fainted)) SetAbilityState(battler, ability, state ^ (1 << fainted));
            return NO_ANNOUNCE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_OTHER; }
        virtual MoveEffectEnum setStateOnEffect() override { return Effect; }
    };

    class PoisonPuppeteer : public virtual PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
        int onReactive(ON_REACTIVE) override {
            return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) -> int { return CanBeConfused(target); }, BattleScript_PoisonPuppeteer);
        }
    };

    class Entrance : public virtual PoisonPuppeteerLike<MOVE_EFFECT_CONFUSION> {
        int onReactive(ON_REACTIVE) override { return PoisonPuppeteerClone(ability, battler, CanInfatuate, BattleScript_Entrance); }
    };

    class Rejection : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gFieldTimers.quashTimer)

            gFieldTimers.quashTimer = QUASH_DURATION;
            gFieldTimers.started.quash = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
        }
    };

    class AppleEnlightenment : public virtual Ability {
        .onDefensiveMultiplier = FurCoat.onDefensiveMultiplier, .breakable = TRUE, .magicGuard = TRUE,
    };

    class BalloonBomber : public virtual Ability {
        int onDefender(ON_DEFENDER) override { return Aftermath.onDefender(DELEGATE_DEFENDER) || Inflatable.onDefender(DELEGATE_DEFENDER); }
    };

    class FlamingMaw : public virtual Ability {
        .onAttacker = FlamingJaws.onAttacker, .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    };

    class Demolitionist : public virtual Ability {
        .onEntry = ReadiedAction.onEntry, InfiltrateType onInfiltrate(ON_INFILTRATE) override {
            if (gVolatileStructs[battler].readiedAction && !IS_MOVE_STATUS(move)) return INFILTRATE_BREAK_SCREENS;
            return INFILTRATE_NONE;
        }
        int onAttacker(ON_ATTACKER) override {
            CHECK(DidMoveHit())
            CHECK(gVolatileStructs[battler].readiedAction)
            int opposingSide = GetBattlerSide(target);
            CHECK(gSideTimers[opposingSide].reflectTimer || gSideTimers[opposingSide].lightscreenTimer || gSideTimers[opposingSide].auroraVeilTimer)
            BattleScriptCall(BattleScript_AttackerShattersScreens);
            return TRUE;
        }
    };

    class RockhardWill : public virtual Ability {
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
    class FragrantDaze : public virtual Ability {
        ON_EITHER_ABILITY(FragrantDaze),
    };

    class LowVisibility : public virtual Ability {
        int onEntry(ON_ENTRY) override {
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

    class OldMariner : public virtual Seaweed, public virtual Amphibious {};

    class Ectoplasm : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId != GetHighestAttackingStatId(battler, TRUE)) return;
            if (IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        }
    };

    class BeautifulMusic : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(Random() % 2)
            CHECK(IsSoundMove(battler, move))

            return AbilityStatusEffect(MOVE_EFFECT_ATTRACT);
        }
        bool canInfatuateAny() override { return true; }
    };

    class SnowSong : LiquidVoiceClone<TYPE_ICE> {};

    class GreaterSpirit : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

            int stat = GetHighestStatId(battler, TRUE);
            CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
            BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
            return TRUE;
        }
    };

    class Resonance : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(IsSoundMove(battler, move))
            CHECK(Random() % 100 < 50)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class EtherealRush : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY)) *stat *= 1.5;
        }
    };

    class CuteAntecedence : public virtual GaleWingsLike<TYPE_FAIRY> {};

    class RecurringNightmare : public virtual Ability {
        int onRevive(ON_REVIVE) override {
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
            return B_MSG_FADE_OUT;
        }
        bool persistent() override { return true; }
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
    class MenacingSituation : public virtual Ability {
        ON_EITHER_ABILITY(MenacingSituation),
    };

    class ShinyLightning : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            if (move == MOVE_THUNDER) return ACCURACY_HITS_IF_POSSIBLE;
            *accuracy *= 1.2;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class Terrify : public Intimidate {};

    class IceDownfall : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_CRASH, 60);
            return FALSE;
        }
    };

    class LastStand : public virtual Breakable {
        void onStat(ON_STAT) override {
            if (statId == STAT_DEF || statId == STAT_SPDEF)
                *stat = *stat + (*stat * 60 * (gBattleMons[battler].maxHP - gBattleMons[battler].hp) / gBattleMons[battler].maxHP / 100);
        }
    };

    class PyroclasticFlow : public virtual MoltenDown, public virtual Corrosion {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            return MoltenDown::onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS) || Corrosion::onTypeEffectiveness(DELEGATE_TYPE_EFFECTIVENESS);
        }
    };

    class BloodBath : public virtual PoisonPuppeteerLike<MOVE_EFFECT_BLEED>, public virtual Breakable {
        int onReactive(ON_REACTIVE) override {
            return PoisonPuppeteerClone(
                ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_BLEED)
            return TRUE;
        }
        bool removesStatusOnImmunity() override { return true; }
    };

    class BattleAura : public virtual Ability {
        int onCrit(ON_CRIT) override { return 2; }
        AbilityApplyOnWithTarget onCritFor() override { return APPLY_ON_ANY; }
    };

    class Bloodlust : public virtual Ability {
        .onReactive = BloodBath.onReactive, int onBattlerFaints(ON_BATTLER_FAINTS) override {
            int result = 0;
            if (battler == attacker) {
                result |= SoulEater.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
            }
            return result | BloodBath.onBattlerFaints(DELEGATE_BATTLER_FAINTS);
        }
        .onStatusImmune = BloodBath.onStatusImmune, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ANY; }
        .breakable = TRUE, .removesStatusOnImmunity = TRUE,
    };

    class PiercingSolo : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(IsSoundMove(battler, move))

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class Rhythmic : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override { MulModifier(modifier, UQ_4_12(1.0) + 10 * gBattleStruct->sameMoveTurns[battler]); }
    };

    class ChunkyBassLine : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(IsSoundMove(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_EARTHQUAKE, 40);
        }
    };

    class DualHammer : public virtual Ability {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            CHECK(gBattleMoves[move].hammerBased)
            return PARENTAL_BOND_DUAL_WIELD;
        }
    };

    class DentingBlows : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(gBattleMoves[move].hammerBased)
            CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

            int affected = GetOncePerTurnAbilityCounter(battler, ability);
            CHECK_NOT(affected & (1 << target))

            SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
            return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
        }
    };

    class IceColdHunter : public virtual HailImmune {
        MultihitType onParentalBond(ON_PARENTAL_BOND) override {
            CHECK(moveType == TYPE_ICE)
            CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
            return PARENTAL_BOND_ICE_COLD_HUNTER;
        }
    };

    class SoulCrusher : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].hammerBased) MUL(1.1);
        }
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
            CHECK(gBattleMoves[move].hammerBased)
            return STAT_SPDEF;
        }
    };

    class ArcFlash : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBeParalyzed(battler, target))
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_PARALYSIS);
        }
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(CanBeBurned(attacker))
            CHECK(Random() % 2)

            AbilityStatusEffect(MOVE_EFFECT_BURN | MOVE_EFFECT_AFFECTS_USER);
            return TRUE;
        }
    };

    class Unicorn : public virtual Ability {
        .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier, ATE_ABILITY(TYPE_FAIRY),
    };

    class OnTheProwl : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
        }
    };

    class Pretentious : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(gVolatileStructs[battler].critBoost < 3);
            gVolatileStructs[battler].critBoost++;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CRIT_INCREASE_1;
            BattleScriptCall(BattleScript_AbilityBoostsCrit);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class VenoblazePincers : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_PHYSICAL(move)) MUL(1.2);
        }
    };

    class EternalBlessing : public virtual Ability {
        .onEndTurn = CelestialBlessing.onEndTurn, .onExit = Regenerator.onExit, .persistent = TRUE,
    };

    class SugarRush : public virtual Ability {
        .onStat = Unburden.onStat
    };

    class PeacefulRest : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class WhiteNoise : public virtual PeacefulRest, public virtual Static {};

    class SmokeyManeuvers : public virtual Breakable {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(IsBattlerWeatherAffected(target, WEATHER_FOG_ANY));
            *accuracy /= 1.25;
            return ACCURACY_MULTIPLICATIVE;
        }
        AbilityApplyOnWithTarget onAccuracyFor() override { return onAccuracyFor; }
    };

    class PowerMetal : LiquidVoiceClone<TYPE_STEEL> {};

    class PowerEdge : public virtual Ability {
        .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier,
    };

    class Superconductor : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_NORMAL && gBattleStruct->ateBoost[battler]) MUL(1.1);
        }
        int onMoveType(ON_MOVE_TYPE) override {
            CHECK(moveType == TYPE_STEEL)
            *ateBoost = TRUE;
            return TYPE_ELECTRIC + 1;
        }
    };

    class UltraInstinct : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_VACUUM_WAVE, 0);
            return FALSE;
        }
        .onDefensiveMultiplier = Parry.onDefensiveMultiplier,
    };

    class UnlockedPotential : public virtual Ability {
        .onDefender = Berserk.onDefender, .onAccuracy = InnerFocus.onAccuracy, .tauntImmune = TRUE,
    };

    class HigherRank : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (GetMovePriority(battler, move, target) > 0) MUL(1.2);
        }
    };

    class FuneralPyre : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE); }
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

    class FlameBubble : public virtual Ability {
        .onOffensiveMultiplier = WaterBubble.onOffensiveMultiplier, .onDefensiveMultiplier = WaterBubble.onDefensiveMultiplier,
        .onPriority = FlamingSoul.onPriority, .onStatusImmune = WaterBubble.onStatusImmune, .breakable = TRUE, .removesStatusOnImmunity = TRUE,
    };

    class ElementalVortex : public virtual Ability {
        int onAbsorb(ON_ABSORB) override { return WaterAbsorb.onAbsorb(DELEGATE_ABSORB) || FlashFire.onAbsorb(DELEGATE_ABSORB); }
        .onOffensiveMultiplier = FlashFire.onOffensiveMultiplier,
    };

    class SnowyWrath : public virtual SnowWarning, public virtual Cryomancy {};

    class PatternChange : public virtual Ability {
        .onEndTurn = ShedSkin.onEndTurn, .onBeforeAttack = Protean.onBeforeAttack,
    };

    class NoTurningBack : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(CheckHalfHpAbility(battler, attacker))
            CHECK_NOT(GetAbilityState(battler, ability))
            CHECK_NOT(gVolatileStructs[battler].noRetreat || gBattleMons[battler].status2 & STATUS2_ESCAPE_PREVENTION)

            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_NoTurningBack);
            return TRUE;
        }
    };

    class FlammableCoat : public virtual FormChange {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler) || (gBattleResources->flags->flags[battler] & RESOURCE_FLAG_FLASH_FIRE))
            CHECK(moveType == TYPE_FIRE)
            CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
            CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

            UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_LUMBERING_SLOTH_ENGULFED);
            gBattleMons[battler].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
            BattleScriptCall(BattleScript_TargetFormChange);
            return TRUE;
        }
        int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) override {
            CHECK(moveType == TYPE_FIRE)
            CHECK(gBattleMons[battler].species == SPECIES_LUMBERING_SLOTH)
            CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)

            UpdateAbilityStateIndicesForNewSpecies(gBattlerAttacker, SPECIES_LUMBERING_SLOTH_ENGULFED);
            gBattleMons[gBattlerAttacker].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
            BattleScriptCall(BattleScript_AttackerFormChange);
            return TRUE;
        }
    };

    class DracoMorale : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_DRAGON_CHEER, 0); }
    };

    class BadOmen : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (isCrit) MUL(.25);
        }
    };

    class MoshPit : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMoves[move].flags & FLAG_RECKLESS_BOOST)
                MUL(1.25);
            else
                MUL(1.5);
        }
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
    class BloodStain : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN); }
        ON_EITHER_ABILITY(BloodStain), int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
        bool unsuppressable() override { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class BloodStigma : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (gBattleMons[target].status1 & STATUS1_BLEED || IsBloodStainAffected(target)) MUL(2);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
        bool unsuppressable() override { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class Slipstream : public virtual Ability {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override { secondaryAtkStatToUse[STAT_SPEED] += 20; }
    };

    class MaximumAcceleration : public virtual Ability {
        .onEndTurn = SpeedBoost.onEndTurn, .onChooseOffensiveStat = Slipstream.onChooseOffensiveStat,
    };

    class Sidewinder : public virtual Ability {
        .onEntry = CoilUp.onEntry, int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(gBattleMoves[gCurrentMove].flags & FLAG_STRONG_JAW_BOOST || !(gStatuses4[battler] & STATUS4_COILED))
            gStatuses4[battler] |= STATUS4_COILED;
            SetAbilityState(battler, ability, TRUE);
            BattleScriptCall(BattleScript_BattlerCoiledUpReturnNoPopup);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Petrify : public Intimidate {
        int onEntry(ON_ENTRY) override {
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

    class Fluffiest : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) RESISTANCE(2.0);
            if (IsMoveMakingContact(move, attacker)) MUL(0.5);
        }
    };

    class WayOfPrecision : public virtual Ability {
        .onAccuracy = InnerFocus.onAccuracy, .onCrit = PreciseFist.onCrit, .onModifyEffectChance = PreciseFist.onModifyEffectChance, .breakable = TRUE,
        bool tauntImmune() override {
            return true;
        }
    };

    class WayOfSwiftness : public virtual Ability {
        .onBattlerFaints = Pretentious.onBattlerFaints, .onStat = SwiftSwim.onStat, AbilityApplyOnWithTarget onBattlerFaintsFor override {
            return APPLY_ON_ATTACKER;
        }
    };

    class AtomicPunch : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            IronFist.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            SteelySpirit.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    class IronGiant : public virtual Breakable {
        .onDefensiveMultiplier = Heatproof.onDefensiveMultiplier, .onChooseOffensiveStat = Juggernaut.onChooseOffensiveStat,
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_PARALYSIS)
            return TRUE;
        }
        bool negatesBurnAtkDrop() { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class MasterHand : public virtual Ability {
        .onBattlerFaints = Rampage.onBattlerFaints, .onOffensiveMultiplier = MegaLauncher.onOffensiveMultiplier,
        AbilityApplyOnWithTarget onBattlerFaintsFor override {
            return APPLY_ON_ATTACKER;
        }
        bool megaLauncherBoost() override { return true; }
    };

    class FinalBlow : public virtual Ability {
        .onAccuracy = FatalPrecision.onAccuracy, .onCrit = FatalPrecision.onCrit,
    };

    class Hospitality : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            gBattlerTarget = BATTLE_PARTNER(battler);
            CHECK(IsBattlerAlive(gBattlerTarget))
            CHECK_NOT(BATTLER_MAX_HP(gBattlerTarget))

            gBattleMoveDamage = -gBattleMons[gBattlerTarget].maxHP / 4;
            if (!gBattleMoveDamage) gBattleMoveDamage = -1;
            BattleScriptPushCursorAndCallback(BattleScript_Hospitality_AfterPopup);
            return TRUE;
        }
    };

    class ButterUp : public virtual Ability {
        int onEntry(ON_ENTRY) override { return Hospitality.onEntry(DELEGATE_ENTRY) | SoothingAroma.onEntry(DELEGATE_ENTRY); }
    };

    class VitalityStrike : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class HugeWings : public virtual GiantWings, public virtual Levitate {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            GiantWings::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Levitate::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    class SwordOfDamnation : public virtual Ability {
        .onStat = SwordOfRuin.onStat, .onStatFor = APPLY_ON_OTHER, .ruinStat = STAT_DEF, .unaware = TRUE,
    };

    class RestrainingOrder : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(GetAbilityState(battler, ability) == RESTRAINING_ORDER_NOT_TRIGGERED)
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(CanBattlerSwitch(battler) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
            CHECK_NOT(gBattleTypeFlags & BATTLE_TYPE_ARENA)
            CHECK(CountUsablePartyMons(battler))

            SetAbilityState(battler, ability, RESTRAINING_ORDER_ACTIVATING);
            return FALSE;
        }
    };

    class AssassinsTools : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class Frostmaw : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanGetFrostbite(target))
            CHECK(gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_FROSTBITE);
        }
    };

    class Patchwork : public virtual Disguise {
        SpeciesEnum onDisguise(ON_DISGUISE) override {
            SpeciesEnum species = Disguise::onDisguise(DELEGATE_DISGUISE);
            if (species && !testOnly) {
                SetOncePerTurnAbilityCounter(battler, ABILITY_PATCHWORK, gBattlerAttacker + 1);
            }
            return species;
        }
        int onDefender(ON_DEFENDER) override {
            int triggeringBattler = GetOncePerTurnAbilityCounter(battler, ability) - 1;
            CHECK(triggeringBattler == attacker)
            SetOncePerTurnAbilityCounter(battler, ability, 0);

            CHECK(IsBattlerAlive(attacker))
            CHECK_NOT(gBattleMons[attacker].status2 & STATUS2_CURSED)

            AbilityStatusEffect(MOVE_EFFECT_CURSE | MOVE_EFFECT_AFFECTS_USER);
            return TRUE;
        }
    };

    class BlindRage : public virtual MoldBreaker, public virtual Scrappy {};

    class ApexPredator : public virtual SoulEater, public virtual ToughClaws {};

    class DragonsRitual : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(CompareStat(battler, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN) || CompareStat(battler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
            BattleScriptCall(BattleScript_DragonsRitual);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class PinnacleBlade : public virtual Ability {
        InfiltrateType onInfiltrate(ON_INFILTRATE) override {
            return gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST ? INFILTRATE_BREAK_SCREENS | INFILTRATE_SUBSTITUTE : INFILTRATE_NONE;
        }
        int onAttacker(ON_ATTACKER) override {
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

    class Energized : public virtual Ability {
        .onEntry = Generator.onEntry, .onTerrain = Generator.onTerrain, int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(moveType == TYPE_ELECTRIC);
            SetOncePerTurnAbilityCounter(battler, ability, TRUE);
            BattleScriptCall(BattleScript_GeneratorActivatesRet);
            return TRUE;
        }
        .onExit = Generator.onExit, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
        .persistent = TRUE,
    };

    class ColorSpectrum : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (StabMultiplierInHalves(battler, moveType, move) > 2) MUL(1.2);
        }
    };

    class SteelBeetle : public virtual RagingBoxer, public virtual Pollinate {};

    class FromTheShadows : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class RagePoint : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(gIsCriticalHit)
            CHECK(CanRaiseStat(battler, STAT_ATK) || CanRaiseStat(battler, STAT_SPATK))

            BattleScriptCall(BattleScript_RagePointActivates);
            return TRUE;
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (HasAnyStatusOrAbility(battler)) MUL(1.5);
        }
        bool negatesBurnAtkDrop() override { return true; }
        bool negatesFrzSpatkDrop() { return true; }
    };

    class HotCoals : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

            gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
            return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
        }
    };

    class TerastalTreasure : public virtual Breakable {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override { MUL(.6); }
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED) *stat *= .8;
        }
    };

    class ShockingMaw : public virtual Ability {
        .onAttacker = ShockingJaws.onAttacker, .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
    };

    class GleamEyes : public Intimidate, public Frisk {
        int onEntry(ON_ENTRY) override { return Intimidate::onEntry(DELEGATE_ENTRY) | Frisk::onEntry(DELEGATE_ENTRY); }
    };

    class RousedFangs : public virtual Ability {
        .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier, .onChooseOffensiveStat = MindCrush.onChooseOffensiveStat,
    };

    class DreamState : public virtual Ability {
        .onDefensiveMultiplier = BattleArmor.onDefensiveMultiplier, .onCrit = BattleArmor.onCrit, .onCritFor = BattleArmor.onCritFor, .breakable = TRUE,
    };

    class DreamWhimsy : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_YAWN, 0); }
    };

    class LunarAffinity : public virtual Ability {
        int onCopyMove(ON_COPY_MOVE) override {
            CHECK(gBattleMoves[move].lunar)
            return UseOutOfTurnAttack(battler, target, ability, move, 0);
        }
    };

    class FlameShield : public virtual Ability {
        .onDefensiveMultiplier = Filter.onDefensiveMultiplier, .breakable = TRUE,
    };

    class AquaticDweller : public virtual Aquatic {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER) MUL(1.5);
        }
    };

    class ApplePie : public virtual Ability {
        .onEndTurn = SelfSufficient.onEndTurn,
    };

    class Hover : public virtual GroundImmune {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_PSYCHIC); }
    };

    class Depravity : public virtual Ability {
        .onCrit = Merciless.onCrit, .onTypeEffectiveness = Overcharge.onTypeEffectiveness, .onCanStatusType = Overcharge.onCanStatusType,
    };

    class Wildfire : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_FIRE_SPIN, 0); }
    };

    class JumpScare : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
            return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
        }
        bool persistent() override { return true; }
    };

    class TarToss : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_TAR_SHOT, 0); }
    };

    class StunShock : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class RagingGoddess : public virtual Rampage, public virtual HyperAggressive {};

    class Whiplash : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(IS_MOVE_PHYSICAL(move))
            CHECK(StatLowerableOrMirrorArmor(target, STAT_DEF))

            int affected = GetOncePerTurnAbilityCounter(battler, ability);
            CHECK_NOT(affected & (1 << target))

            SetOncePerTurnAbilityCounter(battler, ability, affected | (1 << target));
            return AbilityStatusEffect(MOVE_EFFECT_DEF_MINUS_1);
        }
    };

    class SupersweetSyrup : public virtual Breakable {
        int onDefender(ON_DEFENDER) override {
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

    class TrashHeap : public virtual ToxicSpill, public virtual Corrosion {};

    class SludgyMix : public virtual Intoxicate, public virtual PunkRock {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Intoxicate::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            PunkRock::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    class Overwatch : public virtual OnTheProwl, public virtual Stakeout {};

    class WindRage : public virtual GiantWings {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); }
    };

    class VictoryBomb : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK_NOT(IsBattlerAlive(battler))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_EXPLOSION, 100);
            return FALSE;
        }
        int onMoveType(ON_MOVE_TYPE) override {
            CHECK(gProcessingExtraAttacks)
            CHECK(gQueuedExtraAttackData[0].ability == ability)
            return TYPE_FIRE + 1;
        }
    };

    class RazorSharp : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(gIsCriticalHit)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class ToTheBone : public virtual RazorSharp, public virtual Sniper {};

    class BladeDance : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(IsDance(battler, move))
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_ALLOW_SELF))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_LEAF_BLADE, 50);
        }
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
    class ApeShift : public virtual FormChange {
        int onEntry(ON_ENTRY) override { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        int onEndTurn(ON_END_TURN) override { return ApeShiftHandler(battler, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK); }
        int onDefender(ON_DEFENDER) override { return ApeShiftHandler(battler, ABILITY_BS_CALL); }
        int onCrit(ON_CRIT) override {
            CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
            return ALWAYS_CRIT;
        }
    };

    class KnowYourPlace : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK_NOT(gVolatileStructs[target].dazed)
            CHECK(IsMoveMakingContact(move, battler))

            gVolatileStructs[target].dazed = 5;
            BattleScriptCall(BattleScript_TargetDazed);
            return TRUE;
        }
    };

    class DeepCuts : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST)
            CHECK(Random() % 2)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
    };

    class LifeSteal : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class RudeAwakening : public virtual Ability {
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_SLEEP)
            CHECK(GetAbilityState(battler, ability))
            return TRUE;
        }
        bool removesStatusOnImmunity() override { return true; }
    };

    class TeraformZero : public virtual TeraShell {
        int onEntry(ON_ENTRY) override {
            CHECK(!GetSingleUseAbilityCounter(battler, ability));
            SetSingleUseAbilityCounter(battler, ability, TRUE);
            CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
            BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
            return TRUE;
        }
    };

    class SetAblaze : public virtual PoisonPuppeteerLike<MOVE_EFFECT_BURN> {
        int onReactive(ON_REACTIVE) override {
            return PoisonPuppeteerClone(
                ability, battler, +[](int battler, int target) -> int { return !gVolatileStructs[target].fear; }, BattleScript_Bloodlust);
        }
    };

    class Breakwater : public virtual Stall, public virtual SwiftSwim {};

    class MagicalFists : public virtual IronFist {
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            if (IsIronFistBoosted(battler, move)) *atkStatToUse = STAT_SPATK;
        }
    };

    class Cutthroat : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

            gStatuses4[battler] |= STATUS4_CUTTHROAT;
            return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
        }
    };

    class SandBender : public virtual SandStream, public virtual SandForce {};

    class SandPit : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_SAND_TOMB, 20); }
    };

    class DesolateSun : public virtual RandomizerBanned {};

    ON_EITHER(Daybreak) {
        CHECK(ShouldApplyOnHitAffect(opponent))
        CHECK(CanBeBurned(opponent))
        CHECK(IsMoveMakingContact(move, gBattlerAttacker))

        AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, opponent);
        return TRUE;
    }
    class Daybreak : public virtual Ability {
        ON_EITHER_ABILITY(Daybreak),
    };

    class EnergySiphon : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK_NOT(BATTLER_MAX_HP(battler))
            CHECK(CanBattlerHeal(battler))

            gBattleMoveDamage = -gHpDealt / 4;
            if (!gBattleMoveDamage) gBattleMoveDamage = -1;
            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        }
    };

    class Reservoir : public virtual WaterAbsorb, public virtual StormDrain {
        int onAbsorb(ON_ABSORB) override { return WaterAbsorb::onAbsorb(DELEGATE_ABSORB) | StormDrain::onAbsorb(DELEGATE_ABSORB); }
    };

    static int NeurotoxinCondition(int battler, int target) {
        return CanLowerStat(target, STAT_ATK) || CanLowerStat(target, STAT_SPATK) || CanLowerStat(target, STAT_SPEED);
    }
    class Neurotoxin : public virtual PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
        int onReactive(ON_REACTIVE) override { return PoisonPuppeteerClone(ability, battler, NeurotoxinCondition, BattleScript_Neurotoxin); }
    };

    class EnergizedHorns : public virtual MightyHorn {
        int onSwapSplit(ON_SWAP_SPLIT) {
            CHECK(gBattleMoves[move].split == SPLIT_PHYSICAL)
            CHECK(gBattleMoves[move].hornBased);
            return TRUE;
        }
    };

    class SpiderLairUpgrade : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)

            int side = GetOppositeSide(battler);
            gSideTimers[side].started.spiderWeb = TRUE;
            gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
            gSideTimers[side].stickyWebTimer = 7;
            BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
            return TRUE;
        }
    };

    class CrustCoat : public virtual BattleArmor {};

    class Puffy : public virtual Fluffy {};

    class BalloonBlitz : public virtual Inflatable, public virtual HyperAggressive {};

    class StrikerPixilate : public virtual Striker, public virtual Pixilate {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Striker::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Pixilate::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    // 2.6
    class DoomBlast : public virtual Ability {
        int onRecoil(ON_RECOIL) override {
            CHECK(moveType == TYPE_DARK);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 20, 1);
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_DARK) MUL(1.35);
        }
    };

    class Bruteforce : public virtual Reckless, public virtual RockHead {};

    class FaradayCage : public virtual ShellArmor {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_THUNDER_CAGE, 50);
            return FALSE;
        }
    };

    class AcidicSlime : public virtual Corrosion {
        int onStab(ON_STAB) override { return moveType == TYPE_WATER; }
    };

    class RoseGarden : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            u8 targetSide = GetOppositeSide(battler);
            CHECK(gSideTimers[targetSide].toxicSpikesAmount < 2)

            gSideTimers[targetSide].toxicSpikesAmount = 2;
            gSideStatuses[targetSide] |= SIDE_STATUS_TOXIC_SPIKES;
            gBattlerTarget = targetSide;
            BattleScriptPushCursorAndCallback(BattleScript_RoseGarden);
            return TRUE;
        }
    };

    class Qigong : public virtual Rampage, public virtual FightingSpirit {
        void onAccuracy(ON_ACCURACY) override { return ACCURACY_ALWAYS_HITS; }
    };

    class ConjurerOfDeceit : public virtual MagicGuard, public virtual MagicBounce {};

    class DeepFreeze : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER || moveType == TYPE_ICE) MUL(1.25);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE) RESISTANCE(.5);
        }
    };

    class SoulDevourer : public virtual SoulEater, public virtual PhantomPain {};

    class ChampionsEntrance : public virtual Intimidate, public virtual ViolentRush {
        int onEntry(ON_ENTRY) override { return Intimidate::onEntry(DELEGATE_ENTRY) | ViolentRush::onEntry(DELEGATE_ENTRY); }
    };

    class Presto : public virtual Ability {
        int onPriority(ON_PRIORITY) override {
            CHECK(BATTLER_MAX_HP(battler))
            CHECK(IsSoundMove(battler, move))
            return 1;
        }
    };

    class Samba : public virtual Striker, public virtual Dancer {};

    class Gladiator : public virtual BoostedSwarmLike<TYPE_FIGHTING> {};

    class ForsakenHeart : public virtual Ability {
        int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK(ChangeStatBuffs(battler, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL))

            BattleScriptCall(BattleScript_RaiseStatOnFaintingTarget);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ANY; }
    };

    class Relentless : public virtual ExploitWeakness, public virtual Merciless {};

    class Soothsayer : public virtual Breakable {
        int onEntry(ON_ENTRY) override {
            CHECK(!GetSingleUseAbilityCounter(battler, ability))
            SetSingleUseAbilityCounter(battler, ability, TRUE);
            SetAbilityState(battler, ability, 3);
            return SwitchInAnnounce(B_MSG_SWITCHIN_SOOTHSAYER);
        }
        int onEndTurn(ON_END_TURN) override {
            int counter = GetAbilityState(battler, ability);
            if (counter) SetAbilityState(battler, ability, counter - 1);
            return FALSE;
        }
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (!GetAbilityState(battler, ability)) return;
            if (*mod >= UQ_4_12(1.0)) *mod = UQ_4_12(0.5);
        }
        AbilityApplyOnWithTarget onAfterTypeEffectivenessFor() override { return APPLY_ON_TARGET; }
        AbilityApplyOnWithTarget persistent() override { return true; }
    };

    class CorruptedMind : public virtual RandomizerBanned {
        int onTypeEffectiveness(ON_TYPE_EFFECTIVENESS) override {
            CHECK(moveType == TYPE_PSYCHIC)
            if (*mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
            return FALSE;
        }
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            int type;
            GET_MOVE_TYPE(move, type)
            if (type == TYPE_PSYCHIC) *effectChance *= 1.4;
        }
    };

    class FlameCoat : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_FIRE_COAT); }
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

    class UnownPower : public virtual RandomizerBanned {
        int onStab(ON_STAB) override { return TRUE; }
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (*mod < UQ_4_12(2.0) && (move == MOVE_HIDDEN_POWER || move == MOVE_SECRET_POWER)) *mod = UQ_4_12(2.0);
        }
    };

    class SuperScope : public virtual MegaLauncher, public virtual Artillery {};

    class VenomCrown : public virtual PoisonPoint, public virtual MightyHorn, public virtual RandomizerBanned {};

    class BlightScale : public virtual PoisonPoint, public virtual Multiscale, public virtual RandomizerBanned {};

    class Gunman : public virtual MegaLauncher {
        int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
            CHECK(flag == MOVE_FLAG_MEGA_LAUNCHER)
            CHECK(IS_MOVE_STATUS(move))
            return TRUE;
        }
    };

    class Caretaker : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class PoseidonsDominion : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_WHIRLPOOL, 0); }
    };

    class DualShadow : public virtual HungerSwitch {
        int onRecoil(ON_RECOIL) override {
            CHECK(moveType == TYPE_ELECTRIC || moveType == TYPE_DARK);
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
            return max(damage / 10, 1);
        }
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ELECTRIC || moveType == TYPE_DARK) MUL(1.35);
        }
    };

    class Lullaby : public virtual Ability {
        AccuracyPriority onAccuracy(ON_ACCURACY) override {
            CHECK(move == MOVE_SING);
            *accuracy *= 1.5;
            return ACCURACY_MULTIPLICATIVE;
        }
    };

    class CryoArchitect : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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
        int onDefender(ON_DEFENDER) override {
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

    class GlacialRage : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_ICE)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_BLIZZARD, 50);
        }
    };

    class ImmovableObject : public virtual Ability {
        bool magicGuard() override { return true; }
    };

    class FrenziedPhantom : public virtual ShadowTag, public virtual HyperAggressive {};

    class DNAScramble : public virtual FormChange {
        int onBeforeAttack(ABILITY_ON_BEFORE_ATTACK) override {
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

    class MetallicJaws : public virtual Metallic, public virtual PrimalMaw {};

    class Calculative : public virtual Analytic, public virtual Neuroforce {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Analytic::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            Neuroforce::onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
    };

    class EmbodyAspect : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(CanRaiseStat(battler, STAT_SPEED))

            SetStatChanger(STAT_SPEED, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class EmbodyAspectHearthflame : public virtual IntrepidSword {};

    class EmbodyAspectCornerstone : public virtual DauntlessShield {};

    class EmbodyAspectWellspring : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(CanRaiseStat(battler, STAT_SPDEF))

            SetStatChanger(STAT_SPDEF, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class RockhardShaft : public virtual BoostedSwarmLike<TYPE_ROCK> {};

    class HuntersMark : public virtual Deadeye, public virtual Ambush {};

    class Deviate : public virtual AteAbility<TYPE_DARK> {};

    class SunsBounty : public virtual Harvest, public virtual LeafGuard {
        int onEndTurn(ON_END_TURN) override { return Harvest::onEndTurn(DELEGATE_END_TURN) | LeafGuard::onEndTurn(DELEGATE_END_TURN); }
    };

    class RiteOfSpring : public virtual SolarPower, public virtual Chlorophyll {
        void onStat(ON_STAT) override {
            SolarPower::onStat(DELEGATE_STAT);
            Chlorophyll::onStat(DELEGATE_STAT);
        }
    };

    class Headstrong : public virtual Breakable {
        int onEntry(ON_ENTRY) override {
            CHECK(CanRaiseStat(battler, STAT_SPDEF))

            SetStatChanger(STAT_SPDEF, 1);
            BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
            return TRUE;
        }
    };

    class Firefighter : public virtual Breakable {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(target, TYPE_FIRE)) RESISTANCE(1.5);
        }
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_BATTLER_OF_TYPE(attacker, TYPE_FIRE)) MUL(.5);
        }
    };

    class SepiaLens : public virtual SandGuard, public virtual TintedLens {};

    class SuperSniper : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Sniper.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            if (gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ability) {
                MUL(0.5);
            }
        }
        .onPreemptAction = UseTurnAttackAsPursuit,
    };

    class WoodlandCurse : public virtual Ability {
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
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_FORESTS_CURSE, 0); }
        ON_EITHER_ABILITY(WoodlandCurse),
    };

    class Malodor : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))
            CHECK_NOT(gStatuses3[attacker] & STATUS3_GASTRO_ACID)

            gStatuses3[attacker] |= STATUS3_GASTRO_ACID;
            BattleScriptCall(BattleScript_StackAbilitySuppressedMessage);
            return TRUE;
        }
    };

    class Blur : public virtual Ability {
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
            CHECK(IsMoveMakingContact(move, gBattlerAttacker))
            return STAT_SPEED;
        }
        AbilityApplyOnWithTarget onChooseDefensiveStatFor() override { return onChooseDefensiveStatFor; }
    };

    class Elude : public virtual Ability {
        int onChooseDefensiveStat(ON_CHOOSE_DEFENSIVE_STAT) override {
            CHECK_NOT(IsMoveMakingContact(move, gBattlerAttacker))
            return STAT_SPEED;
        }
        AbilityApplyOnWithTarget onChooseDefensiveStatFor() override { return onChooseDefensiveStatFor; }
    };

    class DrakeOfRage : public virtual Rampage, public virtual TintedLens {};

    class MixedMartialArts : public virtual Ability {
        int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
            CHECK(flag == MOVE_FLAG_PUNCH || flag == MOVE_FLAG_KICK)
            CHECK(gBattleMoves[move].type == TYPE_NORMAL)
            return TRUE;
        }
    };

    class StrategicPause : public virtual Ability {
        .onOffensiveMultiplier = Analytic.onOffensiveMultiplier, .onCrit = +[](ON_CRIT) -> int {
            CHECK(GetBattlerTurnOrderNum(target) < gCurrentTurnActionNumber)
            CHECK(gBattleMoves[move].effect != EFFECT_FUTURE_SIGHT)
            return 2;
        },
    };

    class Overrule : public virtual Ability {
        void onAfterTypeEffectiveness(ON_AFTER_TYPE_EFFECTIVENESS) override {
            if (gIsCriticalHit && *mod && *mod < UQ_4_12(1.0)) *mod = UQ_4_12(1.0);
        }
    };

    class MentalPollution : public virtual Ability {
        bool breakable() override { return true; }
    };

    class MadnessEnhancement : public virtual Ability {
        bool breakable() override { return true; }
    };

    class Tentalock : public virtual Ability {
        bool breakable() override { return true; }
    };

    class SerpentBind : public virtual Ability {
        bool breakable() override { return true; }
    };

    class SoulTap : public virtual Ability {
        int onEndTurn(ON_END_TURN) override {
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

    class Scarecrow : public virtual Intimidate, public virtual BadLuck {};

    class OminousShroud : public virtual Phantom, public virtual ShadowShield {};

    class ChillingPresence : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_ICY_WIND, 10); }
    };

    class Frostbind : public virtual PoisonPuppeteerLike<MOVE_EFFECT_FROSTBITE> {
        int onReactive(ON_REACTIVE) override {
            return PoisonPuppeteerClone(ability, battler, +[](int battler, int target) { return (int)CanGetFrostbite(battler); }, BattleScript_Frostbind);
        }
    };

    class TenderAffection : public virtual Ability {
        ON_EITHER_ABILITY(CuteCharm), .onStab = +[](ON_STAB) -> int { return moveType == TYPE_FAIRY; },
    };

    class GlacialGhost : public virtual Ability {
        .onStat = SlushRush.onStat, .onAccuracy = SnowCloak.onAccuracy, .onAccuracyFor = SnowCloak.onAccuracyFor, .breakable = TRUE, .hailImmune = TRUE,
    };

    class WonderScale : public virtual Ability {
        .onEndTurn = ShedSkin.onEndTurn, .fortKnox = TRUE,
    };

    class Overzealous : public virtual Ability {
        bool breakable() override { return true; }
    };

    class StainlessSteel : public virtual Ability {
        ATE_ABILITY(TYPE_STEEL), .fortKnox = TRUE,
    };

    class TemporalRupture : public virtual Ability {
        bool breakable() override { return true; }
    };

    class GrassFlute : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(IsSoundMove(battler, move))
            CHECK_NOT(gVolatileStructs[target].fear)

            return AbilityStatusEffect(MOVE_EFFECT_FEAR);
        }
    };

    class Hemotoxin : public virtual PoisonPuppeteerLike<MOVE_EFFECT_POISON> {
        int onReactive(ON_REACTIVE) override {
            return PoisonPuppeteerClone(
                ability,
                battler,
                [](int battler, int target) -> int { return !(gStatuses3[target] & STATUS3_GASTRO_ACID); },
                BattleScript_StackAbilitySuppressedMessage);
        }
    };

    class Harukaze : public virtual Ability {
        bool breakable() override { return true; }
    };

    class ToxicSurge : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_TOXIC_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESTOXIC;
            BattleScriptPushCursorAndCallback(BattleScript_SurgeActivates);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_TOXIC,
    };

    class PoisonQuills : public virtual PoisonPoint, public virtual RoughSkin {
        int onDefender(ON_DEFENDER) override { return RoughSkin::onDefender(DELEGATE_DEFENDER) | PoisonPoint::onDefender(DELEGATE_DEFENDER); }
    };

    class DraconicMight : public virtual HalfDrake, public virtual AteAbility<TYPE_DRAGON> {};

    class AtlanticRuler : public virtual AquaticDweller, public virtual SwiftSwim {};

    class Biofilm : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPDEF && IsBattlerTerrainAffected(battler, STATUS_FIELD_TOXIC_TERRAIN)) *stat *= 1.5;
        }
    };

    class Chokehold : public virtual Ability {
        bool breakable() override { return true; }
    };

    class GuardianCoat : public virtual SandImmune, public virtual Breakable, public virtual HailImmune {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (IS_MOVE_PHYSICAL(move)) MUL(.8);
        }
        bool powderImmune() { return true; }
    };

    class NeutralizingFog : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_DEFOG, 0); }
    };

    class Festivities : public virtual Ability {
        int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
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

    class FeyFlight : public virtual FairyTale, public virtual GroundImmune {};

    class BestOffense : public virtual Ability {
        .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier, .onSwapSplit = MysticBlades.onSwapSplit,
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            secondaryAtkStatToUse[STAT_SPDEF] += 20;
        }
    };

    class Impaler : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBleed(target))
            CHECK(gBattleMoves[move].hornBased);
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_BLEED);
        }
        .onOffensiveMultiplier = MightyHorn.onOffensiveMultiplier,
    };

    class MagusBlades : public virtual Ability {
        .onParentalBond = DualWield.onParentalBond, .onOffensiveMultiplier = KeenEdge.onOffensiveMultiplier, .onSwapSplit = MysticBlades.onSwapSplit,
        void onChooseOffensiveStat(ON_CHOOSE_OFFENSIVE_STAT) override {
            secondaryAtkStatToUse[STAT_SPDEF] += 20;
        }
    };

    class LightningBorn : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_ELECTRIC); }
    };

    class Superheavy : public virtual Ability {
        bool breakable() override { return true; }
    };

    class WorldSerpent : public virtual Ability {
        .onAttacker = GripPincer.onAttacker, .onOffensiveMultiplier = LongReach.onOffensiveMultiplier, .onAccuracy = GripPincer.onAccuracy,
    };

    class LuckyWings : public virtual Ability {
        .onOffensiveMultiplier = GiantWings.onOffensiveMultiplier, .onModifyEffectChance = SereneGrace.onModifyEffectChance,
    };

    class Komodo : public virtual Ability {
        .onEntry = HalfDrake.onEntry, .onAttacker = ToxicChain.onAttacker,
    };

    class Envenom : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(CanBePoisoned(battler, target, MOVE_NONE))
            CHECK(Random() % 100 < 30)

            return AbilityStatusEffect(MOVE_EFFECT_POISON);
        }
    };

    class PurpleHaze : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_POISON_GAS, 20);
        }
    };

    class GnashingCannon : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            MegaLauncher.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MindCrush.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
        }
        .onChooseOffensiveStat = MindCrush.onChooseOffensiveStat,
    };

    class HyperCleanse : public virtual Ability {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_POISON) RESISTANCE(.5);
        }
        int onStatusImmune(ABILITY_ON_STATUS_IMMUNE) override {
            CHECK(status & CHECK_STATUS1)
            return TRUE;
        }
        bool breakable() override { return true; }
        bool removesStatusOnImmunity() { return true; }
    };

    class MoltenCoat : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(moveType == TYPE_ROCK)
            CHECK(CanBeBurned(target))
            CHECK(Random() % 2)

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
            return TRUE;
        }
        ATE_ABILITY(TYPE_ROCK),
    };

    class RoyalDecree : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(GetSingleUseAbilityCounter(battler, ability)) SetSingleUseAbilityCounter(battler, ability, TRUE);
            return UseEntryMove(battler, ability, MOVE_GLARE, 0);
        }
        .onImmune = QueenlyMajesty.onImmune, .onImmuneFor = APPLY_ON_ALLY, .breakable = TRUE,
    };

    class Tag : public virtual Ability {
        int onPreemptAction(ON_PREEMPT_ACTION) override {
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

    class Surprise : public virtual Ability {
        int onPreemptAction(ON_PREEMPT_ACTION) override {
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

    class BreezyNeigh : public virtual Ability {
        .onBattlerFaints = AdrenalineRush.onBattlerFaints, AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Dreamscape : public virtual Ability {
        .onEntry = Comatose.onEntry, void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            Dreamcatcher.onOffensiveMultiplier(DELEGATE_OFFENSIVE_MULTIPLIER);
            MUL(1.2);
        }
        .onStatusImmune = Comatose.onStatusImmune, .onPreemptAction = Dreamcatcher.onPreemptAction, .unsuppressable = TRUE, .removesStatusOnImmunity = TRUE,
    };

    class HasteMakesWaste : public virtual Ability {
        .onOffensiveMultiplier = Analytic.onOffensiveMultiplier, .onDefensiveMultiplier = Stall.onDefensiveMultiplier, .breakable = TRUE,
    };

    class HungryMaws : public virtual Ability {
        .onBattlerFaints = JawsOfCarnage.onBattlerFaints, .onOffensiveMultiplier = StrongJaw.onOffensiveMultiplier,
        AbilityApplyOnWithTarget onBattlerFaintsFor override {
            return APPLY_ON_ATTACKER;
        }
    };

    class ThermalSlide : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED && IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY | WEATHER_HAIL_ANY)) *stat *= 1.5;
        }
    };

    class Thermomancy : public virtual Ability {
        void onModifyEffectChance(ON_MODIFY_EFFECT_CHANCE) override {
            Cryomancy.onModifyEffectChance(DELEGATE_MODIFY_EFFECT_CHANCE);
            Pyromancy.onModifyEffectChance(DELEGATE_MODIFY_EFFECT_CHANCE);
        }
    };

    class Chuckster : public virtual Ability {
        bool breakable() override { return true; }
    };

    class HeatSink : public virtual Redirects<TYPE_FIRE> {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_FIRE);
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
    };

    class RelicStone : public virtual Ability {
        bool breakable() override { return true; }
    };

    class Supercell : public virtual Ability {
        int onEntry(ON_ENTRY) override { return ElectricSurge.onEntry(DELEGATE_ENTRY) | Drizzle.onEntry(DELEGATE_ENTRY); }
        .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
    };

    class LightningAspect : public virtual Ability {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_ELECTRIC)
            *statId = GetHighestAttackingStatId(battler, TRUE);
            return ABSORB_RESULT_STAT;
        }
        bool breakable() override { return true; }
    };

    class FireAspect : public virtual Ability {
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_FIRE)
            return ABSORB_RESULT_HEAL;
        }
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(moveType == TYPE_FIRE)
            CHECK(CanBeBurned(target))

            AbilityStatusEffectSafe(MOVE_EFFECT_BURN, battler, target);
            return TRUE;
        }
        bool breakable() override { return true; }
    };

    class BlisteringSun : public virtual Ability {
        int onEntry(ON_ENTRY) override { return DesolateLand.onEntry(DELEGATE_ENTRY) | AirBlower.onEntry(DELEGATE_ENTRY); }
    };

    class AurorasGale : public virtual Ability {
        .onEntry = NorthWind.onEntry, .onStat = MajesticBird.onStat, .hailImmune = TRUE,
    };

    class WinterThrone : public virtual Ability {
        int onEntry(ON_ENTRY) override { return SwitchInAnnounce(B_MSG_SWITCHIN_WINTER_THRONE); }
        .onEndTurn = +[](ON_END_TURN) -> int {
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
        },
    };

    class IcePlumes : public virtual Ability {
        .onDefensiveMultiplier = IceScales.onDefensiveMultiplier, .breakable = TRUE,
    };

    class PropellerTail : public virtual Ability {
        .onStat = SwiftSwim.onStat,
    };

    class EnergyTap : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK_NOT(BATTLER_MAX_HP(battler))
            CHECK(CanBattlerHeal(battler))

            gBattleMoveDamage = -gHpDealt / 8;
            if (!gBattleMoveDamage) gBattleMoveDamage = -1;
            BattleScriptCall(BattleScript_HydroCircuitAbsorbEffectActivated);
            return TRUE;
        }
    };

    class MoltenCore : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            Furnace.onEntry(DELEGATE_ENTRY);

            CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
            gSideStatuses[GetBattlerSide(battler)] &= ~SIDE_STATUS_STEALTH_ROCK;
            return SwitchInAnnounce(B_MSG_SWITCHIN_MOLTEN_CORE);
        }
        int onAbsorb(ON_ABSORB) override {
            CHECK(moveType == TYPE_ROCK)
            *statId = STAT_SPEED;
            return ABSORB_RESULT_STAT;
        }
        bool breakable() override { return true; }
        bool absorbUp2() { return true; }
        bool stealthRockImmune() { return true; }
    };

    class Reverberate : public virtual Ability {
        int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
            CHECK(flag == MOVE_FLAG_SOUND)
            CHECK(gBattleMoves[move].type == TYPE_NORMAL)
            return TRUE;
        }
    };

    class Taekkyeon : public virtual Ability {
        int onModifyMoveFlags(ON_MODIFY_MOVE_FLAGS) override {
            CHECK(flag == MOVE_FLAG_DANCE)
            CHECK_NOT(IS_MOVE_STATUS(move))
            return TRUE;
        }
    };

    class SludgeSpit : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(gBattleMoves[move].power)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_VENOM_BOLT, 35);
        }
    };

    class SwampThing : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gSideTimers[GetOppositeSide(battler)].swampTimer)

            AbilityStatusEffectSafe(MOVE_EFFECT_SWAMP, battler, GetOppositeSide(battler));
            InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
            return TRUE;
        }
    };

    class FrostyPrescence : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_MIST, 0); }
    };

    class ChillingPellets : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(attacker))
            CHECK(IsMoveMakingContact(move, attacker))

            UseOutOfTurnAttack(battler, attacker, ability, MOVE_ICICLE_SPEAR, 13);
            return FALSE;
        }
    };

    class PaintShot : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
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

    class Stonecutter : public virtual Ability {
        .onOffensiveMultiplier = Fossilized.onOffensiveMultiplier, .onDefensiveMultiplier = Fossilized.onDefensiveMultiplier,
        int onMoldBreaker(ON_MOLD_BREAKER) override {
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
        bool breakable() override { return true; }
    };

    class Edgelord : public virtual Ability {
        .onEntry = Cutthroat.onEntry, int onBattlerFaints(ON_BATTLER_FAINTS) override {
            CHECK_NOT(gStatuses4[battler] & STATUS4_CUTTHROAT)

            gStatuses4[battler] |= STATUS4_CUTTHROAT;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CUTTHROAT;
            BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsgRet);
            return TRUE;
        }
        AbilityApplyOnWithTarget onBattlerFaintsFor override { return APPLY_ON_ATTACKER; }
    };

    class Warmonger : public virtual Ability {
        void onOffensiveMultiplier(ON_OFFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_ROCK || moveType == TYPE_STEEL || moveType == TYPE_FIGHTING) MUL(1.30);
        }
    };

    class LocustSwarm : public virtual StandardTransformation {};

    class Revelation : public virtual StandardTransformation {};

    class CurseOfFamine : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

            BattleScriptPushCursorAndCallback(BattleScript_CurseOfFamine);
            return TRUE;
        }
    };

    class CrystallineArmor : public virtual Ability {
        bool breakable() override { return true; }
    };

    class SoulHarvest : public virtual Ability {
        void onStat(ON_STAT) override {
            if (statId != STAT_SPEED) *stat = *stat * (20 + min(5, gFaintedMonCount[GetBattlerSide(battler)])) / 20;
        }
        bool breakable() override { return true; }
    };

    class ThickBlubber : public virtual Ability {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_FIRE || moveType == TYPE_ICE) RESISTANCE(.25);
        }
        void onStat(ON_STAT) override {
            if (statId == STAT_SPEED) *stat *= .5;
        }
    };

    class Craving : public virtual Ability {
        bool breakable() override { return true; }
    };

    class RatKing : public virtual Ability {
        void onStat(ON_STAT) override {
            const BaseStats *baseStats = &gBaseStats[gBattleMons[battler].species];
            int bst =
                baseStats->baseHP + baseStats->baseAttack + baseStats->baseDefense + baseStats->baseSpAttack + baseStats->baseSpDefense + baseStats->baseSpeed;
            if (bst >= 400) return;
            *stat *= 1.5;
        }
        .onStatFor = APPLY_ON_ALLY,
    };

    class CrispyCream : public virtual Ability {
        int onDefender(ON_DEFENDER) override { return Random() % 2 ? FlameBody.onDefender(DELEGATE_DEFENDER) : FreezingPoint.onDefender(DELEGATE_DEFENDER); }
    };

    class DeepFried : public virtual Ability {
        int onEntry(ON_ENTRY) override {
            CHECK_NOT(gSideTimers[GetOppositeSide(battler)].fireSeaTimer)

            AbilityStatusEffectSafe(MOVE_EFFECT_FIRE_SEA, battler, GetOppositeSide(battler));
            InsertCorrectEndType(ABILITY_BS_PUSH_CURSOR_AND_CALLBACK);
            return TRUE;
        }
    };

    class FoodLovers : public virtual Ability {
        .onEntry = Hospitality.onEntry, .breakable = TRUE,
    };

    class LunarWrath : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(moveType == TYPE_GHOST)
            CHECK(AdjustFollowupMoveTarget(battler, &target, move, FOLLOWUP_STANDARD))

            return UseAttackerFollowUpMove(battler, target, ability, MOVE_MOONGEIST_BEAM, 50);
        }
    };

    class Spyware : public virtual Ability {
        bool breakable() override { return true; }
    };

    class Virus : public virtual Ability {
        int onAttacker(ON_ATTACKER) override {
            CHECK(ShouldApplyOnHitAffect(target))
            CHECK(moveType == TYPE_ELECTRIC)
            CHECK(CanBePoisoned(battler, target, move))

            return AbilityStatusEffect(MOVE_EFFECT_POISON);
        }
    };

    class PowerLeak : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(ShouldApplyOnHitAffect(battler))
            CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))

            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TERRAINBECOMESELECTRIC;
            BattleScriptCall(BattleScript_SurgeActivatesRet);
            return TRUE;
        }
        .allowTerrainIfAirborne = TERRAIN_ELECTRIC,
    };

    class BackupPower : public virtual Ability {
        int onRevive(ON_REVIVE) override {
            CHECK(IsTerrainActive(STATUS_FIELD_ELECTRIC_TERRAIN))
            return B_MSG_BACKUP_POWER;
        }
        bool persistent() override { return true; }
    };

    class SandFiend : public virtual SandGuard, public virtual SandForce {};

    class Moustache : public virtual Ability {
        int onDefender(ON_DEFENDER) override { return TanglingHair.onDefender(DELEGATE_DEFENDER) | Stamina.onDefender(DELEGATE_DEFENDER); }
    };

    class DepthExplorer : public virtual Ability {
        .onOffensiveMultiplier = FieldExplorer.onOffensiveMultiplier, .onAccuracy = Illuminate.onAccuracy,
    };

    class DuneVeil : public virtual SandGuard, public virtual SelfSufficient {};

    class StrongFoundation : public virtual Ability {
        void onDefensiveMultiplier(ON_DEFENSIVE_MULTIPLIER) override {
            if (moveType == TYPE_WATER || moveType == TYPE_GROUND) RESISTANCE(.50);
        }
    };

    class FogMachine : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
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

    class DropBlocks : public virtual Ability {
        int onDefender(ON_DEFENDER) override {
            CHECK(DidMoveHit())
            CHECK(gSideTimers[BATTLE_OPPOSITE(battler)].spikesAmount < 3)

            BattleScriptCall(BattleScript_DefenderSetsSpikeLayer_Scrapyard);
            return TRUE;
        }
    };

    class LaserDrill : public virtual Ability {
        bool breakable() override { return true; }
    };

    class LightSaber : public virtual Ability {
        bool breakable() override { return true; }
    };

    class LooseThorns : public virtual Ability {
        bool breakable() override { return true; }
    };

    class TurfWar : public virtual Ability {
        bool breakable() override { return true; }
    };

    class Greedy : public virtual Ability {
        bool breakable() override { return true; }
    };

    class MusicalNotes : public virtual Ability {
        bool breakable() override { return true; }
    };

    class Strikeout : public virtual Ability {
        bool breakable() override { return true; }
    };

    class HomeRun : public virtual Ability {
        bool breakable() override { return true; }
    };

    class Bruiser : public virtual Ability {
        int onEntry(ON_ENTRY) override { return AddBattlerType(battler, TYPE_FIGHTING); }
    };

    class LetsDance : public virtual Ability {
        int onEntry(ON_ENTRY) override { return UseEntryMove(battler, ability, MOVE_TEETER_DANCE, 0); }
    };

    class MyceliumMight : public virtual Ability {
        int onMoldBreaker(ON_MOLD_BREAKER) override { return IS_MOVE_STATUS(move); }
    };

    class DeadlyPrecision : public virtual Ability {
        int onMoldBreaker(ON_MOLD_BREAKER) override {
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
        {ABILITY_JUNSHI_SANDA, JunshiSanda},
        {ABILITY_REVERBATE, Reverberate},
        {ABILITY_TAEKKYEON, Taekkyeon},
        {ABILITY_SLUDGE_SPIT, SludgeSpit},
        {ABILITY_SWAMP_THING, SwampThing},
        {ABILITY_FROSTY_PRESCENCE, FrostyPrescence},
        {ABILITY_CHILLING_PELLETS, ChillingPellets},
        {ABILITY_PAINT_SHOT, PaintShot},
        {ABILITY_STONECUTTER, Stonecutter},
        {ABILITY_EDGELORD, Edgelord},
        {ABILITY_WARMONGER, Warmonger},
        {ABILITY_LOCUST_SWARM, LocustSwarm},
        {ABILITY_REVELATION, Revelation},
        {ABILITY_CURSE_OF_FAMINE, CurseOfFamine},
        {ABILITY_CRYSTALLINE_ARMOR, CrystallineArmor},
        {ABILITY_SOUL_HARVEST, SoulHarvest},
        {ABILITY_THICK_BLUBBER, ThickBlubber},
        {ABILITY_CRAVING, Craving},
        {ABILITY_RAT_KING, RatKing},
        {ABILITY_CRISPY_CREAM, CrispyCream},
        {ABILITY_DEEP_FRIED, DeepFried},
        {ABILITY_FOOD_LOVERS, FoodLovers},
        {ABILITY_LUNAR_WRATH, LunarWrath},
        {ABILITY_SPYWARE, Spyware},
        {ABILITY_VIRUS, Virus},
        {ABILITY_POWER_LEAK, PowerLeak},
        {ABILITY_BACKUP_POWER, BackupPower},
        {ABILITY_SAND_FIEND, SandFiend},
        {ABILITY_MOUSTACHE, Moustache},
        {ABILITY_DEPTH_EXPLORER, DepthExplorer},
        {ABILITY_DUNE_VEIL, DuneVeil},
        {ABILITY_STRONG_FOUNDATION, StrongFoundation},
        {ABILITY_FOG_MACHINE, FogMachine},
        {ABILITY_DROP_BLOCKS, DropBlocks},
        {ABILITY_LASER_DRILL, LaserDrill},
        {ABILITY_LIGHT_SABER, LightSaber},
        {ABILITY_LOOSE_THORNS, LooseThorns},
        {ABILITY_TURF_WAR, TurfWar},
        {ABILITY_GREEDY, Greedy},
        {ABILITY_MUSICAL_NOTES, MusicalNotes},
        {ABILITY_STRIKEOUT, Strikeout},
        {ABILITY_HOME_RUN, HomeRun},
        {ABILITY_BRUISER, Bruiser},
        {ABILITY_LETS_DANCE, LetsDance},
        {ABILITY_MYCELIUM_MIGHT, MyceliumMight},
        {ABILITY_DEADLY_PRECISION, DeadlyPrecision},
    };

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
