#include "skse64/NiExtraData.h"
#include "skse64/NiGeometry.h"
#include "skse64/NiAllocator.h"

static const UInt32 s_BSFaceGenBaseMorphExtraDataVtbl = 0x00000000;
static const UInt32 s_NiStringsExtraDataVtbl = 0x00000000;
static const UInt32 s_NiBinaryExtraDataVtbl = 0x00000000;

NiExtraData* NiExtraData::Create(UInt32 size, UInt32 vtbl)
{
	void* memory = Heap_Allocate(size);
	memset(memory, 0, size);
	NiExtraData* xData = (NiExtraData*)memory;
	((UInt32*)memory)[0] = vtbl;
	return xData;
}

NiStringsExtraData * NiStringsExtraData::Create(BSFixedString name, BSFixedString * stringData, UInt32 size)
{
	NiStringsExtraData* data = (NiStringsExtraData*)NiExtraData::Create(sizeof(NiStringsExtraData), s_NiStringsExtraDataVtbl);
	data->m_pcName = const_cast<char*>(name.data);
	data->m_data = (char**)NiAllocate(sizeof(char*) * size);
	data->m_size = size;
	
	for (int i = 0; i < size; i++)
	{
		UInt32 strLength = strlen(stringData[i].data) + 1;
		data->m_data[i] = (char*)NiAllocate(sizeof(char*) * strLength);
		memcpy(data->m_data[i], stringData[i].data, sizeof(char) * strLength);
	}

	return data;
}

void NiStringsExtraData::SetData(BSFixedString * stringData, UInt32 size)
{
	if (size != m_size)
	{
		if (m_data)
		{
			for (int i = 0; i < size; i++)
			{
				NiFree(m_data[i]);
			}
			NiFree(m_data);
			m_data = NULL;
		}
		if (size > 0) {
			m_data = (char**)NiAllocate(sizeof(char*) * size);
		}
		
		m_size = size;
	}

	for (int i = 0; i < size; i++)
	{
		UInt32 strLength = strlen(stringData[i].data) + 1;
		m_data[i] = (char*)NiAllocate(sizeof(char*) * strLength);
		memcpy(m_data[i], stringData[i].data, sizeof(char) * strLength);
	}
}

NiBinaryExtraData * NiBinaryExtraData::Create(BSFixedString name, char * binary, UInt32 size)
{
	NiBinaryExtraData* data = (NiBinaryExtraData*)NiExtraData::Create(sizeof(NiBinaryExtraData), s_NiBinaryExtraDataVtbl);
	data->m_pcName = const_cast<char*>(name.data);
	if (size > 0)
	{
		data->m_data = (char*)NiAllocate(size);
		memcpy(data->m_data, binary, size);
		data->m_size = size;
	}
	else
	{
		data->m_data = NULL;
		data->m_size = 0;
	}
	
	return data;
}

void NiBinaryExtraData::SetData(char * data, UInt32 size)
{
	if (m_data)
	{
		NiFree(m_data);
		m_data = NULL;
	}

	if (size > 0)
	{
		m_data = (char*)NiAllocate(size);
		memcpy(m_data, data, size);
	}

	m_size = size;
}

BSFaceGenBaseMorphExtraData* BSFaceGenBaseMorphExtraData::Create(NiGeometryData * geometryData, bool copy)
{
	BSFaceGenBaseMorphExtraData* data = (BSFaceGenBaseMorphExtraData*)NiExtraData::Create(sizeof(BSFaceGenBaseMorphExtraData), s_BSFaceGenBaseMorphExtraDataVtbl);
	data->m_pcName = const_cast<char*>(BSFixedString("FOD").data);
	data->m_uiRefCount = 0;
	data->modelVertexCount = 0;
	data->vertexCount = 0;
	data->vertexData = NULL;

	if (geometryData) {
		data->vertexCount = geometryData->m_usVertices;
		data->modelVertexCount = geometryData->m_usVertices;

		data->vertexData = (NiPoint3*)Heap_Allocate(sizeof(NiPoint3) * data->vertexCount);
		if(copy)
			memcpy(data->vertexData, geometryData->m_pkVertex, sizeof(NiPoint3) * data->vertexCount);
		else
			memset(data->vertexData, 0, sizeof(NiPoint3) * data->vertexCount);
	}

	return data;
}
