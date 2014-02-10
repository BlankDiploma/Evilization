#include "stdafx.h"
#include <d3d9.h>
#include <math.h>
#include <d3dx9.h>
#include <d3dx9math.h>
#include <assert.h>
#include "StructParse.h"
#include "TextureLibrary.h"
//#include "ModelLibrary.h"

#pragma once

//Renderer should support a table of IDirect3DStateBlock9 objects keyed by the XRendererMode enum.
//See http://msdn.microsoft.com/en-us/library/windows/desktop/bb206121(v=vs.85).aspx for info
#define MOUSELOOK_SENSITIVITY 0.001f
#define MAX_CAM_VELOCITY 1.0f

enum GameTextureType {kTextureType_Invalid = 0, kTextureType_Default, kTextureType_Ninepatch, kTextureType_Font};

PARSE_STRUCT(GameTexturePortion)
{
	const TCHAR* name;
	TEXTURE_REF hTex;
	RECT* rSrc;
	POINT* offset;
	GameTexturePortion()
	{
	}
};

struct ninepatch
{
	RECT rects[9];
	void Set(RECT* pOuter, RECT* pInner)
	{
		SetRect(&rects[0], pOuter->left, pOuter->top, pInner->left, pInner->top);
		SetRect(&rects[1], pInner->left, pOuter->top, pInner->right, pInner->top);
		SetRect(&rects[2], pInner->right, pOuter->top, pOuter->right, pInner->top);
		SetRect(&rects[3], pOuter->left, pInner->top, pInner->left, pInner->bottom);
		SetRect(&rects[4], pInner->left, pInner->top, pInner->right, pInner->bottom);
		SetRect(&rects[5], pInner->right, pInner->top, pOuter->right, pInner->bottom);
		SetRect(&rects[6], pOuter->left, pInner->bottom, pInner->left, pOuter->bottom);
		SetRect(&rects[7], pInner->left, pInner->bottom, pInner->right, pOuter->bottom);
		SetRect(&rects[8], pInner->right, pInner->bottom, pOuter->right, pOuter->bottom);
	}
};


struct GameTexture
{
	GameTextureType eType;
	IDirect3DTexture9* pD3DTex;
	ninepatch* patches;
	int iNum;
	int letterWidth;
	int letterHeight;
	int width, height;
	GameTexture()
	{
		eType = kTextureType_Invalid;
		pD3DTex = NULL;
		patches = NULL;
	}
};

inline int charToHex(TCHAR ch)
{
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	else if (ch >= 'a' && ch <= 'f')
		return (ch-'a')+10;
	else if (ch >= 'A' && ch <= 'F')
		return (ch-'A')+10;

	return 0;
};

struct FlexVertex
{
	float x, y, z;
	DWORD diffuse;
	float u, v;
};
struct FlexVertex2D
{
	float x, y, z, rhw;
	DWORD diffuse;
	float u, v;
};

struct FlexMesh
{
	float x, y, z;
	FlexVertex* vecMeshVerts;
	int iNumTris;
	int iNumVerts;
};

enum FlexPrimitiveType
{
	kPrimitiveType_Cube = 0,
	kPrimitiveType_Cylinder,
	kPrimitiveType_Sphere,
};


enum FlexRendererMode
{
	kRendererMode_Default3D = 0,
	kRendererMode_2D,
	kRendererMode_Wireframe3D,
	kRendererMode_Count
};

//struct representing the camera
struct FlexCamera
{
	D3DXVECTOR3 vEye, vAt, vUp, vRight;
	D3DXVECTOR3 vVelocity;
	float fPitch, fYaw, fRoll;
};

struct ModelCall
{
	D3DXMATRIX matWorld;
	LPDIRECT3DVERTEXBUFFER9* ppVerts;
	int* piNumTris;
	LPDIRECT3DTEXTURE9 pTex;
};

#define RENDER_LIST_BUFFER_SIZE 4096

struct RenderList
{
	ModelCall models[RENDER_LIST_BUFFER_SIZE];
	int modelsUsed;
	ModelCall translucentModels[RENDER_LIST_BUFFER_SIZE];
	int translucentModelsUsed;
	LPDIRECT3DVERTEXBUFFER9 pSpriteVerts;

	LPDIRECT3DTEXTURE9* eaSpriteTextures;
	int spritesUsed;

	volatile bool bUsed;//the only time in history that I have used "volatile"
	RenderList()
	{
		modelsUsed = spritesUsed = translucentModelsUsed = 0;
		eaSpriteTextures = NULL;
		pSpriteVerts = NULL;
		bUsed = true;
	}
};

//Class representing a 3D model?
class FlexModel
{
	FlexVertex** eaVerts;
	IDirect3DTexture9* pTex;
};

class FlexRenderer
{
	IDirect3D9* pD3D;
	IDirect3DDevice9* pD3DDevice;
	IDirect3DStateBlock9* stateBlocks[kRendererMode_Count];
	D3DVIEWPORT9 mainView;
	D3DXMATRIX matView, matProj;
	FlexCamera camera;
	int iScreenW;
	int iScreenH;
	float fAspect;
	boolean bActiveFrame, bActive2D;
	LPDIRECT3DVERTEXBUFFER9* eaVertsToDestroy;

	RenderList* pCurRenderList;//Render list that is currently queued for display.
	RenderList* pNextRenderList;//Render list that is next in line and guaranteed not to be overwritten.
	RenderList* pFutureRenderList;//Render list that may be overwritten entirely before it ever gets to display.

	CRITICAL_SECTION _csNextRenderList;
	CRITICAL_SECTION _csCamera;

	D3DXMATRIX MakeProjectionMatrix(const float near_plane, 
                 const float far_plane,  
                 const float fov_horiz, 
                 const float fov_vert);
	void SetCameraEye(float x, float y, float z);
	void SetCameraAt(float x, float y, float z);
	void SetCameraRight(float x, float y, float z);
	void SetCameraUp(float x, float y, float z);
	void BeginFrame();
	void EndFrame();
	void Begin2D();
	void End2D();
public:
	FlexRenderer();
	~FlexRenderer();

	IDirect3DDevice9* GetD3DDevice()
	{
		return pD3DDevice;
	}
	void Initialize(HWND hWndMain, int screenW, int screenH);
	void SetRenderMode(FlexRendererMode eMode);
	void GetTextureDimensions(IDirect3DTexture9* pTex, float dimensions[2]);

	void DoMouselook(POINT delta);
	void UpdateCamera();
	void MoveCamera(float fForwards, float fStrafe);
	void SetCameraPosition(D3DXVECTOR3* pvPos);
	const FlexCamera* GetCamera()
	{
		return &camera;
	}
	int GetScreenWidth()
	{
		return iScreenW;
	}
	int GetScreenHeight()
	{
		return iScreenH;
	}

	void StartNewRenderList();
	void CommitRenderList();
	void AddModelToRenderList(IDirect3DVertexBuffer9** ppVerts, int* piNumTris, GameTexture* pTex, float pos[3], float scale[3], float rot[3], bool bTranslucent);
	void AddSpriteToRenderList(GameTexture* pTex, RECT* pDst, RECT* pSrc, DWORD color = 0xffffffff);
	void AddSpriteToRenderList(GameTexture* pTex, POINT topleft, RECT* pSrc, DWORD color = 0xffffffff);
	void AddSpriteToRenderList(GameTexture* pTex, int x, int y, RECT* pSrc, DWORD color = 0xffffffff);
	void AddSpriteToRenderList(GameTexturePortion* pTex, POINT topleft, DWORD color = 0xffffffff, float fZoom = 1.0f);
	void AddNinepatchToRenderList(GameTexture* pSet, int iIndex, RECT* pDst, float fBarPct);
	void AddStringToRenderList(GameTexture* pFontTex, const TCHAR* pString, float x, float y, D3DXCOLOR color, bool centered, int wrapWidth, bool bShadow, float fIconScale = 1.0f);
	void AddSolidColorToRenderList(RECT* dst, DWORD color);
	void AddGradientToRenderList(RECT* dst, DWORD colorA, DWORD colorB);
	void ProcessRenderLists();
	HRESULT CreateVertexBuffer(unsigned int Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pHandle);

	void QueueVertexBufferForDestruction(LPDIRECT3DVERTEXBUFFER9 pVerts);

};

extern FlexRenderer g_Renderer;
extern LPDIRECT3DVERTEXBUFFER9 g_pCubeVertex;