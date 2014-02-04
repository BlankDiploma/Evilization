#include "stdafx.h"

#include "ParseTableDict.h"
#include "UILib.h"
#include "UILib_h_ast.h"
#include "techtree.h"
#include "techtree_h_ast.h"
#include "HexMap.h"
#include "HexMap_h_ast.h"
#include "HexFeatures.h"
#include "HexFeatures_h_ast.h"
#include "FlexRenderer.h"
#include "FlexRenderer_h_ast.h"


void PopulateParseTableDict(ParseTableNameHash* phtNames)
{
	(*phtNames)[L"UIButtonDef"] = &parse_UIButtonDef;
	(*phtNames)[L"UITextDef"] = &parse_UITextDef;
	(*phtNames)[L"UILayoutDef"] = &parse_UILayoutDef;
	(*phtNames)[L"UIBoxDef"] = &parse_UIBoxDef;
	(*phtNames)[L"UIChildDef"] = &parse_UIChildDef;
	(*phtNames)[L"UITextureLayer"] = &parse_UITextureLayer;
	(*phtNames)[L"techTreeDef"] = &parse_techTreeDef;
	(*phtNames)[L"techTreeNodeDef"] = &parse_techTreeNodeDef;
	(*phtNames)[L"techEraDef"] = &parse_techEraDef;
	(*phtNames)[L"hexTileDef"] = &parse_hexTileDef;
	(*phtNames)[L"hexBuildingDef"] = &parse_hexBuildingDef;
	(*phtNames)[L"laborSlotDef"] = &parse_laborSlotDef;
	(*phtNames)[L"hexUnitDef"] = &parse_hexUnitDef;
	(*phtNames)[L"hexAbilityDef"] = &parse_hexAbilityDef;
	(*phtNames)[L"GameTexturePortion"] = &parse_GameTexturePortion;
}


void PopulatePolyTableNameHash(PolyTableNameHash* phtPolyNames)
{
	(*phtPolyNames)[L"UIButtonDef"] = &polyNames_UIButtonDef;
	(*phtPolyNames)[L"UITextDef"] = &polyNames_UITextDef;
	(*phtPolyNames)[L"UILayoutDef"] = &polyNames_UILayoutDef;
	(*phtPolyNames)[L"UIBoxDef"] = &polyNames_UIBoxDef;
	(*phtPolyNames)[L"UIChildDef"] = &polyNames_UIChildDef;
	(*phtPolyNames)[L"UITextureLayer"] = &polyNames_UITextureLayer;
	(*phtPolyNames)[L"techTreeDef"] = &polyNames_techTreeDef;
	(*phtPolyNames)[L"techTreeNodeDef"] = &polyNames_techTreeNodeDef;
	(*phtPolyNames)[L"techEraDef"] = &polyNames_techEraDef;
	(*phtPolyNames)[L"hexTileDef"] = &polyNames_hexTileDef;
	(*phtPolyNames)[L"hexBuildingDef"] = &polyNames_hexBuildingDef;
	(*phtPolyNames)[L"laborSlotDef"] = &polyNames_laborSlotDef;
	(*phtPolyNames)[L"hexUnitDef"] = &polyNames_hexUnitDef;
	(*phtPolyNames)[L"hexAbilityDef"] = &polyNames_hexAbilityDef;
	(*phtPolyNames)[L"GameTexturePortion"] = &polyNames_GameTexturePortion;
}


void PopulateObjSizeHash(ObjSizeHash* phtObjSize)
{
	(*phtObjSize)[&parse_UIButtonDef] = sizeof(UIButtonDef);
	(*phtObjSize)[&parse_UITextDef] = sizeof(UITextDef);
	(*phtObjSize)[&parse_UILayoutDef] = sizeof(UILayoutDef);
	(*phtObjSize)[&parse_UIBoxDef] = sizeof(UIBoxDef);
	(*phtObjSize)[&parse_UIChildDef] = sizeof(UIChildDef);
	(*phtObjSize)[&parse_UITextureLayer] = sizeof(UITextureLayer);
	(*phtObjSize)[&parse_techTreeDef] = sizeof(techTreeDef);
	(*phtObjSize)[&parse_techTreeNodeDef] = sizeof(techTreeNodeDef);
	(*phtObjSize)[&parse_techEraDef] = sizeof(techEraDef);
	(*phtObjSize)[&parse_hexTileDef] = sizeof(hexTileDef);
	(*phtObjSize)[&parse_hexBuildingDef] = sizeof(hexBuildingDef);
	(*phtObjSize)[&parse_laborSlotDef] = sizeof(laborSlotDef);
	(*phtObjSize)[&parse_hexUnitDef] = sizeof(hexUnitDef);
	(*phtObjSize)[&parse_hexAbilityDef] = sizeof(hexAbilityDef);
	(*phtObjSize)[&parse_GameTexturePortion] = sizeof(GameTexturePortion);
}
