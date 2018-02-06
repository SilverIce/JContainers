#include "skse64/NiGeometry.h"
#include "skse64/NiAllocator.h"
#include "skse64/GameAPI.h"

void NiGeometryData::AllocateVerts(UInt32 numVerts)
{
	m_pkVertex = (NiPoint3 *)Heap_Allocate(sizeof(NiPoint3) * numVerts);
	m_pkTexture = (NiPoint2 *)Heap_Allocate(sizeof(NiPoint2) * numVerts);

	memset(m_pkVertex, 0, sizeof(NiPoint3) * numVerts);
	memset(m_pkTexture, 0, sizeof(NiPoint2) * numVerts);
}

void NiGeometryData::AllocateNormals(UInt32 numVerts)
{
	m_pkNormal = (NiPoint3 *)Heap_Allocate(sizeof(NiPoint3) * numVerts);
	memset(m_pkNormal, 0, sizeof(NiPoint3) * numVerts);
}

void NiGeometryData::AllocateNBT(UInt32 numVerts)
{
	m_pkNormal = (NiPoint3 *)Heap_Allocate(sizeof(NiPoint3) * 3 * numVerts);
	memset(m_pkNormal, 0, sizeof(NiPoint3) * 3 * numVerts);
}

void NiGeometryData::AllocateColors(UInt32 numVerts)
{
	m_pkColor = (NiColorA *)Heap_Allocate(sizeof(NiColorA) * numVerts);
	memset(m_pkColor, 0, sizeof(NiColorA) * numVerts);
}

void NiSkinPartition::Partition::AllocateWeights(UInt32 numVerts)
{
	m_pfWeights = (float *)Heap_Allocate(sizeof(float) * 4 * numVerts);
	m_pucBonePalette = (UInt8 *)Heap_Allocate(sizeof(UInt8) * 4 * numVerts);

	memset(m_pfWeights, 0, sizeof(float) * 4 * numVerts);
	memset(m_pucBonePalette, 0, sizeof(UInt8) * 4 * numVerts);
}

void NiSkinData::BoneData::AllocateWeights(UInt32 numWeights)
{
	m_pkBoneVertData = (BoneVertData *)Heap_Allocate(sizeof(BoneVertData) * numWeights);
	memset(m_pkBoneVertData, 0, sizeof(BoneVertData) * numWeights);
}

NiSkinInstance * NiSkinInstance::Create()
{
	void* memory = Heap_Allocate(sizeof(NiSkinInstance));
	memset(memory, 0, sizeof(NiSkinInstance));
	NiSkinInstance* xData = (NiSkinInstance*)memory;
	CALL_MEMBER_FN(xData, ctor)();
	return xData;
}

BSDismemberSkinInstance * BSDismemberSkinInstance::Create()
{
	void* memory = Heap_Allocate(sizeof(BSDismemberSkinInstance));
	memset(memory, 0, sizeof(BSDismemberSkinInstance));
	BSDismemberSkinInstance* xData = (BSDismemberSkinInstance*)memory;
	CALL_MEMBER_FN(xData, ctor)();
	return xData;
}

NiSkinInstance * NiSkinInstance::Clone(bool reuse)
{
	NiSkinInstance * newSkinInstance = CALL_MEMBER_FN(this, Copy)();
	if (!reuse && newSkinInstance == this)
	{
		BSDismemberSkinInstance* srcSkin = ni_cast(this, BSDismemberSkinInstance);
		if (srcSkin)
		{
			newSkinInstance = BSDismemberSkinInstance::Create();
			BSDismemberSkinInstance* dstSkin = ni_cast(newSkinInstance, BSDismemberSkinInstance);
			dstSkin->numPartitions = srcSkin->numPartitions;
			dstSkin->partitionFlags = (UInt32 *)Heap_Allocate(sizeof(UInt32) * srcSkin->numPartitions);
			memcpy(dstSkin->partitionFlags, srcSkin->partitionFlags, sizeof(UInt32) * srcSkin->numPartitions);
			dstSkin->unk98 = srcSkin->unk98;
			memcpy(dstSkin->pad99, srcSkin->pad99, 3);
		}
		else
		{
			newSkinInstance = NiSkinInstance::Create();
		}

		newSkinInstance->m_spSkinData = m_spSkinData;
		newSkinInstance->m_spSkinPartition = m_spSkinPartition;
		newSkinInstance->m_pkRootParent = m_pkRootParent;
		newSkinInstance->m_ppkBones = (NiAVObject**)NiAllocate(sizeof(NiAVObject*) * m_uiBoneNodes);
		memcpy(newSkinInstance->m_ppkBones, this->m_ppkBones, sizeof(NiAVObject*) * m_uiBoneNodes);
		newSkinInstance->unk30 = unk30;
		newSkinInstance->m_uiBoneNodes = m_uiBoneNodes;
		newSkinInstance->numFlags = numFlags;
		newSkinInstance->flags = (UInt32 *)NiAllocate(sizeof(UInt32) * numFlags);
		newSkinInstance->unk3C = unk3C;
		memcpy(newSkinInstance->flags, flags, sizeof(UInt32) * numFlags);
		newSkinInstance->unk48 = unk48;
		newSkinInstance->unk4C = unk4C;
		newSkinInstance->unk50 = unk50;
		newSkinInstance->unk58 = unk58;

		NiSkinData * skinData = niptr_cast<NiSkinData>(newSkinInstance->m_spSkinData);
		if (skinData) {
			newSkinInstance->m_worldTransforms = (NiTransform**)NiAllocate(sizeof(NiTransform*) * skinData->m_uiBones);
			memcpy(newSkinInstance->m_worldTransforms, this->m_worldTransforms, sizeof(NiTransform*) * skinData->m_uiBones);
		}
	}
	else
	{
		if (!newSkinInstance->flags && numFlags > 0)
		{
			newSkinInstance->numFlags = numFlags;
			newSkinInstance->flags = (UInt32 *)NiAllocate(sizeof(UInt32) * numFlags);
			memcpy(newSkinInstance->flags, flags, sizeof(UInt32) * numFlags);
		}

		newSkinInstance->m_uiBoneNodes = m_uiBoneNodes;
		newSkinInstance->unk3C = unk3C;
		newSkinInstance->unk30 = unk30;
		newSkinInstance->unk48 = unk48;
		newSkinInstance->unk4C = unk4C;
		newSkinInstance->unk50 = unk50;
		newSkinInstance->unk58 = unk58;
	}

	return newSkinInstance;
}
