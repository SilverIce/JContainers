#pragma once

class VMClassRegistry;
class TESForm;
class BaseExtraList;
class TESObjectREFR;
class BGSRefAlias;
class EnchantmentItem;
class Actor;
class EffectSetting;

#include "PapyrusArgs.h"
#include "GameTypes.h"

namespace referenceUtils
{
	float GetItemHealthPercent(TESForm * baseForm, BaseExtraList* extraData);
	void SetItemHealthPercent(TESForm * baseForm, BaseExtraList* extraData, float value);

	float GetItemMaxCharge(TESForm * baseForm, BaseExtraList* extraData);
	void SetItemMaxCharge(TESForm * baseForm, BaseExtraList* extraData, float maxCharge);

	float GetItemCharge(TESForm * baseForm, BaseExtraList* extraData);
	void SetItemCharge(TESForm * baseForm, BaseExtraList* extraData, float value);

	BSFixedString GetDisplayName(TESForm * baseForm, BaseExtraList* extraData);
	bool SetDisplayName(BaseExtraList* extraData, BSFixedString value, bool force);

	EnchantmentItem * GetEnchantment(BaseExtraList * extraData);
	void CreateEnchantment(TESForm* baseForm, BaseExtraList * extraData, float maxCharge, VMArray<EffectSetting*> effects, VMArray<float> magnitudes, VMArray<UInt32> areas, VMArray<UInt32> durations);
	void SetEnchantment(TESForm* baseForm, BaseExtraList * extraData, EnchantmentItem * enchantment, float maxCharge);

	UInt32 GetNumReferenceAliases(BaseExtraList * extraData);
	BGSRefAlias * GetNthReferenceAlias(BaseExtraList * extraData, UInt32 n);
};

#define WORNOBJECT_PARAMS StaticFunctionTag*, Actor * actor, UInt32 weaponSlot, UInt32 slotMask
#define WORNOBJECT_TEMPLATE Actor*, UInt32, UInt32

namespace papyrusWornObject
{
	
	void RegisterFuncs(VMClassRegistry* registry);
}