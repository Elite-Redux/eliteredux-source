#ifndef GUARD_CONSTANTS_BATTLE_EVENTS_H
#define GUARD_CONSTANTS_BATTLE_EVENTS_H

#define BATTLE_EVENT_NONE               0
#define BATTLE_EVENT_STEADY_OFFENSE     1  //Boost Atk Stat by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_STEADY_DEFENSE     2  //Boost Defense Stat by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_STEADY_SPECIAL     3  //Boost Special Attack Stat by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_STEADY_SPDEF       4  //Boost Special Defense Stat by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_STEADY_SPEED       5  //Boost Speed Stat by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_STEADY_ACCURACY    6  //Boost Accuracy by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_STEADY_CRIT        7  //Boost Crit rate by data0 to the lead + stat boost each turn
#define BATTLE_EVENT_POSTURE_OFFENSE    8  
#define BATTLE_EVENT_POSTURE_DEFENSE    9
#define BATTLE_EVENT_POSTURE_SPECIAL    10
#define BATTLE_EVENT_POSTURE_SPDEF      11
#define BATTLE_EVENT_POSTURE_SPEED      12
#define BATTLE_EVENT_POSTURE_ACCURACY   13
#define BATTLE_EVENT_POSTURE_CRIT       14

#define BATTLE_EVENT_LAST_PARALYZED     30
#define BATTLE_EVENT_LAST_BURNED        31
#define BATTLE_EVENT_LAST_SLEEP         32
#define BATTLE_EVENT_LAST_FROSTBITE     33
#define BATTLE_EVENT_LAST_BLEED         34
#define BATTLE_EVENT_LAST_POISONED      35
#define BATTLE_EVENT_LAST_TOXIC         36

// ON KILLL NO SWITCH OUT after that.
#define BATTLE_EVENT_ONKILLNS_CURSE     50
#define BATTLE_EVENT_ONKILLNS_LEECHSEED 51

#define BATTLE_EVENT_ELECTRIC_SURGE     70
#define BATTLE_EVENT_PSYCHIC_SURGE      71
#define BATTLE_EVENT_GRASSY_SURGE       72
#define BATTLE_EVENT_MISTY_SURGE        73

#define BATTLE_EVENT_RAIN               80
#define BATTLE_EVENT_SUN                81

#define BATTLE_EVENT_LAST_STAND         100
#define BATTLE_EVENT_SUBSTITUTE         101

#define BATTLE_EVENT_ONSTAY_LEECH_SEED  120
#define BATTLE_EVENT_ONSTAY_FORESIGHT   121
#define BATTLE_EVENT_ONSTAY_MAGNET_RISE 122

#define BATTLE_EVENT_ONSWITCH_MAT_BLOCK 140

#define BATTLE_EVENT_PERMA_HEAL_BLOCK   160
#define BATTLE_EVENT_PERMA_NIGHTMARE    161
#define BATTLE_EVENT_PERMA_SMACKDOWN    162 // made specifically with winona in mind but I changed my mind it's a bad idea
#define BATTLE_EVENT_PERMA_WIDE_GUARD   163
#define BATTLE_EVENT_PERMA_STICKY_WEB   164

#define BATTLE_EVENT_ONDS_COPY_STATS    180 //DS => Death Switch (in)

#define BATTLE_EVENT_STEALTH_ROCK       200
#define BATTLE_EVENT_TOXIC_SPIKES       201
#define BATTLE_EVENT_SPIKES             202
#define BATTLE_EVENT_STICKY_WEB         203
#define BATTLE_EVENT_EMBARGO            204
#define BATTLE_EVENT_TAILWIND           205
#define BATTLE_EVENT_REFLECT            206
#define BATTLE_EVENT_LIGHTSCREEN        207
#define BATTLE_EVENT_AURORAVEIL         208
#define BATTLE_EVENT_LUCKY_CHANT        209
#define BATTLE_EVENT_QUICK_GUARD        210

#define BATTLE_EVENT_NO_PROTECT         230

//New
#define BATTLE_EVENT_EVIOLITE           231
#define BATTLE_EVENT_EXTRA_ABILITIES_1  232
#define BATTLE_EVENT_EXTRA_ABILITIES_2  233
#define BATTLE_EVENT_EXTRA_ABILITIES_3  234
#define BATTLE_EVENT_LAST_STAND_STAGES  235 //Similar to BATTLE_EVENT_LAST_STAND but modifies the stat stages rather than the stats directly

#define BATTLE_EVENT_TENSE_BATTLE       255
#define BATTLE_EVENT_MAX_NUMBER         255
#endif // GUARD_CONSTANTS_BATTLE_EVENTS_H
