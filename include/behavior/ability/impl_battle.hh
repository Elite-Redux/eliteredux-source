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

#ifdef __cplusplus
}
#endif
