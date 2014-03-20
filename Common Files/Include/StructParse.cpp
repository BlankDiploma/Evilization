#include "stdafx.h"
#include "structparse.h"
#include "EArray.h"
#include "string.h"
#include "assert.h"
#include "windows.h"
#include "StructParseEnum.h"
#include "FlexErrorWindow.h"

int ParseTableLength(const ParseTable* pTable)
{
	return pTable->iLength;
}

int ParseTableSizeInBytes(const ParseTable* pTable)
{
	int size = 0;
	int iTableLength = ParseTableLength(pTable);
	for (int i = 0; i < iTableLength; i++)
	{
		if (pTable->pEntries[i].pchName)
			size += StructParseEntryTypeSize[pTable->pEntries[i].eType];

	}
	return size;
}

const StructParseEntry* ParseTableFind(const ParseTable* pTable, const TCHAR* pchName)
{
	int iSize = ParseTableLength(pTable);
	for (int i = 0; i < iSize; i++)
	{
		if (_wcsicmp(pTable->pEntries[i].pchName, pchName) == 0)
		{
			return &pTable->pEntries[i];
		}
	}
	return NULL;
}
/*

bool ReadParseTableValue(void* pCurObject, StructParseEntry* pEntry, const TCHAR* pchTokens)
{
	void* pOffset = ((char*)pCurObject) + pEntry->offset;
	void* pAdjustedOffset;
	TCHAR* pchBuf = (TCHAR*)pchTokens;
	TCHAR* pchNextToken = NULL;
	TCHAR* pcTemp;
	int iArraySize = 1;
	int iCur = 0;
	if (pEntry->eFlags & kStructFlag_EArray)
	{
		pchBuf = (TCHAR*)alloca((wcslen(pchTokens)+1) * sizeof(TCHAR));
		wcscpy_s(pchBuf, (wcslen(pchTokens)+1), pchTokens);
		//count commas
		pcTemp = pchBuf;
		while(pcTemp = wcschr(pcTemp, ','))
		{
			iArraySize++;
			pcTemp = pcTemp++;
		}
		//allocate array
		switch (pEntry->eType)
		{
		case kStruct_Int:
			{
				eaCreateInt((int**)pOffset, iArraySize, true);
			}break;
		case kStruct_Boolean:
			{
				eaCreateInt((int**)pOffset, iArraySize, true);
			}break;
		case kStruct_Float:
			{
				eaCreateFlt((float**)pOffset, iArraySize, true);
			}break;
		default:
			eaCreate((void***)pOffset, iArraySize, true);
		}
		pchTokens = wcstok_s(pchBuf, L"	,", &pchNextToken);
	}
	while (pchTokens)
	{
		switch (pEntry->eType)
		{
		case kStruct_Int:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
				*(int*)pAdjustedOffset = _wtoi(pchTokens);
			}break;
		case kStruct_Enum:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
				*(int*)pAdjustedOffset = AutoEnumStringToInt((StringIntHash*)pEntry->pSubTable, pchTokens);
			}break;
		case kStruct_Boolean:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, bool);
				if (_wcsnicmp(pchTokens, L"true", 4) == 0 || _wtoi(pchTokens) == 1)
					*(bool*)pAdjustedOffset = true;
				else
					*(bool*)pAdjustedOffset = false;
			}break;
		case kStruct_Float:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, float);
				*(float*)pAdjustedOffset = (float)_wtof(pchTokens);
			}break;
		case kStruct_String:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '\"')
				{
					//chop off quotes
					int iLen = wcslen(pchTokens)-2;
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen, pchTokens+1, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(pchTokens);
				}
			}break;
		case kStruct_DefRef:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '\"')
				{
					//chop off quotes
					int iLen = wcslen(pchTokens)-2;
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen, pchTokens+1, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(pchTokens);
				}
			}break;
		case kStruct_NinepatchRef:
		case kStruct_TextureRef:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '\"')
				{
					//chop off quotes
					int iLen = wcslen(pchTokens)-2;
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen, pchTokens+1, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(pchTokens);
				}
			}break;
		case kStruct_LuaScript:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '<' && pchTokens[1] == '&')
				{
					const TCHAR* endquote = wcsstr(pchTokens, L"&>");
					//chop off quotes
					int iLen = endquote-(pchTokens+2);
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen, pchTokens+2, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(pchTokens);
				}
			}break;
		}

		if (pEntry->eFlags & kStructFlag_EArray)
		{
			pchTokens = wcstok_s(NULL, L" ,	", &pchNextToken);
			iCur++;
		}
		else
			break;
	}
	return true;
}
*/

bool VerifyStructParseToken(const StructParseEntry* pEntry, const TCHAR* pchTokens, const TCHAR* pchFilename)
{
	//TODO: Add additional validation/errors.
	//trim leading whitespace
	while (IS_WHITESPACE(pchTokens[0]))
	{
		pchTokens++;
	}
	switch (pEntry->eType)
	{
	case kStruct_Int:
		{
			for (const TCHAR* iter = pchTokens; *iter && !IS_WHITESPACE(*iter); iter++)
			{
				if ((*iter < '0' || *iter > '9') && !(iter == pchTokens && *iter == '-'))
				{
					ErrorFilenamef("Expected integer value, found %s.", pchFilename, pchTokens);
					return false;
				}
			}
		}break;
	case kStruct_Enum:
		{
			if (!AutoEnumStringToInt((StringIntHash*)pEntry->pSubTable, pchTokens, NULL))
			{
				ErrorFilenamef("Expected enum value, found %s.", pchFilename, pchTokens);
				return false;
			}
		}break;
	case kStruct_Boolean:
		{
			if (_wcsnicmp(pchTokens, L"true", 4) != 0 && _wtoi(pchTokens) == 0 &&
				_wcsnicmp(pchTokens, L"false", 5) != 0 && pchTokens[0] != '0')
			{
				ErrorFilenamef("Expected boolean value, found %s.", pchFilename, pchTokens);
				return false;
			}
		}break;
	case kStruct_Float:
		{
			bool bFoundPeriod = false;
			for (const TCHAR* iter = pchTokens; *iter && !IS_WHITESPACE(*iter); iter++)
			{
				if ((*iter < '0' || *iter > '9') && !(iter == pchTokens && *iter == '-'))
				{
					if (*iter == '.' && !bFoundPeriod)
					{
						bFoundPeriod = true;
						continue;
					}
					ErrorFilenamef("Expected float value, found %s.", pchFilename, pchTokens);
					return false;
				}
			}
		}break;
	case kStruct_Color:
		{
			for (const TCHAR* iter = pchTokens; *iter && !IS_WHITESPACE(*iter); iter++)
			{
				if (*iter < '0' || *iter > '9')
				{
					ErrorFilenamef("Expected integer between 0 and 255, found %s.", pchFilename, pchTokens);
					return false;
				}
			}
			if (_wtoi(pchTokens) > 255)
			{
				ErrorFilenamef("Expected integer between 0 and 255, found %s.", pchFilename, pchTokens);
				return false;
			}
		}break;
	case kStruct_NinepatchRef:
	case kStruct_TextureRef:
	case kStruct_DefRef:
	case kStruct_String:
		{
			int iQuotes = 0;
			for (const TCHAR* iter = pchTokens; *iter && *iter != '\n'; iter++)
			{
				if (*iter == '\"')
					iQuotes++;
			}
			if (iQuotes & 1)
			{
				ErrorFilenamef("Double quotes must be matching, found %s.", pchFilename, pchTokens);
				return false;
			}
		}break;
	case kStruct_LuaScript:
		{
			if (pchTokens[0] == '<' && pchTokens[1] == '&')
			{
				const TCHAR* endquote = wcsstr(pchTokens, L"&>");
				if (!endquote)
				{
					ErrorFilenamef("<& &> Super-quotes must be matching, found %s.", pchFilename, pchTokens);
					return false;
				} 
			}
		}break;
	}
	return true;
}

bool ReadParseTableValue(void* pCurObject, const StructParseEntry* pEntry, const char* pchTokens, const TCHAR* pchFilename)
{
	void* pOffset = ((char*)pCurObject) + pEntry->offset;
	void* pAdjustedOffset;
	char* pchBuf = NULL;
	char* pchNextToken = NULL;
	char* pcTemp;
	TCHAR widebuf[512];
	int iArraySize = 1;
	int iCur = 0;
	
	if (strlen(pchTokens) >= 511)
	{
		Errorf("Failed to parse struct member %s: Static buffer size exceeded.", pEntry->pchName);
		return false;
	}


	//trim leading whitespace
	while (IS_WHITESPACE(pchTokens[0]))
	{
		pchTokens++;
	}

	

	if (pEntry->eFlags & kStructFlag_EArray)
	{
		pchBuf = (char*)alloca((strlen(pchTokens)+1) * sizeof(char));
		strcpy_s(pchBuf, (strlen(pchTokens)+1), pchTokens);
		//count commas
		pcTemp = pchBuf;
		while(pcTemp = strchr(pcTemp, ','))
		{
			iArraySize++;
			pcTemp = pcTemp++;
		}
		//allocate array
		switch (pEntry->eType)
		{
		case kStruct_Int:
			{
				eaCreateInt((int**)pOffset, iArraySize, true);
			}break;
		case kStruct_Boolean:
			{
				eaCreateInt((int**)pOffset, iArraySize, true);
			}break;
		case kStruct_Float:
			{
				eaCreateFlt((float**)pOffset, iArraySize, true);
			}break;
		default:
			eaCreate((void***)pOffset, iArraySize, true);
		}
		pchTokens = strtok_s(pchBuf, "	,", &pchNextToken);
	}
	else if (pEntry->eFlags & kStructFlag_FlagBitfield || pEntry->eType == kStruct_Color)
	{
		pchBuf = (char*)alloca((strlen(pchTokens)+1) * sizeof(char));
		strcpy_s(pchBuf, (strlen(pchTokens)+1), pchTokens);
		pchTokens = strtok_s(pchBuf, "	, ", &pchNextToken);
	}

	pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
	*(int*)pAdjustedOffset = 0;

	while (pchTokens)
	{
		swprintf_s(widebuf, L"%S", pchTokens);

		if (!VerifyStructParseToken(pEntry, widebuf, pchFilename))
			return false;

		switch (pEntry->eType)
		{
		case kStruct_Int:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
				*(int*)pAdjustedOffset = atoi(pchTokens);
			}break;
		case kStruct_Enum:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, int);
				int val = 0;
				if (AutoEnumStringToInt((StringIntHash*)pEntry->pSubTable, widebuf, &val))
					*(int*)pAdjustedOffset |= val;
			}break;
		case kStruct_Boolean:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, bool);
				if (_strnicmp(pchTokens, "true", 4) == 0 || atoi(pchTokens) == 1)
					*(bool*)pAdjustedOffset = true;
				else
					*(bool*)pAdjustedOffset = false;
			}break;
		case kStruct_Float:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, float);
				*(float*)pAdjustedOffset = (float)atof(pchTokens);
			}break;
		case kStruct_Color:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, DWORD);
				if (iCur == 0)
				{
					*(DWORD*)pAdjustedOffset = *(DWORD*)pAdjustedOffset << 8;
					*(DWORD*)pAdjustedOffset += 0xff;
				}
				*(DWORD*)pAdjustedOffset = *(DWORD*)pAdjustedOffset << 8;
				*(DWORD*)pAdjustedOffset += atoi(pchTokens);
			}break;
		case kStruct_String:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '\"')
				{
					//chop off quotes
					int iLen = strlen(pchTokens)-2;
					char temp = pchTokens[iLen+1];
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen+1, widebuf+1, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(widebuf);
				}
			}break;
		case kStruct_DefRef:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '\"')
				{
					//chop off quotes
					int iLen = strlen(pchTokens)-2;
					char temp = pchTokens[iLen+1];
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen+1, widebuf+1, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					swprintf_s(widebuf, L"%S", pchTokens);
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(widebuf);
				}
			}break;
		case kStruct_NinepatchRef:
		case kStruct_TextureRef:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '\"')
				{
					//chop off quotes
					int iLen = strlen(pchTokens)-2;
					char temp = pchTokens[iLen+1];
					(*(TCHAR**)pAdjustedOffset) = (TCHAR*)malloc(sizeof(TCHAR) * (iLen+1));
					wcsncpy_s((*(TCHAR**)pAdjustedOffset), iLen+1, widebuf+1, iLen);
					(*(TCHAR**)pAdjustedOffset)[iLen] = '\0';
				}
				else
				{
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(widebuf);
				}
			}break;
		case kStruct_LuaScript:
			{
				pAdjustedOffset = ADJUST_OFFSET(pOffset, TCHAR*);
				if (pchTokens[0] == '<' && pchTokens[1] == '&')
				{
					const char* endquote = strstr(pchTokens, "&>");
					//chop off quotes
					int iLen = endquote-(pchTokens+2);
					(*(char**)pAdjustedOffset) = (char*)malloc(sizeof(char) * (iLen+1));
					strncpy_s((*(char**)pAdjustedOffset), iLen+1, pchTokens+2, iLen);
				}
				else
				{
					int iLen = strlen(pchTokens);
					TCHAR* pDynBuf = new TCHAR[iLen+1];
					swprintf_s(pDynBuf, iLen+1, L"%S", pchTokens);
					(*(TCHAR**)pAdjustedOffset) = _wcsdup(pDynBuf);
					delete [] pDynBuf;
				}
			}break;
		}

		if ((pEntry->eFlags & kStructFlag_EArray) || (pEntry->eFlags & kStructFlag_FlagBitfield) || (pEntry->eType == kStruct_Color))
		{
			pchTokens = strtok_s(NULL, " ,	", &pchNextToken);
			iCur++;
		}
		else
			break;
	}
	return true;
}

bool ParseEntryFromStructMember(char* line, StructParseEntry_ForOutput*** eaEntriesOut)
{
	if (!strstr(line, "DEF_REF(") && strstr(line, ")"))
	{
		//line is a function declaration, ignore it
		return false;
	}
	StructParseEntry_ForOutput* pEntry = new StructParseEntry_ForOutput;
	pEntry->eType = kStruct_Invalid;
	pEntry->eFlags = 0;
	char* tok;
	char* pchNextTok;
	
	if (_stricmp(line, "const TCHAR* name;") == 0)
	{
		pEntry->eType = kStruct_String;
		pEntry->eFlags = kStructFlag_AutoSelfName;
		pEntry->name = "name";
	}
	else if (_stricmp(line, "const TCHAR* filename;") == 0)
	{
		pEntry->eType = kStruct_String;
		pEntry->eFlags = kStructFlag_AutoFileName;
		pEntry->name = "filename";
	}
	else
	{
		tok = strtok_s(line, "	 ", &pchNextTok);
	
		do{
			if (STRING_STARTS_WITH(tok, "const"))
			{
				//ignore
			}
			else if (STRING_STARTS_WITH(tok, "PARSE_IGNORE"))
			{
				delete pEntry;
				return false;
			}
			else if (STRING_STARTS_WITH(tok, "int"))
			{
				pEntry->eType = kStruct_Int;
				if (tok[strlen(tok)-1] == '*')
					pEntry->eFlags |= kStructFlag_EArray;
			}
			else if (STRING_STARTS_WITH(tok, "bool"))
			{
				pEntry->eType = kStruct_Boolean;
				if (tok[strlen(tok)-1] == '*')
					pEntry->eFlags |= kStructFlag_EArray;
			}
			else if (strcmp(tok, "TEXTURE_REF") == 0)
			{
				pEntry->eType = kStruct_TextureRef;
			}
			else if (strcmp(tok, "NINEPATCH_REF") == 0)
			{
				pEntry->eType = kStruct_NinepatchRef;
			}
			else if (strcmp(tok, "LuaScript") == 0)
			{
				pEntry->eType = kStruct_LuaScript;
			}
			else if (strcmp(tok, "COLOR_ARGB") == 0)
			{
				pEntry->eType = kStruct_Color;
			}
			else if (STRING_STARTS_WITH(tok, "float"))
			{
				pEntry->eType = kStruct_Float;
				if (tok[strlen(tok)-1] == '*')
					pEntry->eFlags |= kStructFlag_EArray;
			}
			else if (STRING_STARTS_WITH(tok, "char*") || STRING_STARTS_WITH(tok, "TCHAR*"))
			{
				pEntry->eType = kStruct_String;
				if (tok[strlen(tok)-2] == '*')
					pEntry->eFlags |= kStructFlag_EArray;
			}
			else if (STRING_STARTS_WITH(tok, "DEF_REF("))
			{
				pEntry->eType = kStruct_DefRef;
				*strchr(tok, ')') = '\0';
				pEntry->subtableName = tok + 8;
			}
			else if (strcmp(tok, "INLINE_ARG") == 0)
			{
				pEntry->eFlags |= kStructFlag_Inline;
			}
			else if (strcmp(tok, "FLAGS") == 0)
			{
				pEntry->eFlags |= kStructFlag_FlagBitfield;
			}
			else if (tok[strlen(tok)-1] == ';')
			{
				//found the name
				tok[strlen(tok)-1] = '\0';
				pEntry->name = tok;
			}
			else if (tok[strlen(tok)-1] == '*')
			{
				//must be a substruct
				tok[strlen(tok)-1] = '\0';

				if (tok[strlen(tok)-1] == '*')
				{
					pEntry->eFlags |= kStructFlag_EArray;
					tok[strlen(tok)-1] = '\0';
				}
				pEntry->eType = kStruct_SubStruct;
				pEntry->subtableName = tok;
			}
			else
			{
				//assume enum
				pEntry->eType = kStruct_Enum;
				pEntry->subtableName = tok;
			}
		}while (tok = strtok_s(NULL, "	 ", &pchNextTok));
	}

	//make sure we have a type
	assert(pEntry->eType != kStruct_Invalid);

	//make sure we have a subtable name if we need one
	assert(!((pEntry->eType == kStruct_DefRef || pEntry->eType == kStruct_SubStruct) && pEntry->subtableName.length() == 0));

	assert(pEntry->name.length() > 0);

	eaPush(eaEntriesOut, pEntry);

	return true;
}

//global dict of all parse tables

//parse tables for windows structs

static const StructParseEntry parse_entries_RECT[] = {
{_T("left"), kStruct_Int, NULL, 2, offsetof(RECT, left)},
{_T("top"), kStruct_Int, NULL, 2, offsetof(RECT, top)},
{_T("right"), kStruct_Int, NULL, 2, offsetof(RECT, right)},
{_T("bottom"), kStruct_Int, NULL, 2, offsetof(RECT, bottom)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const ParseTable parse_RECT = {L"RECT", 4, parse_entries_RECT};

static const StructParseEntry parse_entries_POINT[] = {
{_T("left"), kStruct_Int, NULL, 2, offsetof(RECT, left)},
{_T("top"), kStruct_Int, NULL, 2, offsetof(RECT, top)},
{_T("right"), kStruct_Int, NULL, 2, offsetof(RECT, right)},
{_T("bottom"), kStruct_Int, NULL, 2, offsetof(RECT, bottom)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const ParseTable parse_POINT = {L"POINT", 2, parse_entries_POINT};