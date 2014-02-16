#include "stdafx.h"
#include "GameState.h"
#include "HexMap.h"
#include "assert.h"
#include "techtree.h"
#include "texturelibrary.h"
#include "flexDebugConsole.h"

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
	CreateSplatBuffers();
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
			if (uiDragStart > 0 && (GetTickCount() - uiDragStart > DRAG_THRESHOLD_MS))
			{
				g_Renderer.GetCamera()->MoveCamera((float)(ptMousePos.x - ptMousePosLastFrame.x), (float)(ptMousePos.y - ptMousePosLastFrame.y ), 0);
			}
			POINT mouseDeltas = {ptMouse.x - ptMousePosLastFrame.x, ptMouse.y - ptMousePosLastFrame.y};
//			g_Renderer.DoMouselook(mouseDeltas);
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
	if (g_Console.IsEnabled())
	{
		g_Console.Update(tick/1000.0f);
	}
	ghettoAnimTick += tick;
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
	case WM_LBUTTONDOWN:
		{
			uiDragStart = GetTickCount();
		}break;
	case WM_LBUTTONUP:
		{
			if (GetTickCount() - uiDragStart < DRAG_THRESHOLD_MS)
				SelectUnitOnTile(PixelToTilePt(pt.x, pt.y));
			uiDragStart = 0;
		}break;
	case WM_RBUTTONUP:
		{
		}break;
	case WM_MOUSEWHEEL:
		{
			int rot = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			//g_Renderer.GetCamera()->ZoomCamera(rot);
			g_Renderer.GetCamera()->MoveCamera(0,0,rot);
		}break;
	}
}

void CGameState::DoGameplayMouseInput_UnitSelected(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		{
			uiDragStart = GetTickCount();
		}break;
	case WM_LBUTTONUP:
		{
			if (GetTickCount() - uiDragStart < DRAG_THRESHOLD_MS)
				SelectUnitOnTile(PixelToTilePt(pt.x, pt.y));
			uiDragStart = 0;
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
			//g_Renderer.GetCamera()->ZoomCamera(rot);
			g_Renderer.GetCamera()->MoveCamera(0,0,rot);
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
			//g_Renderer.GetCamera()->ZoomCamera(rot);
			g_Renderer.GetCamera()->MoveCamera(0,0,rot);
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

				if (!(GetAsyncKeyState(VK_SHIFT) & (1 << 31)))
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
			//g_Renderer.GetCamera()->ZoomCamera(rot);
			g_Renderer.GetCamera()->MoveCamera(0,0,rot);
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
				RenderPath( curSelUnit, pTopOrder->pPath, 0x55);

			HEXPATH* pPath = NULL;
			if (PtInRect(&mapViewport, ptMousePos) && bMouseOverGameplay)
			{
				int iPathLength = pCurrentMap->HexPathfindTile(curSelUnit, curSelUnit->GetLoc(),PixelToTilePt(ptMousePos.x, ptMousePos.y),&pPath);
				if (iPathLength > 0)
				{
					RenderPath(curSelUnit, pPath, 0xff);
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
				//pCurrentMap->RenderInterface(&mapViewport, fpMapOffset, );
				//pCurrentMap->RenderInterface(&mapViewport, fpMapOffset, PixelToMapIntersect(ptMousePos.x, ptMousePos.y));
				POINT mouseoverTile = PixelToTilePt(ptMousePos.x, ptMousePos.y);
				RenderTextureSplat(mouseoverTile.x, mouseoverTile.y, kTextureSplat_SelectedTile, 0, 1.0f);
			}

			UI.Render();
		}break;
	}
	if (g_Console.IsEnabled())
	{
		g_Console.Render();
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
	g_Renderer.AddStringToRenderList(pFont, _T("New game"),(button.right+button.left)/2.0f, (button.bottom+button.top)/2.0f - 6, 0xffff0000, true, false, false);
	OffsetRect(&button, 0, 40);
	g_Renderer.AddSolidColorToRenderList(&button, 0xff333333);
	g_Renderer.AddStringToRenderList(pFont, _T("Exit"), (button.right+button.left)/2.0f, (button.bottom+button.top)/2.0f - 6, 0xffff0000, true, false, false);
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
	D3DXVECTOR3 pos(pt.x * HEX_WIDTH, pt.y * HEX_HEIGHT * 3.0f / 4.0f - 30, -30);
	g_Renderer.GetCamera()->SetCameraPosition(&pos);
}

POINT CGameState::GetViewCenter()
{
	POINT pt = {(LONG)(fpMapOffset.x + mapViewport.right/2)/HEX_SIZE, (LONG)(fpMapOffset.y + mapViewport.bottom/2)/(HEX_SIZE*3/4)};
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
	POINT box = {0,0};
	FLOATPOINT mapIntersect = {0,0};
	//int size = HEX_HEIGHT;
	//bool bNegative = x+fpMapOffset.x < 0;
	//box.y = (LONG)(y+fpMapOffset.y)/(HEX_SIZE*3/4 + 1);
	//box.x = (LONG)((x+fpMapOffset.x)/(HEX_SIZE) - ((box.y % 2) ? 0.5 : 0));
	//y = (int)(y+fpMapOffset.y) % (HEX_SIZE*3/4 + 1);
	//x = ((int)(x+fpMapOffset.x - ((box.y % 2) ? HEX_SIZE/2 : 0)) % HEX_SIZE);
	//if (bNegative)
	//{
	//	box.x -= 1;
	//	x+= HEX_SIZE;
	//}
	//if (y < HEX_SIZE/4)
	//{
	//	if (y < -0.5*x + (HEX_SIZE/4-1))
	//	{
	//		//upper left
	//		box.y--;
	//		if (box.y % 2)
	//			box.x--;
	//	}
	//	else if (y < 0.5*x + -(HEX_SIZE/4+1))
	//	{
	//		//upper right
	//		box.y--;
	//		if (!(box.y % 2))
	//			box.x++;
	//	}
	//}
	float tempX, tempY;

	mapIntersect = PixelToMapIntersect(x, y);

	bool bNegative = mapIntersect.x < 0;
	box.y = (LONG) (mapIntersect.y / (HEX_HEIGHT*3/4));
	box.x = (LONG)(mapIntersect.x /(HEX_WIDTH) - ((box.y % 2) ? 0.5 : 0));
	tempY = (mapIntersect.y);
	tempY -= ((int)(tempY/(HEX_HEIGHT*3/4)))*(HEX_HEIGHT*3/4);
	tempX = mapIntersect.x - ((box.y % 2) ? HEX_WIDTH/2 : 0);
	tempX -= ((int)((tempX/HEX_WIDTH)))*HEX_WIDTH;
	if (bNegative)
	{
		box.x -= 1;
		tempX+= HEX_WIDTH;
	}
	if (tempY < HEX_HEIGHT/4)
	{
		if (tempY < -tempX/SQRT_3 + (HEX_HEIGHT*1/4))
		{
			//lower left
			box.y--;
			if (box.y % 2)
				box.x--;
		}
		else if (tempY < tempX/SQRT_3 - (HEX_HEIGHT*1/4))
		{
			//lower right
			box.y--;
			if (!(box.y % 2))
				box.x++;
		}	
	}

	return box;
}

FLOATPOINT CGameState::PixelToMapIntersect(int x, int y)
{
	FLOATPOINT retCoords = {0,0};
	D3DXVECTOR3 vMapIntersect, vMapPoint, vMapNormal;
	D3DXVECTOR3 vRayOriginDir[2], vRayPoints[2];

	g_Renderer.CastRayThroughPixel(vRayOriginDir, x, y);

	vRayPoints[0] = vRayOriginDir[0];
	vRayPoints[1] = vRayOriginDir[0] + (2*vRayOriginDir[1]);

	vMapPoint = D3DXVECTOR3(0,0,0);
	vMapNormal = D3DXVECTOR3(0,0,-1);
	g_Renderer.PlaneIntersectRay(&vMapIntersect, &vMapPoint, &vMapNormal, &vRayPoints[0], &vRayPoints[1]);

	retCoords.x = vMapIntersect.x;
	retCoords.y = vMapIntersect.y;

	return retCoords;
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
	int mapWidth = 128;
	int mapHeight = 128;
	pCurrentMap->Generate(mapWidth,mapHeight, GetTickCount());
	g_Renderer.GetCamera()->SetAxisWrapValues(0, 0, mapWidth*HEX_WIDTH, true);

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


IDirect3DVertexBuffer9* CGameState::CreateSplatBufferForTexture(GameTexturePortion* pTex, SplattableTextureGeo eGeo)
{
	void* vb_vertices;
	GameTexture* pSourceTexture = pTex->hTex.pTex;
	IDirect3DVertexBuffer9* pSplatBuffer = NULL;
	
	switch(eGeo)
	{
	case kTextureSplatGeo_Hexagon:
		{
			float fXMin, fXHalf, fXMax;
			float fYMin, fYOneQuarter, fYThreeQuarters, fYMax;

			//calculate helpful coordinates
			fXMin = ((float)pTex->rSrc->left)/pSourceTexture->width;
			fXMax = ((float)pTex->rSrc->right)/pSourceTexture->width;
			fXHalf = (fXMin+fXMax)/2;

			fYMin = ((float)pTex->rSrc->top)/pSourceTexture->height;
			fYMax = ((float)pTex->rSrc->bottom)/pSourceTexture->height;
			fYOneQuarter = (fYMin*3+fYMax)/4;
			fYThreeQuarters = (fYMin+fYMax*3)/4;

			FlexVertex hexVerts[] = {
				{0.0f,				HEX_HALF_HEIGHT,				-0.011f, 0xFFFFFFFF,	fXHalf, fYMin},
				{HEX_HALF_WIDTH,	HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF, fXMax, fYOneQuarter},
				{-HEX_HALF_WIDTH,	HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF,	fXMin, fYOneQuarter},
		
				{-HEX_HALF_WIDTH,	HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF,	fXMin, fYOneQuarter},
				{HEX_HALF_WIDTH,	HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF, fXMax, fYOneQuarter},
				{-HEX_HALF_WIDTH,	-HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF,	fXMin, fYThreeQuarters},
		
				{-HEX_HALF_WIDTH,	-HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF,	fXMin, fYThreeQuarters},
				{HEX_HALF_WIDTH,	HEX_HEIGHT/4.0f,		-0.011f, 0xFFFFFFFF, fXMax, fYOneQuarter},
				{HEX_HALF_WIDTH,	-HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF, fXMax, fYThreeQuarters},
		
				{HEX_HALF_WIDTH,	-HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF, fXMax, fYThreeQuarters},
				{0.0f,				-HEX_HALF_HEIGHT,		-0.011f, 0xFFFFFFFF,	fXHalf, fYMax},
				{-HEX_HALF_WIDTH,	-HEX_HALF_HEIGHT/2.0f,	-0.011f, 0xFFFFFFFF,	fXMin, fYThreeQuarters}
			};

			g_Renderer.CreateVertexBuffer(sizeof(FlexVertex)*12, D3DUSAGE_WRITEONLY, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1, D3DPOOL_MANAGED, &pSplatBuffer, NULL);
			
			pSplatBuffer->Lock(0, 0, &vb_vertices, 0);

			memcpy(vb_vertices, hexVerts, sizeof(FlexVertex)*12);

			pSplatBuffer->Unlock();
		}break;
	case kTextureSplatGeo_Rectangle:
		{
			const float fScalar = 0.033f;
			float fUMin, fUMax, fHalfWidth;
			float fVMin, fVMax, fHalfHeight;
			fUMin = ((float)pTex->rSrc->left)/pSourceTexture->width;
			fUMax = ((float)pTex->rSrc->right)/pSourceTexture->width;
			fHalfWidth = (float)(pTex->rSrc->right-pTex->rSrc->left) * fScalar * 0.5f;


			fVMin = ((float)pTex->rSrc->top)/pSourceTexture->height;
			fVMax = ((float)pTex->rSrc->bottom)/pSourceTexture->height;
			fHalfHeight = (float)(pTex->rSrc->bottom-pTex->rSrc->top) * fScalar * 0.5f;

			FlexVertex quadVerts[] = {
				{-fHalfWidth,	+fHalfHeight,	-0.01f, 0xFFFFFFFF,	fUMin, fVMin},
				{fHalfWidth,	+fHalfHeight,	-0.01f, 0xFFFFFFFF,	fUMax, fVMin},
				{-fHalfWidth,	-fHalfHeight,	-0.01f, 0xFFFFFFFF,	fUMin, fVMax},
				{-fHalfWidth,	-fHalfHeight,	-0.01f, 0xFFFFFFFF,	fUMin, fVMax},
				{fHalfWidth,	+fHalfHeight,	-0.01f, 0xFFFFFFFF,	fUMax, fVMin},
				{fHalfWidth,	-fHalfHeight,	-0.01f, 0xFFFFFFFF,	fUMax, fVMax},
			};

			g_Renderer.CreateVertexBuffer(sizeof(FlexVertex)*6, D3DUSAGE_WRITEONLY, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1, D3DPOOL_MANAGED, &pSplatBuffer, NULL);
			
			pSplatBuffer->Lock(0, 0, &vb_vertices, 0);

			memcpy(vb_vertices, quadVerts, sizeof(FlexVertex)*6);

			pSplatBuffer->Unlock();
		}break;
	}


	return pSplatBuffer;
}

void CGameState::CreateSplatBuffers()
{
	GameTexturePortion* pPortion = GET_DEF_FROM_STRING(GameTexturePortion, L"selectedtile");
	pTextureSplatBuffers[kTextureSplat_SelectedTile].pBuf = CreateSplatBufferForTexture(pPortion, kTextureSplatGeo_Hexagon);
	pTextureSplatBuffers[kTextureSplat_SelectedTile].iNumTris = 4;
	pTextureSplatBuffers[kTextureSplat_SelectedTile].eGeo = kTextureSplatGeo_Hexagon;
	pTextureSplatBuffers[kTextureSplat_SelectedTile].pTex = pPortion->hTex.pTex;
	
	pPortion = GET_DEF_FROM_STRING(GameTexturePortion, L"path");
	pTextureSplatBuffers[kTextureSplat_PathBlipSmall].pBuf = CreateSplatBufferForTexture(pPortion, kTextureSplatGeo_Rectangle);
	pTextureSplatBuffers[kTextureSplat_PathBlipSmall].iNumTris = 2;
	pTextureSplatBuffers[kTextureSplat_PathBlipSmall].eGeo = kTextureSplatGeo_Rectangle;
	pTextureSplatBuffers[kTextureSplat_PathBlipSmall].pTex = pPortion->hTex.pTex;
	
	pPortion = GET_DEF_FROM_STRING(GameTexturePortion, L"pathblip");
	pTextureSplatBuffers[kTextureSplat_PathBlipLarge].pBuf = CreateSplatBufferForTexture(pPortion, kTextureSplatGeo_Rectangle);
	pTextureSplatBuffers[kTextureSplat_PathBlipLarge].iNumTris = 2;
	pTextureSplatBuffers[kTextureSplat_PathBlipLarge].eGeo = kTextureSplatGeo_Rectangle;
	pTextureSplatBuffers[kTextureSplat_PathBlipLarge].pTex = pPortion->hTex.pTex;
	
	pPortion = GET_DEF_FROM_STRING(GameTexturePortion, L"pathend");
	pTextureSplatBuffers[kTextureSplat_PathTarget].pBuf = CreateSplatBufferForTexture(pPortion, kTextureSplatGeo_Rectangle);
	pTextureSplatBuffers[kTextureSplat_PathTarget].iNumTris = 2;
	pTextureSplatBuffers[kTextureSplat_PathTarget].eGeo = kTextureSplatGeo_Rectangle;
	pTextureSplatBuffers[kTextureSplat_PathTarget].pTex = pPortion->hTex.pTex;
}

void CGameState::RenderTextureSplat(int x, int y, SplattableTexture eType, float rot, float scale)
{
	float vPos[3] = {x*HEX_WIDTH + HEX_HALF_WIDTH, y*HEX_HEIGHT*3/4 + HEX_HALF_HEIGHT, 0.0f};
	float vRot[3] = {0.0f, 0.0f, rot};
	float vScale[3] = {scale, scale, 1.0f};
	if (y & 1)
		vPos[0] += HEX_HALF_WIDTH;
	g_Renderer.AddModelToRenderList(&pTextureSplatBuffers[eType].pBuf, &pTextureSplatBuffers[eType].iNumTris, pTextureSplatBuffers[eType].pTex, vPos, vScale, vRot, false);
}

void CGameState::RenderPath(CHexUnit* pUnit, HEXPATH* pPath, int alpha )
{
	if (!pPath)
		 return;
	SplattableTexture eSplat;
	//i starts at 1, don't render first tile of the path
	int iMovement = pUnit->GetMovRemaining();
	int iTurn = 0;
	float fRot = 0.0f;
	for (int i = pPath->start; i < pPath->size; i++)
	{
		iMovement -= pCurrentMap->GetTile(pPath->pPoints[i])->pDef->iMoveCost;
		bool bShowTurnCount = false;
		if (i == pPath->size-1)
		{
			eSplat = kTextureSplat_PathTarget;
			bShowTurnCount = true;
			fRot = (ghettoAnimTick % 9000)/9000.0f * PI * 2.0f;
			iTurn++;
		}
		else if (iMovement <= 0)
		{
			eSplat = kTextureSplat_PathBlipLarge;
			bShowTurnCount = true;
			iMovement = pUnit->GetDef()->movement;
			iTurn++;
		}
		else
		{
			eSplat = kTextureSplat_PathBlipSmall;
		}
		RenderTextureSplat(pPath->pPoints[i].x, pPath->pPoints[i].y, eSplat, fRot, 1.0f);
//		g_Renderer.AddSpriteToRenderList(pathSeg, renderPt, alpha << 24, ZOOM_PERCENT);
		if (bShowTurnCount)
		{
			TCHAR buf[4];
			wsprintf(buf, _T("%i"), iTurn);
//			g_Renderer.AddStringToRenderList(pFont, buf, (float)(renderPt.x+HEX_SIZE/2+1), (float)(renderPt.y+HEX_SIZE/2-7+1), (alpha << 24), true, false, false);
//			g_Renderer.AddStringToRenderList(pFont, buf, (float)(renderPt.x+HEX_SIZE/2), (float)(renderPt.y+HEX_SIZE/2-7), (alpha << 24) | (iTurn > 1 ? 0xff0000 : 0xff00), true, false, false);
		}
	}
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
		techProgressHash[_wcsdup(name)] = 0;
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
		techProgressHash[_wcsdup(pDef->name)] = 0;
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

void CHexPlayer::UpdateCachedIncomeValues()
{
	int iGoldIncome = 0, iScienceIncome = 0;
	for (int i = 0; i < eaSize(&eaCities); i++)
	{
		iGoldIncome += eaCities[i]->GetNetGold();
		iScienceIncome += eaCities[i]->GetNetResearch();
	}
	iCachedGoldIncome = iGoldIncome;
	iCachedScienceIncome = iScienceIncome;
	bIncomeDirty = false;
}

techTreeNodeDef* CHexPlayer::GetCurrentTech()
{
	return pCurResearch;
}

CGameState g_GameState;