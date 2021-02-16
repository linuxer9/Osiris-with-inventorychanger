#include "InventoryChanger.h"
#include "../Config.h"
#include "../Memory.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/Localize.h"
#include "../Interfaces.h"
//#include "../SDK/WeaponId.h"

static std::vector<inventoryItemShort> Collectibles;

static void initializeCollectibles() noexcept
{
    static bool initalized = false;
    if (initalized)
        return;
    initalized = true;

    const auto itemSchema = memory->itemSystem()->getItemSchema();

    for (auto& x : itemSchema->itemsSorted)
    {
        if (strcmp(x.value->getItemTypeName(), "#CSGO_Type_Collectible"))
            continue;
        char name[64];
        sprintf(name, "%ws", interfaces->localize->findSafe(x.value->getItemBaseName()));
        inventoryItemShort item;
        item.id = (int)x.value->getWeaponId();
        strcpy(item.name, name);
        Collectibles.push_back(item);
    }
}

const std::vector<inventoryItemShort>& InventoryChanger::getCollectibles() noexcept
{
    initializeCollectibles();
    return Collectibles;
}