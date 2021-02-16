#pragma once
#include <array>
#include <string>
#include <vector>

#include "../imgui/imgui.h"
#include "../SDK/WeaponId.h"
struct inventoryItemShort {
    int id;
    char name[64] = "";
};
struct inventoryItem {
    WeaponId id;
    int equipped = 0;
    char baseName[64];
    int quality = 0;
    int rarity = 0;
    int paintKit = 0;
    int seed = 0;
    int stat_trak = -1;
    float wear = (std::numeric_limits<float>::min)();
    char custom_name[32] = "";
    //std::array<sticker_setting, 5> stickers;
};
namespace InventoryChanger
{
    const std::vector<inventoryItemShort>& getCollectibles() noexcept;
    
}
