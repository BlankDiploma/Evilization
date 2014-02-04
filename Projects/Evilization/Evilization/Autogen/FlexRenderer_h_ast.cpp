#include "stdafx.h"
#include <d3d9.h>
#include <math.h>
#include <d3dx9.h>
#include <d3dx9math.h>
#include <assert.h>
#include "StructParse.h"
#include "TextureLibrary.h"
#include "StructParse.h"
#include "FlexRenderer_h_ast.h"
#include "AutoEnums.h"

StructParseEntry parse_GameTexturePortion[] = {
{_T("name"), (StructParseEntryType)2, NULL, 0, offsetof(GameTexturePortion, name)},
{_T("hTex"), (StructParseEntryType)6, NULL, 0, offsetof(GameTexturePortion, hTex)},
{_T("rSrc"), (StructParseEntryType)3, parse_RECT, 0, offsetof(GameTexturePortion, rSrc)},
{_T("offset"), (StructParseEntryType)3, parse_POINT, 0, offsetof(GameTexturePortion, offset)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_GameTexturePortion[] = {L"GameTexturePortion", NULL};

