#include "skse/NiRenderer.h"

NiDX9Renderer * NiDX9Renderer::GetSingleton()
{
	return *((NiDX9Renderer **)0x01BA76FC);
}

NiRenderManager * NiRenderManager::GetSingleton()
{
	return *((NiRenderManager **)0x01BA7490);
}