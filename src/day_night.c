#include "global.h"
#include "day_night.h"
#include "decompress.h"
#include "event_data.h"
#include "field_tasks.h"
#include "field_weather.h"
#include "overworld.h"
#include "palette.h"
#include "rtc.h"
#include "constants/day_night.h"
#include "constants/rgb.h"
#include "strings.h"
#include "string_util.h"
#include "fieldmap.h"

#define TINT_MORNING Q_8_8(0.7), Q_8_8(0.7), Q_8_8(0.9)
#define TINT_DAY Q_8_8(1.0), Q_8_8(1.0), Q_8_8(1.0)
#define TINT_NIGHT Q_8_8(0.6), Q_8_8(0.6), Q_8_8(0.92)

EWRAM_DATA static u16 sPlttBufferPreDN[PLTT_BUFFER_SIZE] = {0};
EWRAM_DATA const struct PaletteOverride *gPaletteOverrides[4] = {NULL};

static EWRAM_DATA struct {
    bool8 initialized:1;
    bool8 retintPhase:1;
    u8 timeOfDay;
    u16 prevTintPeriod; // tint period associated with currently drawn palettes
    u16 currTintPeriod; // tint period associated with currRGBTint
    u16 currRGBTint[3];
} sDNSystemControl = {0};

static const u16 sTimeOfDayTints[][3] = {
    [0] =   {TINT_NIGHT},
    [1] =   {TINT_NIGHT},
    [2] =   {TINT_NIGHT},
    [3] =   {TINT_NIGHT},
    [4] =   {Q_8_8(0.6), Q_8_8(0.65), Q_8_8(1.0)},
    [5] =   {TINT_MORNING},
    [6] =   {TINT_MORNING},
    [7] =   {TINT_MORNING},
    [8] =   {Q_8_8(0.9), Q_8_8(0.85), Q_8_8(1.0)},
    [9] =   {Q_8_8(1.0), Q_8_8(0.9), Q_8_8(1.0)},
    [10] =  {TINT_DAY},
    [11] =  {TINT_DAY},
    [12] =  {TINT_DAY},
    [13] =  {TINT_DAY},
    [14] =  {TINT_DAY},
    [15] =  {TINT_DAY},
    [16] =  {TINT_DAY},
    [17] =  {Q_8_8(1.0), Q_8_8(0.98), Q_8_8(0.9)},
    [18] =  {Q_8_8(0.9), Q_8_8(0.7), Q_8_8(0.67)},
    [19] =  {Q_8_8(0.75), Q_8_8(0.66), Q_8_8(0.77)},
    [20] =  {Q_8_8(0.7), Q_8_8(0.63), Q_8_8(0.82)},
    [21] =  {TINT_NIGHT},
    [22] =  {TINT_NIGHT},
    [23] =  {TINT_NIGHT},
};

u8 GetCurrentTimeOfDay(void)
{
    if (gLocalTime.hours < HOUR_MORNING)
        return TIME_NIGHT;
    else if (gLocalTime.hours < HOUR_DAY)
        return TIME_MORNING;
    else if (gLocalTime.hours < HOUR_NIGHT)
        return TIME_DAY;

    return TIME_NIGHT;
}

static void LoadPaletteOverrides(void)
{
    u32 i, j;
    const u16 *src;
    u16 *dest;
    s8 hour = gLocalTime.hours;

    for (i = 0; i < ARRAY_COUNT(gPaletteOverrides); i++)
    {
        const struct PaletteOverride *curr = gPaletteOverrides[i];
        if (curr != NULL)
        {
            while (curr->slot != PALOVER_LIST_TERM && curr->palette != NULL)
            {
                if ((curr->startHour < curr->endHour && hour >= curr->startHour && hour < curr->endHour) ||
                    (curr->startHour > curr->endHour && (hour >= curr->startHour || hour < curr->endHour)))
                {
                    for (j = 0, src = curr->palette, dest = &gPlttBufferUnfaded[curr->slot * 16]; j < 16; j++, src++, dest++)
                    {
                        if (*src != RGB_BLACK)
                            *dest = *src;
                    }
                }
                curr++;
            }
        }
    }
}

static void LerpColors(u16 *rgbDest, s32 hour, s32 nextHour, u8 coeff)
{
    const u16 *rgb1, *rgb2;
    u16 rgbTemp[3];

    rgb1 = sTimeOfDayTints[hour];
    memcpy(rgbTemp, rgb1, sizeof(rgbTemp));

    rgb2 = sTimeOfDayTints[nextHour];

    if (rgb1[0] != rgb2[0] ||
        rgb1[1] != rgb2[1] ||
        rgb1[2] != rgb2[2])
    {
        rgbTemp[0] = (((rgb2[0] - rgb1[0]) * coeff) / TINT_PERIODS_PER_HOUR) + rgb1[0];
        rgbTemp[1] = (((rgb2[1] - rgb1[1]) * coeff) / TINT_PERIODS_PER_HOUR) + rgb1[1];
        rgbTemp[2] = (((rgb2[2] - rgb1[2]) * coeff) / TINT_PERIODS_PER_HOUR) + rgb1[2];
    }

    if (rgbTemp[0] != rgbDest[0] ||
        rgbTemp[1] != rgbDest[1] ||
        rgbTemp[2] != rgbDest[2])
    {
        memcpy(rgbDest, rgbTemp, sizeof(rgbTemp));
    }
}

static void TintPalette_CustomToneWithCopy(const u16 *src, u16 *dest, u16 count, u16 rTone, u16 gTone, u16 bTone, bool32 excludeZeroes)
{
    s32 r, g, b, i;

    for (i = 0; i < count; i++, src++, dest++)
    {
        if (excludeZeroes && *src == RGB_BLACK)
            continue;

        r = GET_R(*src);
        g = GET_G(*src);
        b = GET_B(*src);

        r = (u16)((rTone * r)) >> 8;
        g = (u16)((gTone * g)) >> 8;
        b = (u16)((bTone * b)) >> 8;

        if (r > 31)
            r = 31;
        if (g > 31)
            g = 31;
        if (b > 31)
            b = 31;

        *dest = RGB2(r, g, b);
    }
}

static void TintPaletteForDayNight(u16 offset, u16 size)
{
    if (IsMapTypeOutdoors(gMapHeader.mapType))
    {
        s8 hour, nextHour;
        u8 hourPhase;
        u32 period;

        RtcCalcLocalTimeFast();

        hour = gLocalTime.hours;
        hourPhase = gLocalTime.minutes / MINUTES_PER_TINT_PERIOD;

        period = (hour * TINT_PERIODS_PER_HOUR) + hourPhase;

        if (!sDNSystemControl.initialized || sDNSystemControl.currTintPeriod != period)
        {
            sDNSystemControl.initialized = TRUE;
            sDNSystemControl.currTintPeriod = period;
            nextHour = (hour + 1) % 24;
            LerpColors(sDNSystemControl.currRGBTint, hour, nextHour, hourPhase);
        }

        TintPalette_CustomToneWithCopy(&sPlttBufferPreDN[offset], &gPlttBufferUnfaded[offset], size / 2, sDNSystemControl.currRGBTint[0], sDNSystemControl.currRGBTint[1], sDNSystemControl.currRGBTint[2], FALSE);
        LoadPaletteOverrides();
    }
    else
    {
        CpuCopy16(&sPlttBufferPreDN[offset], &gPlttBufferUnfaded[offset], size);
    }
}

void LoadCompressedPaletteDayNight(const u32 *src, u16 offset, u16 size)
{
    LoadCompressedPalette_HandleDayNight(src, offset, size, TRUE);
}

void LoadPaletteDayNight(const void *src, u16 offset, u16 size)
{
    LoadPalette_HandleDayNight(src, offset, size, TRUE);
}

void CheckClockForImmediateTimeEvents(void)
{
    if (!sDNSystemControl.retintPhase && IsMapTypeOutdoors(gMapHeader.mapType))
        RtcCalcLocalTimeFast();
}

void ProcessImmediateTimeEvents(void)
{
    u32 period;

    if (IsMapTypeOutdoors(gMapHeader.mapType))
    {
        if (sDNSystemControl.retintPhase)
        {
            sDNSystemControl.retintPhase = FALSE;
            TintPalette_CustomToneWithCopy(&sPlttBufferPreDN[BG_PLTT_SIZE / 2], &gPlttBufferUnfaded[BG_PLTT_SIZE / 2], OBJ_PLTT_SIZE / 2, sDNSystemControl.currRGBTint[0], sDNSystemControl.currRGBTint[1], sDNSystemControl.currRGBTint[2], TRUE);
            LoadPaletteOverrides();

            if (gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_SCREEN_FADING_IN &&
                gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_SCREEN_FADING_OUT)
            {
                CpuCopy16(gPlttBufferUnfaded, gPlttBufferFaded, PLTT_SIZE);

                #define paletteIndex period
                //for (paletteIndex = 0; paletteIndex < NUM_PALS_TOTAL; paletteIndex++)
                //    ApplyWeatherColorMapToPal(paletteIndex);
                #undef paletteIndex
            }
        }
        else
        {
            s8 hour, nextHour;
            u8 hourPhase;

            hour = gLocalTime.hours;
            hourPhase = gLocalTime.minutes / MINUTES_PER_TINT_PERIOD;

            period = (hour * TINT_PERIODS_PER_HOUR) + hourPhase;

            if (!sDNSystemControl.initialized || sDNSystemControl.prevTintPeriod != period)
            {
                sDNSystemControl.initialized = TRUE;
                sDNSystemControl.prevTintPeriod = sDNSystemControl.currTintPeriod = period;
                nextHour = (hour + 1) % 24;
                LerpColors(sDNSystemControl.currRGBTint, hour, nextHour, hourPhase);
                TintPalette_CustomToneWithCopy(sPlttBufferPreDN, gPlttBufferUnfaded, BG_PLTT_SIZE / 2, sDNSystemControl.currRGBTint[0], sDNSystemControl.currRGBTint[1], sDNSystemControl.currRGBTint[2], TRUE);
                sDNSystemControl.retintPhase = TRUE;
            }
        }
    }

    #define currentTimeOfDay period
    currentTimeOfDay = GetCurrentTimeOfDay();
    if (sDNSystemControl.timeOfDay != currentTimeOfDay)
    {
        sDNSystemControl.timeOfDay = currentTimeOfDay;
        ForceTimeBasedEvents(); // for misc events that should run on time of day boundaries
    }
    #undef currentTimeOfDay
}

#define DISABLE_DAYNIGHT_TINT TRUE

void LoadCompressedPalette_HandleDayNight(const u32 *src, u16 offset, u16 size, bool32 isDayNight)
{
    LZ77UnCompWram(src, gPaletteDecompressionBuffer);
    if (isDayNight && !DISABLE_DAYNIGHT_TINT)
    {
        CpuCopy16(gPaletteDecompressionBuffer, &sPlttBufferPreDN[offset], size);
        TintPaletteForDayNight(offset, size);
        CpuCopy16(&gPlttBufferUnfaded[offset], &gPlttBufferFaded[offset], size);
    }
    else
    {
        CpuCopy16(gPaletteDecompressionBuffer, &gPlttBufferUnfaded[offset], size);
        CpuCopy16(gPaletteDecompressionBuffer, &gPlttBufferFaded[offset], size);
    }
}

void LoadPalette_HandleDayNight(const void *src, u16 offset, u16 size, bool32 isDayNight)
{
    if (isDayNight && !DISABLE_DAYNIGHT_TINT)
    {
        CpuCopy16(src, &sPlttBufferPreDN[offset], size);
        TintPaletteForDayNight(offset, size);
        CpuCopy16(&gPlttBufferUnfaded[offset], &gPlttBufferFaded[offset], size);
    }
    else
    {
        CpuCopy16(src, &gPlttBufferUnfaded[offset], size);
        CpuCopy16(src, &gPlttBufferFaded[offset], size);
    }
}
