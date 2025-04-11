#include "global.h"
#include "graphics.h"
#include "palette.h"
#include "pokemon_icon.h"
#include "sprite.h"
#include "data.h"
#include "mgba_printf/mgba.h"

#define POKE_ICON_BASE_PAL_TAG 56000

#define INVALID_ICON_SPECIES SPECIES_NONE  // Oddly specific, used when an icon should be a ?. Any of the 'old unown' would work

struct MonIconSpriteTemplate {
    const struct OamData *oam;
    const u8 *image;
    const union AnimCmd *const *anims;
    const union AffineAnimCmd *const *affineAnims;
    void (*callback)(struct Sprite *);
    u16 paletteTag;
};

// static functions
static u8 CreateMonIconSprite(struct MonIconSpriteTemplate *, s16, s16, u8);

// .rodata
#include "generated/data/pokemon_graphics/icons.h"

const struct SpritePalette gMonIconPaletteTable[] = {
    {gMonIconPalettes[0], POKE_ICON_BASE_PAL_TAG + 0},
    {gMonIconPalettes[1], POKE_ICON_BASE_PAL_TAG + 1},
    {gMonIconPalettes[2], POKE_ICON_BASE_PAL_TAG + 2},
    {gMonIconPalettes[3], POKE_ICON_BASE_PAL_TAG + 3},
    {gMonIconPalettes[4], POKE_ICON_BASE_PAL_TAG + 4},
    {gMonIconPalettes[5], POKE_ICON_BASE_PAL_TAG + 5},
    {gMonIconPalettes[6], POKE_ICON_BASE_PAL_TAG + 6},
};

const struct OamData sMonIconOamData = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

// fastest to slowest

static const union AnimCmd sAnim_0[] = {
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_FRAME(1, 6),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_1[] = {
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(1, 8),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_2[] = {
    ANIMCMD_FRAME(0, 14),
    ANIMCMD_FRAME(1, 14),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_3[] = {
    ANIMCMD_FRAME(0, 22),
    ANIMCMD_FRAME(1, 22),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_4[] = {
    ANIMCMD_FRAME(0, 29),
    ANIMCMD_FRAME(0, 29),  // frame 0 is repeated
    ANIMCMD_JUMP(0),
};

const union AnimCmd *const sMonIconAnims[] = {
    sAnim_0,
    sAnim_1,
    sAnim_2,
    sAnim_3,
    sAnim_4,
};

static const union AffineAnimCmd sAffineAnim_0[] = {
    AFFINEANIMCMD_FRAME(0, 0, 0, 10),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_1[] = {
    AFFINEANIMCMD_FRAME(-2, -2, 0, 122),
    AFFINEANIMCMD_END,
};

const union AffineAnimCmd *const sMonIconAffineAnims[] = {
    sAffineAnim_0,
    sAffineAnim_1,
};

const u16 sSpriteImageSizes[3][4] = {
    [ST_OAM_SQUARE] =
        {
            [SPRITE_SIZE(8x8)] = 0x20,
            [SPRITE_SIZE(16x16)] = 0x80,
            [SPRITE_SIZE(32x32)] = 0x200,
            [SPRITE_SIZE(64x64)] = 0x800,
        },
    [ST_OAM_H_RECTANGLE] =
        {
            [SPRITE_SIZE(16x8)] = 0x40,
            [SPRITE_SIZE(32x8)] = 0x80,
            [SPRITE_SIZE(32x16)] = 0x100,
            [SPRITE_SIZE(64x32)] = 0x400,
        },
    [ST_OAM_V_RECTANGLE] =
        {
            [SPRITE_SIZE(8x16)] = 0x40,
            [SPRITE_SIZE(8x32)] = 0x80,
            [SPRITE_SIZE(16x32)] = 0x100,
            [SPRITE_SIZE(32x64)] = 0x400,
        },
};

u8 CreateMonIcon(SpeciesEnum species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority, u32 personality) {
    u8 spriteId;
    struct MonIconSpriteTemplate iconTemplate = {
        .oam = &sMonIconOamData,
        .image = GetMonIconPtr(species, personality),
        .anims = sMonIconAnims,
        .affineAnims = sMonIconAffineAnims,
        .callback = callback,
        .paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species],
    };

    if (species > NUM_SPECIES)
        iconTemplate.paletteTag = POKE_ICON_BASE_PAL_TAG;
    else if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
        iconTemplate.paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndicesFemale[species];

    spriteId = CreateMonIconSprite(&iconTemplate, x, y, subpriority);

    UpdateMonIconFrame(&gSprites[spriteId]);

    return spriteId;
}

u8 CreateMonIconNoPersonality(SpeciesEnum species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority) {
    u8 spriteId;
    struct MonIconSpriteTemplate iconTemplate = {
        .oam = &sMonIconOamData,
        .image = NULL,
        .anims = sMonIconAnims,
        .affineAnims = sMonIconAffineAnims,
        .callback = callback,
        .paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species],
    };

    iconTemplate.image = GetMonIconTiles(species, 0);
    spriteId = CreateMonIconSprite(&iconTemplate, x, y, subpriority);

    UpdateMonIconFrame(&gSprites[spriteId]);

    return spriteId;
}

u16 GetIconSpecies(SpeciesEnum species, u32 personality) {
    u16 result;

    if (species == SPECIES_UNOWN) {
        result = GetUnownSpeciesId(personality);
    } else {
        if (species > NUM_SPECIES)
            result = INVALID_ICON_SPECIES;
        else
            result = species;
    }

    return result;
}

u16 GetUnownLetterByPersonality(u32 personality) {
    if (!personality)
        return 0;
    else
        return GET_UNOWN_LETTER(personality);
}

u16 GetIconSpeciesNoPersonality(SpeciesEnum species) {
    if (species > NUM_SPECIES) species = INVALID_ICON_SPECIES;

    return GetIconSpecies(species, 0);
}

const u8 *GetMonIconPtr(SpeciesEnum species, u32 personality) { return GetMonIconTiles(GetIconSpecies(species, personality), personality); }

void FreeAndDestroyMonIconSprite(struct Sprite *sprite) { sub_80D328C(sprite); }

void LoadMonIconPalettes(void) {
    u8 i;
    for (i = 0; i < ARRAY_COUNT(gMonIconPaletteTable); i++) LoadSpritePalette(&gMonIconPaletteTable[i]);
}

void LoadMonIconPalettesTinted(void) {
    u8 i;
    for (i = 0; i < ARRAY_COUNT(gMonIconPaletteTable); i++) {
        LoadSpritePalette(&gMonIconPaletteTable[ARRAY_COUNT(gMonIconPaletteTable)]);
        // TintPalette_GrayScale2(&gPlttBufferUnfaded[0x170 + i*16], 16);
    }
}

// unused
void SafeLoadMonIconPalette(SpeciesEnum species) {
    u8 palIndex;
    if (species > NUM_SPECIES) species = INVALID_ICON_SPECIES;
    palIndex = gMonIconPaletteIndices[species];
    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF) LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
}

void LoadMonIconPalette(SpeciesEnum species) {
    u8 palIndex = gMonIconPaletteIndices[species];
    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF) LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
}

void LoadGenderedMonIconPalette(SpeciesEnum species, u32 personality) {
    u8 palIndex;

    if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE)
        palIndex = gMonIconPaletteIndicesFemale[species];
    else
        palIndex = gMonIconPaletteIndices[species];

    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF) {
        LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
    }
}

void FreeMonIconPalettes(void) {
    u8 i;
    for (i = 0; i < ARRAY_COUNT(gMonIconPaletteTable); i++) FreeSpritePaletteByTag(gMonIconPaletteTable[i].tag);
}

// unused
void SafeFreeMonIconPalette(SpeciesEnum species) {
    u8 palIndex;
    if (species > NUM_SPECIES) species = INVALID_ICON_SPECIES;
    palIndex = gMonIconPaletteIndices[species];
    FreeSpritePaletteByTag(gMonIconPaletteTable[palIndex].tag);
}

void FreeMonIconPalette(SpeciesEnum species) {
    u8 palIndex;
    palIndex = gMonIconPaletteIndices[species];
    FreeSpritePaletteByTag(gMonIconPaletteTable[palIndex].tag);
}

void SpriteCB_MonIcon(struct Sprite *sprite) { UpdateMonIconFrame(sprite); }

const u8 *GetMonIconTiles(SpeciesEnum species, u32 personality) {
    const u8 *iconSprite = gMonIconTable[species];
    if (SpeciesHasGenderDifference[species] && GetGenderFromSpeciesAndPersonality(species, personality) == MON_FEMALE) {
        iconSprite = gMonIconTableFemale[species];
    }
    return iconSprite;
}

void sub_80D304C(u16 offset) {
    s32 i;
    const struct SpritePalette *monIconPalettePtr;

    if (offset <= 0xA0) {
        monIconPalettePtr = gMonIconPaletteTable;
        for (i = 5; i >= 0; i--) {
            LoadPalette(monIconPalettePtr->data, offset, 0x20);
            offset += 0x10;
            monIconPalettePtr++;
        }
    }
}

u8 GetValidMonIconPalIndex(SpeciesEnum species) {
    if (species > NUM_SPECIES) species = INVALID_ICON_SPECIES;
    return gMonIconPaletteIndices[species];
}

u8 GetMonIconPaletteIndexFromSpecies(SpeciesEnum species) { return gMonIconPaletteIndices[species]; }

const u16 *GetValidMonIconPalettePtr(SpeciesEnum species) {
    if (species > NUM_SPECIES) species = INVALID_ICON_SPECIES;
    return gMonIconPaletteTable[gMonIconPaletteIndices[species]].data;
}

u8 UpdateMonIconFrame(struct Sprite *sprite) {
    u8 result = 0;

    if (sprite->animDelayCounter == 0) {
        s16 frame = sprite->anims[sprite->animNum][sprite->animCmdIndex].frame.imageValue;

        switch (frame) {
            case -1:
                break;
            case -2:
                sprite->animCmdIndex = 0;
                break;
            default:
                RequestSpriteCopy(
                    // pointer arithmetic is needed to get the correct pointer to perform the sprite copy on.
                    // because sprite->images is a struct def, it has to be casted to (u8 *) before any
                    // arithmetic can be performed.
                    (u8 *)sprite->images + (sSpriteImageSizes[sprite->oam.shape][sprite->oam.size] * frame),
                    (u8 *)(OBJ_VRAM0 + sprite->oam.tileNum * TILE_SIZE_4BPP),
                    sSpriteImageSizes[sprite->oam.shape][sprite->oam.size]);
                sprite->animDelayCounter = sprite->anims[sprite->animNum][sprite->animCmdIndex].frame.duration & 0xFF;
                sprite->animCmdIndex++;
                result = sprite->animCmdIndex;
                break;
        }
    } else {
        sprite->animDelayCounter--;
    }
    return result;
}

static u8 CreateMonIconSprite(struct MonIconSpriteTemplate *iconTemplate, s16 x, s16 y, u8 subpriority) {
    u8 spriteId;

    struct SpriteFrameImage image = {NULL, sSpriteImageSizes[iconTemplate->oam->shape][iconTemplate->oam->size]};

    struct SpriteTemplate spriteTemplate = {
        .tileTag = 0xFFFF,
        .paletteTag = iconTemplate->paletteTag,
        .oam = iconTemplate->oam,
        .anims = iconTemplate->anims,
        .images = &image,
        .affineAnims = iconTemplate->affineAnims,
        .callback = iconTemplate->callback,
    };

    spriteId = CreateSprite(&spriteTemplate, x, y, subpriority);
    gSprites[spriteId].animPaused = TRUE;
    gSprites[spriteId].animBeginning = FALSE;
    gSprites[spriteId].images = (const struct SpriteFrameImage *)iconTemplate->image;
    return spriteId;
}

void sub_80D328C(struct Sprite *sprite) {
    struct SpriteFrameImage image = {NULL, sSpriteImageSizes[sprite->oam.shape][sprite->oam.size]};
    sprite->images = &image;
    DestroySprite(sprite);
}

void SetPartyHPBarSprite(struct Sprite *sprite, u8 animNum) {
    sprite->animNum = animNum;
    sprite->animDelayCounter = 0;
    sprite->animCmdIndex = 0;
}