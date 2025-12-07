#ifndef GUARD_POKEMON_H
#define GUARD_POKEMON_H

#include "constants/pokemon.h"
#include "generated/constants/species.h"
#include "sprite.h"
#include "constants/region_map_sections.h"
#include "constants/pokemon_config.h"
#include "constants/map_groups.h"
#include "tmhm_struct.h"
#include "generated/constants/battle_move_effects.h"
#include "generated/constants/abilities.h"

#define GET_BASE_SPECIES_ID(speciesId) (GetFormSpeciesId(speciesId, 0))

#define MAX_BEAUTY 255
#define MAX_IVS 31
#define EVENT_LEGAL 1
#define NEW_PERSONALITY 1

#define REMOVE_RIBBONS

struct OldBoxPokemon {
    // Words 1 & 2: PID + Trainer ID
    u32 personality;
    u32 otId;

    // Words 3-5: Pokémon nickname (12 chars)
    u8 nickname[POKEMON_NAME_LENGTH];

    // Word 6: Species + Experience points (used to derive level)
    u32 move1:10;
    u32 experience:21;
    u32 attackDown:1;

    // Words 7 & 8: moves, language, gender, friendship, Pokérus, ppBonuses
    u32 move2:10;
    u32 move3:10;
    u32 language:3;
    u32 isAlpha:1;
    u32 friendship:8;

    u32 species:16;
    u32 move4:10;
    u32 hpType:5;  // Will be used for tera type too
    u32 isEventMon:1;

    // Words 9 - 11: EV's
    u8 hpEV;
    u8 attackEV;
    u8 defenseEV;
    u8 speedEV;
    u8 spAttackEV;
    u8 spDefenseEV;

    // Word 12: miscellaneous data; item, nature, Egg and origin data
    u32 heldItem:10;
    u32 nature:5;
    u32 isEgg:1;
    u32 metLevel:7;
    u32 pokeball:5;  // 31 balls
    u32 isShiny:2;   // 0 = not shiny, 1 = shiny, 2 = rare shiny, 3 = legendary shiny
    u32 maxShiny:2;  // 0 = not shiny, 1 = shiny, 2 = rare shiny, 3 = legendary shiny

    // Words 13 & 14: Trainer name + met location
    u8 metLocation;
    u8 otName[PLAYER_NAME_LENGTH];

    u8 markings:4;
    u8 abilityNum:2;
    u8 speedDown:1;
    u8 otGender:1;
};

STATIC_ASSERT(MOVES_COUNT < 1 << 11, movesFitInBoxPokemon)
struct BoxPokemon {
    u32 personality;
    u32 otId;

    u32 move1:11;
    u32 experience:21;

    u32 move2:11;
    u32 move3:11;
    u32 friendship:8;
    u32 isEventMon:1;
    u32 isAlpha:1;

    u32 species:16;
    u32 move4:11;
    u32 hpType:5;  // Will be used for tera type too

    // Word 12: miscellaneous data; item, nature, Egg and origin data
    u32 heldItem:10;
    u32 nature:5;
    u32 isEgg:1;
    u32 language:3;
    u32 metLevel:7;
    u32 isShiny:2;   // 0 = not shiny, 1 = shiny, 2 = rare shiny, 3 = legendary shiny
    u32 maxShiny:2;  // 0 = not shiny, 1 = shiny, 2 = rare shiny, 3 = legendary shiny
    u32 abilityNum:2;

    // Words 9 - 11: EV's
    u8 hpEV;
    u8 attackEV;
    u8 defenseEV;
    u8 speedEV;
    u8 spAttackEV;
    u8 spDefenseEV;

    // Words 13 & 14: Trainer name + met location
    u8 metLocation;
    u8 otName[PLAYER_NAME_LENGTH];

    // Words 3-5: Pokémon nickname (12 chars)
    u8 nickname[POKEMON_NAME_LENGTH];

    u8 pokeball:5;  // 31 balls
    u8 speedDown:1;
    u8 otGender:1;
    u8 isDisabled:1;

    u8 markings:4;
};

STATIC_ASSERT(sizeof(struct OldBoxPokemon) == sizeof(struct BoxPokemon), BoxPokemonSizesAlign)

struct Pokemon {
    struct BoxPokemon box;
    u8 pp[MAX_MON_MOVES];
    u32 status;
    u8 level;
    u8 mail;
    u16 hp;
    u16 maxHP;
    u16 attack;
    u16 defense;
    u16 speed;
    u16 spAttack;
    u16 spDefense;
};

struct Unknown_806F160_Struct {
    u32 field_0_0:4;
    u32 field_0_1:4;
    u32 field_1:8;
    u16 magic:8;
    u32 field_3_0:4;
    u32 field_3_1:4;
    void* bytes;
    u8** byteArrays;
    struct SpriteTemplate* templates;
    struct SpriteFrameImage* frameImages;
};

struct BattlePokemon {
    u32 experience;
    u32 personality;
    u32 status1;
    u32 status2;
    u32 otId;
    SpeciesEnum species;
    union {
        struct {
            u16 attack;
            u16 defense;
            u16 speed;
            u16 spAttack;
            u16 spDefense;
        } __attribute__((packed, aligned(2)));
        u16 stats[5];
    } __attribute__((packed, aligned(2)));
    MoveEnum moves[MAX_MON_MOVES];
    AbilityEnum abilities[TOTAL_ABILITY_COUNT];
    AbilityEnum extraAbilities[HELL_MODE_EXTRA_ABILITIES];
    u16 hp;
    u16 maxHP;
    u16 item;
    s8 statStages[NUM_BATTLE_STATS];
    u8 type1;
    u8 type2;
    u8 type3;
    u8 pp[MAX_MON_MOVES];
    u8 level;
    u8 friendship;
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u8 ppBonuses;
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u8 nature;
    u8 hpType;
    bool8 wasalreadytotemboosted:1;
    u8 speedDown:1;
    u8 abilityNum:2;
};
// introduced as direct stat increase, similar to stat boosts but cannot be copied or stuff like that
struct RawStatsLevel {
    u8 extraAttackLevel;
    u8 extraDefenseLevel;
    u8 extraSpAttackLevel;
    u8 extraSpDefenseLevel;
    u8 extraSpeedLevel;
};

#define F_ULTRA_BEAST (1 << 0)
#define F_ALOLAN_FORM (1 << 1)
#define F_GALARIAN_FORM (1 << 2)
#define F_TWO_HEADED (1 << 3)
#define F_THREE_HEADED (1 << 4)
#define F_TAG_TEAM (1 << 5)

typedef struct BaseStats {
    /* 0x00 */ u8 baseHP;
    /* 0x01 */ u8 baseAttack;
    /* 0x02 */ u8 baseDefense;
    /* 0x03 */ u8 baseSpeed;
    /* 0x04 */ u8 baseSpAttack;
    /* 0x05 */ u8 baseSpDefense;
    /* 0x06 */ Type type1;
    /* 0x07 */ Type type2;
    /* 0x08 */ u8 catchRate;
    /* 0x09 */ u16 expYield;
    /* 0x0A */ u16 evYield_HP:2;
    /* 0x0A */ u16 evYield_Attack:2;
    /* 0x0A */ u16 evYield_Defense:2;
    /* 0x0A */ u16 evYield_Speed:2;
    /* 0x0B */ u16 evYield_SpAttack:2;
    /* 0x0B */ u16 evYield_SpDefense:2;
    /* 0x0C */ u16 item1;
    /* 0x0E */ u16 item2;
    /* 0x10 */ u8 genderRatio;
    /* 0x11 */ u8 eggCycles;
    /* 0x12 */ u8 friendship;
    /* 0x13 */ u8 growthRate;
    /* 0x14 */ u8 eggGroup1;
    /* 0x15 */ u8 eggGroup2;
    /* 0x16 */ AbilityEnum abilities[NUM_ABILITY_SLOTS];
    /* 0x19 */ u8 safariZoneFleeRate;
    /* 0x1A */ u8 bodyColor:7;
    u8 noFlip:1;
    u8 flags;
    /* 0x16 */ AbilityEnum innates[NUM_INNATE_PER_SPECIES];
    /* 0x15 */ u16 shopPrice;
    u8 tier;
    u8 numShinies:2;  // 1 if it has a rare, 2 if it has legendary, 3 if it has both
} BaseStats;

typedef enum {
    USE_BASE_SPLIT,
    USE_HIGHEST_OFFENSE,
    USE_LOWEST_DEFENSE,
    HITS_SPDEF,
    HITS_DEF,
    USE_HIGHEST_DAMAGE,
} MoveSplitType;

#include "constants/battle_config.h"
struct BattleMove {
    u32 flags;
    MoveBehaviorEnum effect;
    u16 target;
    u16 argument;
    u8 power;
    Type type;
    Type type2;
    u8 accuracy;
    u8 pp;
    u8 secondaryEffectChance;
    s8 priority;
    u8 split;
    u8 parentalBondBanned:1;
    u8 twoTurnMove:1;
    u8 arrowBased:1;
    u8 hornBased:1;
    u8 airBased:1;
    u8 alwaysCrit:1;
    u8 hammerBased:1;
    u8 throwingBased:1;
    u8 doubleDamageVsMega:1;
    MoveSplitType splitFlag:3;
    u8 everyOtherTurn:1;
    u8 lunar:1;
    u8 metronomeBanned:1;
    u8 copycatBanned:1;
    u8 sleepTalkBanned:1;
    u8 mimicBanned:1;
    u8 contact:1;
    u8 drill:1;
};

struct SpindaSpot {
    u8 x, y;
    u16 image[16];
};

typedef struct LevelUpMove {
    u16 move;
    u16 level;
} LevelUpMove;

struct TrainerMonSpread {
    u8 EVs[6];
    u8 IVs[6];
    u8 nature;
};

typedef struct Evolution {
    u16 method;
    u16 param;
    SpeciesEnum targetSpecies;
} Evolution;

struct FormChange {
    u16 method;
    SpeciesEnum targetSpecies;
    u16 param1;
    u16 param2;
};

#define NUM_UNOWN_FORMS 28

#define GET_UNOWN_LETTER(personality)                                                                                     \
    (((((personality) & 0x03000000) >> 18) | (((personality) & 0x00030000) >> 12) | (((personality) & 0x00000300) >> 6) | \
      (((personality) & 0x00000003) >> 0)) %                                                                              \
     NUM_UNOWN_FORMS)

extern u8 gPlayerPartyCount;
extern struct Pokemon gPlayerParty[PARTY_SIZE];
extern u8 gEnemyPartyCount;
extern struct Pokemon gEnemyParty[PARTY_SIZE];
extern struct SpriteTemplate gMultiuseSpriteTemplate;

extern const struct BattleMove gBattleMoves[];
extern const struct TrainerMonSpread gSets[];
extern const u8 gFacilityClassToPicIndex[];
extern const u8 gFacilityClassToTrainerClass[];
extern const struct BaseStats gBaseStats[];
extern const u8* const gItemEffectTable[];
extern const u32 gExperienceTables[][MAX_LEVEL + 1];
extern const struct Evolution* const gEvolutionTable[REAL_SPECIES_COUNT];
extern const struct Evolution* const gFormChangeTable[REAL_SPECIES_COUNT];
extern const struct LevelUpMove* const gLevelUpLearnsets[];
extern const u8 gPPUpGetMask[];
extern const u8 gPPUpSetMask[];
extern const u8 gPPUpAddMask[];
extern const u8 gStatStageRatios[MAX_STAT_STAGE + 1][2];
extern const u16 gLinkPlayerFacilityClasses[];
extern const struct SpriteTemplate gBattlerSpriteTemplates[];
extern const s8 gNatureStatTable[][5];
extern const u16 gSpeciesToNationalPokedexNum[NUM_SPECIES];
extern const SpeciesEnum* const gFormSpeciesIdTables[REAL_SPECIES_COUNT];

void ZeroBoxMonData(struct BoxPokemon* boxMon);
void ZeroMonData(struct Pokemon* mon);
void ZeroPlayerPartyMons(void);
void ZeroEnemyPartyMons(void);
void CreateMon(struct Pokemon* mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId);
void CreateBoxMon(
    struct BoxPokemon* boxMon, SpeciesEnum species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId);
void CreateMonWithNature(struct Pokemon* mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 nature);
void CreateMonWithGenderNatureLetter(struct Pokemon* mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 gender, u8 nature, u8 unownLetter);
void CreateMaleMon(struct Pokemon* mon, SpeciesEnum species, u8 level);
void CreateMonWithIVsPersonality(struct Pokemon* mon, SpeciesEnum species, u8 level, u32 ivs, u32 personality);
void CreateMonWithIVsOTID(struct Pokemon* mon, SpeciesEnum species, u8 level, u8* ivs, u32 otId);
void CreateMonWithEVSpread(struct Pokemon* mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 evSpread);
void CreateBattleTowerMon(struct Pokemon* mon, struct BattleTowerPokemon* src);
void CreateBattleTowerMon2(struct Pokemon* mon, struct BattleTowerPokemon* src, bool8 lvl50);
void CreateApprenticeMon(struct Pokemon* mon, const struct Apprentice* src, u8 monId);
void CreateMonWithEVSpreadNatureOTID(struct Pokemon* mon, SpeciesEnum species, u8 level, u8 nature, u8 fixedIV, u8 evSpread, u32 otId, u16 abilityNum);
void ConvertPokemonToBattleTowerPokemon(struct Pokemon* mon, struct BattleTowerPokemon* dest);
void CreateEventLegalMon(
    struct Pokemon* mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId);
bool8 ShouldIgnoreDeoxysForm(u8 caseId, u8 battlerId);
u16 GetUnionRoomTrainerPic(void);
u16 GetUnionRoomTrainerClass(void);
void CreateEventLegalEnemyMon(void);
void CalculateMonStats(struct Pokemon* mon);
void BoxMonToMon(const struct BoxPokemon* src, struct Pokemon* dest);
u8 GetLevelFromMonExp(struct Pokemon* mon);
u8 GetLevelFromBoxMonExp(struct BoxPokemon* boxMon);
u16 GiveMoveToMon(struct Pokemon* mon, MoveEnum move);
u16 GiveMoveToBoxMon(struct BoxPokemon* boxMon, MoveEnum move);
u16 GiveMoveToBattleMon(struct BattlePokemon* mon, MoveEnum move);
void SetMonMoveSlot(struct Pokemon* mon, MoveEnum move, u8 slot);
void SetBattleMonMoveSlot(struct BattlePokemon* mon, MoveEnum move, u8 slot);
void GiveMonInitialMoveset(struct Pokemon* mon);
void GiveBoxMonInitialMoveset(struct BoxPokemon* boxMon);
u16 MonTryLearningNewMove(struct Pokemon* mon, bool8 firstMove);
u16 MonTryLearningNewEvolutionMove(struct Pokemon* mon, bool8 firstMove);
void DeleteFirstMoveAndGiveMoveToMon(struct Pokemon* mon, MoveEnum move);
void DeleteFirstMoveAndGiveMoveToBoxMon(struct BoxPokemon* boxMon, MoveEnum move);
MoveEnum RandomizeMoves(MoveEnum moves, SpeciesEnum species, u32 personality);

u8 CountAliveMonsInBattle(u8 caseId);
#define BATTLE_ALIVE_EXCEPT_ACTIVE 0
#define BATTLE_ALIVE_ATK_SIDE 1
#define BATTLE_ALIVE_DEF_SIDE 2

#define DAY_START 4
#define NIGHT_START 18

u8 GetDefaultMoveTarget(u8 battlerId);
u8 GetMonGender(struct Pokemon* mon);
u8 GetBoxMonGender(struct BoxPokemon* boxMon);
u8 GetGenderFromSpeciesAndPersonality(SpeciesEnum species, u32 personality);
u32 GetUnownSpeciesId(u32 personality);
void SetMultiuseSpriteTemplateToPokemon(u16 speciesTag, u8 battlerPosition);
void SetMultiuseSpriteTemplateToTrainerBack(u16 trainerSpriteId, u8 battlerPosition);
void SetMultiuseSpriteTemplateToTrainerFront(u16 arg0, u8 battlerPosition);

// These are full type signatures for GetMonData() and GetBoxMonData(),
// but they are not used since some code erroneously omits the third arg.
u32 GetMonDataInternal(struct Pokemon* mon, s32 field, u8* data);
u32 GetBoxMonDataInternal(struct BoxPokemon* boxMon, s32 field, u8* data);
#define __GET_MON_DATA_PICKER__(mon, field, data, method, ...) method
#define GetMonData(mon, field, ...) \
    __GET_MON_DATA_PICKER__(mon, field, ##__VA_ARGS__, GetMonDataInternal(mon, field, __VA_ARGS__), GetMonDataInternal(mon, field, NULL))
#define GetBoxMonData(boxMon, field, ...) \
    __GET_MON_DATA_PICKER__(boxMon, field, ##__VA_ARGS__, GetBoxMonDataInternal(boxMon, field, __VA_ARGS__), GetBoxMonDataInternal(boxMon, field, NULL))

void SetMonData(struct Pokemon* mon, s32 field, const void* dataArg);
void SetBoxMonData(struct BoxPokemon* boxMon, s32 field, const void* dataArg);
void CopyMon(void* dest, void* src, size_t size);
u8 GiveMonToPlayer(struct Pokemon* mon);
u8 SendMonToPC(struct Pokemon* mon);
u8 SendSettingsMonToPC(struct Pokemon* mon);
u8 CalculatePlayerPartyCount(void);
u8 CalculateEnemyPartyCount(void);
u8 CalculatePlayerBattlerPartyCount(void);
u8 GetMonsStateToDoubles(void);
u8 GetMonsStateToDoubles_2(void);
u16 GetAbilityBySpecies(SpeciesEnum species, u8 abilityNum);
u16 GetMonAbility(struct Pokemon* mon);
void CreateSecretBaseEnemyParty(struct SecretBase* secretBaseRecord);
u8 GetSecretBaseTrainerPicIndex(void);
u8 GetSecretBaseTrainerClass(void);
bool8 IsPlayerPartyAndPokemonStorageFull(void);
bool8 IsPokemonStorageFull(void);
void GetSpeciesName(u8* name, SpeciesEnum species);
u8 CalculatePPWithBonus(u16 move, u8 ppBonuses, u8 moveIndex);
u8 CalculatePPWithBonusPlayer(u16 move, u8 ppBonuses, u8 moveIndex);
void RemoveMonPPBonus(struct Pokemon* mon, u8 moveIndex);
void RemoveBattleMonPPBonus(struct BattlePokemon* mon, u8 moveIndex);
void PokemonToBattleMon(struct Pokemon* src, struct BattlePokemon* dst);
void CopyPlayerPartyMonToBattleData(u8 battlerId, u8 partyIndex);
bool8 ExecuteTableBasedItemEffect(struct Pokemon* mon, u16 item, u8 partyIndex, u8 moveIndex);
bool8 PokemonUseItemEffects(struct Pokemon* mon, u16 item, u8 partyIndex, u8 moveIndex, u8 e);
bool8 HealStatusConditions(struct Pokemon* mon, u32 battlePartyId, u32 healMask, u8 battlerId);
u8 GetItemEffectParamOffset(u16 itemId, u8 effectByte, u8 effectBit);
u8* UseStatIncreaseItem(u16 itemId);
u8 GetNature(struct Pokemon* mon);
u8 GetNatureFromPersonality(u32 personality);
u16 GetEvolutionTargetSpecies(struct Pokemon* mon, u8 type, u16 evolutionItem, u16 tradePartnerSpecies);
u16 HoennPokedexNumToSpecies(u16 hoennNum);
u16 NationalPokedexNumToSpecies(u16 nationalNum);
u16 NationalToHoennOrder(u16 nationalNum);
u16 SpeciesToNationalPokedexNum(SpeciesEnum species);
u16 SpeciesToHoennPokedexNum(SpeciesEnum species);
u16 HoennToNationalOrder(u16 hoennNum);
void sub_806D544(SpeciesEnum species, u32 personality, u8* dest);
void DrawSpindaSpots(SpeciesEnum species, u32 personality, u8* dest, u8 a4);
void EvolutionRenameMon(struct Pokemon* mon, u16 oldSpecies, u16 newSpecies);
u8 GetPlayerFlankId(void);
u16 GetLinkTrainerFlankId(u8 id);
s32 GetBattlerMultiplayerId(u16 a1);
u8 GetTrainerEncounterMusicId(u16 trainerOpponentId);
u16 ModifyStatByNature(u8 nature, u16 n, u8 statIndex);
void AdjustFriendship(struct Pokemon* mon, u8 event);
void MonGainEVs(struct Pokemon* mon, u16 defeatedSpecies);
u16 GetMonEVCount(struct Pokemon* mon);
void RandomlyGivePartyPokerus(struct Pokemon* party);
u8 CheckPartyPokerus(struct Pokemon* party, u8 selection);
u8 CheckPartyHasHadPokerus(struct Pokemon* party, u8 selection);
void UpdatePartyPokerusTime(u16 days);
void PartySpreadPokerus(struct Pokemon* party);
bool8 TryIncrementMonLevel(struct Pokemon* mon);
u8 GetMoveRelearnerMoves(struct Pokemon* mon, u16* moves, bool8 disableLearned);
u8 GetLevelUpMovesBySpecies(SpeciesEnum species, u16* moves);
u8 GetNumberOfRelearnableMoves(struct Pokemon* mon);
// Egg Moves ---------------------------------------------------
u8 GetNumberOfEggMoves(struct Pokemon* mon);
u8 GetEggMoveTutorMoves(struct Pokemon* mon, u16* moves);
// TM Moves ---------------------------------------------------
u8 GetNumberOfTMMoves(struct Pokemon* mon);
u8 GetTMMoveTutorMoves(struct Pokemon* mon, u16* moves);
// TM Moves ---------------------------------------------------
u8 GetNumberOfTutorMoves(struct Pokemon* mon);
u8 GetMoveTutorMoves(struct Pokemon* mon, u16* moves);
//-------------------------------------------------------------
u16 SpeciesToPokedexNum(SpeciesEnum species);
bool32 IsSpeciesInHoennDex(SpeciesEnum species);
void ClearBattleMonForms(void);
u16 GetBattleBGM(void);
void PlayBattleBGM(void);
void PlayMapChosenOrBattleBGM(u16 songId);
void CreateTask_PlayMapChosenOrBattleBGM(u16 songId);
const u32* GetShinySpritePal(SpeciesEnum species, u32 isShiny);
const struct CompressedSpritePalette* GetShinySpritePalAddr(SpeciesEnum species, u32 isShiny);
const u32* GetMonFrontSpritePal(struct Pokemon* mon);
const u32* GetMonSpritePal(SpeciesEnum species, u32 personality, u8 isShiny);
const struct CompressedSpritePalette* GetMonSpritePalStruct(struct Pokemon* mon);
const struct CompressedSpritePalette* GetMonSpritePalStructFromOtIdPersonality(SpeciesEnum species, u32 personality, u8 isShiny);
bool32 IsHMMove2(u16 move);
bool8 IsMonSpriteNotFlipped(SpeciesEnum species);
s8 GetMonFlavorRelation(struct Pokemon* mon, u8 flavor);
s8 GetFlavorRelationByNature(u8 nature, u8 flavor);
bool8 IsTradedMon(struct Pokemon* mon);
bool8 IsOtherTrainer(u32 otId, u8* otName);
void MonRestorePP(struct Pokemon* mon);
void SetMonPreventsSwitchingString(void);
void SetWildMonHeldItem(void);
bool8 IsMonShiny(struct Pokemon* mon);
const u8* GetTrainerPartnerName(void);
void BattleAnimateFrontSprite(struct Sprite* sprite, SpeciesEnum species, bool8 noCry, u8 arg3);
void DoMonFrontSpriteAnimation(struct Sprite* sprite, SpeciesEnum species, bool8 noCry, u8 arg3);
void PokemonSummaryDoMonAnimation(struct Sprite* sprite, SpeciesEnum species, bool8 oneFrame);
void StopPokemonAnimationDelayTask(void);
void BattleAnimateBackSprite(struct Sprite* sprite, SpeciesEnum species);
u8 sub_806EF08(u8 arg0);
u8 sub_806EF84(u8 arg0, u8 arg1);
u16 sub_806EFF0(u16 arg0);
u16 FacilityClassToPicIndex(u16 facilityClass);
u16 PlayerGenderToFrontTrainerPicId(u8 playerGender);
void HandleSetPokedexFlag(u16 nationalNum, u8 caseId, u32 personality);
const u8* GetTrainerClassNameFromId(u16 trainerId);
const u8* GetTrainerNameFromId(u16 trainerId);
bool8 HasTwoFramesAnimation(SpeciesEnum species);
struct Unknown_806F160_Struct* sub_806F2AC(u8 id, u8 arg1);
void sub_806F47C(u8 id);
u8* sub_806F4F8(u8 id, u8 arg1);
SpeciesEnum GetFormSpeciesId(SpeciesEnum speciesId, u8 formId);
u8 GetFormIdFromFormSpeciesId(SpeciesEnum formSpeciesId);
SpeciesEnum GetFormChangeTargetSpecies(struct Pokemon* mon, u16 method, u32 arg);
u8 GetLevelCap(void);
u16 getShinyOdds(void);
u16 getRandomSpecies(void);
AbilityEnum GetMonInnate(struct Pokemon* mon, int slot, int disableRandomizer);
bool8 MonHasInnate(struct Pokemon* mon, AbilityEnum ability, bool8 disableRandomizer);
bool8 BoxMonHasInnate(struct BoxPokemon* boxmon, AbilityEnum ability, bool8 disableRandomizer);
bool8 SpeciesHasInnate(SpeciesEnum species, AbilityEnum ability, u8 level, u32 personality, bool8 disablerandomizer, bool8 isEnemyMon);
AbilityEnum RandomizeInnate(AbilityEnum innate, SpeciesEnum species, u32 personality);
AbilityEnum RandomizeAbility(AbilityEnum ability, SpeciesEnum species, u32 personality);
u8 RandomizeType(u8 type, SpeciesEnum species, u32 personality, bool8 isFirstType);
u8 GetSpeciesInnateNum(SpeciesEnum species, AbilityEnum ability, u8 level, u32 personality, bool8 disablerandomizer);
void CreateShinyMonWithNature(struct Pokemon* mon, SpeciesEnum species, u8 level, u8 nature);
u16 getNumberOfUniqueDefeatedTrainers(void);
bool8 enablePokemonChanges(void);
u16 GetRandomPokemonFromTag(u16 rndseed, s8 loc, s8 locG);
u16 GetRandomPokemonFromDiffTag(u16 rndseed, u32 tags, u8 total, u8 tier);
u16 tagSwitch(u8 tag, u16 rndseed);
u16 GetRandomStarter(u8 gen, bool8 enc, bool8 leg, u8 starterID);
void getGenRange(u8 gen, u16* min, u16* max);
u16 GetRandomPokemonFromSpecies(u16 basespecies);
u8 getTier(s8 loc, s8 locG);
u32 getMask(s8 loc, s8 locG);
bool8 isMonNicknamed(struct Pokemon* mon);
bool8 isBoxMonNicknamed(struct BoxPokemon* boxMon);
bool8 CheckBoxMonForBadChecksum(u8 box, u8 slot);
bool8 isSpeciesPlaceholderMon(SpeciesEnum species);
bool8 IsEeveelution(SpeciesEnum species);
SpeciesEnum GetBaseSpeciesFromMega(SpeciesEnum species);
const u8* GetSpeciesLongName(SpeciesEnum species);
const u8* SaveSpeciesWithSurname(SpeciesEnum species);
SpeciesEnum GetRandomSpeciesFromPool(u8 id);
const SpeciesEnum* GetFormSpeciesTable(SpeciesEnum speciesId);
bool8 SpeciesHasDifferentForms(SpeciesEnum speciesId);

SpeciesEnum GetEvolutionForMon(struct Pokemon* mon, u8 num);
SpeciesEnum GetFormChangeForMon(struct Pokemon* mon, u8 num);
u8 getNumofAvailableEvos(struct Pokemon* mon);

void CalculateMonStatsWithoutRestoringPP(struct Pokemon* mon);
void CalculateEnemyTrainerMonStats(struct Pokemon* mon);
void CalculateMonStatsMaster(struct Pokemon* mon, bool8 shouldRestorePP, bool8 isEnemyMon);

bool8 CanDisableInnates(void);
bool8 getInnateDisableLevel(u8 innateNum);
bool8 isMoveDisabled(u8 battler, u16 move);
u16 GetHeldItemIfNotDuplicate(u8 partyId);

void SetNuzlockeCaughtFlag(u8 locationIndex);
void ClearNuzlockeCaughtFlag(u8 locationIndex);
bool8 GetNuzlockeCaughtFlag(u8 locationIndex);
bool8 AreNuzlockeRulesEnabled(void);
void ClearNuzlockeDisableFlags(void);
void ClearAllNuzlockeFlags(void);

AbilityEnum GetExtraAbilityToSetToBattler(u8 abilityNumber, bool8 isEnemy);
AbilityEnum GetExtraAbilityForBattler(u8 battler, u8 abilityNumber);
AbilityEnum GetBattlerAbilityInSlot(u8 battler, u8 abilityNumber);
u8 GetNumPossibleAbilitiesForBattler(void);
bool8 isHellMode(void);
u16 GetSpeciesCry(SpeciesEnum species, bool32 v0);

#endif  // GUARD_POKEMON_H
