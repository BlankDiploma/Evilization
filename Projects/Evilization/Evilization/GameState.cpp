#include "stdafx.h"
#include "GameState.h"
#include "HexMap.h"
#include "assert.h"
#include "techtree.h"
#include "texturelibrary.h"
int g_HexSize;


CGameState::CGameState()
{
	pPlayers = NULL;
	fpMapOffset.x = 16;
	fpMapOffset.y = 8;
	mapViewport.left = mapViewport.top = mapViewport.right = mapViewport.bottom = 0;
	pFont = NULL;
	pCurrentMap = NULL;
	eCurState = kGameState_Invalid;
	g_HexSize = DEFAULT_HEX_SIZE;
	eaMouseStates = NULL;
}

void CGameState::Initialize(int screenW, int screenH)
{
	pFont = GET_TEXTURE(_T("courier_new"));
	pInterface = GET_TEXTURE(_T("interface"));
	pNines = GET_TEXTURE(_T("interfacenines"));
	g_pEvilTechTree = GET_DEF_FROM_STRING(techTreeDef, L"Evil");
	g_pEvilTechTree->eaNodes = NULL;
	for (int i = 0; i < eaSize(&g_pEvilTechTree->eaNames); i++)
	{
		techTreeNodeDef* pNode = GET_DEF_FROM_STRING(techTreeNodeDef, g_pEvilTechTree->eaNames[i]);
		if (pNode)
		{
			pNode->layoutPt.x = -1;
			eaPush(&g_pEvilTechTree->eaNodes, pNode);
		}
	}
	AssignGridPositions(g_pEvilTechTree);
	SwitchToState(kGameState_MainMenu);
	this->screenW = screenW;
	this->screenH = screenH;
	mapViewport.right = screenW;
	mapViewport.bottom = screenH;
	minimapTexture.width = 256;
	minimapTexture.height = 256;
	g_Renderer.GetD3DDevice()->CreateTexture(256, 256, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &minimapTexture.pD3DTex, NULL);
}

void CGameState::Update(DWORD tick)
{
	switch (eCurState)
	{
	case kGameState_MainMenu:
		{
		}break;
	case kGameState_Gameplay:
		{
			for (int i = 0; i < iNumPlayers; i++)
			{
				pPlayers[i].OncePerFrame();
			}
			//all gameplay updates should occur before UI
			POINT ptMouse;
			RECT window = {0,0,screenW, screenH};
			GetCursorPos(&ptMouse);
			ScreenToClient(hWndMain, &ptMouse);
			ptMousePosLastFrame = ptMousePos;
			ptMousePos = ptMouse;
			if (!UI.Update(ptMousePos) && PtInRect(&window, ptMousePos))
			{
				bMouseOverGameplay = true;
			}
			else
			{
				bMouseOverGameplay = false;
			}
		}break;
	}
}

static POINT points[32];
static int num = 0;

void CGameState::IssueOrder(unitOrderType eType, HEXPATH* pPath)
{
	if (curSelUnit)
		curSelUnit->OverwriteQueuedOrders(eType, pPath);
}

void CGameState::DoGameplayMouseInput_Default(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
			SelectUnitOnTile(PixelToTilePt(pt.x, pt.y));
		}break;
	case WM_RBUTTONUP:
		{
		}break;
	case WM_MOUSEWHEEL:
		{
			int rot = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			AdjustMapZoom(rot);
		}break;
	}
}

void CGameState::DoGameplayMouseInput_UnitSelected(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
			SelectUnitOnTile(PixelToTilePt(pt.x, pt.y));
		}break;
	case WM_RBUTTONUP:
		{
			HEXPATH* pPath = NULL;
			pCurrentMap->HexPathfindTile(curSelUnit, curSelUnit->GetLoc(),PixelToTilePt(pt.x, pt.y),&pPath);
			if (pPath)
				IssueOrder(kOrder_Move, pPath);
		}break;
	case WM_MOUSEWHEEL:
		{
			int rot = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			AdjustMapZoom(rot);
		}break;
	}
}

void CGameState::DoGameplayMouseInput_CityView(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
		}break;
	case WM_RBUTTONUP:
		{
			hexTile* pTile = pCurrentMap->GetTile(PixelToTilePt(pt.x, pt.y));
			curSelCity->StartLaborOnTile(pTile);
		}break;
	case WM_MOUSEWHEEL:
		{
			int rot = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			AdjustMapZoom(rot);
		}break;
	}
}

void CGameState::DoGameplayMouseInput_PlaceBuilding(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
			cityProject* pProj = (cityProject*)pHandlerParam;
			if (pProj)
			{
				if (pProj->eType == kProject_Building && !pCurrentMap->BuildingCanBeBuiltOnTile((hexBuildingDef*)pProj->pDef, PixelToTilePt(pt.x, pt.y)))
				{
					MouseHandlerPopState();
					return;
				}

				if (!GetAsyncKeyState(VK_SHIFT))
				{
					curSelCity->ClearProductionQueue();
					MouseHandlerPopState();
				}
				curSelCity->AddQueuedProject(pProj->eType,pProj->pDef, PixelToTilePt(pt.x, pt.y));
			}
		}break;
	case WM_RBUTTONUP:
		{
				MouseHandlerPopState();
		}break;
	case WM_MOUSEWHEEL:
		{
			int rot = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			AdjustMapZoom(rot);
		}break;
	}
}

void CGameState::DoGameplayMouseInput_SelectAbilityTarget(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam)
{
}

void CGameState::GameplayWindowMouseInput(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam)
{
	MouseHandlerState* pCurHandler = eaMouseStates[eaSize(&eaMouseStates)-1];
	switch(pCurHandler->eMouseHandler)
	{
	case kGameplayMouse_Default:
		{
			DoGameplayMouseInput_Default(msg, pt, wParam, lParam, pCurHandler->pMouseHandlerParam);
		}break;
	case kGameplayMouse_UnitSelected:
		{
			DoGameplayMouseInput_UnitSelected(msg, pt, wParam, lParam, pCurHandler->pMouseHandlerParam);
		}break;
	case kGameplayMouse_CityView:
		{
			DoGameplayMouseInput_CityView(msg, pt, wParam, lParam, pCurHandler->pMouseHandlerParam);
		}break;
	case kGameplayMouse_PlaceBuilding:
		{
			DoGameplayMouseInput_PlaceBuilding(msg, pt, wParam, lParam, pCurHandler->pMouseHandlerParam);
		}break;
	case kGameplayMouse_SelectAbilityTarget:
		{
			DoGameplayMouseInput_SelectAbilityTarget(msg, pt, wParam, lParam, pCurHandler->pMouseHandlerParam);
		}break;
	}
	//old handler
	/*
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
			SelectUnitOnTile(PixelToTilePt(pt.x, pt.y));
			//uncomment for path demo
//			points[num++] = PixelToTilePt(pt.x, pt.y);
		}break;
	case WM_RBUTTONUP:
		{
			if (curSelUnit)
			{
				HEXPATH* pPath = NULL;
				pCurrentMap->HexPathfindTile(curSelUnit, curSelUnit->GetLoc(),PixelToTilePt(pt.x, pt.y),&pPath);
				if (pPath)
					IssueOrder(kOrder_Move, pPath);
			}
			else if (curSelCity)
			{
				hexTile* pTile = pCurrentMap->GetTile(PixelToTilePt(pt.x, pt.y));
				curSelCity->StartLaborOnTile(pTile);
			}
		}break;
	case WM_MOUSEWHEEL:
		{
			int rot = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			AdjustMapZoom(rot);
		}break;
		//uncomment to make l/r buttons place units/buildings
		
	case WM_LBUTTONUP:
		{
			hexTile* pTile = pCurrentMap->GetTile(PixelToTilePt(pt.x, pt.y));
			pTile->pUnit = new hexUnit;
		}break;
	case WM_RBUTTONUP:
		{
			hexTile* pTile = pCurrentMap->GetTile(PixelToTilePt(pt.x, pt.y));
			pTile->pBuilding = new hexBuilding;
		}break;
		
	}
*/
}

void CGameState::MouseHandlerAdditionalRendering()
{
	MouseHandlerState* pCurHandler = eaMouseStates[eaSize(&eaMouseStates)-1];
	switch(pCurHandler->eMouseHandler)
	{
	case kGameplayMouse_Default:
		{
		}break;
	case kGameplayMouse_UnitSelected:
		{
			hexUnitOrder* pTopOrder = curSelUnit->GetTopQueuedOrder();
			if (pTopOrder && (pTopOrder->eType == kOrder_Move || pTopOrder->eType == kOrder_AutoExplore))
				pCurrentMap->RenderPath(&mapViewport, fpMapOffset, curSelUnit, pTopOrder->pPath, 0x55);

			HEXPATH* pPath = NULL;
			if (PtInRect(&mapViewport, ptMousePos) && bMouseOverGameplay)
			{
				int iPathLength = pCurrentMap->HexPathfindTile(curSelUnit, curSelUnit->GetLoc(),PixelToTilePt(ptMousePos.x, ptMousePos.y),&pPath);
				if (iPathLength > 0)
				{
					pCurrentMap->RenderPath(&mapViewport, fpMapOffset, curSelUnit, pPath, 0xff);
				}
			}
		}break;
	case kGameplayMouse_CityView:
		{
		}break;
	case kGameplayMouse_PlaceBuilding:
		{
			cityProject* pProj = (cityProject*)pCurHandler->pMouseHandlerParam;
			hexBuildingDef* pDef = (hexBuildingDef*)pProj->pDef;
			DWORD color = 0xaaffffff;
			
			if (pProj->eType == kProject_Building && !pCurrentMap->BuildingCanBeBuiltOnTile((hexBuildingDef*)pProj->pDef, PixelToTilePt(ptMousePos.x, ptMousePos.y)))
			{
				color = 0xaaff0000;
			}
			pCurrentMap->RenderBuildingOnTile(pDef, PixelToTilePt(ptMousePos.x, ptMousePos.y), color, fpMapOffset);
		}break;
	case kGameplayMouse_SelectAbilityTarget:
		{
		}break;
	}
}

void CGameState::Render()
{
	switch (eCurState)
	{
	case kGameState_MainMenu:
		{
			RenderMainMenu();
		}break;
	case kGameState_Gameplay:
		{
			pCurrentMap->Render(&mapViewport, fpMapOffset, &pPlayers[iCurPlayer]);
			if (PtInRect(&mapViewport, ptMousePos) && bMouseOverGameplay)
			{
				MouseHandlerAdditionalRendering();
				pCurrentMap->RenderInterface(&mapViewport, fpMapOffset, PixelToTilePt(ptMousePos.x, ptMousePos.y));
			}

			UI.Render();
		}break;
	}
}

inline void findCity(hexTile* pTile, void* pOwnerID, void*** peaOut)
{
	CHexBuilding* pBuilding = pTile ? pTile->pBuilding : NULL;
	if (pBuilding && pBuilding->GetMyType() == kBuilding_City && (!pOwnerID || (pOwnerID && pBuilding->ownerID == *((int*)pOwnerID))))
	{
		eaPush(peaOut, pTile);
	}
}

inline void findLabor(hexTile* pTile, void* pOwner, void*** peaOut)
{
	CHexBuilding* pBuilding = pTile ? pTile->pBuilding : NULL;
	if (pBuilding && !pBuilding->GetMyType() == kBuilding_City && pBuilding->pDef->eaSlotDefList)
	{
		for (int i = 0; i < eaSize(&pBuilding->eaLaborSlots); i++)
		{
			eaPush(peaOut, pBuilding->eaLaborSlots[i]);
		}

	}
	else if (pTile->slot.pLaborOwner && pTile->slot.pLaborOwner == pOwner)
	{
			eaPush(peaOut, &pTile->slot);
	}
}

void CGameState::GetOnscreenCityList(hexTile*** peaTilesOut, bool bOwned)
{
	pCurrentMap->GetMatchingOnscreenTiles(&mapViewport, fpMapOffset, (void***)peaTilesOut, findCity, bOwned ? &iCurPlayer : NULL);
}

void CGameState::GetOnscreenLaborList(laborSlot*** peaLaborOut, CHexCity* pCity)
{
	pCurrentMap->GetMatchingOnscreenTiles(&mapViewport, fpMapOffset, (void***)peaLaborOut, findLabor, pCity);
}

void CGameState::RenderMainMenu()
{
	RECT button = {-64, -16, 64, 16};
	OffsetRect(&button, screenW/2, screenH/2);

	OffsetRect(&button, 0, -20);
	g_Renderer.AddSolidColorToRenderList(&button, 0xff333333);
	g_Renderer.AddStringToRenderList(pFont, _T("New game"),(button.right+button.left)/2, (button.bottom+button.top)/2 - 6, 0xffff0000, true, false, false);
	OffsetRect(&button, 0, 40);
	g_Renderer.AddSolidColorToRenderList(&button, 0xff333333);
	g_Renderer.AddStringToRenderList(pFont, _T("Exit"), (button.right+button.left)/2, (button.bottom+button.top)/2 - 6, 0xffff0000, true, false, false);
}

void CGameState::SwitchToState(GameState newState)
{
	switch (newState)
	{
	case kGameState_MainMenu:
		{
		}break;
	case kGameState_Gameplay:
		{
			assert(pCurrentMap);//must have a map to transition to gameplay state
			eaPush(&eaMouseStates, new MouseHandlerState);
		}break;
	}
	eCurState = newState;
}

void CGameState::CenterView(POINT pt)
{
	fpMapOffset.x = (pt.x*HEX_SIZE - mapViewport.right/2);
	fpMapOffset.y = (pt.y*HEX_SIZE*3/4 - mapViewport.bottom/2);
}

POINT CGameState::GetViewCenter()
{
	POINT pt = {(fpMapOffset.x + mapViewport.right/2)/HEX_SIZE, (fpMapOffset.y + mapViewport.bottom/2)/(HEX_SIZE*3/4)};
	return pt;
}

void CGameState::MouseInput(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam)
{
	switch (eCurState)
	{
	case kGameState_MainMenu:
		{
			switch (msg)
			{
			case WM_LBUTTONUP:
				{
					RECT button = {-64, -16, 64, 16};
					OffsetRect(&button, screenW/2, screenH/2);

					OffsetRect(&button, 0, -20);
					if (PtInRect(&button, pt))
					{
						StartNewGame();
					}
					OffsetRect(&button, 0, 40);
					if (PtInRect(&button, pt))
					{
						PostQuitMessage(1);
					}
				}break;
			}
		}break;
	case kGameState_Gameplay:
		{
			bool bPrevAutoCancel = eaMouseStates[eaSize(&eaMouseStates)-1]->bPopOnUIClick;
			if (UI.MouseInput(pt, msg))
			{
				if (bPrevAutoCancel && eaMouseStates[eaSize(&eaMouseStates)-1]->bPopOnUIClick)
					MouseHandlerPopState();
			}
			else if (PtInRect(&mapViewport, pt))
			{
				GameplayWindowMouseInput(msg, pt, wParam, lParam);
			}

		}break;
	}
}

POINT CGameState::TilePtToScreenPt(int x, int y)
{
	return TileToScreen(x, y, fpMapOffset);
}

POINT CGameState::PixelToTilePt(int x, int y)
{
	POINT box;
	bool bNegative = x+fpMapOffset.x < 0;
	box.y = (y+fpMapOffset.y)/(HEX_SIZE*3/4 + 1);
	box.x = (x+fpMapOffset.x)/(HEX_SIZE) - ((box.y % 2) ? 0.5 : 0);
	y = (int)(y+fpMapOffset.y) % (HEX_SIZE*3/4 + 1);
	x = ((int)(x+fpMapOffset.x - ((box.y % 2) ? HEX_SIZE/2 : 0)) % HEX_SIZE);
	if (bNegative)
	{
		box.x -= 1;
		x+= HEX_SIZE;
	}
	if (y < HEX_SIZE/4)
	{
		if (y < -0.5*x + (HEX_SIZE/4-1))
		{
			//upper left
			box.y--;
			if (box.y % 2)
				box.x--;
		}
		else if (y < 0.5*x + -(HEX_SIZE/4+1))
		{
			//upper right
			box.y--;
			if (!(box.y % 2))
				box.x++;
		}
	}
	return box;
}

void CGameState::KeyInput(int keyCode, bool bDown)
{
}

void CGameState::EndCurrentGame()
{
	delete pCurrentMap;
	pCurrentMap = NULL;
	SwitchToState(kGameState_MainMenu);
}


void CGameState::StartNewGame()
{
	assert(!pCurrentMap);

	if (!pCurrentMap)
		pCurrentMap = new CHexMap;
	pCurrentMap->Generate(128,128, GetTickCount());

	SwitchToState(kGameState_Gameplay);
	UI.Reset(screenW, screenH);
	UI.AddWindowByName(_T("gameplay"));
	iNumPlayers = 4;
	pPlayers = new CHexPlayer[iNumPlayers];
	for (int i = 0; i < iNumPlayers; i++)
	{
		TCHAR buf[64];
		wsprintf(buf, L"Player %i", i+1);
		pPlayers[i].Init(i, buf, 0xffffffff, 0xff000000, pCurrentMap);
		pCurrentMap->PlacePlayerStart(&pPlayers[i]);
		pPlayers[i].RefreshVisibility(pCurrentMap);
	}
	pPlayers[0].SetType(kPlayerControl_Human_Local);
	StartPlayerTurn(0);
}

void CGameState::StartPlayerTurn(int idx)
{
	CHexPlayer* pPlayer = &pPlayers[idx];
	iCurPlayer = idx;

	pPlayer->StartTurn();

	switch (pPlayer->GetType())
	{
	case kPlayerControl_AI_Local:
		{
			EndCurrentTurn();
		}break;
	case kPlayerControl_Human_Local:
		{
			SelectNextUnit(pPlayer);
		}break;
	case kPlayerControl_AI_Network:
	case kPlayerControl_Human_Network:
		{
			EndCurrentTurn();
		}break;
	}

}

void CGameState::ExecuteQueuedActions()
{
	CHexPlayer* pPlayer = &pPlayers[iCurPlayer];
	for (int i = 0; i < eaSize(&pPlayer->eaUnits); i++)
	{
		hexUnitOrder* pOrder = pPlayer->eaUnits[i]->GetTopQueuedOrder();
		while (pOrder && pPlayer->eaUnits[i]->GetMovRemaining() > 0)
		{
			if (pCurrentMap->ProcessOrder(pPlayer->eaUnits[i], pOrder, pPlayer))
			{
				pPlayer->eaUnits[i]->PopQueuedOrder();
			}
			pOrder = pPlayer->eaUnits[i]->GetTopQueuedOrder();
			pPlayer->RefreshVisibility(pCurrentMap);
		}
	}
}

void CGameState::EndCurrentTurn()
{
	ExecuteQueuedActions();

	iCurPlayer++;
	iCurPlayer = iCurPlayer % iNumPlayers;
	StartPlayerTurn(iCurPlayer);
}

void CGameState::SelectNextUnit( CHexPlayer* pPlayer )
{
	for (int i = 0; i < eaSize(&pPlayer->eaUnits); i++)
	{
		if (!pPlayer->eaUnits[i]->GetTopQueuedOrder())
		{
			SelectUnit(pPlayer->eaUnits[i]);
			CenterView(pPlayer->eaUnits[i]->GetLoc());
			break;
		}
	}
}

void CGameState::RenderMinimap(RECT* uiRect)
{
	pCurrentMap->UpdateMinimapTexture(&minimapTexture, &mapViewport, fpMapOffset, FOG_OF_WAR ? pPlayers[iCurPlayer].GetVisibility() : NULL);

	g_Renderer.AddSolidColorToRenderList(uiRect, 0xff111111);

	g_Renderer.AddSpriteToRenderList(&minimapTexture,uiRect, NULL);
}

void CGameState::RenderSelectedUnit(POINT screenPt, float scale)
{
	if (curSelUnit)
	{
		hexTile* pTile = pCurrentMap->GetTile(curSelUnit->GetLoc());
		pCurrentMap->RenderTile(screenPt, pTile, 0xFFFFFFFF, scale);
	}
}

CHexUnit* CGameState::GetSelectedUnit()
{
	return curSelUnit;
}

void CGameState::SelectCity(CHexCity* pCity)
{
	 curSelCity = pCity;
	 MouseHandlerPushState(kGameplayMouse_CityView, NULL);
}

CHexCity* CGameState::GetSelectedCity()
{
	return curSelCity;
}

void CGameState::ShowDetailedCityView( CHexCity* pCity )
{
	SelectCity(pCity);
	CenterView(pCity->GetLoc());
	UI.Reset(screenW, screenH);
	UI.AddWindowByName(_T("DetailedCityView"));
}

void CGameState::ShowGameplayUI()
{
	UI.Reset(screenW, screenH);
	UI.AddWindowByName(_T("gameplay"));
}

void CGameState::ShowTechTreeUI()
{
	g_GameState.MouseHandlerPushState(kGameplayMouse_Disable, NULL);
	UI.Reset(screenW, screenH);
	UI.AddWindowByName(_T("TechTree"));
}

void CGameState::AdjustMapZoom( int rot )
{
	POINT pt = GetViewCenter();
	for (int i = 0; i < abs(rot); i++)
	{
		if (rot > 0)
		{
			if (HEX_SIZE < 128)
				HEX_SIZE *= 2;
			else
				break;
		}
		else 
		{
			if (HEX_SIZE > 32)
				HEX_SIZE /= 2;
			else
				break;
		}
	}
	CenterView(pt);
}

CHexPlayer* CGameState::GetCurrentPlayer()
{
	return &pPlayers[iCurPlayer];
}

int CHexPlayer::GetTechProgress( LPCTSTR name )
{
	StringIntHash::iterator pIter;
	pIter = techProgressHash.find(name);
	if (pIter != techProgressHash.end())
	{
		return pIter->second;
	}
	return 0;
}

float CHexPlayer::GetTechPct(  )
{
	if (!pCurResearch)
		return 0.0;
	StringIntHash::iterator pIter;
	pIter = techProgressHash.find(pCurResearch->name);
	if (pIter != techProgressHash.end())
	{
		return ((float)pIter->second)/pCurResearch->cost;
	}
	return 0;
}

//returns -1 if research isn't finished, otherwise returns overflow amount
//when research is finished, progress is set to -1
int CHexPlayer::AddResearch( int res )
{
	if (!pCurResearch)
		return res;

	StringIntHash::iterator pIter;
	pIter = techProgressHash.find(pCurResearch->name);
	if (pIter != techProgressHash.end())
	{
		if (pIter->second + res >= pCurResearch->cost)
		{
			//research complete
			for(int i = 0; i < eaSize(&pCurResearch->eaGrants); i++)
			{
				buildPermissions[pCurResearch->eaGrants[i]] = 1;
			}
			int overflow = (pIter->second + res) - pCurResearch->cost;
			pIter->second = -1;
			pCurResearch = NULL;
			AddNotification(kNotify_Tech, NULL, NULL, NULL, _T("Research Complete"));
			return overflow;
		}
		else
		{
			pIter->second += res;
			return -1;
		}
	}
	return res;
}

bool CHexPlayer::StartResearch( LPCTSTR name )
{
	StringIntHash::iterator pIter;
	pIter = techProgressHash.find(name);
	techTreeNodeDef* pDef = GET_DEF_FROM_STRING(techTreeNodeDef, name);
	if (pIter == techProgressHash.end())
	{
		if (!CanResearchTech(pDef))
			return false;
		techProgressHash[wcsdup(name)] = 0;
	}
	pCurResearch = pDef;
	return true;
}

bool CHexPlayer::StartResearch( techTreeNodeDef* pDef )
{
	StringIntHash::iterator pIter;
	pIter = techProgressHash.find(pDef->name);
	if (pIter == techProgressHash.end())
	{
		if (!CanResearchTech(pDef))
			return false;
		techProgressHash[wcsdup(pDef->name)] = 0;
	}
	pCurResearch = pDef;
	return true;
}


bool CHexPlayer::CanResearchTech( techTreeNodeDef* pNode )
{
	for (int i = 0; i < eaSize(&pNode->eaRequiredNames); i++)
	{
		if (GetTechProgress(pNode->eaRequiredNames[i]) != -1)
			return false;
	}
	return true;
}

techTreeNodeDef* CHexPlayer::GetCurrentTech()
{
	return pCurResearch;
}

CGameState g_GameState;