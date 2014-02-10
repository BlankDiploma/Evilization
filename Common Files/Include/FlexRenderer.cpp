#include "stdafx.h"
#include <DxErr.h>
#include "FlexRenderer.h"
#include "earray.h"
#include "DefLibrary.h"

LPDIRECT3DVERTEXBUFFER9 g_pCubeVertex = NULL;
LPDIRECT3DVERTEXBUFFER9 g_p2DVertices = NULL;

void FlexRenderer::SetCameraEye(float x, float y, float z)
{
	camera.vEye.x = x;
	camera.vEye.y = y;
	camera.vEye.z = z;
}

void FlexRenderer::SetCameraAt(float x, float y, float z)
{
	camera.vAt.x = x;
	camera.vAt.y = y;
	camera.vAt.z = z;
}

void FlexRenderer::SetCameraRight(float x, float y, float z)
{
	camera.vRight.x = x;
	camera.vRight.y = y;
	camera.vRight.z = z;
}

void FlexRenderer::SetCameraUp(float x, float y, float z)
{
	camera.vUp.x = x;
	camera.vUp.y = y;
	camera.vUp.z = z;
}
 
void FlexRenderer::MoveCamera(float fForwards, float fStrafe)
{
	EnterCriticalSection(&_csCamera);
	camera.vEye += camera.vAt * fForwards;
	camera.vEye += camera.vRight * fStrafe;
	LeaveCriticalSection(&_csCamera);
}

void FlexRenderer::DoMouselook(POINT delta)
{
	EnterCriticalSection(&_csCamera);
	float xAngle = delta.x * 1.0f * MOUSELOOK_SENSITIVITY;
	float yAngle = delta.y * 1.0f * MOUSELOOK_SENSITIVITY;

    D3DXMATRIX rotation;
	float fMaxPitch = D3DXToRadian( 85.0f );

	D3DXMatrixRotationAxis( &rotation, &camera.vUp, xAngle );
	D3DXVec3TransformNormal( &camera.vRight, &camera.vRight, &rotation );
	D3DXVec3TransformNormal( &camera.vAt, &camera.vAt, &rotation );
	
 
 //   radians = (m_invertY) ? -radians : radians;
    camera.fPitch -= yAngle;
    if ( camera.fPitch > fMaxPitch )
    {
        yAngle += camera.fPitch - fMaxPitch;
		camera.fPitch -= camera.fPitch - fMaxPitch;
    }
    else if ( camera.fPitch < -fMaxPitch )
    {
        yAngle += camera.fPitch + fMaxPitch;
		camera.fPitch -= camera.fPitch + fMaxPitch;
    }
 
    D3DXMatrixRotationAxis( &rotation, &camera.vRight, yAngle );
    D3DXVec3TransformNormal( &camera.vUp, &camera.vUp, &rotation );
    D3DXVec3TransformNormal( &camera.vAt, &camera.vAt, &rotation );
	LeaveCriticalSection(&_csCamera);
}

void FlexRenderer::UpdateCamera()
{
	EnterCriticalSection(&_csCamera);
    D3DXVECTOR3 vLookAt = camera.vEye + camera.vAt;
     
    // Calculate the new view matrix
	D3DXVECTOR3 up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

    D3DXMatrixLookAtLH( &matView, &camera.vEye, &vLookAt, &up );
 
    // Set the camera axes from the view matrix
	camera.vRight.x = matView._11; 
    camera.vRight.y = matView._21; 
    camera.vRight.z = matView._31; 
    camera.vUp.x = matView._12;
    camera.vUp.y = matView._22;
    camera.vUp.z = matView._32;
    camera.vAt.x = matView._13;
    camera.vAt.y = matView._23;
    camera.vAt.z = matView._33;

    // Calculate yaw and pitch
    float lookLengthOnXZ = sqrtf( camera.vAt.z * camera.vAt.z + camera.vAt.x * camera.vAt.x );
	camera.fPitch = atan2f( camera.vAt.y, lookLengthOnXZ );
    camera.fYaw   = atan2f( camera.vAt.x, camera.vAt.z );
	pD3DDevice->SetTransform(D3DTS_VIEW, &matView);
	LeaveCriticalSection(&_csCamera);
}

void FlexRenderer::Initialize(HWND hWndMain, int screenW, int screenH)
{
	HRESULT r = 0;
	D3DDISPLAYMODE d3ddm;
	D3DPRESENT_PARAMETERS d3dpp;
	LPDIRECT3DSURFACE9 pBack = 0;
	D3DXMATRIX matWorld;
	pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D == NULL)
	{
		return;
	}

	r = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	if(FAILED(r))
	{
		return;
	}
	
	InitializeCriticalSection(&_csNextRenderList);
	InitializeCriticalSection(&_csCamera);

	iScreenW = screenW;
	iScreenH = screenH;

	fAspect = (float) iScreenW / (float) iScreenH;

	SetCameraEye(0,0,-8);
	SetCameraAt(0,0,1);
	SetCameraRight(1,0,0);
	SetCameraUp(0,1,0);

	ZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWndMain;
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.BackBufferWidth = iScreenW;
	d3dpp.BackBufferHeight = iScreenH;
	d3dpp.BackBufferCount = 1;
//	d3dpp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES ;
//	d3dpp.MultiSampleQuality = 0;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
//	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	//fallback z-buffer support for SHITTY CARDS
	if(FAILED(pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, 
									D3DDEVTYPE_HAL, 
									d3ddm.Format, 
									D3DUSAGE_DEPTHSTENCIL, 
									D3DRTYPE_SURFACE,
									D3DFMT_D24S8)))
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	r = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pD3DDevice);

	/*
	TCHAR buf[512] = {0};
	if (FAILED(r)) {
		wsprintf(buf, L"Error: %s error description: %s\n",
			DXGetErrorString(r), DXGetErrorDescription(r));
		assert(0);
	}
	*/

	// World matrix setup
	float pos[3] = {0,0,0};
	pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

	// View matrix setup
	ZeroMemory(&matView, sizeof(matView));
	D3DXMatrixLookAtLH(&matView, &camera.vEye, &camera.vAt, &camera.vUp);
	pD3DDevice->SetTransform(D3DTS_VIEW, &matView); 

	// Projection matrix setup
	//g_MatProj = MakeProjectionMatrix(1, 20, 120, 120);
	ZeroMemory(&matProj, sizeof(matProj));
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4.0f, fAspect, 0.1f, 400.0f);
	pD3DDevice->SetTransform(D3DTS_PROJECTION, &matProj);

	mainView.X = 0;
	mainView.Y = 0;
	mainView.Height = screenH;
	mainView.Width = screenW;
	mainView.MinZ = 0.0;
	mainView.MaxZ = 1.0;

	pD3DDevice->SetViewport(&mainView);

	//set vertex shader

	//set up render modes

	//Default 3D Rendering
	pD3DDevice->BeginStateBlock();
	pD3DDevice->SetFVF ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1) ;
	pD3DDevice->SetRenderState ( D3DRS_LIGHTING , FALSE ) ;
	pD3DDevice->SetRenderState ( D3DRS_CULLMODE , D3DCULL_CCW ) ;
	pD3DDevice->SetRenderState ( D3DRS_ZENABLE, D3DZB_TRUE);
	pD3DDevice->SetRenderState ( D3DRS_ZWRITEENABLE, D3DZB_TRUE);
//	pD3DDevice->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS , TRUE);
	pD3DDevice->EndStateBlock( &stateBlocks[kRendererMode_Default3D] );

	//Default 2D Rendering
	pD3DDevice->BeginStateBlock();
	pD3DDevice->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1) ;
	pD3DDevice->SetRenderState ( D3DRS_CULLMODE , D3DCULL_NONE ) ;
	pD3DDevice->SetRenderState ( D3DRS_ZENABLE, D3DZB_FALSE);
	pD3DDevice->SetRenderState ( D3DRS_ZWRITEENABLE, D3DZB_FALSE);
	pD3DDevice->SetRenderState ( D3DRS_LIGHTING , FALSE ) ;
	pD3DDevice->SetRenderState ( D3DRS_ALPHABLENDENABLE , TRUE ) ;
	pD3DDevice->SetRenderState ( D3DRS_ALPHAFUNC , D3DCMP_GREATEREQUAL ) ;
	pD3DDevice->SetRenderState ( D3DRS_ALPHAREF , 1 ) ;
	pD3DDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE  );
	pD3DDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE );
	pD3DDevice->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE );
	pD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	pD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	pD3DDevice->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
	pD3DDevice->EndStateBlock( &stateBlocks[kRendererMode_2D] );

	//Wireframe 3D
	pD3DDevice->BeginStateBlock();
	pD3DDevice->SetRenderState ( D3DRS_LIGHTING , FALSE ) ;
	pD3DDevice->SetRenderState ( D3DRS_CULLMODE , D3DCULL_CCW ) ;
	pD3DDevice->SetRenderState ( D3DRS_FILLMODE , D3DFILL_WIREFRAME ) ;
	pD3DDevice->EndStateBlock( &stateBlocks[kRendererMode_Wireframe3D] );

	stateBlocks[kRendererMode_Default3D]->Apply();

	pD3DDevice->CreateVertexBuffer(36*sizeof(FlexVertex), 0, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &g_pCubeVertex, NULL);

	BYTE* pVertices;
	FlexVertex data[]={
	//Cube vertices
				//Front face
				{0.0f,0.0f,0.0f,0xFF603913,0,0.5},{0.0f, 1.0f,0.0f,0xFF603913,0,0},{ 1.0f, 1.0f,0.0f,0xFF603913,0.5,0},
				{ 1.0f, 1.0f,0.0f,0xFF603913,0.5,0},{ 1.0f,0.0f,0.0f,0xFF603913,0.5,0.5},{0.0f,0.0f,0.0f,0xFF603913,0,0.5},
				//Back face
				{ 1.0f,0.0f, 1.0f,0xFF603913,0,0},{ 1.0f, 1.0f, 1.0f,0xFF603913,0,0},{0.0f, 1.0f, 1.0f,0xFF603913,0,0},
				{0.0f, 1.0f, 1.0f,0xFF603913,0,0},{0.0f,0.0f, 1.0f,0xFF603913,0,0},{ 1.0f,0.0f, 1.0f,0xFF603913,0,0},
				//Top face
				{0.0f, 1.0f,0.0f,0xFF00a651,0,0},{0.0f, 1.0f, 1.0f,0xFF00a651,0,0},{ 1.0f, 1.0f, 1.0f,0xFF00a651,0,0},
				{ 1.0f, 1.0f, 1.0f,0xFF00a651,0,0},{ 1.0f, 1.0f,0.0f,0xFF00a651,0,0},{0.0f, 1.0f,0.0f,0xFF00a651,0,0},
				//Bottom face
				{ 1.0f,0.0f,0.0f,0xFF603913,0,0},{ 1.0f,0.0f, 1.0f,0xFF603913,0,0},{0.0f,0.0f, 1.0f,0xFF603913,0,0},
				{0.0f,0.0f, 1.0f,0xFF603913,0,0},{0.0f,0.0f,0.0f,0xFF603913,0,0},{ 1.0f,0.0f,0.0f,0xFF603913,0,0},
				//Left face
				{0.0f,0.0f, 1.0f,0xFF603913,0,0},{0.0f, 1.0f, 1.0f,0xFF603913,0,0},{0.0f, 1.0f,0.0f,0xFF603913,0,0},
				{0.0f, 1.0f,0.0f,0xFF603913,0,0},{0.0f,0.0f,0.0f,0xFF603913,0,0},{0.0f,0.0f, 1.0f,0xFF603913,0,0},
				//Right face
				{ 1.0f,0.0f,0.0f,0xFF603913,0,0},{ 1.0f, 1.0f,0.0f,0xFF603913,0,0},{ 1.0f, 1.0f, 1.0f,0xFF603913,0,0},
				{ 1.0f, 1.0f, 1.0f,0xFF603913,0,0},{ 1.0f,0.0f, 1.0f,0xFF603913,0,0},{ 1.0f,0.0f,0.0f,0xFF603913,0,0},
			};
	g_pCubeVertex->Lock(0, 0, (void**)&pVertices, 0);
	memcpy(pVertices, data, sizeof(data));
	g_pCubeVertex->Unlock();
	
	//let's assume that 1,000 2d sprites is enough for now
	pD3DDevice->CreateVertexBuffer(4*RENDER_LIST_BUFFER_SIZE*sizeof(FlexVertex2D), D3DUSAGE_DYNAMIC, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &pFutureRenderList->pSpriteVerts, NULL);
	
	pD3DDevice->CreateVertexBuffer(4*RENDER_LIST_BUFFER_SIZE*sizeof(FlexVertex2D), D3DUSAGE_DYNAMIC, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &pNextRenderList->pSpriteVerts, NULL);
	
	pD3DDevice->CreateVertexBuffer(4*RENDER_LIST_BUFFER_SIZE*sizeof(FlexVertex2D), D3DUSAGE_DYNAMIC, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &pCurRenderList->pSpriteVerts, NULL);
	
}


FlexRenderer::FlexRenderer()
{
	pFutureRenderList = new RenderList;
	pNextRenderList = new RenderList;
	pCurRenderList = new RenderList;
	eaVertsToDestroy = NULL;
}

FlexRenderer::~FlexRenderer()
{
	for (int i = 0; i < kRendererMode_Count; i++)
	{
		if (stateBlocks[i])
			stateBlocks[i]->Release();
	}
	if (pD3D)
	{
		pD3D->Release();
	}
	if (pD3DDevice)
	{
		pD3DDevice->Release();
	}
	if (g_pCubeVertex)
		g_pCubeVertex->Release();

	if (pFutureRenderList->pSpriteVerts)
		pFutureRenderList->pSpriteVerts->Release();
	if (pNextRenderList->pSpriteVerts)
		pNextRenderList->pSpriteVerts->Release();
	if (pCurRenderList->pSpriteVerts)
		pCurRenderList->pSpriteVerts->Release();
	
	delete pCurRenderList;
	delete pNextRenderList;
	delete pFutureRenderList;
	
	DeleteCriticalSection(&_csNextRenderList);
	DeleteCriticalSection(&_csCamera);
}

void FlexRenderer::SetRenderMode(FlexRendererMode eMode)
{
}
/*
void FlexRenderer::PutModel(FlexModel* pModel, float pos[3], float scale[3], float rot[3])
{
}

void FlexRenderer::PutVertexBuffer(IDirect3DVertexBuffer9* pVerts, int numTris, LPDIRECT3DTEXTURE9 pTex, float pos[3], float scale[3], float rot[3])
{
	D3DXMATRIX matWorld;
	D3DXMATRIX matTrans, matRot, matScale;

	D3DXMatrixRotationYawPitchRoll(&matRot,0.0f,0.0f,0.0f);
	D3DXMatrixTranslation(&matTrans, pos[0], pos[1], pos[2]);
	D3DXMatrixScaling(&matScale, scale[0], scale[1], scale[2]);
	D3DXMatrixMultiply(&matWorld, &matScale, &matRot);
	D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);

	pD3DDevice->SetTransform(D3DTS_WORLD,&matWorld);

	if (pTex)
		pD3DDevice->SetTexture ( 0 , pTex );
	pD3DDevice->SetStreamSource(0, pVerts, 0, sizeof(FlexVertex));
	pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, numTris);
}

void FlexRenderer::PutPrimitive(FlexPrimitiveType eType, LPDIRECT3DTEXTURE9 pTex, float pos[3], float scale[3], float rot[3])
{
	switch (eType)
	{
	case kPrimitiveType_Cube:
		{
			D3DXMATRIX matWorld;
			D3DXMATRIX matTrans, matRot, matScale;

			D3DXMatrixRotationYawPitchRoll(&matRot,0.0f,0.0f,0.0f);
			D3DXMatrixTranslation(&matTrans, pos[0], pos[1], pos[2]);
			D3DXMatrixScaling(&matScale, scale[0], scale[1], scale[2]);
			D3DXMatrixMultiply(&matWorld, &matScale, &matRot);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);

			pD3DDevice->SetTransform(D3DTS_WORLD,&matWorld);
			
			pD3DDevice->SetStreamSource(0, g_pCubeVertex, 0, sizeof(FlexVertex));
			if (pTex)
				pD3DDevice->SetTexture ( 0 , pTex );
			pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);
		}break;
	}
}



void FlexRenderer::PutTexturedQuad2D(IDirect3DTexture9* pTex, RECT*pDst, RECT*pSrc)
{
	const float pos[2] = {pDst->left, pDst->top};
	const float scale[2] = {(pDst->right-pDst->left)/(pSrc->right-pSrc->left),(pDst->bottom-pDst->top)/(pSrc->bottom-pSrc->top)};


	SetSpriteTransform(pos, scale);
	pSprite->Draw(pTex,pSrc,NULL,NULL,0xFFFFFFFF);

}


void FlexRenderer::PutTexturedQuad2D(IDirect3DTexture9* pTex, const float pos[2], const float scale[2], RECT* pSrc)
{
	const float identityPos[2] = {0,0};
	const float identityScale[2] = {1,1};

	if (!pos)
		pos = identityPos;

	if (!scale)
		scale = identityScale;

	SetSpriteTransform(pos, scale);
	pSprite->Draw(pTex,pSrc,NULL,NULL,0xFFFFFFFF);

}
//void FlexRenderer::PutTexturedQuad2D(IDirect3DTexture9* pTex, const float topleft[2], const float bottomright[2])
//{
//	//pSprite->Draw(pTex,NULL,NULL,&pos,0xFFFFFFFF);
//
//}
//
//void FlexRenderer::PutTexturedQuad2D(IDirect3DTexture9* pTex, RECT* screenArea)
//{
//
//	//pSprite->Draw(pTex,NULL,NULL,&pos,0xFFFFFFFF);
//}

void FlexRenderer::PutTexturedQuad3D(IDirect3DTexture9* pTex, float topleft[3], float bottomright[3])
{
}
*/
	
void FlexRenderer::StartNewRenderList()
{
	pFutureRenderList->bUsed = false;
	pFutureRenderList->modelsUsed = 0;
	pFutureRenderList->translucentModelsUsed = 0;
	pFutureRenderList->spritesUsed = 0;
	eaClear(&pFutureRenderList->eaSpriteTextures);
}

void FlexRenderer::CommitRenderList()
{
	RenderList* pTemp;
	if (pNextRenderList->bUsed)
	{
		pTemp = pNextRenderList;

		EnterCriticalSection(&_csNextRenderList);
		pNextRenderList = pFutureRenderList;
		LeaveCriticalSection(&_csNextRenderList);

		pFutureRenderList = pTemp;
	}
}

void FlexRenderer::AddModelToRenderList(IDirect3DVertexBuffer9** ppVerts, int* piNumTris, GameTexture* pTex, float pos[3], float scale[3], float rot[3], bool bTranslucent)
{
	assert(pFutureRenderList->modelsUsed < RENDER_LIST_BUFFER_SIZE);

	ModelCall* pModel = bTranslucent ? 
		&pFutureRenderList->translucentModels[pFutureRenderList->translucentModelsUsed++] : 
		&pFutureRenderList->models[pFutureRenderList->modelsUsed++];
	pModel->ppVerts = ppVerts;
	pModel->pTex = pTex ? pTex->pD3DTex : NULL;
	pModel->piNumTris = piNumTris;

	D3DXMATRIX matTrans, matRot, matScale;

	D3DXMatrixRotationYawPitchRoll(&matRot,0.0f,0.0f,0.0f);
	D3DXMatrixTranslation(&matTrans, pos[0], pos[1], pos[2]);
	D3DXMatrixScaling(&matScale, scale[0], scale[1], scale[2]);
	D3DXMatrixMultiply(&pModel->matWorld, &matScale, &matRot);
	D3DXMatrixMultiply(&pModel->matWorld, &pModel->matWorld, &matTrans);
}

void FlexRenderer::AddSolidColorToRenderList(RECT* dst, DWORD color)
{
	assert(pFutureRenderList->spritesUsed < RENDER_LIST_BUFFER_SIZE);

	BYTE* pVertices;
	//sprite vertices
	FlexVertex2D data[]={
				{(float)dst->left-0.5f,(float)dst->top-0.5f,0.0f,1.0f,color,0,0},
				{(float)dst->left-0.5f,(float)dst->bottom-0.5f,0.0f,1.0f,color,0,0},
				{(float)dst->right-0.5f,(float)dst->top-0.5f,0.0f,1.0f,color,0,0},
				{(float)dst->right-0.5f,(float)dst->bottom-0.5f,0.0f,1.0f,color,0,0},
			};
	pFutureRenderList->pSpriteVerts->Lock(pFutureRenderList->spritesUsed*4 *sizeof(FlexVertex2D), sizeof(FlexVertex2D)*4, (void**)&pVertices, D3DLOCK_DISCARD);
	memcpy(pVertices, data, sizeof(data));
	pFutureRenderList->pSpriteVerts->Unlock();
	pFutureRenderList->spritesUsed++;
	eaPush(&pFutureRenderList->eaSpriteTextures, NULL);
}

void FlexRenderer::AddGradientToRenderList(RECT* dst, DWORD colorA, DWORD colorB)
{
	assert(pFutureRenderList->spritesUsed < RENDER_LIST_BUFFER_SIZE);

	BYTE* pVertices;
	//sprite vertices
	FlexVertex2D data[]={
				{(float)dst->left-0.5f,(float)dst->top-0.5f,0.0f,1.0f,colorA,0,0},
				{(float)dst->left-0.5f,(float)dst->bottom-0.5f,0.0f,1.0f,colorA,0,0},
				{(float)dst->right-0.5f,(float)dst->top-0.5f,0.0f,1.0f,colorB,0,0},
				{(float)dst->right-0.5f,(float)dst->bottom-0.5f,0.0f,1.0f,colorB,0,0},
			};
	pFutureRenderList->pSpriteVerts->Lock(pFutureRenderList->spritesUsed*4 *sizeof(FlexVertex2D), sizeof(FlexVertex2D)*4, (void**)&pVertices, D3DLOCK_DISCARD);
	memcpy(pVertices, data, sizeof(data));
	pFutureRenderList->pSpriteVerts->Unlock();
	pFutureRenderList->spritesUsed++;
	eaPush(&pFutureRenderList->eaSpriteTextures, NULL);
}

void FlexRenderer::AddSpriteToRenderList(GameTexture* pTex, RECT* dst, RECT* pSrc, DWORD color)
{
	assert(pFutureRenderList->spritesUsed < RENDER_LIST_BUFFER_SIZE);
	float width = (float)pTex->width;
	float height = (float)pTex->height;
	float u1, u2, v1, v2;
	if (pSrc)
	{
		u1 = (pSrc->left)/width;
		u2 = (pSrc->right)/width;
		v1 = (pSrc->top)/height;
		v2 = (pSrc->bottom)/height;
	}
	else
	{
		u1 = 0;
		u2 = 1;
		v1 = 0;
		v2 = 1;
	}

	BYTE* pVertices;
	//sprite vertices
	FlexVertex2D data[]={
				{(float)dst->left-0.5f,(float)dst->top-0.5f,0.0f,1.0f,color, u1, v1},
				{(float)dst->left-0.5f,(float)dst->bottom-0.5f,0.0f,1.0f,color, u1, v2},
				{(float)dst->right-0.5f,(float)dst->top-0.5f,0.0f,1.0f,color, u2, v1},
				{(float)dst->right-0.5f,(float)dst->bottom-0.5f,0.0f,1.0f,color, u2, v2},
			};
	pFutureRenderList->pSpriteVerts->Lock(pFutureRenderList->spritesUsed*4 *sizeof(FlexVertex2D), sizeof(FlexVertex2D)*4, (void**)&pVertices, D3DLOCK_DISCARD);
	memcpy(pVertices, data, sizeof(data));
	pFutureRenderList->pSpriteVerts->Unlock();
	pFutureRenderList->spritesUsed++;
	eaPush(&pFutureRenderList->eaSpriteTextures, pTex->pD3DTex);
}

void FlexRenderer::AddSpriteToRenderList(GameTexture* pTex, POINT topleft, RECT* pSrc, DWORD color)
{
	RECT rDst = {topleft.x, 
		topleft.y, 
		topleft.x + pSrc->right-pSrc->left, 
		topleft.y + pSrc->bottom-pSrc->top};
	AddSpriteToRenderList(pTex, &rDst, pSrc, color);
}

void FlexRenderer::AddSpriteToRenderList(GameTexturePortion* pPortion, POINT dst, DWORD color, float fZoom)
{
	GameTexture* pSrcTex = pPortion->hTex.pTex;
	RECT rDst = {(LONG)(dst.x - pPortion->offset->x * fZoom),
		(LONG)(dst.y - pPortion->offset->y * fZoom),
		(LONG)(dst.x + ((pPortion->rSrc->right-pPortion->rSrc->left) - pPortion->offset->x) * fZoom),
		(LONG)(dst.y + ((pPortion->rSrc->bottom-pPortion->rSrc->top) - pPortion->offset->y) * fZoom)};
	AddSpriteToRenderList(pSrcTex, &rDst, pPortion->rSrc, color);
}

void FlexRenderer::AddSpriteToRenderList(GameTexture* pTex, int x, int y, RECT* pSrc, DWORD color)
{
	RECT rDst = {x, 
		y, 
		x + pSrc->right-pSrc->left, 
		y + pSrc->bottom-pSrc->top};
	AddSpriteToRenderList(pTex, &rDst, pSrc, color);
}

void FlexRenderer::AddNinepatchToRenderList(GameTexture* pSet, int iIndex, RECT* pDst, float fBarPct)
{
	RECT temp;
	RECT renderRect;
	int leftW, centerW, rightW, topH, centerH, bottomH, totalW, totalH;
	float leftPct, midPct, rightPct;
	ninepatch* nine = &(pSet->patches[iIndex]);
	totalW = pDst->right-pDst->left;
	totalH = pDst->bottom-pDst->top;
	leftW = nine->rects[0].right-nine->rects[0].left;
	rightW = nine->rects[2].right-nine->rects[2].left;
	topH = nine->rects[0].bottom-nine->rects[0].top;
	bottomH = nine->rects[6].bottom-nine->rects[6].top;


	centerW = totalW-(leftW + rightW);
	centerH = totalH-(topH + bottomH);

	leftPct = max(0.0f, min(1.0f, fBarPct*totalW/leftW));
	midPct = max(0.0f, min(1.0f, (fBarPct*totalW-leftW)/centerW));
	rightPct = max(0.0f, min(1.0f, (fBarPct*totalW-(leftW+centerW))/rightW));

	CopyRect(&renderRect, pDst);

	//top row
	if (leftPct > 0)
	{
		CopyRect(&temp, &nine->rects[0]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*leftPct);
		SetRect(&renderRect, pDst->left, pDst->top, (LONG)(pDst->left + leftW*leftPct), pDst->top + topH);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}

	if (midPct > 0)
	{
		CopyRect(&temp, &nine->rects[1]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*midPct);
		renderRect.left += leftW;
		renderRect.right += (LONG)(centerW*midPct);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}

	if (rightPct > 0)
	{
		CopyRect(&temp, &nine->rects[2]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*rightPct);
		renderRect.left += centerW;
		renderRect.right += (LONG)(rightW*rightPct);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}

	//middle row
	if (leftPct > 0)
	{
		CopyRect(&temp, &nine->rects[3]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*leftPct);
		SetRect(&renderRect, pDst->left, pDst->top + topH, (LONG)(pDst->left + leftW*leftPct), pDst->top + topH + centerH);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}


	if (midPct > 0)
	{
		CopyRect(&temp, &nine->rects[4]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*midPct);
		renderRect.left += leftW;
		renderRect.right += (LONG)(centerW*midPct);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}
	if (rightPct > 0)
	{
		CopyRect(&temp, &nine->rects[5]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*rightPct);
		renderRect.left += centerW;
		renderRect.right += (LONG)(rightW*rightPct);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}

	//bottom row
	if (leftPct > 0)
	{
		CopyRect(&temp, &nine->rects[6]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*leftPct);
		SetRect(&renderRect, pDst->left, pDst->top + topH + centerH, (LONG)(pDst->left + leftW*leftPct), pDst->top + topH + centerH + bottomH);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}


	if (midPct > 0)
	{
		CopyRect(&temp, &nine->rects[7]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*midPct);
		renderRect.left += leftW;
		renderRect.right += (LONG)(centerW*midPct);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}
	if (rightPct > 0)
	{
		CopyRect(&temp, &nine->rects[8]);
		temp.right = temp.left + (LONG)((temp.right-temp.left)*rightPct);
		renderRect.left += centerW;
		renderRect.right += (LONG)(rightW*rightPct);
		AddSpriteToRenderList(pSet, &renderRect, &temp);
	}
}

void FlexRenderer::ProcessRenderLists()
{

#ifdef FLEX_RENDERER_SINGLE_THREAD
	if (pCurRenderList->bUsed && !pNextRenderList->bUsed)
	{
		RenderList* pTemp;
		pTemp = pNextRenderList;

		EnterCriticalSection(&_csNextRenderList);
		pNextRenderList = pCurRenderList;
		LeaveCriticalSection(&_csNextRenderList);

		pCurRenderList = pTemp;
	}
	else
		return;
#else
	while (pCurRenderList->bUsed)
	{
		if (!pNextRenderList->bUsed)
		{
			RenderList* pTemp;
			pTemp = pNextRenderList;

			EnterCriticalSection(&_csNextRenderList);
			pNextRenderList = pCurRenderList;
			LeaveCriticalSection(&_csNextRenderList);

			pCurRenderList = pTemp;
		}
	}
#endif

	UpdateCamera();

	BeginFrame();

	for (int i = 0; i < pCurRenderList->modelsUsed; i++)
	{
		ModelCall* pCall = &pCurRenderList->models[i];
		pD3DDevice->SetTransform(D3DTS_WORLD,&pCall->matWorld);

		if (pCall->pTex)
			pD3DDevice->SetTexture ( 0 , pCall->pTex );
		pD3DDevice->SetStreamSource(0, *(pCall->ppVerts), 0, sizeof(FlexVertex));
		pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, (*pCall->piNumTris));
	}
	
	pD3DDevice->SetRenderState ( D3DRS_ZWRITEENABLE, D3DZB_FALSE);

	for (int i = 0; i < pCurRenderList->translucentModelsUsed; i++)
	{
		ModelCall* pCall = &pCurRenderList->translucentModels[i];
		pD3DDevice->SetTransform(D3DTS_WORLD,&pCall->matWorld);

		if (pCall->pTex)
			pD3DDevice->SetTexture ( 0 , pCall->pTex );
		pD3DDevice->SetStreamSource(0, *(pCall->ppVerts), 0, sizeof(FlexVertex));
		pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, (*pCall->piNumTris));
	}
	
	Begin2D();

	for (int i = 0; i < pCurRenderList->spritesUsed; i++)
	{
		if (i == 0 || pCurRenderList->eaSpriteTextures[i] != pCurRenderList->eaSpriteTextures[i-1])
			pD3DDevice->SetTexture ( 0 , pCurRenderList->eaSpriteTextures[i] );
		pD3DDevice->SetStreamSource(0, pCurRenderList->pSpriteVerts, 0, sizeof(FlexVertex2D));
		pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, i*4, 2);
	}
	End2D();

	EndFrame();

	if (eaVertsToDestroy)
	{
		for (int i = 0; i < eaSize(&eaVertsToDestroy); i++)
		{
			eaVertsToDestroy[i]->Release();
		}
		eaClear(&eaVertsToDestroy);
	}

	pCurRenderList->bUsed = true;
}

void FlexRenderer::BeginFrame()
{
	bActiveFrame = true;
	pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET  , D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER , D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pD3DDevice->BeginScene();
}

void FlexRenderer::EndFrame()
{
	assert(bActiveFrame);
	pD3DDevice->EndScene();
	pD3DDevice->Present(NULL, NULL,
		NULL, NULL);
	bActiveFrame = false;
}

void FlexRenderer::Begin2D()
{
	assert(bActiveFrame);
	bActive2D = true;
	stateBlocks[kRendererMode_2D]->Apply();
}

void FlexRenderer::End2D()
{
	assert(bActive2D);
	bActive2D = false;
	stateBlocks[kRendererMode_Default3D]->Apply();
}

void FlexRenderer::GetTextureDimensions(IDirect3DTexture9* pTex, float dimensions[2])
{
	D3DSURFACE_DESC desc;
	pTex->GetLevelDesc(0, &desc);
	dimensions[0] = (float)desc.Width;
	dimensions[1] = (float)desc.Height;
}

void FlexRenderer::SetCameraPosition(D3DXVECTOR3* pvPos)
{
	EnterCriticalSection(&_csCamera);
	camera.vEye = (*pvPos);
	LeaveCriticalSection(&_csCamera);
}

void FlexRenderer::QueueVertexBufferForDestruction(LPDIRECT3DVERTEXBUFFER9 pVerts)
{
	eaPush(&eaVertsToDestroy, pVerts);
}

void FlexRenderer::AddStringToRenderList(GameTexture* pFontTex, const TCHAR* pString, float x, float y, D3DXCOLOR color, bool centered, int wrapWidth, bool bShadow, float fIconScale)
{
	if (!pFontTex || pFontTex->eType != kTextureType_Font)
		return;
	int realX = (int)x;
	int bonusW = 0;
	int numNewlines = 0;
	DWORD savedColor = color;
	int iVisibleLetters = 0;
	int validWidth = ((pFontTex->width/pFontTex->letterWidth)*pFontTex->letterWidth);
	int texturewidths = 0;
	if (centered)
	{
		int temp = wcslen(pString);
		const TCHAR* iter = pString;

		//find all invisible chars
		iter = wcschr(iter, '|');
		while (iter) 
		{
			if (*(iter+1) == 'c')
				temp -= 8;
			else if (*(iter+1) == 'x')
				temp -= 2;
			else if (*(iter+1) == 'n')
				temp -= 2;
			else if (*(iter+1) == 'b')
				temp -= 14;
			else if (*(iter+1) == 'a')
			{
				TCHAR buf[32];
				int i = 0;
				const TCHAR* iter2 = iter+2;
				while (*iter2 != '|')
				{
					
					buf[i++] = *iter2;
					iter2++;
				}
				buf[i] = '\0';
				GameTexturePortion* pTex = GET_DEF_FROM_STRING(GameTexturePortion, buf);
				texturewidths += pTex->rSrc->right - pTex->rSrc->left;
				temp -= (iter2-iter)+1;
				iter = iter2;
			}
			iter++;
			iter = wcschr(iter, '|');
		}

		realX -= (int)(temp/2.0f * pFontTex->letterWidth);
		realX -= texturewidths/2;
		realX = max(realX, 0);
	}
	if (pFontTex->letterWidth > 0 && pFontTex->letterHeight > 0)
	{
		for (unsigned int i = 0; i < wcslen(pString); i++)
		{
			GameTexturePortion* pTex = NULL;
			if (pString[i] == '|')
			{
				if (pString[i+1] == 'c')
				{
					i++;
					savedColor = 0xff000000;
					i++;
					savedColor += charToHex(pString[i++]) << 20;
					savedColor += charToHex(pString[i++]) << 16;
					savedColor += charToHex(pString[i++]) << 12;
					savedColor += charToHex(pString[i++]) << 8;
					savedColor += charToHex(pString[i++]) << 4;
					savedColor += charToHex(pString[i]);
					continue;
				}
				else if (pString[i+1] == 'x')
				{
					i++;
					savedColor = color;
					i++;
				}
				else if (pString[i+1] == 'b')
				{
					i++;
					//todo: bar support
					/*
					i++;
					CAtlString floatval;
					floatval.SetString(&pString[i], 5);
					float val = _wtof(floatval);
					i += 5;

					i++;
					DWORD frontColor = 0xff000000;
					frontColor += charToHex(pString[i++]) << 20;
					frontColor += charToHex(pString[i++]) << 16;
					frontColor += charToHex(pString[i++]) << 12;
					frontColor += charToHex(pString[i++]) << 8;
					frontColor += charToHex(pString[i++]) << 4;
					frontColor += charToHex(pString[i++]);

					i++;
					DWORD backColor = 0xff000000;
					backColor += charToHex(pString[i++]) << 20;
					backColor += charToHex(pString[i++]) << 16;
					backColor += charToHex(pString[i++]) << 12;
					backColor += charToHex(pString[i++]) << 8;
					backColor += charToHex(pString[i++]) << 4;
					backColor += charToHex(pString[i]);

					if (wrapWidth > -1)
					{
						RECT barRect = {realX+iVisibleLetters*pFontTex->letterWidth, y + numNewlines*pFontTex->letterHeight, realX+wrapWidth*scale, y + numNewlines*letterHeight + (letterHeight*scale)};
						DrawFilledRect(&barRect, backColor);
						barRect.right = barRect.left + (barRect.right-barRect.left * val);
						DrawFilledRect(&barRect, frontColor);
					}
					numNewlines++;
					iVisibleLetters = 0;
					bonusW = 0;
					*/
					continue;
				}
				else if (pString[i+1] == 'a')
				{
					i++;
					i++;
					int j;
					TCHAR buf[32];
					for (j = i; pString[j] != '|'; j++)
						buf[j-(i)] = pString[j];
					buf[j-(i)] = '\0';
					pTex = GET_DEF_FROM_STRING(GameTexturePortion, buf);

					i = j;
				}
				else if (pString[i+1] == 'n')
				{
					i++;
					numNewlines++;
					iVisibleLetters = 0;
					bonusW = 0;
					continue;
				}
			}
			if (pString[i] == '\n')
			{
				numNewlines++;
				iVisibleLetters = 0;
				bonusW = 0;
				continue;
			}
			int letterIndex = max(pString[i]-' ',0);
			int letterX = (letterIndex * pFontTex->letterWidth) % validWidth;
			int letterY = (letterIndex * pFontTex->letterWidth)/validWidth * pFontTex->letterHeight;
			RECT renderSrc = {letterX, letterY, letterX + pFontTex->letterWidth, letterY + pFontTex->letterHeight};
			int wrappedX = realX+(iVisibleLetters*pFontTex->letterWidth) + bonusW;
			int wrappedY = (int)y + numNewlines*pFontTex->letterHeight;
			if (pTex)
			{
				int w = (int)(RECT_WIDTH(pTex->rSrc->)*fIconScale);
				int h = (int)(RECT_HEIGHT(pTex->rSrc->)*fIconScale);
				POINT pt = {wrappedX - pTex->offset->x, wrappedY - (h-pFontTex->letterHeight + 1)/2 - pTex->offset->y};
				AddSpriteToRenderList(pTex, pt, 0xffffffff, fIconScale);
				bonusW += w;
			}
			else
			{
			
				if (bShadow)
					AddSpriteToRenderList(pFontTex, wrappedX+1, wrappedY+1, &renderSrc, 0xFF000000);
				AddSpriteToRenderList(pFontTex, wrappedX, wrappedY, &renderSrc, savedColor);
				iVisibleLetters++;
			}
			pTex = NULL;
		}
	}
}


FlexRenderer g_Renderer;

#include "Autogen\flexrenderer_h_ast.cpp"