#include "SafeWrite.h"
#include "Utilities.h"

class NiDX9Renderer
{
	NiDX9Renderer();
	~NiDX9Renderer();
};

UInt32 g_tintTextureResolution = 256;

typedef void * (__stdcall * _ClipTextureDimensions)(NiDX9Renderer * a1, UInt32 a2, UInt32 * a3, UInt32 * a4, UInt32 * a5, UInt32 * a6, UInt32 * a7, UInt32 * a8, UInt32 * a9, UInt32 * a10);
_ClipTextureDimensions ClipTextureDimensions = (_ClipTextureDimensions)0x00C8FE10;

void * __stdcall ClipTextureDimensions_Hook(NiDX9Renderer * renderer, UInt32 type, UInt32 * height, UInt32 * width, UInt32 * a5, UInt32 * a6, UInt32 * a7, UInt32 * a8, UInt32 * a9, UInt32 * a10)
{
	void * result = ClipTextureDimensions(renderer, type, height, width, a5, a6, a7, a8, a9, a10);
	switch(type) {
		case 32:
			{
				*height = g_tintTextureResolution;
				*width = g_tintTextureResolution;
			}
			break;
	}
	return result;
}

void Hooks_DX9Renderer_Init(void)
{
	UInt32	tintTextureResolution = 0;
	if(GetConfigOption_UInt32("Display", "iTintTextureResolution", &tintTextureResolution))
	{
		g_tintTextureResolution = tintTextureResolution;
	}
}

void Hooks_DX9Renderer_Commit(void)
{
	WriteRelCall(0x00C90B8C, (UInt32)ClipTextureDimensions_Hook);
	WriteRelCall(0x00C916CC, (UInt32)ClipTextureDimensions_Hook);
}
