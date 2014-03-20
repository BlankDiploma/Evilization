// Testproj.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Evilization.h"
#include <d3d9.h>
#include <math.h>
#include <d3dx9.h>
#include "PerlinMap.h"
#include "mtrand.h"
#include "hexmap.h"
#include "GameState.h"
#include "StringTag.h"
#include "techtree.h"
#include "flexrenderer.h"
#include "deflibrary.h"
#include "texturelibrary.h"
#include "flexlua.h"
#include "HexFeatures.h"
#include "FlexDebugConsole.h"
#include "Autogen/AutoEnums.h"

using namespace luabind;

#define MAX_LOADSTRING 100
#define GAME_WINDOW_WIDTH 1920
#define GAME_WINDOW_HEIGHT 1080

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szChildClass[MAX_LOADSTRING];			// the main window class name
TCHAR szPaletteClass[MAX_LOADSTRING];			// the palette window class name
int count = 0;
int mouseX;
int mouseY;
int numUpdatesInLastSecond = 0;
DebugFlags g_DebugFlags = {0};

MTRand randomFloats(GetTickCount());

//CSpriteSet* g_pOhGodNeverStoreThingsInGlobalVariablesLikeThis = NULL;
//CSimpleAnim* g_pDerpyAnim = NULL;
int g_screenWidth = GAME_WINDOW_WIDTH;
int g_screenHeight = GAME_WINDOW_HEIGHT;
DWORD g_LastTick = 0;

// Forward declarations of functions
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
void				LoadGameData();
void				GameLoop();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	ChildWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void UpdateNoiseTexture(int w, int h, CPerlinMap* pMap);
void DoAllLuaBinds();

HWND hWndMain;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
	
    hInst = hInstance; // Store instance handle in our global variable

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_EVILIZATION, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_EVILIZATION);

	g_Renderer.Initialize(hWndMain, g_screenWidth, g_screenHeight);

	//KRIS: run game init
	LoadGameData();
	

	/*
	g_MainThreadID = GetWindowThreadProcessId(hWndMain, NULL);

	hGameThread = (HANDLE)CreateThread(NULL, 0, TacticsGameThread, hMutex, 0, &g_GameThreadID);

	//Wait for game thread to be able to receieve messages.
	WaitForSingleObject(hMutex, INFINITE);

	CloseHandle(hMutex);

	int ret = AttachThreadInput(g_MainThreadID, g_GameThreadID, true);
	*/
	// Main message loop:
	for ( ; ; ) 
	{
		//check for a message
		if ( PeekMessage( &msg , NULL , 0 , 0 , PM_REMOVE ) )
		{
			//message exists

			//check for quit message
			if ( msg.message == WM_QUIT ) goto quit ;

			//translate the message
			TranslateMessage ( &msg ) ;

			//dispatch the message
			DispatchMessage ( &msg ) ;
		}
		else
		{
			//KRIS: Main loop goes here
			GameLoop ( ) ;
			g_Renderer.ProcessRenderLists();
			
		}
	}
quit:
	//KRIS: Call shutdown
	return (int) msg.wParam;
}



//Windows voodoo
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_EVILIZATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= _T("TestProj");
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//Windows voodoo
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_screenWidth = GetSystemMetrics(SM_CXSCREEN);
	g_screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT window = {0,0,GAME_WINDOW_WIDTH,GAME_WINDOW_HEIGHT};
	//AdjustWindowRect(&window, WS_BORDER | WS_CAPTION, false);
	hWndMain = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, g_screenWidth, g_screenHeight, NULL, NULL, hInstance, NULL);
	SetWindowPos(hWndMain, HWND_TOPMOST, 0, 0, g_screenWidth, g_screenHeight, SWP_SHOWWINDOW);
   if (!hWndMain)
   {
      return FALSE;
   }

   ShowWindow(hWndMain, nCmdShow);
   UpdateWindow(hWndMain);

   return TRUE;
}

//KRIS: This is where you handle input
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
		{
			int keyCode = wParam;
			if (keyCode == VK_OEM_3)
			{
				g_Console.Toggle();
			}
			else if (g_Console.IsEnabled())
			{
				BYTE keyboardState[256];
				TCHAR buffer[2];
				GetKeyboardState(keyboardState);
				if (ToUnicode(keyCode, MapVirtualKey(keyCode, MAPVK_VK_TO_VSC), keyboardState, buffer, 2, 0))
				{
					g_Console.KeyInput(keyCode, buffer[0]);
				}
				else
					g_Console.KeyInput(keyCode, 0);
			}
			g_GameState.KeyInput(keyCode, true);
		}break;
	case WM_KEYUP:
		{
			int keyCode = wParam;
			//KRIS: handle keyCode here
			g_GameState.KeyInput(keyCode, false);
		}break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

		//KRIS: you can extrapolate the rest of the mouse events from this one
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_MOUSEWHEEL:
		{
			POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			g_GameState.MouseInput(message, pt, wParam, lParam);
		}
		break;
	case WM_ACTIVATEAPP:
		{
			SetWindowPos(hWnd, wParam ? HWND_TOPMOST : HWND_BOTTOM, 0, 0, g_screenWidth, g_screenHeight, SWP_NOMOVE | SWP_NOSIZE);
			if (!wParam)
				ShowWindow(hWnd, SW_MINIMIZE);
		}
		break;
	case WM_MOUSEMOVE:
		{
			int mouseXmove = LOWORD(lParam); 
			int mouseYmove = HIWORD(lParam); 
			if (wParam & MK_LBUTTON)
			{
				//left button held while dragging
			}
			else if (wParam & MK_RBUTTON)
			{
				//right button held while dragging
			}
			else
			{

			}
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

//DirectX voodoo you don't need to worry about for now.
LPDIRECT3DTEXTURE9 pPerlinTex = NULL;
#define MAPSIZE 512
void D3DInitialize()
{
	/*
	HRESULT r = 0;
	D3DDISPLAYMODE d3ddm;
	D3DPRESENT_PARAMETERS d3dpp;
	LPDIRECT3DSURFACE9 pBack = 0;
	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_pD3D == NULL)
	{
		return;
	}

	r = g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	if(FAILED(r))
	{
		return;
	}

	ZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.hDeviceWindow = hWndMain;
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.BackBufferWidth = g_screenWidth;
	d3dpp.BackBufferHeight = g_screenHeight;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE ;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	r = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pMainDevice);

	g_pMainView.X = 0;
	g_pMainView.Y = 0;
	g_pMainView.Height = g_screenHeight;
	g_pMainView.Width = g_screenWidth;
	g_pMainView.MinZ = 0.0;
	g_pMainView.MaxZ = 1.0;

	g_pMainDevice->SetViewport(&g_pMainView);
	
	//set vertex shader
	g_pMainDevice->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 ) ;

	//turn off lighting
	g_pMainDevice->SetRenderState ( D3DRS_LIGHTING , FALSE ) ;
//	g_pMainDevice->SetRenderState ( D3DRS_ALPHATESTENABLE , TRUE ) ;
	g_pMainDevice->SetRenderState ( D3DRS_ALPHABLENDENABLE , TRUE ) ;
	g_pMainDevice->SetRenderState ( D3DRS_ALPHAFUNC , D3DCMP_GREATEREQUAL ) ;
	g_pMainDevice->SetRenderState ( D3DRS_ALPHAREF , 1 ) ;
	g_pMainDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE  );
	g_pMainDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE );
	g_pMainDevice->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE );
	g_pMainDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	g_pMainDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	g_pMainDevice->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);

	g_pMainDevice->CreateTexture(MAPSIZE, MAPSIZE, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT , &pPerlinTex, NULL);
	*/
	return;
}


//Load all your data from disk.
void LoadGameData()
{
	PopulateAutoEnumTables();
	g_LUA = luaL_newstate();

	luabind::open(g_LUA);
	
	luaL_openlibs(g_LUA);
	
	DoAllLuaBinds();
	LOAD_TEXTURES("*.*", g_Renderer.GetD3DDevice());
	LOAD_NINEPATCHES("*.*", g_Renderer.GetD3DDevice());
	LOAD_FONTS("*.*", g_Renderer.GetD3DDevice());
//	LOAD_MODELS("*.fbx");
	LOAD_DEFS_FROM_FILE("*.box");
	LOAD_DEFS_FROM_FILE("*.tech");
	LOAD_DEFS_FROM_FILE("*.def");
	g_DefLibrary.ResolvePendingRefs();
	g_DefLibrary.ResolvePendingTextures();
	g_DefLibrary.CompileScripts();
	/*
	//Load our sprites
	g_pOhGodNeverStoreThingsInGlobalVariablesLikeThis = new CSpriteSet(g_pMainDevice, _T("Derp.bmp"), D3DFMT_A8R8G8B8);

	//totally ghetto way of defining animations, you'll want to write something to load this info from disk or whatever.
	frameInfo* pFrameData = new frameInfo[2];
	pFrameData[0].iDelay = 200;
	pFrameData[0].iSpriteIndex = 0;
	pFrameData[1].iDelay = 200;
	pFrameData[1].iSpriteIndex = 1;

	g_pDerpyAnim = new CSimpleAnim(g_pOhGodNeverStoreThingsInGlobalVariablesLikeThis, 2, pFrameData);
*/
	//Store the current tick.

	InitializeBiomeMap(_T("biomes.bmp"));
//	StoreGameExprFuncs();
	g_GameState.Initialize(g_screenWidth, g_screenHeight);
	g_Renderer.CreateAllTextureAtlasBuffers();
//	hexmap.Generate(128, 128, GetTickCount());

}

void RenderHexagon(int x, int y)
{
	/*
	int index ;
	GfxSheetVertex vert [ 6 ] ;

	vert[0].x = x;
	vert[0].y = y + 16;

	vert[1].x = x + 32;
	vert[1].y = y;

	vert[2].x = x + 64;
	vert[2].y = y + 16;

	vert[3].x = x + 64;
	vert[3].y = y + 48;

	vert[4].x = x + 32;
	vert[4].y = y + 64;

	vert[5].x = x + 0;
	vert[5].y = y + 48;

	for ( index = 0 ; index < 6 ; index ++ )
	{
		vert[index].u = 0.0f;
		vert[index].v = 0.0f;
		vert[index].z = 0.0f;
		vert[index].u1 = 0.0f;
		vert[index].v1 = 0.0f;
		vert[index].rhw = 1.0f;
		vert[index].diffuseColor = 0xffbb00bb;
	}

	//set the texture
	g_pMainDevice->SetTexture ( 0 , NULL );
	g_pMainDevice->DrawPrimitiveUP ( D3DPT_TRIANGLEFAN , 4 , &vert , sizeof ( GfxSheetVertex ) );
	//tempDevice->SetTransform(D3DTS_TEXTURE0, NULL );
	*/
}

void UpdateNoiseTexture(int w, int h, CPerlinMap* pMap)//int savedType, int frameType, unsigned int* savedVis, unsigned int* frameVis)
{
	D3DLOCKED_RECT d3dlr;
	pPerlinTex->LockRect(0, &d3dlr, 0, 0); 
	char* pDst = (char*)d3dlr.pBits;
	DWORD* pPixel;
	for (int j = 0; j < w; j++)
	{
		pPixel = (DWORD*)pDst;
		pDst += d3dlr.Pitch;
		for (int i = 0; i < h; i++)
		{
			float val = pMap->GetAt(i, j) * 128 + 128;
			DWORD color = (int)val * 0x010101 | 0xff000000;
//			if (color < 0xff900000)
//				color = 0xff000000;
			*pPixel = color;
			pPixel++;
		}
	}

		pPerlinTex->UnlockRect(0);
		//			D3DXSaveTextureToFile(_T("derp.png"), D3DXIFF_PNG, pTileLightmapDXT1Texture, NULL);
}

void GameLoop()
{
		//Measure elapsed time.
		DWORD currentTick = GetTickCount();
		DWORD elapsed = currentTick - g_LastTick;
		if (elapsed >= 10)
		{
			g_LastTick = currentTick;

			g_GameState.Update(elapsed);

			g_Renderer.StartNewRenderList();
			
			g_GameState.Render();

			g_Renderer.CommitRenderList();
		}
		
}

/**
												Expressions
**/
/*
luabind::object Gameplay_GetPlayerEquipmentList(lua_State *L)
{
	luabind::object o = luabind::newtable(L);
	o[1] = 1;
	o[2] = 1;
	o[3] = 1;
	o[4] = 1;
	o[5] = 1;
	o[6] = 1;
	o[7] = 1;
	o[8] = 1;
	o[9] = 1;
	o[10] = 1;
	return o;
} 

void Gameplay_GetPlayerEquipmentList_RawArray(lua_State *L)
{
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
	lua_pushnumber(g_LUA, 1);
}
*/
void EndTurnButton(lua_State *L)
{
	g_GameState.EndCurrentTurn();
}

CHexUnit* GetSelectedUnit(lua_State *L)
{
	return g_GameState.GetSelectedUnit();
}

void RenderMinimap(lua_State *L)
{
	g_GameState.RenderMinimap(g_pCurContext->pUI->GetScreenRect());
}

void MinimapClick(lua_State *L)
{
	POINT pt = g_pCurContext->mousePt;
	pt.x -= ((UIInstance*)g_pCurContext->pUI)->GetScreenRect()->left;
	pt.y = ((UIInstance*)g_pCurContext->pUI)->GetScreenRect()->bottom-pt.y;
	pt.x /= 2;
	pt.y /= 2;
	g_GameState.CenterView(pt);
}

void SelectedUnitIssueOrder(lua_State *L, int orderType)
{
	g_GameState.IssueOrder((unitOrderType)orderType, NULL);
}

void RenderSelectedUnit(lua_State *L)
{
	POINT pt = {g_pCurContext->pUI->GetScreenRect()->left, g_pCurContext->pUI->GetScreenRect()->top};
	float scale = (g_pCurContext->pUI->GetScreenRect()->right - g_pCurContext->pUI->GetScreenRect()->left)/((float)DEFAULT_HEX_SIZE);
	g_GameState.RenderSelectedUnit(pt, scale);
}

const TCHAR* FormatUnitString(lua_State *L, CHexUnit* pUnit, const char* fmt)
{
	TCHAR buf[256] = {0};

	StringTagContext context;
	context.pUnit = pUnit;

	if (context.pUnit)
		formatStringTags(fmt, buf, 256, formatUnitTag, &context);
	
	return _wcsdup(buf);
}

const TCHAR* FormatCityString(lua_State *L, CHexCity* pCity, const char* fmt)
{
	TCHAR buf[256] = {0};

	StringTagContext context;
	context.pCity = pCity;
	if (context.pCity)
		formatStringTags(fmt, buf, 256, formatCityTag, &context);
	
	return _wcsdup(buf);
}

void ShowDetailedCityView(lua_State *L, CHexCity* pCity)
{
	if (pCity)
	{
		g_GameState.ShowDetailedCityView(pCity);
	}
}

void ShowGameplayUI(lua_State *L)
{
	//called when exiting
	g_GameState.ShowGameplayUI();
}

void ShowTechTreeUI(lua_State *L)
{
	g_GameState.ShowTechTreeUI();
}

hexTile* GetLayoutVarAsTile(lua_State *L)
{
	return (hexTile*)g_pCurContext->pUI->GetLayoutVar();
}

CHexCity* GetLayoutVarAsCity(lua_State *L)
{
	return (CHexCity*)g_pCurContext->pUI->GetLayoutVar();
}

laborSlot* GetLayoutVarAsLaborSlot(lua_State *L)
{
	return (laborSlot*)g_pCurContext->pUI->GetLayoutVar();
}

playerNotification* GetLayoutVarAsNotification(lua_State *L)
{
	return (playerNotification*)g_pCurContext->pUI->GetLayoutVar();
}

techTreeNodeDef* GetLayoutVarAsTechNodeDef(lua_State *L)
{
	return (techTreeNodeDef*)g_pCurContext->pUI->GetLayoutVar();
}

cityProject* GetLayoutVarAsCityProject(lua_State *L)
{
	return (cityProject*)g_pCurContext->pUI->GetLayoutVar();
}

CHexCity* GetSelectedCity(lua_State *L)
{
	return g_GameState.GetSelectedCity();
}

POINT GetTileLoc(lua_State *L, hexTile* pTile)
{
	return pTile->slot.loc;
}

CHexCity* GetCityFromTile(lua_State *L, hexTile* pTile)
{
	if (pTile && pTile->pBuilding && pTile->pBuilding->GetMyType() == kBuilding_City)
	{
		return (CHexCity*)pTile->pBuilding;
	}
	else
	{
		return NULL;
	}
}

POINT TileCoordToScreen(lua_State *L, POINT pt)
{
	return g_GameState.TilePtToScreenPt(pt.x, pt.y);
}

luabind::object GetOnscreenTilesWithCities(lua_State *L)
{
	hexTile** eaTiles = NULL;
	g_GameState.GetOnscreenCityList(&eaTiles, true);
	lua_pushlightuserdata(L, eaTiles);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

float GetMaterialStoragePct(lua_State *L, CHexCity* pCity)
{
	if (pCity)
	{
		return pCity->GetMaterialsPct();
	}
	return 0.0f;
}

luabind::object GetOnscreenTilesWithLabor(lua_State *L, CHexCity* pCity)
{
	laborSlot** eaLabor = NULL;
	g_GameState.GetOnscreenLaborList(&eaLabor, pCity);
	lua_pushlightuserdata(L, eaLabor);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

void LaborToggle(lua_State *L, laborSlot* pSlot)
{
	g_GameState.GetSelectedCity()->ToggleLaborInSlot(pSlot);
}

GameTexturePortion* GetLaborIcon(lua_State *L, laborSlot* pSlot)
{
	if (!pSlot || !pSlot->pLaborOwner)
	{
		return NULL;
	}
	else
	{
		switch (pSlot->pDef->eType)
		{
		case kLabor_Harvest:
			{
				return GET_DEF_FROM_STRING(GameTexturePortion, _T("labor_dig"));
			}break;
		case kLabor_Construction:
			{
				return GET_DEF_FROM_STRING(GameTexturePortion, _T("labor_build"));
			}break;
		}
	}
	return NULL;
}

POINT GetLaborLoc(lua_State *L, laborSlot* pSlot)
{
	return pSlot->loc;
}

const TCHAR* FormatLaborString(lua_State *L, laborSlot* pSlot)
{
	TCHAR buf[128];
	TCHAR* iter = buf;
	if (pSlot)
	{
		if (pSlot->pDef->production > 0)
			iter += wsprintf(iter, _T("|cffffff%i|aicon_production|"), pSlot->pDef->production);
		if (pSlot->pDef->gold > 0)
			iter += wsprintf(iter, _T("|ccccc00%i|aicon_gold|"), pSlot->pDef->gold);
		if (pSlot->pDef->research > 0)
			iter += wsprintf(iter, _T("|cff0000%i|aicon_research|"), pSlot->pDef->research);
		return _wcsdup(buf);
	}
	else
		return NULL;
}

const TCHAR* FormatTopInfoBarString(lua_State *L)
{
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();
	TCHAR buf[128];
	TCHAR* iter = buf;
	if (pPlayer)
	{
		iter += wsprintf(iter, _T(" |cff0000|aicon_research|+%d"), pPlayer->GetSciencePerTurn());
		if (pPlayer->GetGoldPerTurn() >= 0)
			iter += wsprintf(iter, _T("  |ccccc00|aicon_gold|%d(+%d)"), pPlayer->GetBankedGold(), pPlayer->GetGoldPerTurn());
		else
			iter += wsprintf(iter, _T("  |ccccc00|aicon_gold|%d|cff0000(-%d)"), pPlayer->GetBankedGold(), pPlayer->GetGoldPerTurn());
		return _wcsdup(buf);
	}
	else
		return NULL;
}


luabind::object GetCityLabor(lua_State *L, CHexCity* pCity)
{
	laborSlot** eaLabor = NULL;
	laborSlot** eaCityLabor = pCity->GetLaborList();
	eaCopy(&eaLabor, &eaCityLabor);
	
	lua_pushlightuserdata(L, eaLabor);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

luabind::object GetPlayerTechTreeNodeList(lua_State *L)
{
	techTreeNodeDef** eaNodes = NULL;
	eaCopy(&eaNodes, &g_pEvilTechTree->eaNodes);
	
	lua_pushlightuserdata(L, eaNodes);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

/*
	Tech tree currently uses global vars to store scrolling info. Pretty crappy, but temporary.
	*/
int g_TechTreeScrollOffset = 0;

POINT GetTechTreeNodePos(lua_State *L, techTreeNodeDef* pDef)
{
	POINT pt = {(LONG)(g_pCurContext->pUI->GetWidth() * pDef->layoutPt.x + g_TechTreeScrollOffset), (LONG)(g_pCurContext->pUI->GetHeight() * (pDef->layoutPt.y + 0.5))};

	return pt;
}

const TCHAR* GetTechTreeNodeName(lua_State *L, techTreeNodeDef* pDef)
{
	return pDef ? _wcsdup(pDef->name) : NULL;
}


void RenderTechNodeLines(lua_State *L, techTreeNodeDef* pDef)
{
	static GameTexture* pGfx = NULL;
	RECT* pScrRect = g_pCurContext->pUI->GetScreenRect();
	RECT linebox;

	if (!pDef)
		return;

	if (!pGfx)
		pGfx = GET_TEXTURE(_T("interfacenines"));
	for (int i = 0; i < eaSize(&pDef->eaRequiredNames); i++)
	{
		techTreeNodeDef* pReqDef = GET_DEF_FROM_STRING(techTreeNodeDef, pDef->eaRequiredNames[i]);
		POINT reqAnchor = {(LONG)(pScrRect->right - (pDef->layoutPt.x-pReqDef->layoutPt.x)*RECT_WIDTH(pScrRect->) - 56),
							(LONG)(pScrRect->bottom - (((float)pDef->layoutPt.y-pReqDef->layoutPt.y) + 0.5)*RECT_HEIGHT(pScrRect->))};
		linebox.right = pScrRect->left;
		linebox.left  = reqAnchor.x;
		if (pReqDef->layoutPt.y == pDef->layoutPt.y)
		{
			//just a line
			linebox.top = reqAnchor.y - 2;
			linebox.bottom = reqAnchor.y + 1;
			g_Renderer.AddNinepatchToRenderList(pGfx, 10, &linebox, 1.0f);
		}
		else if (pReqDef->layoutPt.y > pDef->layoutPt.y)
		{
			//twisty line
			linebox.top = (g_pCurContext->pUI->GetScreenRect()->bottom + g_pCurContext->pUI->GetScreenRect()->top)/2 - 2;
			linebox.bottom = reqAnchor.y + 1;
			g_Renderer.AddNinepatchToRenderList(pGfx, 9, &linebox, 1.0f);
		}
		else
		{
			//twisty line
			linebox.top = reqAnchor.y - 2;
			linebox.bottom = (g_pCurContext->pUI->GetScreenRect()->bottom + g_pCurContext->pUI->GetScreenRect()->top)/2 + 1;
			g_Renderer.AddNinepatchToRenderList(pGfx, 11, &linebox, 1.0f);
		}
	}
}

void RenderTechBackground(lua_State *L)
{
	UIChildDef* pChildDef = g_pCurContext->pUI->GetDef()->eaChildren[0];

	RECT* pScrRect = g_pCurContext->pUI->GetScreenRect();
	int w = ((UIBoxDef*)(pChildDef->hBox.pObj))->width;
	int h = ((UIBoxDef*)(pChildDef->hBox.pObj))->height;

	//
	DefHash::iterator hashIter;
	DefHash::iterator hashEnd = DEF_ITER_END(techEraDef);

	for(hashIter = DEF_ITER_BEGIN(techEraDef); hashIter != hashEnd; ++hashIter) 
	{
		techEraDef* pDef = (techEraDef*)hashIter->second;
		RECT render = {pDef->iColStart*w + g_TechTreeScrollOffset, pScrRect->top, pDef->iColEnd*w + g_TechTreeScrollOffset, pScrRect->bottom};
		g_Renderer.AddGradientToRenderList(&render, 0xff000033, 0xff000011);
	}
}

void RenderTechButton(lua_State *L)
{
	UIInstance* pUI = g_pCurContext->pUI;
	techTreeNodeDef* pNode =  (techTreeNodeDef*)pUI->GetLayoutVar();
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();

	if (!pNode)
		return;

	bool bOwned = pPlayer->GetTechProgress(pNode->name) == -1;
	bool bValid = !bOwned && pPlayer->CanResearchTech(pNode);
	bool bCurrent = pPlayer->GetCurrentTech() == pNode;
	static GameTexture* pGfx = NULL;

	RECT* pScrRect = pUI->GetScreenRect();

	int iNine = bOwned ? 12 : (bValid ? 4 : 13);
	int iMouseover = bOwned ? 12 : (bValid ? 5 : 13);
	int iClick = bOwned ? 12 : (bValid ? 6 : 13);

	if (!pGfx)
		pGfx = GET_TEXTURE(_T("interfacenines"));

	if (pUI->GetFlags() & UISTATE_MOUSEOVER)
		g_Renderer.AddNinepatchToRenderList(pGfx, iMouseover, pScrRect, 1.0f);
	else if (pUI->GetFlags() & UISTATE_LBUTTONDOWN)
		g_Renderer.AddNinepatchToRenderList(pGfx, iClick, pScrRect, 1.0f);
	else
		g_Renderer.AddNinepatchToRenderList(pGfx, iNine, pScrRect, 1.0f);

	if (bCurrent)
		g_Renderer.AddNinepatchToRenderList(pGfx, 14, pScrRect, ((float)pPlayer->GetTechProgress(pNode->name))/pNode->cost);

}

void TechNodeButtonClick(lua_State *L)
{
	UIInstance* pUI = g_pCurContext->pUI;
	techTreeNodeDef* pNode =  (techTreeNodeDef*)pUI->GetLayoutVar();
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();

	pPlayer->StartResearch(pNode);

}

void TechTreeScrollDisplay(lua_State *L, POINT pt)
{
	g_TechTreeScrollOffset += pt.x;
	if (g_TechTreeScrollOffset > 0)
		g_TechTreeScrollOffset = 0;
}

POINT GetThisFrameMouseDeltas(lua_State *L)
{
	return g_GameState.GetMouseDeltas();
}

luabind::object GetNotifications(lua_State *L)
{
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();
	playerNotification** eaNotes = NULL;
	playerNotification** eaPlayerNotes = pPlayer->GetNotifications();
	eaCopy(&eaNotes, &eaPlayerNotes);
	lua_pushlightuserdata(L, eaNotes);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

void NotificationFocus(lua_State *L, playerNotification* pNote)
{
	switch (pNote->eType)
	{
	case kNotify_Production:
		{
			g_GameState.ShowDetailedCityView((CHexCity*)(pNote->pFocusThing));
		}break;
	case kNotify_Orders:
		{
			g_GameState.SelectUnit((CHexUnit*)(pNote->pFocusThing));
		}break;
	case kNotify_Combat:
		{
			g_GameState.CenterView(pNote->focusLoc);
		}break;
	case kNotify_Tech:
		{
			g_GameState.ShowTechTreeUI();
		}break;
	}
}

void NotificationKill(lua_State *L, playerNotification* pNote)
{
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();
	pPlayer->RemoveNotification(pNote);
}


GameTexturePortion* GetNotificationIcon(lua_State *L, playerNotification* pNote)
{
	return pNote ? pNote->pTex : NULL;
}

const TCHAR* GetNotificationText(lua_State *L, playerNotification* pNote)
{
	return pNote ? _wcsdup(pNote->pchText) : NULL;
}

float GetCurTechProgress(lua_State *L)
{
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();
	return pPlayer->GetTechPct();
}

const TCHAR* GetCurTechName(lua_State *L)
{
	CHexPlayer* pPlayer = g_GameState.GetCurrentPlayer();
	techTreeNodeDef* pNode = pPlayer->GetCurrentTech();
	if (pNode)
		return _wcsdup(pNode->name);
	else
		return _wcsdup(_T("NO RESEARCH"));
}

luabind::object GetAvailableConstructionProjects(lua_State *L, CHexCity* pCity)
{
	cityProject** eaProjects = NULL;
	pCity->GetAvailableProjectList(kProject_None, &eaProjects);
	lua_pushlightuserdata(L, eaProjects);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

void SetNewConstructionProject(lua_State *L, CHexCity* pCity, cityProject* pProject)
{
	switch (pProject->eType)
	{
	case kProject_Building:
		{
			g_GameState.MouseHandlerPushState(kGameplayMouse_PlaceBuilding, pProject, true);
		}break;
	default:
		{
			if (!(GetAsyncKeyState(VK_SHIFT) & (1 << 31)))
				pCity->ClearProductionQueue();
			pCity->AddQueuedProject(pProject->eType, pProject->pDef, pProject->loc);
		}
	}
}

luabind::object GetProductionQueue(lua_State *L, CHexCity* pCity)
{
	cityProject** eaQueue = NULL;
	cityProject** eaCityQueue = pCity->GetProductionQueue();
	for (int i = 1; i < eaSize(&eaCityQueue); i++)
	{
		eaPush(&eaQueue, eaCityQueue[i]);
	}
	lua_pushlightuserdata(L, eaQueue);
	luabind::object ret(luabind::from_stack(L, -1));
	lua_pop(L, 1);
	return ret;
}

cityProject* GetCurrentProject(lua_State *L, CHexCity* pCity)
{
	return pCity->GetCurrentProject();
}

const TCHAR* GetProjectText(lua_State *L, CHexCity* pCity, cityProject* pProject)
{
	TCHAR buf[64] = {0};
	if (pProject)
	{
		switch(pProject->eType)
		{
		case kProject_Building:
			{
				hexBuildingDef* pDef = (hexBuildingDef*)pProject->pDef;
				int estimatedProduction = pCity->GetNetProductionForProjects();
				if (estimatedProduction == 0)
					wsprintf(buf, L"%s - |cff0000----|cffffff turns", pDef->displayName);
				else
					wsprintf(buf, L"%s - %d turns", pDef->displayName, (int)ceilf((pDef->cost-pProject->progress)/estimatedProduction));
				return _wcsdup(buf);
			}break;
		case kProject_Unit:
			{
				hexUnitDef* pDef = (hexUnitDef*)pProject->pDef;
				int estimatedProduction = pCity->GetNetProductionForProjects();
				if (estimatedProduction == 0)
					wsprintf(buf, L"%s - |cff0000----|cffffff turns", pDef->displayName);
				else
					wsprintf(buf, L"%s - %d turns", pDef->displayName, (int)ceilf((pDef->cost-pProject->progress)/estimatedProduction));
				return _wcsdup(buf);
			}break;
		}
	}
	return NULL;
}

GameTexturePortion* GetProjectIcon(lua_State *L, cityProject* pProject)
{
	if (pProject)
	{
		switch(pProject->eType)
		{
		case kProject_Building:
			{
				return GET_REF(GameTexturePortion, ((hexBuildingDef*)pProject->pDef)->hTex);
			}break;
		case kProject_Unit:
			{
				return GET_REF(GameTexturePortion, ((hexUnitDef*)pProject->pDef)->hTex);
			}break;
		}
	}
	return NULL;
}

void PopMouseHandler(lua_State *L)
{
	g_GameState.MouseHandlerPopState();
}

void GenerateMapFromDesc(lua_State *L, const char* pchDescName, int numPlayers)
{
	TCHAR widebuf[32];
	swprintf_s(widebuf, L"%S", pchDescName);
	g_GameState.EndCurrentGame();
	g_GameState.StartNewGame(GET_DEF_FROM_STRING(hexMapGenerationDesc, widebuf), numPlayers);
}

void DisableMapXWrap(lua_State *L, int disable)
{
	g_DebugFlags.disableMapXWrap = disable;
}

int distCalc(POINT a, POINT b)
{
	//swap if necessary
	if (a.y > b.y)
	{
		POINT temp = a;
		a = b;
		b = temp;
	}
	int y = b.y-a.y;
	int halfY = y;
	int x = b.x - a.x;
	if (x > 128/2)
		x = x-128;
	else if (x < -128/2)
		x = x+128;

	//early out if horizontal line, this is required
	if (a.y == b.y)
		return x < 0 ? -x : x;

	//if a is on an odd y-coordinate and halfY is odd, round up the division. Only if positive-x direction.
	if (a.y & 1 && halfY & 1 && x > 0)
		halfY++;
	if (x < 0)
	{
		x = -x;
		if (!(a.y & 1))
			x--;
	}
	
	halfY /= 2;
	if (x < halfY)
		return y;
	
	return y + (x-halfY);
}

void TestDistCalc(lua_State *L, int ax, int ay, int bx, int by)
{
	POINT a = {ax, ay};
	POINT b = {bx, by};
	TCHAR buf[5] = {0};
	wsprintf(buf, L"%d", distCalc(a, b));
	g_Console.AddConsoleString(buf);
}

void ForceWireframe(lua_State *L, int enable)
{
	g_Renderer.ForceWireframe(!!enable);
}

void ForceTextureBlending(lua_State *L, int enable)
{
	g_Renderer.ForceTextureBlending(!!enable);
}

void PrintMouseoverLoc(lua_State *L)
{
	POINT pt;
	TCHAR buf[10] = {0};
	GetCursorPos(&pt);
	pt = g_GameState.PixelToTilePt(pt.x, pt.y);
	wsprintf(buf, L"%d,%d", pt.x, pt.y);
	g_Console.AddConsoleString(buf);
}

void DistCalcUnitTest(lua_State *L)
{
	POINT points[] =	{{0,0},{1,0},	//1
						{0,0},{2,0},	//2
						{0,0},{127,0},	//1
						{0,1},{1,0},	//1
						{0,1},{2,0},	//2
						{0,1},{127,0},	//2
						{1,1},{2,2},	//1
						{1,1},{1,0},	//1
						{1,1},{3,2},	//2
						{3,4},{0,2},	//4
						{3,4},{7,6},	//5
						{4,3},{127,0},	//7
						{3,2},{127,2},	//4
						{1,1},{0,3},	//2
						{1,1},{0,4},	//3
						{1,1},{127,5},	//4
						{2,0},{127,3},	//4
						{3,3},{127,1},	//5
						{3,2},{127,1},	//4
						{3,2},{0,1},	//3
						{3,3},{0,1}		//4
						};
	int answers[] = {1,2,1,1,2,2,1,1,2,4,5,7,4,2,3,4,4,5,4,3,4};
	TCHAR buf[64] = {0};
	int i;
	for (i = 0; i < (sizeof(answers)/sizeof(int)); i++)
	{
		int ans = distCalc(points[i*2], points[i*2+1]);
		if (ans != answers[i])
		{
			wsprintf(buf, L"(%d,%d)->(%d,%d) = %d FAIL, should be %d", points[i*2].x, points[i*2].y, points[i*2+1].x, points[i*2+1].y, ans, answers[i]);
			g_Console.AddConsoleString(buf);
		}
	}
	wsprintf(buf, L"%d calculations performed.", i);
	g_Console.AddConsoleString(buf);
}

void DoAllLuaBinds()
{
	luabind::module(g_LUA)
	[
		luabind::def("Player_EndTurn", &EndTurnButton),
		luabind::def("Player_GetSelectedUnit", &GetSelectedUnit),
		luabind::def("Minimap_Render", &RenderMinimap),
		luabind::def("Minimap_Click", &MinimapClick),
		luabind::def("SelectedUnit_IssueOrder", &SelectedUnitIssueOrder),
		luabind::def("SelectedUnit_Render", &RenderSelectedUnit),
		luabind::def("City_DetailedView", &ShowDetailedCityView),
		luabind::def("Gameplay_ShowUI", &ShowGameplayUI),
		luabind::def("Gameplay_PopMouseHandler", &PopMouseHandler),
		luabind::def("Unit_FormatString", &FormatUnitString),
		luabind::def("City_FormatString", &FormatCityString),
		luabind::def("Labor_FormatString", &FormatLaborString),
		luabind::def("Layout_GetVarAsTile", &GetLayoutVarAsTile),
		luabind::def("Layout_GetVarAsCity", &GetLayoutVarAsCity),
		luabind::def("Layout_GetVarAsLaborSlot", &GetLayoutVarAsLaborSlot),
		luabind::def("Layout_GetVarAsNotification", &GetLayoutVarAsNotification),
		luabind::def("Layout_GetVarAsTechNodeDef", &GetLayoutVarAsTechNodeDef),
		luabind::def("Layout_GetVarAsCityProject", &GetLayoutVarAsCityProject),
		luabind::def("Tile_GetLoc", &GetTileLoc),
		luabind::def("Tile_GetCity", &GetCityFromTile),
		luabind::def("Gameplay_TilePtToScreen", &TileCoordToScreen),
		luabind::def("Gameplay_GetOnscreenTilesWithCities", &GetOnscreenTilesWithCities),
		luabind::def("City_GetOnscreenTilesWithLabor", &GetOnscreenTilesWithLabor),
		luabind::def("City_GetSelected", &GetSelectedCity),
		luabind::def("City_GetMaterialStoragePct", &GetMaterialStoragePct),
		luabind::def("Labor_Toggle", &LaborToggle),
		luabind::def("Labor_GetIcon", &GetLaborIcon),
		luabind::def("Labor_GetLoc", &GetLaborLoc),
		luabind::def("City_GetLaborSlots", &GetCityLabor),
		luabind::def("Tech_GetPlayerTechTreeNodeList", &GetPlayerTechTreeNodeList),
		luabind::def("Tech_GetNodeScreenPos", &GetTechTreeNodePos),
		luabind::def("Tech_GetNodeName", &GetTechTreeNodeName),
		luabind::def("TechTree_ShowUI", &ShowTechTreeUI),
		luabind::def("Tech_RenderTechRequirements", &RenderTechNodeLines),
		luabind::def("Tech_RenderTechTreeBackground", &RenderTechBackground),
		luabind::def("Tech_RenderTechNodeButton", &RenderTechButton),
		luabind::def("Tech_TechNodeButtonClick", &TechNodeButtonClick),
		luabind::def("Gameplay_GetNotificationList", &GetNotifications),
		luabind::def("Notification_Focus", &NotificationFocus),
		luabind::def("Notification_Dismiss", &NotificationKill),
		luabind::def("Notification_GetIcon", &GetNotificationIcon),
		luabind::def("Notification_GetText", &GetNotificationText),
		luabind::def("Player_GetCurrentTechPct", &GetCurTechProgress),
		luabind::def("Player_GetCurrentTechName", &GetCurTechName),
		luabind::def("Tech_ScrollDisplay", &TechTreeScrollDisplay),
		luabind::def("Gameplay_GetThisFrameMouseDeltas", &GetThisFrameMouseDeltas),
		luabind::def("City_GetAvailableProjects", &GetAvailableConstructionProjects),
		luabind::def("City_SetNewProject", &SetNewConstructionProject),
		luabind::def("City_GetProjectText", &GetProjectText),
		luabind::def("CityProject_GetIcon", &GetProjectIcon),
		luabind::def("City_GetProductionQueue", &GetProductionQueue),
		luabind::def("City_GetCurrentProject", &GetCurrentProject),
		luabind::def("Player_FormatTopInfoBar", &FormatTopInfoBarString),
		luabind::def("Debug_GenerateMapFromDesc", &GenerateMapFromDesc),
		luabind::def("Debug_DisableMapXWrap", &DisableMapXWrap),
		luabind::def("Dist", &TestDistCalc),
		luabind::def("DistUnitTest", &DistCalcUnitTest),
		luabind::def("ForceWireframe", &ForceWireframe),
		luabind::def("ForceTextureBlending", &ForceTextureBlending),
		luabind::def("PrintMouseoverLoc", &PrintMouseoverLoc),
		class_<GameTexturePortion>("GameTexturePortion")
		.def(constructor<>()),
		class_<hexTile>("hexTile")
		.def(constructor<>()),
		class_<POINT>("POINT")
		.def(constructor<>()),
		class_<techTreeNodeDef>("techTreeNodeDef")
		.def(constructor<>()),
		class_<playerNotification>("playerNotification")
		.def(constructor<>()),
		class_<laborSlot>("laborSlot")
		.def(constructor<>()),
		class_<CHexCity>("CHexCity")
		.def(constructor<>()),
		class_<TCHAR>("TCHAR")
		.def(constructor<>()),
		class_<CHexUnit>("CHexUnit")
		.def(constructor<>()),
		class_<cityProject>("cityProject")
		.def(constructor<>())
	];
}