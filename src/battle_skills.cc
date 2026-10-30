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

struct InlineEffect {
    enum Kind : u8 { STAT_CHANGE, HAZARD } kind;
    // STAT_CHANGE
    u8 statCount;
    u8 stats[NUM_BATTLE_STATS - 1];  // STAT_*, STAT_HIGHEST_* are picked when it runs
    s8 amount;
    // HAZARD, on the player's side
    u8 hazards;  // bit per SKILL_HAZARD_*
    u8 layers;   // spikes and toxic spikes are brought up to this many, capped at 3 and 2
    bool8 permanent;  // HAZARD: put back at the end of each turn
};

// extra_ability of the skill. The abilities themselves come from Trainer.hellAbilities (GetExtraAbilityToSetToBattler),
// this is for the announce.
struct InlineAbilities {
    u8 count;
    AbilityEnum ability[HELL_MODE_EXTRA_ABILITIES];
};

#define INLINE_MAX_EFFECTS 4

struct InlineTrigger {
    enum Kind : u8 { ENTRY, END_TURN } kind;
    enum When : u8 { EVERY, LEAD } when;  // specific to ENTRY
    u8 effectCount;
    InlineEffect effects[INLINE_MAX_EFFECTS];
};

// One hook firing: the effects change the battle in C and queue their scripts with Show.
// The skill is announced once, before all of them.
struct InlineRun {
    BattleSkillEnum skill;
    u8 battler;
    bool8 shown;
};

// Scripts run last called first, so effects are run in reverse
static void Show(InlineRun& run, const u8* script) {
    if (!run.shown) BattleScriptPushCursorAndCallback(BattleScript_WaitMessageEnd3);
    run.shown = TRUE;
    BattleScriptCall(script);
}

// Returns whether the hazard wasn't there yet, or had fewer layers
static int AddInlineHazard(u8 hazard, u8 layers) {
    struct SideTimer* timer = &gSideTimers[B_SIDE_PLAYER];
    u32* status = &gSideStatuses[B_SIDE_PLAYER];
    switch (hazard) {
        case SKILL_HAZARD_SPIKES:
            CHECK(timer->spikesAmount < min(layers, 3))
            timer->spikesAmount = min(layers, 3);
            *status |= SIDE_STATUS_SPIKES;
            return TRUE;
        case SKILL_HAZARD_TOXIC_SPIKES:
            CHECK(timer->toxicSpikesAmount < min(layers, 2))
            timer->toxicSpikesAmount = min(layers, 2);
            *status |= SIDE_STATUS_TOXIC_SPIKES;
            return TRUE;
        case SKILL_HAZARD_STEALTH_ROCK:
            CHECK_NOT(*status & SIDE_STATUS_STEALTH_ROCK)
            *status |= SIDE_STATUS_STEALTH_ROCK;
            timer->stealthRockType = TYPE_ROCK;
            return TRUE;
        case SKILL_HAZARD_STICKY_WEB:
            CHECK_NOT(*status & SIDE_STATUS_STICKY_WEB)
            *status |= SIDE_STATUS_STICKY_WEB;
            return TRUE;
        case SKILL_HAZARD_CALTROPS:
            CHECK_NOT(timer->caltrops)
            timer->caltrops = TRUE;
            return TRUE;
    }
    return FALSE;
}

static void RunInlineEffect(InlineRun& run, const InlineEffect& effect) {
    switch (effect.kind) {
        case InlineEffect::STAT_CHANGE:
            for (int i = effect.statCount - 1; i >= 0; i--) {
                u8 stat = TranslateStatId(effect.stats[i], run.battler);
                FILTER(effect.amount > 0 ? CanRaiseStat(run.battler, stat) : CanLowerStat(run.battler, stat))
                SetStatChanger(stat, effect.amount);
                Show(run, BattleScript_StackBattlerStatUp);
            }
            break;
        case InlineEffect::HAZARD:
            for (int i = SKILL_HAZARD_COUNT - 1; i >= 0; i--) {
                FILTER(effect.hazards & (1 << i))
                FILTER(AddInlineHazard(i, effect.layers))
                // The script picks the animation, the chooser is kept for it until it runs
                gBattleCommunication[MULTISTRING_CHOOSER] = i;
                Show(run, BattleScript_SkillHazard);
            }
            break;
    }
}

static constexpr bool HasPermanentHazard(const InlineTrigger& trigger) {
    for (int i = 0; i < trigger.effectCount; i++)
        if (trigger.effects[i].kind == InlineEffect::HAZARD && trigger.effects[i].permanent) return true;
    return false;
}

// Take the `on` blocks of the hook, and execute them
template <InlineTrigger... Triggers>
static int RunInlineTriggers(InlineTrigger::Kind hook, BattleSkillEnum skill, u8 battler) {
    // For now, triggers are only checked from the trainer side
    CHECK(GetBattlerSide(battler) == B_SIDE_OPPONENT)

    constexpr InlineTrigger triggers[] = {Triggers...};
    InlineRun run = {.skill = skill, .battler = battler};
    gBattleScripting.abilityPopupOverwrite = skill;
    gStackBattler1 = gBattlerAbility = battler;

    for (int i = ARRAY_COUNT(triggers) - 1; i >= 0; i--) {
        const InlineTrigger& trigger = triggers[i];
        // Switches only get processed after the battle start, so this is only false for the leads
        bool8 on = trigger.kind == hook && (trigger.when != InlineTrigger::LEAD || !gBattleStruct->canProcessSwitches);
        for (int j = trigger.effectCount - 1; j >= 0; j--) {
            const InlineEffect& effect = trigger.effects[j];
            // Permanent hazards are put back at the end of every turn, whatever their trigger
            bool8 reapply = hook == InlineTrigger::END_TURN && effect.kind == InlineEffect::HAZARD && effect.permanent;
            if (on || reapply) RunInlineEffect(run, effect);
        }
    }
    CHECK(run.shown)
    BattleScriptCall(BattleScript_AnnounceBattleSkill);
    return TRUE;
}

// Hooks without a trigger stay null so ON_SKILL skips them if we comment them out from proto
template <InlineAbilities Abilities, InlineTrigger... Triggers>
constexpr BattleSkill InlineSkill = [] {
    BattleSkill impl = {};
    if constexpr (Abilities.count > 0)
        impl.onBattleStart = +[](ON_BATTLE_START) -> int {
            u8* buffers[] = {gBattleTextBuff1, gBattleTextBuff2, gBattleTextBuff3};
            for (int i = 0; i < Abilities.count; i++) PREPARE_ABILITY_BUFFER(buffers[i], Abilities.ability[i]);
            return RunEntryAnnounceScript(skill, BattleScript_AnnounceBattleSkillEnd3);
        };
    if constexpr (((Triggers.kind == InlineTrigger::ENTRY) || ...))
        impl.onEntry = +[](ON_ENTRY) -> int { return RunInlineTriggers<Triggers...>(InlineTrigger::ENTRY, skill, battler); };
    if constexpr (((Triggers.kind == InlineTrigger::END_TURN || HasPermanentHazard(Triggers)) || ...))
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
