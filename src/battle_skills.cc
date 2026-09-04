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
}

#include "type_utils.hh"

#define ON_BATTLE_START opt BattleSkillEnum skill
#define ON_ENTRY opt BattleSkillEnum skill, opt u8 battler
#define ON_ATTACKER opt BattleSkillEnum skill, opt u8 battler, opt u8 target, opt MoveEnum move, opt Type moveType
#define ON_DEFENSIVE_MULTIPLIER opt u8 battler, opt u16 *resistance, opt u16 *modifier

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
        Unregister(skill);
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STICKY_WEB)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STICKY_WEB;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillPermaStickyWeb);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_SPIKES_1_LAYER> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);
        CHECK(gSideTimers[B_SIDE_PLAYER].spikesAmount < 3)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_SPIKES;
        gSideTimers[B_SIDE_PLAYER].spikesAmount++;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillSpikes);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_TOXIC_SPIKES_2_LAYERS> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);
        CHECK(gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount < 2)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_TOXIC_SPIKES;
        gSideTimers[B_SIDE_PLAYER].toxicSpikesAmount = 2;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillToxicSpikes);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_STEALTH_ROCKS> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);
        CHECK_NOT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_STEALTH_ROCK)
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_STEALTH_ROCK;
        gSideTimers[B_SIDE_PLAYER].stealthRockType = TYPE_ROCK;
        return RunEntryAnnounceScript(skill, BattleScript_ExtraSkillStealthRock);
    },
};

template <>
constexpr BattleSkill Impl<SKILL_HAZARDS_ALL> = {
    .onBattleStart = +[](ON_BATTLE_START) -> int {
        Unregister(skill);

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

#include "generated/data/text/battle_skill_text.hh"

template <BattleSkillEnum Id>
constexpr BattleSkill mergeSkill() {
    BattleSkill skill = Impl<Id>;
    skill.name = BattleSkillStrings<Id>.name;
    skill.announceString = BattleSkillStrings<Id>.name;
    return skill;
}

#define FOR_EACH_BATTLE_SKILL_FUNCTION(skill) mergeSkill<skill>(),
const BattleSkill gBattleSkills[] = {FOR_EACH_BATTLE_SKILL};
