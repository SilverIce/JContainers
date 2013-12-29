#pragma once

#include "skse/ScaleformTypes.h"
#include "skse/GameTypes.h"

class GFxState : public GRefCountBase
{
public:
	enum
	{
		kInterface_Unknown =		0x00,

		kInterface_Translator =		0x03,
		kInterface_ImageLoader =	0x05,
		kInterface_External =		0x09,
		kInterface_FileOpener =		0x0A,
		kInterface_ZlibSupport =	0x1E,
	};

	UInt32	interfaceType;	// 08
};

// 08
class TranslationTableItem
{
public:
	wchar_t	* key;
	wchar_t	* translation;

	TranslationTableItem(wchar_t * a_key, wchar_t * a_translation)
		: key(a_key), translation(a_translation) {}

	bool operator==(const TranslationTableItem & rhs) const	{ return key == rhs.key; }
	bool operator==(const wchar_t * a_key) const			{ return key == a_key; }
	operator const wchar_t *() const						{ return key; }

	static inline UInt32 GetHash(const wchar_t ** key)
	{
		UInt32 hash;
		CRC32_Calc4(&hash, (UInt32)*key);
		return hash;
	}

	void Dump(void)
	{
		_MESSAGE("\t\tkey: %S ", key);
		_MESSAGE("\t\ttranslation: %S", translation);
	}
};

typedef tHashSet<TranslationTableItem, const wchar_t *> TranslationTable;

// These are incomplete and only decoded to get the translation table yet.
class BSScaleformTranslator
{
	UInt32		unk_000;
	UInt32		unk_004;
	UInt32		unk_008;
	UInt32		unk_00C;
	UInt32		unk_010;

public:
	TranslationTable	translations;	// 014

	typedef void (__cdecl * _GetCachedString)(wchar_t ** pOut, wchar_t * bufIn);
	static const _GetCachedString GetCachedString;
};

class GFxStateBag
{
protected:
	virtual GFxStateBag *	GetStateBagImpl(void);

public:
	virtual				~GFxStateBag();
	virtual void		SetState(UInt32 type, void * ptr);
	virtual void *		GetStateAddRef(UInt32 type);
	//more

	inline BSScaleformTranslator *		GetTranslator()		{ return (BSScaleformTranslator*)GetStateAddRef(GFxState::kInterface_Translator); }
};
