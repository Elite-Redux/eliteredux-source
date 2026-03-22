package er.move

import er.Generator
import er.GeneratorUtils.MOVE_BEHAVIOR_CONFIGS
import er.proto.MoveBehavior
import java.io.OutputStreamWriter

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

    writer.appendLine(".macro movescripttable")
    for (idNumber in 0..behaviorConfigMap.keys.max()) {
      val behaviorConfig = behaviorConfigMap[idNumber]

      if (behaviorConfig == null) {
        writer.appendLine("\t.4byte BattleScript_EffectPlaceholder @ ID $idNumber")
      } else {
        writer.appendLine("\t.4byte ${behaviorConfig.legacyConfig} @ ${behaviorConfig.id}")
      }
    }
    writer.appendLine(".endm")
  }
}
