#include "global.h"
#include "load_save.h"
#include "nuzlocke.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"

u8 GetRouteIndex(s8 loc, s8 locG) {
    if (MAP_NUM(METEOR_FALLS_1F_2R) == loc && MAP_GROUP(METEOR_FALLS_1F_2R) == locG) {
	    return INDEX_METEOR_FALLS_1F_2R;
    }
    if (MAP_NUM(ROUTE128) == loc && MAP_GROUP(ROUTE128) == locG) {
        return INDEX_ROUTE128;
    }
    if (MAP_NUM(ABANDONED_SHIP_ROOMS_B1F) == loc && MAP_GROUP(ABANDONED_SHIP_ROOMS_B1F) == locG) {
        return INDEX_ABANDONED_SHIP_ROOMS_B1F;
    }
    if (MAP_NUM(JAGGED_PASS) == loc && MAP_GROUP(JAGGED_PASS) == locG) {
        return INDEX_JAGGED_PASS;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_DIANCIES_ROOM) == loc && MAP_GROUP(CAVE_OF_ORIGIN_DIANCIES_ROOM) == locG) {
        return INDEX_CAVE_OF_ORIGIN_DIANCIES_ROOM;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP3) == loc && MAP_GROUP(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP3) == locG) {
        return INDEX_CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP3;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) == locG) {
        return INDEX_SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM;
    }
    if (MAP_NUM(DEWFORD_MEADOW) == loc && MAP_GROUP(DEWFORD_MEADOW) == locG) {
        return INDEX_DEWFORD_MEADOW;
    }
    if (MAP_NUM(ROUTE126) == loc && MAP_GROUP(ROUTE126) == locG) {
        return INDEX_ROUTE126;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM3) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM3) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM3;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM2) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM2) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM2;
    }
    if (MAP_NUM(NEW_MAUVILLE_INSIDE) == loc && MAP_GROUP(NEW_MAUVILLE_INSIDE) == locG) {
        return INDEX_NEW_MAUVILLE_INSIDE;
    }
    if (MAP_NUM(SAFARI_ZONE_SOUTH) == loc && MAP_GROUP(SAFARI_ZONE_SOUTH) == locG) {
        return INDEX_SAFARI_ZONE_SOUTH;
    }
    if (MAP_NUM(MT_PYRE_EXTERIOR) == loc && MAP_GROUP(MT_PYRE_EXTERIOR) == locG) {
        return INDEX_MT_PYRE_EXTERIOR;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_2F_3R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_3R) == locG) {
        return INDEX_MAGMA_HIDEOUT_2F_3R;
    }
    if (MAP_NUM(ROUTE132) == loc && MAP_GROUP(ROUTE132) == locG) {
        return INDEX_ROUTE132;
    }
    if (MAP_NUM(ROUTE105) == loc && MAP_GROUP(ROUTE105) == locG) {
        return INDEX_ROUTE105;
    }
    if (MAP_NUM(ROUTE115) == loc && MAP_GROUP(ROUTE115) == locG) {
        return INDEX_ROUTE115;
    }
    if (MAP_NUM(METEOR_FALLS_B1F_2R) == loc && MAP_GROUP(METEOR_FALLS_B1F_2R) == locG) {
        return INDEX_METEOR_FALLS_B1F_2R;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_2F_1R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_1R) == locG) {
        return INDEX_MAGMA_HIDEOUT_2F_1R;
    }
    if (MAP_NUM(MT_PYRE_1F) == loc && MAP_GROUP(MT_PYRE_1F) == locG) {
        return INDEX_MT_PYRE_1F;
    }
    if (MAP_NUM(RUSTURF_TUNNEL_2F) == loc && MAP_GROUP(RUSTURF_TUNNEL_2F) == locG) {
        return INDEX_RUSTURF_TUNNEL_2F;
    }
    if (MAP_NUM(ROUTE108) == loc && MAP_GROUP(ROUTE108) == locG) {
        return INDEX_ROUTE108;
    }
    if (MAP_NUM(ARTISAN_CAVE_B1F) == loc && MAP_GROUP(ARTISAN_CAVE_B1F) == locG) {
        return INDEX_ARTISAN_CAVE_B1F;
    }
    if (MAP_NUM(MOSSDEEP_CITY) == loc && MAP_GROUP(MOSSDEEP_CITY) == locG) {
        return INDEX_MOSSDEEP_CITY;
    }
    if (MAP_NUM(METEOR_FALLS_1F_1R) == loc && MAP_GROUP(METEOR_FALLS_1F_1R) == locG) {
        return INDEX_METEOR_FALLS_1F_1R;
    }
    if (MAP_NUM(ROUTE109) == loc && MAP_GROUP(ROUTE109) == locG) {
        return INDEX_ROUTE109;
    }
    if (MAP_NUM(VERDANTURF_MEADOW) == loc && MAP_GROUP(VERDANTURF_MEADOW) == locG) {
        return INDEX_VERDANTURF_MEADOW;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM1) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM1) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM1;
    }
    if (MAP_NUM(PETALBURG_WOODS_3) == loc && MAP_GROUP(PETALBURG_WOODS_3) == locG) {
        return INDEX_PETALBURG_WOODS_3;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM8) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM8) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM8;
    }
    if (MAP_NUM(METEOR_FALLS_STEVENS_CAVE) == loc && MAP_GROUP(METEOR_FALLS_STEVENS_CAVE) == locG) {
        return INDEX_METEOR_FALLS_STEVENS_CAVE;
    }
    if (MAP_NUM(ROUTE120) == loc && MAP_GROUP(ROUTE120) == locG) {
        return INDEX_ROUTE120;
    }
    if (MAP_NUM(SANDSTREWN_RUINS) == loc && MAP_GROUP(SANDSTREWN_RUINS) == locG) {
        return INDEX_SANDSTREWN_RUINS;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) == locG) {
        return INDEX_SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM;
    }
    if (MAP_NUM(ROUTE113) == loc && MAP_GROUP(ROUTE113) == locG) {
        return INDEX_ROUTE113;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM7) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM7) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM7;
    }
    if (MAP_NUM(ROUTE119) == loc && MAP_GROUP(ROUTE119) == locG) {
        return INDEX_ROUTE119;
    }
    if (MAP_NUM(RUSTBORO_CITY) == loc && MAP_GROUP(RUSTBORO_CITY) == locG) {
        return INDEX_RUSTBORO_CITY;
    }
    if (MAP_NUM(ARTISAN_CAVE_1F) == loc && MAP_GROUP(ARTISAN_CAVE_1F) == locG) {
        return INDEX_ARTISAN_CAVE_1F;
    }
    if (MAP_NUM(ROUTE118) == loc && MAP_GROUP(ROUTE118) == locG) {
        return INDEX_ROUTE118;
    }
    if (MAP_NUM(ROUTE133) == loc && MAP_GROUP(ROUTE133) == locG) {
        return INDEX_ROUTE133;
    }
    if (MAP_NUM(ROUTE131) == loc && MAP_GROUP(ROUTE131) == locG) {
        return INDEX_ROUTE131;
    }
    if (MAP_NUM(ALTERING_CAVE_1F) == loc && MAP_GROUP(ALTERING_CAVE_1F) == locG) {
        return INDEX_ALTERING_CAVE_1F;
    }
    if (MAP_NUM(ALTERING_CAVE) == loc && MAP_GROUP(ALTERING_CAVE) == locG) {
        return INDEX_ALTERING_CAVE;
    }
    if (MAP_NUM(FIERY_PATH) == loc && MAP_GROUP(FIERY_PATH) == locG) {
        return INDEX_FIERY_PATH;
    }
    if (MAP_NUM(MIRAGE_TOWER_B1F) == loc && MAP_GROUP(MIRAGE_TOWER_B1F) == locG) {
        return INDEX_MIRAGE_TOWER_B1F;
    }
    if (MAP_NUM(VICTORY_ROAD_B1F) == loc && MAP_GROUP(VICTORY_ROAD_B1F) == locG) {
        return INDEX_VICTORY_ROAD_B1F;
    }
    if (MAP_NUM(ROUTE130) == loc && MAP_GROUP(ROUTE130) == locG) {
        return INDEX_ROUTE130;
    }
    if (MAP_NUM(ROUTE110) == loc && MAP_GROUP(ROUTE110) == locG) {
        return INDEX_ROUTE110;
    }
    if (MAP_NUM(PETALBURG_WOODS_2) == loc && MAP_GROUP(PETALBURG_WOODS_2) == locG) {
        return INDEX_PETALBURG_WOODS_2;
    }
    if (MAP_NUM(ROUTE124) == loc && MAP_GROUP(ROUTE124) == locG) {
        return INDEX_ROUTE124;
    }
    if (MAP_NUM(SLATEPORT_CITY) == loc && MAP_GROUP(SLATEPORT_CITY) == locG) {
        return INDEX_SLATEPORT_CITY;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM4) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM4) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM4;
    }
    if (MAP_NUM(SCORCHED_SLAB_B1F) == loc && MAP_GROUP(SCORCHED_SLAB_B1F) == locG) {
        return INDEX_SCORCHED_SLAB_B1F;
    }
    if (MAP_NUM(SAFARI_ZONE_SOUTHEAST) == loc && MAP_GROUP(SAFARI_ZONE_SOUTHEAST) == locG) {
        return INDEX_SAFARI_ZONE_SOUTHEAST;
    }
    if (MAP_NUM(ROUTE123) == loc && MAP_GROUP(ROUTE123) == locG) {
        return INDEX_ROUTE123;
    }
    if (MAP_NUM(PETALBURG_CITY) == loc && MAP_GROUP(PETALBURG_CITY) == locG) {
        return INDEX_PETALBURG_CITY;
    }
    if (MAP_NUM(SEASPRAY_CAVE_B1F) == loc && MAP_GROUP(SEASPRAY_CAVE_B1F) == locG) {
        return INDEX_SEASPRAY_CAVE_B1F;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) == locG) {
        return INDEX_SHOAL_CAVE_LOW_TIDE_INNER_ROOM;
    }
    if (MAP_NUM(NEW_MAUVILLE_ENTRANCE) == loc && MAP_GROUP(NEW_MAUVILLE_ENTRANCE) == locG) {
        return INDEX_NEW_MAUVILLE_ENTRANCE;
    }
    if (MAP_NUM(ROUTE104) == loc && MAP_GROUP(ROUTE104) == locG) {
        return INDEX_ROUTE104;
    }
    if (MAP_NUM(SANDSTREWN_RUINS_2F) == loc && MAP_GROUP(SANDSTREWN_RUINS_2F) == locG) {
        return INDEX_SANDSTREWN_RUINS_2F;
    }
    if (MAP_NUM(ROUTE107) == loc && MAP_GROUP(ROUTE107) == locG) {
        return INDEX_ROUTE107;
    }
    if (MAP_NUM(UNDERWATER_ROUTE126) == loc && MAP_GROUP(UNDERWATER_ROUTE126) == locG) {
        return INDEX_UNDERWATER_ROUTE126;
    }
    if (MAP_NUM(MT_PYRE_SUMMIT) == loc && MAP_GROUP(MT_PYRE_SUMMIT) == locG) {
        return INDEX_MT_PYRE_SUMMIT;
    }
    if (MAP_NUM(GRANITE_CAVE_STEVENS_ROOM) == loc && MAP_GROUP(GRANITE_CAVE_STEVENS_ROOM) == locG) {
        return INDEX_GRANITE_CAVE_STEVENS_ROOM;
    }
    if (MAP_NUM(MIRAGE_TOWER_2F) == loc && MAP_GROUP(MIRAGE_TOWER_2F) == locG) {
        return INDEX_MIRAGE_TOWER_2F;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_1F) == loc && MAP_GROUP(MAGMA_HIDEOUT_1F) == locG) {
        return INDEX_MAGMA_HIDEOUT_1F;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM6) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM6) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM6;
    }
    if (MAP_NUM(SAFARI_ZONE_NORTHEAST) == loc && MAP_GROUP(SAFARI_ZONE_NORTHEAST) == locG) {
        return INDEX_SAFARI_ZONE_NORTHEAST;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_1F) == loc && MAP_GROUP(CAVE_OF_ORIGIN_1F) == locG) {
        return INDEX_CAVE_OF_ORIGIN_1F;
    }
    if (MAP_NUM(GRANITE_CAVE_B1F) == loc && MAP_GROUP(GRANITE_CAVE_B1F) == locG) {
        return INDEX_GRANITE_CAVE_B1F;
    }
    if (MAP_NUM(ROUTE134) == loc && MAP_GROUP(ROUTE134) == locG) {
        return INDEX_ROUTE134;
    }
    if (MAP_NUM(ASHEN_WOODS) == loc && MAP_GROUP(ASHEN_WOODS) == locG) {
        return INDEX_ASHEN_WOODS;
    }
    if (MAP_NUM(ALTERING_CAVE_B1F) == loc && MAP_GROUP(ALTERING_CAVE_B1F) == locG) {
        return INDEX_ALTERING_CAVE_B1F;
    }
    if (MAP_NUM(PACIFIDLOG_TOWN) == loc && MAP_GROUP(PACIFIDLOG_TOWN) == locG) {
        return INDEX_PACIFIDLOG_TOWN;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_3F_3R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_3R) == locG) {
        return INDEX_MAGMA_HIDEOUT_3F_3R;
    }
    if (MAP_NUM(SKY_PILLAR_1F) == loc && MAP_GROUP(SKY_PILLAR_1F) == locG) {
        return INDEX_SKY_PILLAR_1F;
    }
    if (MAP_NUM(SANDSTREWN_RUINS_3F) == loc && MAP_GROUP(SANDSTREWN_RUINS_3F) == locG) {
        return INDEX_SANDSTREWN_RUINS_3F;
    }
    if (MAP_NUM(UNDERWATER_ROUTE124) == loc && MAP_GROUP(UNDERWATER_ROUTE124) == locG) {
        return INDEX_UNDERWATER_ROUTE124;
    }
    if (MAP_NUM(SCORCHED_SLAB) == loc && MAP_GROUP(SCORCHED_SLAB) == locG) {
        return INDEX_SCORCHED_SLAB;
    }
    if (MAP_NUM(SCORCHED_SLAB_B2F) == loc && MAP_GROUP(SCORCHED_SLAB_B2F) == locG) {
        return INDEX_SCORCHED_SLAB_B2F;
    }
    if (MAP_NUM(EMBER_PATH) == loc && MAP_GROUP(EMBER_PATH) == locG) {
        return INDEX_EMBER_PATH;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_4F) == loc && MAP_GROUP(MAGMA_HIDEOUT_4F) == locG) {
        return INDEX_MAGMA_HIDEOUT_4F;
    }
    if (MAP_NUM(MT_PYRE_2F) == loc && MAP_GROUP(MT_PYRE_2F) == locG) {
        return INDEX_MT_PYRE_2F;
    }
    if (MAP_NUM(ROUTE121) == loc && MAP_GROUP(ROUTE121) == locG) {
        return INDEX_ROUTE121;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP1) == loc && MAP_GROUP(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP1) == locG) {
        return INDEX_CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP1;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_3F_1R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_1R) == locG) {
        return INDEX_MAGMA_HIDEOUT_3F_1R;
    }
    if (MAP_NUM(VICTORY_ROAD_B2F) == loc && MAP_GROUP(VICTORY_ROAD_B2F) == locG) {
        return INDEX_VICTORY_ROAD_B2F;
    }
    if (MAP_NUM(SAFARI_ZONE_NORTHWEST) == loc && MAP_GROUP(SAFARI_ZONE_NORTHWEST) == locG) {
        return INDEX_SAFARI_ZONE_NORTHWEST;
    }
    if (MAP_NUM(ROUTE129) == loc && MAP_GROUP(ROUTE129) == locG) {
        return INDEX_ROUTE129;
    }
    if (MAP_NUM(VICTORY_ROAD_1F) == loc && MAP_GROUP(VICTORY_ROAD_1F) == locG) {
        return INDEX_VICTORY_ROAD_1F;
    }
    if (MAP_NUM(DEWFORD_MANOR_1F) == loc && MAP_GROUP(DEWFORD_MANOR_1F) == locG) {
        return INDEX_DEWFORD_MANOR_1F;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_ENTRANCE) == loc && MAP_GROUP(CAVE_OF_ORIGIN_ENTRANCE) == locG) {
        return INDEX_CAVE_OF_ORIGIN_ENTRANCE;
    }
    if (MAP_NUM(OLDALE_TOWN) == loc && MAP_GROUP(OLDALE_TOWN) == locG) {
        return INDEX_OLDALE_TOWN;
    }
    if (MAP_NUM(ROUTE111) == loc && MAP_GROUP(ROUTE111) == locG) {
        return INDEX_ROUTE111;
    }
    if (MAP_NUM(SKY_PILLAR_3F) == loc && MAP_GROUP(SKY_PILLAR_3F) == locG) {
        return INDEX_SKY_PILLAR_3F;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) == locG) {
        return INDEX_SHOAL_CAVE_LOW_TIDE_LOWER_ROOM;
    }
    if (MAP_NUM(ROUTE112) == loc && MAP_GROUP(ROUTE112) == locG) {
        return INDEX_ROUTE112;
    }
    if (MAP_NUM(ROUTE102) == loc && MAP_GROUP(ROUTE102) == locG) {
        return INDEX_ROUTE102;
    }
    if (MAP_NUM(SOOTOPOLIS_CITY) == loc && MAP_GROUP(SOOTOPOLIS_CITY) == locG) {
        return INDEX_SOOTOPOLIS_CITY;
    }
    if (MAP_NUM(ROUTE122) == loc && MAP_GROUP(ROUTE122) == locG) {
        return INDEX_ROUTE122;
    }
    if (MAP_NUM(LILYCOVE_CITY) == loc && MAP_GROUP(LILYCOVE_CITY) == locG) {
        return INDEX_LILYCOVE_CITY;
    }
    if (MAP_NUM(SEASPRAY_CAVE) == loc && MAP_GROUP(SEASPRAY_CAVE) == locG) {
        return INDEX_SEASPRAY_CAVE;
    }
    if (MAP_NUM(DESERT_UNDERPASS) == loc && MAP_GROUP(DESERT_UNDERPASS) == locG) {
        return INDEX_DESERT_UNDERPASS;
    }
    if (MAP_NUM(ROUTE111_RUINS_EXTERIOR) == loc && MAP_GROUP(ROUTE111_RUINS_EXTERIOR) == locG) {
        return INDEX_ROUTE111_RUINS_EXTERIOR;
    }
    if (MAP_NUM(ABANDONED_SHIP_HIDDEN_FLOOR_CORRIDORS) == loc && MAP_GROUP(ABANDONED_SHIP_HIDDEN_FLOOR_CORRIDORS) == locG) {
        return INDEX_ABANDONED_SHIP_HIDDEN_FLOOR_CORRIDORS;
    }
    if (MAP_NUM(ROUTE125) == loc && MAP_GROUP(ROUTE125) == locG) {
        return INDEX_ROUTE125;
    }
    if (MAP_NUM(SCORCHED_SLAB_HEATRANS_ROOM) == loc && MAP_GROUP(SCORCHED_SLAB_HEATRANS_ROOM) == locG) {
        return INDEX_SCORCHED_SLAB_HEATRANS_ROOM;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP2) == loc && MAP_GROUP(CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP2) == locG) {
        return INDEX_CAVE_OF_ORIGIN_UNUSED_RUBY_SAPPHIRE_MAP2;
    }
    if (MAP_NUM(SECRET_DUNGEON) == loc && MAP_GROUP(SECRET_DUNGEON) == locG) {
        return INDEX_SECRET_DUNGEON;
    }
    if (MAP_NUM(ROUTE101) == loc && MAP_GROUP(ROUTE101) == locG) {
        return INDEX_ROUTE101;
    }
    if (MAP_NUM(MT_PYRE_3F) == loc && MAP_GROUP(MT_PYRE_3F) == locG) {
        return INDEX_MT_PYRE_3F;
    }
    if (MAP_NUM(MT_PYRE_6F) == loc && MAP_GROUP(MT_PYRE_6F) == locG) {
        return INDEX_MT_PYRE_6F;
    }
    if (MAP_NUM(ROUTE103) == loc && MAP_GROUP(ROUTE103) == locG) {
        return INDEX_ROUTE103;
    }
    if (MAP_NUM(GRANITE_CAVE_B2F) == loc && MAP_GROUP(GRANITE_CAVE_B2F) == locG) {
        return INDEX_GRANITE_CAVE_B2F;
    }
    if (MAP_NUM(METEOR_FALLS_B1F_1R) == loc && MAP_GROUP(METEOR_FALLS_B1F_1R) == locG) {
        return INDEX_METEOR_FALLS_B1F_1R;
    }
    if (MAP_NUM(SKY_PILLAR_5F) == loc && MAP_GROUP(SKY_PILLAR_5F) == locG) {
        return INDEX_SKY_PILLAR_5F;
    }
    if (MAP_NUM(MIRAGE_TOWER_1F) == loc && MAP_GROUP(MIRAGE_TOWER_1F) == locG) {
        return INDEX_MIRAGE_TOWER_1F;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ROOM5) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM5) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ROOM5;
    }
    if (MAP_NUM(EVER_GRANDE_CITY) == loc && MAP_GROUP(EVER_GRANDE_CITY) == locG) {
        return INDEX_EVER_GRANDE_CITY;
    }
    if (MAP_NUM(SANDSTREWN_RUINS_B1F) == loc && MAP_GROUP(SANDSTREWN_RUINS_B1F) == locG) {
        return INDEX_SANDSTREWN_RUINS_B1F;
    }
    if (MAP_NUM(MIRAGE_TOWER_3F) == loc && MAP_GROUP(MIRAGE_TOWER_3F) == locG) {
        return INDEX_MIRAGE_TOWER_3F;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_2F_2R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_2R) == locG) {
        return INDEX_MAGMA_HIDEOUT_2F_2R;
    }
    if (MAP_NUM(SAFARI_ZONE_NORTH) == loc && MAP_GROUP(SAFARI_ZONE_NORTH) == locG) {
        return INDEX_SAFARI_ZONE_NORTH;
    }
    if (MAP_NUM(GRANITE_CAVE_1F) == loc && MAP_GROUP(GRANITE_CAVE_1F) == locG) {
        return INDEX_GRANITE_CAVE_1F;
    }
    if (MAP_NUM(ROUTE127) == loc && MAP_GROUP(ROUTE127) == locG) {
        return INDEX_ROUTE127;
    }
    if (MAP_NUM(SAFARI_ZONE_SOUTHWEST) == loc && MAP_GROUP(SAFARI_ZONE_SOUTHWEST) == locG) {
        return INDEX_SAFARI_ZONE_SOUTHWEST;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) == locG) {
        return INDEX_SHOAL_CAVE_LOW_TIDE_ICE_ROOM;
    }
    if (MAP_NUM(MT_PYRE_4F) == loc && MAP_GROUP(MT_PYRE_4F) == locG) {
        return INDEX_MT_PYRE_4F;
    }
    if (MAP_NUM(ROUTE106) == loc && MAP_GROUP(ROUTE106) == locG) {
        return INDEX_ROUTE106;
    }
    if (MAP_NUM(RUSTURF_TUNNEL) == loc && MAP_GROUP(RUSTURF_TUNNEL) == locG) {
        return INDEX_RUSTURF_TUNNEL;
    }
    if (MAP_NUM(ROUTE116) == loc && MAP_GROUP(ROUTE116) == locG) {
        return INDEX_ROUTE116;
    }
    if (MAP_NUM(MAGMA_HIDEOUT_3F_2R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_2R) == locG) {
        return INDEX_MAGMA_HIDEOUT_3F_2R;
    }
    if (MAP_NUM(PETALBURG_WOODS) == loc && MAP_GROUP(PETALBURG_WOODS) == locG) {
        return INDEX_PETALBURG_WOODS;
    }
    if (MAP_NUM(DEWFORD_TOWN) == loc && MAP_GROUP(DEWFORD_TOWN) == locG) {
        return INDEX_DEWFORD_TOWN;
    }
    if (MAP_NUM(ROUTE114) == loc && MAP_GROUP(ROUTE114) == locG) {
        return INDEX_ROUTE114;
    }
    if (MAP_NUM(ROUTE117) == loc && MAP_GROUP(ROUTE117) == locG) {
        return INDEX_ROUTE117;
    }
    if (MAP_NUM(MT_PYRE_5F) == loc && MAP_GROUP(MT_PYRE_5F) == locG) {
        return INDEX_MT_PYRE_5F;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ENTRANCE) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ENTRANCE) == locG) {
        return INDEX_SEAFLOOR_CAVERN_ENTRANCE;
    }
    if (MAP_NUM(MIRAGE_TOWER_4F) == loc && MAP_GROUP(MIRAGE_TOWER_4F) == locG) {
        return INDEX_MIRAGE_TOWER_4F;
    }
    return ROUTE_INDEX_COUNT;
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

void ClearAllRouteEncounters() {
    gSaveBlock2Ptr->encounteredroutes1 = 0;
    gSaveBlock2Ptr->encounteredroutes2 = 0;
    gSaveBlock2Ptr->encounteredroutes3 = 0;
    gSaveBlock2Ptr->encounteredroutes4 = 0;
    gSaveBlock2Ptr->encounteredroutes5 = 0;
}
