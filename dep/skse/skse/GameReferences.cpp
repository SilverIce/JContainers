#include "GameReferences.h"
#include "GameObjects.h"
#include "GameForms.h"
#include "GameRTTI.h"
#include "NiNodes.h"

const _CreateRefHandleByREFR	CreateRefHandleByREFR = (_CreateRefHandleByREFR)0x0065CC00;
const _LookupREFRByHandle		LookupREFRByHandle = (_LookupREFRByHandle)0x004A9180;

const UInt32 * g_invalidRefHandle = (UInt32*)0x01310630;

UInt32 TESObjectREFR::CreateRefHandle(void)
{
	if (handleRefObject.GetRefCount() > 0)
	{
		UInt32 refHandle = 0;
		CreateRefHandleByREFR(&refHandle, this);
		return refHandle;
	}
	else
	{
		return *g_invalidRefHandle;
	}
}

TESForm * Actor::GetEquippedObject(bool abLeftHand)
{
	if(!equipData) 
		return NULL;

	if(abLeftHand)
		return equipData->equippedObject[ActorEquipData::kEquippedHand_Left];
	else
		return equipData->equippedObject[ActorEquipData::kEquippedHand_Right];

	return NULL;
}

TintMask * PlayerCharacter::GetOverlayTintMask(TintMask * original)
{
	SInt32 curIndex = -1;
	if(!overlayTintMasks)
		return NULL;

	TintMask * foundMask;
	for(UInt32 i = 0; i < tintMasks.count; i++)
	{
		tintMasks.GetNthItem(i, foundMask);
		if(foundMask == original) {
			curIndex = i;
			break;
		}
	}

	overlayTintMasks->GetNthItem(curIndex, foundMask);
	if(foundMask)
		return foundMask;

	return NULL;
}

void PlayerCharacter::UpdateHairColor()
{
	TESNPC* npc = DYNAMIC_CAST(baseForm, TESForm, TESNPC);
	if(npc && npc->headData) {
		BGSColorForm * hairColor = npc->headData->hairColor;
		if(hairColor) {
			NiColorA val;
			val.r = hairColor->color.red / 128.0;
			val.g = hairColor->color.green / 128.0;
			val.b = hairColor->color.blue / 128.0;
			NiColorA * color = &val;

			if(loadedState && loadedState->node) {
				UpdateModelHair(loadedState->node, &color);
			}
		}
	}
}

void PlayerCharacter::UpdateSkinColor()
{
	TintMask * tintMask = CALL_MEMBER_FN(this, GetTintMask)(TintMask::kMaskType_SkinTone, 0);
	if(tintMask) {
		NiColorA val;
		val.r = tintMask->color.red / 255.0;
		val.g = tintMask->color.green / 255.0;
		val.b = tintMask->color.blue / 255.0;
		NiColorA * color = &val;
		if(loadedState && loadedState->node) {
			UpdateModelSkin(loadedState->node, &color); // Update for 3rd Person
		}
		if(firstPersonSkeleton) {
			UpdateModelSkin(firstPersonSkeleton, &color); // Update for 1st Person
		}
	}
}
