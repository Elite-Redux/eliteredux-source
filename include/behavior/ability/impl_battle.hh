#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "behavior/ability/constants.hh"

AbilityEnum HasChloroplast(int battler);
int IsBreakable(AbilityEnum ability);
int IsUnsuppressable(AbilityEnum ability);
int IsPersistentOrUnsuppressable(AbilityEnum ability);
int IsRandomizerBanned(AbilityEnum ability);
AbilityEnum HasUnaware(int battler);
AbilityEnum IsSoundproof(int battler);
AbilityEnum HasNoRecoil(int battler);
int RecoilReductionCount(int battler);
AbilityEnum HasSkillLink(int battler);
int IsAbsorbUp2(AbilityEnum ability);
int ResistsFortKnox(AbilityEnum ability);
AbilityEnum HasAdaptability(int battler);
AbilityEnum HasMagicGuard(int battler);
AbilityEnum HasMagicBounce(int battler);
int IsGroundImmuneAbility(AbilityEnum ability);
AbilityEnum HasGroundImmuneAbility(int battler);
AbilityEnum HasMegaLauncherBoost(int battler);
AbilityEnum HasUnnerve(int battler);
AbilityEnum IgnoresBurnAttackDrop(int battler);
AbilityEnum IgnoresFrostbiteSpatkDrop(int battler);
AbilityEnum CanInfatuateAny(int battler);
int RemovesStatusAtTurnEnd(AbilityEnum ability);
AbilityEnum IsTauntImmune(int battler);
AbilityEnum FoesMinRoll(int battler);
AbilityEnum HasPowderImmuneAbility(int battler);
AbilityEnum HasSandImmuneAbility(int battler);
AbilityEnum HasHailImmuneAbility(int battler);
AbilityEnum IsToxicTerrainImmune(int battler);
AbilityEnum IsStealthRockImmune(int battler);
AbilityEnum HasSturdy(int battler);
AbilityEnum HasMyceliumMight(int battler);
AbilityEnum HasGoodAsGold(int battler);
AbilityEnum HasComatose(int battler);
int HasCowardTriggered(int battler);
AbilityEnum HasQuickFeet(int battler);
AbilityEnum HasQuickDraw(int battler);
AbilityEnum IgnoresEvasion(int battler);
int GetAvailableAnticipationIndex(int target);
int IsPressureAffected(int battler);
void SetUnburdenState(int battler, u32 value);
u32 GetUnburdenState(int battler);
AbilityEnum HasMirrorArmor(int battler);
AbilityEnum HasShieldDust(int battler);
AbilityEnum HasInnerFocus(int battler);
AbilityEnum HasGrappler(int battler);
AbilityEnum HasAccelerate(int battler);
AbilityEnum HasContrary(int battler);
AbilityEnum HasClearBody(int battler);
AbilityEnum BlocksAllStatDrops(int battler, int fromSelf);
AbilityEnum BlocksStatDropsOfType(int battler, int stat);
int IsSkillSwapBanned(AbilityEnum ability);
int IsRolePlayBanned(AbilityEnum ability);
int IsSimpleBeamBanned(AbilityEnum ability);
AbilityEnum IgnoresRedirection(int battler);
AbilityEnum AbilityMakesMoveSpread(int battler, MoveEnum move);
int IsAlwaysStab(AbilityEnum ability);
int AbilityGetsBonusStab(AbilityEnum ability, Type type);
int GetsBonusStab(int battler, Type type);
int PerformOnEntry(int battler, AbilityEnum ability);
int TestAbsorbingAbilities(int battler, MoveEnum move, Type moveType, AbilityEnum* absorbingAbility, int* statId);
int TestAllImmunityAbilities(int battler, int attacker, MoveEnum move, Type moveType, const u8** immunityScript, u8* overrideBattler, u16* abilityPopup);
void CalcDefensiveMultipliers(
    int battler, int attacker, MoveEnum move, Type moveType, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier);
AbilityEnum Infiltrates(int battler, MoveEnum move, InfiltrateType type);
AbilityEnum TestDoesDisguiseBlockMove(int target, MoveEnum move);
AbilityEnum DoesDisguiseBlockMove(int target, MoveEnum move, SpeciesEnum* newSpecies);
void HandleOnWeather(int battler);
void HandleOnTerrain(int battler);
int InvokeToxicWasteForMonotypeChamp();
int PerformOnEndTurn(int battler, AbilityEnum ability);
int PerformOnAttacker(int battler, int target, AbilityEnum ability, MoveEnum move, Type moveType);
int PerformOnDefender(int battler, int attacker, AbilityEnum ability, MoveEnum move, Type moveType);
void HandleRecoilAbilities(int battler, int moveDamage, Type moveType);
int HandleAllOnReactive(AbilityCallType callType);
void HandleOnBattlerFaints(int attacker, int fainted, MoveEnum move, Type moveType);
MultihitType HandleParentalBond(int battler, int hasFortKnox, MoveEnum move, Type moveType);
void CalcOffensiveMultipliers(
    int battler, int target, MoveEnum move, Type moveType, int basePower, int typeEffectivenessModifier, int isCrit, u16* resistance, u16* modifier);
int OnMoveTypeSingleAbility(AbilityEnum ability, MoveEnum move, Type moveType);
int OnMoveTypeForBattler(int battler, MoveEnum move, Type moveType, u8* ateBoost);
void CalculateStatsFromAbilities(int battler, int statId, u32* stat);
AccuracyPriority CalculateAccuracyFromAbilities(int attacker, int target, MoveEnum move, Type moveType, int* accuracy);


#ifdef __cplusplus
}
#endif
