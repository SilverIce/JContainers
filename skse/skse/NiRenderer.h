#pragma once

#include "skse/NiObjects.h"

class NiRenderer : public NiObject
{
public:
	virtual ~NiRenderer();
};

class NiDX9Renderer : public NiRenderer
{
public:
	virtual ~NiDX9Renderer();

	static NiDX9Renderer * GetSingleton();
};

// Unknown class name, No RTTI
class NiRenderManager
{
public:
	static NiRenderManager * GetSingleton();

	MEMBER_FN_PREFIX(NiRenderManager);
	DEFINE_MEMBER_FN(CreateRenderTarget, NiRenderTarget *, 0x00C91800, NiDX9Renderer * dx9Renderer, UInt32 type, UInt32 unk1, UInt32 unk2);
};