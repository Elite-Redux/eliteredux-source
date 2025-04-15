#include "global.h"
#include "item.h"
#include "berry.h"
#include "string_util.h"
#include "text.h"
#include "event_data.h"
#include "malloc.h"
#include "secret_base.h"
#include "item_menu.h"
#include "tx_registered_items_menu.h"
#include "strings.h"
#include "load_save.h"
#include "item_use.h"
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "constants/items.h"
#include "constants/hold_effects.h"
#include "mgba_printf/mgba.h"

extern u16 gUnknown_0203CF30[];

// this file's functions
static bool8 CheckPyramidBagHasItem(u16 itemId, u16 count);
static bool8 CheckPyramidBagHasSpace(u16 itemId, u16 count);

// rodata
#include "data/text/item_descriptions.h"
#include "generated/data/item/items.h"

#include "generated/data/item/pockets.h"

// code
static u16 GetBagItemQuantity(u16 *quantity) { return gSaveBlock2Ptr->encryptionKey ^ *quantity; }

static void SetBagItemQuantity(u16 *quantity, u16 newValue) { *quantity = newValue ^ gSaveBlock2Ptr->encryptionKey; }

void CopyItemName(u16 itemId, u8 *dst) { StringCopy(dst, ItemId_GetName(itemId)); }

void GetBerryCountString(u8 *dst, const u8 *berryName, u32 quantity) {
    const u8 *berryString;
    u8 *txtPtr;

    if (quantity < 2)
        berryString = gText_Berry;
    else
        berryString = gText_Berries;

    txtPtr = StringCopy(dst, berryName);
    *txtPtr = CHAR_SPACE;
    StringCopy(txtPtr + 1, berryString);
}

#define ITEM_BUCKET(item) gSaveBlock1Ptr->itemFlags[(item) / 16]
#define ITEM_BIT(item) (1 << ((item) % 16))
int HasItem(u16 item) { return (ITEM_BUCKET(item) & ITEM_BIT(item)) != 0; }

void SetItem(u16 item) { ITEM_BUCKET(item) |= ITEM_BIT(item); }

void ClearItem(u16 item) { ITEM_BUCKET(item) &= ~ITEM_BIT(item); }

bool8 IsBagPocketNonEmpty(u8 pocket) {
    int itemCount = gItemCountsForPocket[pocket];
    const u16 *items = gItemsForPocket[pocket];

    for (int i = 0; i < itemCount; i++) {
        FILTER(HasItem(items[i]))
        return TRUE;
    }

    return FALSE;
}

bool8 CheckBagHasItem(u16 itemId, u16 count) {
    if (ItemId_GetPocket(itemId) == 0) return FALSE;
    if (UsingBattlePyramidBag()) return CheckPyramidBagHasItem(itemId, count);
    return HasItem(itemId);
}

bool8 HasAtLeastOneBerry(void) {
    int itemCount = gItemCountsForPocket[POCKET_BERRIES - 1];
    const u16 *items = gItemsForPocket[POCKET_BERRIES - 1];

    for (int i = 0; i < itemCount; i++) {
        if (CheckBagHasItem(items[i], 1) == TRUE) {
            gSpecialVar_Result = TRUE;
            return TRUE;
        }
    }
    gSpecialVar_Result = FALSE;
    return FALSE;
}

bool8 CheckBagHasSpace(u16 itemId, u16 count) {
    if (ItemId_GetPocket(itemId) == POCKET_NONE) return FALSE;

    if (UsingBattlePyramidBag()) {
        return CheckPyramidBagHasSpace(itemId, count);
    }

    return TRUE;
}

int UsingBattlePyramidBag() { return InBattlePyramid() || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG) == TRUE; }

bool8 AddBagItem(u16 itemId, u16 count) {
    if (ItemId_GetPocket(itemId) == POCKET_NONE) return FALSE;

    // check Battle Pyramid Bag
    if (UsingBattlePyramidBag()) {
        return AddPyramidBagItem(itemId, count);
    } else {
        SetItem(itemId);
        return TRUE;
    }
}

bool8 RemoveBagItem(u16 itemId, u16 count) {
    if (ItemId_GetPocket(itemId) == POCKET_NONE || itemId == ITEM_NONE) return FALSE;

    // check Battle Pyramid Bag
    if (UsingBattlePyramidBag()) {
        return RemovePyramidBagItem(itemId, count);
    } else {
        return HasItem(itemId);
    }
}

u8 GetPocketByItemId(u16 itemId) { return ItemId_GetPocket(itemId); }

void SwapRegisteredBike(void) {
    u8 pos_ACRO = TxRegItemsMenu_GetRegisteredItemIndex(ITEM_ACRO_BIKE);
    u8 pos_MACH = TxRegItemsMenu_GetRegisteredItemIndex(ITEM_MACH_BIKE);
    switch (gSaveBlock1Ptr->registeredItemSelect) {
        case ITEM_MACH_BIKE:
            gSaveBlock1Ptr->registeredItemSelect = ITEM_ACRO_BIKE;
            break;
        case ITEM_ACRO_BIKE:
            gSaveBlock1Ptr->registeredItemSelect = ITEM_MACH_BIKE;
            break;
    }
    if (pos_ACRO != 0xFF)
        gSaveBlock1Ptr->registeredItems[pos_ACRO].itemId = ITEM_MACH_BIKE;
    else if (pos_MACH != 0xFF)
        gSaveBlock1Ptr->registeredItems[pos_MACH].itemId = ITEM_ACRO_BIKE;
}

ItemEnum BagGetItemIdByPocketPosition(u8 pocketId, u16 pocketPos) {
    int count = gItemCountsForPocket[pocketId];
    // const u16* items = gItemsForPocket[pocketId];
    if (pocketPos < count)
        return gItemsForPocket[pocketId][pocketPos];
    else
        return ITEM_NONE;
    // for (int i = 0; i < count; i++) {
    //     MGBA_PRINT_VALUES(pocketId, pocketPos, items[i], HasItem(items[i]), pocketPos)
    //     if (HasItem(items[i]) && !(pocketPos--)) return items[i];
    // }
    // return 0;
}

static void SwapItemSlots(struct ItemSlot *a, struct ItemSlot *b) {
    struct ItemSlot temp;
    SWAP(*a, *b, temp);
}

void CompactItemsInBagPocket(struct BagPocket *bagPocket) {
    u16 i, j;

    for (i = 0; i < bagPocket->capacity - 1; i++) {
        for (j = i + 1; j < bagPocket->capacity; j++) {
            if (GetBagItemQuantity(&bagPocket->itemSlots[i].quantity) == 0) SwapItemSlots(&bagPocket->itemSlots[i], &bagPocket->itemSlots[j]);
        }
    }
}

void SortBerriesOrTMHMs(struct BagPocket *bagPocket) {
    u16 i, j;

    for (i = 0; i < bagPocket->capacity - 1; i++) {
        for (j = i + 1; j < bagPocket->capacity; j++) {
            if (GetBagItemQuantity(&bagPocket->itemSlots[i].quantity) != 0) {
                if (GetBagItemQuantity(&bagPocket->itemSlots[j].quantity) == 0) continue;
                if (bagPocket->itemSlots[i].itemId <= bagPocket->itemSlots[j].itemId) continue;
            }
            SwapItemSlots(&bagPocket->itemSlots[i], &bagPocket->itemSlots[j]);
        }
    }
}

void MoveItemSlotInList(struct ItemSlot *itemSlots_, u32 from, u32 to_) {
    // dumb assignments needed to match
    struct ItemSlot *itemSlots = itemSlots_;
    u32 to = to_;

    if (from != to) {
        s16 i, count;
        struct ItemSlot firstSlot = itemSlots[from];

        if (to > from) {
            to--;
            for (i = from, count = to; i < count; i++) itemSlots[i] = itemSlots[i + 1];
        } else {
            for (i = from, count = to; i > count; i--) itemSlots[i] = itemSlots[i - 1];
        }
        itemSlots[to] = firstSlot;
    }
}

void ClearBag(void) { ZERO(gSaveBlock1Ptr->itemFlags) }

static bool8 CheckPyramidBagHasItem(u16 itemId, u16 count) {
    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    for (int i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++) {
        if (items[i] == itemId) {
            if (quantities[i] >= count) return TRUE;

            count -= quantities[i];
            if (count == 0) return TRUE;
        }
    }

    return FALSE;
}

static bool8 CheckPyramidBagHasSpace(u16 itemId, u16 count) {
    u8 i;
    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++) {
        if (items[i] == itemId || items[i] == ITEM_NONE) {
            if (quantities[i] + count <= MAX_BAG_ITEM_CAPACITY) return TRUE;

            count = (quantities[i] + count) - MAX_BAG_ITEM_CAPACITY;
            if (count == 0) return TRUE;
        }
    }

    return FALSE;
}

bool8 AddPyramidBagItem(u16 itemId, u16 count) {
    u16 i;

    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    u16 *newItems = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(u16));
    u8 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(u8));

    memcpy(newItems, items, PYRAMID_BAG_ITEMS_COUNT * sizeof(u16));
    memcpy(newQuantities, quantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(u8));

    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++) {
        if (newItems[i] == itemId && newQuantities[i] < MAX_PYRAMID_BAG_CAPACITY) {
            newQuantities[i] += count;
            if (newQuantities[i] > MAX_PYRAMID_BAG_CAPACITY) {
                count = newQuantities[i] - MAX_PYRAMID_BAG_CAPACITY;
                newQuantities[i] = MAX_PYRAMID_BAG_CAPACITY;
            } else {
                count = 0;
            }

            if (count == 0) break;
        }
    }

    if (count > 0) {
        for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++) {
            if (newItems[i] == ITEM_NONE) {
                newItems[i] = itemId;
                newQuantities[i] = count;
                if (newQuantities[i] > MAX_PYRAMID_BAG_CAPACITY) {
                    count = newQuantities[i] - MAX_PYRAMID_BAG_CAPACITY;
                    newQuantities[i] = MAX_PYRAMID_BAG_CAPACITY;
                } else {
                    count = 0;
                }

                if (count == 0) break;
            }
        }
    }

    if (count == 0) {
        memcpy(items, newItems, PYRAMID_BAG_ITEMS_COUNT * sizeof(u16));
        memcpy(quantities, newQuantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(u8));
        Free(newItems);
        Free(newQuantities);
        return TRUE;
    } else {
        Free(newItems);
        Free(newQuantities);
        return FALSE;
    }
}

bool8 RemovePyramidBagItem(u16 itemId, u16 count) {
    u16 i;

    u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
    u8 *quantities = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];

    i = gPyramidBagMenuState.cursorPosition + gPyramidBagMenuState.scrollPosition;
    if (items[i] == itemId && quantities[i] >= count) {
        quantities[i] -= count;
        if (quantities[i] == 0) items[i] = ITEM_NONE;
        return TRUE;
    } else {
        u16 *newItems = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(u16));
        u8 *newQuantities = Alloc(PYRAMID_BAG_ITEMS_COUNT * sizeof(u8));

        memcpy(newItems, items, PYRAMID_BAG_ITEMS_COUNT * sizeof(u16));
        memcpy(newQuantities, quantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(u8));

        for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++) {
            if (newItems[i] == itemId) {
                if (newQuantities[i] >= count) {
                    newQuantities[i] -= count;
                    count = 0;
                    if (newQuantities[i] == 0) newItems[i] = ITEM_NONE;
                } else {
                    count -= newQuantities[i];
                    newQuantities[i] = 0;
                    newItems[i] = ITEM_NONE;
                }

                if (count == 0) break;
            }
        }

        if (count == 0) {
            memcpy(items, newItems, PYRAMID_BAG_ITEMS_COUNT * sizeof(u16));
            memcpy(quantities, newQuantities, PYRAMID_BAG_ITEMS_COUNT * sizeof(u8));
            Free(newItems);
            Free(newQuantities);
            return TRUE;
        } else {
            Free(newItems);
            Free(newQuantities);
            return FALSE;
        }
    }
}

static u16 SanitizeItemId(u16 itemId) {
    if (itemId >= ITEMS_COUNT)
        return ITEM_NONE;
    else
        return itemId;
}

const u8 *ItemId_GetName(u16 itemId) {
    const u8 *name = gItems[SanitizeItemId(itemId)].name;
    return name ? name : gItems[0].name;
}

u16 ItemId_GetId(u16 itemId) { return gItems[SanitizeItemId(itemId)].itemId; }

u16 ItemId_GetPrice(u16 itemId) { return gItems[SanitizeItemId(itemId)].price; }

u16 ItemId_GetBPPrice(u16 itemId) { return gItems[SanitizeItemId(itemId)].bpPrice; }

u8 ItemId_GetHoldEffect(u16 itemId) { return gItems[SanitizeItemId(itemId)].holdEffect; }

u8 ItemId_GetHoldEffectParam(u16 itemId) { return gItems[SanitizeItemId(itemId)].holdEffectParam; }

const u8 *ItemId_GetDescription(u16 itemId) { return gItems[SanitizeItemId(itemId)].description; }

u8 ItemId_GetImportance(u16 itemId) { return gItems[SanitizeItemId(itemId)].importance; }

// unused
u8 ItemId_GetUnknownValue(u16 itemId) { return gItems[SanitizeItemId(itemId)].unk19; }

u8 ItemId_GetPocket(u16 itemId) { return gItems[SanitizeItemId(itemId)].pocket; }

u8 ItemId_GetType(u16 itemId) { return gItems[SanitizeItemId(itemId)].type; }

ItemUseFunc ItemId_GetFieldFunc(u16 itemId) { return gItems[SanitizeItemId(itemId)].fieldUseFunc; }

u8 ItemId_GetBattleUsage(u16 itemId) { return gItems[SanitizeItemId(itemId)].battleUsage; }

ItemUseFunc ItemId_GetBattleFunc(u16 itemId) { return gItems[SanitizeItemId(itemId)].battleUseFunc; }

u8 ItemId_GetSecondaryId(u16 itemId) { return gItems[SanitizeItemId(itemId)].secondaryId; }

bool32 IsPinchBerryItemEffect(u16 holdEffect) {
    switch (holdEffect) {
        case HOLD_EFFECT_ATTACK_UP:
        case HOLD_EFFECT_DEFENSE_UP:
        case HOLD_EFFECT_SPEED_UP:
        case HOLD_EFFECT_SP_ATTACK_UP:
        case HOLD_EFFECT_SP_DEFENSE_UP:
        case HOLD_EFFECT_CRITICAL_UP:
        case HOLD_EFFECT_RANDOM_STAT_UP:
#ifdef HOLD_EFFECT_CUSTAP_BERRY
        case HOLD_EFFECT_CUSTAP_BERRY:
#endif
#ifdef HOLD_EFFECT_MICLE_BERRY
        case HOLD_EFFECT_MICLE_BERRY:
#endif
            return TRUE;
    }

    return FALSE;
}
