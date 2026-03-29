package er.move

import er.FileGenerator.IND
import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIORS
import er.proto.Battler.BATTLER_ATTACKER
import er.proto.Battler.BATTLER_TARGET
import er.proto.ScriptCondition.ConditionCase
import er.proto.MoveBehaviorConfig.Attack.DamageModifier.ModifierCase
import java.io.OutputStreamWriter

object MoveDamageGenerator : Generator {
  override fun generate(writer: OutputStreamWriter) {
    writer.appendLine(
      """
        |u32 UpdateBaseDamage(u32 baseDamage, u8 $BATTLER_ATTACKER, u8 $BATTLER_TARGET, MoveEnum move, MoveBehaviorEnum effect) {
        |${IND}switch (effect) {"""
        .trimMargin()
    )

    for (behavior in MOVE_BEHAVIORS.filterValues { it.attack.hasDamage() }) {
      writer.appendLine("$IND${IND}case ${behavior.key}:")
      val damage = behavior.value.attack.damage
      for (condition in damage.conditionList) {
        writer.appendLine(
          "$IND$IND${IND}REQUIRE(${when (condition.conditionCase) {
                    ConditionCase.WEATHER -> "WeatherCondition(${condition.weather.weather}, ${condition.weather.battler}, true)"
                    ConditionCase.DAMAGED -> "Damaged(${condition.damaged.battler}, ${condition.damaged.by})"
                    ConditionCase.STATUS -> "StatusCondition(${condition.status.status}, ${condition.status.battler})"
                    ConditionCase.SWITCHING -> "Switching(${condition.switching.battler})"
                    ConditionCase.ACTS_AFTER -> "ActsAfter(${condition.actsAfter.before}, ${condition.actsAfter.after}, ${condition.actsAfter.failIfSwitching})"
                    ConditionCase.TERRAIN -> "TerrainCondition(${condition.terrain.terrain}, ${condition.terrain.battler})"
                    ConditionCase.FIELD_EFFECT -> "FieldEffectCondition(${condition.fieldEffect.effect})"
                    ConditionCase.ABILITY -> "AbilityCondition(${condition.ability.ability}, ${condition.ability.battler}, ${condition.ability.checkMoldBreaker})"
                    ConditionCase.HP -> "Hp(${condition.hp.hp}, ${condition.hp.battler})"
                    ConditionCase.RECENT_FAINTED -> "RecentFainted(${condition.recentFainted.battler})"
                    ConditionCase.CUSTOM -> "CustomMoveCondition<${behavior.key}>(CUSTOM_MOVE_CONDITION_CALL)"
                    ConditionCase.SPECIES -> condition.species.speciesList.joinToString(" || ") { "SpeciesCondition($it, ${condition.species.battler}, ${condition.species.exact})" }
                    ConditionCase.ITEM -> {
                        val items = condition.item.itemList.map { it.name } + condition.item.holdEffectList.map { it.name }
                        items.joinToString(" || ") { "ItemCondition($it, ${condition.item.battler}, ${!condition.item.skipDisabling})" }
                    }
                    ConditionCase.CONDITION_NOT_SET -> error("Condition without case: $behavior")
                }
                })"
        )
      }
      writer.appendLine(
        "$IND$IND${IND}return ${when (damage.modifierCase) {
            ModifierCase.MULTIPLY -> "baseDamage * ${damage.multiply}"
            ModifierCase.ADD -> "baseDamage + ${damage.add}"
            ModifierCase.SET -> "baseDamage + ${damage.set}"
            ModifierCase.CUSTOM -> "CustomMoveDamage<${behavior.key}>(CUSTOM_MOVE_DAMAGE_CALL)"
            ModifierCase.CUSTOM_FROM -> TODO()
            ModifierCase.MODIFIER_NOT_SET -> error("Damage modifier without modification: $behavior")
        }
        };"
      )
      writer.appendLine()
    }

    writer.appendLine(
      """
        |$IND${IND}default: return baseDamage;
        |$IND}
        |${IND}return baseDamage;
        |}
        |"""
        .trimMargin()
    )
  }
}
