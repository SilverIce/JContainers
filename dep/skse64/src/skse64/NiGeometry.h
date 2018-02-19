#pragma once

#include "skse64/NiNodes.h"
#include "skse64/NiProperties.h"

// NiGeometry, NiGeometryData and children
MAKE_NI_POINTER(NiGeometryData);
MAKE_NI_POINTER(NiAdditionalGeometryData);
MAKE_NI_POINTER(NiSkinInstance);
MAKE_NI_POINTER(NiProperty);
MAKE_NI_POINTER(NiSkinData);
MAKE_NI_POINTER(NiSkinPartition);

class NiAdditionalGeometryData;
class NiTriShapeData;
class NiTriStripsData;

struct ID3D11Buffer;

// 138
class NiGeometry : public NiAVObject
{
public:
	virtual void Unk_33(void); // call controller vtbl+0xA0?
	virtual void Unk_34(void); // ret 0
	virtual void Unk_35(void); // same as Unk_33
	virtual void * Unk_36(void); // ret call m_spModelData vtbl+0x9C
	virtual void SetGeometryData(NiGeometryData * unk1); // set and AddRef geometry data
	virtual void * Unk_38(void); // ret call m_spModelData vtbl+0x94
	virtual UInt16 Unk_39(bool unk1); // ??

	NiPropertyPtr		m_spPropertyState;	// 110
	NiPropertyPtr		m_spEffectState;	// 118
	NiSkinInstancePtr	m_spSkinInstance;	// 120
	NiGeometryDataPtr	m_spModelData;		// 128
	UInt64				unk130;				// 130
};

class NiTriBasedGeom : public NiGeometry
{
public:
};

class NiTriShape : public NiTriBasedGeom
{
public:
	static NiTriShape * Create(NiTriShapeData * geometry);

	MEMBER_FN_PREFIX(NiTriShape);
	DEFINE_MEMBER_FN(ctor, NiTriShape *, 0x00000000, NiTriShapeData * geometry);
};

class BSSegmentedTriShape : public NiTriShape
{
public:
};


class NiTriStrips : public NiTriBasedGeom
{
public:
	static NiTriStrips * Create(NiTriStripsData * geometry);

	MEMBER_FN_PREFIX(NiTriStrips);
	DEFINE_MEMBER_FN(ctor, NiTriStrips *, 0x00000000, NiTriStripsData * geometry);
};

class BSGeometryData
{
public:
	ID3D11Buffer * buffer1;		// 00
	ID3D11Buffer * buffer2;		// 08
	UInt16		unk10;			// 10
	UInt16		unk12;			// 12
	UInt16		unk14;			// 14
	UInt16		unk16;			// 16
	volatile	UInt32	refCount;	// 18
	UInt16		unk1C;			// 1C
	UInt16		unk1E;			// 1E
	UInt8		* vertices;		// 20
	UInt8		* triangles;	// 28
};

// 158
class BSGeometry : public NiAVObject
{
public:
	virtual ~BSGeometry();

	UInt64				unk110;				// 110
	UInt64				unk118;				// 118
	NiPropertyPtr		m_spPropertyState;	// 120
	NiPropertyPtr		m_spEffectState;	// 128
	NiSkinInstancePtr	m_spSkinInstance;	// 130
	BSGeometryData		* geometryData;		// 138
	UInt64				unk140;				// 140
	UInt64				unk148;				// 148
	UInt8				unk150;				// 150 - type? 3, 4
	UInt8				unk151;				// 151
	UInt16				unk152;				// 152
	UInt32				unk154;				// 154
};

// class 160
class BSTriShape : public BSGeometry
{
public:
	virtual ~BSTriShape();

	UInt16				unk158;				// 158
	UInt16				unk15A;				// 15A
	UInt16				unk15C;				// 15C
	UInt16				unk15D;				// 15D
};

// 48+
class NiGeometryData : public NiObject
{
public:
	enum
	{
		kDataFlag_HasUVs =	1 << 0,
		kDataFlag_HasNBT =	1 << 12,
	};

	enum
	{
		kConsistency_Mutable	= 0,
		kConsistency_Static		= 0x4000,
		kConsistency_Volatile	= 0x8000,
		kConsistency_Mask		= 0xF000
	};

	enum
	{
		kKeep_XYZ		= 1 << 0,
		kKeep_Norm		= 1 << 1,
		kKeep_Color		= 1 << 2,
		kKeep_UV		= 1 << 3,
		kKeep_Indices	= 1 << 4,
		kKeep_BoneData	= 1 << 5,
		kKeep_All		= (kKeep_XYZ | kKeep_Norm | kKeep_Color | kKeep_UV | kKeep_Indices | kKeep_BoneData)
	};

	UInt16	m_usVertices;				// 08
	UInt16	m_usID;						// 0A
	UInt16	m_usDirtyFlags;				// 0C
	UInt16	m_usDataFlags;				// 0E
	NiBound	m_kBound;					// 10
	NiPoint3	* m_pkVertex;			// 20
	NiPoint3	* m_pkNormal;			// 24 - all normals, then all binormals etc
	NiColorA	* m_pkColor;			// 28 - yes really, floats (b g r a)
	NiPoint2	* m_pkTexture;			// 2C
	UInt32	unk30;						// 30
	UInt32	unk34;						// 34
	UInt32	unkInt2;					// 38
	NiAdditionalGeometryDataPtr	m_spAdditionalGeomData;	// 3C
	UInt32	unk40;						// 40
	UInt8	m_ucKeepFlags;				// 44
	UInt8	m_ucCompressFlags;			// 45
	UInt8	hasGeoData;					// 46

	void	AllocateVerts(UInt32 numVerts);
	void	AllocateNormals(UInt32 numVerts);
	void	AllocateNBT(UInt32 numVerts);
	void	AllocateColors(UInt32 numVerts);

	struct Data0
	{
		UInt32	unk00;
		UInt32	unk04;
		UInt32	unk08;
	};
};

// 4C
class NiTriBasedGeomData : public NiGeometryData
{
public:
	UInt16	m_usTriangles;			// 48
	UInt16	m_usActiveTriangles;	// 4A
};

// 54
class NiTriShapeData : public NiTriBasedGeomData
{
public:
	UInt32	m_uiTriListLength;		// 4C
	UInt16	* m_pusTriList;			// 50
};

// 5C
class BSSharedVertexesTriShapeData : public NiTriShapeData
{
public:
	NiTriShapeData	* m_refData;
	UInt32			unk58;
};

class NiTriStripsData : public NiTriBasedGeomData
{
public:
	UInt16	m_usStrips;
	UInt16	* m_pusStripLengths;
	UInt16	* m_pusStripLists;
};

// 58
class NiTriShapeDynamicData : public NiTriShapeData
{
public:
	struct SharedNormalArray
	{
		UInt16	m_usNumSharedNormals;
		UInt16	* m_pusSharedNormalIndexArray;
	};
	SharedNormalArray * m_pkSharedNormals;	// 54
	UInt16				m_usSharedNormalsArraySize;	// 56
};

// 10
class NiSkinPartition : public NiObject
{
public:
	// 28
	struct Partition
	{
		UInt16		* m_pusBones;			// 00
		float		* m_pfWeights;			// 04
		UInt16		* m_pusVertexMap;		// 08
		UInt8		* m_pucBonePalette;		// 0C
		UInt16		* m_pusTriList;			// 10
		UInt16		* m_pusStripLengths;	// 14
		UInt16		m_usVertices;			// 18
		UInt16		m_usTriangles;			// 1A
		UInt16		m_usBones;				// 1C
		UInt16		m_usStrips;				// 1E
		UInt16		m_usBonesPerVertex;		// 20
		UInt8		pad22[2];				// 22
		UInt32		unk24;					// 24

		void	AllocateWeights(UInt32 numVerts);
	};

	UInt32		m_uiPartitions;		// 08
	Partition	* m_pkPartitions;	// 0C
};

// 48
class NiSkinData : public NiObject
{
public:
	// 08
	struct BoneVertData
	{
		UInt16	m_usVert;	// 00
		UInt8	pad02[2];	// 02
		float	m_fWeight;	// 04?
	};

	// 4C
	struct BoneData
	{
		NiTransform		m_kSkinToBone;		// 00
		NiBound			m_kBound;			// 34
		BoneVertData	* m_pkBoneVertData;	// 44
		UInt16			m_usVerts;			// 48
		UInt8			pad4A[2];			// 4A

		void	AllocateWeights(UInt32 numVerts);
	};

	NiSkinPartition	* m_spSkinPartition;	// 08
	NiTransform		m_kRootParentToSkin;	// 0C
	BoneData		* m_pkBoneData;			// 40
	UInt32			m_uiBones;				// 44

	// ctor - AD4780
};

STATIC_ASSERT(sizeof(NiSkinData::BoneVertData) == 0x08);

// 88
class NiSkinInstance : public NiObject
{
public:
	NiSkinDataPtr		m_spSkinData;		// 08
	NiSkinPartitionPtr	m_spSkinPartition;	// 10
	NiAVObject			* m_pkRootParent;	// 18
	NiAVObject			** m_ppkBones;		// 20
	
	NiTransform			** m_worldTransforms;// 28
	SInt32				unk30;				// 30
	UInt32				m_uiBoneNodes;		// 34
	UInt32				numFlags;			// 38
	UInt32				unk3C;				// 3C
	UInt32 				* flags;			// 40
	UInt32				unk48;				// 48
	UInt32				unk4C;				// 4C
	UInt64				unk50;				// 50
	UInt64				unk58;				// 58
	CRITICAL_SECTION	lock;				// 60

	static NiSkinInstance * Create();

	NiSkinInstance * Clone(bool reuse = true);

	MEMBER_FN_PREFIX(NiSkinInstance);
	DEFINE_MEMBER_FN(Copy, NiSkinInstance*, 0x00C51DE0);
	DEFINE_MEMBER_FN(ctor, NiSkinInstance *, 0x00C7E1F0);
};
//STATIC_ASSERT(sizeof(NiSkinInstance) == 0x38);

// 100
class BSDismemberSkinInstance : public NiSkinInstance
{
public:
	UInt32	numPartitions;					// 88
	UInt32	unk8C;							// 8C
	UInt32	* partitionFlags;				// 90
	UInt8	unk98;							// 98
	UInt8	pad99[3];						// 99

	static BSDismemberSkinInstance * Create();

	MEMBER_FN_PREFIX(BSDismemberSkinInstance);
	DEFINE_MEMBER_FN(ctor, BSDismemberSkinInstance *, 0x00C6B300);
};
//STATIC_ASSERT(sizeof(BSDismemberSkinInstance) == 0x44);
