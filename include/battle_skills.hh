#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "generated/constants/battle_skills.h"

typedef int (*BattleSkillOnBattleStart)(BattleSkillEnum skill);
typedef int (*BattleSkillOnEntryHandler)(BattleSkillEnum skill, u8 battler);
typedef void (*BattleSkillOnDefensiveMultiplier)(u8 battler, u16* resistance, u16* modifier);
typedef int (*BattleSkillOnEndTurn)(BattleSkillEnum skill, u8 battler);

// Events that only inline skills listen to, through onEvent. `events` has a bit per kind so the
// call sites skip the skills that don't care.
typedef enum SkillEventKind {
    SKILL_EVENT_HIT,
    SKILL_EVENT_FAINT,
    SKILL_EVENT_STAT_CHANGE,
    SKILL_EVENT_SWITCH_OUT,
} SkillEventKind;
#define SKILL_EVENT_BIT(kind) (1 << (kind))

typedef struct SkillEvent {
    u8 kind;
    u8 battler;  // the Pokemon it happens to
    u8 other;    // the attacker or the KO'er, MAX_BATTLERS_COUNT when there's none
    u8 moveType;
    u16 move;
    u8 stat;     // STAT_CHANGE
    s8 change;   // STAT_CHANGE: 1 up, -1 down
} SkillEvent;
typedef int (*BattleSkillOnEvent)(BattleSkillEnum skill, const SkillEvent* event);

typedef struct BattleSkill {
    const u8* name;
    const u8* announceString;
    BattleSkillOnBattleStart onBattleStart;
    BattleSkillOnEntryHandler onEntry;
    BattleSkillOnDefensiveMultiplier onDefensiveMultiplier;
    BattleSkillOnEndTurn onEndTurn;
    BattleSkillOnEvent onEvent;
    u8 events;  // SKILL_EVENT_BIT of the kinds onEvent handles
} BattleSkill;

extern const BattleSkill gBattleSkills[BATTLE_SKILL_COUNT];

#ifdef __cplusplus
}
#endif
