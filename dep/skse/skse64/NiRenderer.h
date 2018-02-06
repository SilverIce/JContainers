#pragma once

#include "skse64/NiObjects.h"
#include "skse64/NiTextures.h"

#include "skse64/GameTypes.h"

struct ID3D11Forwarder;
struct ID3D11DeviceContext4;
struct IDXGISwapChain4;
struct ID3D11RenderTargetView1;
struct ID3D11ShaderResourceView1;

// Unknown class name, No RTTI
class BSRenderManager
{
public:
	static BSRenderManager * GetSingleton();

	UInt64						unk00[0x48 >> 3];	// 00
	ID3D11Forwarder				* forwarder;		// 48
	ID3D11DeviceContext4		* context;			// 50
	UInt64						unk58;				// 58
	UInt64						unk60;				// 60
	UInt64						unk68;				// 68
	IDXGISwapChain4				* swapChain;		// 70
	UInt64						unk78;				// 78
	UInt64						unk80;				// 80
	ID3D11RenderTargetView1		* renderView;		// 88
	ID3D11ShaderResourceView1	* resourceView;		// 90

	// 1412F0390 - CreateRenderTargets

	MEMBER_FN_PREFIX(BSRenderManager);
	DEFINE_MEMBER_FN(CreateRenderTexture, NiTexture::RendererData *, 0x00D6D250, UInt32 width, UInt32 height);
};

extern RelocPtr <BSRenderManager> g_renderManager;

class BSShaderResourceManager
{
public:
	virtual ~BSShaderResourceManager();
};
extern RelocPtr <BSShaderResourceManager> g_shaderResourceManager;
