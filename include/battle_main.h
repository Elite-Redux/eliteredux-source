#ifndef GUARD_BATTLE_MAIN_H
#define GUARD_BATTLE_MAIN_H

#include "global.h"

struct TrainerMoney {
    u8 classId;
    u8 value;
};

struct TrainerBall {
    u8 classId;
    u8 Ball;
};

struct UnknownPokemonStruct4 {
    /*0x00*/ SpeciesEnum species;
    /*0x02*/ u16 heldItem;
    /*0x04*/ u8 nickname[POKEMON_NAME_LENGTH + 1];
    /*0x0F*/ u8 level;
    /*0x10*/ u16 hp;
    /*0x12*/ u16 maxhp;
    /*0x14*/ u32 status;
    /*0x18*/ u32 personality;
    /*0x1C*/ u8 gender;
    /*0x1D*/ u8 language;
};

union SpeedValue {
    struct {
        // Compiler lays this out in reverse order
        u16 effectiveSpeed;
        u16 goesLastNegation:2;
        u16 goesFirst:2;
        u16 priority:4;
        u16 dazedNegation:1;
        u16 afterYou:1;
    };
    u32 comparable;
};

#define TYPE_NAME_LENGTH 6
#define ABILITY_NAME_LENGTH 20

// defines for the 'DoBounceEffect' function
#define BOUNCE_MON 0x0
#define BOUNCE_HEALTHBOX 0x1

void CB2_InitBattle(void);
void BattleMainCB2(void);
void CB2_QuitRecordedBattle(void);
void sub_8038528(struct Sprite *sprite);
void sub_8038A04(void);  // unused
void VBlankCB_Battle(void);
void SpriteCB_VsLetterDummy(struct Sprite *sprite);
void SpriteCB_VsLetterInit(struct Sprite *sprite);
void CB2_InitEndLinkBattle(void);
u32 GetBattleBgTemplateData(u8 arrayId, u8 caseId);
u32 GetBattleWindowTemplatePixelWidth(u32 setId, u32 tableId);
void SpriteCb_WildMon(struct Sprite *sprite);
void SpriteCallbackDummy_2(struct Sprite *sprite);
void SpriteCB_FaintOpponentMon(struct Sprite *sprite);
void SpriteCb_ShowAsMoveTarget(struct Sprite *sprite);
void SpriteCb_HideAsMoveTarget(struct Sprite *sprite);
void SpriteCb_OpponentMonFromBall(struct Sprite *sprite);
union SpeedValue GetMoveSpeed(int battler, int ignoreChosenMove);
void SpriteCB_BattleSpriteStartSlideLeft(struct Sprite *sprite);
void SpriteCB_FaintSlideAnim(struct Sprite *sprite);
void DoBounceEffect(u8 battlerId, u8 b, s8 c, s8 d);
void EndBounceEffect(u8 battlerId, bool8 b);
void SpriteCb_PlayerMonFromBall(struct Sprite *sprite);
void sub_8039E60(struct Sprite *sprite);
void SpriteCB_TrainerThrowObject(struct Sprite *sprite);
void sub_8039E9C(struct Sprite *sprite);
void BeginBattleIntroDummy(void);
void BeginBattleIntro(void);
void SwitchInClearSetData(void);
void FaintClearSetData(void);
void BattleTurnPassed(void);
u8 IsRunningFromBattleImpossible(void);
void SwitchPartyOrder(u8 battlerId);
void SwapTurnOrder(u8 id1, u8 id2);
#define TOTAL_SPEED_FULL 0
#define TOTAL_SPEED_PRIMARY 1
#define TOTAL_SPEED_SECONDARY 2
#define TOTAL_SPEED_QUASH 3
u32 GetBattlerTotalSpeedStat(u8 battlerId, u8 calcType);
u16 GetChosenMove(u32 battlerId);
u16 IsMyceliumMightActive(u32 battlerId);
s8 GetChosenMovePriority(u32 battlerId, u32 target);
s8 GetMovePriority(u32 battlerId, MoveEnum move, u32 target);
u8 GetWhoStrikesFirst(u8 battlerId1, u8 battlerId2, bool8 ignoreChosenMoves);
int SortBattlersExcept(u8 *battlerArray, int ignoreChosenMoves, int except);
int GetFastestBattler(int ignoreChosenMoves, int except);
void RunBattleScriptCommands_PopCallbacksStack(void);
void RunBattleScriptCommands(void);
bool8 TryRunFromBattle(u8 battlerId);
void TurnStructsClear(void);
void SetTypeBeforeUsingMove(MoveEnum move, u8 battlerAtk);
u8 GetTypeBeforeUsingMove(MoveEnum move, u8 battlerAtk);
void ApplyTypeOverrideInformation(MoveEnum move, int battlerAtk, int moveType, int ateBoost);
s32 GetHighestLevelInPlayerParty(void);
u16 selectMoves(SpeciesEnum species, u8 i, u16 atk, u16 spAtk);
u8 GetMonMoveType(MoveEnum move, struct Pokemon *mon, bool8 disableRandomizer);
bool32 IsWildMonSmart(void);
void RecalculateMoveOrder(int from, int ignoreChosenMove);
extern struct UnknownPokemonStruct4 gMultiPartnerParty[MULTI_PARTY_SIZE];

extern const struct SpriteTemplate gUnusedBattleInitSprite;
extern const struct OamData gOamData_BattleSpriteOpponentSide;
extern const struct OamData gOamData_BattleSpritePlayerSide;
extern const u8 gTypeNames[NUMBER_OF_MON_TYPES][TYPE_NAME_LENGTH + 1];
extern const struct TrainerMoney gTrainerMoneyTable[];
extern const struct TrainerBall gTrainerBallTable[];

extern const u8 gStatusConditionString_PoisonJpn[8];
extern const u8 gStatusConditionString_SleepJpn[8];
extern const u8 gStatusConditionString_ParalysisJpn[8];
extern const u8 gStatusConditionString_BurnJpn[8];
extern const u8 gStatusConditionString_IceJpn[8];
extern const u8 gStatusConditionString_ConfusionJpn[8];
extern const u8 gStatusConditionString_LoveJpn[8];
extern const u8 gStatusConditionString_BleedJpn[8];

extern const u8 *const gStatusConditionStringsTable[8][2];

#endif  // GUARD_BATTLE_MAIN_H
