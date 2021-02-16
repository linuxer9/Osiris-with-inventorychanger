#include "ProfileChanger.h"
#define _gc2ch MatchmakingGC2ClientHello
#define _pci PlayerCommendationInfo
#define _pri PlayerRankingInfo

bool Inventory::is_knife(const int i)
{
	return (i >= 500 && i < 5027) || i == 59 || i == 42;
}

bool Inventory::is_glove(const int i)
{
	return (i >= 5027 && i <= 5035);
}

bool Inventory::is_uncommon(int index)
{
	switch (index)
	{
	case (int)WeaponId::Deagle:
	case (int)WeaponId::Glock:
	case (int)WeaponId::Ak47:
	case (int)WeaponId::Awp:
	case (int)WeaponId::M4A1:
	case (int)WeaponId::M4a1_s:
	case (int)WeaponId::Hkp2000:
	case (int)WeaponId::Usp_s:
		return true;
	default:
		return false;
	}
}
std::string Inventory::ProfileChanger(void* pubDest, uint32_t* pcubMsgSize)
{
	ProtoWriter msg((void*)((DWORD)pubDest + 8), *pcubMsgSize - 8, 19);
	auto _commendation = msg.has(_gc2ch::commendation) ? msg.get(_gc2ch::commendation).String() : std::string("");
	ProtoWriter commendation(_commendation, 4);
	commendation.replace(Field(_pci::cmd_friendly, TYPE_UINT32, (int64_t)config->profileChanger.friendly));
	commendation.replace(Field(_pci::cmd_teaching, TYPE_UINT32, (int64_t)config->profileChanger.teach));
	commendation.replace(Field(_pci::cmd_leader, TYPE_UINT32, (int64_t)config->profileChanger.leader));
	msg.replace(Field(_gc2ch::commendation, TYPE_STRING, commendation.serialize()));
	auto _ranking = msg.has(_gc2ch::ranking) ? msg.get(_gc2ch::ranking).String() : std::string("");
	ProtoWriter ranking(_ranking, 6);
	ranking.replace(Field(_pri::rank_id, TYPE_UINT32, (int64_t)config->profileChanger.rank));
	ranking.replace(Field(_pri::wins, TYPE_UINT32, (int64_t)config->profileChanger.wins));
	msg.replace(Field(_gc2ch::ranking, TYPE_STRING, ranking.serialize()));
	msg.replace(Field(_gc2ch::player_level, TYPE_INT32, (int64_t)config->profileChanger.level));
	msg.replace(Field(_gc2ch::player_cur_xp, TYPE_INT32, (int64_t)config->profileChanger.exp));

	if (config->profileChanger.ban_type != 0 && config->profileChanger.ban_time != 0)
	{
		msg.replace(Field(_gc2ch::penalty_reason, TYPE_INT32, (int64_t)config->profileChanger.ban_type));
		msg.replace(Field(_gc2ch::penalty_seconds, TYPE_INT32, (int64_t)config->profileChanger.ban_time));
	}

	return msg.serialize();
}

bool Inventory::Presend(uint32_t& unMsgType, void* pubData, uint32_t& cubData)
{
	uint32_t MessageType = unMsgType & 0x7FFFFFFF;

	if (MessageType == k_EMsgGCAdjustItemEquippedState) {

		ProtoWriter msg((void*)((DWORD)pubData + 8), cubData - 8, 19);

		if (!msg.has(CMsgAdjustItemEquippedState::item_id)
			|| !msg.has(CMsgAdjustItemEquippedState::new_class)
			|| !msg.has(CMsgAdjustItemEquippedState::new_slot))
			return true;
		int item_id = (int) msg.get(CMsgAdjustItemEquippedState::item_id).UInt32();
		uint32_t new_class = msg.get(CMsgAdjustItemEquippedState::new_class).UInt32();
		interfaces->gameUI->createCommandMsgBox("inventory", std::to_string(item_id).c_str());
		if (item_id >= 2000000)
		{
			auto& item_inventory = config->inventory.items[START_ITEM_INDEX - item_id];
			interfaces->gameUI->createCommandMsgBox("inventory", "changed!");
			item_inventory.equipped = new_class;
			if (is_knife((int)item_inventory.id))
			{
				auto& item_skin = config->skinChanger[0];
				item_skin.enabled = true;
				item_skin.itemId = WeaponId::Knife;
				item_skin.itemIdIndex = item_skin.definition_override_index = (int)item_inventory.id;
				item_skin.paintKit = item_inventory.paintKit;
				item_skin.seed = item_inventory.paintKit;


			}
		}
		//memory->clientState->ForceFullUpdate(); randomly crashes
		return false; 
	}
	return true;
}

std::string Inventory::Changer(void* pubDest, uint32_t* pcubMsgSize)
{
	ProtoWriter msg((void*)((DWORD)pubDest + 8), *pcubMsgSize - 8, 11);
	if (msg.getAll(CMsgClientWelcome::outofdate_subscribed_caches).empty())
		return msg.serialize();

	ProtoWriter cache(msg.get(CMsgClientWelcome::outofdate_subscribed_caches).String(), 4);
	// If not have items in inventory, Create null inventory
	FixNullInventory(cache);
	// Add custom items
	auto objects = cache.getAll(CMsgSOCacheSubscribed::objects);
	for (size_t i = 0; i < objects.size(); i++)
	{
		ProtoWriter object(objects[i].String(), 2);

		if (!object.has(SubscribedType::type_id))
			continue;

		if (object.get(SubscribedType::type_id).Int32() == 1)
		{
			object.clear(SubscribedType::object_data);
			ClearEquipState(object);
			AddAllItems(object);
			cache.replace(Field(CMsgSOCacheSubscribed::objects, TYPE_STRING, object.serialize()), i);
		}
	}
	msg.replace(Field(CMsgClientWelcome::outofdate_subscribed_caches, TYPE_STRING, cache.serialize()), 0);

	return msg.serialize();
}

void Inventory::AddMedal(ProtoWriter& object, uint64_t account_id, int index, uint32_t MedalIndex)
{
	
	ProtoWriter Medal(19);
	Medal.add(Field(CSOEconItem::account_id, TYPE_UINT32, (int64_t) account_id));
	Medal.add(Field(CSOEconItem::origin, TYPE_UINT32, (int64_t)9));
	Medal.add(Field(CSOEconItem::rarity, TYPE_UINT32, (int64_t)6));
	Medal.add(Field(CSOEconItem::quantity, TYPE_UINT32, (int64_t)1));
	Medal.add(Field(CSOEconItem::quality, TYPE_UINT32, (int64_t)4));
	Medal.add(Field(CSOEconItem::level, TYPE_UINT32, (int64_t)1));

	ProtoWriter time_acquired_attribute(3);
	time_acquired_attribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)222));
	time_acquired_attribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, std::string("\x00\x00\x00\x00")));
	Medal.add(Field(CSOEconItem::attribute, TYPE_STRING, time_acquired_attribute.serialize()));

	Medal.add(Field(CSOEconItem::def_index, TYPE_UINT32, (int64_t)MedalIndex));
	Medal.add(Field(CSOEconItem::inventory, TYPE_UINT32, (int64_t)(START_MEDAL_INDEX + index)));
	Medal.add(Field(CSOEconItem::id, TYPE_UINT64, (int64_t)(START_MEDAL_INDEX + index)));

	ProtoWriter equipped_state(2);
	equipped_state.add(Field(CSOEconItemEquipped::new_class, TYPE_UINT32, (int64_t)0));
	equipped_state.add(Field(CSOEconItemEquipped::new_slot, TYPE_UINT32, (int64_t)55));
	//Medal.add(Field(CSOEconItem::equipped_state, TYPE_STRING, equipped_state.serialize()));

	object.add(Field(SubscribedType::object_data, TYPE_STRING, Medal.serialize()));
}

void Inventory::FixNullInventory(ProtoWriter& cache)
{
	bool inventory_exist = false;
	auto objects = cache.getAll(CMsgSOCacheSubscribed::objects);
	for (size_t i = 0; i < objects.size(); i++)
	{
		ProtoWriter object(objects[i].String(), 2);
		if (!object.has(SubscribedType::type_id))
			continue;
		if (object.get(SubscribedType::type_id).Int32() != 1)
			continue;
		inventory_exist = true;
		break;
	}
	if (!inventory_exist)
	{
		ProtoWriter null_object(2);
		null_object.add(Field(SubscribedType::type_id, TYPE_INT32, (int64_t)1));

		cache.add(Field(CMsgSOCacheSubscribed::objects, TYPE_STRING, null_object.serialize()));
	}
}

void Inventory::ClearEquipState(ProtoWriter& object)
{
	auto object_data = object.getAll(SubscribedType::object_data);
	for (size_t j = 0; j < object_data.size(); j++)
	{
		ProtoWriter item(object_data[j].String(), 19);

		if (item.getAll(CSOEconItem::equipped_state).empty())
			continue;

		// create NOT equiped state for item 
		ProtoWriter null_equipped_state(2);
		null_equipped_state.replace(Field(CSOEconItemEquipped::new_class, TYPE_UINT32, (int64_t)1));
		null_equipped_state.replace(Field(CSOEconItemEquipped::new_slot, TYPE_UINT32, (int64_t)0));
		// unequip all 
		auto equipped_state = item.getAll(CSOEconItem::equipped_state);
		for (size_t k = 0; k < equipped_state.size(); k++)
			item.replace(Field(CSOEconItem::equipped_state, TYPE_STRING, null_equipped_state.serialize()), k);

		object.replace(Field(SubscribedType::object_data, TYPE_STRING, item.serialize()), j);
	}
}
void Inventory::AddAllItems(ProtoWriter& object)
{
	uint64_t account_id = memory->SteamUser->GetSteamID().GetAccountID();
	const auto itemSchema = memory->itemSystem()->getItemSchema();

	int id = 1;
	for (auto& x : config->inventory.items) {
		try
		{
			AddItem(object, account_id, x.first, (int)x.second.id, x.second.equipped, x.second.rarity, x.second.quality, x.second.paintKit, x.second.seed, x.second.wear, x.second.custom_name);
		}
		catch (int a)
		{
			interfaces->gameUI->createCommandMsgBox("inventory error", std::to_string(a).c_str());
		}
		id++;
	}
	for (auto& medal : config->inventory.medals)
	{
		AddMedal(object, account_id, id, (uint32_t)medal.id);
		id++;
	}
}

void Inventory::AddItem(ProtoWriter& object, uint64_t account_id, int index, int itemIndex, int equipped, int rarity, int quality, int paintKit, int seed, float wear, std::string name)
{
	ProtoWriter item(19);
	item.add(Field(CSOEconItem::id, TYPE_UINT64, (int64_t)(START_ITEM_INDEX + index)));
	item.add(Field(CSOEconItem::account_id, TYPE_UINT32, (int64_t)account_id));
	item.add(Field(CSOEconItem::def_index, TYPE_UINT32, (int64_t)itemIndex));
	item.add(Field(CSOEconItem::inventory, TYPE_UINT32, (int64_t)(START_ITEM_INDEX + index)));
	item.add(Field(CSOEconItem::origin, TYPE_UINT32, (int64_t)24));
	item.add(Field(CSOEconItem::quantity, TYPE_UINT32, (int64_t)1));
	item.add(Field(CSOEconItem::level, TYPE_UINT32, (int64_t)1));
	item.add(Field(CSOEconItem::style, TYPE_UINT32, (int64_t)0));
	item.add(Field(CSOEconItem::flags, TYPE_UINT32, (int64_t)0));
	item.add(Field(CSOEconItem::in_use, TYPE_BOOL, (int64_t)true));
	item.add(Field(CSOEconItem::original_id, TYPE_UINT64, (int64_t)0));

	//if (is_uncommon(itemIndex))
	//	rarity++;

	item.add(Field(CSOEconItem::rarity, TYPE_UINT32, (int64_t)rarity));
	item.add(Field(CSOEconItem::quality, TYPE_UINT32, (int64_t)quality));

	if (name.length() > 0)
		item.add(Field(CSOEconItem::custom_name, TYPE_STRING, name));

	// Equip new skins
	//int avalTeam = GetAvailableClassID(itemIndex);

	if (equipped == 1 /*spectator*/ || equipped == 2 /*T*/) {
		ProtoWriter equipped_state(2);
		equipped_state.add(Field(CSOEconItemEquipped::new_class, TYPE_UINT32, (int64_t)2));
		equipped_state.add(Field(CSOEconItemEquipped::new_slot, TYPE_UINT32, (int64_t)GetSlotID(itemIndex)));
		item.add(Field(CSOEconItem::equipped_state, TYPE_STRING, equipped_state.serialize()));
	}
	if (equipped == 1 /*spectator*/ || equipped == 3 /*ct*/) {
		ProtoWriter equipped_state(2);
		equipped_state.add(Field(CSOEconItemEquipped::new_class, TYPE_UINT32, (int64_t)3));
		equipped_state.add(Field(CSOEconItemEquipped::new_slot, TYPE_UINT32, (int64_t)GetSlotID(itemIndex)));
		item.add(Field(CSOEconItem::equipped_state, TYPE_STRING, equipped_state.serialize()));
	}
	

	// Paint Kit
	float _PaintKitAttributeValue = (float)paintKit;
	auto PaintKitAttributeValue = std::string{ reinterpret_cast<const char*>((void*)&_PaintKitAttributeValue), 4 };
	ProtoWriter PaintKitAttribute(3);
	PaintKitAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)6));
	PaintKitAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, PaintKitAttributeValue));
	item.add(Field(CSOEconItem::attribute, TYPE_STRING, PaintKitAttribute.serialize()));

	// Paint Seed
	float _SeedAttributeValue = (float)seed;
	auto SeedAttributeValue = std::string{ reinterpret_cast<const char*>((void*)&_SeedAttributeValue), 4 };
	ProtoWriter SeedAttribute(3);
	SeedAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)7));
	SeedAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, SeedAttributeValue));
	item.add(Field(CSOEconItem::attribute, TYPE_STRING, SeedAttribute.serialize()));

	// Paint Wear
	float _WearAttributeValue = wear;
	auto WearAttributeValue = std::string{ reinterpret_cast<const char*>((void*)&_WearAttributeValue), 4 };
	ProtoWriter WearAttribute(3);
	WearAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)8));
	WearAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, WearAttributeValue));
	item.add(Field(CSOEconItem::attribute, TYPE_STRING, WearAttribute.serialize()));
	if (is_knife(itemIndex) || is_glove(itemIndex))
	{
		object.add(Field(SubscribedType::object_data, TYPE_STRING, item.serialize()));
		return;
	}
	// Stickers
	for (int j = 0; j < 4; j++)
	{
		// Sticker Kit
		ProtoWriter StickerKitAttribute(3);
		StickerKitAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)(113 + 4 * j)));
		StickerKitAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, "\x00\x00\x00\x00"));
		item.add(Field(CSOEconItem::attribute, TYPE_STRING, StickerKitAttribute.serialize()));
		// Sticker Wear
		float _StickerWearAttributeValue = 0.001f;
		auto StickerWearAttributeValue = std::string{ reinterpret_cast<const char*>((void*)&_StickerWearAttributeValue), 4 };
		ProtoWriter StickerWearAttribute(3);
		StickerWearAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)(114 + 4 * j)));
		StickerWearAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, StickerWearAttributeValue));
		item.add(Field(CSOEconItem::attribute, TYPE_STRING, StickerWearAttribute.serialize()));
		// Sticker Scale
		float _StickerScaleAttributeValue = 1.f;
		auto StickerScaleAttributeValue = std::string{ reinterpret_cast<const char*>((void*)&_StickerScaleAttributeValue), 4 };
		ProtoWriter StickerScaleAttribute(3);
		StickerScaleAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)(115 + 4 * j)));
		StickerScaleAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, StickerScaleAttributeValue));
		item.add(Field(CSOEconItem::attribute, TYPE_STRING, StickerScaleAttribute.serialize()));
		// Sticker Rotation
		float _StickerRotationAttributeValue = 0.f;
		auto StickerRotationAttributeValue = std::string{ reinterpret_cast<const char*>((void*)&_StickerRotationAttributeValue), 4 };
		ProtoWriter StickerRotationAttribute(3);
		StickerRotationAttribute.add(Field(CSOEconItemAttribute::def_index, TYPE_UINT32, (int64_t)(116 + 4 * j)));
		StickerRotationAttribute.add(Field(CSOEconItemAttribute::value_bytes, TYPE_STRING, StickerRotationAttributeValue));
		item.add(Field(CSOEconItem::attribute, TYPE_STRING, StickerRotationAttribute.serialize()));
	}
	object.add(Field(SubscribedType::object_data, TYPE_STRING, item.serialize()));
}
static int GetAvailableClassID(int definition_index)
{
	switch (definition_index)
	{
	case (int)WeaponId::Flip:
	case (int)WeaponId::Bayonet:
	case (int)WeaponId::Gut:
	case (int)WeaponId::Karambit:
	case (int)WeaponId::M9Bayonet:
	case (int)WeaponId::Huntsman:
	case (int)WeaponId::Falchion:
	case (int)WeaponId::Bowie:
	case (int)WeaponId::Butterfly:
	case (int)WeaponId::Daggers:
	case (int)WeaponId::SkeletonKnife:
	case (int)WeaponId::SurvivalKnife:
	case (int)WeaponId::NomadKnife:
	case (int)WeaponId::ClassicKnife:
	case (int)WeaponId::Elite:
	case (int)WeaponId::P250:
	case (int)WeaponId::Cz75a:
	case (int)WeaponId::Deagle:
	case (int)WeaponId::Revolver:
	case (int)WeaponId::Mp7:
	case (int)WeaponId::Ump45:
	case (int)WeaponId::P90:
	case (int)WeaponId::Bizon:
	case (int)WeaponId::Ssg08:
	case (int)WeaponId::Awp:
	case (int)WeaponId::Nova:
	case (int)WeaponId::Xm1014:
	case (int)WeaponId::M249:
	case (int)WeaponId::Negev:
	case (int)WeaponId::GloveStuddedBrokenfang:
	case (int)WeaponId::GloveStuddedBloodhound:
	case (int)WeaponId::GloveSporty:
	case (int)WeaponId::GloveSlick:
	case (int)WeaponId::GloveHydra:
	case (int)WeaponId::GloveLeatherWrap:
	case (int)WeaponId::GloveMotorcycle:
	case (int)WeaponId::GloveSpecialist:
		return 1;// TEAM_SPECTATOR;
	case (int)WeaponId::Glock:
	case (int)WeaponId::Tec9:
	case (int)WeaponId::Mac10:
	case (int)WeaponId::GalilAr:
	case (int)WeaponId::Ak47:
	case (int)WeaponId::Sg553:
	case (int)WeaponId::G3SG1:
	case (int)WeaponId::Sawedoff:
		return 2;//TEAM_TERRORIST;
	case (int)WeaponId::Usp_s:
	case (int)WeaponId::Hkp2000:
	case (int)WeaponId::Fiveseven:
	case (int)WeaponId::Mp9:
	case (int)WeaponId::Famas:
	case (int)WeaponId::M4a1_s:
	case (int)WeaponId::M4A1:
	case (int)WeaponId::Aug:
	case (int)WeaponId::Scar20:
	case (int)WeaponId::Mag7:
		return 3;// TEAM_COUNTER_TERRORIST;

	default:
		return 0;// TEAM_UNASSIGNED;
	}
}
static int GetSlotID(int definition_index)
{
	switch (definition_index)
	{
	case (int)WeaponId::Flip:
	case (int)WeaponId::Bayonet:
	case (int)WeaponId::Gut:
	case (int)WeaponId::Karambit:
	case (int)WeaponId::M9Bayonet:
	case (int)WeaponId::Huntsman:
	case (int)WeaponId::Falchion:
	case (int)WeaponId::Bowie:
	case (int)WeaponId::Butterfly:
	case (int)WeaponId::Daggers:
	case (int)WeaponId::SkeletonKnife:
	case (int)WeaponId::SurvivalKnife:
	case (int)WeaponId::NomadKnife:
	case (int)WeaponId::ClassicKnife:
		return 0;
	case (int)WeaponId::Usp_s:
	case (int)WeaponId::Hkp2000:
	case (int)WeaponId::Glock:
		return 2;
	case (int)WeaponId::Elite:
		return 3;
	case (int)WeaponId::P250:
		return 4;
	case (int)WeaponId::Tec9:
	case (int)WeaponId::Cz75a:
	case (int)WeaponId::Fiveseven:
		return 5;
	case (int)WeaponId::Deagle:
	case (int)WeaponId::Revolver:
		return 6;
	case (int)WeaponId::Mp9:
	case (int)WeaponId::Mac10:
		return 8;
	case (int)WeaponId::Mp7:
		return 9;
	case (int)WeaponId::Ump45:
		return 10;
	case (int)WeaponId::P90:
		return 11;
	case (int)WeaponId::Bizon:
		return 12;
	case (int)WeaponId::Famas:
	case (int)WeaponId::GalilAr:
		return 14;
	case (int)WeaponId::M4a1_s:
	case (int)WeaponId::M4A1:
	case (int)WeaponId::Ak47:
		return 15;
	case (int)WeaponId::Ssg08:
		return 16;
	case (int)WeaponId::Sg553:
	case (int)WeaponId::Aug:
		return 17;
	case (int)WeaponId::Awp:
		return 18;
	case (int)WeaponId::G3SG1:
	case (int)WeaponId::Scar20:
		return 19;
	case (int)WeaponId::Nova:
		return 20;
	case (int)WeaponId::Xm1014:
		return 21;
	case (int)WeaponId::Sawedoff:
	case (int)WeaponId::Mag7:
		return 22;
	case (int)WeaponId::M249:
		return 23;
	case (int)WeaponId::Negev:
		return 24;
	case (int)WeaponId::GloveStuddedBrokenfang:
	case (int)WeaponId::GloveStuddedBloodhound:
	case (int)WeaponId::GloveSporty:
	case (int)WeaponId::GloveSlick:
	case (int)WeaponId::GloveHydra:
	case (int)WeaponId::GloveLeatherWrap:
	case (int)WeaponId::GloveMotorcycle:
	case (int)WeaponId::GloveSpecialist:
		return 41;
	default:
		return -1;
	}
}