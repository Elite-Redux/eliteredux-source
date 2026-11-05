#include "battle_skills.hh"

#undef __SIZE_TYPE__
#define __SIZE_TYPE__ uint32_t
#include <cstddef>
#include <array>
#include <utility>

extern "C" {
#include "global.h"
#include "generated/constants/battle_skills.h"
#include "generated/constants/battle_skill_templates.h"
#include "battle_scripts.h"
#include "battle_script_commands.h"
#include "battle_util.h"
#include "battle.h"
#include "battle_anim.h"
#include "pokemon.h"
#include "random.h"
#include "constants/battle_string_ids.h"
}

#include "type_utils.hh"

#define ON_BATTLE_START opt BattleSkillEnum skill
#define ON_ENTRY opt BattleSkillEnum skill, opt u8 battler
#define ON_ATTACKER opt BattleSkillEnum skill, opt u8 battler, opt u8 target, opt MoveEnum move, opt Type moveType
#define ON_DEFENSIVE_MULTIPLIER opt u8 battler, opt u16 *resistance, opt u16 *modifier
#define ON_END_TURN opt BattleSkillEnum skill, opt u8 battler

#define MUL(val) MUL_MODIFIER(modifier, val)

static void Unregister(BattleSkillEnum skill) {
    for (int i = 0; i < ARRAY_COUNT(gActiveSkills); i++) {
        FILTER(gActiveSkills[i] == skill)
        gActiveSkills[i] = SKILL_NONE;
        break;
    }
}

static int RunEntryAnnounceScript(BattleSkillEnum skill, const u8* script) {
    gBattleScripting.abilityPopupOverwrite = skill;
    BattleScriptPushCursorAndCallback(script);
    return TRUE;
}
#define BATTLE_START_SIMPLE(script) .onBattleStart = +[](ON_BATTLE_START) -> int { return RunEntryAnnounceScript(skill, script); }

template <BattleSkillEnum Id>
constexpr BattleSkill Impl = {0};

/*
General idea: Create "base" templates like MoveOnEntry that take arguments, and use these in the
implementations of specific skills, for example:

template <>
constexpr BattleSkill Impl<BATTLE_SKILL_ANCIENT_POWER_ON_ENTRY> = MoveOnEntry<MOVE_ANCIENT_POWER, 60>

This should allow skills to act similarly to abilities (for example: can look up Battle Skills using
just the enum value) while also allowing arguments to be passed so we can reuse the base templates
and ideally write less code

Hopefully this works without any issues :)
*/

template <BattleSkillTemplateEnum Template, auto... Args>
constexpr BattleSkill BattleSkillTemplate;

template <AbilityEnum Ability>
constexpr BattleSkill BattleSkillTemplate<SKILL_TEMPLATE_EXTRA_ABILITY, Ability> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, Ability);

        return RunEntryAnnounceScript(skill, BattleScript_AnnounceBattleSkillEnd3);
    },
};

template <AbilityEnum Ability1, AbilityEnum Ability2>
constexpr BattleSkill BattleSkillTemplate<SKILL_TEMPLATE_EXTRA_ABILITY, Ability1, Ability2> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, Ability1);
        PREPARE_ABILITY_BUFFER(gBattleTextBuff2, Ability2);

        return RunEntryAnnounceScript(skill, BattleScript_AnnounceBattleSkillEnd3);
    },
};

template <AbilityEnum Ability1, AbilityEnum Ability2, AbilityEnum Ability3>
constexpr BattleSkill BattleSkillTemplate<SKILL_TEMPLATE_EXTRA_ABILITY, Ability1, Ability2, Ability3> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, Ability1);
        PREPARE_ABILITY_BUFFER(gBattleTextBuff2, Ability2);
        PREPARE_ABILITY_BUFFER(gBattleTextBuff3, Ability3);

        return RunEntryAnnounceScript(skill, BattleScript_AnnounceBattleSkillEnd3);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_EVIOLITE> = {
    BATTLE_START_SIMPLE(BattleScript_ExtraSkillEviolite),
    .onDefensiveMultiplier =
        +[](ON_DEFENSIVE_MULTIPLIER) {
            if (CanEvolveStrict(gBattleMons[battler].species) && (gBattleMons[battler].species != SPECIES_NECROZMA)) MUL(1.5);
        },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_STICKY_WEB> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STICKY_WEB)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STICKY_WEB;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaStickyWeb);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, add sticky web if there is no sticky web, to make the skill "permanent"
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STICKY_WEB)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STICKY_WEB;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaStickyWeb);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_STICKY_WEB_BOTH_SIDES> = {
    // TODO: Should become permanent as well I think
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STICKY_WEB)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STICKY_WEB;
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_STICKY_WEB)
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_STICKY_WEB;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaStickyWebBothSides);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_SPIKES_1_LAYER> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK(gSideTimers[B_SIDE_PLAYER].spikesAmount < 3)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_SPIKES;
        gSideTimers[B_SIDE_PLAYER].spikesAmount++;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSpikes);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, add one layer of spikes if there are no spikes, to make the skill "permanent"
        CHECK(gSideTimers[B_SIDE_PLAYER].spikesAmount < 1)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_SPIKES;
        gSideTimers[B_SIDE_PLAYER].spikesAmount++;
        // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSpikes);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_TOXIC_SPIKES_2_LAYERS> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK(gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount < 2)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount = 2;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaToxicSpikes);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, add two layers of toxic spikes if there are no toxic spikes, to make the skill "permanent"
        CHECK(gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount < 1)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount = 2;
        // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaToxicSpikes);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_STEALTH_ROCKS> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        gSideTimers[B_SIDE_PLAYER].stealthRockType = TYPE_ROCK;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaStealthRock);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, add stealth rocks if there are no stealth rocks, to make the skill "permanent"
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        gSideTimers[B_SIDE_PLAYER].stealthRockType = TYPE_ROCK;
        // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaStealthRock);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_CALTROPS> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideTimers[B_SIDE_PLAYER].caltrops)
        gSideTimers[B_SIDE_PLAYER].caltrops = TRUE;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaCaltrops);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, add caltrops if there are no caltrops, to make the skill "permanent"
        CHECK_NOT(gSideTimers[B_SIDE_PLAYER].caltrops)
        gSideTimers[B_SIDE_PLAYER].caltrops = TRUE;
        // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaCaltrops);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_HAZARDS_ALL> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        gSideTimers[B_SIDE_PLAYER].stealthRockType = TYPE_ROCK;

        CHECK(gSideTimers[B_SIDE_PLAYER].spikesAmount < 3)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_SPIKES;
        gSideTimers[B_SIDE_PLAYER].spikesAmount++;

        CHECK(gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount < 2)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount++;

        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STICKY_WEB)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STICKY_WEB;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillHazardsAll);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_INVERSE_ROOM> = {
    // TODO: Set Inverse Room again if removed
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);

        CHECK_NOT(gFieldStatuses & STATUS_FIELD_INVERSE_ROOM)
        gFieldTimers.started.inverseRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_INVERSE_ROOM;
        gFieldTimers.inverseRoomTimer = ROOM_DURATION_MAX;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaInverseRoom);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_TRICK_ROOM> = {
    // TODO: Set Trick Room again if removed
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);

        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TRICK_ROOM)
        gFieldTimers.started.trickRoom = TRUE;
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        gFieldTimers.inverseRoomTimer = ROOM_DURATION_MAX;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaTrickRoom);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_RAINBOW> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);

        CHECK_NOT(gSideTimers[B_SIDE_OPPONENT].rainbowTimer)
        gSideTimers[B_SIDE_OPPONENT].started.rainbow = TRUE;
        gSideTimers[B_SIDE_OPPONENT].rainbowTimer = 255;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaRainbow);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_SWAMP> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);

        CHECK_NOT(gSideTimers[B_SIDE_PLAYER].swampTimer)
        gSideTimers[B_SIDE_PLAYER].started.swamp = TRUE;
        gSideTimers[B_SIDE_PLAYER].swampTimer = 255;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSwamp);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_FIRE_SEA> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);

        CHECK_NOT(gSideTimers[B_SIDE_PLAYER].fireSeaTimer)
        gSideTimers[B_SIDE_PLAYER].started.fireSea = TRUE;
        gSideTimers[B_SIDE_PLAYER].fireSeaTimer = 255;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaFireSea);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_MIST> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_MIST)
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_MIST;
        gSideTimers[B_SIDE_OPPONENT].started.mist = TRUE;
        gSideTimers[B_SIDE_OPPONENT].mistTimer = SCREEN_DURATION_MAX;
        gSideTimers[B_SIDE_OPPONENT].mistBattlerId = B_POSITION_OPPONENT_LEFT;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaMist);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, start mist if it's not up to make the skill "permanent"
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_MIST)
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_MIST;
        gSideTimers[B_SIDE_OPPONENT].started.mist = TRUE;
        gSideTimers[B_SIDE_OPPONENT].mistTimer = SCREEN_DURATION_MAX;
        gSideTimers[B_SIDE_OPPONENT].mistBattlerId = B_POSITION_OPPONENT_LEFT;

        // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaMist);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_TAILWIND> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_TAILWIND)
        gSideTimers[B_SIDE_OPPONENT].started.tailwind = TRUE;
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_TAILWIND;
        gSideTimers[B_SIDE_OPPONENT].tailwindBattlerId = B_POSITION_OPPONENT_LEFT;
        gSideTimers[B_SIDE_OPPONENT].tailwindTimer = TAILWIND_DURATION_MAX;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaTailwind);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // Don't think tailwind can actually be removed? But just in case
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_TAILWIND)
        gSideTimers[B_SIDE_OPPONENT].started.tailwind = TRUE;
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_TAILWIND;
        gSideTimers[B_SIDE_OPPONENT].tailwindBattlerId = B_POSITION_OPPONENT_LEFT;
        gSideTimers[B_SIDE_OPPONENT].tailwindTimer = TAILWIND_DURATION_MAX;

         // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaTailwind);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_SAFEGUARD> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_SAFEGUARD)
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_SAFEGUARD;
        gSideTimers[B_SIDE_OPPONENT].started.safeguard = TRUE;
        gSideTimers[B_SIDE_OPPONENT].safeguardTimer = SCREEN_DURATION_MAX;
        gSideTimers[B_SIDE_OPPONENT].safeguardBattlerId = B_POSITION_OPPONENT_LEFT;

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSafeguard);
    },
    .onEndTurn = +[](ON_END_TURN) -> int {
        // On turn end, start mist if it's not up to make the skill "permanent"
        CHECK_NOT(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_SAFEGUARD)
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_SAFEGUARD;
        gSideTimers[B_SIDE_OPPONENT].started.safeguard = TRUE;
        gSideTimers[B_SIDE_OPPONENT].safeguardTimer = SCREEN_DURATION_MAX;
        gSideTimers[B_SIDE_OPPONENT].safeguardBattlerId = B_POSITION_OPPONENT_LEFT;

        // TODO: Change to different string?
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSafeguard);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_SUN> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        SetPermanentWeather(ENUM_WEATHER_SUN);

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSun);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_RAIN> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        SetPermanentWeather(ENUM_WEATHER_RAIN);

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaRain);
    },
};


template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_SANDSTORM> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        SetPermanentWeather(ENUM_WEATHER_SANDSTORM);

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaSandstorm);
    },
};


template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_HAIL> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        SetPermanentWeather(ENUM_WEATHER_HAIL);

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaHail);
    },
};


template <>
constexpr BattleSkill Impl<SKILL_PERMANENT_FOG> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        SetPermanentWeather(ENUM_WEATHER_FOG);

        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaFog);
    },
};
// Inline skills are written in the trainer data: proto/TrainerList.textproto with InlineSkill
// from proto/BattleSkillList.proto
// Codegen turns each `on` block into an InlineTrigger and builds the skill with the InlineSkill template below.

// The event's Pokemon (SUBJECT), its foe or both. Then that Pokemon only, its side of the field, or its whole party
struct InlineTarget {
    enum Who : u8 { SUBJECT, FOE, BOTH } who;
    enum Scope : u8 { MON, FIELD, PARTY } scope;
    u8 slotsFirst;  // PARTY: first and last N of the party, both 0 = all
    u8 slotsLast;
};

struct InlineEffect {
    enum Kind : u8 { STAT_CHANGE, SIDE_CONDITION, WEATHER, TERRAIN, USE_MOVE, FIELD, STATUS } kind;
    // Check proto definition for more info on taret
    InlineTarget target;
    // STAT_CHANGE
    u8 statCount;
    u8 stats[NUM_BATTLE_STATS - 1];  // STAT_*, STAT_HIGHEST_* are picked when it runs
    s8 amount;
    // SIDE_CONDITION, FIELD
    u16 types;  // bit per SKILL_SIDE_* or SKILL_FIELD_*
    // SIDE_CONDITION i think
    u8 layers;  // hazards: brought up to this many, capped at 3 for spikes and 2 for toxic spikes
    // WEATHER, TERRAIN
    u8 weather;  // ENUM_WEATHER_*
    u8 terrain;  // ENUM_TERRAIN_*
    // WEATHER, TERRAIN, SIDE_CONDITION, FIELD
    u8 turns;  // without permanent
    // WEATHER, TERRAIN: locked, nothing replaces or removes it.
    // SIDE_CONDITION, FIELD: put back at the end of each turn
    bool8 permanent;
    // USE_MOVE
    MoveEnum move;
    u8 power;          // 0 keeps the move's power
    bool8 falseSwipe;  // leaves the target at 1 HP at least
    // STATUS
    enum StatusKind : u8 { NO_STATUS, SLEEP, POISON, TOXIC, BURN, PARALYSIS, FROSTBITE, BLEED, CONFUSION, CURSE, SALT_CURE, ATTRACT, DRENCH } status;
};

// extra_ability of the skill. The abilities themselves come from Trainer.hellAbilities (GetExtraAbilityToSetToBattler),
// this is for the announce.
struct InlineAbilities {
    u8 count;
    AbilityEnum ability[HELL_MODE_EXTRA_ABILITIES];
};

#define INLINE_MAX_EFFECTS 4
#define INLINE_MAX_ONCE 4  // once-only triggers per skill, see OnceBit

struct InlineTrigger {
    enum Kind : u8 { ENTRY, END_TURN, BATTLE_START, HIT, FAINT, STAT_CHANGE, SWITCH_OUT } kind;
    enum When : u8 { EVERY, LEAD, LAST } when;  // ENTRY
    enum Side : u8 { TRAINER, PLAYER, BOTH } side;
    enum Once : u8 { ALWAYS, PER_SWITCH_IN, PER_POKEMON, PER_BATTLE } once;
    u8 onceIndex;  // 0 to INLINE_MAX_ONCE - 1 in the skill
    u8 chance;     // percent, 0 = always
    u8 hpBelow;    // percent, 0 = no check
    u8 slotsFirst;  // ENTRY: first and last N of the party, both 0 = all
    u8 slotsLast;
    u8 every;      // END_TURN: every N turns, 0 or 1 = every turn
    bool8 onField;  // END_TURN: count the turns on the field, not the battle turns
    // HIT: the move matches every one that's set
    // use a simple bitmask and check everything at once.
    u32 types;      // bit per TYPE_*, 0 = any
    u8 splits;      // bit per SPLIT_*, 0 = any
    u8 flags;       // bit per MoveFlag. 0 = any
    bool8 firstStrikeOnly;
    enum Effectiveness : u8 { SUPER_EFFECTIVE, NEUTRAL, RESISTED, IMMUNE };
    u8 effectiveness;  // HIT: bit per Effectiveness, 0 = any that dealt damage
    u8 stats;      // STAT_CHANGE: bit per STAT_*, 0 = any
    s8 direction;  // STAT_CHANGE: 1 up, -1 down, 0 both
    u8 effectCount;
    InlineEffect effects[INLINE_MAX_EFFECTS];
};

struct InlineEvent {
    InlineTrigger::Kind kind;
    u8 battler;  // the Pokemon it happens to
    u8 other;    // the attacker, the KO'er, or the foe facing it
    u8 moveType;
    MoveEnum move;
    u8 stat;
    s8 change;
};

// One hook firing: the effects change the battle in C and queue their scripts with Show.
// The skill is announced once, before all of them.
struct InlineRun {
    BattleSkillEnum skill;
    InlineEvent event;
    // How Show starts the scripts: pushed with a callback when no script runs (entry, end of turn, battle start),
    // called from the script that's running (a hit, a faint...), or not at all (a switch out, use_move only)
    enum ScriptMode : u8 { PUSH_CALLBACK, CALL_SCRIPT, NO_SCRIPT } scriptMode;
    bool8 shown;
    bool8 queued;
};

// This is made to ensure we're waiting at the end of effect chains that aren't waiting
// Example: Many stats up don't have a wait message explicitely, this catches it.
// Without a script, only the announce shows.
static void Show(InlineRun& run, const u8* script) {
    if (run.scriptMode == InlineRun::NO_SCRIPT) return;
    if (!run.shown) {
        if (run.scriptMode == InlineRun::PUSH_CALLBACK)
            BattleScriptPushCursorAndCallback(BattleScript_WaitMessageEnd3);
        else
            BattleScriptCall(BattleScript_WaitMessageReturn);
    }
    run.shown = TRUE;
    if (script) BattleScriptCall(script);
}

// A skill's own scripts sit on top of one of these, so it doesn't react to what it does itself
static bool8 InSkillChain() {
    struct BattleScriptsStack* stack = gBattleResources->battleScriptsStack;
    for (int i = 0; i < stack->size; i++)
        if (stack->ptr[i] == BattleScript_WaitMessageEnd3 || stack->ptr[i] == BattleScript_WaitMessageReturn) return TRUE;
    return FALSE;
}

static u8 FacingFoe(u8 battler) {
    u8 foe = BATTLE_OPPOSITE(battler);
    if (!IsBattlerAlive(foe) && IsBattlerAlive(foe ^ BIT_FLANK)) foe ^= BIT_FLANK;
    return foe;
}

static u8 SideBattler(u8 side) {
    u8 battler = GetBattlerAtPosition(side == B_SIDE_PLAYER ? B_POSITION_PLAYER_LEFT : B_POSITION_OPPONENT_LEFT);
    if (!IsBattlerAlive(battler) && IsBattlerAlive(battler ^ BIT_FLANK)) battler ^= BIT_FLANK;
    return battler;
}

// returns the correct pokemon based on required side
static u8 EventMon(const InlineRun& run, u8 side) { return GetBattlerSide(run.event.battler) == side ? run.event.battler : run.event.other; }

// Resolves valid sides (one or all)
static u8 TargetSides(const InlineRun& run, InlineTarget::Who who) {
    u8 sides = 0;
    if (who != InlineTarget::FOE) sides |= 1 << GetBattlerSide(run.event.battler);
    if (who != InlineTarget::SUBJECT) sides |= 1 << GetBattlerSide(run.event.other);
    return sides;
}

static u8 TargetBattlers(const InlineRun& run, const InlineTarget& target) {
    if (target.scope == InlineTarget::MON) {
        u8 battlers = 0;
        if (target.who != InlineTarget::FOE) battlers |= 1 << run.event.battler;
        if (target.who != InlineTarget::SUBJECT) battlers |= 1 << run.event.other;
        return battlers;
    }
    u8 sides = TargetSides(run, target.who), battlers = 0;
    for (int battler = 0; battler < gBattlersCount; battler++)
        if (sides & (1 << GetBattlerSide(battler))) battlers |= 1 << battler;
    return battlers;
}

// gEnemyPartyCount isn't set yet at battle start, so count again
static int PartyCount(u8 side) { return side == B_SIDE_PLAYER ? CalculatePlayerPartyCount() : CalculateEnemyPartyCount(); }

static bool8 InSlots(u8 first, u8 last, u8 index, u8 count) {
    if (!first && !last) return TRUE;
    return index < first || index + last >= count;
}

// Screens, Safeguard, Tailwind...: a status bit, a timer and the battler that set it
#define TIMED_SIDE_STATUS(type, flag, name) \
    case type:                              \
        CHECK_NOT(*status & flag)           \
        *status |= flag;                    \
        timer->name##Timer = turns;         \
        timer->name##BattlerId = battler;   \
        timer->started.name = TRUE;         \
        return TRUE;
// Swamp, Sea of Fire and Rainbow only have a timer
#define TIMER_ONLY(type, name)              \
    case type:                              \
        CHECK_NOT(timer->name##Timer)       \
        timer->name##Timer = turns;         \
        timer->started.name = TRUE;         \
        return TRUE;

// Returns true if we increased or applied some layers
static int AddInlineSideCondition(u8 type, u8 side, const InlineEffect& effect) {
    struct SideTimer* timer = &gSideTimers[side];
    u32* status = &gSideStatuses[side];
    u8 turns = effect.permanent ? 255 : effect.turns;
    u8 battler = SideBattler(side);
    switch (type) {
        case SKILL_SIDE_SPIKES:
            CHECK(timer->spikesAmount < min(effect.layers, 3))
            timer->spikesAmount = min(effect.layers, 3);
            *status |= SIDE_STATUS_SPIKES;
            return TRUE;
        case SKILL_SIDE_TOXIC_SPIKES:
            CHECK(timer->toxicSpikesAmount < min(effect.layers, 2))
            timer->toxicSpikesAmount = min(effect.layers, 2);
            *status |= SIDE_STATUS_TOXIC_SPIKES;
            return TRUE;
        case SKILL_SIDE_STEALTH_ROCK:
            CHECK_NOT(*status & SIDE_STATUS_STEALTH_ROCK)
            *status |= SIDE_STATUS_STEALTH_ROCK;
            timer->stealthRockType = TYPE_ROCK;
            return TRUE;
        case SKILL_SIDE_STICKY_WEB:
            CHECK_NOT(*status & SIDE_STATUS_STICKY_WEB)
            *status |= SIDE_STATUS_STICKY_WEB;
            return TRUE;
        case SKILL_SIDE_CALTROPS:
            CHECK_NOT(timer->caltrops)
            timer->caltrops = TRUE;
            return TRUE;
        TIMED_SIDE_STATUS(SKILL_SIDE_REFLECT, SIDE_STATUS_REFLECT, reflect)
        TIMED_SIDE_STATUS(SKILL_SIDE_LIGHT_SCREEN, SIDE_STATUS_LIGHTSCREEN, lightscreen)
        TIMED_SIDE_STATUS(SKILL_SIDE_AURORA_VEIL, SIDE_STATUS_AURORA_VEIL, auroraVeil)
        TIMED_SIDE_STATUS(SKILL_SIDE_SAFEGUARD, SIDE_STATUS_SAFEGUARD, safeguard)
        TIMED_SIDE_STATUS(SKILL_SIDE_MIST, SIDE_STATUS_MIST, mist)
        TIMED_SIDE_STATUS(SKILL_SIDE_TAILWIND, SIDE_STATUS_TAILWIND, tailwind)
        TIMED_SIDE_STATUS(SKILL_SIDE_LUCKY_CHANT, SIDE_STATUS_LUCKY_CHANT, luckyChant)
        TIMER_ONLY(SKILL_SIDE_SWAMP, swamp)
        TIMER_ONLY(SKILL_SIDE_SEA_OF_FIRE, fireSea)
        TIMER_ONLY(SKILL_SIDE_RAINBOW, rainbow)
    }
    return FALSE;
}
#undef TIMED_SIDE_STATUS
#undef TIMER_ONLY

// The ones a move puts on the foe's side, for the animation to play correctly
static bool8 GoesOnFoeSide(u8 type) {
    return type <= SKILL_SIDE_CALTROPS || type == SKILL_SIDE_SWAMP || type == SKILL_SIDE_SEA_OF_FIRE;
}

static void RunInlineSideCondition(InlineRun& run, const InlineEffect& effect) {
    for (int type = SKILL_SIDE_COUNT - 1; type >= 0; type--) {
        FILTER(effect.types & (1 << type))
        bool8 onFoeSide = GoesOnFoeSide(type);
        u8 sides = TargetSides(run, effect.target.who);
        for (int side = B_SIDE_OPPONENT; side >= B_SIDE_PLAYER; side--) {
            FILTER(sides & (1 << side))
            FILTER(AddInlineSideCondition(type, side, effect))
            // The animation comes from the side that would use the move
            gStackBattler1 = SideBattler(onFoeSide ? side ^ 1 : side);
            gStackBattler2 = SideBattler(onFoeSide ? side : side ^ 1);
            gBattleCommunication[MULTISTRING_CHOOSER] = type;
            Show(run, BattleScript_SkillSideCondition);
        }
    }
}

#define FIELD_STATUS(type, flag, name)      \
    case type:                              \
        CHECK_NOT(gFieldStatuses & flag)    \
        gFieldStatuses |= flag;             \
        gFieldTimers.name##Timer = turns;   \
        gFieldTimers.started.name = TRUE;   \
        return TRUE;

// Rooms are toggles in the engine: using Trick Room while it's up ends it. Skills only ever set them and
// skip one that's already up, so they never turn a room off. A permanent room the player turns off comes
// back at the end of that turn.
static int AddInlineFieldStatus(u8 type, u8 turns) {
    switch (type) {
        FIELD_STATUS(SKILL_FIELD_TRICK_ROOM, STATUS_FIELD_TRICK_ROOM, trickRoom)
        FIELD_STATUS(SKILL_FIELD_WONDER_ROOM, STATUS_FIELD_WONDER_ROOM, wonderRoom)
        FIELD_STATUS(SKILL_FIELD_MAGIC_ROOM, STATUS_FIELD_MAGIC_ROOM, magicRoom)
        FIELD_STATUS(SKILL_FIELD_INVERSE_ROOM, STATUS_FIELD_INVERSE_ROOM, inverseRoom)
        FIELD_STATUS(SKILL_FIELD_GRAVITY, STATUS_FIELD_GRAVITY, gravity)
        FIELD_STATUS(SKILL_FIELD_MUD_SPORT, STATUS_FIELD_MUDSPORT, mudSport)
        FIELD_STATUS(SKILL_FIELD_WATER_SPORT, STATUS_FIELD_WATERSPORT, waterSport)
    }
    return FALSE;
}
#undef FIELD_STATUS

static void RunInlineFieldStatus(InlineRun& run, const InlineEffect& effect) {
    for (int type = SKILL_FIELD_COUNT - 1; type >= 0; type--) {
        FILTER(effect.types & (1 << type))
        FILTER(AddInlineFieldStatus(type, effect.permanent ? 255 : effect.turns))
        gStackBattler1 = EventMon(run, B_SIDE_OPPONENT);
        gBattleCommunication[MULTISTRING_CHOOSER] = type;
        Show(run, BattleScript_SkillField);
    }
}

// Like TranslateStatId for weather
static const u8 sInlineWeatherAnims[] = {
    [ENUM_WEATHER_NONE] = 0,
    [ENUM_WEATHER_RAIN] = B_ANIM_RAIN_CONTINUES,
    [ENUM_WEATHER_SUN] = B_ANIM_SUN_CONTINUES,
    [ENUM_WEATHER_SANDSTORM] = B_ANIM_SANDSTORM_CONTINUES,
    [ENUM_WEATHER_HAIL] = B_ANIM_HAIL_CONTINUES,
    [ENUM_WEATHER_SUN_PRIMAL] = B_ANIM_SUN_CONTINUES,
    [ENUM_WEATHER_RAIN_PRIMAL] = B_ANIM_RAIN_CONTINUES,
    [ENUM_WEATHER_STRONG_WINDS] = B_ANIM_STRONG_WINDS,
    [ENUM_WEATHER_FOG] = B_ANIM_FOG_CONTINUES,
};

static int SetInlineWeather(const InlineEffect& effect, u8 battler) {
    if (effect.permanent) return SetPermanentWeather(effect.weather);
    CHECK(TryChangeBattleWeather(battler, effect.weather, TRUE))
    gWishFutureKnock.weatherDuration = effect.turns;
    return TRUE;
}

// Mapping to grab the string and status
// Like TranslateStatId for terrain
static const struct {
    u16 flag;
    u8 message;
} sInlineTerrains[] = {
    [ENUM_TERRAIN_NONE] = {0, 0},
    [ENUM_TERRAIN_ELECTRIC] = {STATUS_FIELD_ELECTRIC_TERRAIN, B_MSG_TERRAINBECOMESELECTRIC},
    [ENUM_TERRAIN_GRASSY] = {STATUS_FIELD_GRASSY_TERRAIN, B_MSG_TERRAINBECOMESGRASSY},
    [ENUM_TERRAIN_MISTY] = {STATUS_FIELD_MISTY_TERRAIN, B_MSG_TERRAINBECOMESMISTY},
    [ENUM_TERRAIN_PSYCHIC] = {STATUS_FIELD_PSYCHIC_TERRAIN, B_MSG_TERRAINBECOMESPSYCHIC},
    [ENUM_TERRAIN_TOXIC] = {STATUS_FIELD_TOXIC_TERRAIN, B_MSG_TERRAINBECOMESTOXIC},
};

static int SetInlineTerrain(const InlineEffect& effect, u8 battler) {
    u16 flag = sInlineTerrains[effect.terrain].flag;
    if (effect.permanent) {
        CHECK_NOT(gFieldStatuses & STATUS_FIELD_TERRAIN_PERMANENT)
        gFieldStatuses = (gFieldStatuses & ~STATUS_FIELD_TERRAIN_ANY) | flag | STATUS_FIELD_TERRAIN_PERMANENT;
        gBattleScripting.battler = battler;
    } else {
        CHECK(TryChangeBattleTerrain(battler, flag, &gFieldTimers.terrainTimer))
        gFieldTimers.terrainTimer = effect.turns;
    }
    return TRUE;
}

// The move runs as its own action once the current one ends. It's used by the trainer's Pokemon of the
// event at the player's one, or the first foe still standing
static int QueueInlineMove(InlineRun& run, const InlineEffect& effect) {
    u8 user = EventMon(run, B_SIDE_OPPONENT);
    u8 target = EventMon(run, B_SIDE_PLAYER);
    // A fainted Pokemon can still explode, like with abilities (HandleAction_UseMove)
    CHECK(IsBattlerAlive(user) || gBattleMoves[effect.move].effect == EFFECT_EXPLOSION)
    CHECK_NOT(gRoundStructs[user].attackCancelled)
    if (!IsBattlerAlive(target)) target = FacingFoe(user);
    CHECK(IsBattlerAlive(target))
    CHECK(gQueuedAttackCount < MAX_BATTLERS_COUNT)
    // Each move once at a time per skill and Pokemon, multi-hit moves would fill the queue
    for (int i = 1; i <= gQueuedAttackCount; i++)
        CHECK_NOT(gQueuedExtraAttackData[i].skill == run.skill && gQueuedExtraAttackData[i].attacker == user && gQueuedExtraAttackData[i].move == effect.move)
    gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
        .skill = run.skill,
        .move = effect.move,
        .movePower = effect.power,
        .attacker = user,
        .target = target,
        .falseSwipe = effect.falseSwipe,
    };
    run.queued = TRUE;
    return TRUE;
}

static const struct {
    MoveEffectEnum moveEffect;
    u32 status1;  // majors
    u8 immuneTypes[2];  // TYPE_*, TYPE_NONE for none
} sInlineStatuses[] = {
    [InlineEffect::NO_STATUS] = {MOVE_EFFECT_NONE, 0, {TYPE_NONE, TYPE_NONE}},
    [InlineEffect::SLEEP] = {MOVE_EFFECT_SLEEP, STATUS1_SLEEP_TURN(3), {TYPE_NONE, TYPE_NONE}},
    [InlineEffect::POISON] = {MOVE_EFFECT_POISON, STATUS1_POISON, {TYPE_POISON, TYPE_STEEL}},
    [InlineEffect::TOXIC] = {MOVE_EFFECT_TOXIC, STATUS1_TOXIC_POISON, {TYPE_POISON, TYPE_STEEL}},
    [InlineEffect::BURN] = {MOVE_EFFECT_BURN, STATUS1_BURN, {TYPE_FIRE, TYPE_FIRE}},
    [InlineEffect::PARALYSIS] = {MOVE_EFFECT_PARALYSIS, STATUS1_PARALYSIS, {TYPE_ELECTRIC, TYPE_ELECTRIC}},
    [InlineEffect::FROSTBITE] = {MOVE_EFFECT_FROSTBITE, STATUS1_FROSTBITE, {TYPE_ICE, TYPE_ICE}},
    [InlineEffect::BLEED] = {MOVE_EFFECT_BLEED, STATUS1_BLEED, {TYPE_ROCK, TYPE_GHOST}},
    [InlineEffect::CONFUSION] = {MOVE_EFFECT_CONFUSION, 0, {TYPE_NONE, TYPE_NONE}},
    [InlineEffect::CURSE] = {MOVE_EFFECT_CURSE, 0, {TYPE_NONE, TYPE_NONE}},
    [InlineEffect::SALT_CURE] = {MOVE_EFFECT_SALT_CURE, 0, {TYPE_NONE, TYPE_NONE}},
    [InlineEffect::ATTRACT] = {MOVE_EFFECT_ATTRACT, 0, {TYPE_NONE, TYPE_NONE}},
    [InlineEffect::DRENCH] = {MOVE_EFFECT_DRENCH, 0, {TYPE_NONE, TYPE_NONE}},
};

// The same checks as SetMoveEffect, so the skill isn't announced for nothing
static int CanApplyInlineStatus(InlineEffect::StatusKind status, u8 user, u8 target) {
    switch (status) {
        case InlineEffect::SLEEP:
            return CanSleep(target);
        case InlineEffect::POISON:
        case InlineEffect::TOXIC:
            return CanBePoisoned(user, target, MOVE_NONE);
        case InlineEffect::BURN:
            return CanBeBurned(user, target);
        case InlineEffect::PARALYSIS:
            return CanBeParalyzed(user, target);
        case InlineEffect::FROSTBITE:
            return CanGetFrostbite(target);
        case InlineEffect::BLEED:
            return CanBleed(target);
        case InlineEffect::CONFUSION:
            return CanBeConfused(target);
        case InlineEffect::CURSE:
            return !(gBattleMons[target].status2 & STATUS2_CURSED);
        case InlineEffect::SALT_CURE:
            return !(gStatuses4[target] & STATUS4_SALT_CURE);
        case InlineEffect::ATTRACT:
            return CanInfatuate(user, target);
        case InlineEffect::DRENCH:
            return CanBeDrenched(target);
        default:
            return FALSE;
    }
}

// A Pokemon on the field goes through SetMoveEffect: immunities, message and animation
static int StatusBattler(InlineRun& run, const InlineEffect& effect, u8 target) {
    CHECK(IsBattlerAlive(target))
    u8 user = target == run.event.battler ? run.event.other : run.event.battler;
    if (!IsBattlerAlive(user) || GetBattlerSide(user) == GetBattlerSide(target)) user = FacingFoe(target);
    CHECK(CanApplyInlineStatus(effect.status, user, target))
    gStackBattler1 = user;
    gStackBattler2 = target;
    gBattleCommunication[MULTISTRING_CHOOSER] = sInlineStatuses[effect.status].moveEffect;
    Show(run, BattleScript_SkillStatus);
    return TRUE;
}

// A benched Pokemon is changed silently. Only its types' immunity is checked, its ability isn't loaded
static int StatusBenchedMon(struct Pokemon* mon, InlineEffect::StatusKind status) {
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    CHECK(species && !GetMonData(mon, MON_DATA_IS_EGG) && GetMonData(mon, MON_DATA_HP))
    CHECK_NOT(GetMonData(mon, MON_DATA_STATUS))
    for (int i = 0; i < 2; i++) {
        u8 immune = sInlineStatuses[status].immuneTypes[i];
        CHECK_NOT(gBaseStats[species].type1 == immune || gBaseStats[species].type2 == immune)
    }
    u32 status1 = sInlineStatuses[status].status1;
    SetMonData(mon, MON_DATA_STATUS, &status1);
    return TRUE;
}

static void RunInlineStatus(InlineRun& run, const InlineEffect& effect) {
    if (effect.target.scope != InlineTarget::PARTY) {
        u8 battlers = TargetBattlers(run, effect.target);
        for (int battler = gBattlersCount - 1; battler >= 0; battler--)
            if (battlers & (1 << battler)) StatusBattler(run, effect, battler);
        return;
    }

    u8 sides = TargetSides(run, effect.target.who);
    for (int side = B_SIDE_OPPONENT; side >= B_SIDE_PLAYER; side--) {
        FILTER(sides & (1 << side))
        struct Pokemon* party = side == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
        int count = PartyCount(side);
        for (int i = count - 1; i >= 0; i--) {
            FILTER(InSlots(effect.target.slotsFirst, effect.target.slotsLast, i, count))
            u8 onField = MAX_BATTLERS_COUNT;
            for (u8 battler = side; battler < gBattlersCount; battler += 2)
                if (gBattlerPartyIndexes[battler] == i && !(gAbsentBattlerFlags & (1u << battler))) onField = battler;
            if (onField < MAX_BATTLERS_COUNT)
                StatusBattler(run, effect, onField);
            else if (StatusBenchedMon(&party[i], effect.status))
                Show(run, NULL);
        }
    }
}

static void RunInlineEffect(InlineRun& run, const InlineEffect& effect) {
    switch (effect.kind) {
        case InlineEffect::STAT_CHANGE: {
            // One script per stat and Pokemon, like Intimidate
            u8 battlers = TargetBattlers(run, effect.target);
            for (int battler = gBattlersCount - 1; battler >= 0; battler--) {
                FILTER(battlers & (1 << battler) && IsBattlerAlive(battler))
                for (int i = effect.statCount - 1; i >= 0; i--) {
                    u8 stat = TranslateStatId(effect.stats[i], battler);
                    FILTER(effect.amount > 0 ? CanRaiseStat(battler, stat) : CanLowerStat(battler, stat))
                    SetStatChanger(stat, effect.amount);
                    gStackBattler1 = battler;
                    Show(run, BattleScript_StackBattlerStatUp);
                }
            }
            break;
        }
        case InlineEffect::SIDE_CONDITION:
            RunInlineSideCondition(run, effect);
            break;
        case InlineEffect::FIELD:
            RunInlineFieldStatus(run, effect);
            break;
        case InlineEffect::WEATHER:
            gBattleCommunication[MULTISTRING_CHOOSER] = sInlineWeatherAnims[effect.weather];
            if (SetInlineWeather(effect, EventMon(run, B_SIDE_OPPONENT))) Show(run, BattleScript_SkillWeather);
            break;
        case InlineEffect::TERRAIN:
            gBattleCommunication[MULTISTRING_CHOOSER] = sInlineTerrains[effect.terrain].message;
            if (SetInlineTerrain(effect, EventMon(run, B_SIDE_OPPONENT))) Show(run, BattleScript_SurgeActivatesRet);
            break;
        case InlineEffect::USE_MOVE:
            if (QueueInlineMove(run, effect)) Show(run, NULL);
            break;
        case InlineEffect::STATUS:
            RunInlineStatus(run, effect);
            break;
    }
}

// Permanent side conditions and field statuses are put back at the end of every turn, whatever their trigger
static constexpr bool IsReapplied(const InlineEffect& effect) {
    return (effect.kind == InlineEffect::SIDE_CONDITION || effect.kind == InlineEffect::FIELD) && effect.permanent;
}

static constexpr bool HasReappliedEffect(const InlineTrigger& trigger) {
    for (int i = 0; i < trigger.effectCount; i++)
        if (IsReapplied(trigger.effects[i])) return true;
    return false;
}

static bool8 MatchesMoveFlags(u8 flags, const InlineEvent& event) {
    if (flags & 1 && IsMoveMakingContact(event.move, event.other)) return TRUE;
    for (int flag = MOVE_FLAG_PUNCH; flag <= MOVE_FLAG_KEEN_EDGE; flag++)
        if (flags & (1 << flag) && DoesMoveMatchFlag(event.other, event.move, (Type)event.moveType, (MoveFlag)flag)) return TRUE;
    return FALSE;
}

static bool8 MatchesEvent(const InlineTrigger& trigger, const InlineEvent& event) {
    u8 battler = event.battler;
    if (trigger.kind != event.kind) return FALSE;
    if (trigger.kind != InlineTrigger::BATTLE_START && trigger.side != InlineTrigger::BOTH &&
        (trigger.side == InlineTrigger::PLAYER) != (GetBattlerSide(battler) == B_SIDE_PLAYER))
        return FALSE;
    switch (trigger.kind) {
        case InlineTrigger::ENTRY: {
            // Switches only get processed after the battle start, so this is only false for the leads
            if (trigger.when == InlineTrigger::LEAD && gBattleStruct->canProcessSwitches) return FALSE;
            if (trigger.when == InlineTrigger::LAST && CountUsablePartyMons(battler)) return FALSE;
            u8 side = GetBattlerSide(battler);
            if (!InSlots(trigger.slotsFirst, trigger.slotsLast, gBattlerPartyIndexes[battler], PartyCount(side))) return FALSE;
            break;
        }
        case InlineTrigger::END_TURN: {
            // The battle turn counter goes up after the end of turn, turnsOnField too
            u8 turns = trigger.onField ? gVolatileStructs[battler].turnsOnField + 1 : gBattleResults.battleTurnCounter + 1;
            if (trigger.every > 1 && turns % trigger.every) return FALSE;
            break;
        }
        case InlineTrigger::HIT: {
            if (trigger.types && (event.moveType >= 32 || !(trigger.types & (1u << event.moveType)))) return FALSE;
            if (trigger.splits && !(trigger.splits & (1 << GetBattleMoveSplit(event.move)))) return FALSE;
            if (trigger.flags && !MatchesMoveFlags(trigger.flags, event)) return FALSE;
            // The strike count goes up after each strike's move end
            if (trigger.firstStrikeOnly && gTurnStructs[event.other].multiHitsUsed) return FALSE;
            u8 effectiveness = gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE  ? InlineTrigger::IMMUNE
                               : gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE    ? InlineTrigger::SUPER_EFFECTIVE
                               : gMoveResultFlags & MOVE_RESULT_NOT_VERY_EFFECTIVE ? InlineTrigger::RESISTED
                                                                                   : InlineTrigger::NEUTRAL;
            if (trigger.effectiveness ? !(trigger.effectiveness & (1 << effectiveness)) : effectiveness == InlineTrigger::IMMUNE) return FALSE;
            break;
        }
        case InlineTrigger::STAT_CHANGE:
            if (trigger.stats && !(trigger.stats & (1 << event.stat))) return FALSE;
            if (trigger.direction && trigger.direction != event.change) return FALSE;
            break;
        default:
            break;
    }
    if (trigger.hpBelow && gBattleMons[battler].hp * 100 >= gBattleMons[battler].maxHP * trigger.hpBelow) return FALSE;
    return TRUE;
}

// A bit per skill slot and once-only trigger, in the storage of the `once` kind
static u32* OnceStorage(const InlineTrigger& trigger, u8 battler) {
    switch (trigger.once) {
        case InlineTrigger::PER_SWITCH_IN:
            return &gVolatileStructs[battler].oncePerSwitchIn;
        case InlineTrigger::PER_POKEMON:
            return &gBattleStruct->oncePerPokemon[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
        case InlineTrigger::PER_BATTLE:
            return &gBattleStruct->oncePerBattle;
        default:
            return NULL;
    }
}

static u32 OnceBit(const InlineTrigger& trigger, BattleSkillEnum skill) {
    int slot = 0;
    while (slot < ARRAY_COUNT(gActiveSkills) - 1 && gActiveSkills[slot] != skill) slot++;
    return 1u << (slot * INLINE_MAX_ONCE + trigger.onceIndex);
}

// once and chance, after the event matched. A once-only trigger is used up when it fires
static bool8 PassesConditions(const InlineTrigger& trigger, BattleSkillEnum skill, u8 battler) {
    u32* once = OnceStorage(trigger, battler);
    u32 bit = OnceBit(trigger, skill);
    if (once && *once & bit) return FALSE;
    if (trigger.chance && Random() % 100 >= trigger.chance) return FALSE;
    if (once) *once |= bit;
    return TRUE;
}

// Take the `on` blocks of the hook, and execute them
template <InlineTrigger... Triggers>
static int RunInlineTriggers(InlineEvent event, BattleSkillEnum skill, InlineRun::ScriptMode scriptMode) {
    constexpr InlineTrigger triggers[] = {Triggers...};
    if (event.other >= gBattlersCount || GetBattlerSide(event.other) == GetBattlerSide(event.battler)) event.other = FacingFoe(event.battler);
    InlineRun run = {.skill = skill, .event = event, .scriptMode = scriptMode};
    u16 popup = gBattleScripting.abilityPopupOverwrite;
    gBattleScripting.abilityPopupOverwrite = skill;
    gBattlerAbility = event.battler;

    for (int i = ARRAY_COUNT(triggers) - 1; i >= 0; i--) {
        const InlineTrigger& trigger = triggers[i];
        bool8 on = MatchesEvent(trigger, event) && PassesConditions(trigger, skill, event.battler);
        bool8 reapply = event.kind == InlineTrigger::END_TURN && GetBattlerSide(event.battler) == B_SIDE_OPPONENT;
        // Putting things back runs on the trainer's end of turn, so a player trigger's Pokemon is the foe here
        run.event = event;
        if (reapply && trigger.side == InlineTrigger::PLAYER) std::swap(run.event.battler, run.event.other);
        for (int j = trigger.effectCount - 1; j >= 0; j--) {
            const InlineEffect& effect = trigger.effects[j];
            if (on || (reapply && IsReapplied(effect))) RunInlineEffect(run, effect);
        }
    }
    if (!run.shown) {
        gBattleScripting.abilityPopupOverwrite = popup;
        return run.queued;
    }
    BattleScriptCall(BattleScript_AnnounceBattleSkill);
    return TRUE;
}

template <InlineTrigger... Triggers>
static constexpr bool HasTrigger(InlineTrigger::Kind kind) {
    return ((Triggers.kind == kind) || ...);
}

// Hooks without a trigger stay null so ON_SKILL skips them if we comment them out from proto
template <InlineAbilities Abilities, InlineTrigger... Triggers>
constexpr BattleSkill InlineSkill = [] {
    BattleSkill impl = {};
    constexpr bool battleStart = HasTrigger<Triggers...>(InlineTrigger::BATTLE_START);
    if constexpr (Abilities.count > 0 || battleStart)
        impl.onBattleStart = +[](ON_BATTLE_START) -> int {
            u8* buffers[] = {gBattleTextBuff1, gBattleTextBuff2, gBattleTextBuff3};
            for (int i = 0; i < Abilities.count; i++) PREPARE_ABILITY_BUFFER(buffers[i], Abilities.ability[i]);
            if constexpr (battleStart) {
                u8 lead = SideBattler(B_SIDE_OPPONENT);
                InlineEvent event = {.kind = InlineTrigger::BATTLE_START, .battler = lead, .other = FacingFoe(lead)};
                if (RunInlineTriggers<Triggers...>(event, skill, InlineRun::PUSH_CALLBACK)) return TRUE;
            }
            if constexpr (Abilities.count > 0) return RunEntryAnnounceScript(skill, BattleScript_AnnounceBattleSkillEnd3);
            return FALSE;
        };
    if constexpr (HasTrigger<Triggers...>(InlineTrigger::ENTRY))
        impl.onEntry = +[](ON_ENTRY) -> int {
            InlineEvent event = {.kind = InlineTrigger::ENTRY, .battler = battler, .other = FacingFoe(battler)};
            return RunInlineTriggers<Triggers...>(event, skill, InlineRun::PUSH_CALLBACK);
        };
    if constexpr (HasTrigger<Triggers...>(InlineTrigger::END_TURN) || (HasReappliedEffect(Triggers) || ...))
        impl.onEndTurn = +[](ON_END_TURN) -> int {
            InlineEvent event = {.kind = InlineTrigger::END_TURN, .battler = battler, .other = FacingFoe(battler)};
            return RunInlineTriggers<Triggers...>(event, skill, InlineRun::PUSH_CALLBACK);
        };
    constexpr InlineTrigger::Kind eventKinds[] = {InlineTrigger::HIT, InlineTrigger::FAINT, InlineTrigger::STAT_CHANGE, InlineTrigger::SWITCH_OUT};
    for (int i = 0; i < ARRAY_COUNT(eventKinds); i++)
        if (HasTrigger<Triggers...>(eventKinds[i])) impl.events |= SKILL_EVENT_BIT(i);
    if (impl.events)
        impl.onEvent = +[](BattleSkillEnum skill, const SkillEvent* skillEvent) -> int {
            constexpr InlineTrigger::Kind kinds[] = {InlineTrigger::HIT, InlineTrigger::FAINT, InlineTrigger::STAT_CHANGE, InlineTrigger::SWITCH_OUT};
            // Not for the stat changes of skills
            if (skillEvent->kind == SKILL_EVENT_STAT_CHANGE && InSkillChain()) return FALSE;
            InlineEvent event = {
                .kind = kinds[skillEvent->kind],
                .battler = skillEvent->battler,
                .other = skillEvent->other,
                .moveType = skillEvent->moveType,
                .move = (MoveEnum)skillEvent->move,
                .stat = skillEvent->stat,
                .change = skillEvent->change,
            };
            return RunInlineTriggers<Triggers...>(event, skill, skillEvent->kind == SKILL_EVENT_SWITCH_OUT ? InlineRun::NO_SCRIPT : InlineRun::CALL_SCRIPT);
        };
    return impl;
}();

#include "generated/data/text/battle_skill_text.hh"

template <BattleSkillEnum Id>
constexpr BattleSkill mergeSkill() {
    BattleSkill skill = Impl<Id>;
    skill.name = BattleSkillStrings<Id>.name;
    skill.announceString = BattleSkillStrings<Id>.announceString;
    return skill;
}

#define FOR_EACH_BATTLE_SKILL_FUNCTION(skill) mergeSkill<skill>(),
const BattleSkill gBattleSkills[] = {FOR_EACH_BATTLE_SKILL};
