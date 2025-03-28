#ifndef GUARD_BATTLE_SCRIPT_COMMANDS_H
#define GUARD_BATTLE_SCRIPT_COMMANDS_H

#include "abilities.hh"
#include "battle_ai_new.h"
#include "battle_util.h"
#include "constants/pokemon.h"
#include "global.h"

#define WINDOW_CLEAR 0x1
#define WINDOW_x80 0x80

struct StatFractions {
    u8 dividend;
    u8 divisor;
};

// Intimidate Clone Data
enum {
    I_CLONE_INTIMIDATE,
    I_CLONE_SCARE,
    I_CLONE_FEARMONGER,
    I_CLONE_YUKI_ONNA,
    I_CLONE_MONKEY_BUSSINESS,
    I_CLONE_MALICIOUS,
    I_CLONE_TERRIFY,
    I_CLONE_PETRIFY,
    I_CLONE_GLEAM_EYES,
    I_CLONE_CHAMPIONS_ENTRANCE,
    NUM_INTIMIDATE_CLONES
};

struct IntimidateCloneData {
    u16 ability;
    u8 statsLowered[3];    // atk, def, speed
    u8 numStatsLowered:2;  // 1 - 3
    bool8 targetBoth:1;
    u8 statChange:4;
};

extern const struct IntimidateCloneData gIntimidateCloneData[NUM_INTIMIDATE_CLONES];

int GetParentalBondCount(int battler, MultihitType parentalBondType);
MultihitType GetParentalBondType(int battler, int target, MoveEnum move, int moveType);
void CheckForBadEggs(void);
s32 CalcCritChanceStage(u8 battlerAtk, u8 battlerDef, MoveEnum move, bool32 recordAbility);
s8 GetInverseCritChance(u8 battlerAtk, u8 battlerDef, MoveEnum move);
u32 GetTotalAccuracy(u32 battlerAtk, u32 battlerDef, MoveEnum move, struct MoveState* moveState);
u8 GetBattlerTurnOrderNum(u8 battlerId);
bool32 NoAliveMonsForEitherParty(void);
void SetMoveEffect(bool32 primary, u32 certain);
bool32 CanBattlerSwitch(u32 battlerId);
void BattleDestroyYesNoCursorAt(u8 cursorPosition);
void BattleCreateYesNoCursorAt(u8 cursorPosition);
void BattleDestroyYesNoCursorAt_Two(u8 cursorPosition);
void BattleCreateYesNoCursorAt_Two(u8 cursorPosition);
void BufferMoveToLearnIntoBattleTextBuff2(void);
void HandleBattleWindow(u8 xStart, u8 yStart, u8 xEnd, u8 yEnd, u8 flags);
bool8 UproarWakeUpCheck(u8 battlerId);
bool32 DoesSubstituteBlockMove(u8 battlerAtk, u8 battlerDef, MoveEnum move);
bool32 DoesDisguiseBlockMove(u8 battlerAtk, u8 battlerDef, MoveEnum move);
s8 RemainingNoDamageHits(u8 battler);
u16 GetNoDamageAbility(u8 battler);
bool32 CanPoisonType(u8 battlerAttacker, u8 battlerTarget, MoveEnum move);
bool32 CanParalyzeType(u8 battlerAttacker, u8 battlerTarget);
bool32 CanUseLastResort(u8 battlerId);
u32 IsFlowerVeilProtected(u32 battler);
u32 IsLeafGuardProtected(u32 battler);
bool32 IsShieldsDownProtected(u32 battler);
u32 IsAbilityStatusProtected(u32 battler, StatusCheckEnum status);
int GetMoveEffectChance(int battler, MoveEnum move, int moveEffect, int baseChance);
#define RESET_ALL_STATS 0
#define RESET_STAT_BUFFS 1
#define RESET_STAT_DROPS -1
bool32 TryResetBattlerStatChanges(u8 battler, s8 comparison);
bool32 CanCamouflage(u8 battlerId);
u16 GetNaturePowerMove(void);
u16 GetSecretPowerMoveEffect(void);
void StealTargetItem(u8 battlerStealer, u8 battlerItem);
void RemoveItem(u8 battler);
u8 GetCatchingBattler(void);
u32 IsDesertCloakProtected(u32 battler);
u8 getStatToLowerFromIntimidateClone(u16 ability, u8 num);
u16 IsBattlerImmuneToLowerStatsFromIntimidateClone(u8 battler);
void SetStatChanger(u8 statId, s8 change);
s8 ChangeStatBuffsImplicit(s8 statValue, u32 statId, u32 flags, const u8* BS_ptr);
s8 ChangeStatBuffs(u8 battler, s8 statValue, u32 statId, u32 flags, const u8* BS_ptr);
u8 GetFirstFaintedPartyIndex(u8 battler);
void SetCudChew(u32 battlerId, u32 itemId);
void SetBattlerAffectedFlag(int attacker, int target, int ability);
void ClearBattlerAffectedFlag(int attacker, int target, int ability);
int GetWeatherChangeMultistringChooser(int weather);
int ShouldSetMoldBreaker(int battler, MoveEnum move);
int EatTargetBerry(int battler, int target);
void ClearPowerOfAlchemyState(int alchemyBattler, int battler);
int UpdateBattlerItem(int battler, int newItem);
int Infiltrates(int battler, MoveEnum move, InfiltrateType type);
int SetEncore(int target);
bool32 HasAttackerFaintedTarget(void);
int HasFortKnox(int battler);

extern void (*const gBattleScriptingCommandsTable[])(void);
extern const u8 gBattlePalaceNatureToMoveGroupLikelihood[NUM_NATURES][4];
extern const struct StatFractions gAccuracyStageRatios[];

#endif  // GUARD_BATTLE_SCRIPT_COMMANDS_H
