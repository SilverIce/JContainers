#pragma once

#include "GameTypes.h"

class TESObjectREFR;
class TESForm;
class VMClassRegistry;

namespace papyrusObjectReference
{
	void RegisterFuncs(VMClassRegistry* registry);
	UInt32 GetNumItems(TESObjectREFR* pContainerRef);
	TESForm* GetNthForm(TESObjectREFR* pContainerRef, UInt32 n);
	float GetTotalItemWeight(TESObjectREFR* pContainerRef);
	float GetTotalArmorWeight(TESObjectREFR* pContainerRef);

	void SetItemHealthPercent(TESObjectREFR* object, float value);
	float GetItemCharge(TESObjectREFR* object);
	float GetItemMaxCharge(TESObjectREFR* object);
	void SetItemCharge(TESObjectREFR* object, float value);

	bool IsHarvested(TESObjectREFR* pProduceRef);

	bool HasNiNode(TESObjectREFR * obj, BSFixedString nodeName);
	float GetNiNodePositionX(TESObjectREFR * obj, BSFixedString nodeName);
	float GetNiNodePositionY(TESObjectREFR * obj, BSFixedString nodeName);
	float GetNiNodePositionZ(TESObjectREFR * obj, BSFixedString nodeName);
	float GetNiNodeScale(TESObjectREFR * obj, BSFixedString nodeName);
	void SetNiNodeScale(TESObjectREFR * obj, BSFixedString nodeName, float value);
}
