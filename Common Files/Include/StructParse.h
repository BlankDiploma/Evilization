#include "stddef.h"
#include <string>
#include <hash_map>
#include "strhashmap.h"

using namespace std;

struct IDirect3DTexture9;
struct ninepatchSet;
struct GameTexture;

#pragma once

#define parse_NULL NULL

#define INLINE_ARG

#define FLAGS
#define PARSE_IGNORE

#define COLOR_ARGB unsigned long

#define PARSE_CLASS(a) ParseTable parse_##a; class a
#define PARSE_STRUCT(a) ParseTable parse_##a; struct a
#define AUTO_ENUM(a) enum a
 
#define TYPENAME(a) _T(#a)

#define DEF_REF(a) struct {const TCHAR* pchName; void* pObj;}
#define TEXTURE_REF struct {const TCHAR* pchName; GameTexture* pTex;}
#define NINEPATCH_REF struct {const TCHAR* pchName; ninepatchSet* pSet;}

#define PARSE(s, m, b, c, d, e) {_T(b), c, parse_##d, e, offsetof(s, m)}
#define PARSE_END {NULL, kStruct_Int, NULL, 0, 0}

#define IS_WHITESPACE(a) ((a) == ' ' || (a) == '	')
#define STRING_STARTS_WITH(a,b) (strncmp(a, b, strlen(b)) == 0)

#define ADJUST_OFFSET(a, b) (pEntry->eFlags & kStructFlag_EArray) ? &((*(b**)a)[iCur]) : a

//YOU MUST ADD AN ENTRY TO StructParseEntryTypeSize FOR EACH VALUE OF THIS ENUM!!!!!!!!!!
enum StructParseEntryType {kStruct_Invalid = -1, kStruct_Int = 0, kStruct_Float, kStruct_String, kStruct_SubStruct, kStruct_DefRef, kStruct_Enum, kStruct_TextureRef, kStruct_LuaScript, kStruct_Boolean, kStruct_NinepatchRef, kStruct_Color};
const int StructParseEntryTypeSize[] = {sizeof(int), sizeof(float), sizeof(TCHAR*), sizeof(void*), sizeof(TCHAR*) + sizeof(void*), sizeof(int), sizeof(TCHAR*) + sizeof(void*), sizeof(char*) + sizeof(void*) + sizeof(int), sizeof(bool), sizeof(TCHAR*) + sizeof(void*), sizeof(unsigned long)};

enum StructParseEntryFlags {kStructFlag_None = 0, kStructFlag_EArray = 0x1, kStructFlag_Inline = 0x2, kStructFlag_FlagBitfield = 0x4, kStructFlag_AutoSelfName = 0x8, kStructFlag_AutoFileName = 0x10};

struct StructParseEntry;

typedef StructParseEntry ParseTable[];

typedef unsigned int U32;
typedef int S32;


struct StructParseEntry
{
	const TCHAR* pchName;
	const StructParseEntryType eType;
	void* pSubTable;
	const U32 eFlags;
	const U32 offset;
};

struct StructParseEntry_ForOutput
{
	string name;
	StructParseEntryType eType;
	string subtableName;
	U32 eFlags;
};

int ParseTableLength(ParseTable pTable);
int ParseTableSizeInBytes(ParseTable pTable);
StructParseEntry* ParseTableFind(ParseTable pTable, const TCHAR* pchName);
bool ReadParseTableValue(void* pCurObject, StructParseEntry* pEntry, const char* pchTokens);
bool ParseEntryFromStructMember(char* line, StructParseEntry_ForOutput*** eaEntries);

#define INPUT_BUFFER_LEN (1024*1024*5)  //5mb ought to be enough for anybody

inline void RemoveComments(char* input)
{
	char* c = input;
	while (*c)
	{
		if (*c == '/')
		{
			if (c[1] == '/')
			{
				//replace with whitespace through next newline
				while (*c != '\n')
				{
					*c = ' ';
					c++;
				}
			}
			else if (c[1] == '*')
			{
				while (*c != '*' && c[1] != '/')
				{
					*c = ' ';
					c++;
				}
				*c = ' ';
				c[1] = ' ';
			}
		}
		c++;
	}
}

inline char* TrimWhitespace(char* str)
{
	while (IS_WHITESPACE(str[0]))
	{
		str++;
	}
	int len = strlen(str);
	while(IS_WHITESPACE(str[len-1]))
	{
		str[len-1] = '\0';
		len--;
	}
	return str;
}

//advances buffer to point past the line
//return value is next non-empty whitespace-trimmed line
//newlines inside of super-quotes <& &> aren't counted
inline char* GetNextTrimmedLine(char** buffer)
{
	char* trimmed;
	do{
		char* firstSuperQuoteOpen = strstr(*buffer, "<&");
		char* newline;
		if (firstSuperQuoteOpen)
		{
			char* nextSuperQuoteClose = strstr(firstSuperQuoteOpen, "&>");
			newline = strchr(*buffer, '\n');
			while (firstSuperQuoteOpen)
			{
				while (newline > firstSuperQuoteOpen && newline < nextSuperQuoteClose)
				{
					newline = strchr(newline+1, '\n');
				}
				firstSuperQuoteOpen = strstr(nextSuperQuoteClose, "<&");
				if (firstSuperQuoteOpen)
					nextSuperQuoteClose = strstr(firstSuperQuoteOpen, "&>");
			}
		}
		else
			newline = strchr(*buffer, '\n');

		if (newline)
			*newline = '\0';

		trimmed = TrimWhitespace(*buffer);

		if (newline)
			*buffer = newline + 1;
		else
		{
			*buffer = NULL;
			return trimmed;
		}
	}while(strlen(trimmed) == 0);
	return trimmed;
}

//parse tables for windows structs

extern StructParseEntry parse_RECT[];
extern StructParseEntry parse_POINT[];