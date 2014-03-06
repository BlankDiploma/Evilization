#include "stdafx.h"
#include "flexrenderer.h"
#include "HexMap.h"
#include "uilib.h"
#include "techtree.h"
#include "strhashmap.h"
#include "FlexParticleSystem.h"

#pragma once

enum TileVisType {kVis_Shroud = 0, kVis_Fog, kVis_Clear};
enum playerControlType {kPlayerControl_AI_Local = 0, kPlayerControl_AI_Network, kPlayerControl_Human_Local, kPlayerControl_Human_Network};
enum mouseHandlerType {kGameplayMouse_Default, kGameplayMouse_UnitSelected, kGameplayMouse_CityView, kGameplayMouse_PlaceBuilding, kGameplayMouse_SelectAbilityTarget, kGameplayMouse_Disable};

#define DRAG_THRESHOLD_MS 133
#define DRAG_THRESHOLD_PIXELS 3

struct playerVisibility
{
	int* fogMask;
	int* shroudMask;
	int w, h;
	playerVisibility()
	{
		w = -1;
		h = -1;
		fogMask = NULL;
		shroudMask = NULL;
	}
	~playerVisibility()
	{
		if (fogMask)
			delete [] fogMask;
		if (shroudMask)
			delete [] shroudMask;
	}
	void Create(CHexMap* pMap)
	{
		if (fogMask)
			delete [] fogMask;
		if (shroudMask)
			delete [] shroudMask;

		w = pMap->GetWidth()/32;
		h = pMap->GetHeight();
		fogMask = new int[w*h];
		shroudMask = new int[w*h];
		memset(fogMask, 0, w*h*sizeof(int));
		memset(shroudMask, 0, w*h*sizeof(int));
	}
	void RevealTiles (POINT* pTiles, int num)
	{
		for (int i = 0; i < num; i++)
		{
			fogMask[pTiles[i].x / 32 + pTiles[i].y * w] |= 1 << (pTiles[i].x%32);
			shroudMask[pTiles[i].x / 32 + pTiles[i].y * w] |= 1 << (pTiles[i].x%32);
		}
	}
	void ClearFog()
	{
		memset(fogMask, 0, w*h*sizeof(int));
	}
	TileVisType GetTileVis(int i, int j)
	{
		if (shroudMask[i / 32 + j * w] & (1 << (i%32)))
		{
			if (fogMask[i / 32 + j * w] & (1 << (i%32)))
			{
				return kVis_Clear;
			}
			return kVis_Fog;
		}
		return kVis_Shroud;
	}
	TileVisType GetTileVis(POINT pt)
	{
		if (shroudMask[pt.x / 32 + pt.y * w] & (1 << (pt.x%32)))
		{
			if (fogMask[pt.x / 32 + pt.y * w] & (1 << (pt.x%32)))
			{
				return kVis_Clear;
			}
			return kVis_Fog;
		}
		return kVis_Shroud;
	}
};


#define MAX_VIS_RAD 6
#define MAX_VIS_BUFFER (1+(MAX_VIS_RAD*6 + 6)/2 * MAX_VIS_RAD)// num tiles within MAX_VIS_RAD

enum notifyType {kNotify_None = 0, kNotify_Production, kNotify_Tech, kNotify_Combat, kNotify_Orders};

struct playerNotification
{
	notifyType eType;
	POINT focusLoc;
	void* pFocusThing;
	GameTexturePortion* pTex;
	const TCHAR* pchText;
	bool bDelete;
};


class CHexPlayer
{
	TCHAR* name;
	int ID;
	DWORD colorA;
	DWORD colorB;
	playerVisibility vis;
	playerControlType eType;
	int gold;
	int iCachedGoldIncome;
	int iCachedScienceIncome;
	bool bIncomeDirty;
	techTreeNodeDef* pCurResearch;
	StringIntHash techProgressHash;
	StringIntHash buildPermissions;
	playerNotification** eaNotifications;
	void UpdateCachedIncomeValues();

public:
	CHexUnit** eaUnits;
	CHexBuilding** eaBuildings;
	CHexCity** eaCities;
	int GetGoldPerTurn()
	{
		if (bIncomeDirty)
			UpdateCachedIncomeValues();
		return iCachedGoldIncome;
	}
	int GetBankedGold()
	{
		return gold;
	}
	void DirtyCachedIncomeValues()
	{
		bIncomeDirty = true;
	}
	int GetSciencePerTurn()
	{
		if (bIncomeDirty)
			UpdateCachedIncomeValues();
		return iCachedScienceIncome;
	}
	CHexPlayer()
	{
		name = _T("Player");
		ID = -1;
		colorA = 0xffffffff;
		colorB = 0xff000000;
		eaUnits = NULL;
		eaBuildings = NULL;
		eaCities = NULL;
		pCurResearch = NULL;
		eType = kPlayerControl_AI_Local;
		eaNotifications = NULL;
		gold = 0;
		iCachedGoldIncome = 0;
		iCachedScienceIncome = 0;
		bIncomeDirty = true;
		
		for(int i = 0; i < eaSize(&g_pEvilTechTree->eaDefaultGrants); i++)
		{
			buildPermissions[g_pEvilTechTree->eaDefaultGrants[i]] = 1;
		}
	}
	void AddNotification(notifyType eType, POINT* focusLoc, void* focusObj, GameTexturePortion* pTex, LPCTSTR text)
	{
		playerNotification* pNotify = new playerNotification;
		pNotify->eType = eType;
		if (focusLoc)
		{
			pNotify->focusLoc.x = focusLoc->x;
			pNotify->focusLoc.y = focusLoc->y;
		}
		pNotify->pFocusThing = focusObj;
		pNotify->pTex = pTex;
		pNotify->pchText = _wcsdup(text);
		pNotify->bDelete = false;

		eaPush(&eaNotifications, pNotify);
	}
	void RemoveNotification(playerNotification* pNotify)
	{
		for (int i = 0; i < eaSize(&eaNotifications); i++)
		{
			if (eaNotifications[i] == pNotify)
			{
				eaNotifications[i]->bDelete = true;
				return;
			}
		}
	}
	const StringIntHash* GetBuildPermissions()
	{
		return &buildPermissions;
	}
	playerNotification** GetNotifications()
	{
		return eaNotifications;
	}
	inline int GetID()
	{
		return ID;
	}
	void SetType(playerControlType type)
	{
		eType = type;
	}
	playerControlType GetType()
	{
		return eType;
	}
	void AddGold(int x)
	{
		gold += x;
	}
	void RefreshVisibility(CHexMap* pMap)
	{
		static POINT buf[MAX_VIS_BUFFER];
		vis.ClearFog();
		for (int i = 0; i < eaSize(&eaUnits); i++)
		{
			vis.RevealTiles(buf, pMap->GetTilesInRadius(eaUnits[i]->GetLoc(), eaUnits[i]->GetVisRadius(), buf));
		}
		for (int i = 0; i < eaSize(&eaBuildings); i++)
		{
			vis.RevealTiles(buf, pMap->GetTilesInRadius(eaBuildings[i]->GetLoc(), eaBuildings[i]->GetVisRadius(), buf));
		}
		for (int i = 0; i < eaSize(&eaCities); i++)
		{
			vis.RevealTiles(buf, pMap->GetTilesInRadius(eaCities[i]->GetLoc(), eaCities[i]->GetVisRadius(), buf));
		}
	}
	void Init(int idx, TCHAR* name, DWORD colorA, DWORD colorB, CHexMap* pMap)
	{
		ID = idx;
		this->name = _wcsdup(name);
		this->colorA = colorA;
		this->colorB = colorB;
		vis.Create(pMap);
	}
	void TakeOwnership(CHexUnit* pUnit)
	{
		if (pUnit->GetOwnerID() == ID)
			return;
		if (pUnit)
		{
			eaPush(&eaUnits, pUnit);
			pUnit->SetOwnerID(ID);
		}
	}
	void TakeOwnership(CHexBuilding* pBuilding)
	{
		if (pBuilding->GetOwnerID() == ID)
			return;
		if (pBuilding)
		{
			if (pBuilding->GetMyType() == kBuilding_City)
			{
				TakeOwnership((CHexCity*)pBuilding);
				return;
			}
			eaPush(&eaBuildings, pBuilding);
			pBuilding->SetOwnerID(ID);
		}
	}
	void TakeOwnership(CHexCity* pCity)
	{
		if (pCity->GetOwnerID() == ID)
			return;
		if (pCity)
		{
			eaPush(&eaCities, pCity);
			pCity->SetOwnerID(ID);
		}
	}
	playerVisibility* GetVisibility()
	{
		return &vis;
	}
	void StartTurn()
	{
		for (int i = 0; i < eaSize(&eaUnits); i++)
		{
			eaUnits[i]->StartTurn();
		}
		for (int i = 0; i < eaSize(&eaBuildings); i++)
		{
			eaBuildings[i]->StartTurn(this);
		}
		for (int i = 0; i < eaSize(&eaCities); i++)
		{
			eaCities[i]->StartTurn(this);
		}
	}
	void OncePerFrame()
	{
		for (int i = 0; i < eaSize(&eaNotifications); i++)
		{
			if (eaNotifications[i]->bDelete)
			{
				delete eaNotifications[i];
				eaRemove(&eaNotifications, i);
				i--;
			}
		}
	}
	void RemoveOwnership(CHexUnit* pUnit)
	{
		for (int i = 0; i < eaSize(&eaUnits); i++)
		{
			if (pUnit == eaUnits[i])
			{
				eaRemove(&eaUnits, i);
				return;
			}
		}
	}
	int GetTechProgress( LPCTSTR name );
	int AddResearch( int res );
	bool StartResearch( LPCTSTR name );
	bool StartResearch( techTreeNodeDef* pDef );
	bool CanResearchTech( techTreeNodeDef* pNode );
	techTreeNodeDef* GetCurrentTech();
	float GetTechPct( );
};

enum GameState {kGameState_Invalid = 0, kGameState_MainMenu, kGameState_Gameplay};

extern HWND hWndMain;

struct MouseHandlerState
{
	mouseHandlerType eMouseHandler;
	void* pMouseHandlerParam;
	bool bPopOnUIClick;
	MouseHandlerState()
	{
		eMouseHandler = kGameplayMouse_Default;
		pMouseHandlerParam = NULL;
		bPopOnUIClick = false;
	}
};


class CGameState
{
private:

	enum SplattableTexture {kTextureSplat_SelectedTile = 0, kTextureSplat_PathBlipSmall, kTextureSplat_PathBlipLarge, kTextureSplat_PathTarget, kTextureSplat_Count};
	
	enum SplattableTextureGeo {kTextureSplatGeo_Hexagon = 0, kTextureSplatGeo_Rectangle, kTextureSplatGeo_Count};
	
	struct
	{
		SplattableTextureGeo eGeo;
		IDirect3DVertexBuffer9* pVertBuf;
		IDirect3DIndexBuffer9* pIndBuf;
		int iNumTris;
		int iNumVerts;
		const GameTexture* pTex;
	} pTextureSplatBuffers[kTextureSplat_Count];

	CHexPlayer* pPlayers;
	FLOATPOINT fpMapOffset;
	POINT ptMousePos;
	POINT ptMousePosLastFrame;
	RECT mapViewport;
	int screenW;
	int screenH;
	bool bMouseOverGameplay;
	DWORD ghettoAnimTick;
	DWORD uiDragStart;

	//mutable
	GameTexture minimapTexture;

	GameTexture* pFont;
	GameTexture* pInterface;
	GameTexture* pNines;
	UserInterface UI;
	CHexMap* pCurrentMap;
	GameState eCurState;
	CHexUnit* curSelUnit;
	CHexCity* curSelCity;
	CHexUnit** eaDeadUnits;

	int iCurPlayer;
	int iNumPlayers;

	MouseHandlerState** eaMouseStates;
	void CGameState::MouseHandlerAdditionalRendering();
	void DoGameplayMouseInput_Default(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam);
	void DoGameplayMouseInput_UnitSelected(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam);
	void DoGameplayMouseInput_CityView(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam);
	void DoGameplayMouseInput_PlaceBuilding(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam);
	void DoGameplayMouseInput_SelectAbilityTarget(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam, void* pHandlerParam);
	
	IDirect3DVertexBuffer9* CreateSplatVertBufferForTexture(GameTexturePortion* pTex, SplattableTextureGeo eGeo);
	IDirect3DIndexBuffer9* CreateSplatIndexBufferForTexture(SplattableTextureGeo eGeo);
	void CreateSplatBuffers();
	void RenderTextureSplat(int x, int y, GameTexturePortion* pPortion, float rot, float scale);
	void RenderPath(CHexUnit* pUnit, HEXPATH* pPath, int alpha );
	void DeleteUnit(CHexUnit* pUnit);

public:
	CGameState();
	void Update(DWORD tick);
	void Render();
	void RenderMainMenu();
	void RenderUI();
	void RenderMinimap(RECT* uiRect);
	void MouseInput(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam);
	void KeyInput(int keyCode, bool bDown);
	void NewGame();
	void EndCurrentGame();
	void StartNewGame(hexMapGenerationDesc* pMapDesc, int iNumPlayers);
	void Initialize(int screenW, int screenH);
	void SwitchToState(GameState newState);
	void CenterView(POINT pt);
	POINT PixelToTilePt(int x, int y);
	FLOATPOINT PixelToMapIntersect(int x, int y);
	void GameplayWindowMouseInput(UINT msg, POINT pt, WPARAM wParam, LPARAM lParam);
	void RenderTileObject(int x, int y, GameTexturePortion* pPortion, float rot, float scale);
	void RenderDamageText(int damage, POINT tarPt);
	void AddUnitToDeadList(CHexUnit* pUnit);
	
	void MouseHandlerPushState(mouseHandlerType eType, void* pParam, bool bPopOnUIClick = false)
	{
			//Don't allow pushing a duplicate state. Instead just overwrite the param.
		if (eaMouseStates[eaSize(&eaMouseStates)-1]->eMouseHandler == eType)
		{
			eaMouseStates[eaSize(&eaMouseStates)-1]->pMouseHandlerParam = pParam;
			return;
		}
		MouseHandlerState* pState = new MouseHandlerState;
		pState->eMouseHandler = eType;
		pState->pMouseHandlerParam = pParam;
		pState->bPopOnUIClick = bPopOnUIClick;
		eaPush(&eaMouseStates, pState);
	}
	void MouseHandlerPopState()
	{
		delete eaMouseStates[eaSize(&eaMouseStates)-1];
		eaPop(&eaMouseStates);
		assert(eaSize(&eaMouseStates) > 0);
		//can't pop the last mouse handler
	}
	void SelectUnitOnTile(POINT pt)
	{
		CHexUnit* pUnit = pCurrentMap->GetTile(pt)->pUnit;
		if (pUnit && pUnit->GetOwnerID() == iCurPlayer)
		{
			if (!curSelUnit)
			{		
				MouseHandlerPushState(kGameplayMouse_UnitSelected, NULL);
			}
			curSelUnit = pCurrentMap->GetTile(pt)->pUnit;
		}
		else
		{
			if (curSelUnit)
			{		
				MouseHandlerPopState();
			}
			curSelUnit = NULL;
		}
	}
	void SelectUnit(CHexUnit* pUnit)
	{
		if (pUnit && pUnit->GetOwnerID() == iCurPlayer)
		{
			if (!curSelUnit)
			{		
				MouseHandlerPushState(kGameplayMouse_UnitSelected, NULL);
			}
			curSelUnit = pUnit;
		}
	}
	POINT GetMouseDeltas()
	{
		POINT pt = {ptMousePos.x - ptMousePosLastFrame.x, ptMousePos.y - ptMousePosLastFrame.y};
		return pt;
	}
	void SelectNextUnit(CHexPlayer* pPlayer);
	void RenderMouseoverInfo();
	void RenderSelectionInfo();
	void RenderEndTurnButton();
	void StartPlayerTurn(int idx);
	void EndCurrentTurn();
	void ExecuteQueuedActions();
	void IssueOrder(unitOrderType eType, HEXPATH* pPath, POINT targetPt, UnitAbility* pAbility);
	void RenderSelectedUnit(POINT screenPt, float scale = 1.0);
	CHexUnit* GetSelectedUnit();
	POINT TilePtToScreenPt(int x, int y);
	void GetOnscreenCityList(hexTile*** peaTilesOut, bool bOwned);
	void ShowDetailedCityView( CHexCity* pCity );
	void ShowGameplayUI();
	void SelectCity(CHexCity* pCity);
	CHexCity* GetSelectedCity();
	void GetOnscreenLaborList(laborSlot*** peaLaborOut, CHexCity* pCity);
	void AdjustMapZoom( int rot );
	POINT GetViewCenter();
	void ShowTechTreeUI();
	void ShowAbilityUI();
	CHexPlayer* GetCurrentPlayer();

	CHexMap* GetCurrentMap()
	{
		return pCurrentMap;
	}
	CHexPlayer* GetPlayerByID(int id)
	{
		if (id >= 0 && id < iNumPlayers)
			return &pPlayers[id];

		return NULL;
	}
	CHexUnit* GetCurSelUnit()
	{
		return curSelUnit;
	}
};

#define DEFAULT_HEX_SIZE 128

extern int g_HexSize;

extern CGameState g_GameState;