#include "PapyrusIngredient.h"
#include "PapyrusSpell.h"
#include "GameObjects.h"

namespace papyrusIngredient
{

	UInt32 GetNumEffects(IngredientItem* thisMagic)
	{ return magicItemUtils::GetNumEffects(thisMagic); }

	float GetNthEffectMagnitude(IngredientItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectMagnitude(thisMagic, index); }

	UInt32 GetNthEffectArea(IngredientItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectArea(thisMagic, index); }

	UInt32 GetNthEffectDuration(IngredientItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectDuration(thisMagic, index); }

	EffectSetting* GetNthEffectMagicEffect(IngredientItem* thisMagic, UInt32 index)
	{ return magicItemUtils::GetNthEffectMagicEffect(thisMagic, index); }

	UInt32 GetCostliestEffectIndex(IngredientItem* thisMagic)
	{ return magicItemUtils::GetCostliestEffectIndex(thisMagic); }

	void SetNthEffectMagnitude(IngredientItem* thisMagic, UInt32 index, float value)
	{ magicItemUtils::SetNthEffectMagnitude(thisMagic, index, value); }

	void SetNthEffectArea(IngredientItem* thisMagic, UInt32 index, UInt32 value)
	{ magicItemUtils::SetNthEffectArea(thisMagic, index, value); }

	void SetNthEffectDuration(IngredientItem* thisMagic, UInt32 index, UInt32 value)
	{ magicItemUtils::SetNthEffectDuration(thisMagic, index, value); }
}

#include "PapyrusVM.h"
#include "PapyrusNativeFunctions.h"

void papyrusIngredient::RegisterFuncs(VMClassRegistry* registry)
{
	registry->RegisterFunction(
		new NativeFunction0<IngredientItem, UInt32>("GetNumEffects", "Ingredient", papyrusIngredient::GetNumEffects, registry));

	registry->RegisterFunction(
		new NativeFunction1<IngredientItem, float, UInt32>("GetNthEffectMagnitude", "Ingredient", papyrusIngredient::GetNthEffectMagnitude, registry));

	registry->RegisterFunction(
		new NativeFunction1<IngredientItem, UInt32, UInt32>("GetNthEffectArea", "Ingredient", papyrusIngredient::GetNthEffectArea, registry));

	registry->RegisterFunction(
		new NativeFunction1<IngredientItem, UInt32, UInt32>("GetNthEffectDuration", "Ingredient", papyrusIngredient::GetNthEffectDuration, registry));

	registry->RegisterFunction(
		new NativeFunction1<IngredientItem, EffectSetting*, UInt32>("GetNthEffectMagicEffect", "Ingredient", papyrusIngredient::GetNthEffectMagicEffect, registry));

	registry->RegisterFunction(
		new NativeFunction0<IngredientItem, UInt32>("GetCostliestEffectIndex", "Ingredient", papyrusIngredient::GetCostliestEffectIndex, registry));

	// Sets
	registry->RegisterFunction(
		new NativeFunction2<IngredientItem, void, UInt32, float>("SetNthEffectMagnitude", "Ingredient", papyrusIngredient::SetNthEffectMagnitude, registry));

	registry->RegisterFunction(
		new NativeFunction2<IngredientItem, void, UInt32, UInt32>("SetNthEffectArea", "Ingredient", papyrusIngredient::SetNthEffectArea, registry));

	registry->RegisterFunction(
		new NativeFunction2<IngredientItem, void, UInt32, UInt32>("SetNthEffectDuration", "Ingredient", papyrusIngredient::SetNthEffectDuration, registry));
}
