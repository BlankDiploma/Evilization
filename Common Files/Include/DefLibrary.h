#pragma once

#include "stdafx.h"
#include "stddef.h"
#include "strhashmap.h"
#include <fstream>
#include "structparse.h"
#include <vector>

using namespace std;

class TextureLibrary;
struct IDirect3DTexture9;
struct LuaScript;


typedef stdext::hash_map<const wchar_t*, void*, stringHasher> DefHash;

struct DefStorage
{
	StructParseEntry* pParseTable;
	DefHash htDefs;
};

typedef stdext::hash_map<const wchar_t*, DefStorage*, stringHasher> DefTypeHash;

struct PendingRef
{
	void** pReference;
	TCHAR* pchName;
	StructParseEntry* pParseTable;
};

struct PendingTexture
{
	void** pReference;
	TCHAR* pchName;
};

typedef stdext::hash_map<void*, DefStorage*, pointerHasher> ParseTableHash;

typedef stdext::hash_map<TCHAR*, ParseTable*, stringHasher> ParseTableNameHash;
typedef stdext::hash_map<TCHAR*, const TCHAR*(*)[], stringHasher> PolyTableNameHash;
typedef stdext::hash_map<ParseTable*, size_t, pointerHasher> ObjSizeHash;

class DefLibrary
{
	DefTypeHash htDefTypesToDefs;
	ParseTableHash htParseTablesToDefs;
	ParseTableNameHash htNameToParseTableHash;
	PolyTableNameHash htNameToPolyTableHash;
	ObjSizeHash htNameToObjSizeHash;
	vector<PendingRef*> vPendingRefs;
	vector<PendingTexture*> vPendingTextures;
	vector<LuaScript*> vPendingScripts;
	size_t ObjectSizeInBytes(ParseTable* table);



public:
	void* GetDef(const TCHAR* pchType, const TCHAR* pchDef);
	void* GetDefUsingParseTable(void* pParseTable, const TCHAR* pchDef);
	bool LoadDefs(const TCHAR* pchDir, const TCHAR* pchFilename);
	bool LoadDefsFromFileInternal(const TCHAR* pchFilename);
	DefLibrary(void);
	~DefLibrary(void);
	bool SaveDefs(const TCHAR* pchStructName, const TCHAR* pchFilename);
	bool WriteDefToFileInternal(wofstream& file, const TCHAR* pchName, void* pCurObject, StructParseEntry* pEntry, int iIndent);
	void AddPendingDefRef(void* pObj, StructParseEntry* pParseTableEntry);
	void AddPendingTextureRef(void* pObj, StructParseEntry* pParseTableEntry);
	void AddUncompiledLuaScript(void* pObj, StructParseEntry* pParseTableEntry);
	void ResolvePendingRefs();
	void ResolvePendingTextures();
	void CompileScripts();
	int GetNumDefs(const TCHAR* pchType);
	DefHash::iterator GetDefIteratorBegin(const TCHAR* pchType);
	DefHash::iterator GetDefIteratorEnd(const TCHAR* pchType);
};

extern DefLibrary g_DefLibrary;

#define LOAD_DEFS_FROM_FILE(b) g_DefLibrary.LoadDefs(_T("data/defs/"), _T(b));
#define SAVE_DEFS_TO_FILE(a, b) g_DefLibrary.SaveDefs(_T(#a), _T("data/defs/") _T(b));

#define GET_DEF_FROM_STRING(a, b) (a*)g_DefLibrary.GetDef(TYPENAME(a), b)
#define GET_REF(a, b) ((a*)b.pObj)

#define DEF_ITER_BEGIN(a) g_DefLibrary.GetDefIteratorBegin(TYPENAME(a))
#define DEF_ITER_END(a) g_DefLibrary.GetDefIteratorEnd(TYPENAME(a))