#include "global.h"
#include "constants/abilities.h"
#include "abilities.h"
#include "battle.h"
#include "battle_scripts.h"
#include "constants/battle_string_ids.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "string_util.h"
#include "constants/hold_effects.h"
#include "constants/item.h"
#include "constants/items.h"
#include "item.h"

#define NO_ANNOUNCE 2

#define CHECK(effect) if (!(effect)) return FALSE;
#define CHECK_NOT(effect) if (effect) return FALSE;

#define __COMBINE(val1, val2) val1 ## val2
#define COMBINE(val1, val2) __COMBINE(val1, val2)

#define ON_SWITCH static int COMBINE(OnSwitch, CONTEXT)(int ability, int battler)
#define CONTEXT_ON_SWITCH .onSwitch = COMBINE(OnSwitch, CONTEXT)

static int SwitchInAnnounce(int message) {
    gBattleCommunication[MULTISTRING_CHOOSER] = message;
    BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
    return TRUE;
}

static int TryTransformAttacker(int ability, int battler) {
    CHECK(ShouldChangeFormHpBased(battler))
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)
    
    BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
    return TRUE;
}

static int AddBattlerType(int battler, int type) {
    CHECK_NOT(IS_BATTLER_OF_TYPE(battler, type))

    gBattleMons[battler].type3 = type;
    PREPARE_TYPE_BUFFER(gBattleTextBuff2, gBattleMons[battler].type3);
    BattleScriptPushCursorAndCallback(BattleScript_BattlerAddedTheType);
    return TRUE;
}

#define CONTEXT None
static const Ability None = {
    .name = $("-------"),
    .description = $("Empty ability slot."),
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Stench
static const Ability Stench = {
    .name = $("Stench"),
    .description = $("Attacks have a 10% chance to\ncause enemy to flinch."),
};

#undef CONTEXT
#define CONTEXT Drizzle
ON_SWITCH {
    if (TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN, TRUE))
    {
        BattleScriptPushCursorAndCallback(BattleScript_DrizzleActivates);
        return TRUE;
    }
    else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
    {
        BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
        return NO_ANNOUNCE;
    }
    return FALSE;
}
static const Ability Drizzle = {
    .name = $("Drizzle"),
    .description = $("Summons rain on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SpeedBoost
static const Ability SpeedBoost = {
    .name = $("Speed Boost"),
    .description = $("Raises own Speed by one stage\nafter every turn."),
};

#undef CONTEXT
#define CONTEXT BattleArmor
static const Ability BattleArmor = {
    .name = $("Battle Armor"),
    .description = $("Immune to critical hits. Takes\n20% less damage from all attacks."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Sturdy
static const Ability Sturdy = {
    .name = $("Sturdy"),
    .description = $("At full HP, cannot be KO in one\nhit, stays at 1 HP instead."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Damp
static const Ability Damp = {
    .name = $("Damp"),
    .description = $("Makes foe Water-type on contact.\nAlso works on offense."),
};

#undef CONTEXT
#define CONTEXT Limber
static const Ability Limber = {
    .name = $("Limber"),
    .description = $("Immune to paralysis.\nTakes 50% less recoil damage."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SandVeil
static const Ability SandVeil = {
    .name = $("Sand Veil"),
    .description = $("Evasion is boosted by 1.25x\nwhile a sandstorm is active."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Static
static const Ability Static = {
    .name = $("Static"),
    .description = $("30% chance to paralyze on\ncontact. Also works on offense."),
};

#undef CONTEXT
#define CONTEXT VoltAbsorb
static const Ability VoltAbsorb = {
    .name = $("Volt Absorb"),
    .description = $("Heals 25% of max HP when hit\nby an Electric-type move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WaterAbsorb
static const Ability WaterAbsorb = {
    .name = $("Water Absorb"),
    .description = $("Heals 25% of max HP when hit\nby a Water-type move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Oblivious
static const Ability Oblivious = {
    .name = $("Oblivious"),
    .description = $("Immune to infatuation, Scare,\nIntimidate and Taunt."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT CloudNine
ON_SWITCH {
    BattleScriptPushCursorAndCallback(BattleScript_AnnounceAirLockCloudNine);
    return TRUE;
}
static const Ability CloudNine = {
    .name = $("Cloud Nine"),
    .description = $("Clears weather and prevents\nits effects."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT CompoundEyes
static const Ability CompoundEyes = {
    .name = $("Compound Eyes"),
    .description = $("Grants a 1.3x accuracy boost."),
};

#undef CONTEXT
#define CONTEXT Insomnia
static const Ability Insomnia = {
    .name = $("Insomnia"),
    .description = $("Cannot fall asleep.\nRest fails if used."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ColorChange
static const Ability ColorChange = {
    .name = $("Color Change"),
    .description = $("Changes type to a resist or an\nimmunity before getting hit."),
};

#undef CONTEXT
#define CONTEXT Immunity
static const Ability Immunity = {
    .name = $("Immunity"),
    .description = $("Cannot be poisoned. Halves\ndamage taken from Poison moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT FlashFire
static const Ability FlashFire = {
    .name = $("Flash Fire"),
    .description = $("Powers up Fire-type moves by\n1.5x if hit by a Fire-type move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ShieldDust
static const Ability ShieldDust = {
    .name = $("Shield Dust"),
    .description = $("Immune to added move effects and\nall entry hazards."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT OwnTempo
static const Ability OwnTempo = {
    .name = $("Own Tempo"),
    .description = $("Immune to confusion, Intimidate\nand Scare."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SuctionCups
static const Ability SuctionCups = {
    .name = $("Suction Cups"),
    .description = $("Cannot be forced to switch out\nby an enemy's move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Intimidate
static const Ability Intimidate = {
    .name = $("Intimidate"),
    .description = $("Lowers foes' Atk by one stage on\nentry."),
    .onSwitch = UseIntimidateClone,
};

#undef CONTEXT
#define CONTEXT ShadowTag
static const Ability ShadowTag = {
    .name = $("Shadow Tag"),
    .description = $("Opponents can't be switched out.\nGhosts aren't affected."),
};

#undef CONTEXT
#define CONTEXT RoughSkin
static const Ability RoughSkin = {
    .name = $("Rough Skin"),
    .description = $("Enemies lose 1/8 of max HP if\nthey use a contact move."),
};

#undef CONTEXT
#define CONTEXT WonderGuard
static const Ability WonderGuard = {
    .name = $("Wonder Guard"),
    .description = $("Is only hit by Super-effective\nattacks or indirect damage."),
    .breakable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Levitate
static const Ability Levitate = {
    .name = $("Levitate"),
    .description = $("Immune to Ground-type moves.\nUps own Flying moves by 1.25x."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT EffectSpore
static const Ability EffectSpore = {
    .name = $("Effect Spore"),
    .description = $("30% chance to inflict SLP, PARA\nor PSN if hit by a contact move."),
};

#undef CONTEXT
#define CONTEXT Synchronize
static const Ability Synchronize = {
    .name = $("Synchronize"),
    .description = $("Enemies inflicting status on\nthis Pokémon get same status."),
};

#undef CONTEXT
#define CONTEXT ClearBody
static const Ability ClearBody = {
    .name = $("Clear Body"),
    .description = $("Immune to stat drops."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT NaturalCure
static const Ability NaturalCure = {
    .name = $("Natural Cure"),
    .description = $("Heals status condition upon\nswitching out."),
};

#undef CONTEXT
#define CONTEXT LightningRod
static const Ability LightningRod = {
    .name = $("Lightning Rod"),
    .description = $("Redirects Electric moves.\nAbsorbs them, ups highest Atk."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SereneGrace
static const Ability SereneGrace = {
    .name = $("Serene Grace"),
    .description = $("Doubles chance of secondary\neffects on its own moves."),
};

#undef CONTEXT
#define CONTEXT SwiftSwim
static const Ability SwiftSwim = {
    .name = $("Swift Swim"),
    .description = $("This Pokémon's Speed gets a\n1.5x boost if rain is active."),
};

#undef CONTEXT
#define CONTEXT Chlorophyll
static const Ability Chlorophyll = {
    .name = $("Chlorophyll"),
    .description = $("This Pokémon's Speed gets a\n1.5x boost if sun is active."),
};

#undef CONTEXT
#define CONTEXT Illuminate
static const Ability Illuminate = {
    .name = $("Illuminate"),
    .description = $("Grants a 1.2x accuracy boost."),
};

#undef CONTEXT
#define CONTEXT Trace
ON_SWITCH {
    CHECK_NOT(gTurnStructs[battler].traced)

    gBattleResources->flags->flags[battler] |= RESOURCE_FLAG_TRACED;
    gTurnStructs[battler].traced = TRUE;
    return FALSE;
}
static const Ability Trace = {
    .name = $("Trace"),
    .description = $("Copies the foe's ability.\nDoes not copy innates."),
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT HugePower
static const Ability HugePower = {
    .name = $("Huge Power"),
    .description = $("Doubles own Attack stat.\nBoosts raw stat, not base stat."),
};

#undef CONTEXT
#define CONTEXT PoisonPoint
static const Ability PoisonPoint = {
    .name = $("Poison Point"),
    .description = $("30% chance to poison on contact.\nAlso works on offense."),
};

#undef CONTEXT
#define CONTEXT InnerFocus
static const Ability InnerFocus = {
    .name = $("Inner Focus"),
    .description = $("Blocks flinch, Intimidate, Scare.\nFocus Blast never misses."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MagmaArmor
static const Ability MagmaArmor = {
    .name = $("Magma Armor"),
    .description = $("Frostbite-immune. Takes 30% less\ndmg from Water/Ice-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WaterVeil
ON_SWITCH {
    CHECK_NOT(gStatuses3[battler] & STATUS3_AQUA_RING)

    gStatuses3[battler] |= STATUS3_AQUA_RING;
    BattleScriptPushCursorAndCallback(BattleScript_BattlerEnvelopedItselfInAVeil);
    return TRUE;
}
static const Ability WaterVeil = {
    .name = $("Water Veil"),
    .description = $("Burn-immune.\nCasts Aqua Ring on entry."),
    .breakable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT MagnetPull
static const Ability MagnetPull = {
    .name = $("Magnet Pull"),
    .description = $("Traps opposing Steel-types.\nGhosts aren't affected."),
};

#undef CONTEXT
#define CONTEXT Soundproof
static const Ability Soundproof = {
    .name = $("Soundproof"),
    .description = $("Immune to sound-based moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT RainDish
static const Ability RainDish = {
    .name = $("Rain Dish"),
    .description = $("Heals 1/8 of max HP every turn\nif rain is active."),
};

#undef CONTEXT
#define CONTEXT SandStream
ON_SWITCH {
    if (TryChangeBattleWeather(battler, ENUM_WEATHER_SANDSTORM, TRUE))
    {
        BattleScriptPushCursorAndCallback(BattleScript_SandstreamActivates);
        return TRUE;
    }
    else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
    {
        BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
        return NO_ANNOUNCE;
    }
    return FALSE;
}
static const Ability SandStream = {
    .name = $("Sand Stream"),
    .description = $("Summons a sandstorm on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH
};

#undef CONTEXT
#define CONTEXT Pressure
ON_SWITCH {
    int loweredStats = 0;
    for (int i = 0; i < gBattlersCount; i++)
    {
        if (!IsBattlerAlive(i)) continue;
        loweredStats |= TryResetBattlerStatChanges(i, i == battler ? RESET_STAT_DROPS : RESET_STAT_BUFFS);
    }

    if (loweredStats)
    {
        BattleScriptPushCursorAndCallback(BattleScript_PressureRemoveStats);
    }

    SwitchInAnnounce(B_MSG_SWITCHIN_PRESSURE);
    
    return TRUE;
}
static const Ability Pressure = {
    .name = $("Pressure"),
    .description = $("Doubles foe's PP usage.\nClears stat buffs on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT ThickFat
static const Ability ThickFat = {
    .name = $("Thick Fat"),
    .description = $("Takes 1/2 damage from Fire-type\nand Ice-type attacks."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT EarlyBird
static const Ability EarlyBird = {
    .name = $("Early Bird"),
    .description = $("Awakens twice as fast from sleep."),
};

#undef CONTEXT
#define CONTEXT FlameBody
static const Ability FlameBody = {
    .name = $("Flame Body"),
    .description = $("30% chance to burn on contact.\nAlso works on offense."),
};

#undef CONTEXT
#define CONTEXT RunAway
static const Ability RunAway = {
    .name = $("Run Away"),
    .description = $("Guarantees fleeing. Raises Speed\nif stats lowered by an enemy."),
};

#undef CONTEXT
#define CONTEXT KeenEye
static const Ability KeenEye = {
    .name = $("Keen Eye"),
    .description = $("Immune to accuracy drops.\nGrants a 1.2x accuracy boost."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT HyperCutter
static const Ability HyperCutter = {
    .name = $("Hyper Cutter"),
    .description = $("Enemies can't lower Atk/Sp. Atk.\nCrit rate of contact moves: +1."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Pickup
ON_SWITCH {
    CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_HAZARDS_ANY
        || gSideTimers[GetBattlerSide(battler)].hotCoals
        || gSideTimers[GetBattlerSide(battler)].caltrops)
    
    gSideStatuses[GetBattlerSide(battler)] &= ~(SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_SPIKES | SIDE_STATUS_STICKY_WEB);
    gSideTimers[GetBattlerSide(battler)].hotCoals = FALSE;
    gSideTimers[GetBattlerSide(battler)].caltrops = FALSE;
    BattleScriptPushCursorAndCallback(BattleScript_PickUpActivate);
    return TRUE;
}
static const Ability Pickup = {
    .name = $("Pickup"),
    .description = $("Removes all hazards on entry.\nNot immune to hazards."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Truant
static const Ability Truant = {
    .name = $("Truant"),
    .description = $("Can only attack every other turn.\nCan use status moves every turn."),
};

#undef CONTEXT
#define CONTEXT Hustle
static const Ability Hustle = {
    .name = $("Hustle"),
    .description = $("0.9x accuracy.\nRaises Atk & SpAtk by 1.4x."),
};

#undef CONTEXT
#define CONTEXT CuteCharm
static const Ability CuteCharm = {
    .name = $("Cute Charm"),
    .description = $("30% chance to charm attacker on\ncontact, which halves its power."),
};

#undef CONTEXT
#define CONTEXT Plus
static const Ability Plus = {
    .name = $("Plus"),
    .description = $("Deals double damage if an ally\nPokémon has Minus or Plus."),
};

#undef CONTEXT
#define CONTEXT Minus
static const Ability Minus = {
    .name = $("Minus"),
    .description = $("Deals double damage if an ally\nPokémon has Minus or Plus."),
};

#undef CONTEXT
#define CONTEXT Forecast
static const Ability Forecast = {
    .name = $("Forecast"),
    .description = $("Changes form with the weather.\nWeather setting triggers attack."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .onSwitch = TryTransformAttacker,
};

#undef CONTEXT
#define CONTEXT StickyHold
static const Ability StickyHold = {
    .name = $("Sticky Hold"),
    .description = $("Can't lose its item."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ShedSkin
static const Ability ShedSkin = {
    .name = $("Shed Skin"),
    .description = $("30% chance to heal its status\ncondition at the end of a turn."),
};

#undef CONTEXT
#define CONTEXT Guts
static const Ability Guts = {
    .name = $("Guts"),
    .description = $("Ups Atk by 1.5x if suffering\nfrom a status condition."),
};

#undef CONTEXT
#define CONTEXT MarvelScale
static const Ability MarvelScale = {
    .name = $("Marvel Scale"),
    .description = $("Ups Def by 1.5x if suffering\nfrom a status condition."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT LiquidOoze
static const Ability LiquidOoze = {
    .name = $("Liquid Ooze"),
    .description = $("Draining causes harm to enemies\ninstead of healing them."),
};

#undef CONTEXT
#define CONTEXT Overgrow
static const Ability Overgrow = {
    .name = $("Overgrow"),
    .description = $("Boosts Grass-type moves by 1.2x,\nor 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT Blaze
static const Ability Blaze = {
    .name = $("Blaze"),
    .description = $("Boosts Fire-type moves by 1.2x,\nor 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT Torrent
static const Ability Torrent = {
    .name = $("Torrent"),
    .description = $("Boosts Water-type moves by 1.2x,\nor 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT Swarm
static const Ability Swarm = {
    .name = $("Swarm"),
    .description = $("Boosts Bug-type moves by 1.2x,\nor 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT RockHead
static const Ability RockHead = {
    .name = $("Rock Head"),
    .description = $("Immune to recoil damage, but not\nimmune to Explosion/crash dmg."),
};

#undef CONTEXT
#define CONTEXT Drought
ON_SWITCH {
    if (TryChangeBattleWeather(battler, ENUM_WEATHER_SUN, TRUE))
    {
        BattleScriptPushCursorAndCallback(BattleScript_DroughtActivates);
        return TRUE;
    }
    else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
    {
        BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
        return NO_ANNOUNCE;
    }
    return FALSE;
}
static const Ability Drought = {
    .name = $("Drought"),
    .description = $("Summons sun on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT ArenaTrap
static const Ability ArenaTrap = {
    .name = $("Arena Trap"),
    .description = $("Enemies can't flee. Ghosts and\nungrounded Pokémon are immune."),
};

#undef CONTEXT
#define CONTEXT VitalSpirit
static const Ability VitalSpirit = {
    .name = $("Vital Spirit"),
    .description = $("Can't fall asleep. Heals status\nafter using Fighting-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WhiteSmoke
ON_SWITCH {
    CHECK_NOT(gSideTimers[GET_BATTLER_SIDE(battler)].smokescreenTimer)
    
    int side = GET_BATTLER_SIDE(battler);
    gSideTimers[side].smokescreenTimer = GetBattlerHoldEffect(battler, TRUE) == ITEM_LIGHT_CLAY ? SCREEN_DURATION : SCREEN_DURATION_SHORT;
    gSideTimers[side].started.smokescreen = TRUE;
    gSideTimers[side].smokescreenBattler = battler;
    return SwitchInAnnounce(B_MSG_SWITCHIN_WHITE_SMOKE);
}
static const Ability WhiteSmoke = {
    .name = $("White Smoke"),
    .description = $("Sets Smokescreen for 3 turns\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT PurePower
static const Ability PurePower = {
    .name = $("Pure Power"),
    .description = $("Doubles own Attack stat.\nBoosts raw stat, not base stat."),
};

#undef CONTEXT
#define CONTEXT ShellArmor
static const Ability ShellArmor = {
    .name = $("Shell Armor"),
    .description = $("Immune to critical hits. Takes\n20% less damage from all attacks."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT AirLock
static const Ability AirLock = {
    .name = $("Air Lock"),
    .description = $("Clears weather and prevents\nits effects."),
    .onSwitch = CloudNine.onSwitch,
};

#undef CONTEXT
#define CONTEXT TangledFeet
static const Ability TangledFeet = {
    .name = $("Tangled Feet"),
    .description = $("Doubles Evasion when confused."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MotorDrive
static const Ability MotorDrive = {
    .name = $("Motor Drive"),
    .description = $("Boosts Speed instead of being\nhit by Electric-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Rivalry
static const Ability Rivalry = {
    .name = $("Rivalry"),
    .description = $("Deals 1.25x to same gender.\nTakes .75x from opposite gender."),
};

#undef CONTEXT
#define CONTEXT Steadfast
static const Ability Steadfast = {
    .name = $("Steadfast"),
    .description = $("Raises Speed by one stage if\nthis Pokémon flinches."),
};

#undef CONTEXT
#define CONTEXT SnowCloak
static const Ability SnowCloak = {
    .name = $("Snow Cloak"),
    .description = $("Evasion is boosted by 1.25x\nunder hail."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Gluttony
static const Ability Gluttony = {
    .name = $("Gluttony"),
    .description = $("Eats berries early. Berries also\nrestore 1/3 of max HP."),
};

#undef CONTEXT
#define CONTEXT AngerPoint
static const Ability AngerPoint = {
    .name = $("Anger Point"),
    .description = $("Getting hit raises Atk by +1.\nCritical hits maximize Attack."),
};

#undef CONTEXT
#define CONTEXT Unburden
static const Ability Unburden = {
    .name = $("Unburden"),
    .description = $("Consuming its held item doubles\nSpeed until switched out."),
};

#undef CONTEXT
#define CONTEXT Heatproof
static const Ability Heatproof = {
    .name = $("Heatproof"),
    .description = $("Halves damage taken from Fire-\ntype moves. Takes no burn damage."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Simple
static const Ability Simple = {
    .name = $("Simple"),
    .description = $("Doubles all stat changes on\nthis Pokémon."),
};

#undef CONTEXT
#define CONTEXT DrySkin
static const Ability DrySkin = {
    .name = $("Dry Skin"),
    .description = $("Water/Rain heals.\nFire/Sun hurts."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Download
ON_SWITCH {
    gBattlerTarget = BATTLE_OPPOSITE(battler);
    if (!IsBattlerAlive(battler)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
    CHECK(IsBattlerAlive(battler))

    int stat = GetHighestDefendingStatId(gBattlerTarget, TRUE) == STAT_DEF ? STAT_SPATK : STAT_ATK;
    CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
    BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
    return TRUE;
}
static const Ability Download = {
    .name = $("Download"),
    .description = $("Raises Atk/Sp. Atk by one stage\ndepending on opponent."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT IronFist
static const Ability IronFist = {
    .name = $("Iron Fist"),
    .description = $("Boosts the power of punching\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT PoisonHeal
static const Ability PoisonHeal = {
    .name = $("Poison Heal"),
    .description = $("Restores 1/8 of max HP after\neach turn if poisoned."),
};

#undef CONTEXT
#define CONTEXT Adaptability
static const Ability Adaptability = {
    .name = $("Adaptability"),
    .description = $("Increases STAB from 1.5x to 2x."),
};

#undef CONTEXT
#define CONTEXT SkillLink
static const Ability SkillLink = {
    .name = $("Skill Link"),
    .description = $("Multi-hit moves always hit the\nmaximum number of times."),
};

#undef CONTEXT
#define CONTEXT Hydration
static const Ability Hydration = {
    .name = $("Hydration"),
    .description = $("Cures own status at the end of\nevery turn in rain."),
};

#undef CONTEXT
#define CONTEXT SolarPower
static const Ability SolarPower = {
    .name = $("Solar Power"),
    .description = $("Ups highest attacking stat\nby 1.5x in sun."),
};

#undef CONTEXT
#define CONTEXT QuickFeet
static const Ability QuickFeet = {
    .name = $("Quick Feet"),
    .description = $("Ups Speed by 1.5x if suffering\nfrom a status condition."),
};

#undef CONTEXT
#define CONTEXT Normalize
static const Ability Normalize = {
    .name = $("Normalize"),
    .description = $("Its moves become Normal-type,\nget 1.1x boost, ignore resists."),
};

#undef CONTEXT
#define CONTEXT Sniper
static const Ability Sniper = {
    .name = $("Sniper"),
    .description = $("Critical hits have a 2.25x dmg\nmultiplier instead of 1.5x."),
};

#undef CONTEXT
#define CONTEXT MagicGuard
static const Ability MagicGuard = {
    .name = $("Magic Guard"),
    .description = $("Only damaged by attacks."),
};

#undef CONTEXT
#define CONTEXT NoGuard
static const Ability NoGuard = {
    .name = $("No Guard"),
    .description = $("Attacks used by and on this\nPokémon bypass accuracy checks."),
};

#undef CONTEXT
#define CONTEXT Stall
static const Ability Stall = {
    .name = $("Stall"),
    .description = $("Takes 30% less damage if it\nhasn't moved yet."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Technician
static const Ability Technician = {
    .name = $("Technician"),
    .description = $("Moves with 60 BP or less get\na 1.5x boost."),
};

#undef CONTEXT
#define CONTEXT LeafGuard
static const Ability LeafGuard = {
    .name = $("Leaf Guard"),
    .description = $("Immune to status conditions if\nsun is active."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Klutz
static const Ability Klutz = {
    .name = $("Klutz"),
    .description = $("Own held item has no effect.\nMega Stones are unaffected."),
};

#undef CONTEXT
#define CONTEXT MoldBreaker
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_MOLDBREAKER);
}
static const Ability MoldBreaker = {
    .name = $("Mold Breaker"),
    .description = $("Moves hit through abilities.\nAlso affects innates."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SuperLuck
static const Ability SuperLuck = {
    .name = $("Super Luck"),
    .description = $("Raises critical-hit ratio of own\nmoves by +1."),
};

#undef CONTEXT
#define CONTEXT Aftermath
static const Ability Aftermath = {
    .name = $("Aftermath"),
    .description = $("If faints by a contact move,\nattacker takes 25% of max HP."),
};

#undef CONTEXT
#define CONTEXT Anticipation
ON_SWITCH {
    int side = GetBattlerSide(battler);
    int any = FALSE;

    for (int i = 0; i < gBattlersCount; i++)
    {
        if (IsBattlerAlive(i) && side != GetBattlerSide(i))
        {
            for (int j = 0; j < MAX_MON_MOVES; j++)
            {
                int move = gBattleMons[i].moves[j];
                int moveType = gBattleMoves[move].type;
                if (CalcTypeEffectivenessMultiplier(move, moveType, i, battler, FALSE) >= UQ_4_12(2.0))
                {
                    any = TRUE;
                    break;
                }
            }
        }
    }

    CHECK(any)

    return SwitchInAnnounce(B_MSG_SWITCHIN_ANTICIPATION);
}
static const Ability Anticipation = {
    .name = $("Anticipation"),
    .description = $("Senses Super-effective moves.\nBlocks one Super-effective hit."),
    .persistent = TRUE,
    .breakable = TRUE,
    CONTEXT_ON_SWITCH, 
};

#undef CONTEXT
#define CONTEXT Forewarn
ON_SWITCH {
    gBattlerTarget = BATTLE_OPPOSITE(battler);
    if (!IsBattlerAlive(gBattlerTarget) || gWishFutureKnock.futureSightCounter[gBattlerTarget]) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
    CHECK(IsBattlerAlive(gBattlerTarget))
    CHECK_NOT(gWishFutureKnock.futureSightCounter[gBattlerTarget])

    gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] |= SIDE_STATUS_FUTUREATTACK;
    gWishFutureKnock.futureSightMove[gBattlerTarget] = MOVE_FUTURE_SIGHT;
    gWishFutureKnock.futureSightPower[gBattlerTarget] = 50;
    gWishFutureKnock.futureSightAttacker[gBattlerTarget] = battler;
    gWishFutureKnock.futureSightCounter[gBattlerTarget] = 3;

    BattleScriptPushCursorAndCallback(BattleScript_ForewarnReworkActivates);
    return TRUE;
}
static const Ability Forewarn = {
    .name = $("Forewarn"),
    .description = $("Casts a 50 BP Future Sight on\nentry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Unaware
static const Ability Unaware = {
    .name = $("Unaware"),
    .description = $("Ignores foes' stat changes, both\npositive and negative ones."),
    .breakable = TRUE,
    .unaware = TRUE,
};

#undef CONTEXT
#define CONTEXT TintedLens
static const Ability TintedLens = {
    .name = $("Tinted Lens"),
    .description = $("Attacks deal double damage if\nresisted."),
};

#undef CONTEXT
#define CONTEXT Filter
static const Ability Filter = {
    .name = $("Filter"),
    .description = $("Takes 35% less damage from\nSuper-effective moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SlowStart
ON_SWITCH {
    gVolatileStructs[battler].slowStartTimer = 5;
    return SwitchInAnnounce(B_MSG_SWITCHIN_SLOWSTART);
}
static const Ability SlowStart = {
    .name = $("Slow Start"),
    .description = $("Halves Attack and Speed during\nthe first 5 turns out."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Scrappy
static const Ability Scrappy = {
    .name = $("Scrappy"),
    .description = $("Normal/Fighting can hit Ghosts.\nImmune to Intimidate/Scare."),
};

#undef CONTEXT
#define CONTEXT StormDrain
static const Ability StormDrain = {
    .name = $("Storm Drain"),
    .description = $("Redirects Water moves.\nAbsorbs them, ups highest Atk."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT IceBody
static const Ability IceBody = {
    .name = $("Ice Body"),
    .description = $("Heals 1/8 of max HP every turn\nin hail."),
};

#undef CONTEXT
#define CONTEXT SolidRock
static const Ability SolidRock = {
    .name = $("Solid Rock"),
    .description = $("Takes 35% less damage from\nSuper-effective moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SnowWarning
ON_SWITCH {
    if (TryChangeBattleWeather(battler, ENUM_WEATHER_HAIL, TRUE))
    {
        BattleScriptPushCursorAndCallback(BattleScript_SnowWarningActivates);
        return TRUE;
    }
    else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
    {
        BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
        return NO_ANNOUNCE;
    }
    return FALSE;
}
static const Ability SnowWarning = {
    .name = $("Snow Warning"),
    .description = $("Summons hail on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT HoneyGather
static const Ability HoneyGather = {
    .name = $("Honey Gather"),
    .description = $("Has a 50% chance to find Honey\neach turn."),
};

#undef CONTEXT
#define CONTEXT Frisk
ON_SWITCH {
    int any = FALSE;
    for (int i = GetBattlerSide(BATTLE_OPPOSITE(battler)); i < gBattlersCount; i += 2)
    {
        FILTER(IsBattlerAlive(i))
        FILTER(gBattleMons[i].item)
        any = TRUE;
        break;
    }

    CHECK(any)
    BattleScriptPushCursorAndCallback(BattleScript_FriskActivates);
    return TRUE;
}
static const Ability Frisk = {
    .name = $("Frisk"),
    .description = $("Checks foes' item and disables\ntheir items for two turns."),
};

#undef CONTEXT
#define CONTEXT Reckless
static const Ability Reckless = {
    .name = $("Reckless"),
    .description = $("Moves causing recoil damage\ndeal 1.2x more damage."),
};

#undef CONTEXT
#define CONTEXT Multitype
static const Ability Multitype = {
    .name = $("Multitype"),
    .description = $("Held Plate item decides holder's\ntype."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT FlowerGift
static const Ability FlowerGift = {
    .name = $("Flower Gift"),
    .description = $("Increases the party's SpAtk\nand SpDef by 1.5x in Sun."),
    .unsuppressable = TRUE,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
    .onSwitch = TryTransformAttacker,
};

#undef CONTEXT
#define CONTEXT BadDreams
static const Ability BadDreams = {
    .name = $("Bad Dreams"),
    .description = $("Sleeping Pokémon lose 1/4 of max\nHP at the end of each turn."),
};

#undef CONTEXT
#define CONTEXT Pickpocket
static const Ability Pickpocket = {
    .name = $("Pickpocket"),
    .description = $("Steals the foe's held item on\ncontact."),
};

#undef CONTEXT
#define CONTEXT SheerForce
static const Ability SheerForce = {
    .name = $("Sheer Force"),
    .description = $("Exchanges added effects on its\nmoves for 1.3x more power."),
};

#undef CONTEXT
#define CONTEXT Contrary
static const Ability Contrary = {
    .name = $("Contrary"),
    .description = $("Stat raises turn into stat drops\nfor this Pokémon and vice versa."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Unnerve
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_UNNERVE);
}
static const Ability Unnerve = {
    .name = $("Unnerve"),
    .description = $("Foes can't eat Berries as long\nas this Pokémon is in battle."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Defiant
static const Ability Defiant = {
    .name = $("Defiant"),
    .description = $("Raises Attack by two stages if\nstats are lowered by an enemy."),
};

#undef CONTEXT
#define CONTEXT Defeatist
static const Ability Defeatist = {
    .name = $("Defeatist"),
    .description = $("Halves Atk and Sp. Atk stats if\nuser is below 1/3 of max HP."),
};

#undef CONTEXT
#define CONTEXT CursedBody
static const Ability CursedBody = {
    .name = $("Cursed Body"),
    .description = $("30% chance to disable moves\nif enemy makes contact."),
};

#undef CONTEXT
#define CONTEXT Healer
static const Ability Healer = {
    .name = $("Healer"),
    .description = $("30% chance to heal user or ally's\nstatus at the end of each turn."),
};

#undef CONTEXT
#define CONTEXT FriendGuard
static const Ability FriendGuard = {
    .name = $("Friend Guard"),
    .description = $("Reduces damage that ally takes\nby 50% in double battles."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WeakArmor
static const Ability WeakArmor = {
    .name = $("Weak Armor"),
    .description = $("If hit by a contact attack:\n-1 Defense and +2 Speed."),
};

#undef CONTEXT
#define CONTEXT HeavyMetal
static const Ability HeavyMetal = {
    .name = $("Heavy Metal"),
    .description = $("Doubles this Pokémon's weight."),
};

#undef CONTEXT
#define CONTEXT LightMetal
static const Ability LightMetal = {
    .name = $("Light Metal"),
    .description = $("Boosts Speed by 1.3x and halves\nthis Pokémon's weight."),
};

#undef CONTEXT
#define CONTEXT Multiscale
static const Ability Multiscale = {
    .name = $("Multiscale"),
    .description = $("At full HP, halves damage taken\nfrom attacks"),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ToxicBoost
static const Ability ToxicBoost = {
    .name = $("Toxic Boost"),
    .description = $("Ups Atk by 1.5x if poisoned.\nImmune to Poison status damage."),
};

#undef CONTEXT
#define CONTEXT FlareBoost
ON_SWITCH {
    CHECK(CanBeBurned(battler))
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

    gBattleMons[battler].status1 |= STATUS1_BURN;
    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
    MarkBattlerForControllerExec(battler);
    BattleScriptPushCursorAndCallback(BattleScript_FlareBoostEnd3);
    return TRUE;
}
static const Ability FlareBoost = {
    .name = $("Flare Boost"),
    .description = $("Ups Sp. Atk by 1.5x if burned.\nIgnites in fog."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Harvest
static const Ability Harvest = {
    .name = $("Harvest"),
    .description = $("50% chance to recycle a used\nBerry every turn, 100% in sun."),
};

#undef CONTEXT
#define CONTEXT Telepathy
static const Ability Telepathy = {
    .name = $("Telepathy"),
    .description = $("Can't be damaged by ally attacks."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Moody
static const Ability Moody = {
    .name = $("Moody"),
    .description = $("Lowers a random stat by -1 and\nraises another by +2 every turn."),
};

#undef CONTEXT
#define CONTEXT Overcoat
static const Ability Overcoat = {
    .name = $("Overcoat"),
    .description = $("Blocks weather dmg, powder moves.\n20% Special damage reduction."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT PoisonTouch
static const Ability PoisonTouch = {
    .name = $("Poison Touch"),
    .description = $("30% chance to poison on contact.\nAlso works on offense."),
};

#undef CONTEXT
#define CONTEXT Regenerator
static const Ability Regenerator = {
    .name = $("Regenerator"),
    .description = $("Heals 1/3 of max HP upon\nswitching out."),
};

#undef CONTEXT
#define CONTEXT BigPecks
static const Ability BigPecks = {
    .name = $("Big Pecks"),
    .description = $("Boosts the power of contact\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT SandRush
static const Ability SandRush = {
    .name = $("Sand Rush"),
    .description = $("This Pokémon's Speed gets a\n1.5x boost in a sandstorm."),
};

#undef CONTEXT
#define CONTEXT WonderSkin
static const Ability WonderSkin = {
    .name = $("Wonder Skin"),
    .description = $("Opposing status moves have\ntheir accuracy halved."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Analytic
static const Ability Analytic = {
    .name = $("Analytic"),
    .description = $("Attacks get a 1.3x power boost\nif it moves last."),
};

#undef CONTEXT
#define CONTEXT Illusion
static const Ability Illusion = {
    .name = $("Illusion"),
    .description = $("Appears as last party slot and\nboosts power by 1.3x until hit."),
};

#undef CONTEXT
#define CONTEXT Imposter
ON_SWITCH {
    gBattlerTarget = BATTLE_OPPOSITE(battler);
    if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
    CHECK(IsBattlerAlive(gBattlerTarget))
    CHECK_NOT(gBattleMons[gBattlerTarget].status2 & (STATUS2_TRANSFORMED | STATUS2_SUBSTITUTE))
    CHECK_NOT(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)
    CHECK_NOT(gBattleStruct->illusion[gBattlerTarget].on)
    CHECK_NOT(gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE)
    
    BattleScriptPushCursorAndCallback(BattleScript_ImposterActivates);
    return TRUE;
}
static const Ability Imposter = {
    .name = $("Imposter"),
    .description = $("Transforms into the foe on\nentry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Infiltrator
static const Ability Infiltrator = {
    .name = $("Infiltrator"),
    .description = $("Own moves bypass Substitutes\nand damage reduction screens."),
};

#undef CONTEXT
#define CONTEXT Mummy
static const Ability Mummy = {
    .name = $("Mummy"),
    .description = $("If hit, makes the attacker's ability\nMummy."),
};

#undef CONTEXT
#define CONTEXT Moxie
static const Ability Moxie = {
    .name = $("Moxie"),
    .description = $("Dealing a KO raises Attack by\none stage."),
};

#undef CONTEXT
#define CONTEXT Justified
static const Ability Justified = {
    .name = $("Justified"),
    .description = $("Boosts Attack instead of being\nhit by Dark-type moves."),
};

#undef CONTEXT
#define CONTEXT Rattled
static const Ability Rattled = {
    .name = $("Rattled"),
    .description = $("If hit by Bug, Dark or Ghost\nmove, or flinches: +1 Speed."),
};

#undef CONTEXT
#define CONTEXT MagicBounce
static const Ability MagicBounce = {
    .name = $("Magic Bounce"),
    .description = $("Bounces back the effect of\nstatus moves to their user."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SapSipper
static const Ability SapSipper = {
    .name = $("Sap Sipper"),
    .description = $("Boosts highest Atk instead of\nbeing hit by Grass-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Prankster
static const Ability Prankster = {
    .name = $("Prankster"),
    .description = $("Status moves have +1 priority\nbut fail on opposing Dark-types."),
};

#undef CONTEXT
#define CONTEXT SandForce
static const Ability SandForce = {
    .name = $("Sand Force"),
    .description = $("Ups highest attacking stat\nby 1.5x in sand."),
};

#undef CONTEXT
#define CONTEXT IronBarbs
static const Ability IronBarbs = {
    .name = $("Iron Barbs"),
    .description = $("Enemies lose 1/8 of max HP if\nthey use a contact move."),
};

#undef CONTEXT
#define CONTEXT ZenMode
static const Ability ZenMode = {
    .name = $("Zen Mode"),
    .description = $("Transforms into Zen Mode on\nentry until end of battle."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .onSwitch = TryTransformAttacker,
};

#undef CONTEXT
#define CONTEXT VictoryStar
static const Ability VictoryStar = {
    .name = $("Victory Star"),
    .description = $("Gives 1.2x accuracy boost to\nits own and its allies' moves."),
};

#undef CONTEXT
#define CONTEXT Turboblaze
ON_SWITCH {
    return AddBattlerType(battler, TYPE_FIRE);
}
static const Ability Turboblaze = {
    .name = $("Turboblaze"),
    .description = $("Moves hit through abilities.\nAdds Fire type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Teravolt
ON_SWITCH {
    return AddBattlerType(battler, TYPE_ELECTRIC);
}
static const Ability Teravolt = {
    .name = $("Teravolt"),
    .description = $("Moves hit through abilities.\nAdds Electric type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT AromaVeil
static const Ability AromaVeil = {
    .name = $("Aroma Veil"),
    .description = $("Immune to Encore, Attract, Taunt,\nTorment, Disable, Heal Block."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT FlowerVeil
static const Ability FlowerVeil = {
    .name = $("Flower Veil"),
    .description = $("Grass-types on this Pokémon's\nside are immune to stat drops."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT CheekPouch
static const Ability CheekPouch = {
    .name = $("Cheek Pouch"),
    .description = $("This ability has no effect."),
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Protean
static const Ability Protean = {
    .name = $("Protean"),
    .description = $("Changes type depending on the\nmove it's about to use."),
};

#undef CONTEXT
#define CONTEXT FurCoat
static const Ability FurCoat = {
    .name = $("Fur Coat"),
    .description = $("Halves damage taken by Physical\nmoves. Does NOT double Defense."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Magician
static const Ability Magician = {
    .name = $("Magician"),
    .description = $("Steals the foe's held item after\nusing a non-contact move."),
};

#undef CONTEXT
#define CONTEXT Bulletproof
static const Ability Bulletproof = {
    .name = $("Bulletproof"),
    .description = $("Immune to projectile, ball, or\nbomb-based moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Competitive
static const Ability Competitive = {
    .name = $("Competitive"),
    .description = $("Raises Sp. Atk by two stages if\nstats are lowered by an enemy."),
};

#undef CONTEXT
#define CONTEXT StrongJaw
static const Ability StrongJaw = {
    .name = $("Strong Jaw"),
    .description = $("Boosts the power of bite/fang\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT Refrigerate
static const Ability Refrigerate = {
    .name = $("Refrigerate"),
    .description = $("Normal-type moves become Ice-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT SweetVeil
static const Ability SweetVeil = {
    .name = $("Sweet Veil"),
    .description = $("This Pokémon and its ally are\nimmune to sleep."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT StanceChange
static const Ability StanceChange = {
    .name = $("Stance Change"),
    .description = $("Turns into Blade or Shield form\ndepending on move used."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT GaleWings
static const Ability GaleWings = {
    .name = $("Gale Wings"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's Flying-type moves."),
};

#undef CONTEXT
#define CONTEXT MegaLauncher
static const Ability MegaLauncher = {
    .name = $("Mega Launcher"),
    .description = $("Boosts Beam/Pump/Cannon/Shot/\nGun/Pulse, etc. moves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT GrassPelt
static const Ability GrassPelt = {
    .name = $("Grass Pelt"),
    .description = $("This Pokémon's Defense gets a\n1.5x boost in Grassy Terrain."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Symbiosis
static const Ability Symbiosis = {
    .name = $("Symbiosis"),
    .description = $("Passes own item to its ally if\nsaid ally consumes its item."),
};

#undef CONTEXT
#define CONTEXT ToughClaws
static const Ability ToughClaws = {
    .name = $("Tough Claws"),
    .description = $("Boosts the power of contact\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT Pixilate
static const Ability Pixilate = {
    .name = $("Pixilate"),
    .description = $("Normal-type moves become Fairy-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT Gooey
static const Ability Gooey = {
    .name = $("Gooey"),
    .description = $("Lowers Speed of enemies that\nmake contact with this Pokémon."),
};

#undef CONTEXT
#define CONTEXT Aerilate
static const Ability Aerilate = {
    .name = $("Aerilate"),
    .description = $("Normal-type moves become Flying-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT ParentalBond
static const Ability ParentalBond = {
    .name = $("Parental Bond"),
    .description = $("Moves hit twice. 1st hit at 100%\npower, 2nd hit at 25%."),
};

#undef CONTEXT
#define CONTEXT DarkAura
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_DARKAURA);
}
static const Ability DarkAura = {
    .name = $("Dark Aura"),
    .description = $("Boosts Dark moves by 1.33x for\nall while this Pokémon is out."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT FairyAura
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_FAIRYAURA);
}
static const Ability FairyAura = {
    .name = $("Fairy Aura"),
    .description = $("Boosts Fairy moves by 1.33x for\nall while this Pokémon is out."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT AuraBreak
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_AURABREAK);
}
static const Ability AuraBreak = {
    .name = $("Aura Break"),
    .description = $("Cancels aura abilities and makes\nthem 25% weaker instead."),
    .breakable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT PrimordialSea
ON_SWITCH {
    CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_RAIN_PRIMAL, TRUE))
    
    BattleScriptPushCursorAndCallback(BattleScript_PrimordialSeaActivates);
    return TRUE;
}
static const Ability PrimordialSea = {
    .name = $("Primordial Sea"),
    .description = $("Heavy Rain until switched out.\nFire-type moves are unusable."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DesolateLand
ON_SWITCH {
    CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_SUN_PRIMAL, TRUE))
    
    BattleScriptPushCursorAndCallback(BattleScript_DesolateLandActivates);
    return TRUE;
}
static const Ability DesolateLand = {
    .name = $("Desolate Land"),
    .description = $("Intense Sun until switched out.\nWater-type moves are unusable."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DeltaStream
ON_SWITCH {
    CHECK(TryChangeBattleWeather(battler, ENUM_WEATHER_STRONG_WINDS, TRUE))
    
    BattleScriptPushCursorAndCallback(BattleScript_DeltaStreamActivates);
    return TRUE;
}
static const Ability DeltaStream = {
    .name = $("Delta Stream"),
    .description = $("Strong Winds until switched out.\nWeather-based moves not usable."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Stamina
static const Ability Stamina = {
    .name = $("Stamina"),
    .description = $("Getting hit raises Def by +1.\nCritical hits maximize Defense."),
};

#undef CONTEXT
#define CONTEXT WimpOut
static const Ability WimpOut = {
    .name = $("Wimp Out"),
    .description = $("At 1/2 of max HP or below,\ninstantly switches out."),
};

#undef CONTEXT
#define CONTEXT EmergencyExit
static const Ability EmergencyExit = {
    .name = $("Emergency Exit"),
    .description = $("At 1/2 of max HP or below,\ninstantly switches out."),
};

#undef CONTEXT
#define CONTEXT WaterCompaction
static const Ability WaterCompaction = {
    .name = $("Water Compaction"),
    .description = $("Takes 1/2 dmg from Water-type\nmoves. +2 Def when hit by those."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Merciless
static const Ability Merciless = {
    .name = $("Merciless"),
    .description = $("100% crit if targetting slowed,\npoisoned, paralyzed, or bleeding foes."),
};

#undef CONTEXT
#define CONTEXT ShieldsDown
static const Ability ShieldsDown = {
    .name = $("Shields Down"),
    .description = $("At 1/2 of max HP or below,\ntransforms into Core form."),
    .unsuppressable = TRUE,
    .onSwitch = TryTransformAttacker,
};

#undef CONTEXT
#define CONTEXT Stakeout
static const Ability Stakeout = {
    .name = $("Stakeout"),
    .description = $("Deals double damage to opponents\nbeing switched in."),
};

#undef CONTEXT
#define CONTEXT WaterBubble
static const Ability WaterBubble = {
    .name = $("Water Bubble"),
    .description = $("Halves Fire dmg taken, no burns,\ndoubles power of its Water moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Steelworker
static const Ability Steelworker = {
    .name = $("Steelworker"),
    .description = $("Boosts the power of Steel-type\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT Berserk
static const Ability Berserk = {
    .name = $("Berserk"),
    .description = $("Boosts Sp. Atk by one stage when\nat 1/2 of max HP or lower."),
};

#undef CONTEXT
#define CONTEXT SlushRush
static const Ability SlushRush = {
    .name = $("Slush Rush"),
    .description = $("This Pokémon's Speed gets a\n1.5x boost in hail."),
};

#undef CONTEXT
#define CONTEXT LongReach
static const Ability LongReach = {
    .name = $("Long Reach"),
    .description = $("Doesn't make contact. Boosts\nPhys. non-contact moves by 1.2x."),
};

#undef CONTEXT
#define CONTEXT LiquidVoice
static const Ability LiquidVoice = {
    .name = $("Liquid Voice"),
    .description = $("Sound moves get a 1.2x boost\nand become Water if Normal."),
};

#undef CONTEXT
#define CONTEXT Triage
static const Ability Triage = {
    .name = $("Triage"),
    .description = $("Moves that have a healing effect\ngain +3 priority."),
};

#undef CONTEXT
#define CONTEXT Galvanize
static const Ability Galvanize = {
    .name = $("Galvanize"),
    .description = $("Normal-type moves become Elec.-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT SurgeSurfer
static const Ability SurgeSurfer = {
    .name = $("Surge Surfer"),
    .description = $("If Electric Terrain is active,\ngets a 1.5x Speed boost."),
};

#undef CONTEXT
#define CONTEXT Schooling
ON_SWITCH {
    CHECK(gBattleMons[battler].level >= 20)
    return TryTransformAttacker(ability, battler);
}
static const Ability Schooling = {
    .name = $("Schooling"),
    .description = $("If Lv. 20 or more: changes into\nSchool form until 1/4 HP or less."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Disguise
ON_SWITCH {
    CHECK(gBattleMons[battler].species == SPECIES_MIMIKYU_BUSTED || gBattleMons[battler].species == SPECIES_MIMIKYU_RAYQUAZA_BUSTED)
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

    {
    int newSpecies = gBattleMons[battler].species == SPECIES_MIMIKYU_BUSTED ? SPECIES_MIMIKYU : SPECIES_MIMIKYU_RAYQUAZA;
    UpdateAbilityStateIndicesForNewSpecies(battler, newSpecies);
    gBattleMons[battler].species = newSpecies;
    BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3NoPopup);
    }
    return TRUE;
}
static const Ability Disguise = {
    .name = $("Disguise"),
    .description = $("Protects once against an attack.\nRestores protection in fog."),
    .unsuppressable = TRUE,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT BattleBond
static const Ability BattleBond = {
    .name = $("Battle Bond"),
    .description = $("Transforms into Battle Bond form\nafter dealing a KO."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT PowerConstruct
static const Ability PowerConstruct = {
    .name = $("Power Construct"),
    .description = $("At 1/2 of max HP or below,\ntransforms into Complete form."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Corrosion
static const Ability Corrosion = {
    .name = $("Corrosion"),
    .description = $("Steel-types take Supereffective\nfrom Poison. Can poison any type."),
};

#undef CONTEXT
#define CONTEXT Comatose
ON_SWITCH {
    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_COMATOSE;
    BattleScriptPushCursorAndCallback(BattleScript_AnnounceStatusAbility);
    return TRUE;
}
static const Ability Comatose = {
    .name = $("Comatose"),
    .description = $("Can move, but is always asleep.\nImmune to status conditions."),
    .unsuppressable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT QueenlyMajesty
static const Ability QueenlyMajesty = {
    .name = $("Queenly Majesty"),
    .description = $("Protects itself and ally from\npriority moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT InnardsOut
static const Ability InnardsOut = {
    .name = $("Innards Out"),
    .description = $("If KO'd, deals as much damage as\nwhat the fatal attack dealt."),
};

#undef CONTEXT
#define CONTEXT Dancer
static const Ability Dancer = {
    .name = $("Dancer"),
    .description = $("Copies dance moves used by\nothers."),
};

#undef CONTEXT
#define CONTEXT Battery
static const Ability Battery = {
    .name = $("Battery"),
    .description = $("Grants a 1.3x power boost to\nally's Special attacks."),
};

#undef CONTEXT
#define CONTEXT Fluffy
static const Ability Fluffy = {
    .name = $("Fluffy"),
    .description = $("Takes 1/2 dmg from contact moves\nbut Fire moves hurt it 2x more."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Dazzling
static const Ability Dazzling = {
    .name = $("Dazzling"),
    .description = $("Protects itself and ally from\npriority moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SoulHeart
static const Ability SoulHeart = {
    .name = $("Soul-Heart"),
    .description = $("KOs dealt anywhere on the field\nraise Sp. Atk by one stage."),
};

#undef CONTEXT
#define CONTEXT TanglingHair
static const Ability TanglingHair = {
    .name = $("Tangling Hair"),
    .description = $("Lowers Speed of enemies that\nmake contact with this Pokémon."),
};

#undef CONTEXT
#define CONTEXT Receiver
static const Ability Receiver = {
    .name = $("Receiver"),
    .description = $("In Double Battles, copies its\nfainting partner's ability."),
};

#undef CONTEXT
#define CONTEXT PowerOfAlchemy
ON_SWITCH {
    int any = FALSE;
    for (int i = GetBattlerSide(BATTLE_OPPOSITE(battler)); i < gBattlersCount; i += 2)
    {
        FILTER(IsBattlerAlive(i))
        FILTER(ItemId_GetPocket(GetBattlerHoldEffect(i, FALSE)) == POCKET_BERRIES)
        any = TRUE;
        UpdateBattlerItem(i, ITEM_BLACK_SLUDGE);
        BattleScriptPushCursorAndCallback(BattleScript_End3);
        BattleScriptCall(BattleScript_PowerOfAlchemySludgeNoPopup);
    }
    CHECK(any)
    return TRUE;
}
static const Ability PowerOfAlchemy = {
    .name = $("Power of Alchemy"),
    .description = $("Transmutes berries on entry.\nTransmutes items when lost."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT BeastBoost
static const Ability BeastBoost = {
    .name = $("Beast Boost"),
    .description = $("Dealing a KO raises highest\ncalculated stat by one stage."),
};

#undef CONTEXT
#define CONTEXT RksSystem
static const Ability RksSystem = {
    .name = $("RKS System"),
    .description = $("Held Memory determines its type.\nAlso has Protean + Adaptability."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT ElectricSurge
ON_SWITCH {
    CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_ELECTRIC_TERRAIN, &gFieldTimers.terrainTimer))
    
    for (int i = 0; i < gBattlersCount; i++)
    {
        DisableSwitchInAbility(i, ABILITY_GENERATOR);
        DisableSwitchInAbility(i, ABILITY_ENERGIZED);
    }
    BattleScriptPushCursorAndCallback(BattleScript_ElectricSurgeActivates);
    return TRUE;
}
static const Ability ElectricSurge = {
    .name = $("Electro Surge"),
    .description = $("Casts Electric Terrain on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT PsychicSurge
ON_SWITCH {
    CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_PSYCHIC_TERRAIN, &gFieldTimers.terrainTimer))
    
    BattleScriptPushCursorAndCallback(BattleScript_PsychicSurgeActivates);
    return TRUE;
}
static const Ability PsychicSurge = {
    .name = $("Psychic Surge"),
    .description = $("Casts Psychic Terrain on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT MistySurge
ON_SWITCH {
    CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_MISTY_TERRAIN, &gFieldTimers.terrainTimer))
    
    BattleScriptPushCursorAndCallback(BattleScript_MistySurgeActivates);
    return TRUE;
}
static const Ability MistySurge = {
    .name = $("Misty Surge"),
    .description = $("Casts Misty Terrain on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT GrassySurge
ON_SWITCH {
    CHECK(TryChangeBattleTerrain(battler, STATUS_FIELD_GRASSY_TERRAIN, &gFieldTimers.terrainTimer))
    
    BattleScriptPushCursorAndCallback(BattleScript_GrassySurgeActivates);
    return TRUE;
}
static const Ability GrassySurge = {
    .name = $("Grassy Surge"),
    .description = $("Casts Grassy Terrain on entry.\nLasts 8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT FullMetalBody
static const Ability FullMetalBody = {
    .name = $("Full Metal Body"),
    .description = $("Immune to stat drops."),
};

#undef CONTEXT
#define CONTEXT ShadowShield
static const Ability ShadowShield = {
    .name = $("Shadow Shield"),
    .description = $("At full HP, halves damage taken\nfrom attacks"),
};

#undef CONTEXT
#define CONTEXT PrismArmor
static const Ability PrismArmor = {
    .name = $("Prism Armor"),
    .description = $("Takes 35% less damage from\nSuper-effective moves."),
};

#undef CONTEXT
#define CONTEXT Neuroforce
static const Ability Neuroforce = {
    .name = $("Neuroforce"),
    .description = $("Grants an additional 1.25x boost\nto Super-effective moves."),
};

#undef CONTEXT
#define CONTEXT IntrepidSword
ON_SWITCH {
    CHECK(CanRaiseStat(battler, STAT_ATK))

    SetStatChanger(STAT_ATK, 1);
    BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
    return TRUE;
}
static const Ability IntrepidSword = {
    .name = $("Intrepid Sword"),
    .description = $("On entry, raises Attack by one\nstage."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DauntlessShield
ON_SWITCH {
    CHECK(CanRaiseStat(battler, STAT_DEF))

    SetStatChanger(STAT_DEF, 1);
    BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
    return TRUE;
}
static const Ability DauntlessShield = {
    .name = $("Dauntless Shield"),
    .description = $("On entry, raises Defense by one\nstage."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Libero
static const Ability Libero = {
    .name = $("Libero"),
    .description = $("Before using a move, changes its\ntype to the move's type."),
};

#undef CONTEXT
#define CONTEXT BallFetch
static const Ability BallFetch = {
    .name = $("Ball Fetch"),
    .description = $("No effect in battle."),
};

#undef CONTEXT
#define CONTEXT CottonDown
static const Ability CottonDown = {
    .name = $("Cotton Down"),
    .description = $("Lowers the Speed of all foes\nby one stage when hit."),
};

#undef CONTEXT
#define CONTEXT PropellerTail
static const Ability PropellerTail = {
    .name = $("Propeller Tail"),
    .description = $("Isn't affected by target\nredirection."),
};

#undef CONTEXT
#define CONTEXT MirrorArmor
static const Ability MirrorArmor = {
    .name = $("Mirror Armor"),
    .description = $("Bounces back any stat drops\ninflicted by an enemy."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT GulpMissile
static const Ability GulpMissile = {
    .name = $("Gulp Missile"),
    .description = $("Gulps a prey after Dive/Surf.\nIf hit, shoots prey at enemy."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Stalwart
static const Ability Stalwart = {
    .name = $("Stalwart"),
    .description = $("Isn't affected by target\nredirection."),
};

#undef CONTEXT
#define CONTEXT SteamEngine
static const Ability SteamEngine = {
    .name = $("Steam Engine"),
    .description = $("Maximizes Speed if hit by a\nFire-type or Water-type attack."),
};

#undef CONTEXT
#define CONTEXT PunkRock
static const Ability PunkRock = {
    .name = $("Punk Rock"),
    .description = $("Sound moves deal 1.3x more dmg.\nTakes -50% dmg from sound moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SandSpit
static const Ability SandSpit = {
    .name = $("Sand Spit"),
    .description = $("If hit, summons a sandstorm that\nlasts 8 turns."),
};

#undef CONTEXT
#define CONTEXT IceScales
static const Ability IceScales = {
    .name = $("Ice Scales"),
    .description = $("Halves damage taken by Special\nmoves. Does NOT double SpDef."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Ripen
static const Ability Ripen = {
    .name = $("Ripen"),
    .description = $("Doubles resistance, healing and\nstat raises provided by Berries."),
};

#undef CONTEXT
#define CONTEXT IceFace
ON_SWITCH {
    CHECK(gBattleMons[battler].species == SPECIES_EISCUE_NOICE_FACE)
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_HAIL_ANY))
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

    UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_EISCUE);
    gBattleMons[battler].species = SPECIES_EISCUE;
    BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3NoPopup);
    return TRUE;
}
static const Ability IceFace = {
    .name = $("Ice Face"),
    .description = $("Protects once against an attack.\nRestores protection under hail."),
    .unsuppressable = TRUE,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT PowerSpot
static const Ability PowerSpot = {
    .name = $("Power Spot"),
    .description = $("Grants a 1.3x boost to ally's\nattacks."),
};

#undef CONTEXT
#define CONTEXT Mimicry
ON_SWITCH {
    CHECK(IsBattlerAlive(battler))
    CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

    TryToApplyMimicry(battler, FALSE);
    return TRUE;
}
static const Ability Mimicry = {
    .name = $("Mimicry"),
    .description = $("Changes type depending on active\nTerrain."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT ScreenCleaner
ON_SWITCH {
    CHECK(TryRemoveScreens(battler))

    return SwitchInAnnounce(B_MSG_SWITCHIN_SCREENCLEANER);
}
static const Ability ScreenCleaner = {
    .name = $("Screen Cleaner"),
    .description = $("Clears screens and Aurora Veil\nfrom both sides on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SteelySpirit
static const Ability SteelySpirit = {
    .name = $("Steely Spirit"),
    .description = $("Boosts own & ally's Steel-type\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT PerishBody
static const Ability PerishBody = {
    .name = $("Perish Body"),
    .description = $("If hit, casts Perish Song."),
};

#undef CONTEXT
#define CONTEXT WanderingSpirit
static const Ability WanderingSpirit = {
    .name = $("WandrngSprit"),
    .description = $("Trades ability with attacker on\ncontact."),
};

#undef CONTEXT
#define CONTEXT GorillaTactics
static const Ability GorillaTactics = {
    .name = $("Gorilla Tactics"),
    .description = $("Raises own Atk by 1.5x, but can\nonly use the first chosen move."),
};

#undef CONTEXT
#define CONTEXT NeutralizingGas
static const Ability NeutralizingGas = {
    .name = $("Neutralizing Gas"),
    .description = $("All abilities are nullified."),
    .unsuppressable = TRUE,
};

#undef CONTEXT
#define CONTEXT PastelVeil
ON_SWITCH {
    CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_SAFEGUARD)

    int side = GetBattlerSide(battler);
    gSideTimers[side].started.safeguard = TRUE;
    gSideStatuses[side] |= SIDE_STATUS_SAFEGUARD;
    gSideTimers[side].safeguardBattlerId = battler;
    gSideTimers[side].safeguardTimer = SCREEN_DURATION;
    BattleScriptPushCursorAndCallback(BattleScript_PastelVeilActivated);
    return TRUE;
}
static const Ability PastelVeil = {
    .name = $("Pastel Veil"),
    .description = $("Casts Safeguard on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT HungerSwitch
static const Ability HungerSwitch = {
    .name = $("HungerSwitch"),
    .description = $("Changes between Full and Hangry\nforms after each turn."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT QuickDraw
static const Ability QuickDraw = {
    .name = $("Quick Draw"),
    .description = $("30% chance to move first."),
};

#undef CONTEXT
#define CONTEXT UnseenFist
static const Ability UnseenFist = {
    .name = $("Unseen Fist"),
    .description = $("Its contact moves hit enemies,\neven if they protect themselves."),
};

#undef CONTEXT
#define CONTEXT CuriousMedicine
ON_SWITCH {
    CHECK(IsDoubleBattle())
    CHECK(IsBattlerAlive(BATTLE_PARTNER(battler)))
    CHECK(TryResetBattlerStatChanges(BATTLE_PARTNER(battler), RESET_ALL_STATS))
    
    gEffectBattler = BATTLE_PARTNER(battler);
    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_CURIOUS_MEDICINE;
    BattleScriptPushCursorAndCallback(BattleScript_SwitchInAbilityMsg);
    return TRUE;
}
static const Ability CuriousMedicine = {
    .name = $("CuriusMedicn"),
    .description = $("Resets its ally's stat changes\non entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Transistor
static const Ability Transistor = {
    .name = $("Transistor"),
    .description = $("Boosts the power of Electric-\ntype moves by 1.5x."),
};

#undef CONTEXT
#define CONTEXT DragonsMaw
static const Ability DragonsMaw = {
    .name = $("Dragon's Maw"),
    .description = $("Boosts the power of Dragon-type\nmoves by 1.5x."),
};

#undef CONTEXT
#define CONTEXT ChillingNeigh
static const Ability ChillingNeigh = {
    .name = $("ChillngNeigh"),
    .description = $("KOs raise Attack by one stage."),
};

#undef CONTEXT
#define CONTEXT GrimNeigh
static const Ability GrimNeigh = {
    .name = $("Grim Neigh"),
    .description = $("KOs raise Sp. Atk by one stage."),
};

#undef CONTEXT
#define CONTEXT AsOneIceRider
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_ASONE);
}
static const Ability AsOneIceRider = {
    .name = $("As One"),
    .description = $("Unnerve + Chilling Neigh."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT AsOneShadowRider
static const Ability AsOneShadowRider = {
    .name = $("As One"),
    .description = $("Unnerve + Grim Neigh."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    .onSwitch = AsOneIceRider.onSwitch,
};

#undef CONTEXT
#define CONTEXT Chloroplast
static const Ability Chloroplast = {
    .name = $("Chloroplast"),
    .description = $("Weather Ball, Solar Beam/Blade,\nGrowth act as if used in sun."),
};

#undef CONTEXT
#define CONTEXT Whiteout
static const Ability Whiteout = {
    .name = $("Whiteout"),
    .description = $("Ups highest attacking stat\nby 1.5x in hail."),
};

#undef CONTEXT
#define CONTEXT Pyromancy
static const Ability Pyromancy = {
    .name = $("Pyromancy"),
    .description = $("Moves inflict burn 5x as often."),
};

#undef CONTEXT
#define CONTEXT KeenEdge
static const Ability KeenEdge = {
    .name = $("Keen Edge"),
    .description = $("Boosts the power of slashing\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT PrismScales
static const Ability PrismScales = {
    .name = $("Prism Scales"),
    .description = $("Takes 30% less damage from\nSpecial attacks."),
};

#undef CONTEXT
#define CONTEXT PowerFists
static const Ability PowerFists = {
    .name = $("Power Fists"),
    .description = $("Iron Fist moves target Special\nDefense and get a 1.3x boost."),
};

#undef CONTEXT
#define CONTEXT SandSong
static const Ability SandSong = {
    .name = $("Sand Song"),
    .description = $("Sound moves get a 1.2x boost\nand become Ground if Normal."),
};

#undef CONTEXT
#define CONTEXT Rampage
static const Ability Rampage = {
    .name = $("Rampage"),
    .description = $("No recharge after a KO, if it\nusually would need to recharge."),
};

#undef CONTEXT
#define CONTEXT Vengeance
static const Ability Vengeance = {
    .name = $("Vengeance"),
    .description = $("Boosts Ghost-type moves by 1.2x,\nor 1.5x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT BlitzBoxer
static const Ability BlitzBoxer = {
    .name = $("Blitz Boxer"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's punching moves."),
};

#undef CONTEXT
#define CONTEXT AntarcticBird
static const Ability AntarcticBird = {
    .name = $("Antarctic Bird"),
    .description = $("Ice-type and Flying-type moves\nget a 1.3x power boost."),
};

#undef CONTEXT
#define CONTEXT Immolate
static const Ability Immolate = {
    .name = $("Immolate"),
    .description = $("Normal-type moves become Fire-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT Crystallize
static const Ability Crystallize = {
    .name = $("Crystallize"),
    .description = $("Rock-type moves become Ice-type\nmoves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT Electrocytes
static const Ability Electrocytes = {
    .name = $("Electrocytes"),
    .description = $("Boosts the power of Electric-\ntype moves by 1.25x."),
};

#undef CONTEXT
#define CONTEXT Aerodynamics
static const Ability Aerodynamics = {
    .name = $("Aerodynamics"),
    .description = $("Boosts Speed instead of being\nhit by Flying-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ChristmasSpirit
static const Ability ChristmasSpirit = {
    .name = $("Christmas Spirit"),
    .description = $("Takes 50% less damage if hail is\nactive."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ExploitWeakness
static const Ability ExploitWeakness = {
    .name = $("Exploit Weakness"),
    .description = $("Moves are 1.25x stronger on foes\naffected by a status condition."),
};

#undef CONTEXT
#define CONTEXT GroundShock
static const Ability GroundShock = {
    .name = $("Ground Shock"),
    .description = $("Target Grounds aren't immune to\nElectric but resist it instead."),
};

#undef CONTEXT
#define CONTEXT AncientIdol
static const Ability AncientIdol = {
    .name = $("Ancient Idol"),
    .description = $("Uses Def and Sp. Def instead of\nAtk and Sp. Atk when attacking."),
};

#undef CONTEXT
#define CONTEXT MysticPower
static const Ability MysticPower = {
    .name = $("Mystic Power"),
    .description = $("All moves gain the 1.5x power\nboost from STAB."),
};

#undef CONTEXT
#define CONTEXT Perfectionist
static const Ability Perfectionist = {
    .name = $("Perfectionist"),
    .description = $("Move BP < 51 BP: +1 to crit rate.\nMove BP < 26 BP: +1 priority too."),
};

#undef CONTEXT
#define CONTEXT GrowingTooth
static const Ability GrowingTooth = {
    .name = $("Growing Tooth"),
    .description = $("Raises Attack by one stage after\nusing a biting move."),
};

#undef CONTEXT
#define CONTEXT Inflatable
static const Ability Inflatable = {
    .name = $("Inflatable"),
    .description = $("Ups Def and Sp. Def by one stage\nif hit by Flying or Fire moves."),
};

#undef CONTEXT
#define CONTEXT AuroraBorealis
static const Ability AuroraBorealis = {
    .name = $("Aurora Borealis"),
    .description = $("Boosts the power of Ice-type\nmoves by 1.5x (due to STAB)."),
};

#undef CONTEXT
#define CONTEXT Avenger
static const Ability Avenger = {
    .name = $("Avenger"),
    .description = $("If a party Pokémon fainted last\nturn, next move gets 1.5x boost."),
};

#undef CONTEXT
#define CONTEXT LetsRoll
ON_SWITCH {
    CHECK(CanRaiseStat(battler, STAT_DEF))

    SetStatChanger(STAT_DEF, 1);
    gBattleMons[battler].status2 = STATUS2_DEFENSE_CURL;
    BattleScriptPushCursorAndCallback(BattleScript_BattlerInnateStatRaiseOnSwitchIn);
    return TRUE;
}
static const Ability LetsRoll = {
    .name = $("Let's Roll"),
    .description = $("Casts Defense Curl on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Aquatic
static const Ability Aquatic = {
    .name = $("Aquatic"),
    .description = $("Adds Water type to itself."),
};

#undef CONTEXT
#define CONTEXT LoudBang
static const Ability LoudBang = {
    .name = $("Loud Bang"),
    .description = $("Sound-based moves have 50%\nchance to confuse the foe."),
};

#undef CONTEXT
#define CONTEXT LeadCoat
static const Ability LeadCoat = {
    .name = $("Lead Coat"),
    .description = $("Takes 40% less from Phys. moves.\nThis Pokémon's Speed is 0.9x."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Amphibious
static const Ability Amphibious = {
    .name = $("Amphibious"),
    .description = $("Boosts the power of Water-type\nmoves by 1.5x (due to STAB)."),
};

#undef CONTEXT
#define CONTEXT Grounded
ON_SWITCH {
    return AddBattlerType(battler, TYPE_GROUND);
}
static const Ability Grounded = {
    .name = $("Grounded"),
    .description = $("Adds Ground type to itself."),
};

#undef CONTEXT
#define CONTEXT Earthbound
static const Ability Earthbound = {
    .name = $("Earthbound"),
    .description = $("Boosts Ground-type moves by\n1.2x, or 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT FightSpirit
static const Ability FightSpirit = {
    .name = $("Fighting Spirit"),
    .description = $("Normal-type moves become Fight.-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT FelineProwess
static const Ability FelineProwess = {
    .name = $("Feline Prowess"),
    .description = $("Doubles own Sp. Atk stat.\nBoosts raw stat, not base stat."),
};

#undef CONTEXT
#define CONTEXT CoilUp
ON_SWITCH {
    CHECK_NOT(gStatuses4[battler] & STATUS4_COILED)

    gStatuses4[battler] |= STATUS4_COILED;
    BattleScriptPushCursorAndCallback(BattleScript_BattlerCoiledUp);
    return TRUE;
}
static const Ability CoilUp = {
    .name = $("Coil Up"),
    .description = $("On entry, gives +1 priority once\nto the first biting move used."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Fossilized
static const Ability Fossilized = {
    .name = $("Fossilized"),
    .description = $("Halves dmg taken by Rock moves.\nBoosts own Rock moves by 1.2x."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MagicalDust
static const Ability MagicalDust = {
    .name = $("Magical Dust"),
    .description = $("If hit by a contact move, gives\nPsychic type to the attacker."),
};

#undef CONTEXT
#define CONTEXT Dreamcatcher
static const Ability Dreamcatcher = {
    .name = $("Dreamcatcher"),
    .description = $("Doubles move power if anyone on\nthe field is asleep."),
};

#undef CONTEXT
#define CONTEXT Nocturnal
static const Ability Nocturnal = {
    .name = $("Nocturnal"),
    .description = $("Boosts own Dark moves by 1.25x.\nTakes -25% dmg from Dark/Fairy."),
};

#undef CONTEXT
#define CONTEXT SelfSufficient
static const Ability SelfSufficient = {
    .name = $("Self Sufficient"),
    .description = $("Recovers 1/16 of max HP at the\nend of each turn."),
};

#undef CONTEXT
#define CONTEXT Tectonize
static const Ability Tectonize = {
    .name = $("Tectonize"),
    .description = $("Normal-type moves become Ground-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT IceAge
ON_SWITCH {
    return AddBattlerType(battler, TYPE_ICE);
}
static const Ability IceAge = {
    .name = $("Ice Age"),
    .description = $("Adds Ice type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT HalfDrake
ON_SWITCH {
    return AddBattlerType(battler, TYPE_DRAGON);
}
static const Ability HalfDrake = {
    .name = $("Half Drake"),
    .description = $("Adds Dragon type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Liquified
static const Ability Liquified = {
    .name = $("Liquified"),
    .description = $("Takes 1/2 dmg from contact moves\nbut Water moves hurt it 2x more."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Dragonfly
static const Ability Dragonfly = {
    .name = $("Dragonfly"),
    .description = $("Adds Dragon type to itself.\nAvoids Ground attacks."),
    .breakable = TRUE,
    .onSwitch = HalfDrake.onSwitch,
};

#undef CONTEXT
#define CONTEXT Dragonslayer
static const Ability Dragonslayer = {
    .name = $("Dragonslayer"),
    .description = $("Deals 1.5x damage to Dragons.\nTakes .5x damage from Dragons."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Mountaineer
static const Ability Mountaineer = {
    .name = $("Mountaineer"),
    .description = $("Immune to Rock-type attacks and\nStealth Rock damage."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Hydrate
static const Ability Hydrate = {
    .name = $("Hydrate"),
    .description = $("Normal-type moves become Water-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT Metallic
ON_SWITCH {
    return AddBattlerType(battler, TYPE_STEEL);
}
static const Ability Metallic = {
    .name = $("Metallic"),
    .description = $("Adds Steel type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Permafrost
static const Ability Permafrost = {
    .name = $("Permafrost"),
    .description = $("Takes 25% less damage from\nSuper-effective moves."),
};

#undef CONTEXT
#define CONTEXT PrimalArmor
static const Ability PrimalArmor = {
    .name = $("Primal Armor"),
    .description = $("Takes 50% less damage from\nSuper-effective moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT RagingBoxer
static const Ability RagingBoxer = {
    .name = $("Raging Boxer"),
    .description = $("Punching moves hit twice. 1st hit\nat 100% power, 2nd hit at 40%."),
};

#undef CONTEXT
#define CONTEXT AirBlower
ON_SWITCH {
    CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND)
    int side = GetBattlerSide(battler);
    gSideTimers[side].started.tailwind = TRUE;
    gSideStatuses[side] |= SIDE_STATUS_TAILWIND;
    gSideTimers[side].tailwindBattlerId = battler;
    gSideTimers[side].tailwindTimer = TAILWIND_DURATION_SHORT;
    
    DisableSwitchInAbility(battler, ABILITY_WIND_RIDER);
    DisableSwitchInAbility(BATTLE_PARTNER(battler), ABILITY_WIND_RIDER);

    BattleScriptPushCursorAndCallback(BattleScript_AirBlowerActivated);
    return TRUE;
}
static const Ability AirBlower = {
    .name = $("Air Blower"),
    .description = $("Casts a 3-turn Tailwind on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Juggernaut
static const Ability Juggernaut = {
    .name = $("Juggernaut"),
    .description = $("Paralysis-immune. Uses 20% of its\nDef when using a contact move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ShortCircuit
static const Ability ShortCircuit = {
    .name = $("Short Circuit"),
    .description = $("Boosts Elec.-type moves by 1.2x,\nor 1.5x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT MajesticBird
static const Ability MajesticBird = {
    .name = $("Majestic Bird"),
    .description = $("Boosts own Sp. Atk by 1.5x.\nBoosts raw stat, not base stat."),
};

#undef CONTEXT
#define CONTEXT Phantom
ON_SWITCH {
    return AddBattlerType(battler, TYPE_GHOST);
}
static const Ability Phantom = {
    .name = $("Phantom"),
    .description = $("Adds Ghost type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Intoxicate
static const Ability Intoxicate = {
    .name = $("Intoxicate"),
    .description = $("Normal-type moves become Poison-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT Impenetrable
static const Ability Impenetrable = {
    .name = $("Impenetrable"),
    .description = $("Only damaged by attacks."),
};

#undef CONTEXT
#define CONTEXT Hypnotist
static const Ability Hypnotist = {
    .name = $("Hypnotist"),
    .description = $("Hypnosis accuracy is 90% when\nused by this Pokémon."),
};

#undef CONTEXT
#define CONTEXT Overwhelm
static const Ability Overwhelm = {
    .name = $("Overwhelm"),
    .description = $("Hits Fairies with Dragon moves.\nImmune to Intimidate and Scare."),
};

#undef CONTEXT
#define CONTEXT Scare
static const Ability Scare = {
    .name = $("Scare"),
    .description = $("Lowers foes' Sp. Atk by one\nstage on entry."),
    .onSwitch = UseIntimidateClone,
};

#undef CONTEXT
#define CONTEXT MajesticMoth
ON_SWITCH {
    CHECK(ChangeStatBuffs(battler, 1, GetHighestStatId(battler, TRUE), MOVE_EFFECT_AFFECTS_USER, NULL))
    
    BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
    return TRUE;
}
static const Ability MajesticMoth = {
    .name = $("Majestic Moth"),
    .description = $("On entry, raises highest\ncalculated stat by one stage."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SoulEater
static const Ability SoulEater = {
    .name = $("Soul Eater"),
    .description = $("Dealing a KO heals 1/4 of this\nPokémon's max HP."),
};

#undef CONTEXT
#define CONTEXT SoulLinker
static const Ability SoulLinker = {
    .name = $("Soul Linker"),
    .description = $("Enemies take all the damage they\ndeal, same for this Pokémon."),
};

#undef CONTEXT
#define CONTEXT SweetDreams
static const Ability SweetDreams = {
    .name = $("Sweet Dreams"),
    .description = $("Heals 1/8 of max HP every turn\nif asleep. Immune to Bad Dreams."),
};

#undef CONTEXT
#define CONTEXT BadLuck
static const Ability BadLuck = {
    .name = $("Bad Luck"),
    .description = $("Foes hit the lowest damage roll,\nhave 5% less acc. and can't crit."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT HauntedSpirit
static const Ability HauntedSpirit = {
    .name = $("Haunted Spirit"),
    .description = $("When this Pokémon is KO'd, casts\na Curse on the attacker."),
};

#undef CONTEXT
#define CONTEXT ElectricBurst
static const Ability ElectricBurst = {
    .name = $("Electric Burst"),
    .description = $("Boosts own Elec. moves by 1.35x,\ntakes 10% of dmg dealt as recoil."),
};

#undef CONTEXT
#define CONTEXT RawWood
static const Ability RawWood = {
    .name = $("Raw Wood"),
    .description = $("Halves dmg taken by Grass moves.\nBoosts own Grass moves by 1.2x."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Solenoglyphs
static const Ability Solenoglyphs = {
    .name = $("Solenoglyphs"),
    .description = $("Biting moves have a 50% chance to\nbadly poison the target."),
};

#undef CONTEXT
#define CONTEXT SpiderLair
ON_SWITCH {
    CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)
    
    int side = BATTLE_OPPOSITE(battler);
    gSideTimers[side].started.spiderWeb = TRUE;
    gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
    gSideTimers[side].stickyWebTimer = 5;
    BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
    return TRUE;
}
static const Ability SpiderLair = {
    .name = $("Spider Lair"),
    .description = $("Casts Sticky Web on entry.\nLasts 5 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT FatalPrecision
static const Ability FatalPrecision = {
    .name = $("Fatal Precision"),
    .description = $("Super-effective moves never miss\nand get a 1.2x boost."),
};

#undef CONTEXT
#define CONTEXT FortKnox
static const Ability FortKnox = {
    .name = $("Fort Knox"),
    .description = $("Blocks most damage boosting\nand multihit abilities."),
};

#undef CONTEXT
#define CONTEXT Seaweed
static const Ability Seaweed = {
    .name = $("Seaweed"),
    .description = $("Takes 1/2 dmg from Fire if Grass,\ndoubles Grass dmg on Fire-types."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT PsychicMind
static const Ability PsychicMind = {
    .name = $("Psychic Mind"),
    .description = $("Boosts Psychic-type moves by\n1.2x, or 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT PoisonAbsorb
static const Ability PoisonAbsorb = {
    .name = $("Poison Absorb"),
    .description = $("Heals 25% of max HP when hit\nby a Poison-type move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Scavenger
static const Ability Scavenger = {
    .name = $("Scavenger"),
    .description = $("Dealing a KO heals 1/4 of this\nPokémon's max HP."),
};

#undef CONTEXT
#define CONTEXT TwistedDimension
ON_SWITCH {
    CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)
    
    gFieldTimers.started.trickRoom = TRUE;
    gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
    gFieldTimers.trickRoomTimer = TRICK_ROOM_DURATION_SHORT;
    BattleScriptPushCursorAndCallback(BattleScript_TwistedDimensionActivated);
    return TRUE;
}
static const Ability TwistedDimension = {
    .name = $("Twist. Dimension"),
    .description = $("Sets up Trick Room on\nentry, lasts 3 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT MultiHeaded
static const Ability MultiHeaded = {
    .name = $("Multi Headed"),
    .description = $("Hits as many times,\nas it has heads."),
};

#undef CONTEXT
#define CONTEXT NorthWind
ON_SWITCH {
    CHECK_NOT(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_AURORA_VEIL)
    
    int side = GetBattlerSide(battler);
    gSideTimers[side].started.auroraVeil = TRUE;
    gSideStatuses[side] |= SIDE_STATUS_AURORA_VEIL;
    if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_LIGHT_CLAY)
        gSideTimers[side].auroraVeilTimer = SCREEN_DURATION;
    else
        gSideTimers[side].auroraVeilTimer = SCREEN_DURATION_SHORT;
    BattleScriptPushCursorAndCallback(BattleScript_NorthWindActivated);

    return TRUE;
}
static const Ability NorthWind = {
    .name = $("North Wind"),
    .description = $("3 turns Aurora Veil on entry.\nImmune to Hail damage."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Overcharge
static const Ability Overcharge = {
    .name = $("Overcharge"),
    .description = $("Electric is super effective vs\nElectric. Can paralyze Electric."),
};

#undef CONTEXT
#define CONTEXT ViolentRush
ON_SWITCH {
    gVolatileStructs[battler].violentRush = gVolatileStructs[battler].started.violentRush = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_VIOLENT_RUSH);
}
static const Ability ViolentRush = {
    .name = $("Violent Rush"),
    .description = $("Boosts Speed by 50% + Attack\nby 20% on first turn."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT FlamingSoul
static const Ability FlamingSoul = {
    .name = $("Flaming Soul"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's Fire-type moves."),
};

#undef CONTEXT
#define CONTEXT SagePower
static const Ability SagePower = {
    .name = $("Sage Power"),
    .description = $("Ups Special Attack by 50%\nand locks move."),
};

#undef CONTEXT
#define CONTEXT BoneZone
static const Ability BoneZone = {
    .name = $("Bone Zone"),
    .description = $("Bone moves ignore immunities and\ndeal 2x on not very effective."),
};

#undef CONTEXT
#define CONTEXT WeatherControl
static const Ability WeatherControl = {
    .name = $("Weather Control"),
    .description = $("Negates all weather based\nmoves from enemies."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SpeedForce
static const Ability SpeedForce = {
    .name = $("Speed Force"),
    .description = $("Contact moves use 20% of its\nSpeed stat additionally."),
};

#undef CONTEXT
#define CONTEXT SeaGuardian
ON_SWITCH {
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_RAIN_ANY))

    int stat = GetHighestStatId(battler, TRUE);
    CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
    SetStatChanger(stat, 1);
    BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
    return TRUE;
}
static const Ability SeaGuardian = {
    .name = $("Sea Guardian"),
    .description = $("Ups highest stat by +1\non entry when it rains."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT MoltenDown
static const Ability MoltenDown = {
    .name = $("Molten Down"),
    .description = $("Fire-type is super effective\nagainst Rock-type."),
};

#undef CONTEXT
#define CONTEXT HyperAggressive
static const Ability HyperAggressive = {
    .name = $("Hyper Aggressive"),
    .description = $("Moves hit twice.\nSecond hit does 25% damage."),
};

#undef CONTEXT
#define CONTEXT Flock
static const Ability Flock = {
    .name = $("Flock"),
    .description = $("Boosts Flying-type moves by 1.2x,\nor 1.5x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT FieldExplorer
static const Ability FieldExplorer = {
    .name = $("Field Explorer"),
    .description = $("Boosts field moves by 50%.\nCut, Surf, Strength etc."),
};

#undef CONTEXT
#define CONTEXT Striker
static const Ability Striker = {
    .name = $("Striker"),
    .description = $("Boosts the power of kicking\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT FrozenSoul
static const Ability FrozenSoul = {
    .name = $("Frozen Soul"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's Ice-type moves."),
};

#undef CONTEXT
#define CONTEXT Predator
static const Ability Predator = {
    .name = $("Predator"),
    .description = $("Dealing a KO heals 1/4 of this\nPokémon's max HP."),
};

#undef CONTEXT
#define CONTEXT Looter
static const Ability Looter = {
    .name = $("Looter"),
    .description = $("Dealing a KO heals 1/4 of this\nPokémon's max HP."),
};

#undef CONTEXT
#define CONTEXT LunarEclipse
static const Ability LunarEclipse = {
    .name = $("Lunar Eclipse"),
    .description = $("Fairy & Dark gains STAB.\nHypnosis has 1.5x accuracy."),
};

#undef CONTEXT
#define CONTEXT SolarFlare
static const Ability SolarFlare = {
    .name = $("Solar Flare"),
    .description = $("Chloroplast + Immolate.\nFire moves gain STAB."),
};

#undef CONTEXT
#define CONTEXT PowerCore
static const Ability PowerCore = {
    .name = $("Power Core"),
    .description = $("The Pokémon uses +20% of its\nDefense or SpDef during moves."),
};

#undef CONTEXT
#define CONTEXT SightingSystem
static const Ability SightingSystem = {
    .name = $("Sighting System"),
    .description = $("Moves always hit. Moves last\nfor moves less than 80% accuracy."),
};

#undef CONTEXT
#define CONTEXT BadCompany
static const Ability BadCompany = {
    .name = $("Bad Company"),
    .description = $("Not implemented right now.\nHas no effect."),
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Opportunist
static const Ability Opportunist = {
    .name = $("Opportunist"),
    .description = $("If target has less than 1/2 HP,\nsingle-target moves get +1 prio."),
};

#undef CONTEXT
#define CONTEXT GiantWings
static const Ability GiantWings = {
    .name = $("Giant Wings"),
    .description = $("Boosts the power of wing, wind\nor air-based moves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT Momentum
static const Ability Momentum = {
    .name = $("Momentum"),
    .description = $("Contact moves use the Speed stat\nfor damage calculation."),
};

#undef CONTEXT
#define CONTEXT GripPincer
static const Ability GripPincer = {
    .name = $("Grip Pincer"),
    .description = $("50% chance to trap. Then ignores\nDefense & accuracy checks."),
};

#undef CONTEXT
#define CONTEXT BigLeaves
static const Ability BigLeaves = {
    .name = $("Big Leaves"),
    .description = $("Chloroplast + Chlorophyll + Leaf\nGuard + Harvest + Solar Power."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT PreciseFist
static const Ability PreciseFist = {
    .name = $("Precise Fist"),
    .description = $("Punching moves get +1 crit\nand 5x effect chance."),
};

#undef CONTEXT
#define CONTEXT Deadeye
static const Ability Deadeye = {
    .name = $("Deadeye"),
    .description = $("Never misses. Arrow and cannon\nmoves hit weakest defense."),
};

#undef CONTEXT
#define CONTEXT Artillery
static const Ability Artillery = {
    .name = $("Artillery"),
    .description = $("Mega Launcher moves always hit.\nSingle-target now hits both foes."),
};

#undef CONTEXT
#define CONTEXT Amplifier
static const Ability Amplifier = {
    .name = $("Amplifier"),
    .description = $("Ups sound moves by 30% and\nmakes them hit both foes."),
};

#undef CONTEXT
#define CONTEXT IceDew
static const Ability IceDew = {
    .name = $("Ice Dew"),
    .description = $("Boosts highest Atk instead of\nbeing hit by Ice-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SunWorship
ON_SWITCH {
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))

    int stat = GetHighestStatId(battler, TRUE);
    CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
    BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
    return TRUE;
}
static const Ability SunWorship = {
    .name = $("Sun Worship"),
    .description = $("Ups highest stat by +1\non entry when sunny."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Pollinate
static const Ability Pollinate = {
    .name = $("Pollinate"),
    .description = $("Normal-type moves become Bug-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT VolcanoRage
static const Ability VolcanoRage = {
    .name = $("Volcano Rage"),
    .description = $("Triggers 50 BP Eruption after\nusing a Fire-type move."),
};

#undef CONTEXT
#define CONTEXT ColdRebound
static const Ability ColdRebound = {
    .name = $("Cold Rebound"),
    .description = $("Attacks with Icy Wind\nwhen hit by a contact move."),
};

#undef CONTEXT
#define CONTEXT LowBlow
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_FEINT_ATTACK, 40);
}
static const Ability LowBlow = {
    .name = $("Low Blow"),
    .description = $("Attacks with 40BP Feint\nAttack on switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Nosferatu
static const Ability Nosferatu = {
    .name = $("Nosferatu"),
    .description = $("Contact moves do +20% damage\nand heal 1/2 of damage dealt."),
};

#undef CONTEXT
#define CONTEXT SpectralShroud
static const Ability SpectralShroud = {
    .name = $("Spectral Shroud"),
    .description = $("Spectralize + 30% chance\nto badly poison the foe."),
};

#undef CONTEXT
#define CONTEXT Discipline
static const Ability Discipline = {
    .name = $("Discipline"),
    .description = $("Rampage moves no longer trap you.\nCan't be confused or intimidated."),
};

#undef CONTEXT
#define CONTEXT Thundercall
static const Ability Thundercall = {
    .name = $("Thundercall"),
    .description = $("Triggers Smite at 20% power\nwhen using an Electric move."),
};

#undef CONTEXT
#define CONTEXT MarineApex
static const Ability MarineApex = {
    .name = $("Marine Apex"),
    .description = $("50% more damage to Water-\ntypes + Infiltrator."),
};

#undef CONTEXT
#define CONTEXT MightyHorn
static const Ability MightyHorn = {
    .name = $("Mighty Horn"),
    .description = $("Boosts the power of horn and\ndrill-based by 1.3x."),
};

#undef CONTEXT
#define CONTEXT HardenedSheath
static const Ability HardenedSheath = {
    .name = $("Hardened Sheath"),
    .description = $("Ups Attack by +1\nwhen using horn moves."),
};

#undef CONTEXT
#define CONTEXT ArcticFur
static const Ability ArcticFur = {
    .name = $("Arctic Fur"),
    .description = $("Weakens incoming physical\nand special moves by 35%."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Spectralize
static const Ability Spectralize = {
    .name = $("Spectralize"),
    .description = $("Normal-type moves become Ghost-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT Lethargy
ON_SWITCH {
    TryResetBattlerStatChanges(battler, RESET_ALL_STATS);
    gVolatileStructs[battler].slowStartTimer = 5;
    BattleScriptPushCursorAndCallback(BattleScript_LethargyEnters);
    return TRUE;
}
static const Ability Lethargy = {
    .name = $("Lethargy"),
    .description = $("Damage drops 20% each turn to 20%.\nResets on switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT IronBarrage
static const Ability IronBarrage = {
    .name = $("Iron Barrage"),
    .description = $("COMBINEs Mega Launcher\nwith Sighting System."),
};

#undef CONTEXT
#define CONTEXT SteelBarrel
static const Ability SteelBarrel = {
    .name = $("Steel Barrel"),
    .description = $("Immune to recoil damage, but not\nimmune to Explosion/crash dmg."),
};

#undef CONTEXT
#define CONTEXT PyroShells
static const Ability PyroShells = {
    .name = $("Pyro Shells"),
    .description = $("Triggers 50 BP Outburst after\nusing a Mega Launcher move."),
};

#undef CONTEXT
#define CONTEXT FungalInfection
static const Ability FungalInfection = {
    .name = $("Fungal Infection"),
    .description = $("Contact moves inflict\nLeech Seed on the target."),
};

#undef CONTEXT
#define CONTEXT Parry
static const Ability Parry = {
    .name = $("Parry"),
    .description = $("Counters contact with Mach\nPunch. Takes 20% less damage."),
};

#undef CONTEXT
#define CONTEXT Scrapyard
static const Ability Scrapyard = {
    .name = $("Scrapyard"),
    .description = $("Sets a layer of Spikes when hit\n(contact move)."),
};

#undef CONTEXT
#define CONTEXT LooseQuills
static const Ability LooseQuills = {
    .name = $("Loose Quills"),
    .description = $("Sets a layer of Spikes when hit\n(contact move)."),
};

#undef CONTEXT
#define CONTEXT ToxicDebris
static const Ability ToxicDebris = {
    .name = $("Toxic Debris"),
    .description = $("Sets a layer of Toxic Spikes\nwhen hit by contact moves."),
};

#undef CONTEXT
#define CONTEXT Roundhouse
static const Ability Roundhouse = {
    .name = $("Roundhouse"),
    .description = $("Kicks always hit.\nDamages foes' weaker defenses."),
};

#undef CONTEXT
#define CONTEXT Mineralize
static const Ability Mineralize = {
    .name = $("Mineralize"),
    .description = $("Normal-type moves become Rock-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT LooseRocks
static const Ability LooseRocks = {
    .name = $("Loose Rocks"),
    .description = $("Deploys Stealth Rocks\nwhen hit by contact."),
};

#undef CONTEXT
#define CONTEXT SpinningTop
static const Ability SpinningTop = {
    .name = $("Spinning Top"),
    .description = $("Fighting moves up speed +1\nand clear hazards."),
};

#undef CONTEXT
#define CONTEXT RetributionBlow
static const Ability RetributionBlow = {
    .name = $("Retribution Blow"),
    .description = $("Uses Hyper Beam if any foe\nuses an stat boosting move."),
};

#undef CONTEXT
#define CONTEXT Fearmonger
static const Ability Fearmonger = {
    .name = $("Fearmonger"),
    .description = $("Intimidate + Scare; 10%\npara chance on contact moves."),
    .onSwitch = UseIntimidateClone,
};

#undef CONTEXT
#define CONTEXT KingsWrath
static const Ability KingsWrath = {
    .name = $("King's Wrath"),
    .description = $("Lowering any stats on its\nside raises Atk and Def."),
};

#undef CONTEXT
#define CONTEXT QueensMourning
static const Ability QueensMourning = {
    .name = $("Queen's Mourning"),
    .description = $("Lowering any stats on its\nside raises SpAtk and SpDef."),
};

#undef CONTEXT
#define CONTEXT ToxicSpill
ON_SWITCH {
    BattleScriptPushCursorAndCallback(BattleScript_BattlerAnnouncedToxicSpill);
    return TRUE;
}
static const Ability ToxicSpill = {
    .name = $("Toxic Spill"),
    .description = $("Non-Poison-types take 1/8 dmg\nevery turn when on field."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DesertCloak
static const Ability DesertCloak = {
    .name = $("Desert Cloak"),
    .description = $("Protects its side from status\nand secondary effects in sand."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Draconize
static const Ability Draconize = {
    .name = $("Draconize"),
    .description = $("Normal-type moves become Dragon-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT PrettyPrincess
static const Ability PrettyPrincess = {
    .name = $("Pretty Princess"),
    .description = $("Does 50% more damage if the\ntarget has any lowered stat."),
};

#undef CONTEXT
#define CONTEXT SelfRepair
static const Ability SelfRepair = {
    .name = $("Self Repair"),
    .description = $("Self Sufficient + Natural Cure."),
};

#undef CONTEXT
#define CONTEXT AtomicBurst
static const Ability AtomicBurst = {
    .name = $("Atomic Burst"),
    .description = $("When hit super-effectively,\ntriggers 50 BP Hyper Beam."),
};

#undef CONTEXT
#define CONTEXT Hellblaze
static const Ability Hellblaze = {
    .name = $("Hellblaze"),
    .description = $("Boosts Fire-type moves by 1.3x,\nor 1.8x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT Riptide
static const Ability Riptide = {
    .name = $("Riptide"),
    .description = $("Boosts Water-type moves by 1.3x,\nor 1.8x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT ForestRage
static const Ability ForestRage = {
    .name = $("Forest Rage"),
    .description = $("Boosts Grass-type moves by 1.3x,\nor 1.8x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT PrimalMaw
static const Ability PrimalMaw = {
    .name = $("Primal Maw"),
    .description = $("Biting moves hit twice.\n2nd hit does 0.4x damage."),
};

#undef CONTEXT
#define CONTEXT SweepingEdge
static const Ability SweepingEdge = {
    .name = $("Sweeping Edge"),
    .description = $("Keen Edge moves always hit.\nSingle-target now hits both foes."),
};

#undef CONTEXT
#define CONTEXT GiftedMind
static const Ability GiftedMind = {
    .name = $("Gifted Mind"),
    .description = $("Nulls Psychic weakness;\nstatus moves always hit."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT HydroCircuit
static const Ability HydroCircuit = {
    .name = $("Hydro Circuit"),
    .description = $("Electric moves +50%;\nWater moves siphon 25% damage."),
};

#undef CONTEXT
#define CONTEXT Equinox
static const Ability Equinox = {
    .name = $("Equinox"),
    .description = $("Boosts Atk or SpAtk to\nmatch the higher value."),
};

#undef CONTEXT
#define CONTEXT Absorbant
static const Ability Absorbant = {
    .name = $("Absorbant"),
    .description = $("Drain moves recover +50%\nHP & apply Leech Seed."),
};

#undef CONTEXT
#define CONTEXT Clueless
static const Ability Clueless = {
    .name = $("Clueless"),
    .description = $("Negates Weather, Rooms\nand Terrains."),
    .unsuppressable = TRUE,
    .onSwitch = CloudNine.onSwitch,
};

#undef CONTEXT
#define CONTEXT CheatingDeath
ON_SWITCH {
    int uses = 2 - GetSingleUseAbilityCounter(battler, ability);
    CHECK(uses)

    if (uses == 1)
        BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
    else if (uses > 1) {
        ConvertIntToDecimalStringN(gBattleTextBuff4, uses, STR_CONV_MODE_LEFT_ALIGN, 2);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerHasNoDamageHits);
    }
    return TRUE;
}
static const Ability CheatingDeath = {
    .name = $("Cheating Death"),
    .description = $("Gets no damage for\nthe first two hits."),
    .persistent = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT CheapTactics
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_SCRATCH, 0);
}
static const Ability CheapTactics = {
    .name = $("Cheap Tactics"),
    .description = $("Attacks with Scratch\non switch-in."),
    CONTEXT_ON_SWITCH
};

#undef CONTEXT
#define CONTEXT Coward
ON_SWITCH {
    CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))
    
    SetSingleUseAbilityCounter(battler, ability, TRUE);
    gRoundStructs[battler].protectedThisTurn = TRUE;
    BattleScriptPushCursorAndCallback(BattleScript_BattlerIsProtectedForThisTurn);
    return TRUE;
}
static const Ability Coward = {
    .name = $("Coward"),
    .description = $("Sets up Protect on switch-in.\nOnly works once."),
    .persistent = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT VoltRush
static const Ability VoltRush = {
    .name = $("Volt Rush"),
    .description = $("At full HP, gives +1 priority to\nits Electric-type moves."),
};

#undef CONTEXT
#define CONTEXT DuneTerror
static const Ability DuneTerror = {
    .name = $("Dune Terror"),
    .description = $("Sand reduces damage by 35%.\nBoosts Ground moves by 20%."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT InfernalRage
static const Ability InfernalRage = {
    .name = $("Infernal Rage"),
    .description = $("Fire-type moves are boosted\nby 35% with 5% recoil."),
};

#undef CONTEXT
#define CONTEXT DualWield
static const Ability DualWield = {
    .name = $("Dual Wield"),
    .description = $("Mega Launcher and Keen Edge\nmoves hit twice for 70% damage."),
};

#undef CONTEXT
#define CONTEXT ElementalCharge
static const Ability ElementalCharge = {
    .name = $("Elemental Charge"),
    .description = $("20% chance to BRN/FRZ/PARA\nwith respective types."),
};

#undef CONTEXT
#define CONTEXT Ambush
static const Ability Ambush = {
    .name = $("Ambush"),
    .description = $("Guaranteed critical hit\non first turn."),
};

#undef CONTEXT
#define CONTEXT Atlas
ON_SWITCH {
    CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)
    
    gFieldTimers.started.gravity = TRUE;
    gFieldTimers.gravityTimer = GRAVITY_DURATION_EXTENDED;
    gFieldStatuses |= STATUS_FIELD_GRAVITY;
    BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
    return TRUE;
}
static const Ability Atlas = {
    .name = $("Atlas"),
    .description = $("Sets Gravity on entry for\n8 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Radiance
static const Ability Radiance = {
    .name = $("Radiance"),
    .description = $("+20% accuracy; Dark moves\nfail when user is present."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT JawsOfCarnage
static const Ability JawsOfCarnage = {
    .name = $("Jaws of Carnage"),
    .description = $("Devours 1/2 of the foe\nwhen defeating it."),
};

#undef CONTEXT
#define CONTEXT AngelsWrath
static const Ability AngelsWrath = {
    .name = $("Angel's Wrath"),
    .description = $("Drastically alters all\nof the users moves."),
};

#undef CONTEXT
#define CONTEXT PrismaticFur
static const Ability PrismaticFur = {
    .name = $("Prismatic Fur"),
    .description = $("Color Change + Protean +\nFur Coat + Ice Scales."),
};

#undef CONTEXT
#define CONTEXT ShockingJaws
static const Ability ShockingJaws = {
    .name = $("Shocking Jaws"),
    .description = $("Biting moves have 50% chance\nto paralyze the target."),
};

#undef CONTEXT
#define CONTEXT FaeHunter
static const Ability FaeHunter = {
    .name = $("Fae Hunter"),
    .description = $("Does 50% more damage to\nFairy-types."),
};

#undef CONTEXT
#define CONTEXT GravityWell
ON_SWITCH {
    CHECK_NOT(gFieldStatuses & STATUS_FIELD_GRAVITY)
    
    gFieldTimers.started.gravity = TRUE;
    gFieldTimers.gravityTimer = GRAVITY_DURATION;
    gFieldStatuses |= STATUS_FIELD_GRAVITY;
    BattleScriptPushCursorAndCallback(BattleScript_GravityStarts);
    return TRUE;
}
static const Ability GravityWell = {
    .name = $("Gravity Well"),
    .description = $("Sets Gravity on entry for\n5 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Evaporate
static const Ability Evaporate = {
    .name = $("Evaporate"),
    .description = $("Takes no damage and sets Mist\nif hit by water."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Lumberjack
static const Ability Lumberjack = {
    .name = $("Lumberjack"),
    .description = $("1.5x damage to Grass types."),
};

#undef CONTEXT
#define CONTEXT WellBakedBody
static const Ability WellBakedBody = {
    .name = $("Well Baked Body"),
    .description = $("Boosts Defense sharply instead\nof being hit by Fire-type moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Furnace
ON_SWITCH {
    CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_STEALTH_ROCK)
    CHECK(gSideTimers[GetBattlerSide(battler)].stealthRockType == TYPE_ROCK)
    CHECK(IsBattlerAlive(battler))
    CHECK(ChangeStatBuffs(battler, 2, STAT_SPEED, MOVE_EFFECT_AFFECTS_USER, NULL))

    BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
    return TRUE;
}
static const Ability Furnace = {
    .name = $("Furnace"),
    .description = $("User gains +2 Speed when\nwhen hit by rocks."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Electromorphosis
static const Ability Electromorphosis = {
    .name = $("Electromorphosis"),
    .description = $("Charges up when getting hit."),
};

#undef CONTEXT
#define CONTEXT RockyPayload
static const Ability RockyPayload = {
    .name = $("Rocky Payload"),
    .description = $("Boosts the power of Rock-type\nand throwing moves by 1.5x."),
};

#undef CONTEXT
#define CONTEXT EarthEater
static const Ability EarthEater = {
    .name = $("Earth Eater"),
    .description = $("Heals 25% of max HP when hit\nby a Ground move."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT LingeringAroma
static const Ability LingeringAroma = {
    .name = $("Lingering Aroma"),
    .description = $("If hit, makes the attacker's ability\nLingering Aroma."),
};

#undef CONTEXT
#define CONTEXT FairyTale
ON_SWITCH {
    return AddBattlerType(battler, TYPE_FAIRY);
}
static const Ability FairyTale = {
    .name = $("Fairy Tale"),
    .description = $("Adds Fairy type to itself."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT RagingMoth
static const Ability RagingMoth = {
    .name = $("Raging Moth"),
    .description = $("Fire moves hits twice,\nboth hits at 70% power."),
};

#undef CONTEXT
#define CONTEXT AdrenalineRush
static const Ability AdrenalineRush = {
    .name = $("Adrenaline Rush"),
    .description = $("KOs raise Speed by one stage."),
};

#undef CONTEXT
#define CONTEXT Archmage
static const Ability Archmage = {
    .name = $("Archmage"),
    .description = $("30% chance of adding a type\nrelated effect to each move."),
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Cryomancy
static const Ability Cryomancy = {
    .name = $("Cryomancy"),
    .description = $("Moves inflict frostbite\n5x as often."),
};

#undef CONTEXT
#define CONTEXT PhantomPain
static const Ability PhantomPain = {
    .name = $("Phantom Pain"),
    .description = $("Ghost type moves can hit normal\ntype pokemon for neutral damage."),
};

#undef CONTEXT
#define CONTEXT Purgatory
static const Ability Purgatory = {
    .name = $("Purgatory"),
    .description = $("Boosts Ghost-type moves by 1.3x,\nor 1.8x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT Emanate
static const Ability Emanate = {
    .name = $("Emanate"),
    .description = $("Normal-type moves become Psy.-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT KunoichiBlade
static const Ability KunoichiBlade = {
    .name = $("Kunoichi's Blade"),
    .description = $("Boosts weaker moves and increases\nthe frequency of multi-hit moves."),
};

#undef CONTEXT
#define CONTEXT MonkeyBusiness
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_TICKLE, 0);
}
static const Ability MonkeyBusiness = {
    .name = $("Monkey Business"),
    .description = $("Uses Tickle on entry."),
};

#undef CONTEXT
#define CONTEXT CombatSpecialist
static const Ability CombatSpecialist = {
    .name = $("Combat Specialist"),
    .description = $("Boosts the power of punching and\nkicking moves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT JunglesGuard
static const Ability JunglesGuard = {
    .name = $("Jungle's Guard"),
    .description = $("Grass-types on user side: immune\nto status/stat drops from enemy."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT HuntersHorn
static const Ability HuntersHorn = {
    .name = $("Hunter's Horn"),
    .description = $("Boost horn moves and heals\n1/4 HP when defeating an enemy."),
};

#undef CONTEXT
#define CONTEXT PixiePower
static const Ability PixiePower = {
    .name = $("Pixie Power"),
    .description = $("1.2x accuracy. Boosts Fairy\nmoves by 1.33x for all."),
    .onSwitch = FairyAura.onSwitch,
};

#undef CONTEXT
#define CONTEXT PlasmaLamp
static const Ability PlasmaLamp = {
    .name = $("Plasma Lamp"),
    .description = $("Boost accuracy & power of Fire\n& Electric type moves by 1.2x."),
};

#undef CONTEXT
#define CONTEXT MagmaEater
static const Ability MagmaEater = {
    .name = $("Magma Eater"),
    .description = $("Predator + Molten Down."),
};

#undef CONTEXT
#define CONTEXT SuperHotGoo
static const Ability SuperHotGoo = {
    .name = $("Super Hot Goo"),
    .description = $("Inflicts burn and lowers\nSpeed on contact."),
};

#undef CONTEXT
#define CONTEXT Nika
static const Ability Nika = {
    .name = $("Nika"),
    .description = $("Iron fist + Water moves\nfunction normally under sun."),
};

#undef CONTEXT
#define CONTEXT Archer
static const Ability Archer = {
    .name = $("Archer"),
    .description = $("Boosts the power of arrow moves\nby 1.3x."),
};

#undef CONTEXT
#define CONTEXT ColdPlasma
static const Ability ColdPlasma = {
    .name = $("Cold Plasma"),
    .description = $("Electric type moves now\ninflict burn instead of paralysis."),
};

#undef CONTEXT
#define CONTEXT SuperSlammer
static const Ability SuperSlammer = {
    .name = $("Super Slammer"),
    .description = $("Boosts the power of hammer and\nslamming moves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT InverseRoom
ON_SWITCH {
    CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)
    
    gFieldTimers.started.inverseRoom = TRUE;
    gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
    gFieldTimers.inverseRoomTimer = INVERSE_ROOM_DURATION_SHORT;
    BattleScriptPushCursorAndCallback(BattleScript_InversedRoomActivated);
    return TRUE;
}
static const Ability InverseRoom = {
    .name = $("Inversion"),
    .description = $("Sets up Inverse Room on\nentry, lasts 3 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Accelerate
static const Ability Accelerate = {
    .name = $("Accelerate"),
    .description = $("Moves that need a charge turn\nare now used instantly."),
};

#undef CONTEXT
#define CONTEXT FrostBurn
static const Ability FrostBurn = {
    .name = $("Frost Burn"),
    .description = $("Triggers 40BP Ice Beam after\nusing a Fire-type move."),
};

#undef CONTEXT
#define CONTEXT ItchyDefense
static const Ability ItchyDefense = {
    .name = $("Itchy Defense"),
    .description = $("Causes infestation when\nhit by a contact move."),
};

#undef CONTEXT
#define CONTEXT Generator
ON_SWITCH {
    CHECK_NOT(gStatuses3[battler] & STATUS3_CHARGED_UP)

    gStackBattler1 = battler;
    if (TERRAIN_HAS_EFFECT && gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
    {
        BattleScriptPushCursorAndCallback(BattleScript_GeneratorActivates);
        return TRUE;
    }
    else if (!GetSingleUseAbilityCounter(battler, ability))
    {
        SetSingleUseAbilityCounter(battler, ability, TRUE);
        BattleScriptPushCursorAndCallback(BattleScript_GeneratorActivates);
        return TRUE;
    }
    return FALSE;
}
static const Ability Generator = {
    .name = $("Generator"),
    .description = $("Charges up once on entry or\nwhen electric terrain is active."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT MoonSpirit
static const Ability MoonSpirit = {
    .name = $("Moon Spirit"),
    .description = $("Fairy & Dark gains STAB.\nMoonlight recovers 75% HP."),
};

#undef CONTEXT
#define CONTEXT DustCloud
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_SAND_ATTACK, 0);
}
static const Ability DustCloud = {
    .name = $("Dust Cloud"),
    .description = $("Attacks with Sand Attack\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT BerserkerRage
static const Ability BerserkerRage = {
    .name = $("Berserker Rage"),
    .description = $("Berserk + Rampage."),
};

#undef CONTEXT
#define CONTEXT Trickster
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_DISABLE, 0);
}
static const Ability Trickster = {
    .name = $("Trickster"),
    .description = $("Uses Disable\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SandGuard
static const Ability SandGuard = {
    .name = $("Sand Guard"),
    .description = $("Blocks priority and reduces\nspecial damage by 1/2 in sand."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT NaturalRecovery
static const Ability NaturalRecovery = {
    .name = $("Natural Recovery"),
    .description = $("Natural Cure + Regenerator."),
};

#undef CONTEXT
#define CONTEXT WindRider
ON_SWITCH {
    CHECK(gSideStatuses[GetBattlerSide(battler)] & SIDE_STATUS_TAILWIND)
    CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE)))

    BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityHighestAttackingStatRaiseOnSwitchIn);
    return TRUE;
}
static const Ability WindRider = {
    .name = $("Wind Rider"),
    .description = $("Increases attack in tailwind or\nwhen hit by wind move."),
    .breakable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SoothingAroma
ON_SWITCH {
    int anyStatus = FALSE;
    struct Pokemon *party;

    if (GetBattlerSide(battler) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;
        
    for (int i = 0; i < PARTY_SIZE; i++) {
        u32 status1 = GetMonData(&party[i], MON_DATA_STATUS);
        if (status1 & STATUS1_ANY) {
            anyStatus = TRUE;
            break;
        }
    }

    CHECK(anyStatus)

    BattleScriptPushCursorAndCallback(BattleScript_EffectSoothingAroma);
    return TRUE;
}
static const Ability SoothingAroma = {
    .name = $("Soothing Aroma"),
    .description = $("Cures party status on entry."),
};

#undef CONTEXT
#define CONTEXT PrimAndProper
static const Ability PrimAndProper = {
    .name = $("Prim and Proper"),
    .description = $("Wonder Skin + Cute Charm."),
};

#undef CONTEXT
#define CONTEXT SuperStrain
static const Ability SuperStrain = {
    .name = $("Super Strain"),
    .description = $("KOs lower Attack by +1.\nTake 25% recoil damage."),
};

#undef CONTEXT
#define CONTEXT TippingPoint
static const Ability TippingPoint = {
    .name = $("Tipping Point"),
    .description = $("Getting hit raises SpAtk.\nCritical hits maximize SpAtk."),
};

#undef CONTEXT
#define CONTEXT Enlightened
static const Ability Enlightened = {
    .name = $("Enlightened"),
    .description = $("Emanate + Inner Focus."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT PeacefulSlumber
static const Ability PeacefulSlumber = {
    .name = $("Peaceful Slumber"),
    .description = $("Sweet Dreams + Self Sufficient."),
};

#undef CONTEXT
#define CONTEXT Aftershock
static const Ability Aftershock = {
    .name = $("Aftershock"),
    .description = $("Triggers Magnitude 4-7 after\nusing a damaging move."),
};

#undef CONTEXT
#define CONTEXT FreezingPoint
static const Ability FreezingPoint = {
    .name = $("Freezing Point"),
    .description = $("30% chance to get frostbitten\non contact."),
};

#undef CONTEXT
#define CONTEXT CryoProficiency
static const Ability CryoProficiency = {
    .name = $("Cryo Proficiency"),
    .description = $("Triggers hail when hit. 30%\nchance to frostbite on contact."),
};

#undef CONTEXT
#define CONTEXT ArcaneForce
static const Ability ArcaneForce = {
    .name = $("Arcane Force"),
    .description = $("All moves gain STAB.\nUps “supereffective” by 10%."),
};

#undef CONTEXT
#define CONTEXT Doombringer
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_DOOM_DESIRE, 0);
}
static const Ability Doombringer = {
    .name = $("Doombringer"),
    .description = $("Uses Doom Desire\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Wishmaker
ON_SWITCH {
    int counter = GetSingleUseAbilityCounter(battler, ability);
    CHECK(counter < 3)
    CHECK(UseEntryMove(battler, ability, MOVE_WISH, 0))
    
    SetSingleUseAbilityCounter(battler, ability, counter + 1);
    return TRUE;
}
static const Ability Wishmaker = {
    .name = $("Wishmaker"),
    .description = $("Uses Wish on switch-in.\nThree uses per battle."),
    .persistent = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT YukiOnna
static const Ability YukiOnna = {
    .name = $("Yuki Onna"),
    .description = $("Scare + Intimidate.\n10% chance to infatuate on hit."),
    .onSwitch = UseIntimidateClone,
};

#undef CONTEXT
#define CONTEXT Suppress
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_TORMENT, 0);
}
static const Ability Suppress = {
    .name = $("Suppress"),
    .description = $("Casts Torment on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Refrigerator
static const Ability Refrigerator = {
    .name = $("Refrigerator"),
    .description = $("Refrigerate + Illuminate."),
};

#undef CONTEXT
#define CONTEXT HeavenAsunder
static const Ability HeavenAsunder = {
    .name = $("Heaven Asunder"),
    .description = $("Spacial Rend always crits.\nUps crit level by +1."),
};

#undef CONTEXT
#define CONTEXT PurifyingWaters
static const Ability PurifyingWaters = {
    .name = $("Purifying Waters"),
    .description = $("Hydration + Water Veil."),
    .onSwitch = WaterVeil.onSwitch,
};

#undef CONTEXT
#define CONTEXT Seaborne
static const Ability Seaborne = {
    .name = $("Seaborne"),
    .description = $("Drizzle + Swift Swim."),
    .onSwitch = Drizzle.onSwitch,
};

#undef CONTEXT
#define CONTEXT HighTide
static const Ability HighTide = {
    .name = $("High Tide"),
    .description = $("Triggers 50 BP Surf after\nusing a Water-type move."),
};

#undef CONTEXT
#define CONTEXT ChangeOfHeart
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_HEART_SWAP, 0);
}
static const Ability ChangeOfHeart = {
    .name = $("Change of Heart"),
    .description = $("Uses Heart Swap\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT MysticBlades
static const Ability MysticBlades = {
    .name = $("Mystic Blades"),
    .description = $("Keen edge moves become special\nand deal 30% more damage."),
};

#undef CONTEXT
#define CONTEXT Determination
static const Ability Determination = {
    .name = $("Determination"),
    .description = $("Ups Special Attack by 50%\nif suffering."),
};

#undef CONTEXT
#define CONTEXT Fertilize
static const Ability Fertilize = {
    .name = $("Fertilize"),
    .description = $("Normal-type moves become Grass-\ntype moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT PureLove
static const Ability PureLove = {
    .name = $("Pure Love"),
    .description = $("Infatuates on contact.\nHeal 25% damage vs infatuated."),
};

#undef CONTEXT
#define CONTEXT Fighter
static const Ability Fighter = {
    .name = $("Fighter"),
    .description = $("Boosts Fight.-type moves by 1.2x,\nor 1.5x when below 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT MyceliumMight
static const Ability MyceliumMight = {
    .name = $("Mycelium Might"),
    .description = $("Status moves ignore immunities\nbut go last."),
};

#undef CONTEXT
#define CONTEXT Telekinetic
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_TELEKINESIS, 0);
}
static const Ability Telekinetic = {
    .name = $("Telekinetic"),
    .description = $("Casts Telekinesis on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Combustion
static const Ability Combustion = {
    .name = $("Combustion"),
    .description = $("Boosts the power of Fire-type\nmoves by 1.5x."),
};

#undef CONTEXT
#define CONTEXT PonyPower
static const Ability PonyPower = {
    .name = $("Pony Power"),
    .description = $("Keen Edge + Mystic Blades."),
};

#undef CONTEXT
#define CONTEXT PowderBurst
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_POWDER, 0);
}
static const Ability PowderBurst = {
    .name = $("Powder Burst"),
    .description = $("Casts Powder on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Retriever
static const Ability Retriever = {
    .name = $("Retriever"),
    .description = $("Retrieves item on switch-out."),
};

#undef CONTEXT
#define CONTEXT MonsterMash
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_TRICK_OR_TREAT, 0);
}
static const Ability MonsterMash = {
    .name = $("Monster Mash"),
    .description = $("Casts Trick-or-Treat on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT TwoStep
static const Ability TwoStep = {
    .name = $("Two Step"),
    .description = $("Triggers 50BP Revelation Dance\nafter using a Dance move."),
};

#undef CONTEXT
#define CONTEXT Spiteful
static const Ability Spiteful = {
    .name = $("Spiteful"),
    .description = $("Reduces attacker's PP\non contact."),
};

#undef CONTEXT
#define CONTEXT Fortitude
static const Ability Fortitude = {
    .name = $("Fortitude"),
    .description = $("Boosts SpDef +1 when hit.\nMaxes SpDef on crit."),
};

#undef CONTEXT
#define CONTEXT Devourer
static const Ability Devourer = {
    .name = $("Devourer"),
    .description = $("Strong Jaw + Primal Maw."),
};

#undef CONTEXT
#define CONTEXT PhantomThief
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_SPECTRAL_THIEF, 40);
}
static const Ability PhantomThief = {
    .name = $("Phantom Thief"),
    .description = $("Attacks with 40BP Spectral Thief\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT EarlyGrave
static const Ability EarlyGrave = {
    .name = $("Early Grave"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's Ghost-type moves."),
};

#undef CONTEXT
#define CONTEXT Grappler
static const Ability Grappler = {
    .name = $("Grappler"),
    .description = $("Trapping moves last 6 turns.\nTrapping deals 1/6 HP."),
};

#undef CONTEXT
#define CONTEXT BassBoosted
static const Ability BassBoosted = {
    .name = $("Bass Boosted"),
    .description = $("Amplifier + Punk Rock."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT FlamingJaws
static const Ability FlamingJaws = {
    .name = $("Flaming Jaws"),
    .description = $("Biting moves have 50% chance\nto burn the target."),
};

#undef CONTEXT
#define CONTEXT MonsterHunter
static const Ability MonsterHunter = {
    .name = $("Monster Hunter"),
    .description = $("Deals 1.5x more damage to\nDark-types."),
};

#undef CONTEXT
#define CONTEXT CrownedSword
static const Ability CrownedSword = {
    .name = $("Crowned Sword"),
    .description = $("Intrepid Sword + Anger Point."),
    .onSwitch = IntrepidSword.onSwitch,
};

#undef CONTEXT
#define CONTEXT CrownedShield
static const Ability CrownedShield = {
    .name = $("Crowned Shield"),
    .description = $("Dauntless Shield + Stamina."),
    .onSwitch = DauntlessShield.onSwitch,
};

#undef CONTEXT
#define CONTEXT BerserkDna
ON_SWITCH {
    CHECK(CanRaiseStat(battler, GetHighestAttackingStatId(battler, TRUE)))
    if (CanBeConfused(battler))
    {
        gBattleMons[battler].status2 |= STATUS2_CONFUSION_TURN(3);
        BattleScriptPushCursorAndCallback(BattleScript_BerserkDNA);
    }
    else BattleScriptPushCursorAndCallback(BattleScript_BerserkDNANoConfusion);
    return TRUE;
}
static const Ability BerserkDna = {
    .name = $("Berserk DNA"),
    .description = $("Sharply ups highest attacking stat\nbut confuses on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT CrownedKing
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_CROWNEDKING);
}
static const Ability CrownedKing = {
    .name = $("Crowned King"),
    .description = $("Unnerve + Grim Neigh +\nChilling Neigh."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SnapTrapWhenHit
static const Ability SnapTrapWhenHit = {
    .name = $("Clap Trap"),
    .description = $("Counters contact with\n50BP Snap Trap."),
};

#undef CONTEXT
#define CONTEXT Permanence
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_PERMANENCE);
}
static const Ability Permanence = {
    .name = $("Permanence"),
    .description = $("Foes can't heal in any way."),
};

#undef CONTEXT
#define CONTEXT Hubris
static const Ability Hubris = {
    .name = $("Hubris"),
    .description = $("KOs raise SpAtk by one stage."),
};

#undef CONTEXT
#define CONTEXT CosmicDaze
static const Ability CosmicDaze = {
    .name = $("Cosmic Daze"),
    .description = $("2x damage vs confused. Enemies\ntake 2x confusion damage."),
};

#undef CONTEXT
#define CONTEXT MindsEye
static const Ability MindsEye = {
    .name = $("Mind's Eye"),
    .description = $("Hits Ghost-type Pokémon.\nAccuracy can't be lowered."),
};

#undef CONTEXT
#define CONTEXT BloodPrice
static const Ability BloodPrice = {
    .name = $("Blood Price"),
    .description = $("Does 30% more damage but\nlose 10% HP when attacking."),
};

#undef CONTEXT
#define CONTEXT SpikeArmor
static const Ability SpikeArmor = {
    .name = $("Spike Armor"),
    .description = $("30% chance to bleed\non contact or offense."),
};

#undef CONTEXT
#define CONTEXT VoodooPower
static const Ability VoodooPower = {
    .name = $("Voodoo Power"),
    .description = $("30% chance to bleed when\nhit by special attacks."),
};

#undef CONTEXT
#define CONTEXT ChromeCoat
static const Ability ChromeCoat = {
    .name = $("Chrome Coat"),
    .description = $("Reduces special damage taken by\n40%, but decreases Speed by 10%."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Banshee
static const Ability Banshee = {
    .name = $("Banshee"),
    .description = $("Sound moves get a 1.2x boost\nand become Ghost if Normal."),
};

#undef CONTEXT
#define CONTEXT WebSpinner
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_STRING_SHOT, 0);
}
static const Ability WebSpinner = {
    .name = $("Web Spinner"),
    .description = $("Uses String Shot\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT ShowdownMode
ON_SWITCH {
    gVolatileStructs[battler].showdownMode = gVolatileStructs[battler].started.showdownMode = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_SHOWDOWN_MODE);
}
static const Ability ShowdownMode = {
    .name = $("Showdown Mode"),
    .description = $("Ambush + Violent Rush."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SeedSower
static const Ability SeedSower = {
    .name = $("Seed Sower"),
    .description = $("Sets Grassy Terrain when hit.\nHeals party status when it does."),
};

#undef CONTEXT
#define CONTEXT Airborne
static const Ability Airborne = {
    .name = $("Airborne"),
    .description = $("Boosts own & ally's Flying-type\nmoves by 1.3x."),
};

#undef CONTEXT
#define CONTEXT Parroting
static const Ability Parroting = {
    .name = $("Parroting"),
    .description = $("Copies sound moves used by\nothers. Immune to sound."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SaltCircle
ON_SWITCH {
    int anyBlocked = FALSE;
    gBattlerTarget = BATTLE_OPPOSITE(battler);

    if (IsBattlerAlive(gBattlerTarget) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION)) {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
        gVolatileStructs[gBattlerTarget].battlerPreventingEscape = battler;
        anyBlocked = TRUE;
    }

    gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
    if (IsBattlerAlive(gBattlerTarget) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION)) {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
        gVolatileStructs[gBattlerTarget].battlerPreventingEscape = battler;
        anyBlocked = TRUE;
    }

    CHECK(anyBlocked)
    return SwitchInAnnounce(B_MSG_SWITCHIN_SALT_CIRCLE);
}
static const Ability SaltCircle = {
    .name = $("Salt Circle"),
    .description = $("Prevents opposing pokemon\nfrom fleeing on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT PurifyingSalt
static const Ability PurifyingSalt = {
    .name = $("Purifying Salt"),
    .description = $("Immune to status conditions.\nTake 1/2 damage from Ghost."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Protosynthesis
ON_SWITCH {
    if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY))
    {
        struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_WEATHER_ACTIVE };
        SetAbilityStateAs(battler, ability, (union AbilityStates) { .paradoxBoost = boost });
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_WEATHER;
        BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
        return TRUE;
    }
    
    if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY)
    {
        struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_BOOSTER_ENERGY };
        SetAbilityStateAs(battler, ability, (union AbilityStates) { .paradoxBoost = boost });
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
        RemoveItem(battler);
        BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
        return TRUE;
    }
    return FALSE;
}
static const Ability Protosynthesis = {
    .name = $("Protosynthesis"),
    .description = $("Boosts highest stat in Sun\nor with Booster Energy."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT QuarkDrive
ON_SWITCH {
    if (TERRAIN_HAS_EFFECT && gFieldStatuses & STATUS_FIELD_ELECTRIC_TERRAIN)
    {
        struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_WEATHER_ACTIVE };
        SetAbilityStateAs(battler, ability, (union AbilityStates) { .paradoxBoost = boost });
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_TERRAIN;
        BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
        return TRUE;
    }

    if (GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_BOOSTER_ENERGY)
    {
        struct ParadoxBoost boost = { .statId = GetHighestStatId(battler, TRUE), .source = PARADOX_BOOSTER_ENERGY };
        SetAbilityStateAs(battler, ability, (union AbilityStates) { .paradoxBoost = boost });
        SetStatChanger(boost.statId, 0);
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PARADOX_BOOST_ITEM;
        RemoveItem(battler);
        BattleScriptPushCursorAndCallback(BattleScript_ParadoxBoostActivates);
        return TRUE;
    }
    return FALSE;
}
static const Ability QuarkDrive = {
    .name = $("Quark Drive"),
    .description = $("Boosts highest stat in Electric\nTerrain or with Booster Energy."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT WindPower
static const Ability WindPower = {
    .name = $("Wind Power"),
    .description = $("Charges up when hit by wind\nmoves or Tailwind starts."),
};

#undef CONTEXT
#define CONTEXT Impulse
static const Ability Impulse = {
    .name = $("Impulse"),
    .description = $("Non-contact moves use the\nSpeed stat for damage."),
};

#undef CONTEXT
#define CONTEXT TerminalVelocity
static const Ability TerminalVelocity = {
    .name = $("Terminal Velocity"),
    .description = $("Special moves use 20% of its\nSpeed stat additionally."),
};

#undef CONTEXT
#define CONTEXT GuardDog
static const Ability GuardDog = {
    .name = $("Guard Dog"),
    .description = $("Can't be forced out.\nInverts Intimidate effects."),
};

#undef CONTEXT
#define CONTEXT AngerShell
static const Ability AngerShell = {
    .name = $("Anger Shell"),
    .description = $("Applies Shell Smash when\nreduced below 1/2 HP."),
};

#undef CONTEXT
#define CONTEXT Egoist
static const Ability Egoist = {
    .name = $("Egoist"),
    .description = $("Raises its own stats when\nfoes raise theirs."),
};

#undef CONTEXT
#define CONTEXT Subdue
static const Ability Subdue = {
    .name = $("Subdue"),
    .description = $("Doubles the power of\nstat dropping moves."),
};

#undef CONTEXT
#define CONTEXT ReadiedAction
ON_SWITCH {
    gVolatileStructs[battler].readiedAction = gVolatileStructs[battler].started.readiedAction = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_READIED_ACTION);
}
static const Ability ReadiedAction = {
    .name = $("Readied Action"),
    .description = $("Doubles attack on\nfirst turn."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DarkGaleWings
static const Ability DarkGaleWings = {
    .name = $("Stygian Rush"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's Dark-type moves."),
};

#undef CONTEXT
#define CONTEXT GuiltTrip
static const Ability GuiltTrip = {
    .name = $("Guilt Trip"),
    .description = $("Sharply lowers attacker's Attack\nand SpAtk when fainting."),
};

#undef CONTEXT
#define CONTEXT WaterGaleWings
static const Ability WaterGaleWings = {
    .name = $("Tidal Rush"),
    .description = $("At full HP, gives +1 priority to\nthis Pokémon's Water-type moves."),
};

#undef CONTEXT
#define CONTEXT ZeroToHero
ON_SWITCH {
    CHECK(gBattleMons[battler].species == SPECIES_PALAFIN)
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)

    UpdateAbilityStateIndicesForNewSpecies(battler, SPECIES_PALAFIN_HERO);
    gBattleMons[battler].species = SPECIES_PALAFIN_HERO;
    BattleScriptPushCursorAndCallback(BattleScript_AttackerFormChangeEnd3);
    return TRUE;
}
static const Ability ZeroToHero = {
    .name = $("Zero To Hero"),
    .description = $("Changes forms after\nswitching out."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Costar
ON_SWITCH {
    CHECK(IsBattlerAlive(BATTLE_PARTNER(battler)))
    
    int anyChanged = FALSE;
    for (int i = STAT_ATK; i < NUM_BATTLE_STATS; i++)
    {
        if (gBattleMons[battler].statStages[i] != gBattleMons[BATTLE_PARTNER(battler)].statStages[i])
        {
            gBattleMons[battler].statStages[i] = gBattleMons[BATTLE_PARTNER(battler)].statStages[i];
            anyChanged = TRUE;
        }
    }

    CHECK(anyChanged)
    return SwitchInAnnounce(B_MSG_SWITCHIN_COSTAR);
}
static const Ability Costar = {
    .name = $("Costar"),
    .description = $("Copies its ally's stat changes\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Commander
static const Ability Commander = {
    .name = $("Commander"),
    .description = $("Hops inside an allied Dondozo.\nBoosts its ally but can't act."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT EjectPackAbility
static const Ability EjectPackAbility = {
    .name = $("Tactical Retreat"),
    .description = $("Flees when stats are lowered."),
    .persistent = TRUE,
};

#undef CONTEXT
#define CONTEXT VengefulSpirit
static const Ability VengefulSpirit = {
    .name = $("Vengeful Spirit"),
    .description = $("Haunted Spirit + Vengeance."),
};

#undef CONTEXT
#define CONTEXT CudChew
static const Ability CudChew = {
    .name = $("Cud Chew"),
    .description = $("Eats berries again at the\nend of the next turn."),
};

#undef CONTEXT
#define CONTEXT ArmorTail
static const Ability ArmorTail = {
    .name = $("Armor Tail"),
    .description = $("Protects itself and ally from\npriority moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MindCrush
static const Ability MindCrush = {
    .name = $("Mind Crunch"),
    .description = $("Biting moves use SpAtk and\ndeal 30% more damage."),
};

#undef CONTEXT
#define CONTEXT SupremeOverlord
ON_SWITCH {
    CHECK(gFaintedMonCount[GetBattlerSide(battler)])

    return SwitchInAnnounce(B_MSG_SWITCHIN_SUPREME_OVERLORD);
}
static const Ability SupremeOverlord = {
    .name = $("Supreme Overlord"),
    .description = $("Each fainted ally increases\nAttack and SpAtk by 10%."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT IllWill
static const Ability IllWill = {
    .name = $("Ill Will"),
    .description = $("Deletes the PP of the move\nthat faints this Pokemon."),
};

#undef CONTEXT
#define CONTEXT FireScales
static const Ability FireScales = {
    .name = $("Fire Scales"),
    .description = $("Halves damage taken by Special\nmoves. Does NOT double SpDef."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WatchYourStep
ON_SWITCH {
    u8 targetSide = GetBattlerSide(BATTLE_OPPOSITE(battler));
    CHECK(gSideTimers[targetSide].spikesAmount < 3)

    gSideTimers[targetSide].spikesAmount = min(gSideTimers[targetSide].spikesAmount + 2, 3);
    gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
    BattleScriptPushCursorAndCallback(BattleScript_DoubleSpikesOnEntry);
    return TRUE;
}
static const Ability WatchYourStep = {
    .name = $("Watch Your Step"),
    .description = $("Spreads two layers of\nSpikes on switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT RapidResponse
ON_SWITCH {
    gVolatileStructs[battler].rapidResponse = gVolatileStructs[battler].started.rapidResponse = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_RAPID_RESPONSE);
}
static const Ability RapidResponse = {
    .name = $("Rapid Response"),
    .description = $("Boosts Speed by 50% + SpAtk\nby 20% on first turn."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DoubleIronBarbs
static const Ability DoubleIronBarbs = {
    .name = $("Sharp Edges"),
    .description = $("1/6 HP damage when touched."),
};

#undef CONTEXT
#define CONTEXT ThermalExchange
static const Ability ThermalExchange = {
    .name = $("Thermal Exchange"),
    .description = $("Ups Attack when hit by Fire.\nImmune to burn."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT GoodAsGold
static const Ability GoodAsGold = {
    .name = $("Good As Gold"),
    .description = $("Immune to all Status moves,\nunless whole field is affected."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SharingIsCaring
static const Ability SharingIsCaring = {
    .name = $("Sharing Is Caring"),
    .description = $("Stat changes are shared\nbetween all battlers."),
};

#undef CONTEXT
#define CONTEXT TabletsOfRuin
static const Ability TabletsOfRuin = {
    .name = $("Tablets Of Ruin"),
    .description = $("Lowers the Attack of\nother Pokemon by 25%."),
};

#undef CONTEXT
#define CONTEXT SwordOfRuin
static const Ability SwordOfRuin = {
    .name = $("Sword Of Ruin"),
    .description = $("Lowers the Defense of\nother Pokemon by 25%."),
};

#undef CONTEXT
#define CONTEXT VesselOfRuin
static const Ability VesselOfRuin = {
    .name = $("Vessel Of Ruin"),
    .description = $("Lowers the Special Attack of\nother Pokemon by 25%."),
};

#undef CONTEXT
#define CONTEXT BeadsOfRuin
static const Ability BeadsOfRuin = {
    .name = $("Beads Of Ruin"),
    .description = $("Lowers the Special Defense\nof other Pokemon by 25%."),
};

#undef CONTEXT
#define CONTEXT PermafrostClone
static const Ability PermafrostClone = {
    .name = $("Thick Skin"),
    .description = $("Takes 25% less damage from\nSuper-effective moves."),
};

#undef CONTEXT
#define CONTEXT Gallantry
ON_SWITCH {
    CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))

    BattleScriptPushCursorAndCallback(BattleScript_BattlerHasASingleNoDamageHit);
    return TRUE;
}
static const Ability Gallantry = {
    .name = $("Gallantry"),
    .description = $("Gets no damage for\nfirst hit."),
    .persistent = TRUE,
    .breakable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT OrichalcumPulse
static const Ability OrichalcumPulse = {
    .name = $("Orichalcum Pulse"),
    .description = $("Summons sun on entry.\nRaises Atk by 1.33x in sun."),
    .onSwitch = Drought.onSwitch,
};

#undef CONTEXT
#define CONTEXT SunBasking
static const Ability SunBasking = {
    .name = $("Sun Basking"),
    .description = $("Blocks priority and reduces\nphysical damage by 1/2 in sun."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WingedKing
static const Ability WingedKing = {
    .name = $("Winged King"),
    .description = $("Ups “supereffective” by 33%."),
};

#undef CONTEXT
#define CONTEXT HadronEngine
static const Ability HadronEngine = {
    .name = $("Hadron Engine"),
    .description = $("Field becomes Electric.\n+33% SpAtk in Electric Terrain."),
    .onSwitch = ElectricSurge.onSwitch,
};

#undef CONTEXT
#define CONTEXT IronSerpent
static const Ability IronSerpent = {
    .name = $("Iron Serpent"),
    .description = $("Ups “supereffective” by 33%."),
};

#undef CONTEXT
#define CONTEXT WeatherDoubleBoost
static const Ability WeatherDoubleBoost = {
    .name = $("Catastrophe"),
    .description = $("Sun boosts Water.\nRain boosts Fire."),
};

#undef CONTEXT
#define CONTEXT SweepingEdgePlus
static const Ability SweepingEdgePlus = {
    .name = $("Blademaster"),
    .description = $("Sweeping Edge + Keen Edge."),
};

#undef CONTEXT
#define CONTEXT CelestialBlessing
static const Ability CelestialBlessing = {
    .name = $("Celestial Blessing"),
    .description = $("Recovers 1/12 of its health each\nturn under Misty Terrain."),
};

#undef CONTEXT
#define CONTEXT MinionControl
static const Ability MinionControl = {
    .name = $("Minion Control"),
    .description = $("Moves hit an extra time for\neach healthy party member."),
};

#undef CONTEXT
#define CONTEXT MoltenBlades
static const Ability MoltenBlades = {
    .name = $("Molten Blades"),
    .description = $("Keen Edge + Keen Edge moves\nhave a 20% chance to burn."),
};

#undef CONTEXT
#define CONTEXT HauntingFrenzy
static const Ability HauntingFrenzy = {
    .name = $("Haunting Frenzy"),
    .description = $("20% chance to flinch the\nopponent. +1 speed on kill."),
};

#undef CONTEXT
#define CONTEXT NoiseCancel
static const Ability NoiseCancel = {
    .name = $("Noise Cancel"),
    .description = $("Protects the party from sound-\nbased moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT RadioJam
static const Ability RadioJam = {
    .name = $("Radio Jam"),
    .description = $("Sound-based moves have a 20%\nchance to inflict disable."),
};

#undef CONTEXT
#define CONTEXT Ole
static const Ability Ole = {
    .name = $("Olé!"),
    .description = $("30% chance to evade single-\ntarget moves."),
};

#undef CONTEXT
#define CONTEXT Malicious
static const Ability Malicious = {
    .name = $("Malicious"),
    .description = $("Lowers the foe's highest\nAttack and Defense stat."),
    .onSwitch = UseIntimidateClone,
};

#undef CONTEXT
#define CONTEXT DeadPower
static const Ability DeadPower = {
    .name = $("Dead Power"),
    .description = $("1.5x Attack boost. 20% chance\nto curse on contact moves."),
};

#undef CONTEXT
#define CONTEXT BrawlingWyvern
static const Ability BrawlingWyvern = {
    .name = $("Brawling Wyvern"),
    .description = $("No guard + Dragon type\nmoves become punching moves."),
};

#undef CONTEXT
#define CONTEXT MythicalArrows
static const Ability MythicalArrows = {
    .name = $("Mythical Arrows"),
    .description = $("Arrow moves become special\nand deal 30% more damage."),
};

#undef CONTEXT
#define CONTEXT Lawnmower
ON_SWITCH {
    CHECK(gFieldStatuses & STATUS_FIELD_TERRAIN_ANY)

    BattleScriptPushCursorAndCallback(BattleScript_Lawnmower);
    return TRUE;
}
static const Ability Lawnmower = {
    .name = $("Lawnmower"),
    .description = $("Removes terrain on switch-in.\nStat up if terrain removed."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Flourish
static const Ability Flourish = {
    .name = $("Flourish"),
    .description = $("Boosts Grass moves by 50% in\ngrassy terrain."),
};

#undef CONTEXT
#define CONTEXT DesertSpirit
static const Ability DesertSpirit = {
    .name = $("Desert Spirit"),
    .description = $("Summons sand on entry. Ground\nmoves hit airborne in sand."),
    .onSwitch = SandStream.onSwitch,
};

#undef CONTEXT
#define CONTEXT Contempt
static const Ability Contempt = {
    .name = $("Contempt"),
    .description = $("Ignores opposing stat changes.\nBoosts Attack when stat lowered."),
    .unaware = TRUE,
};

#undef CONTEXT
#define CONTEXT Aerialist
static const Ability Aerialist = {
    .name = $("Aerialist"),
    .description = $("Levitate + Flock."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT TeraShell
static const Ability TeraShell = {
    .name = $("Tera Shell"),
    .description = $("All hits will be not very effective\nwhile at full HP."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ToxicChain
static const Ability ToxicChain = {
    .name = $("Toxic Chain"),
    .description = $("Moves have a 30% chance to\nbadly poison the foe."),
};

#undef CONTEXT
#define CONTEXT ParasiticSpores
ON_SWITCH {
    CHECK_NOT(gVolatileStructs[battler].parasiticSpores)
    
    gVolatileStructs[battler].parasiticSpores = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_PARASITIC_SPORES);
}
static const Ability ParasiticSpores = {
    .name = $("Parasitic Spores"),
    .description = $("Deals 1/8 HP damage to non-\nGhost. Spreads on contact."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT PoisonPuppeteer
static const Ability PoisonPuppeteer = {
    .name = $("Poison Puppeteer"),
    .description = $("Poison also inflicts confusion."),
};

#undef CONTEXT
#define CONTEXT Entrance
static const Ability Entrance = {
    .name = $("Entrance"),
    .description = $("Confusion also inflicts\ninfatuation."),
};

#undef CONTEXT
#define CONTEXT Rejection
ON_SWITCH {
    CHECK_NOT(gFieldTimers.quashTimer)

    gFieldTimers.quashTimer = QUASH_DURATION;
    gFieldTimers.started.quash = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_REJECTION);
}
static const Ability Rejection = {
    .name = $("Rejection"),
    .description = $("Applies Quash on switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT AppleEnlightenment
static const Ability AppleEnlightenment = {
    .name = $("Apple Enlightenment"),
    .description = $("Fur coat + Magic Guard."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT BalloonBomber
static const Ability BalloonBomber = {
    .name = $("Balloon Bomb"),
    .description = $("Aftermath + Inflatable"),
};

#undef CONTEXT
#define CONTEXT FlamingMaw
static const Ability FlamingMaw = {
    .name = $("Flaming Maw"),
    .description = $("Strong Jaw + Flaming Jaws"),
};

#undef CONTEXT
#define CONTEXT Demolitionist
static const Ability Demolitionist = {
    .name = $("Demolitionist"),
    .description = $("Readied Action + Ignores Protect\n+ screens break on readied turn"),
    .onSwitch = ReadiedAction.onSwitch,
};

#undef CONTEXT
#define CONTEXT RockhardWill
static const Ability RockhardWill = {
    .name = $("Rockhard Will"),
    .description = $("Boosts Rock-type moves by 1.2x,\nor 1.5x when under 1/3 HP."),
};

#undef CONTEXT
#define CONTEXT FragrantDaze
static const Ability FragrantDaze = {
    .name = $("Fragrant Daze"),
    .description = $("30% chance to confuse on contact."),
};

#undef CONTEXT
#define CONTEXT LowVisibility
ON_SWITCH {
    if (TryChangeBattleWeather(battler, ENUM_WEATHER_FOG, TRUE))
    {
        BattleScriptPushCursorAndCallback(BattleScript_BadOmensActivates);
        return TRUE;
    }
    else if (gBattleWeather & WEATHER_PRIMAL_ANY && WEATHER_HAS_EFFECT)
    {
        BattleScriptPushCursorAndCallback(BattleScript_BlockedByPrimalWeatherEnd3);
        return NO_ANNOUNCE;
    }
    return FALSE;
}
static const Ability LowVisibility = {
    .name = $("Low Visibility"),
    .description = $("Summons Eerie Fog on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT OldMariner
static const Ability OldMariner = {
    .name = $("Old Mariner"),
    .description = $("Seaweed + Water STAB."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Ectoplasm
static const Ability Ectoplasm = {
    .name = $("Ectoplasm"),
    .description = $("Ups highest attacking stat\nby 1.5x in fog."),
};

#undef CONTEXT
#define CONTEXT BeautifulMusic
static const Ability BeautifulMusic = {
    .name = $("Beautiful Music"),
    .description = $("Sound-based moves have 50% chance\nto infatuate the foe."),
};

#undef CONTEXT
#define CONTEXT Surprise
static const Ability Surprise = {
    .name = $("Surprise!"),
    .description = $("Astonishes enemy priority users\nin fog."),
};

#undef CONTEXT
#define CONTEXT SnowSong
static const Ability SnowSong = {
    .name = $("Snow Song"),
    .description = $("Sound moves get a 1.2x boost\nand become Ice if Normal."),
};

#undef CONTEXT
#define CONTEXT GreaterSpirit
ON_SWITCH {
    CHECK(IsBattlerWeatherAffected(battler, WEATHER_FOG_ANY))

    int stat = GetHighestStatId(battler, TRUE);
    CHECK(ChangeStatBuffs(battler, 1, stat, MOVE_EFFECT_AFFECTS_USER, NULL))
    BattleScriptPushCursorAndCallback(BattleScript_AttackerAbilityStatRaiseEnd3);
    return TRUE;
}
static const Ability GreaterSpirit = {
    .name = $("Greater Spirit"),
    .description = $("Ups highest stat by +1\non entry in fog."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Resonance
static const Ability Resonance = {
    .name = $("Resonance"),
    .description = $("Sound moves cause the target to \nbleed."),
};

#undef CONTEXT
#define CONTEXT EtherealRush
static const Ability EtherealRush = {
    .name = $("Ethereal Rush"),
    .description = $("This Pokémon's Speed gets a\n1.5x boost in fog."),
};

#undef CONTEXT
#define CONTEXT CuteAntecedence
static const Ability CuteAntecedence = {
    .name = $("Cute Antecedence"),
    .description = $("At full HP, gives +1 priority to\nits Fairy-type moves."),
};

#undef CONTEXT
#define CONTEXT RecurringNightmare
static const Ability RecurringNightmare = {
    .name = $("Shallow Grave"),
    .description = $("Revives at 25% HP once after\nfainting in fog."),
    .persistent = TRUE,
};

#undef CONTEXT
#define CONTEXT MenacingSituation
static const Ability MenacingSituation = {
    .name = $("Menacing Situation"),
    .description = $("20% chance to Fear on contact.\nAlso works on offense."),
};

#undef CONTEXT
#define CONTEXT ShinyLightning
static const Ability ShinyLightning = {
    .name = $("Shiny Lightning"),
    .description = $("Grants a 1.2x accuracy boost.\nThunder never misses."),
};

#undef CONTEXT
#define CONTEXT Terrify
static const Ability Terrify = {
    .name = $("Terrify"),
    .description = $("Lowers foes' Sp. Atk by two\nstages on entry."),
    .onSwitch = UseIntimidateClone,
};

#undef CONTEXT
#define CONTEXT IceDownfall
static const Ability IceDownfall = {
    .name = $("Ice Downfall"),
    .description = $("Counters contact with\n60BP Icicle Crash."),
};

#undef CONTEXT
#define CONTEXT LastStand
static const Ability LastStand = {
    .name = $("Last Stand"),
    .description = $("Def and SpDef increase as\nHP drops. Max 1.6x."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT PyroclasticFlow
static const Ability PyroclasticFlow = {
    .name = $("Pyroclastic Flow"),
    .description = $("Molten Down + Corrosion."),
};

#undef CONTEXT
#define CONTEXT BloodBath
static const Ability BloodBath = {
    .name = $("Blood Bath"),
    .description = $("Immune to bleed. Inflict fear\nwhen inflicting bleed."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT BattleAura
static const Ability BattleAura = {
    .name = $("Battle Aura"),
    .description = $("Sharply increases the critical\nhit rate for all while on the field."),
};

#undef CONTEXT
#define CONTEXT Bloodlust
static const Ability Bloodlust = {
    .name = $("Bloodlust"),
    .description = $("Blood Bath + Soul Eater."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT PiercingSolo
static const Ability PiercingSolo = {
    .name = $("Piercing Solo"),
    .description = $("Sound moves have a 30%\nchance to cause bleeding."),
};

#undef CONTEXT
#define CONTEXT Rhythmic
static const Ability Rhythmic = {
    .name = $("Rhythmic"),
    .description = $("Deals 10% more damage for\neach repeated move use."),
};

#undef CONTEXT
#define CONTEXT ChunkyBassLine
static const Ability ChunkyBassLine = {
    .name = $("Chunky Bass Line"),
    .description = $("Triggers a 40BP Earthquake\nafter using a sound move."),
};

#undef CONTEXT
#define CONTEXT DualHammer
static const Ability DualHammer = {
    .name = $("Jackhammer"),
    .description = $("Super Slammer moves hit twice\nfor 70% damage."),
};

#undef CONTEXT
#define CONTEXT DentingBlows
static const Ability DentingBlows = {
    .name = $("Denting Blows"),
    .description = $("Hammer moves lower Defense."),
};

#undef CONTEXT
#define CONTEXT IceColdHunter
static const Ability IceColdHunter = {
    .name = $("Ice Cold Hunter"),
    .description = $("Ice-type moves hit twice in hail."),
};

#undef CONTEXT
#define CONTEXT SoulCrusher
static const Ability SoulCrusher = {
    .name = $("Soul Crusher"),
    .description = $("Hammer moves become Special\nand get a 1.1x power boost."),
};

#undef CONTEXT
#define CONTEXT ArcFlash
static const Ability ArcFlash = {
    .name = $("Arc Flash"),
    .description = $("50% chance to burn when hit or\nparalyze when dealing damage."),
};

#undef CONTEXT
#define CONTEXT Unicorn
static const Ability Unicorn = {
    .name = $("Unicorn"),
    .description = $("Mighty Horn + Dazzling."),
};

#undef CONTEXT
#define CONTEXT OnTheProwl
ON_SWITCH {
    gVolatileStructs[battler].onTheProwl = gVolatileStructs[battler].started.onTheProwl = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_ON_THE_PROWL);
}
static const Ability OnTheProwl = {
    .name = $("On the Prowl"),
    .description = $("+1 priority for the first turn.\nNegative priority becomes +0."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Pretentious
static const Ability Pretentious = {
    .name = $("Pretentious"),
    .description = $("Dealing a KO raises Crit by\none stage."),
};

#undef CONTEXT
#define CONTEXT VenoblazePincers
static const Ability VenoblazePincers = {
    .name = $("Venoblaze Pincers"),
    .description = $("1.2x boost to physical moves and\n20% chance to Burn or Poison."),
};

#undef CONTEXT
#define CONTEXT EternalBlessing
static const Ability EternalBlessing = {
    .name = $("Eternal Blessing"),
    .description = $("Celestial Blessing + Regenerator."),
    .persistent = TRUE,
};

#undef CONTEXT
#define CONTEXT SugarRush
static const Ability SugarRush = {
    .name = $("Sugar Rush"),
    .description = $("Gluttony + eats foe's berry when\nhitting with contact move."),
};

#undef CONTEXT
#define CONTEXT PeacefulRest
static const Ability PeacefulRest = {
    .name = $("Rest in Peace"),
    .description = $("Heals 1/8 of max HP every turn\nin fog."),
};

#undef CONTEXT
#define CONTEXT WhiteNoise
static const Ability WhiteNoise = {
    .name = $("White Noise"),
    .description = $("Static + Rest in Peace."),
};

#undef CONTEXT
#define CONTEXT SmokeyManeuvers
static const Ability SmokeyManeuvers = {
    .name = $("Smokey Maneuvers"),
    .description = $("Evasion is boosted by 1.25x\nin fog."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT Tag
static const Ability Tag = {
    .name = $("Tag"),
    .description = $("Attacks switching opponents\nwith a 20BP Pursuit."),
};

#undef CONTEXT
#define CONTEXT PowerMetal
static const Ability PowerMetal = {
    .name = $("Power Metal"),
    .description = $("Sound moves get a 1.2x boost\nand become Steel if Normal."),
};

#undef CONTEXT
#define CONTEXT PowerEdge
static const Ability PowerEdge = {
    .name = $("Power Edge"),
    .description = $("Keen Edge moves target Special\nDefense and get a 1.3x boost."),
};

#undef CONTEXT
#define CONTEXT Superconductor
static const Ability Superconductor = {
    .name = $("Superconductor"),
    .description = $("Steel-type moves become Electric\n-type moves and get a 1.1x boost."),
};

#undef CONTEXT
#define CONTEXT UltraInstinct
static const Ability UltraInstinct = {
    .name = $("Ultra Instinct"),
    .description = $("Counters contact with Vacuum\nWave. Takes 20% less damage."),
};

#undef CONTEXT
#define CONTEXT UnlockedPotential
static const Ability UnlockedPotential = {
    .name = $("Unlocked Potential"),
    .description = $("Inner Focus + Berserk."),
};

#undef CONTEXT
#define CONTEXT HigherRank
static const Ability HigherRank = {
    .name = $("Higher Rank"),
    .description = $("Priority moves get a 1.2x boost."),
};

#undef CONTEXT
#define CONTEXT FuneralPyre
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_FUNERAL_PYRE);
}
static const Ability FuneralPyre = {
    .name = $("Funeral Pyre"),
    .description = $("Non-Ghost and Dark-types\ntake 1/4 damage every turn."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT FlameBubble
static const Ability FlameBubble = {
    .name = $("Flame Bubble"),
    .description = $("Water Bubble + Flaming Soul."),
};

#undef CONTEXT
#define CONTEXT ElementalVortex
static const Ability ElementalVortex = {
    .name = $("Elemental Vortex"),
    .description = $("Flash Fire + Water Absorb."),
};

#undef CONTEXT
#define CONTEXT SnowyWrath
static const Ability SnowyWrath = {
    .name = $("Snowy Wrath"),
    .description = $("Snow Warning + Whiteout."),
};

#undef CONTEXT
#define CONTEXT PatternChange
static const Ability PatternChange = {
    .name = $("Pattern Change"),
    .description = $("Changes type depending on the\nmove it's about to use."),
};

#undef CONTEXT
#define CONTEXT NoTurningBack
static const Ability NoTurningBack = {
    .name = $("No Turning Back"),
    .description = $("Boosts all stats but can't retreat\nwhen below 1/2 max HP."),
};

#undef CONTEXT
#define CONTEXT FlammableCoat
static const Ability FlammableCoat = {
    .name = $("Flammable Coat"),
    .description = $("Changes forms when using or\nhit by a Fire-type move."),
    .unsuppressable = TRUE,
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT DracoMorale
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_DRAGON_CHEER, 0);
}
static const Ability DracoMorale = {
    .name = $("Draco Morale"),
    .description = $("Uses Dragon Cheer\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT BadOmen
static const Ability BadOmen = {
    .name = $("Bad Omen"),
    .description = $("Foes min roll and may miss.\nTakes 1/4 damage from crits."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MoshPit
static const Ability MoshPit = {
    .name = $("Mosh Pit"),
    .description = $("Ally's attacks get a 1.25x boost.\n1.5x if attack causes recoil."),
};

#undef CONTEXT
#define CONTEXT BloodStain
ON_SWITCH {
    return SwitchInAnnounce(B_MSG_SWITCHIN_BLOOD_STAIN);
}
static const Ability BloodStain = {
    .name = $("Blood Stain"),
    .description = $("Bleeds if not immune. Can't get\nother status. Spreads on contact."),
    .unsuppressable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT BloodStigma
static const Ability BloodStigma = {
    .name = $("Blood Stigma"),
    .description = $("Immune to status. Gets a 50%\nboost vs bleeding foes."),
    .unsuppressable = TRUE,
};

#undef CONTEXT
#define CONTEXT MaximumAcceleration
static const Ability MaximumAcceleration = {
    .name = $("Max Acceleration"),
    .description = $("Moves use the Speed stat\nfor damage calculations."),
};

#undef CONTEXT
#define CONTEXT Sidewinder
static const Ability Sidewinder = {
    .name = $("Sidewinder"),
    .description = $("First biting move each entry gets\n+1 priority. Resets on KO."),
    .onSwitch = CoilUp.onSwitch,
};

#undef CONTEXT
#define CONTEXT Petrify
ON_SWITCH {
    int loweredStats = 0;
    int intimidated = UseIntimidateClone(battler, ability);
    for (int i = BATTLE_OPPOSITE(GET_BATTLER_SIDE(battler)); i < gBattlersCount; i += 2)
    {
        if (!IsBattlerAlive(i)) continue;
        loweredStats |= TryResetBattlerStatChanges(i, RESET_STAT_BUFFS);
    }

    if (loweredStats)
    {
        BattleScriptPushCursorAndCallback(BattleScript_Petrify);
    }
    return intimidated || loweredStats;
}
static const Ability Petrify = {
    .name = $("Petrify"),
    .description = $("Clears stat buffs then lowers\nspeed by one stage on entry."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Fluffiest
static const Ability Fluffiest = {
    .name = $("Fluffiest"),
    .description = $("Quarters contact damage taken.\n4x weak to fire."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WayOfPrecision
static const Ability WayOfPrecision = {
    .name = $("Way of Precision"),
    .description = $("Inner Focus + Precise Fist."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT WayOfSwiftness
static const Ability WayOfSwiftness = {
    .name = $("Way of Swiftness"),
    .description = $("Pretentious + Swift Swim."),
};

#undef CONTEXT
#define CONTEXT AtomicPunch
static const Ability AtomicPunch = {
    .name = $("Atomic Punch"),
    .description = $("Iron Fist + Steelworker."),
};

#undef CONTEXT
#define CONTEXT IronGiant
static const Ability IronGiant = {
    .name = $("Iron Giant"),
    .description = $("Heatproof + Juggernaut."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MasterHand
static const Ability MasterHand = {
    .name = $("Master Hand"),
    .description = $("Mega Launcher + Rampage."),
};

#undef CONTEXT
#define CONTEXT FinalBlow
static const Ability FinalBlow = {
    .name = $("Final Blow"),
    .description = $("Unseen Fist + Fatal Precision."),
};

#undef CONTEXT
#define CONTEXT Hospitality
static const Ability Hospitality = {
    .name = $("Hospitality"),
    .description = $("Heals partner for 25% of its max\nHP on switch-in."),
};

#undef CONTEXT
#define CONTEXT ButterUp
ON_SWITCH {
    return Hospitality.onSwitch(ability, battler) | SoothingAroma.onSwitch(ability, battler);
}
static const Ability ButterUp = {
    .name = $("Butter Up"),
    .description = $("Hospitality + Soothing Aroma"),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT VitalityStrike
static const Ability VitalityStrike = {
    .name = $("Vitality Strike"),
    .description = $("Heals for 10% of the damage\ndealt by punching moves."),
};

#undef CONTEXT
#define CONTEXT HugeWings
static const Ability HugeWings = {
    .name = $("Huge Wings"),
    .description = $("Giant Wings + Levitate."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT SwordOfDamnation
static const Ability SwordOfDamnation = {
    .name = $("Sword of Damnation"),
    .description = $("Unaware + Sword of Ruin."),
    .unaware = TRUE,
};

#undef CONTEXT
#define CONTEXT RestrainingOrder
static const Ability RestrainingOrder = {
    .name = $("Restraining Order"),
    .description = $("Forces the attacker when hit\nonce each switch-in."),
};

#undef CONTEXT
#define CONTEXT AssassinsTools
static const Ability AssassinsTools = {
    .name = $("Assassin's Tools"),
    .description = $("Contact moves have a 30%\nchance to PSN, PRLZ, or BLD."),
};

#undef CONTEXT
#define CONTEXT Frostmaw
static const Ability Frostmaw = {
    .name = $("Frostmaw"),
    .description = $("Biting moves have a 50% chance\nto inflict frostbite."),
};

#undef CONTEXT
#define CONTEXT Patchwork
static const Ability Patchwork = {
    .name = $("Patchwork"),
    .description = $("Disguise + curses the opponent\nwhen its Disguise breaks."),
    .unsuppressable = TRUE,
    .breakable = TRUE,
    .randomizerBanned = TRUE,
    .onSwitch = Disguise.onSwitch,
};

#undef CONTEXT
#define CONTEXT BlindRage
static const Ability BlindRage = {
    .name = $("Blind Rage"),
    .description = $("Scrappy + Mold Breaker."),
    .onSwitch = MoldBreaker.onSwitch,
};

#undef CONTEXT
#define CONTEXT Slipstream
static const Ability Slipstream = {
    .name = $("Slipstream"),
    .description = $("Moves use 20% of its Speed\nstat additionally."),
};

#undef CONTEXT
#define CONTEXT ApexPredator
static const Ability ApexPredator = {
    .name = $("Apex Predator"),
    .description = $("Tough Claws + Predator."),
};

#undef CONTEXT
#define CONTEXT DragonsRitual
static const Ability DragonsRitual = {
    .name = $("Dragon's Ritual"),
    .description = $("Dealing a KO raises Attack and\nSpeed by one stage."),
};

#undef CONTEXT
#define CONTEXT PinnacleBlade
static const Ability PinnacleBlade = {
    .name = $("Pinnacle Blade"),
    .description = $("Slashing moves always hit and\nbreak protection and barriers."),
};

#undef CONTEXT
#define CONTEXT Energized
static const Ability Energized = {
    .name = $("Energized"),
    .description = $("Generator + charges up on KO\nwith an Electric-type move."),
    .persistent = TRUE,
    .onSwitch = Generator.onSwitch,
};

#undef CONTEXT
#define CONTEXT ColorSpectrum
static const Ability ColorSpectrum = {
    .name = $("Color Spectrum"),
    .description = $("Same-type attacks get a 1.2x\nboost. Changes type each turn."),
};

#undef CONTEXT
#define CONTEXT SteelBeetle
static const Ability SteelBeetle = {
    .name = $("Steel Beetle"),
    .description = $("Raging Boxer + Pollinate."),
};

#undef CONTEXT
#define CONTEXT FromTheShadows
static const Ability FromTheShadows = {
    .name = $("From the Shadows"),
    .description = $("Attacks trap and have a 20%\nflinch chance when moving first."),
};

#undef CONTEXT
#define CONTEXT RagePoint
static const Ability RagePoint = {
    .name = $("Rage Point"),
    .description = $("Gets a 1.5x boost while statused.\nRaises offenses when crit."),
};

#undef CONTEXT
#define CONTEXT HotCoals
ON_SWITCH {
    CHECK_NOT(gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals)

    gSideTimers[BATTLE_OPPOSITE(battler)].hotCoals = TRUE;
    return SwitchInAnnounce(B_MSG_SWITCHIN_HOT_COALS);
}
static const Ability HotCoals = {
    .name = $("Hot Coals"),
    .description = $("Sets a trap that burns the next\nfoe that switches in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT TerastalTreasure
static const Ability TerastalTreasure = {
    .name = $("Terastal Treasure"),
    .description = $("Reduces damage taken by 40%,\nbut lowers speed by 20%."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT ShockingMaw
static const Ability ShockingMaw = {
    .name = $("Shocking Maw"),
    .description = $("Strong Jaw + Bite moves have\n50% paralysis chance."),
};

#undef CONTEXT
#define CONTEXT GleamEyes
ON_SWITCH {
    return UseIntimidateClone(battler, ability) | Frisk.onSwitch(battler, ability);
}
static const Ability GleamEyes = {
    .name = $("Gleam Eyes"),
    .description = $("Frisk + Scare."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT RousedFangs
static const Ability RousedFangs = {
    .name = $("Megabite"),
    .description = $("Biting moves use SpAtk and\ndeal 30% more damage."),
};

#undef CONTEXT
#define CONTEXT DreamState
static const Ability DreamState = {
    .name = $("Dream State"),
    .description = $("Immune to critical hits. Takes\n20% less damage from all attacks."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT DreamWhimsy
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_YAWN, 0);
}
static const Ability DreamWhimsy = {
    .name = $("Dream Whimsy"),
    .description = $("Uses Yawn on switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT LunarAffinity
static const Ability LunarAffinity = {
    .name = $("Lunar Affinity"),
    .description = $("Copies lunar moves used by\nothers."),
};

#undef CONTEXT
#define CONTEXT FlameShield
static const Ability FlameShield = {
    .name = $("Flame Shield"),
    .description = $("Takes 35% less damage from\nSuper-effective moves."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT AquaticDweller
ON_SWITCH {
    return AddBattlerType(battler, TYPE_WATER);
}
static const Ability AquaticDweller = {
    .name = $("Aquatic Dweller"),
    .description = $("Boosts the power of Water-type\nmoves by 1.5x."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT ApplePie
static const Ability ApplePie = {
    .name = $("Apple Pie"),
    .description = $("Self Sufficient + Ripen."),
};

#undef CONTEXT
#define CONTEXT Hover
ON_SWITCH {
    return AddBattlerType(battler, TYPE_PSYCHIC);
}
static const Ability Hover = {
    .name = $("Hover"),
    .description = $("Adds Psychic type to itself.\nAvoids Ground attacks."),
    .breakable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT Depravity
static const Ability Depravity = {
    .name = $("Depravity"),
    .description = $("Merciless + Overcharge."),
};

#undef CONTEXT
#define CONTEXT Wildfire
static const Ability Wildfire = {
    .name = $("Wildfire"),
    .description = $("Attacks with 20BP Fire Spin\nwhen hit by a contact move."),
};

#undef CONTEXT
#define CONTEXT JumpScare
ON_SWITCH {
    CHECK_NOT(GetSingleUseAbilityCounter(battler, ability))
    SetSingleUseAbilityCounter(battler, ability, TRUE);
    return UseEntryMove(battler, ability, MOVE_ASTONISH, 0);
}
static const Ability JumpScare = {
    .name = $("Jumpscare"),
    .description = $("Attacks with Astonish on first\nswitch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT TarToss
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_TAR_SHOT, 0);
}
static const Ability TarToss = {
    .name = $("Tar Toss"),
    .description = $("Uses Tar Shot on switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT StunShock
static const Ability StunShock = {
    .name = $("Stun Shock"),
    .description = $("Attacks have a 60% chance to\nParalyze or Poison."),
};

#undef CONTEXT
#define CONTEXT RagingGoddess
static const Ability RagingGoddess = {
    .name = $("Raging Goddess"),
    .description = $("Rampage + Hyper Aggressive."),
};

#undef CONTEXT
#define CONTEXT Whiplash
static const Ability Whiplash = {
    .name = $("Whiplash"),
    .description = $("Physical attacks have a 50%\nchance to lower Defense."),
};

#undef CONTEXT
#define CONTEXT SupersweetSyrup
static const Ability SupersweetSyrup = {
    .name = $("Supersweet Syrup"),
    .description = $("Can't lose its item. Disables foe's\nitem for 2 turns on contact."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT LuckyHalo
static const Ability LuckyHalo = {
    .name = $("Lucky Halo"),
    .description = $("Negates self stat drops. Survives\nthe first hit that would KO it."),
};

#undef CONTEXT
#define CONTEXT TrashHeap
static const Ability TrashHeap = {
    .name = $("Trash Heap"),
    .description = $("Corrosion + Toxic Spill."),
    .onSwitch = ToxicSpill.onSwitch,
};

#undef CONTEXT
#define CONTEXT SludgyMix
static const Ability SludgyMix = {
    .name = $("Sludgy Mix"),
    .description = $("Intoxicate + Punk Rock."),
};

#undef CONTEXT
#define CONTEXT Overwatch
static const Ability Overwatch = {
    .name = $("Overwatch"),
    .description = $("On the Prowl + Stakeout."),
    .onSwitch = OnTheProwl.onSwitch,
};

#undef CONTEXT
#define CONTEXT WindRage
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_DEFOG, 0);
}
static const Ability WindRage = {
    .name = $("Wind Rage"),
    .description = $("Uses Defog on switch-in. Air-\nbased moves get a 1.3x boost."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT VictoryBomb
static const Ability VictoryBomb = {
    .name = $("Victory Bomb"),
    .description = $("Attacks with a 100BP Fire-type\nExplosion on fainting."),
};

#undef CONTEXT
#define CONTEXT RazorSharp
static const Ability RazorSharp = {
    .name = $("Razor Sharp"),
    .description = $("Critical hits also inflict bleeding."),
};

#undef CONTEXT
#define CONTEXT ToTheBone
static const Ability ToTheBone = {
    .name = $("To The Bone"),
    .description = $("Critical hits get a 1.5x boost and\ninflict bleeding."),
};

#undef CONTEXT
#define CONTEXT BladeDance
static const Ability BladeDance = {
    .name = $("Blade Dance"),
    .description = $("Triggers 50 BP Leaf Blade after\nusing a dance move."),
};

#undef CONTEXT
#define CONTEXT Taekkyeon
static const Ability Taekkyeon = {
    .name = $("Taekkyeon"),
    .description = $("All attacks are dances."),
};

#undef CONTEXT
#define CONTEXT ApeShift
ON_SWITCH {
    CHECK_NOT(gBattleMons[battler].status2 && STATUS2_TRANSFORMED)
    CHECK(gBattleMons[battler].species == SPECIES_SLAKING_MEGA || gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT)
    CHECK(ShouldChangeFormHpBased(battler))

    gStackBattler1 = battler;
    BattleScriptPushCursorAndCallback(BattleScript_End3);
    BattleScriptCall(gBattleMons[battler].species == SPECIES_SLAKING_MEGA_APE_SHIFT ? BattleScript_ApeShift : BattleScript_AttackerFormChangeNoPopup);
    return TRUE;
}
static const Ability ApeShift = {
    .name = $("Ape Shift"),
    .description = $("Transforms when below 50% HP,\ncuring status and always critting."),
    .randomizerBanned = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT KnowYourPlace
static const Ability KnowYourPlace = {
    .name = $("Know Your Place"),
    .description = $("Contact attacks make foes move\nlast for 5 turns."),
};

#undef CONTEXT
#define CONTEXT DeepCuts
static const Ability DeepCuts = {
    .name = $("Deep Cuts"),
    .description = $("Slashing moves have a 50%\nchance to inflict bleeding."),
};

#undef CONTEXT
#define CONTEXT LifeSteal
static const Ability LifeSteal = {
    .name = $("Life Steal"),
    .description = $("Steals 1/10 HP from foes each\nturn."),
};

#undef CONTEXT
#define CONTEXT RudeAwakening
static const Ability RudeAwakening = {
    .name = $("Rude Awakening"),
    .description = $("Raises all stats becomes immune\nto sleep after waking up."),
};

#undef CONTEXT
#define CONTEXT TeraformZero
ON_SWITCH {
    CHECK(!GetSingleUseAbilityCounter(battler, ability))
    SetSingleUseAbilityCounter(battler, ability, TRUE);
    CHECK(IsWeatherActive(WEATHER_ANY) || IsTerrainActive(STATUS_FIELD_TERRAIN_ANY))
    BattleScriptPushCursorAndCallback(BattleScript_TeraformZero);
    return TRUE;
}
static const Ability TeraformZero = {
    .name = $("Teraform Zero"),
    .description = $("Tera Shell + clears weather and\nterrain on first entry."),
    .breakable = TRUE,
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SetAblaze
static const Ability SetAblaze = {
    .name = $("Set Ablaze"),
    .description = $("Inflicting burn also inflicts fear."),
};

#undef CONTEXT
#define CONTEXT Breakwater
static const Ability Breakwater = {
    .name = $("Breakwater"),
    .description = $("Swift Swim + Stall."),
    .breakable = TRUE,
};

#undef CONTEXT
#define CONTEXT MagicalFists
static const Ability MagicalFists = {
    .name = $("Magical Fists"),
    .description = $("Punching moves use Special\nAttack and get a 1.3x boost."),
};

#undef CONTEXT
#define CONTEXT Cutthroat
ON_SWITCH {
    CHECK_NOT(GetAbilityState(battler, ability))

    gStatuses4[battler] |= STATUS4_CUTTHROAT;
    return SwitchInAnnounce(B_MSG_SWITCHIN_CUTTHROAT);
}
static const Ability Cutthroat = {
    .name = $("Cutthroat"),
    .description = $("The first slicing move used on\neach entry in gets +1 priority."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT SandBender
static const Ability SandBender = {
    .name = $("Sand Bender"),
    .description = $("Sand Stream + Sand Force."),
    .onSwitch = SandStream.onSwitch,
};

#undef CONTEXT
#define CONTEXT SandPit
ON_SWITCH {
    return UseEntryMove(battler, ability, MOVE_SAND_TOMB, 20);
}
static const Ability SandPit = {
    .name = $("Sand Pit"),
    .description = $("Attacks with 20BP Sand Tomb\non switch-in."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT DesolateSun
static const Ability DesolateSun = {
    .name = $("Desolate Sun"),
    .description = $("Desolate Land + Earth Eater."),
    .randomizerBanned = TRUE,
};

#undef CONTEXT
#define CONTEXT Daybreak
static const Ability Daybreak = {
    .name = $("Daybreak"),
    .description = $("Burns the foe on contact.\nAlso works on offense."),
};

#undef CONTEXT
#define CONTEXT EnergySiphon
static const Ability EnergySiphon = {
    .name = $("Energy Siphon"),
    .description = $("Heals the user for 1/4\nof the damage they deal."),
};

#undef CONTEXT
#define CONTEXT Reservoir
static const Ability Reservoir = {
    .name = $("Reservoir"),
    .description = $("Water Absorb + Storm Drain."),
};

#undef CONTEXT
#define CONTEXT Neurotoxin
static const Ability Neurotoxin = {
    .name = $("Neurotoxin"),
    .description = $("Inflicting poison also lowers\nAttack, Defense, and Speed."),
};

#undef CONTEXT
#define CONTEXT EnergizedHorns
static const Ability EnergizedHorns = {
    .name = $("Energy Horns"),
    .description = $("Mighty horn moves become special\nand deal 30% more damage."),
};

#undef CONTEXT
#define CONTEXT SpiderLairUpgrade
ON_SWITCH {
    CHECK_NOT(gSideStatuses[BATTLE_OPPOSITE(battler)] & SIDE_STATUS_STICKY_WEB)
    
    int side = BATTLE_OPPOSITE(battler);
    gSideTimers[side].started.spiderWeb = TRUE;
    gSideStatuses[side] |= SIDE_STATUS_STICKY_WEB;
    gSideTimers[side].stickyWebTimer = 7;
    BattleScriptPushCursorAndCallback(BattleScript_SpiderLairActivated);
    return TRUE;
}
static const Ability SpiderLairUpgrade = {
    .name = $("Rising Dough"),
    .description = $("Casts Sticky Web on entry.\nLasts 7 turns."),
    CONTEXT_ON_SWITCH,
};

#undef CONTEXT
#define CONTEXT CrustCoat
static const Ability CrustCoat = {
    .name = $("Crust Coat"),
    .description = $("Immune to critical hits. Takes\n20% less damage from all attacks."),
};

#undef CONTEXT
#define CONTEXT Puffy
static const Ability Puffy = {
    .name = $("Puffy"),
    .description = $("Takes 1/2 dmg from contact moves\nbut Fire moves hurt it 2x more."),
};

#undef CONTEXT
#define CONTEXT BalloonBlitz
static const Ability BalloonBlitz = {
    .name = $("Balloon Blitz"),
    .description = $("Inflatable + Hyper Aggressive."),
};

const Ability gAbilities[] = {
[ABILITY_NONE] = None,
[ABILITY_STENCH] = Stench,
[ABILITY_DRIZZLE] = Drizzle,
[ABILITY_SPEED_BOOST] = SpeedBoost,
[ABILITY_BATTLE_ARMOR] = BattleArmor,
[ABILITY_STURDY] = Sturdy,
[ABILITY_DAMP] = Damp,
[ABILITY_LIMBER] = Limber,
[ABILITY_SAND_VEIL] = SandVeil,
[ABILITY_STATIC] = Static,
[ABILITY_VOLT_ABSORB] = VoltAbsorb,
[ABILITY_WATER_ABSORB] = WaterAbsorb,
[ABILITY_OBLIVIOUS] = Oblivious,
[ABILITY_CLOUD_NINE] = CloudNine,
[ABILITY_COMPOUND_EYES] = CompoundEyes,
[ABILITY_INSOMNIA] = Insomnia,
[ABILITY_COLOR_CHANGE] = ColorChange,
[ABILITY_IMMUNITY] = Immunity,
[ABILITY_FLASH_FIRE] = FlashFire,
[ABILITY_SHIELD_DUST] = ShieldDust,
[ABILITY_OWN_TEMPO] = OwnTempo,
[ABILITY_SUCTION_CUPS] = SuctionCups,
[ABILITY_INTIMIDATE] = Intimidate,
[ABILITY_SHADOW_TAG] = ShadowTag,
[ABILITY_ROUGH_SKIN] = RoughSkin,
[ABILITY_WONDER_GUARD] = WonderGuard,
[ABILITY_LEVITATE] = Levitate,
[ABILITY_EFFECT_SPORE] = EffectSpore,
[ABILITY_SYNCHRONIZE] = Synchronize,
[ABILITY_CLEAR_BODY] = ClearBody,
[ABILITY_NATURAL_CURE] = NaturalCure,
[ABILITY_LIGHTNING_ROD] = LightningRod,
[ABILITY_SERENE_GRACE] = SereneGrace,
[ABILITY_SWIFT_SWIM] = SwiftSwim,
[ABILITY_CHLOROPHYLL] = Chlorophyll,
[ABILITY_ILLUMINATE] = Illuminate,
[ABILITY_TRACE] = Trace,
[ABILITY_HUGE_POWER] = HugePower,
[ABILITY_POISON_POINT] = PoisonPoint,
[ABILITY_INNER_FOCUS] = InnerFocus,
[ABILITY_MAGMA_ARMOR] = MagmaArmor,
[ABILITY_WATER_VEIL] = WaterVeil,
[ABILITY_MAGNET_PULL] = MagnetPull,
[ABILITY_SOUNDPROOF] = Soundproof,
[ABILITY_RAIN_DISH] = RainDish,
[ABILITY_SAND_STREAM] = SandStream,
[ABILITY_PRESSURE] = Pressure,
[ABILITY_THICK_FAT] = ThickFat,
[ABILITY_EARLY_BIRD] = EarlyBird,
[ABILITY_FLAME_BODY] = FlameBody,
[ABILITY_RUN_AWAY] = RunAway,
[ABILITY_KEEN_EYE] = KeenEye,
[ABILITY_HYPER_CUTTER] = HyperCutter,
[ABILITY_PICKUP] = Pickup,
[ABILITY_TRUANT] = Truant,
[ABILITY_HUSTLE] = Hustle,
[ABILITY_CUTE_CHARM] = CuteCharm,
[ABILITY_PLUS] = Plus,
[ABILITY_MINUS] = Minus,
[ABILITY_FORECAST] = Forecast,
[ABILITY_STICKY_HOLD] = StickyHold,
[ABILITY_SHED_SKIN] = ShedSkin,
[ABILITY_GUTS] = Guts,
[ABILITY_MARVEL_SCALE] = MarvelScale,
[ABILITY_LIQUID_OOZE] = LiquidOoze,
[ABILITY_OVERGROW] = Overgrow,
[ABILITY_BLAZE] = Blaze,
[ABILITY_TORRENT] = Torrent,
[ABILITY_SWARM] = Swarm,
[ABILITY_ROCK_HEAD] = RockHead,
[ABILITY_DROUGHT] = Drought,
[ABILITY_ARENA_TRAP] = ArenaTrap,
[ABILITY_VITAL_SPIRIT] = VitalSpirit,
[ABILITY_WHITE_SMOKE] = WhiteSmoke,
[ABILITY_PURE_POWER] = PurePower,
[ABILITY_SHELL_ARMOR] = ShellArmor,
[ABILITY_AIR_LOCK] = AirLock,
[ABILITY_TANGLED_FEET] = TangledFeet,
[ABILITY_MOTOR_DRIVE] = MotorDrive,
[ABILITY_RIVALRY] = Rivalry,
[ABILITY_STEADFAST] = Steadfast,
[ABILITY_SNOW_CLOAK] = SnowCloak,
[ABILITY_GLUTTONY] = Gluttony,
[ABILITY_ANGER_POINT] = AngerPoint,
[ABILITY_UNBURDEN] = Unburden,
[ABILITY_HEATPROOF] = Heatproof,
[ABILITY_SIMPLE] = Simple,
[ABILITY_DRY_SKIN] = DrySkin,
[ABILITY_DOWNLOAD] = Download,
[ABILITY_IRON_FIST] = IronFist,
[ABILITY_POISON_HEAL] = PoisonHeal,
[ABILITY_ADAPTABILITY] = Adaptability,
[ABILITY_SKILL_LINK] = SkillLink,
[ABILITY_HYDRATION] = Hydration,
[ABILITY_SOLAR_POWER] = SolarPower,
[ABILITY_QUICK_FEET] = QuickFeet,
[ABILITY_NORMALIZE] = Normalize,
[ABILITY_SNIPER] = Sniper,
[ABILITY_MAGIC_GUARD] = MagicGuard,
[ABILITY_NO_GUARD] = NoGuard,
[ABILITY_STALL] = Stall,
[ABILITY_TECHNICIAN] = Technician,
[ABILITY_LEAF_GUARD] = LeafGuard,
[ABILITY_KLUTZ] = Klutz,
[ABILITY_MOLD_BREAKER] = MoldBreaker,
[ABILITY_SUPER_LUCK] = SuperLuck,
[ABILITY_AFTERMATH] = Aftermath,
[ABILITY_ANTICIPATION] = Anticipation,
[ABILITY_FOREWARN] = Forewarn,
[ABILITY_UNAWARE] = Unaware,
[ABILITY_TINTED_LENS] = TintedLens,
[ABILITY_FILTER] = Filter,
[ABILITY_SLOW_START] = SlowStart,
[ABILITY_SCRAPPY] = Scrappy,
[ABILITY_STORM_DRAIN] = StormDrain,
[ABILITY_ICE_BODY] = IceBody,
[ABILITY_SOLID_ROCK] = SolidRock,
[ABILITY_SNOW_WARNING] = SnowWarning,
[ABILITY_HONEY_GATHER] = HoneyGather,
[ABILITY_FRISK] = Frisk,
[ABILITY_RECKLESS] = Reckless,
[ABILITY_MULTITYPE] = Multitype,
[ABILITY_FLOWER_GIFT] = FlowerGift,
[ABILITY_BAD_DREAMS] = BadDreams,
[ABILITY_PICKPOCKET] = Pickpocket,
[ABILITY_SHEER_FORCE] = SheerForce,
[ABILITY_CONTRARY] = Contrary,
[ABILITY_UNNERVE] = Unnerve,
[ABILITY_DEFIANT] = Defiant,
[ABILITY_DEFEATIST] = Defeatist,
[ABILITY_CURSED_BODY] = CursedBody,
[ABILITY_HEALER] = Healer,
[ABILITY_FRIEND_GUARD] = FriendGuard,
[ABILITY_WEAK_ARMOR] = WeakArmor,
[ABILITY_HEAVY_METAL] = HeavyMetal,
[ABILITY_LIGHT_METAL] = LightMetal,
[ABILITY_MULTISCALE] = Multiscale,
[ABILITY_TOXIC_BOOST] = ToxicBoost,
[ABILITY_FLARE_BOOST] = FlareBoost,
[ABILITY_HARVEST] = Harvest,
[ABILITY_TELEPATHY] = Telepathy,
[ABILITY_MOODY] = Moody,
[ABILITY_OVERCOAT] = Overcoat,
[ABILITY_POISON_TOUCH] = PoisonTouch,
[ABILITY_REGENERATOR] = Regenerator,
[ABILITY_BIG_PECKS] = BigPecks,
[ABILITY_SAND_RUSH] = SandRush,
[ABILITY_WONDER_SKIN] = WonderSkin,
[ABILITY_ANALYTIC] = Analytic,
[ABILITY_ILLUSION] = Illusion,
[ABILITY_IMPOSTER] = Imposter,
[ABILITY_INFILTRATOR] = Infiltrator,
[ABILITY_MUMMY] = Mummy,
[ABILITY_MOXIE] = Moxie,
[ABILITY_JUSTIFIED] = Justified,
[ABILITY_RATTLED] = Rattled,
[ABILITY_MAGIC_BOUNCE] = MagicBounce,
[ABILITY_SAP_SIPPER] = SapSipper,
[ABILITY_PRANKSTER] = Prankster,
[ABILITY_SAND_FORCE] = SandForce,
[ABILITY_IRON_BARBS] = IronBarbs,
[ABILITY_ZEN_MODE] = ZenMode,
[ABILITY_VICTORY_STAR] = VictoryStar,
[ABILITY_TURBOBLAZE] = Turboblaze,
[ABILITY_TERAVOLT] = Teravolt,
[ABILITY_AROMA_VEIL] = AromaVeil,
[ABILITY_FLOWER_VEIL] = FlowerVeil,
[ABILITY_CHEEK_POUCH] = CheekPouch,
[ABILITY_PROTEAN] = Protean,
[ABILITY_FUR_COAT] = FurCoat,
[ABILITY_MAGICIAN] = Magician,
[ABILITY_BULLETPROOF] = Bulletproof,
[ABILITY_COMPETITIVE] = Competitive,
[ABILITY_STRONG_JAW] = StrongJaw,
[ABILITY_REFRIGERATE] = Refrigerate,
[ABILITY_SWEET_VEIL] = SweetVeil,
[ABILITY_STANCE_CHANGE] = StanceChange,
[ABILITY_GALE_WINGS] = GaleWings,
[ABILITY_MEGA_LAUNCHER] = MegaLauncher,
[ABILITY_GRASS_PELT] = GrassPelt,
[ABILITY_SYMBIOSIS] = Symbiosis,
[ABILITY_TOUGH_CLAWS] = ToughClaws,
[ABILITY_PIXILATE] = Pixilate,
[ABILITY_GOOEY] = Gooey,
[ABILITY_AERILATE] = Aerilate,
[ABILITY_PARENTAL_BOND] = ParentalBond,
[ABILITY_DARK_AURA] = DarkAura,
[ABILITY_FAIRY_AURA] = FairyAura,
[ABILITY_AURA_BREAK] = AuraBreak,
[ABILITY_PRIMORDIAL_SEA] = PrimordialSea,
[ABILITY_DESOLATE_LAND] = DesolateLand,
[ABILITY_DELTA_STREAM] = DeltaStream,
[ABILITY_STAMINA] = Stamina,
[ABILITY_WIMP_OUT] = WimpOut,
[ABILITY_EMERGENCY_EXIT] = EmergencyExit,
[ABILITY_WATER_COMPACTION] = WaterCompaction,
[ABILITY_MERCILESS] = Merciless,
[ABILITY_SHIELDS_DOWN] = ShieldsDown,
[ABILITY_STAKEOUT] = Stakeout,
[ABILITY_WATER_BUBBLE] = WaterBubble,
[ABILITY_STEELWORKER] = Steelworker,
[ABILITY_BERSERK] = Berserk,
[ABILITY_SLUSH_RUSH] = SlushRush,
[ABILITY_LONG_REACH] = LongReach,
[ABILITY_LIQUID_VOICE] = LiquidVoice,
[ABILITY_TRIAGE] = Triage,
[ABILITY_GALVANIZE] = Galvanize,
[ABILITY_SURGE_SURFER] = SurgeSurfer,
[ABILITY_SCHOOLING] = Schooling,
[ABILITY_DISGUISE] = Disguise,
[ABILITY_BATTLE_BOND] = BattleBond,
[ABILITY_POWER_CONSTRUCT] = PowerConstruct,
[ABILITY_CORROSION] = Corrosion,
[ABILITY_COMATOSE] = Comatose,
[ABILITY_QUEENLY_MAJESTY] = QueenlyMajesty,
[ABILITY_INNARDS_OUT] = InnardsOut,
[ABILITY_DANCER] = Dancer,
[ABILITY_BATTERY] = Battery,
[ABILITY_FLUFFY] = Fluffy,
[ABILITY_DAZZLING] = Dazzling,
[ABILITY_SOUL_HEART] = SoulHeart,
[ABILITY_TANGLING_HAIR] = TanglingHair,
[ABILITY_RECEIVER] = Receiver,
[ABILITY_POWER_OF_ALCHEMY] = PowerOfAlchemy,
[ABILITY_BEAST_BOOST] = BeastBoost,
[ABILITY_RKS_SYSTEM] = RksSystem,
[ABILITY_ELECTRIC_SURGE] = ElectricSurge,
[ABILITY_PSYCHIC_SURGE] = PsychicSurge,
[ABILITY_MISTY_SURGE] = MistySurge,
[ABILITY_GRASSY_SURGE] = GrassySurge,
[ABILITY_FULL_METAL_BODY] = FullMetalBody,
[ABILITY_SHADOW_SHIELD] = ShadowShield,
[ABILITY_PRISM_ARMOR] = PrismArmor,
[ABILITY_NEUROFORCE] = Neuroforce,
[ABILITY_INTREPID_SWORD] = IntrepidSword,
[ABILITY_DAUNTLESS_SHIELD] = DauntlessShield,
[ABILITY_LIBERO] = Libero,
[ABILITY_BALL_FETCH] = BallFetch,
[ABILITY_COTTON_DOWN] = CottonDown,
[ABILITY_PROPELLER_TAIL] = PropellerTail,
[ABILITY_MIRROR_ARMOR] = MirrorArmor,
[ABILITY_GULP_MISSILE] = GulpMissile,
[ABILITY_STALWART] = Stalwart,
[ABILITY_STEAM_ENGINE] = SteamEngine,
[ABILITY_PUNK_ROCK] = PunkRock,
[ABILITY_SAND_SPIT] = SandSpit,
[ABILITY_ICE_SCALES] = IceScales,
[ABILITY_RIPEN] = Ripen,
[ABILITY_ICE_FACE] = IceFace,
[ABILITY_POWER_SPOT] = PowerSpot,
[ABILITY_MIMICRY] = Mimicry,
[ABILITY_SCREEN_CLEANER] = ScreenCleaner,
[ABILITY_STEELY_SPIRIT] = SteelySpirit,
[ABILITY_PERISH_BODY] = PerishBody,
[ABILITY_WANDERING_SPIRIT] = WanderingSpirit,
[ABILITY_GORILLA_TACTICS] = GorillaTactics,
[ABILITY_NEUTRALIZING_GAS] = NeutralizingGas,
[ABILITY_PASTEL_VEIL] = PastelVeil,
[ABILITY_HUNGER_SWITCH] = HungerSwitch,
[ABILITY_QUICK_DRAW] = QuickDraw,
[ABILITY_UNSEEN_FIST] = UnseenFist,
[ABILITY_CURIOUS_MEDICINE] = CuriousMedicine,
[ABILITY_TRANSISTOR] = Transistor,
[ABILITY_DRAGONS_MAW] = DragonsMaw,
[ABILITY_CHILLING_NEIGH] = ChillingNeigh,
[ABILITY_GRIM_NEIGH] = GrimNeigh,
[ABILITY_AS_ONE_ICE_RIDER] = AsOneIceRider,
[ABILITY_AS_ONE_SHADOW_RIDER] = AsOneShadowRider,
[ABILITY_CHLOROPLAST] = Chloroplast,
[ABILITY_WHITEOUT] = Whiteout,
[ABILITY_PYROMANCY] = Pyromancy,
[ABILITY_KEEN_EDGE] = KeenEdge,
[ABILITY_PRISM_SCALES] = PrismScales,
[ABILITY_POWER_FISTS] = PowerFists,
[ABILITY_SAND_SONG] = SandSong,
[ABILITY_RAMPAGE] = Rampage,
[ABILITY_VENGEANCE] = Vengeance,
[ABILITY_BLITZ_BOXER] = BlitzBoxer,
[ABILITY_ANTARCTIC_BIRD] = AntarcticBird,
[ABILITY_IMMOLATE] = Immolate,
[ABILITY_CRYSTALLIZE] = Crystallize,
[ABILITY_ELECTROCYTES] = Electrocytes,
[ABILITY_AERODYNAMICS] = Aerodynamics,
[ABILITY_CHRISTMAS_SPIRIT] = ChristmasSpirit,
[ABILITY_EXPLOIT_WEAKNESS] = ExploitWeakness,
[ABILITY_GROUND_SHOCK] = GroundShock,
[ABILITY_ANCIENT_IDOL] = AncientIdol,
[ABILITY_MYSTIC_POWER] = MysticPower,
[ABILITY_PERFECTIONIST] = Perfectionist,
[ABILITY_GROWING_TOOTH] = GrowingTooth,
[ABILITY_INFLATABLE] = Inflatable,
[ABILITY_AURORA_BOREALIS] = AuroraBorealis,
[ABILITY_AVENGER] = Avenger,
[ABILITY_LETS_ROLL] = LetsRoll,
[ABILITY_AQUATIC] = Aquatic,
[ABILITY_LOUD_BANG] = LoudBang,
[ABILITY_LEAD_COAT] = LeadCoat,
[ABILITY_AMPHIBIOUS] = Amphibious,
[ABILITY_GROUNDED] = Grounded,
[ABILITY_EARTHBOUND] = Earthbound,
[ABILITY_FIGHT_SPIRIT] = FightSpirit,
[ABILITY_FELINE_PROWESS] = FelineProwess,
[ABILITY_COIL_UP] = CoilUp,
[ABILITY_FOSSILIZED] = Fossilized,
[ABILITY_MAGICAL_DUST] = MagicalDust,
[ABILITY_DREAMCATCHER] = Dreamcatcher,
[ABILITY_NOCTURNAL] = Nocturnal,
[ABILITY_SELF_SUFFICIENT] = SelfSufficient,
[ABILITY_TECTONIZE] = Tectonize,
[ABILITY_ICE_AGE] = IceAge,
[ABILITY_HALF_DRAKE] = HalfDrake,
[ABILITY_LIQUIFIED] = Liquified,
[ABILITY_DRAGONFLY] = Dragonfly,
[ABILITY_DRAGONSLAYER] = Dragonslayer,
[ABILITY_MOUNTAINEER] = Mountaineer,
[ABILITY_HYDRATE] = Hydrate,
[ABILITY_METALLIC] = Metallic,
[ABILITY_PERMAFROST] = Permafrost,
[ABILITY_PRIMAL_ARMOR] = PrimalArmor,
[ABILITY_RAGING_BOXER] = RagingBoxer,
[ABILITY_AIR_BLOWER] = AirBlower,
[ABILITY_JUGGERNAUT] = Juggernaut,
[ABILITY_SHORT_CIRCUIT] = ShortCircuit,
[ABILITY_MAJESTIC_BIRD] = MajesticBird,
[ABILITY_PHANTOM] = Phantom,
[ABILITY_INTOXICATE] = Intoxicate,
[ABILITY_IMPENETRABLE] = Impenetrable,
[ABILITY_HYPNOTIST] = Hypnotist,
[ABILITY_OVERWHELM] = Overwhelm,
[ABILITY_SCARE] = Scare,
[ABILITY_MAJESTIC_MOTH] = MajesticMoth,
[ABILITY_SOUL_EATER] = SoulEater,
[ABILITY_SOUL_LINKER] = SoulLinker,
[ABILITY_SWEET_DREAMS] = SweetDreams,
[ABILITY_BAD_LUCK] = BadLuck,
[ABILITY_HAUNTED_SPIRIT] = HauntedSpirit,
[ABILITY_ELECTRIC_BURST] = ElectricBurst,
[ABILITY_RAW_WOOD] = RawWood,
[ABILITY_SOLENOGLYPHS] = Solenoglyphs,
[ABILITY_SPIDER_LAIR] = SpiderLair,
[ABILITY_FATAL_PRECISION] = FatalPrecision,
[ABILITY_FORT_KNOX] = FortKnox,
[ABILITY_SEAWEED] = Seaweed,
[ABILITY_PSYCHIC_MIND] = PsychicMind,
[ABILITY_POISON_ABSORB] = PoisonAbsorb,
[ABILITY_SCAVENGER] = Scavenger,
[ABILITY_TWISTED_DIMENSION] = TwistedDimension,
[ABILITY_MULTI_HEADED] = MultiHeaded,
[ABILITY_NORTH_WIND] = NorthWind,
[ABILITY_OVERCHARGE] = Overcharge,
[ABILITY_VIOLENT_RUSH] = ViolentRush,
[ABILITY_FLAMING_SOUL] = FlamingSoul,
[ABILITY_SAGE_POWER] = SagePower,
[ABILITY_BONE_ZONE] = BoneZone,
[ABILITY_WEATHER_CONTROL] = WeatherControl,
[ABILITY_SPEED_FORCE] = SpeedForce,
[ABILITY_SEA_GUARDIAN] = SeaGuardian,
[ABILITY_MOLTEN_DOWN] = MoltenDown,
[ABILITY_HYPER_AGGRESSIVE] = HyperAggressive,
[ABILITY_FLOCK] = Flock,
[ABILITY_FIELD_EXPLORER] = FieldExplorer,
[ABILITY_STRIKER] = Striker,
[ABILITY_FROZEN_SOUL] = FrozenSoul,
[ABILITY_PREDATOR] = Predator,
[ABILITY_LOOTER] = Looter,
[ABILITY_LUNAR_ECLIPSE] = LunarEclipse,
[ABILITY_SOLAR_FLARE] = SolarFlare,
[ABILITY_POWER_CORE] = PowerCore,
[ABILITY_SIGHTING_SYSTEM] = SightingSystem,
[ABILITY_BAD_COMPANY] = BadCompany,
[ABILITY_OPPORTUNIST] = Opportunist,
[ABILITY_GIANT_WINGS] = GiantWings,
[ABILITY_MOMENTUM] = Momentum,
[ABILITY_GRIP_PINCER] = GripPincer,
[ABILITY_BIG_LEAVES] = BigLeaves,
[ABILITY_PRECISE_FIST] = PreciseFist,
[ABILITY_DEADEYE] = Deadeye,
[ABILITY_ARTILLERY] = Artillery,
[ABILITY_AMPLIFIER] = Amplifier,
[ABILITY_ICE_DEW] = IceDew,
[ABILITY_SUN_WORSHIP] = SunWorship,
[ABILITY_POLLINATE] = Pollinate,
[ABILITY_VOLCANO_RAGE] = VolcanoRage,
[ABILITY_COLD_REBOUND] = ColdRebound,
[ABILITY_LOW_BLOW] = LowBlow,
[ABILITY_NOSFERATU] = Nosferatu,
[ABILITY_SPECTRAL_SHROUD] = SpectralShroud,
[ABILITY_DISCIPLINE] = Discipline,
[ABILITY_THUNDERCALL] = Thundercall,
[ABILITY_MARINE_APEX] = MarineApex,
[ABILITY_MIGHTY_HORN] = MightyHorn,
[ABILITY_HARDENED_SHEATH] = HardenedSheath,
[ABILITY_ARCTIC_FUR] = ArcticFur,
[ABILITY_SPECTRALIZE] = Spectralize,
[ABILITY_LETHARGY] = Lethargy,
[ABILITY_IRON_BARRAGE] = IronBarrage,
[ABILITY_STEEL_BARREL] = SteelBarrel,
[ABILITY_PYRO_SHELLS] = PyroShells,
[ABILITY_FUNGAL_INFECTION] = FungalInfection,
[ABILITY_PARRY] = Parry,
[ABILITY_SCRAPYARD] = Scrapyard,
[ABILITY_LOOSE_QUILLS] = LooseQuills,
[ABILITY_TOXIC_DEBRIS] = ToxicDebris,
[ABILITY_ROUNDHOUSE] = Roundhouse,
[ABILITY_MINERALIZE] = Mineralize,
[ABILITY_LOOSE_ROCKS] = LooseRocks,
[ABILITY_SPINNING_TOP] = SpinningTop,
[ABILITY_RETRIBUTION_BLOW] = RetributionBlow,
[ABILITY_FEARMONGER] = Fearmonger,
[ABILITY_KINGS_WRATH] = KingsWrath,
[ABILITY_QUEENS_MOURNING] = QueensMourning,
[ABILITY_TOXIC_SPILL] = ToxicSpill,
[ABILITY_DESERT_CLOAK] = DesertCloak,
[ABILITY_DRACONIZE] = Draconize,
[ABILITY_PRETTY_PRINCESS] = PrettyPrincess,
[ABILITY_SELF_REPAIR] = SelfRepair,
[ABILITY_ATOMIC_BURST] = AtomicBurst,
[ABILITY_HELLBLAZE] = Hellblaze,
[ABILITY_RIPTIDE] = Riptide,
[ABILITY_FOREST_RAGE] = ForestRage,
[ABILITY_PRIMAL_MAW] = PrimalMaw,
[ABILITY_SWEEPING_EDGE] = SweepingEdge,
[ABILITY_GIFTED_MIND] = GiftedMind,
[ABILITY_HYDRO_CIRCUIT] = HydroCircuit,
[ABILITY_EQUINOX] = Equinox,
[ABILITY_ABSORBANT] = Absorbant,
[ABILITY_CLUELESS] = Clueless,
[ABILITY_CHEATING_DEATH] = CheatingDeath,
[ABILITY_CHEAP_TACTICS] = CheapTactics,
[ABILITY_COWARD] = Coward,
[ABILITY_VOLT_RUSH] = VoltRush,
[ABILITY_DUNE_TERROR] = DuneTerror,
[ABILITY_INFERNAL_RAGE] = InfernalRage,
[ABILITY_DUAL_WIELD] = DualWield,
[ABILITY_ELEMENTAL_CHARGE] = ElementalCharge,
[ABILITY_AMBUSH] = Ambush,
[ABILITY_ATLAS] = Atlas,
[ABILITY_RADIANCE] = Radiance,
[ABILITY_JAWS_OF_CARNAGE] = JawsOfCarnage,
[ABILITY_ANGELS_WRATH] = AngelsWrath,
[ABILITY_PRISMATIC_FUR] = PrismaticFur,
[ABILITY_SHOCKING_JAWS] = ShockingJaws,
[ABILITY_FAE_HUNTER] = FaeHunter,
[ABILITY_GRAVITY_WELL] = GravityWell,
[ABILITY_EVAPORATE] = Evaporate,
[ABILITY_LUMBERJACK] = Lumberjack,
[ABILITY_WELL_BAKED_BODY] = WellBakedBody,
[ABILITY_FURNACE] = Furnace,
[ABILITY_ELECTROMORPHOSIS] = Electromorphosis,
[ABILITY_ROCKY_PAYLOAD] = RockyPayload,
[ABILITY_EARTH_EATER] = EarthEater,
[ABILITY_LINGERING_AROMA] = LingeringAroma,
[ABILITY_FAIRY_TALE] = FairyTale,
[ABILITY_RAGING_MOTH] = RagingMoth,
[ABILITY_ADRENALINE_RUSH] = AdrenalineRush,
[ABILITY_ARCHMAGE] = Archmage,
[ABILITY_CRYOMANCY] = Cryomancy,
[ABILITY_PHANTOM_PAIN] = PhantomPain,
[ABILITY_PURGATORY] = Purgatory,
[ABILITY_EMANATE] = Emanate,
[ABILITY_KUNOICHI_BLADE] = KunoichiBlade,
[ABILITY_MONKEY_BUSINESS] = MonkeyBusiness,
[ABILITY_COMBAT_SPECIALIST] = CombatSpecialist,
[ABILITY_JUNGLES_GUARD] = JunglesGuard,
[ABILITY_HUNTERS_HORN] = HuntersHorn,
[ABILITY_PIXIE_POWER] = PixiePower,
[ABILITY_PLASMA_LAMP] = PlasmaLamp,
[ABILITY_MAGMA_EATER] = MagmaEater,
[ABILITY_SUPER_HOT_GOO] = SuperHotGoo,
[ABILITY_NIKA] = Nika,
[ABILITY_ARCHER] = Archer,
[ABILITY_COLD_PLASMA] = ColdPlasma,
[ABILITY_SUPER_SLAMMER] = SuperSlammer,
[ABILITY_INVERSE_ROOM] = InverseRoom,
[ABILITY_ACCELERATE] = Accelerate,
[ABILITY_FROST_BURN] = FrostBurn,
[ABILITY_ITCHY_DEFENSE] = ItchyDefense,
[ABILITY_GENERATOR] = Generator,
[ABILITY_MOON_SPIRIT] = MoonSpirit,
[ABILITY_DUST_CLOUD] = DustCloud,
[ABILITY_BERSERKER_RAGE] = BerserkerRage,
[ABILITY_TRICKSTER] = Trickster,
[ABILITY_SAND_GUARD] = SandGuard,
[ABILITY_NATURAL_RECOVERY] = NaturalRecovery,
[ABILITY_WIND_RIDER] = WindRider,
[ABILITY_SOOTHING_AROMA] = SoothingAroma,
[ABILITY_PRIM_AND_PROPER] = PrimAndProper,
[ABILITY_SUPER_STRAIN] = SuperStrain,
[ABILITY_TIPPING_POINT] = TippingPoint,
[ABILITY_ENLIGHTENED] = Enlightened,
[ABILITY_PEACEFUL_SLUMBER] = PeacefulSlumber,
[ABILITY_AFTERSHOCK] = Aftershock,
[ABILITY_FREEZING_POINT] = FreezingPoint,
[ABILITY_CRYO_PROFICIENCY] = CryoProficiency,
[ABILITY_ARCANE_FORCE] = ArcaneForce,
[ABILITY_DOOMBRINGER] = Doombringer,
[ABILITY_WISHMAKER] = Wishmaker,
[ABILITY_YUKI_ONNA] = YukiOnna,
[ABILITY_SUPPRESS] = Suppress,
[ABILITY_REFRIGERATOR] = Refrigerator,
[ABILITY_HEAVEN_ASUNDER] = HeavenAsunder,
[ABILITY_PURIFYING_WATERS] = PurifyingWaters,
[ABILITY_SEABORNE] = Seaborne,
[ABILITY_HIGH_TIDE] = HighTide,
[ABILITY_CHANGE_OF_HEART] = ChangeOfHeart,
[ABILITY_MYSTIC_BLADES] = MysticBlades,
[ABILITY_DETERMINATION] = Determination,
[ABILITY_FERTILIZE] = Fertilize,
[ABILITY_PURE_LOVE] = PureLove,
[ABILITY_FIGHTER] = Fighter,
[ABILITY_MYCELIUM_MIGHT] = MyceliumMight,
[ABILITY_TELEKINETIC] = Telekinetic,
[ABILITY_COMBUSTION] = Combustion,
[ABILITY_PONY_POWER] = PonyPower,
[ABILITY_POWDER_BURST] = PowderBurst,
[ABILITY_RETRIEVER] = Retriever,
[ABILITY_MONSTER_MASH] = MonsterMash,
[ABILITY_TWO_STEP] = TwoStep,
[ABILITY_SPITEFUL] = Spiteful,
[ABILITY_FORTITUDE] = Fortitude,
[ABILITY_DEVOURER] = Devourer,
[ABILITY_PHANTOM_THIEF] = PhantomThief,
[ABILITY_EARLY_GRAVE] = EarlyGrave,
[ABILITY_GRAPPLER] = Grappler,
[ABILITY_BASS_BOOSTED] = BassBoosted,
[ABILITY_FLAMING_JAWS] = FlamingJaws,
[ABILITY_MONSTER_HUNTER] = MonsterHunter,
[ABILITY_CROWNED_SWORD] = CrownedSword,
[ABILITY_CROWNED_SHIELD] = CrownedShield,
[ABILITY_BERSERK_DNA] = BerserkDna,
[ABILITY_CROWNED_KING] = CrownedKing,
[ABILITY_SNAP_TRAP_WHEN_HIT] = SnapTrapWhenHit,
[ABILITY_PERMANENCE] = Permanence,
[ABILITY_HUBRIS] = Hubris,
[ABILITY_COSMIC_DAZE] = CosmicDaze,
[ABILITY_MINDS_EYE] = MindsEye,
[ABILITY_BLOOD_PRICE] = BloodPrice,
[ABILITY_SPIKE_ARMOR] = SpikeArmor,
[ABILITY_VOODOO_POWER] = VoodooPower,
[ABILITY_CHROME_COAT] = ChromeCoat,
[ABILITY_BANSHEE] = Banshee,
[ABILITY_WEB_SPINNER] = WebSpinner,
[ABILITY_SHOWDOWN_MODE] = ShowdownMode,
[ABILITY_SEED_SOWER] = SeedSower,
[ABILITY_AIRBORNE] = Airborne,
[ABILITY_PARROTING] = Parroting,
[ABILITY_SALT_CIRCLE] = SaltCircle,
[ABILITY_PURIFYING_SALT] = PurifyingSalt,
[ABILITY_PROTOSYNTHESIS] = Protosynthesis,
[ABILITY_QUARK_DRIVE] = QuarkDrive,
[ABILITY_WIND_POWER] = WindPower,
[ABILITY_IMPULSE] = Impulse,
[ABILITY_TERMINAL_VELOCITY] = TerminalVelocity,
[ABILITY_GUARD_DOG] = GuardDog,
[ABILITY_ANGER_SHELL] = AngerShell,
[ABILITY_EGOIST] = Egoist,
[ABILITY_SUBDUE] = Subdue,
[ABILITY_READIED_ACTION] = ReadiedAction,
[ABILITY_DARK_GALE_WINGS] = DarkGaleWings,
[ABILITY_GUILT_TRIP] = GuiltTrip,
[ABILITY_WATER_GALE_WINGS] = WaterGaleWings,
[ABILITY_ZERO_TO_HERO] = ZeroToHero,
[ABILITY_COSTAR] = Costar,
[ABILITY_COMMANDER] = Commander,
[ABILITY_EJECT_PACK_ABILITY] = EjectPackAbility,
[ABILITY_VENGEFUL_SPIRIT] = VengefulSpirit,
[ABILITY_CUD_CHEW] = CudChew,
[ABILITY_ARMOR_TAIL] = ArmorTail,
[ABILITY_MIND_CRUSH] = MindCrush,
[ABILITY_SUPREME_OVERLORD] = SupremeOverlord,
[ABILITY_ILL_WILL] = IllWill,
[ABILITY_FIRE_SCALES] = FireScales,
[ABILITY_WATCH_YOUR_STEP] = WatchYourStep,
[ABILITY_RAPID_RESPONSE] = RapidResponse,
[ABILITY_DOUBLE_IRON_BARBS] = DoubleIronBarbs,
[ABILITY_THERMAL_EXCHANGE] = ThermalExchange,
[ABILITY_GOOD_AS_GOLD] = GoodAsGold,
[ABILITY_SHARING_IS_CARING] = SharingIsCaring,
[ABILITY_TABLETS_OF_RUIN] = TabletsOfRuin,
[ABILITY_SWORD_OF_RUIN] = SwordOfRuin,
[ABILITY_VESSEL_OF_RUIN] = VesselOfRuin,
[ABILITY_BEADS_OF_RUIN] = BeadsOfRuin,
[ABILITY_PERMAFROST_CLONE] = PermafrostClone,
[ABILITY_GALLANTRY] = Gallantry,
[ABILITY_ORICHALCUM_PULSE] = OrichalcumPulse,
[ABILITY_SUN_BASKING] = SunBasking,
[ABILITY_WINGED_KING] = WingedKing,
[ABILITY_HADRON_ENGINE] = HadronEngine,
[ABILITY_IRON_SERPENT] = IronSerpent,
[ABILITY_WEATHER_DOUBLE_BOOST] = WeatherDoubleBoost,
[ABILITY_SWEEPING_EDGE_PLUS] = SweepingEdgePlus,
[ABILITY_CELESTIAL_BLESSING] = CelestialBlessing,
[ABILITY_MINION_CONTROL] = MinionControl,
[ABILITY_MOLTEN_BLADES] = MoltenBlades,
[ABILITY_HAUNTING_FRENZY] = HauntingFrenzy,
[ABILITY_NOISE_CANCEL] = NoiseCancel,
[ABILITY_RADIO_JAM] = RadioJam,
[ABILITY_OLE] = Ole,
[ABILITY_MALICIOUS] = Malicious,
[ABILITY_DEAD_POWER] = DeadPower,
[ABILITY_BRAWLING_WYVERN] = BrawlingWyvern,
[ABILITY_MYTHICAL_ARROWS] = MythicalArrows,
[ABILITY_LAWNMOWER] = Lawnmower,
[ABILITY_FLOURISH] = Flourish,
[ABILITY_DESERT_SPIRIT] = DesertSpirit,
[ABILITY_CONTEMPT] = Contempt,
[ABILITY_AERIALIST] = Aerialist,
[ABILITY_TERA_SHELL] = TeraShell,
[ABILITY_TOXIC_CHAIN] = ToxicChain,
[ABILITY_PARASITIC_SPORES] = ParasiticSpores,
[ABILITY_POISON_PUPPETEER] = PoisonPuppeteer,
[ABILITY_ENTRANCE] = Entrance,
[ABILITY_REJECTION] = Rejection,
[ABILITY_APPLE_ENLIGHTENMENT] = AppleEnlightenment,
[ABILITY_BALLOON_BOMBER] = BalloonBomber,
[ABILITY_FLAMING_MAW] = FlamingMaw,
[ABILITY_DEMOLITIONIST] = Demolitionist,
[ABILITY_ROCKHARD_WILL] = RockhardWill,
[ABILITY_FRAGRANT_DAZE] = FragrantDaze,
[ABILITY_LOW_VISIBILITY] = LowVisibility,
[ABILITY_OLD_MARINER] = OldMariner,
[ABILITY_ECTOPLASM] = Ectoplasm,
[ABILITY_BEAUTIFUL_MUSIC] = BeautifulMusic,
[ABILITY_SURPRISE] = Surprise,
[ABILITY_SNOW_SONG] = SnowSong,
[ABILITY_GREATER_SPIRIT] = GreaterSpirit,
[ABILITY_RESONANCE] = Resonance,
[ABILITY_ETHEREAL_RUSH] = EtherealRush,
[ABILITY_CUTE_ANTECEDENCE] = CuteAntecedence,
[ABILITY_RECURRING_NIGHTMARE] = RecurringNightmare,
[ABILITY_MENACING_SITUATION] = MenacingSituation,
[ABILITY_SHINY_LIGHTNING] = ShinyLightning,
[ABILITY_TERRIFY] = Terrify,
[ABILITY_ICE_DOWNFALL] = IceDownfall,
[ABILITY_LAST_STAND] = LastStand,
[ABILITY_PYROCLASTIC_FLOW] = PyroclasticFlow,
[ABILITY_BLOOD_BATH] = BloodBath,
[ABILITY_BATTLE_AURA] = BattleAura,
[ABILITY_BLOODLUST] = Bloodlust,
[ABILITY_PIERCING_SOLO] = PiercingSolo,
[ABILITY_RHYTHMIC] = Rhythmic,
[ABILITY_CHUNKY_BASS_LINE] = ChunkyBassLine,
[ABILITY_DUAL_HAMMER] = DualHammer,
[ABILITY_DENTING_BLOWS] = DentingBlows,
[ABILITY_ICE_COLD_HUNTER] = IceColdHunter,
[ABILITY_SOUL_CRUSHER] = SoulCrusher,
[ABILITY_ARC_FLASH] = ArcFlash,
[ABILITY_UNICORN] = Unicorn,
[ABILITY_ON_THE_PROWL] = OnTheProwl,
[ABILITY_PRETENTIOUS] = Pretentious,
[ABILITY_VENOBLAZE_PINCERS] = VenoblazePincers,
[ABILITY_ETERNAL_BLESSING] = EternalBlessing,
[ABILITY_SUGAR_RUSH] = SugarRush,
[ABILITY_PEACEFUL_REST] = PeacefulRest,
[ABILITY_WHITE_NOISE] = WhiteNoise,
[ABILITY_SMOKEY_MANEUVERS] = SmokeyManeuvers,
[ABILITY_TAG] = Tag,
[ABILITY_POWER_METAL] = PowerMetal,
[ABILITY_POWER_EDGE] = PowerEdge,
[ABILITY_SUPERCONDUCTOR] = Superconductor,
[ABILITY_ULTRA_INSTINCT] = UltraInstinct,
[ABILITY_UNLOCKED_POTENTIAL] = UnlockedPotential,
[ABILITY_HIGHER_RANK] = HigherRank,
[ABILITY_FUNERAL_PYRE] = FuneralPyre,
[ABILITY_FLAME_BUBBLE] = FlameBubble,
[ABILITY_ELEMENTAL_VORTEX] = ElementalVortex,
[ABILITY_SNOWY_WRATH] = SnowyWrath,
[ABILITY_PATTERN_CHANGE] = PatternChange,
[ABILITY_NO_TURNING_BACK] = NoTurningBack,
[ABILITY_FLAMMABLE_COAT] = FlammableCoat,
[ABILITY_DRACO_MORALE] = DracoMorale,
[ABILITY_BAD_OMEN] = BadOmen,
[ABILITY_MOSH_PIT] = MoshPit,
[ABILITY_BLOOD_STAIN] = BloodStain,
[ABILITY_BLOOD_STIGMA] = BloodStigma,
[ABILITY_MAXIMUM_ACCELERATION] = MaximumAcceleration,
[ABILITY_SIDEWINDER] = Sidewinder,
[ABILITY_PETRIFY] = Petrify,
[ABILITY_FLUFFIEST] = Fluffiest,
[ABILITY_WAY_OF_PRECISION] = WayOfPrecision,
[ABILITY_WAY_OF_SWIFTNESS] = WayOfSwiftness,
[ABILITY_ATOMIC_PUNCH] = AtomicPunch,
[ABILITY_IRON_GIANT] = IronGiant,
[ABILITY_MASTER_HAND] = MasterHand,
[ABILITY_FINAL_BLOW] = FinalBlow,
[ABILITY_HOSPITALITY] = Hospitality,
[ABILITY_BUTTER_UP] = ButterUp,
[ABILITY_VITALITY_STRIKE] = VitalityStrike,
[ABILITY_HUGE_WINGS] = HugeWings,
[ABILITY_SWORD_OF_DAMNATION] = SwordOfDamnation,
[ABILITY_RESTRAINING_ORDER] = RestrainingOrder,
[ABILITY_ASSASSINS_TOOLS] = AssassinsTools,
[ABILITY_FROSTMAW] = Frostmaw,
[ABILITY_PATCHWORK] = Patchwork,
[ABILITY_BLIND_RAGE] = BlindRage,
[ABILITY_SLIPSTREAM] = Slipstream,
[ABILITY_APEX_PREDATOR] = ApexPredator,
[ABILITY_DRAGONS_RITUAL] = DragonsRitual,
[ABILITY_PINNACLE_BLADE] = PinnacleBlade,
[ABILITY_ENERGIZED] = Energized,
[ABILITY_COLOR_SPECTRUM] = ColorSpectrum,
[ABILITY_STEEL_BEETLE] = SteelBeetle,
[ABILITY_FROM_THE_SHADOWS] = FromTheShadows,
[ABILITY_RAGE_POINT] = RagePoint,
[ABILITY_HOT_COALS] = HotCoals,
[ABILITY_TERASTAL_TREASURE] = TerastalTreasure,
[ABILITY_SHOCKING_MAW] = ShockingMaw,
[ABILITY_GLEAM_EYES] = GleamEyes,
[ABILITY_ROUSED_FANGS] = RousedFangs,
[ABILITY_DREAM_STATE] = DreamState,
[ABILITY_DREAM_WHIMSY] = DreamWhimsy,
[ABILITY_LUNAR_AFFINITY] = LunarAffinity,
[ABILITY_FLAME_SHIELD] = FlameShield,
[ABILITY_AQUATIC_DWELLER] = AquaticDweller,
[ABILITY_APPLE_PIE] = ApplePie,
[ABILITY_HOVER] = Hover,
[ABILITY_DEPRAVITY] = Depravity,
[ABILITY_WILDFIRE] = Wildfire,
[ABILITY_JUMP_SCARE] = JumpScare,
[ABILITY_TAR_TOSS] = TarToss,
[ABILITY_STUN_SHOCK] = StunShock,
[ABILITY_RAGING_GODDESS] = RagingGoddess,
[ABILITY_WHIPLASH] = Whiplash,
[ABILITY_SUPERSWEET_SYRUP] = SupersweetSyrup,
[ABILITY_LUCKY_HALO] = LuckyHalo,
[ABILITY_TRASH_HEAP] = TrashHeap,
[ABILITY_SLUDGY_MIX] = SludgyMix,
[ABILITY_OVERWATCH] = Overwatch,
[ABILITY_WIND_RAGE] = WindRage,
[ABILITY_VICTORY_BOMB] = VictoryBomb,
[ABILITY_RAZOR_SHARP] = RazorSharp,
[ABILITY_TO_THE_BONE] = ToTheBone,
[ABILITY_BLADE_DANCE] = BladeDance,
[ABILITY_TAEKKYEON] = Taekkyeon,
[ABILITY_APE_SHIFT] = ApeShift,
[ABILITY_KNOW_YOUR_PLACE] = KnowYourPlace,
[ABILITY_DEEP_CUTS] = DeepCuts,
[ABILITY_LIFE_STEAL] = LifeSteal,
[ABILITY_RUDE_AWAKENING] = RudeAwakening,
[ABILITY_TERAFORM_ZERO] = TeraformZero,
[ABILITY_SET_ABLAZE] = SetAblaze,
[ABILITY_BREAKWATER] = Breakwater,
[ABILITY_MAGICAL_FISTS] = MagicalFists,
[ABILITY_CUTTHROAT] = Cutthroat,
[ABILITY_SAND_BENDER] = SandBender,
[ABILITY_SAND_PIT] = SandPit,
[ABILITY_DESOLATE_SUN] = DesolateSun,
[ABILITY_DAYBREAK] = Daybreak,
[ABILITY_ENERGY_SIPHON] = EnergySiphon,
[ABILITY_RESERVOIR] = Reservoir,
[ABILITY_NEUROTOXIN] = Neurotoxin,
[ABILITY_ENERGIZED_HORNS] = EnergizedHorns,
[ABILITY_SPIDER_LAIR_UPGRADE] = SpiderLairUpgrade,
[ABILITY_CRUST_COAT] = CrustCoat,
[ABILITY_PUFFY] = Puffy,
[ABILITY_BALLOON_BLITZ] = BalloonBlitz,
};