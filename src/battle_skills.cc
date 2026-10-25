#include "battle_skills.hh"

#undef __SIZE_TYPE__
#define __SIZE_TYPE__ uint32_t
#include <cstddef>
#include <array>

extern "C" {
#include "global.h"
#include "generated/constants/battle_skills.h"
#include "generated/constants/battle_skill_templates.h"
#include "battle_scripts.h"
#include "battle_script_commands.h"
#include "battle_util.h"
#include "battle.h"
#include "battle_anim.h"
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
// For now, very simple structure that we can evolve later, no need to get in complex stuff for now

struct InlineEffect {
    enum Kind : u8 { STAT_CHANGE } kind;
    u8 stat;
    s8 amount;
};

struct InlineTrigger {
    enum Kind : u8 { ENTRY, END_TURN } kind;
    enum When : u8 { EVERY, LEAD } when;  // specific to ENTRY
    InlineEffect effect;
};

static int RunInlineEffect(const InlineEffect& effect, BattleSkillEnum skill, u8 battler) {
    switch (effect.kind) {
        case InlineEffect::STAT_CHANGE: {
            u8 stat = TranslateStatId(effect.stat, battler);
            CHECK(effect.amount > 0 ? CanRaiseStat(battler, stat) : CanLowerStat(battler, stat))
            SetStatChanger(stat, effect.amount);
            return RunEntryAnnounceScript(skill, BattleScript_BattleSkillStatChange);
        }
    }
    return FALSE;
}

static int RunInlineTrigger(const InlineTrigger& trigger, InlineTrigger::Kind hook, BattleSkillEnum skill, u8 battler) {
    CHECK(trigger.kind == hook)
    // For now, triggers are only checked from the trainer side
    CHECK(GetBattlerSide(battler) == B_SIDE_OPPONENT)

    // Switches only get processed after the battle start, so this is only false for the leads
    CHECK(trigger.when != InlineTrigger::LEAD || !gBattleStruct->canProcessSwitches)
    return RunInlineEffect(trigger.effect, skill, battler);
}

// Take the `on` blocks, and execute them 
template <InlineTrigger... Triggers>
static int RunInlineTriggers(InlineTrigger::Kind hook, BattleSkillEnum skill, u8 battler) {
    constexpr InlineTrigger triggers[] = {Triggers...};
    int ran = FALSE;
    // Reverse order because pushed scripts run LIFO, and want it FIFO from the proto
    for (int i = ARRAY_COUNT(triggers) - 1; i >= 0; i--) ran |= RunInlineTrigger(triggers[i], hook, skill, battler);
    return ran;
}

// Hooks without a trigger stay null so ON_SKILL skips them if we comment them out from proto
template <InlineTrigger... Triggers>
constexpr BattleSkill InlineSkill = [] {
    BattleSkill impl = {};
    if constexpr (((Triggers.kind == InlineTrigger::ENTRY) || ...))
        impl.onEntry = +[](ON_ENTRY) -> int { return RunInlineTriggers<Triggers...>(InlineTrigger::ENTRY, skill, battler); };
    if constexpr (((Triggers.kind == InlineTrigger::END_TURN) || ...))
        impl.onEndTurn = +[](ON_END_TURN) -> int { return RunInlineTriggers<Triggers...>(InlineTrigger::END_TURN, skill, battler); };
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
