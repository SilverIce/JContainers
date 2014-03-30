#include "ScaleformLoader.h"
#include "Translation.h"

UInt32 GFxLoader::ctor_Hook(void)
{
	UInt32 result = CALL_MEMBER_FN(this, ctor)();

	// Read plugin list, load translation files
	Translation::ImportTranslationFiles(stateBag->GetTranslator());

	return result;
}

GFxLoader * GFxLoader::GetSingleton()
{
	return *((GFxLoader **)0x01B2E9B0);
}