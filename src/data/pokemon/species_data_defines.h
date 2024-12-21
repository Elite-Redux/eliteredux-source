#pragma once
#include "constants/moves.h"
#include "tmhm_struct.h"

#define SPECIES_DATA_STRUCT(species) DataSpecies##species
#define SPECIES_ENTRY(species) [species] = DataSpecies##species

#define DUPLICATE_ICON(species) .icon = DataSpecies##species.icon, .iconPalette = DataSpecies##species.iconPalette
#define DUPLICATE_IMAGE(species)                                                                                                                 \
    .frontPic = DataSpecies##species.frontPic, .frontAnimId = DataSpecies##species.frontAnimId, .frontCoords = DataSpecies##species.frontCoords, \
    .backPic = DataSpecies##species.backPic, .backAnimId = DataSpecies##species.backAnimId, .backCoords = DataSpecies##species.backCoords,       \
    .palette = DataSpecies##species.palette, .shinyPalette = DataSpecies##species.shinyPalette, .frontAnim = DataSpecies##species.frontAnim

#define TUTOR_MOVE(tutor) .TUTOR_BIT_FIELD(tutor) = TRUE

#define TUTOR_LEARNSET_START               \
    {                                      \
        .fields = {                        \
            TUTOR_MOVE(MOVE_ENDURE),       \
            TUTOR_MOVE(MOVE_HELPING_HAND), \
            TUTOR_MOVE(MOVE_PROTECT),      \
            TUTOR_MOVE(MOVE_REST),         \
            TUTOR_MOVE(MOVE_SLEEP_TALK),   \
            TUTOR_MOVE(MOVE_SUBSTITUTE)
#define TUTOR_LEARNSET_END \
    }                      \
    }
#define LEVEL_UP_MOVE(lvl, moveLearned) {.move = moveLearned, .level = lvl}

#define LEVEL_UP_LIST (const LevelUpMove[]) {
#define LEVEL_UP_END \
    {0},             \
    }
#define EVO_LIST(vals...) \
    (const Evolution[]) { \
        vals, { 0 }       \
    }
#define U16_LIST(vals...) \
    (const u16[]) { vals, 0 }
#define ANIM_LIST(vals...) \
    (const AnimCmd* const[]) { vals }
#define ANIM_PART(vals...) \
    (const AnimCmd[]) { vals }

extern const union AnimCmd sAnim_GeneralFrame0[];

#define ANIM_LIST_GENERIC ANIM_LIST(sAnim_GeneralFrame0, ANIM_PART(ANIMCMD_FRAME(0, 1), ANIMCMD_END, ))

#define UNIVERSAL_TUTORS_GENDERLESS_NO_ATTACKS

#define UNIVERSAL_TUTORS_NO_ATTACKS TUTOR_MOVE(MOVE_ATTRACT)

#define UNIVERSAL_TUTORS_GENDERLESS TUTOR_MOVE(MOVE_RETURN), TUTOR_MOVE(MOVE_HIDDEN_POWER), TUTOR_MOVE(MOVE_SECRET_POWER)

#define UNIVERSAL_TUTORS UNIVERSAL_TUTORS_GENDERLESS, TUTOR_MOVE(MOVE_ATTRACT)

#define INCBIN_PTR_U8(val) (const u8[]) INCBIN##_U8(val)
#define INCBIN_PTR_U32(val) (const u32[]) INCBIN##_U32(val)

#define SPECIES_SPRITE(species, sprite) {(sprite), MON_PIC_SIZE, species}
#define SPECIES_PAL(species, pal) {(pal), species}
#define SPECIES_SHINY_PAL(species, pal) {(pal), species + SPECIES_SHINY_TAG}

#define PERCENT_FEMALE(percent) min(254, ((percent * 255) / 100))
