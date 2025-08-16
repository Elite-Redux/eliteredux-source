package er.scripting.move

import er.GeneratorUtils.MOVES_BEHAVIOR_CONFIGS_LIST
import er.proto.AttackBehaviorConfig
import er.proto.MoveBehaviorConfig

object MoveScriptingUtils {
    fun <T> groupAttackData(map: (AttackBehaviorConfig) -> List<T>) =
        MOVES_BEHAVIOR_CONFIGS_LIST.groupBy({
            when (it.configCase) {
                MoveBehaviorConfig.ConfigCase.ATTACK -> map(it.attack)
                MoveBehaviorConfig.ConfigCase.STATUS -> emptyList()
                MoveBehaviorConfig.ConfigCase.TWO_TURN -> map(it.twoTurn.turn2Attack)
                MoveBehaviorConfig.ConfigCase.CUSTOM_BEHAVIOR -> emptyList()
                MoveBehaviorConfig.ConfigCase.ALLY_ENEMY_DIFFER -> map(it.allyEnemyDiffer.enemy)
                MoveBehaviorConfig.ConfigCase.CONFIG_NOT_SET -> emptyList()
            }
        }, { it.id }).filterKeys { it.isNotEmpty() }
}