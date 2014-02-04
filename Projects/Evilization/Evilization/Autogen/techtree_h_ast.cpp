#include "stdafx.h"
#include "deflibrary.h"
#include "structparse.h"
#include "earray.h"
#include "StructParse.h"
#include "techtree_h_ast.h"
#include "AutoEnums.h"

StructParseEntry parse_techTreeDef[] = {
{_T("eaNames"), (StructParseEntryType)2, NULL, 1, offsetof(techTreeDef, eaNames)},
{_T("eaNodes"), (StructParseEntryType)3, parse_techTreeNodeDef, 1, offsetof(techTreeDef, eaNodes)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_techTreeDef[] = {L"techTreeDef", NULL};

StructParseEntry parse_techTreeNodeDef[] = {
{_T("hIcon"), (StructParseEntryType)6, NULL, 0, offsetof(techTreeNodeDef, hIcon)},
{_T("cost"), (StructParseEntryType)0, NULL, 0, offsetof(techTreeNodeDef, cost)},
{_T("eaRequiredNames"), (StructParseEntryType)2, NULL, 1, offsetof(techTreeNodeDef, eaRequiredNames)},
{_T("eaGrants"), (StructParseEntryType)2, NULL, 1, offsetof(techTreeNodeDef, eaGrants)},
{_T("hEraDef"), (StructParseEntryType)4, parse_techEraDef, 0, offsetof(techTreeNodeDef, hEraDef)},
{_T("name"), (StructParseEntryType)2, NULL, 8, offsetof(techTreeNodeDef, name)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_techTreeNodeDef[] = {L"techTreeNodeDef", NULL};

StructParseEntry parse_techEraDef[] = {
{_T("hIcon"), (StructParseEntryType)6, NULL, 0, offsetof(techEraDef, hIcon)},
{_T("hNinepatch"), (StructParseEntryType)6, NULL, 0, offsetof(techEraDef, hNinepatch)},
{_T("iNinepatch"), (StructParseEntryType)0, NULL, 0, offsetof(techEraDef, iNinepatch)},
{_T("index"), (StructParseEntryType)0, NULL, 0, offsetof(techEraDef, index)},
{_T("iColStart"), (StructParseEntryType)0, NULL, 0, offsetof(techEraDef, iColStart)},
{_T("iColEnd"), (StructParseEntryType)0, NULL, 0, offsetof(techEraDef, iColEnd)},
{_T("name"), (StructParseEntryType)2, NULL, 8, offsetof(techEraDef, name)},
{_T("displayName"), (StructParseEntryType)2, NULL, 0, offsetof(techEraDef, displayName)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_techEraDef[] = {L"techEraDef", NULL};

