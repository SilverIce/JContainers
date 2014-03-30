#include "GameMenus.h"

const _CreateUIMessageData CreateUIMessageData = (_CreateUIMessageData)0x00547A00;

bool MenuManager::IsMenuOpen(BSFixedString * menuName)
{
	return CALL_MEMBER_FN(this, IsMenuOpen)(menuName);
}

GFxMovieView * MenuManager::GetMovieView(BSFixedString * menuName)
{
	IMenu * menu = GetMenu(menuName);
	if (!menu)
		return NULL;

	return menu->view;
}

IMenu * MenuManager::GetMenu(BSFixedString * menuName)
{
	if (!menuName->data)
		return NULL;

	MenuTableItem * item = menuTable.Find(menuName);

	if (!item)
		return NULL;

	IMenu * menu = item->menuInstance;
	if(!menu)
		return NULL;

	return menu;
}

RaceMenuSlider::RaceMenuSlider(UInt32 filterFlag, const char * sliderName, const char * callbackName, UInt32 sliderId, UInt32 index, UInt32 type, UInt8 unk8, float min, float max, float value, float interval, UInt32 unk13)
{
	CALL_MEMBER_FN(this, Construct)(filterFlag, sliderName, callbackName, sliderId, index, type, unk8, min, max, value, interval, unk13);
}