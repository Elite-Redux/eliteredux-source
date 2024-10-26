#ifndef GUARD_BATTLE_AI_SCORING_H
#define GUARD_BATTLE_AI_SCORING_H

#define AI_SCORE_POISON_MOVE(battler) 0
#define AI_SCORE_SLEEP_MOVE(battler) 0
#define AI_SCORE_ABSORB_MOVE(percent) 0
#define AI_SCORE_BURN_MOVE(battler) 0
#define AI_SCORE_FROSTBITE_MOVE(battler) 0
#define AI_SCORE_PARALYSIS(battler) 0
#define AI_SCORE_ATTACK_UP(battler, stages) 0
#define AI_SCORE_DEFENSE_UP(battler, stages) 0
#define AI_SCORE_SPEED_UP(battler, stages) 0
#define AI_SCORE_SPATK_UP(battler, stages) 0
#define AI_SCORE_SPDEF_UP(battler, stages) 0
#define AI_SCORE_ACC_UP(battler, stages) 0
#define AI_SCORE_EVASION_UP(battler, stages) 0
#define AI_SCORE_CRIT_UP(battler, stages) 0
#define AI_SCORE_STAT(battler, stat, stage) 0
#define AI_SCORE_RESET_STAT_CHANGES(battler) 0
#define AI_SCORE_APE_SHIFT(battler) 0
#define AI_SCORE_MAKE_IT_RAIN 0
#define AI_SCORE_RAINBOW 0
#define AI_SCORE_SWAMP 0
#define AI_SCORE_FIRE_SEA 0
#define AI_SCORE_RANDOM_SWITCH(battlerDef) 0
#define AI_SCORE_FLINCH(battler) 0
#define AI_SCORE_HEAL(battler, percent) 0
#define AI_SCORE_TOXIC(battler) 0
#define AI_SCORE_LIGHTSCREEN 0
#define AI_SCORE_CURE_STATUS(battler) 0
#define AI_SCORE_WRAP(battlerAtk, battlerDef) 0
#define AI_SCORE_HEAL_BLOCK(duration) 0
#define AI_SCORE_MIST 0
#define AI_SCORE_RECOIL(battler, percent, noMitigation) 0
#define AI_SCORE_SET_TYPE(battler, type) 0
#define AI_SCORE_CONFUSION(battler) 0
#define AI_SCORE_TRANSFORM 0
#define AI_SCORE_REFLECT 0
#define AI_SCORE_SUBSTITUTE 0
#define AI_SCORE_LOSE_HP(battlerDef, percent) 0
#define AI_SCORE_LEECH_SEED 0
#define AI_SCORE_DISABLE(battlerDef) 0
#define AI_SCORE_COUNTER 0
#define AI_SCORE_ENCORE 0
#define AI_SCORE_PAIN_SPLIT 0
#define AI_SCORE_LOCK_ON 0
#define AI_SCORE_DESTINY_BOND 0
#define AI_SCORE_PP_DOWN(battlerDef, amount) 0
#define AI_SCORE_INNARDS_OUT(battlerAtk, battlerDef) 0
#define AI_SCORE_CURE_PARTY_STATUS(battlerAtk) 0
#define AI_SCORE_THIEF 0
#define AI_SCORE_TRAP(battler) 0
#define AI_SCORE_NIGHTMARE 0
#define AI_SCORE_CURSE(battlerDef) 0
#define AI_SCORE_ADD_TYPE(battler, type) 0
#define AI_SCORE_PROTECT 0
#define AI_SCORE_SPIKES(battler) 0
#define AI_SCORE_PERISH_SONG(battlerDef) 0
#define AI_SCORE_SANDSTORM 0
#define AI_SCORE_ENDURE 0
#define AI_SCORE_ATTRACT(battlerAtk, battlerDef) 0
#define AI_SCORE_SAFEGUARD 0
#define AI_SCORE_SWITCH(battlerAtk) 0
#define AI_SCORE_REVIVE(battler, percent) 0
#define AI_SCORE_CLEAR_HAZARDS(side) 0
#define AI_SCORE_RAIN 0
#define AI_SCORE_SUN 0
#define AI_SCORE_HAIL 0
#define AI_SCORE_PRIMAL_SUN 0
#define AI_SCORE_PRIMAL_RAIN 0
#define AI_SCORE_STRONG_WINDS 0
#define AI_SCORE_GET_STATS_OF(to, from) 0
#define AI_SCORE_MIRROR_COAT 0
#define AI_SCORE_FUTURE_SIGHT 0
#define AI_SCORE_DEFENSE_CURL 0
#define AI_SCORE_REPLACE_ABILITY(battler, ability) 0
#define AI_SCORE_SUPPRESS_ABILITY 0
#define AI_SCORE_FEAR(battlerDef) 0
#define AI_SCORE_TORMENT 0
#define AI_SCORE_FOLLOW_ME 0
#define AI_SCORE_CHARGE(battlerAtk) 0
#define AI_SCORE_LUMBERING_SLOTH_ENGULFED 0
#define AI_SCORE_TAUNT 0
#define AI_SCORE_SYRUP 0
#define AI_SCORE_HELPING_HAND 0
#define AI_SCORE_GHASTLY_ECHO 0
#define AI_SCORE_LOSE_ITEM(battler) 0
#define AI_SCORE_GIVE_ITEM(battler, item) 0
#define AI_SCORE_EMBARGO(battler) 0
#define AI_SCORE_SWAP_ITEMS 0
#define AI_SCORE_QUICK_GUARD 0
#define AI_SCORE_INGRAIN 0
#define AI_SCORE_RECYCLE 0
#define AI_SCORE_BREAK_SCREENS(battlerDef) 0
#define AI_SCORE_BREAK_SUBSTITUTE 0
#define AI_SCORE_DROWSY 0
#define AI_SCORE_ENDEAVOR 0
#define AI_SCORE_IMPRISON 0
#define AI_SCORE_CURE_STATUS_AND_HEAL(battler, percent) 0
#define AI_SCORE_GRAVITY 0
#define AI_SCORE_TAILWIND 0
#define AI_SCORE_AQUA_RING 0
#define AI_SCORE_TRICK_ROOM(duration) 0
#define AI_SCORE_WONDER_ROOM 0
#define AI_SCORE_MAGIC_ROOM 0
#define AI_SCORE_MAGNET_RISE 0
#define AI_SCORE_TOXIC_SPIKES(battlerAtk) 0
#define AI_SCORE_STEALTH_ROCK(battlerAtk, type) 0
#define AI_SCORE_TELEKINESIS 0
#define AI_SCORE_STICKY_WEB 0
#define AI_SCORE_METAL_BURST 0
#define AI_SCORE_LUCKY_CHANT 0
#define AI_SCORE_CONTACT(effect) 0
#define AI_SCORE_CALTROPS 0
#define AI_SCORE_QUASH 0
#define AI_SCORE_INVERT_STAT_CHANGES 0
#define AI_SCORE_PSYCHIC_TERRAIN 0
#define AI_SCORE_MISTY_TERRAIN 0
#define AI_SCORE_EXTRA_MOVE(battlerAtk, battlerDef, move, power) 0
#define AI_SCORE_ELECTRIC_TERRAIN 0
#define AI_SCORE_GRASSY_TERRAIN 0
#define AI_SCORE_FOG 0
#define AI_SCORE_COILED_UP 0
#define AI_SCORE_ARGUMENT_MOVE_EFFECT 0
#define AI_SCORE_BLEED(battler) 0
#define AI_SCORE_AURORA_VEIL(battler, duration) 0
#define AI_SCORE_BREAK_PROTECT 0
#define AI_SCORE_GLAIVE_RUSH 0
#define AI_SCORE_POWDER 0
#define AI_SCORE_THROAT_CHOP 0
#define AI_SCORE_EAT_BERRY(battler) 0
#define AI_SCORE_HEAL_FIXED(amount) 0
#define AI_SCORE_HAZE 0
#define AI_SCORE_NO_ESCAPE(battlerAtk) 0
#define AI_SCORE_INVERSE_ROOM(duration) 0
#define AI_SCORE_REMOVE_TERRAIN 0
#define AI_SCORE_PARALYSIS_IGNORE_TYPE 0
#define AI_SCORE_REMOVE_WEATHER 0
#define AI_SCORE_SAFE_PASSAGE 0
#define AI_SCORE_SMOKESCREEN 0
#define AI_SCORE_CLEAR_SKIES 0
#define AI_SCORE_TREPIDATION 0
#define AI_SCORE_SALT_CURE 0
#define AI_SCORE_DAZE(battler) 0
#define AI_SCORE_GET_PARASITIC_SPORES(battler) 0
#define AI_SCORE_SET_ANTICIPATION 0
#define AI_SCORE_FORM_CHANGE(battler, species) 0
#define AI_SCORE_HOT_COALS 0
#define AI_SCORE_CLEAR_STAT_BUFFS(battler) 0
#define AI_SCORE_PARADOX_BOOST 0
#define AI_SCORE_VIOLENT_RUSH 0
#define AI_SCORE_RAPID_RESPONSE 0
#define AI_SCORE_READIED_ACTION 0
#define AI_SCORE_ON_THE_PROWL 0

#endif