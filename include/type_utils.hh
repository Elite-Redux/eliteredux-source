extern "C" {
    #include "global.h"
    #include "abilities.hh"
    #include "battle_util.h"
    #include "pokemon.h"
    #include "constants/battle.h"
}

#ifndef max
inline int max(int a, int b) { return a > b ? a : b; }
#endif

#ifndef min
inline int min(int a, int b) { return a < b ? a : b; }
#endif

class __EnumHack {
   public:
    operator int() const { return 0; }
    operator u32() const { return 0; }
    operator AccuracyPriority() const { return ACCURACY_NO_RESULT; }
    operator MultihitType() const { return MULTIHIT_SINGLE; }
    operator StatDropBlockType() const { return STAT_DROP_BLOCK_NONE; }
    operator InfiltrateType() const { return INFILTRATE_NONE; }
    operator MoveTarget() const { return MOVE_TARGET_SELECTED; }
};

#define ENUM_OR(enumType) \
    inline constexpr enumType operator|(enumType a, enumType b) { return static_cast<enumType>(static_cast<u32>(a) | static_cast<u32>(b)); }

#define ENUM_BIT_OPERATIONS(enumType)                                                                                                        \
    ENUM_OR(enumType)                                                                                                                        \
    inline constexpr enumType& operator|=(enumType& a, const enumType b) {                                                                   \
        a = a | b;                                                                                                                           \
        return a;                                                                                                                            \
    }                                                                                                                                        \
    inline constexpr enumType operator&(enumType a, enumType b) { return static_cast<enumType>(static_cast<u32>(a) & static_cast<u32>(b)); } \
    inline constexpr enumType& operator&=(enumType& a, enumType b) {                                                                         \
        a = a & b;                                                                                                                           \
        return a;                                                                                                                            \
    }                                                                                                                                        \
    inline constexpr enumType operator~(enumType a) { return static_cast<enumType>(~static_cast<u32>(a)); }

#define ENUM_ADD(enumType)                                  \
    inline constexpr enumType& operator++(enumType& a) {    \
        a = static_cast<enumType>(static_cast<int>(a) + 1); \
        return a;                                           \
    }

STATIC_ASSERT(sizeof(Status1) == sizeof(u32), BadStatus1Size)
STATIC_ASSERT(sizeof(Status2) == sizeof(u32), BadStatus2Size)

ENUM_OR(InfiltrateType)
ENUM_OR(MoveEffectEnum)
ENUM_OR(TerrainType)
ENUM_OR(NonStackingState)
ENUM_OR(FollowupType)

ENUM_BIT_OPERATIONS(Status1)
ENUM_BIT_OPERATIONS(Status2)
ENUM_BIT_OPERATIONS(Status3)
ENUM_BIT_OPERATIONS(Status4)
ENUM_BIT_OPERATIONS(WeatherFlag)

ENUM_ADD(Type)

#define CHECK(effect) \
    if (!(effect)) return __EnumHack();
#define CHECK_NOT(effect) \
    if (effect) return __EnumHack();

#define __COMBINE(val1, val2) val1##val2
#define COMBINE(val1, val2) __COMBINE(val1, val2)

#define opt [[maybe_unused]]
