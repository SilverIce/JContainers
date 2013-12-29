#pragma once

#include "GameTypes.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "GameReferences.h"

class BSFile;

struct FormRecordData
{
	UInt8		typeID;		// corresponds to kFormType_XXX
	UInt32		typeCode;	// i.e. 'GMST', 'FACT'
	UInt32		unk08;		// only seen zero
};

struct ChunkHeader
{
	UInt32	type : 4;	// i.e. 'XGRD', 'DATA'
	UInt16	size : 2;
};

struct ModInfo		// referred to by game as TESFile
{
	ModInfo();
	~ModInfo();

	// 18 info about currently loading form
	struct FormInfo
	{
		UInt32		recordType;			// 00 i.e. 'FACT', 'GMST'
		UInt32		unk04;				// 04 looks like size of entire record
		UInt32		formFlags;			// 08 copied to TESForm->flags
		UInt32		formID;				// 0C 
		UInt32		unk10;				// 10
		UInt16		unk14;				// 14 always initialized to 0F on SaveForm. 
		UInt16		unk16;
	};

	tList<UInt32>						unkList;			// 000
	UInt32 /*NiTPointerMap<TESFile*>*/	* pointerMap;		// 008
	UInt32								unk00C;				// 00C
	BSFile*								unkFile;			// 010
	UInt32								unk014;				// 014 
	void								* unk018;			// 018 seen all zeroes. size unknown
	void								* unk01C;			// 01C as above
	char								name[0x104];		// 020
	char								filepath[0x104];	// 124
	UInt32								unk228;				// 228
	UInt32								unk22C;				// init'd to dword_F469CC (0x2800) same val as BSFile+10?
	UInt32								unk230;				// 230
	UInt32								unk234;				// 234
	UInt32								unk238;				// 238
	UInt32								unk23C;				// 23C
	FormInfo							formInfo;			// 240
	ChunkHeader							subRecord;			// 258
	UInt32								unk260;				// 260
	UInt32								fileOffset;			// 264
	UInt32								dataOffset;			// 268 index into dataBuf
	UInt32								subrecordBytesRead;	// 26C generates error on Read if != expected length
	UInt32						unk268[(0x298-0x270) >> 2];	// 270
	UInt8								unk298;				// 298
	UInt8								bIsBigEndian;		// 299
	UInt8								unk29A;				// 29A
	UInt8								pad29B;
	WIN32_FIND_DATA						fileData;			// 29C
	float								unk3DC;				// 3DC init'd to 0.94
	UInt32								unk3E0;				// 3E0
	UInt32								flags;				// 3E4 init'd to 0x00000800. 4000 and 40000 do stuff
	UInt8								unk3E8;				// 3E8
	UInt8								pad3E9[3];
	UInt32								unk3EC;				// 3EC
	UInt32								unk3F0;				// 3F0
	UInt32								unk3F4;				// 3F4
	UInt32								unk3F8;				// 3F8
	UInt32								numRefMods;			// 3FC related to modindex; see 4472D0
																// formIDs in mod are as saved in GECK, must fix up at runtime
	ModInfo								** refModInfo;		// 400 used to look up modInfo based on fixed mod index, double-check
	UInt32								unk404;				// 404
	UInt32								unk408;				// 408
	UInt8								modIndex;			// 40C init to 0xFF
	UInt8								pad40D[3];
	BSString							author;				// 410
	BSString							description;		// 418
	void								* dataBuf;			// 420 
	UInt32								unk424;				// 424 looks like size of entire record
	UInt8								unk428;				// 428
	UInt8								pad429[3];
	
	bool IsLoaded() const { return true; }
};

struct ModList
{
	tList<ModInfo>		modInfoList;
	UInt32				loadedModCount;
	ModInfo*			loadedMods[0xFF];
};

class DataHandler
{
public:
	static DataHandler* GetSingleton();

	// loads of tArrays of object types, at least a good number in formType order
	UInt32						unk000;
	UInt32						unk004;
	UnkFormArray				unk008;
	UnkFormArray				unk014;
	UnkFormArray				unk020;
	UnkFormArray				unk02C;
	tArray<BGSKeyword*>			keywords;
	tArray<BGSLocationRefType*>	locRefTypes;
	tArray<BGSAction*>			actions;
	tArray<BGSTextureSet*>		textureSets;
	tArray<BGSMenuIcon*>			menuIcons;
	tArray<TESGlobal*>			globals;
	tArray<TESClass*>			classes;
	tArray<TESFaction*>			factions;
	tArray<BGSHeadPart*>			headParts;
	tArray<TESEyes*>				eyes;
	tArray<TESRace*>				races;
	tArray<TESSound*>			sounds;
	tArray<BGSAcousticSpace*>	acousticSpaces;
	UnkFormArray				unkSkills;
	tArray<EffectSetting*>		magicEffects;
	tArray<Script*>				scripts;
	tArray<TESLandTexture*>		landTextures;
	tArray<EnchantmentItem*>	enchantments;
	tArray<SpellItem*>			spellItems;
	tArray<ScrollItem*>			scrolls;
	tArray<TESObjectACTI*>		activators;
	tArray<BGSTalkingActivator*>	talkingActivators;
	tArray<TESObjectARMO*>		armors;
	tArray<TESObjectBOOK*>		books;
	tArray<TESObjectCONT*>		containers;
	tArray<TESObjectDOOR*>		doors;
	tArray<IngredientItem*>		ingredients;
	tArray<TESObjectLIGH*>		lights;
	tArray<TESObjectMISC*>		miscObjects;
	tArray<BGSApparatus*>		apparatuses;
	tArray<TESObjectSTAT*>		statics;
	tArray<BGSStaticCollection*>	staticCollections;
	tArray<BGSMovableStatic*>	movableStatics;
	tArray<TESGrass*>			grasses;
	tArray<TESObjectTREE*>		trees;
	tArray<TESFlora*>			flora;
	tArray<TESFurniture*>		furniture;
	tArray<TESObjectWEAP*>		weapons;
	tArray<TESAmmo*>				ammo;
	tArray<TESNPC*>				npcs;
	tArray<TESLevCharacter*>		levCharacters;
	tArray<TESKey*>				keys;
	tArray<AlchemyItem*>			potions;
	tArray<BGSIdleMarker*>		idleMarkers;
	tArray<BGSNote*>				notes;
	tArray<BGSConstructibleObject*> constructibles;
	tArray<BGSProjectile*>		projectiles;
	tArray<BGSHazard*>			bgsHazards;
	tArray<TESSoulGem*>			soulGems;
	tArray<TESLevItem*>			levItems;
	tArray<TESWeather*>			weather;
	tArray<TESClimate*>			climates;
	tArray<BGSShaderParticleGeometryData*>	shaderParticleGeometryData;
	tArray<BGSReferenceEffect*>	referenceEffects;
	tArray<TESRegion*>			regions;
	tArray<NavMeshInfoMap*>		navMeshInfoMaps;
	tArray<TESObjectCELL*>		cells;
	tArray<TESObjectREFR*>		refs;	// could be actors
	tArray<Character*>			characters;
	tArray<MissileProjectile*>	missleProjs;
	tArray<ArrowProjectile*>		arrowProjs;
	tArray<GrenadeProjectile*>	grenadeProjs;
	tArray<BeamProjectile*>		beamProjs;
	tArray<FlameProjectile*>		flameProjs;
	tArray<ConeProjectile*>		coneProjs;
	tArray<BarrierProjectile*>	barrierProjs;
	tArray<Hazard*>				hazards;
	tArray<TESWorldSpace*>		worldSpaces;
	tArray<TESObjectLAND*>		lands;
	tArray<NavMesh*>				navMeshes;
	UnkFormArray				unkTLOD;
	tArray<TESTopic*>			topics;
	tArray<TESTopicInfo*>		topicInfos;
	tArray<TESQuest*>			quests;
	tArray<TESIdleForm*>			idleForms;
	tArray<TESPackage*>			packages;
	tArray<TESCombatStyle*>		combatStyles;
	tArray<TESLoadScreen*>		loadScreens;
	tArray<TESLevSpell*>			levSpells;
	tArray<TESObjectANIO*>		anios;
	tArray<TESWaterForm*>		waterForms;
	tArray<TESEffectShader*>		effectShaders;
	UnkFormArray				unkTOFTs;
	tArray<BGSExplosion*>		explosions;
	tArray<BGSDebris*>			debris;
	tArray<TESImageSpace*>		imageSpaces;
	tArray<TESImageSpaceModifier*>	imageSpaceModifiers;
	tArray<BGSListForm*>			listForms;
	tArray<BGSPerk*>				perks;
	tArray<BGSBodyPartData*>		bodyPartData;
	tArray<BGSAddonNode*>		addonNodes;
	tArray<ActorValueInfo*>		actorValueInfos;
	tArray<BGSCameraShot*>		cameraShots;
	tArray<BGSCameraPath*>		cameraPaths;
	tArray<BGSVoiceType*>		voiceTypes;
	tArray<BGSMaterialType*>		materialTypes;
	tArray<BGSImpactData*>		impactData;
	tArray<BGSImpactDataSet*>	impactDataSets;
	tArray<TESObjectARMA*>		armorAddons;
	tArray<BGSEncounterZone*>	encounterZones;
	tArray<BGSLocation*>			locations;
	tArray<BGSMessage*>			messages;
	tArray<BGSRagdoll*>			ragdolls;
	UnkFormArray				unkDOBJs;
	tArray<BGSLightingTemplate*>	lightingTemplates;
	tArray<BGSMusicType*>		musicTypes;
	tArray<BGSFootstep*>			footsteps;
	tArray<BGSFootstepSet*>		footstepSets;
	tArray<BGSStoryManagerBranchNode*>	branchNodes;
	tArray<BGSStoryManagerQuestNode*>	questNodes;
	tArray<BGSStoryManagerEventNode*>	eventNodes;
	tArray<BGSDialogueBranch*>	dialogBranches;
	tArray<BGSMusicTrackFormWrapper*>	musicTrackFormWrappers;
	UnkFormArray				unkDLVWs;
	tArray<TESWordOfPower*>		wordOfPowers;
	tArray<TESShout*>			shouts;
	tArray<BGSEquipSlot*>			equipSlots;
	tArray<BGSRelationship*>		relationships;
	tArray<BGSScene*>			scenes;
	tArray<BGSAssociationType*>	associationTypes;
	tArray<BGSOutfit*>			outfits;
	tArray<BGSArtObject*>		artObjects;
	tArray<BGSMaterialObject*>	materialObjects;
	tArray<BGSMovementType*>		movementTypes;
	tArray<BGSSoundDescriptorForm*>	soundDescriptors;
	tArray<BGSDualCastData*>		dualCastData;
	tArray<BGSSoundCategory*>	soundCategories;
	tArray<BGSSoundOutput*>		soundOutputs;
	tArray<BGSCollisionLayer*>	collisonLayers;
	tArray<BGSColorForm*>		colors;
	tArray<BGSReverbParameters*>	reverbParams;
	UInt32 unks[0x0E]; // 03 Cell** 06 TESGlobal**
	ModList							modList;
	UInt32 moreunks[100];

	const ModInfo* LookupModByName(const char* modName);
	UInt8 GetModIndex(const char* modName);
};

// 58
class BGSSaveLoadManager
{
public:
	enum
	{
		kEvent_Autosave =	1 << 0,
		kEvent_Save =		1 << 1,
		kEvent_Unk02 =		1 << 2,
		kEvent_Unk03 =		1 << 3,
		kEvent_Unk04 =		1 << 4,

		kEvent_Unk07 =		1 << 7,
	};

	static BGSSaveLoadManager *	GetSingleton(void);

	void	Save(const char * name);
	void	Load(const char * name);

	// used by Hooks_SaveLoad
	void	SaveGame_Hook(const char * saveName);
	bool	LoadGame_Hook(const char * saveName, bool unk1);
	void	ProcessEvents_Hook(void);

	// use these when calling from a papyrus thread
	void	RequestSave(const char * name);
	void	RequestLoad(const char * name);

	MEMBER_FN_PREFIX(BGSSaveLoadManager);

	tList<const char*>	* saveList;			// 00
	UInt32				unk04;				// 04
	UInt32				unk08;				// 08
	UInt32				unk0C;				// 0C
	UInt8				unk10;				// 10
	UInt8				unk11;				// 11
	UInt8				pad12[2];			// 12
	UInt32				pendingEvents;		// 14
	UInt32				unk18;				// 18
	UInt32				startTickCount;		// 1C - GetTickCount when constructed
	UInt8				unk20;				// 20 - init'd to 0x01
	UInt8				pad21[3];			// 21
	UInt32				unk24;				// 24
	UInt32				unk28;				// 28 - init'd to 0xFFFFFFFF
	bool				unk2C;				// 2C
	UInt8				pad2E[3];			// 2E
	void				* unk30;			// 30
	UInt8				unk34;				// 34 - init'd to 0x01
	UInt8				unk35;				// 35
	UInt8				pad36[2];			// 36
	UInt32				unk38;				// 38
	UInt32				unk3C;				// 3C
	UInt32				unk40;				// 40
	UInt32				unk44;				// 44
	UInt32				unk48;				// 48
	UInt32				unk4C;				// 4C
	UInt32				unk50;				// 50
	void				* unk54;			// 54

private:
	DEFINE_MEMBER_FN(Save_Internal, bool, 0x006814D0, const char * name, int unk1, UInt32 unk2);
	DEFINE_MEMBER_FN(Load_Internal, bool, 0x006821C0, const char * name, int unk1, UInt32 unk2, UInt32 unk3);

	DEFINE_MEMBER_FN(SaveGame_HookTarget, void, 0x00679200, const char * fileName);
	DEFINE_MEMBER_FN(LoadGame_HookTarget, bool, 0x0067B720, const char * fileName, bool unk0);

	DEFINE_MEMBER_FN(ProcessEvents_Internal, void, 0x00682400);
};

STATIC_ASSERT(sizeof(BGSSaveLoadManager) == 0x58);

class MiscStatManager
{
public:
	static MiscStatManager *	GetSingleton(void);

	// 14
	struct MiscStat
	{
		const char	* name;		// 00
		const char	* unk04;	// 04
		UInt32		value;		// 08
		UInt32		unk0C;		// 0C
		UInt8		unk10;		// 10
		UInt8		pad11[3];	// 11
	};

	class Visitor
	{
	public:
		virtual void	Visit(MiscStat * stat, void * stat_unk04, UInt32 stat_unk0C, UInt32 value, UInt32 stat_unk10) = 0;
	};

	MEMBER_FN_PREFIX(MiscStatManager);
	DEFINE_MEMBER_FN(Visit, void, 0x00488120, Visitor ** visitor);

	MiscStat	* m_stats;	// 00
	UInt32		unk04;		// 04
	UInt32		m_numStats;	// 08

	MiscStat *	Get(const char * name);
};
 
class EquipManager
{
public:
	virtual ~EquipManager();
 
	static EquipManager *   GetSingleton(void);

	MEMBER_FN_PREFIX(EquipManager);
	DEFINE_MEMBER_FN(EquipItem, void, 0x006EF3E0, Actor * actor, TESForm * item, BaseExtraList * extraData, SInt32 count, BGSEquipSlot * equipSlot, bool withEquipSound, bool preventUnequip, bool showMsg, void * unk);
	DEFINE_MEMBER_FN(UnequipItem, bool, 0x006EE560, Actor * actor, TESForm * item, BaseExtraList * extraData, SInt32 count, BGSEquipSlot * equipSlot, bool unkFlag1 , bool preventEquip, bool unkFlag2, bool unkFlag3, void * unk);
};

typedef BGSEquipSlot * (* _GetEitherHandSlot)();
extern const _GetEitherHandSlot GetEitherHandSlot;
 
typedef BGSEquipSlot * (* _GetRightHandSlot)();
extern const _GetRightHandSlot GetRightHandSlot;
 
typedef BGSEquipSlot * (* _GetLeftHandSlot)();
extern const _GetLeftHandSlot GetLeftHandSlot;

typedef UInt32 (* _LookupActorValueByName)(const char * name);
extern const _LookupActorValueByName LookupActorValueByName;

class ActorValueList
{
public:
	enum {
		kNumActorValues = 164
	};

	static ActorValueList * GetSingleton(void);
	ActorValueInfo * GetActorValue(UInt32 id);

private:
	UInt32 unk04;
	ActorValueInfo * actorValues[kNumActorValues];
};

class FaceMorphList
{
public:
	enum {
		kNumMorphs = 19
	};

	enum {
		kMorph_NoseShortLong = 0,
		kMorph_NoseDownUp,
		kMorph_JawUpDown,
		kMorph_JawNarrowWide,
		kMorph_JawBackForward,
		kMorph_CheeksDownUp,
		kMorph_CheeksInOut,
		kMorph_EyesMoveDownUp,
		kMorph_BrowDownUp,
		kMorph_BrowInOut,
		kMorph_BrowBackForward,
		kMorph_LipMoveDownUp,
		kMorph_LipMoveInOut,
		kMorph_ChinThinWide,
		kMorph_ChinMoveUpDown,
		kMorph_OverbiteUnderbite,
		kMorph_EyesBackForward
	};

	static FaceMorphList * GetSingleton(void);

	struct Morph
	{
		UInt32 type;
		const char * lowerName;
		const char * upperName;
	};

	Morph morphs[kNumMorphs];
};

class FacePresetData
{
public:
	virtual ~FacePresetData();

	UInt32 unk08;	// Always 10?
	const char * gameSettingName;
};

class FacePresetList
{
public:
	enum {
		kNumPresets = 4
	};
	enum {
		kPreset_NoseType,
		kPreset_BrowType,
		kPreset_EyesType,
		kPreset_LipType
	};

	static FacePresetList * GetSingleton(void);

	struct Preset
	{
		const char * presetName;
		FacePresetData * data;
	};

	Preset presets[kNumPresets];
};

// 0x00882290 RaceMenu ctor
// 0x0087F6E0 Morph Callback Handler
// 0x005A4870 Apply Morph?
// 0x005610F0 GetMorphName by Index and value
// 0x00561180 SetMorph?

class FaceGen
{
public:
	static FaceGen *	GetSingleton(void);

	struct Action {
		BSFixedString name;
		UInt32	unk04;
		float	delta;
	};

	UInt32	unk00[0x3C >> 2];	// 00
	UInt8	unk3C;				// 3C
	UInt8	pad3D[3];			// 3D

	MEMBER_FN_PREFIX(FaceGen);
	DEFINE_MEMBER_FN(RegenerateHead, void, 0x005A4B80, BSFaceGenNiNode * headNode, BGSHeadPart * head, TESNPC * npc);
	DEFINE_MEMBER_FN(ApplyMorph, void, 0x005A4070, BSFaceGenNiNode * faceGenNode, BGSHeadPart * headPart, BSFixedString * morphName, float relative);
};
STATIC_ASSERT(offsetof(FaceGen, unk3C) == 0x3C);

// Changes one HeadPart to another
typedef void (* _ChangeActorHeadPart)(Actor*, BGSHeadPart* oldPart, BGSHeadPart* newPart);
extern const _ChangeActorHeadPart ChangeActorHeadPart;

// Regenerates dynamic tints
typedef UInt32 (* _UpdatePlayerTints)();
extern const _UpdatePlayerTints UpdatePlayerTints;

typedef BGSHeadPart ** (* _GetActorBaseOverlays)(TESNPC * npc);
extern const _GetActorBaseOverlays GetActorBaseOverlays;

typedef UInt32 (* _GetNumActorBaseOverlays)(TESNPC * npc);
extern const _GetNumActorBaseOverlays GetNumActorBaseOverlays;

typedef bool (* _ApplyMasksToRenderTarget)(tArray<TintMask*> * tintMask, NiRenderTarget ** renderTarget);
extern const _ApplyMasksToRenderTarget ApplyMasksToRenderTarget;

// Loads a TRI file into the FaceGenDB, parameters are unknown ptrs
// unk1 seems to be inited to zero before calling however
// unk2 is a numeric value from some other object it seems
// making it zero seems to cache anyway
typedef bool (* _CacheTRIFile)(const char * filePath, UInt32 * unk1, UInt32 * unk2);
extern const _CacheTRIFile CacheTRIFile;

// 20
class MagicFavorites
{
	//	void			** _vtbl;	// 00
	UInt32			unk004;		// 04
	UnkFormArray	spells;		// 08
	UnkFormArray	hotkeys;	// 14

public:
	virtual	~MagicFavorites();

	void		SetHotkey(TESForm * form, SInt8 idx);
	void		ClearHotkey(SInt8 idx);
	TESForm	*	GetSpell(SInt8 idx);
	bool		IsFavorited(TESForm * form);

	static MagicFavorites * GetSingleton(void)
	{
		return *((MagicFavorites **)0x01B2E39C);
	}
};
