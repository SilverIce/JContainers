#include "IdentifyEXE.h"
#include "Error.h"
#include "skse/skse_version.h"

static bool GetFileVersion(const char * path, VS_FIXEDFILEINFO * info)
{
	bool result = false;

	UInt32	versionSize = GetFileVersionInfoSize(path, NULL);
	if(!versionSize)
	{
		_ERROR("GetFileVersionInfoSize failed (%08X)", GetLastError());
		return false;
	}

	UInt8	* versionBuf = new UInt8[versionSize];
	if(versionBuf)
	{
		if(GetFileVersionInfo(path, NULL, versionSize, versionBuf))
		{
			VS_FIXEDFILEINFO	* retrievedInfo = NULL;
			UInt32				realVersionSize = sizeof(VS_FIXEDFILEINFO);

			if(VerQueryValue(versionBuf, "\\", (void **)&retrievedInfo, (PUINT)&realVersionSize) && retrievedInfo)
			{
				*info = *retrievedInfo;
				result = true;
			}
			else
			{
				_ERROR("VerQueryValue failed (%08X)", GetLastError());
			}
		}
		else
		{
			_ERROR("GetFileVersionInfo failed (%08X)", GetLastError());
		}

		delete [] versionBuf;
	}

	return result;
}

static bool GetFileVersionNumber(const char * path, UInt64 * out)
{
	VS_FIXEDFILEINFO	versionInfo;
	if(!GetFileVersion(path, &versionInfo))
	{
		return false;
	}

	_MESSAGE("dwSignature = %08X", versionInfo.dwSignature);
	_MESSAGE("dwStrucVersion = %08X", versionInfo.dwStrucVersion);
	_MESSAGE("dwFileVersionMS = %08X", versionInfo.dwFileVersionMS);
	_MESSAGE("dwFileVersionLS = %08X", versionInfo.dwFileVersionLS);
	_MESSAGE("dwProductVersionMS = %08X", versionInfo.dwProductVersionMS);
	_MESSAGE("dwProductVersionLS = %08X", versionInfo.dwProductVersionLS);
	_MESSAGE("dwFileFlagsMask = %08X", versionInfo.dwFileFlagsMask);
	_MESSAGE("dwFileFlags = %08X", versionInfo.dwFileFlags);
	_MESSAGE("dwFileOS = %08X", versionInfo.dwFileOS);
	_MESSAGE("dwFileType = %08X", versionInfo.dwFileType);
	_MESSAGE("dwFileSubtype = %08X", versionInfo.dwFileSubtype);
	_MESSAGE("dwFileDateMS = %08X", versionInfo.dwFileDateMS);
	_MESSAGE("dwFileDateLS = %08X", versionInfo.dwFileDateLS);

	UInt64 version = (((UInt64)versionInfo.dwFileVersionMS) << 32) | versionInfo.dwFileVersionLS;

	*out = version;

	return true;
}

const IMAGE_SECTION_HEADER * GetImageSection(const UInt8 * base, const char * name)
{
	const IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_SECTION_HEADER	* sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	for(UInt32 i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER	* section = &sectionHeader[i];

		if(!strcmp((const char *)section->Name, name))
		{
			return section;
		}
	}

	return NULL;
}

// steam EXE will have the .bind section
bool IsSteamImage(const UInt8 * base)
{
	return GetImageSection(base, ".bind") != NULL;
}

bool IsUPXImage(const UInt8 * base)
{
	return GetImageSection(base, "UPX0") != NULL;
}

struct DebugHeader
{
	char	sig[4];		// "RSDS"
	UInt8	guid[0x10];
	UInt32	age;
	// path follows

	bool SignatureValid(void) const
	{
		return memcmp(sig, "RSDS", 4) == 0;
	}

	void ReadInfo(ProcHookInfo * out, bool * probablyValid) const
	{
		*probablyValid = false;

		out->noGore = false;
		out->russian = false;

		if(SignatureValid())
		{
			const char	* path = (const char *)(this + 1);

			// path will start with <drive letter> colon backslash
			if(path[1] != ':') return;
			if(path[2] != '\\') return;

			// make sure the string isn't stupidly long and only contains printable characters
			for(UInt32 i = 0; i < 0x80; i++)
			{
				char	data = path[i];

				if(!data)
				{
					*probablyValid = strstr(path, ".pdb") != NULL;

					out->noGore = strstr(path, "TESVng.pdb") != NULL;	// ### not sure what this will be
					out->russian = strstr(path, "RussianCode") != NULL;
					out->japanese = strstr(path, "TESV_JP_1.1.21_PC") != NULL;

					break;
				}

				if((data < 0) || !isprint(data))
					break;
			}
		}
	}
};

const UInt8 * GetVirtualAddress(const UInt8 * base, UInt32 addr)
{
	const IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_SECTION_HEADER	* sectionHeader = IMAGE_FIRST_SECTION(ntHeader);

	for(UInt32 i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER	* section = &sectionHeader[i];

		if((addr >= section->VirtualAddress) && (addr <= (section->VirtualAddress + section->SizeOfRawData)))
		{
			UInt32	offset = addr - section->VirtualAddress;

			return base + section->PointerToRawData + offset;
		}
	}

	return NULL;
}

void BranchScan_Basic(const UInt8 * base, ProcHookInfo * hookInfo, bool * probable)
{
	*probable = false;

	const IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	const IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	const IMAGE_DEBUG_DIRECTORY	* debugDir =
		(IMAGE_DEBUG_DIRECTORY *)GetVirtualAddress(base, ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);

	if(!debugDir) return;
	if(debugDir->Characteristics) return;
	if(debugDir->Type != IMAGE_DEBUG_TYPE_CODEVIEW) return;
	if(debugDir->SizeOfData >= 0x100) return;
	if(debugDir->AddressOfRawData >= 0x10000000) return;
	if(debugDir->PointerToRawData >= 0x10000000) return;

	const DebugHeader	* debugHeader = (DebugHeader *)(base + debugDir->PointerToRawData);

	if(!debugHeader->SignatureValid()) return;

	*probable = true;

	bool	headerProbable;	// thrown away
	debugHeader->ReadInfo(hookInfo, &headerProbable);
}

// nogore EXE has debug info pointing to TESVng.pdb (probably something else)
// however sometimes the debug info is pointing to the wrong place?
void BranchScan(const UInt8 * base, ProcHookInfo * hookInfo)
{
	bool	result = false;
	bool	probable = false;

	// first check the header where it says it should be
	BranchScan_Basic(base, hookInfo, &probable);

	if(!probable)
	{
		_MESSAGE("using slow branch check");

		// keep scanning, now do the slow and manual way
		// look for RSDS header in .rdata

		const IMAGE_SECTION_HEADER	* rdataSection = GetImageSection(base, ".rdata");
		if(rdataSection)
		{
			const UInt8	* sectionBase = base + rdataSection->PointerToRawData;
			UInt32		sectionLen = rdataSection->SizeOfRawData;

			__try
			{
				for(UInt32 i = 0; (i + sizeof(DebugHeader)) <= sectionLen; i += 4)
				{
					const DebugHeader	* header = (const DebugHeader *)(sectionBase + i);

					header->ReadInfo(hookInfo, &probable);

					if(probable)
						break;
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				_WARNING("exception while scanning for branch");
			}
		}
	}
}

bool ScanEXE(const char * path, ProcHookInfo * hookInfo)
{
	// open and map the file in to memory
	HANDLE	file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		_ERROR("ScanEXE: couldn't open file (%d)", GetLastError());
		return false;
	}

	bool result = false;

	HANDLE	mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	if(mapping)
	{
		const UInt8	* fileBase = (const UInt8 *)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
		if(fileBase)
		{
			// scan for packing type
			bool	isSteam = IsSteamImage(fileBase);
			bool	isUPX = IsUPXImage(fileBase);

			if(isUPX)
			{
				hookInfo->procType = kProcType_Packed;
			}
			else if(isSteam)
			{
				hookInfo->procType = kProcType_Steam;
			}
			else
			{
				hookInfo->procType = kProcType_Normal;
			}

			// scan for branch (russian/nogore)
			BranchScan(fileBase, hookInfo);

			result = true;

			UnmapViewOfFile(fileBase);
		}
		else
		{
			_ERROR("ScanEXE: couldn't map file (%d)", GetLastError());
		}

		CloseHandle(mapping);
	}
	else
	{
		_ERROR("ScanEXE: couldn't create file mapping (%d)", GetLastError());
	}

	CloseHandle(file);

	return result;
}

#pragma warning (push)
#pragma warning (disable : 4065)

bool IdentifyEXE(const char * procName, bool isEditor, std::string * dllSuffix, ProcHookInfo * hookInfo)
{
	UInt64	version;

	// check file version
	if(!GetFileVersionNumber(procName, &version))
	{
		PrintLoaderError("Couldn't retrieve EXE version information.");
		return false;
	}

	_MESSAGE("version = %016I64X", version);

	// check protection type
	if(!ScanEXE(procName, hookInfo))
	{
		PrintLoaderError("Failed to identify EXE type.");
		return false;
	}

	// ### how to tell the difference between nogore versions and standard without a global checksum?
	// ### since we're mapping the exe to check for steam anyway, checksum the .text segment maybe?

	switch(hookInfo->procType)
	{
		case kProcType_Steam:	_MESSAGE("steam exe"); break;
		case kProcType_Normal:	_MESSAGE("normal exe"); break;
		case kProcType_Packed:	_MESSAGE("packed exe"); break;
		case kProcType_Unknown:
		default:				_MESSAGE("unknown exe type"); break;
	}

	if(hookInfo->noGore) _MESSAGE("nogore");
	if(hookInfo->russian) _MESSAGE("russian");
	if(hookInfo->japanese) _MESSAGE("japanese");

	bool result = false;

	const UInt64 kCurVersion = 0x0001000900200000;	// 1.9.32.0

	if(version < kCurVersion)
	{
		PrintLoaderError("Please update to the latest version of %s.", (isEditor ? "the Creation Kit" : "Skyrim"));
	}
	else if(version > kCurVersion)
	{
		PrintLoaderError("You are using a newer version of %s than this version of SKSE supports. If the patch to this version just came out, please be patient while we update our code. In the meantime, please check http://skse.silverlock.org to make sure you're using the latest version of SKSE. (version = %016I64X %08X)", (isEditor ? "the Creation Kit" : "Skyrim"), version, PACKED_SKSE_VERSION);
	}
	else if(isEditor)
	{
		switch(hookInfo->procType)
		{
		case kProcType_Steam:
		case kProcType_Normal:
			hookInfo->hookCallAddr = 0x00E7DA9B;
			hookInfo->loadLibAddr = 0x00FA6118;
			*dllSuffix = "1_9_32";

			result = true;

			break;
		case kProcType_Unknown:
		default:
			PrintLoaderError("Unsupported editor executable type.");
			break;
		}
	}
	else
	{
		switch(hookInfo->procType)
		{
			case kProcType_Steam:
			case kProcType_Normal:
				if(hookInfo->noGore)
				{
					PrintLoaderError("No-gore versions of the runtime are not currently supported.");
				}
				else if(hookInfo->russian)
				{
					// these alternate forks of the code base will be supported once they are merged with the trunk
					PrintLoaderError("The Russian version of Skyrim uses a different runtime than other regions. It is currently unsupported.");
				}
				else if(hookInfo->japanese)
				{
					// these alternate forks of the code base will be supported once they are merged with the trunk
					PrintLoaderError("The Japanese version of Skyrim uses a different runtime than other regions. It is currently unsupported.");
				}
				else
				{
					hookInfo->hookCallAddr = 0x00F56831;
					hookInfo->loadLibAddr = 0x0106B0B4;
					*dllSuffix = "1_9_32";
					
					result = true;
				}
				break;

			case kProcType_Packed:
				PrintLoaderError("Packed versions of Skyrim are not supported.");
				break;

			case kProcType_Unknown:
			default:
				PrintLoaderError("Unknown executable type.");
				break;
		}
	}

	return result;
}

#pragma warning (pop)
