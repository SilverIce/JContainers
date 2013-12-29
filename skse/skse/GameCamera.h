#pragma once

#include "GameTypes.h"
#include "GameInput.h"

#include "skse/NiObjects.h"
#include "skse/NiTypes.h"

class TESCamera;
class NiNode;

class TESCameraState
{
public:
	TESCameraState();
	virtual ~TESCameraState();

	virtual void Unk_01();
	virtual void Unk_02();
	virtual void Unk_03();
	virtual void Unk_04();
	virtual void Unk_05();
	virtual void Unk_06();
	virtual void Unk_07();
	virtual void Unk_08();

	BSIntrusiveRefCounted	refCount;		// 04
	TESCamera				* camera;		// 08
	UInt32					stateId;		// 0C
};

class DragonCameraState : public TESCameraState
{
public:
	DragonCameraState();
	virtual ~DragonCameraState();

	PlayerInputHandler		inputHandler;		// 10
	NiNode					* cameraNode;		// 18
	NiNode					* controllerNode;	// 1C
	float					unk20[0x03];		// 20
	UInt32					unk2C[0x07];		// 2C
	float					unk48[0x03];		// 48
	UInt32					unk54[0x11];		// 54
	float					unk98[0x03];		// 98
	UInt32					unkA4[0x04];		// A4
	UInt8					unkB4[0x07];		// B4
	UInt8					padBB;
	// More
};

class HorseCameraState : public TESCameraState
{
public:
	HorseCameraState();
	virtual ~HorseCameraState();

	PlayerInputHandler		inputHandler;		// 10
	NiNode					* cameraNode;		// 18
	NiNode					* controllerNode;	// 1C
	float					unk20[0x03];		// 20
	UInt32					unk2C[0x07];		// 2C
	float					unk48[0x03];		// 48
	UInt32					unk54[0x11];		// 54
	float					unk98[0x03];		// 98
	UInt32					unkA4[0x04];		// A4
	UInt8					unkB4[0x07];		// B4
	UInt8					padBB;
	// More
};

class TweenMenuCameraState : public TESCameraState
{
public:
	TweenMenuCameraState();
	virtual ~TweenMenuCameraState();

	UInt32	unk10[0x0B];	// 10
	UInt8	unk3C;			// 3C
	UInt8	pad3D;			// 3D
	UInt16	pad3E;			// 3E
};

class VATSCameraState : public TESCameraState
{
public:
	VATSCameraState();
	virtual ~VATSCameraState();

	UInt32	unk10[0x03];	// 10
};

class FreeCameraState : public TESCameraState
{
public:
	FreeCameraState();
	virtual ~FreeCameraState();

	PlayerInputHandler		inputHandler;	// 10
	float					unk18;			// 18
	float					unk1C;			// 1C
	float					unk20;			// 20
	UInt32					unk24[0x04];	// 24
	UInt16					unk34;			// 34
	UInt8					unk36;			// 36
	UInt8					unk37;			// 37
};

class AutoVanityState : public TESCameraState
{
public:
	AutoVanityState();
	virtual ~AutoVanityState();
};

class FurnitureCameraState : public TESCameraState
{
public:
	FurnitureCameraState();
	virtual ~FurnitureCameraState();

	UInt32	unk10;	// 10
	float	unk14;	// 14
	float	unk18;	// 18
	float	unk1C;	// 1C
	UInt32	unk20;	// 20
	UInt32	unk24;	// 24
	UInt32	unk28;	// 28
	UInt8	unk2C;	// 2C
	UInt8	unk2D;	// 2D
	UInt8	unk2E;	// 2E
	UInt8	pad2F;	// 2F
};

class IronSightsState : public TESCameraState
{
public:
	IronSightsState();
	virtual ~IronSightsState();

	UInt32	unk10;	// 10
};

class FirstPersonState : public TESCameraState
{
public:
	FirstPersonState();
	virtual ~FirstPersonState();

	PlayerInputHandler		inputHandler;		// 10
	NiNode					* cameraNode;		// 18
	NiNode					* controllerNode;	// 1C
	UInt32					unk20;				// 20
	float					unk24;				// 24
	float					unk28;				// 28
	UInt32					unk2C[0x03];		// 2C
	UInt8					unk38[0x05];		// 38
	UInt8					pad3D;				// 3D
	UInt16					pad3E;				// 3E
};

class ThirdPersonState : public TESCameraState
{
public:
	ThirdPersonState();
	virtual ~ThirdPersonState();
	virtual void Unk_09(void);
	virtual void Unk_0A(void);
	virtual void UpdateMode(bool weaponDrawn);

	PlayerInputHandler		inputHandler;		// 10
	NiNode					* cameraNode;		// 18
	NiNode					* controllerNode;	// 1C
	float					unk20[0x03];		// 20
	UInt32					unk2C[0x04];		// 2C
	float					fOverShoulderPosX;	// 3C
	float					fOverShoulderCombatAddY;	// 40
	float					fOverShoulderPosZ;	// 44
	float					unk48[0x03];		// 48
	UInt32					unk54[0x11];		// 54
	float					unk98[0x03];		// 98
	UInt32					unkA4[0x04];		// A4
	UInt8					unkB4[0x07];		// B4
	UInt8					padBB;
};

STATIC_ASSERT(offsetof(ThirdPersonState, fOverShoulderPosX) == 0x3C);
STATIC_ASSERT(offsetof(ThirdPersonState, unk48) == 0x48);

class LocalMapCameraState : public TESCameraState
{
public:
	NiPoint3	unk10;	// 10
	NiPoint3	unk1C;	// 1C
	UInt32		unk28;	// 28
	float		minFrustumWidth;
	float		minFrustumHeight;
};

class BleedoutCameraState : public ThirdPersonState
{
public:
	BleedoutCameraState();
	virtual ~BleedoutCameraState();

	PlayerInputHandler		inputHandler;		// 10
	NiNode					* cameraNode;		// 18
	NiNode					* controllerNode;	// 1C
	float					unk20[0x03];		// 20
	UInt32					unk2C[0x07];		// 2C
	float					unk48[0x03];		// 48
	UInt32					unk54[0x11];		// 54
	float					unk98[0x03];		// 98
	UInt32					unkA4[0x04];		// A4
	UInt8					unkB4[0x07];		// B4
	UInt8					padBB;
	// More
};

class PlayerCameraTransitionState : public TESCameraState
{
public:
	PlayerCameraTransitionState();
	virtual ~PlayerCameraTransitionState();

	UInt32	unk10[0x05];	// 10
	UInt8	unk24;			// 24
	UInt8	unk25;			// 25
	UInt16	pad26;
};

class TESCamera
{
public:
	TESCamera();
	virtual ~TESCamera();

	virtual void Unk_01();
	virtual void Unk_02();

	UInt32			unk04[0x06];	// 04
	NiNode			* niNode;		// 1C
	TESCameraState	* cameraState;	// 20
	UInt8			unk24;	// 24
	UInt8			pad25[3];	// 25

	MEMBER_FN_PREFIX(TESCamera);
	DEFINE_MEMBER_FN(SetCameraState, UInt32, 0x006533D0, TESCameraState * cameraState);
};

STATIC_ASSERT(offsetof(TESCamera, niNode) == 0x1C);

class LocalMapCamera : public TESCamera
{
public:
	LocalMapCamera();
	virtual ~LocalMapCamera();

	NiPoint3	areaBoundsMin;	// 28
	NiPoint3	areaBoundsMax;	// 34
	LocalMapCameraState	* defaultState;	// 40
	NiObject	* niCamera;		// 44
	float		northRotation;	// 48
};

STATIC_ASSERT(offsetof(LocalMapCamera, northRotation) == 0x48);

class MapCamera : public TESCamera
{
public:
	MapCamera();
	virtual ~MapCamera();
};

class RaceSexCamera : public TESCamera
{
public:
	RaceSexCamera();
	virtual ~RaceSexCamera();
};

class PlayerCamera : public TESCamera
{
public:
	PlayerCamera();
	virtual ~PlayerCamera();

	static PlayerCamera *	GetSingleton(void)
	{
		return *((PlayerCamera **)0x012E7288);
	}

	enum
	{
		kCameraState_FirstPerson = 0,
		kCameraState_AutoVanity,
		kCameraState_VATS,
		kCameraState_Free,
		kCameraState_IronSights,
		kCameraState_Furniture,
		kCameraState_Transition,
		kCameraState_TweenMenu,
		kCameraState_ThirdPerson1,
		kCameraState_ThirdPerson2,
		kCameraState_Horse,
		kCameraState_Bleedout,
		kCameraState_Dragon,
		kNumCameraStates
	};

	UInt32 unk34[(0x6C - 0x28) >> 2];					// 34
	TESCameraState * cameraStates[kNumCameraStates];	// 6C
	UInt32	unkA0;										// A0
	UInt32	unkA4;										// A4
	UInt32	unkA8;										// A8
	float	worldFOV;									// AC
	float	firstPersonFOV;								// B0
	UInt32	unkB4[(0xD0 - 0xB4) >> 2];					// B4
	UInt8	unkD0;										// D0
	UInt8	unkD1;										// D1
	UInt8	unkD2;										// D2
	UInt8	unkD3;										// D3
	UInt8	unkD4;										// D4
	UInt8	unkD5;										// D5
	UInt8	padD6[2];									// D6

	MEMBER_FN_PREFIX(PlayerCamera);
	DEFINE_MEMBER_FN(UpdateThirdPerson, void, 0x0083C7E0, bool weaponDrawn);
};

STATIC_ASSERT(offsetof(PlayerCamera, cameraStates) == 0x6C);
STATIC_ASSERT(offsetof(PlayerCamera, padD6) == 0xD6);