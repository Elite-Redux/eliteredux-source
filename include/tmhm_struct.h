#ifndef GUARD_TMHM_STRUCT_H
#define GUARD_TMHM_STRUCT_H

#include "global.h"
#include "generated/constants/moves.h"

#define ALL_TMS \
TM_DECORATOR(MOVE_PROTECT)

#define TM_ENUM(tm) TM_ENUM_##tm
#define TM_DECORATOR(tm) TM_ENUM(tm),

typedef enum
{
    ALL_TMS
    TM_COUNT
} TmHmEnum;

#undef TM_DECORATOR

#define TM_BIT_FIELD(tm) TM_FIELD_##tm
#define TM_DECORATOR(tm) bool8 TM_BIT_FIELD(tm):1;

struct TmHmStruct
{
    ALL_TMS
};

union TmHmUnion
{
    u32 bits[(TM_COUNT + 31) / 32];
    struct TmHmStruct fields;
};

#undef TM_DECORATOR

extern const u16 gTmMoveMapping[];

#include "generated/data/all_tutors.h"

#define TUTOR_ENUM(tutor) TUTOR_ENUM_##tutor
#define TUTOR_DECORATOR(tutor) TUTOR_ENUM(tutor),

typedef enum
{
    ALL_TUTORS
    TUTOR_COUNT
} TutorEnum;

#undef TUTOR_DECORATOR

#define TUTOR_BIT_FIELD(tutor) TUTOR_FIELD_##tutor
#define TUTOR_DECORATOR(tutor) bool8 TUTOR_BIT_FIELD(tutor):1;

struct TutorStruct
{
    ALL_TUTORS
};

typedef union TutorUnion
{
    u16 bits[(TUTOR_COUNT + 15) / 16];
    struct TutorStruct fields;
} TutorUnion;

#undef TUTOR_DECORATOR

extern const u16 gTutorMoveMapping[];

u16 GetTmMove(u8 tmId);
u16 GetTutorMove(u8 tutorId);

#endif