#include "stdafx.h"
#include <hash_map>
#include "StructParse.h"
#include "EArray.h"
#include "StructParseLuabind.h"
#include "StructParse.h"
#include "UILib_h_ast.h"
#include "AutoEnums.h"

StructParseEntry parse_UIButtonDef[] = {
{_T("width"), (StructParseEntryType)0, NULL, 0, offsetof(UIButtonDef, width)},
{_T("height"), (StructParseEntryType)0, NULL, 0, offsetof(UIButtonDef, height)},
{_T("fWidthPct"), (StructParseEntryType)1, NULL, 0, offsetof(UIButtonDef, fWidthPct)},
{_T("fHeightPct"), (StructParseEntryType)1, NULL, 0, offsetof(UIButtonDef, fHeightPct)},
{_T("bCaptureMouse"), (StructParseEntryType)8, NULL, 0, offsetof(UIButtonDef, bCaptureMouse)},
{_T("eaLayers"), (StructParseEntryType)3, parse_UITextureLayer, 1, offsetof(UIButtonDef, eaLayers)},
{_T("eaChildren"), (StructParseEntryType)3, parse_UIChildDef, 1, offsetof(UIButtonDef, eaChildren)},
{_T("eType"), (StructParseEntryType)5, &htUIBoxType, 0, offsetof(UIButtonDef, eType)},
{_T("dragFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UIButtonDef, dragFunc)},
{_T("text"), (StructParseEntryType)2, NULL, 0, offsetof(UIButtonDef, text)},
{_T("textcolor"), (StructParseEntryType)10, NULL, 0, offsetof(UIButtonDef, textcolor)},
{_T("textFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UIButtonDef, textFunc)},
{_T("hFont"), (StructParseEntryType)6, NULL, 0, offsetof(UIButtonDef, hFont)},
{_T("textFlags"), (StructParseEntryType)5, &htUITextFlags, 4, offsetof(UIButtonDef, textFlags)},
{_T("clickFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UIButtonDef, clickFunc)},
{_T("rClickFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UIButtonDef, rClickFunc)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_UIButtonDef[] = {L"UIButtonDef", L"UITextDef", L"UIBoxDef", NULL};

StructParseEntry parse_UITextDef[] = {
{_T("width"), (StructParseEntryType)0, NULL, 0, offsetof(UITextDef, width)},
{_T("height"), (StructParseEntryType)0, NULL, 0, offsetof(UITextDef, height)},
{_T("fWidthPct"), (StructParseEntryType)1, NULL, 0, offsetof(UITextDef, fWidthPct)},
{_T("fHeightPct"), (StructParseEntryType)1, NULL, 0, offsetof(UITextDef, fHeightPct)},
{_T("bCaptureMouse"), (StructParseEntryType)8, NULL, 0, offsetof(UITextDef, bCaptureMouse)},
{_T("eaLayers"), (StructParseEntryType)3, parse_UITextureLayer, 1, offsetof(UITextDef, eaLayers)},
{_T("eaChildren"), (StructParseEntryType)3, parse_UIChildDef, 1, offsetof(UITextDef, eaChildren)},
{_T("eType"), (StructParseEntryType)5, &htUIBoxType, 0, offsetof(UITextDef, eType)},
{_T("dragFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UITextDef, dragFunc)},
{_T("text"), (StructParseEntryType)2, NULL, 0, offsetof(UITextDef, text)},
{_T("textcolor"), (StructParseEntryType)10, NULL, 0, offsetof(UITextDef, textcolor)},
{_T("textFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UITextDef, textFunc)},
{_T("hFont"), (StructParseEntryType)6, NULL, 0, offsetof(UITextDef, hFont)},
{_T("textFlags"), (StructParseEntryType)5, &htUITextFlags, 4, offsetof(UITextDef, textFlags)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_UITextDef[] = {L"UITextDef", L"UIBoxDef", NULL};

StructParseEntry parse_UILayoutDef[] = {
{_T("width"), (StructParseEntryType)0, NULL, 0, offsetof(UILayoutDef, width)},
{_T("height"), (StructParseEntryType)0, NULL, 0, offsetof(UILayoutDef, height)},
{_T("fWidthPct"), (StructParseEntryType)1, NULL, 0, offsetof(UILayoutDef, fWidthPct)},
{_T("fHeightPct"), (StructParseEntryType)1, NULL, 0, offsetof(UILayoutDef, fHeightPct)},
{_T("bCaptureMouse"), (StructParseEntryType)8, NULL, 0, offsetof(UILayoutDef, bCaptureMouse)},
{_T("eaLayers"), (StructParseEntryType)3, parse_UITextureLayer, 1, offsetof(UILayoutDef, eaLayers)},
{_T("eaChildren"), (StructParseEntryType)3, parse_UIChildDef, 1, offsetof(UILayoutDef, eaChildren)},
{_T("eType"), (StructParseEntryType)5, &htUIBoxType, 0, offsetof(UILayoutDef, eType)},
{_T("dragFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UILayoutDef, dragFunc)},
{_T("pointFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UILayoutDef, pointFunc)},
{_T("listFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UILayoutDef, listFunc)},
{_T("eLayoutType"), (StructParseEntryType)5, &htUILayoutType, 0, offsetof(UILayoutDef, eLayoutType)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_UILayoutDef[] = {L"UILayoutDef", L"UIBoxDef", NULL};

StructParseEntry parse_UIBoxDef[] = {
{_T("width"), (StructParseEntryType)0, NULL, 0, offsetof(UIBoxDef, width)},
{_T("height"), (StructParseEntryType)0, NULL, 0, offsetof(UIBoxDef, height)},
{_T("fWidthPct"), (StructParseEntryType)1, NULL, 0, offsetof(UIBoxDef, fWidthPct)},
{_T("fHeightPct"), (StructParseEntryType)1, NULL, 0, offsetof(UIBoxDef, fHeightPct)},
{_T("bCaptureMouse"), (StructParseEntryType)8, NULL, 0, offsetof(UIBoxDef, bCaptureMouse)},
{_T("eaLayers"), (StructParseEntryType)3, parse_UITextureLayer, 1, offsetof(UIBoxDef, eaLayers)},
{_T("eaChildren"), (StructParseEntryType)3, parse_UIChildDef, 1, offsetof(UIBoxDef, eaChildren)},
{_T("eType"), (StructParseEntryType)5, &htUIBoxType, 0, offsetof(UIBoxDef, eType)},
{_T("dragFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UIBoxDef, dragFunc)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_UIBoxDef[] = {L"UIBoxDef", NULL};

StructParseEntry parse_UIChildDef[] = {
{_T("hBox"), (StructParseEntryType)4, parse_UIBoxDef, 2, offsetof(UIChildDef, hBox)},
{_T("x"), (StructParseEntryType)0, NULL, 0, offsetof(UIChildDef, x)},
{_T("y"), (StructParseEntryType)0, NULL, 0, offsetof(UIChildDef, y)},
{_T("anchor"), (StructParseEntryType)5, &htUIAnchorFlags, 4, offsetof(UIChildDef, anchor)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_UIChildDef[] = {L"UIChildDef", NULL};

StructParseEntry parse_UITextureLayer[] = {
{_T("eLayerType"), (StructParseEntryType)5, &htUIRenderLayer, 2, offsetof(UITextureLayer, eLayerType)},
{_T("hPortion"), (StructParseEntryType)4, parse_GameTexturePortion, 0, offsetof(UITextureLayer, hPortion)},
{_T("hTex"), (StructParseEntryType)6, NULL, 0, offsetof(UITextureLayer, hTex)},
{_T("color"), (StructParseEntryType)10, NULL, 0, offsetof(UITextureLayer, color)},
{_T("textureFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UITextureLayer, textureFunc)},
{_T("renderFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UITextureLayer, renderFunc)},
{_T("barPctFunc"), (StructParseEntryType)7, NULL, 0, offsetof(UITextureLayer, barPctFunc)},
{_T("ninepatchIdx"), (StructParseEntryType)0, NULL, 0, offsetof(UITextureLayer, ninepatchIdx)},
{_T("mouseoverIdx"), (StructParseEntryType)0, NULL, 0, offsetof(UITextureLayer, mouseoverIdx)},
{_T("disabledIdx"), (StructParseEntryType)0, NULL, 0, offsetof(UITextureLayer, disabledIdx)},
{_T("pressedIdx"), (StructParseEntryType)0, NULL, 0, offsetof(UITextureLayer, pressedIdx)},
{_T("fullbarIdx"), (StructParseEntryType)0, NULL, 0, offsetof(UITextureLayer, fullbarIdx)},
{NULL, kStruct_Int, NULL, 0, 0}
};
const TCHAR* polyNames_UITextureLayer[] = {L"UITextureLayer", NULL};

