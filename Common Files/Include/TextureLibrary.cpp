#pragma once

#include "stdafx.h"
#include "stddef.h"
#include "strhashmap.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "FlexRenderer.h"
#include "TextureLibrary.h"
#include "structparse.h"
#include <stdlib.h>
#include <dxerr.h>
#include "flexerrorwindow.h"

using namespace std;

TextureLibrary g_TextureLibrary;

GameTexture* TextureLibrary::GetTexture(const TCHAR* pchName)
{
	TextureHash::iterator hashIter;

	hashIter = htTextures.find(pchName);
	if(hashIter != htTextures.end())
	{
		return hashIter->second;
	}
	return NULL;
}

bool TextureLibrary::LoadTextures(const TCHAR* pchDir, const TCHAR* pchFilename, IDirect3DDevice9* pDevice)
{
	TCHAR directory[260] = {0};
	WIN32_FIND_DATA f;
	wcscpy_s(directory, pchDir);
	wcscat_s(directory, pchFilename);
	HANDLE h = FindFirstFile(directory, &f);
	TCHAR fnameBuf[_MAX_FNAME];
	if(h != INVALID_HANDLE_VALUE)
	{
		do{
			wcscpy_s(directory, pchDir);
			wcscat_s(directory, f.cFileName);
			//load texture here
			if (f.cFileName[0] == '.')
				continue;
			IDirect3DTexture9* pTex = NULL;
			HRESULT error = D3DXCreateTextureFromFileEx(pDevice, directory, 0, 0, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_NONE, 0xffff00ff, NULL, NULL, &pTex);
			
			if (FAILED(error))
			{
				Errorf("DirectX Create Texture Error: %s (%s)", DXGetErrorString(error), DXGetErrorString(error));
			}
			_wsplitpath_s(directory, NULL, 0, NULL, 0, fnameBuf, _MAX_FNAME, NULL, 0);

			GameTexture* pGameTex = new GameTexture;
			pGameTex->eType = kTextureType_Default;
			pGameTex->pD3DTex = pTex;
			
			D3DSURFACE_DESC pDesc;
			pGameTex->pD3DTex->GetLevelDesc(0, &pDesc);
			pGameTex->width = pDesc.Width;
			pGameTex->height = pDesc.Height;

			htTextures[_wcsdup(fnameBuf)] = pGameTex;

		} while(FindNextFile(h, &f));
	}
	else
	{
		fprintf(stderr, "Error opening directory\n");
		return false;
	}
	FindClose(h);
	return true;

}

TextureLibrary::~TextureLibrary()
{
	TextureHash::iterator hashIter;
	for (hashIter = htTextures.begin(); hashIter != htTextures.end(); hashIter++)
	{
		if (hashIter->second)
		{
			if (hashIter->second->pD3DTex)
				hashIter->second->pD3DTex->Release();
			if (hashIter->second->patches)
				delete [] hashIter->second->patches;
			delete hashIter->second;
		}
	}
}

GameTexture* TextureLibrary::LoadNinepatchSetFromFile( const TCHAR* pchFileName, IDirect3DDevice9* pDevice) 
{ 
	GameTexture* pSet = new GameTexture;
	D3DXCreateTextureFromFileEx(pDevice, pchFileName, 0, 0, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0xffff00ff, NULL, NULL, &pSet->pD3DTex);
	D3DSURFACE_DESC pDesc;
	pSet->eType = kTextureType_Ninepatch;
	pSet->pD3DTex->GetLevelDesc(0, &pDesc);
	D3DLOCKED_RECT d3dlr;
	HRESULT hr = pSet->pD3DTex->LockRect(0, &d3dlr, 0, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;
	int iCount = 0;
	RECT innerBuffer[64];
	RECT outerBuffer[64];
	pSet->width = pDesc.Width;
	pSet->height = pDesc.Height;

	for (unsigned int x = 0; x < pDesc.Width; x++)
	{
		for (unsigned int y = 0; y < pDesc.Height; y++)
		{
			if (pDst[y*DPitch + x] == 0xff00ffff)
			{
				int ix = x;
				outerBuffer[iCount].left = ix+1;
				while (pDst[y*DPitch + ++ix] != 0xff000000);
				innerBuffer[iCount].left = ix;
				while (pDst[y*DPitch + ++ix] == 0xff000000);
				innerBuffer[iCount].right = ix;
				while (pDst[y*DPitch + ix++] != 0xffff0000);
				outerBuffer[iCount].right = ix;

				int iy = y;
				outerBuffer[iCount].top = iy+1;
				while (pDst[++iy*DPitch + x] != 0xff000000);
				innerBuffer[iCount].top = iy;
				while (pDst[++iy*DPitch + x] == 0xff000000);
				innerBuffer[iCount].bottom = iy;
				while (pDst[iy++*DPitch + x] != 0xffff0000);
				outerBuffer[iCount].bottom = iy;

				iCount++;
			}
		}
	}
	pSet->patches = new ninepatch[iCount];
	for (int i = 0; i < iCount; i++)
	{
		pSet->patches[i].Set(&outerBuffer[i], &innerBuffer[i]);
	}
	pSet->iNum = iCount;

	pSet->pD3DTex->UnlockRect(0);

	return pSet;
}

bool TextureLibrary::LoadNinepatchSets(const TCHAR* pchDir, const TCHAR* pchFilename, IDirect3DDevice9* pDevice)
{
	TCHAR directory[260] = {0};
	WIN32_FIND_DATA f;
	wcscpy_s(directory, pchDir);
	wcscat_s(directory, pchFilename);
	HANDLE h = FindFirstFile(directory, &f);
	TCHAR fnameBuf[_MAX_FNAME];
	if(h != INVALID_HANDLE_VALUE)
	{
		do{
			wcscpy_s(directory, pchDir);
			wcscat_s(directory, f.cFileName);
			//load texture here
			if (f.cFileName[0] == '.')
				continue;
			
			GameTexture* pSet = LoadNinepatchSetFromFile(directory, pDevice);
			_wsplitpath_s(directory, NULL, 0, NULL, 0, fnameBuf, _MAX_FNAME, NULL, 0);
			htTextures[_wcsdup(fnameBuf)] = pSet;

		} while(FindNextFile(h, &f));
	}
	else
	{
		fprintf(stderr, "Error opening directory\n");
		return false;
	}
	FindClose(h);
	return true;

}

GameTexture* TextureLibrary::LoadFontFromFile( const TCHAR* pchFileName, IDirect3DDevice9* pDevice) 
{ 
	GameTexture* pSet = new GameTexture;
	D3DXCreateTextureFromFileEx(pDevice, pchFileName, 0, 0, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0xffff00ff, NULL, NULL, &pSet->pD3DTex);
	D3DSURFACE_DESC pDesc;
	pSet->eType = kTextureType_Font;
	pSet->pD3DTex->GetLevelDesc(0, &pDesc);
	pSet->width = pDesc.Width;
	pSet->height = pDesc.Height;
	D3DLOCKED_RECT d3dlr;
	HRESULT hr = pSet->pD3DTex->LockRect(0, &d3dlr, 0, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;
	
	for (unsigned int x = 0; x < pDesc.Width; x++)
	{
		if (pDst[x] == 0xff000000)
		{
			pSet->letterWidth = x+1;
			pDst[x] = 0x00000000;
			break;
		}
	}

	for (unsigned int y = 0; y < pDesc.Height; y++)
	{
		if (pDst[y*DPitch] == 0xff000000)
		{
			pDst[y*DPitch] = 0x00000000;
			pSet->letterHeight = y + 1;
			break;
		}
	}

	pSet->pD3DTex->UnlockRect(0);

	return pSet;
}

bool TextureLibrary::LoadFonts(const TCHAR* pchDir, const TCHAR* pchFilename, IDirect3DDevice9* pDevice)
{
	TCHAR directory[260] = {0};
	WIN32_FIND_DATA f;
	wcscpy_s(directory, pchDir);
	wcscat_s(directory, pchFilename);
	HANDLE h = FindFirstFile(directory, &f);
	TCHAR fnameBuf[_MAX_FNAME];
	if(h != INVALID_HANDLE_VALUE)
	{
		do{
			wcscpy_s(directory, pchDir);
			wcscat_s(directory, f.cFileName);
			//load texture here
			if (f.cFileName[0] == '.')
				continue;
			
			GameTexture* pFont = LoadFontFromFile(directory, pDevice);
			_wsplitpath_s(directory, NULL, 0, NULL, 0, fnameBuf, _MAX_FNAME, NULL, 0);
			htTextures[_wcsdup(fnameBuf)] = pFont;

		} while(FindNextFile(h, &f));
	}
	else
	{
		fprintf(stderr, "Error opening directory\n");
		return false;
	}
	FindClose(h);
	return true;

}