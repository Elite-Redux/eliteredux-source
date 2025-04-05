#include "global.h"
#include "main.h"
#include "task.h"
#include "palette.h"
#include "overworld.h"
#include "ui_information_menu.h"
#include "malloc.h"
#include "menu_helpers.h"
#include "menu.h"
#include "scanline_effect.h"
#include "string_util.h"
#include "strings.h"
#include "trig.h"
#include "bg.h"
#include "constants/rgb.h"
#include "gpu_regs.h"
#include "decompress.h"
#include "text_window.h"
#include "sound.h"
#include "constants/songs.h"
#include "window.h"
#include "generated/data/text/help_articles.h"

#define MAX_ENTRIES_ON_SCREEN 5

// Sprites
enum {
    START_MENU_SPRITE_UP_ARROW,
    START_MENU_SPRITE_DOWN_ARROW,
    NUM_INFORMATION_MENU_SPRITES,
};

struct MenuResources {
    MainCallback savedCallback;
    u8 gfxLoadState;
    u8 selectorOffset;
    u8 selectorPosY;
    u8 currentTab;
    u8 entryIdx;
    u8 pageIdx;
    bool8 isEntryOpen;
    u8 spriteIDs[NUM_INFORMATION_MENU_SPRITES];
};

enum WindowIds {
    WINDOW_1,
};

// EWRAW
static EWRAM_DATA struct MenuResources *sMenuDataPtr = NULL;
static EWRAM_DATA u8 *sBg1TilemapBuffer = NULL;

// static defines
static void Menu_RunSetup(void);
static bool8 Menu_DoGfxSetup(void);
static bool8 Menu_InitBgs(void);
static void Menu_FadeAndBail(void);
static bool8 Menu_LoadGraphics(void);
static void Menu_InitWindows(void);
static void PrintToWindow(void);
static void Task_MenuWaitFadeIn(u8 taskId);
static void Task_MenuMain(u8 taskId);
static u8 getCurrentTabEntries(void);
static void LoadTabPalette(void);

// const data
static const struct BgTemplate sMenuBgTemplates[] = {{.bg = 0,  // windows, etc
                                                      .charBaseIndex = 0,
                                                      .mapBaseIndex = 31,
                                                      .priority = 1},
                                                     {.bg = 1,  // this bg loads the UI tilemap
                                                      .charBaseIndex = 3,
                                                      .mapBaseIndex = 30,
                                                      .priority = 2},
                                                     {.bg = 2,  // this bg loads the UI tilemap
                                                      .charBaseIndex = 0,
                                                      .mapBaseIndex = 28,
                                                      .priority = 0}};

static const struct WindowTemplate sMenuWindowTemplates[] = {
    [WINDOW_1] =
        {
            .bg = 0,           // which bg to print text on
            .tilemapLeft = 0,  // position from left (per 8 pixels)
            .tilemapTop = 0,   // position from top (per 8 pixels)
            .width = 30,       // width (per 8 pixels)
            .height = 20,      // height (per 8 pixels)
            .paletteNum = 0,   // palette index to use for text
            .baseBlock = 1,    // tile start in VRAM
        },
};

static const u32 sMenuTiles[] = INCBIN_U32("graphics/ui_menus/information_menu/tiles.4bpp.lz");
static const u32 sMenuTilemap[] = INCBIN_U32("graphics/ui_menus/information_menu/tilemap.bin.lz");
static const u32 sMenuTilemap2[] = INCBIN_U32("graphics/ui_menus/information_menu/tilemap_selected.bin.lz");

// Palettes
static const u16 sMenuPalette[] = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette.gbapal");
static const u16 sMenuPalette_Blue[] = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_blue.gbapal");
static const u16 sMenuPalette_Yellow[] = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_yellow.gbapal");
static const u16 sMenuPalette_Green[] = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_green.gbapal");
static const u16 sMenuPalette_Dark[] = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_dark.gbapal");
static const u16 sMenuPalette_Red[] = INCBIN_U16("graphics/ui_menus/sharedPalettes/palette_red.gbapal");

enum Colors {
    FONT_BLACK,
    FONT_BLACK_2,
    FONT_WHITE,
    FONT_WHITE_2,
    FONT_RED,
    FONT_BLUE,
};

static const u8 sMenuWindowFontColors[][3] = {
    [FONT_BLACK] = {TEXT_COLOR_TRANSPARENT, 3, 2},
    [FONT_BLACK_2] = {TEXT_COLOR_TRANSPARENT, 3, TEXT_COLOR_TRANSPARENT},
    [FONT_WHITE] = {TEXT_COLOR_TRANSPARENT, 1, 2},
    [FONT_WHITE_2] = {TEXT_COLOR_TRANSPARENT, 1, TEXT_COLOR_TRANSPARENT},
    [FONT_RED] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_GRAY},
    [FONT_BLUE] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_BLUE, TEXT_COLOR_LIGHT_GRAY},
};

static const u8 sSelector[] = INCBIN_U8("graphics/ui_menus/information_menu/selector.4bpp");
static const u8 sSelector2[] = INCBIN_U8("graphics/ui_menus/information_menu/selector2.4bpp");

// functions
void Task_OpenInformationMenuFromStartMenu(u8 taskId) {
    if (!gPaletteFade.active) {
        CleanupOverworldWindowsAndTilemaps();
        InformationMenu_Init(CB2_ReturnToFieldWithOpenMenu);
        DestroyTask(taskId);
    }
}

void InformationMenu_Init(MainCallback callback) {
    if ((sMenuDataPtr = AllocZeroed(sizeof(struct MenuResources))) == NULL) {
        SetMainCallback2(callback);
        return;
    }

    // initialize stuff
    sMenuDataPtr->gfxLoadState = 0;
    sMenuDataPtr->currentTab = 0;
    sMenuDataPtr->savedCallback = callback;

    SetMainCallback2(Menu_RunSetup);
}

void InformationMenu_Init_From_Battle(MainCallback callback) {
    if ((sMenuDataPtr = AllocZeroed(sizeof(struct MenuResources))) == NULL) {
        SetMainCallback2(callback);
        return;
    }

    // initialize stuff
    sMenuDataPtr->gfxLoadState = 0;
    sMenuDataPtr->currentTab = 0;
    sMenuDataPtr->savedCallback = callback;

    SetMainCallback2(Menu_RunSetup);
}

static void Menu_RunSetup(void) {
    while (1) {
        if (Menu_DoGfxSetup() == TRUE) break;
    }
}

static void Menu_MainCB(void) {
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void Menu_VBlankCB(void) {
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Arrows
static void SpriteCallback_Inventory_UpArrow(struct Sprite *sprite) {
    u8 val = sprite->data[0];
    sprite->y2 = gSineTable[val] / 128;
    sprite->data[0] += 8;

    if (sMenuDataPtr->entryIdx == 0 || sMenuDataPtr->isEntryOpen)
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
}

static void SpriteCallback_Inventory_DownArrow(struct Sprite *sprite) {
    u8 numEntries = getCurrentTabEntries();
    u8 val = sprite->data[0] + 128;
    sprite->y2 = gSineTable[val] / 128;
    sprite->data[0] += 8;

    if (sMenuDataPtr->entryIdx >= (numEntries - 1) || sMenuDataPtr->isEntryOpen)  // Because of the Exit Button
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
}

#define PAL_UI_SPRITES 0
static const u32 sStartMenuUpArrow_Gfx[] = INCBIN_U32("graphics/ui_menus/start_menu/arrow_up.4bpp.lz");
static const u32 sStartMenuDownArrow_Gfx[] = INCBIN_U32("graphics/ui_menus/start_menu/arrow_down.4bpp.lz");
static const u16 sMenuInterfacePalette_Sprites[] = INCBIN_U16("graphics/ui_menus/start_menu/palette_interface.gbapal");

static const struct SpritePalette sInventoryInterfaceSpritePalette[] = {
    {sMenuInterfacePalette_Sprites, PAL_UI_SPRITES},
};

#define UP_ARROW_X 120 - 8
#define UP_ARROW_Y 12 + 8

static void CreateUpArrowSprite(void) {
    u8 spriteId;
    u8 SpriteTag = START_MENU_SPRITE_UP_ARROW;
    struct CompressedSpriteSheet sSpriteSheet_InventoryUpArrow = {sStartMenuUpArrow_Gfx, 0x0800, SpriteTag};
    struct SpriteTemplate TempSpriteTemplate = gDummySpriteTemplate;

    TempSpriteTemplate.tileTag = SpriteTag;
    TempSpriteTemplate.callback = SpriteCallback_Inventory_UpArrow;

    LoadCompressedSpriteSheet(&sSpriteSheet_InventoryUpArrow);
    LoadSpritePalette(sInventoryInterfaceSpritePalette);
    spriteId = CreateSprite(&TempSpriteTemplate, UP_ARROW_X, UP_ARROW_Y, 0);
    sMenuDataPtr->spriteIDs[SpriteTag] = spriteId;

    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.shape = SPRITE_SHAPE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.size = SPRITE_SIZE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.priority = 1;
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.paletteNum = PAL_UI_SPRITES;
}

#define DOWN_ARROW_X UP_ARROW_X
#define DOWN_ARROW_Y 96 + 40

static void CreateDownArrowSprite(void) {
    u8 spriteId;
    u8 SpriteTag = START_MENU_SPRITE_DOWN_ARROW;
    struct CompressedSpriteSheet sSpriteSheet_InventoryDownArrow = {sStartMenuDownArrow_Gfx, 0x0800, SpriteTag};
    struct SpriteTemplate TempSpriteTemplate = gDummySpriteTemplate;

    TempSpriteTemplate.tileTag = SpriteTag;
    TempSpriteTemplate.callback = SpriteCallback_Inventory_DownArrow;

    LoadCompressedSpriteSheet(&sSpriteSheet_InventoryDownArrow);
    spriteId = CreateSprite(&TempSpriteTemplate, DOWN_ARROW_X, DOWN_ARROW_Y, 0);
    sMenuDataPtr->spriteIDs[SpriteTag] = spriteId;

    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.shape = SPRITE_SHAPE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.size = SPRITE_SIZE(16x16);
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.priority = 1;
    gSprites[sMenuDataPtr->spriteIDs[SpriteTag]].oam.paletteNum = PAL_UI_SPRITES;
}

static bool8 Menu_DoGfxSetup(void) {
    switch (gMain.state) {
        case 0:
            DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000) SetVBlankHBlankCallbacksToNull();
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
            if (Menu_InitBgs()) {
                sMenuDataPtr->gfxLoadState = 0;
                gMain.state++;
            } else {
                Menu_FadeAndBail();
                return TRUE;
            }
            break;
        case 3:
            if (Menu_LoadGraphics() == TRUE) gMain.state++;
            break;
        case 4:
            CreateUpArrowSprite();
            CreateDownArrowSprite();
            LoadMessageBoxAndBorderGfx();
            Menu_InitWindows();
            gMain.state++;
            break;
        case 5:
            PrintToWindow();
            CreateTask(Task_MenuWaitFadeIn, 0);
            BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
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

#define try_free(ptr)                     \
    ({                                    \
        void **ptr__ = (void **)&(ptr);   \
        if (*ptr__ != NULL) Free(*ptr__); \
    })

static void Menu_UpdateTilemap(void) {
    try_free(sBg1TilemapBuffer);
    sBg1TilemapBuffer = NULL;

    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = Alloc(0x800);

    memset(sBg1TilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sMenuBgTemplates, NELEMS(sMenuBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    ScheduleBgCopyTilemapToVram(1);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);

    if (sMenuDataPtr->isEntryOpen)
        LZDecompressWram(sMenuTilemap2, sBg1TilemapBuffer);
    else
        LZDecompressWram(sMenuTilemap, sBg1TilemapBuffer);
}

static void Menu_FreeResources(void) {
    try_free(sMenuDataPtr);
    try_free(sBg1TilemapBuffer);
    FreeAllWindowBuffers();
}

static void Task_MenuWaitFadeAndBail(u8 taskId) {
    if (!gPaletteFade.active) {
        SetMainCallback2(sMenuDataPtr->savedCallback);
        Menu_FreeResources();
        DestroyTask(taskId);
    }
}

static void Menu_FadeAndBail(void) {
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_MenuWaitFadeAndBail, 0);
    SetVBlankCallback(Menu_VBlankCB);
    SetMainCallback2(Menu_MainCB);
}

static bool8 Menu_InitBgs(void) {
    ResetVramOamAndBgCntRegs();
    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = AllocZeroed(BG_SCREEN_SIZE);
    if (sBg1TilemapBuffer == NULL) return FALSE;

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sMenuBgTemplates, NELEMS(sMenuBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    return TRUE;
}

static bool8 Menu_LoadGraphics(void) {
    switch (sMenuDataPtr->gfxLoadState) {
        case 0:
            ResetTempTileDataBuffers();
            DecompressAndCopyTileDataToVram(1, sMenuTiles, 0, 0, 0);
            sMenuDataPtr->gfxLoadState++;
            break;
        case 1:
            if (FreeTempTileDataBuffersIfPossible() != TRUE) {
                LZDecompressWram(sMenuTilemap, sBg1TilemapBuffer);
                sMenuDataPtr->gfxLoadState++;
            }
            break;
        case 2:
            LoadTabPalette();
            sMenuDataPtr->gfxLoadState++;
            break;
        default:
            sMenuDataPtr->gfxLoadState = 0;
            return TRUE;
    }
    return FALSE;
}

static void Menu_InitWindows(void) {
    INIT_WINDOWS(sMenuWindowTemplates);
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0);

    FillWindowPixelBuffer(WINDOW_1, 0);
    LoadUserWindowBorderGfx(WINDOW_1, 720, 14 * 16);
    PutWindowTilemap(WINDOW_1);
    CopyWindowToVram(WINDOW_1, 3);

    ScheduleBgCopyTilemapToVram(2);
}

static void LoadTabPalette(void) {
    u8 currentTab = sMenuDataPtr->currentTab;
    switch (gHelpArticles[currentTab].color) {
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

#define SPACE_BETWEEN_INFORMATION_START 24
#define SPACE_BETWEEN_INFORMATION_OPTIONS 24
#define INFORMATION_HELP_BAR_Y (18 * 8)
static const u8 sText_PageNum[] = _("Page {STR_VAR_1}/{STR_VAR_2}");
const u8 sText_Information_Menu_Title[] = _("Pokémon Elite Redux - {STR_VAR_1}");
const u8 sText_Help_Bar_Information[] = _("{A_BUTTON} View Info {DPAD_LEFTRIGHT} Move Tab {DPAD_UPDOWN} Move Entry {B_BUTTON} Exit");
const u8 sText_Help_Bar_Information2[] = _("{DPAD_LEFTRIGHT} Page {DPAD_UPDOWN} Entry {B_BUTTON} Exit");

static void PrintToWindow(void) {
    u8 windowId = WINDOW_1;
    u8 fontTitle = FONT_NARROW;
    u8 fontText = FONT_SMALL_NARROW;
    u8 currentTab = sMenuDataPtr->currentTab;
    u8 entryIdx = sMenuDataPtr->entryIdx;
    u8 pageIdx = sMenuDataPtr->pageIdx;
    u8 offset = sMenuDataPtr->selectorOffset;
    u8 numPages = gHelpArticles[currentTab].entries[entryIdx].numPages;
    u8 numEntries = min(MAX_ENTRIES_ON_SCREEN, getCurrentTabEntries());
    u8 infoTitleFontColor = FONT_BLACK_2;
    u8 helpBarFontColor = FONT_WHITE_2;
    u8 titleFontColor = FONT_BLACK;
    u8 textFontColor = FONT_BLACK;
    u8 i = 0;
    u8 x = 16;
    u8 y = 0;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

    // Help Bar
    StringCopy(gStringVar1, gHelpArticles[currentTab].title);
    StringExpandPlaceholders(gStringVar4, sText_Information_Menu_Title);
    AddTextPrinterParameterized4(windowId, fontTitle, x, y, 0, 0, sMenuWindowFontColors[infoTitleFontColor], 0xFF, gStringVar4);

    if (!sMenuDataPtr->isEntryOpen) {
        for (i = 0; i < numEntries; i++) {
            y = SPACE_BETWEEN_INFORMATION_START + (SPACE_BETWEEN_INFORMATION_OPTIONS * i);
            AddTextPrinterParameterized4(
                windowId, fontTitle, x, y, 0, 0, sMenuWindowFontColors[titleFontColor], 0xFF, gHelpArticles[currentTab].entries[offset + i].title);
        }
        BlitBitmapToWindow(windowId, sSelector, 8, SPACE_BETWEEN_INFORMATION_START + (SPACE_BETWEEN_INFORMATION_OPTIONS * sMenuDataPtr->selectorPosY), 8, 16);
        BlitBitmapToWindow(
            windowId, sSelector2, (8 * 28), SPACE_BETWEEN_INFORMATION_START + (SPACE_BETWEEN_INFORMATION_OPTIONS * sMenuDataPtr->selectorPosY), 8, 16);

        // Help Bar
        x = (2 * 8);
        y = INFORMATION_HELP_BAR_Y;
        AddTextPrinterParameterized4(windowId, fontTitle, 8, y, 0, 0, sMenuWindowFontColors[helpBarFontColor], 0xFF, sText_Help_Bar_Information);
    } else {
        x = (2 * 8);
        y = (3 * 8);
        AddTextPrinterParameterized4(
            windowId, fontTitle, x, y, 0, 0, sMenuWindowFontColors[titleFontColor], 0xFF, gHelpArticles[currentTab].entries[entryIdx].title);
        y = y + (2 * 8) + 4;
        AddTextPrinterParameterized4(
            windowId, fontText, x, y, 0, 0, sMenuWindowFontColors[textFontColor], 0xFF, gHelpArticles[currentTab].entries[entryIdx].pages[pageIdx]);

        // Num Pages
        y = y + (10 * 8);

        if (numPages == 0) numPages = 1;

        ConvertIntToDecimalStringN(gStringVar1, sMenuDataPtr->pageIdx + 1, STR_CONV_MODE_RIGHT_ALIGN, 1);
        ConvertIntToDecimalStringN(gStringVar2, numPages, STR_CONV_MODE_RIGHT_ALIGN, 1);
        StringExpandPlaceholders(gStringVar4, sText_PageNum);

        AddTextPrinterParameterized4(windowId, fontText, x, y, 0, 0, sMenuWindowFontColors[textFontColor], 0xFF, gStringVar4);

        // Help Bar
        x = (2 * 8);
        y = INFORMATION_HELP_BAR_Y;
        AddTextPrinterParameterized4(windowId, fontTitle, 8, y, 0, 0, sMenuWindowFontColors[helpBarFontColor], 0xFF, sText_Help_Bar_Information2);
    }

    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, 3);
}

static void Task_MenuWaitFadeIn(u8 taskId) {
    if (!gPaletteFade.active) gTasks[taskId].func = Task_MenuMain;
}

static void Task_MenuTurnOff(u8 taskId) {
    if (!gPaletteFade.active) {
        SetMainCallback2(sMenuDataPtr->savedCallback);
        Menu_FreeResources();
        DestroyTask(taskId);
    }
}

static u8 getCurrentTabEntries(void) {
    u8 currentTab = sMenuDataPtr->currentTab;
    u8 numEntries = gHelpArticles[currentTab].numEntries;

    return numEntries;
}

static void Menu_PressedButtonUp(void) {
    u8 numEntries = getCurrentTabEntries();
    u8 halfScreen = MAX_ENTRIES_ON_SCREEN / 2;
    u8 maxOffset = numEntries - MAX_ENTRIES_ON_SCREEN;
    u8 maxPosY = MAX_ENTRIES_ON_SCREEN - 1;

    if (numEntries < MAX_ENTRIES_ON_SCREEN) {
        if (sMenuDataPtr->selectorPosY > 0)
            sMenuDataPtr->selectorPosY--;
        else
            sMenuDataPtr->selectorPosY = numEntries - 1;
    } else {
        if (sMenuDataPtr->selectorPosY > halfScreen)
            sMenuDataPtr->selectorPosY--;
        else if (sMenuDataPtr->selectorOffset > 0)
            sMenuDataPtr->selectorOffset--;
        else if (sMenuDataPtr->selectorPosY > 0)
            sMenuDataPtr->selectorPosY--;
        else {
            sMenuDataPtr->selectorPosY = maxPosY;
            sMenuDataPtr->selectorOffset = maxOffset;
        }
    }

    sMenuDataPtr->entryIdx = sMenuDataPtr->selectorPosY + sMenuDataPtr->selectorOffset;
    sMenuDataPtr->pageIdx = 0;
}

static void Menu_PressedButtonDown(void) {
    u8 numEntries = getCurrentTabEntries() - 1;
    u8 halfScreen = MAX_ENTRIES_ON_SCREEN / 2;
    u8 maxOffset = numEntries - MAX_ENTRIES_ON_SCREEN;
    u8 maxPosY = MAX_ENTRIES_ON_SCREEN - 1;

    if (numEntries < MAX_ENTRIES_ON_SCREEN) {
        if (sMenuDataPtr->selectorPosY < numEntries)
            sMenuDataPtr->selectorPosY++;
        else
            sMenuDataPtr->selectorPosY = 0;
    } else {
        if (sMenuDataPtr->selectorPosY < halfScreen)
            sMenuDataPtr->selectorPosY++;
        else if (sMenuDataPtr->selectorOffset < maxOffset)
            sMenuDataPtr->selectorOffset++;
        else if (sMenuDataPtr->selectorPosY < maxPosY)
            sMenuDataPtr->selectorPosY++;
        else {
            sMenuDataPtr->selectorPosY = 0;
            sMenuDataPtr->selectorOffset = 0;
        }
    }

    sMenuDataPtr->entryIdx = sMenuDataPtr->selectorPosY + sMenuDataPtr->selectorOffset;
    sMenuDataPtr->pageIdx = 0;
}

/* This is the meat of the UI. This is where you wait for player inputs and can branch to other tasks accordingly */
static void Task_MenuMain(u8 taskId) {
    u8 entryIdx = sMenuDataPtr->entryIdx;
    u8 currentTab = sMenuDataPtr->currentTab;

    if (sMenuDataPtr->isEntryOpen) {
        u8 numPages = gHelpArticles[currentTab].entries[entryIdx].numPages;
        if (JOY_NEW(DPAD_RIGHT)) {
            PlaySE(SE_SELECT);
            if (sMenuDataPtr->pageIdx < numPages - 1)
                sMenuDataPtr->pageIdx++;
            else
                sMenuDataPtr->pageIdx = 0;
            PrintToWindow();
        }

        if (JOY_NEW(DPAD_LEFT)) {
            PlaySE(SE_SELECT);
            if (sMenuDataPtr->pageIdx != 0)
                sMenuDataPtr->pageIdx--;
            else
                sMenuDataPtr->pageIdx = numPages - 1;
            PrintToWindow();
        }
    } else {
        u8 numTabs = ARRAY_COUNT(gHelpArticles);
        if (JOY_NEW(DPAD_RIGHT)) {
            PlaySE(SE_SELECT);
            if (sMenuDataPtr->currentTab < numTabs - 1)
                sMenuDataPtr->currentTab++;
            else
                sMenuDataPtr->currentTab = 0;

            sMenuDataPtr->selectorPosY = 0;
            sMenuDataPtr->selectorOffset = 0;
            LoadTabPalette();
            PrintToWindow();
        }

        if (JOY_NEW(DPAD_LEFT)) {
            PlaySE(SE_SELECT);
            if (sMenuDataPtr->currentTab != 0)
                sMenuDataPtr->currentTab--;
            else
                sMenuDataPtr->currentTab = numTabs - 1;
            sMenuDataPtr->selectorPosY = 0;
            sMenuDataPtr->selectorOffset = 0;
            LoadTabPalette();
            PrintToWindow();
        }
    }

    if (JOY_NEW(A_BUTTON)) {
        PlaySE(SE_SELECT);
        sMenuDataPtr->isEntryOpen = !sMenuDataPtr->isEntryOpen;
        Menu_UpdateTilemap();
        PrintToWindow();
    }

    if (JOY_NEW(DPAD_UP)) {
        PlaySE(SE_SELECT);
        Menu_PressedButtonUp();
        PrintToWindow();
    }

    if (JOY_NEW(DPAD_DOWN)) {
        PlaySE(SE_SELECT);
        Menu_PressedButtonDown();
        PrintToWindow();
    }

    if (JOY_NEW(B_BUTTON) && sMenuDataPtr->isEntryOpen) {
        PlaySE(SE_SELECT);
        sMenuDataPtr->isEntryOpen = FALSE;
        Menu_UpdateTilemap();
        PrintToWindow();
    } else if (JOY_NEW(B_BUTTON) && !sMenuDataPtr->isEntryOpen) {
        PlaySE(SE_PC_OFF);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_MenuTurnOff;
    }
}