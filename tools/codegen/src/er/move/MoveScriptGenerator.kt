package er.move

import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIOR_CONFIGS
import er.proto.MoveBehavior
import er.proto.MoveBehaviorConfig
import java.io.OutputStreamWriter

private const val BATTLE_SCRIPT_HIT = "BattleScript_EffectHit"

object MoveScriptGenerator : Generator {
  override fun generate(writer: OutputStreamWriter) {
    val behaviorConfigMap = MOVE_BEHAVIOR_CONFIGS.associateBy { it.id.number }
    val duplicateConfigMap = MOVE_BEHAVIOR_CONFIGS.groupBy { it.id }.filter { it.value.size > 1 }
    check(duplicateConfigMap.isEmpty()) {
      "Found duplicate configs for move behaviors: ${duplicateConfigMap.keys}"
    }
    val missingConfigList =
      (MoveBehavior.entries -
        MOVE_BEHAVIOR_CONFIGS.map { it.id }.toSet() -
        MoveBehavior.UNRECOGNIZED)
    check(missingConfigList.isEmpty()) { "Missing config for behaviors: $missingConfigList" }

    writer.appendLine(".align 4\ngBattleScriptsForMoveEffects::")
    for (idNumber in 0..behaviorConfigMap.keys.max()) {
      val behaviorConfig = behaviorConfigMap[idNumber]

      if (behaviorConfig == null) {
        writer.appendLine("\t.4byte BattleScript_EffectPlaceholder @ ID $idNumber")
      } else {
        val scriptName: String =
          when {
            behaviorConfig.hasLegacyConfig() -> behaviorConfig.legacyConfig
            behaviorConfig.hasAttack() -> AttackScriptGenerator.getMoveConfigName(behaviorConfig)
            else -> TODO()
          }
        writer.appendLine("\t.4byte $scriptName @ ${behaviorConfig.id}")
      }
    }
    writer.appendLine()

    for (config in behaviorConfigMap.values) {
      when {
        config.hasLegacyConfig() -> continue
        config.hasAttack() -> AttackScriptGenerator.generateForBehavior(config, writer)
      }
    }
  }
}

private fun OutputStreamWriter.writeScriptLines(vararg lines: String) =
  appendLine(lines.joinToString("\n\t", prefix = "\t"))

private object AttackScriptGenerator {
  private fun hasNoBehavior(config: MoveBehaviorConfig) = config.attack.effectList.isEmpty()

  fun getMoveConfigName(config: MoveBehaviorConfig): String =
    if (hasNoBehavior(config)) BATTLE_SCRIPT_HIT else "__BATTLE_SCRIPT_GENERATED_${config.id}__"

  fun MoveBehaviorConfig.Attack.AttackMoveEffect.fullString() =
    listOfNotNull(
        effect.name,
        "MOVE_EFFECT_AFFECTS_USER".takeIf { affectsUser },
        "MOVE_EFFECT_CERTAIN".takeIf { certain },
      )
      .joinToString(" | ")

  fun generateForBehavior(config: MoveBehaviorConfig, writer: OutputStreamWriter) {
    val attackScript = getMoveConfigName(config)

    if (attackScript == BATTLE_SCRIPT_HIT) return

    val attack = config.attack

    writer.appendLine("$attackScript:")

    if (attack.effectList.size == 1 && attack.effectList.single().hasMoveEffect()) {
      val effect = attack.effectList.single().moveEffect
      writer.writeScriptLines("setmoveeffect ${effect.fullString()}", "goto $BATTLE_SCRIPT_HIT")
      writer.appendLine()
      return
    }

    TODO()
  }
}
