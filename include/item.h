#ifndef GUARD_ITEM_H
#define GUARD_ITEM_H

#include "constants/item.h"
#include "constants/item_config.h"
#include "generated/constants/hold_effects.h"
#include "generated/constants/items.h"

typedef void (*ItemUseFunc)(u8);

struct Item {
    const u8 *name;
    ItemEnum itemId;
    u32 price;
    u8 bpPrice;
    HoldEffectEnum holdEffect;
    u8 holdEffectParam;
    const u8 *description;
    u8 importance;
    u8 unk19;
    u8 pocket;
    u8 type;
    ItemUseFunc fieldUseFunc;
    u8 battleUsage;
    ItemUseFunc battleUseFunc;
    u8 secondaryId;
};

struct BagPocket {
    struct ItemSlot *itemSlots;
    u8 capacity;
};

extern const u16 *const gItemsForPocket[POCKETS_COUNT];
extern const u16 gItemCountsForPocket[POCKETS_COUNT];

void CopyItemName(ItemEnum itemId, u8 *dst);
void GetBerryCountString(u8 *dst, const u8 *berryName, u32 quantity);
bool8 IsBagPocketNonEmpty(u8 pocket);
bool8 CheckBagHasItem(ItemEnum itemId, u16 count);
bool8 HasAtLeastOneBerry(void);
bool8 CheckBagHasSpace(ItemEnum itemId, u16 count);
int UsingBattlePyramidBag();
bool8 AddBagItem(ItemEnum itemId, u16 count);
bool8 RemoveBagItem(ItemEnum itemId, u16 count);
u8 GetPocketByItemId(ItemEnum itemId);
void SwapRegisteredBike(void);
ItemEnum BagGetItemIdByPocketPosition(u8 pocketId, u16 pocketPos);
void CompactItemsInBagPocket(struct BagPocket *bagPocket);
void SortBerriesOrTMHMs(struct BagPocket *bagPocket);
void MoveItemSlotInList(struct ItemSlot *itemSlots_, u32 from, u32 to_);
void ClearBag(void);
bool8 AddPyramidBagItem(ItemEnum itemId, u16 count);
bool8 RemovePyramidBagItem(ItemEnum itemId, u16 count);
const u8 *ItemId_GetName(ItemEnum itemId);
ItemEnum ItemId_GetId(ItemEnum itemId);
u16 ItemId_GetPrice(ItemEnum itemId);
u16 ItemId_GetBPPrice(ItemEnum itemId);
HoldEffectEnum ItemId_GetHoldEffect(ItemEnum itemId);
u8 ItemId_GetHoldEffectParam(ItemEnum itemId);
const u8 *ItemId_GetDescription(ItemEnum itemId);
u8 ItemId_GetImportance(ItemEnum itemId);
u8 ItemId_GetUnknownValue(ItemEnum itemId);
Pocket ItemId_GetPocket(ItemEnum itemId);
u8 ItemId_GetType(ItemEnum itemId);
ItemUseFunc ItemId_GetFieldFunc(ItemEnum itemId);
u8 ItemId_GetBattleUsage(ItemEnum itemId);
ItemUseFunc ItemId_GetBattleFunc(ItemEnum itemId);
u8 ItemId_GetSecondaryId(ItemEnum itemId);
bool32 IsPinchBerryItemEffect(HoldEffectEnum holdEffect);
int HasItem(ItemEnum item);
void SetItem(ItemEnum item);
void ClearItem(ItemEnum item);

#endif  // GUARD_ITEM_H
