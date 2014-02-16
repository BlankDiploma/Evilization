#include "stdafx.h"
#include <DxErr.h>
#include "FlexRenderer.h"
#include "earray.h"
#include "DefLibrary.h"

LPDIRECT3DVERTEXBUFFER9 g_pCubeVertex = NULL;
LPDIRECT3DVERTEXBUFFER9 g_p2DVertices = NULL;

FlexCamera::FlexCamera()
{
	InitializeCriticalSection(&_csCamera);
	bCameraWrapsAlongAxis[0] = bCameraWrapsAlongAxis[1] = bCameraWrapsAlongAxis[2] = false;
}

FlexCamera::~FlexCamera()
{
	DeleteCriticalSection(&_csCamera);
}

void FlexCamera::SetCameraEye(float x, float y, float z)
{
	EnterCriticalSection(&_csCamera);
	vEye.x = x;
	vEye.y = y;
	vEye.z = z;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::SetCameraAt(float x, float y, float z)
{
	EnterCriticalSection(&_csCamera);
	vAt.x = x;
	vAt.y = y;
	vAt.z = z;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::SetCameraUp(float x, float y, float z)
{
	EnterCriticalSection(&_csCamera);
	vUp.x = x;
	vUp.y = y;
	vUp.z = z;
	LeaveCriticalSection(&_csCamera);
}
 
void FlexCamera::SetCameraEye(D3DXVECTOR3 newEye)
{
	EnterCriticalSection(&_csCamera);
	vEye = newEye;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::SetCameraAt(D3DXVECTOR3 newAt)
{
	EnterCriticalSection(&_csCamera);
	vAt = newAt;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::SetCameraUp(D3DXVECTOR3 newUp)
{
	EnterCriticalSection(&_csCamera);
	vUp = newUp;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::SetViewMatrix(D3DXMATRIX newMatView)
{
	EnterCriticalSection(&_csCamera);
	matView = newMatView;
	LeaveCriticalSection(&_csCamera);
}
void FlexCamera::SetProjMatrix(D3DXMATRIX newMatProj)
{
	EnterCriticalSection(&_csCamera);
	matProj = newMatProj;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::GetCameraEye(D3DXVECTOR3* pOut)
{
	EnterCriticalSection(&_csCamera);
	pOut->x= vEye.x;
	pOut->y= vEye.y;
	pOut->z= vEye.z;
	LeaveCriticalSection(&_csCamera);
}
void FlexCamera::GetCameraAt(D3DXVECTOR3* pOut)
{
	EnterCriticalSection(&_csCamera);
	pOut->x= vAt.x;
	pOut->y= vAt.y;
	pOut->z= vAt.z;
	LeaveCriticalSection(&_csCamera);
}
void FlexCamera::GetCameraUp(D3DXVECTOR3* pOut)
{
	EnterCriticalSection(&_csCamera);
	pOut->x= vUp.x;
	pOut->y= vUp.y;
	pOut->z= vUp.z;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::GetViewMatrix(D3DXMATRIX* pOut)
{
	EnterCriticalSection(&_csCamera);
	*pOut = matView;
	LeaveCriticalSection(&_csCamera);
}
void FlexCamera::GetProjMatrix(D3DXMATRIX* pOut)
{
	EnterCriticalSection(&_csCamera);
	*pOut = matProj;
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::MoveCamera(float fHoriz, float fVert, int iZoom)
{
	EnterCriticalSection(&_csCamera);
	D3DXVECTOR3 vDir, vHoriz, vVert, vZoom;

	D3DXVec3Normalize(&vVert, &vUp);
	vDir = vAt - vEye;
	D3DXVec3Normalize(&vDir, &vDir);
	D3DXVec3Cross(&vHoriz, &vVert, &vDir);

	vHoriz = D3DXVECTOR3(vHoriz.x*fHoriz*MOUSEDRAG_SENSITIVITY, 0, 0);
	vVert = D3DXVECTOR3(0, vVert.y*fVert*MOUSEDRAG_SENSITIVITY, 0);
	vZoom = D3DXVECTOR3(vDir.x*iZoom*MOUSEZOOM_SENSITIVITY, vDir.y*iZoom*MOUSEZOOM_SENSITIVITY,vDir.z*iZoom*MOUSEZOOM_SENSITIVITY);

	vEye -= vHoriz;
	vAt -= vHoriz;

	vEye += vVert;
	vAt +=  vVert;

	for (int i = 0; i < 3; i++)
	{
		if (bCameraWrapsAlongAxis[i])
		{
			while (vEye[i] < fCameraWrapBoundaries[i][0])
			{
				float fCorrection = (fCameraWrapBoundaries[i][1] - fCameraWrapBoundaries[i][0]);
				vEye[i] += fCorrection;
				vAt[i] += fCorrection;
			}

			while (vEye[i] > fCameraWrapBoundaries[i][1])
			{
				float fCorrection = -(fCameraWrapBoundaries[i][1] - fCameraWrapBoundaries[i][0]);
				vEye[i] += fCorrection;
				vAt[i] += fCorrection;
			}
		}
	}
	
	if (!(((vEye.z + vZoom.z) >= -15.0f) || ((vEye.z + vZoom.z) <= -70.0f)))
	{
		vEye += vZoom;
		vAt += vZoom;
	}
	
	BuildViewFrustum();
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::SetCameraPosition(D3DXVECTOR3* pvPos)
{
	EnterCriticalSection(&_csCamera);
	D3DXVECTOR3 vDiff = (*pvPos) - vEye;
	vAt += vDiff;
	vEye = (*pvPos);
	BuildViewFrustum();
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::BuildViewFrustum()
{
	cameraFrustum.CalcWorldSpacePlanes(vEye, vAt, vUp);
}

void FlexCamera::BuildViewMatrix()
{
	EnterCriticalSection(&_csCamera);
	ZeroMemory(&matView, sizeof(matView));
	D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::BuildProjMatrix(int screenW, int screenH)
{
	EnterCriticalSection(&_csCamera);
	ZeroMemory(&matProj, sizeof(matProj));
	float zn = 0.1f;
	float zf = 400.0f;
	float fAspect = (float) screenW / (float) screenH;
	D3DXMatrixPerspectiveFovLH(&matProj, FOVY, fAspect, zn, zf);
	//Calculate near and far planes of frustum
	cameraFrustum.CalcNearFarPlaneDimensions(FOVY, fAspect, zn, zf);
	LeaveCriticalSection(&_csCamera);
}

int FlexCamera::CameraFrustumPlaneIntersection(D3DXVECTOR3 pOut[4], D3DXVECTOR3* pPoint, D3DXVECTOR3* pNorm)
{
	EnterCriticalSection(&_csCamera);

	int ret = cameraFrustum.FrustumPlaneIntersection(pOut, pPoint, pNorm);

	LeaveCriticalSection(&_csCamera);

	return ret;
}

void FlexCamera::CalcFrustumNearFarPlaneDimensions(float fovy, float Aspect, float zn, float zf)
{
	EnterCriticalSection(&_csCamera);

	cameraFrustum.CalcNearFarPlaneDimensions(fovy, Aspect, zn, zf);

	LeaveCriticalSection(&_csCamera);
}

void FlexCamera::Rotate(float rot[3])
{
	EnterCriticalSection(&_csCamera);
	D3DXVECTOR3 vDirection,vRight;
	D3DXMATRIX matRotAxis,matRotY;

	D3DXVec3Normalize(&vDirection,&(vAt - vEye));
	D3DXVec3Cross(&vRight,&vDirection,&vUp);
	D3DXVec3Normalize(&vRight,&vRight);

	D3DXMatrixRotationAxis(&matRotAxis,&vRight,D3DXToRadian(rot[1]));
	D3DXMatrixRotationZ(&matRotY,D3DXToRadian(rot[0]));

	D3DXVec3TransformCoord(&vDirection,&vDirection,&(matRotAxis * matRotY));
	D3DXVec3TransformCoord(&vUp,&vUp,&(matRotAxis * matRotY));
	vAt = vDirection + vEye;

	LeaveCriticalSection(&_csCamera);
}

void FlexRenderer::UpdateCamera()
{
	D3DXMATRIX matView;
	pCamera->BuildViewMatrix();
	pCamera->GetViewMatrix(&matView);
	pD3DDevice->SetTransform(D3DTS_VIEW, &matView);
}

void FlexFrustum::CalcNearFarPlaneDimensions(float fovy, float Aspect, float zn, float zf)
{
	this->zn = zn;
	this->zf = zf;
	fHnear = 2.0f * tan(fovy/2.0f) * zn;
	fWnear = fHnear * Aspect;
	fHfar = 2.0f * tan(fovy/2.0f) * zf;
	fWfar = fHfar * Aspect;
}

void FlexFrustum::CalcWorldSpacePlanes(D3DXVECTOR3 vEye, D3DXVECTOR3 vAt, D3DXVECTOR3 vUp)
{
	camDir = vAt - vEye;
	D3DXVec3Normalize(&camDir, &camDir);
	D3DXVECTOR3 vRight;
	D3DXVec3Cross(&vRight, &vUp, &camDir);
	D3DXVec3Normalize(&vRight, &vRight);
	D3DXVec3Normalize(&vUp, &vUp);
	D3DXVECTOR3 farCenter = vEye + camDir * zf;
	D3DXVECTOR3 nearCenter = vEye + camDir * zn;

	ftl = farCenter + (vUp * fHfar/2.0f) - (vRight * fWfar/2.0f);
	ftr = farCenter + (vUp * fHfar/2.0f) + (vRight * fWfar/2.0f);
	fbl = farCenter - (vUp * fHfar/2.0f) - (vRight * fWfar/2.0f);
	fbr = farCenter - (vUp * fHfar/2.0f) + (vRight * fWfar/2.0f);

	ntl = nearCenter + (vUp * fHnear/2.0f) - (vRight * fWnear/2.0f);
	ntr = nearCenter + (vUp * fHnear/2.0f) + (vRight * fWnear/2.0f);
	nbl = nearCenter - (vUp * fHnear/2.0f) - (vRight * fWnear/2.0f);
	nbr = nearCenter - (vUp * fHnear/2.0f) + (vRight * fWnear/2.0f);


}

int FlexFrustum::FrustumPlaneIntersection(D3DXVECTOR3 pOut[4], D3DXVECTOR3* pPoint, D3DXVECTOR3* pNorm)
{
	D3DXPLANE plMap;
	int ret = 0;

	D3DXPlaneFromPointNormal(&plMap, pPoint, pNorm);
	D3DXPlaneNormalize(&plMap,&plMap);

	if (D3DXPlaneIntersectLine(&pOut[0], &plMap, &ntl, &ftl))
		ret++;
	if (D3DXPlaneIntersectLine(&pOut[1], &plMap, &ntr, &ftr))
		ret++;
	if (D3DXPlaneIntersectLine(&pOut[2], &plMap, &nbl, &fbl))
		ret++;
	if (D3DXPlaneIntersectLine(&pOut[3], &plMap, &nbr, &fbr))
		ret++;
	return ret;
}

float FlexFrustum::GetNearPlaneDist()
{
	return zn;
}

float FlexFrustum::GetFarPlaneDist()
{
	return zf;
}

void FlexFrustum::PointToFrustumRay(D3DXVECTOR3 pOut[2], float fScaledX, float fScaledY)
{
	pOut[0].x = fScaledX * fWnear + ntl.x;
	pOut[0].y = fScaledY * fHnear + ntl.y;
	//pOut[0].y = ntl.y - (fScaledY * fHnear);
	pOut[0].z = fScaledY * fHnear * (ntl.z - nbl.z) + ntl.z;

	pOut[1].x = fScaledX * fWfar + ftl.x;
	pOut[1].y = fScaledY * fHfar + ftl.y;
	//pOut[1].y = ftl.y - (fScaledY * fHfar);
	pOut[1].z = fScaledY * fHfar * (ftl.z - fbl.z) + ftl.z;
}

void FlexRenderer::PlaneIntersectRay(D3DXVECTOR3* pOut, const D3DXVECTOR3* pPlanePoint, const D3DXVECTOR3* pPlaneNorm, const D3DXVECTOR3* pRayPoint1, const D3DXVECTOR3* pRayPoint2)
{

	D3DXPLANE plMap;
	D3DXPlaneFromPointNormal(&plMap, pPlanePoint, pPlaneNorm);
	D3DXPlaneIntersectLine(pOut, &plMap, pRayPoint1, pRayPoint2);
}

FLOATPOINT FlexRenderer::ScaleScreenCoords(int x, int y)
{
	FLOATPOINT scaledCoords;
	float fAspect;
	static float frustCenterToEdgeDistance = tanf(FOVY*0.5f);

	fAspect = (float) iScreenW / (float) iScreenH;
	scaledCoords.x = (frustCenterToEdgeDistance * (((float) x / ((float) iScreenW * 0.5f)) - 1.0f) / fAspect);
	scaledCoords.y = (frustCenterToEdgeDistance * (1.0f - y) / ((float) iScreenH * 0.5f));

	return scaledCoords;
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

	iScreenW = screenW;
	iScreenH = screenH;

	fAspect = (float) iScreenW / (float) iScreenH;

	pCamera->SetCameraEye(0, 0,-3);
	//pCamera->SetCameraAt(5,-3.25,0);
	pCamera->SetCameraAt(0,0,1);
	pCamera->SetCameraUp(0,1,0);

	float rot[3] = {0, CAM_ANGLE, 0};

	pCamera->Rotate(rot);

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
	D3DXMATRIX matView;
	pCamera->BuildViewMatrix();
	pCamera->GetViewMatrix(&matView);
	pD3DDevice->SetTransform(D3DTS_VIEW, &matView); 

	// Projection matrix setup
	//g_MatProj = MakeProjectionMatrix(1, 20, 120, 120);
	D3DXMATRIX matProj;
	pCamera->BuildProjMatrix(screenW, screenH);
	pCamera->GetProjMatrix(&matProj);
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
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_NONE);			//anisotropic filtering
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC);
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MAXANISOTROPY, 8);
//	pD3DDevice->SetRenderState ( D3DRS_FILLMODE, D3DFILL_WIREFRAME);
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
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_NONE);
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_NONE);
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	pD3DDevice->SetSamplerState( 0, D3DSAMP_MAXANISOTROPY, 1);
	pD3DDevice->EndStateBlock( &stateBlocks[kRendererMode_2D] );

	//Wireframe 3D
	pD3DDevice->BeginStateBlock();
	pD3DDevice->SetRenderState ( D3DRS_LIGHTING , FALSE ) ;
	pD3DDevice->SetRenderState ( D3DRS_CULLMODE , D3DCULL_CCW ) ;
	pD3DDevice->SetRenderState ( D3DRS_FILLMODE , D3DFILL_WIREFRAME ) ;
	pD3DDevice->EndStateBlock( &stateBlocks[kRendererMode_Wireframe3D] );

	stateBlocks[kRendererMode_Default3D]->Apply();
	//stateBlocks[kRendererMode_Wireframe3D]->Apply();

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
	pCamera = new FlexCamera();
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
	
	delete pCamera;
	delete pCurRenderList;
	delete pNextRenderList;
	delete pFutureRenderList;
	
	DeleteCriticalSection(&_csNextRenderList);
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

	D3DXMatrixRotationYawPitchRoll(&matRot,rot[1],rot[0],rot[2]);
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

		pD3DDevice->SetTexture ( 0 , pCall->pTex );
		pD3DDevice->SetStreamSource(0, *(pCall->ppVerts), 0, sizeof(FlexVertex));
		pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, (*pCall->piNumTris));
	}
	
	pD3DDevice->SetRenderState ( D3DRS_ZWRITEENABLE, D3DZB_FALSE);

	for (int i = 0; i < pCurRenderList->translucentModelsUsed; i++)
	{
		ModelCall* pCall = &pCurRenderList->translucentModels[i];
		pD3DDevice->SetTransform(D3DTS_WORLD,&pCall->matWorld);

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
	//stateBlocks[kRendererMode_Wireframe3D]->Apply();
}

void FlexRenderer::GetTextureDimensions(IDirect3DTexture9* pTex, float dimensions[2])
{
	D3DSURFACE_DESC desc;
	pTex->GetLevelDesc(0, &desc);
	dimensions[0] = (float)desc.Width;
	dimensions[1] = (float)desc.Height;
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

HRESULT FlexRenderer::CreateVertexBuffer(unsigned int Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pHandle)
{
	return pD3DDevice->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pHandle);
}
void FlexRenderer::RenderCubeAtPoint(D3DXVECTOR3 vPoint)
{
	float pos[3] = {vPoint.x - 0.5f, vPoint.y - 0.5f, -0.5f};
	float scale[3] = {1,1,1};
	float rot[3] = {0,0,0};
	static int numCubeTris = 12;
	AddModelToRenderList(&g_pCubeVertex, &numCubeTris, NULL, pos, scale, rot, true);
}

void FlexRenderer::PixelToFrustumRay(D3DXVECTOR3 pOut[2], int x, int y)
{
	float fScaledX, fScaledY;
	fScaledX = x/(float) iScreenW;
	fScaledY = y/(float) iScreenH;

	pCamera->GetFrustum()->PointToFrustumRay(pOut, fScaledX, fScaledY);
}

void FlexCamera::CastRayThroughPixel(D3DXVECTOR3 pOut[2], int x, int y, int iWidth, int iHeight)
{
	D3DXVECTOR3 vCamDir, vHoriz, vVert, vRayOrigin, vRayDir;
	float vLength, hLength, newX, newY;

	vCamDir = vAt - vEye;
	D3DXVec3Normalize(&vCamDir, &vCamDir);

	D3DXVec3Cross(&vHoriz, &vCamDir, &vUp);
	D3DXVec3Normalize(&vHoriz, &vHoriz);

	D3DXVec3Cross(&vVert, &vHoriz, &vCamDir);
	D3DXVec3Normalize(&vVert, &vVert);

	vLength = tanf( FOVY / 2.0f ) * cameraFrustum.GetNearPlaneDist();
	hLength = vLength * ((float) iWidth/ (float) iHeight);

	vVert *= vLength;
	vHoriz *= hLength;

	//translate mouse coords
	newX = x - (iWidth / 2.0f);
	newY = y - (iHeight / 2.0f);

	//scale mouse coords
	newY /= (iHeight / 2.0f);
	newX /= (iWidth / 2.0f);

	vRayOrigin = vEye + (vCamDir * cameraFrustum.GetNearPlaneDist()) + (vHoriz*newX*(-1)) + (vVert*newY*(-1));
	vRayDir = vRayOrigin - vEye;

	pOut[0] = vRayOrigin;
	pOut[1] = vRayDir;
}

void FlexRenderer::CastRayThroughPixel(D3DXVECTOR3 pOut[2], int x, int y)
{
	pCamera->CastRayThroughPixel(pOut, x, y, iScreenW, iScreenH);
}
FlexRenderer g_Renderer;

#include "Autogen\flexrenderer_h_ast.cpp"