#include "global.h"
#include "malloc.h"
#include "apprentice.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_util.h"
#include "data.h"
#include "dexnav.h"
#include "daycare.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "field_specials.h"
#include "field_weather.h"
#include "item.h"
#include "link.h"
#include "main.h"
#include "overworld.h"
#include "m4a.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokeblock.h"
#include "pokemon.h"
#include "pokemon_animation.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "recorded_battle.h"
#include "rtc.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "trainer_hill.h"
#include "util.h"
#include "quests.h"
#include "generated/constants/abilities.h"
#include "constants/battle_frontier.h"
#include "generated/constants/battle_move_effects.h"
#include "constants/hold_effects.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "generated/constants/moves.h"
#include "constants/songs.h"
#include "constants/party_menu.h"
#include "generated/constants/species.h"
#include "constants/trainers.h"
#include "constants/weather.h"
#include "constants/battle_config.h"
#include "data/trainer_spreads.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "script_pokemon_util.h"
#include "tmhm_struct.h"
#include "abilities.hh"
#include "day_night.h"
#include "constants/day_night.h"

struct SpeciesItem {
    SpeciesEnum species;
    u16 item;
};

// this file's functions
static void Task_PlayMapChosenOrBattleBGM(u8 taskId);
// static u16 GiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move);
static bool8 ShouldSkipFriendshipChange(void);
static void ShuffleStatArray(u8 *statArray);

// EWRAM vars
EWRAM_DATA static u8 sLearningMoveTableID = 0;
EWRAM_DATA u8 gPlayerPartyCount = 0;
EWRAM_DATA u8 gEnemyPartyCount = 0;
EWRAM_DATA struct Pokemon gPlayerParty[PARTY_SIZE] = {0};
EWRAM_DATA struct Pokemon gEnemyParty[PARTY_SIZE] = {0};
EWRAM_DATA struct SpriteTemplate gMultiuseSpriteTemplate = {0};
EWRAM_DATA struct Unknown_806F160_Struct *gUnknown_020249B4[2] = {NULL};

// const rom data
#include "generated/data/battle_moves.h"

const struct IntimidateCloneData gIntimidateCloneData[] = {
    {
        .ability = ABILITY_INTIMIDATE,
        .numStatsLowered = 1,
        .statsLowered = {STAT_ATK, 0, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_SCARE,
        .numStatsLowered = 1,
        .statsLowered = {STAT_SPATK, 0, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_FEARMONGER,
        .numStatsLowered = 2,
        .statsLowered = {STAT_ATK, STAT_SPATK, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_YUKI_ONNA,
        .numStatsLowered = 2,
        .statsLowered = {STAT_ATK, STAT_SPATK, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_MONKEY_BUSINESS,
        .numStatsLowered = 2,
        .statsLowered = {STAT_ATK, STAT_DEF, 0},
        .targetBoth = FALSE,
    },
    {
        .ability = ABILITY_MALICIOUS,
        .numStatsLowered = 2,
        .statsLowered = {STAT_HIGHEST_ATTACKING | STAT_USE_STAT_BOOSTS_IN_CALC, STAT_HIGHEST_DEFENDING | STAT_USE_STAT_BOOSTS_IN_CALC, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_TERRIFY,
        .numStatsLowered = 1,
        .statsLowered = {STAT_SPATK, 0, 0},
        .statChange = 2,
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_PETRIFY,
        .numStatsLowered = 1,
        .statsLowered = {STAT_SPEED, 0, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_GLEAM_EYES,
        .numStatsLowered = 1,
        .statsLowered = {STAT_SPATK, 0, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_CHAMPIONS_ENTRANCE,
        .numStatsLowered = 1,
        .statsLowered = {STAT_ATK, 0, 0},
        .targetBoth = TRUE,
    },
    {
        .ability = ABILITY_SCARECROW,
        .numStatsLowered = 1,
        .statsLowered = {STAT_SPATK, 0, 0},
        .targetBoth = TRUE,
    },
};

// Used in an unreferenced function in RS.
// Unreferenced here and in FRLG.
struct CombinedMove {
    u16 move1;
    u16 move2;
    u16 newMove;
};

static const struct CombinedMove sCombinedMoves[2] = {{MOVE_EMBER, MOVE_GUST, MOVE_HEAT_WAVE}, {0xFFFF, 0xFFFF, 0xFFFF}};

#include "generated/data/pokemon/pokedex_numbers.h"

const struct SpindaSpot gSpindaSpotGraphics[] = {{16, 7, INCBIN_U16("graphics/spinda_spots/spot_0.bin")},
                                                 {40, 8, INCBIN_U16("graphics/spinda_spots/spot_1.bin")},
                                                 {22, 25, INCBIN_U16("graphics/spinda_spots/spot_2.bin")},
                                                 {34, 26, INCBIN_U16("graphics/spinda_spots/spot_3.bin")}};

#include "data/pokemon/item_effects.h"

#define __NATURE_STAT(stat, up, down) (up == stat) - (down == stat)
#define NATURE_STAT(up, down)             \
    {__NATURE_STAT(STAT_ATK, up, down),   \
     __NATURE_STAT(STAT_DEF, up, down),   \
     __NATURE_STAT(STAT_SPEED, up, down), \
     __NATURE_STAT(STAT_SPATK, up, down), \
     __NATURE_STAT(STAT_SPDEF, up, down)}
const s8 gNatureStatTable[NUM_NATURES][NUM_NATURE_STATS] = {
    // Atk Def Spd Sp.Atk Sp.Def
    [NATURE_HARDY] = NATURE_STAT(STAT_ATK, STAT_ATK),       [NATURE_LONELY] = NATURE_STAT(STAT_ATK, STAT_DEF),
    [NATURE_BRAVE] = NATURE_STAT(STAT_ATK, STAT_SPEED),     [NATURE_ADAMANT] = NATURE_STAT(STAT_ATK, STAT_SPATK),
    [NATURE_NAUGHTY] = NATURE_STAT(STAT_ATK, STAT_SPDEF),   [NATURE_BOLD] = NATURE_STAT(STAT_DEF, STAT_ATK),
    [NATURE_DOCILE] = NATURE_STAT(STAT_DEF, STAT_DEF),      [NATURE_RELAXED] = NATURE_STAT(STAT_DEF, STAT_SPEED),
    [NATURE_IMPISH] = NATURE_STAT(STAT_DEF, STAT_SPATK),    [NATURE_LAX] = NATURE_STAT(STAT_DEF, STAT_SPDEF),
    [NATURE_TIMID] = NATURE_STAT(STAT_SPEED, STAT_ATK),     [NATURE_HASTY] = NATURE_STAT(STAT_SPEED, STAT_DEF),
    [NATURE_SERIOUS] = NATURE_STAT(STAT_SPEED, STAT_SPEED), [NATURE_JOLLY] = NATURE_STAT(STAT_SPEED, STAT_SPATK),
    [NATURE_NAIVE] = NATURE_STAT(STAT_SPEED, STAT_SPDEF),   [NATURE_MODEST] = NATURE_STAT(STAT_SPATK, STAT_ATK),
    [NATURE_MILD] = NATURE_STAT(STAT_SPATK, STAT_DEF),      [NATURE_QUIET] = NATURE_STAT(STAT_SPATK, STAT_SPEED),
    [NATURE_BASHFUL] = NATURE_STAT(STAT_SPATK, STAT_SPATK), [NATURE_RASH] = NATURE_STAT(STAT_SPATK, STAT_SPDEF),
    [NATURE_CALM] = NATURE_STAT(STAT_SPDEF, STAT_ATK),      [NATURE_GENTLE] = NATURE_STAT(STAT_SPDEF, STAT_DEF),
    [NATURE_SASSY] = NATURE_STAT(STAT_SPDEF, STAT_SPEED),   [NATURE_CAREFUL] = NATURE_STAT(STAT_SPDEF, STAT_SPATK),
    [NATURE_QUIRKY] = NATURE_STAT(STAT_SPDEF, STAT_SPDEF),
};

#include "data/pokemon/trainer_class_lookups.h"
#include "data/pokemon/experience_tables.h"
#include "generated/data/pokemon/base_stats.h"
#include "generated/data/pokemon/level_up_learnset_pointers.h"
#include "generated/data/pokemon/evolution.h"
#include "generated/data/pokemon/form_species_table_pointers.h"
#include "data/pokemon/form_change_tables.h"
#include "data/pokemon/form_change_table_pointers.h"
#include "data/randomizer.h"
// SPECIES_NONE are ignored in the following two tables, so decrement before accessing these arrays to get the right result

#include "generated/data/pokemon_graphics/front_anim_ids.h"

static const u8 sMonAnimationDelayTable[NUM_SPECIES - 1] = {
    [SPECIES_BLASTOISE - 1] = 50, [SPECIES_WEEDLE - 1] = 10,     [SPECIES_KAKUNA - 1] = 20,    [SPECIES_BEEDRILL - 1] = 35, [SPECIES_PIDGEOTTO - 1] = 25,
    [SPECIES_FEAROW - 1] = 2,     [SPECIES_EKANS - 1] = 30,      [SPECIES_NIDORAN_F - 1] = 28, [SPECIES_NIDOKING - 1] = 25, [SPECIES_PARAS - 1] = 10,
    [SPECIES_PARASECT - 1] = 45,  [SPECIES_VENONAT - 1] = 20,    [SPECIES_DIGLETT - 1] = 25,   [SPECIES_DUGTRIO - 1] = 35,  [SPECIES_MEOWTH - 1] = 40,
    [SPECIES_PERSIAN - 1] = 20,   [SPECIES_MANKEY - 1] = 20,     [SPECIES_GROWLITHE - 1] = 30, [SPECIES_ARCANINE - 1] = 40, [SPECIES_POLIWHIRL - 1] = 5,
    [SPECIES_WEEPINBELL - 1] = 3, [SPECIES_MUK - 1] = 45,        [SPECIES_SHELLDER - 1] = 20,  [SPECIES_HAUNTER - 1] = 23,  [SPECIES_DROWZEE - 1] = 48,
    [SPECIES_HYPNO - 1] = 40,     [SPECIES_HITMONCHAN - 1] = 25, [SPECIES_SCYTHER - 1] = 10,   [SPECIES_TAUROS - 1] = 10,   [SPECIES_TYPHLOSION - 1] = 20,
    [SPECIES_FERALIGATR - 1] = 5, [SPECIES_NATU - 1] = 30,       [SPECIES_MAREEP - 1] = 50,    [SPECIES_AMPHAROS - 1] = 10, [SPECIES_POLITOED - 1] = 40,
    [SPECIES_DUNSPARCE - 1] = 10, [SPECIES_STEELIX - 1] = 45,    [SPECIES_QWILFISH - 1] = 39,  [SPECIES_SCIZOR - 1] = 19,   [SPECIES_OCTILLERY - 1] = 20,
    [SPECIES_SMOOCHUM - 1] = 40,  [SPECIES_TYRANITAR - 1] = 10,  [SPECIES_LUGIA - 1] = 20,     [SPECIES_WAILORD - 1] = 10,  [SPECIES_KECLEON - 1] = 30,
    [SPECIES_MILOTIC - 1] = 45,   [SPECIES_SPHEAL - 1] = 15,     [SPECIES_SNORUNT - 1] = 20,   [SPECIES_GRUMPIG - 1] = 15,  [SPECIES_WYNAUT - 1] = 15,
    [SPECIES_DUSCLOPS - 1] = 30,  [SPECIES_ABSOL - 1] = 45,      [SPECIES_SALAMENCE - 1] = 70, [SPECIES_KYOGRE - 1] = 60,   [SPECIES_RAYQUAZA - 1] = 60,
    [SPECIES_TAPU_FINI - 1] = 5,  [SPECIES_ROTOM_FAN - 1] = 7,
};

const u8 gPPUpGetMask[] = {0x03, 0x0c, 0x30, 0xc0};  // Masks for getting PP Up count, also PP Max values
const u8 gPPUpSetMask[] = {0xfc, 0xf3, 0xcf, 0x3f};  // Masks for setting PP Up count
const u8 gPPUpAddMask[] = {0x01, 0x04, 0x10, 0x40};  // Values added to PP Up count

const u8 gStatStageRatios[MAX_STAT_STAGE + 1][2] = {
    {10, 40},  // -6, MIN_STAT_STAGE
    {10, 35},  // -5
    {10, 30},  // -4
    {10, 25},  // -3
    {10, 20},  // -2
    {10, 15},  // -1
    {10, 10},  //  0, DEFAULT_STAT_STAGE
    {15, 10},  // +1
    {20, 10},  // +2
    {25, 10},  // +3
    {30, 10},  // +4
    {35, 10},  // +5
    {40, 10},  // +6, MAX_STAT_STAGE
};

const u16 gLinkPlayerFacilityClasses[NUM_MALE_LINK_FACILITY_CLASSES + NUM_FEMALE_LINK_FACILITY_CLASSES] = {
    // Male classes
    FACILITY_CLASS_COOLTRAINER_M,
    FACILITY_CLASS_BLACK_BELT,
    FACILITY_CLASS_CAMPER,
    FACILITY_CLASS_YOUNGSTER,
    FACILITY_CLASS_PSYCHIC_M,
    FACILITY_CLASS_BUG_CATCHER,
    FACILITY_CLASS_PKMN_BREEDER_M,
    FACILITY_CLASS_GUITARIST,
    // Female Classes
    FACILITY_CLASS_COOLTRAINER_F,
    FACILITY_CLASS_HEX_MANIAC,
    FACILITY_CLASS_PICNICKER,
    FACILITY_CLASS_LASS,
    FACILITY_CLASS_PSYCHIC_F,
    FACILITY_CLASS_BATTLE_GIRL,
    FACILITY_CLASS_PKMN_BREEDER_F,
    FACILITY_CLASS_BEAUTY};

const struct SpriteTemplate gBattlerSpriteTemplates[MAX_BATTLERS_COUNT] = {
    [B_POSITION_PLAYER_LEFT] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gBattlerPicTable_PlayerLeft,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [B_POSITION_OPPONENT_LEFT] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpriteOpponentSide,
            .anims = NULL,
            .images = gBattlerPicTable_OpponentLeft,
            .affineAnims = gAffineAnims_BattleSpriteOpponentSide,
            .callback = SpriteCb_WildMon,
        },
    [B_POSITION_PLAYER_RIGHT] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gBattlerPicTable_PlayerRight,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [B_POSITION_OPPONENT_RIGHT] = {.tileTag = 0xFFFF,
                                   .paletteTag = 0,
                                   .oam = &gOamData_BattleSpriteOpponentSide,
                                   .anims = NULL,
                                   .images = gBattlerPicTable_OpponentRight,
                                   .affineAnims = gAffineAnims_BattleSpriteOpponentSide,
                                   .callback = SpriteCb_WildMon},
};

static const struct SpriteTemplate sTrainerBackSpriteTemplates[] = {
    [TRAINER_BACK_PIC_BRENDAN] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_Brendan,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_MAY] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_May,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_RED] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_Red,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_LEAF] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_Leaf,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_RubySapphireBrendan,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_RUBY_SAPPHIRE_MAY] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_RubySapphireMay,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_WALLY] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_Wally,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
    [TRAINER_BACK_PIC_STEVEN] =
        {
            .tileTag = 0xFFFF,
            .paletteTag = 0,
            .oam = &gOamData_BattleSpritePlayerSide,
            .anims = NULL,
            .images = gTrainerBackPicTable_Steven,
            .affineAnims = gAffineAnims_BattleSpritePlayerSide,
            .callback = SpriteCB_BattleSpriteStartSlideLeft,
        },
};

static const u8 sSecretBaseFacilityClasses[2][5] = {
    {FACILITY_CLASS_YOUNGSTER, FACILITY_CLASS_BUG_CATCHER, FACILITY_CLASS_RICH_BOY, FACILITY_CLASS_CAMPER, FACILITY_CLASS_COOLTRAINER_M},
    {FACILITY_CLASS_LASS, FACILITY_CLASS_SCHOOL_KID_F, FACILITY_CLASS_LADY, FACILITY_CLASS_PICNICKER, FACILITY_CLASS_COOLTRAINER_F}};

static const u8 sGetMonDataEVConstants[] = {MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPEED_EV, MON_DATA_SPDEF_EV, MON_DATA_SPATK_EV};

static const u8 sGetMonDataIVConstants[] = {MON_DATA_HP_IV, MON_DATA_ATK_IV, MON_DATA_DEF_IV, MON_DATA_SPEED_IV, MON_DATA_SPDEF_IV, MON_DATA_SPATK_IV};

// For stat-raising items
static const u8 sStatsToRaise[] = {
#ifndef ITEM_EXPANSION
    STAT_ATK, STAT_ATK, STAT_SPEED, STAT_DEF, STAT_SPATK, STAT_ACC
#else
    STAT_ATK, STAT_ATK, STAT_DEF, STAT_SPEED, STAT_SPATK, STAT_SPDEF, STAT_ACC
#endif
};

// 3 modifiers each for how much to change friendship for different ranges
// 0-99, 100-199, 200+
static const s8 sFriendshipEventModifiers[][3] = {
    [FRIENDSHIP_EVENT_GROW_LEVEL] = {5, 3, 2},
    [FRIENDSHIP_EVENT_VITAMIN] = {5, 3, 2},
    [FRIENDSHIP_EVENT_BATTLE_ITEM] = {1, 1, 0},
    [FRIENDSHIP_EVENT_LEAGUE_BATTLE] = {3, 2, 1},
    [FRIENDSHIP_EVENT_LEARN_TMHM] = {1, 1, 0},
    [FRIENDSHIP_EVENT_WALKING] = {1, 1, 1},
    [FRIENDSHIP_EVENT_FAINT_SMALL] = {-1, -1, -1},
    [FRIENDSHIP_EVENT_FAINT_FIELD_PSN] = {-5, -5, -10},
    [FRIENDSHIP_EVENT_FAINT_LARGE] = {-5, -5, -10},
};

static const u16 sHMMoves[] = {MOVE_CUT, MOVE_FLY, MOVE_SURF, MOVE_STRENGTH, MOVE_FLASH, MOVE_ROCK_SMASH, MOVE_WATERFALL, MOVE_DIVE, 0xFFFF};

static const struct SpeciesItem sAlteringCaveWildMonHeldItems[] = {
    {SPECIES_NONE, ITEM_NONE},
    {SPECIES_MAREEP, ITEM_GANLON_BERRY},
    {SPECIES_PINECO, ITEM_APICOT_BERRY},
    {SPECIES_HOUNDOUR, ITEM_BIG_MUSHROOM},
    {SPECIES_TEDDIURSA, ITEM_PETAYA_BERRY},
    {SPECIES_AIPOM, ITEM_BERRY_JUICE},
    {SPECIES_SHUCKLE, ITEM_BERRY_JUICE},
    {SPECIES_STANTLER, ITEM_PETAYA_BERRY},
    {SPECIES_SMEARGLE, ITEM_SALAC_BERRY},
};

static const struct OamData sOamData_8329F20 = {.y = 0,
                                                .affineMode = ST_OAM_AFFINE_OFF,
                                                .objMode = ST_OAM_OBJ_NORMAL,
                                                .mosaic = 0,
                                                .bpp = ST_OAM_4BPP,
                                                .shape = SPRITE_SHAPE(64x64),
                                                .x = 0,
                                                .matrixNum = 0,
                                                .size = SPRITE_SIZE(64x64),
                                                .tileNum = 0,
                                                .priority = 0,
                                                .paletteNum = 0,
                                                .affineParam = 0};

static const struct SpriteTemplate gUnknown_08329F28 = {
    .tileTag = 0xFFFF,
    .paletteTag = 0xFFFF,
    .oam = &sOamData_8329F20,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// code
void ZeroBoxMonData(struct BoxPokemon *boxMon) {
    u8 *raw = (u8 *)boxMon;
    u32 i;
    for (i = 0; i < sizeof(struct BoxPokemon); i++) raw[i] = 0;
}

void ZeroMonData(struct Pokemon *mon) {
    u32 arg;
    ZeroBoxMonData(&mon->box);
    arg = 0;
    SetMonData(mon, MON_DATA_STATUS, &arg);
    SetMonData(mon, MON_DATA_LEVEL, &arg);
    SetMonData(mon, MON_DATA_HP, &arg);
    SetMonData(mon, MON_DATA_MAX_HP, &arg);
    SetMonData(mon, MON_DATA_ATK, &arg);
    SetMonData(mon, MON_DATA_DEF, &arg);
    SetMonData(mon, MON_DATA_SPEED, &arg);
    SetMonData(mon, MON_DATA_SPATK, &arg);
    SetMonData(mon, MON_DATA_SPDEF, &arg);
    arg = 255;
    SetMonData(mon, MON_DATA_MAIL, &arg);
}

void ZeroPlayerPartyMons(void) {
    s32 i;
    for (i = 0; i < PARTY_SIZE; i++) ZeroMonData(&gPlayerParty[i]);
}

void ZeroEnemyPartyMons(void) {
    s32 i;
    for (i = 0; i < PARTY_SIZE; i++) ZeroMonData(&gEnemyParty[i]);
}

void CreateMon(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId) {
    u32 arg;
    ZeroMonData(mon);
    CreateBoxMon(&mon->box, species, level, fixedIV, hasFixedPersonality, fixedPersonality, otIdType, fixedOtId);
    SetMonData(mon, MON_DATA_LEVEL, &level);
    arg = 255;
    SetMonData(mon, MON_DATA_MAIL, &arg);
    CalculateMonStats(mon);
}

u8 GenerateShinyForm(SpeciesEnum species) {
    u8 numShinies = gBaseStats[species].numShinies;
    u8 isShiny = SHINY_VANILLA;
    u16 rand = Random();  // Max value is 65535

    // Rare shiny creation
    if (rand < LEGENDARY_SHINY_ODDS && numShinies >= SHINY_LEGENDARY)
        isShiny = SHINY_LEGENDARY;
    else if (rand < RARE_SHINY_ODDS && numShinies >= SHINY_RARE)
        isShiny = SHINY_RARE;

    return isShiny;
}

void CreateBoxMon(
    struct BoxPokemon *boxMon, SpeciesEnum species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId) {
    u8 speciesName[POKEMON_NAME_LENGTH + 1];
    u32 personality;
    u32 value;
    u8 nature;
    u8 maxIV = MAX_IV_MASK;
    u8 statIDs[NUM_STATS] = {0, 1, 2, 3, 4, 5};
    u8 hpType;
    u8 isShiny = SHINY_NONE;
    bool8 isAlpha = FALSE;

    ZeroBoxMonData(boxMon);

    if (hasFixedPersonality)
        personality = fixedPersonality;
    else
        personality = Random32();

    // Determine original trainer ID
    if (otIdType == OT_ID_RANDOM_NO_SHINY) {
        value = Random32();
    } else if (otIdType == OT_ID_PRESET)  // Pokemon has a preset OT ID
    {
        value = fixedOtId;
    } else  // Player is the OT
    {
        u32 shinyRolls = (CheckBagHasItem(ITEM_SHINY_CHARM, 1)) ? 3 : 1;
        u8 i;

        value = gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8) | (gSaveBlock2Ptr->playerTrainerId[2] << 16) |
                (gSaveBlock2Ptr->playerTrainerId[3] << 24);

        if (!FlagGet(FLAG_SHINY_CREATION)) {
            for (i = 0; i < shinyRolls; i++) {
                if (Random() < getShinyOdds()) {
                    FlagSet(FLAG_SHINY_CREATION);
                    break;
                }
            }
        }

        if (FlagGet(FLAG_SHINY_CREATION)) isShiny = GenerateShinyForm(species);

        FlagClear(FLAG_SHINY_CREATION);
    }

    if (FlagGet(FLAG_ALPHA_CREATION)) isAlpha = TRUE;

    FlagClear(FLAG_ALPHA_CREATION);

    SetBoxMonData(boxMon, MON_DATA_PERSONALITY, &personality);
    SetBoxMonData(boxMon, MON_DATA_OT_ID, &value);

    GetSpeciesName(speciesName, species);
    SetBoxMonData(boxMon, MON_DATA_NICKNAME, speciesName);
    SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &gGameLanguage);
    SetBoxMonData(boxMon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetBoxMonData(boxMon, MON_DATA_SPECIES, &species);
    SetBoxMonData(boxMon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][level]);
    value = 4;
    SetBoxMonData(boxMon, MON_DATA_FRIENDSHIP, &value);
    value = GetCurrentRegionMapSectionId();
    SetBoxMonData(boxMon, MON_DATA_MET_LOCATION, &value);
    SetBoxMonData(boxMon, MON_DATA_MET_LEVEL, &level);
    SetBoxMonData(boxMon, MON_DATA_MET_GAME, &gGameVersion);
    value = ITEM_POKE_BALL;
    SetBoxMonData(boxMon, MON_DATA_POKEBALL, &value);
    SetBoxMonData(boxMon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);
    SetBoxMonData(boxMon, MON_DATA_IS_SHINY, &isShiny);
    SetBoxMonData(boxMon, MON_DATA_MAX_SHINY, &isShiny);
    SetBoxMonData(boxMon, MON_DATA_IS_ALPHA, &isAlpha);

    if (isShiny != SHINY_NONE && VarGet(VAR_DEXNAV_SHINY_FLAG) == 1) VarSet(VAR_DEXNAV_SHINY_FLAG, 2);

    if (fixedIV < USE_RANDOM_IVS) {
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &fixedIV);
    } else {
        u32 iv, i;
        value = Random();

        iv = value & MAX_IV_MASK;
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &iv);
        iv = (value & (MAX_IV_MASK << 5)) >> 5;
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &iv);
        iv = (value & (MAX_IV_MASK << 10)) >> 10;
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &iv);

        value = Random();

        iv = value & MAX_IV_MASK;
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &iv);
        iv = (value & (MAX_IV_MASK << 5)) >> 5;
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &iv);
        iv = (value & (MAX_IV_MASK << 10)) >> 10;
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &iv);

        // Set three random IVs to 31
        ShuffleStatArray(statIDs);

        if (GetBoxMonDataAt(TOTAL_BOXES_COUNT - 1, IN_BOX_COUNT - 1, MON_DATA_HP_IV) == 1 &&
            GetBoxMonDataAt(TOTAL_BOXES_COUNT - 1, IN_BOX_COUNT - 1, MON_DATA_EXP) == 0) {
            for (i = 0; i < 6; i++) {
                SetBoxMonData(boxMon, MON_DATA_HP_IV + statIDs[i], &maxIV);
            }
        } else {
            for (i = 0; i < 3; i++) {
                SetBoxMonData(boxMon, MON_DATA_HP_IV + statIDs[i], &maxIV);
            }
        }
    }

    do {
        hpType = Random() % (NUMBER_OF_MON_TYPES - 1);  // No stellar type
    } while (hpType == TYPE_MYSTERY);

    SetBoxMonData(boxMon, MON_DATA_HP_TYPE, &hpType);

    nature = personality % 25;
    SetBoxMonData(boxMon, MON_DATA_NATURE, &nature);

    if (gBaseStats[species].abilities[1]) {
        value = personality & 1;
        SetBoxMonData(boxMon, MON_DATA_ABILITY_NUM, &value);
    }

    GiveBoxMonInitialMoveset(boxMon);
}

void CreateMonWithNature(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 nature) {
    u32 personality = Random32();
    CreateMon(mon, species, level, fixedIV, 0, personality, OT_ID_PLAYER_ID, 0);
    SetMonData(mon, MON_DATA_NATURE, &nature);
    CalculateMonStats(mon);
}

void CreateMonWithGenderNatureLetter(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 gender, u8 nature, u8 unownLetter) {
    u32 personality;

    if ((u8)(unownLetter - 1) < NUM_UNOWN_FORMS) {
        u16 actualLetter;

        do {
            personality = Random32();
            actualLetter = GET_UNOWN_LETTER(personality);
        } while (gender != GetGenderFromSpeciesAndPersonality(species, personality) || actualLetter != unownLetter - 1);
    } else {
        do {
            personality = Random32();
        } while (gender != GetGenderFromSpeciesAndPersonality(species, personality));
    }

    CreateMon(mon, species, level, fixedIV, 1, personality, OT_ID_PLAYER_ID, 0);
    SetMonData(mon, MON_DATA_NATURE, &nature);
    CalculateMonStats(mon);
}

// This is only used to create Wally's Ralts.
void CreateMaleMon(struct Pokemon *mon, SpeciesEnum species, u8 level) {
    u32 personality;
    u32 otId;

    do {
        otId = Random32();
        personality = Random32();
    } while (GetGenderFromSpeciesAndPersonality(species, personality) != MON_MALE);
    CreateMon(mon, species, level, 31, 1, personality, OT_ID_PRESET, otId);
}

void CreateMonWithIVsPersonality(struct Pokemon *mon, SpeciesEnum species, u8 level, u32 ivs, u32 personality) {
    CreateMon(mon, species, level, 0, 1, personality, OT_ID_PLAYER_ID, 0);
    SetMonData(mon, MON_DATA_IVS, &ivs);
    CalculateMonStats(mon);
}

void CreateMonWithIVsOTID(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 *ivs, u32 otId) {
    CreateMon(mon, species, level, 0, 0, 0, OT_ID_PRESET, otId);
    SetMonData(mon, MON_DATA_HP_IV, &ivs[0]);
    SetMonData(mon, MON_DATA_ATK_IV, &ivs[1]);
    SetMonData(mon, MON_DATA_DEF_IV, &ivs[2]);
    SetMonData(mon, MON_DATA_SPEED_IV, &ivs[3]);
    SetMonData(mon, MON_DATA_SPATK_IV, &ivs[4]);
    SetMonData(mon, MON_DATA_SPDEF_IV, &ivs[5]);
    CalculateMonStats(mon);
}

void CreateMonWithEVSpread(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 evSpread) {
    s32 i;
    s32 statCount = 0;
    u16 evAmount;
    u8 evsBits;

    CreateMon(mon, species, level, fixedIV, 0, 0, OT_ID_PLAYER_ID, 0);

    evsBits = evSpread;

    for (i = 0; i < NUM_STATS; i++) {
        if (evsBits & 1) statCount++;
        evsBits >>= 1;
    }

    evAmount = MAX_TOTAL_EVS / statCount;

    evsBits = 1;

    for (i = 0; i < NUM_STATS; i++) {
        if (evSpread & evsBits) SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);
        evsBits <<= 1;
    }

    CalculateMonStats(mon);
}

void CreateBattleTowerMon(struct Pokemon *mon, struct BattleTowerPokemon *src) {
    s32 i;
    u8 nickname[30];
    u8 language;
    u8 value;

    CreateMon(mon, src->species, src->level, 0, 1, src->personality, OT_ID_PRESET, src->otId);

    for (i = 0; i < MAX_MON_MOVES; i++) SetMonMoveSlot(mon, src->moves[i], i);

    SetMonData(mon, MON_DATA_PP_BONUSES, &src->ppBonuses);
    SetMonData(mon, MON_DATA_HELD_ITEM, &src->heldItem);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &src->friendship);

    StringCopy(nickname, src->nickname);

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN) {
        language = LANGUAGE_JAPANESE;
        StripExtCtrlCodes(nickname);
    } else {
        language = GAME_LANGUAGE;
    }

    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_NICKNAME, nickname);
    SetMonData(mon, MON_DATA_HP_EV, &src->hpEV);
    SetMonData(mon, MON_DATA_ATK_EV, &src->attackEV);
    SetMonData(mon, MON_DATA_DEF_EV, &src->defenseEV);
    SetMonData(mon, MON_DATA_SPEED_EV, &src->speedEV);
    SetMonData(mon, MON_DATA_SPATK_EV, &src->spAttackEV);
    SetMonData(mon, MON_DATA_SPDEF_EV, &src->spDefenseEV);
    value = src->abilityNum;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = src->hpIV;
    SetMonData(mon, MON_DATA_HP_IV, &value);
    value = src->attackIV;
    SetMonData(mon, MON_DATA_ATK_IV, &value);
    value = src->defenseIV;
    SetMonData(mon, MON_DATA_DEF_IV, &value);
    value = src->speedIV;
    SetMonData(mon, MON_DATA_SPEED_IV, &value);
    value = src->spAttackIV;
    SetMonData(mon, MON_DATA_SPATK_IV, &value);
    value = src->spDefenseIV;
    SetMonData(mon, MON_DATA_SPDEF_IV, &value);
    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void CreateBattleTowerMon2(struct Pokemon *mon, struct BattleTowerPokemon *src, bool8 lvl50) {
    s32 i;
    u8 nickname[30];
    u8 level;
    u8 language;
    u8 value;

    if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_50)
        level = GetFrontierEnemyMonLevel(gSaveBlock2Ptr->frontier.lvlMode);
    else if (lvl50)
        level = 50;
    else
        level = src->level;

    CreateMon(mon, src->species, level, 0, 1, src->personality, OT_ID_PRESET, src->otId);

    for (i = 0; i < MAX_MON_MOVES; i++) SetMonMoveSlot(mon, src->moves[i], i);

    SetMonData(mon, MON_DATA_PP_BONUSES, &src->ppBonuses);
    SetMonData(mon, MON_DATA_HELD_ITEM, &src->heldItem);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &src->friendship);

    StringCopy(nickname, src->nickname);

    if (nickname[0] == EXT_CTRL_CODE_BEGIN && nickname[1] == EXT_CTRL_CODE_JPN) {
        language = LANGUAGE_JAPANESE;
        StripExtCtrlCodes(nickname);
    } else {
        language = GAME_LANGUAGE;
    }

    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_NICKNAME, nickname);
    SetMonData(mon, MON_DATA_HP_EV, &src->hpEV);
    SetMonData(mon, MON_DATA_ATK_EV, &src->attackEV);
    SetMonData(mon, MON_DATA_DEF_EV, &src->defenseEV);
    SetMonData(mon, MON_DATA_SPEED_EV, &src->speedEV);
    SetMonData(mon, MON_DATA_SPATK_EV, &src->spAttackEV);
    SetMonData(mon, MON_DATA_SPDEF_EV, &src->spDefenseEV);
    value = src->abilityNum;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = src->hpIV;
    SetMonData(mon, MON_DATA_HP_IV, &value);
    value = src->attackIV;
    SetMonData(mon, MON_DATA_ATK_IV, &value);
    value = src->defenseIV;
    SetMonData(mon, MON_DATA_DEF_IV, &value);
    value = src->speedIV;
    SetMonData(mon, MON_DATA_SPEED_IV, &value);
    value = src->spAttackIV;
    SetMonData(mon, MON_DATA_SPATK_IV, &value);
    value = src->spDefenseIV;
    SetMonData(mon, MON_DATA_SPDEF_IV, &value);
    SetMonData(mon, MON_DATA_HP_TYPE, &src->hpType);
    MonRestorePP(mon);
    CalculateMonStats(mon);
}

void CreateApprenticeMon(struct Pokemon *mon, const struct Apprentice *src, u8 monId) {
    s32 i;
    u16 evAmount;
    u8 language;
    u32 otId = gApprentices[src->id].otId;
    u32 personality = ((gApprentices[src->id].otId >> 8) | ((gApprentices[src->id].otId & 0xFF) << 8)) + src->party[monId].species + src->number;

    CreateMon(mon, src->party[monId].species, GetFrontierEnemyMonLevel(src->lvlMode - 1), MAX_PER_STAT_IVS, TRUE, personality, OT_ID_PRESET, otId);

    SetMonData(mon, MON_DATA_HELD_ITEM, &src->party[monId].item);
    for (i = 0; i < MAX_MON_MOVES; i++) SetMonMoveSlot(mon, src->party[monId].moves[i], i);

    evAmount = MAX_TOTAL_EVS / NUM_STATS;
    for (i = 0; i < NUM_STATS; i++) SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);

    language = src->language;
    SetMonData(mon, MON_DATA_LANGUAGE, &language);
    SetMonData(mon, MON_DATA_OT_NAME, GetApprenticeNameInLanguage(src->id, language));
    CalculateMonStats(mon);
}

void CreateMonWithEVSpreadNatureOTID(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 nature, u8 fixedIV, u8 evSpread, u32 otId, u16 abilityNum) {
    s32 i;
    s32 statCount = 0;
    u8 evsBits;
    u16 evAmount;

    // i is reused as personality value
    i = Random32();

    CreateMon(mon, species, level, fixedIV, TRUE, i, OT_ID_PRESET, otId);
    evsBits = evSpread;
    for (i = 0; i < NUM_STATS; i++) {
        if (evsBits & 1) statCount++;
        evsBits >>= 1;
    }

    evAmount = MAX_TOTAL_EVS / statCount;
    evsBits = 1;
    for (i = 0; i < NUM_STATS; i++) {
        if (evSpread & evsBits) SetMonData(mon, MON_DATA_HP_EV + i, &evAmount);
        evsBits <<= 1;
    }
    SetMonData(mon, MON_DATA_NATURE, &nature);
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilityNum);
    CalculateMonStats(mon);
}

void ConvertPokemonToBattleTowerPokemon(struct Pokemon *mon, struct BattleTowerPokemon *dest) {
    s32 i;
    u16 heldItem;

    dest->species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);

    if (heldItem == ITEM_ENIGMA_BERRY) heldItem = ITEM_NONE;

    dest->heldItem = heldItem;

    for (i = 0; i < MAX_MON_MOVES; i++) dest->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);

    dest->level = GetMonData(mon, MON_DATA_LEVEL, NULL);
    dest->ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    dest->otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
    dest->hpEV = GetMonData(mon, MON_DATA_HP_EV, NULL);
    dest->attackEV = GetMonData(mon, MON_DATA_ATK_EV, NULL);
    dest->defenseEV = GetMonData(mon, MON_DATA_DEF_EV, NULL);
    dest->speedEV = GetMonData(mon, MON_DATA_SPEED_EV, NULL);
    dest->spAttackEV = GetMonData(mon, MON_DATA_SPATK_EV, NULL);
    dest->spDefenseEV = GetMonData(mon, MON_DATA_SPDEF_EV, NULL);
    dest->friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
    dest->hpIV = GetMonData(mon, MON_DATA_HP_IV, NULL);
    dest->attackIV = GetMonData(mon, MON_DATA_ATK_IV, NULL);
    dest->defenseIV = GetMonData(mon, MON_DATA_DEF_IV, NULL);
    dest->speedIV = GetMonData(mon, MON_DATA_SPEED_IV, NULL);
    dest->spAttackIV = GetMonData(mon, MON_DATA_SPATK_IV, NULL);
    dest->spDefenseIV = GetMonData(mon, MON_DATA_SPDEF_IV, NULL);
    dest->abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);
    dest->personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    dest->hpType = GetMonData(mon, MON_DATA_HP_TYPE, NULL);
    GetMonData(mon, MON_DATA_NICKNAME, dest->nickname);
}

void CreateEventLegalMon(
    struct Pokemon *mon, SpeciesEnum species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId) {
    bool32 isEventLegal = TRUE;

    CreateMon(mon, species, level, fixedIV, hasFixedPersonality, fixedPersonality, otIdType, fixedOtId);
    SetMonData(mon, MON_DATA_EVENT_LEGAL, &isEventLegal);
}

// If FALSE, should load this game's Deoxys form. If TRUE, should load normal Deoxys form
bool8 ShouldIgnoreDeoxysForm(u8 caseId, u8 battlerId) {
    switch (caseId) {
        case 0:
        default:
            return FALSE;
        case 1:  // Player's side in battle
            if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI)) return FALSE;
            if (!gMain.inBattle) return FALSE;
            if (gLinkPlayers[GetMultiplayerId()].id == battlerId) return FALSE;
            break;
        case 2:
            break;
        case 3:  // Summary Screen
            if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI)) return FALSE;
            if (!gMain.inBattle) return FALSE;
            if (battlerId == 1 || battlerId == 4 || battlerId == 5) return TRUE;
            return FALSE;
        case 4:
            break;
        case 5:  // In move animation, e.g. in Role Play or Snatch
            if (gBattleTypeFlags & BATTLE_TYPE_LINK) {
                if (!gMain.inBattle) return FALSE;
                if (gBattleTypeFlags & BATTLE_TYPE_MULTI) {
                    if (gLinkPlayers[GetMultiplayerId()].id == battlerId) return FALSE;
                } else {
                    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER) return FALSE;
                }
            } else {
                if (!gMain.inBattle) return FALSE;
                if (GetBattlerSide(battlerId) == B_SIDE_PLAYER) return FALSE;
            }
            break;
    }

    return TRUE;
}

u16 GetUnionRoomTrainerPic(void) {
    u8 linkId;
    u32 arrId;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        linkId = gRecordedBattleMultiplayerId ^ 1;
    else
        linkId = GetMultiplayerId() ^ 1;

    arrId = gLinkPlayers[linkId].trainerId & 7;
    arrId |= gLinkPlayers[linkId].gender << 3;
    return FacilityClassToPicIndex(gLinkPlayerFacilityClasses[arrId]);
}

u16 GetUnionRoomTrainerClass(void) {
    u8 linkId;
    u32 arrId;

    if (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)
        linkId = gRecordedBattleMultiplayerId ^ 1;
    else
        linkId = GetMultiplayerId() ^ 1;

    arrId = gLinkPlayers[linkId].trainerId & 7;
    arrId |= gLinkPlayers[linkId].gender << 3;
    return gFacilityClassToTrainerClass[gLinkPlayerFacilityClasses[arrId]];
}

void CreateEventLegalEnemyMon(void) {
    s32 species = gSpecialVar_0x8004;
    s32 level = gSpecialVar_0x8005;
    s32 itemId = gSpecialVar_0x8006;

    ZeroEnemyPartyMons();
    CreateEventLegalMon(&gEnemyParty[0], species, level, USE_RANDOM_IVS, 0, 0, 0, 0);
    if (itemId) {
        u8 heldItem[2];
        heldItem[0] = itemId;
        heldItem[1] = itemId >> 8;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, heldItem);
    }
}

#define CALC_STAT(base, iv, ev, statIndex, field)                   \
    {                                                               \
        u8 baseStat = gBaseStats[species].base;                     \
        s32 n = (((2 * baseStat + iv + ev / 4) * level) / 100) + 5; \
        u8 nature = GetMonData(mon, MON_DATA_NATURE);               \
        n = ModifyStatByNature(nature, n, statIndex);               \
        SetMonData(mon, field, &n);                                 \
    }

void CalculateMonStatsWithoutRestoringPP(struct Pokemon *mon){
    CalculateMonStatsMaster(mon, FALSE, FALSE);
}

void CalculateMonStats(struct Pokemon *mon)
{
    CalculateMonStatsMaster(mon, TRUE, FALSE);
}

void CalculateEnemyTrainerMonStats(struct Pokemon *mon)
{
    CalculateMonStatsMaster(mon, TRUE, TRUE);
}

void CalculateMonStatsMaster(struct Pokemon *mon, bool8 shouldRestorePP, bool8 isEnemyMon){
    s32 newMaxHP, movePP, i;
    s32 hpIV, attackIV, defenseIV, speedIV, spAttackIV, spDefenseIV;

    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    s32 level = GetLevelFromMonExp(mon);

    s32 oldMaxHP    = GetMonData(mon, MON_DATA_MAX_HP, NULL);
    s32 currentHP   = GetMonData(mon, MON_DATA_HP, NULL);
    s32 hpEV        = GetMonData(mon, MON_DATA_HP_EV, NULL);
    s32 attackEV    = GetMonData(mon, MON_DATA_ATK_EV, NULL);
    s32 defenseEV   = GetMonData(mon, MON_DATA_DEF_EV, NULL);
    s32 speedEV     = GetMonData(mon, MON_DATA_SPEED_EV, NULL);
    s32 spAttackEV  = GetMonData(mon, MON_DATA_SPATK_EV, NULL);
    s32 spDefenseEV = GetMonData(mon, MON_DATA_SPDEF_EV, NULL);
    bool8 speedDown = GetMonData(mon, MON_DATA_SPEED_DOWN, NULL);
    u8 difficulty   = gSaveBlock2Ptr->gameDifficulty;

    SetMonData(mon, MON_DATA_LEVEL, &level);

    if (isEnemyMon || difficulty != DIFFICULTY_HELL || !FlagGet(HELL_MODE_0_IVS_FLAG)) // Ivs are maxed normally
        hpIV = attackIV = defenseIV = spAttackIV = spDefenseIV = speedIV = MAX_IVS;
    else
        hpIV = attackIV = defenseIV = spAttackIV = spDefenseIV = speedIV = 0;

    if (speedDown)
        speedIV = 0;

    if (!gSaveBlock2Ptr->enableEvs) //Evs are Disabled
        hpEV = attackEV = defenseEV = spAttackEV = spDefenseEV = speedEV = 0;

    if (species == SPECIES_SHEDINJA || species == SPECIES_SHEDINJA_MEGA) {
        newMaxHP = 1;
    }
    else
    {
        s32 n = 2 * gBaseStats[species].baseHP + hpIV;
        newMaxHP = (((n + hpEV / 4) * level) / 100) + level + 10;
    }

    gBattleScripting.levelUpHP = newMaxHP - oldMaxHP;
    if (gBattleScripting.levelUpHP == 0)
        gBattleScripting.levelUpHP = 1;

    SetMonData(mon, MON_DATA_MAX_HP, &newMaxHP);

    // Set move PP
    if(shouldRestorePP){
        for (i = 0; i < MAX_MON_MOVES; i++) {
            movePP = gBattleMoves[GetMonData(mon, MON_DATA_MOVE1 + i, NULL)].pp;
            SetMonData(mon, MON_DATA_PP1 + i, &movePP);
        }
    }

    MonRestorePP(mon);

    CALC_STAT(baseAttack,    attackIV,    attackEV,    STAT_ATK,   MON_DATA_ATK)
    CALC_STAT(baseDefense,   defenseIV,   defenseEV,   STAT_DEF,   MON_DATA_DEF)
    CALC_STAT(baseSpeed,     speedIV,     speedEV,     STAT_SPEED, MON_DATA_SPEED)
    CALC_STAT(baseSpAttack,  spAttackIV,  spAttackEV,  STAT_SPATK, MON_DATA_SPATK)
    CALC_STAT(baseSpDefense, spDefenseIV, spDefenseEV, STAT_SPDEF, MON_DATA_SPDEF)

    if (species == SPECIES_SHEDINJA) 
    {
        if (currentHP != 0 || oldMaxHP == 0)
            currentHP = 1;
        else
            return;
    } 
    else
    {
        if (currentHP == 0 && oldMaxHP == 0)
            currentHP = newMaxHP;
        else if (currentHP != 0) {
            currentHP += newMaxHP - oldMaxHP;

            if (currentHP <= 0) 
                currentHP = 1;
        }
        else
            return;
    }

    SetMonData(mon, MON_DATA_HP, &currentHP);
}

void BoxMonToMon(const struct BoxPokemon *src, struct Pokemon *dest) {
    u32 value = 0;
    dest->box = *src;
    SetMonData(dest, MON_DATA_STATUS, &value);
    SetMonData(dest, MON_DATA_HP, &value);
    SetMonData(dest, MON_DATA_MAX_HP, &value);
    value = 255;
    SetMonData(dest, MON_DATA_MAIL, &value);
    CalculateMonStats(dest);
}

u8 GetLevelFromMonExp(struct Pokemon *mon) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u32 exp = GetMonData(mon, MON_DATA_EXP, NULL);
    s32 level = 1;

    while (level <= MAX_LEVEL && gExperienceTables[gBaseStats[species].growthRate][level] <= exp) level++;

    return level - 1;
}

u8 GetLevelFromBoxMonExp(struct BoxPokemon *boxMon) {
    SpeciesEnum species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 exp = GetBoxMonData(boxMon, MON_DATA_EXP, NULL);
    s32 level = 1;

    while (level <= MAX_LEVEL && gExperienceTables[gBaseStats[species].growthRate][level] <= exp) level++;

    return level - 1;
}

u16 GiveMoveToMon(struct Pokemon *mon, MoveEnum move) {
    s32 i;
    struct BoxPokemon *boxMon = &mon->box;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        MoveEnum existingMove = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, NULL);
        if (!existingMove) {
            SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &move);
            SetMonData(mon, MON_DATA_PP1 + i, &gBattleMoves[move].pp);
            return move;
        }
        if (existingMove == move) return MON_ALREADY_KNOWS_MOVE;
    }
    return MON_HAS_MAX_MOVES;
}

u16 GiveMoveToBoxMon(struct BoxPokemon *boxMon, MoveEnum move) {
    s32 i;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        MoveEnum existingMove = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, NULL);
        if (existingMove == MOVE_NONE) {
            SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &move);
            return move;
        }
        if (existingMove == move) return MON_ALREADY_KNOWS_MOVE;
    }
    return MON_HAS_MAX_MOVES;
}

u16 GiveMoveToBattleMon(struct BattlePokemon *mon, MoveEnum move) {
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (!mon->moves[i]) {
            mon->moves[i] = move;
            mon->pp[i] = gBattleMoves[move].pp;
            return move;
        }
    }

    return 0xFFFF;
}

void SetMonMoveSlot(struct Pokemon *mon, MoveEnum move, u8 slot) {
    SetMonData(mon, MON_DATA_MOVE1 + slot, &move);
    SetMonData(mon, MON_DATA_PP1 + slot, &gBattleMoves[move].pp);
}

void SetBattleMonMoveSlot(struct BattlePokemon *mon, MoveEnum move, u8 slot) {
    mon->moves[slot] = move;
    mon->pp[slot] = gBattleMoves[move].pp;
}

void GiveMonInitialMoveset(struct Pokemon *mon) { GiveBoxMonInitialMoveset(&mon->box); }

void GiveBoxMonInitialMoveset(struct BoxPokemon *boxMon) {
    SpeciesEnum species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    s32 level = GetLevelFromBoxMonExp(boxMon);
    u32 personality = GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL);
    s32 i;

    for (i = 0; gLevelUpLearnsets[species][i].move != LEVEL_UP_END; i++) {
        if (gLevelUpLearnsets[species][i].level > level) break;

        if (GiveMoveToBoxMon(boxMon, RandomizeMoves(gLevelUpLearnsets[species][i].move, species, personality)) == MON_HAS_MAX_MOVES)
            DeleteFirstMoveAndGiveMoveToBoxMon(boxMon, RandomizeMoves(gLevelUpLearnsets[species][i].move, species, personality));
    }
}

u16 MonTryLearningNewMove(struct Pokemon *mon, bool8 firstMove) {
    u32 retVal = MOVE_NONE;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, NULL);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);

    // since you can learn more than one move per level
    // the game needs to know whether you decided to
    // learn it or keep the old set to avoid asking
    // you to learn the same move over and over again
    if (firstMove) {
        sLearningMoveTableID = 0;

        while (gLevelUpLearnsets[species][sLearningMoveTableID].level != level) {
            sLearningMoveTableID++;
            if (gLevelUpLearnsets[species][sLearningMoveTableID].move == LEVEL_UP_END) return MOVE_NONE;
        }
    }

    if (gLevelUpLearnsets[species][sLearningMoveTableID].level == level) {
        gMoveToLearn = RandomizeMoves(gLevelUpLearnsets[species][sLearningMoveTableID].move, species, personality);
        sLearningMoveTableID++;
        retVal = GiveMoveToMon(mon, gMoveToLearn);
    }

    return retVal;
}

u16 MonTryLearningNewEvolutionMove(struct Pokemon *mon, bool8 firstMove) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, NULL);

    // since you can learn more than one move per level
    // the game needs to know whether you decided to
    // learn it or keep the old set to avoid asking
    // you to learn the same move over and over again
    if (firstMove) {
        sLearningMoveTableID = 0;
    }
    while (gLevelUpLearnsets[species][sLearningMoveTableID].move != LEVEL_UP_END) {
        u16 moveLevel;
        moveLevel = (gLevelUpLearnsets[species][sLearningMoveTableID].level);
        while (moveLevel == 0 || moveLevel == level) {
            gMoveToLearn = (gLevelUpLearnsets[species][sLearningMoveTableID].move);
            sLearningMoveTableID++;
            return GiveMoveToMon(mon, gMoveToLearn);
        }
        sLearningMoveTableID++;
    }
    return 0;
}

void DeleteFirstMoveAndGiveMoveToMon(struct Pokemon *mon, MoveEnum move) {
    s32 i;
    MoveEnum moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;

    for (i = 0; i < MAX_MON_MOVES - 1; i++) {
        moves[i] = GetMonData(mon, MON_DATA_MOVE2 + i, NULL);
        pp[i] = GetMonData(mon, MON_DATA_PP2 + i, NULL);
    }

    ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses >>= 2;
    moves[3] = move;
    pp[3] = gBattleMoves[move].pp;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &pp[i]);
    }

    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void DeleteFirstMoveAndGiveMoveToBoxMon(struct BoxPokemon *boxMon, MoveEnum move) {
    s32 i;
    MoveEnum moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 ppBonuses;

    for (i = 0; i < MAX_MON_MOVES - 1; i++) moves[i] = GetBoxMonData(boxMon, MON_DATA_MOVE2 + i, NULL);

    ppBonuses = GetBoxMonData(boxMon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses >>= 2;
    moves[3] = move;
    pp[3] = gBattleMoves[move].pp;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &moves[i]);
        SetBoxMonData(boxMon, MON_DATA_PP1 + i, &pp[i]);
    }

    SetBoxMonData(boxMon, MON_DATA_PP_BONUSES, &ppBonuses);
}

u8 CountAliveMonsInBattle(u8 caseId) {
    s32 i;
    u8 retVal = 0;

    switch (caseId) {
        case BATTLE_ALIVE_EXCEPT_ACTIVE:
            for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
                if (i != gActiveBattler && !(gAbsentBattlerFlags & gBitTable[i])) retVal++;
            }
            break;
        case BATTLE_ALIVE_ATK_SIDE:
            for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
                if (GetBattlerSide(i) == GetBattlerSide(gBattlerAttacker) && !(gAbsentBattlerFlags & gBitTable[i])) retVal++;
            }
            break;
        case BATTLE_ALIVE_DEF_SIDE:
            for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
                if (GetBattlerSide(i) == GetBattlerSide(gBattlerTarget) && !(gAbsentBattlerFlags & gBitTable[i])) retVal++;
            }
            break;
    }

    return retVal;
}

u8 GetDefaultMoveTarget(u8 battlerId) {
    u8 opposing = BATTLE_OPPOSITE(GetBattlerPosition(battlerId) & BIT_SIDE);

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE)) return GetBattlerAtPosition(opposing);
    if (CountAliveMonsInBattle(BATTLE_ALIVE_EXCEPT_ACTIVE) > 1) {
        u8 position;

        if ((Random() & 1) == 0)
            position = BATTLE_PARTNER(opposing);
        else
            position = opposing;

        return GetBattlerAtPosition(position);
    } else {
        if ((gAbsentBattlerFlags & gBitTable[opposing]))
            return GetBattlerAtPosition(BATTLE_PARTNER(opposing));
        else
            return GetBattlerAtPosition(opposing);
    }
}

u8 GetMonGender(struct Pokemon *mon) { return GetBoxMonGender(&mon->box); }

u8 GetBoxMonGender(struct BoxPokemon *boxMon) {
    SpeciesEnum species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u32 personality = GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL);

    return GetGenderFromSpeciesAndPersonality(species, personality);
}

u8 GetGenderFromSpeciesAndPersonality(SpeciesEnum species, u32 personality) {
    switch (gBaseStats[species].genderRatio) {
        case MON_MALE:
        case MON_FEMALE:
        case MON_GENDERLESS:
            return gBaseStats[species].genderRatio;
    }

    if (gBaseStats[species].genderRatio > ((personality + (GET_BASE_SPECIES_ID(species) == SPECIES_SYLVEON)) & 0xFF))
        return MON_FEMALE;
    else
        return MON_MALE;
}

u32 GetUnownSpeciesId(u32 personality) {
    u16 unownLetter = GetUnownLetterByPersonality(personality);

    if (unownLetter == 0) return SPECIES_UNOWN;
    return unownLetter + SPECIES_UNOWN_B - 1;
}

void SetMultiuseSpriteTemplateToPokemon(u16 speciesTag, u8 battlerPosition) {
    if (gMonSpritesGfxPtr != NULL)
        gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
    else if (gUnknown_020249B4[0])
        gMultiuseSpriteTemplate = gUnknown_020249B4[0]->templates[battlerPosition];
    else if (gUnknown_020249B4[1])
        gMultiuseSpriteTemplate = gUnknown_020249B4[1]->templates[battlerPosition];
    else
        gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];

    gMultiuseSpriteTemplate.paletteTag = speciesTag;
    if (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_PLAYER_RIGHT)
        gMultiuseSpriteTemplate.anims = gAnims_MonPic;
    else if (speciesTag > SPECIES_SHINY_TAG)
        gMultiuseSpriteTemplate.anims = gMonFrontAnimsPtrTable[speciesTag - SPECIES_SHINY_TAG];
    else
        gMultiuseSpriteTemplate.anims = gMonFrontAnimsPtrTable[speciesTag];
}

void SetMultiuseSpriteTemplateToTrainerBack(u16 trainerSpriteId, u8 battlerPosition) {
    gMultiuseSpriteTemplate.paletteTag = trainerSpriteId;
    if (battlerPosition == B_POSITION_PLAYER_LEFT || battlerPosition == B_POSITION_PLAYER_RIGHT) {
        gMultiuseSpriteTemplate = sTrainerBackSpriteTemplates[trainerSpriteId];
        gMultiuseSpriteTemplate.anims = gTrainerBackAnimsPtrTable[trainerSpriteId];
    } else {
        if (gMonSpritesGfxPtr != NULL)
            gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
        else
            gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];
        gMultiuseSpriteTemplate.anims = gTrainerFrontAnimsPtrTable[trainerSpriteId];
    }
}

void SetMultiuseSpriteTemplateToTrainerFront(u16 arg0, u8 battlerPosition) {
    if (gMonSpritesGfxPtr != NULL)
        gMultiuseSpriteTemplate = gMonSpritesGfxPtr->templates[battlerPosition];
    else
        gMultiuseSpriteTemplate = gBattlerSpriteTemplates[battlerPosition];

    gMultiuseSpriteTemplate.paletteTag = arg0;
    gMultiuseSpriteTemplate.anims = gTrainerFrontAnimsPtrTable[arg0];
}

u32 GetMonDataInternal(struct Pokemon *mon, s32 field, u8 *data) {
    u32 ret;

    switch (field) {
        case MON_DATA_STATUS:
            ret = mon->status;
            break;
        case MON_DATA_LEVEL:
            ret = mon->level;
            break;
        case MON_DATA_HP:
            ret = mon->hp;
            break;
        case MON_DATA_MAX_HP:
            ret = mon->maxHP;
            break;
        case MON_DATA_ATK:
            ret = mon->attack;
            break;
        case MON_DATA_DEF:
            ret = mon->defense;
            break;
        case MON_DATA_SPEED:
            ret = mon->speed;
            break;
        case MON_DATA_SPATK:
            ret = mon->spAttack;
            break;
        case MON_DATA_SPDEF:
            ret = mon->spDefense;
            break;
        case MON_DATA_ATK2:
            ret = mon->attack;
            break;
        case MON_DATA_DEF2:
            ret = mon->defense;
            break;
        case MON_DATA_SPEED2:
            ret = mon->speed;
            break;
        case MON_DATA_SPATK2:
            ret = mon->spAttack;
            break;
        case MON_DATA_SPDEF2:
            ret = mon->spDefense;
            break;
        case MON_DATA_MAIL:
            ret = mon->mail;
            break;
        case MON_DATA_PP1:
        case MON_DATA_PP2:
        case MON_DATA_PP3:
        case MON_DATA_PP4:
            ret = mon->pp[field - MON_DATA_PP1];
            break;
        default:
            ret = GetBoxMonData(&mon->box, field, data);
            break;
    }
    return ret;
}

bool8 CheckBoxMonForBadChecksum(u8 box, u8 slot) { return FALSE; }

u32 GetBoxMonDataInternal(struct BoxPokemon *boxMon, s32 field, u8 *data) {
    u32 retVal = 0;

    switch (field) {
        case MON_DATA_PERSONALITY:
            retVal = boxMon->personality;
            break;
        case MON_DATA_OT_ID:
            retVal = boxMon->otId;
            break;
        case MON_DATA_NICKNAME:
            REQUIRE(data) {
                if (boxMon->isEgg) {
                    StringCopy(data, gText_EggNickname);
                    retVal = StringLength(data);
                } else {
                    for (retVal = 0; retVal < POKEMON_NAME_LENGTH; data[retVal] = boxMon->nickname[retVal], retVal++) {
                    }

                    data[retVal] = EOS;
                }
                break;
            }
        case MON_DATA_LANGUAGE:
            retVal = LANGUAGE_ENGLISH;
            break;
        case MON_DATA_SANITY_IS_BAD_EGG:
            retVal = 0;
            break;
        case MON_DATA_SANITY_HAS_SPECIES:
            retVal = (boxMon->species);
            break;
        case MON_DATA_SANITY_IS_EGG:
            retVal = boxMon->isEgg;
            break;
        case MON_DATA_OT_NAME:
            REQUIRE(data) {
                retVal = 0;

                while (retVal < PLAYER_NAME_LENGTH) {
                    data[retVal] = boxMon->otName[retVal];
                    retVal++;
                }

                data[retVal] = EOS;
                break;
            }
        case MON_DATA_MARKINGS:
            retVal = boxMon->markings;
            break;
        case MON_DATA_SPECIES:
            retVal = boxMon->species;
            break;
        case MON_DATA_HELD_ITEM:
            retVal = boxMon->heldItem;
            break;
        case MON_DATA_EXP:
            retVal = boxMon->experience;
            break;
        case MON_DATA_PP_BONUSES:
            retVal = 255;
            break;
        case MON_DATA_FRIENDSHIP:
            retVal = boxMon->friendship;
            break;
        case MON_DATA_MOVE1:
            retVal = boxMon->move1;
            break;
        case MON_DATA_MOVE2:
            retVal = boxMon->move2;
            break;
        case MON_DATA_MOVE3:
            retVal = boxMon->move3;
            break;
        case MON_DATA_MOVE4:
            retVal = boxMon->move4;
            break;
        case MON_DATA_HP_EV:
            retVal = boxMon->hpEV;
            break;
        case MON_DATA_ATK_EV:
            retVal = boxMon->attackEV;
            break;
        case MON_DATA_DEF_EV:
            retVal = boxMon->defenseEV;
            break;
        case MON_DATA_SPEED_EV:
            retVal = boxMon->speedEV;
            break;
        case MON_DATA_SPATK_EV:
            retVal = boxMon->spAttackEV;
            break;
        case MON_DATA_SPDEF_EV:
            retVal = boxMon->spDefenseEV;
            break;
        case MON_DATA_COOL:
        case MON_DATA_BEAUTY:
        case MON_DATA_CUTE:
        case MON_DATA_SMART:
        case MON_DATA_TOUGH:
        case MON_DATA_SHEEN:
            retVal = MAX_BEAUTY;
            break;
        case MON_DATA_POKERUS:
            retVal = FALSE;
            break;
        case MON_DATA_MET_LOCATION:
            retVal = boxMon->metLocation;
            break;
        case MON_DATA_MET_LEVEL:
            retVal = boxMon->metLevel;
            break;
        case MON_DATA_MET_GAME:
            retVal = VERSION_EMERALD;
            break;
        case MON_DATA_POKEBALL:
            retVal = boxMon->pokeball;
            break;
        case MON_DATA_OT_GENDER:
            retVal = boxMon->otGender;
            break;
        case MON_DATA_HP_IV:
        case MON_DATA_ATK_IV:
        case MON_DATA_DEF_IV:
        case MON_DATA_SPATK_IV:
        case MON_DATA_SPDEF_IV:
            retVal = MAX_IVS;
            break;
        case MON_DATA_SPEED_IV:
            retVal = boxMon->speedDown ? 0 : MAX_IVS;
            break;
        case MON_DATA_IS_EGG:
            retVal = boxMon->isEgg;
            break;
        case MON_DATA_IS_ALPHA:
            retVal = boxMon->isAlpha;
            break;
        case MON_DATA_HP_TYPE:
            retVal = boxMon->hpType;
            break;
        case MON_DATA_IS_SHINY:
            retVal = boxMon->isShiny;
            break;
        case MON_DATA_MAX_SHINY:
            retVal = boxMon->maxShiny;
            break;
        case MON_DATA_SPEED_DOWN:
            retVal = boxMon->speedDown;
            break;
        case MON_DATA_ABILITY_NUM:
            retVal = boxMon->abilityNum;
            break;
        case MON_DATA_COOL_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->coolRibbon;
#endif
            break;
        case MON_DATA_BEAUTY_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->beautyRibbon;
#endif
            break;
        case MON_DATA_CUTE_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->cuteRibbon;
#endif
            break;
        case MON_DATA_SMART_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->smartRibbon;
#endif
            break;
        case MON_DATA_TOUGH_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->toughRibbon;
#endif
            break;
        case MON_DATA_CHAMPION_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->championRibbon;
#endif
            break;
        case MON_DATA_WINNING_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->winningRibbon;
#endif
            break;
        case MON_DATA_VICTORY_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->victoryRibbon;
#endif
            break;
        case MON_DATA_ARTIST_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->artistRibbon;
#endif
            break;
        case MON_DATA_EFFORT_RIBBON:
#ifdef REMOVE_RIBBONS
            retVal = FALSE;
#else
            retVal = boxMon->effortRibbon;
#endif
            break;
        case MON_DATA_IS_EVENT_MON:
            retVal = boxMon->isEventMon;
            break;
        case MON_DATA_EVENT_LEGAL:
            retVal = EVENT_LEGAL;
            break;
        case MON_DATA_SPECIES2:
            retVal = boxMon->species;
            if (boxMon->species && boxMon->isEgg) retVal = SPECIES_EGG;
            break;
        case MON_DATA_IVS:
            retVal = MAX_IVS;
            break;
        case MON_DATA_NATURE:
            retVal = boxMon->nature;
            break;
        case MON_DATA_KNOWN_MOVES:
            if (boxMon->species && !boxMon->isEgg) {
                u16 *moves = (u16 *)data;
                s32 i = 0;

                while (moves[i] != MOVES_COUNT) {
                    u16 move = moves[i];
                    if (boxMon->move1 == move || boxMon->move2 == move || boxMon->move3 == move || boxMon->move4 == move) retVal |= gBitTable[i];
                    i++;
                }
            }
            break;
        case MON_DATA_RIBBON_COUNT:
#ifdef REMOVE_RIBBONS
            retVal = 0;
#else
            retVal = 0;
            if (boxMon->species && !boxMon->isEgg) {
                retVal += boxMon->coolRibbon;
                retVal += boxMon->beautyRibbon;
                retVal += boxMon->cuteRibbon;
                retVal += boxMon->smartRibbon;
                retVal += boxMon->toughRibbon;
                retVal += boxMon->championRibbon;
                retVal += boxMon->winningRibbon;
                retVal += boxMon->victoryRibbon;
                retVal += boxMon->artistRibbon;
                retVal += boxMon->effortRibbon;
            }
#endif
            break;
        case MON_DATA_RIBBONS:
#ifdef REMOVE_RIBBONS
            retVal = 0;
#else
            retVal = 0;
            if (boxMon->species && !boxMon->isEgg) {
                retVal = boxMon->championRibbon | (boxMon->coolRibbon << 1) | (boxMon->beautyRibbon << 4) | (boxMon->cuteRibbon << 7) |
                         (boxMon->smartRibbon << 10) | (boxMon->toughRibbon << 13) | (boxMon->winningRibbon << 16) | (boxMon->victoryRibbon << 17) |
                         (boxMon->artistRibbon << 18) | (boxMon->effortRibbon << 19);
            }
#endif
            break;
        default:
            break;
    }

    return retVal;
}

#define SET8(lhs) (lhs) = *data
#define SET16(lhs) (lhs) = data[0] + (data[1] << 8)
#define SET32(lhs) (lhs) = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)

void SetMonData(struct Pokemon *mon, s32 field, const void *dataArg) {
    const u8 *data = dataArg;

    switch (field) {
        case MON_DATA_STATUS:
            SET32(mon->status);
            break;
        case MON_DATA_LEVEL:
            SET8(mon->level);
            break;
        case MON_DATA_HP:
            SET16(mon->hp);
            break;
        case MON_DATA_MAX_HP:
            SET16(mon->maxHP);
            break;
        case MON_DATA_ATK:
            SET16(mon->attack);
            break;
        case MON_DATA_DEF:
            SET16(mon->defense);
            break;
        case MON_DATA_SPEED:
            SET16(mon->speed);
            break;
        case MON_DATA_SPATK:
            SET16(mon->spAttack);
            break;
        case MON_DATA_SPDEF:
            SET16(mon->spDefense);
            break;
        case MON_DATA_MAIL:
            SET8(mon->mail);
            break;
        case MON_DATA_PP1:
        case MON_DATA_PP2:
        case MON_DATA_PP3:
        case MON_DATA_PP4:
            SET8(mon->pp[field - MON_DATA_PP1]);
            break;
        case MON_DATA_SPECIES2:
            break;
        default:
            SetBoxMonData(&mon->box, field, data);
            break;
    }
}

void SetBoxMonData(struct BoxPokemon *boxMon, s32 field, const void *dataArg) {
    const u8 *data = dataArg;

    switch (field) {
        case MON_DATA_PERSONALITY:
            SET32(boxMon->personality);
            break;
        case MON_DATA_OT_ID:
            SET32(boxMon->otId);
            break;
        case MON_DATA_NICKNAME: {
            s32 i;
            for (i = 0; i < POKEMON_NAME_LENGTH; i++) boxMon->nickname[i] = data[i];
            break;
        }
        case MON_DATA_LANGUAGE:
            SET8(boxMon->language);
            break;
        case MON_DATA_SANITY_IS_EGG:
            SET8(boxMon->isEgg);
            break;
        case MON_DATA_OT_NAME: {
            s32 i;
            for (i = 0; i < PLAYER_NAME_LENGTH; i++) boxMon->otName[i] = data[i];
            break;
        }
        case MON_DATA_MARKINGS:
#ifdef REMOVE_RIBBONS
            SET8(boxMon->markings);
#else
            SET32(boxMon->markings);
#endif
            break;
        case MON_DATA_SPECIES: {
            SET16(boxMon->species);
            break;
        }
        case MON_DATA_HELD_ITEM:
            SET16(boxMon->heldItem);
            break;
        case MON_DATA_EXP:
            SET32(boxMon->experience);
            break;
        case MON_DATA_FRIENDSHIP:
            SET8(boxMon->friendship);
            break;
        case MON_DATA_MOVE1:
            SET16(boxMon->move1);
            break;
        case MON_DATA_MOVE2:
            SET16(boxMon->move2);
            break;
        case MON_DATA_MOVE3:
            SET16(boxMon->move3);
            break;
        case MON_DATA_MOVE4:
            SET16(boxMon->move4);
            break;
        case MON_DATA_HP_EV:
            SET8(boxMon->hpEV);
            break;
        case MON_DATA_ATK_EV:
            SET8(boxMon->attackEV);
            break;
        case MON_DATA_DEF_EV:
            SET8(boxMon->defenseEV);
            break;
        case MON_DATA_SPEED_EV:
            SET8(boxMon->speedEV);
            break;
        case MON_DATA_SPATK_EV:
            SET8(boxMon->spAttackEV);
            break;
        case MON_DATA_SPDEF_EV:
            SET8(boxMon->spDefenseEV);
            break;
        case MON_DATA_COOL:
        case MON_DATA_BEAUTY:
        case MON_DATA_CUTE:
        case MON_DATA_SMART:
        case MON_DATA_TOUGH:
        case MON_DATA_SHEEN:
        case MON_DATA_PP_BONUSES:
        case MON_DATA_POKERUS:
        case MON_DATA_MET_GAME:
            break;
        case MON_DATA_MET_LOCATION:
            SET8(boxMon->metLocation);
            break;
        case MON_DATA_MET_LEVEL: {
            u8 metLevel = *data;
            boxMon->metLevel = metLevel;
            break;
        }
        case MON_DATA_POKEBALL: {
            u8 pokeball = *data;
            boxMon->pokeball = pokeball;
            break;
        }
        case MON_DATA_OT_GENDER:
            SET8(boxMon->otGender);
            break;
        case MON_DATA_IS_EGG:
            SET8(boxMon->isEgg);
            break;
        case MON_DATA_IS_ALPHA:
            SET8(boxMon->isAlpha);
            break;
        case MON_DATA_HP_TYPE:
            SET8(boxMon->hpType);
            break;
        case MON_DATA_IS_SHINY:
            SET8(boxMon->isShiny);
            break;
        case MON_DATA_MAX_SHINY:
            SET8(boxMon->maxShiny);
            break;
        case MON_DATA_SPEED_DOWN:
            SET8(boxMon->speedDown);
            break;
        case MON_DATA_ABILITY_NUM:
#ifdef REMOVE_RIBBONS
            SET8(boxMon->abilityNum);
#else
            SET32(boxMon->abilityNum);
#endif
            break;
        case MON_DATA_COOL_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->coolRibbon);
            break;
#endif
        case MON_DATA_BEAUTY_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->beautyRibbon);
            break;
#endif
        case MON_DATA_CUTE_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->cuteRibbon);
            break;
#endif
        case MON_DATA_SMART_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->smartRibbon);
            break;
#endif
        case MON_DATA_TOUGH_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->toughRibbon);
            break;
#endif
        case MON_DATA_CHAMPION_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->championRibbon);
            break;
#endif
        case MON_DATA_WINNING_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->winningRibbon);
            break;
#endif
        case MON_DATA_VICTORY_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->victoryRibbon);
            break;
#endif
        case MON_DATA_ARTIST_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->artistRibbon);
            break;
#endif
        case MON_DATA_EFFORT_RIBBON:
#ifdef REMOVE_RIBBONS
            break;
#else
            SET8(boxMon->effortRibbon);
            break;
#endif
        case MON_DATA_NATURE:
            SET8(boxMon->nature);
            break;
        case MON_DATA_IS_EVENT_MON:
            SET8(boxMon->isEventMon);
            break;
        case MON_DATA_HP_IV:
        case MON_DATA_ATK_IV:
        case MON_DATA_DEF_IV:
        case MON_DATA_SPEED_IV:
        case MON_DATA_SPATK_IV:
        case MON_DATA_SPDEF_IV:
        case MON_DATA_EVENT_LEGAL:
        case MON_DATA_IVS:
        default:
            break;
    }
}

void CopyMon(void *dest, void *src, size_t size) { memcpy(dest, src, size); }

u8 GiveMonToPlayer(struct Pokemon *mon) {
    s32 i;

    SetMonData(mon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetMonData(mon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);
    SetMonData(mon, MON_DATA_OT_ID, gSaveBlock2Ptr->playerTrainerId);

    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE) break;
    }

    if (!UsingBattlePyramidBag()) AddBagItem(mon->box.heldItem, 1);

    if (i >= PARTY_SIZE) return SendMonToPC(mon);

    CopyMon(&gPlayerParty[i], mon, sizeof(*mon));
    gPlayerPartyCount = i + 1;
    return MON_GIVEN_TO_PARTY;
}

u8 SendMonToPC(struct Pokemon *mon) {
    s32 boxNo, boxPos;

    SetPCBoxToSendMon(VarGet(VAR_PC_BOX_TO_SEND_MON));

    boxNo = StorageGetCurrentBox();

    do {
        for (boxPos = 0; boxPos < IN_BOX_COUNT; boxPos++) {
            struct BoxPokemon *checkingMon = GetBoxedMonPtr(boxNo, boxPos);
            if (GetBoxMonData(checkingMon, MON_DATA_SPECIES, NULL) == SPECIES_NONE) {
                MonRestorePP(mon);
                CopyMon(checkingMon, &mon->box, sizeof(mon->box));
                gSpecialVar_MonBoxId = boxNo;
                gSpecialVar_MonBoxPos = boxPos;
                if (GetPCBoxToSendMon() != boxNo) FlagClear(FLAG_SHOWN_BOX_WAS_FULL_MESSAGE);
                VarSet(VAR_PC_BOX_TO_SEND_MON, boxNo);
                return MON_GIVEN_TO_PC;
            }
        }

        boxNo++;
        if (boxNo == TOTAL_BOXES_COUNT) boxNo = 0;
    } while (boxNo != StorageGetCurrentBox());

    return MON_CANT_GIVE;
}

u8 SendSettingsMonToPC(struct Pokemon *mon) {
    s32 boxNo, boxPos, i, j;
    boxNo = TOTAL_BOXES_COUNT - 1;
    boxPos = IN_BOX_COUNT - 1;

    do {
        j = 0;
        for (i = 0; i < 1; i++) {
            struct BoxPokemon *checkingMon = GetBoxedMonPtr(boxNo, boxPos);
            if (GetBoxMonData(checkingMon, MON_DATA_SPECIES, NULL) == SPECIES_NONE) {
                MonRestorePP(mon);
                CopyMon(checkingMon, &mon->box, sizeof(mon->box));
                gSpecialVar_MonBoxId = boxNo;
                gSpecialVar_MonBoxPos = boxPos;
                VarSet(VAR_RESULT, MON_GIVEN_TO_PC);
                return MON_GIVEN_TO_PC;
            } else if (GetBoxMonData(checkingMon, MON_DATA_SPECIES, NULL) == SPECIES_RATTATA && GetBoxMonData(checkingMon, MON_DATA_EXP, NULL) == 0) {
                VarSet(VAR_RESULT, MON_GIVEN_TO_PC);
                return MON_GIVEN_TO_PC;
            }
        }
        j++;
    } while (j < 0);
    VarSet(VAR_RESULT, MON_CANT_GIVE);
    return MON_CANT_GIVE;
}

u8 CalculatePlayerPartyCount(void) {
    gPlayerPartyCount = 0;

    while (gPlayerPartyCount < PARTY_SIZE && GetMonData(&gPlayerParty[gPlayerPartyCount], MON_DATA_SPECIES, NULL) != SPECIES_NONE) {
        gPlayerPartyCount++;
    }

    return gPlayerPartyCount;
}

u8 CalculateEnemyPartyCount(void) {
    gEnemyPartyCount = 0;

    while (gEnemyPartyCount < PARTY_SIZE && GetMonData(&gEnemyParty[gEnemyPartyCount], MON_DATA_SPECIES, NULL) != SPECIES_NONE) {
        gEnemyPartyCount++;
    }

    return gEnemyPartyCount;
}

// Basically GetMonsStateToDoubles, but includes fainted Pokemon
u8 CalculatePlayerBattlerPartyCount(void) {
    s32 battlerCount = 0;
    s32 i;
    CalculatePlayerPartyCount();

    if (gPlayerPartyCount == 1) return gPlayerPartyCount;  // PLAYER_HAS_ONE_MON

    for (i = 0; i < gPlayerPartyCount; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_EGG && GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_NONE)
            battlerCount++;
    }

    return battlerCount;
}

u8 GetMonsStateToDoubles(void) {
    s32 aliveCount = 0;
    s32 i;
    CalculatePlayerPartyCount();

    if (gPlayerPartyCount == 1) return gPlayerPartyCount;  // PLAYER_HAS_ONE_MON

    if (!FlagGet(FLAG_SYS_DISABLE_AUTOHEAL)) HealPlayerParty();

    for (i = 0; i < gPlayerPartyCount; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_EGG && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0 &&
            GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_NONE)
            aliveCount++;
    }

    return (aliveCount > 1) ? PLAYER_HAS_TWO_USABLE_MONS : PLAYER_HAS_ONE_USABLE_MON;
}

u8 GetMonsStateToDoubles_2(void) {
    s32 aliveCount = 0;
    s32 i;

    HealPlayerParty();

    for (i = 0; i < PARTY_SIZE; i++) {
        u32 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
        if (species != SPECIES_EGG && species != SPECIES_NONE && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0) aliveCount++;
    }

    if (aliveCount == 1) return PLAYER_HAS_ONE_MON;  // may have more than one, but only one is alive

    return (aliveCount > 1) ? PLAYER_HAS_TWO_USABLE_MONS : PLAYER_HAS_ONE_USABLE_MON;
}

u16 GetAbilityBySpecies(SpeciesEnum species, u8 abilityNum) {
    AbilityEnum ability;
    if (abilityNum < NUM_ABILITY_SLOTS)
        ability = gBaseStats[species].abilities[abilityNum];
    else
        ability = gBaseStats[species].abilities[0];

    if (ability == ABILITY_NONE) ability = gBaseStats[species].abilities[0];

    return ability;
}

u16 GetMonAbility(struct Pokemon *mon) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);
    return GetAbilityBySpecies(species, abilityNum);
}

void CreateSecretBaseEnemyParty(struct SecretBase *secretBaseRecord) {
    s32 i, j;

    ZeroEnemyPartyMons();
    *gBattleResources->secretBase = *secretBaseRecord;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (gBattleResources->secretBase->party.species[i]) {
            CreateMon(&gEnemyParty[i],
                      gBattleResources->secretBase->party.species[i],
                      gBattleResources->secretBase->party.levels[i],
                      15,
                      1,
                      gBattleResources->secretBase->party.personality[i],
                      OT_ID_RANDOM_NO_SHINY,
                      0);

            SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBattleResources->secretBase->party.heldItems[i]);

            for (j = 0; j < NUM_STATS; j++) SetMonData(&gEnemyParty[i], MON_DATA_HP_EV + j, &gBattleResources->secretBase->party.EVs[i]);

            for (j = 0; j < MAX_MON_MOVES; j++) {
                SetMonData(&gEnemyParty[i], MON_DATA_MOVE1 + j, &gBattleResources->secretBase->party.moves[i * MAX_MON_MOVES + j]);
                SetMonData(&gEnemyParty[i], MON_DATA_PP1 + j, &gBattleMoves[gBattleResources->secretBase->party.moves[i * MAX_MON_MOVES + j]].pp);
            }
        }
    }
}

u8 GetSecretBaseTrainerPicIndex(void) {
    u8 facilityClass = sSecretBaseFacilityClasses[gBattleResources->secretBase->gender][gBattleResources->secretBase->trainerId[0] % 5];
    return gFacilityClassToPicIndex[facilityClass];
}

u8 GetSecretBaseTrainerClass(void) {
    u8 facilityClass = sSecretBaseFacilityClasses[gBattleResources->secretBase->gender][gBattleResources->secretBase->trainerId[0] % 5];
    return gFacilityClassToTrainerClass[facilityClass];
}

bool8 IsPlayerPartyAndPokemonStorageFull(void) {
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE) return FALSE;

    return IsPokemonStorageFull();
}

bool8 IsPokemonStorageFull(void) {
    s32 i, j;

    for (i = 0; i < TOTAL_BOXES_COUNT; i++)
        for (j = 0; j < IN_BOX_COUNT; j++)
            if (GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == SPECIES_NONE) return FALSE;

    return TRUE;
}

void GetSpeciesName(u8 *name, SpeciesEnum species) {
    s32 i;

    for (i = 0; i <= POKEMON_NAME_LENGTH; i++) {
        if (species > NUM_SPECIES)
            name[i] = gSpeciesNames[0][i];
        else
            name[i] = gSpeciesNames[species][i];

        if (name[i] == EOS) break;
    }

    name[i] = EOS;
}

#define HELL_MODE_REDUCED_PP_FOR_SLEEPING_MOVES 2
u8 CalculatePPWithBonusPlayer(u16 move, u8 ppBonuses, u8 moveIndex) {
    bool8 isHellMode = gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_HELL;
    u8 basePP = gBattleMoves[move].pp;

    if(isHellMode){
        //Sleep-inflicting moves have 2 PP (for the player)
        //Rest and Sleep Talk reduced to 2 PP (for the player)
        switch(gBattleMoves[move].effect){
            case EFFECT_SLEEP_HIT:
            case EFFECT_SLEEP:
            case EFFECT_SNORE:
            case EFFECT_REST:
            case EFFECT_SLEEP_TALK:
                return HELL_MODE_REDUCED_PP_FOR_SLEEPING_MOVES;
            break;
        }
        return basePP; //Player's moves are not PP Maxed (ie: Fire Blast PP is 5 instead of 8 for the player only)
    }

    return basePP + ((basePP * 20 * ((gPPUpGetMask[moveIndex] & ppBonuses) >> (2 * moveIndex))) / 100);
}

u8 CalculatePPWithBonus(u16 move, u8 ppBonuses, u8 moveIndex) {
    u8 basePP = gBattleMoves[move].pp;
    return basePP + ((basePP * 20 * ((gPPUpGetMask[moveIndex] & ppBonuses) >> (2 * moveIndex))) / 100);
}

void RemoveMonPPBonus(struct Pokemon *mon, u8 moveIndex) {
    u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
    ppBonuses &= gPPUpSetMask[moveIndex];
    SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
}

void RemoveBattleMonPPBonus(struct BattlePokemon *mon, u8 moveIndex) { mon->ppBonuses &= gPPUpSetMask[moveIndex]; }

void PokemonToBattleMon(struct Pokemon *src, struct BattlePokemon *dst) {
    s32 i;
    u8 nickname[POKEMON_NAME_LENGTH * 2];

    for (i = 0; i < MAX_MON_MOVES; i++) {
        dst->moves[i] = GetMonData(src, MON_DATA_MOVE1 + i, NULL);
        dst->pp[i] = GetMonData(src, MON_DATA_PP1 + i, NULL);
    }

    dst->species = GetMonData(src, MON_DATA_SPECIES, NULL);
    dst->item = GetMonData(src, MON_DATA_HELD_ITEM, NULL);
    dst->ppBonuses = GetMonData(src, MON_DATA_PP_BONUSES, NULL);
    dst->friendship = GetMonData(src, MON_DATA_FRIENDSHIP, NULL);
    dst->experience = GetMonData(src, MON_DATA_EXP, NULL);
    dst->personality = GetMonData(src, MON_DATA_PERSONALITY, NULL);
    dst->status1 = GetMonData(src, MON_DATA_STATUS, NULL);
    dst->level = GetMonData(src, MON_DATA_LEVEL, NULL);
    dst->hp = GetMonData(src, MON_DATA_HP, NULL);
    dst->maxHP = GetMonData(src, MON_DATA_MAX_HP, NULL);
    dst->attack = GetMonData(src, MON_DATA_ATK, NULL);
    dst->defense = GetMonData(src, MON_DATA_DEF, NULL);
    dst->speed = GetMonData(src, MON_DATA_SPEED, NULL);
    dst->spAttack = GetMonData(src, MON_DATA_SPATK, NULL);
    dst->spDefense = GetMonData(src, MON_DATA_SPDEF, NULL);
    dst->abilityNum = GetMonData(src, MON_DATA_ABILITY_NUM, NULL);
    dst->otId = GetMonData(src, MON_DATA_OT_ID, NULL);
    dst->hpType = GetMonData(src, MON_DATA_HP_TYPE, NULL);
    dst->type1 = gBaseStats[dst->species].type1;
    dst->type2 = gBaseStats[dst->species].type2;
    dst->type3 = TYPE_MYSTERY;
    dst->abilities[0] = GetAbilityBySpecies(dst->species, dst->abilityNum);
    dst->abilities[1] = GetInnateInSlot(dst->level, dst->species, 0, dst->personality, FALSE);
    dst->abilities[2] = GetInnateInSlot(dst->level, dst->species, 1, dst->personality, FALSE);
    dst->abilities[3] = GetInnateInSlot(dst->level, dst->species, 2, dst->personality, FALSE);
    dst->nature = GetMonData(src, MON_DATA_NATURE, NULL);
    GetMonData(src, MON_DATA_NICKNAME, nickname);
    StringCopy10(dst->nickname, nickname);
    GetMonData(src, MON_DATA_OT_NAME, dst->otName);

    for (i = 0; i < NUM_BATTLE_STATS; i++) dst->statStages[i] = 6;

    dst->status2 = 0;
}

void CopyPlayerPartyMonToBattleData(u8 battlerId, u8 partyIndex) {
    PokemonToBattleMon(&gPlayerParty[partyIndex], &gBattleMons[battlerId]);
    gBattleStruct->hpOnSwitchout[GetBattlerSide(battlerId)] = gBattleMons[battlerId].hp;
    UpdateSentPokesToOpponentValue(battlerId);
    ClearTemporarySpeciesSpriteData(battlerId, FALSE);
}

bool8 ExecuteTableBasedItemEffect(struct Pokemon *mon, u16 item, u8 partyIndex, u8 moveIndex) {
    return PokemonUseItemEffects(mon, item, partyIndex, moveIndex, FALSE);
}

#define UPDATE_FRIENDSHIP_FROM_ITEM                                                                               \
    {                                                                                                             \
        if ((retVal == 0 || friendshipOnly) && !ShouldSkipFriendshipChange() && friendshipChange == 0) {          \
            friendshipChange = itemEffect[itemEffectParam];                                                       \
            friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);                                              \
            if (friendshipChange > 0 && holdEffect == HOLD_EFFECT_FRIENDSHIP_UP)                                  \
                friendship += 150 * friendshipChange / 100;                                                       \
            else                                                                                                  \
                friendship += friendshipChange;                                                                   \
            if (friendshipChange > 0) {                                                                           \
                if (GetMonData(mon, MON_DATA_POKEBALL, NULL) == ITEM_LUXURY_BALL) friendship++;                   \
                if (GetMonData(mon, MON_DATA_MET_LOCATION, NULL) == GetCurrentRegionMapSectionId()) friendship++; \
            }                                                                                                     \
            if (friendship < 0) friendship = 0;                                                                   \
            if (friendship > MAX_FRIENDSHIP) friendship = MAX_FRIENDSHIP;                                         \
            SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);                                                    \
            retVal = FALSE;                                                                                       \
        }                                                                                                         \
    }

#define CANDY_BOX_LEVELS 5

// Returns TRUE if the item has no effect on the Pokémon, FALSE otherwise
bool8 PokemonUseItemEffects(struct Pokemon *mon, u16 item, u8 partyIndex, u8 moveIndex, bool8 usedByAI) {
    u32 dataUnsigned;
    s32 dataSigned, ivMax;
    s32 friendship;
    s32 i;
    bool8 retVal = TRUE;
    const u8 *itemEffect;
    u8 itemEffectParam = ITEM_EFFECT_ARG_START;
    u32 temp1, temp2;
    s8 friendshipChange = 0;
    u8 holdEffect;
    u8 battlerId = MAX_BATTLERS_COUNT;
    u32 friendshipOnly = FALSE;
    u16 heldItem;
    u8 effectFlags;
    s8 evChange;
    u8 levelUp;

    // Get item hold effect
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
    if (heldItem == ITEM_ENIGMA_BERRY) {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[gBattlerInMenuId].holdEffect;
        else {
#ifndef FREE_ENIGMA_BERRY
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
#else
            holdEffect = 0;
#endif
        }
    } else {
        holdEffect = ItemId_GetHoldEffect(heldItem);
    }

    // Get battler id (if relevant)
    gPotentialItemEffectBattler = gBattlerInMenuId;
    if (gMain.inBattle) {
        gActiveBattler = gBattlerInMenuId;
        i = (GetBattlerSide(gActiveBattler) != B_SIDE_PLAYER);
        while (i < gBattlersCount) {
            if (gBattlerPartyIndexes[i] == partyIndex) {
                battlerId = i;
                break;
            }
            i += 2;
        }
    } else {
        gActiveBattler = 0;
        battlerId = MAX_BATTLERS_COUNT;
    }

    // Skip using the item if it won't do anything
    if (!ITEM_HAS_EFFECT(item) && ItemId_GetFieldFunc(item) != ItemId_GetFieldFunc(ITEM_FIRE_STONE)) return TRUE;
    if (gItemEffectTable[item - ITEM_POTION] == NULL && item != ITEM_ENIGMA_BERRY) return TRUE;

    // Get item effect
    if (item == ITEM_ENIGMA_BERRY) {
        if (gMain.inBattle)
            itemEffect = gEnigmaBerries[gActiveBattler].itemEffect;
        else {
#ifndef FREE_ENIGMA_BERRY
            itemEffect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
#else
            itemEffect = 0;
#endif
        }
    } else {
        itemEffect = gItemEffectTable[item - ITEM_POTION];
    }

    // Do item effect
    for (i = 0; i < ITEM_EFFECT_ARG_START; i++) {
        switch (i) {
            // Handle ITEM0 effects (infatuation, Dire Hit, X Attack). ITEM0_SACRED_ASH is handled in party_menu.c
            case 0:
                // Cure infatuation
                if ((itemEffect[i] & ITEM0_INFATUATION) && gMain.inBattle && battlerId != MAX_BATTLERS_COUNT &&
                    (gBattleMons[battlerId].status2 & STATUS2_INFATUATION)) {
                    gBattleMons[battlerId].status2 &= ~STATUS2_INFATUATION;
                    retVal = FALSE;
                }

                // Dire Hit
                if ((itemEffect[i] & ITEM0_DIRE_HIT) && gVolatileStructs[gActiveBattler].critBoost < 3) {
                    int increase = min(2, 3 - gVolatileStructs[gActiveBattler].critBoost);
                    gVolatileStructs[gActiveBattler].critBoost += increase;
                    retVal = FALSE;
                }
#ifndef ITEM_EXPANSION
                // X Attack
                if ((itemEffect[i] & ITEM0_X_ATTACK) && gBattleMons[gActiveBattler].statStages[STAT_ATK] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_ATK] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_ATK] += itemEffect[i] & ITEM0_X_ATTACK;
                    if (gBattleMons[gActiveBattler].statStages[STAT_ATK] > MAX_STAT_STAGE) gBattleMons[gActiveBattler].statStages[STAT_ATK] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }
#endif
                break;

// Handle ITEM1 effects (in-battle stat boosting effects)
#ifndef ITEM_EXPANSION
            case 1:
                // X Defend
                if ((itemEffect[i] & ITEM1_X_DEFEND) && gBattleMons[gActiveBattler].statStages[STAT_DEF] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_DEF] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_DEF] += (itemEffect[i] & ITEM1_X_DEFEND) >> 4;
                    if (gBattleMons[gActiveBattler].statStages[STAT_DEF] > MAX_STAT_STAGE) gBattleMons[gActiveBattler].statStages[STAT_DEF] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Speed
                if ((itemEffect[i] & ITEM1_X_SPEED) && gBattleMons[gActiveBattler].statStages[STAT_SPEED] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_SPEED] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_SPEED] += itemEffect[i] & ITEM1_X_SPEED;
                    if (gBattleMons[gActiveBattler].statStages[STAT_SPEED] > MAX_STAT_STAGE)
                        gBattleMons[gActiveBattler].statStages[STAT_SPEED] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }
                break;
            // Handle ITEM2 effects (more stat boosting effects)
            case 2:
                // X Accuracy
                if ((itemEffect[i] & ITEM2_X_ACCURACY) && gBattleMons[gActiveBattler].statStages[STAT_ACC] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_ACC] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_ACC] += (itemEffect[i] & ITEM2_X_ACCURACY) >> 4;
                    if (gBattleMons[gActiveBattler].statStages[STAT_ACC] > MAX_STAT_STAGE) gBattleMons[gActiveBattler].statStages[STAT_ACC] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Sp Attack
                if ((itemEffect[i] & ITEM2_X_SPATK) && gBattleMons[gActiveBattler].statStages[STAT_SPATK] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_SPATK] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_SPATK] += itemEffect[i] & ITEM2_X_SPATK;
                    if (gBattleMons[gActiveBattler].statStages[STAT_SPATK] > MAX_STAT_STAGE)
                        gBattleMons[gActiveBattler].statStages[STAT_SPATK] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }
                break;
#else
            // Handle ITEM1 effects (in-battle stat boosting effects)
            case 1:
                // X Attack
                if ((itemEffect[i] & ITEM1_X_ATTACK) && gBattleMons[gActiveBattler].statStages[STAT_ATK] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_ATK] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_ATK] += 1;
                    if (gBattleMons[gActiveBattler].statStages[STAT_ATK] > MAX_STAT_STAGE) gBattleMons[gActiveBattler].statStages[STAT_ATK] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Defense
                if ((itemEffect[i] & ITEM1_X_DEFENSE) && gBattleMons[gActiveBattler].statStages[STAT_DEF] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_DEF] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_DEF] += 1;
                    if (gBattleMons[gActiveBattler].statStages[STAT_DEF] > MAX_STAT_STAGE) gBattleMons[gActiveBattler].statStages[STAT_DEF] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Speed
                if ((itemEffect[i] & ITEM1_X_SPEED) && gBattleMons[gActiveBattler].statStages[STAT_SPEED] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_SPEED] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_SPEED] += 1;
                    if (gBattleMons[gActiveBattler].statStages[STAT_SPEED] > MAX_STAT_STAGE)
                        gBattleMons[gActiveBattler].statStages[STAT_SPEED] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Sp Attack
                if ((itemEffect[i] & ITEM1_X_SPATK) && gBattleMons[gActiveBattler].statStages[STAT_SPATK] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_SPATK] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_SPATK] += 1;
                    if (gBattleMons[gActiveBattler].statStages[STAT_SPATK] > MAX_STAT_STAGE)
                        gBattleMons[gActiveBattler].statStages[STAT_SPATK] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Sp Defense
                if ((itemEffect[i] & ITEM1_X_SPDEF) && gBattleMons[gActiveBattler].statStages[STAT_SPDEF] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_SPDEF] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_SPDEF] += 1;
                    if (gBattleMons[gActiveBattler].statStages[STAT_SPDEF] > MAX_STAT_STAGE)
                        gBattleMons[gActiveBattler].statStages[STAT_SPDEF] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }

                // X Accuracy
                if ((itemEffect[i] & ITEM1_X_ACCURACY) && gBattleMons[gActiveBattler].statStages[STAT_ACC] < MAX_STAT_STAGE) {
                    if (B_X_ITEMS_BUFF >= GEN_7)
                        gBattleMons[gActiveBattler].statStages[STAT_ACC] += 2;
                    else
                        gBattleMons[gActiveBattler].statStages[STAT_ACC] += 1;
                    if (gBattleMons[gActiveBattler].statStages[STAT_ACC] > MAX_STAT_STAGE) gBattleMons[gActiveBattler].statStages[STAT_ACC] = MAX_STAT_STAGE;
                    retVal = FALSE;
                }
                break;
            // Formerly used by the item effects of the X Sp. Atk and the X Accuracy
            case 2:
                break;
#endif
            // Handle ITEM3 effects (Guard Spec, Rare Candy, cure status)
            case 3:
                // Guard Spec
                if ((itemEffect[i] & ITEM3_GUARD_SPEC) && gSideTimers[GetBattlerSide(gActiveBattler)].mistTimer == 0) {
                    gSideTimers[GetBattlerSide(gActiveBattler)].started.mist = TRUE;
                    gSideTimers[GetBattlerSide(gActiveBattler)].mistTimer = SCREEN_DURATION;
                    retVal = FALSE;
                }

                // Candy Box
                if ((itemEffect[i] & ITEM3_LEVEL_UP) && GetMonData(mon, MON_DATA_LEVEL, NULL) != MAX_LEVEL &&
                    GetMonData(mon, MON_DATA_LEVEL, NULL) < GetLevelCap() && FlagGet(FLAG_USED_CANDY_BOX)) {
                    if (VarGet(VAR_CANDY_BOX_LEVEL) == 0) {
                        levelUp = GetLevelCap() - GetMonData(mon, MON_DATA_LEVEL, NULL);
                    } else {
                        levelUp = VarGet(VAR_CANDY_BOX_LEVEL) - 1;
                        if (levelUp > CANDY_BOX_LEVELS) levelUp = CANDY_BOX_LEVELS;
                    }

                    if ((GetMonData(mon, MON_DATA_LEVEL, NULL) + levelUp) >= MAX_LEVEL) levelUp = MAX_LEVEL - GetMonData(mon, MON_DATA_LEVEL, NULL);

                    dataUnsigned =
                        gExperienceTables[gBaseStats[GetMonData(mon, MON_DATA_SPECIES, NULL)].growthRate][GetMonData(mon, MON_DATA_LEVEL, NULL) + levelUp];
                    SetMonData(mon, MON_DATA_EXP, &dataUnsigned);
                    CalculateMonStats(mon);
                    retVal = FALSE;
                }

                // Rare Candy
                if ((itemEffect[i] & ITEM3_LEVEL_UP) && GetMonData(mon, MON_DATA_LEVEL, NULL) != MAX_LEVEL &&
                    GetMonData(mon, MON_DATA_LEVEL, NULL) < GetLevelCap()) {
                    levelUp = 1;
                    dataUnsigned =
                        gExperienceTables[gBaseStats[GetMonData(mon, MON_DATA_SPECIES, NULL)].growthRate][GetMonData(mon, MON_DATA_LEVEL, NULL) + levelUp];
                    SetMonData(mon, MON_DATA_EXP, &dataUnsigned);
                    CalculateMonStats(mon);
                    retVal = FALSE;
                }

                // Cure status
                if ((itemEffect[i] & ITEM3_SLEEP) && HealStatusConditions(mon, partyIndex, STATUS1_SLEEP, battlerId) == 0) {
                    if (battlerId != MAX_BATTLERS_COUNT) gBattleMons[battlerId].status2 &= ~STATUS2_NIGHTMARE;
                    retVal = FALSE;
                }
                if ((itemEffect[i] & ITEM3_POISON) && HealStatusConditions(mon, partyIndex, STATUS1_PSN_ANY | STATUS1_TOXIC_COUNTER, battlerId) == 0)
                    retVal = FALSE;
                if ((itemEffect[i] & ITEM3_BURN) && HealStatusConditions(mon, partyIndex, STATUS1_BURN, battlerId) == 0) retVal = FALSE;
                if ((itemEffect[i] & ITEM3_FREEZE) && HealStatusConditions(mon, partyIndex, STATUS1_FREEZE | STATUS1_FROSTBITE, battlerId) == 0) retVal = FALSE;
                if ((itemEffect[i] & ITEM3_PARALYSIS) && HealStatusConditions(mon, partyIndex, STATUS1_PARALYSIS, battlerId) == 0) retVal = FALSE;
                if ((itemEffect[i] & ITEM3_CONFUSION)  // heal confusion
                    && gMain.inBattle && battlerId != MAX_BATTLERS_COUNT && (gBattleMons[battlerId].status2 & STATUS2_CONFUSION)) {
                    gBattleMons[battlerId].status2 &= ~STATUS2_CONFUSION;
                    retVal = FALSE;
                }
                if ((itemEffect[i] & ITEM3_BLEED) && HealStatusConditions(mon, partyIndex, STATUS1_BLEED, battlerId) == 0) retVal = FALSE;
                break;

            // Handle ITEM4 effects (Change HP/Atk EVs, HP heal, PP heal, PP up, Revive, and evolution stones)
            case 4:
                effectFlags = itemEffect[i];

                // PP Up
                if (effectFlags & ITEM4_PP_UP) {
                    effectFlags &= ~ITEM4_PP_UP;
                    dataUnsigned = (GetMonData(mon, MON_DATA_PP_BONUSES, NULL) & gPPUpGetMask[moveIndex]) >> (moveIndex * 2);
                    temp1 = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                    if (dataUnsigned <= 2 && temp1 > 4) {
                        dataUnsigned = GetMonData(mon, MON_DATA_PP_BONUSES, NULL) + gPPUpAddMask[moveIndex];
                        SetMonData(mon, MON_DATA_PP_BONUSES, &dataUnsigned);

                        dataUnsigned = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), dataUnsigned, moveIndex) - temp1;
                        dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL) + dataUnsigned;
                        SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                        retVal = FALSE;
                    }
                }
                temp1 = 0;

                // Loop through and try each of the remaining ITEM4 effects
                while (effectFlags != 0) {
                    if (effectFlags & 1) {
                        switch (temp1) {
                            case 0:  // ITEM4_EV_HP
                            case 1:  // ITEM4_EV_ATK
                                temp2 = itemEffect[itemEffectParam];
                                dataSigned = GetMonData(mon, sGetMonDataIVConstants[temp1], NULL);
                                evChange = temp2;

                                if (evChange > 0)  // Increasing IV (HP or Atk)
                                {
                                    ivMax = MAX_IV_MASK;

                                    if (dataSigned >= ivMax) break;

                                    // Limit the increase
                                    if (dataSigned + evChange > ivMax)
                                        temp2 = ivMax - (dataSigned + evChange) + evChange;
                                    else
                                        temp2 = evChange;

                                    dataSigned += temp2;
                                    SetMonData(mon, sGetMonDataIVConstants[temp1], &dataSigned);
                                } else  // Decreasing EV (HP or Atk)
                                {
                                    dataSigned = GetMonData(mon, sGetMonDataEVConstants[temp1], NULL);
                                    if (dataSigned == 0) {
                                        // No EVs to lose, but make sure friendship updates anyway
                                        friendshipOnly = TRUE;
                                        itemEffectParam++;
                                        break;
                                    }
                                    dataSigned += evChange;
                                    if (dataSigned < 0) dataSigned = 0;

                                    SetMonData(mon, sGetMonDataEVConstants[temp1], &dataSigned);
                                }

                                // Update EVs and stats
                                CalculateMonStats(mon);
                                itemEffectParam++;
                                retVal = FALSE;
                                break;

                            case 2:  // ITEM4_HEAL_HP
                                // If Revive, update number of times revive has been used
                                if (effectFlags & (ITEM4_REVIVE >> 2)) {
                                    if (GetMonData(mon, MON_DATA_HP, NULL) != 0) {
                                        itemEffectParam++;
                                        break;
                                    }
                                    if (gMain.inBattle) {
                                        if (battlerId != MAX_BATTLERS_COUNT) {
                                            gAbsentBattlerFlags &= ~gBitTable[battlerId];
                                            CopyPlayerPartyMonToBattleData(battlerId, GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[battlerId]));
                                            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER && gBattleResults.numRevivesUsed < 255)
                                                gBattleResults.numRevivesUsed++;
                                        } else {
                                            gAbsentBattlerFlags &= ~gBitTable[gActiveBattler ^ 2];
                                            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER && gBattleResults.numRevivesUsed < 255)
                                                gBattleResults.numRevivesUsed++;
                                        }
                                    }
                                } else {
                                    if (GetMonData(mon, MON_DATA_HP, NULL) == 0) {
                                        itemEffectParam++;
                                        break;
                                    }
                                }

                                // Get amount of HP to restore
                                dataUnsigned = itemEffect[itemEffectParam++];
                                switch (dataUnsigned) {
                                    case ITEM6_HEAL_FULL:
                                        dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL) - GetMonData(mon, MON_DATA_HP, NULL);
                                        break;
                                    case ITEM6_HEAL_HALF:
                                        dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL) / 2;
                                        if (dataUnsigned == 0) dataUnsigned = 1;
                                        break;
                                    case ITEM6_HEAL_QUARTER:
                                        dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL) / 4;
                                        if (dataUnsigned == 0) dataUnsigned = 1;
                                        break;
                                    case ITEM6_HEAL_LVL_UP:
                                        dataUnsigned = gBattleScripting.levelUpHP;
                                        break;
                                }

                                // Only restore HP if not at max health
                                if (GetMonData(mon, MON_DATA_MAX_HP, NULL) != GetMonData(mon, MON_DATA_HP, NULL)) {
                                    if (!usedByAI) {
                                        // Restore HP
                                        dataUnsigned = GetMonData(mon, MON_DATA_HP, NULL) + dataUnsigned;
                                        if (dataUnsigned > GetMonData(mon, MON_DATA_MAX_HP, NULL)) dataUnsigned = GetMonData(mon, MON_DATA_MAX_HP, NULL);
                                        SetMonData(mon, MON_DATA_HP, &dataUnsigned);

                                        // Update battler (if applicable)
                                        if (gMain.inBattle && battlerId != MAX_BATTLERS_COUNT) {
                                            gBattleMons[battlerId].hp = dataUnsigned;
                                            if (!(effectFlags & (ITEM4_REVIVE >> 2)) && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
                                                if (gBattleResults.numHealingItemsUsed < 255) gBattleResults.numHealingItemsUsed++;

                                                temp2 = gActiveBattler;
                                                gActiveBattler = battlerId;
                                                BtlController_EmitGetMonData(0, REQUEST_ALL_BATTLE, 0);
                                                MarkBattlerForControllerExec(gActiveBattler);
                                                gActiveBattler = temp2;
                                            }
                                        }
                                    } else {
                                        gBattleMoveDamage = -dataUnsigned;
                                    }
                                    retVal = FALSE;
                                }
                                effectFlags &= ~(ITEM4_REVIVE >> 2);
                                break;

                            case 3:  // ITEM4_HEAL_PP
                                if (!(effectFlags & (ITEM4_HEAL_PP_ONE >> 3))) {
                                    // Heal PP for all moves
                                    for (temp2 = 0; (signed)(temp2) < (signed)(MAX_MON_MOVES); temp2++) {
                                        u16 moveId;
                                        dataUnsigned = GetMonData(mon, MON_DATA_PP1 + temp2, NULL);
                                        moveId = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL);
                                        if (dataUnsigned != CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2)) {
                                            dataUnsigned += itemEffect[itemEffectParam];
                                            moveId = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL);  // Redundant
                                            if (dataUnsigned > CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2)) {
                                                moveId = GetMonData(mon, MON_DATA_MOVE1 + temp2, NULL);  // Redundant
                                                dataUnsigned = CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), temp2);
                                            }
                                            SetMonData(mon, MON_DATA_PP1 + temp2, &dataUnsigned);

                                            // Heal battler PP too (if applicable)
                                            if (gMain.inBattle && battlerId != MAX_BATTLERS_COUNT && !(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED) &&
                                                !(gVolatileStructs[battlerId].mimickedMoves & gBitTable[temp2]))
                                                gBattleMons[battlerId].pp[temp2] = dataUnsigned;

                                            retVal = FALSE;
                                        }
                                    }
                                    itemEffectParam++;
                                } else {
                                    // Heal PP for one move
                                    u16 moveId;
                                    dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL);
                                    moveId = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL);
                                    if (dataUnsigned != CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex)) {
                                        dataUnsigned += itemEffect[itemEffectParam++];
                                        moveId = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL);  // Redundant
                                        if (dataUnsigned > CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex)) {
                                            moveId = GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL);  // Redundant
                                            dataUnsigned = CalculatePPWithBonus(moveId, GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                                        }
                                        SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);

                                        // Heal battler PP too (if applicable)
                                        if (gMain.inBattle && battlerId != MAX_BATTLERS_COUNT && !(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED) &&
                                            !(gVolatileStructs[battlerId].mimickedMoves & gBitTable[moveIndex]))
                                            gBattleMons[battlerId].pp[moveIndex] = dataUnsigned;

                                        retVal = FALSE;
                                    }
                                }
                                break;

                                // cases 4-6 are ITEM4_HEAL_PP_ONE, ITEM4_PP_UP, and ITEM4_REVIVE, which
                                // are already handled above by other cases or before the loop

                            case 7:  // ITEM4_EVO_STONE
                            {
                                u16 targetSpecies = GetEvolutionTargetSpecies(mon, EVO_MODE_ITEM_USE, item, SPECIES_NONE);

                                if (targetSpecies != SPECIES_NONE) {
                                    BeginEvolutionScene(mon, targetSpecies, FALSE, partyIndex);
                                    return FALSE;
                                }
                            } break;
                        }
                    }
                    temp1++;
                    effectFlags >>= 1;
                }
                break;

            // Handle ITEM5 effects (Change Def/SpDef/SpAtk/Speed EVs, PP Max, and friendship changes)
            case 5:
                effectFlags = itemEffect[i];
                temp1 = 0;

                // Loop through and try each of the ITEM5 effects
                while (effectFlags != 0) {
                    if (effectFlags & 1) {
                        switch (temp1) {
                            case 0:  // ITEM5_EV_DEF
                            case 1:  // ITEM5_EV_SPEED
                            case 2:  // ITEM5_EV_SPDEF
                            case 3:  // ITEM5_EV_SPATK
                                temp2 = itemEffect[itemEffectParam];
                                dataSigned = GetMonData(mon, sGetMonDataIVConstants[temp1 + 2], NULL);
                                evChange = temp2;

                                if (evChange > 0)  // Increasing IV
                                {
                                    ivMax = MAX_IV_MASK;

                                    if (dataSigned >= ivMax) break;

                                    // Limit the increase
                                    if (dataSigned + evChange > ivMax)
                                        temp2 = ivMax - (dataSigned + evChange) + evChange;
                                    else
                                        temp2 = evChange;

                                    dataSigned += temp2;
                                    SetMonData(mon, sGetMonDataIVConstants[temp1 + 2], &dataSigned);
                                } else  // Decreasing EV
                                {
                                    dataSigned = GetMonData(mon, sGetMonDataEVConstants[temp1 + 2], NULL);
                                    if (dataSigned == 0) {
                                        // No EVs to lose, but make sure friendship updates anyway
                                        friendshipOnly = TRUE;
                                        itemEffectParam++;
                                        break;
                                    }
                                    dataSigned += evChange;
                                    if (dataSigned < 0) dataSigned = 0;

                                    SetMonData(mon, sGetMonDataEVConstants[temp1 + 2], &dataSigned);
                                }

                                // Update EVs and stats
                                CalculateMonStats(mon);
                                itemEffectParam++;
                                retVal = FALSE;
                                break;

                            case 4:  // ITEM5_PP_MAX
                                dataUnsigned = (GetMonData(mon, MON_DATA_PP_BONUSES, NULL) & gPPUpGetMask[moveIndex]) >> (moveIndex * 2);
                                temp2 = CalculatePPWithBonus(
                                    GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), GetMonData(mon, MON_DATA_PP_BONUSES, NULL), moveIndex);
                                if (dataUnsigned < 3 && temp2 > 4) {
                                    dataUnsigned = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
                                    dataUnsigned &= gPPUpSetMask[moveIndex];
                                    dataUnsigned += gPPUpAddMask[moveIndex] * 3;

                                    SetMonData(mon, MON_DATA_PP_BONUSES, &dataUnsigned);
                                    dataUnsigned = CalculatePPWithBonus(GetMonData(mon, MON_DATA_MOVE1 + moveIndex, NULL), dataUnsigned, moveIndex) - temp2;
                                    dataUnsigned = GetMonData(mon, MON_DATA_PP1 + moveIndex, NULL) + dataUnsigned;
                                    SetMonData(mon, MON_DATA_PP1 + moveIndex, &dataUnsigned);
                                    retVal = FALSE;
                                }
                                break;

                            case 5:  // ITEM5_FRIENDSHIP_LOW
                                // Changes to friendship are given differently depending on
                                // how much friendship the Pokémon already has.
                                // In general, Pokémon with lower friendship receive more,
                                // and Pokémon with higher friendship receive less.
                                if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) < 100) {
                                    UPDATE_FRIENDSHIP_FROM_ITEM;
                                }
                                itemEffectParam++;
                                break;

                            case 6:  // ITEM5_FRIENDSHIP_MID
                                if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) >= 100 && GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) < 200) {
                                    UPDATE_FRIENDSHIP_FROM_ITEM;
                                }
                                itemEffectParam++;
                                break;

                            case 7:  // ITEM5_FRIENDSHIP_HIGH
                                if (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) >= 200) {
                                    UPDATE_FRIENDSHIP_FROM_ITEM;
                                }
                                itemEffectParam++;
                                break;
                        }
                    }
                    temp1++;
                    effectFlags >>= 1;
                }
                break;
        }
    }
    return retVal;
}

bool8 HealStatusConditions(struct Pokemon *mon, u32 battlePartyId, u32 healMask, u8 battlerId) {
    u32 status = GetMonData(mon, MON_DATA_STATUS, 0);

    if (status & healMask) {
        status &= ~healMask;
        SetMonData(mon, MON_DATA_STATUS, &status);
        if (gMain.inBattle && battlerId != MAX_BATTLERS_COUNT) gBattleMons[battlerId].status1 &= ~healMask;
        return FALSE;
    } else {
        return TRUE;
    }
}

u8 GetItemEffectParamOffset(u16 itemId, u8 effectByte, u8 effectBit) {
    const u8 *temp;
    const u8 *itemEffect;
    u8 offset;
    int i;
    u8 j;
    u8 effectFlags;

    offset = ITEM_EFFECT_ARG_START;

    temp = gItemEffectTable[itemId - ITEM_POTION];

    if (!temp && itemId != ITEM_ENIGMA_BERRY) return 0;

    if (itemId == ITEM_ENIGMA_BERRY) {
        temp = gEnigmaBerries[gActiveBattler].itemEffect;
    }

    itemEffect = temp;

    for (i = 0; i < ITEM_EFFECT_ARG_START; i++) {
        switch (i) {
            case 0:
            case 1:
            case 2:
            case 3:
                if (i == effectByte) return 0;
                break;
            case 4:
                effectFlags = itemEffect[4];
                if (effectFlags & ITEM4_PP_UP) effectFlags &= ~(ITEM4_PP_UP);
                j = 0;
                while (effectFlags) {
                    if (effectFlags & 1) {
                        switch (j) {
                            case 2:  // ITEM4_HEAL_HP
                                if (effectFlags & (ITEM4_REVIVE >> 2)) effectFlags &= ~(ITEM4_REVIVE >> 2);
                                FALLTHROUGH
                            case 0:  // ITEM4_EV_HP
                                if (i == effectByte && (effectFlags & effectBit)) return offset;
                                offset++;
                                break;
                            case 1:  // ITEM4_EV_ATK
                                if (i == effectByte && (effectFlags & effectBit)) return offset;
                                offset++;
                                break;
                            case 3:  // ITEM4_HEAL_PP
                                if (i == effectByte && (effectFlags & effectBit)) return offset;
                                offset++;
                                break;
                            case 7:  // ITEM4_EVO_STONE
                                if (i == effectByte) return 0;
                                break;
                        }
                    }
                    j++;
                    effectFlags >>= 1;
                    if (i == effectByte) effectBit >>= 1;
                }
                break;
            case 5:
                effectFlags = itemEffect[5];
                j = 0;
                while (effectFlags) {
                    if (effectFlags & 1) {
                        switch (j) {
                            case 0:  // ITEM5_EV_DEF
                            case 1:  // ITEM5_EV_SPEED
                            case 2:  // ITEM5_EV_SPDEF
                            case 3:  // ITEM5_EV_SPATK
                            case 4:  // ITEM5_PP_MAX
                            case 5:  // ITEM5_FRIENDSHIP_LOW
                            case 6:  // ITEM5_FRIENDSHIP_MID
                                if (i == effectByte && (effectFlags & effectBit)) return offset;
                                offset++;
                                break;
                            case 7:  // ITEM5_FRIENDSHIP_HIGH
                                if (i == effectByte) return 0;
                                break;
                        }
                    }
                    j++;
                    effectFlags >>= 1;
                    if (i == effectByte) effectBit >>= 1;
                }
                break;
        }
    }

    return offset;
}

static void BufferStatRoseMessage(s32 arg0) {
    gBattlerTarget = gBattlerInMenuId;
    StringCopy(gBattleTextBuff1, gStatNamesTable[sStatsToRaise[arg0]]);
    if (B_X_ITEMS_BUFF >= GEN_7) {
        StringCopy(gBattleTextBuff2, gText_StatSharply);
        StringAppend(gBattleTextBuff2, gText_StatRose);
    } else {
        StringCopy(gBattleTextBuff2, gText_StatRose);
    }
    BattleStringExpandPlaceholdersToDisplayedString(gText_DefendersStatRose);
}

u8 *UseStatIncreaseItem(u16 itemId) {
    const u8 *itemEffect;

    if (itemId == ITEM_ENIGMA_BERRY) {
#ifndef ITEM_EXPANSION
        if (gMain.inBattle)
            itemEffect = gEnigmaBerries[gBattlerInMenuId].itemEffect;
        else
            itemEffect = gSaveBlock1Ptr->enigmaBerry.itemEffect;
#else
        itemEffect = 0;
#endif
    } else {
        itemEffect = gItemEffectTable[itemId - ITEM_POTION];
    }

    gPotentialItemEffectBattler = gBattlerInMenuId;

#ifndef ITEM_EXPANSION
    for (i = 0; i < 3; i++) {
        if (itemEffect[i] & (ITEM0_X_ATTACK | ITEM1_X_SPEED | ITEM2_X_SPATK)) BufferStatRoseMessage(i * 2);

        if (itemEffect[i] & (ITEM0_DIRE_HIT | ITEM1_X_DEFEND | ITEM2_X_ACCURACY)) {
            if (i != 0)  // Dire Hit is the only ITEM0 above
            {
                BufferStatRoseMessage(i * 2 + 1);
            } else {
                gBattlerAttacker = gBattlerInMenuId;
                BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnGettingPumped);
            }
        }
    }

    if (itemEffect[3] & ITEM3_GUARD_SPEC) {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnShroudedInMist);
    }
#else
    if (itemEffect[0] & ITEM0_DIRE_HIT) {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnGettingPumped);
    }

    switch (itemEffect[1]) {
        case ITEM1_X_ATTACK:
            BufferStatRoseMessage(STAT_ATK);
            break;
        case ITEM1_X_DEFENSE:
            BufferStatRoseMessage(STAT_DEF);
            break;
        case ITEM1_X_SPEED:
            BufferStatRoseMessage(STAT_SPEED);
            break;
        case ITEM1_X_SPATK:
            BufferStatRoseMessage(STAT_SPATK);
            break;
        case ITEM1_X_SPDEF:
            BufferStatRoseMessage(STAT_SPDEF);
            break;
        case ITEM1_X_ACCURACY:
            BufferStatRoseMessage(STAT_ACC);
            break;
    }

    if (itemEffect[3] & ITEM3_GUARD_SPEC) {
        gBattlerAttacker = gBattlerInMenuId;
        BattleStringExpandPlaceholdersToDisplayedString(gText_PkmnShroudedInMist);
    }
#endif

    return gDisplayedStringBattle;
}

u8 GetNature(struct Pokemon *mon) { return GetMonData(mon, MON_DATA_PERSONALITY, 0) % NUM_NATURES; }

u8 GetNatureFromPersonality(u32 personality) { return personality % NUM_NATURES; }

u16 GetEvolutionTargetSpecies(struct Pokemon *mon, u8 mode, u16 evolutionItem, u16 tradePartnerSpecies) {
    int i, j;
    u16 targetSpecies = 0;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    u8 level;
    u16 friendship;
    u8 beauty = GetMonData(mon, MON_DATA_BEAUTY, 0);
    u16 upperPersonality = personality >> 16;
    u8 holdEffect;
    u16 currentMap;

    if (heldItem == ITEM_ENIGMA_BERRY) {
#ifndef FREE_ENIGMA_BERRY
        holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
#else
        holdEffect = 0;
#endif
    } else
        holdEffect = ItemId_GetHoldEffect(heldItem);

    // Prevent evolution with Everstone, unless we're just viewing the party menu with an evolution item
    if (((holdEffect == HOLD_EFFECT_PREVENT_EVOLVE) && mode != EVO_MODE_ITEM_CHECK) || !gSaveBlock2Ptr->automaticEvolution) return SPECIES_NONE;

    /*Old code with Eviolite preventing evolutions
    if ((holdEffect == HOLD_EFFECT_PREVENT_EVOLVE || ItemId_GetId(heldItem) == ITEM_EVIOLITE) && mode != EVO_MODE_ITEM_CHECK)
        return SPECIES_NONE;*/

    switch (mode) {
        case EVO_MODE_NORMAL:
            level = GetMonData(mon, MON_DATA_LEVEL, 0);
            friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, 0);

            for (i = 0; gEvolutionTable[species][i].method; i++) {
                switch (gEvolutionTable[species][i].method) {
                    case EVO_FRIENDSHIP:
                        if (friendship >= 160) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_FRIENDSHIP_DAY:
                        RtcCalcLocalTime();
                        if (GetCurrentTimeOfDay() != TIME_NIGHT && friendship >= 160) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_DAY:
                        RtcCalcLocalTime();
                        if (GetCurrentTimeOfDay() != TIME_NIGHT && gEvolutionTable[species][i].param <= level) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_FRIENDSHIP_NIGHT:
                        RtcCalcLocalTime();
                        if (GetCurrentTimeOfDay() == TIME_NIGHT && friendship >= 160) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_NIGHT:
                        RtcCalcLocalTime();
                        if (GetCurrentTimeOfDay() == TIME_NIGHT && gEvolutionTable[species][i].param <= level) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_ITEM_HOLD_NIGHT:
                        RtcCalcLocalTime();
                        if (GetCurrentTimeOfDay() == TIME_NIGHT && heldItem == gEvolutionTable[species][i].param) {
                            heldItem = 0;
                            SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        }
                        break;
                    case EVO_ITEM_HOLD_DAY:
                        RtcCalcLocalTime();
                        if (GetCurrentTimeOfDay() != TIME_NIGHT && heldItem == gEvolutionTable[species][i].param) {
                            heldItem = 0;
                            SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        }
                        break;
                    case EVO_ITEM_HOLD:
                        if (heldItem == gEvolutionTable[species][i].param) {
                            heldItem = 0;
                            SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        }
                        break;
                    case EVO_LEVEL_DUSK:
                        RtcCalcLocalTime();
                        if (IsCurrentlyDusk() && gEvolutionTable[species][i].param <= level) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL:
                        if (gEvolutionTable[species][i].param <= level) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_FEMALE:
                        if (gEvolutionTable[species][i].param <= level && GetMonGender(mon) == MON_FEMALE)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_MALE:
                        if (gEvolutionTable[species][i].param <= level && GetMonGender(mon) == MON_MALE)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_ATK_GT_DEF:
                        if (gEvolutionTable[species][i].param <= level)
                            if (GetMonData(mon, MON_DATA_ATK, 0) > GetMonData(mon, MON_DATA_DEF, 0)) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_ATK_EQ_DEF:
                        if (gEvolutionTable[species][i].param <= level)
                            if (GetMonData(mon, MON_DATA_ATK, 0) == GetMonData(mon, MON_DATA_DEF, 0)) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_ATK_LT_DEF:
                        if (gEvolutionTable[species][i].param <= level)
                            if (GetMonData(mon, MON_DATA_ATK, 0) < GetMonData(mon, MON_DATA_DEF, 0)) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_SILCOON:
                        if (gEvolutionTable[species][i].param <= level && (upperPersonality % 10) <= 4)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_CASCOON:
                        if (gEvolutionTable[species][i].param <= level && (upperPersonality % 10) > 4)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_LEVEL_NINJASK:
                        if (gEvolutionTable[species][i].param <= level) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_BEAUTY:
                        if (gEvolutionTable[species][i].param <= beauty) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_MOVE:
                        if (MonKnowsMove(mon, gEvolutionTable[species][i].param)) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_MOVE_TYPE:
                        for (j = 0; j < 4; j++) {
                            if (gBattleMoves[GetMonData(mon, MON_DATA_MOVE1 + j, NULL)].type == gEvolutionTable[species][i].param) {
                                targetSpecies = gEvolutionTable[species][i].targetSpecies;
                                break;
                            }
                        }
                        break;
                    case EVO_SPECIFIC_MON_IN_PARTY:
                        for (j = 0; j < PARTY_SIZE; j++) {
                            if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) == gEvolutionTable[species][i].param) {
                                targetSpecies = gEvolutionTable[species][i].targetSpecies;
                                break;
                            }
                        }
                        break;
                    case EVO_LEVEL_DARK_TYPE_MON_IN_PARTY:
                        if (gEvolutionTable[species][i].param <= level) {
                            for (j = 0; j < PARTY_SIZE; j++) {
                                u16 currSpecies = GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL);
                                if (gBaseStats[currSpecies].type1 == TYPE_DARK || gBaseStats[currSpecies].type2 == TYPE_DARK) {
                                    targetSpecies = gEvolutionTable[species][i].targetSpecies;
                                    break;
                                }
                            }
                        }
                        break;
                    case EVO_LEVEL_RAIN:
                        j = GetCurrentWeather();
                        if (j == WEATHER_RAIN || j == WEATHER_RAIN_THUNDERSTORM || j == WEATHER_DOWNPOUR)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_SPECIFIC_MAPSEC:
                        if (gMapHeader.regionMapSectionId == gEvolutionTable[species][i].param) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_SPECIFIC_MAP:
                        currentMap = ((gSaveBlock1Ptr->location.mapGroup) << 8 | gSaveBlock1Ptr->location.mapNum);
                        if (currentMap == gEvolutionTable[species][i].param) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                }
            }
            break;
        case EVO_MODE_TRADE:
            for (i = 0; gEvolutionTable[species][i].method; i++) {
                switch (gEvolutionTable[species][i].method) {
                    case EVO_TRADE:
                        targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_TRADE_ITEM:
                        if (gEvolutionTable[species][i].param == heldItem) {
                            heldItem = 0;
                            SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        }
                        break;
                    case EVO_TRADE_SPECIFIC_MON:
                        if (gEvolutionTable[species][i].param == tradePartnerSpecies) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                }
            }
            break;
        case EVO_MODE_ITEM_USE:
        case EVO_MODE_ITEM_CHECK:
            for (i = 0; gEvolutionTable[species][i].method; i++) {
                switch (gEvolutionTable[species][i].method) {
                    case EVO_ITEM:
                        if (gEvolutionTable[species][i].param == evolutionItem) targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_ITEM_FEMALE:
                        if (GetMonGender(mon) == MON_FEMALE && gEvolutionTable[species][i].param == evolutionItem)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                    case EVO_ITEM_MALE:
                        if (GetMonGender(mon) == MON_MALE && gEvolutionTable[species][i].param == evolutionItem)
                            targetSpecies = gEvolutionTable[species][i].targetSpecies;
                        break;
                }
            }
            break;
    }

    return targetSpecies;
}

u16 HoennPokedexNumToSpecies(u16 hoennNum) {
    if (!hoennNum) return SPECIES_NONE;

    for (SpeciesEnum species = 1; species < NUM_SPECIES; species++) {
        if (gSpeciesToHoennPokedexNum[species] == hoennNum) return species;
    }

    return SPECIES_NONE;
}

u16 NationalPokedexNumToSpecies(u16 nationalNum) {
    if (!nationalNum) return SPECIES_NONE;

    for (SpeciesEnum species = 1; species < NUM_SPECIES; species++) {
        if (gSpeciesToNationalPokedexNum[species] == nationalNum) return species;
    }

    return SPECIES_NONE;
}

u16 NationalToHoennOrder(u16 nationalNum) {
    if (!nationalNum) return 0;

    for (int hoennNum = 1; hoennNum < ARRAY_COUNT(gHoennToNationalOrder); hoennNum++) {
        if (gHoennToNationalOrder[hoennNum] == nationalNum) return hoennNum;
    }

    return 0;
}

u16 SpeciesToNationalPokedexNum(SpeciesEnum species) {
    if (!species) return 0;

    return gSpeciesToNationalPokedexNum[GET_BASE_SPECIES_ID(species)];
}

u16 SpeciesToHoennPokedexNum(SpeciesEnum species) {
    if (!species) return 0;

    return gSpeciesToHoennPokedexNum[GET_BASE_SPECIES_ID(species)];
}

u16 HoennToNationalOrder(u16 hoennNum) {
    if (hoennNum > ARRAY_COUNT(gHoennToNationalOrder)) return 0;

    return gHoennToNationalOrder[hoennNum];
}

#define DRAW_SPINDA_SPOTS                                                                               \
    {                                                                                                   \
        int i;                                                                                          \
        for (i = 0; i < 4; i++) {                                                                       \
            int j;                                                                                      \
            u8 x = gSpindaSpotGraphics[i].x + ((personality & 0x0F) - 8);                               \
            u8 y = gSpindaSpotGraphics[i].y + (((personality & 0xF0) >> 4) - 8);                        \
                                                                                                        \
            for (j = 0; j < 16; j++) {                                                                  \
                int k;                                                                                  \
                s32 row = gSpindaSpotGraphics[i].image[j];                                              \
                                                                                                        \
                for (k = x; k < x + 16; k++) {                                                          \
                    u8 *val = dest + ((k / 8) * 32) + ((k % 8) / 2) + ((y >> 3) << 8) + ((y & 7) << 2); \
                                                                                                        \
                    if (row & 1) {                                                                      \
                        if (k & 1) {                                                                    \
                            if ((u8)((*val & 0xF0) - 0x10) <= 0x20) *val += 0x40;                       \
                        } else {                                                                        \
                            if ((u8)((*val & 0xF) - 0x01) <= 0x02) *val += 0x04;                        \
                        }                                                                               \
                    }                                                                                   \
                                                                                                        \
                    row >>= 1;                                                                          \
                }                                                                                       \
                                                                                                        \
                y++;                                                                                    \
            }                                                                                           \
                                                                                                        \
            personality >>= 8;                                                                          \
        }                                                                                               \
    }

static void DrawSpindaSpotsUnused(SpeciesEnum species, u32 personality, u8 *dest) {
    if (species == SPECIES_SPINDA && dest != gMonSpritesGfxPtr->sprites.ptr[0] && dest != gMonSpritesGfxPtr->sprites.ptr[2]) DRAW_SPINDA_SPOTS;
}

void DrawSpindaSpots(SpeciesEnum species, u32 personality, u8 *dest, bool8 isFrontPic) {
    if (species == SPECIES_SPINDA && isFrontPic) DRAW_SPINDA_SPOTS;
}

void EvolutionRenameMon(struct Pokemon *mon, u16 oldSpecies, u16 newSpecies) {
    u8 language;
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    language = GetMonData(mon, MON_DATA_LANGUAGE, &language);
    if (language == GAME_LANGUAGE && !StringCompare(gSpeciesNames[oldSpecies], gStringVar1)) SetMonData(mon, MON_DATA_NICKNAME, gSpeciesNames[newSpecies]);
}

// The below two functions determine which side of a multi battle the trainer battles on
// 0 is the left (top in  party menu), 1 is right (bottom in party menu)
u8 GetPlayerFlankId(void) {
    u8 flankId = 0;
    switch (gLinkPlayers[GetMultiplayerId()].id) {
        case 0:
        case 3:
            flankId = 0;
            break;
        case 1:
        case 2:
            flankId = 1;
            break;
    }
    return flankId;
}

u16 GetLinkTrainerFlankId(u8 linkPlayerId) {
    u16 flankId = 0;
    switch (gLinkPlayers[linkPlayerId].id) {
        case 0:
        case 3:
            flankId = 0;
            break;
        case 1:
        case 2:
            flankId = 1;
            break;
    }
    return flankId;
}

s32 GetBattlerMultiplayerId(u16 a1) {
    s32 id;
    for (id = 0; id < MAX_LINK_PLAYERS; id++)
        if (gLinkPlayers[id].id == a1) break;
    return id;
}

u8 GetTrainerEncounterMusicId(u16 trainerOpponentId) {
    if (InBattlePyramid())
        return GetBattlePyramindTrainerEncounterMusicId(trainerOpponentId);
    else if (InTrainerHillChallenge())
        return GetTrainerEncounterMusicIdInTrainerHill(trainerOpponentId);
    else
        return TRAINER_ENCOUNTER_MUSIC(trainerOpponentId);
}

u16 ModifyStatByNature(u8 nature, u16 n, u8 statIndex) {
    u32 retVal;
    // Don't modify HP, Accuracy, or Evasion by nature
    if (statIndex <= STAT_HP || statIndex > NUM_NATURE_STATS) {
        return n;
    }

    switch (gNatureStatTable[nature][statIndex - 1]) {
        case 1:
            retVal = n * 110;
            retVal /= 100;
            break;
        case -1:
            retVal = n * 90;
            retVal /= 100;
            break;
        default:
            retVal = n;
            break;
    }

    return retVal;
}

#define IS_LEAGUE_BATTLE                                                              \
    ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) &&                                      \
     (gTrainers[gTrainerBattleOpponent_A].trainerClass == TRAINER_CLASS_ELITE_FOUR || \
      gTrainers[gTrainerBattleOpponent_A].trainerClass == TRAINER_CLASS_LEADER || gTrainers[gTrainerBattleOpponent_A].trainerClass == TRAINER_CLASS_CHAMPION))

void AdjustFriendship(struct Pokemon *mon, u8 event) {
    SpeciesEnum species;
    ItemEnum heldItem;
    u8 holdEffect;

    if (ShouldSkipFriendshipChange()) return;

    species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, 0);

    if (heldItem == ITEM_ENIGMA_BERRY) {
        if (gMain.inBattle)
            holdEffect = gEnigmaBerries[0].holdEffect;
        else {
#ifndef FREE_ENIGMA_BERRY
            holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
#else
            holdEffect = 0;
#endif
        }
    } else {
        holdEffect = ItemId_GetHoldEffect(heldItem);
    }

    if (species && species != SPECIES_EGG) {
        u8 friendshipLevel = 0;
        s16 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, 0);

        if (friendship > 99) friendshipLevel++;
        if (friendship > 199) friendshipLevel++;

        if ((event != FRIENDSHIP_EVENT_WALKING || !(Random() & 1)) && (event != FRIENDSHIP_EVENT_LEAGUE_BATTLE || IS_LEAGUE_BATTLE)) {
            s8 mod = sFriendshipEventModifiers[event][friendshipLevel];
            if (mod > 0 && holdEffect == HOLD_EFFECT_FRIENDSHIP_UP) mod = (150 * mod) / 100;
            friendship += mod;
            if (mod > 0) {
                if (GetMonData(mon, MON_DATA_POKEBALL, 0) == ITEM_LUXURY_BALL) friendship++;
                if (GetMonData(mon, MON_DATA_MET_LOCATION, 0) == GetCurrentRegionMapSectionId()) friendship++;
            }
            if (friendship < 0) friendship = 0;
            if (friendship > MAX_FRIENDSHIP) friendship = MAX_FRIENDSHIP;
            SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
        }
    }
}

void MonGainEVs(struct Pokemon *mon, u16 defeatedSpecies) {
    u8 evs[NUM_STATS];
    u16 evIncrease = 0;
    u16 totalEVs = 0;
    int i, multiplier;

    for (i = 0; i < NUM_STATS; i++) {
        evs[i] = GetMonData(mon, MON_DATA_HP_EV + i, 0);
        totalEVs += evs[i];
    }

    for (i = 0; i < NUM_STATS; i++) {
        if (totalEVs >= MAX_TOTAL_EVS || !gSaveBlock2Ptr->automaticEVGain) break;

        if (CheckPartyHasHadPokerus(mon, 0))
            multiplier = 2;
        else
            multiplier = 1;

        switch (i) {
            case STAT_HP:
                evIncrease = gBaseStats[defeatedSpecies].evYield_HP * multiplier;
                break;
            case STAT_ATK:
                evIncrease = gBaseStats[defeatedSpecies].evYield_Attack * multiplier;
                break;
            case STAT_DEF:
                evIncrease = gBaseStats[defeatedSpecies].evYield_Defense * multiplier;
                break;
            case STAT_SPEED:
                evIncrease = gBaseStats[defeatedSpecies].evYield_Speed * multiplier;
                break;
            case STAT_SPATK:
                evIncrease = gBaseStats[defeatedSpecies].evYield_SpAttack * multiplier;
                break;
            case STAT_SPDEF:
                evIncrease = gBaseStats[defeatedSpecies].evYield_SpDefense * multiplier;
                break;
        }

        if (totalEVs + (s16)evIncrease > MAX_TOTAL_EVS) evIncrease = ((s16)evIncrease + MAX_TOTAL_EVS) - (totalEVs + evIncrease);

        if (evs[i] + (s16)evIncrease > MAX_PER_STAT_EVS) {
            int val1 = (s16)evIncrease + MAX_PER_STAT_EVS;
            int val2 = evs[i] + evIncrease;
            evIncrease = val1 - val2;
        }

        evs[i] += evIncrease;
        totalEVs += evIncrease;
        SetMonData(mon, MON_DATA_HP_EV + i, &evs[i]);
    }
}

u16 GetMonEVCount(struct Pokemon *mon) {
    int i;
    u16 count = 0;

    for (i = 0; i < NUM_STATS; i++) count += GetMonData(mon, MON_DATA_HP_EV + i, 0);

    return count;
}

void RandomlyGivePartyPokerus(struct Pokemon *party) {
    u16 rnd = Random();
    if (rnd == 0x4000 || rnd == 0x8000 || rnd == 0xC000) {
        struct Pokemon *mon;

        do {
            do {
                rnd = Random() % PARTY_SIZE;
                mon = &party[rnd];
            } while (!GetMonData(mon, MON_DATA_SPECIES, 0));
        } while (GetMonData(mon, MON_DATA_IS_EGG, 0));

        if (!(CheckPartyHasHadPokerus(party, gBitTable[rnd]))) {
            u8 rnd2;

            do {
                rnd2 = Random();
            } while ((rnd2 & 0x7) == 0);

            if (rnd2 & 0xF0) rnd2 &= 0x7;

            rnd2 |= (rnd2 << 4);
            rnd2 &= 0xF3;
            rnd2++;

            SetMonData(&party[rnd], MON_DATA_POKERUS, &rnd2);
        }
    }
}

u8 CheckPartyPokerus(struct Pokemon *party, u8 selection) {
    u8 retVal;

    int partyIndex = 0;
    unsigned curBit = 1;
    retVal = 0;

    if (selection) {
        do {
            if ((selection & 1) && (GetMonData(&party[partyIndex], MON_DATA_POKERUS, 0) & 0xF)) retVal |= curBit;
            partyIndex++;
            curBit <<= 1;
            selection >>= 1;
        } while (selection);
    } else if (GetMonData(&party[0], MON_DATA_POKERUS, 0) & 0xF) {
        retVal = 1;
    }

    return retVal;
}

u8 CheckPartyHasHadPokerus(struct Pokemon *party, u8 selection) {
    u8 retVal;

    int partyIndex = 0;
    unsigned curBit = 1;
    retVal = 0;

    if (selection) {
        do {
            if ((selection & 1) && GetMonData(&party[partyIndex], MON_DATA_POKERUS, 0)) retVal |= curBit;
            partyIndex++;
            curBit <<= 1;
            selection >>= 1;
        } while (selection);
    } else if (GetMonData(&party[0], MON_DATA_POKERUS, 0)) {
        retVal = 1;
    }

    return retVal;
}

void UpdatePartyPokerusTime(u16 days) {
    int i;
    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, 0)) {
            u8 pokerus = GetMonData(&gPlayerParty[i], MON_DATA_POKERUS, 0);
            if (pokerus & 0x3) {
                if ((pokerus & 0xF) < days || days > 4)
                    pokerus &= 0x4;
                else
                    pokerus -= days;

                if (pokerus == 0) pokerus = 0x4;

                SetMonData(&gPlayerParty[i], MON_DATA_POKERUS, &pokerus);
            }
        }
    }
}

void PartySpreadPokerus(struct Pokemon *party) {
    if ((Random() % 3) == 0) {
        int i;
        for (i = 0; i < PARTY_SIZE; i++) {
            if (GetMonData(&party[i], MON_DATA_SPECIES, 0)) {
                u8 pokerus = GetMonData(&party[i], MON_DATA_POKERUS, 0);
                u8 curPokerus = pokerus;
                if (pokerus) {
                    if (pokerus & 0xF) {
                        // Spread to adjacent party members.
                        if (i != 0 && !(GetMonData(&party[i - 1], MON_DATA_POKERUS, 0) & 0xF0)) SetMonData(&party[i - 1], MON_DATA_POKERUS, &curPokerus);
                        if (i != (PARTY_SIZE - 1) && !(GetMonData(&party[i + 1], MON_DATA_POKERUS, 0) & 0xF0)) {
                            SetMonData(&party[i + 1], MON_DATA_POKERUS, &curPokerus);
                            i++;
                        }
                    }
                }
            }
        }
    }
}

bool8 TryIncrementMonLevel(struct Pokemon *mon) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u8 nextLevel = GetMonData(mon, MON_DATA_LEVEL, 0) + 1;
    u32 expPoints = GetMonData(mon, MON_DATA_EXP, 0);
    if (expPoints > gExperienceTables[gBaseStats[species].growthRate][MAX_LEVEL]) {
        expPoints = gExperienceTables[gBaseStats[species].growthRate][MAX_LEVEL];
        SetMonData(mon, MON_DATA_EXP, &expPoints);
    }
    if (nextLevel > MAX_LEVEL || expPoints < gExperienceTables[gBaseStats[species].growthRate][nextLevel]) {
        return FALSE;
    } else {
        SetMonData(mon, MON_DATA_LEVEL, &nextLevel);
        return TRUE;
    }
}

u8 GetMoveRelearnerMoves(struct Pokemon *mon, u16 *moves, bool8 disableLearned) {
    u16 learnedMoves[4];
    u8 numMoves = 0;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    int i, j = 0, k;

    if (disableLearned) {
        for (i = 0; i < MAX_MON_MOVES; i++) learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);
    }

    for (i = 0; i < MAX_LEVEL_UP_MOVES; i++) {
        u16 moveLevel;

        if (gLevelUpLearnsets[species][i].move == LEVEL_UP_END) break;

        moveLevel = gLevelUpLearnsets[species][i].level;

        if (moveLevel <= level) {
            if (disableLearned) {
                for (j = 0; j < MAX_MON_MOVES && learnedMoves[j] != gLevelUpLearnsets[species][i].move; j++);
            }

            if (j == MAX_MON_MOVES) {
                for (k = 0; k < numMoves && moves[k] != gLevelUpLearnsets[species][i].move; k++);

                if (k == numMoves) moves[numMoves++] = RandomizeMoves(gLevelUpLearnsets[species][i].move, species, personality);
            }
        }
    }

    return numMoves;
}

u8 GetLevelUpMovesBySpecies(SpeciesEnum species, u16 *moves) {
    u8 numMoves = 0;
    int i;

    for (i = 0; i < MAX_LEVEL_UP_MOVES && gLevelUpLearnsets[species][i].move != LEVEL_UP_END; i++) moves[numMoves++] = gLevelUpLearnsets[species][i].move;

    return numMoves;
}

u8 GetNumberOfRelearnableMoves(struct Pokemon *mon) {
    u16 learnedMoves[MAX_MON_MOVES];
    u16 moves[MAX_LEVEL_UP_MOVES];
    u8 numMoves = 0;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, 0);
    int i, j, k;

    if (species == SPECIES_EGG) return 0;

    for (i = 0; i < MAX_MON_MOVES; i++) learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (i = 0; i < MAX_LEVEL_UP_MOVES; i++) {
        u16 moveLevel;

        if (gLevelUpLearnsets[species][i].move == LEVEL_UP_END) break;

        moveLevel = gLevelUpLearnsets[species][i].level;

        if (moveLevel <= level) {
            for (j = 0; j < MAX_MON_MOVES && learnedMoves[j] != gLevelUpLearnsets[species][i].move; j++);

            if (j == MAX_MON_MOVES) {
                for (k = 0; k < numMoves && moves[k] != gLevelUpLearnsets[species][i].move; k++);

                if (k == numMoves) moves[numMoves++] = gLevelUpLearnsets[species][i].move;
            }
        }
    }

    return numMoves;
}

// Egg Moves --------------------------------------------------------------------
u8 GetNumberOfEggMoves(struct Pokemon *mon) {
    u16 eggMoveBuffer[EGG_MOVES_ARRAY_COUNT];
    u16 learnedMoves[MAX_MON_MOVES];
    u8 numMoves = 0;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u16 firstStage = GetEggSpecies(species);
    u8 numEggMoves = GetEggMovesSpecies(firstStage, eggMoveBuffer);
    int i, j;
    bool8 hasMonMove = FALSE;

    if (species == SPECIES_EGG) return 0;

    for (i = 0; i < MAX_MON_MOVES; i++) learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (i = 0; i < numEggMoves; i++) {
        hasMonMove = FALSE;

        for (j = 0; j < MAX_MON_MOVES; j++) {
            if (learnedMoves[j] == eggMoveBuffer[i]) hasMonMove = TRUE;
        }

        if (!hasMonMove) numMoves++;
    }

    return numMoves;
}

u8 GetEggMoveTutorMoves(struct Pokemon *mon, u16 *moves) {
    u16 learnedMoves[4];
    u8 numMoves = 0;
    u16 eggMoveBuffer[EGG_MOVES_ARRAY_COUNT];
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    u16 firsStage = GetEggSpecies(species);
    u16 numEggMoves = GetEggMovesSpecies(firsStage, eggMoveBuffer);
    int i, j;
    bool8 hasMonMove = FALSE;

    for (i = 0; i < MAX_MON_MOVES; i++) learnedMoves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, 0);

    for (i = 0; i < numEggMoves; i++) {
        hasMonMove = FALSE;

        for (j = 0; j < MAX_MON_MOVES; j++) {
            if (learnedMoves[j] == RandomizeMoves(eggMoveBuffer[i], firsStage, personality)) hasMonMove = TRUE;
        }

        if (!hasMonMove) moves[numMoves++] = RandomizeMoves(eggMoveBuffer[i], firsStage, personality);
    }

    return numMoves;
}
// TM Moves --------------------------------------------------------------------
u8 GetNumberOfTMMoves(struct Pokemon *mon) { return 0; }

u8 GetTMMoveTutorMoves(struct Pokemon *mon, u16 *moves) { return 0; }
// Tutor Moves --------------------------------------------------------------------
u8 GetNumberOfTutorMoves(struct Pokemon *mon) {
    u8 numMoves = 0;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, 0);
    int i;

    if (species == SPECIES_EGG) return 0;

    for (i = 0; i < TUTOR_COUNT; i++) {
        if (CanLearnTutorMove(species, i) && !MonKnowsMove(mon, GetTutorMove(i)) && gBattleMoves[GetTutorMove(i)].effect != EFFECT_PLACEHOLDER) numMoves++;
    }

    return numMoves;
}

u8 GetMoveTutorMoves(struct Pokemon *mon, u16 *moves) {
    u8 numMoves = 0;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    int i;

    if (species == SPECIES_EGG) return 0;

    for (i = 0; i < TUTOR_COUNT; i++) {
        if (CanLearnTutorMove(species, i) && !MonKnowsMove(mon, RandomizeMoves(GetTutorMove(i), species, personality)) &&
            gBattleMoves[GetTutorMove(i)].effect != EFFECT_PLACEHOLDER) {
            moves[numMoves] = RandomizeMoves(GetTutorMove(i), species, personality);
            numMoves++;
        }
    }

    return numMoves;
}

u16 SpeciesToPokedexNum(SpeciesEnum species) {
    if (IsNationalPokedexEnabled()) {
        return SpeciesToNationalPokedexNum(species);
    } else {
        species = SpeciesToHoennPokedexNum(species);
        if (species <= HOENN_DEX_COUNT) return species;
        return 0xFFFF;
    }
}

bool32 IsSpeciesInHoennDex(SpeciesEnum species) {
    if (SpeciesToHoennPokedexNum(species) > HOENN_DEX_COUNT)
        return FALSE;
    else
        return TRUE;
}

void ClearBattleMonForms(void) {
    int i;
    for (i = 0; i < MAX_BATTLERS_COUNT; i++) gBattleMonForms[i] = 0;
}

u16 GetBattleBGM(void) {
    if (gBattleTypeFlags & BATTLE_TYPE_KYOGRE_GROUDON)
        return MUS_VS_KYOGRE_GROUDON;
    else if (gBattleTypeFlags & BATTLE_TYPE_REGI)
        return MUS_VS_REGI;
    else if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        return MUS_VS_TRAINER;
    else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER) {
        u8 trainerClass;

        if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
            trainerClass = GetFrontierOpponentClass(gTrainerBattleOpponent_A);
        else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL)
            trainerClass = TRAINER_CLASS_EXPERT;
        else
            trainerClass = gTrainers[gTrainerBattleOpponent_A].trainerClass;

        switch (trainerClass) {
            case TRAINER_CLASS_AQUA_LEADER:
            case TRAINER_CLASS_MAGMA_LEADER:
                return MUS_VS_AQUA_MAGMA_LEADER;
            case TRAINER_CLASS_TEAM_AQUA:
            case TRAINER_CLASS_TEAM_MAGMA:
            case TRAINER_CLASS_AQUA_ADMIN:
            case TRAINER_CLASS_MAGMA_ADMIN:
                return MUS_VS_AQUA_MAGMA;
            case TRAINER_CLASS_LEADER:
                return MUS_VS_GYM_LEADER;
            case TRAINER_CLASS_CHAMPION:
            case TRAINER_CLASS_PKMN_TRAINER_5:  // Steven
                return MUS_VS_CHAMPION;
            case TRAINER_CLASS_PKMN_TRAINER_3:
                return MUS_VS_RIVAL;
            case TRAINER_CLASS_ELITE_FOUR:
                return MUS_VS_ELITE_FOUR;
            case TRAINER_CLASS_MAGIKARP_GUY:
                return MUS_RG_VS_CHAMPION;
            case TRAINER_CLASS_SALON_MAIDEN:
            case TRAINER_CLASS_DOME_ACE:
            case TRAINER_CLASS_PALACE_MAVEN:
            case TRAINER_CLASS_ARENA_TYCOON:
            case TRAINER_CLASS_FACTORY_HEAD:
            case TRAINER_CLASS_PIKE_QUEEN:
            case TRAINER_CLASS_PYRAMID_KING:
            case TRAINER_CLASS_PKMN_TRAINER_4:
                return MUS_VS_FRONTIER_BRAIN;
            case TRAINER_CLASS_PKMN_TRAINER_1:  // Cynthia
                return DP_SEQ_BA_CHANP;
            case TRAINER_CLASS_BUFFEL:
                return PL_SEQ_PL_BA_BRAIN;
            case TRAINER_CLASS_JOHTO_CHAMP:
                return HG_SEQ_GS_VS_CHAMP;  // Leaf/Red/Lance
            default:
                return MUS_VS_TRAINER;
        }
    } else {
        switch (GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL)) {
            case SPECIES_ARTICUNO:
            case SPECIES_ZAPDOS:
            case SPECIES_MOLTRES:
                return MUS_RG_VS_LEGEND;
            case SPECIES_MEWTWO:
                return MUS_RG_VS_MEWTWO;
            case SPECIES_MEW:
            case SPECIES_CELEBI:
                return MUS_VS_MEW;
            case SPECIES_LUGIA:
                return HG_SEQ_GS_VS_LUGIA;
            case SPECIES_HO_OH:
                return HG_SEQ_GS_VS_HOUOU;
            case SPECIES_REGIROCK:
            case SPECIES_REGICE:
            case SPECIES_REGISTEEL:
                return MUS_VS_REGI;
            case SPECIES_GROUDON:
            case SPECIES_KYOGRE:
            case SPECIES_RAYQUAZA:
                return MUS_VS_KYOGRE_GROUDON;
            case SPECIES_DEOXYS:
                return MUS_RG_VS_DEOXYS;
            case SPECIES_ROTOM:
            case SPECIES_HEATRAN:
            case SPECIES_DIANCIE:
            case SPECIES_JIRACHI:
                return DP_SEQ_BA_SECRET2;
            case SPECIES_REGIGIGAS:
                return PL_SEQ_PL_BA_REGI;
            default:
                return MUS_VS_WILD;
        }
    }
}

void PlayBattleBGM(void) {
    ResetMapMusic();
    m4aMPlayAllStop();
    PlayBGM(GetBattleBGM());
}

void PlayMapChosenOrBattleBGM(u16 songId) {
    ResetMapMusic();
    m4aMPlayAllStop();
    if (songId)
        PlayNewMapMusic(songId);
    else
        PlayNewMapMusic(GetBattleBGM());
}

// Identical to PlayMapChosenOrBattleBGM, but uses a task instead
// Only used by Battle Dome
#define tSongId data[0]
void CreateTask_PlayMapChosenOrBattleBGM(u16 songId) {
    u8 taskId;

    ResetMapMusic();
    m4aMPlayAllStop();

    taskId = CreateTask(Task_PlayMapChosenOrBattleBGM, 0);
    gTasks[taskId].tSongId = songId;
}

static void Task_PlayMapChosenOrBattleBGM(u8 taskId) {
    if (gTasks[taskId].tSongId)
        PlayNewMapMusic(gTasks[taskId].tSongId);
    else
        PlayNewMapMusic(GetBattleBGM());
    DestroyTask(taskId);
}

#undef tSongId

const u32 *GetShinySpritePal(SpeciesEnum species, u32 isShiny) {
    u8 numShinies = gBaseStats[species].numShinies;
    switch (isShiny) {
        case SHINY_VANILLA:
            return gMonShinyPaletteTable[species].data;
            break;
        case SHINY_RARE:
            if (numShinies >= SHINY_RARE)
                return gMonRareShinyPaletteTable[species].data;
            else
                return gMonShinyPaletteTable[species].data;
            break;
        case SHINY_LEGENDARY:
            if (numShinies >= SHINY_LEGENDARY)
                return gMonLegendaryShinyPaletteTable[species].data;
            else
                return gMonShinyPaletteTable[species].data;
            break;
    }
    return gMonShinyPaletteTable[species].data;
}

const struct CompressedSpritePalette *GetShinySpritePalAddr(SpeciesEnum species, u32 isShiny) {
    u8 numShinies = gBaseStats[species].numShinies;
    switch (isShiny) {
        case SHINY_VANILLA:
            return &gMonShinyPaletteTable[species];
            break;
        case SHINY_RARE:
            if (numShinies >= SHINY_RARE)
                return &gMonRareShinyPaletteTable[species];
            else
                return &gMonShinyPaletteTable[species];
            break;
        case SHINY_LEGENDARY:
            if (numShinies >= SHINY_RARE)
                return &gMonLegendaryShinyPaletteTable[species];
            else
                return &gMonShinyPaletteTable[species];
            break;
    }
    return &gMonShinyPaletteTable[species];
}
const u32 *GetMonFrontSpritePal(struct Pokemon *mon) {
    u8 isShiny = GetMonData(mon, MON_DATA_IS_SHINY, 0);
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);

    if (isShiny) {
        if (SpeciesHasGenderDifference(species) && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return gMonShinyPaletteTableFemale[species].data;
        else
            return GetShinySpritePal(species, isShiny);
    } else {
        if (SpeciesHasGenderDifference(species) && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return gMonPaletteTableFemale[species].data;
        else
            return gMonPaletteTable[species].data;
    }
}

const u32 *GetMonSpritePal(SpeciesEnum species, u32 personality, u8 isShiny) {
    if (isShiny != SHINY_NONE) {
        if (SpeciesHasGenderDifference(species) && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return gMonShinyPaletteTableFemale[species].data;
        else
            return GetShinySpritePal(species, isShiny);
    } else {
        if (SpeciesHasGenderDifference(species) && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return gMonPaletteTableFemale[species].data;
        else
            return gMonPaletteTable[species].data;
    }
}

const struct CompressedSpritePalette *GetMonSpritePalStruct(struct Pokemon *mon) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES2, 0);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    u8 isShiny = GetMonData(mon, MON_DATA_IS_SHINY, 0);
    return GetMonSpritePalStructFromOtIdPersonality(species, personality, isShiny);
}

const struct CompressedSpritePalette *GetMonSpritePalStructFromOtIdPersonality(SpeciesEnum species, u32 personality, u8 isShiny) {
    if (isSpeciesPlaceholderMon(species)) species = SPECIES_NONE;

    if (isShiny != SHINY_NONE) {
        if (SpeciesHasGenderDifference(species) && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return &gMonShinyPaletteTableFemale[species];
        else
            return GetShinySpritePalAddr(species, isShiny);
    } else {
        if (SpeciesHasGenderDifference(species) && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
            return &gMonPaletteTableFemale[species];
        else
            return &gMonPaletteTable[species];
    }
}

bool32 IsHMMove2(u16 move) {
    int i = 0;
    while (sHMMoves[i] != 0xFFFF) {
        if (sHMMoves[i++] == move) return TRUE;
    }
    return FALSE;
}

bool8 IsMonSpriteNotFlipped(SpeciesEnum species) { return gBaseStats[species].noFlip; }

s8 GetMonFlavorRelation(struct Pokemon *mon, u8 flavor) {
    u8 nature = GetMonData(mon, MON_DATA_NATURE, NULL);
    return gPokeblockFlavorCompatibilityTable[nature * FLAVOR_COUNT + flavor];
}

s8 GetFlavorRelationByNature(u8 nature, u8 flavor)  // Used to be GetFlavorRelationByPersonality (u32 personality, u8 flavor)
{
    return gPokeblockFlavorCompatibilityTable[nature * FLAVOR_COUNT + flavor];
}

bool8 IsTradedMon(struct Pokemon *mon) {
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u32 otId;
    GetMonData(mon, MON_DATA_OT_NAME, otName);
    otId = GetMonData(mon, MON_DATA_OT_ID, 0);
    return IsOtherTrainer(otId, otName);
}

bool8 IsOtherTrainer(u32 otId, u8 *otName) {
    if (otId == (gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8) | (gSaveBlock2Ptr->playerTrainerId[2] << 16) |
                 (gSaveBlock2Ptr->playerTrainerId[3] << 24))) {
        int i;

        for (i = 0; otName[i] != EOS; i++)
            if (otName[i] != gSaveBlock2Ptr->playerName[i]) return TRUE;
        return FALSE;
    }

    return TRUE;
}

void MonRestorePP(struct Pokemon *mon) {
    int i;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (GetMonData(mon, MON_DATA_MOVE1 + i, 0)) {
            u16 move = GetMonData(mon, MON_DATA_MOVE1 + i, 0);
            u16 bonus = GetMonData(mon, MON_DATA_PP_BONUSES, 0);
            u8 pp = CalculatePPWithBonus(move, bonus, i);
            SetMonData(mon, MON_DATA_PP1 + i, &pp);
        }
    }
}

void SetMonPreventsSwitchingString(void) {
    if (GetBattlerSide(gBattleStruct->battlerPreventingSwitchout) == B_SIDE_PLAYER)
        GetMonNickname(&gPlayerParty[GetPartyIdFromBattlePartyId(gBattlerPartyIndexes[gBattleStruct->battlerPreventingSwitchout])], gStringVar1);
    else
        GetMonNickname(&gEnemyParty[gBattlerPartyIndexes[gBattleStruct->battlerPreventingSwitchout]], gStringVar1);

    StringExpandPlaceholders(gStringVar4, gText_PkmnsXPreventsSwitching);
}

static s32 GetWildMonTableIdInAlteringCave(SpeciesEnum species) {
    s32 i;
    for (i = 0; i < (s32)ARRAY_COUNT(sAlteringCaveWildMonHeldItems); i++)
        if (sAlteringCaveWildMonHeldItems[i].species == species) return i;
    return 0;
}

void SetWildMonHeldItem(void) {
    u16 rnd, species, var1, var2, i, count;

    if (gBattleTypeFlags & (BATTLE_TYPE_LEGENDARY | BATTLE_TYPE_TRAINER | BATTLE_TYPE_PYRAMID | BATTLE_TYPE_PIKE) || gDexnavBattle ||
        FlagGet(FLAG_TOTEM_BATTLE))
        return;

    count = (WILD_DOUBLE_BATTLE) ? 2 : 1;
    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG, 0) &&
        (GetMonAbility(&gPlayerParty[0]) == ABILITY_COMPOUND_EYES || GetMonAbility(&gPlayerParty[0]) == ABILITY_SUPER_LUCK)) {
        var1 = 20;
        var2 = 80;
    } else {
        var1 = 45;
        var2 = 95;
    }

    for (i = 0; i < count; i++) {
        rnd = Random() % 100;
        species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES, 0);
        if (gMapHeader.mapLayoutId == LAYOUT_ALTERING_CAVE) {
            s32 alteringCaveId = GetWildMonTableIdInAlteringCave(species);
            if (alteringCaveId != 0) {
                if (rnd < var2) continue;
                SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &sAlteringCaveWildMonHeldItems[alteringCaveId].item);
            } else {
                if (rnd < var1) continue;
                if (rnd < var2)
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item1);
                else
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item2);
            }
        } else {
            if (gBaseStats[species].item1 == gBaseStats[species].item2 && gBaseStats[species].item1 != 0) {
                SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item1);
            } else {
                if (rnd < var1) continue;
                if (rnd < var2)
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item1);
                else
                    SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &gBaseStats[species].item2);
            }
        }
    }
}

u8 IsMonShiny(struct Pokemon *mon) {
    u8 isShiny = GetMonData(mon, MON_DATA_IS_SHINY, NULL);
    return isShiny;
}

const u8 *GetTrainerPartnerName(void) {
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) {
        if (gPartnerTrainerId == TRAINER_STEVEN_PARTNER) {
            return gTrainers[TRAINER_STEVEN].trainerName;
        } else {
            GetFrontierTrainerName(gStringVar1, gPartnerTrainerId);
            return gStringVar1;
        }
    } else {
        u8 id = GetMultiplayerId();
        return gLinkPlayers[GetBattlerMultiplayerId(gLinkPlayers[id].id ^ 2)].name;
    }
}

#define READ_PTR_FROM_TASK(taskId, dataId) (void *)(((u16)(gTasks[taskId].data[dataId]) | ((u16)(gTasks[taskId].data[dataId + 1]) << 16)))

#define STORE_PTR_IN_TASK(ptr, taskId, dataId)              \
    {                                                       \
        gTasks[taskId].data[dataId] = (u32)(ptr);           \
        gTasks[taskId].data[dataId + 1] = (u32)(ptr) >> 16; \
    }

static void Task_AnimateAfterDelay(u8 taskId) {
    if (--gTasks[taskId].data[3] == 0) {
        LaunchAnimationTaskForFrontSprite(READ_PTR_FROM_TASK(taskId, 0), gTasks[taskId].data[2]);
        DestroyTask(taskId);
    }
}

static void Task_PokemonSummaryAnimateAfterDelay(u8 taskId) {
    if (--gTasks[taskId].data[3] == 0) {
        StartMonSummaryAnimation(READ_PTR_FROM_TASK(taskId, 0), gTasks[taskId].data[2]);
        SummaryScreen_SetAnimDelayTaskId(TASK_NONE);
        DestroyTask(taskId);
    }
}

void BattleAnimateFrontSprite(struct Sprite *sprite, SpeciesEnum species, bool8 noCry, u8 arg3) {
    if (gHitMarker & HITMARKER_NO_ANIMATIONS && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))
        DoMonFrontSpriteAnimation(sprite, species, noCry, arg3 | 0x80);
    else
        DoMonFrontSpriteAnimation(sprite, species, noCry, arg3);
}

void DoMonFrontSpriteAnimation(struct Sprite *sprite, SpeciesEnum species, bool8 noCry, u8 arg3) {
    s8 pan;
    switch (arg3 & 0x7F) {
        case 0:
            pan = -25;
            break;
        case 1:
            pan = 25;
            break;
        default:
            pan = 0;
            break;
    }
    if (arg3 & 0x80) {
        if (!noCry) PlayCry1(species, pan);
        sprite->callback = SpriteCallbackDummy;
    } else {
        if (!noCry) {
            PlayCry1(species, pan);
            if (HasTwoFramesAnimation(species)) StartSpriteAnim(sprite, 1);
        }
        if (sMonAnimationDelayTable[species - 1] != 0) {
            u8 taskId = CreateTask(Task_AnimateAfterDelay, 0);
            STORE_PTR_IN_TASK(sprite, taskId, 0);
            gTasks[taskId].data[2] = sMonFrontAnimIdsTable[species - 1];
            gTasks[taskId].data[3] = sMonAnimationDelayTable[species - 1];
        } else {
            LaunchAnimationTaskForFrontSprite(sprite, sMonFrontAnimIdsTable[species - 1]);
        }
        sprite->callback = SpriteCallbackDummy_2;
    }
}

void PokemonSummaryDoMonAnimation(struct Sprite *sprite, SpeciesEnum species, bool8 oneFrame) {
    if (!oneFrame && HasTwoFramesAnimation(species)) StartSpriteAnim(sprite, 1);
    if (sMonAnimationDelayTable[species - 1] != 0) {
        u8 taskId = CreateTask(Task_PokemonSummaryAnimateAfterDelay, 0);
        STORE_PTR_IN_TASK(sprite, taskId, 0);
        gTasks[taskId].data[2] = sMonFrontAnimIdsTable[species - 1];
        gTasks[taskId].data[3] = sMonAnimationDelayTable[species - 1];
        SummaryScreen_SetAnimDelayTaskId(taskId);
        SetSpriteCB_MonAnimDummy(sprite);
    } else {
        StartMonSummaryAnimation(sprite, sMonFrontAnimIdsTable[species - 1]);
    }
}

void StopPokemonAnimationDelayTask(void) {
    u8 delayTaskId = FindTaskIdByFunc(Task_PokemonSummaryAnimateAfterDelay);
    if (delayTaskId != TASK_NONE) DestroyTask(delayTaskId);
}

void BattleAnimateBackSprite(struct Sprite *sprite, SpeciesEnum species) {
    u8 speciesBackAnimSet = GetSpeciesBackAnimSet(species);

    if ((speciesBackAnimSet == 0) && (gHitMarker & HITMARKER_NO_ANIMATIONS && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)))) {
        sprite->callback = SpriteCallbackDummy;
    } else {
        LaunchAnimationTaskForBackSprite(sprite, speciesBackAnimSet);
        sprite->callback = SpriteCallbackDummy_2;
    }
}

u8 sub_806EF08(u8 arg0) {
    s32 i;
    s32 var = 0;
    u8 multiplayerId = GetMultiplayerId();
    switch (gLinkPlayers[multiplayerId].id) {
        case 0:
        case 2:
            var = (arg0 != 0) ? 1 : 3;
            break;
        case 1:
        case 3:
            var = (arg0 != 0) ? 2 : 0;
            break;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++) {
        if (gLinkPlayers[i].id == (s16)(var)) break;
    }
    return i;
}

u8 sub_806EF84(u8 arg0, u8 arg1) {
    s32 i;
    s32 var = 0;
    switch (gLinkPlayers[arg1].id) {
        case 0:
        case 2:
            var = (arg0 != 0) ? 1 : 3;
            break;
        case 1:
        case 3:
            var = (arg0 != 0) ? 2 : 0;
            break;
    }
    for (i = 0; i < MAX_LINK_PLAYERS; i++) {
        if (gLinkPlayers[i].id == (s16)(var)) break;
    }
    return i;
}

u16 FacilityClassToPicIndex(u16 facilityClass) { return gFacilityClassToPicIndex[facilityClass]; }

u16 PlayerGenderToFrontTrainerPicId(u8 playerGender) {
    if (playerGender != MALE)
        return FacilityClassToPicIndex(FACILITY_CLASS_MAY);
    else
        return FacilityClassToPicIndex(FACILITY_CLASS_BRENDAN);
}

void HandleSetPokedexFlag(u16 nationalNum, u8 caseId, u32 personality) {
    u8 getFlagCaseId = (caseId == FLAG_SET_SEEN) ? FLAG_GET_SEEN : FLAG_GET_CAUGHT;
    if (!GetSetPokedexFlag(nationalNum, getFlagCaseId))  // don't set if it's already set
    {
        GetSetPokedexFlag(nationalNum, caseId);
        if (NationalPokedexNumToSpecies(nationalNum) == SPECIES_UNOWN) gSaveBlock2Ptr->pokedex.unownPersonality = personality;
        if (NationalPokedexNumToSpecies(nationalNum) == SPECIES_SPINDA) gSaveBlock2Ptr->pokedex.spindaPersonality = personality;
    }
}

const u8 *GetTrainerClassNameFromId(u16 trainerId) {
    if (trainerId >= TRAINERS_COUNT) trainerId = TRAINER_NONE;
    return gTrainerClassNames[gTrainers[trainerId].trainerClass];
}

const u8 *GetTrainerNameFromId(u16 trainerId) {
    if (trainerId >= TRAINERS_COUNT) trainerId = TRAINER_NONE;
    return gTrainers[trainerId].trainerName;
}

bool8 HasTwoFramesAnimation(SpeciesEnum species) {
    return (species != SPECIES_CASTFORM && species != SPECIES_SPINDA && species != SPECIES_UNOWN && species != SPECIES_CHERRIM);
}

static bool8 ShouldSkipFriendshipChange(void) {
    if (gMain.inBattle && gBattleTypeFlags & (BATTLE_TYPE_FRONTIER)) return TRUE;
    if (!gMain.inBattle && (InBattlePike() || InBattlePyramid())) return TRUE;
    return FALSE;
}

static void sub_806F160(struct Unknown_806F160_Struct *structPtr) {
    u16 i, j;
    for (i = 0; i < structPtr->field_0_0; i++) {
        structPtr->templates[i] = gBattlerSpriteTemplates[i];
        for (j = 0; j < structPtr->field_1; j++) {
            structPtr->frameImages[i * structPtr->field_1 + j].data = &structPtr->byteArrays[i][j * 0x800];
        }
        structPtr->templates[i].images = &structPtr->frameImages[i * structPtr->field_1];
    }
}

static void sub_806F1FC(struct Unknown_806F160_Struct *structPtr) {
    u16 i, j;
    for (i = 0; i < structPtr->field_0_0; i++) {
        structPtr->templates[i] = gUnknown_08329F28;
        for (j = 0; j < structPtr->field_1; j++) {
            structPtr->frameImages[i * structPtr->field_0_0 + j].data = &structPtr->byteArrays[i][j * 0x800];
        }
        structPtr->templates[i].images = &structPtr->frameImages[i * structPtr->field_0_0];
        structPtr->templates[i].anims = gAnims_MonPic;
        structPtr->templates[i].paletteTag = i;
    }
}

struct Unknown_806F160_Struct *sub_806F2AC(u8 id, u8 arg1) {
    u8 i;
    u8 flags;
    struct Unknown_806F160_Struct *structPtr;

    flags = 0;
    id %= 2;
    structPtr = AllocZeroed(sizeof(*structPtr));
    if (structPtr == NULL) return NULL;

    switch (arg1) {
        case 2:
            structPtr->field_0_0 = 7;
            structPtr->field_0_1 = 7;
            structPtr->field_1 = 4;
            structPtr->field_3_0 = 1;
            structPtr->field_3_1 = 2;
            break;
        case 0:
        default:
            structPtr->field_0_0 = 4;
            structPtr->field_0_1 = 4;
            structPtr->field_1 = 4;
            structPtr->field_3_0 = 1;
            structPtr->field_3_1 = 0;
            break;
    }

    structPtr->bytes = AllocZeroed(structPtr->field_3_0 * 0x800 * 4 * structPtr->field_0_0);
    structPtr->byteArrays = AllocZeroed(structPtr->field_0_0 * 32);
    if (structPtr->bytes == NULL || structPtr->byteArrays == NULL) {
        flags |= 1;
    } else {
        for (i = 0; i < structPtr->field_0_0; i++) structPtr->byteArrays[i] = structPtr->bytes + (structPtr->field_3_0 * (i << 13));
    }

    structPtr->templates = AllocZeroed(sizeof(struct SpriteTemplate) * structPtr->field_0_0);
    structPtr->frameImages = AllocZeroed(sizeof(struct SpriteFrameImage) * structPtr->field_0_0 * structPtr->field_1);
    if (structPtr->templates == NULL || structPtr->frameImages == NULL) {
        flags |= 2;
    } else {
        for (i = 0; i < structPtr->field_1 * structPtr->field_0_0; i++) structPtr->frameImages[i].size = 0x800;

        switch (structPtr->field_3_1) {
            case 2:
                sub_806F1FC(structPtr);
                break;
            case 0:
            case 1:
            default:
                sub_806F160(structPtr);
                break;
        }
    }

    if (flags & 2) {
        if (structPtr->frameImages != NULL) FREE_AND_SET_NULL(structPtr->frameImages);
        if (structPtr->templates != NULL) FREE_AND_SET_NULL(structPtr->templates);
    }
    if (flags & 1) {
        if (structPtr->byteArrays != NULL) FREE_AND_SET_NULL(structPtr->byteArrays);
        if (structPtr->bytes != NULL) FREE_AND_SET_NULL(structPtr->bytes);
    }

    if (flags) {
        memset(structPtr, 0, sizeof(*structPtr));
        Free(structPtr);
    } else {
        structPtr->magic = 0xA3;
        gUnknown_020249B4[id] = structPtr;
    }

    return gUnknown_020249B4[id];
}

void sub_806F47C(u8 id) {
    struct Unknown_806F160_Struct *structPtr;

    id %= 2;
    structPtr = gUnknown_020249B4[id];
    if (structPtr == NULL) return;

    if (structPtr->magic != 0xA3) {
        memset(structPtr, 0, sizeof(struct Unknown_806F160_Struct));
    } else {
        if (structPtr->frameImages != NULL) FREE_AND_SET_NULL(structPtr->frameImages);
        if (structPtr->templates != NULL) FREE_AND_SET_NULL(structPtr->templates);
        if (structPtr->byteArrays != NULL) FREE_AND_SET_NULL(structPtr->byteArrays);
        if (structPtr->bytes != NULL) FREE_AND_SET_NULL(structPtr->bytes);

        memset(structPtr, 0, sizeof(struct Unknown_806F160_Struct));
        Free(structPtr);
    }
}

u8 *sub_806F4F8(u8 id, u8 arg1) {
    struct Unknown_806F160_Struct *structPtr = gUnknown_020249B4[id % 2];
    if (structPtr->magic != 0xA3) {
        return NULL;
    } else {
        if (arg1 >= structPtr->field_0_0) arg1 = 0;

        return structPtr->byteArrays[arg1];
    }
}

SpeciesEnum GetFormSpeciesId(SpeciesEnum speciesId, u8 formId) {
    if (gFormSpeciesIdTables[speciesId] != NULL)
        return gFormSpeciesIdTables[speciesId][formId];
    else
        return speciesId;
}

const SpeciesEnum *GetFormSpeciesTable(SpeciesEnum speciesId) {
    if (gFormSpeciesIdTables[speciesId] != NULL)
        return gFormSpeciesIdTables[speciesId];
    else
        return gFormSpeciesIdTables[SPECIES_NONE];
}

bool8 SpeciesHasDifferentForms(SpeciesEnum speciesId) { return gFormSpeciesIdTables[speciesId] != NULL; }

u8 GetFormIdFromFormSpeciesId(SpeciesEnum formSpeciesId) {
    u8 targetFormId = 0;

    if (gFormSpeciesIdTables[formSpeciesId] != NULL) {
        for (targetFormId = 0; gFormSpeciesIdTables[formSpeciesId][targetFormId]; targetFormId++) {
            if (formSpeciesId == gFormSpeciesIdTables[formSpeciesId][targetFormId]) break;
        }
    }
    return targetFormId;
}

// Get the latest badge the player has earned (unless they skipped Winona)
// League is counted as badge 9 for simplicity
static u16 getHighestBadge(void) {
    if (FlagGet(FLAG_SYS_GAME_CLEAR)) return 9;
    if (FlagGet(FLAG_BADGE08_GET)) return 8;
    if (FlagGet(FLAG_BADGE07_GET)) return 7;
    if (FlagGet(FLAG_BADGE06_GET)) return 6;
    if (FlagGet(FLAG_BADGE05_GET)) return 5;
    if (FlagGet(FLAG_BADGE04_GET)) return 4;
    if (FlagGet(FLAG_BADGE03_GET)) return 3;
    if (FlagGet(FLAG_BADGE02_GET)) return 2;
    if (FlagGet(FLAG_BADGE01_GET)) return 1;

    return 0;
}

u8 GetLevelCap(void) {
    u8 currentLevelCap;
    u8 levelCapSetting = gSaveBlock2Ptr->levelCaps;
    u16 currentBadge = getHighestBadge();

    static const u8 levelCapsStandard[] = {20, 28, 44, 55, 65, 80, 90, 101, 101, 101};  // Easy
    static const u8 levelCapsMore[] = {18, 25, 40, 50, 55, 70, 85, 92, 95, 101};        // More
    static const u8 levelCapsStrict[] = {16, 23, 36, 45, 50, 55, 60, 70, 80, 101};      // Elite

    switch (levelCapSetting) {
        default:
        case LEVEL_CAPS_EASY:
            currentLevelCap = levelCapsStandard[currentBadge];
            break;
        case LEVEL_CAPS_MORE:
            currentLevelCap = levelCapsMore[currentBadge];
            break;
        case LEVEL_CAPS_ELITE:
            currentLevelCap = levelCapsStrict[currentBadge];
            break;
    }
    return currentLevelCap;
}

static void ShuffleStatArray(u8 *statArray) {
    int i;

    // Shuffle the stats array using Fisher-Yates shuffle
    for (i = NUM_STATS - 1; i > 0; i--) {
        u8 temp;
        int j = Random() % (i + 1);
        SWAP(statArray[i], statArray[j], temp);
    }
}

// returns SPECIES_NONE if no form change is possible
SpeciesEnum GetFormChangeTargetSpecies(struct Pokemon *mon, u16 method, u32 arg) {
    u32 i;
    u16 targetSpecies = SPECIES_NONE;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    const struct FormChange *formChanges = gFormChangeTablePointers[species];
    u16 heldItem;
    AbilityEnum ability;

    if (formChanges != NULL) {
        heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
        ability = GetAbilityBySpecies(species, GetMonData(mon, MON_DATA_ABILITY_NUM, NULL));

        for (i = 0; formChanges[i].method != FORM_CHANGE_END; i++) {
            if (method == formChanges[i].method) {
                switch (method) {
                    case FORM_ITEM_HOLD:
                        if (heldItem == formChanges[i].param1) targetSpecies = formChanges[i].targetSpecies;
                        break;
                    case FORM_ITEM_USE:
                        if (arg == formChanges[i].param1) targetSpecies = formChanges[i].targetSpecies;
                        break;
                    case FORM_MOVE:
                        if (MonKnowsMove(mon, formChanges[i].param1) != formChanges[i].param2) targetSpecies = formChanges[i].targetSpecies;
                        break;
                    case FORM_ITEM_HOLD_ABILITY:
                        if (heldItem == formChanges[i].param1 && ability == formChanges[i].param2) targetSpecies = formChanges[i].targetSpecies;
                        break;
                    case FORM_ITEM_USE_TIME:
                        RtcCalcLocalTime();
                        if (arg == formChanges[i].param1) {
                            switch (formChanges[i].param2) {
                                case DAY:
                                    if (gLocalTime.hours >= 12 && gLocalTime.hours < 24) targetSpecies = formChanges[i].targetSpecies;
                                    break;
                                case NIGHT:
                                    if (gLocalTime.hours >= 0 && gLocalTime.hours < 12) targetSpecies = formChanges[i].targetSpecies;
                                    break;
                            }
                        }
                        break;
                }
            }
        }
    }

    return species != targetSpecies ? targetSpecies : SPECIES_NONE;
}

u16 getShinyOdds(void) {
    if (gSaveBlock2Ptr->shinyrate == 1)
        return SHINY_ODDS_1;
    else if (gSaveBlock2Ptr->shinyrate == 2)
        return SHINY_ODDS_2;
    else
        return SHINY_ODDS;
}

u16 getRandomSpecies(void) {
    static const u16 Obt[][1] = {
        {SPECIES_ARTICUNO},  {SPECIES_ZAPDOS},    {SPECIES_MOLTRES},     {SPECIES_MEWTWO},    {SPECIES_MEW},       {SPECIES_LUGIA},     {SPECIES_HO_OH},
        {SPECIES_REGIROCK},  {SPECIES_REGICE},    {SPECIES_REGISTEEL},   {SPECIES_LATIAS},    {SPECIES_LATIOS},    {SPECIES_KYOGRE},    {SPECIES_GROUDON},
        {SPECIES_RAYQUAZA},  {SPECIES_JIRACHI},   {SPECIES_DEOXYS},      {SPECIES_HEATRAN},   {SPECIES_REGIGIGAS}, {SPECIES_DIANCIE},   {SPECIES_COSMOG},
        {SPECIES_MAGEARNA},  {SPECIES_MELTAN},    {SPECIES_RAIKOU},      {SPECIES_ENTEI},     {SPECIES_SUICUNE},   {SPECIES_CELEBI},    {SPECIES_UXIE},
        {SPECIES_MESPRIT},   {SPECIES_AZELF},     {SPECIES_DIALGA},      {SPECIES_PALKIA},    {SPECIES_GIRATINA},  {SPECIES_CRESSELIA}, {SPECIES_MANAPHY},
        {SPECIES_DARKRAI},   {SPECIES_SHAYMIN},   {SPECIES_ARCEUS},      {SPECIES_VICTINI},   {SPECIES_COBALION},  {SPECIES_TERRAKION}, {SPECIES_VIRIZION},
        {SPECIES_TORNADUS},  {SPECIES_THUNDURUS}, {SPECIES_RESHIRAM},    {SPECIES_ZEKROM},    {SPECIES_LANDORUS},  {SPECIES_KYUREM},    {SPECIES_KELDEO},
        {SPECIES_GENESECT},  {SPECIES_XERNEAS},   {SPECIES_YVELTAL},     {SPECIES_ZYGARDE},   {SPECIES_HOOPA},     {SPECIES_VOLCANION}, {SPECIES_TYPE_NULL},
        {SPECIES_SILVALLY},  {SPECIES_TAPU_KOKO}, {SPECIES_TAPU_LELE},   {SPECIES_TAPU_BULU}, {SPECIES_TAPU_FINI}, {SPECIES_NIHILEGO},  {SPECIES_BUZZWOLE},
        {SPECIES_PHEROMOSA}, {SPECIES_XURKITREE}, {SPECIES_CELESTEELA},  {SPECIES_KARTANA},   {SPECIES_GUZZLORD},  {SPECIES_NECROZMA},  {SPECIES_MARSHADOW},
        {SPECIES_NAGANADEL}, {SPECIES_STAKATAKA}, {SPECIES_BLACEPHALON}, {SPECIES_ZERAORA},
    };

    static const u16 ObtLegends[][1] = {{SPECIES_ARTICUNO},  {SPECIES_ZAPDOS},   {SPECIES_MOLTRES},  {SPECIES_MEWTWO},    {SPECIES_MEW},      {SPECIES_LUGIA},
                                        {SPECIES_HO_OH},     {SPECIES_REGIROCK}, {SPECIES_REGICE},   {SPECIES_REGISTEEL}, {SPECIES_LATIAS},   {SPECIES_LATIOS},
                                        {SPECIES_KYOGRE},    {SPECIES_GROUDON},  {SPECIES_RAYQUAZA}, {SPECIES_JIRACHI},   {SPECIES_DEOXYS},   {SPECIES_HEATRAN},
                                        {SPECIES_REGIGIGAS}, {SPECIES_MELOETTA}, {SPECIES_DIANCIE},  {SPECIES_COSMOG},    {SPECIES_MAGEARNA}, {SPECIES_MELTAN}};

    static const u16 NonObt[][1] = {
        /*
    {SPECIES_UNOWN},
    {SPECIES_GROOKEY},
    {SPECIES_SCORBUNNY},
    {SPECIES_SOBBLE},
    {SPECIES_SKWOVET},
    {SPECIES_ROOKIDEE},
    {SPECIES_BLIPBUG},
    {SPECIES_NICKIT},
    {SPECIES_GOSSIFLEUR},
    {SPECIES_WOOLOO},
    {SPECIES_CHEWTLE},
    {SPECIES_YAMPER},
    {SPECIES_ROLYCOLY},
    {SPECIES_APPLIN},
    {SPECIES_SILICOBRA},
    {SPECIES_CRAMORANT},
    {SPECIES_ARROKUDA},
    {SPECIES_TOXEL},
    {SPECIES_SIZZLIPEDE},
    {SPECIES_CLOBBOPUS},
    {SPECIES_SINISTEA},
    {SPECIES_HATENNA},
    {SPECIES_IMPIDIMP},
    {SPECIES_MILCERY},
    {SPECIES_FALINKS},
    {SPECIES_PINCURCHIN},
    {SPECIES_SNOM},
    {SPECIES_STONJOURNER},
    {SPECIES_EISCUE},
    {SPECIES_INDEEDEE},
    {SPECIES_MORPEKO},
    {SPECIES_CUFANT},
    {SPECIES_DRACOZOLT},
    {SPECIES_ARCTOZOLT},
    {SPECIES_DRACOVISH},
    {SPECIES_ARCTOVISH},
    {SPECIES_DURALUDON},
    {SPECIES_DREEPY}
    */
    };

    static const u16 NonObtLegends[][1] = {
        /*
    {SPECIES_RAIKOU},
    {SPECIES_ENTEI},
    {SPECIES_SUICUNE},
    {SPECIES_CELEBI},
    {SPECIES_UXIE},
    {SPECIES_MESPRIT},
    {SPECIES_AZELF},
    {SPECIES_DIALGA},
    {SPECIES_PALKIA},
    {SPECIES_GIRATINA},
    {SPECIES_CRESSELIA},
    {SPECIES_PHIONE},
    {SPECIES_MANAPHY},
    {SPECIES_DARKRAI},
    {SPECIES_SHAYMIN},
    {SPECIES_ARCEUS},
    {SPECIES_VICTINI},
    {SPECIES_COBALION},
    {SPECIES_TERRAKION},
    {SPECIES_VIRIZION},
    {SPECIES_TORNADUS},
    {SPECIES_THUNDURUS},
    {SPECIES_RESHIRAM},
    {SPECIES_ZEKROM},
    {SPECIES_LANDORUS},
    {SPECIES_KYUREM},
    {SPECIES_KELDEO},
    {SPECIES_GENESECT},
    {SPECIES_XERNEAS},
    {SPECIES_YVELTAL},
    {SPECIES_ZYGARDE},
    {SPECIES_HOOPA},
    {SPECIES_VOLCANION},
    {SPECIES_TYPE_NULL},
    {SPECIES_SILVALLY},
    {SPECIES_TAPU_KOKO},
    {SPECIES_TAPU_LELE},
    {SPECIES_TAPU_BULU},
    {SPECIES_TAPU_FINI},
    {SPECIES_NIHILEGO},
    {SPECIES_BUZZWOLE},
    {SPECIES_PHEROMOSA},
    {SPECIES_XURKITREE},
    {SPECIES_CELESTEELA},
    {SPECIES_KARTANA},
    {SPECIES_GUZZLORD},
    {SPECIES_NECROZMA},
    {SPECIES_MARSHADOW},
    {SPECIES_POIPOLE},
    {SPECIES_NAGANADEL},
    {SPECIES_STAKATAKA},
    {SPECIES_BLACEPHALON},
    {SPECIES_ZERAORA},
    {SPECIES_ZACIAN},
    {SPECIES_ZAMAZENTA},
    {SPECIES_ETERNATUS},
    {SPECIES_KUBFU},
    {SPECIES_ZARUDE},
    {SPECIES_REGIELEKI},
    {SPECIES_REGIDRAGO},
    {SPECIES_GLASTRIER},
    {SPECIES_SPECTRIER},
    {SPECIES_CALYREX}
    */
    };

    u16 numObt = ARRAY_COUNT(Obt);
    u16 numObtLegends = ARRAY_COUNT(ObtLegends);
    u16 numNonObt = ARRAY_COUNT(NonObt);
    u16 numNonObtLegends = ARRAY_COUNT(NonObtLegends);
    u8 ObtLegendsEnabled = 0;
    u8 NonObtEnabled = 0;
    u8 NonObtLegendsEnabled = 0;
    u16 num = numObt;
    u8 altForms = 0;
    u16 randSpecies = 0;
    u16 Rand = 0;
    SpeciesEnum species = 0;
    u8 countAltForms = 0;

    if (GetBoxMonDataAt(TOTAL_BOXES_COUNT - 1, IN_BOX_COUNT - 1, MON_DATA_CUTE) == 1)  // Legendary enabled
    {
        num += numObtLegends;
        ObtLegendsEnabled = 1;
    }
    if (GetBoxMonDataAt(TOTAL_BOXES_COUNT - 1, IN_BOX_COUNT - 1, MON_DATA_TOUGH) == 1)  // Unobtainable enbaled
    {
        num += numNonObt;
        NonObtEnabled = 1;
        if (ObtLegendsEnabled == 1) {
            num += numNonObtLegends;
            NonObtLegendsEnabled = 1;
        }
    }
    if (GetBoxMonDataAt(TOTAL_BOXES_COUNT - 1, IN_BOX_COUNT - 1, MON_DATA_SHEEN) == 1)  // Alt Forms enbaled
        altForms = 1;

    Rand = Random() % num;
    if (Rand < numObt) {
        randSpecies = Random() % numObt;
        species = Obt[randSpecies][0];
    } else if (Rand < (numObt + numNonObt) && NonObtEnabled == 1)  // Unobtainable enbaled
    {
        randSpecies = Random() % numNonObt;
        species = NonObt[randSpecies][0];
    } else if (Rand < (numObt + numNonObt + numNonObtLegends) && NonObtLegendsEnabled == 1)  // Unobtainable&Legendary enbaled
    {
        randSpecies = Random() % numNonObtLegends;
        species = NonObtLegends[randSpecies][0];
    } else if (ObtLegendsEnabled == 1) {
        randSpecies = Random() % numObtLegends;
        species = ObtLegends[randSpecies][0];
    } else {
        randSpecies = Random() % numObt;
        species = Obt[randSpecies][0];
    }

    if (altForms == 1) {
        if (species == SPECIES_MEOWTH && NonObtEnabled != 1)  // SPECIES_MEOWTH_GALARIAN exception
        {
            if (Random() % 2 == 1) species = SPECIES_MEOWTH_ALOLAN;
        } else if (species == SPECIES_NECROZMA)  // SPECIES_NECROZMA_ULTRA exception
        {
            Rand = Random() % 3;
            if (Rand == 1)
                species = SPECIES_NECROZMA_DUSK_MANE;
            else if (Rand == 2)
                species = SPECIES_NECROZMA_DAWN_WINGS;
        } else if (species == SPECIES_RATTATA || species == SPECIES_SANDSHREW || species == SPECIES_VULPIX || species == SPECIES_DIGLETT ||
                   species == SPECIES_MEOWTH || species == SPECIES_GEODUDE || species == SPECIES_GRIMER || species == SPECIES_PUMPKABOO ||
                   species == SPECIES_ORICORIO ||
                   ((species == SPECIES_PONYTA || species == SPECIES_SLOWPOKE || species == SPECIES_FARFETCHD || species == SPECIES_CORSOLA ||
                     species == SPECIES_ZIGZAGOON || species == SPECIES_SHELLOS || species == SPECIES_BASCULIN || species == SPECIES_DARUMAKA ||
                     species == SPECIES_YAMASK || species == SPECIES_STUNFISK || species == SPECIES_SINISTEA || species == SPECIES_INDEEDEE) &&
                    NonObtEnabled == 1) ||
                   ((species == SPECIES_ARTICUNO || species == SPECIES_ZAPDOS || species == SPECIES_MOLTRES || species == SPECIES_GIRATINA ||
                     species == SPECIES_SHAYMIN || species == SPECIES_ARCEUS || species == SPECIES_TORNADUS || species == SPECIES_THUNDURUS ||
                     species == SPECIES_LANDORUS || species == SPECIES_KYUREM || species == SPECIES_KELDEO || species == SPECIES_HOOPA ||
                     species == SPECIES_SILVALLY || species == SPECIES_MAGEARNA || species == SPECIES_CALYREX) &&
                    NonObtLegendsEnabled == 1)) {
            for (countAltForms = 0; gFormSpeciesIdTables[species][countAltForms]; countAltForms++) {
            }
            randSpecies = Random() % countAltForms;
            species = gFormSpeciesIdTables[species][randSpecies];
        }
    }
    return species;
}

bool8 SpeciesHasInnate(SpeciesEnum species, AbilityEnum ability, u8 level, u32 personality, bool8 disablerandomizer, bool8 isEnemyMon) {
    AbilityEnum innate1 = gBaseStats[species].innates[0];
    AbilityEnum innate2 = gBaseStats[species].innates[1];
    AbilityEnum innate3 = gBaseStats[species].innates[2];

    if (!disablerandomizer) {
        innate1 = RandomizeInnate(gBaseStats[species].innates[0], species, personality);
        innate2 = RandomizeInnate(gBaseStats[species].innates[1], species, personality);
        innate3 = RandomizeInnate(gBaseStats[species].innates[2], species, personality);
    }

    if (innate1 == ability && (isEnemyMon || !CanDisableInnates() || level >= getInnateDisableLevel(SPECIES_INNATE_NUM_1)))
        return TRUE;
    else if (innate2 == ability && (isEnemyMon || !CanDisableInnates() || level >= getInnateDisableLevel(SPECIES_INNATE_NUM_2)))
        return TRUE;
    else if (innate3 == ability && (isEnemyMon || !CanDisableInnates() || level >= getInnateDisableLevel(SPECIES_INNATE_NUM_3)))
        return TRUE;
    else
        return FALSE;
}

MoveEnum RandomizeMoves(MoveEnum moves, SpeciesEnum species, u32 personality) {
    if (gSaveBlock2Ptr->moveRandomizedMode == 1 && moves != MOVE_NONE) {
        u32 randomizedMoveSeed = moves ^ ISO_RANDOMIZE1(species) ^ personality;
        MoveEnum randomizedMove;
        do {
            randomizedMove = RandRangeDeterministic(1, MOVES_COUNT - 1, &randomizedMoveSeed);
        } while (gBattleMoves[randomizedMove].effect == EFFECT_PLACEHOLDER || randomizedMove == MOVE_DARK_VOID);

        return randomizedMove;
    } else
        return moves;
}

AbilityEnum RandomizeInnate(AbilityEnum innate, SpeciesEnum species, u32 personality) {
    if (gSaveBlock2Ptr->innaterandomizedMode == 1 && !gAbilities[innate].randomizerBanned) {
        // Only Randomize if you have the Innate Randomized Mode Enabled
        // Exclude form change abilities from being randomized and other mons can't get them either
        u32 randomizedInnateSeed = (innate ^ ISO_RANDOMIZE1(species) ^ personality);
        AbilityEnum randomizedInnate;
        do {
            randomizedInnate = RandRangeDeterministic(1, ABILITIES_COUNT - 1, &randomizedInnateSeed);
        } while (gAbilities[randomizedInnate].randomizerBanned);
        return randomizedInnate;
    } else
        return innate;
}

// #define BALANCE_RANDOMIZER_ABILITIES

AbilityEnum RandomizeAbility(AbilityEnum ability, SpeciesEnum species, u32 personality) {
    if (gSaveBlock2Ptr->abilityRandomizedMode == 1 && !gAbilities[ability].randomizerBanned) {
        // Only Randomize if you have the Ability Randomized Mode Enabled
        // Exclude form change abilities from being randomized and other mons can't get them either
        u32 randomizedAbilitySeed = ability ^ ISO_RANDOMIZE1(species) ^ personality;
        AbilityEnum randomizedAbility;
        do {
            randomizedAbility = RandRangeDeterministic(1, ABILITIES_COUNT - 1, &randomizedAbilitySeed);
        } while (gAbilities[randomizedAbility].randomizerBanned);
        return randomizedAbility;
    } else
        return ability;
}

u8 RandomizeType(u8 type, SpeciesEnum species, u32 personality, bool8 isFirstType) {
    if (gSaveBlock2Ptr->typeRandomizedMode == 1 && type != TYPE_MYSTERY) {
        // Only Randomize if you have the Type Randomized Mode Enabled
        // Exclude form change abilities from being randomized and other mons can't get them either
        u32 randomizedTypeSeed = type ^ ISO_RANDOMIZE1(species) ^ personality ^ isFirstType;
        u8 randomizedType;

        do {
            randomizedType = RandRangeDeterministic(0, NUMBER_OF_MON_TYPES - 2, &randomizedTypeSeed);
        } while (randomizedType == TYPE_MYSTERY || randomizedType == type);
        return randomizedType;
    } else
        return type;
}

AbilityEnum GetMonInnate(struct Pokemon *mon, int slot, int disableRandomizer) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    int personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    int level = GetMonData(mon, MON_DATA_LEVEL, NULL);

    AbilityEnum innate = gBaseStats[species].innates[slot];

    if (!disableRandomizer)
        innate = RandomizeInnate(innate, species, personality);

    //Disable Innate if it does not meet the level requirments
    if (!disableRandomizer && (CanDisableInnates() && level < getInnateDisableLevel(slot)))
        return ABILITY_NONE;

    return innate;
}

bool8 MonHasInnate(struct Pokemon *mon, AbilityEnum ability, bool8 disableRandomizer) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, NULL);

    return SpeciesHasInnate(species, ability, level, personality, disableRandomizer, disableRandomizer);
}

bool8 BoxMonHasInnate(struct BoxPokemon *boxmon, AbilityEnum ability, bool8 disableRandomizer) {
    SpeciesEnum species = GetBoxMonData(boxmon, MON_DATA_SPECIES, NULL);
    u32 personality = GetBoxMonData(boxmon, MON_DATA_PERSONALITY, NULL);
    u8 level = GetBoxMonData(boxmon, MON_DATA_LEVEL, NULL);

    return SpeciesHasInnate(species, ability, level, personality, disableRandomizer, disableRandomizer);
}

u8 GetSpeciesInnateNum(SpeciesEnum species, AbilityEnum ability, u8 level, u32 personality, bool8 disablerandomizer) {
    AbilityEnum innate1 = gBaseStats[species].innates[0];
    AbilityEnum innate2 = gBaseStats[species].innates[1];
    AbilityEnum innate3 = gBaseStats[species].innates[2];

    if (!disablerandomizer) {
        innate1 = RandomizeInnate(gBaseStats[species].innates[0], species, personality);
        innate2 = RandomizeInnate(gBaseStats[species].innates[1], species, personality);
        innate3 = RandomizeInnate(gBaseStats[species].innates[2], species, personality);
    }

    if (innate1 == ability)
        return 0;
    else if (innate2 == ability)
        return 1;
    else if (innate3 == ability)
        return 2;
    else
        return 3;
}

void CreateShinyMonWithNature(struct Pokemon *mon, SpeciesEnum species, u8 level, u8 nature) {
    u32 personality;
    u32 otid = gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8) | (gSaveBlock2Ptr->playerTrainerId[2] << 16) |
               (gSaveBlock2Ptr->playerTrainerId[3] << 24);
    do {
        personality = Random32();
        personality = ((((Random() % 8) ^ (HIHALF(otid) ^ LOHALF(otid))) ^ LOHALF(personality)) << 16) | LOHALF(personality);
    } while (nature != GetNatureFromPersonality(personality));

    CreateMon(mon, species, level, 32, 1, personality, OT_ID_PRESET, otid);
}

// replaced by GetTrainerBattleWins()
u16 getNumberOfUniqueDefeatedTrainers(void) {
    u16 defeatedTrainers = 0;
    u16 i;

    for (i = 0; i < TRAINERS_COUNT; i++) {
        if (i <= MAX_OLD_TRAINERS_COUNT || i == TRAINER_OLDPLAYER) {
            if (FlagGet(TRAINER_FLAGS_START + i)) defeatedTrainers++;
        } else {
            if (FlagGet(gTrainers[i].trainerFlag)) defeatedTrainers++;
        }
    }

    return defeatedTrainers;
}

bool8 enablePokemonChanges(void) {
    bool8 enable = TRUE;

    if (FlagGet(FLAG_SYS_LOCKED_MODE)) {
        switch (gSaveBlock1Ptr->location.mapNum) {
            case MAP_NUM(EVER_GRANDE_CITY_SIDNEYS_ROOM):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_SIDNEYS_ROOM)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_PHOEBES_ROOM):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_PHOEBES_ROOM)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_GLACIAS_ROOM):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_GLACIAS_ROOM)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_DRAKES_ROOM):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_DRAKES_ROOM)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_CHAMPIONS_ROOM):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_CHAMPIONS_ROOM)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_HALL1):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL1)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_HALL2):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL2)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_HALL3):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL3)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_HALL4):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL4)) enable = FALSE;
                break;
            case MAP_NUM(EVER_GRANDE_CITY_HALL5):
                if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL5)) enable = FALSE;
                break;
        }
    }

    return enable;
}

bool8 isMonNicknamed(struct Pokemon *mon) {
    u8 i;
    bool8 nicknamed = FALSE;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 strArray[POKEMON_SPECIES_NAME_LENGTH + 1];
    u8 strNickname[POKEMON_NAME_LENGTH + 1];

    StringCopy(strArray, gSpeciesNames[species]);
    GetMonData(mon, MON_DATA_NICKNAME, strNickname);

    for (i = 0; i < POKEMON_NAME_LENGTH; i++) {
        if (strNickname[i] != strArray[i]) {
            nicknamed = TRUE;
        }
    }

    return nicknamed;
}

bool8 isBoxMonNicknamed(struct BoxPokemon *boxMon) {
    u8 i;
    bool8 nicknamed = FALSE;
    SpeciesEnum species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    u8 strArray[POKEMON_SPECIES_NAME_LENGTH + 1];
    u8 strNickname[POKEMON_NAME_LENGTH + 1];

    StringCopy(strArray, gSpeciesNames[species]);
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, strNickname);

    for (i = 0; i < POKEMON_NAME_LENGTH; i++) {
        if (strNickname[i] != strArray[i]) {
            nicknamed = TRUE;
        }
    }

    return nicknamed;
}

u16 GetRandomPokemonFromTag(u16 rndseed, s8 loc, s8 locG) {
    u8 i, j = 0;
    u32 tags = 0;
    u32 mask = getMask(loc, locG);
    u8 tag;
    if (mask == 0) {
        return ARRAY_MODULO(gAllFirstStage_species, rndseed);
    }
    for (i = 0; i < 21; i++) {
        if ((mask >> i) & 1) {
            tags = (tags << 5) + i;
            j++;
        }
    }
    tag = (tags >> ((rndseed % j) * 5)) & 31;
    mask = rndseed;
    if (RandRangeDeterministic(0, 8, &mask) == 0) {
        return GetRandomPokemonFromDiffTag(rndseed, tags, j, getTier(loc, locG));
    } else {
        return tagSwitch(tag, rndseed);
    }
}

u16 GetRandomPokemonFromDiffTag(u16 rndseed, u32 tags, u8 total, u8 tier) {
    u32 rand = rndseed;
    u8 tag, cur, i = 0;
    u16 mon;
    u8 mon_tier;
    do {
        tag = RandRangeDeterministic(0, 21, &rand);
        for (i = 0; i < total; i++) {
            cur = tags >> (i * 5);
            if (cur == tag) {
                continue;
            }
        }
        mon = tagSwitch(tag, rand >> 16);
        mon_tier = gBaseStats[mon].tier;
        if (tier == 2 && mon_tier == 1) {
            mon_tier = 2;
        }
        if (tier == 1 && mon_tier == 2) {
            mon_tier = 1;
        }
    } while (mon_tier != tier);
    return mon;
}

u16 tagSwitch(u8 tag, u16 rndseed) {
    switch (tag) {
        case 0:
            return ARRAY_MODULO(gForest_species, rndseed);
        case 1:
            return ARRAY_MODULO(gField_species, rndseed);
        case 2:
            return ARRAY_MODULO(gAquatic_species, rndseed);
        case 3:
            return ARRAY_MODULO(gCave_species, rndseed);
        case 4:
            return ARRAY_MODULO(gCold_species, rndseed);
        case 5:
            return ARRAY_MODULO(gHot_species, rndseed);
        case 6:
            return ARRAY_MODULO(gDark_species, rndseed);
        case 7:
            return ARRAY_MODULO(gDeep_species, rndseed);
        case 8:
            return ARRAY_MODULO(gFossil_species, rndseed);
        case 9:
            return ARRAY_MODULO(gAsh_species, rndseed);
        case 10:
            return ARRAY_MODULO(gHaunted_species, rndseed);
        case 11:
            return ARRAY_MODULO(gTropical_species, rndseed);
        case 12:
            return ARRAY_MODULO(gDesert_species, rndseed);
        case 13:
            return ARRAY_MODULO(gTown_species, rndseed);
        case 14:
            return ARRAY_MODULO(gVolcano_species, rndseed);
        case 15:
            return ARRAY_MODULO(gMountain_species, rndseed);
        case 16:
            return ARRAY_MODULO(gSpace_species, rndseed);
        case 17:
            return ARRAY_MODULO(gMonster_species, rndseed);
        case 18:
            return ARRAY_MODULO(gMineral_species, rndseed);
        case 19:
            return ARRAY_MODULO(gHumanlike_species, rndseed);
        case 20:
            return ARRAY_MODULO(gBeach_species, rndseed);
    }
    return 0;
}
u16 GetRandomStarter(u8 gen, bool8 enc, bool8 leg, u8 starterID) {
    u32 rndSeed = VarGet(VAR_RANDOMIZED_SEED);
    u16 min, max = 0;
    SpeciesEnum species;
    if (rndSeed == 0) {
        u16 newseed = Random();
        VarSet(VAR_RANDOMIZED_SEED, newseed);
        rndSeed = VarGet(VAR_RANDOMIZED_SEED);
    }
    rndSeed ^= starterID;
    getGenRange(gen, &min, &max);
    if (!enc && leg) {
        do {
            species = RandRangeDeterministic(min, max, &rndSeed);
        } while (gBaseStats[species].tier != 3);
    } else if (enc && leg) {
        return RandRangeDeterministic(min, max, &rndSeed);
    } else {
        do {
            species = RandRangeDeterministic(min, max, &rndSeed);
        } while (species == SPECIES_ARTICUNO || species == SPECIES_ZAPDOS || species == SPECIES_MOLTRES || species == SPECIES_RAIKOU ||
                 species == SPECIES_ENTEI || species == SPECIES_SUICUNE || species == SPECIES_REGICE || species == SPECIES_REGIROCK ||
                 species == SPECIES_REGISTEEL || species == SPECIES_LATIAS || species == SPECIES_LATIOS || species == SPECIES_UXIE ||
                 species == SPECIES_MESPRIT || species == SPECIES_AZELF || species == SPECIES_HEATRAN || species == SPECIES_REGIGIGAS ||
                 species == SPECIES_CRESSELIA || species == SPECIES_COBALION || species == SPECIES_TERRAKION || species == SPECIES_VIRIZION ||
                 species == SPECIES_TORNADUS || species == SPECIES_THUNDURUS || species == SPECIES_TYPE_NULL || species == SPECIES_SILVALLY ||
                 species == SPECIES_TAPU_KOKO || species == SPECIES_TAPU_LELE || species == SPECIES_TAPU_BULU || species == SPECIES_TAPU_FINI ||
                 species == SPECIES_NIHILEGO || species == SPECIES_BUZZWOLE || species == SPECIES_PHEROMOSA || species == SPECIES_XURKITREE ||
                 species == SPECIES_CELESTEELA || species == SPECIES_KARTANA || species == SPECIES_GUZZLORD || species == SPECIES_POIPOLE ||
                 species == SPECIES_NAGANADEL || species == SPECIES_STAKATAKA || species == SPECIES_BLACEPHALON ||
                 // Legendary
                 species == SPECIES_MEWTWO || species == SPECIES_LUGIA || species == SPECIES_HO_OH || species == SPECIES_KYOGRE || species == SPECIES_GROUDON ||
                 species == SPECIES_RAYQUAZA || species == SPECIES_DIALGA || species == SPECIES_PALKIA || species == SPECIES_GIRATINA ||
                 species == SPECIES_RESHIRAM || species == SPECIES_ZEKROM || species == SPECIES_KYUREM || species == SPECIES_XERNEAS ||
                 species == SPECIES_YVELTAL || species == SPECIES_ZYGARDE || species == SPECIES_COSMOG || species == SPECIES_COSMOEM ||
                 species == SPECIES_SOLGALEO || species == SPECIES_LUNALA || species == SPECIES_NECROZMA ||
                 // Mythical
                 species == SPECIES_MEW || species == SPECIES_CELEBI || species == SPECIES_JIRACHI || species == SPECIES_DEOXYS || species == SPECIES_PHIONE ||
                 species == SPECIES_MANAPHY || species == SPECIES_DARKRAI || species == SPECIES_SHAYMIN || species == SPECIES_ARCEUS ||
                 species == SPECIES_VICTINI || species == SPECIES_KELDEO || species == SPECIES_MELOETTA || species == SPECIES_GENESECT ||
                 species == SPECIES_DIANCIE || species == SPECIES_HOOPA || species == SPECIES_VOLCANION || species == SPECIES_MAGEARNA ||
                 species == SPECIES_MARSHADOW || species == SPECIES_ZERAORA || species == SPECIES_MELTAN || species == SPECIES_MELMETAL ||
                 species == SPECIES_ZACIAN ||                      // Unfinished
                 species == SPECIES_ZAMAZENTA ||                   // Unfinished
                 species == SPECIES_ETERNATUS ||                   // Unfinished
                 species == SPECIES_KUBFU ||                       // Unfinished
                 species == SPECIES_URSHIFU ||                     // Unfinished
                 species == SPECIES_ZARUDE ||                      // Unfinished
                 species == SPECIES_REGIELEKI ||                   // Unfinished
                 species == SPECIES_REGIDRAGO ||                   // Unfinished
                 species == SPECIES_GLASTRIER ||                   // Unfinished
                 species == SPECIES_SPECTRIER ||                   // Unfinished
                 species == SPECIES_CALYREX ||                     // Unfinished
                 species == SPECIES_ZACIAN_CROWNED_SWORD ||        // Unfinished
                 species == SPECIES_ZAMAZENTA_CROWNED_SHIELD ||    // Unfinished
                 species == SPECIES_ETERNATUS_ETERNAMAX ||         // Unfinished
                 species == SPECIES_URSHIFU_RAPID_STRIKE_STYLE ||  // Unfinished
                 species == SPECIES_ZARUDE_DADA);
    }
    return species;
}
#define GEN1_START SPECIES_BULBASAUR
#define GEN2_START SPECIES_CHIKORITA
#define GEN3_START SPECIES_TREECKO
#define GEN4_START SPECIES_TURTWIG
#define GEN5_START SPECIES_SNIVY
#define GEN6_START SPECIES_CHESPIN
#define GEN7_START SPECIES_ROWLET
#define GEN8_START SPECIES_GROOKEY
#define GEN9_START SPECIES_SPRIGATITO

void getGenRange(u8 gen, u16 *min, u16 *max) {
    switch (gen) {
        case 0:
        case 3:
            *min = GEN3_START;
            *max = GEN4_START;
            break;
        case 1:
            *min = GEN1_START;
            *max = GEN2_START;
            break;
        case 2:
            *min = GEN2_START;
            *max = GEN3_START;
            break;
        case 4:
            *min = GEN4_START;
            *max = GEN5_START;
            break;
        case 5:
            *min = GEN5_START;
            *max = GEN6_START;
            break;
        case 6:
            *min = GEN6_START;
            *max = GEN7_START;
            break;
        case 7:
            *min = GEN7_START;
            *max = GEN8_START;
            break;
        case 8:
            *min = GEN8_START;
            *max = GEN9_START;
            break;
        case 9:
            *min = GEN9_START;
            *max = SPECIES_PECHARUNT + 1;
            break;
        default:
            *min = GEN1_START;
            *max = SPECIES_PECHARUNT + 1;
            break;
    }
    *max -= 2;
}

#include "generated/data/pokemon/randomizer_banned.h"

u16 GetRandomPokemonFromSpecies(u16 basespecies) {
    SpeciesEnum species = basespecies;
    u32 rndSeed = VarGet(VAR_RANDOMIZED_SEED);
    u16 loc = gSaveBlock1Ptr->location.mapNum;
    u16 locG = gSaveBlock1Ptr->location.mapGroup;
    u8 map_tier = getTier(loc, locG);
    u8 map_tier_extra = map_tier;
    if (VarGet(VAR_RANDOMIZED_SEED) == 0) {
        u16 newseed = Random();
        VarSet(VAR_RANDOMIZED_SEED, newseed);
        rndSeed = VarGet(VAR_RANDOMIZED_SEED);
    }
    if (map_tier == 2) {
        map_tier_extra = 1;
    }
    if (map_tier == 1) {
        map_tier_extra = 2;
    }

    rndSeed ^= basespecies;
    rndSeed = ISO_RANDOMIZE1(rndSeed) >> 16;
    if (rndSeed % 10 == 0 && map_tier < 5) {
        map_tier++;
    }
    if (!gSaveBlock2Ptr->encounterRandomizedMode && gSaveBlock2Ptr->encounterRandomizedLegendaryMode && basespecies != SPECIES_NONE) {
        do {
            rndSeed = ISO_RANDOMIZE1(rndSeed);
            species = GetRandomPokemonFromTag(rndSeed >> 16, loc, locG);
        } while (species == SPECIES_NONE || (gBaseStats[species].tier != map_tier && gBaseStats[species].tier != map_tier_extra));
        return species;
    }
    if (gSaveBlock2Ptr->encounterRandomizedMode == TRUE && basespecies != SPECIES_NONE) {
        if (gSaveBlock2Ptr->encounterRandomizedLegendaryMode == FALSE) {
            // Legendary Mons Disabled

            do {
                species = RandRangeDeterministic(0, REAL_SPECIES_COUNT - 1, &rndSeed);
            } while (!IsSpeciesRandomizerAllowed(species, FALSE, TRUE));
        } else {
            // Legendary Mons Enabled
            do {
                species = RandRangeDeterministic(0, REAL_SPECIES_COUNT - 1, &rndSeed);
            } while (!IsSpeciesRandomizerAllowed(species, TRUE, TRUE));
        }
    }

    return species;
}
u8 getTier(s8 loc, s8 locG) {
    if ((loc == MAP_NUM(PETALBURG_CITY) && locG == MAP_GROUP(PETALBURG_CITY)) || (loc == MAP_NUM(SLATEPORT_CITY) && locG == MAP_GROUP(SLATEPORT_CITY)) ||
        (loc == MAP_NUM(RUSTBORO_CITY) && locG == MAP_GROUP(RUSTBORO_CITY)) || (loc == MAP_NUM(LITTLEROOT_TOWN) && locG == MAP_GROUP(LITTLEROOT_TOWN)) ||
        (loc == MAP_NUM(OLDALE_TOWN) && locG == MAP_GROUP(OLDALE_TOWN)) || (loc == MAP_NUM(DEWFORD_TOWN) && locG == MAP_GROUP(DEWFORD_TOWN)) ||
        (loc == MAP_NUM(ROUTE101) && locG == MAP_GROUP(ROUTE101)) || (loc == MAP_NUM(ROUTE102) && locG == MAP_GROUP(ROUTE102)) ||
        (loc == MAP_NUM(ROUTE103) && locG == MAP_GROUP(ROUTE103)) || (loc == MAP_NUM(ROUTE104) && locG == MAP_GROUP(ROUTE104)) ||
        (loc == MAP_NUM(ROUTE106) && locG == MAP_GROUP(ROUTE106)) || (loc == MAP_NUM(ROUTE107) && locG == MAP_GROUP(ROUTE107)) ||
        (loc == MAP_NUM(ROUTE109) && locG == MAP_GROUP(ROUTE109)) || (loc == MAP_NUM(ROUTE110) && locG == MAP_GROUP(ROUTE110)) ||
        (loc == MAP_NUM(ROUTE116) && locG == MAP_GROUP(ROUTE116)) || (loc == MAP_NUM(RUSTURF_TUNNEL) && locG == MAP_GROUP(RUSTURF_TUNNEL)) ||
        (loc == MAP_NUM(GRANITE_CAVE_1F) && locG == MAP_GROUP(GRANITE_CAVE_1F)) || (loc == MAP_NUM(GRANITE_CAVE_B1F) && locG == MAP_GROUP(GRANITE_CAVE_B1F)) ||
        (loc == MAP_NUM(GRANITE_CAVE_B2F) && locG == MAP_GROUP(GRANITE_CAVE_B2F)) ||
        (loc == MAP_NUM(GRANITE_CAVE_STEVENS_ROOM) && locG == MAP_GROUP(GRANITE_CAVE_STEVENS_ROOM)) ||
        (loc == MAP_NUM(PETALBURG_WOODS) && locG == MAP_GROUP(PETALBURG_WOODS)) ||
        (loc == MAP_NUM(PETALBURG_WOODS_2) && locG == MAP_GROUP(PETALBURG_WOODS_2)) ||
        (loc == MAP_NUM(PETALBURG_WOODS_3) && locG == MAP_GROUP(PETALBURG_WOODS_3)) || (loc == MAP_NUM(SEASPRAY_CAVE) && locG == MAP_GROUP(SEASPRAY_CAVE)) ||
        (loc == MAP_NUM(SEASPRAY_CAVE_B1F) && locG == MAP_GROUP(SEASPRAY_CAVE_B1F)) || (loc == MAP_NUM(DEWFORD_MEADOW) && locG == MAP_GROUP(DEWFORD_MEADOW)) ||
        (loc == MAP_NUM(DEWFORD_MANOR_1F) && locG == MAP_GROUP(DEWFORD_MANOR_1F))) {
        return 1;
    }
    if ((loc == MAP_NUM(MAUVILLE_CITY) && locG == MAP_GROUP(MAUVILLE_CITY)) || (loc == MAP_NUM(FALLARBOR_TOWN) && locG == MAP_GROUP(FALLARBOR_TOWN)) ||
        (loc == MAP_NUM(VERDANTURF_TOWN) && locG == MAP_GROUP(VERDANTURF_TOWN)) || (loc == MAP_NUM(ROUTE111) && locG == MAP_GROUP(ROUTE111)) ||
        (loc == MAP_NUM(ROUTE112) && locG == MAP_GROUP(ROUTE112)) || (loc == MAP_NUM(ROUTE113) && locG == MAP_GROUP(ROUTE113)) ||
        (loc == MAP_NUM(ROUTE114) && locG == MAP_GROUP(ROUTE114)) || (loc == MAP_NUM(ROUTE117) && locG == MAP_GROUP(ROUTE117)) ||
        (loc == MAP_NUM(METEOR_FALLS_1F_1R) && locG == MAP_GROUP(METEOR_FALLS_1F_1R)) ||
        (loc == MAP_NUM(METEOR_FALLS_1F_2R) && locG == MAP_GROUP(METEOR_FALLS_1F_2R)) || (loc == MAP_NUM(DESERT_RUINS) && locG == MAP_GROUP(DESERT_RUINS)) ||
        (loc == MAP_NUM(MT_CHIMNEY) && locG == MAP_GROUP(MT_CHIMNEY)) || (loc == MAP_NUM(JAGGED_PASS) && locG == MAP_GROUP(JAGGED_PASS)) ||
        (loc == MAP_NUM(FIERY_PATH) && locG == MAP_GROUP(FIERY_PATH)) || (loc == MAP_NUM(EMBER_PATH) && locG == MAP_GROUP(EMBER_PATH)) ||
        (loc == MAP_NUM(ASHEN_WOODS) && locG == MAP_GROUP(ASHEN_WOODS)) || (loc == MAP_NUM(VERDANTURF_MEADOW) && locG == MAP_GROUP(VERDANTURF_MEADOW))) {
        return 2;
    }
    if ((loc == MAP_NUM(FORTREE_CITY) && locG == MAP_GROUP(FORTREE_CITY)) || (loc == MAP_NUM(LILYCOVE_CITY) && locG == MAP_GROUP(LILYCOVE_CITY)) ||
        (loc == MAP_NUM(LAVARIDGE_TOWN) && locG == MAP_GROUP(LAVARIDGE_TOWN)) || (loc == MAP_NUM(ROUTE105) && locG == MAP_GROUP(ROUTE105)) ||
        (loc == MAP_NUM(ROUTE108) && locG == MAP_GROUP(ROUTE108)) || (loc == MAP_NUM(ROUTE115) && locG == MAP_GROUP(ROUTE115)) ||
        (loc == MAP_NUM(ROUTE118) && locG == MAP_GROUP(ROUTE118)) || (loc == MAP_NUM(ROUTE119) && locG == MAP_GROUP(ROUTE119)) ||
        (loc == MAP_NUM(ROUTE120) && locG == MAP_GROUP(ROUTE120)) || (loc == MAP_NUM(ROUTE121) && locG == MAP_GROUP(ROUTE121)) ||
        (loc == MAP_NUM(ROUTE122) && locG == MAP_GROUP(ROUTE122)) || (loc == MAP_NUM(ROUTE123) && locG == MAP_GROUP(ROUTE123)) ||
        (loc == MAP_NUM(MT_PYRE_1F) && locG == MAP_GROUP(MT_PYRE_1F)) || (loc == MAP_NUM(MT_PYRE_2F) && locG == MAP_GROUP(MT_PYRE_2F)) ||
        (loc == MAP_NUM(MT_PYRE_3F) && locG == MAP_GROUP(MT_PYRE_3F)) || (loc == MAP_NUM(MT_PYRE_4F) && locG == MAP_GROUP(MT_PYRE_4F)) ||
        (loc == MAP_NUM(MT_PYRE_5F) && locG == MAP_GROUP(MT_PYRE_5F)) || (loc == MAP_NUM(MT_PYRE_6F) && locG == MAP_GROUP(MT_PYRE_6F)) ||
        (loc == MAP_NUM(MT_PYRE_EXTERIOR) && locG == MAP_GROUP(MT_PYRE_EXTERIOR)) || (loc == MAP_NUM(MT_PYRE_SUMMIT) && locG == MAP_GROUP(MT_PYRE_SUMMIT)) ||
        (loc == MAP_NUM(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM)) ||
        (loc == MAP_NUM(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_LOW_TIDE_INNER_ROOM)) ||
        (loc == MAP_NUM(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM)) ||
        (loc == MAP_NUM(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM)) ||
        (loc == MAP_NUM(SHOAL_CAVE_HIGH_TIDE_ENTRANCE_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_HIGH_TIDE_ENTRANCE_ROOM)) ||
        (loc == MAP_NUM(SHOAL_CAVE_HIGH_TIDE_INNER_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_HIGH_TIDE_INNER_ROOM)) ||
        (loc == MAP_NUM(NEW_MAUVILLE_ENTRANCE) && locG == MAP_GROUP(NEW_MAUVILLE_ENTRANCE)) ||
        (loc == MAP_NUM(NEW_MAUVILLE_INSIDE) && locG == MAP_GROUP(NEW_MAUVILLE_INSIDE)) ||
        (loc == MAP_NUM(ABANDONED_SHIP_ROOMS_1F) && locG == MAP_GROUP(ABANDONED_SHIP_ROOMS_1F)) ||
        (loc == MAP_NUM(ABANDONED_SHIP_CORRIDORS_B1F) && locG == MAP_GROUP(ABANDONED_SHIP_CORRIDORS_B1F)) ||
        (loc == MAP_NUM(ABANDONED_SHIP_UNDERWATER1) && locG == MAP_GROUP(ABANDONED_SHIP_UNDERWATER1)) ||
        (loc == MAP_NUM(MIRAGE_TOWER_1F) && locG == MAP_GROUP(MIRAGE_TOWER_1F)) || (loc == MAP_NUM(MIRAGE_TOWER_2F) && locG == MAP_GROUP(MIRAGE_TOWER_2F)) ||
        (loc == MAP_NUM(MIRAGE_TOWER_3F) && locG == MAP_GROUP(MIRAGE_TOWER_3F)) || (loc == MAP_NUM(MIRAGE_TOWER_4F) && locG == MAP_GROUP(MIRAGE_TOWER_4F)) ||
        (loc == MAP_NUM(MIRAGE_TOWER_B1F) && locG == MAP_GROUP(MIRAGE_TOWER_B1F)) ||
        (loc == MAP_NUM(DESERT_UNDERPASS) && locG == MAP_GROUP(DESERT_UNDERPASS)) ||
        (loc == MAP_NUM(SANDSTREWN_RUINS) && locG == MAP_GROUP(SANDSTREWN_RUINS)) ||
        (loc == MAP_NUM(SANDSTREWN_RUINS_B1F) && locG == MAP_GROUP(SANDSTREWN_RUINS_B1F)) ||
        (loc == MAP_NUM(ROUTE111_RUINS_EXTERIOR) && locG == MAP_GROUP(ROUTE111_RUINS_EXTERIOR)) ||
        (loc == MAP_NUM(SANDSTREWN_RUINS_2F) && locG == MAP_GROUP(SANDSTREWN_RUINS_2F)) ||
        (loc == MAP_NUM(SANDSTREWN_RUINS_3F) && locG == MAP_GROUP(SANDSTREWN_RUINS_3F))) {
        return 3;
    }
    if ((loc == MAP_NUM(MOSSDEEP_CITY) && locG == MAP_GROUP(MOSSDEEP_CITY)) || (loc == MAP_NUM(SOOTOPOLIS_CITY) && locG == MAP_GROUP(SOOTOPOLIS_CITY)) ||
        (loc == MAP_NUM(ROUTE124) && locG == MAP_GROUP(ROUTE124)) || (loc == MAP_NUM(ROUTE125) && locG == MAP_GROUP(ROUTE125)) ||
        (loc == MAP_NUM(ROUTE126) && locG == MAP_GROUP(ROUTE126)) || (loc == MAP_NUM(ROUTE127) && locG == MAP_GROUP(ROUTE127)) ||
        (loc == MAP_NUM(ROUTE128) && locG == MAP_GROUP(ROUTE128)) || (loc == MAP_NUM(ROUTE129) && locG == MAP_GROUP(ROUTE129)) ||
        (loc == MAP_NUM(ROUTE130) && locG == MAP_GROUP(ROUTE130)) || (loc == MAP_NUM(ROUTE131) && locG == MAP_GROUP(ROUTE131)) ||
        (loc == MAP_NUM(ROUTE132) && locG == MAP_GROUP(ROUTE132)) || (loc == MAP_NUM(ROUTE133) && locG == MAP_GROUP(ROUTE133)) ||
        (loc == MAP_NUM(ROUTE134) && locG == MAP_GROUP(ROUTE134)) || (loc == MAP_NUM(UNDERWATER_ROUTE124) && locG == MAP_GROUP(UNDERWATER_ROUTE124)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE126) && locG == MAP_GROUP(UNDERWATER_ROUTE126)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE127) && locG == MAP_GROUP(UNDERWATER_ROUTE127)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE128) && locG == MAP_GROUP(UNDERWATER_ROUTE128)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE129) && locG == MAP_GROUP(UNDERWATER_ROUTE129)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE105) && locG == MAP_GROUP(UNDERWATER_ROUTE105)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE125) && locG == MAP_GROUP(UNDERWATER_ROUTE125)) ||
        (loc == MAP_NUM(UNDERWATER_SOOTOPOLIS_CITY) && locG == MAP_GROUP(UNDERWATER_SOOTOPOLIS_CITY)) ||
        (loc == MAP_NUM(AQUA_HIDEOUT_1F) && locG == MAP_GROUP(AQUA_HIDEOUT_1F)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ENTRANCE) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ENTRANCE)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM1) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM1)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM2) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM2)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM3) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM3)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM4) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM4)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM5) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM5)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM6) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM6)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM7) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM7)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM8) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM8)) ||
        (loc == MAP_NUM(SEAFLOOR_CAVERN_ROOM9) && locG == MAP_GROUP(SEAFLOOR_CAVERN_ROOM9)) ||
        (loc == MAP_NUM(UNDERWATER_ROUTE134) && locG == MAP_GROUP(UNDERWATER_ROUTE134)) ||
        (loc == MAP_NUM(SCORCHED_SLAB) && locG == MAP_GROUP(SCORCHED_SLAB)) ||
        (loc == MAP_NUM(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) && locG == MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ICE_ROOM)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_1F) && locG == MAP_GROUP(MAGMA_HIDEOUT_1F)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_2F_1R) && locG == MAP_GROUP(MAGMA_HIDEOUT_2F_1R)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_2F_2R) && locG == MAP_GROUP(MAGMA_HIDEOUT_2F_2R)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_3F_1R) && locG == MAP_GROUP(MAGMA_HIDEOUT_3F_1R)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_3F_2R) && locG == MAP_GROUP(MAGMA_HIDEOUT_3F_2R)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_4F) && locG == MAP_GROUP(MAGMA_HIDEOUT_4F)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_3F_3R) && locG == MAP_GROUP(MAGMA_HIDEOUT_3F_3R)) ||
        (loc == MAP_NUM(MAGMA_HIDEOUT_2F_3R) && locG == MAP_GROUP(MAGMA_HIDEOUT_2F_3R)) ||
        (loc == MAP_NUM(SCORCHED_SLAB_B1F) && locG == MAP_GROUP(SCORCHED_SLAB_B1F)) ||
        (loc == MAP_NUM(SCORCHED_SLAB_B2F) && locG == MAP_GROUP(SCORCHED_SLAB_B2F)) ||
        (loc == MAP_NUM(SCORCHED_SLAB_HEATRANS_ROOM) && locG == MAP_GROUP(SCORCHED_SLAB_HEATRANS_ROOM))) {
        return 4;
    }
    if ((loc == MAP_NUM(EVER_GRANDE_CITY) && locG == MAP_GROUP(EVER_GRANDE_CITY)) || (loc == MAP_NUM(PACIFIDLOG_TOWN) && locG == MAP_GROUP(PACIFIDLOG_TOWN)) ||
        (loc == MAP_NUM(METEOR_FALLS_B1F_1R) && locG == MAP_GROUP(METEOR_FALLS_B1F_1R)) ||
        (loc == MAP_NUM(METEOR_FALLS_B1F_2R) && locG == MAP_GROUP(METEOR_FALLS_B1F_2R)) ||
        (loc == MAP_NUM(CAVE_OF_ORIGIN_ENTRANCE) && locG == MAP_GROUP(CAVE_OF_ORIGIN_ENTRANCE)) ||
        (loc == MAP_NUM(CAVE_OF_ORIGIN_1F) && locG == MAP_GROUP(CAVE_OF_ORIGIN_1F)) ||
        (loc == MAP_NUM(CAVE_OF_ORIGIN_B1F) && locG == MAP_GROUP(CAVE_OF_ORIGIN_B1F)) ||
        (loc == MAP_NUM(VICTORY_ROAD_1F) && locG == MAP_GROUP(VICTORY_ROAD_1F)) || (loc == MAP_NUM(VICTORY_ROAD_B1F) && locG == MAP_GROUP(VICTORY_ROAD_B1F)) ||
        (loc == MAP_NUM(VICTORY_ROAD_B2F) && locG == MAP_GROUP(VICTORY_ROAD_B2F)) || (loc == MAP_NUM(SKY_PILLAR_1F) && locG == MAP_GROUP(SKY_PILLAR_1F)) ||
        (loc == MAP_NUM(SKY_PILLAR_2F) && locG == MAP_GROUP(SKY_PILLAR_2F)) || (loc == MAP_NUM(SKY_PILLAR_3F) && locG == MAP_GROUP(SKY_PILLAR_3F)) ||
        (loc == MAP_NUM(SKY_PILLAR_4F) && locG == MAP_GROUP(SKY_PILLAR_4F)) || (loc == MAP_NUM(SKY_PILLAR_5F) && locG == MAP_GROUP(SKY_PILLAR_5F)) ||
        (loc == MAP_NUM(ARTISAN_CAVE_B1F) && locG == MAP_GROUP(ARTISAN_CAVE_B1F)) || (loc == MAP_NUM(ARTISAN_CAVE_1F) && locG == MAP_GROUP(ARTISAN_CAVE_1F)) ||
        (loc == MAP_NUM(ALTERING_CAVE) && locG == MAP_GROUP(ALTERING_CAVE)) ||
        (loc == MAP_NUM(METEOR_FALLS_STEVENS_CAVE) && locG == MAP_GROUP(METEOR_FALLS_STEVENS_CAVE)) ||
        (loc == MAP_NUM(CAVE_OF_ORIGIN_DIANCIES_ROOM) && locG == MAP_GROUP(CAVE_OF_ORIGIN_DIANCIES_ROOM)) ||
        (loc == MAP_NUM(METEOR_FALLS_JIRACHIS_ROOM) && locG == MAP_GROUP(METEOR_FALLS_JIRACHIS_ROOM)) ||
        (loc == MAP_NUM(ALTERING_CAVE_1F) && locG == MAP_GROUP(ALTERING_CAVE_1F)) ||
        (loc == MAP_NUM(ALTERING_CAVE_B1F) && locG == MAP_GROUP(ALTERING_CAVE_B1F))) {
        return 5;
    }
    return 1;
}
u32 getMask(s8 loc, s8 locG) {
    if ((MAP_NUM(PETALBURG_CITY) == loc && MAP_GROUP(PETALBURG_CITY) == locG) ||
        (MAP_NUM(NEW_MAUVILLE_ENTRANCE) == loc && MAP_GROUP(NEW_MAUVILLE_ENTRANCE) == locG) ||
        (MAP_NUM(NEW_MAUVILLE_INSIDE) == loc && MAP_GROUP(NEW_MAUVILLE_INSIDE) == locG)) {
        return 532480;
    }
    if (MAP_NUM(SLATEPORT_CITY) == loc && MAP_GROUP(SLATEPORT_CITY) == locG) {
        return 1581056;
    }
    if (MAP_NUM(RUSTBORO_CITY) == loc && MAP_GROUP(RUSTBORO_CITY) == locG) {
        return 794624;
    }
    if ((MAP_NUM(LITTLEROOT_TOWN) == loc && MAP_GROUP(LITTLEROOT_TOWN) == locG) || (MAP_NUM(OLDALE_TOWN) == loc && MAP_GROUP(OLDALE_TOWN) == locG) ||
        (MAP_NUM(MAUVILLE_CITY) == loc && MAP_GROUP(MAUVILLE_CITY) == locG) || (MAP_NUM(FALLARBOR_TOWN) == loc && MAP_GROUP(FALLARBOR_TOWN) == locG) ||
        (MAP_NUM(VERDANTURF_TOWN) == loc && MAP_GROUP(VERDANTURF_TOWN) == locG) || (MAP_NUM(FORTREE_CITY) == loc && MAP_GROUP(FORTREE_CITY) == locG) ||
        (MAP_NUM(LILYCOVE_CITY) == loc && MAP_GROUP(LILYCOVE_CITY) == locG) || (MAP_NUM(LAVARIDGE_TOWN) == loc && MAP_GROUP(LAVARIDGE_TOWN) == locG) ||
        (MAP_NUM(EVER_GRANDE_CITY) == loc && MAP_GROUP(EVER_GRANDE_CITY) == locG)) {
        return 8192;
    }
    if (MAP_NUM(DEWFORD_TOWN) == loc && MAP_GROUP(DEWFORD_TOWN) == locG) {
        return 1058816;
    }
    if ((MAP_NUM(ROUTE101) == loc && MAP_GROUP(ROUTE101) == locG) || (MAP_NUM(ROUTE102) == loc && MAP_GROUP(ROUTE102) == locG) ||
        (MAP_NUM(ROUTE103) == loc && MAP_GROUP(ROUTE103) == locG) || (MAP_NUM(ROUTE121) == loc && MAP_GROUP(ROUTE121) == locG)) {
        return 2;
    }
    if (MAP_NUM(ROUTE104) == loc && MAP_GROUP(ROUTE104) == locG) {
        return 3;
    }
    if ((MAP_NUM(ROUTE106) == loc && MAP_GROUP(ROUTE106) == locG) || (MAP_NUM(ROUTE109) == loc && MAP_GROUP(ROUTE109) == locG)) {
        return 1050628;
    }
    if ((MAP_NUM(ROUTE107) == loc && MAP_GROUP(ROUTE107) == locG) || (MAP_NUM(ROUTE108) == loc && MAP_GROUP(ROUTE108) == locG) ||
        (MAP_NUM(ROUTE124) == loc && MAP_GROUP(ROUTE124) == locG) || (MAP_NUM(ROUTE125) == loc && MAP_GROUP(ROUTE125) == locG) ||
        (MAP_NUM(ROUTE126) == loc && MAP_GROUP(ROUTE126) == locG) || (MAP_NUM(ROUTE127) == loc && MAP_GROUP(ROUTE127) == locG) ||
        (MAP_NUM(ROUTE128) == loc && MAP_GROUP(ROUTE128) == locG) || (MAP_NUM(ROUTE129) == loc && MAP_GROUP(ROUTE129) == locG) ||
        (MAP_NUM(ROUTE130) == loc && MAP_GROUP(ROUTE130) == locG) || (MAP_NUM(ROUTE131) == loc && MAP_GROUP(ROUTE131) == locG) ||
        (MAP_NUM(ROUTE132) == loc && MAP_GROUP(ROUTE132) == locG) || (MAP_NUM(ROUTE133) == loc && MAP_GROUP(ROUTE133) == locG) ||
        (MAP_NUM(ROUTE134) == loc && MAP_GROUP(ROUTE134) == locG)) {
        return 4;
    }
    if (MAP_NUM(ROUTE110) == loc && MAP_GROUP(ROUTE110) == locG) {
        return 8194;
    }
    if (MAP_NUM(ROUTE116) == loc && MAP_GROUP(ROUTE116) == locG) {
        return 32769;
    }
    if ((MAP_NUM(RUSTURF_TUNNEL) == loc && MAP_GROUP(RUSTURF_TUNNEL) == locG) || (MAP_NUM(VICTORY_ROAD_1F) == loc && MAP_GROUP(VICTORY_ROAD_1F) == locG) ||
        (MAP_NUM(VICTORY_ROAD_B1F) == loc && MAP_GROUP(VICTORY_ROAD_B1F) == locG) ||
        (MAP_NUM(VICTORY_ROAD_B2F) == loc && MAP_GROUP(VICTORY_ROAD_B2F) == locG)) {
        return 72;
    }
    if (MAP_NUM(GRANITE_CAVE_1F) == loc && MAP_GROUP(GRANITE_CAVE_1F) == locG) {
        return 8;
    }
    if ((MAP_NUM(GRANITE_CAVE_B1F) == loc && MAP_GROUP(GRANITE_CAVE_B1F) == locG) ||
        (MAP_NUM(GRANITE_CAVE_B2F) == loc && MAP_GROUP(GRANITE_CAVE_B2F) == locG)) {
        return 328;
    }
    if (MAP_NUM(GRANITE_CAVE_STEVENS_ROOM) == loc && MAP_GROUP(GRANITE_CAVE_STEVENS_ROOM) == locG) {
        return 264;
    }
    if ((MAP_NUM(PETALBURG_WOODS) == loc && MAP_GROUP(PETALBURG_WOODS) == locG) ||
        (MAP_NUM(PETALBURG_WOODS_2) == loc && MAP_GROUP(PETALBURG_WOODS_2) == locG)) {
        return 1;
    }
    if (MAP_NUM(PETALBURG_WOODS_3) == loc && MAP_GROUP(PETALBURG_WOODS_3) == locG) {
        return 65;
    }
    if (MAP_NUM(SEASPRAY_CAVE) == loc && MAP_GROUP(SEASPRAY_CAVE) == locG) {
        return 12;
    }
    if (MAP_NUM(SEASPRAY_CAVE_B1F) == loc && MAP_GROUP(SEASPRAY_CAVE_B1F) == locG) {
        return 24;
    }
    if (MAP_NUM(DEWFORD_MEADOW) == loc && MAP_GROUP(DEWFORD_MEADOW) == locG) {
        return 1050626;
    }
    if (MAP_NUM(DEWFORD_MANOR_1F) == loc && MAP_GROUP(DEWFORD_MANOR_1F) == locG) {
        return 1024;
    }
    if (MAP_NUM(ROUTE111) == loc && MAP_GROUP(ROUTE111) == locG) {
        return 36864;
    }
    if (MAP_NUM(ROUTE112) == loc && MAP_GROUP(ROUTE112) == locG) {
        return 34;
    }
    if (MAP_NUM(ROUTE113) == loc && MAP_GROUP(ROUTE113) == locG) {
        return 546;
    }
    if (MAP_NUM(ROUTE114) == loc && MAP_GROUP(ROUTE114) == locG) {
        return 32770;
    }
    if ((MAP_NUM(ROUTE117) == loc && MAP_GROUP(ROUTE117) == locG) || (MAP_NUM(ROUTE119) == loc && MAP_GROUP(ROUTE119) == locG) ||
        (MAP_NUM(ROUTE120) == loc && MAP_GROUP(ROUTE120) == locG) || (MAP_NUM(ROUTE123) == loc && MAP_GROUP(ROUTE123) == locG)) {
        return 2050;
    }
    if ((MAP_NUM(METEOR_FALLS_1F_1R) == loc && MAP_GROUP(METEOR_FALLS_1F_1R) == locG) ||
        (MAP_NUM(METEOR_FALLS_1F_2R) == loc && MAP_GROUP(METEOR_FALLS_1F_2R) == locG) ||
        (MAP_NUM(METEOR_FALLS_B1F_1R) == loc && MAP_GROUP(METEOR_FALLS_B1F_1R) == locG) ||
        (MAP_NUM(METEOR_FALLS_B1F_2R) == loc && MAP_GROUP(METEOR_FALLS_B1F_2R) == locG) ||
        (MAP_NUM(METEOR_FALLS_STEVENS_CAVE) == loc && MAP_GROUP(METEOR_FALLS_STEVENS_CAVE) == locG) ||
        (MAP_NUM(METEOR_FALLS_JIRACHIS_ROOM) == loc && MAP_GROUP(METEOR_FALLS_JIRACHIS_ROOM) == locG)) {
        return 98312;
    }
    if (MAP_NUM(DESERT_RUINS) == loc && MAP_GROUP(DESERT_RUINS) == locG) {
        return 69920;
    }
    if ((MAP_NUM(MT_CHIMNEY) == loc && MAP_GROUP(MT_CHIMNEY) == locG) || (MAP_NUM(JAGGED_PASS) == loc && MAP_GROUP(JAGGED_PASS) == locG)) {
        return 33312;
    }
    if ((MAP_NUM(FIERY_PATH) == loc && MAP_GROUP(FIERY_PATH) == locG) || (MAP_NUM(SCORCHED_SLAB) == loc && MAP_GROUP(SCORCHED_SLAB) == locG) ||
        (MAP_NUM(SCORCHED_SLAB_B1F) == loc && MAP_GROUP(SCORCHED_SLAB_B1F) == locG) ||
        (MAP_NUM(SCORCHED_SLAB_B2F) == loc && MAP_GROUP(SCORCHED_SLAB_B2F) == locG) ||
        (MAP_NUM(SCORCHED_SLAB_HEATRANS_ROOM) == loc && MAP_GROUP(SCORCHED_SLAB_HEATRANS_ROOM) == locG)) {
        return 40;
    }
    if ((MAP_NUM(EMBER_PATH) == loc && MAP_GROUP(EMBER_PATH) == locG) || (MAP_NUM(SANDSTREWN_RUINS) == loc && MAP_GROUP(SANDSTREWN_RUINS) == locG) ||
        (MAP_NUM(SANDSTREWN_RUINS_B1F) == loc && MAP_GROUP(SANDSTREWN_RUINS_B1F) == locG) ||
        (MAP_NUM(SANDSTREWN_RUINS_2F) == loc && MAP_GROUP(SANDSTREWN_RUINS_2F) == locG) ||
        (MAP_NUM(SANDSTREWN_RUINS_3F) == loc && MAP_GROUP(SANDSTREWN_RUINS_3F) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM1) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM1) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM2) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM2) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM3) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM3) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM4) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM4) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM5) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM5) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM6) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM6) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM7) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM7) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM8) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM8) == locG) ||
        (MAP_NUM(SEAFLOOR_CAVERN_ROOM9) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ROOM9) == locG) ||
        (MAP_NUM(ARTISAN_CAVE_B1F) == loc && MAP_GROUP(ARTISAN_CAVE_B1F) == locG) || (MAP_NUM(ARTISAN_CAVE_1F) == loc && MAP_GROUP(ARTISAN_CAVE_1F) == locG) ||
        (MAP_NUM(ALTERING_CAVE) == loc && MAP_GROUP(ALTERING_CAVE) == locG) || (MAP_NUM(ALTERING_CAVE_1F) == loc && MAP_GROUP(ALTERING_CAVE_1F) == locG) ||
        (MAP_NUM(ALTERING_CAVE_B1F) == loc && MAP_GROUP(ALTERING_CAVE_B1F) == locG)) {
        return 0;
    }
    if (MAP_NUM(ASHEN_WOODS) == loc && MAP_GROUP(ASHEN_WOODS) == locG) {
        return 33313;
    }
    if (MAP_NUM(VERDANTURF_MEADOW) == loc && MAP_GROUP(VERDANTURF_MEADOW) == locG) {
        return 10242;
    }
    if ((MAP_NUM(ROUTE105) == loc && MAP_GROUP(ROUTE105) == locG) || (MAP_NUM(ROUTE118) == loc && MAP_GROUP(ROUTE118) == locG)) {
        return 1048580;
    }
    if (MAP_NUM(ROUTE115) == loc && MAP_GROUP(ROUTE115) == locG) {
        return 1081346;
    }
    if (MAP_NUM(ROUTE122) == loc && MAP_GROUP(ROUTE122) == locG) {
        return 1028;
    }
    if ((MAP_NUM(MT_PYRE_1F) == loc && MAP_GROUP(MT_PYRE_1F) == locG) || (MAP_NUM(MT_PYRE_2F) == loc && MAP_GROUP(MT_PYRE_2F) == locG) ||
        (MAP_NUM(MT_PYRE_3F) == loc && MAP_GROUP(MT_PYRE_3F) == locG) || (MAP_NUM(MT_PYRE_4F) == loc && MAP_GROUP(MT_PYRE_4F) == locG) ||
        (MAP_NUM(MT_PYRE_5F) == loc && MAP_GROUP(MT_PYRE_5F) == locG) || (MAP_NUM(MT_PYRE_6F) == loc && MAP_GROUP(MT_PYRE_6F) == locG)) {
        return 1088;
    }
    if ((MAP_NUM(MT_PYRE_EXTERIOR) == loc && MAP_GROUP(MT_PYRE_EXTERIOR) == locG) || (MAP_NUM(MT_PYRE_SUMMIT) == loc && MAP_GROUP(MT_PYRE_SUMMIT) == locG)) {
        return 33792;
    }
    if ((MAP_NUM(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ENTRANCE_ROOM) == locG) ||
        (MAP_NUM(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_INNER_ROOM) == locG) ||
        (MAP_NUM(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_STAIRS_ROOM) == locG) ||
        (MAP_NUM(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_LOWER_ROOM) == locG)) {
        return 1048600;
    }
    if ((MAP_NUM(SHOAL_CAVE_HIGH_TIDE_ENTRANCE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_HIGH_TIDE_ENTRANCE_ROOM) == locG) ||
        (MAP_NUM(SHOAL_CAVE_HIGH_TIDE_INNER_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_HIGH_TIDE_INNER_ROOM) == locG)) {
        return 28;
    }
    if ((MAP_NUM(ABANDONED_SHIP_ROOMS_1F) == loc && MAP_GROUP(ABANDONED_SHIP_ROOMS_1F) == locG) ||
        (MAP_NUM(ABANDONED_SHIP_CORRIDORS_B1F) == loc && MAP_GROUP(ABANDONED_SHIP_CORRIDORS_B1F) == locG) ||
        (MAP_NUM(ABANDONED_SHIP_UNDERWATER1) == loc && MAP_GROUP(ABANDONED_SHIP_UNDERWATER1) == locG)) {
        return 1152;
    }
    if ((MAP_NUM(MIRAGE_TOWER_1F) == loc && MAP_GROUP(MIRAGE_TOWER_1F) == locG) || (MAP_NUM(MIRAGE_TOWER_2F) == loc && MAP_GROUP(MIRAGE_TOWER_2F) == locG) ||
        (MAP_NUM(MIRAGE_TOWER_3F) == loc && MAP_GROUP(MIRAGE_TOWER_3F) == locG) || (MAP_NUM(MIRAGE_TOWER_4F) == loc && MAP_GROUP(MIRAGE_TOWER_4F) == locG) ||
        (MAP_NUM(MIRAGE_TOWER_B1F) == loc && MAP_GROUP(MIRAGE_TOWER_B1F) == locG)) {
        return 5376;
    }
    if (MAP_NUM(DESERT_UNDERPASS) == loc && MAP_GROUP(DESERT_UNDERPASS) == locG) {
        return 4104;
    }
    if (MAP_NUM(ROUTE111_RUINS_EXTERIOR) == loc && MAP_GROUP(ROUTE111_RUINS_EXTERIOR) == locG) {
        return 6144;
    }
    if (MAP_NUM(MOSSDEEP_CITY) == loc && MAP_GROUP(MOSSDEEP_CITY) == locG) {
        return 73732;
    }
    if (MAP_NUM(SOOTOPOLIS_CITY) == loc && MAP_GROUP(SOOTOPOLIS_CITY) == locG) {
        return 8324;
    }
    if ((MAP_NUM(UNDERWATER_ROUTE124) == loc && MAP_GROUP(UNDERWATER_ROUTE124) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE126) == loc && MAP_GROUP(UNDERWATER_ROUTE126) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE127) == loc && MAP_GROUP(UNDERWATER_ROUTE127) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE128) == loc && MAP_GROUP(UNDERWATER_ROUTE128) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE129) == loc && MAP_GROUP(UNDERWATER_ROUTE129) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE105) == loc && MAP_GROUP(UNDERWATER_ROUTE105) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE125) == loc && MAP_GROUP(UNDERWATER_ROUTE125) == locG) ||
        (MAP_NUM(UNDERWATER_SOOTOPOLIS_CITY) == loc && MAP_GROUP(UNDERWATER_SOOTOPOLIS_CITY) == locG) ||
        (MAP_NUM(UNDERWATER_ROUTE134) == loc && MAP_GROUP(UNDERWATER_ROUTE134) == locG)) {
        return 128;
    }
    if (MAP_NUM(AQUA_HIDEOUT_1F) == loc && MAP_GROUP(AQUA_HIDEOUT_1F) == locG) {
        return 524292;
    }
    if (MAP_NUM(SEAFLOOR_CAVERN_ENTRANCE) == loc && MAP_GROUP(SEAFLOOR_CAVERN_ENTRANCE) == locG) {
        return 140;
    }
    if (MAP_NUM(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) == loc && MAP_GROUP(SHOAL_CAVE_LOW_TIDE_ICE_ROOM) == locG) {
        return 16;
    }
    if ((MAP_NUM(MAGMA_HIDEOUT_1F) == loc && MAP_GROUP(MAGMA_HIDEOUT_1F) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_2F_1R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_1R) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_2F_2R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_2R) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_3F_1R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_1R) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_3F_2R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_2R) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_4F) == loc && MAP_GROUP(MAGMA_HIDEOUT_4F) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_3F_3R) == loc && MAP_GROUP(MAGMA_HIDEOUT_3F_3R) == locG) ||
        (MAP_NUM(MAGMA_HIDEOUT_2F_3R) == loc && MAP_GROUP(MAGMA_HIDEOUT_2F_3R) == locG)) {
        return 104;
    }
    if (MAP_NUM(PACIFIDLOG_TOWN) == loc && MAP_GROUP(PACIFIDLOG_TOWN) == locG) {
        return 8196;
    }
    if ((MAP_NUM(CAVE_OF_ORIGIN_ENTRANCE) == loc && MAP_GROUP(CAVE_OF_ORIGIN_ENTRANCE) == locG) ||
        (MAP_NUM(CAVE_OF_ORIGIN_1F) == loc && MAP_GROUP(CAVE_OF_ORIGIN_1F) == locG) ||
        (MAP_NUM(CAVE_OF_ORIGIN_B1F) == loc && MAP_GROUP(CAVE_OF_ORIGIN_B1F) == locG)) {
        return 65864;
    }
    if ((MAP_NUM(SKY_PILLAR_1F) == loc && MAP_GROUP(SKY_PILLAR_1F) == locG) || (MAP_NUM(SKY_PILLAR_2F) == loc && MAP_GROUP(SKY_PILLAR_2F) == locG) ||
        (MAP_NUM(SKY_PILLAR_3F) == loc && MAP_GROUP(SKY_PILLAR_3F) == locG) || (MAP_NUM(SKY_PILLAR_4F) == loc && MAP_GROUP(SKY_PILLAR_4F) == locG) ||
        (MAP_NUM(SKY_PILLAR_5F) == loc && MAP_GROUP(SKY_PILLAR_5F) == locG)) {
        return 525320;
    }
    if (MAP_NUM(CAVE_OF_ORIGIN_DIANCIES_ROOM) == loc && MAP_GROUP(CAVE_OF_ORIGIN_DIANCIES_ROOM) == locG) {
        return 65800;
    }

    return 0;
}
bool8 isSpeciesPlaceholderMon(SpeciesEnum species) { return ((u16)species) >= REAL_SPECIES_COUNT || !gEvolutionTable[species]; }

bool8 IsEeveelution(SpeciesEnum species) {
    switch (species) {
        case SPECIES_VAPOREON:
        case SPECIES_JOLTEON:
        case SPECIES_FLAREON:
        case SPECIES_UMBREON:
        case SPECIES_ESPEON:
        case SPECIES_LEAFEON:
        case SPECIES_GLACEON:
        case SPECIES_SYLVEON:
            return TRUE;

        default:
            return FALSE;
    }
}

typedef enum {
    MEGA_NONE,
    MEGA_MEGA_UNSPECIFIED,
    MEGA_MEGA_X,
    MEGA_MEGA_Y,
    MEGA_MEGA_Z,
    MEGA_MEGA_A,
    MEGA_MEGA_B,
    MEGA_MEGA_C,
    MEGA_PRIMAL,
    MEGA_CROWNED,
    MEGA_ORIGIN,
    MEGA_ULTRA,
} MegaType;

typedef enum {
    REGION_NONE,
    REGION_ALOLA,
    REGION_GALAR,
    REGION_HISUI,
    REGION_PALDEA,
    REGION_REDUX,
} RegionType;

#include "generated/data/text/species_long_names.h"

const u8 gText_Subname12[] = _("{STR_VAR_1} {STR_VAR_2}");
const u8 gText_Subname13[] = _("{STR_VAR_1} {STR_VAR_3}");
const u8 gText_Subname21[] = _("{STR_VAR_2} {STR_VAR_1}");
const u8 gText_Subname123[] = _("{STR_VAR_1} {STR_VAR_2} {STR_VAR_3}");
const u8 gText_Mega[] = _("Mega");
const u8 gText_Primal[] = _("Primal");
const u8 gText_Ultra[] = _("Ultra");
const u8 gText_Crowned[] = _("Crowned");
const u8 gText_Origin[] = _("Origin");
const u8 gText_Alolan[] = _("Alolan");
const u8 gText_Galarian[] = _("Galarian");
const u8 gText_Hisuian[] = _("Hisuian");
const u8 gText_Paldean[] = _("Paldean");
const u8 gText_Redux[] = _("Redux");
const u8 gText_X[] = _("X");
const u8 gText_Y[] = _("Y");
const u8 gText_Z[] = _("Z");
const u8 gText_A[] = _("A");
const u8 gText_B[] = _("B");
const u8 gText_C[] = _("C");

const u8 *SaveSpeciesWithSurname(SpeciesEnum species) {
    const u8 *speciesName = GetSpeciesLongName(species);
    if (!speciesName) speciesName = gSpeciesNames[species];
    MegaType megaType = GetSpeciesMegaType(species);
    RegionType regionType = GetSpeciesRegionType(species);

    u8 *regionOutput = megaType ? gStringVar2 : gStringVar4;

    switch (regionType) {
        case REGION_ALOLA:
            StringCopy(gStringVar1, gText_Alolan);
            StringCopy(gStringVar3, speciesName);
            StringExpandPlaceholders(regionOutput, gText_Subname13);
            break;
        case REGION_GALAR:
            StringCopy(gStringVar1, gText_Galarian);
            StringCopy(gStringVar3, speciesName);
            StringExpandPlaceholders(regionOutput, gText_Subname13);
            break;
        case REGION_HISUI:
            StringCopy(gStringVar1, gText_Hisuian);
            StringCopy(gStringVar3, speciesName);
            StringExpandPlaceholders(regionOutput, gText_Subname13);
            break;
        case REGION_PALDEA:
            StringCopy(gStringVar1, gText_Paldean);
            StringCopy(gStringVar3, speciesName);
            StringExpandPlaceholders(regionOutput, gText_Subname13);
            break;
        case REGION_REDUX:
            StringCopy(gStringVar1, speciesName);
            StringCopy(gStringVar3, gText_Redux);
            StringExpandPlaceholders(regionOutput, gText_Subname13);
            break;

        default:
            StringCopy(regionOutput, speciesName);
            break;
    }

    switch (megaType) {
        case MEGA_MEGA_A:
            StringCopy(gStringVar1, gText_Mega);
            StringCopy(gStringVar3, gText_A);
            StringExpandPlaceholders(gStringVar4, gText_Subname123);
            break;
        case MEGA_MEGA_B:
            StringCopy(gStringVar1, gText_Mega);
            StringCopy(gStringVar3, gText_B);
            StringExpandPlaceholders(gStringVar4, gText_Subname123);
            break;
        case MEGA_MEGA_C:
            StringCopy(gStringVar1, gText_Mega);
            StringCopy(gStringVar3, gText_C);
            StringExpandPlaceholders(gStringVar4, gText_Subname123);
            break;
        case MEGA_MEGA_X:
            StringCopy(gStringVar1, gText_Mega);
            StringCopy(gStringVar3, gText_X);
            StringExpandPlaceholders(gStringVar4, gText_Subname123);
            break;
        case MEGA_MEGA_Y:
            StringCopy(gStringVar1, gText_Mega);
            StringCopy(gStringVar3, gText_Y);
            StringExpandPlaceholders(gStringVar4, gText_Subname123);
            break;
        case MEGA_MEGA_Z:
            StringCopy(gStringVar1, gText_Mega);
            StringCopy(gStringVar3, gText_Z);
            StringExpandPlaceholders(gStringVar4, gText_Subname123);
            break;
        case MEGA_MEGA_UNSPECIFIED:
            StringCopy(gStringVar1, gText_Mega);
            StringExpandPlaceholders(gStringVar4, gText_Subname12);
            break;
        case MEGA_PRIMAL:
            StringCopy(gStringVar1, gText_Primal);
            StringExpandPlaceholders(gStringVar4, gText_Subname12);
            break;
        case MEGA_ULTRA:
            StringCopy(gStringVar1, gText_Ultra);
            StringExpandPlaceholders(gStringVar4, gText_Subname12);
            break;
        case MEGA_ORIGIN:
            StringCopy(gStringVar1, gText_Origin);
            StringExpandPlaceholders(gStringVar4, gText_Subname21);
            break;
        case MEGA_CROWNED:
            StringCopy(gStringVar1, gText_Crowned);
            StringExpandPlaceholders(gStringVar4, gText_Subname21);
            break;
    }

    return gStringVar4;
}

SpeciesEnum GetRandomSpeciesFromPool(u8 id) {
    u8 rand = Random();
    switch (id) {
        case POKEMON_POOL_STARTERS: {
            SpeciesEnum PossibleSpecies[] = {
                SPECIES_BULBASAUR, SPECIES_CHARMANDER, SPECIES_SQUIRTLE, SPECIES_CHIKORITA,  SPECIES_CYNDAQUIL, SPECIES_TOTODILE, SPECIES_TREECKO,
                SPECIES_TORCHIC,   SPECIES_MUDKIP,     SPECIES_TURTWIG,  SPECIES_CHIMCHAR,   SPECIES_PIPLUP,    SPECIES_SNIVY,    SPECIES_TEPIG,
                SPECIES_OSHAWOTT,  SPECIES_CHESPIN,    SPECIES_FENNEKIN, SPECIES_FROAKIE,    SPECIES_ROWLET,    SPECIES_LITTEN,   SPECIES_POPPLIO,
                SPECIES_GROOKEY,   SPECIES_SCORBUNNY,  SPECIES_SOBBLE,   SPECIES_SPRIGATITO, SPECIES_FUECOCO,   SPECIES_QUAXLY,
            };
            return PossibleSpecies[rand % (sizeof(PossibleSpecies) / sizeof(PossibleSpecies[0]))];
        } break;
        default: {
            SpeciesEnum PossibleSpecies[] = {
                SPECIES_BULBASAUR, SPECIES_CHARMANDER, SPECIES_SQUIRTLE, SPECIES_CHIKORITA,  SPECIES_CYNDAQUIL, SPECIES_TOTODILE, SPECIES_TREECKO,
                SPECIES_TORCHIC,   SPECIES_MUDKIP,     SPECIES_TURTWIG,  SPECIES_CHIMCHAR,   SPECIES_PIPLUP,    SPECIES_SNIVY,    SPECIES_TEPIG,
                SPECIES_OSHAWOTT,  SPECIES_CHESPIN,    SPECIES_FENNEKIN, SPECIES_FROAKIE,    SPECIES_ROWLET,    SPECIES_LITTEN,   SPECIES_POPPLIO,
                SPECIES_GROOKEY,   SPECIES_SCORBUNNY,  SPECIES_SOBBLE,   SPECIES_SPRIGATITO, SPECIES_FUECOCO,   SPECIES_QUAXLY,
            };
            return PossibleSpecies[rand % (sizeof(PossibleSpecies) / sizeof(PossibleSpecies[0]))];
        } break;
    }
}

SpeciesEnum GetFormChangeForMon(struct Pokemon *mon, u8 num) {
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);

    switch (gFormChangeTable[species][num].method) {
        case EVO_FORM_SHIFT:
            return gFormChangeTable[species][num].targetSpecies;
            break;
        case EVO_FORM_SHIFT_GENDER:
            REQUIRE(GetMonGender(mon) == gFormChangeTable[species][num].param)
            return gFormChangeTable[species][num].targetSpecies;
            break;
        case EVO_MEGA_EVOLUTION:
        case EVO_PRIMAL_REVERSION:
            REQUIRE(gSaveBlock2Ptr->permanentMegaMode && CheckBagHasItem(ITEM_MEGA_BRACELET, 1) && FlagGet(FLAG_SYS_RECEIVED_KEYSTONE))
            REQUIRE(gFormChangeTable[species][num].param == GetMonData(mon, MON_DATA_HELD_ITEM, NULL) ||
                    CheckBagHasItem(gFormChangeTable[species][num].param, 1))
            return gFormChangeTable[species][num].targetSpecies;
            break;
        case EVO_MOVE_MEGA_EVOLUTION:
            REQUIRE(gSaveBlock2Ptr->permanentMegaMode && CheckBagHasItem(ITEM_MEGA_BRACELET, 1) && FlagGet(FLAG_SYS_RECEIVED_KEYSTONE))
            u8 i;
            for (i = 0; i < MAX_MON_MOVES; i++) {
                if (GetMonData(mon, MON_DATA_MOVE1, 0) == gFormChangeTable[species][num].param) break;
            }
            REQUIRE(i < MAX_MON_MOVES)
            return gFormChangeTable[species][num].targetSpecies;
            break;
        case EVO_UNMEGA:
            return gFormChangeTable[species][num].targetSpecies;
            break;
    }

    return SPECIES_NONE;
}

u8 getNumofAvailableEvos(struct Pokemon *mon) {
    u8 i;
    u8 count = 0;

    for (i = 0; gEvolutionTable[mon->box.species][i].method; i++) {
        if (GetEvolutionForMon(mon, i) != SPECIES_NONE) count++;
    }

    return count;
}

SpeciesEnum GetEvolutionForMon(struct Pokemon *mon, u8 num) {
    u8 i, j;
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 level = GetMonData(mon, MON_DATA_LEVEL, NULL);
    u8 friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, 0);
    u16 upperPersonality = personality >> 16;
    u8 beauty = GetMonData(mon, MON_DATA_BEAUTY, 0);
    u16 currentMap;

    i = num;

    switch (gEvolutionTable[species][i].method) {
        case EVO_DEEVOLUTION:
            return gEvolutionTable[species][i].targetSpecies;
            break;
        case EVO_FRIENDSHIP:
            if (friendship >= 220) return gEvolutionTable[species][i].targetSpecies;
            break;
        case EVO_FRIENDSHIP_DAY:
            RtcCalcLocalTime();
            if (IsCurrentlyDay() && friendship >= 220) return gEvolutionTable[species][i].targetSpecies;
            break;
        case EVO_LEVEL_DUSK:
            RtcCalcLocalTime();
            if (IsCurrentlyDusk() && gEvolutionTable[species][i].param <= level) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_DAY:
            RtcCalcLocalTime();
            if (IsCurrentlyDay() && gEvolutionTable[species][i].param <= level && !(IsCurrentlyDusk() && species == SPECIES_ROCKRUFF))
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_NIGHT:
            RtcCalcLocalTime();
            if (!IsCurrentlyDay() && gEvolutionTable[species][i].param <= level && !(IsCurrentlyDusk() && species == SPECIES_ROCKRUFF))
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_FRIENDSHIP_NIGHT:
            RtcCalcLocalTime();
            if (!IsCurrentlyDay() && friendship >= 220) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_ITEM_HOLD_NIGHT:
            RtcCalcLocalTime();
            if (!IsCurrentlyDay() && heldItem == gEvolutionTable[species][i].param) {
                heldItem = 0;
                SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            }
            break;
        case EVO_ITEM_HOLD_DAY:
            RtcCalcLocalTime();
            if (IsCurrentlyDay() && heldItem == gEvolutionTable[species][i].param) {
                heldItem = 0;
                SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            }
            break;
        case EVO_LEVEL:
            if (gEvolutionTable[species][i].param <= level) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_FEMALE:
            if (gEvolutionTable[species][i].param <= level && GetMonGender(mon) == MON_FEMALE)
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_MALE:
            if (gEvolutionTable[species][i].param <= level && GetMonGender(mon) == MON_MALE)
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_ATK_GT_DEF:
            if (gEvolutionTable[species][i].param <= level && (GetMonData(mon, MON_DATA_ATK, 0) > GetMonData(mon, MON_DATA_DEF, 0)))
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_ATK_EQ_DEF:
            if (gEvolutionTable[species][i].param <= level && GetMonData(mon, MON_DATA_ATK, 0) == GetMonData(mon, MON_DATA_DEF, 0))
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_ATK_LT_DEF:
            if (gEvolutionTable[species][i].param <= level && GetMonData(mon, MON_DATA_ATK, 0) < GetMonData(mon, MON_DATA_DEF, 0))
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_SILCOON:
            if (gEvolutionTable[species][i].param <= level && (upperPersonality % 10) <= 4)
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_CASCOON:
            if (gEvolutionTable[species][i].param <= level && (upperPersonality % 10) > 4)
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_LEVEL_NINJASK:
            if (gEvolutionTable[species][i].param <= level) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_BEAUTY:
            if (gEvolutionTable[species][i].param <= beauty) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_MOVE:
            if (MonKnowsMove(mon, gEvolutionTable[species][i].param)) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_MOVE_TYPE:
            for (j = 0; j < 4; j++) {
                if (gBattleMoves[GetMonData(mon, MON_DATA_MOVE1 + j, NULL)].type == gEvolutionTable[species][i].param)
                    return gEvolutionTable[species][i].targetSpecies;  // Get base species
            }
            break;
        case EVO_SPECIFIC_MON_IN_PARTY:
            for (j = 0; j < PARTY_SIZE; j++) {
                if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) == gEvolutionTable[species][i].param)
                    return gEvolutionTable[species][i].targetSpecies;  // Get base species
            }
            break;
        case EVO_LEVEL_DARK_TYPE_MON_IN_PARTY:
            if (gEvolutionTable[species][i].param <= level) {
                for (j = 0; j < PARTY_SIZE; j++) {
                    u16 partyspecies = GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL);
                    if (gBaseStats[partyspecies].type1 == TYPE_DARK || gBaseStats[partyspecies].type2 == TYPE_DARK)
                        return gEvolutionTable[species][i].targetSpecies;  // Get base species
                }
            }
            break;
        case EVO_LEVEL_RAIN:
            j = GetCurrentWeather();
            if (j == WEATHER_RAIN || j == WEATHER_RAIN_THUNDERSTORM || j == WEATHER_DOWNPOUR)
                return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
        case EVO_SPECIFIC_MAP:
            currentMap = ((gSaveBlock1Ptr->location.mapGroup) << 8 | gSaveBlock1Ptr->location.mapNum);
            if (currentMap == gEvolutionTable[species][i].param) return gEvolutionTable[species][i].targetSpecies;  // Get base species
            break;
    }

    return SPECIES_NONE;
}

// If this is ever used
#define NEW_SHINY_VARIANTS 2

struct ShinyUnlock {
    u16 flag;
    u16 questComplete;
    u16 variable;
    u8 value;
};

static const struct ShinyUnlock sShinyUnlock[NUM_SPECIES][NEW_SHINY_VARIANTS] = {
    [SPECIES_BULBASAUR] =
        {
            {// First Shiny
             .flag = FLAG_RECEIVED_POKEDEX_FROM_BIRCH,
             .questComplete = 0,
             .variable = 0,
             .value = 0},
            {// Second Shiny
             .flag = 0,
             .questComplete = SIDE_QUEST_1,
             .variable = 0,
             .value = 0},
        },
    [SPECIES_CHARMANDER] =
        {
            {// First Shiny
             .flag = 0,
             .questComplete = 0,
             .variable = VAR_BIRCH_STATE,
             .value = 3},
            {// Second Shiny
             .variable = VAR_BIRCH_STATE,
             .value = 3},
        },
};

bool8 isShinyVariantUnlocked(SpeciesEnum species, u8 variant) {
    struct ShinyUnlock sSpeciesShinyUnlock = sShinyUnlock[species][variant];

    if (sSpeciesShinyUnlock.flag != 0 && !FlagGet(sSpeciesShinyUnlock.flag))
        return FALSE;
    else if (sSpeciesShinyUnlock.questComplete != 0 && GetSetQuestFlag(sSpeciesShinyUnlock.questComplete, FLAG_GET_COMPLETED))
        return FALSE;
    else if (sSpeciesShinyUnlock.variable != 0 && VarGet(sSpeciesShinyUnlock.variable) >= sSpeciesShinyUnlock.value)
        return FALSE;
    else
        return TRUE;
}

bool8 CanDisableInnates(void){
    return (gSaveBlock2Ptr->gameDifficulty >= P_DISABLE_FIRST_DIFFICULTY);
}

u8 getInnateDisableLevel(u8 innateNum){
    if(CanDisableInnates()){
        switch(innateNum){
            default://Fallback
            case SPECIES_INNATE_NUM_1:
                if(gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_ELITE)
                    return INNATE_1_LEVEL_ELITE;
                else
                    return INNATE_1_LEVEL_HELL;
            break;
            case SPECIES_INNATE_NUM_2:
                if(gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_ELITE)
                    return INNATE_2_LEVEL_ELITE;
                else
                    return INNATE_2_LEVEL_HELL;
            break;
            case SPECIES_INNATE_NUM_3:
                if(gSaveBlock2Ptr->gameDifficulty == DIFFICULTY_ELITE)
                    return INNATE_3_LEVEL_ELITE;
                else
                    return INNATE_3_LEVEL_HELL;
            break;
        }
    }
    
    return 0;
}

bool8 isMoveDisabled(u8 battler, u16 move){
    /*u16 difficulty    = gSaveBlock2Ptr->gameDifficulty;
    u16 moveEffect    = gBattleMoves[move].effect;
    bool8 isPlayerMon = (GetBattlerSide(battler) == B_SIDE_PLAYER);

    if(isPlayerMon){
        //Player Limitations
        switch(difficulty){
            case DIFFICULTY_HELL:
            {
                //Evasion Clause enforced on the player’s side (exceptions being moves like Detect or Mind Reader)
                switch(moveEffect){
                    //Evasion Up Moves
                    case EFFECT_EVASION_UP:
                    case EFFECT_MINIMIZE:
                    //Accuracy Down Moves
                    case EFFECT_ACCURACY_DOWN:
                        return TRUE;
                    break;
                }
            }
            break;
        }
    }*/

    return FALSE;
}

u16 GetHeldItemIfNotDuplicate(u8 partyId){
    u8 i;
    u16 heldItem = GetMonData(&gPlayerParty[partyId], MON_DATA_HELD_ITEM);

    if(heldItem == ITEM_NONE || gSaveBlock2Ptr->gameDifficulty != DIFFICULTY_HELL)
        return heldItem;

    for(i = 0; i < PARTY_SIZE; i++){
        u16 species   = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES);
        u16 heldItem2 = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);

        //The first Pokémon holding the held item can keep it
        if(i == partyId)
            return heldItem;

        if(species != SPECIES_NONE && heldItem2 == heldItem){
            heldItem = ITEM_NONE;
            SetMonData(&gPlayerParty[partyId], MON_DATA_HELD_ITEM, &heldItem);
            return heldItem;
        }
    }
    
    return heldItem;
}
