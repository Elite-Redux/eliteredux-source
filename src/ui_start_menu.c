#include "global.h"
#include "ui_start_menu.h"
#include "strings.h"
#include "bg.h"
#include "random.h"
#include "battle_setup.h"
#include "bike.h"
#include "data.h"
#include "decompress.h"
#include "dexnav.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "frontier_pass.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "item.h"
#include "item_menu.h"
#include "item_menu_icons.h"
#include "list_menu.h"
#include "link.h"
#include "item_icon.h"
#include "item_use.h"
#include "field_screen_effect.h"
#include "international_string_util.h"
#include "main.h"
#include "main_menu.h"
#include "malloc.h"
#include "menu.h"
#include "metatile_behavior.h"
#include "menu_helpers.h"
#include "money.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon_icon.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "start_menu.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "quests.h"
#include "debug.h"
#include "trainer_card.h"
#include "text_window.h"
#include "trig.h"
#include "overworld.h"
#include "field_player_avatar.h"
#include "script_pokemon_util.h"
#include "pokenav.h"
#include "option_menu.h"
#include "ui_intro_options.h"
#include "ui_information_menu.h"
#include "wallclock.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "constants/party_menu.h"
#include "constants/items.h"
#include "constants/field_weather.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "naming_screen.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"

// Menu actions
enum
{
    START_MENU_ACTION_POKEDEX,
    START_MENU_ACTION_POKEMON,
	START_MENU_ACTION_PLAYER,
    START_MENU_ACTION_BAG,
    START_MENU_ACTION_DEXNAV,
    START_MENU_ACTION_POKENAV,
	START_MENU_ACTION_GUIDE,
	START_MENU_ACTION_OPTIONS,
	#ifdef DEBUG_BUILD
    START_MENU_ACTION_DEBUG,
    #endif
	START_MENU_ACTION_PC,
    START_MENU_ACTION_SAVE,
    START_MENU_ACTION_EXIT,
    NUM_START_MENU_ACTIONS,
};

//Sprites
enum{
    START_MENU_SPRITE_UP_ARROW,
    START_MENU_SPRITE_DOWN_ARROW,
    START_MENU_SPRITE_POKEMON_1,
    NUM_START_MENU_SPRITES = START_MENU_SPRITE_POKEMON_1 + PARTY_SIZE,
};

enum{
	START_MENU_BG_NORMAL,
	START_MENU_BG_TRANSPARENT,
	START_MENU_BG_SCROLLING,
	NUM_START_MENU_BACKGROUNDS,
};

//==========DEFINES==========//
struct MenuResources
{
    MainCallback savedCallback;     // determines callback to run when we exit. e.g. where do we want to go after closing the menu
    u8 gfxLoadState;
	u8 cursorRowY;
	u8 FirstItem;
	u8 KonamiCodeState;
	u8 CurrentMessage;
	u16 bgTilemapBuffers[NUM_START_MENU_BACKGROUNDS][0x400];
	u8 spriteIDs[NUM_START_MENU_SPRITES];
	u8 MenuOptions[NUM_START_MENU_ACTIONS];
	u8 actionNumber;
};

enum WindowIds
{
    WINDOW_1,
};

enum MessagesIds
{
    MESSAGE_HELP_BAR,
	MESSAGE_CANT_USE_BIKE,
	MESSAGE_CANT_USE_DEXNAV,
	MESSAGE_CANT_SAVE,
	MESSAGE_CANT_CHANGE_TIME,
	MESSAGE_STEPS_RESET,
	NUM_MESSAGES,
};

#define START_MENU_ACTION_NAME_LENGTH 20
#define MAX_START_MENU_ACTION_DESCRIPTION_LENGTH 100
#define MAX_SHOWN_START_MENU_ROWS 5
#define MAX_SHOWN_START_MENU_OPTIONS MAX_SHOWN_START_MENU_ROWS * 2

#define FLAG_NONE 0

struct StartMenuActionData
{
    const u8 title[START_MENU_ACTION_NAME_LENGTH];
    const u8 description[MAX_START_MENU_ACTION_DESCRIPTION_LENGTH];
	u16 flag;
};

static const struct StartMenuActionData StartMenuActions[NUM_START_MENU_ACTIONS] = {
    [START_MENU_ACTION_POKEDEX] =
    {
        .title = _("Pokedex"),
        .description = _(
			"Open a database of all\n"
			"the Pokémon you have\n"
			"seen and all their\n"
			"information."
		),
		.flag = FLAG_SYS_POKEDEX_GET,
    },
	[START_MENU_ACTION_PLAYER] =
    {
        .title = _("{PLAYER}."),
        .description = _(
			"See all your Trainer\n"
			"information, money,\n"
			"battle points, etc."
		),
		.flag = FLAG_NONE,
    },
	[START_MENU_ACTION_POKEMON] =
    {
        .title = _("Pokemon"),
        .description = _(
			"Organize your Pokémon\n"
			"party, see their stats,\n"
			"change their moves\n"
			"or even evolve them."
		),
		.flag = FLAG_SYS_POKEMON_GET,
    },
	[START_MENU_ACTION_PC] =
    {
        .title = _("Use the PC"),
        .description = _(
			"Open the Pokemon\n"
			"Storage System without\n"
			"having to go to any\n"
			"Pokemon Center."
		),
		.flag = FLAG_SYS_POKEMON_GET,
    },
	[START_MENU_ACTION_BAG] =
    {
        .title = _("Inventory"),
        .description = _(
			"Organize your Inventory,\n"
			"use your items or\n"
			"power up your Pokémon\n"
			"party."
		),
		.flag = FLAG_NONE,
    },
	[START_MENU_ACTION_OPTIONS] =
    {
        .title = _("Settings"),
        .description = _(
			"Change your settings,\n"
			"character colors,\n"
			"bike type or disable\n"
			"stuff you don't like."
		),
		.flag = FLAG_NONE,
    },
	[START_MENU_ACTION_POKENAV] =
    {
        .title = _("PokeNav"),
        .description = _(
			"Change your settings,\n"
			"character colors,\n"
			"bike type or disable\n"
			"stuff you don't like."
		),
		.flag = FLAG_SYS_POKENAV_GET,
    },
	[START_MENU_ACTION_GUIDE] =
    {
        .title = _("Information"),
        .description = _(
			"See some in-game,\n"
			"changes and tips to\n"
			"make your adventure\n"
			"easier."
		),
		.flag = FLAG_NONE,
    },
	[START_MENU_ACTION_DEXNAV] =
    {
        .title = _("DexNav"),
        .description = _(
			"Search for nearby\n"
			"Pokémon, chain them,\n"
			"or just see the list\n"
			"of available Pokémon."
		),
		.flag = FLAG_SYS_DEXNAV_GET,
    },
	#ifdef DEBUG_BUILD
	[START_MENU_ACTION_DEBUG] =
    {
        .title = _("Debug"),
        .description = _(
			"Open some developer\n"
			"options that let you\n"
			"cheat anything into\n"
			"the game."
		),
		.flag = FLAG_NONE,
    },
    #endif
	[START_MENU_ACTION_SAVE] =
    {
        .title = _("Save"),
        .description = _(
			"Save your game with\n"
			"a complete record of\n"
			"your progress to take\n"
			"a break."
		),
		.flag = FLAG_NONE,
    },
	[START_MENU_ACTION_EXIT] =
    {
        .title = _("Exit"),
        .description = _(
			"Exit this menu and\n"
			"continue your Pokémon\n"
			"adventure!"
		),
		.flag = FLAG_NONE,
    },
};

//==========EWRAM==========//
static EWRAM_DATA struct MenuResources *sMenuDataPtr = NULL;

// IWRAM common
//bool8 (*gMenuCallback)(void);
//bool8 (*gUIStartMenuCallback)(void);

//==========STATIC=DEFINES==========//
static void Menu_RunSetup(void);
static bool8 Menu_DoGfxSetup(void);
static bool8 Menu_Start_InitBgs(void);
static void setScrollingBackground(void);
static void setNormalBackground(void);
static void setTransparentBackground(void);
static void Menu_FadeAndBail(void);
static bool8 Menu_LoadGraphics(void);
static void Menu_Start_InitWindows(void);
static void PrintToWindow(void);
static void Task_MenuWaitFadeIn(u8 taskId);
static void Task_MenuMain(u8 taskId);
bool8 CanUseBikeFromStartMenu(void);

void Task_OpenPokemonPartyFromStartMenu(u8 taskId);
void Task_OpenPokedexFromStartMenu(u8 taskId);
static u8 ShowSpeciesIcon(u8 slot, u8 x, u8 y);

//==========CONST=DATA==========//
static const struct BgTemplate sMenuBgTemplates[NUM_START_MENU_BACKGROUNDS + 1] =
{
	//Windows
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .priority = 0,
    },
	//Front Background
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 29,
        .priority = 1,
    },
	//Transparent Background
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 27,
        .priority = 2,
    },
	//Scrolling Background
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 25,
        .priority = 3,
    },
};

static const struct WindowTemplate sMenuWindowTemplates[] = 
{
    [WINDOW_1] = 
    {
        .bg = 0,            // which bg to print text on
        .tilemapLeft = 0,   // position from left (per 8 pixels)
        .tilemapTop = 0,    // position from top (per 8 pixels)
        .width = 30,        // width (per 8 pixels)
        .height = 20,       // height (per 8 pixels)
        .paletteNum = 1,    // palette index to use for text
        .baseBlock = 1,     // tile start in VRAM
    },
};

static const u32 sMenuTiles[] 	           = INCBIN_U32("graphics/ui_menus/start_menu/tiles.4bpp.lz");
static const u16 sMenuPalette_Interface[]  = INCBIN_U16("graphics/ui_menus/start_menu/palette_interface.gbapal");

//Palettes
static const u16 sMenuPalette[]            = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette.gbapal");
static const u16 sMenuPalette_Blue[]       = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_blue.gbapal");
static const u16 sMenuPalette_Yellow[]     = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_yellow.gbapal");
static const u16 sMenuPalette_Green[]      = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_green.gbapal");
static const u16 sMenuPalette_Dark[]       = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_dark.gbapal");
static const u16 sMenuPalette_Red[]        = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_red.gbapal");

const u32 sMenuTilemap[]                   = INCBIN_U32("graphics/ui_menus/start_menu/tilemap.bin.lz");
const u32 sStartMenuScrollingBGTilemap[]   = INCBIN_U32("graphics/ui_menus/start_menu/scrolling_bg.bin.lz");
const u32 sStartMenuTransparentBGTilemap[] = INCBIN_U32("graphics/ui_menus/start_menu/tilemap_transparent.bin.lz");

static void LoadStartMenuPalette(void) {
    u8 menuColor = gSaveBlock2Ptr->startMenuPaletteNum;

    switch (menuColor){
        case MENU_COLOR_BLUE:
            LoadPalette(sMenuPalette_Blue, 0, 32);
        break;
        case MENU_COLOR_YELLOW:
            LoadPalette(sMenuPalette_Yellow, 0, 32);
        break;
        case MENU_COLOR_RED:
            LoadPalette(sMenuPalette_Red, 0, 32);
        break;
        case MENU_COLOR_GREEN:
            LoadPalette(sMenuPalette_Green, 0, 32);
        break;
        case MENU_COLOR_DARK:
            LoadPalette(sMenuPalette_Dark, 0, 32);
        break;
        default:
            LoadPalette(sMenuPalette, 0, 32);
        break;
    }
}

enum Colors
{
    FONT_BLACK,
    FONT_BLACK_2,
    FONT_WHITE,
    FONT_WHITE_2,
    FONT_RED,
    FONT_BLUE,
};

const u8 sMenuWindowFontColors[][3] = 
{
    [FONT_BLACK]    = {TEXT_COLOR_TRANSPARENT,  7,  5},
    [FONT_BLACK_2]  = {TEXT_COLOR_TRANSPARENT,  7,  TEXT_COLOR_TRANSPARENT},
    [FONT_WHITE]    = {TEXT_COLOR_TRANSPARENT,  1,  2},
    [FONT_WHITE_2]  = {TEXT_COLOR_TRANSPARENT,  1,  TEXT_COLOR_TRANSPARENT},
    [FONT_RED]      = {TEXT_COLOR_TRANSPARENT,  8,  9},
    [FONT_BLUE]     = {TEXT_COLOR_TRANSPARENT,  13, TEXT_COLOR_TRANSPARENT},
};

//==========FUNCTIONS==========//
// UI loader template
void Task_OpenStartMenuFromStartMenu(u8 taskId)
{
    //s16 *data = gTasks[taskId].data;
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        Menu_Start_Init(CB2_ReturnToField);
        DestroyTask(taskId);
    }
}

// This is our main initialization function if you want to call the menu from elsewhere
void Menu_Start_Init(MainCallback callback)
{
	u8 i;
	u8 j = 0;
	if ((sMenuDataPtr = AllocZeroed(sizeof(struct MenuResources))) == NULL)
    {
        SetMainCallback2(callback);
        return;
    }

    // initialize stuff
    sMenuDataPtr->gfxLoadState = 0;
    sMenuDataPtr->savedCallback = callback;
	
	sMenuDataPtr->cursorRowY = gSaveBlock2Ptr->start_cursorRowY;
	sMenuDataPtr->FirstItem  = gSaveBlock2Ptr->start_FirstItem;
	
	sMenuDataPtr->KonamiCodeState = 0;
	//setCorrectSeason();

	for(i = 0; i < NUM_START_MENU_SPRITES; i++)
		sMenuDataPtr->spriteIDs[i] = 0xFF;

	for(i = 0; i < NUM_START_MENU_ACTIONS; i++){
		if(FlagGet(StartMenuActions[i].flag) || StartMenuActions[i].flag == FLAG_NONE){
			sMenuDataPtr->MenuOptions[j] = i;
			j++;
		}
	}

	sMenuDataPtr->actionNumber = j;

    SetMainCallback2(Menu_RunSetup);
}

static void Menu_RunSetup(void)
{
	while (1)
    {
        if (Menu_DoGfxSetup() == TRUE)
            break;
    }
}

static void Menu_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

enum{
	BACKGROUND_WINDOWS,
	BACKGROUND_NORMAL,
	BACKGROUND_SCROLLING,
	BACKGROUND_TRANSPARENT,
	NUM_EXTRA_BACKGROUNDS,
};

static void Menu_VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();

	//Scroll Background
	//ChangeBgX(BACKGROUND_SCROLLING, 128, 1);
    //ChangeBgY(BACKGROUND_SCROLLING, 128, 2);
}

//Arrows
static void SpriteCallback_Inventory_UpArrow(struct Sprite *sprite)
{
    sprite->data[0] += 8;

    if(sMenuDataPtr->cursorRowY == 0)
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
}

static void SpriteCallback_Inventory_DownArrow(struct Sprite *sprite)
{
    u8 numitems = sMenuDataPtr->actionNumber;
    sprite->data[0] += 8;

    if(sMenuDataPtr->cursorRowY >= numitems -1) //Because of the Exit Button
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
}

#define PAL_UI_SPRITES 0
static const u32 sStartMenuUpArrow_Gfx[]         = INCBIN_U32("graphics/ui_menus/start_menu/arrow_up.4bpp.lz");
static const u32 sStartMenuDownArrow_Gfx[]       = INCBIN_U32("graphics/ui_menus/start_menu/arrow_down.4bpp.lz");
static const u16 sMenuInterfacePalette_Sprites[] = INCBIN_U16("graphics/ui_menus/start_menu/palette_interface.gbapal");

static const struct SpritePalette sInventoryInterfaceSpritePalette[] = {
    {sMenuInterfacePalette_Sprites, PAL_UI_SPRITES},
};

#define UP_ARROW_X 64
#define UP_ARROW_Y 12

static void CreateUpArrowSprite(void)
{
    u8 spriteId;
    u8 SpriteTag = START_MENU_SPRITE_UP_ARROW;
    struct CompressedSpriteSheet sSpriteSheet_InventoryUpArrow = {sStartMenuUpArrow_Gfx, 0x0800, SpriteTag};
    struct SpriteTemplate TempSpriteTemplate = gDummySpriteTemplate;

    TempSpriteTemplate.tileTag  = SpriteTag;
    TempSpriteTemplate.callback = SpriteCallback_Inventory_UpArrow;

    LoadCompressedSpriteSheet(&sSpriteSheet_InventoryUpArrow);
    LoadSpritePalette(sInventoryInterfaceSpritePalette);
    spriteId = CreateSprite(&TempSpriteTemplate, UP_ARROW_X, UP_ARROW_Y, 0);
    sMenuDataPtr->spriteIDs[SpriteTag] = spriteId;

    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.shape       = SPRITE_SHAPE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.size        = SPRITE_SIZE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.priority    = 1;
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.paletteNum  = PAL_UI_SPRITES;
}

#define DOWN_ARROW_X UP_ARROW_X
#define DOWN_ARROW_Y 96 + 40

static void CreateDownArrowSprite(void)
{
    u8 spriteId;
    u8 SpriteTag = START_MENU_SPRITE_DOWN_ARROW;
    struct CompressedSpriteSheet sSpriteSheet_InventoryDownArrow = {sStartMenuDownArrow_Gfx, 0x0800, SpriteTag};
    struct SpriteTemplate TempSpriteTemplate = gDummySpriteTemplate;

    TempSpriteTemplate.tileTag  = SpriteTag;
    TempSpriteTemplate.callback = SpriteCallback_Inventory_DownArrow;

    LoadCompressedSpriteSheet(&sSpriteSheet_InventoryDownArrow);
    spriteId = CreateSprite(&TempSpriteTemplate, DOWN_ARROW_X, DOWN_ARROW_Y, 0);
    sMenuDataPtr->spriteIDs[SpriteTag] = spriteId;

    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.shape      = SPRITE_SHAPE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.size       = SPRITE_SIZE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.priority   = 1;
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.paletteNum = PAL_UI_SPRITES;
}

#define ICON_STARTING_X 176 - 40
#define ICON_STARTING_Y 32
#define X_DIFFERENCE_BETWEEN_SPECIES 40
#define Y_DIFFERENCE_BETWEEN_SPECIES 40

static bool8 Menu_DoGfxSetup(void)
{
	u8 x, y, i;

    switch (gMain.state)
    {
    case 0:
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000)
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
		ResetVramOamAndBgCntRegs();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        gMain.state++;
        break;
    case 2:
        if (Menu_Start_InitBgs())
        {
            sMenuDataPtr->gfxLoadState = 0;
            gMain.state++;
        }
        else
        {
            Menu_FadeAndBail();
            return TRUE;
        }
        break;
    case 3:
        if (Menu_LoadGraphics() == TRUE)
            gMain.state++;
        break;
    case 4:
        CreateUpArrowSprite();
        CreateDownArrowSprite();
        LoadMessageBoxAndBorderGfx();
        Menu_Start_InitWindows();
        gMain.state++;
        break;
    case 5:
        PrintToWindow();
        //taskId = CreateTask(Task_MenuWaitFadeIn, 0);
        CreateTask(Task_MenuWaitFadeIn, 0);
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);

		// Pokemon Icons --------------------------------------------------------------------------------------------------------------------
        x = ICON_STARTING_X;
		y = ICON_STARTING_Y;

        for(i = 0; i < PARTY_SIZE; i++){
            if(GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE)
                ShowSpeciesIcon(i, x, y);

			if(i != 2){
				x = x + X_DIFFERENCE_BETWEEN_SPECIES;
			}
			else{
				x = ICON_STARTING_X;
				y = y + Y_DIFFERENCE_BETWEEN_SPECIES;
			}
        }
		
        gMain.state++;
        break;
    case 6:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(Menu_VBlankCB);
        SetMainCallback2(Menu_MainCB);
        return TRUE;
    }
    return FALSE;
}

#define try_free(ptr) ({        \
    void ** ptr__ = (void **)&(ptr);   \
    if (*ptr__ != NULL)                \
        Free(*ptr__);                  \
})

static void Menu_FreeResources(void)
{
    try_free(sMenuDataPtr);
    FreeAllWindowBuffers();
}


static void Task_MenuWaitFadeAndBail(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sMenuDataPtr->savedCallback);
        Menu_FreeResources();
        DestroyTask(taskId);
    }
}

static void Menu_FadeAndBail(void)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_MenuWaitFadeAndBail, 0);
    SetVBlankCallback(Menu_VBlankCB);
    SetMainCallback2(Menu_MainCB);
}

static bool8 Menu_Start_InitBgs(void)
{
	u8 i;
    ResetBgsAndClearDma3BusyFlags(0);
    ResetAllBgsCoordinates();

    InitBgsFromTemplates(0, sMenuBgTemplates, ARRAY_COUNT(sMenuBgTemplates));

	for(i = 1; i < NUM_EXTRA_BACKGROUNDS; i++){
		switch(i){
			case BACKGROUND_TRANSPARENT:
    			setTransparentBackground();
			break;
			case BACKGROUND_NORMAL:
    			setNormalBackground();
			break;
			case BACKGROUND_SCROLLING:
    			setScrollingBackground();
			break;
		}
	}

    return TRUE;
}

static void setNormalBackground(){
    SetBgAttribute(BACKGROUND_NORMAL, BG_ATTR_PRIORITY, BACKGROUND_NORMAL);
    SetBgTilemapBuffer(BACKGROUND_NORMAL, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_TRANSPARENT]);
    ScheduleBgCopyTilemapToVram(BACKGROUND_NORMAL);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);

    ShowBg(0);
    ShowBg(BACKGROUND_NORMAL);
	ChangeBgX(BACKGROUND_NORMAL, 0, 1);
	ChangeBgY(BACKGROUND_NORMAL, 0, 1);
}

#define START_MENU_TRANSPARENCY_STRENGTH 10
static void setTransparentBackground(){
    SetBgAttribute(BACKGROUND_TRANSPARENT, BG_ATTR_PRIORITY, START_MENU_BG_TRANSPARENT);
    SetBgTilemapBuffer(BACKGROUND_TRANSPARENT, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_NORMAL]);
    ScheduleBgCopyTilemapToVram(BACKGROUND_TRANSPARENT);

    //Transparency
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL | BLDCNT_TGT1_BG1); //Blend Background over the rest
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(START_MENU_TRANSPARENCY_STRENGTH, START_MENU_TRANSPARENCY_STRENGTH));
    SetGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR);

    ShowBg(BACKGROUND_TRANSPARENT);
    ChangeBgX(BACKGROUND_TRANSPARENT, 0, 0);
    ChangeBgY(BACKGROUND_TRANSPARENT, 0, 0);
}

static void removeTransparentBackground(){
    SetBgAttribute(BACKGROUND_TRANSPARENT, BG_ATTR_PRIORITY, START_MENU_BG_TRANSPARENT);
    SetBgTilemapBuffer(BACKGROUND_TRANSPARENT, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_NORMAL]);
    ScheduleBgCopyTilemapToVram(BACKGROUND_TRANSPARENT);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(BACKGROUND_TRANSPARENT);
}

static void setScrollingBackground(){
    SetBgAttribute(BACKGROUND_SCROLLING, BG_ATTR_PRIORITY, START_MENU_BG_SCROLLING);
	SetBgTilemapBuffer(BACKGROUND_SCROLLING, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_SCROLLING]);
    ScheduleBgCopyTilemapToVram(BACKGROUND_SCROLLING);
    ShowBg(BACKGROUND_SCROLLING);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);

	ChangeBgX(BACKGROUND_SCROLLING, 0, 1);
	ChangeBgY(BACKGROUND_SCROLLING, 0, 1);
}

static bool8 Menu_LoadGraphics(void)
{
    switch (sMenuDataPtr->gfxLoadState)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, sMenuTiles, 0, 0, 0);
        sMenuDataPtr->gfxLoadState++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
			LZDecompressWram(sMenuTilemap, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_NORMAL]);
            sMenuDataPtr->gfxLoadState++;
        }
		break;
	case 2:
		//Load Background
		LZDecompressWram(sStartMenuTransparentBGTilemap, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_TRANSPARENT]);
        sMenuDataPtr->gfxLoadState++;
        break;
	break;
	case 3:
		//Load Background
		LZDecompressWram(sStartMenuScrollingBGTilemap, sMenuDataPtr->bgTilemapBuffers[START_MENU_BG_SCROLLING]);
        sMenuDataPtr->gfxLoadState++;
        break;
    case 4:
        LoadStartMenuPalette();
        LoadPalette(sMenuPalette_Interface, 16, 32);
        sMenuDataPtr->gfxLoadState++;
        break;
    default:
        sMenuDataPtr->gfxLoadState = 0;
        return TRUE;
    }
    return FALSE;
}

static void Menu_Start_InitWindows(void)
{
    INIT_WINDOWS(sMenuWindowTemplates);
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0);

    FillWindowPixelBuffer(WINDOW_1, 0);
    LoadUserWindowBorderGfx(WINDOW_1, 720, 14 * 16);
    PutWindowTilemap(WINDOW_1);
    CopyWindowToVram(WINDOW_1, 3);

    //ScheduleBgCopyTilemapToVram(2);
}

//HP Bar
//const u8 sStartMenu_HPBar_Gfx[]              = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar.4bpp");
const u8 sStartMenu_HPBar_Full_Gfx[]         = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_full.4bpp");
const u8 sStartMenu_HPBar_90_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_90_percent.4bpp");
const u8 sStartMenu_HPBar_80_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_80_percent.4bpp");
const u8 sStartMenu_HPBar_70_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_70_percent.4bpp");
const u8 sStartMenu_HPBar_60_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_60_percent.4bpp");
const u8 sStartMenu_HPBar_50_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_50_percent.4bpp");
const u8 sStartMenu_HPBar_40_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_40_percent.4bpp");
const u8 sStartMenu_HPBar_30_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_30_percent.4bpp");
const u8 sStartMenu_HPBar_20_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_20_percent.4bpp");
const u8 sStartMenu_HPBar_10_Percent_Gfx[]   = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_10_percent.4bpp");
const u8 sStartMenu_HPBar_Fainted_Gfx[]      = INCBIN_U8("graphics/ui_menus/start_menu/hpbar/hp_bar_fainted.4bpp");

static const u8 *GetBarGfx(u32 percent)
{
    u32 modifiedPercent = percent / 10;

    const u8 *sStartMenu_HPBar_Fill_Gfx[] = {
        sStartMenu_HPBar_Fainted_Gfx,
        sStartMenu_HPBar_10_Percent_Gfx,
        sStartMenu_HPBar_20_Percent_Gfx,
        sStartMenu_HPBar_30_Percent_Gfx,
        sStartMenu_HPBar_40_Percent_Gfx,
        sStartMenu_HPBar_50_Percent_Gfx,
        sStartMenu_HPBar_60_Percent_Gfx,
        sStartMenu_HPBar_70_Percent_Gfx,
        sStartMenu_HPBar_80_Percent_Gfx,
        sStartMenu_HPBar_90_Percent_Gfx,
        sStartMenu_HPBar_Full_Gfx,
    };

    if (percent == 0)
        return sStartMenu_HPBar_Fill_Gfx[percent];

    if (modifiedPercent == 0)
        return sStartMenu_HPBar_Fill_Gfx[1];

    return sStartMenu_HPBar_Fill_Gfx[modifiedPercent];
}

//Pokemon Icon Stuff
static void sSpriteCB_Dummy(struct Sprite *sprite)
{
}

static u32 GetHPEggCyclePercent(u32 partyIndex)
{
    struct Pokemon *mon = &gPlayerParty[partyIndex];

    if (!GetMonData(mon, MON_DATA_IS_EGG))
        return ((GetMonData(mon, MON_DATA_HP)) * 100 / (GetMonData(mon,MON_DATA_MAX_HP)));
    else
        return ((GetMonData(mon, MON_DATA_FRIENDSHIP)) * 100 / (gBaseStats[GetMonData(mon,MON_DATA_SPECIES)].eggCycles));
}
static void *GetSpriteCallbackForIcon(u32 percent, bool32 isEgg)
{
    if (((percent > 20) && isEgg) || percent == 0)
        return sSpriteCB_Dummy;

    return SpriteCB_MonIcon;
}

static u8 ShowSpeciesIcon(u8 slot, u8 x, u8 y)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    u8 SpriteID       = 0xFF;
	u16 species       = GetMonData(mon, MON_DATA_SPECIES);
	u32 personality   = GetMonData(mon, MON_DATA_PERSONALITY);
    //u16 currentHP     = GetMonData(mon, MON_DATA_HP);
    //u32 currentStatus = GetAilmentFromStatus(GetMonData(mon, MON_DATA_STATUS));
    //u8 palette        = LoadMonIconPaletteWithAilment(species, personality, currentStatus, currentHP, slot + 2);
    u32 percent       = GetHPEggCyclePercent(slot);
	bool8 isEgg       = GetMonData(mon, MON_DATA_IS_EGG);

	//mgba_printf(MGBA_LOG_WARN, "ShowSpeciesIcon Num %d Species %d formID %d speciesID %d", slot, species, formID, speciesID);

	LoadMonIconPalette(species);
    SpriteID = CreateMonIcon(GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES), GetSpriteCallbackForIcon(percent, isEgg), x, y, 0, personality);

    gSprites[SpriteID].oam.priority   = 2;
    //gSprites[SpriteID].oam.paletteNum = palette;

    return SpriteID;
}

const u8 sStartMenuCursor_Gfx[]           = INCBIN_U8("graphics/ui_menus/start_menu/menu_cursor.4bpp");

//Icons
const u8 sStartMenu_IconGfx_Achievements[]   = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_achievements.4bpp");
const u8 sStartMenu_IconGfx_Bag[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_bag.4bpp");
const u8 sStartMenu_IconGfx_DexNav[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_dexnav.4bpp");
const u8 sStartMenu_IconGfx_Exit[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_exit.4bpp");
const u8 sStartMenu_IconGfx_Options[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_options.4bpp");
const u8 sStartMenu_IconGfx_Player[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_player.4bpp");
const u8 sStartMenu_IconGfx_Pokedex[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pokedex.4bpp");
const u8 sStartMenu_IconGfx_Pokemon[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pokemon.4bpp");
const u8 sStartMenu_IconGfx_PokeNav[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pokenav.4bpp");
const u8 sStartMenu_IconGfx_Save[]   	 	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_save.4bpp");
const u8 sStartMenu_IconGfx_Skills[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_skills.4bpp");
const u8 sStartMenu_IconGfx_Use_PC[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pss.4bpp");
const u8 sStartMenu_IconGfx_Info[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_info.4bpp");
const u8 sStartMenu_IconGfx_Debug[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_debug.4bpp");

const u8 sStartMenu_IconGfx_Achievements_Selected[]  = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_achievements_selected.4bpp");
const u8 sStartMenu_IconGfx_Bag_Selected[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_bag_selected.4bpp");
const u8 sStartMenu_IconGfx_DexNav_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_dexnav_selected.4bpp");
const u8 sStartMenu_IconGfx_Exit_Selected[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_exit_selected.4bpp");
const u8 sStartMenu_IconGfx_Options_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_options_selected.4bpp");
const u8 sStartMenu_IconGfx_Player_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_player_selected.4bpp");
const u8 sStartMenu_IconGfx_Pokedex_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pokedex_selected.4bpp");
const u8 sStartMenu_IconGfx_Pokemon_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pokemon_selected.4bpp");
const u8 sStartMenu_IconGfx_PokeNav_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pokenav_selected.4bpp");
const u8 sStartMenu_IconGfx_Save_Selected[]   	 	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_save_selected.4bpp");
const u8 sStartMenu_IconGfx_Skills_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_skills_selected.4bpp");
const u8 sStartMenu_IconGfx_Use_PC_Selected[]   	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_pss_selected.4bpp");
const u8 sStartMenu_IconGfx_Info_Selected[]   	 	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_info_selected.4bpp");
const u8 sStartMenu_IconGfx_Debug_Selected[]   	 	 = INCBIN_U8("graphics/ui_menus/start_menu/icons/icon_debug_selected.4bpp");

static const u8 sStartMenuHeldItem_Gfx[]   		 = INCBIN_U8("graphics/ui_menus/start_menu/icons/held_item.4bpp");
static const u8 sStartMenuStatus_Burn_Gfx[]      = INCBIN_U8("graphics/ui_menus/start_menu/icons/status_burn.4bpp");
static const u8 sStartMenuStatus_Poison_Gfx[]    = INCBIN_U8("graphics/ui_menus/start_menu/icons/status_poison.4bpp");
static const u8 sStartMenuStatus_Freeze_Gfx[]    = INCBIN_U8("graphics/ui_menus/start_menu/icons/status_freeze.4bpp");
static const u8 sStartMenuStatus_Paralysis_Gfx[] = INCBIN_U8("graphics/ui_menus/start_menu/icons/status_paralysis.4bpp");
static const u8 sStartMenuStatus_Sleep_Gfx[]     = INCBIN_U8("graphics/ui_menus/start_menu/icons/status_sleep.4bpp");

//Saving Stuff
enum
{
    SAVE_MODE_NOT_ENGAGED,
    SAVE_MODE_ASK,
    SAVE_MODE_IN_PROGRESS,
    SAVE_MODE_SUCCESS,
    SAVE_MODE_CANCELED,
    SAVE_MODE_ERROR,
    SAVE_MODE_OVERWRITE
};

//Days of the Week
enum
{
    DAYS_OF_THE_WEEK_SUNDAY,
    DAYS_OF_THE_WEEK_MONDAY,
    DAYS_OF_THE_WEEK_TUESDAY,
    DAYS_OF_THE_WEEK_WEDNESDAY,
    DAYS_OF_THE_WEEK_THURSDAY,
    DAYS_OF_THE_WEEK_FRIDAY,
    DAYS_OF_THE_WEEK_SATURDAY,
    NUM_DAYS_OF_THE_WEEK,
};

static const struct StringList DaysOfTheWeek[NUM_DAYS_OF_THE_WEEK] = {
	[DAYS_OF_THE_WEEK_SUNDAY]    = _("Sunday"),
	[DAYS_OF_THE_WEEK_MONDAY]    = _("Monday"),
	[DAYS_OF_THE_WEEK_TUESDAY]   = _("Tuesday"),
	[DAYS_OF_THE_WEEK_WEDNESDAY] = _("Wednesday"),
	[DAYS_OF_THE_WEEK_THURSDAY]  = _("Thursday"),
	[DAYS_OF_THE_WEEK_FRIDAY]    = _("Friday"),
	[DAYS_OF_THE_WEEK_SATURDAY]  = _("Saturday"),
};

#define SEASON_SPRING 0
#define SEASON_SUMMER 1
#define SEASON_AUTUMN 2
#define SEASON_WINTER 3
#define NUM_SEASONS   4

static const struct StringList SeasonList[NUM_SEASONS] = {
	[SEASON_SPRING] = _("Spring"),
	[SEASON_SUMMER] = _("Summer"),
	[SEASON_AUTUMN] = _("Autumn"),
	[SEASON_WINTER] = _("Winter"),
};

static const struct StringList MonthList[MONTH_DEC + 1] = {
	[MONTH_JAN] = _("January"),
	[MONTH_FEB] = _("February"),
	[MONTH_MAR] = _("March"),
	[MONTH_APR] = _("April"),
	[MONTH_MAY] = _("May"),
	[MONTH_JUN] = _("June"),
	[MONTH_JUL] = _("July"),
	[MONTH_AUG] = _("August"),
	[MONTH_SEP] = _("September"),
	[MONTH_OCT] = _("October"),
	[MONTH_NOV] = _("November"),
	[MONTH_DEC] = _("December"),
};

static const struct StringList DayNames[32] = {
    [1]  = _("1st"),
    [2]  = _("2nd"),
    [3]  = _("3rd"),
    [4]  = _("4th"),
    [5]  = _("5th"),
    [6]  = _("6th"),
    [7]  = _("7th"),
    [8]  = _("8th"),
    [9]  = _("9th"),
    [10] = _("10th"),
    [11] = _("11th"),
    [12] = _("12th"),
    [13] = _("13th"),
    [14] = _("14th"),
    [15] = _("15th"),
    [16] = _("16th"),
    [17] = _("17th"),
    [18] = _("18th"),
    [19] = _("19th"),
    [20] = _("20th"),
    [21] = _("21st"),
    [22] = _("22nd"),
    [23] = _("23rd"),
    [24] = _("24th"),
    [25] = _("25th"),
    [26] = _("26th"),
    [27] = _("27th"),
    [28] = _("28th"),
    [29] = _("29th"),
    [30] = _("30th"),
    [31] = _("31st")
};

    
const u8 SeasonDisplay[] =  _("{STR_VAR_1} ({STR_VAR_2})");
const u8 DateDisplay1[]  =  _("{STR_VAR_1}, {STR_VAR_2}");
const u8 MonthString[]   =  _("{STR_VAR_1} {STR_VAR_2}, {STR_VAR_3}");
const u8 TimeDisplay[]   =  _("{STR_VAR_1}: {STR_VAR_2}");
const u8 TimeDisplay2[]  =  _("{STR_VAR_1}: 0{STR_VAR_2}");
const u8 Steps[]         =  _("{STR_VAR_1} - You have walked {STR_VAR_2} steps$");
const u8 sEliteReduxTitle[] =  _("Pokémon Elite Redux {STR_VAR_1}");

const u8 sText_Help_Bar_Enable[]  = _("Enable");
const u8 sText_Help_Bar_Disable[] = _("Disable");
const u8 sText_Help_Bar_Use[]     = _("Use");
const u8 sText_Help_Bar_Unmount[] = _("Dismount");

const u8 sText_Help_Bar[]         = _("{START_BUTTON} Save {L_BUTTON} {STR_VAR_2} Repel {R_BUTTON} {STR_VAR_1} Auto Run");
const u8 sText_Help_Bar_No_Bike[] = _("{START_BUTTON} Save {R_BUTTON} {STR_VAR_1} Auto Run");

const u8 sText_Message_Test[] = _(
	"Save your game with\n"
	"a complete record of\n"
	"your progress to take\n"
	"a break."
);

const u8 sText_Message_Cant_Save[] = _(
	"You can't save here!"
);

const u8 sText_Message_Cant_Use_Dexnav[] = _(
	"There are no Pokémon in\n"
	"this area."
);

const u8 sText_Message_Cant_Use_Bike[] = _(
	"You can't use your bike\n"
	"here."
);

const u8 sText_Message_Steps_Reset[] = _(
	"The step counter\n"
	"was reset."
);

const u8 sText_Message_Cant_Change_Time[] = _(
	"You can't change the\n"
	"time again today."
);

const u8 sText_Message_Saving[] = _(
	"Saving please wait...\n"
	"Don't turn off your\n"
	"console to avoid any\n"
	"problem."
);

const u8 sText_Message_TrainerInfo[] = _(
	"See all your information\n"
	"Battle Points: {STR_VAR_1}\n"
	"Defeated Trainers: {STR_VAR_2}\n"
	"Losses: {STR_VAR_3}"
);

#define EXTRA_SPACE_BETWEEN_OPTIONS_X (10 * 8)
#define EXTRA_SPACE_BETWEEN_OPTIONS_Y 3
#define EXTRA_SPACE_FOR_ICONS 22
#define NUM_POKEMON_PER_ROW 3
#define ICON_STARTING_X2 3

#define GFX_HELD_ITEM_X 3
#define GFX_HELD_ITEM_Y 3

#define GFX_HELD_ITEM_X_EXTRA 4
#define GFX_HELD_ITEM_Y_EXTRA 3

#define GFX_STATUS_MINUS_X 3

u8 getCurrentOptionIndex(u8 num){
	return sMenuDataPtr->MenuOptions[num];
}

static void PrintToWindow(void)
{
	u8 windowId = WINDOW_1;
	u8 i, j, x, y, x2, y2;
	u8 strArray[16]; //For the Player Name
	u8 hours     = gLocalTime.hours;
	u8 minutes   = gLocalTime.minutes;
	u8 font      = FONT_SMALL_NARROW;
	u8 fontColor = FONT_BLACK_2;
	bool8 isRepelEnabled = gSaveBlock2Ptr->permanentRepel;
	
	FillWindowPixelBuffer(windowId, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

	//Selection Sprite
	j = sMenuDataPtr->cursorRowY - sMenuDataPtr->FirstItem;
	x = 2;
	y = (j * EXTRA_SPACE_BETWEEN_OPTIONS_Y) + 2;
	
	BlitBitmapToWindow(windowId, sStartMenuCursor_Gfx, (x*8), (y*8), 96, 24);

	//Show Item Names
    x  = 5;
    y  = 2;
	x2 = 2;
	y2 = 4;

	for(i = 0; i < MAX_SHOWN_START_MENU_ROWS; i++){
		j = i + sMenuDataPtr->FirstItem;

		if(j == START_MENU_ACTION_PLAYER)
			StringCopy(&strArray[0], gSaveBlock2Ptr->playerName);
		else
			StringCopy(&strArray[0], StartMenuActions[getCurrentOptionIndex(j)].title);
		
		AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, &strArray[0]);

		switch(getCurrentOptionIndex(j)){
			case START_MENU_ACTION_POKEDEX:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Pokedex_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Pokedex, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_POKEMON:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Pokemon_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Pokemon, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_PLAYER:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Player_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Player, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_DEXNAV:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_DexNav_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_DexNav, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_BAG:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Bag_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Bag, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_PC:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Use_PC_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Use_PC, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			#ifdef DEBUG_BUILD
			case START_MENU_ACTION_DEBUG:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Debug_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Debug, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
    		#endif
			case START_MENU_ACTION_POKENAV:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_PokeNav_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_PokeNav, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_OPTIONS:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Options_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Options, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_GUIDE:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Info_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Info, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_SAVE:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Save_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Save, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
			case START_MENU_ACTION_EXIT:
			default:
				if(j == sMenuDataPtr->cursorRowY)
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Exit_Selected, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
				else
					BlitBitmapToWindow(windowId, sStartMenu_IconGfx_Exit, (x * 8) - EXTRA_SPACE_FOR_ICONS, (y * 8), 24, 24);
			break;
		}
		y = y + EXTRA_SPACE_BETWEEN_OPTIONS_Y;
	}
	
	//Help Bar
	x = 0;
    y = 18;
	ConvertIntToDecimalStringN(gStringVar2, hours, STR_CONV_MODE_RIGHT_ALIGN, 2);
	ConvertIntToDecimalStringN(gStringVar3, minutes, STR_CONV_MODE_LEFT_ALIGN, 2);
	if(!gSaveBlock2Ptr->autoRun)
		StringCopy(gStringVar1, sText_Help_Bar_Enable);
	else
		StringCopy(gStringVar1, sText_Help_Bar_Disable);

	if(!isRepelEnabled)
		StringCopy(gStringVar2, sText_Help_Bar_Enable);
	else
		StringCopy(gStringVar2, sText_Help_Bar_Disable);

	StringExpandPlaceholders(gStringVar4, sText_Help_Bar);
	AddTextPrinterParameterized4(windowId, font, (x*8)+4, (y*8), 0, 0, sMenuWindowFontColors[FONT_WHITE_2], 0xFF, gStringVar4);
	
	//Current Option Description
    x  = 16;
    y  = 12;
	x2 = 0;
	y2 = 4;

	switch(sMenuDataPtr->CurrentMessage){
		case MESSAGE_HELP_BAR:
			if(sMenuDataPtr->cursorRowY != START_MENU_ACTION_PLAYER)
				StringCopy(gStringVar4, StartMenuActions[getCurrentOptionIndex(sMenuDataPtr->cursorRowY)].description);
			else{
				u32 defeats      = 0 + VarGet(VAR_TIMES_WHITED_OUT);
				u32 battlepoints = gSaveBlock2Ptr->frontier.battlePoints;
				u16 trainers     = GetTrainerBattleWins();

				ConvertIntToDecimalStringN(gStringVar1, battlepoints, STR_CONV_MODE_LEFT_ALIGN, 5);
				ConvertIntToDecimalStringN(gStringVar2, trainers, STR_CONV_MODE_LEFT_ALIGN, 4);
				ConvertIntToDecimalStringN(gStringVar3, defeats, STR_CONV_MODE_LEFT_ALIGN, 8);
				StringExpandPlaceholders(gStringVar4, sText_Message_TrainerInfo);
			}
		break;
		case MESSAGE_CANT_SAVE:
			StringCopy(gStringVar4, sText_Message_Cant_Save);
		break;
		case MESSAGE_CANT_USE_BIKE:
			StringCopy(gStringVar4, sText_Message_Cant_Use_Bike);
		break;
		case MESSAGE_CANT_USE_DEXNAV:
			StringCopy(gStringVar4, sText_Message_Cant_Use_Dexnav);
		break;
		case MESSAGE_CANT_CHANGE_TIME:
			StringCopy(gStringVar4, sText_Message_Cant_Change_Time);
		break;
		case MESSAGE_STEPS_RESET:
			StringCopy(gStringVar4, sText_Message_Steps_Reset);
		break;
	}

	AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);

	//Top Bar
	x = 0;
    y = 0;
	x2 = 4;

	StringCopy(gStringVar1, gText_SavingVersionNum);
	StringExpandPlaceholders(gStringVar4, sEliteReduxTitle);
	AddTextPrinterParameterized4(windowId, font, (x*8) + x2, (y*8), 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);

	// HP Bars --------------------------------------------------------------------------------------------------------
    x = ICON_STARTING_X - 8;
	x2 = 0;
    y = ICON_STARTING_Y + 16;
	y2 = 0;

    for(i = 0; i < PARTY_SIZE; i++){
        if(GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE){
        	u32 currentStatus = GetAilmentFromStatus(GetMonData(&gPlayerParty[i], MON_DATA_STATUS));

            BlitBitmapToWindow(windowId, GetBarGfx(GetHPEggCyclePercent(i)), (x + x2) - ICON_STARTING_X2, (y + y2), 24, 8);
			
			if(GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM) != ITEM_NONE)
				BlitBitmapToWindow(windowId, sStartMenuHeldItem_Gfx, (x + x2) - ICON_STARTING_X2 - 4, (y + y2) - 8, 8, 8);

			switch(currentStatus){
				case AILMENT_BRN:
					BlitBitmapToWindow(windowId, sStartMenuStatus_Burn_Gfx, (x + x2) - ICON_STARTING_X2 + 20, (y + y2) - 8, 8, 8);
				break;
				case AILMENT_PRZ:
					BlitBitmapToWindow(windowId, sStartMenuStatus_Paralysis_Gfx, (x + x2) - ICON_STARTING_X2 + 20, (y + y2) - 8, 8, 8);
				break;
				case AILMENT_FRZ:
				case AILMENT_FSB:
					BlitBitmapToWindow(windowId, sStartMenuStatus_Freeze_Gfx, (x + x2) - ICON_STARTING_X2 + 20, (y + y2) - 8, 8, 8);
				break;
				case AILMENT_PSN:
					BlitBitmapToWindow(windowId, sStartMenuStatus_Poison_Gfx, (x + x2) - ICON_STARTING_X2 + 20, (y + y2) - 8, 8, 8);
				break;
				case AILMENT_SLP:
					BlitBitmapToWindow(windowId, sStartMenuStatus_Sleep_Gfx, (x + x2) - ICON_STARTING_X2 + 20, (y + y2) - 8, 8, 8);
				break;
        	}

			if((i + 1) == NUM_POKEMON_PER_ROW){
				y2 = y2 + Y_DIFFERENCE_BETWEEN_SPECIES;
				x2 = 0;
			}
			else{
				x2 = x2 + X_DIFFERENCE_BETWEEN_SPECIES;
			}
        }
    }
	
    CopyWindowToVram(windowId, 3);
	PutWindowTilemap(windowId);
}

//Map Stuff
/*static void CB2_OpenTownMapFromBag(void)
{
    OpenPokenavForTownMap(CB2_ReturnToUIMenu);
}

static void Task_OpenRegisteredTownMap(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        OpenPokenavForTownMap(CB2_ReturnToUIMenu);
        DestroyTask(taskId);
    }
}

void ItemUseOutOfBattle_TownMap(u8 taskId)
{
    if (gTasks[taskId].tUsingRegisteredKeyItem != TRUE)
    {
        gBagMenu->newScreenCallback = CB2_OpenTownMapFromBag;
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        gFieldCallback = FieldCB_ReturnToFieldNoScript;
        FadeScreen(FADE_TO_BLACK, 0);
        gTasks[taskId].func = Task_OpenRegisteredTownMap;
    }
}*/

static void Task_MenuWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_MenuMain;
}

static void Task_MenuTurnOff(u8 taskId)
{
    //s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        SetMainCallback2(sMenuDataPtr->savedCallback);
        Menu_FreeResources();
        DestroyTask(taskId);
    }
}

static void Task_MenuTurnOff_Bike(u8 taskId)
{
    //s16 *data = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
		/*if (gSaveBlock2Ptr->optionsBikeType == 0)
			GetOnOffBike(PLAYER_AVATAR_FLAG_MACH_BIKE);
		else*/
			GetOnOffBike(PLAYER_AVATAR_FLAG_ACRO_BIKE);
		ScriptUnfreezeObjectEvents();
		ScriptContext2_Disable();

        SetMainCallback2(sMenuDataPtr->savedCallback);
        Menu_FreeResources();
        DestroyTask(taskId);
    }
}

void Task_OpenPokedexFromStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		IncrementGameStat(GAME_STAT_CHECKED_POKEDEX);
		PlayRainStoppingSoundEffect();
		CleanupOverworldWindowsAndTilemaps();
		SetMainCallback2(CB2_OpenPokedex);
		DestroyTask(taskId);
    }
}

void Task_OpenPokemonPartyFromStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		PlayRainStoppingSoundEffect();
		CleanupOverworldWindowsAndTilemaps();
		SetMainCallback2(CB2_PartyMenuFromStartMenu); // Display party menu
    }
}

void Task_OpenTrainerCardFromStartMenu(u8 taskId)
{
	if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		PlayRainStoppingSoundEffect();
		CleanupOverworldWindowsAndTilemaps();
		
		if (InUnionRoom())
			ShowPlayerTrainerCard(CB2_ReturnToUIMenu); // Display trainer card
		else if (FlagGet(FLAG_SYS_FRONTIER_PASS))
			ShowFrontierPass(CB2_ReturnToUIMenu); // Display frontier pass
		else
			ShowPlayerTrainerCard(CB2_ReturnToUIMenu); // Display trainer card
    }
}

void Task_OpenBagFromStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		PlayRainStoppingSoundEffect();
		CleanupOverworldWindowsAndTilemaps();
		SetMainCallback2(CB2_BagMenuFromStartMenu); // Display bag menu
    }
}

void Task_OpenOptionsMenuStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		PlayRainStoppingSoundEffect();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitOptionMenu); // Display option menu
        gMain.savedCallback = CB2_ReturnToUIMenu;
    }
}

void Task_OpenInformationMenu(u8 taskId)
{
	if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        InformationMenu_Init(CB2_ReturnToUIMenu);
        DestroyTask(taskId);
    }
} 

void Task_OpenPokenavStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		PlayRainStoppingSoundEffect();
		CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitPokeNav);  // Display PokeNav
		DestroyTask(taskId);
    }
}  

void Task_OpenDebugStartMenu(u8 taskId)
{
	if (!gPaletteFade.active)
    {
		Menu_FreeResources();
        CleanupOverworldWindowsAndTilemaps();
    	FlagSet(FLAG_SYS_DEBUG_MENU_OPENED);
        Intro_Options_Menu_Init(CB2_ReturnToUIMenu);
        DestroyTask(taskId);
    }
}  

void Task_OpenVanillaStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
		Menu_FreeResources();
		SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

void Task_ChangeTrainerName(u8 taskId)
{
	Menu_FreeResources();
    if (!gPaletteFade.active)
    {
		NewGameBirchSpeech_SetDefaultPlayerName(Random() % 20);
		//DoNamingScreen(0, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToUIMenu, 0);
    }
}

void Task_OpenPokemonStorageSystemFromStartMenu(u8 taskId)
{
	Menu_FreeResources();
    if (!gPaletteFade.active)
    {
		SetMainCallback2(CB2_StartWallClock);
		gMain.savedCallback = CB2_ReturnToUIMenu;

		//NewGameBirchSpeech_SetDefaultPlayerName(Random() % 20);
		//DoNamingScreen(0, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToUIMenu, 0);
    }
}

static bool8 QuestMenuCallback(void)
{
    CreateTask(Task_OpenPokemonStorageSystemFromStartMenu, 0);
    return TRUE;
}

static void PressedDownButton(){
    u8 halfScreen = (MAX_SHOWN_START_MENU_ROWS) / 2;
    u8 finalhalfScreen = sMenuDataPtr->actionNumber - halfScreen;

    if(sMenuDataPtr->cursorRowY < halfScreen){
        sMenuDataPtr->cursorRowY++;
    }
	else if(sMenuDataPtr->cursorRowY >= (sMenuDataPtr->actionNumber - 1)){ 
		//If you are in the last option go to the first one
		sMenuDataPtr->cursorRowY = 0;
		sMenuDataPtr->FirstItem = 0;
    }
    else if(sMenuDataPtr->cursorRowY >= (finalhalfScreen - 1)){
        sMenuDataPtr->cursorRowY++;
    }
	else{
        sMenuDataPtr->cursorRowY++;
        sMenuDataPtr->FirstItem++;
    }
}

static void PressedUpButton(){
    u8 halfScreen = (MAX_SHOWN_START_MENU_ROWS) / 2;
    u8 finalhalfScreen = sMenuDataPtr->actionNumber - halfScreen;

    if(sMenuDataPtr->cursorRowY > halfScreen && sMenuDataPtr->cursorRowY <= (finalhalfScreen - 1)){
        sMenuDataPtr->cursorRowY--;
        sMenuDataPtr->FirstItem--;
    }
	else if(sMenuDataPtr->cursorRowY == 0){ 
		//If you are in the first option go to the last one
		sMenuDataPtr->cursorRowY = sMenuDataPtr->actionNumber - 1;
		sMenuDataPtr->FirstItem = sMenuDataPtr->actionNumber - MAX_SHOWN_START_MENU_ROWS;
    }
    else{
        sMenuDataPtr->cursorRowY--;
    }
}

static bool8 canUsePC(void){
    bool8 disablePC = FALSE;

	switch(gSaveBlock1Ptr->location.mapNum){
		case MAP_NUM(EVER_GRANDE_CITY_SIDNEYS_ROOM):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_SIDNEYS_ROOM))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_PHOEBES_ROOM):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_PHOEBES_ROOM))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_GLACIAS_ROOM):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_GLACIAS_ROOM))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_DRAKES_ROOM):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_DRAKES_ROOM))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_CHAMPIONS_ROOM):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_CHAMPIONS_ROOM))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_HALL1):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL1))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_HALL2):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL2))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_HALL3):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL3))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_HALL4):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL4))
				disablePC = TRUE;
		break;
		case MAP_NUM(EVER_GRANDE_CITY_HALL5):
			if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL5))
				disablePC = TRUE;
		break;
	}

	if(!disablePC && FlagGet(FLAG_SYS_POKEMON_GET))
	    return TRUE;

	return FALSE;
}

static bool8 ShouldDisableSaving(){
    u16 mapSection = 0; //getCurrentMapSection();

	if(mapSection == MAPSEC_SAFARI_ZONE)
		return TRUE;

	return FALSE;
}

static bool8 ShouldDisablePokemonStorageSystem(void){
	bool8 disablePC = FALSE;

	switch (gSaveBlock1Ptr->location.mapNum) {
        case MAP_NUM(EVER_GRANDE_CITY_SIDNEYS_ROOM):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_SIDNEYS_ROOM))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_PHOEBES_ROOM):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_PHOEBES_ROOM))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_GLACIAS_ROOM):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_GLACIAS_ROOM))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_DRAKES_ROOM):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_DRAKES_ROOM))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_CHAMPIONS_ROOM):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_CHAMPIONS_ROOM))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_HALL1):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL1))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_HALL2):
    	    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL2))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_HALL3):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL3))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_HALL4):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL4))
                disablePC = TRUE;
        break;
        case MAP_NUM(EVER_GRANDE_CITY_HALL5):
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EVER_GRANDE_CITY_HALL5))
                disablePC = TRUE;
        break;
    }

	return disablePC;
}

static void UIStartMenuOpenNormanMenuOption(u8 taskId, u8 option){
	gSaveBlock2Ptr->startMenuOptionToOpen = option;
	PlaySE(SE_SELECT);
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
	gTasks[taskId].func = Task_OpenVanillaStartMenu;
}

static void StartMenuTrySaving(u8 taskId){
	if(!ShouldDisableSaving()){
		UIStartMenuOpenNormanMenuOption(taskId, MENU_ACTION_SAVE);
	}
	else{
		PlaySE(SE_BOO);
		sMenuDataPtr->CurrentMessage = MESSAGE_CANT_SAVE;
	}
}

static void StartMenuTryUsingPC(u8 taskId){
	if(!ShouldDisablePokemonStorageSystem()){
		UIStartMenuOpenNormanMenuOption(taskId, MENU_ACTION_ACCESS_PC);
	}
	else{
		PlaySE(SE_BOO);
		sMenuDataPtr->CurrentMessage = MESSAGE_CANT_SAVE;
	}
}

bool8 CanUseBikeFromStartMenu(void){
	s16 coordsY;
	s16 coordsX;
	u8 behavior;
	PlayerGetDestCoords(&coordsX, &coordsY);
	behavior = MapGridGetMetatileBehaviorAt(coordsX, coordsY);

	if(!CheckBagHasItem(ITEM_ACRO_BIKE, 1) && !CheckBagHasItem(ITEM_MACH_BIKE, 1))
		return FALSE;

	if (FlagGet(FLAG_SYS_CYCLING_ROAD) == TRUE                      || 
	    MetatileBehavior_IsVerticalRail(behavior) == TRUE           || 
		MetatileBehavior_IsHorizontalRail(behavior) == TRUE         || 
		MetatileBehavior_IsIsolatedVerticalRail(behavior) == TRUE   || 
		MetatileBehavior_IsIsolatedHorizontalRail(behavior) == TRUE ||
		IsBikingDisallowedByPlayer())
		return FALSE;

	return TRUE;
}

/* This is the meat of the UI. This is where you wait for player inputs and can branch to other tasks accordingly */
static void Task_MenuMain(u8 taskId)
{
	sMenuDataPtr->CurrentMessage = MESSAGE_HELP_BAR;

	gSaveBlock2Ptr->start_cursorRowY = sMenuDataPtr->cursorRowY;
	gSaveBlock2Ptr->start_FirstItem  = sMenuDataPtr->FirstItem;
	
	if (JOY_NEW(B_BUTTON))
    {
		PlaySE(SE_PC_OFF);
		BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
		gTasks[taskId].func = Task_MenuTurnOff;
    }
	
	if (JOY_NEW(A_BUTTON))
    {
		switch(getCurrentOptionIndex(sMenuDataPtr->cursorRowY))
		{
			case START_MENU_ACTION_POKEDEX:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenPokedexFromStartMenu;
			break;
			case START_MENU_ACTION_PLAYER:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenTrainerCardFromStartMenu;
			break;
			case START_MENU_ACTION_POKEMON:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenPokemonPartyFromStartMenu;
			break;
			case START_MENU_ACTION_PC:
				StartMenuTryUsingPC(taskId);
			break;
			case START_MENU_ACTION_BAG:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenBagFromStartMenu;
			break;
			case START_MENU_ACTION_OPTIONS:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenOptionsMenuStartMenu;
			break;
			case START_MENU_ACTION_GUIDE:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenInformationMenu;
			break;
			case START_MENU_ACTION_POKENAV:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenPokenavStartMenu;
			break;
			case START_MENU_ACTION_DEXNAV:
				if(canOpenDexnav()){
					PlaySE(SE_SELECT);
					BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
					gTasks[taskId].func = Task_OpenDexNavFromStartMenu;
				}
				else{
					PlaySE(SE_BOO);
					sMenuDataPtr->CurrentMessage = MESSAGE_CANT_USE_DEXNAV;
				}
			break;
			#ifdef DEBUG_BUILD
			case START_MENU_ACTION_DEBUG:
				PlaySE(SE_SELECT);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_OpenDebugStartMenu;
			break;
			#endif
			case START_MENU_ACTION_SAVE:
				StartMenuTrySaving(taskId);
			break;
			case START_MENU_ACTION_EXIT:
				PlaySE(SE_PC_OFF);
				BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
				gTasks[taskId].func = Task_MenuTurnOff;
			break;
		}

		PrintToWindow();
	}
	
	if (JOY_NEW(R_BUTTON)){
		
		if (gSaveBlock2Ptr->autoRun){
			gSaveBlock2Ptr->autoRun = FALSE;
			PlaySE(SE_BOO);
		}
		else{
			gSaveBlock2Ptr->autoRun = TRUE;
			PlaySE(SE_SELECT);
		}
		PrintToWindow();
	}
	
	if (JOY_NEW(START_BUTTON)){
		StartMenuTrySaving(taskId);
	}
	
	if (JOY_NEW(L_BUTTON)){
		PlaySE(SE_SELECT);
		gSaveBlock2Ptr->permanentRepel = !gSaveBlock2Ptr->permanentRepel;
		PrintToWindow();
	}
	
    if ((JOY_NEW(DPAD_DOWN)) || (JOY_REPEAT(DPAD_DOWN))){
		PressedDownButton();

		PlaySE(SE_SELECT);
		PrintToWindow();
	}
	
    if ((JOY_NEW(DPAD_UP)) || (JOY_REPEAT(DPAD_UP))){
		
		PressedUpButton();
		
		PlaySE(SE_SELECT);	
		PrintToWindow();
	}
}
