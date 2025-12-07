#ifndef GUARD_BATTLE_UTIL_H
#define GUARD_BATTLE_UTIL_H

#include "abilities.hh"
#include "battle_ai_new.h"

#define MOVE_LIMITATION_ZEROMOVE (1 << 0)
#define MOVE_LIMITATION_PP (1 << 1)
#define MOVE_LIMITATION_DISABLED (1 << 2)
#define MOVE_LIMITATION_TORMENTED (1 << 3)
#define MOVE_LIMITATION_TAUNT (1 << 4)
#define MOVE_LIMITATION_IMPRISON (1 << 5)

#define ABILITYEFFECT_ON_SWITCHIN 0
#define ABILITYEFFECT_ENDTURN 1
#define ABILITYEFFECT_MOVES_BLOCK 2
#define ABILITYEFFECT_ABSORBING 3
#define ABILITYEFFECT_MOVE_END_ATTACKER 4
#define ABILITYEFFECT_MOVE_END 5
#define ABILITYEFFECT_IMMUNITY 6
#define ABILITYEFFECT_FORECAST 7
#define ABILITYEFFECT_SYNCHRONIZE 8
#define ABILITYEFFECT_ATK_SYNCHRONIZE 9
#define ABILITYEFFECT_INTIMIDATE1 10
#define ABILITYEFFECT_INTIMIDATE2 11
#define ABILITYEFFECT_TRACE1 12
#define ABILITYEFFECT_TRACE2 13
#define ABILITYEFFECT_MOVE_END_OTHER 14
#define ABILITYEFFECT_NEUTRALIZINGGAS 15
#define ABILITYEFFECT_AFTER_RECOIL 16
#define ABILITYEFFECT_REACTIVE 17
#define ABILITYEFFECT_MOVE_END_EITHER 18
// Special cases
#define ABILITYEFFECT_SWITCH_IN_TERRAIN 0xFE
#define ABILITYEFFECT_SWITCH_IN_WEATHER 0xFF

#define ITEMEFFECT_ON_SWITCH_IN 0x0
#define ITEMEFFECT_MOVE_END 0x3
#define ITEMEFFECT_KINGSROCK 0x4
#define ITEMEFFECT_TARGET 0x5
#define ITEMEFFECT_ORBS 0x6
#define ITEMEFFECT_LIFEORB_SHELLBELL 0x7
#define ITEMEFFECT_BATTLER_MOVE_END 0x8  // move end effects for just the battler, not whole field

#define WEATHER_HAS_EFFECT \
    (!gFieldTimers.clearSkiesTimer && !IsAbilityOnField(ABILITY_CLOUD_NINE) && !IsAbilityOnField(ABILITY_AIR_LOCK) && !IsAbilityOnField(ABILITY_CLUELESS))
#define TERRAIN_HAS_EFFECT (!IsAbilityOnField(ABILITY_CLUELESS))
#define ROOM_HAS_EFFECT (!IsAbilityOnField(ABILITY_CLUELESS))

#define BATTLER_NONE 0
#define BATTLER_ABILITY 1
#define BATTLER_INNATE 2

#define WEATHER_DURATION 8
#define WEATHER_DURATION_EXTENDED 12
#define TERRAIN_DURATION 8
#define TERRAIN_DURATION_EXTENDED 12
#define GRAVITY_DURATION 5
#define GRAVITY_DURATION_EXTENDED 8
#define TRICK_ROOM_DURATION 5
#define TRICK_ROOM_DURATION_SHORT 3
#define WONDER_ROOM_DURATION 5
#define MAGIC_ROOM_DURATION 5
#define INVERSE_ROOM_DURATION 5
#define INVERSE_ROOM_DURATION_SHORT 3
#define ROOM_DURATION_MAX 255
#define SCREEN_DURATION 5
#define SCREEN_DURATION_EXTENDED 8
#define SCREEN_DURATION_SHORT 3
#define TAILWIND_DURATION 3
#define TAILWIND_DURATION_SHORT 3
#define PLEDGE_DURATION 3
#define SPORT_DURATION 5
#define QUASH_DURATION 5

#define IS_WHOLE_SIDE_ALIVE(battler) ((IsBattlerAlive(battler) && IsBattlerAlive(BATTLE_PARTNER(battler))))
#define BATTLER_HAS_ABILITY(battlerId, ability) BattlerHasAbility((battlerId), (ability), TRUE)
#define BATTLER_HAS_ABILITY_AND_ALIVE(battlerId, ability, checkMoldBreaker) \
    (IsBattlerAlive(battlerId) && BattlerHasAbility(battlerId, ability, checkMoldBreaker))

enum MiscMoveEffects {
    MISC_EFFECT_SUPEREFFECTIVE_BOOST = 1,
    MISC_EFFECT_FAINTED_MON_BOOST,
    MISC_EFFECT_ELECTRIC_TERRAIN_BOOST,
    MISC_EFFECT_TOOK_DAMAGE_BOOST,
    MISC_EFFECT_INCREASED_CRIT_DAMAGE,
    MISC_EFFECT_IVY_CUDGEL,
    MISC_EFFECT_DOUBLE_DAMAGE,
    MISC_EFFECT_DOUBLE_DAMAGE_VS_BLEEDING,
    MISC_EFFECT_50_PERCENT_PLUS_DAMAGE_VS_BLEEDING,
    MISC_EFFECT_TRANSMUTE,
    MISC_EFFECT_DOUBLE_DAMAGE_IN_FOG,
};

// for Natural Gift and Fling
typedef struct TypePower {
    u8 type;
    u8 power;
    u16 effect;
} TypePower;

typedef enum {
    COMMANDER_NOT_ACTIVE = 0,
    COMMANDER_ACTIVE,
} CommanderState;

typedef enum {
    RESTRAINING_ORDER_NOT_TRIGGERED = 0,
    RESTRAINING_ORDER_ACTIVATING,
    RESTRAINING_ORDER_DONE,
} RestrainingOrderState;

typedef enum {
    PARADOX_BOOST_NOT_ACTIVE = 0,
    PARADOX_BOOSTER_ENERGY = 1,
    PARADOX_WEATHER_ACTIVE = 2,
} ParadoxBoostSource;

typedef struct {
    ParadoxBoostSource source:2;
    u8 statId:3;
} ParadoxBoost;

typedef struct {
    bool8 inProgress:1;
    u8 battler:3;
    u8 stat:4;
    bool8 announced:1;
} StatCopyState;

typedef struct {
    u16 itemId;
    bool8 setThisTurn:1;
    bool8 activating:1;
} CudChewState;

typedef struct {
    u8 type1:5;
    u8 type2:5;
    u8 active:1;
} MimicryState;

typedef union AbilityStates {
    ParadoxBoost paradoxBoost;
    StatCopyState statCopyState;
    CudChewState cudChewState;
    MimicryState mimicryState;
    u32 intValue;
} AbilityStates;

#define ABILITY_SUPPRESSION_PERSISTENT (1 << 14)
#define ABILITY_SUPPRESSION_ABILITY (1 << 15)
#define ABILITY_SUPPRESSION_MASK (ABILITY_SUPPRESSION_ABILITY | ABILITY_SUPPRESSION_PERSISTENT)

extern const struct TypePower gNaturalGiftTable[];
extern const u16 gPercentToModifier[];
typedef struct {
    AbilityEnum ability;
    SpeciesEnum highHpSpecies;
    SpeciesEnum lowHpSpecies;
    u8 hpFraction;
} HpTransformation;
extern const HpTransformation gHpTransformations[11];

int GetAbilityIndex(int battler, AbilityEnum ability, int checkMoldBreaker);
s32 CountUsablePartyMons(u8 battlerId);
void HandleAction_ThrowBall(void);
void HandleAction_ShowBattleInfo(void);
void HandleAction_ShowInGameWiki(void);
void HandleAction_OpenInfoMenu(void);
bool32 IsAffectedByFollowMe(u32 battlerAtk, u32 defSide, u32 move);
void HandleAction_UseMove(void);
void HandleAction_Switch(void);
void HandleAction_UseItem(void);
void HandleAction_Run(void);
void HandleAction_WatchesCarefully(void);
void HandleAction_SafariZoneBallThrow(void);
void HandleAction_ThrowPokeblock(void);
void HandleAction_GoNear(void);
bool8 CanUseExtraMove(u8 sBattlerAttacker, u8 sBattlerTarget);
void HandleAction_SafariZoneRun(void);
void HandleAction_WallyBallThrow(void);
void HandleAction_TryFinish(void);
void HandleAction_NothingIsFainted(void);
void HandleAction_ActionFinished(void);
u8 GetBattlerForBattleScript(u8 caseId);
bool8 IsSleepDisabled(u8 battlerId);
bool8 IsSleepClauseDisablingMove(u8 battlerId, MoveEnum move);
u16 GetParentalBondMultiplier(MultihitType parentalBondType, int turn);
void MarkAllBattlersForControllerExec(void);  // unused
bool32 IsBattlerMarkedForControllerExec(u8 battlerId);
void MarkBattlerForControllerExec(u8 battlerId);
void MarkBattlerReceivedLinkData(u8 arg0);
void CancelMultiTurnMoves(u8 battlerId);
bool8 WasUnableToUseMove(u8 battlerId);
void PrepareStringBattle(u16 stringId, u8 battlerId);
void ResetSentPokesToOpponentValue(void);
void OpponentSwitchInResetSentPokesToOpponentValue(u8 battlerId);
void UpdateSentPokesToOpponentValue(u8 battlerId);
void BattleScriptPush(const u8* bsPtr);
void BattleScriptPushCursor(void);
void BattleScriptCall(const u8* command);
void BattleScriptSaveCurrentStackData();
void BattleScriptPop(void);
void ReadActiveScriptInitialStackState();
u8 TrySetCantSelectMoveBattleScript(void);
u8 CheckMoveLimitations(u8 battlerId, u8 unusableMoves, u8 check);
bool8 AreAllMovesUnusable(void);
u8 GetImprisonedMovesCount(u8 battlerId, MoveEnum move);
u8 DoFieldEndTurnEffects(void);
s32 GetDrainedBigRootHp(u32 battler, s32 hp);
u8 DoBattlerEndTurnEffects(void);
bool8 HandleWishPerishSongOnTurnEnd(void);
bool8 HandleFaintedMonActions(void);
void TryClearRageAndFuryCutter(void);
u8 AtkCanceller_UnableToUseMove(void);
bool8 HasNoMonsToSwitch(u8 battlerId, u8 r1, u8 r2);
bool32 TryChangeBattleWeather(u8 battler, u32 weatherEnumId, bool32 viaAbility);
bool32 SetPermanentWeather(u32 weatherEnumId);
u8 AbilityBattleEffects(u8 caseID, u8 battlerId, AbilityEnum ability, u8 extraArg, MoveEnum moveArg);
int HandleAttackerAbility(int abilityNumber, int battler, int target, MoveEnum move);
int HandleDefenderAbility(int abilityNumber, int battler, int attacker, MoveEnum move);
int HandleAttackerOrDefenderAbility(AbilityEnum ability, int battler, int opponent, MoveEnum move);
int HandleMiscAbilityMoveEffects(int battler, int opponent, MoveEnum move);
int HandleSwitchInAbility(int abilityNumber, int battler);
int HandleEndTurnAbility(int abilityNumber, int battler);
int WasMoveSuccessful();
int DidMoveHit();
int ShouldApplyOnHitEffect(int applyTo);
void ReplaceAbility(int battler, AbilityEnum ability);
int HasAbilityIgnoringSuppression(int battler, AbilityEnum ability);
AbilityEnum GetAbilityAtIndex(int battler, int abilityNumber, int checkMoldBreaker);
int IsSuppressed(int battler, AbilityEnum ability, int checkMoldBreaker);
int AbilityHealMonStatus(u8 battler, AbilityEnum ability);
int CheckHalfHpAbility(int battlerDef, int battlerAtk);
bool8 UseOutOfTurnAttack(u8 battler, u8 target, AbilityEnum ability, MoveEnum move, u8 movePower);
u16 UseAttackerFollowUpMove(u8 battler, int target, AbilityEnum ability, u16 extraMove, u8 movePower);

#define ON_ABILITY(battler, checkMoldBreaker, condition, callback)             \
    for (int idx = GetNumPossibleAbilitiesForBattler() - 1; idx >= 0; idx--) { \
        AbilityEnum ability = GetBattlerAbilityInSlot(battler, idx);           \
        FILTER(condition)                                                      \
        FILTER_NOT(IsSuppressed(battler, ability, checkMoldBreaker))           \
        callback;                                                              \
    }

#define RETURN_ABILITY_IF_FLAG(battler, checkMoldBreaker, flag) ON_ABILITY(battler, checkMoldBreaker, gAbilities[ability].flag, return ability)

void GetAllBattlerAbilities(AbilityEnum* abilities, int battler, int battlerAtk);
u32 IsAbilityOnSide(u32 battlerId, AbilityEnum ability);
u32 IsAbilityOnOpposingSide(u32 battlerId, AbilityEnum ability);
u32 IsAbilityOnField(AbilityEnum ability);
u32 IsAbilityOnFieldExcept(u32 battlerId, AbilityEnum ability);
u32 IsAbilityPreventingEscape(u32 battlerId);
bool32 IsBattlerProtected(u8 battlerId, MoveEnum move);
bool32 CanBattlerEscape(u32 battlerId);  // no ability check
void BattleScriptExecute(const u8* BS_ptr);
void BattleScriptPushCursorAndCallback(const u8* BS_ptr);
u8 ItemBattleEffects(u8 caseID, u8 battlerId, bool8 moveTurn);
void ClearFuryCutterDestinyBondGrudge(u8 battlerId);
void HandleAction_RunBattleScript(void);
u32 SetRandomTarget(u32 battlerId);
u32 GetMoveTarget(MoveEnum move, u8 setTarget);
u8 IsMonDisobedient(void);
bool8 IsItemNegated(u8 battlerId);
u32 GetBattlerHoldEffect(u8 battlerId, bool32 checkNegating);
u32 GetBattlerHoldEffectParam(u8 battlerId);
bool32 IsMoveMakingContact(MoveEnum move, u8 battlerAtk);
bool32 IsBattlerGrounded(u8 battlerId);
bool32 IsBattlerGroundedIgnoreType(u8 battlerId);
int IsSoundMove(int battler, MoveEnum move);
int IsKeenEdge(int battler, MoveEnum move, Type moveType);
bool32 IsBattlerAlive(u8 battlerId);
u8 GetBattleMonMoveSlot(struct BattlePokemon* battleMon, MoveEnum move);
u32 GetBattlerWeight(u8 battlerId);
s32 CalculateMoveDamage(MoveEnum move, u8 battlerAtk, u8 battlerDef, u8* moveType, s32 fixedBasePower, u8 critRoll, bool32 randomFactor, bool32 updateFlags);
s32 CalculateMoveDamageAndEffectiveness(MoveEnum move, u8 battlerAtk, u8 battlerDef, u8* moveType, u16* typeEffectivenessModifier);
u32 CalcMoveBasePowerAfterModifiers(MoveEnum move, u8 fixedPower, u8 battlerAtk, u8 battlerDef, u8 moveType, bool32 updateFlags);
int CalcMoveDamageAi(MoveEnum move, int battlerAtk, int battlerDef, u8* moveType, int fixedBasePower, struct MoveState* moveState);
u16 CalcTypeEffectivenessMultiplier(MoveEnum move, u8 moveType, u8 battlerAtk, u8 battlerDef, bool32 recordAbilities);
u16 CalcPartyMonTypeEffectivenessMultiplier(MoveEnum move, SpeciesEnum speciesDef, u16 abilityDef, u8 leveldef);
u16 GetTypeModifier(int atkType, int defType, int miracleEyeAtk, int miracleEyeDef);
s32 GetStealthHazardDamage(u8 hazardType, u8 battlerId);
SpeciesEnum GetMegaEvolutionSpecies(SpeciesEnum preEvoSpecies, u16 heldItemId);
SpeciesEnum GetPrimalReversionSpecies(SpeciesEnum preEvoSpecies, u16 heldItemId);
SpeciesEnum GetWishMegaEvolutionSpecies(SpeciesEnum preEvoSpecies, MoveEnum moveId1, MoveEnum moveId2, MoveEnum moveId3, MoveEnum moveId4);
bool32 CanMegaEvolve(u8 battlerId);
void UndoMegaEvolution(u32 monId);
void UndoFormChange(u32 monId, u32 side, bool32 isSwitchingOut);
bool32 DoBattlersShareType(u32 battler1, u32 battler2);
bool32 CanBattlerGetOrLoseItem(u8 battlerId, u16 itemId);
bool32 DoesBattlerIgnoreAbilityorInnateChecks(u8 battler);
struct Pokemon* GetIllusionMonPtr(u32 battlerId);
void ClearIllusionMon(u32 battlerId);
bool32 SetIllusionMon(struct Pokemon* mon, u32 battlerId);
void TryPreemptiveActions();
u8 GetBattleMoveSplit(MoveEnum moveId);
bool32 TestMoveFlags(MoveEnum move, u32 flag);
struct Pokemon* GetBattlerPartyData(u8 battlerId);
bool32 CanFling(u8 battlerId);
bool32 IsTelekinesisBannedSpecies(SpeciesEnum species);
bool32 IsHealBlockPreventingMove(u8 battler, u32 move);
bool32 HasEnoughHpToEatBerry(u32 battlerId, u32 hpFraction, u32 itemId);
void SortBattlersBySpeed(u8* battlers, bool8 slowToFast);
bool32 TestSheerForceFlag(u8 battler, MoveEnum move);
void TryRestoreStolenItems(void);
bool32 CanStealItem(u8 battlerStealing, u8 battlerItem, u16 item);
void TrySaveExchangedItem(u8 battlerId, u16 stolenItem);
bool32 IsPartnerMonFromSameTrainer(u8 battlerId);
u8 TryHandleSeed(u8 battler, u32 terrainFlag, u8 statId, u16 itemId, bool32 execute);
bool32 IsBattlerAffectedByHazards(u8 battlerId, bool32 stealthRock);
bool32 CompareStat(u8 battlerId, u8 statId, u8 cmpTo, u8 cmpKind);
int StatLowerableOrMirrorArmor(int battler, int stat);
bool32 TryRoomService(u8 battlerId);
void BufferStatChange(u8 battlerId, u8 statId, u8 stringId);
void DoBurmyFormChange(u32 monId);
bool32 BlocksPrankster(MoveEnum move, u8 battlerPrankster, u8 battlerDef, bool32 checkTarget);
u16 GetUsedHeldItem(u8 battler);
bool32 IsBattlerWeatherAffected(u8 battlerId, u32 weatherFlags);
bool8 IsMoveAffectedByParentalBond(MoveEnum move, u8 battlerId);
u8 GetBattlerBattleMoveTargetFlags(MoveEnum moveId, u8 battler);
bool32 ShouldChangeFormHpBased(u32 battler);
u32 CountBattlerStatIncreases(u8 battlerId, bool32 countEvasionAcc);
int CountBattlerStatDecreases(int battler);
s32 GetCurrentTerrain(void);
bool8 IsTrickRoomActive(void);
bool8 IsInverseRoomActive(void);
bool8 IsGravityActive(void);
bool8 isMagicRoomActive(void);
bool8 isWonderRoomActive(void);
bool32 TryPrimalReversion(u8 battlerId, int useReturn);
bool8 HasAnyLoweredStat(u8 battler);
u32 CalculateStat(
    u8 battler, u8 statEnum, u8 secondaryStat[NUM_STATS], MoveEnum move, bool8 isAttack, bool8 isCrit, bool8 isUnaware, bool8 calculatingSecondary);
bool8 CheckAndSetSwitchInAbility(u8 battlerId, AbilityEnum ability);
s8 GetSingleUseAbilityCounter(u8 battler, AbilityEnum ability);
void SetSingleUseAbilityCounter(u8 battler, AbilityEnum ability, u8 value);
void IncrementSingleUseAbilityCounter(u8 battler, AbilityEnum ability, u8 value);
u32 GetAbilityState(u8 battler, AbilityEnum ability);
void SetAbilityState(u8 battler, AbilityEnum ability, u32 value);
AbilityStates GetAbilityStateAs(u8 battler, AbilityEnum ability);
void SetAbilityStateAs(u8 battler, AbilityEnum ability, AbilityStates value);
void IncrementAbilityState(u8 battler, AbilityEnum ability, u32 value);
int GetHighestStatIdExcept(int battlerId, int includeStatStages, int exclude);
u8 GetHighestStatId(u8 battlerId, u8 includeStatStages);
u8 GetHighestAttackingStatId(u8 battlerId, u8 includeStatStages);
bool8 CanMoveHaveExtraFlinchChance(MoveEnum move);
u8 GetHighestDefendingStatId(u8 battlerId, u8 includeStatStages);
u8 TranslateStatId(u8 statId, u8 battlerId);
bool32 IsAlly(u32 battlerAtk, u32 battlerDef);
void UpdateAbilityStateIndices(u8 battler, u16 newAbilities[]);
void UpdateAbilityStateIndicesForNewAbility(u8 battler, u16 newAbility);
void UpdateAbilityStateIndicesForNewSpecies(u8 battler, u16 newSpecies);
bool32 IsUnsuppressableAbility(AbilityEnum ability);
int IsPersistentOrUnsuppressableAbility(AbilityEnum ability);
bool8 CanBeDisabled(u8 battlerId);
bool8 DoesBattlerHaveAbilityShield(u8 battlerId);
u16 IsSoundproof(u8 battlerId);
AbilityEnum BattlerHasAbility(int battler, AbilityEnum ability, int checkMoldBreaker);
u8 GetTurnBattler();
void ReadActiveScriptInitialStackState();
void SetActiveMultistringChooser(u8 messageId);
void SetActiveAbilityPopupOverride(u16 messageId);
void SetActiveStackBattler(u8 battler, u8 number);
void SetActiveStatChanger(int stat, s8 change);
AbilityEnum GetInnateInSlot(int level, SpeciesEnum species, u8 position, u32 personality, u8 isPlayer);
void ClearMiscTurnFlags();
u8 StabMultiplierInHalves(u8 battler, u8 moveType, MoveEnum move);
bool32 IsHealingMoveEffect(MoveBehaviorEnum effect);
AbilityEnum IsMagicGuardProtected(int battler);
#define ABSORB_RESULT_HEAL 1 << 0
#define ABSORB_RESULT_STAT 1 << 1
#define ABSORB_RESULT_FLASH_FIRE 1 << 2
#define ABSORB_RESULT_EVAPORATE 1 << 3
int TestAbsorbingAbilitiesOnly(int target, int gActiveBattler, MoveEnum move, int moveType);
int TestAbsorbingAbilities(int battler, int battlerAtk, MoveEnum move, int moveType, int* statId, u16* ability);
u16 CalculateAbilityMultipliers(
    int battlerAtk, int battlerDef, MoveEnum move, int moveType, int basePower, int typeEffectivenessMultiplier, int isCrit, u16* resistanceMultiplier);
int TestImmunityAbilitiesOnly(int battler, int attacker, MoveEnum move, int moveType);
int TestImmunityAbilities(int battler, int attacker, MoveEnum move, int moveType, const u8** immunityScript, u8* overrideBattler, u16* abilityPopup);
#define MUL_MODIFIER(modifier, val) MulModifier(modifier, UQ_4_12(val))
u16 DivideModifier(u16 mod1, u16 mod2);
void MulModifier(u16* modifier, u16 val);
u32 ApplyModifier(u16 modifier, u32 val);
int CanBattlerHeal(int battler);
int BenefitsFromStatBuffs(int battler);
AbilityEnum IsComatose(int battler);
int IsBloodStainAffected(int battler);
AbilityEnum IsUnaware(int battler);
int GetOncePerTurnAbilityCounter(int battler, AbilityEnum ability);
void SetOncePerTurnAbilityCounter(int battler, AbilityEnum ability, int value);
int HasRipenEffect(int battler);
int IsDance(int attacker, MoveEnum move);
int HasAnyStatusOrAbility(int battler);
void RepopulateAbilities(int battler);
AbilityEnum GetBattlerAbility(int battler);
void HandleFollowupAttackAbilities(int battler, int target, MoveEnum move);
int CheckAndSetOncePerTurnAbility(int battler, AbilityEnum ability);
AbilityEnum IsStickyHold(int battler);
AbilityEnum HasChloroplast(int battler);
AbilityEnum HasAuroraBorealis(int battler);
AbilityEnum HasRedirectionAbility(int battlerAtk, int battlerDef, MoveEnum move, int type);
AbilityEnum HasGrappler(int battler);
AbilityEnum HasMirrorArmor(int battler);
int CanRaiseStat(int battler, int stat);
int CanLowerStat(int battler, int stat);
bool8 UseEntryMove(u8 battler, AbilityEnum ability, u16 extraMove, u8 movePower);
int UseIntimidateClone(AbilityEnum abilityToCheck, int battler);
bool32 TryRemoveScreens(u8 battler);
void DisableSwitchInAbility(u8 battlerId, AbilityEnum ability);
bool32 TryChangeBattleTerrain(u32 battler, u32 statusFlag, u8* timer);
AbilityEnum HasSkillLink(int battler);
int IsMegaLauncherBoosted(int battler, MoveEnum move);
int IsIronFistBoosted(int battler, MoveEnum move);
int IsStrikerBoosted(int battler, MoveEnum move);
AbilityEnum IsUnnerveAbilityOnOpposingSide(u8 battlerId);
AbilityEnum IgnoresBurnAtkDrop(int battler);
AbilityEnum IgnoresFrostbiteSpatkDrop(int battler);
int IsStatusImmune(u8 battlerId, StatusCheckEnum status);
u16 IsPowderImmune(int battler, int checkMoldBreaker);
s8 GetSingleUseAbilityCountByIndex(u8 battler, int index);
void SetSingleUseAbilityCountByIndex(u8 battler, int index, u8 value);

MultihitType GetMultihitType(int battler, MoveEnum move);

// Ability checks
bool32 IsRolePlayBannedAbilityAtk(AbilityEnum ability);
bool32 IsRolePlayBannedAbility(AbilityEnum ability);
bool32 IsWorrySeedBannedAbility(AbilityEnum ability);
bool32 IsGastroAcidBannedAbility(AbilityEnum ability);
bool32 IsEntrainmentBannedAbilityAttacker(AbilityEnum ability);
bool32 IsEntrainmentTargetOrSimpleBeamBannedAbility(AbilityEnum ability);

bool32 CanSleep(u8 battlerId);
bool32 CanBePoisoned(u8 battlerAttacker, u8 battlerTarget, MoveEnum move);
bool32 CanBeBurned(u8 battlerId);
bool32 CanBeParalyzed(u8 battlerAttacker, u8 battlerTarget);
bool32 CanBeParalyzedIgnoreType(u8 battlerAttacker, u8 battlerTarget);
bool32 CanBeFrozen(u8 battlerId);
bool32 CanGetFrostbite(u8 battlerId);
bool32 CanBeConfused(u8 battlerId);
bool32 CanBleed(u8 battlerId);
int CanInfatuate(int battlerAtk, int battlerDef);
bool32 IsBattlerTerrainAffected(u8 battlerId, u32 terrainFlag);
int IsTerrainActive(int terrainFlag);
int IsWeatherActive(int weather);
u8 getMonotypeChampType(void);
int NaturalGiftPriority(ItemEnum item);
int IsPoisonedForMove(int battler);

// Move checks
bool8 IsTwoStrikesMove(MoveEnum move);

u32 CalcFinalDmg(u32 dmg, MoveEnum move, u8 battlerAtk, u8 battlerDef, u8 moveType, u16 typeEffectivenessModifier, bool32 isCrit, bool32 updateFlags);
void MulByTypeEffectiveness(u16* modifier, MoveEnum move, u8 moveType, u8 battlerDef, u8 defType, u8 battlerAtk, bool32 recordAbilities);

u32 GetIllusionMonSpecies(u32 battlerId);
s32 DoMoveDamageCalcBattleMenu(MoveEnum move, u8 battlerAtk, u8 battlerDef, u8* moveType, u8 critRoll, u8 randomFactor, u16* typeEffectivenessModifier);

// Monotype funcs
bool8 IsBattlerCursed(u8 battler);
void MakePlayerTeamAsleep(void);

bool8 IsEvasionClauseDisablingMove(u8 battlerId, MoveEnum move);
#endif  // GUARD_BATTLE_UTIL_H
