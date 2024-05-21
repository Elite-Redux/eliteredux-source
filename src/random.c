#include "global.h"
#include "random.h"

EWRAM_DATA static u8 sUnknown = 0;
EWRAM_DATA static u32 sRandCount = 0;

// IWRAM common
u32 gRngValue;
u32 gRng2Value;

u16 Random(void)
{
    gRngValue = ISO_RANDOMIZE1(gRngValue);
    sRandCount++;
    return gRngValue >> 16;
}

void SeedRng(u16 seed)
{
    gRngValue = seed;
    sUnknown = 0;
}

void SeedRng2(u16 seed)
{
    gRng2Value = seed;
}

u16 Random2(void)
{
    gRng2Value = ISO_RANDOMIZE1(gRng2Value);
    return gRng2Value >> 16;
}

// NEW
u16 RandRange(u16 min, u16 max)
{    
    if (min == max)
        return min;

    max++;   // make inclusive
    return (Random() % (max - min)) + min;
}

u16 RandRangeDeterministic(u16 min, u16 max, u16 seed)
{
    if (min == max)
        return min;
    max++;   // make inclusive
    return (Random16(seed) % (max - min)) + min;
}

u16 Random16(u16 x){
    u16 seed = 31;
    x ^= seed;
    x ^= x >> 4;
    x *= 0x27d4eb2d;
    x ^= x >> 15;
    return x;
}

u16 Random16ModReduced(u16 x, u16 mod){
    u16 limit = 0xFFFF - (0xFFFF % mod);
    x = Random16(x);
    while (x >= limit){
        x = Random16(x);
    }
    return x % mod;
}