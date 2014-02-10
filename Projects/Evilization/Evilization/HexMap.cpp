
#include "stdafx.h"
#include "HexMap.h"
#include "PerlinMap.h"
#include "mtrand.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <queue>
#include "HexFeatures.h"
#include "GameState.h"
#include "flexrenderer.h"

inline int nextPowerOfTwo(int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v++;
}

hexTileDef* pOcean;
hexTileDef* pShallow;
hexTileDef* pMountain;

#define SEA_LEVEL 0.55

#define MOUNTAIN_ELEV_THRESHOLD 0.875
#define LAND_ELEV_THRESHOLD SEA_LEVEL
#define SHALLOWS_ELEV_THRESHOLD (SEA_LEVEL-0.055)
#define BIOME_MAP_SIZE 32

union biomeIndex{
	DWORD color;
	hexTileDef* pDef;
};

biomeIndex biomeMap[BIOME_MAP_SIZE][BIOME_MAP_SIZE];

void InitializeBiomeMap(LPCTSTR  filename)
{
	LPDIRECT3DTEXTURE9 pBitmap = NULL;
	HRESULT hr = D3DXCreateTextureFromFileEx(g_Renderer.GetD3DDevice(), filename, 0,0,0,0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, NULL, NULL, &pBitmap);

	D3DLOCKED_RECT d3dlr;
	pBitmap->LockRect(0, &d3dlr, 0, 0); 
	char* pDst = (char*)d3dlr.pBits;
	DWORD* pPixel;
	for (int j = 0; j < BIOME_MAP_SIZE; j++)
	{
		pPixel = (DWORD*)pDst;
		pDst += d3dlr.Pitch;
		for (int i = 0; i < BIOME_MAP_SIZE; i++)
		{
			biomeMap[j][i].color = *pPixel;
			pPixel++;
		}
	}

	pBitmap->UnlockRect(0);
	pBitmap->Release();
	pBitmap = NULL;

	DefHash::iterator hashIter;
	DefHash::iterator hashEnd = DEF_ITER_END(hexTileDef);

	for(hashIter = DEF_ITER_BEGIN(hexTileDef); hashIter != hashEnd; ++hashIter) 
	{
		for (int i = 0; i < BIOME_MAP_SIZE; i++)
			for (int j = 0; j < BIOME_MAP_SIZE; j++)
			{
				if (biomeMap[i][j].color == ((hexTileDef*)(hashIter->second))->color)
				{
					biomeMap[i][j].pDef = (hexTileDef*)(hashIter->second);
				}
			}
	}

	pOcean = GET_DEF_FROM_STRING(hexTileDef, L"ocean");
	pShallow = GET_DEF_FROM_STRING(hexTileDef, L"shallows");
	pMountain = GET_DEF_FROM_STRING(hexTileDef, L"mountain");
}


hexTileDef* BiomeLookup(float elev, float temp, float rain)
{
	temp *= BIOME_MAP_SIZE-1;
	rain *= BIOME_MAP_SIZE-1;
	if (elev <= SHALLOWS_ELEV_THRESHOLD)
		return pOcean;
	else if (elev <= LAND_ELEV_THRESHOLD)
		return pShallow;
	else if (elev > MOUNTAIN_ELEV_THRESHOLD)
		return pMountain;
	else
	{
		return biomeMap[(int)temp][(int)rain].pDef;
	}
	return NULL;
}

void CHexMap::Generate(int w, int h, int seed)
{
	assert (w <= 128 && h <= 128 && w > 0 && h > 0);
	if (pTiles)
		pTiles = NULL;
	randomFloats.seed(seed);
	CPerlinMap elev;
 	CPerlinMap temp;
	CPerlinMap rain;
	elev.GenerateMutlipleLevels(max(w,h), 4, 16, NULL);
	temp.GenerateMutlipleLevels(max(w,h), 4, 8, NULL);
	rain.GenerateMutlipleLevels(max(w,h), 2, 8, NULL);
	pTiles = new hexTile[w*h];
	this->w = w;
	this->h = h;
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++)
		{
			hexTileDef* pDef = BiomeLookup(elev.GetAt(i, j), temp.GetAt(i, j), rain.GetAt(i, j));
			pTiles[i + j*w].pDef = pDef;
			pTiles[i + j*w].pUnit = NULL;
			pTiles[i + j*w].pBuilding = NULL;
			pTiles[i + j*w].iPillageTurnsRemaining = pDef->iMaxPillageTurns;
			pTiles[i + j*w].slot.pDef = pDef->slotDef;
			pTiles[i + j*w].slot.pLaborOwner = NULL;
			pTiles[i + j*w].slot.loc.x = i;
			pTiles[i + j*w].slot.loc.y = j;
		}
		
	pStickFigure = GET_DEF_FROM_STRING(GameTexturePortion, _T("stickfigure"));
	pHouse = GET_DEF_FROM_STRING(GameTexturePortion, _T("house"));
	pSelectedTile = GET_DEF_FROM_STRING(GameTexturePortion, _T("selectedtile"));
	ppath = GET_DEF_FROM_STRING(GameTexturePortion, _T("path"));
	ppathblip = GET_DEF_FROM_STRING(GameTexturePortion, _T("pathblip"));
	ppathend = GET_DEF_FROM_STRING(GameTexturePortion, _T("pathend"));
	pFont = GET_TEXTURE(_T("courier_new"));
}

void CHexMap::RenderBuildingOnTile(hexBuildingDef* pDef, POINT pt, DWORD color, FLOATPOINT fpMapOffset)
{
	g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pDef->hTex), TileToScreen(pt.x, pt.y, fpMapOffset), color, ZOOM_PERCENT);
}

// Verteces are in this order:
//		     2
//		  1     3
//		
//		  6     4
//		     5

RECT selectedTile = {0,0,64,64};
RECT unit = {64,0,128,64};
RECT building = {128,0,192,64};
void CHexMap::RenderTile(POINT tilePt, hexTile* pTile, DWORD color, float scale)
{
	/*
	int index ;
	GfxSheetVertex vert [ 6 ] ;

	vert[0].x = tilePt.x;
	vert[0].y = tilePt.y + HEX_SIZE/4;

	vert[1].x = tilePt.x + HEX_SIZE/2;
	vert[1].y = tilePt.y;

	vert[2].x = tilePt.x + HEX_SIZE;
	vert[2].y = tilePt.y + HEX_SIZE/4;

	vert[3].x = tilePt.x + HEX_SIZE;
	vert[3].y = tilePt.y + HEX_SIZE*3/4;

	vert[4].x = tilePt.x + HEX_SIZE/2;
	vert[4].y = tilePt.y + HEX_SIZE;

	vert[5].x = tilePt.x;
	vert[5].y = tilePt.y + HEX_SIZE*3/4;

	for ( index = 0 ; index < 6 ; index ++ )
	{
		vert[index].u = 0.0f;
		vert[index].v = 0.0f;
		vert[index].z = 0.0f;
		vert[index].u1 = 0.0f;
		vert[index].v1 = 0.0f;
		vert[index].rhw = 1.0f;
		vert[index].diffuseColor = pTile->pDef->color;
	}

	//set the texture
	g_pMainDevice->SetTexture ( 0 , NULL );
	g_pMainDevice->DrawPrimitiveUP ( D3DPT_TRIANGLEFAN , 4 , &vert , sizeof ( GfxSheetVertex ) );

*/
	if (pTile->pDef->hTex.pObj)
		g_Renderer.AddSpriteToRenderList((GameTexturePortion*)pTile->pDef->hTex.pObj, tilePt, color, scale);
	if (pTile->pBuilding)
		g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pTile->pBuilding->pDef->hTex), tilePt, color, scale);
	if (pTile->pUnit)
		g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pTile->pUnit->GetDef()->hTex), tilePt, color, scale);
	//tempDevice->SetTransform(D3DTS_TEXTURE0, NULL );
}
void CHexMap::RenderFog(POINT tilePt)
{
	/*
	int index ;
	GfxSheetVertex vert [ 6 ] ;

 	vert[0].x = (float)tilePt.x;
	vert[0].y = (float)tilePt.y + HEX_SIZE/4;

	vert[1].x = (float)tilePt.x + HEX_SIZE/2;
	vert[1].y = (float)tilePt.y;

	vert[2].x = (float)tilePt.x + HEX_SIZE;
	vert[2].y = (float)tilePt.y + HEX_SIZE/4;

	vert[3].x = (float)tilePt.x + HEX_SIZE;
	vert[3].y = (float)tilePt.y + HEX_SIZE*3/4;

	vert[4].x = (float)tilePt.x + HEX_SIZE/2;
	vert[4].y = (float)tilePt.y + HEX_SIZE;

	vert[5].x = (float)tilePt.x;
	vert[5].y = (float)tilePt.y + HEX_SIZE*3/4;

	for ( index = 0 ; index < 6 ; index ++ )
	{
		vert[index].u = 0.0f;
		vert[index].v = 0.0f;
		vert[index].z = 0.0f;
		vert[index].u1 = 0.0f;
		vert[index].v1 = 0.0f;
		vert[index].rhw = 1.0f;
		vert[index].diffuseColor = 0xcc000000;
	}

	//set the texture
	g_pMainDevice->SetTexture ( 0 , NULL );
	g_pMainDevice->DrawPrimitiveUP ( D3DPT_TRIANGLEFAN , 4 , &vert , sizeof ( GfxSheetVertex ) );
	//tempDevice->SetTransform(D3DTS_TEXTURE0, NULL );
	*/
}

//l/r wrap
inline DWORD CHexMap::GetMinimapColorAt(int x, int y)
{
	return pTiles[x + y*w].pDef->color;
}

void CHexMap::UpdateMinimapTexture(GameTexture* pTex, RECT* view, FLOATPOINT fpMapOffset, playerVisibility* pVis)
{
	RECT tilesToRender;
	CopyRect(&tilesToRender, view);
	OffsetRect(&tilesToRender, (int)fpMapOffset.x, (int)fpMapOffset.y);
	tilesToRender.left /= HEX_SIZE;
	tilesToRender.right /= HEX_SIZE;
	tilesToRender.top /= HEX_SIZE*3/4;
	tilesToRender.bottom /= HEX_SIZE*3/4;
	D3DLOCKED_RECT d3dlr;
	pTex->pD3DTex->LockRect(0, &d3dlr, 0, 0); 
	char* pDst = (char*)d3dlr.pBits;
	DWORD* pPixel;
	DWORD color;
	for (int j = 0; j < h*2; j++)
	{
		pPixel = (DWORD*)pDst;
		pDst += d3dlr.Pitch;
		if ((j/2) % 2)
			pPixel++;
		for (int i = 0; i < w; i++)
		{

			if (!pVis || pVis->GetTileVis(i, j/2) == kVis_Clear)
			{
				color = GetMinimapColorAt(i, j/2);
			}
			else if (pVis->GetTileVis(i, j/2) == kVis_Fog)
			{
				color = GetMinimapColorAt(i, j/2);
 				color = color >> 2;
				color &= ~0x00C0C0C0;
				color |= 0xff000000;
			}
			else
				color = 0xff000000;

			*pPixel = color;
			pPixel++;

			if (!((j/2) % 2) || i < w-1)
			{
				*pPixel = color;
				pPixel++;
			}
		}
	}
	//render "what you're looking at" box
	//top and bottom
	pDst = (char*)d3dlr.pBits + d3dlr.Pitch*tilesToRender.top*2;
	for (int j = 0; j < RECT_HEIGHT(tilesToRender.)*2; j++)
	{
		pPixel = (DWORD*)pDst + tilesToRender.left*2;
		pDst += d3dlr.Pitch;
		*pPixel = 0xffff00ff;
		pPixel+= (tilesToRender.right-tilesToRender.left)*2-1;
		*pPixel = 0xffff00ff;
	}
	//left and right
	pDst = (char*)d3dlr.pBits + d3dlr.Pitch*tilesToRender.top*2;
	DWORD* pPixel2;
	pPixel = (DWORD*)pDst + tilesToRender.left*2;
	pPixel2 = (DWORD*)(pDst + d3dlr.Pitch * ((tilesToRender.bottom-tilesToRender.top)*2-1)) + tilesToRender.left*2;
	for (int i = 0; i < RECT_WIDTH(tilesToRender.)*2; i++)
	{
		*pPixel++ = 0xffff00ff;
		*pPixel2++ = 0xffff00ff;
	}

	pTex->pD3DTex->UnlockRect(0);
	//			D3DXSaveTextureToFile(_T("derp.png"), D3DXIFF_PNG, pTileLightmapDXT1Texture, NULL);
}

inline POINT TileToScreen(int x, int y, FLOATPOINT fpOffset)
{
	POINT pt = {(LONG)(x*HEX_SIZE + (y%2 ? HEX_SIZE/2 : 0) - fpOffset.x), (LONG)(y * (HEX_SIZE*3/4 + 1) - fpOffset.y)};
	return pt;
}

void CHexMap::Render(RECT* view, FLOATPOINT fpMapOffset, CHexPlayer* pPlayer)
{
	RECT tilesToRender;
	playerVisibility* pVis = FOG_OF_WAR ? pPlayer->GetVisibility() : NULL;
	CopyRect(&tilesToRender, view);
	OffsetRect(&tilesToRender, (int)fpMapOffset.x, (int)fpMapOffset.y);
	tilesToRender.left /= HEX_SIZE;
	tilesToRender.right /= HEX_SIZE;
	tilesToRender.top /= HEX_SIZE*3/4;
	tilesToRender.bottom /= HEX_SIZE*3/4;
	InflateRect(&tilesToRender, 1, 1);
	for (int j = max(0,tilesToRender.top); j < min(tilesToRender.bottom,h); j++)
		for (int i = tilesToRender.left; i < tilesToRender.right; i++)
		{
			POINT validPt = {i, j};
			POINT screenPt = TileToScreen(i, j, fpMapOffset);
			MakeLocValid(&validPt);
			if (!pVis || pVis->GetTileVis(validPt.x, validPt.y) != kVis_Shroud)
			{
				if (pVis && pVis->GetTileVis(validPt.x, validPt.y) == kVis_Fog)
					RenderTile(screenPt, GetTile(i, j), 0xff333333, ZOOM_PERCENT);
				else
					RenderTile(screenPt, GetTile(i, j), 0xFFFFFFFF, ZOOM_PERCENT);
			}
/*
			POINT sanitizedPt = {i,j};
			MakeLocValid(&sanitizedPt);
			if (pPathMap && pPathMap[sanitizedPt.x + sanitizedPt.y*w] > 0)
			{
				CAtlString str;
				str.Format(_T("%i"), pPathMap[sanitizedPt.x + sanitizedPt.y*w]);
				POINT pt = TileToScreen(i, j, fpMapOffset);
				pFont->RenderString(str, pt.x + 32, pt.y + 32, 0xffff0000);
			}
			*/
		}
	for (int iCity = 0; iCity < eaSize(&pPlayer->eaCities); iCity++)
	{
		cityProject** eaProjects = pPlayer->eaCities[iCity]->GetProductionQueue();
		for (int iProj = 0; iProj < eaSize(&eaProjects); iProj++)
		{
			if (eaProjects[iProj]->eType == kProject_Building)
			{
				POINT screenPt = TileToScreen(eaProjects[iProj]->loc.x, eaProjects[iProj]->loc.y, fpMapOffset);
				hexBuildingDef* pDef = (hexBuildingDef*)eaProjects[iProj]->pDef;
				g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pDef->hTex), screenPt, 0xaaFFFFFF, ZOOM_PERCENT);
			}
		}
	}
}

void CHexMap::RenderPath( RECT* view, FLOATPOINT fpMapOffset, CHexUnit* pUnit, HEXPATH* pPath, int alpha )
{
	if (!pPath)
		 return;
	RECT tilesToRender;
	GameTexturePortion* pathSeg;
	CopyRect(&tilesToRender, view);
	OffsetRect(&tilesToRender, (int)(fpMapOffset.x), (int)(fpMapOffset.y));
	tilesToRender.left /= HEX_SIZE;
	tilesToRender.right /= HEX_SIZE;
	tilesToRender.top /= HEX_SIZE*3/4;
	tilesToRender.bottom /= HEX_SIZE*3/4;
	InflateRect(&tilesToRender, 1, 1);
	//i starts at 1, don't render first tile of the path
	int iMovement = pUnit->GetMovRemaining();
	int iTurn = 0;
	for (int i = pPath->start; i < pPath->size; i++)
	{
		iMovement -= GetTile(pPath->pPoints[i])->pDef->iMoveCost;
		bool bShowTurnCount = false;
		if (i == pPath->size-1)
		{
			pathSeg = ppathend;
			bShowTurnCount = true;
			iTurn++;
		}
		else if (iMovement <= 0)
		{
			pathSeg = ppathblip;
			bShowTurnCount = true;
			iMovement = pUnit->GetDef()->movement;
			iTurn++;
		}
		else
		{
			pathSeg = ppath;
		}
		if (PtInRect(&tilesToRender, pPath->pPoints[i]))
		{
			POINT renderPt = TileToScreen(pPath->pPoints[i].x, pPath->pPoints[i].y, fpMapOffset);
			g_Renderer.AddSpriteToRenderList(pathSeg, renderPt, alpha << 24, ZOOM_PERCENT);
			if (bShowTurnCount)
			{
				TCHAR buf[4];
				wsprintf(buf, _T("%i"), iTurn);
				g_Renderer.AddStringToRenderList(pFont, buf, (float)(renderPt.x+HEX_SIZE/2+1), (float)(renderPt.y+HEX_SIZE/2-7+1), (alpha << 24), true, false, false);
				g_Renderer.AddStringToRenderList(pFont, buf, (float)(renderPt.x+HEX_SIZE/2), (float)(renderPt.y+HEX_SIZE/2-7), (alpha << 24) | (iTurn > 1 ? 0xff0000 : 0xff00), true, false, false);
			}
		}
	}
}

void CHexMap::GetMatchingOnscreenTiles(RECT* mapViewport, FLOATPOINT fpMapOffset, void*** peaTilesOut, findTileFunc pFunc, void* pData)
{
	RECT tilesToRender;
	eaClear(peaTilesOut);
	CopyRect(&tilesToRender, mapViewport);
	OffsetRect(&tilesToRender, (int)(fpMapOffset.x), (int)(fpMapOffset.y));
	tilesToRender.left /= HEX_SIZE;
	tilesToRender.right /= HEX_SIZE;
	tilesToRender.top /= HEX_SIZE*3/4;
	tilesToRender.bottom /= HEX_SIZE*3/4;
	InflateRect(&tilesToRender, 1, 1);
	for (int i = tilesToRender.left; i < tilesToRender.right; i++)
		for (int j = max(0,tilesToRender.top); j < min(tilesToRender.bottom,h); j++)
		{
			hexTile* pTile = GetTile(i, j);
			pFunc(pTile, pData, peaTilesOut);
		}
}

void CHexMap::RenderInterface( RECT* mapViewport, FLOATPOINT fpMapOffset, POINT ptMouseoverTile )
{
	POINT pt = TileToScreen(ptMouseoverTile.x, ptMouseoverTile.y, fpMapOffset);
	g_Renderer.AddSpriteToRenderList(pSelectedTile, pt, 0xffffffff, ZOOM_PERCENT);
	// show pathfinding info
/*
	RECT tilesToRender;
	CopyRect(&tilesToRender, mapViewport);
	OffsetRect(&tilesToRender, fpMapOffset.x, fpMapOffset.y);
	tilesToRender.left /= HEX_SIZE;
	tilesToRender.right /= HEX_SIZE;
	tilesToRender.top /= HEX_SIZE*3/4;
	tilesToRender.bottom /= HEX_SIZE*3/4;
	InflateRect(&tilesToRender, 1, 1);
	for (int i = tilesToRender.left; i < tilesToRender.right; i++)
		for (int j = max(0,tilesToRender.top); j < min(tilesToRender.bottom,h); j++)
		{
			POINT sanitizedPt = {i,j};
			MakeLocValid(&sanitizedPt);
			if (pPathMap && pPathMap[sanitizedPt.x + sanitizedPt.y*w] != 0)
			{
				CAtlString str;
				str.Format(_T("%i"), pPathMap[sanitizedPt.x + sanitizedPt.y*w]);
				POINT pt = TileToScreen(i, j, fpMapOffset);
				pFont->RenderString(str, pt.x + 32, pt.y + 32, 0xffff0000);
			}

		}
		*/
}

void CHexMap::GetTileDescription( hexTile* pTile, TCHAR* pchDescOut )
{
	wsprintf(pchDescOut, L"%s", pTile->pDef->name);
}

bool CHexMap::PointIsValidStartPos(POINT pt)
{
	//needs to be empty dry land
	hexTile* pTile = GetTile(pt);
	if (!(pTile->pDef->eFlags & kTileFlag_Walkable) ||
		pTile->pUnit || pTile->pBuilding)
		return false;

	//must have at least one valid adjacent tile
	//(later will expand this to "must start on a big enough landmass")
	for (int i = 0; i < 6; i++)
	{
		POINT adjPt = GetTileInDirection(pt, i);
		pTile = GetTile(adjPt);
		if ((pTile->pDef->eFlags & kTileFlag_Walkable))
			return true;
	}
	return false;
}

POINT CHexMap::GetRandomStartingPos()
{
	//derp
	POINT pt;
	do
	{
		pt.x = (LONG)(randomFloats() * w);
		pt.y = (LONG)(randomFloats() * h);
	} while (!PointIsValidStartPos(pt));
	return pt;
}

inline bool CHexMap::MakeLocValid(POINT* pt)
{
	if (pt->y < 0 || pt->y >= h)
		return false;
	if (pt->x < 0)
		pt->x += w;
	if (pt->x >= w)
		pt->x -= w;
	return true;
}

inline bool findShroud(POINT pt, void* pData)
{
	playerVisibility* pVis = (playerVisibility*)pData;
	return (pVis->GetTileVis(pt) == kVis_Shroud);
}

inline bool findTile(POINT pt, void* pData)
{
	POINT* pDst = (POINT*)pData;
	return pDst->x == pt.x && pDst->y == pt.y;
}

inline bool TileIsReachable(hexTile* pTile)
{
	return ((pTile->pDef->eFlags & kTileFlag_Walkable)) && !pTile->pUnit;
}

//pathfinds to the nearest tile that would reveal shrouded tiles
int CHexMap::HexPathfindInternal(POINT a, CHexUnit* pUnit, finishedFunc pfFinished, void* pData, HEXPATH** pPathOut, int maxUnreachableDist)
{
	priority_queue<PATHFINDPOINT, std::deque<PATHFINDPOINT>, tilePathfindCompare> tileQueue;
	if (!pPathMap)
		pPathMap = new int [w*h];
	BOOL bFoundPath = false;
	int curDist = 1;
	int maxDist = 1;
	POINT nextPoint;

	memset(pPathMap, 0, sizeof(int)*w * h);

	PATHFINDPOINT currentPoint = {a.x, a.y, 1, 0};
	tileQueue.push(currentPoint);
	pPathMap[a.x + a.y*w] = 1;

	while (!tileQueue.empty() && !bFoundPath)
	{
		currentPoint = tileQueue.top();
		tileQueue.pop();
		curDist = pPathMap[currentPoint.x + currentPoint.y*w];
		if (curDist > maxDist)
			maxDist = curDist;
		for (int i = 0; i < 6; i++)
		{
			nextPoint = GetTileInDirection(&currentPoint, (HexDirection)i);
			if (!MakeLocValid(&nextPoint))
				continue;
			if ((maxUnreachableDist == 0 || currentPoint.unreachableDist == 0 || (currentPoint.unreachableDist < maxUnreachableDist && !TileIsReachable(&pTiles[nextPoint.x + nextPoint.y*w]))) && pfFinished(nextPoint, pData))
			{
				currentPoint.x = nextPoint.x;
				currentPoint.y = nextPoint.y;
				pPathMap[nextPoint.x + nextPoint.y*w] = TileIsReachable(&pTiles[nextPoint.x + nextPoint.y*w]) ? curDist + 1 : -(currentPoint.unreachableDist+1);
				bFoundPath = true;
				break;
			}
			if (pPathMap[nextPoint.x + nextPoint.y*w] == 0 || (pPathMap[nextPoint.x + nextPoint.y*w] < -(currentPoint.unreachableDist+1)))
			{
				if (TileIsReachable(&pTiles[nextPoint.x + nextPoint.y*w]))
				{
					if (maxUnreachableDist == 0 || currentPoint.unreachableDist == 0)
					{
						pPathMap[nextPoint.x + nextPoint.y*w] = curDist+pTiles[nextPoint.x + nextPoint.y*w].pDef->iMoveCost;
						PATHFINDPOINT next = {nextPoint.x, nextPoint.y, pPathMap[nextPoint.x + nextPoint.y*w], 0};
						tileQueue.push(next);
					}
				}
				else if (currentPoint.unreachableDist < maxUnreachableDist)
				{
					pPathMap[nextPoint.x + nextPoint.y*w] = -(currentPoint.unreachableDist+1);
					PATHFINDPOINT next = {nextPoint.x, nextPoint.y, -(currentPoint.unreachableDist+1), currentPoint.unreachableDist+1};
					tileQueue.push(next);
				}
			}
			
		}

	}
	if (!bFoundPath)
		return 0;
	maxDist++;
	int iTile = 1;
	*pPathOut = new HEXPATH(maxDist);
	//currentpoint = a

	POINT smallestPoint;
	int smallestAdj;

	(*pPathOut)->start = maxDist;
	if (pPathMap[currentPoint.x + currentPoint.y*w] > 0)
	{
		(*pPathOut)->pPoints[maxDist-1].x = currentPoint.x;
		(*pPathOut)->pPoints[maxDist-1].y = currentPoint.y;
		(*pPathOut)->start--;
	}

	while (!(currentPoint.x == a.x && currentPoint.y == a.y))
	{
		smallestAdj = pPathMap[currentPoint.x + currentPoint.y*w];
		if (smallestAdj < 0)
			smallestAdj = INT_MAX;
		for (int i = 0; i < 6; i++)
		{
			nextPoint = GetTileInDirection(&currentPoint, (HexDirection)i);
			if (MakeLocValid(&nextPoint))
			{
				//only allowed to pick unreachable tiles up to maxUnreachableDist
				if ((pPathMap[nextPoint.x + nextPoint.y*w] > 0 || (pPathMap[nextPoint.x + nextPoint.y*w] < 0 && iTile < maxUnreachableDist)) &&
					abs(pPathMap[nextPoint.x + nextPoint.y*w]) < abs(smallestAdj))
				{
					smallestAdj = pPathMap[nextPoint.x + nextPoint.y*w];
					smallestPoint = nextPoint;
				}
			}
		}
		currentPoint.x = smallestPoint.x;
		currentPoint.y = smallestPoint.y;
		if (pPathMap[smallestPoint.x + smallestPoint.y*w] > 0)
			(*pPathOut)->pPoints[--((*pPathOut)->start)] = smallestPoint;
		iTile++;
	}

	if ((*pPathOut)->start >= maxDist-1)
	{//we didn't actually get a path that goes anywhere
		delete (*pPathOut);
		 (*pPathOut) = NULL;
		return 0;
	}
	return (*pPathOut)->size;
}

int CHexMap::HexPathfindTile(CHexUnit* pUnit, POINT a, POINT b, HEXPATH** pPathOut)
{
	HEXPATH* pNew = NULL;
	if (pCachedPath && pCachedPath->pPoints[pCachedPath->start].x == a.x && pCachedPath->pPoints[pCachedPath->start].y == a.y &&
		pCachedPath->pPoints[pCachedPath->size-1].x == b.x && pCachedPath->pPoints[pCachedPath->size-1].y == b.y)
	{
		//use cached path
		*pPathOut = pCachedPath;
		return pCachedPath->size;
	}

	//exit early if dest is unreachable or points are unsanitizable
	if ((!MakeLocValid(&a) || !MakeLocValid(&b)) || !TileIsReachable(&pTiles[b.x + b.y*w]))
		return 0;

	HexPathfindInternal(a, pUnit,findTile, &b, &pNew, 0);

	if (pNew)
	{
		delete pCachedPath;
		pCachedPath = pNew;
		pCachedPath->start++;//skip origin tile
	}
	*pPathOut = pCachedPath;
	return pCachedPath ? pCachedPath->size : 0;
}

//doesn't use path caching
int CHexMap::HexPathfindShroud(CHexUnit* pUnit, playerVisibility* pVis, POINT a, HEXPATH** pPathOut)
{
	HEXPATH* pNew = NULL;

	//exit early if dest is unreachable or points are unsanitizable
	if (!MakeLocValid(&a))
		return 0;

	HexPathfindInternal(a, pUnit,findShroud, pVis, &pNew, pUnit->GetVisRadius());
	if (pNew)
	{
		pNew->start++;//skip origin tile
		*pPathOut = pNew;
		return pNew->size;
	}

	*pPathOut = NULL;
	return 0;
}

CHexBuilding* CHexMap::CreateBuilding(hexBuildingDef* pDef, CHexPlayer* pOwner, POINT loc)
{
	hexTile* pTile = GetTile(loc);
	if (pDef->eType == kBuilding_City)
		pTile->pBuilding = new CHexCity(pDef, pOwner);
	else
		pTile->pBuilding = new CHexBuilding(pDef, pOwner);
	pTile->pBuilding->SetLoc(loc);
	return pTile->pBuilding;
}

CHexUnit* CHexMap::CreateUnit(hexUnitDef* pDef, CHexPlayer* pOwner, POINT loc)
{
	hexTile* pTile = GetTile(loc);
	pTile->pUnit = new CHexUnit(pDef, pOwner);
	pTile->pUnit->SetLoc(loc);
	return pTile->pUnit;
}

void CHexMap::PlacePlayerStart( CHexPlayer* pPlayer )
{
	POINT pt = GetRandomStartingPos();
	CHexBuilding* pBuilding = CreateBuilding(GET_DEF_FROM_STRING(hexBuildingDef, L"house"), pPlayer, pt);
	((CHexCity*)pBuilding)->AdjustPop(2);

	for (int i = 0; i < 6; i++)
	{
		POINT adjPt = GetTileInDirection(pt, (int)i);
		hexTile* pTile = GetTile(adjPt);
		if ((pTile->pDef->eFlags & kTileFlag_Walkable))
		{
			CreateUnit(GET_DEF_FROM_STRING(hexUnitDef, L"stickfigure"), pPlayer, adjPt);
			break;
		}
	}
}

//returns true if the order has been fully completed
bool CHexMap::ProcessOrder( CHexUnit* pUnit, hexUnitOrder* pOrder, CHexPlayer* pOwner )
{
	switch (pOrder->eType)
	{
	case kOrder_Move:
		{
			if (!MoveUnit(pUnit, pOrder->pPath->pPoints[pOrder->pPath->start]))
				return false;

			pOrder->pPath->start++;
			if (pOrder->pPath->start < pOrder->pPath->size)
				return false;
			return true;
		}break;
	case kOrder_AutoExplore:
		{
			if (!pOrder->pPath)
			{
				if (!HexPathfindShroud(pUnit, pOwner->GetVisibility(), pUnit->GetLoc(), &pOrder->pPath))
				{
					//no path to shroud
					return true;
				}
			}
			if (!MoveUnit(pUnit, pOrder->pPath->pPoints[pOrder->pPath->start]))
			{
				delete pOrder->pPath;
				pOrder->pPath = NULL;
				if (!HexPathfindShroud(pUnit, pOwner->GetVisibility(), pUnit->GetLoc(), &pOrder->pPath))
				{
					//no path to shroud
					return true;
				}
				return false;
			}

			pOrder->pPath->start++;
			if (pOrder->pPath->start >= pOrder->pPath->size)
			{
				delete pOrder->pPath;
				pOrder->pPath = NULL;
			}
			return false;
		}break;
	case kOrder_Sleep:
		{
			pUnit->SpendMov(999);
			return false;
		}break;
	case kOrder_Melee:
		{
		}break;
	}
	return false;
}

bool CHexMap::MoveUnit( CHexUnit* pUnit, POINT pt )
{
	if (pUnit && pUnit->GetMovRemaining() > 0)
	{
		hexTile* a = GetTile(pUnit->GetLoc());
		hexTile* b = GetTile(pt);
		if ((b->pDef->eFlags & kTileFlag_Walkable) && !b->pUnit)
		{
			pUnit->SpendMov(b->pDef->iMoveCost);
			a->pUnit = NULL;
			b->pUnit = pUnit;
			pUnit->SetLoc(pt);
			return true;
		}
	}
	return false;
}

int CHexMap::GetTilesInRadius( POINT pt, int rad, POINT* ptTilesOut )
{
	if (rad < 0 || !ptTilesOut)
		return 0;
	int num = 0;
	float left = 0.5, right = 0.5;
	if (pt.y % 2)//odd center row
	{
		left = 1.0;
		right = 0.0;
	}
	//center row
	for (int iX = pt.x-rad; iX <= pt.x+rad; iX++)
	{
		ptTilesOut[num].x = iX;
		ptTilesOut[num].y = pt.y;
		MakeLocValid(&ptTilesOut[num]);
		num++;
	}
	for (int iY = 1; iY <= rad; iY++)
	{
		//top
		for (float iX = pt.x-rad+left; iX <= pt.x+rad-right; iX++)
		{
			if (iX < 0)
				ptTilesOut[num].x = (LONG)(iX-0.5);
			else
				ptTilesOut[num].x = (LONG)(iX);
			ptTilesOut[num].y = pt.y-iY;
			MakeLocValid(&ptTilesOut[num]);
			num++;
		}
		//bottom
		for (float iX = pt.x-rad+left; iX <= pt.x+rad-right; iX++)
		{
			if (iX < 0)
				ptTilesOut[num].x = (LONG)(iX-0.5);
			else
				ptTilesOut[num].x = (LONG)(iX);
			ptTilesOut[num].y = pt.y+iY;
			MakeLocValid(&ptTilesOut[num]);
			num++;
		}
		left += 0.5;
		right += 0.5;
	}
	return num;
}

bool CHexMap::BuildingCanBeBuiltOnTile(hexBuildingDef* pBuildingDef, POINT tilePt)
{
	hexTile* pTile = GetTile(tilePt);
	if (pTile->pBuilding)
		return false;

	//tile types
	if ((pTile->pDef->eFlags & kTileFlag_Water) && (pBuildingDef->eFlags & kBuildingFlag_BuildOnOcean))
		return true;
	if ((pTile->pDef->eFlags & kTileFlag_Mountain) && (pBuildingDef->eFlags & kBuildingFlag_BuildOnMountain))
		return true;
	if ((pTile->pDef->eFlags & kTileFlag_Walkable) && (pBuildingDef->eFlags & kBuildingFlag_BuildOnLand))
		return true;

	return false;
}

#include "Autogen\HexMap_h_ast.cpp"