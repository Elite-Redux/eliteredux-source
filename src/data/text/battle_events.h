#include "constants/battle_events.h"

const u8 gBattleEventNames[BATTLE_EVENT_MAX_NUMBER + 1][20] = {
    [BATTLE_EVENT_NONE] = _("huh?"),
    [BATTLE_EVENT_STEADY_OFFENSE] = _("Steady Offense"),
    [BATTLE_EVENT_STEADY_DEFENSE] = _("Steady Defense"),
    [BATTLE_EVENT_STEADY_SPECIAL] = _("Steady Special"),
    [BATTLE_EVENT_STEADY_SPDEF] = _("Steady SpeDef"),
    [BATTLE_EVENT_STEADY_SPEED] = _("Steady Speed"),
    [BATTLE_EVENT_STEADY_ACCURACY] = _("Steady Acc"),
    [BATTLE_EVENT_STEADY_CRIT] = _("Steady Crit"),
    [BATTLE_EVENT_POSTURE_OFFENSE] = _("Posture Offense"),  
    [BATTLE_EVENT_POSTURE_DEFENSE] = _("Posture Defense"),
    [BATTLE_EVENT_POSTURE_SPECIAL] = _("Posture Special"),
    [BATTLE_EVENT_POSTURE_SPDEF] = _("Posture  SpeDef"),
    [BATTLE_EVENT_POSTURE_SPEED] = _("Posture Speed"),
    [BATTLE_EVENT_POSTURE_ACCURACY] = _("Posture Accuracy"),
    [BATTLE_EVENT_POSTURE_CRIT] = _("Posture Crit"),

    [BATTLE_EVENT_LAST_PARALYZED] = _("Last Paralyzed"),
    [BATTLE_EVENT_LAST_BURNED] = _("Last Burned"),
    [BATTLE_EVENT_LAST_SLEEP] = _("Last Sleep"),
    [BATTLE_EVENT_LAST_FROSTBITE] = _("Last Frostbite"),
    [BATTLE_EVENT_LAST_BLEED] = _("Last Bleed"),
    [BATTLE_EVENT_LAST_POISONED] = _("Last Poisoned"),
    [BATTLE_EVENT_LAST_TOXIC] = _("Last Toxic"),


    [BATTLE_EVENT_ONKILLNS_CURSE] = _("Curse"),
    [BATTLE_EVENT_ONKILLNS_LEECHSEED] = _("Leech Seed"),

    [BATTLE_EVENT_ELECTRIC_SURGE] = _(""),
    [BATTLE_EVENT_PSYCHIC_SURGE] = _(""),
    [BATTLE_EVENT_GRASSY_SURGE] = _(""),
    [BATTLE_EVENT_MISTY_SURGE] = _(""),

    [BATTLE_EVENT_RAIN] = _(""),
    [BATTLE_EVENT_SUN] = _(""),

    [BATTLE_EVENT_LAST_STAND] = _("Last Stand"),
    [BATTLE_EVENT_SUBSTITUTE] = _("Substitute"),

    [BATTLE_EVENT_ONSTAY_LEECH_SEED] = _("Leech Seed"),
    [BATTLE_EVENT_ONSTAY_FORESIGHT] = _("Foresight"),
    [BATTLE_EVENT_ONSTAY_MAGNET_RISE] = _("Magnet Rise"),

    [BATTLE_EVENT_ONSWITCH_MAT_BLOCK] = _("Mat Block"),

    [BATTLE_EVENT_PERMA_HEAL_BLOCK] = _("Heal Block"),
    [BATTLE_EVENT_PERMA_NIGHTMARE] = _("Nightmare"),
    [BATTLE_EVENT_PERMA_SMACKDOWN] = _("Smack Down"), 
    [BATTLE_EVENT_PERMA_WIDE_GUARD] = _("Wide Guard"),

    [BATTLE_EVENT_ONDS_COPY_STATS] = _("Copy Stat"),

    [BATTLE_EVENT_STEALTH_ROCK] = _("Stealth Rocks"),
    [BATTLE_EVENT_TOXIC_SPIKES] = _("Toxic Spikes"),
    [BATTLE_EVENT_SPIKES] = _("Spikes"),
    [BATTLE_EVENT_STICKY_WEB] = _("Sticky Web"),
    [BATTLE_EVENT_EMBARGO] = _("Embargo"),
    [BATTLE_EVENT_TAILWIND] = _("Tailwind"),
    [BATTLE_EVENT_REFLECT] = _("Reflect"),
    [BATTLE_EVENT_LIGHTSCREEN] = _("Lightscreen"),
    [BATTLE_EVENT_AURORAVEIL] = _("Aurora Veil"),
    [BATTLE_EVENT_LUCKY_CHANT] = _("Lucky Chant"),
    [BATTLE_EVENT_QUICK_GUARD] = _("Quick Guard"),

    [BATTLE_EVENT_NO_PROTECT] = _("No Protect"),

    [BATTLE_EVENT_TENSE_BATTLE] = _("Tense Battle"),
};