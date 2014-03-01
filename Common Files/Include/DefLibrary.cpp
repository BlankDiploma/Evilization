#include "StdAfx.h"
#include "DefLibrary.h"
#include <fstream>
#include "strhashmap.h"
#include "new.h"
#include "assert.h"
#include "EArray.h"
#include <stack>
#include "structparse.h"
#include "TextureLibrary.h"
#include "FlexLua.h"
#include "Autogen/ParseTableDict.h"
#include "Autogen/AutoEnums.h"
#include <luabind/luabind.hpp>
#include "structparseluabind.h"
#include "FlexErrorWindow.h"

using namespace std;

DefLibrary g_DefLibrary;

DefLibrary::DefLibrary(void)
{
	PopulateParseTableDict(&htNameToParseTableHash);
	PopulatePolyTableNameHash(&htNameToPolyTableHash);
	PopulateObjSizeHash(&htNameToObjSizeHash);
}

DefHash::iterator DefLibrary::GetDefIteratorBegin(const TCHAR* pchType)
{
	DefTypeHash::iterator hashIter;
	DefHash::iterator defHashIter;
	DefStorage* pStorage = NULL;

	hashIter = htDefTypesToDefs.find(pchType);
	if(hashIter != htDefTypesToDefs.end())
	{
		pStorage = hashIter->second;
	}
	
	assert(pStorage);
	
	return pStorage->htDefs.begin();
}

DefHash::iterator DefLibrary::GetDefIteratorEnd(const TCHAR* pchType)
{
	DefTypeHash::iterator hashIter;
	DefHash::iterator defHashIter;
	DefStorage* pStorage = NULL;

	hashIter = htDefTypesToDefs.find(pchType);
	if(hashIter != htDefTypesToDefs.end())
	{
		pStorage = hashIter->second;
	}
	
	assert(pStorage);
	
	return pStorage->htDefs.end();
}

size_t DefLibrary::ObjectSizeInBytes(const ParseTable* table)
{
	ObjSizeHash::iterator hashIter;

	hashIter = htNameToObjSizeHash.find(table);
	if(hashIter != htNameToObjSizeHash.end())
	{
		return hashIter->second;
	}
	
	return 0;
}

void DoNewObjectPolish(const ParseTable* pTable, void* pObj, const TCHAR* pchName, const TCHAR* pchFilename)
{
	int iTableLength = ParseTableLength(pTable);
	for(int i = 0; i < iTableLength; i++)
	{
		if (pTable->pEntries[i].eFlags & kStructFlag_AutoSelfName)
		{
			void* pOffset = ((char*)pObj) + pTable->pEntries[i].offset;
			(*(const TCHAR**)pOffset) = pchName;
		}
		else if (pTable->pEntries[i].eFlags & kStructFlag_AutoFileName)
		{
			void* pOffset = ((char*)pObj) + pTable->pEntries[i].offset;
			(*(TCHAR**)pOffset) = _wcsdup(pchFilename);
		}
	}
}

bool DefLibrary::LoadDefsFromFileInternal(const TCHAR* pchFilename)
{
	PopulateAutoEnumTables();
	try
	{
		FILE* input = NULL;
		_wfopen_s(&input, pchFilename, L"r");
		
		if (!input)
		{
			ErrorFilenamef("Failed to open def file %s for parsing.", pchFilename, pchFilename);
			return false;
		}

		char* buf = new char[INPUT_BUFFER_LEN+1];
		TCHAR widebuf[512];
		const char* pchToken = NULL;
		DefStorage* pStorage;

		stack<const ParseTable*> sParseTableStack;
		stack<void*> sObjStack;
		stack<TCHAR*> sObjNameStack;

		int iCurObjSize;
		int iStructParam = 0;
		DefTypeHash::iterator hashIter;
		ParseTableHash::iterator hashIterParse;
		ParseTableNameHash::iterator hashIterParseName;
		PolyTableNameHash::iterator hashIterPolyName;
		bool bExpectOpeningBrace = false;

		int numRead = fread(buf, sizeof(char), INPUT_BUFFER_LEN, input);
		
		if (numRead == INPUT_BUFFER_LEN)
		{
			ErrorFilenamef("Def file %s exceeds 5mb in size.", pchFilename, pchFilename);
			
			delete [] buf;
			fclose(input);
			return false;
		}

		buf[numRead] = '\0';
		RemoveComments(buf);

		char* line = buf;
		

		while (line)
		{
			char* pchCurTrimmedLine = GetNextTrimmedLine(&line);
		
			char* pchNextToken = NULL;

			pchToken = strtok_s(pchCurTrimmedLine, " 	", &pchNextToken);
			while (pchToken)
			{
				if (!pchToken || pchToken[0] == '\0')
				{
					pchToken = strtok_s(NULL, " 	", &pchNextToken);
					continue;
				}
				else if (pchToken[0] == '{')
				{
					if (!bExpectOpeningBrace || sParseTableStack.empty())
					{
						ErrorFilenamef("Unexpected opening brace in def file %s.", pchFilename, pchFilename);
						
						delete [] buf;
						fclose(input);
						return false;
					}
					pchToken = strtok_s(NULL, " 	", &pchNextToken);
					bExpectOpeningBrace = false;
					continue;
				}
				else if (bExpectOpeningBrace)
				{
					sParseTableStack.pop();
					sObjStack.pop();
					sObjNameStack.pop();
					bExpectOpeningBrace = false;
					continue;
				}
				else if (pchToken[0] == L'}')
				{
					
					if (sParseTableStack.empty())
					{
						ErrorFilenamef("Unexpected closing brace in def file %s.", pchFilename, pchFilename);
						
						delete [] buf;
						fclose(input);
						return false;
					}
					sParseTableStack.pop();
					sObjStack.pop();
					sObjNameStack.pop();
					//finished parsing an object.
					pchToken = strtok_s(NULL, " 	", &pchNextToken);
					bExpectOpeningBrace = false;
					continue;
				}
				swprintf_s(widebuf, L"%S", pchToken);
				hashIterParseName = htNameToParseTableHash.find(widebuf);

				//if we encounter the name of a parse table, push it onto the stack and go to the next token.
				if (hashIterParseName != htNameToParseTableHash.end())
				{
					const ParseTable* pTable = hashIterParseName->second ? hashIterParseName->second : NULL;
					sParseTableStack.push(pTable);

					hashIter = htDefTypesToDefs.find(widebuf);
					if(hashIter != htDefTypesToDefs.end())
					{
						pStorage = hashIter->second;
					}
					else
					{
						pStorage = new DefStorage;
						pStorage->pParseTable = pTable;
						htDefTypesToDefs[_wcsdup(widebuf)] = pStorage;
					}
		
					hashIterParse = htParseTablesToDefs.find(pTable);
					if(hashIterParse == htParseTablesToDefs.end())
					{
						htParseTablesToDefs[pTable] = pStorage;
					}
					pchToken = strtok_s(NULL, " 	", &pchNextToken);
					swprintf_s(widebuf, L"%S", pchToken);
				}
				if (sParseTableStack.empty())
				{
					ErrorFilenamef("Unexpected token %s found outside of any struct in def file %s.", pchFilename, widebuf, pchFilename);
					continue;
				}
				const ParseTable* pCurTable = sParseTableStack.top();
				const StructParseEntry* pEntry = ParseTableFind(pCurTable, widebuf);
				if (sObjStack.empty() || pEntry && pEntry->eType == kStruct_SubStruct)
				{
					//first line of an object - parse name and structparams
					
					const char* pchStructParam = strtok_s(NULL, " 	", &pchNextToken);

					if (!sObjStack.empty())
					{
						//We are dealing with a substruct
						TCHAR polynameBuf[512];
						const TCHAR* (*polynames)[] = NULL;
						swprintf_s(polynameBuf, L"%S", pchStructParam);
						//Determine polytype of substruct if necessary.
						hashIterParseName = htNameToParseTableHash.find(polynameBuf);
						if (hashIterParseName != htNameToParseTableHash.end())
						{
							const ParseTable* pTable = hashIterParseName->second;
							//we have a parsetable name, check if it's a valid polytype of our base
							hashIterPolyName = htNameToPolyTableHash.find(polynameBuf);
							if(hashIterPolyName != htNameToPolyTableHash.end())
							{
								polynames = hashIterPolyName->second;
							}
							const TCHAR** pchPolyName = (*polynames);
							while((*pchPolyName))
							{
								hashIterParseName = htNameToParseTableHash.find((TCHAR*)*pchPolyName);
								if (hashIterParseName != htNameToParseTableHash.end() &&
									hashIterParseName->second == pEntry->pSubTable)
								{
									//This is a valid polytype.
									sParseTableStack.push(pTable);
									pchStructParam = strtok_s(NULL, " 	", &pchNextToken);
									break;
								}
								
								pchPolyName++;
							}
						}
						else
							sParseTableStack.push((const ParseTable*)pEntry->pSubTable);
						pCurTable = sParseTableStack.top();
					}

					iCurObjSize = ObjectSizeInBytes(pCurTable);
					int iNextInlineParam = 0;
					void* pObj = operator new (iCurObjSize);
					memset(pObj, 0, iCurObjSize);
					TCHAR* pchObjName = _wcsdup(widebuf);

					//Add pObj to parent
					if (!sObjStack.empty())
					{
						void* pParent = sObjStack.top();
						if (pEntry->eFlags & kStructFlag_EArray)
						{
							void*** ea = (void***)(((char*)pParent + pEntry->offset));
							eaPush(ea, pObj);
						}
						else
							*(void**)((char*)pParent + pEntry->offset) = pObj;
					}

					sObjStack.push(pObj);
					sObjNameStack.push(pchObjName);

					if (sObjStack.size() == 1)
					{
						const TCHAR* (*polynames)[] = NULL;
						hashIterPolyName = htNameToPolyTableHash.find(pCurTable->pchName);
						if(hashIterPolyName != htNameToPolyTableHash.end())
						{
							polynames = hashIterPolyName->second;
						}
						const TCHAR** pchPolyName = (*polynames);
						/**
							Here's where new objects get added and initialized. Stop forgetting, fuckface.
						**/
						while((*pchPolyName))
						{
							hashIter = htDefTypesToDefs.find(*pchPolyName);
							if(hashIter != htDefTypesToDefs.end())
							{
								DefStorage* pPolyStorage = hashIter->second;
								pPolyStorage->htDefs[pchObjName] = sObjStack.top();
							}
							pchPolyName++;
						}
						DoNewObjectPolish(pCurTable, sObjStack.top(), pchObjName, pchFilename);
					}

					while (pchStructParam && pchStructParam[0])
					{
						int i;
						bool bFound = false;
						int iTableLength = ParseTableLength(pCurTable);
						for(i = iNextInlineParam; i < iTableLength; i++)
						{
							if (pCurTable->pEntries[i].eFlags & kStructFlag_Inline)
							{
								iNextInlineParam = i+1;
								bFound = true;
								break;
							}
						}
						if (!bFound)
						{
							ErrorFilenamef("Unknown structparam %s in parse table %s", pchFilename, pchStructParam, pCurTable->pchName);
							continue;
						}
						ReadParseTableValue(sObjStack.top(), &pCurTable->pEntries[i], pchStructParam, pchFilename);
						
						if (pCurTable->pEntries[i].eType == kStruct_DefRef)
							AddPendingDefRef(sObjStack.top(), &pCurTable->pEntries[i], pchObjName, pCurTable->pchName);
						else if (pCurTable->pEntries[i].eType == kStruct_TextureRef)
							AddPendingTextureRef(sObjStack.top(), &pCurTable->pEntries[i], sObjNameStack.top(), pCurTable->pchName);
						else if (pCurTable->pEntries[i].eType == kStruct_NinepatchRef)
							AddPendingTextureRef(sObjStack.top(), &pCurTable->pEntries[i], sObjNameStack.top(), pCurTable->pchName);
						else if (pCurTable->pEntries[i].eType == kStruct_LuaScript)
							AddUncompiledLuaScript(sObjStack.top(), &pCurTable->pEntries[i]);
						
						iStructParam++;
						pchStructParam = strtok_s(NULL, " 	", &pchNextToken);
					}

					bExpectOpeningBrace = true;

					//go to next token
					pchToken = strtok_s(NULL, " \n	", &pchNextToken);
				}
				else
				{
					if (!pEntry)
					{
						ErrorFilenamef("Unknown token %s in parse table %s", pchFilename, widebuf, pCurTable->pchName);
						continue;
					}

					//hand the whole rest of the line off to be parsed, this should always consume all remaining tokens
					ReadParseTableValue(sObjStack.top(), pEntry, pchNextToken, pchFilename);
					
					if (pEntry->eType == kStruct_DefRef)
						AddPendingDefRef(sObjStack.top(), pEntry, sObjNameStack.top(), pCurTable->pchName);
					else if (pEntry->eType == kStruct_TextureRef)
						AddPendingTextureRef(sObjStack.top(), pEntry, sObjNameStack.top(), pCurTable->pchName);
					else if (pEntry->eType == kStruct_NinepatchRef)
						AddPendingTextureRef(sObjStack.top(), pEntry, sObjNameStack.top(), pCurTable->pchName);
					else if (pEntry->eType == kStruct_LuaScript)
						AddUncompiledLuaScript(sObjStack.top(), pEntry);
					pchToken = NULL;
				}
			}
		}
		delete [] buf;
		fclose(input);
		return true;
	}
	catch (ios_base::failure* e)
	{
		//This is just for debugging purposes.
		assert(!e);
		e = NULL;
	}
	return false;
}

bool DefLibrary::WriteDefToFileInternal(wofstream& file, const TCHAR* pchName, const void* pCurObject, const ParseTable* pTable, int iIndent)
{
	bool bLastWasInline = true;
	TCHAR indentBuffer[16];
	int i;
	const StructParseEntry* pEntry = pTable->pEntries;
	for (i = 0; i < iIndent; i++) 
		indentBuffer[i] = '	';
	indentBuffer[i] = '\0';

	file << indentBuffer << pchName;
	while (pEntry && pEntry->pchName)
	{
		void* pOffset = ((char*)pCurObject) + pEntry->offset;
		void* pAdjustedOffset;
		int iCur;
		int iArraySize = (pEntry->eFlags & kStructFlag_EArray) ? eaSize(pOffset) : 1;
		if (!(pEntry->eFlags & kStructFlag_Inline) && bLastWasInline)
		{
			bLastWasInline = false;
			file << endl << indentBuffer << '{' << endl;
		}
		for (iCur = 0; iCur < iArraySize; iCur++)
		{
			if (pEntry->eType != kStruct_SubStruct)
			{
				if (iCur > 0)
					file << L", ";
				else if (!(pEntry->eFlags & kStructFlag_Inline))
					file << indentBuffer << '	' << pEntry->pchName << ' ';
				else
					file << ' ';
			}
			switch (pEntry->eType)
			{
			case kStruct_Int:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
					file << *(int*)(pAdjustedOffset);
				}break;
			case kStruct_Float:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, float);
					file << *(float*)(pAdjustedOffset);
				}break;
			case kStruct_String:
				{
					if (pEntry->eFlags & kStructFlag_AutoSelfName)
						break;
					pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
					file << '\"' << *(TCHAR**)(pAdjustedOffset) << '\"';
				}break;
			case kStruct_Enum:
				{
					assert(0);
					//unimplemented due to inability to find appropriate int-to-string hash based on parse table
					/*
					pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
					int value = AutoEnumStringToInt((StringIntHash*)pEntry->pSubTable, widebuf)
					file << '\"' << *(TCHAR**)(pAdjustedOffset) << '\"';
					*/
				}break;
			case kStruct_SubStruct:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, intptr_t);
					WriteDefToFileInternal(file, pEntry->pchName, *(int**)(pAdjustedOffset), (const ParseTable*)pEntry->pSubTable, iIndent+1);
					//unimplemented
				}break;
			case kStruct_DefRef:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
					file << '\"' << *(TCHAR**)(pAdjustedOffset) << '\"';
				}break;
			case kStruct_NinepatchRef:
			case kStruct_TextureRef:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
					file << '\"' << *(TCHAR**)(pAdjustedOffset) << '\"';
				}break;
			case kStruct_LuaScript:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
					file << "<&" << *(TCHAR**)(pAdjustedOffset) << "&>";
				}break;
			case kStruct_Boolean:
				{
					pAdjustedOffset = ADJUST_OFFSET(pOffset, bool);
					file << (*(bool*)(pAdjustedOffset) ? "true" : "false");
				}break;
			}
			if (iCur == iArraySize-1 && pEntry->eType != kStruct_SubStruct)
			{
				if (!bLastWasInline)
					file << endl;
			}
			
		}
		pEntry++;
	}
	if (!bLastWasInline)
		file << indentBuffer <<  '}';

	file << endl;
	
	return true;
}

bool DefLibrary::SaveDefs(const TCHAR* pchStructName, const TCHAR* pchFilename)
{
	wofstream output(pchFilename);
	DefTypeHash::iterator hashIter;
	DefHash::iterator defIter;
	DefStorage* pStorage;
	hashIter = htDefTypesToDefs.find(pchStructName);
	if(hashIter != htDefTypesToDefs.end())
	{
		pStorage = hashIter->second;
	}

	for(defIter = pStorage->htDefs.begin(); defIter != pStorage->htDefs.end(); ++defIter) 
	{
		WriteDefToFileInternal(output, defIter->first, defIter->second, pStorage->pParseTable, 0);
	}
	output.close();
	return true;
}

bool DefLibrary::LoadDefs(const TCHAR* pchDir, const TCHAR* pchFilename)
{
	TCHAR directory[260] = {0};
	WIN32_FIND_DATA f;
	wcscpy_s(directory, pchDir);
	wcscat_s(directory, pchFilename);
	HANDLE h = FindFirstFile(directory, &f);
	if(h != INVALID_HANDLE_VALUE)
	{
		do{
			wcscpy_s(directory, pchDir);
			wcscat_s(directory, f.cFileName);
			if (!LoadDefsFromFileInternal(directory))
				return false;
		} while(FindNextFile(h, &f));
	}
	else
	{
		fprintf(stderr, "Error opening directory\n");
		return false;
	}
	FindClose(h);
	return true;
}

const void* DefLibrary::GetDef(const TCHAR* pchType, const TCHAR* pchDef)
{
	
	DefTypeHash::iterator hashIter;
	DefHash::iterator defHashIter;
	DefStorage* pStorage = NULL;

	if (!pchDef)
		return NULL;

	hashIter = htDefTypesToDefs.find(pchType);
	if(hashIter != htDefTypesToDefs.end())
	{
		pStorage = hashIter->second;
	}

	if (pStorage)
	{
		defHashIter = pStorage->htDefs.find(pchDef);
		if(defHashIter != pStorage->htDefs.end())
		{
			return defHashIter->second;
		}
	}

	Errorf("Unknown def %s of type %s", pchDef, pchType);

	return NULL;
}


const void* DefLibrary::GetDefUsingParseTable(const ParseTable* pParseTable, const TCHAR* pchDef)
{
	ParseTableHash::iterator hashIter;
	DefHash::iterator defHashIter;
	DefStorage* pStorage = NULL;

	hashIter = htParseTablesToDefs.find(pParseTable);
	if(hashIter != htParseTablesToDefs.end())
	{
		pStorage = hashIter->second;
	}

	if (pStorage)
	{
		defHashIter = pStorage->htDefs.find(pchDef);
		if(defHashIter != pStorage->htDefs.end())
		{
			return defHashIter->second;
		}
	}
	return NULL;
}

DefLibrary::~DefLibrary(void)
{
}

void DefLibrary::AddPendingDefRef(void* pObj, const StructParseEntry* pParseTableEntry, const TCHAR* pchObjName, const TCHAR* pchObjType)
{
	PendingRef* pPending = new PendingRef;
	pPending->pchName = *(TCHAR**)((char*)pObj + pParseTableEntry->offset);
	pPending->pParseTable = (const ParseTable*)pParseTableEntry->pSubTable;
	pPending->pReference = (const void**)((char*)pObj + pParseTableEntry->offset + sizeof(TCHAR*));
	pPending->pParentObjName = pchObjName;
	pPending->pParentObjType = pchObjType;
	vPendingRefs.push_back(pPending);
}

void DefLibrary::AddPendingTextureRef(void* pObj, const StructParseEntry* pParseTableEntry, const TCHAR* pchObjName, const TCHAR* pchObjType)
{
	PendingTexture* pPending = new PendingTexture;
	pPending->pchName = *(TCHAR**)((char*)pObj + pParseTableEntry->offset);
	pPending->pReference = (void**)((char*)pObj + pParseTableEntry->offset + sizeof(TCHAR*));
	pPending->pParentObjName = pchObjName;
	pPending->pParentObjType = pchObjType;
	vPendingTextures.push_back(pPending);
}

void DefLibrary::AddUncompiledLuaScript(void* pObj, const StructParseEntry* pParseTableEntry)
{
//	PendingScript* pScript = new PendingScript;
//	pScript->pPlaintext = (char**)((char*)pObj + pParseTableEntry->offset);
//	pScript->pObj = (luabind::object*)((char*)pObj + pParseTableEntry->offset);
//	pScript->pBytecode = (BYTE**)((char*)pObj + pParseTableEntry->offset + sizeof(char*));
//	pScript->piSize = (int*)((char*)pObj + pParseTableEntry->offset + sizeof(char*) + sizeof(BYTE*));
	vPendingScripts.push_back((LuaScript*)((char*)pObj + pParseTableEntry->offset));
}

//call after loading all defs
void DefLibrary::ResolvePendingRefs()
{
	while (!vPendingRefs.empty())
	{
		PendingRef* pPending = vPendingRefs.back();
		const void* pDef = GetDefUsingParseTable(pPending->pParseTable, pPending->pchName);
		if (!pDef)
		{
			Errorf("Bad ref to %s \"%s\" found while loading %s \"%s\".", pPending->pParseTable->pchName, pPending->pchName, pPending->pParentObjType, pPending->pParentObjName);
		}
		(*pPending->pReference) = pDef;
		delete pPending;
		vPendingRefs.pop_back();
	}
}

//call after loading all textures
void DefLibrary::ResolvePendingTextures()
{
	while (!vPendingTextures.empty())
	{
		PendingTexture* pPending = vPendingTextures.back();
		GameTexture* pTex = g_TextureLibrary.GetTexture(pPending->pchName);
		if (!pTex)
		{
			Errorf("Bad ref to texture \"%s\" found while loading %s \"%s\".", pPending->pchName, pPending->pParentObjType, pPending->pParentObjName);
		}
		(*pPending->pReference) = pTex;
		delete pPending;
		vPendingTextures.pop_back();
	}
}

//call when LUA is ready
void DefLibrary::CompileScripts()
{
	while (!vPendingScripts.empty())
	{
		LuaScript* pPending = vPendingScripts.back();
		int error = luaL_loadbuffer(g_LUA, pPending->pchPlaintext, strlen(pPending->pchPlaintext), NULL);
		if (error)
		{
			const char* error = lua_tostring(g_LUA, -1);
			Errorf("Lua compile error: %S", error);
			lua_pop(g_LUA, 1);  /* pop error message from the stack */
		  
			pPending->func = NULL;
			vPendingScripts.pop_back();
			continue;
		}
		luabind::object* TheChunk = new luabind::object(luabind::from_stack(g_LUA, -1));
		pPending->func = TheChunk;
		lua_pop(g_LUA, 1);

//		delete pPending;
		vPendingScripts.pop_back();
	}
}

int DefLibrary::GetNumDefs(const TCHAR* pchType)
{
	
	DefTypeHash::iterator hashIter;
	DefHash::iterator defHashIter;
	DefStorage* pStorage = NULL;

	hashIter = htDefTypesToDefs.find(pchType);
	if(hashIter != htDefTypesToDefs.end())
	{
		pStorage = hashIter->second;
	}

	if (pStorage)
	{
		return pStorage->htDefs.size();
	}
	Errorf("Unrecognized type \"%s\" passed to GetNumDefs()", pchType);
	return NULL;
}


#include "Autogen/ParseTableDict.cpp"
#include "Autogen/AutoEnums.cpp"