#pragma once
#include "../ProfileChanger/ProtoWriter.h"
#include "../ProfileChanger/Messages.h"
#include "../SDK/SteamAPI.h"
#include "../ProfileChanger/Protobuffs.h"
#include "../Config.h"
#include "../Memory.h"
#include "../Interfaces.h"
#include "../SDK/Engine.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/Entity.h"
#include "../SDK/GameUI.h"
#define START_MUSICKIT_INDEX 1500000
#define START_ITEM_INDEX     2000000
#define START_MEDAL_INDEX     3000000


struct wskin
{
	int wId;
	int paintKit;
	int quality;
	float wear;
	int seed;
	std::string name;
	bool in_use_t;
	bool in_use_ct;
	int slot;
};

inline std::unordered_map<int, wskin> g_InventorySkins;

class Inventory
{
public:
	void FixNullInventory(ProtoWriter& cache);
	void ClearEquipState(ProtoWriter& object);
	void AddAllItems(ProtoWriter& object);
	void AddItem(ProtoWriter& object, uint64_t account_id, int index, int itemIndex, int equipped, int rarity, int quality, int paintKit, int seed, float wear, std::string name);
	void AddMedal(ProtoWriter& object, uint64_t account_id, int index, uint32_t MedalIndex);
	bool Presend(uint32_t& unMsgType, void* pubData, uint32_t& cubData);
	//static int GetAvailableClassID(int definition_index);
	std::string Changer(void* pubDest, uint32_t* pcubMsgSize);
	std::string ProfileChanger(void* pubDest, uint32_t* pcubMsgSize);
private:
	bool is_knife(const int i);
	bool is_glove(const int i);
	bool is_uncommon(int index);
};
static int GetAvailableClassID(int definition_index);
static int GetSlotID(int definition_index);

inline Inventory* g_Inventory;
