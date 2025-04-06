#ifndef GUARD_POKEMON_ICON_H
#define GUARD_POKEMON_ICON_H

extern const u8 gMonIconPaletteIndices[];
extern const u8 gMonIconPaletteIndicesFemale[];

const u8 *GetMonIconTiles(SpeciesEnum species, u32 personality);
void sub_80D304C(u16 offset);
u8 GetValidMonIconPalIndex(SpeciesEnum species);
const u8 *GetMonIconPtr(SpeciesEnum speciesId, u32 personality);
const u16 *GetValidMonIconPalettePtr(SpeciesEnum speciesId);
u16 GetIconSpecies(SpeciesEnum species, u32 personality);
u16 GetUnownLetterByPersonality(u32 personality);
u16 GetIconSpeciesNoPersonality(SpeciesEnum speciesId);
void LoadMonIconPalettes(void);
void LoadMonIconPalettesTinted(void);
void LoadMonIconPalette(SpeciesEnum species);
void FreeMonIconPalettes(void);
u8 CreateMonIconNoPersonality(SpeciesEnum species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority);
void FreeMonIconPalette(SpeciesEnum species);
void FreeAndDestroyMonIconSprite(struct Sprite *sprite);
u8 CreateMonIcon(SpeciesEnum species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority, u32 personality);
u8 UpdateMonIconFrame(struct Sprite *sprite);
void LoadMonIconPalette(SpeciesEnum species);
void LoadGenderedMonIconPalette(SpeciesEnum species, u32 personality);
void sub_80D328C(struct Sprite *sprite);
void SpriteCB_MonIcon(struct Sprite *sprite);
void SetPartyHPBarSprite(struct Sprite *sprite, u8 animNum);
u8 GetMonIconPaletteIndexFromSpecies(SpeciesEnum species);
void SafeFreeMonIconPalette(SpeciesEnum species);

#endif // GUARD_POKEMON_ICON_H
