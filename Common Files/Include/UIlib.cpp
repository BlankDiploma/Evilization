#include "stdafx.h"
#include "UILib.h"
#include "DefLibrary.h"
#include "EArray.h"
#include "FlexRenderer.h"
#include "FlexLua.h"

UIInstance::UIInstance()
{
	pDef = NULL;
	eaChildren = NULL;
	stateFlags = 0;
	pLayoutVar = NULL;
}

UIInstance::UIInstance(UIBoxDef* def, int w, int h)
{
	pDef = NULL;
	eaChildren = NULL;
	stateFlags = 0;
	pLayoutVar = NULL;
	InitFromDef(def, w, h);
}

int UIInstance::GetWidth()
{
	return screenRect.right-screenRect.left;
}

int UIInstance::GetHeight()
{
	return screenRect.bottom-screenRect.top;
}

int UIInstance::GetFlags()
{
	return stateFlags;
}

UIInstance::~UIInstance()
{
	if (eaChildren)
		eaDestroy(&eaChildren);
}

UIBoxDef* UIInstance::GetDef()
{
	return pDef;
}

void UIInstance::UpdateLayout()
{
	LuaContext context;
	UILayoutDef* pLayoutDef = ((UILayoutDef*)pDef);
	void** eaObjects = NULL;
	context.pUI = this;
	FlexLua_ExecuteScript_EArrayReturn(&((UILayoutDef*)pDef)->listFunc, &eaObjects, &context);
	int listSize = eaSize(&eaObjects);
	int idx = 0;
	int idy = 0;
	UIBoxDef* pChildDef = GET_REF(UIBoxDef, pDef->eaChildren[0]->hBox);
	int numperrow = (int)(pChildDef->fWidthPct > 0 ? (1/pChildDef->fWidthPct) : GetWidth()/pChildDef->width);

	while (eaSize(&eaChildren) != listSize)
	{
		if (eaSize(&eaChildren) < listSize)
		{
			UIInstance* pNewChild = new UIInstance;
			pNewChild->InitFromDef(this, pDef->eaChildren[0], screenRect.right, screenRect.bottom);
			eaPush(&eaChildren, pNewChild);
		}
		else
		{
			int last = eaSize(&eaChildren)-1;
			delete eaChildren[last];
			eaRemoveFast(&eaChildren, last);
		}
	}
	if (eaObjects && pDef->eaChildren)
	{
		for (int i = 0; i < eaSize(&eaChildren); i++)
		{
			POINT pt;
			UIBoxDef* pChildDef = GET_REF(UIBoxDef, pDef->eaChildren[0]->hBox);
			eaChildren[i]->SetLayoutVar(eaObjects[i]);
			context.pUI = eaChildren[i];

			if (pLayoutDef->eLayoutType == kLayout_LUA) 
			{
				pt = FlexLua_ExecuteScript_PointReturn(&((UILayoutDef*)pDef)->pointFunc, &context);
				//pt = temp.GetPoint();
			}
			else if (pLayoutDef->eLayoutType == kLayout_Grid)
			{
				if (pChildDef->fWidthPct > 0)
					pt.x = (LONG)(idx * pChildDef->fWidthPct*screenRect.right-screenRect.left);
				else
					pt.x = idx * pChildDef->width;

				if (pChildDef->fHeightPct > 0)
					pt.y = (LONG)(idy * pChildDef->fHeightPct*screenRect.bottom-screenRect.top);
				else
					pt.y = idy * pChildDef->height;

				if (pLayoutDef->bInvertGridYGrowth)
					pt.y = -pt.y;
				idx++;
				if (idx >= numperrow)
				{
					idx = 0;
					idy++;
				}
			}
			pt.x += pDef->eaChildren[0]->x;
			pt.y += pDef->eaChildren[0]->y;
			eaChildren[i]->AnchorToRect(pt.x, pt.y, &screenRect, pDef->eaChildren[0]->anchor);
		}
	}
	if (eaObjects)
		eaDestroy(&eaObjects);
}

void UIInstance::InitChildren(int w, int h)
{
	for (int i = 0; i < eaSize(&pDef->eaChildren); i++)
	{
		UIInstance* pChild = new UIInstance;
		pChild->InitFromDef(this, pDef->eaChildren[i], w, h);
		eaPush(&eaChildren, pChild);
	}
	for (int i = 0; i < eaSize(&pDef->eaInlineChildren); i++)
	{
		UIInstance* pChild = new UIInstance;
		pChild->InitFromInlineChildDef(this, pDef->eaInlineChildren[i], w, h);
		eaPush(&eaChildren, pChild);
	}
}

void UIInstance::InitFromDef(UIBoxDef* def, int w, int h)
{
	pDef = def;
	SetRect(&screenRect, 0, 0, 0, 0);
	if (pDef->fWidthPct > 0)
		screenRect.right = (LONG)(w * pDef->fWidthPct);
	else
		screenRect.right = pDef->width;
	if (pDef->fHeightPct > 0)
		screenRect.bottom = (LONG)(h * pDef->fHeightPct);
	else
		screenRect.bottom = pDef->height;

	InitChildren(w, h);
}

void UIInstance::AnchorToRect(int x, int y, RECT* pRect, int anchor)
{
	MoveTo(0, 0);

	if (anchor & kUIAnchor_Left)
	{
		Offset(pRect->left, 0);
	}
	else if (anchor & kUIAnchor_Right)
	{
		Offset(pRect->right-(screenRect.right-screenRect.left), 0);
	}
	else
	{
		Offset(pRect->left + (pRect->right-pRect->left)/2 - (screenRect.right-screenRect.left)/2, 0);
	}
	if (anchor & kUIAnchor_Top)
	{
		Offset(0, pRect->top);
	}
	else if (anchor & kUIAnchor_Bottom)
	{
		Offset(0, pRect->bottom-(screenRect.bottom-screenRect.top));
	}
	else
	{
		Offset(0, pRect->top + (pRect->bottom-pRect->top)/2 - (screenRect.bottom-screenRect.top)/2);
	}
	Offset(x, y);
}

void UIInstance::InitFromInlineChildDef(UIInstance* pParent, UIInlineChildDef* pInlineDef, int w, int h)
{
	pDef = pInlineDef->def;
	SetRect(&screenRect, 0, 0, 0, 0);

	if (pDef->fWidthPct > 0)
		screenRect.right = (LONG)(pParent->screenRect.right * pDef->fWidthPct);
	else
		screenRect.right = pDef->width;
	if (pDef->fHeightPct > 0)
		screenRect.bottom = (LONG)(pParent->screenRect.bottom * pDef->fHeightPct);
	else
		screenRect.bottom = pDef->height;

	AnchorToRect(pInlineDef->x, pInlineDef->y, &pParent->screenRect, pInlineDef->anchor);

	InitChildren(w, h);
}

void UIInstance::InitFromDef(UIInstance* pParent, UIChildDef* def, int w, int h)
{
	pDef = GET_REF(UIBoxDef, def->hBox);
	SetRect(&screenRect, 0, 0, 0, 0);

	if (pDef->fWidthPct > 0)
		screenRect.right = (LONG)(pParent->screenRect.right * pDef->fWidthPct);
	else
		screenRect.right = pDef->width;
	if (pDef->fHeightPct > 0)
		screenRect.bottom = (LONG)(pParent->screenRect.bottom * pDef->fHeightPct);
	else
		screenRect.bottom = pDef->height;

	AnchorToRect(def->x, def->y, &pParent->screenRect, def->anchor);

	InitChildren(w, h);
}

void UIInstance::MoveTo(POINT pt)
{
	pt.x -= screenRect.left;
	pt.y -= screenRect.top;
	Offset(&pt);
}

void UIInstance::MoveTo(int x, int y)
{
	x -= screenRect.left;
	y -= screenRect.top;
	Offset(x, y);
}

void UIInstance::Offset(int x, int y)
{
	OffsetRect(&screenRect, x, y);
	for (int i = 0; i < eaSize(&eaChildren); i++)
	{
		eaChildren[i]->Offset(x, y);
	}
}

void UIInstance::Offset(POINT* delta)
{
	Offset(delta->x, delta->y);
}

void UIInstance::Render()
{
	LuaContext context;
	context.pUI = this;
	for (int i = 0; i < eaSize(&pDef->eaLayers); i++)
	{
		UITextureLayer* pLayer = pDef->eaLayers[i];
		switch (pLayer->eLayerType)
		{
		case kRenderLayer_Ninepatch:
			{
				g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->ninepatchIdx, &screenRect, 1.0f);

				if (pDef->eType == kBox_Button)
				{
					UIButtonDef* pButtonDef = (UIButtonDef*)pDef;
				
					if (stateFlags & UISTATE_DISABLED)
						g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->disabledIdx, &screenRect, 1.0f);
					else if (stateFlags & UISTATE_LBUTTONDOWN)
						g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->pressedIdx, &screenRect, 1.0f);
					else if (stateFlags & UISTATE_MOUSEOVER)
						g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->mouseoverIdx, &screenRect, 1.0f);
					else
						g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->ninepatchIdx, &screenRect, 1.0f);
					
				}

			}break;
		case kRenderLayer_Func:
			{
				FlexLua_ExecuteScript_NoReturn(&pLayer->renderFunc, &context);
			}break;
		case kRenderLayer_Color:
			{
	//			DrawFilledRect(g_pMainDevice, &screenRect, pDef->color);
			}break;
		case kRenderLayer_Bar:
			{
				double pct = FlexLua_ExecuteScript_DoubleReturn(&pLayer->barPctFunc, &context);
				g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->ninepatchIdx, &screenRect, 1.0f);
				if (pct > 0)
					g_Renderer.AddNinepatchToRenderList(pLayer->hTex.pTex, pLayer->fullbarIdx, &screenRect, (float)pct);
			}break;
		case kRenderLayer_TexturePortion:
			{
				if (pLayer->hPortion.pObj)
				{
					POINT pt = {screenRect.left, screenRect.top};
					g_Renderer.AddSpriteToRenderList((GameTexturePortion*)pLayer->hPortion.pObj, pt);
				}
				else
				{
					POINT pt = {screenRect.left, screenRect.top};
					g_pCurContext = &context;
					GameTexturePortion* pPortion = luabind::call_function<GameTexturePortion*>(*pLayer->textureFunc.func);
					if (pPortion)
						g_Renderer.AddSpriteToRenderList(pPortion, pt);
				}
			}break;
		}
	}


	if (pDef->eType == kBox_Text || pDef->eType == kBox_Button)
	{
		UITextDef* pTextDef = (UITextDef*)pDef;
		int x, y;
		int numlines = 1;
		const TCHAR* pchText = NULL;
		bool bOwned = false;
		if (pTextDef->text)
			pchText = pTextDef->text;
		else if (pTextDef->textFunc.func)
		{
			pchText = FlexLua_ExecuteScript_StringReturn(&pTextDef->textFunc, &context);
			bOwned = true;
		}
		if (pchText)
		{
			if (pTextDef->textFlags & kUITextFlag_CenterX)
				x = (screenRect.right+screenRect.left)/2 + ((stateFlags & UISTATE_LBUTTONDOWN) ? 1 : 0);
			else
				x = screenRect.left + ((stateFlags & UISTATE_LBUTTONDOWN) ? 1 : 0);

			if (pTextDef->textFlags & kUITextFlag_CenterY)
			{
				int idx = 0;
				while (wcschr(pchText, '\n'))
					numlines++;
				y = (screenRect.bottom+screenRect.top)/2 - pTextDef->hFont.pTex->letterHeight*numlines/2 + ((stateFlags & UISTATE_LBUTTONDOWN) ? 1 : 0);
			}
			else
				y = screenRect.top + ((stateFlags & UISTATE_LBUTTONDOWN) ? 1 : 0);
			g_Renderer.AddStringToRenderList(pTextDef->hFont.pTex, pchText, (float)x, (float)y, pTextDef->textcolor, !!(pTextDef->textFlags & kUITextFlag_CenterX), false, !!(pTextDef->textFlags & kUITextFlag_DropShadow), pTextDef->fIconScale > 0 ? pTextDef->fIconScale : 1.0f); 

			if (bOwned)
				free((TCHAR*)pchText);
		}

	}
	for (int i = 0; i < eaSize(&eaChildren); i++)
	{
		eaChildren[i]->Render();
	}
}

bool UIInstance::MouseEvent(POINT pt, UINT msg)
{
	bool bRet = false;
	for (int i = 0; i < eaSize(&eaChildren); i++)
	{
		bRet |= eaChildren[i]->MouseEvent(pt, msg);
	}
	LuaContext context;
	context.pUI = this;
	context.mousePt = pt;
	if (pDef->bCaptureMouse || pDef->eType == kBox_Button)
	{
		if (PtInRect(&screenRect, pt))
		{
			if (!(stateFlags & UISTATE_DISABLED))
			{
				if (msg == WM_LBUTTONUP && (stateFlags & UISTATE_LBUTTONDOWN))
				{
					if (pDef->eType == kBox_Button)
					{
						UIButtonDef* pButton = (UIButtonDef*)pDef;
						if (pButton->clickFunc.func)
							FlexLua_ExecuteScript_NoReturn(&pButton->clickFunc, &context);
					}
					stateFlags &= ~UISTATE_LBUTTONDOWN;
				}
				else if (msg == WM_LBUTTONDOWN)
				{
					stateFlags |= UISTATE_LBUTTONDOWN;
				}
				else if (msg == WM_RBUTTONUP && (stateFlags & UISTATE_RBUTTONDOWN))
				{
					if (pDef->eType == kBox_Button)
					{
						UIButtonDef* pButton = (UIButtonDef*)pDef;
						if (pButton->rClickFunc.func)
							FlexLua_ExecuteScript_NoReturn(&pButton->rClickFunc, &context);
					}

					stateFlags &= ~UISTATE_RBUTTONDOWN;
				}
				else if (msg == WM_RBUTTONDOWN)
				{
					stateFlags |= UISTATE_RBUTTONDOWN;
				}
			}
			return true;
		}
	}

	return bRet;
}

bool UIInstance::Update(POINT ptMouse, bool bSetMouseover)
{
	bool bRet = false;
	LuaContext context;
	for (int i = 0; i < eaSize(&eaChildren); i++)
	{
		bRet |= eaChildren[i]->Update(ptMouse, !bRet);
		bSetMouseover = !bRet;
	}
	
	context.pUI = this;

	if (pDef->eType == kBox_Layout)
		UpdateLayout();
	if (bSetMouseover && eaSize(&pDef->eaLayers) > 0 && PtInRect(&screenRect, ptMouse))
	{
		stateFlags |= UISTATE_MOUSEOVER;
		if (stateFlags & UISTATE_LBUTTONDOWN && pDef->dragFunc.func)
		{
			FlexLua_ExecuteScript_NoReturn(&pDef->dragFunc, &context);
		}
		bRet = true;
	}
	else
	{
		stateFlags &= ~UISTATE_MOUSEOVER;
	}
	return bRet;
}

RECT* UIInstance::GetScreenRect()
{
	return &screenRect;
}

void* UIInstance::GetLayoutVar()
{
	return pLayoutVar;
}

void UIInstance::SetLayoutVar(void* pVar)
{
	for (int i = 0; i < eaSize(&eaChildren); i++)
	{
		eaChildren[i]->SetLayoutVar(pVar);
	}
	pLayoutVar = pVar;
}


/*****************************************************************

						UserInterface

******************************************************************/

bool UserInterface::MouseInput(POINT pt, UINT msg)
{
	for (int i = 0; i < eaSize(&eaActiveWindows); i++)
	{
		if (eaActiveWindows[i]->MouseEvent(pt, msg))
			return true;
	}
	return false;
}
bool UserInterface::Update(POINT ptMousePos)
{
	if (eaDeletedWindows)
	{
		for (int i = 0; i < eaSize(&eaDeletedWindows); i++)
		{
			delete eaDeletedWindows[i];
		}
		eaDestroy(&eaDeletedWindows);
	}

	bool bRet = false;
	for (int i = 0; i < eaSize(&eaActiveWindows); i++)
	{
		bRet |= eaActiveWindows[i]->Update(ptMousePos);
	}
	return bRet;
}
void UserInterface::Render()
{
	bool bRet = false;
	for (int i = 0; i < eaSize(&eaActiveWindows); i++)
	{
		eaActiveWindows[i]->Render();
	}
}
void UserInterface::Reset(int w, int h)
{
	eaDeletedWindows = eaActiveWindows;
	eaActiveWindows = NULL;
	this->w = w;
	this->h = h;
}
void UserInterface::AddWindowByName(LPCTSTR name)
{
	UIBoxDef* pDef = GET_DEF_FROM_STRING(UIBoxDef, name);
	if (pDef)
	{
		eaPush(&eaActiveWindows, new UIInstance(pDef, w, h));
	}
}
void UserInterface::DestroyWindow(LPCTSTR name)
{
	UIBoxDef* pDef = GET_DEF_FROM_STRING(UIBoxDef, name);

	//iterate backwards so removing one doesn't fuck everything up
	for (int i = eaSize(&eaActiveWindows)-1; i >= 0 ; i--)
	{
		if (eaActiveWindows[i]->GetDef() == pDef)
		{
			delete eaActiveWindows[i];
			eaRemoveFast(&eaActiveWindows, i);
		}
	}
}

UserInterface g_UI;

#include "Autogen/UILib_h_ast.cpp"