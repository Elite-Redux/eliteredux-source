package er.move

import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIORS
import er.GeneratorUtils.MOVE_BEHAVIOR_CONFIGS
import er.proto.MoveBehavior
import er.proto.MoveBehaviorConfig
import er.proto.MoveBehaviorConfig.Attack.AttackSecondaryEffect.AttackEffectCase
import java.io.OutputStreamWriter

private const val BATTLE_SCRIPT_HIT = "BattleScript_EffectHit"
private const val BATTLE_SCRIPT_ARGUMENT_HIT = "BattleScript_ArgumentHit"
private const val BATTLE_SCRIPT_HIT_RETURN_FOR_EFFECT = "BattleScript_EffectHitUntilArgumentReturn"

object MoveScriptGenerator : Generator {
  override fun generate(writer: OutputStreamWriter) {
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
    for ((name, config) in MOVE_BEHAVIORS) {
      val scriptName: String =
        when {
          config.hasLegacyConfig() -> config.legacyConfig
          config.hasAttack() -> AttackScriptGenerator.getMoveConfigName(name, config)
          else -> TODO()
        }
      writer.appendLine("\t.4byte $scriptName @ $name")
    }
    writer.appendLine()

    for ((name, config) in MOVE_BEHAVIORS) {
      when {
        config.hasLegacyConfig() -> continue
        config.hasAttack() -> AttackScriptGenerator.generateForBehavior(name, config, writer)
      }
    }
  }
}

private fun OutputStreamWriter.writeScriptLines(vararg lines: String) =
  appendLine(lines.joinToString("\n\t", prefix = "\t"))

private object AttackScriptGenerator {
  private fun hasNoBehavior(config: MoveBehaviorConfig) = config.attack.effectList.isEmpty()

  fun getMoveConfigName(name: String, config: MoveBehaviorConfig): String = when {
    hasNoBehavior(config) -> BATTLE_SCRIPT_HIT
    config.attack.effectList.map { it.argumentEffect } == listOf(true) -> BATTLE_SCRIPT_ARGUMENT_HIT
    else ->  "__BATTLE_SCRIPT_GENERATED_$name"
  }

  fun MoveBehaviorConfig.Attack.AttackMoveEffect.fullString() =
    listOfNotNull(
        effect.name,
        "MOVE_EFFECT_AFFECTS_USER".takeIf { affectsUser },
        "MOVE_EFFECT_CERTAIN".takeIf { certain },
      )
      .joinToString(" | ")

  fun generateForBehavior(name: String, config: MoveBehaviorConfig, writer: OutputStreamWriter) {
    val attackScript = getMoveConfigName(name, config)

    if (attackScript == BATTLE_SCRIPT_HIT || attackScript == BATTLE_SCRIPT_ARGUMENT_HIT) return

    val attack = config.attack

    writer.appendLine("$attackScript:")

    if (attack.effectList.size == 1 && attack.effectList.single().hasMoveEffect()) {
      val effect = attack.effectList.single().moveEffect
      writer.writeScriptLines("setmoveeffect ${effect.fullString()}", "goto $BATTLE_SCRIPT_HIT")
      writer.appendLine()
      return
    }

    writer.writeScriptLines("call $BATTLE_SCRIPT_HIT_RETURN_FOR_EFFECT")

    for (effect in attack.effectList) {
      when (effect.attackEffectCase) {
        AttackEffectCase.MOVE_EFFECT -> {
          if (effect.chance != 0) writer.writeScriptLines("setmoveeffectchance ${effect.chance}")
          writer.writeScriptLines(
            "setmoveeffect ${effect.moveEffect.fullString()}",
            "seteffectwithchance",
          )
        }
        AttackEffectCase.ARGUMENT_EFFECT -> {
          if (effect.chance != 0) writer.writeScriptLines("setmoveeffectchance ${effect.chance}")
          writer.writeScriptLines(
            "argumenttomoveeffect",
            "seteffectwithchance",
          )
        }
        else -> TODO()
      }
    }

    writer.writeScriptLines("goto BattleScript_MoveEndTryFaintTarget")
    writer.appendLine()
  }
}
