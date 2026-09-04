package er.defines

import er.proto.BattleSkillTemplateEnum

object BattleSkillTemplateEnumGenerator :
    EnumGenerator(
        name = "BattleSkillTemplateEnum",
        keyMap =
            BattleSkillTemplateEnum.entries
                .filter { it != BattleSkillTemplateEnum.UNRECOGNIZED }
                .associate { it.name to it.number },
    )
