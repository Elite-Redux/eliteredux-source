#pragma once

extern "C" {
#include "generated/constants/abilities.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "generated/constants/battle_move_effects.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_string_ids.h"
#include "constants/hold_effects.h"
#include "constants/item.h"
#include "constants/items.h"
#include "global.h"
#include "item.h"
#include "mgba_printf/mgba.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
}

class Implementation {
   public:
    virtual ~Implementation() = default;
};