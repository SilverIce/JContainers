#include "PapyrusNetImmerse.h"

#include "GameAPI.h"
#include "GameForms.h"
#include "GameRTTI.h"
#include "GameReferences.h"
#include "GameObjects.h"
#include "GameThreads.h"

#include "NiNodes.h"
#include "NiGeometry.h"


namespace papyrusNetImmerse
{
	NiAVObject * ResolveNode(TESObjectREFR * obj, BSFixedString nodeName, bool firstPerson)
	{
		if(!obj) return NULL;

		NiAVObject	* result = obj->GetNiNode();

		// special-case for the player, switch between first/third-person
		PlayerCharacter * player = DYNAMIC_CAST(obj, TESObjectREFR, PlayerCharacter);
		if(player && player->loadedState)
			result = firstPerson ? player->firstPersonSkeleton : player->loadedState->node;

		// name lookup
		if(nodeName.data[0] && result)
			result = result->GetObjectByName(&nodeName.data);

		return result;
	}

	bool HasNode(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, bool firstPerson)
	{
		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		return object != NULL;
	}

	float GetNodePositionX(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, bool firstPerson)
	{
		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		return object ? object->m_worldTransform.pos.x : 0;
	}

	float GetNodePositionY(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, bool firstPerson)
	{
		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		return object ? object->m_worldTransform.pos.y : 0;
	}

	float GetNodePositionZ(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, bool firstPerson)
	{
		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		return object ? object->m_worldTransform.pos.z : 0;
	}

	float GetNodeScale(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, bool firstPerson)
	{
		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		return object ? object->m_localTransform.scale : 0;
	}

	void SetNodeScale(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, float value, bool firstPerson)
	{
		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		if(object)
		{
			object->m_localTransform.scale = value;
			NiAVObject::ControllerUpdateContext ctx;
			object->UpdateWorldData(&ctx);
		}
	}

	void SetNodeTextureSet(StaticFunctionTag* base, TESObjectREFR * obj, BSFixedString nodeName, BGSTextureSet * textureSet, bool firstPerson)
	{
		if(!textureSet) return;

		NiAVObject	* object = ResolveNode(obj, nodeName, firstPerson);

		if(object)
		{
			NiGeometry * geometry = object->GetAsNiGeometry();
			if(geometry)
			{
				BSTaskPool * taskPool = BSTaskPool::GetSingleton();
				if(taskPool)
				{
					CALL_MEMBER_FN(taskPool, SetNiGeometryTexture)(geometry, textureSet);
				}
			}
		}
	}
};

#include "PapyrusVM.h"
#include "PapyrusNativeFunctions.h"

void papyrusNetImmerse::RegisterFuncs(VMClassRegistry* registry)
{
	// NiNode Manipulation
	registry->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, bool, TESObjectREFR*, BSFixedString, bool>("HasNode", "NetImmerse", papyrusNetImmerse::HasNode, registry));

	registry->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, float, TESObjectREFR*, BSFixedString, bool>("GetNodePositionX", "NetImmerse", papyrusNetImmerse::GetNodePositionX, registry));

	registry->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, float, TESObjectREFR*, BSFixedString, bool>("GetNodePositionY", "NetImmerse", papyrusNetImmerse::GetNodePositionY, registry));

	registry->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, float, TESObjectREFR*, BSFixedString, bool>("GetNodePositionZ", "NetImmerse", papyrusNetImmerse::GetNodePositionZ, registry));

	registry->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, float, TESObjectREFR*, BSFixedString, bool>("GetNodeScale", "NetImmerse", papyrusNetImmerse::GetNodeScale, registry));

	registry->RegisterFunction(
		new NativeFunction4<StaticFunctionTag, void, TESObjectREFR*, BSFixedString, float, bool>("SetNodeScale", "NetImmerse", papyrusNetImmerse::SetNodeScale, registry));

	registry->RegisterFunction(
		new NativeFunction4<StaticFunctionTag, void, TESObjectREFR*, BSFixedString, BGSTextureSet*, bool>("SetNodeTextureSet", "NetImmerse", papyrusNetImmerse::SetNodeTextureSet, registry));

	registry->SetFunctionFlags("NetImmerse", "HasNode", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags("NetImmerse", "GetNodePositionX", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags("NetImmerse", "GetNodePositionY", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags("NetImmerse", "GetNodePositionZ", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags("NetImmerse", "GetNodeScale", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags("NetImmerse", "SetNodeScale", VMClassRegistry::kFunctionFlag_NoWait);
}
