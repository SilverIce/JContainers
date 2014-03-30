#include "PapyrusEnchantment.h"
#include "PapyrusSpell.h"
#include "GameObjects.h"

namespace papyrusEnchantment
{

	UInt32 GetNumEffects(EnchantmentItem* thisMagic)
	{ return magicItemUtils::GetNumEffects(thisMagic); }

	float GetNthEffectMagnitude(EnchantmentItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectMagnitude(thisMagic, index); }

	UInt32 GetNthEffectArea(EnchantmentItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectArea(thisMagic, index); }

	UInt32 GetNthEffectDuration(EnchantmentItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectDuration(thisMagic, index); }

	EffectSetting* GetNthEffectMagicEffect(EnchantmentItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectMagicEffect(thisMagic, index); }

	UInt32 GetCostliestEffectIndex(EnchantmentItem* thisMagic)
	{ return magicItemUtils::GetCostliestEffectIndex(thisMagic); }
}

#include "PapyrusVM.h"
#include "PapyrusNativeFunctions.h"

void papyrusEnchantment::RegisterFuncs(VMClassRegistry* registry)
{
	registry->RegisterFunction(
		new NativeFunction0<EnchantmentItem, UInt32>("GetNumEffects", "Enchantment", papyrusEnchantment::GetNumEffects, registry));

	registry->RegisterFunction(
		new NativeFunction1<EnchantmentItem, float, UInt32>("GetNthEffectMagnitude", "Enchantment", papyrusEnchantment::GetNthEffectMagnitude, registry));

	registry->RegisterFunction(
		new NativeFunction1<EnchantmentItem, UInt32, UInt32>("GetNthEffectArea", "Enchantment", papyrusEnchantment::GetNthEffectArea, registry));

	registry->RegisterFunction(
		new NativeFunction1<EnchantmentItem, UInt32, UInt32>("GetNthEffectDuration", "Enchantment", papyrusEnchantment::GetNthEffectDuration, registry));

	registry->RegisterFunction(
		new NativeFunction1<EnchantmentItem, EffectSetting*, UInt32>("GetNthEffectMagicEffect", "Enchantment", papyrusEnchantment::GetNthEffectMagicEffect, registry));

	registry->RegisterFunction(
		new NativeFunction0<EnchantmentItem, UInt32>("GetCostliestEffectIndex", "Enchantment", papyrusEnchantment::GetCostliestEffectIndex, registry));
}
