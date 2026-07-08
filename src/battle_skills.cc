#include "battle_skills.hh"

#undef __SIZE_TYPE__
#define __SIZE_TYPE__ uint32_t
#include <cstddef>
#include <array>

extern "C" {
#include "global.h"
#include "generated/constants/battle_skills.h"
#include "battle_scripts.h"
#include "battle_script_commands.h"
#include "battle_util.h"
}

#include "type_utils.hh"

#define ON_ENTRY opt BattleSkillEnum skill, opt u8 battler
#define ON_ATTACKER opt BattleSkillEnum skill, opt u8 battler, opt u8 target, opt MoveEnum move, opt Type moveType

template <BattleSkillEnum Id>
constexpr BattleSkill Impl = {0};

#include "generated/data/text/battle_skill_text.hh"

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

template <int stat, s8 stage>
constexpr BattleSkill StatOnSwitchIn = {
    .onEntry = +[](ON_ENTRY) {
        CHECK(CanRaiseStat(battler, stat))

        SetStatChanger(stat, stage);
        BattleScriptPushCursorAndCallback(BattleScript_BattlerAbilityStatRaiseOnSwitchIn);
    },
};

template <MoveEnum entryMove, u8 entryMovePower>
constexpr BattleSkill MoveOnEntry = {
    .onEntry = +[](ON_ENTRY) {
        // Not sure if ABILITY_NONE will work properly or not, should probably change but maybe OK for now
        return UseEntryMove(battler, ABILITY_NONE, entryMove, entryMovePower);
    },
};

template <BattleSkillEnum Id>
constexpr BattleSkill mergeSkill() {
    BattleSkill skill = Impl<Id>;
    skill.name = BattleSkillStrings<Id>.name;
    return skill;
}

#define FOR_EACH_BATTLE_SKILL_FUNCTION(skill) mergeSkill<skill>(),
const BattleSkill gBattleSkills[] = {FOR_EACH_BATTLE_SKILL};
