#include "stdafx.h"
#include "mtrand.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "HexFeatures.h"
#include <queue>
#include "structparse.h"
#include "earray.h"
#include "flexrenderer.h"
#include "StructParse.h"
#include "HexMap_h_ast.h"
#include "AutoEnums.h"

StructParseEntry parse_hexTileDef[] = {
{_T("iMaxPillageTurns"), (StructParseEntryType)0, NULL, 0, offsetof(hexTileDef, iMaxPillageTurns)},
{_T("name"), (StructParseEntryType)2, NULL, 8, offsetof(hexTileDef, name)},
{_T("displayName"), (StructParseEntryType)2, NULL, 0, offsetof(hexTileDef, displayName)},
{_T("bWalkable"), (StructParseEntryType)8, NULL, 0, offsetof(hexTileDef, bWalkable)},
{_T("iMoveCost"), (StructParseEntryType)0, NULL, 0, offsetof(hexTileDef, iMoveCost)},
{_T("color"), (StructParseEntryType)10, NULL, 0, offsetof(hexTileDef, color)},
{_T("slotDef"), (StructParseEntryType)3, parse_laborSlotDef, 0, offsetof(hexTileDef, slotDef)},
{_T("hTex"), (StructParseEntryType)4, parse_GameTexturePortion, 0, offsetof(hexTileDef, hTex)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_hexTileDef[] = {L"hexTileDef", NULL};

