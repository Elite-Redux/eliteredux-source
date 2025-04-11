#ifndef GUARD_CONSTANTS_ITEMS_H
#define GUARD_CONSTANTS_ITEMS_H

#include "generated/constants/items.h"
#include "generated/data/megas/legendary_sage.h"
#include "generated/data/megas/adoption_center.h"

#define ITEM_FIELD_ARROW ITEMS_COUNT

#define FIRST_BERRY_INDEX             ITEM_CHERI_BERRY
#define LAST_BERRY_INDEX              ITEM_ENIGMA_BERRY
#define FIRST_BALL_INDEX              ITEM_MASTER_BALL
#define LAST_BALL_INDEX               ITEM_PREMIER_BALL

// Range of berries given out by various NPCS
#define FIRST_BERRY_MASTER_BERRY      ITEM_POMEG_BERRY
#define LAST_BERRY_MASTER_BERRY       ITEM_NOMEL_BERRY
#define FIRST_BERRY_MASTER_WIFE_BERRY ITEM_CHERI_BERRY
#define LAST_BERRY_MASTER_WIFE_BERRY  ITEM_SITRUS_BERRY
#define FIRST_KIRI_BERRY              ITEM_POMEG_BERRY
#define LAST_KIRI_BERRY               ITEM_NOMEL_BERRY
#define FIRST_ROUTE_114_MAN_BERRY     ITEM_RAZZ_BERRY
#define LAST_ROUTE_114_MAN_BERRY      ITEM_PINAP_BERRY

#define NUM_BERRY_MASTER_BERRIES          (LAST_BERRY_MASTER_BERRY - FIRST_BERRY_MASTER_BERRY + 1)
#define NUM_BERRY_MASTER_BERRIES_SKIPPED  (FIRST_BERRY_MASTER_BERRY - FIRST_BERRY_INDEX)
#define NUM_BERRY_MASTER_WIFE_BERRIES     (LAST_BERRY_MASTER_WIFE_BERRY - FIRST_BERRY_MASTER_WIFE_BERRY + 1)
#define NUM_KIRI_BERRIES                  (LAST_KIRI_BERRY - FIRST_KIRI_BERRY + 1)
#define NUM_KIRI_BERRIES_SKIPPED          (FIRST_KIRI_BERRY - FIRST_BERRY_INDEX)
#define NUM_ROUTE_114_MAN_BERRIES         (LAST_ROUTE_114_MAN_BERRY - FIRST_ROUTE_114_MAN_BERRY + 1)
#define NUM_ROUTE_114_MAN_BERRIES_SKIPPED (FIRST_ROUTE_114_MAN_BERRY - FIRST_BERRY_INDEX)

#define ITEM_TO_BERRY(itemId)(((itemId) - FIRST_BERRY_INDEX) + 1)
#define ITEM_TO_MAIL(itemId)((itemId) - FIRST_MAIL_INDEX)
#define MAIL_NONE 0xFF

#define NUM_TECHNICAL_MACHINES 100
#define NUM_HIDDEN_MACHINES     8

#define MAX_BAG_ITEM_CAPACITY  999
#define MAX_PC_ITEM_CAPACITY   999
#define MAX_BERRY_CAPACITY     999
#define MAX_TMHM_CAPACITY      1
#define MAX_PYRAMID_BAG_CAPACITY 99

#define BAG_ITEM_CAPACITY_DIGITS 3
#define BERRY_CAPACITY_DIGITS 3
#define MAX_ITEM_DIGITS BERRY_CAPACITY_DIGITS

// Secondary IDs for rods
#define OLD_ROD   0
#define GOOD_ROD  1
#define SUPER_ROD 2

// Secondary IDs for bikes
#define MACH_BIKE 0
#define ACRO_BIKE 1

// Item type IDs (used to determine the exit callback)
#define ITEM_USE_MAIL        0
#define ITEM_USE_PARTY_MENU  1
#define ITEM_USE_FIELD       2
#define ITEM_USE_PBLOCK_CASE 3
#define ITEM_USE_BAG_MENU    4 // No exit callback, stays in bag menu

// Item battle usage IDs (only checked to see if nonzero)
#define ITEM_B_USE_MEDICINE 1
#define ITEM_B_USE_OTHER    2

// Check if the item is one that can be used on a Pokemon.
#define ITEM_HAS_EFFECT(item) ((item) >= ITEM_POTION && (item) <= ITEM_RED_FLUTE)

#endif  // GUARD_CONSTANTS_ITEMS_H