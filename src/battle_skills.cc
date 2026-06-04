#include "battle_skills.hh"

#undef __SIZE_TYPE__
#define __SIZE_TYPE__ uint32_t
#include <cstddef>
#include <array>

extern "C" {
#include "global.h"
#include "generated/constants/battle_skills.h"
}

#include "type_utils.hh"

template <BattleSkillEnum Id>
constexpr BattleSkill Impl = {0};

#include "generated/data/text/battle_skill_text.hh"

template <BattleSkillEnum Id>
constexpr BattleSkill mergeSkill() {
    BattleSkill skill = Impl<Id>;
    skill.name = BattleSkillStrings<Id>.name;
    return skill;
}

#define FOR_EACH_BATTLE_SKILL_FUNCTION(skill) mergeSkill<skill>(),
const BattleSkill gBattleSkills[] = {FOR_EACH_BATTLE_SKILL};
