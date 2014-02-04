#include "stdafx.h"
#include "structparse.h"
#include "earray.h"
#include "StructParse.h"
#include "HexFeatures_h_ast.h"
#include "AutoEnums.h"

StructParseEntry parse_hexBuildingDef[] = {
{_T("cost"), (StructParseEntryType)0, NULL, 0, offsetof(hexBuildingDef, cost)},
{_T("eType"), (StructParseEntryType)5, &htbuildingType, 0, offsetof(hexBuildingDef, eType)},
{_T("eaSlotDefList"), (StructParseEntryType)3, parse_laborSlotDef, 1, offsetof(hexBuildingDef, eaSlotDefList)},
{_T("maxHealth"), (StructParseEntryType)0, NULL, 0, offsetof(hexBuildingDef, maxHealth)},
{_T("visRadius"), (StructParseEntryType)0, NULL, 0, offsetof(hexBuildingDef, visRadius)},
{_T("name"), (StructParseEntryType)2, NULL, 8, offsetof(hexBuildingDef, name)},
{_T("displayName"), (StructParseEntryType)2, NULL, 0, offsetof(hexBuildingDef, displayName)},
{_T("hTex"), (StructParseEntryType)4, parse_GameTexturePortion, 0, offsetof(hexBuildingDef, hTex)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_hexBuildingDef[] = {L"hexBuildingDef", NULL};

StructParseEntry parse_laborSlotDef[] = {
{_T("laborName"), (StructParseEntryType)2, NULL, 0, offsetof(laborSlotDef, laborName)},
{_T("eType"), (StructParseEntryType)5, &htlaborType, 0, offsetof(laborSlotDef, eType)},
{_T("production"), (StructParseEntryType)0, NULL, 0, offsetof(laborSlotDef, production)},
{_T("gold"), (StructParseEntryType)0, NULL, 0, offsetof(laborSlotDef, gold)},
{_T("research"), (StructParseEntryType)0, NULL, 0, offsetof(laborSlotDef, research)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_laborSlotDef[] = {L"laborSlotDef", NULL};

StructParseEntry parse_hexUnitDef[] = {
{_T("movement"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, movement)},
{_T("maxHealth"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, maxHealth)},
{_T("TypeFlags"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, TypeFlags)},
{_T("numAbilities"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, numAbilities)},
{_T("cost"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, cost)},
{_T("meleeStr"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, meleeStr)},
{_T("visRadius"), (StructParseEntryType)0, NULL, 0, offsetof(hexUnitDef, visRadius)},
{_T("pAbilities"), (StructParseEntryType)3, parse_hexAbilityDef, 1, offsetof(hexUnitDef, pAbilities)},
{_T("hTex"), (StructParseEntryType)4, parse_GameTexturePortion, 0, offsetof(hexUnitDef, hTex)},
{_T("name"), (StructParseEntryType)2, NULL, 8, offsetof(hexUnitDef, name)},
{_T("displayName"), (StructParseEntryType)2, NULL, 0, offsetof(hexUnitDef, displayName)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_hexUnitDef[] = {L"hexUnitDef", NULL};

StructParseEntry parse_hexAbilityDef[] = {
{_T("cooldown"), (StructParseEntryType)0, NULL, 0, offsetof(hexAbilityDef, cooldown)},
{_T("range"), (StructParseEntryType)0, NULL, 0, offsetof(hexAbilityDef, range)},
{_T("damage"), (StructParseEntryType)0, NULL, 0, offsetof(hexAbilityDef, damage)},
{_T("flags"), (StructParseEntryType)0, NULL, 0, offsetof(hexAbilityDef, flags)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_hexAbilityDef[] = {L"hexAbilityDef", NULL};

