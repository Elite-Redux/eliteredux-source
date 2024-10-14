#define TMHM_LEARNSET { .fields = {
#define TMHM_LEARNSET_END }},
#define TM(tm) .TM_BIT_FIELD(tm) = TRUE,

const union TmHmUnion gTMHMLearnsets[NUM_SPECIES] =
{
    [SPECIES_BULBASAUR] = TMHM_LEARNSET
        TM(MOVE_PROTECT)
        TMHM_LEARNSET_END
};

