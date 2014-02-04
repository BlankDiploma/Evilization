#pragma once

#include "stdafx.h"
#include "stddef.h"
#include "strhashmap.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "structparse.h"

using namespace std;
typedef stdext::hash_map<const wchar_t*, GameTexture*, stringHasher> TextureHash;

class TextureLibrary
{
	TextureHash htTextures;

public:

	GameTexture* GetTexture(const TCHAR* pchName);

	bool LoadTextures(const TCHAR* pchDir, const TCHAR* pchFilename, IDirect3DDevice9* pDevice);

	GameTexture* LoadNinepatchSetFromFile(const TCHAR* pchFilename, IDirect3DDevice9* pDevice);
	bool LoadNinepatchSets(const TCHAR* pchDir, const TCHAR* pchFilename, IDirect3DDevice9* pDevice);

	GameTexture* LoadFontFromFile( const TCHAR* pchFileName, IDirect3DDevice9* pDevice) ;
	bool LoadFonts(const TCHAR* pchDir, const TCHAR* pchFilename, IDirect3DDevice9* pDevice);

	~TextureLibrary();
};

extern TextureLibrary g_TextureLibrary;


#define LOAD_TEXTURES(a, b) g_TextureLibrary.LoadTextures(_T("data/textures/tex/"), _T(a), b);

#define GET_TEXTURE(a) g_TextureLibrary.GetTexture(a)

#define LOAD_NINEPATCHES(a, b) g_TextureLibrary.LoadNinepatchSets(_T("data/textures/ninepatch/"), _T(a), b);
#define LOAD_FONTS(a, b) g_TextureLibrary.LoadFonts(_T("data/textures/fonts/"), _T(a), b);