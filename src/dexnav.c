#include "global.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "bg.h"
#include "data.h"
#include "daycare.h"
#include "decompress.h"
#include "dexnav.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_message_box.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "fieldmap.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "item.h"
#include "international_string_util.h"
#include "load_save.h"
#include "m4a.h"
#include "map_name_popup.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "metatile_behavior.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_summary_screen.h"
#include "random.h"
#include "region_map.h"
#include "scanline_effect.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "sprite.h"
#include "start_menu.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "wild_encounter.h"
#include "window.h"
#include "wild_encounter.h"
#include "constants/map_types.h"
#include "constants/species.h"
#include "constants/maps.h"
#include "constants/field_effects.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/abilities.h"
#include "constants/rgb.h"
#include "constants/region_map_sections.h"
#include "gba/m4a_internal.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"
#include "abilities.h"

#define GLOBAL_DEXNAV_SEARCH_LEVEL 0
#define HIDDEN_WILD_COUNT 3
#define TEXT_SKIP_DRAW 0xFF
#define NUM_POKEMON_ICONS 12
#define DEXNAV_MAX_SHOWN_ROWS 5

//Config
#define DEXNAV_PLUS_UNLOCK_FLAG          FLAG_RESCUED_BIRCH
#define DEXNAV_HONEY_MONS_UNLOCK_FLAG    FLAG_RESCUED_BIRCH
#define DEXNAV_HEADBUTT_MONS_UNLOCK_FLAG FLAG_RESCUED_BIRCH
#define DEXNAV_FISHING_MONS_UNLOCK_FLAG  FLAG_BADGE02_GET
#define DEXNAV_WATER_MONS_UNLOCK_FLAG    FLAG_BADGE02_GET

#define DEXNAV_ENABLE_ROUTE_115_FLAG FLAG_BADGE02_GET

#define DEXNAV_ENABLE_ROUTE_118_FLAG FLAG_BADGE05_GET

#define IF_ROUTE_DISABLED(route) if (!FlagGet(DEXNAV_ENABLE_ROUTE_##route##_FLAG) && gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE##route))

int IsDisabledForRoute(int row)
{
    IF_ROUTE_DISABLED(115) return TRUE;
    IF_ROUTE_DISABLED(118) {
        switch (row) {
            case ROW_LAND_TOP:
            case ROW_ROCK_SMASH:
            case ROW_HONEY:
                return TRUE;
            default:
                return FALSE;
        }
    }
    return FALSE;
}

// Defines
enum WindowIds
{
    WINDOW_INFO,
    WINDOW_COUNT,
};

enum Statuses
{
    STATUS_INVALID_SEARCH,
    STATUS_CHOOSE_MON,
    STATUS_LOCKED,
    STATUS_NO_DATA,
    STATUS_INCORRECT_AREA,
};

enum{
    DEXNAV_SPRITE_ICON,
    DEXNAV_SPRITE_TYPE_ICON_1 = NUM_POKEMON_ICONS,
    DEXNAV_SPRITE_TYPE_ICON_2,
    DEXNAV_SPRITE_CURSOR,
    DEXNAV_FIELD_ICON_LAND,
    DEXNAV_FIELD_ICON_WATER,
    DEXNAV_FIELD_ICON_FISHING,
    DEXNAV_FIELD_ICON_HEADBUTT,
    DEXNAV_FIELD_ICON_HIDDEN,
    DEXNAV_FIELD_ICON_HONEY,
    NUM_DEXNAV_SPRITES,
};

struct DexNavSearch
{
    u16 species;
    u16 moves[MAX_MON_MOVES];
    u16 heldItem;
    u8 abilityNum;
    u8 potential;
    u8 searchLevel;
    u8 monLevel;
    u8 proximity;
    s16 tileX;
    s16 tileY;
    u8 fldEffSpriteId;
    u8 fldEffId;
    u8 movementCount;
    u8 windowId;
    u8 iconSpriteId;
    u8 eyeSpriteId;
    u8 itemSpriteId;
    u8 starSpriteIds[3];
    u8 ownedIconSpriteId;
    u8 exclamationSpriteId;
    u8 hiddenSearch:1;
    u8 isHiddenMon:1;
    u8 unk:6;
    u16 palBuffer[16];
};

struct DexNavGUI
{
    MainCallback savedCallback;
    u8 state;
    u16 routeSpecies[DEXNAV_ROWS_COUNT][LAND_WILD_COUNT];
    u16 routeSpeciesNum[DEXNAV_ROWS_COUNT];
    u8 routeRows[DEXNAV_ROWS_COUNT];
    u8 rowNum;
    u8 currentEnviorment;
    u8 currentMessage;
    u8 cursorRow;
    u8 cursorCol;
    u8 potential;
    u8 starSpriteIds[3];
    u8 DexnavSprites[NUM_DEXNAV_SPRITES];
    u8 DexnavSpeciesIconsSprites[DEXNAV_ROWS_COUNT][NUM_POKEMON_ICONS];
};

// RAM

EWRAM_DATA static struct DexNavSearch *sDexNavSearchDataPtr = NULL;
EWRAM_DATA static struct DexNavGUI *sDexNavUiDataPtr = NULL;
EWRAM_DATA static u8 *sBg1TilemapBuffer = NULL;
EWRAM_DATA bool8 gDexnavBattle = FALSE;

//// Function Declarations
//GUI
static void Task_DexNavWaitFadeIn(u8 taskId);
static void Task_DexNavMain(u8 taskId);
static void PrintCurrentSpeciesInfo(void);
// SEARCH
static bool8 TryStartHiddenMonFieldEffect(u8 environment, u8 xSize, u8 ySize, bool8 smallScan);
static void DexNavGenerateMoveset(u16 species, u8 searchLevel, u8 encounterLevel, u16* moveDst);
static u16 DexNavGenerateHeldItem(u16 species, u8 searchLevel);
static u8 DexNavGetAbilityNum(u16 species, u8 searchLevel);
static u8 DexNavGeneratePotential(u8 searchLevel);
static u8 DexNavTryGenerateMonLevel(u16 species, u8 environment);
static u8 GetEncounterLevelFromMapData(u16 species, u8 environment);
static void CreateDexNavWildMon(u16 species, u8 potential, u8 level, u8 abilityNum, u16 item, u16* moves);
static u8 GetPlayerDistance(s16 x, s16 y);
static u8 DexNavPickTile(u8 environment, u8 xSize, u8 ySize, bool8 smallScan);
static bool8 DexnavIsTileUsable(u8 environment);
static void DexNavProximityUpdate(void);
static void DexNavDrawIcons(void);
static void DexNavUpdateSearchWindow(u8 proximity, u8 searchLevel);
//static void DexNavSightUpdate(u8 index);
static void Task_DexNavSearch(u8 taskId);
static void EndDexNavSearchSetupScript(const u8 *script, u8 taskId);
// HIDDEN MONS
static void DexNavDrawHiddenIcons(void);
static void DrawHiddenSearchWindow(u8 width);
bool8 CanFindHiddenPokemon(void);
bool8 hasAllMonsInEnviorment(void);

//// Const Data
// gui image data
static const u32 sDexNavGuiTiles[] = INCBIN_U32("graphics/ui_menus/dexnav/tiles.4bpp.lz");
static const u32 sDexNavGuiTilemap[] = INCBIN_U32("graphics/ui_menus/dexnav/tilemap.bin.lz");
static const u32 sDexNavGuiPal[] = INCBIN_U32("graphics/ui_menus/dexnav/dexnav_palette.gbapal");

static const u32 sSelectionCursorGfx[] = INCBIN_U32("graphics/dexnav/cursor.4bpp.lz");
static const u16 sSelectionCursorPal[] = INCBIN_U16("graphics/dexnav/cursor.gbapal");
static const u32 sCapturedAllMonsTiles[] = INCBIN_U32("graphics/dexnav/captured_all.4bpp.lz");  //uses selection cursor pal

static const u32 sNoDataGfx[] = INCBIN_U32("graphics/dexnav/no_data.4bpp.lz");

// searching image data
static const u32 sPotentialStarGfx[] = INCBIN_U32("graphics/dexnav/star.4bpp.lz");
//static const u32 sEyeGfx[] = INCBIN_U32("graphics/dexnav/vision.4bpp.lz");
static const u32 sHiddenSearchIconGfx[] = INCBIN_U32("graphics/dexnav/hidden_search.4bpp.lz");
static const u32 sOwnedIconGfx[] = INCBIN_U32("graphics/dexnav/owned_icon.4bpp.lz");
static const u32 sHiddenMonIconGfx[] = INCBIN_U32("graphics/dexnav/hidden.4bpp.lz");

// strings
static const u8 sText_DexNav_NoInfo[] = _("--------");
static const u8 sText_DexNav_CaptureToSee[] = _("Capture first!");
static const u8 sText_DexNav_PressRToRegister[] = _("Dexnav+");
static const u8 sText_DexNav_SearchForRegisteredSpecies[] = _("Search {STR_VAR_1}");
static const u8 sText_DexNav_NotFoundHere[] = _("This Pokémon cannot be found here!");
static const u8 sText_ThreeQmarks[] = _("???");
static const u8 sText_SearchLevel[] = _("SEARCH {LV}. {STR_VAR_1}");
static const u8 sText_MonLevel[] = _("{LV}. {STR_VAR_1}");
static const u8 sText_EggMove[] = _("MOVE: {STR_VAR_1}");
static const u8 sText_HeldItem[] = _("{STR_VAR_1}");
static const u8 sText_StartExit[] = _("{START_BUTTON} EXIT");
static const u8 sText_DexNavChain[] = _("{NO} {STR_VAR_1}");
static const u8 sText_DexNavChainLong[] = _("{NO}{STR_VAR_1}");

static const u8 sText_ArrowLeft[] = _("{LEFT_ARROW}");
static const u8 sText_ArrowRight[] = _("{RIGHT_ARROW}");
static const u8 sText_ArrowUp[] = _("{UP_ARROW}");
static const u8 sText_ArrowDown[] = _("{DOWN_ARROW}");

static const struct WindowTemplate sDexNavGuiWindowTemplates[] =
{
    [WINDOW_INFO] =
    {
        .bg = 0,            // which bg to print text on
        .tilemapLeft = 0,   // position from left (per 8 pixels)
        .tilemapTop = 0,    // position from top (per 8 pixels)
        .width = 30,        // width (per 8 pixels)
        .height = 20,       // height (per 8 pixels)
        .paletteNum = 1,    // palette index to use for text
        .baseBlock = 1,     // tile start in VRAM
    }
};

//gui font
static const u8 sFontColor_Black[3] = {0, 1, 2};
static const u8 sFontColor_White[3] = {0, 3, 1};
static const u8 sSearchFontColor[3] = {0, 15, 13};

static const struct OamData sNoDataIconOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 1,
};

static const struct OamData sHeldItemOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 0,
    .paletteNum = 13,
};

static const struct OamData sCapturedAllOam =
{
    .y = 0,
    .affineMode = 1,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0, //Highest
    .paletteNum = 12,
    .affineParam = 0,
};

static const struct OamData sSearchIconOam =
{
    .y = 0,
    .affineMode = 0,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = 0,
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0, // above BG layers
    .paletteNum = 13,
    .affineParam = 0
};

static const struct OamData sSelectionCursorOam =
{
    .y = 0,
    .affineMode = 0,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = 0,
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0, // above BG layers
    .paletteNum = 12,
    .affineParam = 0
};

static const struct OamData sSightOam =
{
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(16x8),
    .size = SPRITE_SIZE(16x8),
    .priority = 0,
};
static const union AnimCmd sAnimCmdSight0[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};
static const union AnimCmd sAnimCmdSight1[] =
{
    ANIMCMD_FRAME(2, 1),
    ANIMCMD_END
};
static const union AnimCmd sAnimCmdSight2[] =
{
    ANIMCMD_FRAME(4, 1),
    ANIMCMD_END
};
static const union AnimCmd *const sAnimCmdTable_Sight[] =
{
    sAnimCmdSight0,
    sAnimCmdSight1,
    sAnimCmdSight2,
};

// gui sprite templates
static const struct SpriteTemplate sNoDataIconTemplate =
{
    .tileTag = ICON_GFX_TAG,
    .paletteTag = ICON_PAL_TAG,
    .oam = &sNoDataIconOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sCaptureAllMonsSpriteTemplate =
{
    .tileTag = CAPTURED_ALL_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sCapturedAllOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sSelectionCursorSpriteTemplate =
{
    .tileTag = SELECTION_CURSOR_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sSelectionCursorOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// search window sprite templates
static const struct SpriteTemplate sHeldItemTemplate =
{
    .tileTag = HELD_ITEM_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sHeldItemOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sPotentialStarTemplate =
{
    .tileTag = LIT_STAR_TILE_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sHeldItemOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

/*static const struct SpriteTemplate sSightTemplate =
{
    .tileTag = SIGHT_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sSightOam,
    .anims = sAnimCmdTable_Sight,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};*/

static const struct SpriteTemplate sSearchIconSpriteTemplate =
{
    .tileTag = HIDDEN_SEARCH_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sSearchIconOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sOwnedIconTemplate = 
{
    .tileTag = OWNED_ICON_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sHeldItemOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sHiddenMonIconTemplate = 
{
    .tileTag = HIDDEN_MON_ICON_TAG,
    .paletteTag = 0xFFFF,   //held item pal
    .oam = &sHeldItemOam,
    .anims =  gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

// gui sprite sheets
static const struct CompressedSpriteSheet sNoDataIconSpriteSheet = {sNoDataGfx, (32 * 32) / 2, ICON_GFX_TAG};
static const struct CompressedSpriteSheet sCapturedAllPokemonSpriteSheet = {sCapturedAllMonsTiles, (8 * 8) / 2, CAPTURED_ALL_TAG};
// search sprite sheets
static const struct CompressedSpriteSheet sPotentialStarSpriteSheet = {sPotentialStarGfx, (8 * 8) / 2, LIT_STAR_TILE_TAG};
//static const struct CompressedSpriteSheet sSightSpriteSheet = {sEyeGfx, (16 * 8 * 3) / 2, SIGHT_TAG};
static const struct CompressedSpriteSheet sOwnedIconSpriteSheet = {sOwnedIconGfx, (8 * 8) / 2, OWNED_ICON_TAG};
static const struct CompressedSpriteSheet sHiddenMonIconSpriteSheet = {sHiddenMonIconGfx, (8 * 8) / 2, HIDDEN_MON_ICON_TAG};

//// functions
///////////////////////
//// DEXNAV SEARCH ////
///////////////////////
static inline int GetSpecies(const struct WildPokemonInfo* info, int index)
{
    return GetRandomPokemonFromSpecies(info->wildPokemon[index].species);
}

static s16 GetSearchWindowY(void)
{
    return (GetWindowAttribute(sDexNavSearchDataPtr->windowId, WINDOW_TILEMAP_TOP) * 8);
}

#define SPECIES_ICON_X 28
static void DrawDexNavSearchMonIcon(u16 species, u8 *dst, bool8 owned)
{
    u8 spriteId;

    LoadMonIconPalette(species);
    spriteId = CreateMonIcon(species, SpriteCB_MonIcon, SPECIES_ICON_X - 6, GetSearchWindowY() + 8, 0, 0xFFFFFFFF);
    gSprites[spriteId].oam.priority = 0;
    *dst = spriteId;
    
    if (owned)
        sDexNavSearchDataPtr->ownedIconSpriteId = CreateSprite(&sOwnedIconTemplate, SPECIES_ICON_X + 6, GetSearchWindowY() + 4, 0);
}

static void AddSearchWindow(u8 width)
{
    struct WindowTemplate template;
    u16 y = 16;
    
    if (sDexNavSearchDataPtr->tileY > (gSaveBlock1Ptr->pos.y + 7))
        y = 1;  //draw at top if chosen tile is below

    LoadDexNavWindowGfx(sDexNavSearchDataPtr->windowId, 0x1d5, 14 * 16);
    
    SetWindowTemplateFields(&template, 0, 1, y, width, 3, 14, 8);
    
    sDexNavSearchDataPtr->windowId = AddWindow(&template);
    FillWindowPixelBuffer(sDexNavSearchDataPtr->windowId, PIXEL_FILL(1));
    PutWindowTilemap(sDexNavSearchDataPtr->windowId);
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 3);
    
    DrawStdFrameWithCustomTileAndPalette(sDexNavSearchDataPtr->windowId, TRUE, 0x214, 14);
}

#define WINDOW_COL_0        (SPECIES_ICON_X + 4)
#define WINDOW_COL_1        (WINDOW_COL_0 + (GetFontAttribute(sDexNavSearchDataPtr->windowId, FONTATTR_MAX_LETTER_WIDTH) * (POKEMON_NAME_LENGTH)))
#define WINDOW_MOVE_NAME_X  (WINDOW_COL_1 + (GetFontAttribute(sDexNavSearchDataPtr->windowId, FONTATTR_MAX_LETTER_WIDTH) * 6))
#define SEARCH_ARROW_X      (WINDOW_MOVE_NAME_X + 90)
#define SEARCH_ARROW_Y      0
static void AddSearchWindowText(u16 species, u8 proximity, u8 searchLevel, bool8 hidden)
{
    u8 windowId = sDexNavSearchDataPtr->windowId;
    
    //species name - always present
    if (hidden)
    {
        StringCopy(gStringVar4, sText_ThreeQmarks);
        AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, WINDOW_COL_0, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
        return;
    }
    else
    {
        StringCopy(gStringVar1, gSpeciesNames[species]);
        AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, WINDOW_COL_0, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar1);
    }
    
    //level - always present
    ConvertIntToDecimalStringN(gStringVar1, sDexNavSearchDataPtr->monLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar4, sText_MonLevel);
    AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, WINDOW_COL_1, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
    
    if (proximity <= SNEAKING_PROXIMITY)
    {
        PlaySE(SE_POKENAV_ON);
        // move
        if (searchLevel > 1 && sDexNavSearchDataPtr->moves[0])
        {
            StringCopy(gStringVar1, gMoveNames[sDexNavSearchDataPtr->moves[0]]);
            StringExpandPlaceholders(gStringVar4, sText_EggMove);
            AddTextPrinterParameterized3(windowId, 0, WINDOW_MOVE_NAME_X, 0, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
        }
        
        if (searchLevel > 2)
        {            
            // ability name
            StringCopy(gStringVar1, gAbilities[GetAbilityBySpecies(species, sDexNavSearchDataPtr->abilityNum)].name);
            AddTextPrinterParameterized3(windowId, 0, WINDOW_COL_1 + 16, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar1);
        
            // item name
            if (sDexNavSearchDataPtr->heldItem)
            {
                CopyItemName(sDexNavSearchDataPtr->heldItem, gStringVar1);
                StringExpandPlaceholders(gStringVar4, sText_HeldItem);
                AddTextPrinterParameterized3(windowId, 0, WINDOW_COL_0, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
            }
        }
    }
    
    //chain level - always present
    ConvertIntToDecimalStringN(gStringVar1, gSaveBlock1Ptr->dexNavChain, STR_CONV_MODE_LEFT_ALIGN, 3);
    if (gSaveBlock1Ptr->dexNavChain > 99)
        StringExpandPlaceholders(gStringVar4, sText_DexNavChainLong);
    else
        StringExpandPlaceholders(gStringVar4, sText_DexNavChain);
    AddTextPrinterParameterized3(windowId, 0, SEARCH_ARROW_X - 16, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);    
    
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 2);
}

static void DrawSearchWindow(u16 species, u8 potential, bool8 hidden)
{
    u8 searchLevel = sDexNavSearchDataPtr->searchLevel;
    
    AddSearchWindow(28);
    AddSearchWindowText(species, sDexNavSearchDataPtr->proximity, searchLevel, hidden);
}

static void RemoveDexNavWindowAndGfx(void)
{
    u32 i;
    
    // try remove sprites
    if (sDexNavSearchDataPtr->iconSpriteId != 0xFF)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->iconSpriteId]);
    if (sDexNavSearchDataPtr->itemSpriteId != 0xFF)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->itemSpriteId]);
    if (sDexNavSearchDataPtr->eyeSpriteId != 0xFF)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->eyeSpriteId]);
    if (sDexNavSearchDataPtr->ownedIconSpriteId != 0xFF)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->ownedIconSpriteId]);
    if (sDexNavSearchDataPtr->exclamationSpriteId != 0xFF)
        DestroySprite(&gSprites[sDexNavSearchDataPtr->exclamationSpriteId]);
    
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
    {
        if (sDexNavSearchDataPtr->starSpriteIds[i] != 0xFF)
            DestroySprite(&gSprites[sDexNavSearchDataPtr->starSpriteIds[i]]);
    }
    
    FreeSpriteTilesByTag(HELD_ITEM_TAG);
    FreeSpriteTilesByTag(OWNED_ICON_TAG);
    FreeSpriteTilesByTag(HIDDEN_SEARCH_TAG);
    FreeSpriteTilesByTag(HIDDEN_MON_ICON_TAG);
    FreeSpriteTilesByTag(LIT_STAR_TILE_TAG);
    FreeSpritePaletteByTag(HELD_ITEM_TAG);
    SafeFreeMonIconPalette(sDexNavSearchDataPtr->species);
    
    // remove window
    ClearStdWindowAndFrameToTransparent(sDexNavSearchDataPtr->windowId, FALSE);
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 3);
    RemoveWindow(sDexNavSearchDataPtr->windowId);
}


//////////////////////
////DEXNAV SEARCH/////
//////////////////////
static u8 GetPlayerDistance(s16 x, s16 y)
{
    u16 deltaX = abs(x - (gSaveBlock1Ptr->pos.x + 7));
    u16 deltaY = abs(y - (gSaveBlock1Ptr->pos.y + 7));
    return deltaX + deltaY;
}

static void DexNavProximityUpdate(void)
{
    sDexNavSearchDataPtr->proximity = GetPlayerDistance(sDexNavSearchDataPtr->tileX, sDexNavSearchDataPtr->tileY);
}

static bool8 DexnavIsTileUsable(u8 environment) {
    s16 posX;
    s16 posY;
    u8 tileBehaviour;

    PlayerGetDestCoords(&posX, &posY);
    tileBehaviour = MapGridGetMetatileBehaviorAt(posX, posY);
    switch (environment)
    {
        case ROW_LAND_TOP:
        case ROW_ROCK_SMASH:
        case ROW_HONEY:
            if (MetatileBehavior_IsLandWildEncounter(tileBehaviour))
                return TRUE;
        break;
        case ROW_WATER:
        case ROW_FISHING:
            if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehaviour))
                return TRUE;
        break;
    }
    return FALSE;
}

//Pick a specific tile based on environment
static bool8 DexNavPickTile(u8 environment, u8 areaX, u8 areaY, bool8 smallScan)
{
    // area of map to cover starting from camera position {-7, -7}
    s16 topX = gSaveBlock1Ptr->pos.x - SCANSTART_X + (smallScan * 5);
    s16 topY = gSaveBlock1Ptr->pos.y - SCANSTART_Y + (smallScan * 5);
    s16 botX = topX + areaX;
    s16 botY = topY + areaY;
    u8 i;
    bool8 nextIter;
    u8 currMapType = GetCurrentMapType();
    u8 tileBehaviour;
    bool8 correctTile = FALSE;
    
    // loop through every tile in area and evaluate
    while (topY < botY)
    {
        while (topX < botX)
        {
            tileBehaviour = MapGridGetMetatileBehaviorAt(topX, topY);
            correctTile = FALSE;
            
            //gSpecialVar_0x8005 = tileBehaviour;
            
            //Check for objects
            nextIter = FALSE;
            
            /*if (GetPlayerDistance(topX, topY) <= tileBuffer)
            {
                // tile too close to player
                topX++;
                continue;
            }*/
            
            for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
            {
                if (gObjectEvents[i].currentCoords.x == topX && gObjectEvents[i].currentCoords.y == topY)
                {
                    // cannot be on a tile where an object exists
                    nextIter = TRUE;
                    break;
                }
            }
            
            if (nextIter)
            {
                topX++;
                continue;
            }
            
            switch (environment)
            {
            case ENCOUNTER_TYPE_LAND:
                if (MetatileBehavior_IsLandWildEncounter(tileBehaviour))
                {
                    if (currMapType == MAP_TYPE_UNDERGROUND)
                    { // inside (cave)
                        if (IsZCoordMismatchAt(gObjectEvents[gPlayerAvatar.spriteId].currentElevation, topX, topY))
                            break; //occurs at same z coord
                        
                        //Check if it's possible to find a mon there
                        if (!MapGridIsImpassableAt(topX, topY)) {
                            correctTile = TRUE;
                        }
                    }
                    else
                    { // outdoors: grass
                        //Check if it's possible to find a mon there
                        if (!MapGridIsImpassableAt(topX, topY)) {
                            correctTile = TRUE;
                        }
                    }
                }
                break;
            case ENCOUNTER_TYPE_WATER:
                if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehaviour))
                {
                    if (IsZCoordMismatchAt(gObjectEvents[gPlayerAvatar.spriteId].currentElevation, topX, topY))
                        break;

                    //Check if it's possible to find a mon there
                    if (!MapGridIsImpassableAt(topX, topY)) {
                        correctTile = TRUE;
                    }
                }
                break;
            default:
                break;
            }

            if (correctTile)
            {
                sDexNavSearchDataPtr->tileX = topX;
                sDexNavSearchDataPtr->tileY = topY;
                return TRUE;
            }
            
            topX++;
        }
        
        topY++;
        topX = gSaveBlock1Ptr->pos.x - SCANSTART_X + (smallScan * 5);
    }

    return FALSE;
}


static bool8 TryStartHiddenMonFieldEffect(u8 environment, u8 xSize, u8 ySize, bool8 smallScan)
{
    u8 currMapType = GetCurrentMapType();
    u8 fldEffId = 0;
    
    if (DexNavPickTile(environment, xSize, ySize, smallScan))
    {
        u8 metatileBehaviour = MapGridGetMetatileBehaviorAt(sDexNavSearchDataPtr->tileX, sDexNavSearchDataPtr->tileY);

        switch (environment)
        {
        case ENCOUNTER_TYPE_LAND:
            if (currMapType == MAP_TYPE_UNDERGROUND)
            {
                fldEffId = FLDEFF_CAVE_DUST;
            }
            else if (IsMapTypeIndoors(currMapType))
            {
                if (MetatileBehavior_IsTallGrass(metatileBehaviour)) //Grass in cave
                    fldEffId = FLDEFF_SHAKING_GRASS;
                else if (MetatileBehavior_IsLongGrass(metatileBehaviour)) //Really tall grass
                    fldEffId = FLDEFF_SHAKING_LONG_GRASS;
                else if (MetatileBehavior_IsSandOrDeepSand(metatileBehaviour))
                    fldEffId = FLDEFF_SAND_HOLE;
                else
                    fldEffId = FLDEFF_CAVE_DUST;
            }
            else //outdoor, underwater
            {
                if (MetatileBehavior_IsTallGrass(metatileBehaviour)) //Regular grass
                    fldEffId = FLDEFF_SHAKING_GRASS;
                else if (MetatileBehavior_IsLongGrass(metatileBehaviour)) //Really tall grass
                    fldEffId = FLDEFF_SHAKING_LONG_GRASS;
                else if (MetatileBehavior_IsSandOrDeepSand(metatileBehaviour)) //Desert Sand
                    fldEffId = FLDEFF_SAND_HOLE;
                else if (MetatileBehavior_IsMountain(metatileBehaviour)) //Rough Terrain
                    fldEffId = FLDEFF_CAVE_DUST;
                else if (MetatileBehavior_IsAshGrass(metatileBehaviour))//Ash Grass
                    fldEffId = FLDEFF_CAVE_DUST;
                else
                    fldEffId = FLDEFF_BERRY_TREE_GROWTH_SPARKLE; //default
            }
            break;
        case ENCOUNTER_TYPE_WATER:
            fldEffId = FLDEFF_WATER_SURFACING;
            break;
        default:
            return FALSE;
        }
        
        if (fldEffId != 0)
        {
            gFieldEffectArguments[0] = sDexNavSearchDataPtr->tileX;
            gFieldEffectArguments[1] = sDexNavSearchDataPtr->tileY;
            gFieldEffectArguments[2] = 0xFF; // subpriority
            gFieldEffectArguments[3] = 2;   //priority
            sDexNavSearchDataPtr->fldEffSpriteId = FieldEffectStart(fldEffId);
            if (sDexNavSearchDataPtr->fldEffSpriteId == 0xFF)
                return FALSE;
            
            sDexNavSearchDataPtr->fldEffId = fldEffId;
            return TRUE;
        }
    }

    return FALSE;
}

static void DrawDexNavSearchHeldItem(u8* dst)
{
    *dst = CreateSprite(&sHeldItemTemplate, SPECIES_ICON_X + 6, GetSearchWindowY() + 18, 0);
    if (*dst != 0xFF)
        gSprites[*dst].invisible = TRUE;
}

static void LoadSearchIconData(void)
{
    // palettes clash with mon icon, so must load manually
    LoadSpriteSheet(&gSpriteSheet_HeldItem);
    LoadPalette(gHeldItemPalette, 0x100 + (16 * sHeldItemOam.paletteNum), 32);
    LoadCompressedSpriteSheetUsingHeap(&sPotentialStarSpriteSheet);
    //LoadCompressedSpriteSheetUsingHeap(&sSightSpriteSheet);   //eye replaced with arrow
    LoadCompressedSpriteSheetUsingHeap(&sOwnedIconSpriteSheet);
    LoadCompressedSpriteSheetUsingHeap(&sHiddenMonIconSpriteSheet);
}

#define tProximity          data[0]
#define tFrameCount         data[1]
#define tSpecies            data[2]
#define tEnvironment        data[3]
#define tRevealed           data[4]
static void Task_SetUpDexNavSearch(u8 taskId)
{
    u16 species = sDexNavSearchDataPtr->species;
    u8 searchLevel = GLOBAL_DEXNAV_SEARCH_LEVEL;
    
    // init sprites
    sDexNavSearchDataPtr->iconSpriteId = 0xFF;
    sDexNavSearchDataPtr->itemSpriteId = 0xFF;
    sDexNavSearchDataPtr->eyeSpriteId = 0xFF;
    sDexNavSearchDataPtr->starSpriteIds[0] = 0xFF;
    sDexNavSearchDataPtr->starSpriteIds[1] = 0xFF;
    sDexNavSearchDataPtr->starSpriteIds[2] = 0xFF;
    sDexNavSearchDataPtr->ownedIconSpriteId = 0xFF;
    sDexNavSearchDataPtr->exclamationSpriteId = 0xFF;    
    sDexNavSearchDataPtr->searchLevel = searchLevel;
    
    DexNavGenerateMoveset(species, searchLevel, sDexNavSearchDataPtr->monLevel, &sDexNavSearchDataPtr->moves[0]);
    sDexNavSearchDataPtr->heldItem = DexNavGenerateHeldItem(species, searchLevel);
    sDexNavSearchDataPtr->abilityNum = DexNavGetAbilityNum(species, searchLevel);
    sDexNavSearchDataPtr->potential = DexNavGeneratePotential(searchLevel);
    DexNavProximityUpdate();
    
    LoadSearchIconData();
    if (sDexNavSearchDataPtr->hiddenSearch)
    {
        DexNavDrawHiddenIcons();
    }
    else
    {
        DexNavDrawIcons();
        DexNavUpdateSearchWindow(sDexNavSearchDataPtr->proximity, searchLevel);
    }
    
    FlagSet(FLAG_SYS_DEXNAV_SEARCH);
    gPlayerAvatar.creeping = TRUE;  //initialize as true in case mon appears beside you
    gTasks[taskId].tProximity = gSprites[gPlayerAvatar.spriteId].x;
    gTasks[taskId].tFrameCount = 0;
    gTasks[taskId].func = Task_DexNavSearch;
    IncrementGameStat(GAME_STAT_DEXNAV_SCANNED);
}

static void Task_InitDexNavSearch(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u16 species    = task->tSpecies;
    u8 environment = task->tEnvironment;
    
    sDexNavSearchDataPtr = AllocZeroed(sizeof(struct DexNavSearch));
    
    // assign non-objects to struct
    sDexNavSearchDataPtr->species = species;
    sDexNavUiDataPtr->currentEnviorment = environment;  //updated in DexNavTryGenerateMonLevel if hidden mon
    sDexNavSearchDataPtr->isHiddenMon = (environment == ENCOUNTER_TYPE_HIDDEN) ? TRUE : FALSE;
    sDexNavSearchDataPtr->monLevel = DexNavTryGenerateMonLevel(species, environment);
    
    if (gSaveBlock1Ptr->flashLevel > 0)
    {
        Free(sDexNavSearchDataPtr);
        FreeMonIconPalettes();
        ScriptContext1_SetupScript(EventScript_TooDark);
        DestroyTask(taskId);
        return;
    }
    else if (sDexNavSearchDataPtr->monLevel == MON_LEVEL_NONEXISTENT || !DexnavIsTileUsable(sDexNavUiDataPtr->currentEnviorment))
    {
        Free(sDexNavSearchDataPtr);
        FreeMonIconPalettes();
        ScriptContext1_SetupScript(EventScript_NotFoundNearby);
        DestroyTask(taskId);
        return;
    }
    else {
        //Start Encounter
        CreateDexNavWildMon(sDexNavSearchDataPtr->species, sDexNavSearchDataPtr->potential, sDexNavSearchDataPtr->monLevel, 
          sDexNavSearchDataPtr->abilityNum, sDexNavSearchDataPtr->heldItem, sDexNavSearchDataPtr->moves);
        
        FlagClear(FLAG_SYS_DEXNAV_SEARCH);
        gDexnavBattle = TRUE;        
        ScriptContext1_SetupScript(EventScript_StartDexNavBattle);
        Free(sDexNavSearchDataPtr);
        DestroyTask(taskId);
        return;
    }
    
    /*sDexNavSearchDataPtr->hiddenSearch = FALSE;
    task->tRevealed = TRUE; //search window revealed
    task->func = Task_SetUpDexNavSearch;*/
}

static void DexNavDrawPotentialStars(u8 potential, u8* dst)
{
    u8 spriteId;
    u32 i;
    
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
    {
        spriteId = 0xFF;
        if (potential > i)
            spriteId = CreateSprite(&sPotentialStarTemplate, SPECIES_ICON_X - 20, GetSearchWindowY() + 4 + (i * 8), 0);
        
        dst[i] = spriteId;
        if (spriteId != 0xFF)
            gSprites[spriteId].invisible = TRUE;
    }
}

/*static void DexNavDrawSight(u8 sightLevel, u8* dst)
{
    //LoadSpritePalette(&sHeldItemSpritePalette);
    *dst = CreateSprite(&sSightTemplate, 176 + (16 / 2), GetSearchWindowY() + 18, 0);
    if (*dst != 0xFF)
        DexNavSightUpdate(sightLevel);
};*/

static void DexNavUpdateDirectionArrow(void)
{
    u16 tileX = sDexNavSearchDataPtr->tileX;
    u16 tileY = sDexNavSearchDataPtr->tileY;
    u16 playerX = gSaveBlock1Ptr->pos.x + 7;
    u16 playerY = gSaveBlock1Ptr->pos.y + 7;
    u16 deltaX = abs(tileX - playerX);
    u16 deltaY = abs(tileY - playerY);
    const u8 *str;
    u8 windowId = sDexNavSearchDataPtr->windowId;
    
    FillWindowPixelRect(windowId, PIXEL_FILL(1), SEARCH_ARROW_X, SEARCH_ARROW_Y, 12, 12);
    if (deltaX <= 1 && deltaY <= 1)
    {
        str = gText_EmptyString2;
    }
    else if (deltaX > deltaY)
    {
        if (playerX > tileX)
            str = sText_ArrowLeft;  //player to right
        else
            str = sText_ArrowRight; //player to left
    }
    else //greater Y diff
    {
        if (playerY > tileY)
            str = sText_ArrowUp;    //player below
        else
            str = sText_ArrowDown;  //player above
    }
    
    AddTextPrinterParameterized3(windowId, 1, SEARCH_ARROW_X, SEARCH_ARROW_Y, sSearchFontColor, TEXT_SKIP_DRAW, str);
    CopyWindowToVram(windowId, 2);
}

static void DexNavDrawIcons(void)
{
    u16 species = sDexNavSearchDataPtr->species;
    
    // init sprite ids
    /*sDexNavSearchDataPtr->iconSpriteId = 0xFF;
    sDexNavSearchDataPtr->itemSpriteId = 0xFF;
    for (i = 0; i < NELEMS(sDexNavSearchDataPtr->starSpriteIds); i++)
        sDexNavSearchDataPtr->starSpriteIds[i] = 0xFF;*/
    
    DrawSearchWindow(species, sDexNavSearchDataPtr->potential, FALSE);
    DrawDexNavSearchMonIcon(species, &sDexNavSearchDataPtr->iconSpriteId, GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT));
    DrawDexNavSearchHeldItem(&sDexNavSearchDataPtr->itemSpriteId);
    DexNavDrawPotentialStars(sDexNavSearchDataPtr->potential, &sDexNavSearchDataPtr->starSpriteIds[0]);
    DexNavUpdateDirectionArrow();
    //DexNavDrawSight(sDexNavSearchDataPtr->proximity, &sDexNavSearchDataPtr->eyeSpriteId);
}

/////////////////////
//// SEARCH TASK ////
/////////////////////
bool8 TryStartDexnavSearch(void)
{
    u8 taskId;
    u16 species    = VarGet(VAR_DEXNAV_SPECIES);
    u16 enviorment = VarGet(VAR_DEXNAV_ENVIORMENT);
    
    if (FlagGet(FLAG_SYS_DEXNAV_SEARCH) || species == SPECIES_NONE)
        return FALSE;
    
    HideMapNamePopUpWindow();
    ChangeBgY_ScreenOff(0, 0, 0);
    taskId = CreateTask(Task_InitDexNavSearch, 0);
    gTasks[taskId].tSpecies     = species;
    gTasks[taskId].tEnvironment = enviorment;
    PlaySE(SE_DEX_SEARCH);
    return FALSE;   //we dont actually want to enable the script context
}

void EndDexNavSearch(u8 taskId)
{
    FlagClear(FLAG_SYS_DEXNAV_SEARCH);
    DestroyTask(taskId);
    RemoveDexNavWindowAndGfx();
    FieldEffectStop(&gSprites[sDexNavSearchDataPtr->fldEffSpriteId], sDexNavSearchDataPtr->fldEffId);
    Free(sDexNavSearchDataPtr);
}

static void EndDexNavSearchSetupScript(const u8 *script, u8 taskId)
{
    gSaveBlock1Ptr->dexNavChain = 0;   //reset chain
    EndDexNavSearch(taskId);
    ScriptContext1_SetupScript(script);
}

static u8 GetMovementProximityBySearchLevel(void)
{
    if (sDexNavSearchDataPtr->searchLevel < 20)
        return 2;
    else if (sDexNavSearchDataPtr->searchLevel < 50)
        return 3;
    else
        return 4;
}

static void Task_RevealHiddenMon(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u16 species = sDexNavSearchDataPtr->species;
    
    // remove owned icon if it exists
    if (sDexNavSearchDataPtr->ownedIconSpriteId != 0xFF)
    {
        DestroySprite(&gSprites[sDexNavSearchDataPtr->ownedIconSpriteId]);
        sDexNavSearchDataPtr->ownedIconSpriteId = 0xFF;
    }
    
    // remove exclamation if it exists
    if (sDexNavSearchDataPtr->exclamationSpriteId != 0xFF)
    {
        DestroySprite(&gSprites[sDexNavSearchDataPtr->exclamationSpriteId]);
        sDexNavSearchDataPtr->exclamationSpriteId = 0xFF;
    }
    
    if (species == SPECIES_NONE)
    {
        u8 index;
        
        //if not seen, hide name and whiteout mon
        DrawSearchWindow(species, sDexNavSearchDataPtr->potential, TRUE);
        DrawDexNavSearchMonIcon(species, &sDexNavSearchDataPtr->iconSpriteId, FALSE);
        // whiteout icon
        index = IndexOfSpritePaletteTag(gSprites[sDexNavSearchDataPtr->iconSpriteId].template->paletteTag);
        CpuCopy16(&gPlttBufferUnfaded[0x100 + index * 16], sDexNavSearchDataPtr->palBuffer, 32);
        TintPalette_CustomTone(sDexNavSearchDataPtr->palBuffer, 16, 510, 510, 510);
        LoadPalette(sDexNavSearchDataPtr->palBuffer, 0x100 + index * 16, 32);
    }
    else
    {
        DrawSearchWindow(species, sDexNavSearchDataPtr->potential, FALSE);
        DrawDexNavSearchMonIcon(species, &sDexNavSearchDataPtr->iconSpriteId, GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT));
    }
    
    DexNavUpdateDirectionArrow();
    task->func = Task_DexNavSearch;
    task->tFrameCount = 0;  //restart search clock
}

static void Task_DexNavSearch(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    
    if (sDexNavSearchDataPtr->proximity > MAX_PROXIMITY)
    { // out of range
        if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed)
            EndDexNavSearch(taskId);
        else
            EndDexNavSearchSetupScript(EventScript_LostSignal, taskId);
        return;
    }
    
    /*/if (sDexNavSearchDataPtr->proximity <= CREEPING_PROXIMITY && !gPlayerAvatar.creeping && task->tFrameCount > 60)
    { //should be creeping but player walks normally
        if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed)
            EndDexNavSearch(taskId);
        else
            EndDexNavSearchSetupScript(EventScript_MovedTooFast, taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->proximity <= SNEAKING_PROXIMITY && TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_DASH | PLAYER_AVATAR_FLAG_BIKE)) 
    { // running/biking too close
        //always do event script, even if player hasn't revealed a hidden mon. It's assumed they would be creeping towards it
        EndDexNavSearchSetupScript(EventScript_MovedTooFast, taskId);
        return;
    }/*/
    
    if (ScriptContext2_IsEnabled() == TRUE)
    { // check if script just executed
        //gSaveBlock1Ptr->dexNavChain = 0;  //issue with reusable repels
        EndDexNavSearch(taskId);
        return;
    }
    
    if (gTasks[taskId].tFrameCount > DEXNAV_TIMEOUT * 60)
    { // player took too long
        if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed)
            EndDexNavSearch(taskId);
        else
            EndDexNavSearchSetupScript(EventScript_PokemonGotAway, taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->proximity < 1)
    {
        CreateDexNavWildMon(sDexNavSearchDataPtr->species, sDexNavSearchDataPtr->potential, sDexNavSearchDataPtr->monLevel, 
          sDexNavSearchDataPtr->abilityNum, sDexNavSearchDataPtr->heldItem, sDexNavSearchDataPtr->moves);
        
        FlagClear(FLAG_SYS_DEXNAV_SEARCH);
        gDexnavBattle = TRUE;        
        ScriptContext1_SetupScript(EventScript_StartDexNavBattle);
        Free(sDexNavSearchDataPtr);
        DestroyTask(taskId);
        return;
    }
    
    if (sDexNavSearchDataPtr->hiddenSearch && !task->tRevealed &&
        (JOY_NEW(R_BUTTON) || (sDexNavSearchDataPtr->proximity < CREEPING_PROXIMITY)))
    {
        PlaySE(SE_DEX_SEARCH);
        ClearStdWindowAndFrameToTransparent(sDexNavSearchDataPtr->windowId, FALSE);
        CopyWindowToVram(sDexNavSearchDataPtr->windowId, 3);
        RemoveWindow(sDexNavSearchDataPtr->windowId);
        DestroySprite(&gSprites[sDexNavSearchDataPtr->iconSpriteId]);
        task->tRevealed = TRUE; //regular dexnav search
        //sDexNavSearchDataPtr->hiddenSearch = FALSE; //now its a regular dexnav search
        task->func = Task_RevealHiddenMon;
        return;
    }

    /*/Caves and water the pokemon moves around
    if ((sDexNavUiDataPtr->currentEnviorment == ENCOUNTER_TYPE_WATER || GetCurrentMapType() == MAP_TYPE_UNDERGROUND)
        && sDexNavSearchDataPtr->proximity < GetMovementProximityBySearchLevel() && sDexNavSearchDataPtr->movementCount < 2
        && task->tRevealed)
    {
        bool8 ret;
        
        FieldEffectStop(&gSprites[sDexNavSearchDataPtr->fldEffSpriteId], sDexNavSearchDataPtr->fldEffId);
        while (1) {
            if (TryStartHiddenMonFieldEffect(sDexNavUiDataPtr->currentEnviorment, 10, 10, TRUE))
                break;
        }
        
        sDexNavSearchDataPtr->movementCount++;
    }*/

    DexNavProximityUpdate();
    if (task->tProximity != sDexNavSearchDataPtr->proximity)
    {
        //player has moved
        if (task->tRevealed)    //update search window info only if hidden mon has been revealed (always true for search mode)
            DexNavUpdateSearchWindow(sDexNavSearchDataPtr->proximity, sDexNavSearchDataPtr->searchLevel);
        
        task->tProximity = sDexNavSearchDataPtr->proximity;
    }
    
    task->tFrameCount++;
}

/*static void DexNavSightUpdate(u8 index)
{
    u8 spriteId = sDexNavSearchDataPtr->eyeSpriteId;
    
    if (spriteId != 0xFF)
        StartSpriteAnim(&gSprites[spriteId], index);
}*/

static void DexNavUpdateSearchWindow(u8 proximity, u8 searchLevel)
{
    bool8 hideName = FALSE;
    u16 species = sDexNavSearchDataPtr->species;

    if (species == SPECIES_NONE)
        hideName = TRUE;    //if a detector mode hidden search and player hasn't seen the mon, hide info
    
    FillWindowPixelBuffer(sDexNavSearchDataPtr->windowId, PIXEL_FILL(1));   //clear window
    AddSearchWindowText(species, proximity, searchLevel, hideName);
    
    DexNavUpdateDirectionArrow();
    
    //init hidden sprites
    if (sDexNavSearchDataPtr->itemSpriteId != 0xFF)
        gSprites[sDexNavSearchDataPtr->itemSpriteId].invisible = TRUE;
    if (sDexNavSearchDataPtr->starSpriteIds[0] != 0xFF)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[0]].invisible = TRUE;
    if (sDexNavSearchDataPtr->starSpriteIds[1] != 0xFF)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[1]].invisible = TRUE;
    if (sDexNavSearchDataPtr->starSpriteIds[2] != 0xFF)
        gSprites[sDexNavSearchDataPtr->starSpriteIds[2]].invisible = TRUE;
    
    if (proximity <= SNEAKING_PROXIMITY)
    {
        if (searchLevel > 2 && sDexNavSearchDataPtr->heldItem)
        {
            // toggle item view
            if (sDexNavSearchDataPtr->itemSpriteId != 0xFF)
                gSprites[sDexNavSearchDataPtr->itemSpriteId].invisible = FALSE;
        }
        
        if (searchLevel > 4)
        {
            if (sDexNavSearchDataPtr->starSpriteIds[0] != 0xFF)
                gSprites[sDexNavSearchDataPtr->starSpriteIds[0]].invisible = FALSE;

            if (sDexNavSearchDataPtr->starSpriteIds[1] != 0xFF)
                gSprites[sDexNavSearchDataPtr->starSpriteIds[1]].invisible = FALSE;

            if (sDexNavSearchDataPtr->starSpriteIds[2] != 0xFF)
                gSprites[sDexNavSearchDataPtr->starSpriteIds[2]].invisible = FALSE;
        }
    }
}

//////////////////////////////
//// DEXNAV MON GENERATOR ////
//////////////////////////////
static void CreateDexNavWildMon(u16 species, u8 potential, u8 level, u8 abilityNum, u16 item, u16* moves)
{
    struct Pokemon* mon = &gEnemyParty[0];
    u8 iv[3] = {NUM_STATS};
    u8 i;
    u8 perfectIv = 31;
    
    if (DexNavTryMakeShinyMon())
        FlagSet(FLAG_SHINY_CREATION); // just easier this way
    
    CreateWildMon(species, level, FALSE);  // shiny rate bonus handled in CreateBoxMon
    
    // Pick random, unique IVs to set to 31. The number of perfect IVs that are assigned is equal to the potential
    iv[0] = Random() % NUM_STATS;               // choose 1st perfect stat
    do {
        iv[1] = Random() % NUM_STATS;
        iv[2] = Random() % NUM_STATS;
    } while ((iv[1] == iv[0])                   // unique 2nd perfect stat
      || (iv[2] == iv[0] || iv[2] == iv[1]));   // unique 3rd perfect stat
    
    if (potential > 2 && iv[2] != NUM_STATS)
        SetMonData(mon, MON_DATA_HP_IV + iv[2], &perfectIv);
    if (potential > 1 && iv[1] != NUM_STATS)
        SetMonData(mon, MON_DATA_HP_IV + iv[1], &perfectIv);
    if (potential > 0 && iv[0] != NUM_STATS)
        SetMonData(mon, MON_DATA_HP_IV + iv[0], &perfectIv);
    
    //Set ability
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilityNum);
    
    // Set Held Item
    if (item)
        SetMonData(mon, MON_DATA_HELD_ITEM, &item);

    //Set moves
    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (moves[i] != MOVE_NONE)
            SetMonMoveSlot(mon, moves[i], i);
    }

    CalculateMonStats(mon);
    FlagClear(FLAG_SHINY_CREATION);
}

// gets a random level of the species based on map data.
//if it was a hidden encounter, updates the environment it is to be found from the wildheader encounterRate
static u8 DexNavTryGenerateMonLevel(u16 species, u8 environment)
{
    u8 levelBase = ChooseWildMonLevel();
    u8 levelBonus = gSaveBlock1Ptr->dexNavChain / 5;

    if (GetEncounterLevelFromMapData(species, environment) == MON_LEVEL_NONEXISTENT || gMapHeader.mapType == MAP_TYPE_UNDERWATER)
        return MON_LEVEL_NONEXISTENT; //species not found in the area
    
    if (Random() % 100 < 4)
        levelBonus += 10; //4% chance of having a +10 level

    if (levelBase + levelBonus > GetLevelCap() && GetLevelCap() < MAX_LEVEL)
        return GetLevelCap();
    else if (levelBase + levelBonus > MAX_LEVEL)
        return MAX_LEVEL;
    else
        return levelBase + levelBonus;
}

static void DexNavGenerateMoveset(u16 species, u8 searchLevel, u8 encounterLevel, u16* moveDst)
{
    bool8 genMove = FALSE;
    u16 randVal = Random() % 100;
    u16 i;
    u16 eggMoveBuffer[EGG_MOVES_ARRAY_COUNT];

    // see if first move slot should be an egg move
    if (searchLevel < 5)
    {
        #if (SEARCHLEVEL0_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL0_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 10)
    {
        #if (SEARCHLEVEL5_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL5_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 25)
    {
        #if (SEARCHLEVEL10_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL10_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 50)
    {
        #if (SEARCHLEVEL25_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL25_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else if (searchLevel < 100)
    {
        #if (SEARCHLEVEL50_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL50_MOVECHANCE)
            genMove = TRUE;
        #endif
    }
    else
    {
        #if (SEARCHLEVEL100_MOVECHANCE != 0)
        if (randVal < SEARCHLEVEL100_MOVECHANCE)
            genMove = TRUE;
        #endif
    }

    // Generate a wild mon just to get the initial moveset (later overwritten by CreateDexNavWildMon)
    CreateWildMon(species, encounterLevel, FALSE);

    // Store generated mon moves into Dex Nav Struct
    for (i = 0; i < MAX_MON_MOVES; i++)
        moveDst[i] = GetMonData(&gEnemyParty[0], MON_DATA_MOVE1 + i, NULL);

    // set first move slot to a random egg move if search level is good enough    
    if (genMove)
    {
        u8 numEggMoves = GetEggMoves(GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL), eggMoveBuffer);
        if (numEggMoves != 0)
            moveDst[0] = eggMoveBuffer[Random() % numEggMoves];
    }
}

static u16 DexNavGenerateHeldItem(u16 species, u8 searchLevel)
{
    u16 randVal = Random() % 100;
    u8 searchLevelInfluence = searchLevel >> 1;
    u16 item1 = gBaseStats[species].item1;
    u16 item2 = gBaseStats[species].item2;
    
    // if both are the same, 100% to hold
    if (item1 == item2)
        return item1;

    // if no items can be held, then yeah...no items
    if (item2 == ITEM_NONE && item1 == ITEM_NONE)
        return ITEM_NONE;

    // if only one entry, 50% chance
    if (item2 == ITEM_NONE && item1 != ITEM_NONE)
        return (randVal < 50) ? item1 : ITEM_NONE;

    // if both are distinct item1 = 50% + srclvl/2; item2 = 5% + srchlvl/2
    if (randVal < (50 + searchLevelInfluence + 5 + searchLevel))
        return (randVal > 5 + searchLevelInfluence) ? item1 : item2;
    else
        return ITEM_NONE;

    return ITEM_NONE;
}

static u8 DexNavGetAbilityNum(u16 species, u8 searchLevel)
{
    bool8 genAbility = FALSE;
    u16 randVal = Random() % 100;
    u8 abilityNum = 0;
    
    if (searchLevel < 5)
    {
        #if (SEARCHLEVEL0_ABILITYCHANCE != 0)
        if (randVal < SEARCHLEVEL0_ABILITYCHANCE)
            genAbility = TRUE;
        #endif
    }
    else if (searchLevel < 10)
    {
        #if (SEARCHLEVEL5_ABILITYCHANCE != 0)
        if (randVal < SEARCHLEVEL5_ABILITYCHANCE)
            genAbility = TRUE;
        #endif
    }
    else if (searchLevel < 25)
    {
        #if (SEARCHLEVEL10_ABILITYCHANCE != 0)
        if (randVal < SEARCHLEVEL10_ABILITYCHANCE)
            genAbility = TRUE;
        #endif
    }
    else if (searchLevel < 50)
    {
        #if (SEARCHLEVEL25_ABILITYCHANCE != 0)
        if (randVal < SEARCHLEVEL25_ABILITYCHANCE)
            genAbility = TRUE;
        #endif
    }
    else if (searchLevel < 100)
    {
        #if (SEARCHLEVEL50_ABILITYCHANCE != 0)
        if (randVal < SEARCHLEVEL50_ABILITYCHANCE)
            genAbility = TRUE;
        #endif
    }
    else
    {
        #if (SEARCHLEVEL100_ABILITYCHANCE != 0)
        if (randVal < SEARCHLEVEL100_ABILITYCHANCE)
            genAbility = TRUE;
        #endif
    }
    
    #ifdef BATTLE_ENGINE    // if using RHH, the base stats abilities field is expanded
    if (genAbility && gBaseStats[species].abilities[2] != ABILITY_NONE && GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
    #else
    if (genAbility && gBaseStats[species].abilityHidden != ABILITY_NONE && GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
    #endif
    {
        //Only give hidden ability if Pokemon has been caught before
        abilityNum = 2;
    }
    else
    {
        //Pick a normal ability of that Pokemon
        if (gBaseStats[species].abilities[1] != ABILITY_NONE)
            abilityNum = Random() & 1;
        else
            abilityNum = 0;
    }
    
    return abilityNum;
}

static u8 DexNavGeneratePotential(u8 searchLevel)
{
    u8 genChance = 0;
    int randVal = Random() % 100;
    
    if (searchLevel < 5)
    {
        genChance = SEARCHLEVEL0_ONESTAR + SEARCHLEVEL0_TWOSTAR + SEARCHLEVEL0_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL0_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL0_ONESTAR + SEARCHLEVEL0_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 10)
    {
        genChance = SEARCHLEVEL5_ONESTAR + SEARCHLEVEL5_TWOSTAR + SEARCHLEVEL5_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL5_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL5_ONESTAR + SEARCHLEVEL5_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 25)
    {
        genChance = SEARCHLEVEL10_ONESTAR + SEARCHLEVEL10_TWOSTAR + SEARCHLEVEL10_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL10_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL10_ONESTAR + SEARCHLEVEL10_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 50)
    {
        genChance = SEARCHLEVEL25_ONESTAR + SEARCHLEVEL25_TWOSTAR + SEARCHLEVEL25_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL25_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL25_ONESTAR + SEARCHLEVEL25_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else if (searchLevel < 100)
    {
        genChance = SEARCHLEVEL50_ONESTAR + SEARCHLEVEL50_TWOSTAR + SEARCHLEVEL50_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL50_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL50_ONESTAR + SEARCHLEVEL50_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    else
    {
        genChance = SEARCHLEVEL100_ONESTAR + SEARCHLEVEL100_TWOSTAR + SEARCHLEVEL100_THREESTAR;
        if (randVal < genChance)
        {
            // figure out which star it is
            if (randVal < SEARCHLEVEL100_ONESTAR)
                return 1;
            else if (randVal < (SEARCHLEVEL100_ONESTAR + SEARCHLEVEL100_TWOSTAR))
                return 2;
            else
                return 3;
        }
    }
    
    return 0;   // No potential
}

static u8 GetEncounterLevelFromMapData(u16 species, u8 environment)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    const struct WildPokemonInfo *landMonsInfo      = gWildMonHeaders[headerId].landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo     = gWildMonHeaders[headerId].waterMonsInfo;
    const struct WildPokemonInfo *hiddenMonsInfo    = gWildMonHeaders[headerId].hiddenMonsInfo;
    const struct WildPokemonInfo* honeyMonsInfo     = gWildMonHeaders[headerId].honeyMonsInfo;
    const struct WildPokemonInfo* rockSmashMonsInfo = gWildMonHeaders[headerId].rockSmashMonsInfo;
    const struct WildPokemonInfo* fishingMonsInfo   = gWildMonHeaders[headerId].fishingMonsInfo;
    u8 min = 100;
    u8 max = 0;
    u8 i;

    if (IsDisabledForRoute(environment)) return MON_LEVEL_NONEXISTENT;
    
    switch (environment)
    {
    case ROW_LAND_TOP:    // grass
        if (landMonsInfo == NULL)
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles
        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            if (GetSpecies(landMonsInfo, i) == species)
            {
                min = (min < landMonsInfo->wildPokemon[i].minLevel) ? min : landMonsInfo->wildPokemon[i].minLevel;
                max = (max > landMonsInfo->wildPokemon[i].maxLevel) ? max : landMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ROW_WATER:    //water
        if (waterMonsInfo == NULL || !FlagGet(DEXNAV_WATER_MONS_UNLOCK_FLAG))
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles

        for (i = 0; i < WATER_WILD_COUNT; i++)
        {
            if (GetSpecies(waterMonsInfo, i) == species)
            {
                min = (min < waterMonsInfo->wildPokemon[i].minLevel) ? min : waterMonsInfo->wildPokemon[i].minLevel;
                max = (max > waterMonsInfo->wildPokemon[i].maxLevel) ? max : waterMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ROW_FISHING:
        // Fishing mons
        if (fishingMonsInfo == NULL || !FlagGet(DEXNAV_FISHING_MONS_UNLOCK_FLAG))
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles

        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            if (GetSpecies(fishingMonsInfo, i) == species)
            {
                min = (min < fishingMonsInfo->wildPokemon[i].minLevel) ? min : fishingMonsInfo->wildPokemon[i].minLevel;
                max = (max > fishingMonsInfo->wildPokemon[i].maxLevel) ? max : fishingMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ROW_ROCK_SMASH:
        // Rock Smash mons
        if (rockSmashMonsInfo == NULL  || !FlagGet(DEXNAV_HEADBUTT_MONS_UNLOCK_FLAG))
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles

        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            if (GetSpecies(rockSmashMonsInfo, i) == species)
            {
                min = (min < rockSmashMonsInfo->wildPokemon[i].minLevel) ? min : rockSmashMonsInfo->wildPokemon[i].minLevel;
                max = (max > rockSmashMonsInfo->wildPokemon[i].maxLevel) ? max : rockSmashMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ROW_HONEY:
        // Honey mons
        if (honeyMonsInfo == NULL || !FlagGet(DEXNAV_HONEY_MONS_UNLOCK_FLAG))
            return MON_LEVEL_NONEXISTENT; //Hidden pokemon should only appear on walkable tiles or surf tiles

        for (i = 0; i < LAND_WILD_COUNT; i++)
        {
            if (GetSpecies(honeyMonsInfo, i) == species)
            {
                min = (min < honeyMonsInfo->wildPokemon[i].minLevel) ? min : honeyMonsInfo->wildPokemon[i].minLevel;
                max = (max > honeyMonsInfo->wildPokemon[i].maxLevel) ? max : honeyMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        break;
    case ROW_HIDDEN:
        if (hiddenMonsInfo == NULL || !CanFindHiddenPokemon())
            return MON_LEVEL_NONEXISTENT;
        
        for (i = 0; i < HIDDEN_WILD_COUNT; i++)
        {
            if (GetSpecies(hiddenMonsInfo, i) == species)
            {
                min = (min < hiddenMonsInfo->wildPokemon[i].minLevel) ? min : hiddenMonsInfo->wildPokemon[i].minLevel;
                max = (max > hiddenMonsInfo->wildPokemon[i].maxLevel) ? max : hiddenMonsInfo->wildPokemon[i].maxLevel;
            }
        }
        
        // use encounter rate to signify is hidden pokemon are on land or in water
        if (hiddenMonsInfo->encounterRate == 1)
            sDexNavUiDataPtr->currentEnviorment = ROW_WATER;
        else
            sDexNavUiDataPtr->currentEnviorment = ROW_LAND_TOP;
        break;
    default:
        return MON_LEVEL_NONEXISTENT;
    }

    if (max == 0)
        return MON_LEVEL_NONEXISTENT;

    #undef DISABLE_FOR_ROUTE

    return RandRange(min, max);
}


///////////
/// GUI ///
///////////
static const struct BgTemplate sDexNavMenuBgTemplates[2] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .priority = 0
    }, 
    {
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .priority = 1
    }
};

static void DexNav_VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void DexNav_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static bool8 DexNav_InitBgs(void)
{
    ResetVramOamAndBgCntRegs();
    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = Alloc(0x800);
    if (sBg1TilemapBuffer == NULL)
        return FALSE;
    
    memset(sBg1TilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sDexNavMenuBgTemplates, NELEMS(sDexNavMenuBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    ScheduleBgCopyTilemapToVram(1);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT , 0);
    ShowBg(0);
    ShowBg(1);
    return TRUE;
}

static bool8 DexNav_LoadGraphics(void)
{
    switch (sDexNavUiDataPtr->state)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, sDexNavGuiTiles, 0, 0, 0);
        sDexNavUiDataPtr->state++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(sDexNavGuiTilemap, sBg1TilemapBuffer);
            sDexNavUiDataPtr->state++;
        }
        break;
    case 2:
        LoadPalette(sDexNavGuiPal, 0, 64);
        sDexNavUiDataPtr->state++;
        break;
    default:
        sDexNavUiDataPtr->state = 0;
        return TRUE;
    }
    
    return FALSE;
}

static void UpdateCursorPosition(void)
{
    gSprites[sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_CURSOR]].x = ROW_LAND_ICON_X     + (24 * sDexNavUiDataPtr->cursorCol);
    gSprites[sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_CURSOR]].y = ROW_LAND_TOP_ICON_Y + (24 * sDexNavUiDataPtr->cursorRow);
    
    PrintCurrentSpeciesInfo();
}

static void CreateSelectionCursor(void)
{
    u8 spriteId;
    struct CompressedSpriteSheet spriteSheet;
    
    spriteSheet.data = sSelectionCursorGfx;
    spriteSheet.size = 0x200;
    spriteSheet.tag = SELECTION_CURSOR_TAG;
    LoadCompressedSpriteSheet(&spriteSheet);
    
    LoadPalette(sSelectionCursorPal, (16 * sSelectionCursorOam.paletteNum) + 0x100, 32);
    
    spriteId = CreateSprite(&sSelectionCursorSpriteTemplate, 12, 32, 0);  
    //gSprites[spriteId].data[1] = -1;
    
    sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_CURSOR] = spriteId;
    UpdateCursorPosition();
}

static u8 CreateNoDataIcon(s16 x, s16 y)
{
    return CreateSprite(&sNoDataIconTemplate, x, y, 0);
}

static bool8 CapturedAllLandMons(u16 headerId)
{
    u16 i, species;
    int count = 0;
    const struct WildPokemonInfo* landMonsInfo = gWildMonHeaders[headerId].landMonsInfo;
        
    if (landMonsInfo != NULL)
    {        
        for (i = 0; i < LAND_WILD_COUNT; ++i)
        {
            species = GetSpecies(landMonsInfo, i);
            if (species != SPECIES_NONE)
            {
                if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    break;
                
                count++;
            }
        }

        if (i >= LAND_WILD_COUNT && count > 0) //All land mons caught
            return TRUE;
    }
    else
    {
        return TRUE;    //technically, no mon data means you caught them all
    }

    return FALSE;
}

//Checks if all Pokemon that can be encountered while surfing have been capture
static bool8 CapturedAllWaterMons(u16 headerId)
{
    u32 i;
    u16 species;
    u8 count = 0;
    const struct WildPokemonInfo* waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;

    if (waterMonsInfo != NULL)
    {
        for (i = 0; i < WATER_WILD_COUNT; ++i)
        {
            species = GetSpecies(waterMonsInfo, i);
            if (species != SPECIES_NONE)
            {
                count++;
                if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    break;
            }
        }

        if (i >= WATER_WILD_COUNT && count > 0)
            return TRUE;
    }
    else
    {
        return TRUE;    //technically, no mon data means you caught them all
    }

    return FALSE;
}

static bool8 CapturedAllHiddenMons(u16 headerId)
{
    u32 i;
    u16 species;
    u8 count = 0;
    const struct WildPokemonInfo* hiddenMonsInfo = gWildMonHeaders[headerId].hiddenMonsInfo;
    
    if (hiddenMonsInfo != NULL)
    {
        for (i = 0; i < HIDDEN_WILD_COUNT; ++i)
        {
            species = GetSpecies(hiddenMonsInfo, i);
            if (species != SPECIES_NONE)
            {
                count++;
                if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    break;
            }
        }

        if (i >= HIDDEN_WILD_COUNT && count > 0)
            return TRUE;
    }
    else
    {
        return TRUE;    //technically, no mon data means you caught them all
    }

    return FALSE;
}

static void DexNavLoadCapturedAllSymbols(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    
    LoadCompressedSpriteSheetUsingHeap(&sCapturedAllPokemonSpriteSheet);

    if (CapturedAllLandMons(headerId))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, 152, 58, 0);

    if (CapturedAllWaterMons(headerId))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, 139, 17, 0);
    
    if (CapturedAllHiddenMons(headerId))
        CreateSprite(&sCaptureAllMonsSpriteTemplate, 114, 123, 0);
}

//#define WIN_DETAILS_TILE        0x3a3
static void DexNav_InitWindows(void)
{
    INIT_WINDOWS(sDexNavGuiWindowTemplates);
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0);
}

static void DexNavGuiFreeResources(void)
{
    Free(sDexNavUiDataPtr);
    Free(sBg1TilemapBuffer);
    FreeAllWindowBuffers();
}

static void CB1_InitDexNavSearch(void)
{
    u8 taskId;
    
    if (!gPaletteFade.active && !ScriptContext2_IsEnabled() && gMain.callback2 == CB2_Overworld)
    {
        SetMainCallback1(CB1_Overworld);
        taskId = CreateTask(Task_InitDexNavSearch, 0);
        gTasks[taskId].tSpecies = gSpecialVar_0x8000;
        gTasks[taskId].tEnvironment = gSpecialVar_0x8001;
    }
}

static void CB1_DexNavSearchCallback(void)
{
    CB1_InitDexNavSearch();
}

static void Task_DexNavExitAndSearch(u8 taskId)
{
    DexNavGuiFreeResources();
    DestroyTask(taskId);
    SetMainCallback1(CB1_DexNavSearchCallback);
    SetMainCallback2(CB2_ReturnToField);
}

static void Task_DexNavFadeAndExit(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sDexNavUiDataPtr->savedCallback);
        DexNavGuiFreeResources();
        DestroyTask(taskId);
    }
}

static void DexNavFadeAndExit(void)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_DexNavFadeAndExit, 0);
    SetVBlankCallback(DexNav_VBlankCB);
    SetMainCallback2(DexNav_MainCB);
}

static bool8 SpeciesInArray(u16 species, u8 section)
{
    u32 i;

    for (i = 0; i < LAND_WILD_COUNT; i++)
    {
        if (sDexNavUiDataPtr->routeSpecies[section][i] == species)
            return TRUE;
    }
    
    return FALSE;
}

// get unique wild encounters on current map
static void DexNavLoadEncounterData(void)
{
    u8 index = 0;
    u16 species;
    u32 i, j;
    u16 headerId = GetCurrentMapWildMonHeaderId();
    u8 newEnviorment = DEXNAV_ROWS_COUNT;
    const struct WildPokemonInfo* landMonsInfo      = gWildMonHeaders[headerId].landMonsInfo;
    const struct WildPokemonInfo* waterMonsInfo     = gWildMonHeaders[headerId].waterMonsInfo;
    const struct WildPokemonInfo* hiddenMonsInfo    = gWildMonHeaders[headerId].hiddenMonsInfo;
    const struct WildPokemonInfo* honeyMonsInfo     = gWildMonHeaders[headerId].honeyMonsInfo;
    const struct WildPokemonInfo* rockSmashMonsInfo = gWildMonHeaders[headerId].rockSmashMonsInfo;
    const struct WildPokemonInfo* fishingMonsInfo   = gWildMonHeaders[headerId].fishingMonsInfo;
    
    // nop struct data
    memset(sDexNavUiDataPtr->routeSpecies, 0, sizeof(sDexNavUiDataPtr->routeSpecies));

    for (i = 0; i < DEXNAV_ROWS_COUNT; i++) {
        index = 0;
        sDexNavUiDataPtr->routeSpeciesNum[i] = 0;

        if (IsDisabledForRoute(i)) continue;

        switch (i) {
            case ROW_LAND_TOP:
                // Land mons
                if (landMonsInfo != NULL && landMonsInfo->encounterRate != 0)
                {
                    if (!gSaveBlock2Ptr->encounterRandomizedMode && gSaveBlock2Ptr->encounterRandomizedLegendaryMode) {
                        for (j = 0; j < LAND_WILD_COUNT / 2; j++)
                        {
                            species = GetSpecies(landMonsInfo, j);
                            if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                                sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                        }
                    }
                    else {
                        for (j = 0; j < LAND_WILD_COUNT; j++)
                        {
                            species = GetSpecies(landMonsInfo, j);
                            if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                                sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                        }
                    }
                    
                    newEnviorment = i;
                }
            break;
            case ROW_WATER:
                // Water mons
                if (waterMonsInfo != NULL && waterMonsInfo->encounterRate != 0 && FlagGet(DEXNAV_WATER_MONS_UNLOCK_FLAG))
                {
                    for (j = 0; j < WATER_WILD_COUNT; j++)
                    {
                        species = GetSpecies(waterMonsInfo, j);
                        if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                            sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                    }

                    if (newEnviorment == DEXNAV_ROWS_COUNT)
                        newEnviorment = i;
                }
            break;
            case ROW_FISHING:
                // Fishing mons
                if (fishingMonsInfo != NULL && fishingMonsInfo->encounterRate != 0 && FlagGet(DEXNAV_FISHING_MONS_UNLOCK_FLAG))
                {
                    for (j = 0; j < FISH_WILD_COUNT; j++)
                    {
                        species = GetSpecies(fishingMonsInfo, j);
                        if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                            sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                    }

                    if (newEnviorment == DEXNAV_ROWS_COUNT)
                        newEnviorment = i;
                }
            break;
            case ROW_ROCK_SMASH:
                // Rock Smash mons
                if (rockSmashMonsInfo != NULL && rockSmashMonsInfo->encounterRate != 0 && FlagGet(DEXNAV_HEADBUTT_MONS_UNLOCK_FLAG))
                {
                    for (j = 0; j < ROCK_WILD_COUNT; j++)
                    {
                        species = GetSpecies(rockSmashMonsInfo, j);
                        if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                            sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                    }
                    
                    if (newEnviorment == DEXNAV_ROWS_COUNT)
                        newEnviorment = i;
                }
            break;
            case ROW_HONEY:
                // Honey mons
                if (honeyMonsInfo != NULL && honeyMonsInfo->encounterRate != 0 && FlagGet(DEXNAV_HONEY_MONS_UNLOCK_FLAG))
                {
                    for (j = 0; j < HONEY_WILD_COUNT; j++)
                    {
                        species = GetSpecies(honeyMonsInfo, j);
                        if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                            sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                    }
                    
                    if (newEnviorment == DEXNAV_ROWS_COUNT)
                        newEnviorment = i;
                }
            break;
            case ROW_HIDDEN:
                // Hidden mons
                if (hiddenMonsInfo != NULL && CanFindHiddenPokemon() && CanFindHiddenPokemon())
                {
                    for (j = 0; j < HIDDEN_WILD_COUNT; j++)
                    {
                        species = GetSpecies(hiddenMonsInfo, j);
                        if (species != SPECIES_NONE && !SpeciesInArray(species, i))
                            sDexNavUiDataPtr->routeSpecies[i][index++] = species;
                    }
                    
                    if (newEnviorment == DEXNAV_ROWS_COUNT)
                        newEnviorment = i;
                }
            break;
        }

        // Set the used rows in an array
        if (index != 0) {
            sDexNavUiDataPtr->routeRows[sDexNavUiDataPtr->rowNum] = i;
            sDexNavUiDataPtr->rowNum++;
        }
        
        sDexNavUiDataPtr->routeSpeciesNum[i] = index;
    }
    sDexNavUiDataPtr->currentEnviorment = newEnviorment;
}

#define TAG_DEXNAV_POKEMON_ICON 4135
static void FreeDexnavSpeciesIconSprite(u8 row, u8 num)
{
    if (sDexNavUiDataPtr->DexnavSpeciesIconsSprites[row][DEXNAV_SPRITE_ICON + num] != 0xFF)
    {
        FreeSpriteOamMatrix(&gSprites[sDexNavUiDataPtr->DexnavSpeciesIconsSprites[row][DEXNAV_SPRITE_ICON + num]]);
        DestroySprite(&gSprites[sDexNavUiDataPtr->DexnavSpeciesIconsSprites[row][DEXNAV_SPRITE_ICON + num]]);

        sDexNavUiDataPtr->DexnavSpeciesIconsSprites[row][DEXNAV_SPRITE_ICON + num] = 0xFF;
    }
}

static void DestroyAllMonIcons(void)
{
    u32 i, j;

    for (i = 0; i < DEXNAV_ROWS_COUNT; i++) {
        for (j = 0; j < NUM_POKEMON_ICONS; j++)
        {
           sDexNavUiDataPtr->DexnavSpeciesIconsSprites[i][j] = 0xFF;
        }
    }
}

static u8 TryDrawIconInSlot(u8 enviorment, u8 num, s16 x, s16 y)
{
    u16 species = sDexNavUiDataPtr->routeSpecies[enviorment][num];

    if (sDexNavUiDataPtr->DexnavSpeciesIconsSprites[enviorment][num] != 0xFF) //Already created
        return sDexNavUiDataPtr->DexnavSpeciesIconsSprites[enviorment][num];
    else if (species == SPECIES_NONE || species > NUM_SPECIES || sDexNavUiDataPtr->routeSpeciesNum[enviorment] == 0) //No species or enviorment locked
        return 0xFF;

	LoadGenderedMonIconPalette(species, 0);
    sDexNavUiDataPtr->DexnavSpeciesIconsSprites[enviorment][num] = CreateMonIcon(species, SpriteCB_MonIcon, x, y, 0, 0);
    
    gSprites[sDexNavUiDataPtr->DexnavSpeciesIconsSprites[enviorment][num]].invisible = TRUE;

    return sDexNavUiDataPtr->DexnavSpeciesIconsSprites[enviorment][num];
}

static void HideSpeciesIcons(void) {
    u8 i, j, spriteId;
    for (i = 0; i < DEXNAV_ROWS_COUNT; i++) {
        for (j = 0; j < NUM_POKEMON_ICONS; j++)
        {
            spriteId = sDexNavUiDataPtr->DexnavSpeciesIconsSprites[i][j];
            if (spriteId != 0xFF) {
                if (i == sDexNavUiDataPtr->currentEnviorment)
                    gSprites[spriteId].invisible = FALSE;
                else
                    gSprites[spriteId].invisible = TRUE;
            }
        }
    }
}

static void DrawSpeciesIcons(void)
{
    u8 i, j;
    s16 x, y;
    
    DestroyAllMonIcons();
    //LoadCompressedSpriteSheetUsingHeap(&sNoDataIconSpriteSheet);

    for (i = 0; i < DEXNAV_ROWS_COUNT; i++) {
        for (j = 0; j < NUM_POKEMON_ICONS; j++)
        {
            x = ROW_LAND_ICON_X     + (24 * (j % 3));
            y = ROW_LAND_TOP_ICON_Y + (24 * (j / 3));
            TryDrawIconInSlot(i, j, x, y);
        }
    }

    HideSpeciesIcons();
}

static u16 DexNavGetSpecies(void)
{
    u16 species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][sDexNavUiDataPtr->cursorCol + (sDexNavUiDataPtr->cursorRow * 3)];
    
    if (species >= NUM_SPECIES)
        return SPECIES_NONE;
    
    return species;
}

static void SetSpriteInvisibility(u8 spriteArrayId, bool8 invisible)
{
    gSprites[sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_TYPE_ICON_1 + spriteArrayId]].invisible = invisible;
}

// different from pokemon_summary_screen
#define TYPE_ICON_PAL_NUM_0     13
#define TYPE_ICON_PAL_NUM_1     14
#define TYPE_ICON_PAL_NUM_2     15
static const u8 sMoveTypeToOamPaletteNum[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL] = TYPE_ICON_PAL_NUM_0,
    [TYPE_FIGHTING] = TYPE_ICON_PAL_NUM_0,
    [TYPE_FLYING] = TYPE_ICON_PAL_NUM_1,
    [TYPE_POISON] = TYPE_ICON_PAL_NUM_1,
    [TYPE_GROUND] = TYPE_ICON_PAL_NUM_0,
    [TYPE_ROCK] = TYPE_ICON_PAL_NUM_0,
    [TYPE_BUG] = TYPE_ICON_PAL_NUM_2,
    [TYPE_GHOST] = TYPE_ICON_PAL_NUM_1,
    [TYPE_STEEL] = TYPE_ICON_PAL_NUM_0,
    [TYPE_MYSTERY] = TYPE_ICON_PAL_NUM_2,
    [TYPE_FIRE] = TYPE_ICON_PAL_NUM_0,
    [TYPE_WATER] = TYPE_ICON_PAL_NUM_1,
    [TYPE_GRASS] = TYPE_ICON_PAL_NUM_2,
    [TYPE_ELECTRIC] = TYPE_ICON_PAL_NUM_0,
    [TYPE_PSYCHIC] = TYPE_ICON_PAL_NUM_1,
    [TYPE_ICE] = TYPE_ICON_PAL_NUM_1,
    [TYPE_DRAGON] = TYPE_ICON_PAL_NUM_2,
    [TYPE_DARK] = TYPE_ICON_PAL_NUM_0,
    [TYPE_FAIRY] = TYPE_ICON_PAL_NUM_1, //based on battle_engine
    [TYPE_STELLAR] = TYPE_ICON_PAL_NUM_2,
};
static void SetTypeIconPosAndPal(u8 typeId, u8 x, u8 y, u8 spriteArrayId)
{
    struct Sprite *sprite;
        
    sprite = &gSprites[sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_TYPE_ICON_1 + spriteArrayId]];
    StartSpriteAnim(sprite, typeId);
    sprite->oam.paletteNum = sMoveTypeToOamPaletteNum[typeId];
    sprite->x = x + 16;
    sprite->y = y + 8;
    SetSpriteInvisibility(spriteArrayId, FALSE);
}

static const u8 sText_DexNav_Species[]          = _("Species");
static const u8 sText_DexNav_Type[]             = _("Type");
static const u8 sText_DexNav_MapName[]          = _("{STR_VAR_1} - {STR_VAR_3}{STR_VAR_2}");
static const u8 sText_DexNav_SpeciesName[]      = _("{NO}{STR_VAR_2} {STR_VAR_3}");
static const u8 sText_DexNav_SpeciesName_None[] = _("---");
static const u8 sText_DexNav_Caught_Type_No[]   = _("");
static const u8 sText_DexNav_Caught_Type_Yes[]  = _("{POKEBALL_ICON}");
static const u8 sText_DexNav_Innates[]          = _("Innates:");

static const u8 sText_DexNav_Land[]     = _("Land");
static const u8 sText_DexNav_Land_2[]   = _("Land 2");
static const u8 sText_DexNav_Water[]    = _("Water");
static const u8 sText_DexNav_Fishing[]  = _("Fishing");
static const u8 sText_DexNav_Hidden[]   = _("Hidden");
static const u8 sText_DexNav_Headbutt[] = _("Headbutt");//Also used for Rock Smash
static const u8 sText_DexNav_Honey[]    = _("Honey");
#define DEXNAV_TYPE_ICON_Y 69 - 12

enum{
    DEXNAV_MESSAGE_NONE,
    DEXNAV_MESSAGE_SEARCH,
    DEXNAV_MESSAGE_BUY,
    DEXNAV_MESSAGE_BUY_ALL,
    DEXNAV_MESSAGE_BUY_ALL_NOT_OWNED,
    DEXNAV_THANKS_FOR_PURCHASE,
    DEXNAV_THANKS_FOR_PURCHASE_SHINY,
    DEXNAV_COULD_NOT_GIVE_MON,
    DEXNAV_COULD_NOT_ENOUGH_FUNDS,
    NUM_DEXNAV_MESSAGES,
};

static const u8 sText_DexNav_Plus_Title[]              = _("Pokémon Elite Redux DexNav+");
static const u8 sText_DexNav_Title[]                   = _("Pokémon Elite Redux DexNav");
static const u8 sText_DexNav_Plus_Message_Default[]    = _("Welcome to the DexNav+! {R_BUTTON} Search\n{A_BUTTON} Get {START_BUTTON} Get All {STR_VAR_1}");
static const u8 sText_DexNav_Plus_Message_1[]          = _("{SELECT_BUTTON} Get New");
static const u8 sText_DexNav_Plus_Message_2[]          = _("{B_BUTTON} Exit");
static const u8 sText_DexNav_Message_Default[]         = _("Welcome to the Dexnav!\n{R_BUTTON} Search {B_BUTTON} Exit");
static const u8 sText_DexNav_Message_Buy[]             = _("Do you want to get {STR_VAR_1}?\n{A_BUTTON} Get {B_BUTTON} Cancel");
//static const u8 sText_DexNav_Message_Buy[]           = _("Do you want to get {STR_VAR_1}\nfor {STR_VAR_2} BP? {A_BUTTON} Get {B_BUTTON} Cancel");
static const u8 sText_DexNav_Message_Buy_All[]         = _("Get all the {STR_VAR_3} Pokémon on the\nroute? {A_BUTTON} Get {B_BUTTON} Cancel");
static const u8 sText_DexNav_Message_Search[]          = _("Do you want to search {STR_VAR_1}?\n{A_BUTTON} Get {B_BUTTON} Cancel");
static const u8 sText_DexNav_CouldNotGiveMon[]         = _("You don't have enough space for\nthis Pokemon {A_BUTTON} Continue.");
static const u8 sText_DexNav_CouldNotEnoughFunds[]     = _("You don't have enough Battle\n Points for this! {A_BUTTON} Continue.");
static const u8 sText_DexNav_ThanksForPurchase[]       = _("Enjoy your new Pokémon!\n{A_BUTTON} Continue.");
static const u8 sText_DexNav_ThanksForPurchase_Shiny[] = _("Enjoy your new Pokémon{SUM_SHINY}\n{A_BUTTON} Continue.");
static const u8 sText_DexNav_CurrentBP[]               = _("Current BP: {STR_VAR_1}");
static const u8 sText_DexNav_Price[]                   = _("Price: {STR_VAR_1}");

static const u8  gDexnavFieldIcon_Forest[]   = INCBIN_U8("graphics/ui_menus/dexnav/fields/forest.4bpp");
static const u8  gDexnavFieldIcon_Water[]    = INCBIN_U8("graphics/ui_menus/dexnav/fields/water.4bpp");
static const u8  gDexnavFieldIcon_Fishing[]  = INCBIN_U8("graphics/ui_menus/dexnav/fields/fishing.4bpp");
static const u8  gDexnavFieldIcon_Headbutt[] = INCBIN_U8("graphics/ui_menus/dexnav/fields/headbutt.4bpp");
static const u8  gDexnavFieldIcon_Hidden[]   = INCBIN_U8("graphics/ui_menus/dexnav/fields/hidden.4bpp");
static const u8  gDexnavFieldIcon_Honey[]    = INCBIN_U8("graphics/ui_menus/dexnav/fields/honey.4bpp");
static const u8  gDexnavFieldIcon_Selector[] = INCBIN_U8("graphics/ui_menus/dexnav/fields/selector.4bpp");

#define DEFAULT_DEXNAV_ROW 2

static u8 getEnviormentAt(u8 row) {
    u8 currentrow = 0;
    u8 rowToReturn = DEFAULT_DEXNAV_ROW;
    u8 numRows = sDexNavUiDataPtr->rowNum;

    do {
        currentrow = (currentrow + 1) % numRows;
    }
    while (sDexNavUiDataPtr->routeRows[currentrow] != sDexNavUiDataPtr->currentEnviorment);

    if (row == DEFAULT_DEXNAV_ROW)
        rowToReturn = sDexNavUiDataPtr->currentEnviorment;
    else if (row == DEFAULT_DEXNAV_ROW + 1)
        rowToReturn = sDexNavUiDataPtr->routeRows[(currentrow + 1) % numRows];
    else if (row == DEFAULT_DEXNAV_ROW + 2)
        rowToReturn = sDexNavUiDataPtr->routeRows[(currentrow + 2) % numRows];
    else if (row == DEFAULT_DEXNAV_ROW - 1) {
        rowToReturn = ((currentrow + numRows - 1)) % numRows;
        rowToReturn = sDexNavUiDataPtr->routeRows[rowToReturn];
    }
    else if (row == DEFAULT_DEXNAV_ROW - 2) {
        rowToReturn = ((currentrow + numRows - 2)) % numRows;
        rowToReturn = sDexNavUiDataPtr->routeRows[rowToReturn];
    }

    return rowToReturn;
}

u8 getMonPrice(u16 species)
{
    //To be expanded in the future
    return DEFAULT_DEXNAV_MON_PRICE;
}

static void EnviormentToStringVar(u8 enviorment) {
    //This only copies to gStringVar3
    switch (enviorment) {
        case ROW_LAND_TOP:
            StringCopy(gStringVar3, sText_DexNav_Land);
        break;
        case ROW_WATER:
            StringCopy(gStringVar3, sText_DexNav_Water);
        break;
        case ROW_HIDDEN:
            StringCopy(gStringVar3, sText_DexNav_Hidden);
        break;
        case ROW_FISHING:
            StringCopy(gStringVar3, sText_DexNav_Fishing);
        break;
        case ROW_ROCK_SMASH:
            StringCopy(gStringVar3, sText_DexNav_Headbutt);
        break;
        case ROW_HONEY:
            StringCopy(gStringVar3, sText_DexNav_Honey);
        break;
    }

}
    
static void PrintCurrentSpeciesInfo(void)
{
    u16 species = DexNavGetSpecies();
    u32 i, j, x, y;
    u8 type1, type2, offset;
    u8 price = getMonPrice(species);
    u8 font = FONT_SMALL_NARROW;
    u8 numRows = sDexNavUiDataPtr->rowNum;
    bool8 isEverythingCaught = hasAllMonsInEnviorment();

    // Clear windows
    FillWindowPixelBuffer(WINDOW_INFO, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    
    for (i = 0; i < DEXNAV_MAX_SHOWN_ROWS; i++) {
        j = getEnviormentAt(i);

        if ((numRows < 5 && (i == 0 || i == DEXNAV_MAX_SHOWN_ROWS - 1)) || (numRows == 1 && i != DEFAULT_DEXNAV_ROW))
            j++; //This does nothing
        else {
            switch (j) {
                case ROW_LAND_TOP:
                    BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Forest, 0, (i * 32), 48, 32);
                break;
                case ROW_WATER:
                    BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Water, 0, (i * 32), 48, 32);
                break;
                case ROW_HIDDEN:
                    BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Hidden, 0, (i * 32), 48, 32);
                break;
                case ROW_FISHING:
                    BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Fishing, 0, (i * 32), 48, 32);
                break;
                case ROW_ROCK_SMASH:
                    BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Headbutt, 0, (i * 32), 48, 32);
                break;
                case ROW_HONEY:
                    BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Honey, 0, (i * 32), 48, 32);
                break;
            }

            if (i == DEFAULT_DEXNAV_ROW)
                BlitBitmapToWindow(WINDOW_INFO, gDexnavFieldIcon_Selector, 0, (i * 32), 48, 32);
        }
    }

    //Title
    x = 9;
    y = 0;
    if (!FlagGet(DEXNAV_PLUS_UNLOCK_FLAG))
        AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8), sFontColor_White, TEXT_SKIP_DRAW, sText_DexNav_Title);
    else
        AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8), sFontColor_White, TEXT_SKIP_DRAW, sText_DexNav_Plus_Title);

    // Map Name
    x = 8;
    y = 3;
    GetMapName(gStringVar1, GetCurrentRegionMapSectionId(), 0);
    EnviormentToStringVar(sDexNavUiDataPtr->currentEnviorment);

    if (isEverythingCaught)
        StringCopy(gStringVar2, sText_DexNav_Caught_Type_Yes);
    else
        StringCopy(gStringVar2, sText_DexNav_Caught_Type_No);

    StringExpandPlaceholders(gStringVar4, sText_DexNav_MapName);

    AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_White, 0, gStringVar4);

    // BP
    x = 21;
    if (FlagGet(DEXNAV_PLUS_UNLOCK_FLAG) && !DEXNAV_MON_FREE) {
        ConvertIntToDecimalStringN(gStringVar1, gSaveBlock2Ptr->frontier.battlePoints, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar4, sText_DexNav_CurrentBP);
        AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_White, 0, gStringVar4);
    }
    
    // Species name
    y++;
    if (species == SPECIES_NONE)
        StringCopy(gStringVar1, sText_DexNav_SpeciesName_None);
    else
        StringCopy(gStringVar1, gSpeciesNames[species]);

    offset = GetStringCenterAlignXOffset(font, gStringVar1, 68);
    AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8) - 4 + offset, (y * 8) - 4, sFontColor_Black, 0, gStringVar1);

    y++;
    if (species == SPECIES_NONE)
        StringCopy(gStringVar4, sText_DexNav_SpeciesName_None);
    else {
        StringCopy(gStringVar1, gSpeciesNames[species]);
        if (GetSetPokedexFlag(species, FLAG_GET_CAUGHT))
            StringCopy(gStringVar3, sText_DexNav_Caught_Type_Yes);
        else
            StringCopy(gStringVar3, sText_DexNav_Caught_Type_No);
        ConvertIntToDecimalStringN(gStringVar2, SpeciesToNationalPokedexNum(species), STR_CONV_MODE_LEFT_ALIGN, 4);
        StringExpandPlaceholders(gStringVar4, sText_DexNav_SpeciesName);
    }

    offset = GetStringCenterAlignXOffset(font, gStringVar4, 68);
    AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8) - 4 + offset, (y * 8) - 4, sFontColor_Black, 0, gStringVar4);
        
    //Price
    y++;
    if (FlagGet(DEXNAV_PLUS_UNLOCK_FLAG) && !DEXNAV_MON_FREE) {
        ConvertIntToDecimalStringN(gStringVar1, price, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar4, sText_DexNav_Price);
    }
    else {
        StringCopy(gStringVar4, sText_DexNav_Type);
    }

    offset = GetStringCenterAlignXOffset(font, gStringVar4, 68);
    AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8) - 4 + offset, (y * 8) - 4, sFontColor_Black, 0, gStringVar4);

    // Type icon(s)
    type1 = gBaseStats[species].type1;
    type2 = gBaseStats[species].type2;
    if (species == SPECIES_NONE)
        type1 = type2 = TYPE_MYSTERY;
    
    if (type1 == type2)
    {
        SetTypeIconPosAndPal(type1, 160 + 22, DEXNAV_TYPE_ICON_Y, 0);
        SetSpriteInvisibility(1, TRUE);
    }
    else
    {
        SetTypeIconPosAndPal(type1, 165, DEXNAV_TYPE_ICON_Y, 0);
        SetTypeIconPosAndPal(type2, 166 + 33, DEXNAV_TYPE_ICON_Y, 1);
    }

    // Innates
    y = y + 4;
    StringCopy(gStringVar1, sText_DexNav_Innates);
    offset = GetStringCenterAlignXOffset(font, gStringVar1, 68);
    AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8) - 4 + offset, (y * 8) - 4, sFontColor_Black, 0, gStringVar1);
    y++;
    for (i = 0; i < NUM_ABILITY_SLOTS; i++)
    {
        if (gBaseStats[species].innates[i] == ABILITY_NONE)
            break;

        StringCopy(gStringVar1, gAbilities[gBaseStats[species].innates[i]].name);
        offset = GetStringCenterAlignXOffset(font, gStringVar1, 68);
        AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8) - 4 + offset, (y * 8) - 4, sFontColor_Black, 0, gStringVar1);
        y++;
    }

    // Message
    x = 9;
    y = 17;

    switch (sDexNavUiDataPtr->currentMessage)
    {
        default:
            if (!isEverythingCaught)
                StringCopy(gStringVar1, sText_DexNav_Plus_Message_1);
            else
                StringCopy(gStringVar1, sText_DexNav_Plus_Message_2);

            if (FlagGet(DEXNAV_PLUS_UNLOCK_FLAG) == TRUE)
                StringExpandPlaceholders(gStringVar4, sText_DexNav_Plus_Message_Default);
            else
                StringExpandPlaceholders(gStringVar4, sText_DexNav_Message_Default);

            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, gStringVar4);
        break;
        case DEXNAV_MESSAGE_BUY_ALL_NOT_OWNED:
            price = 0;
            for (i = 0; i < NUM_POKEMON_ICONS; i++)
            {
                species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][i];
                if (species != SPECIES_NONE && !GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                    price += getMonPrice(species);
            }

            price = (price * DEXNAV_BUY_ALL_DISCOUNT) / 100;
            ConvertIntToDecimalStringN(gStringVar1, price, STR_CONV_MODE_LEFT_ALIGN, 3);
            EnviormentToStringVar(sDexNavUiDataPtr->currentEnviorment);
            StringExpandPlaceholders(gStringVar4, sText_DexNav_Message_Buy_All);
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, gStringVar4);
        break;
        case DEXNAV_MESSAGE_BUY_ALL:
            price = 0;
            for (i = 0; i < NUM_POKEMON_ICONS; i++)
            {
                species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][i];
                if (species != SPECIES_NONE)
                    price += getMonPrice(species);
            }

            price = (price * DEXNAV_BUY_ALL_DISCOUNT) / 100;
            ConvertIntToDecimalStringN(gStringVar1, price, STR_CONV_MODE_LEFT_ALIGN, 3);
            EnviormentToStringVar(sDexNavUiDataPtr->currentEnviorment);
            StringExpandPlaceholders(gStringVar4, sText_DexNav_Message_Buy_All);
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, gStringVar4);
            break;
        case DEXNAV_THANKS_FOR_PURCHASE:
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, sText_DexNav_ThanksForPurchase);
        break;
        case DEXNAV_THANKS_FOR_PURCHASE_SHINY:
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, sText_DexNav_ThanksForPurchase_Shiny);
            break;
        case DEXNAV_COULD_NOT_ENOUGH_FUNDS:
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, sText_DexNav_CouldNotEnoughFunds);
        break;
        case DEXNAV_COULD_NOT_GIVE_MON:
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, sText_DexNav_CouldNotGiveMon);
        break;
        case DEXNAV_MESSAGE_SEARCH:
            StringCopy(gStringVar1, gSpeciesNames[species]);
            StringExpandPlaceholders(gStringVar4, sText_DexNav_Message_Search);
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, gStringVar4);
        break;
        case DEXNAV_MESSAGE_BUY:
            StringCopy(gStringVar1, gSpeciesNames[species]);
            ConvertIntToDecimalStringN(gStringVar2, price, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringExpandPlaceholders(gStringVar4, sText_DexNav_Message_Buy);
            AddTextPrinterParameterized3(WINDOW_INFO, font, (x * 8), (y * 8) - 4, sFontColor_Black, 0, gStringVar4);
        break;
    }
    
    CopyWindowToVram(WINDOW_INFO, 3);
    PutWindowTilemap(WINDOW_INFO);
}

static void CreateTypeIconSprites(void)
{
    u8 i;

    LoadCompressedSpriteSheet(&sSpriteSheet_MoveTypes);
    LoadCompressedPalette(gMoveTypes_Pal, 0x1D0, 0x60);
    for (i = 0; i < 2; i++)
    {
        if (sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_TYPE_ICON_1 + i] == 0xFF)
            sDexNavUiDataPtr->DexnavSprites[DEXNAV_SPRITE_TYPE_ICON_1 + i] = CreateSprite(&sSpriteTemplate_MoveTypes, 10, 10, 2);    
    
        SetSpriteInvisibility(i, TRUE);
    }
}

static bool8 DexNav_DoGfxSetup(void)
{
    u8 taskId, i;
    
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        gMain.state++;
        break;
    case 4:
        if (DexNav_InitBgs())
        {
            sDexNavUiDataPtr->state = 0;
            gMain.state++;
        }
        else
        {
            DexNavFadeAndExit();
            return TRUE;
        }
        break;
    case 5:
        if (DexNav_LoadGraphics() == TRUE)
            gMain.state++;
        break;
    case 6:
        DexNav_InitWindows();
        sDexNavUiDataPtr->currentEnviorment = ROW_LAND_TOP;
        sDexNavUiDataPtr->cursorCol = 0;
        gMain.state++;
        break;
    case 7:
        sDexNavUiDataPtr->currentMessage = 0;
        DexNavLoadEncounterData();
        gMain.state++;
        break;
    case 8:
        taskId = CreateTask(Task_DexNavWaitFadeIn, 0);
        gTasks[taskId].tSpecies = 0;
        gTasks[taskId].tEnvironment = sDexNavUiDataPtr->currentEnviorment;
        gMain.state++;
        break;
    case 9:
        for (i = 0; i < NUM_DEXNAV_SPRITES; i++)
        {
            sDexNavUiDataPtr->DexnavSprites[i] = 0xFF;
        }

        CreateTypeIconSprites();
        gMain.state++;
        break;
    case 10:
        DrawSpeciesIcons();
        CreateSelectionCursor();
        gMain.state++;
        break;
    case 11:
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 12:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(DexNav_VBlankCB);
        SetMainCallback2(DexNav_MainCB);
        return TRUE;
    }
    
    return FALSE;
}

static void DexNav_RunSetup(void)
{
    while (!DexNav_DoGfxSetup()) {}
}

static void DexNavGuiInit(MainCallback callback)
{
    if ((sDexNavUiDataPtr = AllocZeroed(sizeof(struct DexNavGUI))) == NULL)
    {
        SetMainCallback2(callback);
        return;
    }
    
    sDexNavUiDataPtr->state = 0;
    sDexNavUiDataPtr->savedCallback = callback;
    SetMainCallback2(DexNav_RunSetup);
}

void Task_OpenDexNavFromStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        DexNavGuiInit(CB2_ReturnToFieldWithOpenMenu);
        DestroyTask(taskId);
    }
}

static void Task_DexNavWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_DexNavMain;
}

#define DEXNAV_NUM_SPECIES_ROWS 4
#define DEXNAV_NUM_SPECIES_PER_ROW 3
#define DENAV_MAX_NUM_SPECIES 12

static void Task_DexNavMain(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u16 species;
    bool8 windowChanged = FALSE;
    
    if (IsSEPlaying())
        return;

    if (sDexNavUiDataPtr->currentMessage == DEXNAV_MESSAGE_NONE && !(JOY_NEW(A_BUTTON)) && !(JOY_NEW(B_BUTTON)))
    {
        if (JOY_NEW(DPAD_UP))
        {
            do {
                if (sDexNavUiDataPtr->cursorRow > 0)
                    sDexNavUiDataPtr->cursorRow--;
                else {
                    if (sDexNavUiDataPtr->currentEnviorment == 0)
                        sDexNavUiDataPtr->currentEnviorment = DEXNAV_ROWS_COUNT - 1;
                    else
                        sDexNavUiDataPtr->currentEnviorment--;

                    if (DexNavGetSpecies() == SPECIES_NONE)
                        sDexNavUiDataPtr->cursorRow = 0;

                    windowChanged = TRUE;
                }
            }
            while (sDexNavUiDataPtr->routeSpeciesNum[sDexNavUiDataPtr->currentEnviorment] == 0 || DexNavGetSpecies() == SPECIES_NONE);

            if (windowChanged)
                HideSpeciesIcons();

            PlaySE(SE_RG_BAG_CURSOR);
            UpdateCursorPosition();
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            do {
                if (sDexNavUiDataPtr->cursorRow < DEXNAV_NUM_SPECIES_ROWS - 1)
                    sDexNavUiDataPtr->cursorRow++;
                else {
                    if (sDexNavUiDataPtr->currentEnviorment < DEXNAV_ROWS_COUNT - 1)
                        sDexNavUiDataPtr->currentEnviorment++;
                    else
                        sDexNavUiDataPtr->currentEnviorment = 0;

                    sDexNavUiDataPtr->cursorRow = 0;
                    windowChanged = TRUE;
                }
            }
            while (sDexNavUiDataPtr->routeSpeciesNum[sDexNavUiDataPtr->currentEnviorment] == 0 || DexNavGetSpecies() == SPECIES_NONE);

            if (windowChanged)
                HideSpeciesIcons();
            
            PlaySE(SE_RG_BAG_CURSOR);
            UpdateCursorPosition();
        }
        else if (JOY_NEW(DPAD_LEFT))
        {
            
            do {
                if (sDexNavUiDataPtr->cursorCol > 0)
                    sDexNavUiDataPtr->cursorCol--;
                else
                    sDexNavUiDataPtr->cursorCol = DEXNAV_NUM_SPECIES_PER_ROW - 1;
            }
            while (DexNavGetSpecies() == SPECIES_NONE);
            
            PlaySE(SE_RG_BAG_CURSOR);
            UpdateCursorPosition();
        }
        else if (JOY_NEW(DPAD_RIGHT))
        {
            do {
                if (sDexNavUiDataPtr->cursorCol < DEXNAV_NUM_SPECIES_PER_ROW - 1)
                    sDexNavUiDataPtr->cursorCol++;
                else
                    sDexNavUiDataPtr->cursorCol = 0;
            }
            while (DexNavGetSpecies() == SPECIES_NONE);
            
            PlaySE(SE_RG_BAG_CURSOR);
            UpdateCursorPosition();
        }
        else if (JOY_NEW(R_BUTTON))
        {
            // check selection is valid. Play sound if invalid
            species = DexNavGetSpecies();
            
            if (species != SPECIES_NONE)
            {            
                //PlaySE(SE_DEX_SEARCH);
                //PlayCry(species, 0); ToDo
                
                // create value to store in a var
                VarSet(VAR_DEXNAV_SPECIES, species);
                VarSet(VAR_DEXNAV_ENVIORMENT, sDexNavUiDataPtr->currentEnviorment);

                gSpecialVar_0x8000 = species;
                gSpecialVar_0x8001 = sDexNavUiDataPtr->currentEnviorment;
                gSpecialVar_0x8002 = (sDexNavUiDataPtr->cursorRow == ROW_HIDDEN) ? TRUE : FALSE;
                PlaySE(SE_DEX_SEARCH);
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
                task->func = Task_DexNavExitAndSearch;
            }
            else
            {
                PlaySE(SE_FAILURE);
            }
        }
        else if (JOY_NEW(START_BUTTON) && FlagGet(DEXNAV_PLUS_UNLOCK_FLAG)) {
            if (sDexNavUiDataPtr->currentMessage == DEXNAV_MESSAGE_NONE) {
                sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_BUY_ALL;
                UpdateCursorPosition();
            }
        }
        else if (JOY_NEW(SELECT_BUTTON) && FlagGet(DEXNAV_PLUS_UNLOCK_FLAG) && !hasAllMonsInEnviorment()) {
            if (sDexNavUiDataPtr->currentMessage == DEXNAV_MESSAGE_NONE) {
                sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_BUY_ALL_NOT_OWNED;
                UpdateCursorPosition();
            }
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {   
        bool8 gotShiny = FALSE;
        u8 level = GetLevelCap();
        u16 loc = gSaveBlock1Ptr->location.mapNum;
        u16 locG = gSaveBlock1Ptr->location.mapGroup;
        if (FlagGet(DEXNAV_PLUS_UNLOCK_FLAG)) {
            switch (sDexNavUiDataPtr->currentMessage) {
                default:
                    sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_NONE;
                    UpdateCursorPosition();
                break;
                case DEXNAV_MESSAGE_NONE:
                    sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_BUY;
                    UpdateCursorPosition();
                break;
                case DEXNAV_MESSAGE_BUY:
                    species = DexNavGetSpecies();
                    if (gSaveBlock2Ptr->frontier.battlePoints < getMonPrice(species) && !DEXNAV_MON_FREE) {
                        sDexNavUiDataPtr->currentMessage = DEXNAV_COULD_NOT_ENOUGH_FUNDS;
                        UpdateCursorPosition();
                    }
                    else {
                        bool8 couldGiveMon = FALSE;
                        if (level >= MAX_LEVEL)
                            level = MAX_LEVEL;
                        
                        VarSet(VAR_DEXNAV_SHINY_FLAG, 1);
                        if (!IsRouteEncountered(loc, locG) || !gSaveBlock2Ptr->nuzlockeCaptures) {
                            couldGiveMon = ScriptGiveMon(species, level, ITEM_NONE, 0, 0, 0);
                        }
                        else {
                            couldGiveMon = 3;
                        }
                        if (couldGiveMon < 2) {
                            MarkRouteAsEncountered(loc, locG);
                            if (VarGet(VAR_DEXNAV_SHINY_FLAG) == 2)
                                gotShiny = TRUE;
                            VarSet(VAR_DEXNAV_SHINY_FLAG, 0);

                            if (!DEXNAV_MON_FREE)
                                gSaveBlock2Ptr->frontier.battlePoints = gSaveBlock2Ptr->frontier.battlePoints - getMonPrice(species);
                            if (gotShiny)
                                sDexNavUiDataPtr->currentMessage = DEXNAV_THANKS_FOR_PURCHASE_SHINY;
                            else
                                sDexNavUiDataPtr->currentMessage = DEXNAV_THANKS_FOR_PURCHASE;
                        }
                        else
                            sDexNavUiDataPtr->currentMessage = DEXNAV_COULD_NOT_GIVE_MON;
                        UpdateCursorPosition();
                    }
                break;
                case DEXNAV_MESSAGE_BUY_ALL_NOT_OWNED:{
                    u16 price = 0;
                    u8 i;
                    for (i = 0; i < NUM_POKEMON_ICONS; i++)
                    {
                        species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][i];
                        if (species != SPECIES_NONE && !GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
                            price += getMonPrice(species);
                    }
                    price = (price * DEXNAV_BUY_ALL_DISCOUNT) / 100;

                    if (gSaveBlock2Ptr->frontier.battlePoints < price && !DEXNAV_MON_FREE) {
                        sDexNavUiDataPtr->currentMessage = DEXNAV_COULD_NOT_ENOUGH_FUNDS;
                        UpdateCursorPosition();
                    }
                    else {
                        bool8 couldGiveMon = FALSE;
                        price = 0;

                        if (level >= MAX_LEVEL)
                            level = MAX_LEVEL;

                        for (i = 0; i < NUM_POKEMON_ICONS; i++)
                        {
                            couldGiveMon = FALSE;
                            species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][i];
                            if (species != SPECIES_NONE && !GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT) && !gSaveBlock2Ptr->nuzlockeCaptures) {
                                couldGiveMon = ScriptGiveMon(species, level, ITEM_NONE, 0, 0, 0);
                                if (couldGiveMon < 2) {
                                    VarSet(VAR_DEXNAV_SHINY_FLAG, 1);
                                    price += getMonPrice(species);
                                    if (VarGet(VAR_DEXNAV_SHINY_FLAG) == 2)
                                        gotShiny = TRUE;
                                    VarSet(VAR_DEXNAV_SHINY_FLAG, 0);
                                }
                            }
                        }

                        if (price != 0) {
                            if (!DEXNAV_MON_FREE)
                                price = (price * DEXNAV_BUY_ALL_DISCOUNT) / 100;
                            else
                                price = 0;
                            gSaveBlock2Ptr->frontier.battlePoints = gSaveBlock2Ptr->frontier.battlePoints - price;
                            if (gotShiny)
                                sDexNavUiDataPtr->currentMessage = DEXNAV_THANKS_FOR_PURCHASE_SHINY;
                            else
                                sDexNavUiDataPtr->currentMessage = DEXNAV_THANKS_FOR_PURCHASE;
                        }
                        else
                            sDexNavUiDataPtr->currentMessage = DEXNAV_COULD_NOT_GIVE_MON;
                        UpdateCursorPosition();
                    }
                }
                break;
                case DEXNAV_MESSAGE_BUY_ALL:{
                    u16 price = 0;
                    u8 i;
                    for (i = 0; i < NUM_POKEMON_ICONS; i++)
                    {
                        species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][i];
                        if (species != SPECIES_NONE)
                            price += getMonPrice(species);
                    }
                    price = (price * DEXNAV_BUY_ALL_DISCOUNT) / 100;

                    if (gSaveBlock2Ptr->frontier.battlePoints < price && !DEXNAV_MON_FREE) {
                        sDexNavUiDataPtr->currentMessage = DEXNAV_COULD_NOT_ENOUGH_FUNDS;
                        UpdateCursorPosition();
                    }
                    else {
                        bool8 couldGiveMon = FALSE;
                        u8 level = GetLevelCap();
                        price = 0;

                        if (level >= MAX_LEVEL)
                            level = MAX_LEVEL;

                        for (i = 0; i < NUM_POKEMON_ICONS; i++)
                        {
                            couldGiveMon = FALSE;
                            species = sDexNavUiDataPtr->routeSpecies[sDexNavUiDataPtr->currentEnviorment][i];
                            if (species != SPECIES_NONE && !gSaveBlock2Ptr->nuzlockeCaptures) {
                                VarSet(VAR_DEXNAV_SHINY_FLAG, 1);
                                couldGiveMon = ScriptGiveMon(species, level, ITEM_NONE, 0, 0, 0);
                                if (couldGiveMon < 2) {
                                    price += getMonPrice(species);
                                    if (VarGet(VAR_DEXNAV_SHINY_FLAG) == 2)
                                        gotShiny = TRUE;
                                    VarSet(VAR_DEXNAV_SHINY_FLAG, 0);
                                }
                            }
                        }

                        if (price != 0) {
                            if (!DEXNAV_MON_FREE)
                                price = (price * DEXNAV_BUY_ALL_DISCOUNT) / 100;
                            else
                                price = 0;
                            gSaveBlock2Ptr->frontier.battlePoints = gSaveBlock2Ptr->frontier.battlePoints - price;
                            if (gotShiny)
                                sDexNavUiDataPtr->currentMessage = DEXNAV_THANKS_FOR_PURCHASE_SHINY;
                            else
                                sDexNavUiDataPtr->currentMessage = DEXNAV_THANKS_FOR_PURCHASE;
                        }
                        else
                            sDexNavUiDataPtr->currentMessage = DEXNAV_COULD_NOT_GIVE_MON;
                        UpdateCursorPosition();
                    }
                }
                break;
                case DEXNAV_MESSAGE_SEARCH:
                    species = DexNavGetSpecies();
            
                    if (species != SPECIES_NONE)
                    {            
                        //PlaySE(SE_DEX_SEARCH);
                        //PlayCry(species, 0); ToDo
                        
                        // create value to store in a var
                        VarSet(VAR_DEXNAV_SPECIES, species);
                        VarSet(VAR_DEXNAV_ENVIORMENT, sDexNavUiDataPtr->currentEnviorment);

                        gSpecialVar_0x8000 = species;
                        gSpecialVar_0x8001 = sDexNavUiDataPtr->currentEnviorment;
                        gSpecialVar_0x8002 = (sDexNavUiDataPtr->cursorRow == ROW_HIDDEN) ? TRUE : FALSE;
                        PlaySE(SE_DEX_SEARCH);
                        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
                        task->func = Task_DexNavExitAndSearch;
                    }
                    else
                    {
                        PlaySE(SE_FAILURE);
                        sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_NONE;
                        UpdateCursorPosition();
                    }
                break;
            }
        }
        else {
            //Regular Dexnav can only search
            species = DexNavGetSpecies();
            if (species != SPECIES_NONE)
            {            
                //PlaySE(SE_DEX_SEARCH);
                //PlayCry(species, 0); ToDo
                        
                // create value to store in a var
                VarSet(VAR_DEXNAV_SPECIES, species);
                VarSet(VAR_DEXNAV_ENVIORMENT, sDexNavUiDataPtr->currentEnviorment);

                gSpecialVar_0x8000 = species;
                gSpecialVar_0x8001 = sDexNavUiDataPtr->currentEnviorment;
                gSpecialVar_0x8002 = (sDexNavUiDataPtr->cursorRow == ROW_HIDDEN) ? TRUE : FALSE;
                PlaySE(SE_DEX_SEARCH);
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
                task->func = Task_DexNavExitAndSearch;
            }
            else
            {
                PlaySE(SE_FAILURE);
                sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_NONE;
                UpdateCursorPosition();
            }
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        switch (sDexNavUiDataPtr->currentMessage) {
            default:
                sDexNavUiDataPtr->currentMessage = DEXNAV_MESSAGE_NONE;
                UpdateCursorPosition();
                break;
            case DEXNAV_MESSAGE_NONE:
                PlaySE(SE_POKENAV_OFF);
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
                task->func = Task_DexNavFadeAndExit;
            break;
        }
    }
}

/////////////////////////
//// HIDDEN POKEMON /////
/////////////////////////
bool8 TryFindHiddenPokemon(void)
{
    u16 *stepPtr = GetVarPointer(VAR_DEXNAV_STEP_COUNTER);
    u32 attempts = 0;

    if (!CanFindHiddenPokemon() || FlagGet(FLAG_SYS_DEXNAV_SEARCH) || gSaveBlock1Ptr->flashLevel > 0)
    {
        (*stepPtr) = 0;
        return FALSE;
    }
    
    (*stepPtr)++;
    (*stepPtr) %= HIDDEN_MON_STEP_COUNT;
    if ((*stepPtr) == 0 && (Random() % 100 < HIDDEN_MON_SEARCH_RATE))
    {
        // hidden pokemon
        u16 headerId = GetCurrentMapWildMonHeaderId();
        u8 index;
        u16 species;
        u8 environment;
        u8 taskId;
        const struct WildPokemonInfo* hiddenMonsInfo = gWildMonHeaders[headerId].hiddenMonsInfo;
        bool8 isHiddenMon = FALSE;
        
        //while you can still technically find hidden pokemon if there are not hidden-only pokemon on a map,
        // this prevents any potential lagging on maps you dont want hidden pokemon to appear on
        if (hiddenMonsInfo == NULL)
            return FALSE;
        
        //encounter rate signifies surfing (1) or land mons (0)!
        // again, for simplicity
        switch (hiddenMonsInfo->encounterRate)
        {
        case 0: //land
            // there are surely better ways to do this, but this allows greatest flexibility
            if (Random() % 100 < HIDDEN_MON_PROBABILTY)
            {
                index = ChooseHiddenMonIndex();
                if (index == 0xFF)
                    return FALSE;//no hidden info
                species = hiddenMonsInfo->wildPokemon[index].species;
                isHiddenMon = TRUE;
                environment = ENCOUNTER_TYPE_HIDDEN;
            }
            else
            {
                species = gWildMonHeaders[headerId].landMonsInfo->wildPokemon[ChooseWildMonIndex_Land()].species;
                environment = ENCOUNTER_TYPE_LAND;
            }
            break;
        case 1: //water
            if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
            {
                if (Random() % 100 < HIDDEN_MON_PROBABILTY)
                {
                    index = ChooseHiddenMonIndex();
                    if (index == 0xFF)
                        return FALSE;//no hidden info
                    species = hiddenMonsInfo->wildPokemon[index].species;
                    isHiddenMon = TRUE;
                    environment = ENCOUNTER_TYPE_HIDDEN;
                }
                else
                {
                    species = gWildMonHeaders[headerId].waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
                    environment = ENCOUNTER_TYPE_WATER;

                }
            }
            else
            {
                return FALSE;   //not surfing -> cant find hidden water mons
            }
            break;
        default:
            return FALSE;
        }
        
        if (species == SPECIES_NONE)
            return FALSE;
        
        sDexNavSearchDataPtr = AllocZeroed(sizeof(struct DexNavSearch));
                
        // init search data
        sDexNavSearchDataPtr->isHiddenMon = isHiddenMon;
        sDexNavSearchDataPtr->species = species;
        sDexNavSearchDataPtr->hiddenSearch = TRUE;
        sDexNavUiDataPtr->currentEnviorment = environment;  //updated in DexNavTryGenerateMonLevel if hidden mon
        sDexNavSearchDataPtr->monLevel = DexNavTryGenerateMonLevel(species, environment);
        if (sDexNavSearchDataPtr->monLevel == MON_LEVEL_NONEXISTENT)
        {
            Free(sDexNavSearchDataPtr);
            return FALSE;
        }

        // find tile for hidden mon and start effect if possible
        while (1) {
            if (TryStartHiddenMonFieldEffect(sDexNavUiDataPtr->currentEnviorment, 8, 8, TRUE))
                break;
            if (++attempts > 20)
                return FALSE;   //cannot find suitable tile
        }

        // exclamation mark over player
        gFieldEffectArguments[0] = gSaveBlock1Ptr->pos.x;
        gFieldEffectArguments[1] = gSaveBlock1Ptr->pos.y;
        gFieldEffectArguments[2] = gSprites[gPlayerAvatar.spriteId].subpriority - 1;
        gFieldEffectArguments[3] = 2;
        ObjectEventGetLocalIdAndMap(&gObjectEvents[gPlayerAvatar.objectEventId], &gFieldEffectArguments[0], &gFieldEffectArguments[1], &gFieldEffectArguments[2]);
        FieldEffectStart(FLDEFF_EXCLAMATION_MARK_ICON);
        
        //PlayCry_Script(species, 0);
        taskId = CreateTask(Task_SetUpDexNavSearch, 0);
        gTasks[taskId].tSpecies = sDexNavSearchDataPtr->species;
        gTasks[taskId].tEnvironment = sDexNavUiDataPtr->currentEnviorment;
        gTasks[taskId].tRevealed = FALSE;
        HideMapNamePopUpWindow();
        ChangeBgY_ScreenOff(0, 0, 0);
        return FALSE;   //we dont actually want to enable the script context or the game will freeze
    }
    
    return FALSE;
}

static void DrawSearchIcon(void)
{
    struct CompressedSpriteSheet spriteSheet;       
    
    spriteSheet.data = sHiddenSearchIconGfx;
    spriteSheet.size = 0x200;
    spriteSheet.tag = SELECTION_CURSOR_TAG;
    LoadCompressedSpriteSheet(&spriteSheet);
    sDexNavSearchDataPtr->iconSpriteId = CreateSprite(&sSearchIconSpriteTemplate, 18, GetSearchWindowY() + 12, 0);
}

// the initial hidden icon window ONLY shows search icon, ??? instead of name, and the search level (and pokeball icon if owned)
//  if the player presses R or moves close enough, the full search window will be created
//  this way, if the player is not interested in hidden pokemon it will not be too intrusive
static void DrawHiddenSearchWindow(u8 width)
{
    AddSearchWindow(width);
    AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, SPECIES_ICON_X + 4, 0, sSearchFontColor, TEXT_SKIP_DRAW, sText_ThreeQmarks);
    
    ConvertIntToDecimalStringN(gStringVar1, sDexNavSearchDataPtr->searchLevel, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(gStringVar4, sText_SearchLevel);
    AddTextPrinterParameterized3(sDexNavSearchDataPtr->windowId, 0, SPECIES_ICON_X + 4, 12, sSearchFontColor, TEXT_SKIP_DRAW, gStringVar4);
    CopyWindowToVram(sDexNavSearchDataPtr->windowId, 2);
}

static void DexNavDrawHiddenIcons(void)
{
    u16 species = sDexNavSearchDataPtr->species;
    
    DrawHiddenSearchWindow(12);
    DrawSearchIcon();
    
    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
        sDexNavSearchDataPtr->ownedIconSpriteId = CreateSprite(&sOwnedIconTemplate, SPECIES_ICON_X + 6, GetSearchWindowY() + 2, 0);
    
    if (sDexNavSearchDataPtr->isHiddenMon)
        sDexNavSearchDataPtr->exclamationSpriteId = CreateSprite(&sHiddenMonIconTemplate, SPECIES_ICON_X + 34, GetSearchWindowY() + 8, 0);
}

/////////////////////////
//// GENERAL UTILITY ////
/////////////////////////
bool8 DexNavTryMakeShinyMon(void)
{
    u32 i, shinyRolls, chainBonus, rndBonus;
    u32 shinyRate = 0;
    u32 charmBonus = 0;
    u8 searchLevel = sDexNavSearchDataPtr->searchLevel;
    u8 chain = gSaveBlock1Ptr->dexNavChain;
    
    #ifdef ITEM_SHINY_CHARM
    charmBonus = (CheckBagHasItem(ITEM_SHINY_CHARM, 1) > 0) ? 2 : 0;
    #endif
    
    chainBonus = (chain == 50) ? 5 : (chain == 100) ? 10 : 0;
    rndBonus = (Random() % 100 < 4 ? 4 : 0);
    shinyRolls = 1 + charmBonus + chainBonus + rndBonus;

    if (searchLevel > 200)
    {
        shinyRate += searchLevel - 200;
        searchLevel = 200;
    }
    if (searchLevel > 100)
    {
        shinyRate += (searchLevel * 2) - 200;
        searchLevel = 100;
    }
    if (searchLevel > 0)
    {
        shinyRate += searchLevel * 6;
    }
    
    shinyRate /= 100;
    for (i = 0; i < shinyRolls; i++)
    {
        if (Random() % 10000 < shinyRate)
            return TRUE;
    }
    
    return FALSE;
}

void TryIncrementSpeciesSearchLevel(u16 dexNum)
{
    //if (gMapHeader.regionMapSectionId != MAPSEC_BATTLE_FRONTIER && gSaveBlock1Ptr->dexNavSearchLevels[dexNum] < 255)
    //    gSaveBlock1Ptr->dexNavSearchLevels[dexNum]++;
}

void ResetDexNavSearch(void)
{
    gSaveBlock1Ptr->dexNavChain = 0;    //reset dex nav chaining on new map
    VarSet(VAR_DEXNAV_STEP_COUNTER, 0); //reset hidden pokemon step counter
    if (FlagGet(FLAG_SYS_DEXNAV_SEARCH))
        EndDexNavSearch(FindTaskIdByFunc(Task_DexNavSearch));   //moving to new map ends dexnav search
}

void IncrementDexNavChain(void)
{
    if (gSaveBlock1Ptr->dexNavChain < DEXNAV_CHAIN_MAX)
        gSaveBlock1Ptr->dexNavChain++;
}

#define MAP_GROUP_ROUTES_AND_TOWNS MAP_GROUP(PETALBURG_CITY)
#define MAP_GROUP_DUNGEONS         MAP_GROUP(METEOR_FALLS_1F_1R)

bool8 CanFindHiddenPokemon(void)
{
    bool8 CanFindHiddenMon = FALSE;
    switch (gSaveBlock1Ptr->location.mapGroup) {
		case MAP_GROUP_ROUTES_AND_TOWNS:
			if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE102)) { // Route 102
                if ( HasTrainerBeenFought(TRAINER_RICK)        && 
                    //HasTrainerBeenFought(TRAINER_OLDPLAYER) && 
                    HasTrainerBeenFought(TRAINER_CALVIN_1)    &&
                    HasTrainerBeenFought(TRAINER_ALLEN)       && 
                    HasTrainerBeenFought(TRAINER_TIANA))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE111)) { // Route 111 (Desert)
                if ( HasTrainerBeenFought(TRAINER_DREW)      &&
                    HasTrainerBeenFought(TRAINER_BEAU)   &&
                    HasTrainerBeenFought(TRAINER_HEIDI)    &&
                    HasTrainerBeenFought(TRAINER_BECKY)  &&
                    HasTrainerBeenFought(TRAINER_DUSTY_1)    &&
                    HasTrainerBeenFought(TRAINER_CELIA)    &&
                    HasTrainerBeenFought(TRAINER_BRANDEN)    &&
                    HasTrainerBeenFought(TRAINER_BRYAN))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE109)) { // Route 109 (Beach)
                if ( HasTrainerBeenFought(TRAINER_HUEY)      &&
                    HasTrainerBeenFought(TRAINER_RICKY_1)   &&
                    HasTrainerBeenFought(TRAINER_LOLA_1)    &&
                    HasTrainerBeenFought(TRAINER_CHANDLER)  &&
                    HasTrainerBeenFought(TRAINER_HAILEY)    &&
                    HasTrainerBeenFought(TRAINER_EDMOND)    &&
                    HasTrainerBeenFought(TRAINER_TIANA))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE113)) { // Route 113
                if ( HasTrainerBeenFought(TRAINER_JAYLEN)        &&
                    HasTrainerBeenFought(TRAINER_DILLON)        &&
                    HasTrainerBeenFought(TRAINER_MADELINE_1)    &&
                    HasTrainerBeenFought(TRAINER_LAO_1)         &&
                    HasTrainerBeenFought(TRAINER_LUNG)          &&
                    HasTrainerBeenFought(TRAINER_TORI_AND_TIA)  &&
                    HasTrainerBeenFought(TRAINER_SOPHIE)        &&
                    HasTrainerBeenFought(TRAINER_COBY)          &&
                    HasTrainerBeenFought(TRAINER_LAWRENCE)      &&
                    HasTrainerBeenFought(TRAINER_WYATT))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE114)) { // Route 114
                if ( HasTrainerBeenFought(TRAINER_LENNY)         &&
                    HasTrainerBeenFought(TRAINER_LUCAS_1)       &&
                    HasTrainerBeenFought(TRAINER_SHANE)         &&
                    HasTrainerBeenFought(TRAINER_NANCY)         &&
                    HasTrainerBeenFought(TRAINER_STEVE_1)       &&
                    HasTrainerBeenFought(TRAINER_BERNIE_1)      &&
                    HasTrainerBeenFought(TRAINER_CLAUDE)        &&
                    HasTrainerBeenFought(TRAINER_NOLAN)         &&
                    HasTrainerBeenFought(TRAINER_TYRA_AND_IVY)  &&
                    HasTrainerBeenFought(TRAINER_ANGELINA)      &&
                    HasTrainerBeenFought(TRAINER_CHARLOTTE)     &&
                    HasTrainerBeenFought(TRAINER_KAI))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE115)) { // Route 115
                if ( HasTrainerBeenFought(TRAINER_TIMOTHY_1) &&
                    HasTrainerBeenFought(TRAINER_KOICHI)    &&
                    HasTrainerBeenFought(TRAINER_NOB_1)     &&
                    HasTrainerBeenFought(TRAINER_CYNDY_1)   &&
                    HasTrainerBeenFought(TRAINER_HECTOR)    &&
                    HasTrainerBeenFought(TRAINER_KYRA)      &&
                    HasTrainerBeenFought(TRAINER_JAIDEN)    &&
                    HasTrainerBeenFought(TRAINER_ALIX)      &&
                    HasTrainerBeenFought(TRAINER_HELENE)    &&
                    HasTrainerBeenFought(TRAINER_MARLENE))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE116)) { // Route 116
                if ( HasTrainerBeenFought(TRAINER_JOEY)      &&
                    HasTrainerBeenFought(TRAINER_JOSE)      &&
                    HasTrainerBeenFought(TRAINER_JERRY_1)   &&
                    HasTrainerBeenFought(TRAINER_CLARK)     &&
                    HasTrainerBeenFought(TRAINER_JANICE)    &&
                    HasTrainerBeenFought(TRAINER_KAREN_1)   &&
                    HasTrainerBeenFought(TRAINER_SARAH)     &&
                    HasTrainerBeenFought(TRAINER_DAWSON)    &&
                    HasTrainerBeenFought(TRAINER_DEVAN)     &&
                    HasTrainerBeenFought(TRAINER_JOHNSON))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE117)) { // Route 117
                if ( HasTrainerBeenFought(TRAINER_ISAAC_1)           &&
                    HasTrainerBeenFought(TRAINER_LYDIA_1)           &&
                    HasTrainerBeenFought(TRAINER_DYLAN_1)           &&
                    HasTrainerBeenFought(TRAINER_MARIA_1)           &&
                    HasTrainerBeenFought(TRAINER_DEREK)             &&
                    HasTrainerBeenFought(TRAINER_ANNA_AND_MEG_1)    &&
                    HasTrainerBeenFought(TRAINER_MELINA)            &&
                    HasTrainerBeenFought(TRAINER_BRANDI)            &&
                    HasTrainerBeenFought(TRAINER_AISHA))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE118)) { // Route 118
                if ( HasTrainerBeenFought(TRAINER_ROSE_1)    &&
                    HasTrainerBeenFought(TRAINER_BARNY)     &&
                    HasTrainerBeenFought(TRAINER_WADE)      &&
                    HasTrainerBeenFought(TRAINER_DALTON_1)  &&
                    HasTrainerBeenFought(TRAINER_PERRY)     &&
                    HasTrainerBeenFought(TRAINER_CHESTER)   &&
                    HasTrainerBeenFought(TRAINER_DEANDRE))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE119)) { // Route 119
                if ( HasTrainerBeenFought(TRAINER_BRENT)         &&
                    HasTrainerBeenFought(TRAINER_DONALD)        &&
                    HasTrainerBeenFought(TRAINER_TAYLOR)        &&
                    HasTrainerBeenFought(TRAINER_DOUG)          &&
                    HasTrainerBeenFought(TRAINER_GREG)          &&
                    HasTrainerBeenFought(TRAINER_KENT)          &&
                    HasTrainerBeenFought(TRAINER_JACKSON_1)     &&
                    HasTrainerBeenFought(TRAINER_CATHERINE_1)   &&
                    HasTrainerBeenFought(TRAINER_HUGH)          &&
                    HasTrainerBeenFought(TRAINER_PHIL)          &&
                    HasTrainerBeenFought(TRAINER_YASU)          &&
                    HasTrainerBeenFought(TRAINER_TAKASHI)       &&
                    HasTrainerBeenFought(TRAINER_HIDEO)         &&
                    HasTrainerBeenFought(TRAINER_CHRIS)         &&
                    HasTrainerBeenFought(TRAINER_FABIAN)        &&
                    HasTrainerBeenFought(TRAINER_DAYTON)        &&
                    HasTrainerBeenFought(TRAINER_RACHEL))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE120)) { // Route 120
                if ( HasTrainerBeenFought(TRAINER_COLIN)     &&
                    HasTrainerBeenFought(TRAINER_ROBERT_1)  &&
                    HasTrainerBeenFought(TRAINER_LORENZO)   &&
                    HasTrainerBeenFought(TRAINER_JENNA)     &&
                    HasTrainerBeenFought(TRAINER_JEFFREY_1) &&
                    HasTrainerBeenFought(TRAINER_JENNIFER)  &&
                    HasTrainerBeenFought(TRAINER_CHIP)      &&
                    HasTrainerBeenFought(TRAINER_CLARISSA)  &&
                    HasTrainerBeenFought(TRAINER_ANGELICA)  &&
                    HasTrainerBeenFought(TRAINER_KEIGO)     &&
                    HasTrainerBeenFought(TRAINER_RILEY)     &&
                    HasTrainerBeenFought(TRAINER_LEONEL)    &&
                    HasTrainerBeenFought(TRAINER_CALLIE))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE121)) { // Route 121
                if ( HasTrainerBeenFought(TRAINER_VANESSA)       &&
                    HasTrainerBeenFought(TRAINER_WALTER_1)      &&
                    HasTrainerBeenFought(TRAINER_TAMMY)         &&
                    HasTrainerBeenFought(TRAINER_KATE_AND_JOY)  &&
                    HasTrainerBeenFought(TRAINER_JESSICA_1)     &&
                    HasTrainerBeenFought(TRAINER_CALE)          &&
                    HasTrainerBeenFought(TRAINER_MYLES)         &&
                    HasTrainerBeenFought(TRAINER_PAT)           &&
                    HasTrainerBeenFought(TRAINER_MARCEL)        &&
                    HasTrainerBeenFought(TRAINER_CRISTIN_1))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE123)) { // Route 123
                if ( HasTrainerBeenFought(TRAINER_WENDY)         &&
                    HasTrainerBeenFought(TRAINER_BRAXTON)       &&
                    HasTrainerBeenFought(TRAINER_VIOLET)        &&
                    HasTrainerBeenFought(TRAINER_CAMERON_1)     &&
                    HasTrainerBeenFought(TRAINER_JACKI_1)       &&
                    HasTrainerBeenFought(TRAINER_MIU_AND_YUKI)  &&
                    HasTrainerBeenFought(TRAINER_KINDRA)        &&
                    HasTrainerBeenFought(TRAINER_FREDRICK)      &&
                    HasTrainerBeenFought(TRAINER_ALBERTO)       &&
                    HasTrainerBeenFought(TRAINER_ED)            &&
                    HasTrainerBeenFought(TRAINER_KAYLEY)        &&
                    HasTrainerBeenFought(TRAINER_JONAS)         &&
                    HasTrainerBeenFought(TRAINER_JAZMYN)        &&
                    HasTrainerBeenFought(TRAINER_DAVIS)         &&
                    HasTrainerBeenFought(TRAINER_FERNANDO_1))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE127)) { // Route 127
                if ( HasTrainerBeenFought(TRAINER_KOJI_1) && 
                    HasTrainerBeenFought(TRAINER_ATHENA) && 
                    HasTrainerBeenFought(TRAINER_AIDAN)  &&
                    HasTrainerBeenFought(TRAINER_ROGER)  && 
                    HasTrainerBeenFought(TRAINER_HENRY)  && 
                    HasTrainerBeenFought(TRAINER_JONAH)  && 
                    HasTrainerBeenFought(TRAINER_CAMDEN) && 
                    HasTrainerBeenFought(TRAINER_DONNY))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE128)) { // Route 128
                if ( HasTrainerBeenFought(TRAINER_CARLEE)    && 
                    HasTrainerBeenFought(TRAINER_HARRISON)  && 
                    HasTrainerBeenFought(TRAINER_WAYNE)     &&
                    HasTrainerBeenFought(TRAINER_RUBEN)     && 
                    HasTrainerBeenFought(TRAINER_ALEXA)     && 
                    HasTrainerBeenFought(TRAINER_KATELYN_1) && 
                    HasTrainerBeenFought(TRAINER_ISAIAH_1))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE131)) { // Route 131
                if ( HasTrainerBeenFought(TRAINER_RICHARD)      && 
                    HasTrainerBeenFought(TRAINER_HERMAN)       && 
                    HasTrainerBeenFought(TRAINER_SUSIE)        &&
                    HasTrainerBeenFought(TRAINER_KARA)         && 
                    HasTrainerBeenFought(TRAINER_RELI_AND_IAN) && 
                    HasTrainerBeenFought(TRAINER_TALIA)        && 
                    HasTrainerBeenFought(TRAINER_KEVIN))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE132)) { // Route 132
                if ( HasTrainerBeenFought(TRAINER_MAKAYLA)  && 
                    HasTrainerBeenFought(TRAINER_JONATHAN) && 
                    HasTrainerBeenFought(TRAINER_DARCY)    &&
                    HasTrainerBeenFought(TRAINER_PAXTON)   && 
                    HasTrainerBeenFought(TRAINER_KIYO)     && 
                    HasTrainerBeenFought(TRAINER_RONALD)   && 
                    HasTrainerBeenFought(TRAINER_DANA)     && 
                    HasTrainerBeenFought(TRAINER_GILBERT))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE133)) { // Route 133
                if ( HasTrainerBeenFought(TRAINER_CONOR)  && 
                    HasTrainerBeenFought(TRAINER_MOLLIE) && 
                    HasTrainerBeenFought(TRAINER_BECK)   &&
                    HasTrainerBeenFought(TRAINER_WARREN) && 
                    HasTrainerBeenFought(TRAINER_LINDA)  && 
                    HasTrainerBeenFought(TRAINER_DEBRA))
                    CanFindHiddenMon = TRUE;
            }
            else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE134)) { // Route 134
                if ( HasTrainerBeenFought(TRAINER_KELVIN)  && 
                    HasTrainerBeenFought(TRAINER_MARLEY)  && 
                    HasTrainerBeenFought(TRAINER_REYNA)   &&
                    HasTrainerBeenFought(TRAINER_HUDSON)  && 
                    HasTrainerBeenFought(TRAINER_HITOSHI) && 
                    HasTrainerBeenFought(TRAINER_AARON)   && 
                    HasTrainerBeenFought(TRAINER_ALEX)    && 
                    HasTrainerBeenFought(TRAINER_LAUREL)  && 
                    HasTrainerBeenFought(TRAINER_JACK))
                    CanFindHiddenMon = TRUE;
            }
		break;
        case MAP_GROUP_DUNGEONS:
            if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(VICTORY_ROAD_1F)) { //Victory Road 1F
                if ( HasTrainerBeenFought(TRAINER_WALLY_VR_1) && 
                    HasTrainerBeenFought(TRAINER_EDGAR)      && 
                    HasTrainerBeenFought(TRAINER_ALBERT)     &&
                    HasTrainerBeenFought(TRAINER_HOPE)       &&
                    HasTrainerBeenFought(TRAINER_QUINCY)     &&
                    HasTrainerBeenFought(TRAINER_KATELYNN))
                    CanFindHiddenMon = TRUE;
            }
			else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(VICTORY_ROAD_B1F)) { //Victory Road B1F
                if ( HasTrainerBeenFought(TRAINER_HALLE)    && 
                    HasTrainerBeenFought(TRAINER_MITCHELL) && 
                    HasTrainerBeenFought(TRAINER_SHANNON)  &&
                    HasTrainerBeenFought(TRAINER_SAMUEL))
                    CanFindHiddenMon = TRUE;
            }
			else if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(VICTORY_ROAD_B2F)) { //Victory Road B2F
                if ( HasTrainerBeenFought(TRAINER_JULIE)    && 
                    HasTrainerBeenFought(TRAINER_CAROLINE) && 
                    HasTrainerBeenFought(TRAINER_OWEN)     &&
                    HasTrainerBeenFought(TRAINER_VITO)     && 
                    HasTrainerBeenFought(TRAINER_FELIX)    && 
                    HasTrainerBeenFought(TRAINER_DIANNE))
                    CanFindHiddenMon = TRUE;
            }
		break;
    }

    if (!FlagGet(FLAG_SYS_GAME_CLEAR))
        CanFindHiddenMon = FALSE;

    return CanFindHiddenMon;
}

bool8 hasAllMonsInEnviorment(void) {
    u8 i;
    u8 enviorment = sDexNavUiDataPtr->currentEnviorment;
    u16 species;
    for (i = 0; i < NUM_POKEMON_ICONS; i++) {
        species = sDexNavUiDataPtr->routeSpecies[enviorment][i];
        if (species != SPECIES_NONE && !GetSetPokedexFlag(species, FLAG_GET_CAUGHT))
            return  FALSE;
    }
    return TRUE;
}

bool8 IsRouteDexnavLocked() {
    bool8 disableDexnav = FALSE;
    u16 loc = gSaveBlock1Ptr->location.mapNum;
    u16 locG = gSaveBlock1Ptr->location.mapGroup;
    switch (gSaveBlock1Ptr->location.mapNum) {
		case MAP_NUM(ROUTE115):
			if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(ROUTE115) && !FlagGet(DEXNAV_ENABLE_ROUTE_115_FLAG))
				disableDexnav = TRUE;
		break;
    }
    if (IsRouteEncountered(loc, locG) && gSaveBlock2Ptr->nuzlockeCaptures) {
        disableDexnav = TRUE;
    }
    return disableDexnav;
}

bool8 MapHasMons(void) {
	u16 i;

    for (i = 0; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeaders[i];
        if (wildHeader->mapGroup == 0xFF)
            break;

        if (gWildMonHeaders[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeaders[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {
			return TRUE;
        }
    }

    return FALSE;
}

bool8 canOpenDexnav(void) {
    u16 headerId = GetCurrentMapWildMonHeaderId();
    u8 i;
    const struct WildPokemonInfo *landMonsInfo      = gWildMonHeaders[headerId].landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo     = gWildMonHeaders[headerId].waterMonsInfo;
    const struct WildPokemonInfo *hiddenMonsInfo    = gWildMonHeaders[headerId].hiddenMonsInfo;
    const struct WildPokemonInfo* honeyMonsInfo     = gWildMonHeaders[headerId].honeyMonsInfo;
    const struct WildPokemonInfo* rockSmashMonsInfo = gWildMonHeaders[headerId].rockSmashMonsInfo;
    const struct WildPokemonInfo* fishingMonsInfo   = gWildMonHeaders[headerId].fishingMonsInfo;

    if (IsRouteDexnavLocked() || !MapHasMons())
        return FALSE;

    for (i = 0; i < DEXNAV_ROWS_COUNT; i++) {
        if (IsDisabledForRoute(i)) continue;
        switch (i) {
            case ROW_LAND_TOP:    
                // Land Encounters
                if (landMonsInfo != NULL && 
                    landMonsInfo->encounterRate != 0) {
                    return TRUE;
                }
            break;
            case ROW_WATER:
                // Water Encounters
                if (waterMonsInfo != NULL && 
                    waterMonsInfo->encounterRate != 0 && 
                    FlagGet(DEXNAV_WATER_MONS_UNLOCK_FLAG))
                    return TRUE;
            break;
            case ROW_FISHING:
                // Fishing Encounters
                if (fishingMonsInfo != NULL && 
                    fishingMonsInfo->encounterRate != 0 && 
                    FlagGet(DEXNAV_FISHING_MONS_UNLOCK_FLAG))
                    return TRUE;
            break;
            case ROW_ROCK_SMASH:
                // Rock Smash Encounters
                if (rockSmashMonsInfo != NULL && 
                    rockSmashMonsInfo->encounterRate != 0 && 
                    FlagGet(DEXNAV_HEADBUTT_MONS_UNLOCK_FLAG))
                    return TRUE;
            break;
            case ROW_HONEY:
                // Honey Encounters
                if (honeyMonsInfo != NULL && 
                    honeyMonsInfo->encounterRate != 0 && 
                    FlagGet(DEXNAV_HONEY_MONS_UNLOCK_FLAG))
                    return TRUE;
            break;
            case ROW_HIDDEN:
                // Hidden Encounters
                if (hiddenMonsInfo != NULL && 
                    CanFindHiddenPokemon())
                    return TRUE;
            break;
        }
    }

    return FALSE;
}