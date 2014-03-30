#pragma once

#include "skse/NiTypes.h"
#include "skse/NiObjects.h"

#include "GameCamera.h"

class BSFaceGenAnimationData;

// B8
class NiNode : public NiAVObject
{
public:
	virtual void	AttachChild(NiAVObject * obj, bool firstAvail);
	virtual void	DetachChild(UInt32 unk1, NiAVObject * obj);
	virtual void	Unk_35(void);
	virtual void	RemoveChild(NiAVObject * obj);
	virtual void	Unk_37(void);
	virtual void	Unk_38(void);
	virtual void	Unk_39(void);
	virtual void	Unk_3A(void);
	virtual void	UpdateUpwardPass(void);

	NiTArray <NiAVObject *>	m_children;	// A8
};

STATIC_ASSERT(sizeof(NiNode) == 0xB8);

// EC
class BSFaceGenNiNode : public NiNode
{
public:
	UInt32	unkBC;
	UInt32	unkC0;
	UInt32	unkC4;
	float	unkC8;
	UInt32	unkCC;
	UInt32	unkD0;
	UInt32	unkD4;
	float	unkD8;
	BSFaceGenAnimationData	* animData;
	float	unkE0;
	UInt32	unkE4;
	UInt32	unkE8;
	UInt32	unkEC;

	enum {
		kAdjustType_Unk0 = 0,
		kAdjustType_Unk1 = 1,
		kAdjustType_Unk2 = 2,
		kAdjustType_Neck = 3,
	};

	MEMBER_FN_PREFIX(BSFaceGenNiNode);
	DEFINE_MEMBER_FN(AdjustHeadMorph, void, 0x005A8270, UInt32 unk04, UInt32 unk08, float delta);
};

STATIC_ASSERT(sizeof(BSFaceGenNiNode) == 0xEC);


class NiSwitchNode : public NiNode
{
public:
	// Nothing yet
};

typedef UInt32 (* _UpdateModelSkin)(NiNode*, NiColorA**);
extern _UpdateModelSkin UpdateModelSkin;

typedef UInt32 (* _UpdateModelHair)(NiNode*, NiColorA**);
extern _UpdateModelHair UpdateModelHair;

typedef UInt32 (* _UpdateModelFace)(NiNode*);
extern _UpdateModelFace UpdateModelFace;

// 110 ?
class NiCullingProcess
{
public:
	virtual ~NiCullingProcess();
	virtual void	Unk_01(void); // 01 - 10 unused
	virtual void	Unk_02(void);
	virtual void	Unk_03(void);
	virtual void	Unk_04(void);
	virtual void	Unk_05(void);
	virtual void	Unk_06(void);
	virtual void	Unk_07(void);
	virtual void	Unk_08(void);
	virtual void	Unk_09(void);
	virtual void	Unk_0A(void);
	virtual void	Unk_0B(void);
	virtual void	Unk_0C(void);
	virtual void	Unk_0D(void);
	virtual void	Unk_0E(void);
	virtual void	Unk_0F(void);
	virtual void	Unk_10(void);
	virtual void	Unk_11(void);
	virtual void	Unk_12(void);
	virtual void	Unk_13(void);

	UInt8	unk04;									// 04 Perhaps refcount?
	UInt8	pad05[3];
	UInt32	visibleArray; 							// 08 NiVisibleArray *
	UInt32	camera;   								// 0C NiCamera *
	UInt32	fustrum[(0x30 - 0x10) >> 2];			// 10 NiFustrum
	UInt32	fustrumPlanes[(0x90 - 0x30) >> 2];		// 2C NiFrustumPlanes
	UInt32	activePlanes;							// 90
	UInt32	unk94;									// 94
	UInt32	unk98;									// 98
	UInt32	fustrumPlanes2[(0x100 - 0x9C) >> 2];	// 9C NiFrustumPlanes
	UInt32	activePlanes2;							// 100
	UInt32	unk104;									// 104
	UInt32	unk108;									// 108
	UInt32	unk10C;									// 10C
};

STATIC_ASSERT(sizeof(NiCullingProcess) == 0x110);

// 170
class BSCullingProcess : public NiCullingProcess
{
public:
	virtual ~BSCullingProcess();
	virtual void	Unk_14(void);
	virtual void	Unk_15(void);
	virtual void	Unk_16(void);
	virtual void	Unk_17(void);
	virtual void	Unk_18(void);

	UInt32	unk110[(0x170 - 0x110) >> 2];
};

STATIC_ASSERT(sizeof(BSCullingProcess) == 0x170);

// 238
class LocalMapCullingProcess : public BSCullingProcess
{
public:
	LocalMapCamera	localMapCamera;					// 170
	void			* shaderAccumulator;			// 1BC
	NiRenderTarget	* localMapTexture;				// 1C0
	UInt32			unk1C4[(0x230 - 0x1C4) >> 2];	// 1C4
	UInt32			width;							// 230
	UInt32			height;							// 234
	NiNode			* niNode;						// 238
};

STATIC_ASSERT(offsetof(LocalMapCullingProcess, localMapCamera) == 0x170);
STATIC_ASSERT(offsetof(LocalMapCullingProcess, niNode) == 0x238);