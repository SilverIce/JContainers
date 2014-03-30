#pragma once

#include "skse/NiTypes.h"
#include "skse/NiObjects.h"
#include "skse/GameTypes.h"

class BSFaceGenKeyframe
{
public:
	virtual ~BSFaceGenKeyframe();

	virtual void Unk_01(void); // pure
	virtual void Unk_02(void); // pure
	virtual void Unk_03(void); // pure
	virtual void Unk_04(void); // pure
	virtual void Unk_05(void); // pure
	virtual void Unk_06(void); // pure
	virtual void Unk_07(void); // pure
	virtual void Unk_08(void); // pure
	virtual void Unk_09(void); // pure
	virtual void Unk_0A(void); // pure
	virtual void Unk_0B(void); // pure
	virtual void Unk_0C(void);
	virtual void Unk_0D(void);
};

class BSFaceGenKeyframeMultiple : public BSFaceGenKeyframe
{
public:
	UInt32	unk04;	// 04
	UInt32	unk08;	// 08
	UnkArray	unk0C;	// 0C
};
STATIC_ASSERT(sizeof(BSFaceGenKeyframeMultiple) == 0x18);

class NiExtraData : public NiObject
{
public:
	const char *	unk08;	// 08
};

// 1B4
class BSFaceGenAnimationData : public NiExtraData
{
public:
	enum {
		kNumKeyframes = 12
	};
	void	* unk08;						// 08
	UInt32	unk0C;							// 0C
	BSFaceGenKeyframeMultiple	keyFrames[kNumKeyframes];	// 10
	UInt32	unk1A0[(0x1A0 - 0x130) >> 2];	// 1A0
	UInt8	unk1A4;							// 1A4
	UInt8	unk1A5;							// 1A5
	UInt8	overrideFlag;					// 1A6
	UInt8	unk1A7;							// 1A7
	UInt32	unk1A8[(0x1B4 - 0x1A8) >> 2];	// 1A8

	MEMBER_FN_PREFIX(BSFaceGenAnimationData);
	DEFINE_MEMBER_FN(SetExpression, void, 0x0059DB90, UInt32 type, float value);
	DEFINE_MEMBER_FN(SetPhonome, void, 0x005352D0, UInt32 type, float value);
	DEFINE_MEMBER_FN(SetModifier, void, 0x005352A0, UInt32 type, float value);
	DEFINE_MEMBER_FN(SetCustom, void, 0x00535300, UInt32 type, float value);
	DEFINE_MEMBER_FN(Reset, void, 0x0059E320, float value, UInt8 unk1, UInt8 unk2, UInt8 unk3, UInt8 unk4);
	
};

STATIC_ASSERT(offsetof(BSFaceGenAnimationData, overrideFlag) == 0x1A6);
STATIC_ASSERT(sizeof(BSFaceGenAnimationData) == 0x1B4);
