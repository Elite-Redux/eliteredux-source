#ifndef GUARD_BATTLE_AI_ATTACK_H
#define GUARD_BATTLE_AI_ATTACK_H

#include "battle_ai_new.h"

int ScoreMoveDamage(
    int battlerAtk, int battlerDef, int move, AiProcessingPhase phase, struct MoveState* moveState, struct MoveContainer* moveContainer, struct AiData* aiData);

#endif