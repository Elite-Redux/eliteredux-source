#ifndef GUARD_ITEM_CONSTANTS_H
#define GUARD_ITEM_CONSTANTS_H

#include "generated/constants/pockets.h"

#define ITEMS_POCKET       (POCKET_ITEMS - 1)
#define MEDICINE_POCKET    (POCKET_MEDICINE - 1)
#define BATTLE_POCKET      (POCKET_BATTLE - 1)
#define TMHM_POCKET        (POCKET_TM_HM - 1)
#define BERRIES_POCKET     (POCKET_BERRIES - 1)
#define BALLS_POCKET       (POCKET_POKE_BALLS - 1)
#define KEYITEMS_POCKET    (POCKET_KEY_ITEMS - 1)
#define MEGA_STONES_POCKET (POCKET_MEGA_STONES - 1)
#define POCKETS_COUNT      (POCKET_MEGA_STONES)

// The Items pocket is the largest pocket, so the maximum amount of items
// in a pocket is its count + 1 for the cancel option
#define MAX_POCKET_ITEMS   (BAG_BATTLE_COUNT + 1)


#endif // GUARD_ITEM_CONSTANTS_H
