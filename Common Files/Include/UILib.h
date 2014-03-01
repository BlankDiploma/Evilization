#include "stdafx.h"
#include <hash_map>
#include "StructParse.h"
#include "EArray.h"
#include "Autogen/UILib_h_ast.h"
#include "StructParseLuabind.h"
#pragma once

AUTO_ENUM(UIRenderLayer) {kRenderLayer_Color = 0, kRenderLayer_Ninepatch, kRenderLayer_Bar, kRenderLayer_Func, kRenderLayer_TexturePortion};
AUTO_ENUM(UIBoxType) {kBox_Box = 0, kBox_Text, kBox_Button, kBox_Layout};
AUTO_ENUM(UILayoutType) {kLayout_None = 0, kLayout_LUA = 1, kLayout_Grid = 2};

AUTO_ENUM(UIAnchorFlags) {kUIAnchor_Left = 1, kUIAnchor_Top = 2, kUIAnchor_Right = 4, kUIAnchor_Bottom = 8};

class UIInstance;
struct UIBoxDef;
struct LuaScript;

PARSE_STRUCT(UITextureLayer)
{
	INLINE_ARG UIRenderLayer eLayerType;
	DEF_REF(GameTexturePortion) hPortion;
	TEXTURE_REF hTex;
	COLOR_ARGB color;
	LuaScript textureFunc;
	LuaScript renderFunc;
	LuaScript barPctFunc;
	int ninepatchIdx;
	int mouseoverIdx;
	int disabledIdx;
	int pressedIdx;
	int fullbarIdx;
};

PARSE_STRUCT(UIChildDef)
{
	INLINE_ARG DEF_REF(UIBoxDef) hBox;
	int x;
	int y;
	FLAGS UIAnchorFlags anchor;
};

PARSE_STRUCT(UIInlineChildDef)
{
	int x;
	int y;
	FLAGS UIAnchorFlags anchor;
	UIBoxDef* def;
};

PARSE_STRUCT(UIBoxDef)
{
	int width;
	int height;
	float fWidthPct;
	float fHeightPct;
	bool bCaptureMouse;
//	int flags;
	UITextureLayer** eaLayers;
	UIChildDef** eaChildren;
	UIInlineChildDef** eaInlineChildren;
	UIBoxType eType;
	LuaScript dragFunc;
	const TCHAR* name;
	const TCHAR* filename;
};

PARSE_STRUCT(UILayoutDef) : public UIBoxDef
{
	LuaScript pointFunc;
	LuaScript listFunc;
	UILayoutType eLayoutType;
	bool bInvertGridYGrowth;
};

AUTO_ENUM(UITextFlags) {kUITextFlag_CenterX = 1, kUITextFlag_CenterY = 2, kUITextFlag_Wrap = 4, kUITextFlag_DropShadow = 8};

PARSE_STRUCT(UITextDef) : public UIBoxDef
{
	const TCHAR* text;
	COLOR_ARGB textcolor;
	LuaScript textFunc;
	TEXTURE_REF hFont;
	FLAGS UITextFlags textFlags;
	float fIconScale;
};


PARSE_STRUCT(UIButtonDef) : public UITextDef
{
	LuaScript clickFunc;
	LuaScript rClickFunc;
	UIButtonDef()
	{
		bCaptureMouse = true;
	}
};

#define UISTATE_DISABLED 1 << 0
#define UISTATE_MOUSEOVER 1 << 1
#define UISTATE_LBUTTONDOWN 1 << 2
#define UISTATE_RBUTTONDOWN 1 << 4

class UIInstance
{
private:
	RECT screenRect;
	int stateFlags;
	UIBoxDef* pDef;
	UIInstance** eaChildren;
	void* pLayoutVar;
public:
	
	UIInstance();
	UIInstance(UIBoxDef* def, int w, int h);
	int GetWidth();
	int GetHeight();
	int GetFlags();
	~UIInstance();
	UIBoxDef* GetDef();
	void UpdateLayout();
	void InitChildren(int w, int h);
	void InitFromDef(UIBoxDef* def, int w, int h);
	void AnchorToRect(int x, int y, RECT* pRect, int anchor);
	void InitFromDef(UIInstance* pParent, UIChildDef* def, int w, int h);
	void InitFromInlineChildDef(UIInstance* pParent, UIInlineChildDef* def, int w, int h);
	void MoveTo(POINT pt);
	void MoveTo(int x, int y);
	void Offset(int x, int y);
	void Offset(POINT* delta);
	void Render();
	bool MouseEvent(POINT pt, UINT msg);
	bool Update(POINT ptMouse, bool bSetMouseover = true);
	RECT* GetScreenRect();
	void* GetLayoutVar();
	void SetLayoutVar(void* pVar);

};

class UserInterface
{
private:
	//sorted by z-order
	UIInstance** eaActiveWindows;
	UIInstance** eaDeletedWindows;
	int w, h;
public:
	bool MouseInput(POINT pt, UINT msg);
	bool Update(POINT ptMousePos);
	void Render();
	void Reset(int w, int h);
	void AddWindowByName(LPCTSTR name);
	void DestroyWindow(LPCTSTR name);
	UserInterface()
	{
		eaActiveWindows = eaDeletedWindows = NULL;
		w = 0;
		h = 0;
	}
};

extern UserInterface g_UI;