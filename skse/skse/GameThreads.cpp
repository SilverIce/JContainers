#include "GameThreads.h"
#include "GameAPI.h"
#include "GameReferences.h"
#include "GameData.h"
#include "GameForms.h"
#include "GameRTTI.h"

#include "NiNodes.h"

#include "common/IMemPool.h"

IThreadSafeBasicMemPool<SKSETaskUpdateTintMasks,10>		s_updateTintMasksDelegatePool;
IThreadSafeBasicMemPool<SKSETaskUpdateHairColor,10>		s_updateHairColorDelegatePool;
IThreadSafeBasicMemPool<SKSETaskUpdateWeight,10>		s_updateWeightDelegatePool;
IThreadSafeBasicMemPool<SKSETaskRegenHead,10>			s_regenHeadDelegatePool;
IThreadSafeBasicMemPool<SKSETaskChangeHeadPart,10>		s_changeHeadPartDelegatePool;

void BSTaskPool::UpdateTintMasks()
{
	SKSETaskUpdateTintMasks * cmd = s_updateTintMasksDelegatePool.Allocate();
	if(cmd) {
		QueueTask(cmd);
	}
}

void BSTaskPool::UpdateHairColor()
{
	SKSETaskUpdateHairColor * cmd = s_updateHairColorDelegatePool.Allocate();
	if(cmd) {
		QueueTask(cmd);
	}
}

void BSTaskPool::RegenerateHead(Actor * actor)
{
	SKSETaskRegenHead * cmd = SKSETaskRegenHead::Create(actor);
	if(cmd) {
		QueueTask(cmd);
	}
}

void BSTaskPool::ChangeHeadPart(Actor * actor, BGSHeadPart * oldPart, BGSHeadPart * newPart)
{
	SKSETaskChangeHeadPart * cmd = SKSETaskChangeHeadPart::Create(actor, oldPart, newPart);
	if(cmd) {
		QueueTask(cmd);
	}
}

void BSTaskPool::UpdateWeight(Actor * actor, float delta)
{
	SKSETaskUpdateWeight * cmd = SKSETaskUpdateWeight::Create(actor, delta);
	if(cmd) {
		QueueTask(cmd);
	}
}

void SKSETaskUpdateTintMasks::Dispose(void)
{
	s_updateTintMasksDelegatePool.Free(this);
}

void SKSETaskUpdateTintMasks::Run()
{
	(*g_thePlayer)->UpdateSkinColor();
	UpdatePlayerTints();
}

void SKSETaskUpdateHairColor::Run()
{
	(*g_thePlayer)->UpdateHairColor();
}

void SKSETaskUpdateHairColor::Dispose(void)
{
	s_updateHairColorDelegatePool.Free(this);
}

SKSETaskRegenHead * SKSETaskRegenHead::Create(Actor * actor)
{
	SKSETaskRegenHead * cmd = s_regenHeadDelegatePool.Allocate();
	if (cmd)
	{
		cmd->m_actor = actor;
	}
	return cmd;
}

void SKSETaskRegenHead::Dispose(void)
{
	s_regenHeadDelegatePool.Free(this);
}

void SKSETaskRegenHead::Run()
{
	TESNPC * npc = DYNAMIC_CAST(m_actor->baseForm, TESForm, TESNPC);
	BSFaceGenNiNode * faceNode = m_actor->GetFaceGenNiNode();
	BGSHeadPart * facePart = NULL;
	if(CALL_MEMBER_FN(npc, HasOverlays)()) {
		facePart = npc->GetHeadPartOverlayByType(BGSHeadPart::kTypeFace);
	} else {
		facePart = CALL_MEMBER_FN(npc, GetHeadPartByType)(BGSHeadPart::kTypeFace);
	}
	if(npc && faceNode && facePart) {
		CALL_MEMBER_FN(FaceGen::GetSingleton(), RegenerateHead)(faceNode, facePart, npc);
	}
}


SKSETaskChangeHeadPart * SKSETaskChangeHeadPart::Create(Actor * actor, BGSHeadPart* oldPart, BGSHeadPart* newPart)
{
	SKSETaskChangeHeadPart * cmd = s_changeHeadPartDelegatePool.Allocate();
	if (cmd)
	{
		cmd->m_actor = actor;
		cmd->m_newPart = newPart;
		cmd->m_oldPart = oldPart;
	}
	return cmd;
}

void SKSETaskChangeHeadPart::Dispose(void)
{
	s_changeHeadPartDelegatePool.Free(this);
}

void SKSETaskChangeHeadPart::Run()
{
	if(m_actor) {
		ChangeActorHeadPart(m_actor, m_oldPart, m_newPart);
	}
}

SKSETaskUpdateWeight * SKSETaskUpdateWeight::Create(Actor * actor, float delta)
{
	SKSETaskUpdateWeight * cmd = s_updateWeightDelegatePool.Allocate();
	if (cmd)
	{
		cmd->m_actor = actor;
		cmd->m_delta = delta;
	}
	return cmd;
}

void SKSETaskUpdateWeight::Dispose(void)
{
	s_updateWeightDelegatePool.Free(this);
}

void SKSETaskUpdateWeight::Run()
{
	if(m_actor) {
		TESNPC * npc = DYNAMIC_CAST(m_actor->baseForm, TESForm, TESNPC);
		if(npc) {
			BSFaceGenNiNode * faceNode = m_actor->GetFaceGenNiNode();
			CALL_MEMBER_FN(faceNode, AdjustHeadMorph)(BSFaceGenNiNode::kAdjustType_Neck, 0, m_delta);
			UpdateModelFace(faceNode);

			ActorWeightModel * lowModel = m_actor->GetWeightModel(ActorWeightModel::kWeightModel_Small);
			if(lowModel && lowModel->weightData)
				CALL_MEMBER_FN(lowModel->weightData, UpdateWeightData)();

			ActorWeightModel * highModel = m_actor->GetWeightModel(ActorWeightModel::kWeightModel_Large);
			if(highModel && highModel->weightData)
				CALL_MEMBER_FN(highModel->weightData, UpdateWeightData)();

			UInt32 updateFlags = ActorEquipData::kFlags_Unk01 | ActorEquipData::kFlags_Unk02 | ActorEquipData::kFlags_Unk03 | ActorEquipData::kFlags_Mobile;
			 // Resets ActorState
			//updateFlags |= ActorEquipData::kFlags_DrawHead | ActorEquipData::kFlags_Reset;
			

			CALL_MEMBER_FN(m_actor->equipData, SetEquipFlag)(updateFlags);
			CALL_MEMBER_FN(m_actor->equipData, UpdateEquipment)(m_actor);

			// Force redraw weapon, weight model update causes weapon position to be reset
			// Looking at DrawSheatheWeapon there is a lot of stuff going on, hard to find
			// out how to just manually move the weapon to its intended position
			if(m_actor->actorState.IsWeaponDrawn()) {
				m_actor->DrawSheatheWeapon(false);
				m_actor->DrawSheatheWeapon(true);
			}
		}
	}
}
