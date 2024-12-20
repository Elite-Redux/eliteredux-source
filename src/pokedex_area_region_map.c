#include "global.h"
#include "main.h"
#include "menu.h"
#include "bg.h"
#include "malloc.h"
#include "palette.h"
#include "pokedex_area_region_map.h"

static EWRAM_DATA u8 *sPokedexAreaMapBgNum = NULL;

static const u16 sPokedexAreaMap_Pal[] = INCBIN_U16("graphics/interface/region_map.gbapal");
static const u32 sPokedexAreaMap_Gfx[] = INCBIN_U32("graphics/interface/region_map.8bpp.lz");
static const u32 sPokedexAreaMap_Tilemap[] = INCBIN_U32("graphics/interface/region_map.bin.lz");
static const u32 sPokedexAreaMapAffine_Gfx[] = INCBIN_U32("graphics/interface/region_map_affine.8bpp.lz");
static const u32 sPokedexAreaMapAffine_Tilemap[] = INCBIN_U32("graphics/interface/region_map_affine.bin.lz");

void LoadPokedexAreaMapGfx(const struct PokedexAreaMapTemplate *spriteTemplate)
{
    u8 mode;
    sPokedexAreaMapBgNum = Alloc(4);
    mode = spriteTemplate->mode;

    if (mode == 0)
    {
        SetBgAttribute(spriteTemplate->bg, BG_ATTR_METRIC, 0);
        DecompressAndCopyTileDataToVram(spriteTemplate->bg, sPokedexAreaMap_Gfx, 0, spriteTemplate->offset, 0);
        sub_8199D3C(DecompressAndCopyTileDataToVram(spriteTemplate->bg, sPokedexAreaMap_Tilemap, 0, 0, 1), spriteTemplate->offset, 32, 32, FALSE);
    }
    else
    {
        SetBgAttribute(spriteTemplate->bg, BG_ATTR_METRIC, 2);
        SetBgAttribute(spriteTemplate->bg, BG_ATTR_TYPE, 1);
        DecompressAndCopyTileDataToVram(spriteTemplate->bg, sPokedexAreaMapAffine_Gfx, 0, spriteTemplate->offset, 0);
        sub_8199D3C(DecompressAndCopyTileDataToVram(spriteTemplate->bg, sPokedexAreaMapAffine_Tilemap, 0, 0, 1), spriteTemplate->offset, 64, 64, TRUE);
    }

    ChangeBgX(spriteTemplate->bg, 0, 0);
    ChangeBgY(spriteTemplate->bg, 0, 0);
    SetBgAttribute(spriteTemplate->bg, BG_ATTR_PALETTEMODE, 1);
    CpuCopy32(sPokedexAreaMap_Pal, &gPlttBufferUnfaded[0x70], 0x60);
    *sPokedexAreaMapBgNum = spriteTemplate->bg;
}

bool32 sub_81C4E90(void)
{
    if (!FreeTempTileDataBuffersIfPossible())
    {
        ShowBg(*sPokedexAreaMapBgNum);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void FreePokedexAreaMapBgNum(void)
{
    if (sPokedexAreaMapBgNum != NULL)
        FREE_AND_SET_NULL(sPokedexAreaMapBgNum);
}

void PokedexAreaMapChangeBgY(u32 a0)
{
    ChangeBgY(*sPokedexAreaMapBgNum, a0 * 0x100, 0);
}
