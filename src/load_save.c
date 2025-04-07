#include "global.h"
#include "malloc.h"
#include "berry_powder.h"
#include "item.h"
#include "load_save.h"
#include "main.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "save_location.h"
#include "trainer_hill.h"
#include "gba/flash_internal.h"
#include "decoration_inventory.h"
#include "agb_flash.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"

static void ApplyNewEncryptionKeyToAllEncryptedData(u32 encryptionKey);

#define SAVEBLOCK_MOVE_RANGE    128

struct LoadedSaveData
{
    struct ItemSlot items[BAG_ITEMS_COUNT];
    struct ItemSlot keyItems[BAG_KEYITEMS_COUNT];
    struct ItemSlot pokeBalls[BAG_POKEBALLS_COUNT];
    struct ItemSlot TMsHMs[BAG_TMHM_COUNT];
    struct ItemSlot berries[BAG_BERRIES_COUNT];
    struct ItemSlot medicine[BAG_MEDICINE_COUNT];
    struct ItemSlot battle[BAG_BATTLE_COUNT];
    struct ItemSlot megaStones[BAG_MEGASTONES_COUNT];
    struct MailStruct mail[MAIL_COUNT];
};

// EWRAM DATA
EWRAM_DATA struct SaveBlock2 gSaveblock2 = {0};
EWRAM_DATA u8 gSaveblock2_DMA[SAVEBLOCK_MOVE_RANGE] = {0};

EWRAM_DATA struct SaveBlock1 gSaveblock1 = {0};
EWRAM_DATA u8 gSaveblock1_DMA[SAVEBLOCK_MOVE_RANGE] = {0};

EWRAM_DATA struct PokemonStorage gPokemonStorage = {0};
EWRAM_DATA u8 gSaveblock3_DMA[SAVEBLOCK_MOVE_RANGE] = {0};

EWRAM_DATA struct LoadedSaveData gLoadedSaveData = {0};
EWRAM_DATA u32 gLastEncryptionKey = 0;

// IWRAM common
bool32 gFlashMemoryPresent;
struct SaveBlock1 *gSaveBlock1Ptr;
struct SaveBlock2 *gSaveBlock2Ptr;
struct PokemonStorage *gPokemonStoragePtr;

// code
void CheckForFlashMemory(void)
{
    if (!IdentifyFlash())
    {
        gFlashMemoryPresent = TRUE;
        InitFlashTimer();
    }
    else
    {
        gFlashMemoryPresent = FALSE;
    }
}

void ClearSav2(void)
{
    CpuFill16(0, &gSaveblock2, sizeof(struct SaveBlock2) + sizeof(gSaveblock2_DMA));
}

void ClearSav1(void)
{
    CpuFill16(0, &gSaveblock1, sizeof(struct SaveBlock1) + sizeof(gSaveblock1_DMA));
}

void SetSaveBlocksPointers(u16 offset)
{
    struct SaveBlock1** sav1_LocalVar = &gSaveBlock1Ptr;

    offset = 0;

    gSaveBlock2Ptr = (void*)(&gSaveblock2) + offset;
    *sav1_LocalVar = (void*)(&gSaveblock1) + offset;
    gPokemonStoragePtr = (void*)(&gPokemonStorage) + offset;

    SetDecorationInventoriesPointers();
}

void MoveSaveBlocks_ResetHeap(void)
{
    void *vblankCB, *hblankCB;
    u32 encryptionKey;
    struct SaveBlock2 *saveBlock2Copy;

    // save interrupt functions and turn them off
    vblankCB = gMain.vblankCallback;
    hblankCB = gMain.hblankCallback;
    gMain.vblankCallback = NULL;
    gMain.hblankCallback = NULL;
    gTrainerHillVBlankCounter = NULL;

    saveBlock2Copy = (struct SaveBlock2 *)(gHeap);

    { STATIC_ASSERT(HEAP_SIZE >= sizeof(struct SaveBlock1) + sizeof(struct SaveBlock2) + sizeof(struct PokemonStorage), heapCannotStoreSaveBlock) }

    // change saveblocks' pointers
    // argument is a sum of the individual trainerId bytes
    SetSaveBlocksPointers(
      saveBlock2Copy->playerTrainerId[0] +
      saveBlock2Copy->playerTrainerId[1] +
      saveBlock2Copy->playerTrainerId[2] +
      saveBlock2Copy->playerTrainerId[3]);

    // heap was destroyed in the copying process, so reset it
    InitHeap(gHeap, HEAP_SIZE);

    // restore interrupt functions
    gMain.hblankCallback = hblankCB;
    gMain.vblankCallback = vblankCB;

    // create a new encryption key
    encryptionKey = 0;
    ApplyNewEncryptionKeyToAllEncryptedData(encryptionKey);
    gSaveBlock2Ptr->encryptionKey = encryptionKey;
}

u32 UseContinueGameWarp(void)
{
    return gSaveBlock2Ptr->specialSaveWarpFlags & CONTINUE_GAME_WARP;
}

void ClearContinueGameWarpStatus(void)
{
    gSaveBlock2Ptr->specialSaveWarpFlags &= ~CONTINUE_GAME_WARP;
}

void SetContinueGameWarpStatus(void)
{
    gSaveBlock2Ptr->specialSaveWarpFlags |= CONTINUE_GAME_WARP;
}

void SetContinueGameWarpStatusToDynamicWarp(void)
{
    SetContinueGameWarpToDynamicWarp(0);
    gSaveBlock2Ptr->specialSaveWarpFlags |= CONTINUE_GAME_WARP;
}

void ClearContinueGameWarpStatus2(void)
{
    gSaveBlock2Ptr->specialSaveWarpFlags &= ~CONTINUE_GAME_WARP;
}

void SavePlayerParty(void)
{
    int i;

    gSaveBlock1Ptr->playerPartyCount = gPlayerPartyCount;

    for (i = 0; i < PARTY_SIZE; i++)
        gSaveBlock1Ptr->playerParty[i] = gPlayerParty[i];
}

void LoadPlayerParty(void)
{
    int i;

    gPlayerPartyCount = gSaveBlock1Ptr->playerPartyCount;

    for (i = 0; i < PARTY_SIZE; i++)
        gPlayerParty[i] = gSaveBlock1Ptr->playerParty[i];
}

void SaveObjectEvents(void)
{
    int i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        gSaveBlock1Ptr->objectEvents[i] = gObjectEvents[i];
}

void LoadObjectEvents(void)
{
    int i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        gObjectEvents[i] = gSaveBlock1Ptr->objectEvents[i];
}

void SaveSerializedGame(void)
{
    SavePlayerParty();
    SaveObjectEvents();
}

void LoadSerializedGame(void)
{
    LoadPlayerParty();
    LoadObjectEvents();
}

void ApplyNewEncryptionKeyToHword(u16 *hWord, u32 newKey)
{
    *hWord ^= gSaveBlock2Ptr->encryptionKey;
    *hWord ^= newKey;
}

void ApplyNewEncryptionKeyToWord(u32 *word, u32 newKey)
{
    *word ^= gSaveBlock2Ptr->encryptionKey;
    *word ^= newKey;
}

static void ApplyNewEncryptionKeyToAllEncryptedData(u32 encryptionKey)
{
    ApplyNewEncryptionKeyToGameStats(encryptionKey);
    ApplyNewEncryptionKeyToBerryPowder(encryptionKey);
    ApplyNewEncryptionKeyToWord(&gSaveBlock1Ptr->money, encryptionKey);
    ApplyNewEncryptionKeyToHword(&gSaveBlock1Ptr->coins, encryptionKey);
}

//Functions to load/save encountered routes for nuzlocke capture mode

u8 GetRouteIndex(s8 loc, s8 locG) {
    if (MAP_NUM(METEOR_FALLS_1F_2R) == loc && MAP_GROUP(METEOR_FALLS_1F_2R) == locG) {
	    return 0;
    }
    if (MAP_NUM(ROUTE128) == loc && MAP_GROUP(ROUTE128) == locG) {
        return 1;
    }
    if (MAP_NUM(ABANDONED_SHIP_ROOMS_B1F) == loc && MAP_GROUP(ABANDONED_SHIP_ROOMS_B1F) == locG) {
        return 2;
    }
    if (MAP_NUM(JAGGED_PASS) == loc && MAP_GROUP(JAGGED_PASS) == locG) {
        return 3;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_DIANCIES_ROOM) == loc && MAP_GROUP(CAVE_OF_ORIGIN_DIANCIES_ROOM) == locG) {
        return 4;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP3) == loc && MAP_GROUP(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP3) == locG) {
        return 5;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) == locG) {
        return 6;
    }
    if (MAP_NUM(DEWFORD_MEADOW) == loc && MAP_GROUP(DEWFORD_MEADOW) == locG) {
        return 7;
    }
    if (MAP_NUM(ROUTE126) == loc && MAP_GROUP(ROUTE126) == locG) {
        return 8;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM3) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM3) == locG) {
        return 9;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM2) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM2) == locG) {
        return 10;
    }
    if (MAP_NUM(NEW_MAUVILLE_INSIDE) == loc && MAP_GROUP(NEW_MAUVILLE_INSIDE) == locG) {
        return 11;
    }
    if (MAP_NUM(SAFARI_ZONE_SOUTH) == loc && MAP_GROUP(SAFARI_ZONE_SOUTH) == locG) {
        return 12;
    }
    if (MAP_NUM(MT_PYRE_EXTERIOR) == loc && MAP_GROUP(MT_PYRE_EXTERIOR) == locG) {
        return 13;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_2F_3R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_3R) == locG) {
        return 14;
    }
    if (MAP_NUM(ROUTE132) == loc && MAP_GROUP(ROUTE132) == locG) {
        return 15;
    }
    if (MAP_NUM(ROUTE105) == loc && MAP_GROUP(ROUTE105) == locG) {
        return 16;
    }
    if (MAP_NUM(ROUTE115) == loc && MAP_GROUP(ROUTE115) == locG) {
        return 17;
    }
    if (MAP_NUM(METEOR_FALLS_B1F_2R) == loc && MAP_GROUP(METEOR_FALLS_B1F_2R) == locG) {
        return 18;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_2F_1R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_1R) == locG) {
        return 19;
    }
    if (MAP_NUM(MT_PYRE_1F) == loc && MAP_GROUP(MT_PYRE_1F) == locG) {
        return 20;
    }
    if (MAP_NUM(RUSTURF_TUNNEL_2F) == loc && MAP_GROUP(RUSTURF_TUNNEL_2F) == locG) {
        return 21;
    }
    if (MAP_NUM(ROUTE108) == loc && MAP_GROUP(ROUTE108) == locG) {
        return 22;
    }
    if (MAP_NUM(ARTISAN_CAVE_B1F) == loc && MAP_GROUP(ARTISAN_CAVE_B1F) == locG) {
        return 23;
    }
    if (MAP_NUM(MOSSDEEP_CITY) == loc && MAP_GROUP(MOSSDEEP_CITY) == locG) {
        return 24;
    }
    if (MAP_NUM(METEOR_FALLS_1F_1R) == loc && MAP_GROUP(METEOR_FALLS_1F_1R) == locG) {
        return 25;
    }
    if (MAP_NUM(ROUTE109) == loc && MAP_GROUP(ROUTE109) == locG) {
        return 26;
    }
    if (MAP_NUM(VERDANTURF_MEADOW) == loc && MAP_GROUP(VERDANTURF_MEADOW) == locG) {
        return 27;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM1) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM1) == locG) {
        return 28;
    }
    if (MAP_NUM(PETALBURG_WOODS_3) == loc && MAP_GROUP(PETALBURG_WOODS_3) == locG) {
        return 29;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM8) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM8) == locG) {
        return 30;
    }
    if (MAP_NUM(METEOR_FALLS_STEVENS_CAVE) == loc && MAP_GROUP(METEOR_FALLS_STEVENS_CAVE) == locG) {
        return 31;
    }
    if (MAP_NUM(ROUTE120) == loc && MAP_GROUP(ROUTE120) == locG) {
        return 32;
    }
    if (MAP_NUM(SANDSTREWN_RUINS) == loc && MAP_GROUP(SANDSTREWN_RUINS) == locG) {
        return 33;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) == locG) {
        return 34;
    }
    if (MAP_NUM(ROUTE113) == loc && MAP_GROUP(ROUTE113) == locG) {
        return 35;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM7) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM7) == locG) {
        return 36;
    }
    if (MAP_NUM(ROUTE119) == loc && MAP_GROUP(ROUTE119) == locG) {
        return 37;
    }
    if (MAP_NUM(RUSTBORO_CITY) == loc && MAP_GROUP(RUSTBORO_CITY) == locG) {
        return 38;
    }
    if (MAP_NUM(ARTISAN_CAVE_1F) == loc && MAP_GROUP(ARTISAN_CAVE_1F) == locG) {
        return 39;
    }
    if (MAP_NUM(ROUTE118) == loc && MAP_GROUP(ROUTE118) == locG) {
        return 40;
    }
    if (MAP_NUM(ROUTE133) == loc && MAP_GROUP(ROUTE133) == locG) {
        return 41;
    }
    if (MAP_NUM(ROUTE131) == loc && MAP_GROUP(ROUTE131) == locG) {
        return 42;
    }
    if (MAP_NUM(ALTERING_CAVE_1F) == loc && MAP_GROUP(ALTERING_CAVE_1F) == locG) {
        return 43;
    }
    if (MAP_NUM(ALTERING_CAVE) == loc && MAP_GROUP(ALTERING_CAVE) == locG) {
        return 44;
    }
    if (MAP_NUM(FIERY_PATH) == loc && MAP_GROUP(FIERY_PATH) == locG) {
        return 45;
    }
    if (MAP_NUM(MIRAGE_TOWER_B1F) == loc && MAP_GROUP(MIRAGE_TOWER_B1F) == locG) {
        return 46;
    }
    if (MAP_NUM(VICTORY_ROAD_B1F) == loc && MAP_GROUP(VICTORY_ROAD_B1F) == locG) {
        return 47;
    }
    if (MAP_NUM(ROUTE130) == loc && MAP_GROUP(ROUTE130) == locG) {
        return 48;
    }
    if (MAP_NUM(ROUTE110) == loc && MAP_GROUP(ROUTE110) == locG) {
        return 49;
    }
    if (MAP_NUM(PETALBURG_WOODS_2) == loc && MAP_GROUP(PETALBURG_WOODS_2) == locG) {
        return 50;
    }
    if (MAP_NUM(ROUTE124) == loc && MAP_GROUP(ROUTE124) == locG) {
        return 51;
    }
    if (MAP_NUM(SLATEPORT_CITY) == loc && MAP_GROUP(SLATEPORT_CITY) == locG) {
        return 52;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM4) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM4) == locG) {
        return 53;
    }
    if (MAP_NUM(SCORCHED_SLAB_B1F) == loc && MAP_GROUP(SCORCHED_SLAB_B1F) == locG) {
        return 54;
    }
    if (MAP_NUM(SAFARI_ZONE_SOUTHEAST) == loc && MAP_GROUP(SAFARI_ZONE_SOUTHEAST) == locG) {
        return 55;
    }
    if (MAP_NUM(ROUTE123) == loc && MAP_GROUP(ROUTE123) == locG) {
        return 56;
    }
    if (MAP_NUM(PETALBURG_CITY) == loc && MAP_GROUP(PETALBURG_CITY) == locG) {
        return 57;
    }
    if (MAP_NUM(SEASPRAY_CAVE_B1F) == loc && MAP_GROUP(SEASPRAY_CAVE_B1F) == locG) {
        return 58;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) == locG) {
        return 59;
    }
    if (MAP_NUM(NEW_MAUVILLE_ENTRANCE) == loc && MAP_GROUP(NEW_MAUVILLE_ENTRANCE) == locG) {
        return 60;
    }
    if (MAP_NUM(ROUTE104) == loc && MAP_GROUP(ROUTE104) == locG) {
        return 61;
    }
    if (MAP_NUM(SANDSTREWN_RUINS_2F) == loc && MAP_GROUP(SANDSTREWN_RUINS_2F) == locG) {
        return 62;
    }
    if (MAP_NUM(ROUTE107) == loc && MAP_GROUP(ROUTE107) == locG) {
        return 63;
    }
    if (MAP_NUM(UNDERWATER_ROUTE126) == loc && MAP_GROUP(UNDERWATER_ROUTE126) == locG) {
        return 64;
    }
    if (MAP_NUM(MT_PYRE_SUMMIT) == loc && MAP_GROUP(MT_PYRE_SUMMIT) == locG) {
        return 65;
    }
    if (MAP_NUM(GRANITE_CAVE_STEVENS_ROOM) == loc && MAP_GROUP(GRANITE_CAVE_STEVENS_ROOM) == locG) {
        return 66;
    }
    if (MAP_NUM(MIRAGE_TOWER_2F) == loc && MAP_GROUP(MIRAGE_TOWER_2F) == locG) {
        return 67;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_1F) == loc && MAP_GROUP(MAGMA_HIDEOUT_1F) == locG) {
        return 68;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM6) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM6) == locG) {
        return 69;
    }
    if (MAP_NUM(SAFARI_ZONE_NORTHEAST) == loc && MAP_GROUP(SAFARI_ZONE_NORTHEAST) == locG) {
        return 70;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_1F) == loc && MAP_GROUP(CAVE_OF_ORIGIN_1F) == locG) {
        return 71;
    }
    if (MAP_NUM(GRANITE_CAVE_B1F) == loc && MAP_GROUP(GRANITE_CAVE_B1F) == locG) {
        return 72;
    }
    if (MAP_NUM(ROUTE134) == loc && MAP_GROUP(ROUTE134) == locG) {
        return 73;
    }
    if (MAP_NUM(ASHEN_WOODS) == loc && MAP_GROUP(ASHEN_WOODS) == locG) {
        return 74;
    }
    if (MAP_NUM(ALTERING_CAVE_B1F) == loc && MAP_GROUP(ALTERING_CAVE_B1F) == locG) {
        return 75;
    }
    if (MAP_NUM(PACIFIDLOG_TOWN) == loc && MAP_GROUP(PACIFIDLOG_TOWN) == locG) {
        return 76;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_3F_3R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_3R) == locG) {
        return 77;
    }
    if (MAP_NUM(SKY_PILLAR_1F) == loc && MAP_GROUP(SKY_PILLAR_1F) == locG) {
        return 78;
    }
    if (MAP_NUM(SANDSTREWN_RUINS_3F) == loc && MAP_GROUP(SANDSTREWN_RUINS_3F) == locG) {
        return 79;
    }
    if (MAP_NUM(UNDERWATER_ROUTE124) == loc && MAP_GROUP(UNDERWATER_ROUTE124) == locG) {
        return 80;
    }
    if (MAP_NUM(SCORCHED_SLAB) == loc && MAP_GROUP(SCORCHED_SLAB) == locG) {
        return 81;
    }
    if (MAP_NUM(SCORCHED_SLAB_B2F) == loc && MAP_GROUP(SCORCHED_SLAB_B2F) == locG) {
        return 82;
    }
    if (MAP_NUM(EMBER_PATH) == loc && MAP_GROUP(EMBER_PATH) == locG) {
        return 83;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_4F) == loc && MAP_GROUP(MAGMA_HIDEOUT_4F) == locG) {
        return 84;
    }
    if (MAP_NUM(MT_PYRE_2F) == loc && MAP_GROUP(MT_PYRE_2F) == locG) {
        return 85;
    }
    if (MAP_NUM(ROUTE121) == loc && MAP_GROUP(ROUTE121) == locG) {
        return 86;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP1) == loc && MAP_GROUP(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP1) == locG) {
        return 87;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_3F_1R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_1R) == locG) {
        return 88;
    }
    if (MAP_NUM(VICTORY_ROAD_B2F) == loc && MAP_GROUP(VICTORY_ROAD_B2F) == locG) {
        return 89;
    }
    if (MAP_NUM(SAFARI_ZONE_NORTHWEST) == loc && MAP_GROUP(SAFARI_ZONE_NORTHWEST) == locG) {
        return 90;
    }
    if (MAP_NUM(ROUTE129) == loc && MAP_GROUP(ROUTE129) == locG) {
        return 91;
    }
    if (MAP_NUM(VICTORY_ROAD_1F) == loc && MAP_GROUP(VICTORY_ROAD_1F) == locG) {
        return 92;
    }
    if (MAP_NUM(DEWFORD_MANOR_1F) == loc && MAP_GROUP(DEWFORD_MANOR_1F) == locG) {
        return 93;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_ENTRANCE) == loc && MAP_GROUP(CAVE_OF_ORIGIN_ENTRANCE) == locG) {
        return 94;
    }
    if (MAP_NUM(OLDALE_TOWN) == loc && MAP_GROUP(OLDALE_TOWN) == locG) {
        return 95;
    }
    if (MAP_NUM(ROUTE111) == loc && MAP_GROUP(ROUTE111) == locG) {
        return 96;
    }
    if (MAP_NUM(SKY_PILLAR_3F) == loc && MAP_GROUP(SKY_PILLAR_3F) == locG) {
        return 97;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) == locG) {
        return 98;
    }
    if (MAP_NUM(ROUTE112) == loc && MAP_GROUP(ROUTE112) == locG) {
        return 99;
    }
    if (MAP_NUM(ROUTE102) == loc && MAP_GROUP(ROUTE102) == locG) {
        return 100;
    }
    if (MAP_NUM(SOOTOPOLIS_CITY) == loc && MAP_GROUP(SOOTOPOLIS_CITY) == locG) {
        return 101;
    }
    if (MAP_NUM(ROUTE122) == loc && MAP_GROUP(ROUTE122) == locG) {
        return 102;
    }
    if (MAP_NUM(LILYCOVE_CITY) == loc && MAP_GROUP(LILYCOVE_CITY) == locG) {
        return 103;
    }
    if (MAP_NUM(SEASPRAY_CAVE) == loc && MAP_GROUP(SEASPRAY_CAVE) == locG) {
        return 104;
    }
    if (MAP_NUM(DESERT_UNDERPASS) == loc && MAP_GROUP(DESERT_UNDERPASS) == locG) {
        return 105;
    }
    if (MAP_NUM(ROUTE111_RUINS_EXTERIOR) == loc && MAP_GROUP(ROUTE111_RUINS_EXTERIOR) == locG) {
        return 106;
    }
    if (MAP_NUM(ABANDONED_SHIP_HIDDEN_FLOOR_CORRIDORS) == loc && MAP_GROUP(ABANDONED_SHIP_HIDDEN_FLOOR_CORRIDORS) == locG) {
        return 107;
    }
    if (MAP_NUM(ROUTE125) == loc && MAP_GROUP(ROUTE125) == locG) {
        return 108;
    }
    if (MAP_NUM(SCORCHED_SLAB_HEATRANS_ROOM) == loc && MAP_GROUP(SCORCHED_SLAB_HEATRANS_ROOM) == locG) {
        return 109;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP2) == loc && MAP_GROUP(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP2) == locG) {
        return 110;
    }
    if (MAP_NUM(SECRET_DUNGEON) == loc && MAP_GROUP(SECRET_DUNGEON) == locG) {
        return 111;
    }
    if (MAP_NUM(ROUTE101) == loc && MAP_GROUP(ROUTE101) == locG) {
        return 112;
    }
    if (MAP_NUM(MT_PYRE_3F) == loc && MAP_GROUP(MT_PYRE_3F) == locG) {
        return 113;
    }
    if (MAP_NUM(MT_PYRE_6F) == loc && MAP_GROUP(MT_PYRE_6F) == locG) {
        return 114;
    }
    if (MAP_NUM(ROUTE103) == loc && MAP_GROUP(ROUTE103) == locG) {
        return 115;
    }
    if (MAP_NUM(GRANITE_CAVE_B2F) == loc && MAP_GROUP(GRANITE_CAVE_B2F) == locG) {
        return 116;
    }
    if (MAP_NUM(METEOR_FALLS_B1F_1R) == loc && MAP_GROUP(METEOR_FALLS_B1F_1R) == locG) {
        return 117;
    }
    if (MAP_NUM(SKY_PILLAR_5F) == loc && MAP_GROUP(SKY_PILLAR_5F) == locG) {
        return 118;
    }
    if (MAP_NUM(MIRAGE_TOWER_1F) == loc && MAP_GROUP(MIRAGE_TOWER_1F) == locG) {
        return 119;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM5) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM5) == locG) {
        return 120;
    }
    if (MAP_NUM(EVER_GRANDE_CITY) == loc && MAP_GROUP(EVER_GRANDE_CITY) == locG) {
        return 121;
    }
    if (MAP_NUM(SANDSTREWN_RUINS_B1F) == loc && MAP_GROUP(SANDSTREWN_RUINS_B1F) == locG) {
        return 122;
    }
    if (MAP_NUM(MIRAGE_TOWER_3F) == loc && MAP_GROUP(MIRAGE_TOWER_3F) == locG) {
        return 123;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_2F_2R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_2R) == locG) {
        return 124;
    }
    if (MAP_NUM(SAFARI_ZONE_NORTH) == loc && MAP_GROUP(SAFARI_ZONE_NORTH) == locG) {
        return 125;
    }
    if (MAP_NUM(GRANITE_CAVE_1F) == loc && MAP_GROUP(GRANITE_CAVE_1F) == locG) {
        return 126;
    }
    if (MAP_NUM(ROUTE127) == loc && MAP_GROUP(ROUTE127) == locG) {
        return 127;
    }
    if (MAP_NUM(SAFARI_ZONE_SOUTHWEST) == loc && MAP_GROUP(SAFARI_ZONE_SOUTHWEST) == locG) {
        return 128;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) == locG) {
        return 129;
    }
    if (MAP_NUM(MT_PYRE_4F) == loc && MAP_GROUP(MT_PYRE_4F) == locG) {
        return 130;
    }
    if (MAP_NUM(ROUTE106) == loc && MAP_GROUP(ROUTE106) == locG) {
        return 131;
    }
    if (MAP_NUM(RUSTURF_TUNNEL) == loc && MAP_GROUP(RUSTURF_TUNNEL) == locG) {
        return 132;
    }
    if (MAP_NUM(ROUTE116) == loc && MAP_GROUP(ROUTE116) == locG) {
        return 133;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_3F_2R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_2R) == locG) {
        return 134;
    }
    if (MAP_NUM(PETALBURG_WOODS) == loc && MAP_GROUP(PETALBURG_WOODS) == locG) {
        return 135;
    }
    if (MAP_NUM(DEWFORD_TOWN) == loc && MAP_GROUP(DEWFORD_TOWN) == locG) {
        return 136;
    }
    if (MAP_NUM(ROUTE114) == loc && MAP_GROUP(ROUTE114) == locG) {
        return 137;
    }
    if (MAP_NUM(ROUTE117) == loc && MAP_GROUP(ROUTE117) == locG) {
        return 138;
    }
    if (MAP_NUM(MT_PYRE_5F) == loc && MAP_GROUP(MT_PYRE_5F) == locG) {
        return 139;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ENTRANCE) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ENTRANCE) == locG) {
        return 140;
    }
    if (MAP_NUM(MIRAGE_TOWER_4F) == loc && MAP_GROUP(MIRAGE_TOWER_4F) == locG) {
        return 141;
    }
    return 142;
}

bool8 IsRouteEncountered(s8 loc, s8 locG) {
    u8 index = GetRouteIndex(loc, locG);
    u8 maskIndex = index / 32;
    u8 bitIndex = index % 32;
    u32 mask;
    switch (maskIndex) {
        case 0:
            mask = gSaveBlock2Ptr->encounteredroutes1;
            break;
        case 1:
            mask = gSaveBlock2Ptr->encounteredroutes2;
            break;
        case 2:
            mask = gSaveBlock2Ptr->encounteredroutes3;
            break;
        case 3:
            mask = gSaveBlock2Ptr->encounteredroutes4;
            break;
        case 4:
            mask = gSaveBlock2Ptr->encounteredroutes5;
            break;
        // This shouldn't happen
        default:
            return TRUE;
    }
    return (mask >> bitIndex) & 1;
}

void MarkRouteAsEncountered(s8 loc, s8 locG) {
    u8 index = GetRouteIndex(loc, locG);
    u8 maskIndex = index / 32;
    u8 bitIndex = index % 32;
    switch (maskIndex) {
        case 0:
            gSaveBlock2Ptr->encounteredroutes1 |= (1 << bitIndex);
            break;
        case 1:
            gSaveBlock2Ptr->encounteredroutes2 |= (1 << bitIndex);
            break;
        case 2:
            gSaveBlock2Ptr->encounteredroutes3 |= (1 << bitIndex);
            break;
        case 3:
            gSaveBlock2Ptr->encounteredroutes4 |= (1 << bitIndex);
            break;
        case 4:
            gSaveBlock2Ptr->encounteredroutes5 |= (1 << bitIndex);
            break;
    }
    
}



