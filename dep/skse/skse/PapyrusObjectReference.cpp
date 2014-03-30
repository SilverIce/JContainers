#include "PapyrusObjectReference.h"

#include "GameAPI.h"
#include "GameFormComponents.h"
#include "GameForms.h"
#include "GameRTTI.h"
#include "GameObjects.h"
#include "GameExtraData.h"

#include "NiNodes.h"

#include <vector>
#include <map>

typedef std::vector<ExtraContainerChanges::EntryData*> ExtraDataVec;
typedef std::map<TESForm*, UInt32> ExtraContainerMap;

class ExtraContainerInfo
{
	ExtraDataVec	m_vec;
	ExtraContainerMap m_map;
public:
	ExtraContainerInfo(ExtraContainerChanges::EntryDataList * entryList) : m_map(), m_vec()
	{
		m_vec.reserve(128);
		if (entryList) {
			entryList->Visit(*this);
		}
	}

	bool Accept(ExtraContainerChanges::EntryData* data) 
	{
		if (data) {
			m_vec.push_back(data);
			m_map[data->type] = m_vec.size()-1;
		}
		return true;
	}

	bool IsValidEntry(TESContainer::Entry* pEntry, SInt32& numObjects)
	{
		if (pEntry) {
			numObjects = pEntry->count;
			TESForm* pForm = pEntry->form;

			if (DYNAMIC_CAST(pForm, TESForm, TESLevItem))
				return false;

			ExtraContainerMap::iterator it = m_map.find(pForm);
			ExtraContainerMap::iterator itEnd = m_map.end();
			if (it != itEnd) {
				UInt32 index = it->second;
				ExtraContainerChanges::EntryData* pXData = m_vec[index];
				if (pXData) {
					numObjects += pXData->countDelta;
				}
				// clear the object from the vector so we don't bother to look for it
				// in the second step
				m_vec[index] = NULL;
			}

			if (numObjects > 0) {
				//if (IsConsoleMode()) {
				//	PrintItemType(pForm);
				//}
				return true;
			}
		}
		return false;
	}

	// returns the count of items left in the vector
	UInt32 CountItems() {
		UInt32 count = 0;
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd) {
			ExtraContainerChanges::EntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0)) {
				count++;
				//if (IsConsoleMode()) {
				//	PrintItemType(extraData->type);
				//}
			}
			++it;
		}
		return count;
	}

	// returns the count of items left in the vector
	//UInt32 GetTotalWeight() {
	//	ExtraDataVec::iterator itEnd = m_vec.end();
	//	ExtraDataVec::iterator it = m_vec.begin();
	//	while (it != itEnd) {
	//		ExtraContainerChanges::EntryData* extraData = (*it);
	//		if (extraData && (extraData->countDelta > 0)) {
	//			
	//		}
	//		++it;
	//	}
	//	return count;
	//}


	ExtraContainerChanges::EntryData* GetNth(UInt32 n, UInt32 count) {
		ExtraDataVec::iterator itEnd = m_vec.end();
		ExtraDataVec::iterator it = m_vec.begin();
		while (it != itEnd) {
			ExtraContainerChanges::EntryData* extraData = (*it);
			if (extraData && (extraData->countDelta > 0)) {
				if(count == n)
				{
					return extraData;
				}
				count++;
			}
			++it;
		}
		return NULL;
	}

};


class ContainerCountIf
{
	ExtraContainerInfo& m_info;
public:
	ContainerCountIf(ExtraContainerInfo& info) : m_info(info) { }

	bool Accept(TESContainer::Entry* pEntry) const
	{
		SInt32 numObjects = 0; // not needed in this count
		return m_info.IsValidEntry(pEntry, numObjects);
	}
};

class ContainerFindNth
{
	ExtraContainerInfo& m_info;
	UInt32 m_findIndex;
	UInt32 m_curIndex;
public:
	ContainerFindNth(ExtraContainerInfo& info, UInt32 findIndex) : m_info(info), m_findIndex(findIndex), m_curIndex(0) { }

	bool Accept(TESContainer::Entry* pEntry)
	{
		SInt32 numObjects = 0;
		if (m_info.IsValidEntry(pEntry, numObjects)) {
			if (m_curIndex == m_findIndex) {
				return true;
			}
			m_curIndex++;
		}
		return false;
	}

	UInt32 GetCurIdx() { return m_curIndex; }
};

//class ContainerTotalWeight
//{
//	ExtraContainerInfo& m_info;
//	float totalWeight;
//public:
//	ContainerTotalWeight(ExtraContainerInfo& info) : m_info(info), totalWeight(0.0) { }
//
//	bool Accept(TESCOntainer::Entry* pEntry)
//
//
//}

namespace papyrusObjectReference
{
	UInt32 GetNumItems(TESObjectREFR* pContainerRef)
	{
		if (!pContainerRef)
			return 0;

		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm) {
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return 0;

		UInt32 count = 0;
			
		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);


		// first walk the base container
		if (pContainer) {
			ContainerCountIf counter(info);
			count = pContainer->CountIf(counter);
		}

		// now count the remaining items
		count += info.CountItems();

		return count;
	}
	

	TESForm* GetNthForm(TESObjectREFR* pContainerRef, UInt32 n)
	{
		if (!pContainerRef)
			return NULL;

		TESContainer* pContainer = NULL;
		TESForm* pBaseForm = pContainerRef->baseForm;
		if (pBaseForm) {
			pContainer = DYNAMIC_CAST(pBaseForm, TESForm, TESContainer);
		}
		if (!pContainer)
			return NULL;
		
		UInt32 count = 0;

		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->data->objList : NULL);

		// first look in the base container
		if (pContainer) {
			ContainerFindNth finder(info, n);
			TESContainer::Entry* pFound = pContainer->Find(finder);
			if (pFound) {
				return pFound->form;
			} else {
				count = finder.GetCurIdx();
			}
		}

		// now walk the remaining items in the map
		ExtraContainerChanges::EntryData* pEntryData = info.GetNth(n, count);
		if (pEntryData) {
			return pEntryData->type;
		}
		return NULL;
	}

	float GetTotalItemWeight(TESObjectREFR* pContainerRef)
	{
		if (!pContainerRef)
			return 0;
		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		if (!pXContainerChanges)
			return 0.0;
		
		// skyrim keeps track of the player's total item weight
		if (pContainerRef == *g_thePlayer)
			return pXContainerChanges->data->totalWeight;

		// but not so much for anything else - so we need to calculate
		return 0.0;
	}

	float GetTotalArmorWeight(TESObjectREFR* pContainerRef)
	{
		if (!pContainerRef)
			return 0;
		ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>(pContainerRef->extraData.GetByType(kExtraData_ContainerChanges));
		return (pXContainerChanges) ? pXContainerChanges->data->armorWeight : 0.0;
	}

	bool IsHarvested(TESObjectREFR* pProduceRef)
	{
		UInt8 formType = pProduceRef->baseForm->formType;
		if (formType == kFormType_Tree || formType == kFormType_Flora) {
			return (pProduceRef->flags & TESObjectREFR::kFlag_Harvested) != 0;
		}
		return false;
	}

	void SetHarvested(TESObjectREFR * pProduceRef, bool isHarvested)
	{
		UInt8 formType = pProduceRef->baseForm->formType;
		if (formType == kFormType_Tree || formType == kFormType_Flora) {
			if(isHarvested)
				pProduceRef->flags |= TESObjectREFR::kFlag_Harvested;
			else
				pProduceRef->flags &= ~TESObjectREFR::kFlag_Harvested;
		}
	}

	void SetItemHealthPercent(TESObjectREFR* object, float value)
	{
		// Object must be a weapon, or armor
		if(object) {
			if(DYNAMIC_CAST(object->baseForm, TESForm, TESObjectWEAP) || DYNAMIC_CAST(object->baseForm, TESForm, TESObjectARMO)) {
				ExtraHealth* xHealth = static_cast<ExtraHealth*>(object->extraData.GetByType(kExtraData_Health));
				if(xHealth) {
					xHealth->health = value;
				} else  {
					ExtraHealth* newHealth = ExtraHealth::Create();
					newHealth->health = value;
					object->extraData.Add(kExtraData_Health, newHealth);
				}
			}
		}
	}

	float GetItemMaxCharge(TESObjectREFR* object)
	{
		if (!object)
			return 0.0;
		TESObjectWEAP * weapon = DYNAMIC_CAST(object->baseForm, TESForm, TESObjectWEAP);
		if(!weapon) // Object is not a weapon
			return 0.0;
		float maxCharge = 0;
		if(weapon->enchantable.enchantment != NULL) // Base enchant
			maxCharge = (float)weapon->enchantable.maxCharge;
		else if(ExtraEnchantment* extraEnchant = static_cast<ExtraEnchantment*>(object->extraData.GetByType(kExtraData_Enchantment))) // Enchanted
			maxCharge = (float)extraEnchant->maxCharge;
		return maxCharge;
	}

	float GetItemCharge(TESObjectREFR* object)
	{
		if (!object)
			return 0.0;
		TESObjectWEAP * weapon = DYNAMIC_CAST(object->baseForm, TESForm, TESObjectWEAP);
		if(!weapon) // Object is not a weapon
			return 0.0;
		ExtraCharge* xCharge = static_cast<ExtraCharge*>(object->extraData.GetByType(kExtraData_Charge));
		return (xCharge) ? xCharge->charge : GetItemMaxCharge(object); // When charge value is not present on an enchanted weapon, maximum charge is assumed
	}

	void SetItemCharge(TESObjectREFR* object, float value)
	{
		// Object must be an enchanted weapon
		if(object) {
			TESObjectWEAP * weapon = DYNAMIC_CAST(object->baseForm, TESForm, TESObjectWEAP);
			if(weapon && ((object->extraData.GetByType(kExtraData_Enchantment) || weapon->enchantable.enchantment != NULL))) {
				ExtraCharge* xCharge = static_cast<ExtraCharge*>(object->extraData.GetByType(kExtraData_Charge));
				if(xCharge) {
					xCharge->charge = value;
				} else {
					ExtraCharge* newCharge = ExtraCharge::Create();
					newCharge->charge = value;
					object->extraData.Add(kExtraData_Charge, newCharge);
				}
			}
		}
	}

	void ResetInventory(TESObjectREFR * obj)
	{
		obj->ResetInventory(false);
	}

	bool IsOffLimits(TESObjectREFR * obj)
	{
		return CALL_MEMBER_FN(obj, IsOffLimits)();
	}
};

#include "PapyrusVM.h"
#include "PapyrusNativeFunctions.h"

void papyrusObjectReference::RegisterFuncs(VMClassRegistry* registry)
{
	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, UInt32>("GetNumItems", "ObjectReference", papyrusObjectReference::GetNumItems, registry));

	registry->RegisterFunction(
		new NativeFunction1<TESObjectREFR, TESForm*, UInt32>("GetNthForm", "ObjectReference", papyrusObjectReference::GetNthForm, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, float>("GetTotalItemWeight", "ObjectReference", papyrusObjectReference::GetTotalItemWeight, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, float>("GetTotalArmorWeight", "ObjectReference", papyrusObjectReference::GetTotalArmorWeight, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, bool>("IsHarvested", "ObjectReference", papyrusObjectReference::IsHarvested, registry));

	registry->RegisterFunction(
		new NativeFunction1<TESObjectREFR, void, bool>("SetHarvested", "ObjectReference", papyrusObjectReference::SetHarvested, registry));

	// Item modifications, Tempering/Charges
	registry->RegisterFunction(
		new NativeFunction1<TESObjectREFR, void, float>("SetItemHealthPercent", "ObjectReference", papyrusObjectReference::SetItemHealthPercent, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, float>("GetItemMaxCharge", "ObjectReference", papyrusObjectReference::GetItemMaxCharge, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, float>("GetItemCharge", "ObjectReference", papyrusObjectReference::GetItemCharge, registry));

	registry->RegisterFunction(
		new NativeFunction1<TESObjectREFR, void, float>("SetItemCharge", "ObjectReference", papyrusObjectReference::SetItemCharge, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, void>("ResetInventory", "ObjectReference", papyrusObjectReference::ResetInventory, registry));

	registry->RegisterFunction(
		new NativeFunction0<TESObjectREFR, bool>("IsOffLimits", "ObjectReference", papyrusObjectReference::IsOffLimits, registry));
}
