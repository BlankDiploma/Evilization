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
#define MOUSEDRAG_SENSITIVITY 0.1f
#define MOUSEZOOM_SENSITIVITY 2.5f
#define MAX_CAM_VELOCITY 1.0f
#define CAM_ANGLE 45.0f
#define FOVY D3DX_PI/4.0f

#define PI D3DX_PI

enum GameTextureType {kTextureType_Invalid = 0, kTextureType_Default, kTextureType_Ninepatch, kTextureType_Font};

PARSE_STRUCT(GameTexturePortion)
{
	const TCHAR* name;
	TEXTURE_REF hTex;
	RECT* rSrc;
	POINT* offset;
	COLOR_ARGB color;
	PARSE_IGNORE IDirect3DVertexBuffer9* pVerts;
	PARSE_IGNORE int iVertIndexStart;
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
	float u1, v1;
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



class FlexFrustum
{
	//Near and far plane distances
	float zn, zf;
	//Width and height of near/far planes
	float fHnear, fWnear, fHfar, fWfar;
	//Eight points that define the corners of the view frustum
	//These are the corners of the near/far planes in world space
	D3DXVECTOR3 ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr;
	//The camera's view ray
	D3DXVECTOR3 camDir;
public:
	//only needs to be called when matProj changes
	void CalcNearFarPlaneDimensions(float fovy, float Aspect, float zn, float zf);


	float GetNearPlaneDist();
	float GetFarPlaneDist();
	void GetNearTopLeft(D3DXVECTOR3* pOut)
	{
		pOut->x = ntl.x;
		pOut->y = ntl.y;
		pOut->z = ntl.z;
	}
	void GetFarTopLeft(D3DXVECTOR3* pOut)
	{
		pOut->x = ftl.x;
		pOut->y = ftl.y;
		pOut->z = ftl.z;
	}
	float GetNearPlaneWidth()
	{
		return fWnear;
	}
	float GetNearPlaneHeight()
	{
		return fHnear;
	}
	float GetFarPlaneWidth()
	{
		return fWfar;
	}
	float GetFarPlaneHeight()
	{
		return fHfar;
	}
	void CalcWorldSpacePlanes(D3DXVECTOR3 vEye, D3DXVECTOR3 vAt, D3DXVECTOR3 vUp);
	int FrustumPlaneIntersection(D3DXVECTOR3 pOut[4], D3DXVECTOR3* pPoint, D3DXVECTOR3* pNorm);

};

//struct representing the camera
class FlexCamera
{
	D3DXMATRIX matView, matProj;
	D3DXVECTOR3 vEye, vAt, vUp;
	CRITICAL_SECTION _csCamera;
	FlexFrustum cameraFrustum;	
	float fCameraWrapBoundaries[3][2];	//[x y z][min max]
	bool bCameraWrapsAlongAxis[3];	//[x y z]
	
	void BuildViewFrustum();

public:
	FlexCamera();
	~FlexCamera();
	void SetCameraEye(float x, float y, float z);
	void SetCameraAt(float x, float y, float z);
	void SetCameraUp(float x, float y, float z);
	void SetCameraEye(D3DXVECTOR3 newEye);
	void SetCameraAt(D3DXVECTOR3 newAt);
	void SetCameraUp(D3DXVECTOR3 newUp);
	void SetViewMatrix(D3DXMATRIX newMatView);
	void SetProjMatrix(D3DXMATRIX newMatProj);
	void BuildViewMatrix();
	void BuildProjMatrix(int screenW, int screenH);
	void GetCameraAt(D3DXVECTOR3* pOut);
	void GetCameraEye(D3DXVECTOR3* pOut);
	void GetCameraUp(D3DXVECTOR3* pOut);
	void GetViewMatrix(D3DXMATRIX* pOut);
	void GetProjMatrix(D3DXMATRIX* pOut);
	void MoveCamera(float fHoriz, float fVert, int iZoom);
	void SetCameraPosition(D3DXVECTOR3* pvPos);
	void CalcFrustumNearFarPlaneDimensions(float fovy, float Aspect, float zn, float zf);
	int CameraFrustumPlaneIntersection(D3DXVECTOR3 pOut[4], D3DXVECTOR3* pPoint, D3DXVECTOR3* pNorm);
	void Rotate(float rot[3]);
	void WorldSpaceToHomogenousScreen(D3DXVECTOR3* pWorld, D3DXVECTOR3* pOut);
	void SetAxisWrapValues(int axis, float min, float max, bool bEnable)
	{
		fCameraWrapBoundaries[axis][0] = min;
		fCameraWrapBoundaries[axis][1] = max;
		bCameraWrapsAlongAxis[axis] = bEnable;
	}
	FlexFrustum* GetFrustum()
	{
		return &cameraFrustum;
	}
	void CastRayThroughPixel(D3DXVECTOR3 pOut[2], int x, int y, int iWidth, int iHeight);
};

struct ModelCall
{
	D3DXMATRIX matWorld;
	const LPDIRECT3DVERTEXBUFFER9* ppVerts;
	IDirect3DIndexBuffer9* pIndices;
	int* piNumTris;
	int* piNumVerts;
	int iVertStart;
	LPDIRECT3DTEXTURE9 pTex;
	D3DPRIMITIVETYPE ePrimitiveType;
};

#define RENDER_LIST_BUFFER_SIZE 2048

enum ShaderTechniques3D {kShader3D_Default = 0, kShader3D_2xBlend, kShader3D_3xBlend, kShader3D_Translucent2xBlend, kShader3D_Translucent, kShader3D_Count};

struct RenderList
{
	ModelCall pCallsByTechnique[kShader3D_Count][RENDER_LIST_BUFFER_SIZE];
	int pModelsUsedByTechnique[kShader3D_Count];

	LPDIRECT3DVERTEXBUFFER9 pSpriteVerts;

	LPDIRECT3DTEXTURE9* eaSpriteTextures;
	int spritesUsed;

	volatile bool bUsed;//the only time in history that I have used "volatile"
	RenderList()
	{
		for (int i = 0; i < kShader3D_Count; i++)
		{
			pModelsUsedByTechnique[i] = 0;
		}
		spritesUsed = 0;
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

class FlexScratchSurface
{
	int size;
	IDirect3DSurface9* pRenderTarget;
	IDirect3DSurface9* pOffscreenPlain;
public:
	FlexScratchSurface(int size);
	~FlexScratchSurface();
	void GetData(DWORD* pDataOut);
	void SaveToPNG(TCHAR* filename);
	IDirect3DSurface9* GetRenderTarget();
};

class FlexRenderer
{
	IDirect3D9* pD3D;
	IDirect3DDevice9* pD3DDevice;
	IDirect3DStateBlock9* stateBlocks[kRendererMode_Count];
	D3DVIEWPORT9 mainView;
	FlexCamera* pCamera;
	ID3DXEffect* pDefaultShader;
	LPDIRECT3DVERTEXDECLARATION9 FlexVertexDecl, FlexVertex2DDecl;
	D3DXHANDLE pShaderTechniques[kShader3D_Count];
	D3DXHANDLE default2DTech;

	int iScreenW;
	int iScreenH;
	float fAspect;

	boolean bActiveFrame, bActive2D;
	LPDIRECT3DVERTEXBUFFER9* eaVertsToDestroy;

	RenderList* pCurRenderList;//Render list that is currently queued for display.
	RenderList* pNextRenderList;//Render list that is next in line and guaranteed not to be overwritten.
	RenderList* pFutureRenderList;//Render list that may be overwritten entirely before it ever gets to display.

	CRITICAL_SECTION _csNextRenderList;

	D3DXMATRIX MakeProjectionMatrix(const float near_plane, 
                 const float far_plane,  
                 const float fov_horiz, 
                 const float fov_vert);
	void UpdateCamera();
	void BeginFrame();
	void EndFrame();
	void Begin2D();
	void End2D();
	void AddParticlesToRenderList();

	IDirect3DVertexBuffer9* pCubeVertBuffer;
	

	void CreateTextureAtlasVertexBuffer(const GameTexture* pSrcTexture, GameTexturePortion** eaPortions);
	LPDIRECT3DVERTEXBUFFER9* eaAtlasVertexBuffers;

	
	bool bForceWireframe;
	bool bForceTextureBlending;
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
	void ForceWireframe(bool bEnable)
	{
		bForceWireframe = bEnable;
	}
	void ForceTextureBlending(bool bEnable)
	{
		bForceTextureBlending = bEnable;
	}

	FlexCamera* GetCamera()
	{
		return pCamera;
	}
	int GetScreenWidth()
	{
		return iScreenW;
	}
	int GetScreenHeight()
	{
		return iScreenH;
	}

	void PlaneIntersectRay(D3DXVECTOR3* pOut, const D3DXVECTOR3* pPlanePoint, const D3DXVECTOR3* pPlaneNorm, const D3DXVECTOR3* pRayPoint1, const D3DXVECTOR3* pRayPoint2);
	FLOATPOINT ScaleScreenCoords(int x, int y);
	
	void WorldSpaceToScreen(D3DXVECTOR3* pWorld, POINT* pOut);
	void StartNewRenderList();
	void CommitRenderList();
	void Add3DTexturePortionToRenderList(const GameTexturePortion* pTex, float pos[3], float scale[3], float rot[3], ShaderTechniques3D eTechnique);
	void AddModelToRenderList(IDirect3DVertexBuffer9** ppVerts, IDirect3DIndexBuffer9* pIndices, int* piNumTris, int* piNumVerts, const GameTexture* pTex, float pos[3], float scale[3], float rot[3], ShaderTechniques3D eTechnique);
	void AddSpriteToRenderList(const GameTexture* pTex, RECT* pDst, RECT* pSrc, DWORD color = 0xffffffff);
	void AddSpriteToRenderList(const GameTexture* pTex, POINT topleft, RECT* pSrc, DWORD color = 0xffffffff);
	void AddSpriteToRenderList(const GameTexture* pTex, int x, int y, RECT* pSrc, DWORD color = 0xffffffff);
	void AddSpriteToRenderList(const GameTexturePortion* pTex, POINT topleft, DWORD color = 0xffffffff, float fZoom = 1.0f);
	void AddNinepatchToRenderList(const GameTexture* pSet, int iIndex, RECT* pDst, float fBarPct);
	void AddStringToRenderList(const GameTexture* pFontTex, const TCHAR* pString, float x, float y, D3DXCOLOR color, bool centered, int wrapWidth, bool bShadow, float fIconScale = 1.0f);
	void AddSolidColorToRenderList(RECT* dst, DWORD color);
	void AddGradientToRenderList(RECT* dst, DWORD colorA, DWORD colorB);
	void ProcessRenderLists();
	HRESULT CreateVertexBuffer(unsigned int Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pHandle);
	void RenderCubeAtPoint(D3DXVECTOR3 vPoint);
	void QueueVertexBufferForDestruction(LPDIRECT3DVERTEXBUFFER9 pVerts);
	void CastRayThroughPixel(D3DXVECTOR3 pOut[2], int x, int y);
	void CreateAllTextureAtlasBuffers();
	void TessellateTriangleIntoBuffer(FlexVertex* vA, FlexVertex* vB, FlexVertex* vC, float fBlendA, float fBlendB, float fBlendC, int iDegree, FlexVertex** vertBufferOut);
	int GetNumTessellatedTriangles(int iDegree);
	void RenderToScratchSurface(FlexScratchSurface* pSurface, GameTexture* pTexture, FlexVertex2D* ppVerts, int iNumVerts);
};

extern FlexRenderer g_Renderer;
extern LPDIRECT3DVERTEXBUFFER9 g_pCubeVertex;