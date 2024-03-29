#include "stdafx.h"
#include <DxErr.h>
#include "FlexRenderer.h"
#include "earray.h"
#include "DefLibrary.h"
#include "FlexErrorWindow.h"

LPDIRECT3DVERTEXBUFFER9 g_pCubeVertex = NULL;
LPDIRECT3DINDEXBUFFER9 g_pCubeIndex = NULL;
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

void FlexCamera::WorldSpaceToHomogenousScreen(D3DXVECTOR3* pWorld, D3DXVECTOR3* pOut)
{
	EnterCriticalSection(&_csCamera);
	D3DXMATRIX matViewProj = matProj * matView;
	LeaveCriticalSection(&_csCamera);

	D3DXVec3TransformCoord(pOut, pWorld, &matView);
	D3DXVec3TransformCoord(pOut, pOut, &matProj);
}

void FlexRenderer::WorldSpaceToScreen(D3DXVECTOR3* pWorld, POINT* pOut)
{
	D3DXVECTOR3 homogenous(0,0,0);
	pCamera->WorldSpaceToHomogenousScreen(pWorld ,&homogenous);
	pOut->x = (LONG)(((homogenous[0] + 1)/2.0f)*iScreenW);
	pOut->y = (LONG)(((1-homogenous[1])/2.0f)*iScreenH);
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

#define DWORD_TO_R_CHANNEL(a) (((a) >> 16) & 0xff) 
#define DWORD_TO_G_CHANNEL(a) (((a) >> 8) & 0xff) 
#define DWORD_TO_B_CHANNEL(a) ((a) & 0xff) 
#define DWORD_TO_A_CHANNEL(a) (((a) >> 24) & 0xff) 

static inline void DoVertexLerp(FlexVertex* pA, FlexVertex* pB, FlexVertex* pOut)
{
	pOut->u = (pA->u+pB->u)/2;
	pOut->v = (pA->v+pB->v)/2;
	pOut->u1 = (pA->u1+pB->u1)/2;
	pOut->v1 = (pA->v1+pB->v1)/2;
	pOut->x = (pA->x+pB->x)/2;
	pOut->y = (pA->y+pB->y)/2;
	pOut->z = (pA->z+pB->z)/2;
	int r = (DWORD_TO_R_CHANNEL(pB->diffuse) + DWORD_TO_R_CHANNEL(pA->diffuse))/2;
	int g = (DWORD_TO_G_CHANNEL(pB->diffuse) + DWORD_TO_G_CHANNEL(pA->diffuse))/2;
	int b = (DWORD_TO_B_CHANNEL(pB->diffuse) + DWORD_TO_B_CHANNEL(pA->diffuse))/2;
	int a = (DWORD_TO_A_CHANNEL(pB->diffuse) + DWORD_TO_A_CHANNEL(pA->diffuse))/2;
	pOut->diffuse = D3DCOLOR_ARGB(a, r, g, b);
}

static void _tessellateTriangleRecurse(FlexVertex* vA, FlexVertex* vB, FlexVertex* vC, float fBlendA, float fBlendB, float fBlendC, int iDegree, FlexVertex** vertBufferOut)
{
	//input:
	//	A
	//
	//			B
	//
	//	C

	//output:
	//  0
	//		1
	//  3		2		->	6 unique verts, 12 index entries (NYI)
	//		4
	//  5
	FlexVertex vertsTmp[6] = {0};
	float fBlends[6] = {fBlendA, (fBlendA+fBlendB)/2.0f, fBlendB, (fBlendA+fBlendC)/2.0f, (fBlendB+fBlendC)/2.0f, fBlendC};
	vertsTmp[0] = *vA;
	vertsTmp[2] = *vB;
	vertsTmp[5] = *vC;
	DoVertexLerp(vA, vB, &vertsTmp[1]);
	DoVertexLerp(vA, vC, &vertsTmp[3]);
	DoVertexLerp(vB, vC, &vertsTmp[4]);
	if (iDegree > 1)
	{
		//once for each new tri
		_tessellateTriangleRecurse(&vertsTmp[0], &vertsTmp[1], &vertsTmp[3], fBlends[0], fBlends[1], fBlends[3], iDegree-1, vertBufferOut);
		_tessellateTriangleRecurse(&vertsTmp[1], &vertsTmp[2], &vertsTmp[4], fBlends[1], fBlends[2], fBlends[4], iDegree-1, vertBufferOut);
		_tessellateTriangleRecurse(&vertsTmp[3], &vertsTmp[4], &vertsTmp[5], fBlends[3], fBlends[4], fBlends[5], iDegree-1, vertBufferOut);
		_tessellateTriangleRecurse(&vertsTmp[3], &vertsTmp[1], &vertsTmp[4], fBlends[3], fBlends[1], fBlends[4], iDegree-1, vertBufferOut);
	}
	else
	{
		for (int i = 0; i < 6; i++)
		{
			vertsTmp[i].diffuse &= 0xff00ffff;
			vertsTmp[i].diffuse |= ((int)(max(fBlends[i], 0.0f) * 255)) << 16;
		}
		*((*vertBufferOut)++) = vertsTmp[0];
		*((*vertBufferOut)++) = vertsTmp[1];
		*((*vertBufferOut)++) = vertsTmp[3];

		*((*vertBufferOut)++) = vertsTmp[1];
		*((*vertBufferOut)++) = vertsTmp[2];
		*((*vertBufferOut)++) = vertsTmp[4];

		*((*vertBufferOut)++) = vertsTmp[3];
		*((*vertBufferOut)++) = vertsTmp[4];
		*((*vertBufferOut)++) = vertsTmp[5];

		*((*vertBufferOut)++) = vertsTmp[3];
		*((*vertBufferOut)++) = vertsTmp[1];
		*((*vertBufferOut)++) = vertsTmp[4];
	}
}

void FlexRenderer::TessellateTriangleIntoBuffer(FlexVertex* vA, FlexVertex* vB, FlexVertex* vC, float fBlendA, float fBlendB, float fBlendC, int iDegree, FlexVertex** vertBufferOut)
{
	if (iDegree < 1)
	{
		Errorf("TessellateTriangleIntoBuffer called with a degree of 0 or less, which is invalid.");
		return;
	}
	_tessellateTriangleRecurse(vA, vB, vC, fBlendA, fBlendB, fBlendC, iDegree, vertBufferOut);
}

int FlexRenderer::GetNumTessellatedTriangles(int iDegree)
{
	return (int)pow(4, iDegree);
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
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
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

	// World matrix setup
	float pos[3] = {0,0,0};
	pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

	// View matrix setup
	D3DXMATRIX matView;
	pCamera->BuildViewMatrix();
	pCamera->GetViewMatrix(&matView);
	pD3DDevice->SetTransform(D3DTS_VIEW, &matView); 

	// Projection matrix setup
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
	LPD3DXBUFFER effectErrors = NULL;

	HRESULT error = D3DXCreateEffectFromFileW(pD3DDevice,
		_T("data/shaders/DefaultShaderNoNorms.fx"),
		0,
		0,
		0,
		0,
		&pDefaultShader,
		&effectErrors
		);


	if (FAILED(error) && effectErrors)
	{
		ErrorFilenamef("Shader compile errors: %S", L"data/shaders/DefaultShaderNoNorms.fx", (char*)effectErrors->GetBufferPointer());
		effectErrors->Release();
	}

	D3DVERTEXELEMENT9 decl3D[MAX_FVF_DECL_SIZE];
	D3DVERTEXELEMENT9 decl2D[MAX_FVF_DECL_SIZE];

	D3DXDeclaratorFromFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2, decl3D);
	D3DXDeclaratorFromFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, decl2D);

	pD3DDevice->CreateVertexDeclaration(decl3D, &FlexVertexDecl);

	pD3DDevice->CreateVertexDeclaration(decl2D, &FlexVertex2DDecl);

	pD3DDevice->SetVertexDeclaration(FlexVertexDecl);

	if (pDefaultShader)
	{
		pShaderTechniques[kShader3D_Default] = pDefaultShader->GetTechniqueByName("Default3D");
		pShaderTechniques[kShader3D_2xBlend] = pDefaultShader->GetTechniqueByName("TextureBlend3D");
		default2DTech = pDefaultShader->GetTechniqueByName("Default2D");
//		wireframe3DTech = pDefaultShader->GetTechniqueByName("Wireframe3D");
		pShaderTechniques[kShader3D_Translucent] = pDefaultShader->GetTechniqueByName("Translucent3D");
		pShaderTechniques[kShader3D_Translucent2xBlend] = pDefaultShader->GetTechniqueByName("TranslucentBlend3D");
	}

	if (!pDefaultShader || FAILED(pDefaultShader->SetTechnique(pShaderTechniques[kShader3D_Default])))
	{
		Errorf("Fatal error: Failed to set default shader technique to Default3D");
	}

	pD3DDevice->CreateVertexBuffer(8*sizeof(FlexVertex), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &g_pCubeVertex, NULL);
	
	pD3DDevice->CreateIndexBuffer(36*sizeof(int), D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &g_pCubeIndex, NULL);

	BYTE* pData;
	FlexVertex verts[]=
	{
	//Cube vertices
				{0.0f,0.0f,0.0f,0xFF603913,0,0},{0.0f, 1.0f,0.0f,0xFF603913,0,0},
				{1.0f, 1.0f,0.0f,0xFF603913,0,0},{1.0f,0.0f,0.0f,0xFF603913,0,0},
				{1.0f,0.0f, 1.0f,0xFF603913,0,0},{1.0f, 1.0f, 1.0f,0xFF603913,0,0},
				{0.0f, 1.0f, 1.0f,0xFF603913,0,0},{0.0f,0.0f, 1.0f,0xFF603913,0,0}
	};

	int indices[] = {0,1,2, 2,3,0,
					 4,5,6, 6,7,4,
				     0,3,5, 5,4,0,
					 3,2,6, 6,5,3,
					 2,1,7, 7,6,2,
					 1,0,4, 4,7,1};

	g_pCubeVertex->Lock(0, 0, (void**)&pData, 0);
	memcpy(pData, verts, sizeof(verts));
	g_pCubeVertex->Unlock();

	g_pCubeIndex->Lock(0, 0, (void**)&pData, 0);
	memcpy(pData, indices, sizeof(indices));
	g_pCubeIndex->Unlock();

	//let's assume that 1,000 2d sprites is enough for now
	pD3DDevice->CreateVertexBuffer(4*RENDER_LIST_BUFFER_SIZE*sizeof(FlexVertex2D), D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &pFutureRenderList->pSpriteVerts, NULL);
	
	pD3DDevice->CreateVertexBuffer(4*RENDER_LIST_BUFFER_SIZE*sizeof(FlexVertex2D), D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &pNextRenderList->pSpriteVerts, NULL);
	
	pD3DDevice->CreateVertexBuffer(4*RENDER_LIST_BUFFER_SIZE*sizeof(FlexVertex2D), D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &pCurRenderList->pSpriteVerts, NULL);
	
}


FlexRenderer::FlexRenderer()
{
	pCamera = new FlexCamera();
	pFutureRenderList = new RenderList;
	pNextRenderList = new RenderList;
	pCurRenderList = new RenderList;
	eaVertsToDestroy = NULL;
	eaAtlasVertexBuffers = NULL;
	bForceWireframe = false;
	bForceTextureBlending = true;
}

FlexRenderer::~FlexRenderer()
{
	for (int i = 0; i < kRendererMode_Count; i++)
	{
		if (stateBlocks[i])
			stateBlocks[i]->Release();
	}
	for (int i = 0; i < eaSize(&eaAtlasVertexBuffers); i++)
	{
		if (eaAtlasVertexBuffers[i])
			eaAtlasVertexBuffers[i]->Release();
	}
	for (int i = 0; i < eaSize(&eaVertsToDestroy); i++)
	{
		eaVertsToDestroy[i]->Release();
	}
	eaDestroy(&eaAtlasVertexBuffers);
	eaDestroy(&eaVertsToDestroy);

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
	
	if (pDefaultShader)
		pDefaultShader->Release();

	delete pCamera;
	delete pCurRenderList;
	delete pNextRenderList;
	delete pFutureRenderList;
	
	DeleteCriticalSection(&_csNextRenderList);
}

void FlexRenderer::SetRenderMode(FlexRendererMode eMode)
{
}

void FlexRenderer::StartNewRenderList()
{
	pFutureRenderList->bUsed = false;
	memset(pFutureRenderList->pModelsUsedByTechnique, 0, sizeof(int)*kShader3D_Count);
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

void FlexRenderer::AddModelToRenderList(IDirect3DVertexBuffer9** ppVerts, IDirect3DIndexBuffer9* pIndices, int* piNumTris, int* piNumVerts, const GameTexture* pTex, float pos[3], float scale[3], float rot[3], ShaderTechniques3D eTechnique)
{
	assert(pFutureRenderList->pModelsUsedByTechnique[eTechnique] < RENDER_LIST_BUFFER_SIZE);

	ModelCall* pModel = &pFutureRenderList->pCallsByTechnique[eTechnique][pFutureRenderList->pModelsUsedByTechnique[eTechnique]++];

	pModel->ppVerts = ppVerts;
	pModel->pIndices = pIndices;
	pModel->pTex = pTex ? pTex->pD3DTex : NULL;
	pModel->piNumTris = piNumTris;
	pModel->piNumVerts = piNumVerts;
	pModel->iVertStart = 0;
	pModel->ePrimitiveType = D3DPT_TRIANGLELIST;

	D3DXMATRIX matTrans, matRot, matScale;

	D3DXMatrixRotationYawPitchRoll(&matRot,rot[1],rot[0],rot[2]);
	D3DXMatrixTranslation(&matTrans, pos[0], pos[1], pos[2]);
	D3DXMatrixScaling(&matScale, scale[0], scale[1], scale[2]);
	D3DXMatrixMultiply(&pModel->matWorld, &matScale, &matRot);
	D3DXMatrixMultiply(&pModel->matWorld, &pModel->matWorld, &matTrans);
}

void FlexRenderer::Add3DTexturePortionToRenderList(const GameTexturePortion* pTex, float pos[3], float scale[3], float rot[3], ShaderTechniques3D eTechnique)
{
	assert(pFutureRenderList->pModelsUsedByTechnique[eTechnique] < RENDER_LIST_BUFFER_SIZE);

	ModelCall* pModel = &pFutureRenderList->pCallsByTechnique[eTechnique][pFutureRenderList->pModelsUsedByTechnique[eTechnique]++];

	static int iNumTris = 2;

	pModel->ppVerts = &pTex->pVerts;
	pModel->pTex = pTex->hTex.pTex->pD3DTex;
	pModel->piNumTris = &iNumTris;
	//pModel->piNumVerts = &iNumVerts;

	pModel->iVertStart = pTex->iVertIndexStart;
	pModel->ePrimitiveType = D3DPT_TRIANGLESTRIP;

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

void FlexRenderer::AddSpriteToRenderList(const GameTexture* pTex, RECT* dst, RECT* pSrc, DWORD color)
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

void FlexRenderer::AddSpriteToRenderList(const GameTexture* pTex, POINT topleft, RECT* pSrc, DWORD color)
{
	RECT rDst = {topleft.x, 
		topleft.y, 
		topleft.x + pSrc->right-pSrc->left, 
		topleft.y + pSrc->bottom-pSrc->top};
	AddSpriteToRenderList(pTex, &rDst, pSrc, color);
}

void FlexRenderer::AddSpriteToRenderList(const GameTexturePortion* pPortion, POINT dst, DWORD color, float fZoom)
{
	if (!pPortion)
		return;
	const GameTexture* pSrcTex = pPortion->hTex.pTex;
	RECT rDst = {(LONG)(dst.x - pPortion->offset->x * fZoom),
		(LONG)(dst.y - pPortion->offset->y * fZoom),
		(LONG)(dst.x + ((pPortion->rSrc->right-pPortion->rSrc->left) - pPortion->offset->x) * fZoom),
		(LONG)(dst.y + ((pPortion->rSrc->bottom-pPortion->rSrc->top) - pPortion->offset->y) * fZoom)};
	AddSpriteToRenderList(pSrcTex, &rDst, pPortion->rSrc, color);
}

void FlexRenderer::AddSpriteToRenderList(const GameTexture* pTex, int x, int y, RECT* pSrc, DWORD color)
{
	RECT rDst = {x, 
		y, 
		x + pSrc->right-pSrc->left, 
		y + pSrc->bottom-pSrc->top};
	AddSpriteToRenderList(pTex, &rDst, pSrc, color);
}

void FlexRenderer::AddNinepatchToRenderList(const GameTexture* pSet, int iIndex, RECT* pDst, float fBarPct)
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
	static IDirect3DTexture9* pWhite = NULL;

	if (!pWhite)
	{
		GameTexture* pGameTex = GET_TEXTURE(L"white");
		assert(pGameTex);	//there must be a "white" texture available
		pWhite = pGameTex->pD3DTex;
	}

	if (!pDefaultShader)
		return;

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
	/*
	if (bForceWireframe)
		pDefaultShader->SetTechnique(wireframe3DTech);
	else if (bForceTextureBlending)
		pDefaultShader->SetTechnique(blendTextures3DTech);
	*/
	for (int iTechnique = 0; iTechnique < kShader3D_Count; iTechnique++)
	{
		int iModelsUsed = pCurRenderList->pModelsUsedByTechnique[iTechnique];
		if (iModelsUsed > 0)
		{
			pDefaultShader->SetTechnique(pShaderTechniques[iTechnique]);
			for (int i = 0; i < iModelsUsed; i++)
			{
				ModelCall* pCall = &pCurRenderList->pCallsByTechnique[iTechnique][i];
				pD3DDevice->SetTransform(D3DTS_WORLD,&pCall->matWorld);

				pD3DDevice->SetStreamSource(0, *(pCall->ppVerts), pCall->iVertStart*sizeof(FlexVertex), sizeof(FlexVertex));
				if (pCall->ePrimitiveType != D3DPT_TRIANGLESTRIP && pCall->pIndices)
					pD3DDevice->SetIndices(pCall->pIndices);

				D3DXMATRIXA16 matWorld, matProj, matView;
				D3DXVECTOR3 camEye;
				pD3DDevice->GetTransform(D3DTS_WORLD, &matWorld);
				pCamera->GetProjMatrix(&matProj);
				pCamera->GetViewMatrix(&matView);
				pCamera->GetCameraEye(&camEye);

				pDefaultShader->SetMatrix("matWorld", &matWorld);
				pDefaultShader->SetMatrix("matProj", &matProj);
				pDefaultShader->SetMatrix("matView", &matView);
				pDefaultShader->SetVector("eye",(D3DXVECTOR4*) &camEye);
				pDefaultShader->SetTexture("shaderTexture", pCall->pTex ? pCall->pTex : pWhite);

				pDefaultShader->CommitChanges();

				unsigned passes = 0;
				pDefaultShader->Begin(&passes,0);
				for(unsigned j = 0; j < passes; j++)
				{
					pDefaultShader->BeginPass(j);

					if (pCall->ePrimitiveType != D3DPT_TRIANGLESTRIP && pCall->pIndices)
						pD3DDevice->DrawIndexedPrimitive(pCall->ePrimitiveType, 0, 0, (*pCall->piNumVerts), 0, (*pCall->piNumTris));
					else
						pD3DDevice->DrawPrimitive(pCall->ePrimitiveType, 0, (*pCall->piNumTris));
					pDefaultShader->EndPass();
				}
				pDefaultShader->End();
			}
		}
	}
	
	Begin2D();
	
	pD3DDevice->SetStreamSource(0, pCurRenderList->pSpriteVerts, 0, sizeof(FlexVertex2D));

	for (int i = 0; i < pCurRenderList->spritesUsed; i++)
	{
		if (i == 0 || pCurRenderList->eaSpriteTextures[i] != pCurRenderList->eaSpriteTextures[i-1])
		{
			pDefaultShader->SetTexture("shaderTexture", pCurRenderList->eaSpriteTextures[i] ? pCurRenderList->eaSpriteTextures[i] : pWhite);
		}

		unsigned passes = 0;
		pDefaultShader->Begin(&passes,0);
		for(unsigned j = 0; j < passes; j++)
		{
			pDefaultShader->BeginPass(j);

			pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, i*4, 2);
			pDefaultShader->EndPass();
		}
		pDefaultShader->End();
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
	pD3DDevice->SetVertexDeclaration(FlexVertex2DDecl);
	pDefaultShader->SetTechnique(default2DTech);
}

void FlexRenderer::End2D()
{
	assert(bActive2D);
	bActive2D = false;
	pD3DDevice->SetVertexDeclaration(FlexVertexDecl);
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

void FlexRenderer::AddStringToRenderList(const GameTexture* pFontTex, const TCHAR* pString, float x, float y, D3DXCOLOR color, bool centered, int wrapWidth, bool bShadow, float fIconScale)
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
				const GameTexturePortion* pTex = GET_DEF_FROM_STRING(GameTexturePortion, buf);
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
			const GameTexturePortion* pTex = NULL;
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
				{
					DWORD shadowColor = savedColor & 0xFF000000;
					AddSpriteToRenderList(pFontTex, wrappedX+1, wrappedY+1, &renderSrc, shadowColor);
				}
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
	static int numCubeVerts = 8;
	AddModelToRenderList(&g_pCubeVertex, g_pCubeIndex, &numCubeTris, &numCubeVerts, NULL, pos, scale, rot, kShader3D_Default);
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

void FlexRenderer::CreateTextureAtlasVertexBuffer(const GameTexture* pSrcTexture, GameTexturePortion** eaPortions)
{
	int iNumEntries = eaSize(&eaPortions);
	int numVerts = iNumEntries*4;
	IDirect3DVertexBuffer9* pNewBuf = NULL;
	pD3DDevice->CreateVertexBuffer(numVerts*sizeof(FlexVertex), 0, 0, D3DPOOL_DEFAULT, &pNewBuf, NULL);
	BYTE* pVerts;
	
	pNewBuf->Lock(0, 0, (void**)&pVerts, 0);
	
	for (int i = 0; i < iNumEntries; i++)
	{
		float minU, maxU, minV, maxV, XYRatio;
		if ((float)eaPortions[i]->rSrc->left == 0 && (float)eaPortions[i]->rSrc->right == 0 && (float)eaPortions[i]->rSrc->top == 0 && (float)eaPortions[i]->rSrc->bottom == 0)
		{
			minU = 0;
			maxU = 1.0f;
			minV = 0;
			maxV = 1.0f;
			XYRatio = ((float)(pSrcTexture->width))/((float)(pSrcTexture->height));
		}
		else
		{
			minU = ((float)eaPortions[i]->rSrc->left)/pSrcTexture->width;
			maxU = ((float)eaPortions[i]->rSrc->right)/pSrcTexture->width;
			minV = ((float)eaPortions[i]->rSrc->top)/pSrcTexture->height;
			maxV = ((float)eaPortions[i]->rSrc->bottom)/pSrcTexture->height;
			XYRatio = ((float)(eaPortions[i]->rSrc->right-eaPortions[i]->rSrc->left))/(eaPortions[i]->rSrc->bottom-eaPortions[i]->rSrc->top);
		}
		eaPortions[i]->iVertIndexStart = i*4;
		eaPortions[i]->pVerts = pNewBuf;

		COLOR_ARGB color;
		color = 0xFFFFFFFF;


		FlexVertex verts[4] = 
		{
			{-XYRatio*0.5f,	0.5f,	0.0f,	color,minU,minV},
			{XYRatio*0.5f,	0.5f,	0.0f,	color,maxU,minV},
			{-XYRatio*0.5f,	-0.5f,	0.0f,	color,minU,maxV},
			{XYRatio*0.5f,	-0.5f,	0.0f,	color,maxU,maxV}
		};
		
		memcpy(pVerts, verts, sizeof(verts));
		pVerts += sizeof(verts);
	}
	pNewBuf->Unlock();

	eaPush(&eaAtlasVertexBuffers, pNewBuf);
}

void FlexRenderer::CreateAllTextureAtlasBuffers()
{
	DefHash::iterator hashIter;
	DefHash::iterator hashEnd = DEF_ITER_END(GameTexturePortion);
	PointerPointerHash htTexturePointerToAtlasList;
	PointerPointerHash::iterator atlasIter;

	for(hashIter = DEF_ITER_BEGIN(GameTexturePortion); hashIter != hashEnd; ++hashIter) 
	{
		GameTexturePortion* pPortion = (GameTexturePortion*)hashIter->second;
		GameTexturePortion** eaPortions = NULL;
		if (!pPortion->hTex.pTex)
			continue;
		if (htTexturePointerToAtlasList.find(pPortion->hTex.pTex) != htTexturePointerToAtlasList.end())
		{
			eaPortions = (GameTexturePortion**)htTexturePointerToAtlasList[pPortion->hTex.pTex];
		}	

		eaPush(&eaPortions, pPortion);
		htTexturePointerToAtlasList[pPortion->hTex.pTex] = eaPortions;
		
	}

	for (atlasIter = htTexturePointerToAtlasList.begin(); atlasIter != htTexturePointerToAtlasList.end(); atlasIter++)
	{
		//for each referenced texture, iterate over all its portions and build a vertex buffer.
		CreateTextureAtlasVertexBuffer((GameTexture*)atlasIter->first, (GameTexturePortion**)atlasIter->second);
		eaDestroy(&atlasIter->second);
	}
}
FlexScratchSurface::FlexScratchSurface(int size)
{
	this->size = size;
	g_Renderer.GetD3DDevice()->CreateRenderTarget(size, size, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pRenderTarget, NULL);
	pOffscreenPlain = NULL;
}

void FlexRenderer::RenderToScratchSurface(FlexScratchSurface* pSurface, GameTexture* pTexture, FlexVertex2D* ppVerts, int iNumVerts)
{
	IDirect3DSurface9* origTarget = NULL;

	assert(!bActiveFrame);

	bActiveFrame = true;
	
	pD3DDevice->GetRenderTarget(0, &origTarget);
	pD3DDevice->SetRenderTarget(0, pSurface->GetRenderTarget());
	pD3DDevice->SetTexture(0, pTexture->pD3DTex);

	Begin2D();
	pD3DDevice->BeginScene();

	pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER  , D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);
	//lol slow as shit
	pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, iNumVerts/3, ppVerts, sizeof(FlexVertex2D));
	
	pD3DDevice->EndScene();
	End2D();

	pD3DDevice->SetRenderTarget(0, origTarget);

	bActiveFrame = false;
}

void FlexScratchSurface::SaveToPNG(TCHAR* filename)
{
	D3DXSaveSurfaceToFile(filename,  D3DXIFF_PNG, pRenderTarget, NULL, NULL);
}

IDirect3DSurface9* FlexScratchSurface::GetRenderTarget()
{
	return pRenderTarget;
}


FlexScratchSurface::~FlexScratchSurface()
{
	if (pRenderTarget)
		pRenderTarget->Release();
	if (pOffscreenPlain)
		pOffscreenPlain->Release();
}

void FlexScratchSurface::GetData(DWORD* pDataOut)
{
	if (!pOffscreenPlain)
		g_Renderer.GetD3DDevice()->CreateOffscreenPlainSurface(size, size, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &pOffscreenPlain, NULL);

	D3DSURFACE_DESC desc;
	pOffscreenPlain->GetDesc(&desc);
	
	g_Renderer.GetD3DDevice()->GetRenderTargetData(pRenderTarget, pOffscreenPlain);

	D3DLOCKED_RECT d3dlr;
	pOffscreenPlain->LockRect(&d3dlr, 0, 0); 
	char* pDst = (char*)d3dlr.pBits;
	memcpy(pDataOut, d3dlr.pBits, d3dlr.Pitch*desc.Height);
	pOffscreenPlain->UnlockRect();
}


FlexRenderer g_Renderer;

#include "Autogen\flexrenderer_h_ast.cpp"