#define EGGMOVES_SPECIES_OFFSET 20000
#define EGG_MOVES_TERMINATOR 0xFFFF
#define egg_moves(species, moves...) (SPECIES##species + EGG_MOVES_SPECIES_OFFSET), moves

const u16 gEggMoves[] = {

    EGG_MOVES_TERMINATOR
};