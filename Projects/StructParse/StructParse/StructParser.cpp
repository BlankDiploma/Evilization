// StructParse.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <string>
#include <stack>
#include <vector>
#include "Shlwapi.h"
#include "stdio.h"
#include "assert.h"
#include "StructParse.h"
#include "EArray.h"
#include <direct.h>
#include "flexerrorwindow.h"

using namespace std;

wstring ParseTableHashIncludeBuf;
string EnumHashIncludeBuf;
string EnumHashBuf;
string EnumHashTableDeclBuf;
string ParseTableHashBuf;
string PolyNameHashBuf;
string ObjSizeHashBuf;

struct ParsedStruct
{
	StructParseEntry_ForOutput** pParseTable;
	string** eaPolyNames;
	string name;
};

struct ParsedEnum
{
	string name;
	string** eaValueNames;
	int* ea32Values;
};

StructParseEntry_ForOutput** ParseStruct(char** buf, StructParseEntry_ForOutput*** peaAppendTo)
{
	char* trimmed = GetNextTrimmedLine(buf);
	StructParseEntry_ForOutput** eaParseTableOut = NULL;
	int iNum = 0;
	
	if (!peaAppendTo)
		peaAppendTo = &eaParseTableOut;
	assert(strcmp(trimmed, "{") == 0);

	trimmed = GetNextTrimmedLine(buf);

	while (trimmed[0] != '}')
	{
		if (strchr(trimmed, '{'))
		{
			//skip everything inside function declarations
			do {
				trimmed = GetNextTrimmedLine(buf);
			} while (!strchr(trimmed, '}'));
		}
		else
		{
			if (ParseEntryFromStructMember(trimmed, peaAppendTo))
				iNum++;
			trimmed = GetNextTrimmedLine(buf);
		}
	}
	return (*peaAppendTo);
}

ParsedEnum* ParseEnum(char* line, char** buf)
{
	char* name = strchr(line, '(') + 1;
	char* endofname = strchr(name, ')');
	*endofname = '\0';
	ParsedEnum* pEnum = new ParsedEnum;
	pEnum->name = name;
	pEnum->ea32Values = NULL;
	pEnum->eaValueNames = NULL;
	line = endofname+1;
	//fast-forward to the meat
	char* openbracket;
	while (!(openbracket = strchr(line, '{')))
	{
		line = GetNextTrimmedLine(buf);
	}
	char* tok;
	char* pchNextTok;
	int lastval = -1;
	line = openbracket+1;

	while (1)
	{
		tok = strtok_s(line, "	 ,{=;\n", &pchNextTok);
		if (tok)
		{
			do
			{
				int val = 0;
				char* closebracket = NULL;
				if (closebracket = strchr(tok, '}'))
					*closebracket = '\0';
				if (!(*tok))
				{
					return pEnum;
				}
				else if (tok[0] >= '0' && tok[0] <= '9')
				{
					//value
					int val = atoi(tok);
					pEnum->ea32Values[eaSize(&pEnum->ea32Values)-1] = val;
					lastval = val;
				}
				else
				{
					//name
					char* underscore = strchr(tok, '_');
					eaPush(&pEnum->eaValueNames, new string(underscore ? underscore + 1 : tok));
					eaPushInt(&pEnum->ea32Values, ++lastval);
				}
				if (closebracket)
					return pEnum;
			} while (tok = strtok_s(NULL, "	 {,=;\n", &pchNextTok));
		}
		line = GetNextTrimmedLine(buf);
	}
}

void WriteParseTableToFiles(ParsedStruct* pStruct, FILE* pOutputCpp, FILE* pOutputH)
{
	fprintf(pOutputCpp, "const StructParseEntry parse_entries_%s[] = {\n", pStruct->name.c_str());
	for (int i = 0; i < eaSize(&pStruct->pParseTable); i++)
	{
		StructParseEntry_ForOutput* pEntry = pStruct->pParseTable[i];
		if (pEntry->subtableName.length() > 0)
		{
			if(pEntry->eType == kStruct_Enum)
				fprintf(pOutputCpp, "{_T(\"%s\"), (StructParseEntryType)%d, &ht%s, %d, offsetof(%s, %s)},\n", pEntry->name.c_str(), pEntry->eType, pEntry->subtableName.c_str(), pEntry->eFlags, pStruct->name.c_str(), pEntry->name.c_str());
			else
				fprintf(pOutputCpp, "{_T(\"%s\"), (StructParseEntryType)%d, &parse_%s, %d, offsetof(%s, %s)},\n", pEntry->name.c_str(), pEntry->eType, pEntry->subtableName.c_str(), pEntry->eFlags, pStruct->name.c_str(), pEntry->name.c_str());
		}
		else
			fprintf(pOutputCpp, "{_T(\"%s\"), (StructParseEntryType)%d, NULL, %d, offsetof(%s, %s)},\n", pEntry->name.c_str(), pEntry->eType, pEntry->eFlags, pStruct->name.c_str(), pEntry->name.c_str());
	}
	fprintf(pOutputCpp, "{NULL, kStruct_Int, NULL, 0, 0}};\n");
	fprintf(pOutputCpp, "const ParseTable parse_%s = {L\"%s\", %d, parse_entries_%s};\n\n", pStruct->name.c_str(), pStruct->name.c_str(), eaSize(&pStruct->pParseTable), pStruct->name.c_str());
	fprintf(pOutputCpp, "const TCHAR* polyNames_%s[] = {L\"%s\"", pStruct->name.c_str(), pStruct->name.c_str());
	for (int i = 0; i < eaSize(&pStruct->eaPolyNames); i++)
	{
		fprintf(pOutputCpp, ", L\"%s\"", pStruct->eaPolyNames[i]->c_str());

	}
	fprintf(pOutputCpp, ", NULL};\n\n");
	
	fprintf(pOutputH, "extern const ParseTable parse_%s;\n", pStruct->name.c_str());
	fprintf(pOutputH, "extern const TCHAR* polyNames_%s[];\n", pStruct->name.c_str());
}

bool ParseFile(FILE* input, const TCHAR* pchOutputDir, const TCHAR* pchFilename, bool bWriteAutogen)
{
	static char buf[INPUT_BUFFER_LEN+1];
	char includebuf[2048] = "";
	bool bRet = false;

	int numRead = fread(buf, sizeof(char), INPUT_BUFFER_LEN, input);
	assert(numRead);
	buf[numRead] = '\0';
	RemoveComments(buf);
	vector<ParsedStruct*> vParsedStructs;
	vector<ParsedEnum*> vParsedEnums;

	char* line = buf;

	while (line)
	{
		char* trimmed = GetNextTrimmedLine(&line);
		
		if (STRING_STARTS_WITH(trimmed, "#include") && !strstr(trimmed, "_ast"))
		{
			strcat(includebuf, trimmed);
			strcat(includebuf, "\n");
		}
		else if (STRING_STARTS_WITH(trimmed, "PARSE_STRUCT") || STRING_STARTS_WITH(trimmed, "PARSE_CLASS"))
		{
			//Begin parsing a struct def.

			//Get the name of the struct.
			char* name = strchr(trimmed, '(') + 1;
			char* endofname = strchr(name, ')');
			char* polytype;
			StructParseEntry_ForOutput** eaPolymorphTable = NULL;
			*endofname = '\0';
			
			ParsedStruct* pNewStruct = new ParsedStruct;
			pNewStruct->name = name;
			pNewStruct->eaPolyNames = NULL;

			if (polytype = strchr(endofname+1, ':'))
			{
				assert(!strchr(polytype, ','));//if you hit this assert, you are using multiple inheritance. Cut that shit out.
				//polymorphism
				polytype++;
				polytype = TrimWhitespace(polytype);
				//skip over "public" or "private"
				polytype = strchr(polytype, ' ');
				if (polytype)
				{
					polytype++;
					for (int i = 0; i < vParsedStructs.size(); i++)
					{
						ParsedStruct* pStruct = vParsedStructs.at(i);
						if (pStruct->name.compare(polytype) == 0)
						{
							//found
							eaPush(&pNewStruct->eaPolyNames, new string(polytype));
							for (int iPoly = 0; iPoly < eaSize(&pStruct->eaPolyNames); iPoly++)
							{
								eaPush(&pNewStruct->eaPolyNames, pStruct->eaPolyNames[iPoly]);
							}
							for (int j = 0; j < eaSize(&pStruct->pParseTable); j++)
							{
								StructParseEntry_ForOutput* pDup = new StructParseEntry_ForOutput;
								pDup->eFlags = pStruct->pParseTable[j]->eFlags;
								pDup->eType = pStruct->pParseTable[j]->eType;
								pDup->name = pStruct->pParseTable[j]->name;
								pDup->subtableName = pStruct->pParseTable[j]->subtableName;
								eaPush(&eaPolymorphTable, pDup);
							}
							break;
						}
					}
				}
			}
			StructParseEntry_ForOutput** pParseTable = ParseStruct(&line, &eaPolymorphTable);
			pNewStruct->pParseTable = pParseTable;
			vParsedStructs.push_back(pNewStruct);
		}
		else if (STRING_STARTS_WITH(trimmed, "AUTO_ENUM"))
		{
			ParsedEnum* pEnum = ParseEnum(trimmed, &line);
			vParsedEnums.push_back(pEnum);
		}
	}

	if (vParsedStructs.size() > 0 && pchOutputDir && pchFilename)
	{
		TCHAR nameBuf[260];
		TCHAR* name;
		TCHAR* pC;
		FILE* pOutputCpp;
		FILE* pOutputH;
		
		wcscpy(nameBuf, pchFilename);

		pC = nameBuf;

		while (pC = wcschr(pC+1, L'\\'))
			name = pC+1;

		pC = name;
		
		ParseTableHashIncludeBuf += L"#include \"";
		ParseTableHashIncludeBuf += name;
		ParseTableHashIncludeBuf += L"\"\n";

		while (pC = wcschr(pC+1, L'.'))
			*pC = '_';

		wstring outputFilename = name;
		outputFilename += L"_ast";
		wstring outputPathC = pchOutputDir;
		wstring outputPathH = pchOutputDir;

		_wmkdir(pchOutputDir);

		outputPathC += outputFilename;
		outputPathH += outputFilename;
		outputPathC += L".cpp";
		outputPathH += L".h";

		if (bWriteAutogen)
		{
			pOutputCpp = _wfopen(outputPathC.c_str(), L"w");
			pOutputH = _wfopen(outputPathH.c_str(), L"w");

			fputs(includebuf, pOutputCpp);
			fputs("#include \"StructParse.h\"\n", pOutputCpp);
			fwprintf(pOutputCpp,L"#include \"%s.h\"\n", outputFilename.c_str());
			fputs("#include \"AutoEnums.h\"", pOutputCpp);
			fputs("\n\n", pOutputCpp);

			fputs(includebuf, pOutputH);
			fputs("#include \"StructParse.h\"", pOutputH);
			fputs("\n\n", pOutputH);
		}

		ParseTableHashIncludeBuf += L"#include \"";
		ParseTableHashIncludeBuf += outputFilename.c_str();
		ParseTableHashIncludeBuf += L".h\"\n";


		ParsedStruct* pStruct;
		while(!vParsedStructs.empty() && (pStruct = vParsedStructs.back()))
		{
			if (bWriteAutogen)
				WriteParseTableToFiles(pStruct, pOutputCpp, pOutputH);
			ParseTableHashBuf += "	(*phtNames)[L\"";
			ParseTableHashBuf += pStruct->name;
			ParseTableHashBuf += "\"] = &parse_";
			ParseTableHashBuf += pStruct->name;
			ParseTableHashBuf += ";\n";

			PolyNameHashBuf += "	(*phtPolyNames)[L\"";
			PolyNameHashBuf += pStruct->name;
			PolyNameHashBuf += "\"] = &polyNames_";
			PolyNameHashBuf += pStruct->name;
			PolyNameHashBuf += ";\n";

			ObjSizeHashBuf += "	(*phtObjSize)[&parse_";
			ObjSizeHashBuf += pStruct->name;
			ObjSizeHashBuf += "] = sizeof(";
			ObjSizeHashBuf += pStruct->name;
			ObjSizeHashBuf += ");\n";
			vParsedStructs.pop_back();
		}
		if (bWriteAutogen)
		{
			fclose(pOutputCpp);
			fclose(pOutputH);
		}
		bRet = true;
	}

	if (vParsedEnums.size() > 0 && pchOutputDir && pchFilename)
	{
		TCHAR nameBuf[260];
		TCHAR* name;
		TCHAR* pC;
		FILE* pOutputCpp;
		FILE* pOutputH;
		
		wcscpy(nameBuf, pchFilename);

		pC = nameBuf;

		while (pC = wcschr(pC+1, L'\\'))
			name = pC+1;

		pC = name;

		while (pC = wcschr(pC+1, L'.'))
			*pC = '_';

		wstring outputFilename = name;
		outputFilename += L"_ast";

//		EnumHashIncludeBuf += L"#include \"";
//		EnumHashIncludeBuf += outputFilename.c_str();
//		EnumHashIncludeBuf += L".h\"\n";

		ParsedEnum* pEnum;
		while(!vParsedEnums.empty() && (pEnum = vParsedEnums.back()))
		{
			EnumHashIncludeBuf += "extern StringIntHash ht";
			EnumHashIncludeBuf += pEnum->name;
			EnumHashIncludeBuf += ";\n";
			EnumHashIncludeBuf += "extern IntStringHash ht";
			EnumHashIncludeBuf += pEnum->name;
			EnumHashIncludeBuf += "Reverse;\n";
			for (int i = 0; i < eaSize(&pEnum->eaValueNames); i++)
			{
				char buf[16];
				sprintf(buf, "%d", pEnum->ea32Values[i]);
				EnumHashBuf += "	ht";
				EnumHashBuf += pEnum->name;
				EnumHashBuf += "[L\"";
				EnumHashBuf += (*pEnum->eaValueNames[i]);
				EnumHashBuf += "\"] = ";
				EnumHashBuf += buf;
				EnumHashBuf += ";\n";
				
				//reverse lookup
				EnumHashBuf += "	ht";
				EnumHashBuf += pEnum->name;
				EnumHashBuf += "Reverse[";
				EnumHashBuf += buf;
				EnumHashBuf += "] = L\"";
				EnumHashBuf += (*pEnum->eaValueNames[i]);
				EnumHashBuf += "\";\n";
			}
				
			EnumHashTableDeclBuf += "StringIntHash ht";
			EnumHashTableDeclBuf += pEnum->name;
			EnumHashTableDeclBuf += ";\n";
			EnumHashTableDeclBuf += "IntStringHash ht";
			EnumHashTableDeclBuf += pEnum->name;
			EnumHashTableDeclBuf += "Reverse;\n";
			vParsedEnums.pop_back();
		}
		bRet = true;
	}
	return bRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	WIN32_FIND_DATA f;
    wstring spec;
	wstring path = argv[1];
    stack<wstring> directories;
	vector<wstring> filesToProcess;
	vector<wstring> filesToProcessReadOnly;
	HANDLE h;
	wstring timestampFilename = argv[1];
	timestampFilename += L"\\autogen.timestamp";
	WIN32_FILE_ATTRIBUTE_DATA dat;
	bool bWroteAnything = false;
	bool bErrored = false;

	Errorf("y helo thar");
	GetFileAttributesEx(timestampFilename.c_str(), GetFileExInfoStandard, &dat);

	for (int i = 1; i < argc; i++)
	{
		directories.push(argv[i]);
	}
	filesToProcess.clear();
	
	while (!directories.empty())
	{
		path = directories.top();
		spec = path + L"\\*";
		directories.pop();
		
		h = FindFirstFile(spec.c_str(), &f);
		if (h == INVALID_HANDLE_VALUE)
			return false;

		do{
			if (wcscmp(f.cFileName, L".") != 0 && wcscmp(f.cFileName, L"..") != 0)
			{
				if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
					(wcsicmp(f.cFileName, L"Autogen") != 0) && 
					(wcsicmp(f.cFileName, L"boost") != 0) && 
					(wcsicmp(f.cFileName, L"lua") != 0) && 
					(wcsicmp(f.cFileName, L"luabind") != 0))
					directories.push(path + L"\\" + f.cFileName);
				else
				{
					if (wcsicmp(f.cFileName, L"StructParse.h") == 0)
						continue;

					TCHAR* ext = PathFindExtension(f.cFileName);
					if ((wcscmp(ext+1, L"cpp") == 0 || wcscmp(ext+1, L"h") == 0) && !wcsstr(f.cFileName, L"_ast."))
					{
						//check to see if this file was changed since our last successful run
						if (CompareFileTime(&dat.ftLastWriteTime, &dat.ftCreationTime) == 0 ||
							CompareFileTime(&f.ftLastWriteTime, &dat.ftLastWriteTime) == 1)
						{
							filesToProcess.push_back(path + L"\\" + f.cFileName);
						}
						else
							filesToProcessReadOnly.push_back(path + L"\\" + f.cFileName);
					}
				}
			}
		} while (FindNextFile(h, &f) != 0);
		
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            FindClose(h);
            return false;
        }

        FindClose(h);
        h = INVALID_HANDLE_VALUE;
	}

	if (filesToProcess.empty())
	{
		Errorf("WARNING: Structparse found no files to process. This may be because you didn't modify any relevant files prior to this compilation.");
		return 0;
	}
	
	while (!filesToProcess.empty())
	{
		wstring autogenFilename = argv[1];
		autogenFilename += L"\\Autogen\\";
		path = filesToProcess.back();
		filesToProcess.pop_back();
		FILE* input = _wfopen(path.c_str(), L"r");

		if (!input)
		{
			Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for reading.", path.c_str());
			return ERROR_INVALID_DATA;
		}

		bWroteAnything |= ParseFile(input, autogenFilename.c_str(), path.c_str(), true);
		fclose(input);
	}

	if (!bWroteAnything)
	{
		Errorf("WARNING: Structparse processed files but didn't output anything.");
		return 0;
	}
	
	while (!filesToProcessReadOnly.empty())
	{
		wstring autogenFilename = argv[1];
		autogenFilename += L"\\Autogen\\";
		path = filesToProcessReadOnly.back();
		filesToProcessReadOnly.pop_back();
		FILE* input = _wfopen(path.c_str(), L"r");

		if (!input)
		{
			Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for reading.", path.c_str());
			return ERROR_INVALID_DATA;
		}

		ParseFile(input, autogenFilename.c_str(), path.c_str(), false);
		fclose(input);
	}
	if (ParseTableHashBuf.length() > 0)
	{
		wstring autogenFilename = argv[1];
		autogenFilename += L"\\Autogen\\ParseTableDict.h";
		FILE* input = _wfopen(autogenFilename.c_str(), L"w");

		if (!input)
		{
			Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for writing.", autogenFilename.c_str());
			return ERROR_INVALID_DATA;
		}

		fputs("#include \"stdafx.h\"\n\nvoid PopulateParseTableDict(ParseTableNameHash* phtNames);\nvoid PopulatePolyTableNameHash(PolyTableNameHash* phtPolyNames);\nvoid PopulateObjSizeHash(ObjSizeHash* phtObjSize);\n", input);
		fclose(input);

		autogenFilename = argv[1];
		autogenFilename += L"\\Autogen\\ParseTableDict.cpp";
		input = _wfopen(autogenFilename.c_str(), L"w");

		if (!input)
		{
			Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for writing.", autogenFilename.c_str());
			return ERROR_INVALID_DATA;
		}

		fputs("#include \"stdafx.h\"\n\n#include \"ParseTableDict.h\"\n", input);
		fputws(ParseTableHashIncludeBuf.c_str(), input);

		fputs("\n\nvoid PopulateParseTableDict(ParseTableNameHash* phtNames)\n{\n", input);
		fputs(ParseTableHashBuf.c_str(), input);
		fputs("}\n", input);

		fputs("\n\nvoid PopulatePolyTableNameHash(PolyTableNameHash* phtPolyNames)\n{\n", input);
		fputs(PolyNameHashBuf.c_str(), input);
		fputs("}\n", input);

		fputs("\n\nvoid PopulateObjSizeHash(ObjSizeHash* phtObjSize)\n{\n", input);
		fputs(ObjSizeHashBuf.c_str(), input);
		fputs("}\n", input);

		fclose(input);
	}
	if (EnumHashBuf.length() > 0)
	{
		wstring autogenFilename = argv[1];
		autogenFilename += L"\\Autogen\\AutoEnums.h";
		FILE* input = _wfopen(autogenFilename.c_str(), L"w");

		if (!input)
		{
			Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for writing.", autogenFilename.c_str());
			return ERROR_INVALID_DATA;
		}

		fputs("#include \"stdafx.h\"\n#include \"strhashmap.h\"\n\n\nvoid PopulateAutoEnumTables();\n", input);
		fputs(EnumHashIncludeBuf.c_str(), input);
		fclose(input);

		autogenFilename = argv[1];
		autogenFilename += L"\\Autogen\\AutoEnums.cpp";
		input = _wfopen(autogenFilename.c_str(), L"w");

		if (!input)
		{
			Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for writing.", autogenFilename.c_str());
			return ERROR_INVALID_DATA;
		}

		fputs("#include \"stdafx.h\"\n\n#include \"AutoEnums.h\"\n", input);

		fputs("\n\nvoid PopulateAutoEnumTables()\n{\n", input);
		fputs(EnumHashBuf.c_str(), input);
		fputs("}\n\n", input);

		fputs(EnumHashTableDeclBuf.c_str(), input);

		fclose(input);
	}
	
	
	FILE* timestampFile = _wfopen(timestampFilename.c_str(), L"w");

	if (!timestampFile)
	{
		Errorf("FATAL ERROR: Structpare.exe was unable to open file %s for writing.", timestampFilename.c_str());
		return ERROR_INVALID_DATA;
	}

	fclose(timestampFile);

	return 0;
}

