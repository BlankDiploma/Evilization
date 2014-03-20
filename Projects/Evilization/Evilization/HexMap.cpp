
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
#include "flexErrorWindow.h"

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

#define BIOME_MAP_SIZE 32

//vertex buffer constants
#define TERRAIN_CHUNK_WIDTH 16
#define TERRAIN_CHUNK_HEIGHT 16

#define TESSELLATION_DEGREE_COASTLINE 3
#define TESSELLATION_DEGREE_OTHER 1

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


hexTileDef* BiomeLookup(hexMapGenerationDesc* pDesc, float elev, float temp, float rain)
{
	temp *= BIOME_MAP_SIZE-1;
	rain *= BIOME_MAP_SIZE-1;
	if (elev <= pDesc->fOceanThreshold)
		return pOcean;
	else if (elev > pDesc->fMountainThreshold)
		return pMountain;
	else
	{
		return biomeMap[(int)temp][(int)rain].pDef;
	}
	return NULL;
}

void CHexMap::ApplyShallowWater()
{
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++)
		{
			if (pTiles[i + j*w].pDef == pOcean)
			{
				for (int iNeighbor = kHexDir_W; iNeighbor < kHexDir_Count; iNeighbor++)
				{
					hexTile* pNeighbor = GetTile(GetTileInDirection(i, j, iNeighbor));
					if (pNeighbor && pNeighbor->pDef != pShallow && pNeighbor->pDef != pOcean)
					{
						pTiles[i + j*w].pDef = pShallow;
						break;
					}
				}
			}
		}
}

void CHexMap::Generate(hexMapGenerationDesc* pDesc, int seed)
{
	assert (pDesc && pDesc->w <= 128 && pDesc->h <= 128 && pDesc->w > 0 && pDesc->h > 0);
	assert((pDesc->w % TERRAIN_CHUNK_WIDTH == 0) && (pDesc->h % TERRAIN_CHUNK_HEIGHT == 0));
	if (pTiles)
		pTiles = NULL;

	this->w = pDesc->w;
	this->h = pDesc->h;

	randomFloats.seed(seed);
	CPerlinMap elev;
 	CPerlinMap temp;
	CPerlinMap rain;

	elev.GenerateMutlipleLevels(max(w,h), 4, 16, NULL, true);
	temp.GenerateMutlipleLevels(max(w,h), 4, 16, NULL, true);
	rain.GenerateMutlipleLevels(max(w,h), 2, 8, NULL, true);

	pTiles = new hexTile[w*h];
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++)
		{
			hexTileDef* pDef = BiomeLookup(pDesc, elev.GetAt(i, j), temp.GetAt(i, j), rain.GetAt(i, j));
			pTiles[i + j*w].pDef = pDef;
			pTiles[i + j*w].pUnit = NULL;
			pTiles[i + j*w].pBuilding = NULL;
			pTiles[i + j*w].iPillageTurnsRemaining = pDef->iMaxPillageTurns;
			pTiles[i + j*w].slot.pDef = pDef->slotDef;
			pTiles[i + j*w].slot.pLaborOwner = NULL;
			pTiles[i + j*w].slot.loc.x = i;
			pTiles[i + j*w].slot.loc.y = j;
		}
	
	ApplyShallowWater();

	pStickFigure = GET_DEF_FROM_STRING(GameTexturePortion, _T("stickfigure"));
	pHouse = GET_DEF_FROM_STRING(GameTexturePortion, _T("house"));
	pSelectedTile = GET_DEF_FROM_STRING(GameTexturePortion, _T("selectedtile"));
	ppath = GET_DEF_FROM_STRING(GameTexturePortion, _T("path"));
	ppathblip = GET_DEF_FROM_STRING(GameTexturePortion, _T("pathblip"));
	ppathend = GET_DEF_FROM_STRING(GameTexturePortion, _T("pathend"));
	pFont = GET_TEXTURE(_T("courier_new"));
	CreateAllTerrainVertexBuffers();
}

void CHexMap::RenderBuildingOnTile(hexBuildingDef* pDef, POINT pt, DWORD color, FLOATPOINT fpMapOffset)
{
	g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pDef->hTex), TileToScreen(pt.x, pt.y, fpMapOffset), color, ZOOM_PERCENT);
}

// Vertices are in this order:
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
	if (pTile->pDef->hTex.pObj)
		g_Renderer.AddSpriteToRenderList((GameTexturePortion*)pTile->pDef->hTex.pObj, tilePt, color, scale);
	if (pTile->pBuilding)
		g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pTile->pBuilding->pDef->hTex), tilePt, color, scale);
	if (pTile->pUnit)
		g_Renderer.AddSpriteToRenderList(GET_REF(GameTexturePortion, pTile->pUnit->GetDef()->hTex), tilePt, color, scale);
}

#define OCEAN_FLOOR_VERTEX_DEPTH 2.0f
#define OCEAN_SURFACE_VERTEX_DEPTH 0.5f

#define HEIGHT_MAP_RESOLUTION 128

void CHexMap::GenerateHeightmapForTile(FlexScratchSurface* pOut, hexTile* pTile, hexTile* neighbors[6])
{
	static GameTexture* pCoastline = NULL;
	static bool bInitialized = false;

	//these verts are arranged in the same order as the HexDirection enum
	//within a set of 3 verts, vert 0 will always be the one closer to the "previous" direction, vert 1 will be the "next" direction, and vert 2 is the center point.
	static FlexVertex2D vertsByDirection[6][3] = {
		
		{{0, HEX_HALF_HEIGHT*3/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{0, HEX_HALF_HEIGHT/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HALF_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f}}, 
		
		{{0, HEX_HALF_HEIGHT/2, 0.0f, 1.0f, 0xFFFFFFFF,	1.0f, 1.0f},
		{HEX_HALF_WIDTH, 0, 0.0f, 1.0f, 0xFFFFFFFF,	1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HALF_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f}},
		
		{{HEX_HALF_WIDTH, 0, 0.0f, 1.0f, 0xFFFFFFFF,	1.0f, 1.0f},
		{HEX_WIDTH, HEX_HALF_HEIGHT/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HALF_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f}},
		
		{{HEX_WIDTH, HEX_HALF_HEIGHT/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_WIDTH, HEX_HALF_HEIGHT*3/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HALF_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f}},

		{{HEX_WIDTH, HEX_HALF_HEIGHT*3/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HALF_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f}},

		{{HEX_HALF_WIDTH, HEX_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{0, HEX_HALF_HEIGHT*3/2, 0.0f, 1.0f, 0xFFFFFFFF,1.0f, 1.0f},
		{HEX_HALF_WIDTH, HEX_HALF_HEIGHT, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f}}


	};
	FlexVertex2D vertsToDraw[18] = {0};
	FlexVertex2D* pVertIter = vertsToDraw;
	if (!bInitialized)
	{
		pCoastline = GET_TEXTURE(L"Coastline");
		//too lazy to inline this math in the above declaration, sue me
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				vertsByDirection[i][j].x *= HEIGHT_MAP_RESOLUTION/HEX_HEIGHT;
				vertsByDirection[i][j].y *= HEIGHT_MAP_RESOLUTION/HEX_HEIGHT;

				/*
				ugly, magic adjustments to minimize seams until I get a real fix.
				*/
				if (vertsByDirection[i][j].x < HEIGHT_MAP_RESOLUTION/2)
					vertsByDirection[i][j].x;
				else if (vertsByDirection[i][j].x > HEIGHT_MAP_RESOLUTION/2)
					vertsByDirection[i][j].x+=2;
				if (vertsByDirection[i][j].y < HEIGHT_MAP_RESOLUTION/2)
					vertsByDirection[i][j].y;
				else if (vertsByDirection[i][j].y > HEIGHT_MAP_RESOLUTION/2)
					vertsByDirection[i][j].y+=2;

				vertsByDirection[i][j].x -= 0.5f;
				vertsByDirection[i][j].y -= 0.5f;
			}
		}
		bInitialized = true;
	}
	for (int iDir = kHexDir_W; iDir < kHexDir_Count; iDir++)
	{
		if (!neighbors[iDir])
			continue;

		if (neighbors[iDir]->pDef->eFlags & kTileFlag_Water)
		{
			//this neighbor is water
			int iPrevClockwise = (iDir+5) % kHexDir_Count;
			int iNextClockwise = (iDir+1) % kHexDir_Count;

			(*pVertIter) = vertsByDirection[iDir][0];
			if (neighbors[iPrevClockwise] && !(neighbors[iPrevClockwise]->pDef->eFlags & kTileFlag_Water))
			{
				pVertIter->u = 1.0f;
				pVertIter->v = 1.0f;
			}
			else
			{
				pVertIter->u = 0.0f;
				pVertIter->v = 0.0f;
			}
			pVertIter++;

			(*pVertIter) = vertsByDirection[iDir][1];
			if (neighbors[iNextClockwise] && !(neighbors[iNextClockwise]->pDef->eFlags & kTileFlag_Water))
			{
				pVertIter->u = 1.0f;
				pVertIter->v = 1.0f;
			}
			else
			{
				pVertIter->u = 0.0f;
				pVertIter->v = 0.0f;
			}
			pVertIter++;

			(*pVertIter) = vertsByDirection[iDir][2];
			pVertIter->u = 0.0f;
			pVertIter->v = 0.0f;
			pVertIter++;
		}
		else
		{
			(*pVertIter++) = vertsByDirection[iDir][0];
			(*pVertIter++) = vertsByDirection[iDir][1];
			(*pVertIter++) = vertsByDirection[iDir][2];
		}
	}
	g_Renderer.RenderToScratchSurface(pOut, pCoastline, vertsToDraw, pVertIter-vertsToDraw);
}


static DWORD heightmapDataBuffer[HEIGHT_MAP_RESOLUTION*HEIGHT_MAP_RESOLUTION];

void CHexMap::ApplyHeightmapToTileVerts(int x, int y, int chunkX, int chunkY, FlexVertex* pStart, FlexVertex* pEnd)
{
	hexTile* pTile = GetTile(x, y);
	x -= chunkX*TERRAIN_CHUNK_WIDTH;
	y -= chunkY*TERRAIN_CHUNK_HEIGHT;
	for (FlexVertex* pIter = pStart; pIter < pEnd; pIter++)
	{
		float adjX = pIter->x - ((y & 1 ? HEX_HALF_WIDTH : 0) + x * HEX_WIDTH);
		float adjY = pIter->y - (y * HEX_HEIGHT*3/4);
		adjX *= HEIGHT_MAP_RESOLUTION/HEX_HEIGHT;
		adjY *= HEIGHT_MAP_RESOLUTION/HEX_HEIGHT;
		adjY = HEIGHT_MAP_RESOLUTION-adjY;
		int index = ((int)adjX) + ((int)adjY)*HEIGHT_MAP_RESOLUTION;
		if (heightmapDataBuffer[index] == 0)
			pIter->z = OCEAN_FLOOR_VERTEX_DEPTH;
		else
			pIter->z = OCEAN_FLOOR_VERTEX_DEPTH * (1.0f - (((float)(heightmapDataBuffer[index] & 0xffffff)) / 0xffffff));
	}
}

bool CHexMap::TileIsCoastline(int x, int y)
{
	for (int iDir = 0; iDir < kHexDir_Count; iDir++)
	{
		hexTile* pTile = GetTile(GetTileInDirection(x, y, iDir));
		if (pTile && !(pTile->pDef->eFlags & kTileFlag_Water))
			return true;
	}
	return false;
}

static void CalcTileUVsFromGameTexturePortion(const GameTexturePortion* pTileTex, float* fMinU, float* fMaxU, float* fMinV, float* fMaxV)
{
	const GameTexture* pSrcTex = pTileTex->hTex.pTex;
	*fMinU = (pTileTex->rSrc->left+1.0f)/pSrcTex->width;
	*fMaxU = (pTileTex->rSrc->right-1.0f)/pSrcTex->width;
	*fMinV = (pTileTex->rSrc->top+1.0f)/pSrcTex->height;
	*fMaxV = (pTileTex->rSrc->bottom-1.0f)/pSrcTex->height;
}

void CHexMap::WriteHexSliceVertsToBuffer(FlexVertex* pVA, FlexVertex* pVB, FlexVertex* pVCenter, float fBlendA, float fBlendB, float fBlendC, int iTessellationDegree, FlexVertex** ppVertIter)
{
	g_Renderer.TessellateTriangleIntoBuffer(pVA, pVB, pVCenter, fBlendA, fBlendB, fBlendC, iTessellationDegree, ppVertIter);
}

void CHexMap::PostProcessTileVerts(GameTexturePortion* pTex, GameTexturePortion* pBlendTex, float fZ, float fAlpha, FlexVertex* pStart, FlexVertex* pEnd)
{
	float fMinU;
	float fMaxU;
	float fMinV;
	float fMaxV;
	float fMinU1;
	float fMaxU1;
	float fMinV1;
	float fMaxV1;
	
	CalcTileUVsFromGameTexturePortion(pTex, &fMinU, &fMaxU, &fMinV, &fMaxV);
	CalcTileUVsFromGameTexturePortion(pBlendTex, &fMinU1, &fMaxU1, &fMinV1, &fMaxV1);
		
	for (FlexVertex* pIter = pStart; pIter < pEnd; pIter++)
	{
		pIter->z = fZ;

		pIter->u *= fMaxU-fMinU;
		pIter->u += fMinU;
		pIter->v *= fMaxV-fMinV;
		pIter->v += fMinV;
		
		pIter->u1 *= fMaxU1-fMinU1;
		pIter->u1 += fMinU1;
		pIter->v1 *= fMaxV1-fMinV1;
		pIter->v1 += fMinV1;

		pIter->diffuse &= 0xffffff;
		pIter->diffuse |= ((int)(fAlpha * 255) << 24);
	}
}

void CHexMap::WriteSingleHexVertsToBuffers(int tileX, int tileY, int chunkX, int chunkY, FlexVertex** ppVertIter, FlexVertex** ppTranslucentVertIter)
{
	//tileX and tileY are relative to current chunk 
	int iActualTileX = (chunkX*TERRAIN_CHUNK_WIDTH + tileX);//x value in map coordinates
	int iActualTileY = (chunkY*TERRAIN_CHUNK_HEIGHT + tileY);//y value in map coordinates
	FlexVertex hexVerts[] = {
		{HEX_HALF_WIDTH,	HEX_HEIGHT,				0.0f, 0xFFFFFFFF, 0.5f, 0, 0.5f, 0},//0
		{HEX_WIDTH,			HEX_HEIGHT*3.0f/4.0f,	0.0f, 0xFFFFFFFF, 1.0f, 0.25f, 1.0f, 0.25f},//1
		{0.0f,				HEX_HEIGHT*3.0f/4.0f,	0.0f, 0xFFFFFFFF, 0, 0.25f, 0, 0.25f},//2
		{HEX_HALF_WIDTH,	HEX_HALF_HEIGHT,		0.0f, 0xFFFFFFFF, 0.5f, 0.5f, 0.5f, 0.5f},//3
		{0.0f,				HEX_HALF_HEIGHT/2.0f,	0.0f, 0xFFFFFFFF, 0, 0.75f, 0, 0.75f},//4
		{HEX_WIDTH,			HEX_HALF_HEIGHT/2.0f,	0.0f, 0xFFFFFFFF, 1.0f, 0.75f, 1.0f, 0.75f},//5
		{HEX_HALF_WIDTH,	0.0f,					0.0f, 0xFFFFFFFF, 0.5f, 1.0f, 0.5f, 1.0f},//6
	};
/*
			0
		2		1
			3
		4		5
			6
*/

	hexTile* currTile = &pTiles[iActualTileX + iActualTileY*w];
	GameTexturePortion* pTileTex = GET_REF(GameTexturePortion, currTile->pDef->hTex);
	static GameTexturePortion* pSandTexture = GET_DEF_FROM_STRING(GameTexturePortion, L"terrain_desert");
	const int DirectionToVertexIndex[] = {4, 2, 0, 1, 5, 6, 4}; //clockwise order of verts to pull from for each direction
	bool bWater = !!(currTile->pDef->eFlags & kTileFlag_Water);
	bool bDeepWater = !!(currTile->pDef->eFlags & kTileFlag_DeepWater);
	
	hexTile* neighbors[6];
	static FlexScratchSurface heightmap(HEIGHT_MAP_RESOLUTION);

	//Generate heightmap if necessary
	if (bWater && !bDeepWater)
	{
		for (int iDir = 0; iDir < kHexDir_Count; iDir++)
		{
			neighbors[iDir] = GetTile(GetTileInDirection(iActualTileX, iActualTileY, iDir));
		}
		GenerateHeightmapForTile(&heightmap, currTile, neighbors);
		heightmap.GetData(heightmapDataBuffer);
	}

	//adjust for x/y coordinates
	for (int k = 0; k < 7; k++)
	{
		hexVerts[k].x += tileX*(HEX_WIDTH);
		if (tileY&1)
			hexVerts[k].x += HEX_HALF_WIDTH;

		hexVerts[k].y += tileY*(HEX_HEIGHT * (3.0f/4.0f));
	}

	//Write verts for each wedge
	FlexVertex* pHeightmapIter = (*ppVertIter);
	for (int eDir = 0; eDir < kHexDir_Count; eDir++)
	{
		//First verts for actual landmass
		hexTile* pNeighbor = GetTile(GetTileInDirection(iActualTileX, iActualTileY, eDir));
		GameTexturePortion* pBaseTex;
		GameTexturePortion* pBlendTex = pNeighbor ? GET_REF(GameTexturePortion, pNeighbor->pDef->hTex) : NULL;
		bool bCoastline = pNeighbor && (!(pNeighbor->pDef->eFlags & kTileFlag_Water) && bWater);
		
		if (pNeighbor && (pNeighbor->pDef->eFlags & kTileFlag_Water))
			pBlendTex = pSandTexture;
		
		if (bWater)
			pBaseTex = pSandTexture;
		else
			pBaseTex = pTileTex;

		FlexVertex* pHeightmapIter = (*ppVertIter);
		WriteHexSliceVertsToBuffer(&hexVerts[DirectionToVertexIndex[eDir]], &hexVerts[DirectionToVertexIndex[eDir+1]], &hexVerts[3], 0.5f, 0.5f, -0.5f, (bWater && !bDeepWater) ? TESSELLATION_DEGREE_COASTLINE : TESSELLATION_DEGREE_OTHER, ppVertIter);
		
		PostProcessTileVerts(pBaseTex, pBlendTex ? pBlendTex : pBaseTex, bDeepWater ? OCEAN_FLOOR_VERTEX_DEPTH : 0.0f, 1.0f, pHeightmapIter, *ppVertIter);

		//Heightmap the coastline if necessary
		if (bWater && !bDeepWater)
			ApplyHeightmapToTileVerts(iActualTileX, iActualTileY, chunkX, chunkY, pHeightmapIter, *ppVertIter);
		
		//Verts for water surface
		if (bWater)
		{
			if (pNeighbor->pDef->eFlags & kTileFlag_Water)
				pBlendTex = GET_REF(GameTexturePortion, pNeighbor->pDef->hTex);
			else
				pBlendTex = pTileTex;
		
			pBaseTex = pTileTex;
			pHeightmapIter = (*ppTranslucentVertIter);
			//need to write water's surface tris too
			WriteHexSliceVertsToBuffer(&hexVerts[DirectionToVertexIndex[eDir]], &hexVerts[DirectionToVertexIndex[eDir+1]], &hexVerts[3], 0.5f, 0.5f, -0.5f, TESSELLATION_DEGREE_OTHER, ppTranslucentVertIter);
		
			PostProcessTileVerts(pBaseTex, pBlendTex, OCEAN_SURFACE_VERTEX_DEPTH, 0.8f, pHeightmapIter, *ppTranslucentVertIter);
		}
		
	}
				
}

int CHexMap::GetNumTrisForTesselatedTile(int x, int y)
{
	hexTile* currTile = &pTiles[x + y*w];
	int iRet = 0;
	if ((currTile->pDef->eFlags & kTileFlag_Water) && TileIsCoastline(x, y))
	{
		iRet += g_Renderer.GetNumTessellatedTriangles(TESSELLATION_DEGREE_COASTLINE)*6;

	}
	else
		iRet += g_Renderer.GetNumTessellatedTriangles(TESSELLATION_DEGREE_OTHER)*6;

	return iRet;
}

int CHexMap::GetNumTranslucentTrisForTesselatedTile(int x, int y)
{
	hexTile* currTile = &pTiles[x + y*w];
	int iRet = 0;
	if ((currTile->pDef->eFlags & kTileFlag_Water))
	{
		iRet += 24;
	}
	return iRet;
}

TerrainVertexChunk* CHexMap::CreateTerrainVertexBufferChunk(int x, int y)
{
	TerrainVertexChunk* pNewBuffer = new TerrainVertexChunk;
	static GameTexturePortion* pSandTexture = GET_DEF_FROM_STRING(GameTexturePortion, L"terrain_desert");
	
/*
			0
		2		1
			3
		4		5
			6
*/
	//red channel used to denote % blend of two textures

	int iNumTris = 0;
	int iNumTranslucentTris = 0;
	
	//count tris needed
	for (int i = 0; i < TERRAIN_CHUNK_WIDTH; i++)
	{
		for (int j = 0; j < TERRAIN_CHUNK_HEIGHT; j++)
		{
			int iActualTileX = (x*TERRAIN_CHUNK_WIDTH + i);
			int iActualTileY = (y*TERRAIN_CHUNK_HEIGHT + j);

			iNumTris += GetNumTrisForTesselatedTile(iActualTileX, iActualTileY);
			iNumTranslucentTris += GetNumTranslucentTrisForTesselatedTile(iActualTileX, iActualTileY);
		}
	}

	g_Renderer.CreateVertexBuffer(sizeof(FlexVertex)*iNumTris*3, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &pNewBuffer->pVerts, NULL);
	g_Renderer.CreateVertexBuffer(sizeof(FlexVertex)*iNumTranslucentTris*3, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &pNewBuffer->pTranslucentVerts, NULL);
	pNewBuffer->iTris = iNumTris;
	pNewBuffer->iTranslucentTris = iNumTranslucentTris;

	void* vb_vertices;
	pNewBuffer->pVerts->Lock(0, 0, &vb_vertices, 0);
	FlexVertex* pIter = (FlexVertex*)vb_vertices;

	void* vb_vertices_translucent;
	pNewBuffer->pTranslucentVerts->Lock(0, 0, &vb_vertices_translucent, 0);
	FlexVertex* pTranslucentIter = (FlexVertex*)vb_vertices_translucent;


	for (int i = 0; i < TERRAIN_CHUNK_WIDTH; i++)
	{
		for (int j = 0; j < TERRAIN_CHUNK_HEIGHT; j++)
		{
			WriteSingleHexVertsToBuffers(i, j, x, y, &pIter, &pTranslucentIter);
			/*
			currTile = &pTiles[iActualTileX + iActualTileY*w];

			const GameTexturePortion* pTileTex = GET_REF(GameTexturePortion, currTile->pDef->hTex);
			float fMinU;
			float fMaxU;
			float fMinV;
			float fMaxV;

			if ((currTile->pDef->eFlags & kTileFlag_Water))
				CalcTileUVsFromGameTexturePortion(pSandTexture, &fMinU, &fMaxU, &fMinV, &fMaxV);
			else
			{
				CalcTileUVsFromGameTexturePortion(pTileTex, &fMinU, &fMaxU, &fMinV, &fMaxV);
			}

			float fZ = 0.0f;

			if (currTile->pDef->eFlags & kTileFlag_DeepWater)
				fZ = OCEAN_SURFACE_VERTEX_DEPTH;
			
			FlexVertex adjustedHexVerts[7];

			//prep verts for tessellation
			for (int k = 0; k < 7; k++)
			{
				float fMinU1;
				float fMaxU1;
				float fMinV1;
				float fMaxV1;
				adjustedHexVerts[k] = hexVerts[k];
				adjustedHexVerts[k].x += i*(HEX_WIDTH);
				if (j&1)
					adjustedHexVerts[k].x += HEX_HALF_WIDTH;

				adjustedHexVerts[k].y += j*(HEX_HEIGHT * (3.0f/4.0f));
				
				adjustedHexVerts[k].z = fZ;

				adjustedHexVerts[k].u *= fMaxU-fMinU;
				adjustedHexVerts[k].u += fMinU;

				adjustedHexVerts[k].v *= fMaxV-fMinV;
				adjustedHexVerts[k].v += fMinV;

				adjustedHexVerts[k].diffuse = 0xffffffff;
			}

			FlexVertex* pHeightmapIter = pIter;

			int tessellationDegree = TESSELLATION_DEGREE_OTHER;
			//tessellate
			if ((currTile->pDef->eFlags & kTileFlag_Water) && TileIsCoastline(iActualTileX, iActualTileY))
			{
				tessellationDegree = TESSELLATION_DEGREE_COASTLINE;
			}
			g_Renderer.TessellateTriangleIntoBuffer(&adjustedHexVerts[2], &adjustedHexVerts[0], &adjustedHexVerts[3], tessellationDegree, &pIter);
			g_Renderer.TessellateTriangleIntoBuffer(&adjustedHexVerts[0], &adjustedHexVerts[1], &adjustedHexVerts[3], tessellationDegree, &pIter);
			g_Renderer.TessellateTriangleIntoBuffer(&adjustedHexVerts[3], &adjustedHexVerts[1], &adjustedHexVerts[5], tessellationDegree, &pIter);
			g_Renderer.TessellateTriangleIntoBuffer(&adjustedHexVerts[3], &adjustedHexVerts[5], &adjustedHexVerts[6], tessellationDegree, &pIter);
			g_Renderer.TessellateTriangleIntoBuffer(&adjustedHexVerts[4], &adjustedHexVerts[3], &adjustedHexVerts[6], tessellationDegree, &pIter);
			g_Renderer.TessellateTriangleIntoBuffer(&adjustedHexVerts[2], &adjustedHexVerts[3], &adjustedHexVerts[4], tessellationDegree, &pIter);

			//apply heightmap data if necessary
			if (currTile->pDef->eFlags & kTileFlag_Water)
			{
				POINT pt = {iActualTileX, iActualTileY};
				ApplyHeightmapToTileVerts(pt, x, y, pHeightmapIter, pIter, (tessellationDegree == TESSELLATION_DEGREE_COASTLINE));
				
				CalcTileUVsFromGameTexturePortion(pTileTex, &fMinU, &fMaxU, &fMinV, &fMaxV);
			
				//prep verts for tessellation
				for (int k = 0; k < 7; k++)
				{
					adjustedHexVerts[k] = hexVerts[k];
					adjustedHexVerts[k].x += i*(HEX_WIDTH);
					if (j&1)
						adjustedHexVerts[k].x += HEX_HALF_WIDTH;

					adjustedHexVerts[k].y += j*(HEX_HEIGHT * (3.0f/4.0f));
				
					adjustedHexVerts[k].z = OCEAN_SURFACE_VERTEX_DEPTH;

					adjustedHexVerts[k].u *= fMaxU-fMinU;
					adjustedHexVerts[k].u += fMinU;

					adjustedHexVerts[k].v *= fMaxV-fMinV;
					adjustedHexVerts[k].v += fMinV;

					adjustedHexVerts[k].diffuse = 0xffffffff;
				}
				(*pIter++) = adjustedHexVerts[2];
				(*pIter++) = adjustedHexVerts[0];
				(*pIter++) = adjustedHexVerts[3];
				
				(*pIter++) = adjustedHexVerts[0];
				(*pIter++) = adjustedHexVerts[1];
				(*pIter++) = adjustedHexVerts[3];
				
				(*pIter++) = adjustedHexVerts[3];
				(*pIter++) = adjustedHexVerts[1];
				(*pIter++) = adjustedHexVerts[5];
				
				(*pIter++) = adjustedHexVerts[3];
				(*pIter++) = adjustedHexVerts[5];
				(*pIter++) = adjustedHexVerts[6];
				
				(*pIter++) = adjustedHexVerts[4];
				(*pIter++) = adjustedHexVerts[3];
				(*pIter++) = adjustedHexVerts[6];
				
				(*pIter++) = adjustedHexVerts[2];
				(*pIter++) = adjustedHexVerts[3];
				(*pIter++) = adjustedHexVerts[4];
			}

			//if this hex has water, draw another hex at water-level with a different texture
			*/
			
		}
	}

	pNewBuffer->pVerts->Unlock();
	pNewBuffer->pTranslucentVerts->Unlock();

	return pNewBuffer;
}

void CHexMap::CreateAllTerrainVertexBuffers()
{
	//create enough buffers of the specified size to hold the entire map
	iNumChunksWide = (int)ceil(((float)w)/TERRAIN_CHUNK_WIDTH);
	iNumChunksHigh = (int)ceil(((float)h)/TERRAIN_CHUNK_HEIGHT);
	ppVertChunks = new TerrainVertexChunk*[iNumChunksWide*iNumChunksHigh];
	for (int iChunk = 0; iChunk < iNumChunksWide; iChunk++)
	{
		for (int jChunk = 0; jChunk < iNumChunksHigh; jChunk++)
		{
			ppVertChunks[iChunk + jChunk*iNumChunksWide] = CreateTerrainVertexBufferChunk(iChunk, jChunk);
		}
	}
	pIndexBuffer = NULL;
}

void CHexMap::GetTilespaceCullRect(RECT* pOut)
{
	D3DXVECTOR3 cameraCullPoints[4];
	D3DXVECTOR3 planePoint(0,0,0);
	D3DXVECTOR3 planeNormal(0,0,-1);

	float chunkRect[4] = {FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN};//minx miny maxx maxy
	g_Renderer.GetCamera()->CameraFrustumPlaneIntersection(cameraCullPoints, &planePoint, &planeNormal); //topleft topright bottomleft bottomright

	for (int i = 0; i < 4; i++)
	{
		if (cameraCullPoints[i].x < chunkRect[0])
			chunkRect[0] = cameraCullPoints[i].x;
		if (cameraCullPoints[i].x > chunkRect[2])
			chunkRect[2] = cameraCullPoints[i].x;

		if (cameraCullPoints[i].y < chunkRect[1])
			chunkRect[1] = cameraCullPoints[i].y;
		if (cameraCullPoints[i].y > chunkRect[3])
			chunkRect[3] = cameraCullPoints[i].y;
	}

	//adjust edges to account for hex staggering
	chunkRect[0] -= HEX_WIDTH/2;
	chunkRect[1] -= HEX_HEIGHT/4;


	//calculate tile coordinates
	chunkRect[0] /= (HEX_WIDTH);
	chunkRect[1] /= (HEX_HEIGHT*3.0f/4.0f);
	chunkRect[2] /= (HEX_WIDTH);
	chunkRect[3] /= (HEX_HEIGHT*3.0f/4.0f);

	SetRect(pOut, (LONG)chunkRect[0], (LONG)chunkRect[1], (LONG)chunkRect[2], (LONG)chunkRect[3]);
}

void CHexMap::GetWorldspaceCullTrapezoid(D3DXVECTOR3 pointsOut[4])
{
	D3DXVECTOR3 cameraCullPoints[4];
	D3DXVECTOR3 planePoint(0,0,0);
	D3DXVECTOR3 planeNormal(0,0,-1);

	g_Renderer.GetCamera()->CameraFrustumPlaneIntersection(pointsOut, &planePoint, &planeNormal); //topleft topright bottomleft bottomright

}

void CHexMap::GetChunkspaceCullRect(RECT* pOut)
{
	D3DXVECTOR3 cameraCullPoints[4];
	D3DXVECTOR3 planePoint(0,0,0);
	D3DXVECTOR3 planeNormal(0,0,-1);

	float chunkRect[4] = {FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN};//minx miny maxx maxy
	g_Renderer.GetCamera()->CameraFrustumPlaneIntersection(cameraCullPoints, &planePoint, &planeNormal); //topleft topright bottomleft bottomright

	for (int i = 0; i < 4; i++)
	{
		if (cameraCullPoints[i].x < chunkRect[0])
			chunkRect[0] = cameraCullPoints[i].x;
		if (cameraCullPoints[i].x > chunkRect[2])
			chunkRect[2] = cameraCullPoints[i].x;

		if (cameraCullPoints[i].y < chunkRect[1])
			chunkRect[1] = cameraCullPoints[i].y;
		if (cameraCullPoints[i].y > chunkRect[3])
			chunkRect[3] = cameraCullPoints[i].y;
	}

	//adjust edges to account for hex staggering
	chunkRect[0] -= HEX_WIDTH/2;
	chunkRect[1] -= HEX_HEIGHT/4;


	//calculate tile coordinates
	chunkRect[0] /= (TERRAIN_CHUNK_WIDTH*HEX_WIDTH);
	chunkRect[1] /= (TERRAIN_CHUNK_HEIGHT*HEX_HEIGHT*3.0f/4.0f);
	chunkRect[2] /= (TERRAIN_CHUNK_WIDTH*HEX_WIDTH);
	chunkRect[3] /= (TERRAIN_CHUNK_HEIGHT*HEX_HEIGHT*3.0f/4.0f);

	SetRect(pOut, (LONG)floor(chunkRect[0]), (LONG)floor(chunkRect[1]), (LONG)ceil(chunkRect[2]), (LONG)ceil(chunkRect[3]));
}

void CHexMap::RenderTerrain()
{
	float pos[3] = {0.0f, 0.0f, 0.0f};
	float scl[3] = {1.0f, 1.0f, 1.0f};
	float rot[3] = {0.0f, 0.0f, 0.0f};
	RECT chunks;
	GetChunkspaceCullRect(&chunks);
	const GameTexture* pTerrainTex = GET_TEXTURE(L"terrain");

	for (int i = chunks.left; i < chunks.right; i++)
	{
		if ((i < 0 || i >= iNumChunksWide) && g_DebugFlags.disableMapXWrap)
				continue;
		int iNormalized = i;
		while (iNormalized < 0)
			iNormalized += iNumChunksWide;
		while (iNormalized >= iNumChunksWide)
			iNormalized -= iNumChunksWide;
		for (int j = chunks.top; j < chunks.bottom; j++)
		{
			if (j < 0 || j >= iNumChunksHigh)
				continue;
			pos[0] = ((int)i) * TERRAIN_CHUNK_WIDTH*HEX_WIDTH;
			pos[1] = ((int)j) * TERRAIN_CHUNK_HEIGHT*HEX_HEIGHT*3.0f/4.0f;
			TerrainVertexChunk* pChunk = ppVertChunks[((int)iNormalized) + ((int)j) * iNumChunksWide];
			g_Renderer.AddModelToRenderList(&pChunk->pVerts, pIndexBuffer, &pChunk->iTris, NULL, pTerrainTex, pos, scl, rot, kShader3D_2xBlend);
			g_Renderer.AddModelToRenderList(&pChunk->pTranslucentVerts, pIndexBuffer, &pChunk->iTranslucentTris, NULL, pTerrainTex, pos, scl, rot, kShader3D_Translucent2xBlend);
		}
	}
}

void CHexMap::RenderFog(POINT tilePt)
{
}

//l/r wrap
inline DWORD CHexMap::GetMinimapColorAt(int x, int y)
{
	return pTiles[x + y*w].pDef->color;
}

void D3DLockedRectDrawLine(D3DLOCKED_RECT* pD3dlr, int w, int h, D3DXVECTOR3* pVecA, D3DXVECTOR3* pVecB, DWORD color)
{
	if (abs(pVecB->y-pVecA->y) > abs(pVecB->x-pVecA->x))
	{
		//y axis distance is greater
		if (pVecB->y < pVecA->y)
		{
			D3DXVECTOR3* temp = pVecB;
			pVecB = pVecA;
			pVecA = temp;
		}
		int vAX = (int)pVecA->x * 2;
		int vAY = (int)pVecA->y * 2;
		int vBX = (int)pVecB->x * 2;
		int vBY = (int)pVecB->y * 2;
		for (int i = vAY; i < vBY; i++)
		{
			int iLerpX = (vBX-vAX) * (i-vAY);
			iLerpX /= (vBY-vAY);
			iLerpX += vAX;
			int offset = (iLerpX + ((h - i - 1) * h));
			if (offset >= w*h)
				continue;
			DWORD* pPixel = (DWORD*)pD3dlr->pBits + offset;
			(*pPixel) = color;
		}
	}
	else
	{
		//x axis distance is greater
		if (pVecB->x < pVecA->x)
		{
			D3DXVECTOR3* temp = pVecB;
			pVecB = pVecA;
			pVecA = temp;
		}
		int vAX = (int)pVecA->x * 2;
		int vAY = (int)pVecA->y * 2;
		int vBX = (int)pVecB->x * 2;
		int vBY = (int)pVecB->y * 2;
		for (int i = vAX; i < vBX; i++)
		{
			int iLerpY = (vBY-vAY) * (i-vAX);
			iLerpY /= (vBX-vAX);
			iLerpY += vAY;
			int offset = (((h - iLerpY - 1) *  h) + i);
			if (offset >= w*h)
				continue;
			DWORD* pPixel = (DWORD*)pD3dlr->pBits + offset;
			(*pPixel) = color;
		}
	}
}

void CHexMap::UpdateMinimapTexture(GameTexture* pTex, RECT* view, FLOATPOINT fpMapOffset, playerVisibility* pVis)
{
	D3DXVECTOR3 trapezoid[4];
	D3DLOCKED_RECT d3dlr;
	pTex->pD3DTex->LockRect(0, &d3dlr, 0, 0); 
	char* pDst = (char*)d3dlr.pBits;
	DWORD* pPixel;
	DWORD color;
	for (int j = 0; j < h*2; j++)
	{
		pPixel = (DWORD*)pDst;
		pDst += d3dlr.Pitch;
		if (!((j/2) & 1))
			pPixel++;
		for (int i = 0; i < w; i++)
		{

			if (!pVis || pVis->GetTileVis(i, (h - (j/2+1))) == kVis_Clear)
			{
				color = GetMinimapColorAt(i, (h - (j/2+1)));
			}
			else if (pVis->GetTileVis(i, (h - (j/2+1))) == kVis_Fog)
			{
				color = GetMinimapColorAt(i, (h - (j/2+1)));
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
	
	GetWorldspaceCullTrapezoid(trapezoid);

	for (int i = 0; i < 4; i++)
	{
		trapezoid[i].x /= HEX_WIDTH;
		trapezoid[i].y /= (HEX_HEIGHT*3/4);
	}
	D3DLockedRectDrawLine(&d3dlr, 256, 256, &trapezoid[0], &trapezoid[1], 0xffff00ff);
	D3DLockedRectDrawLine(&d3dlr, 256, 256, &trapezoid[1], &trapezoid[3], 0xffff00ff);
	D3DLockedRectDrawLine(&d3dlr, 256, 256, &trapezoid[3], &trapezoid[2], 0xffff00ff);
	D3DLockedRectDrawLine(&d3dlr, 256, 256, &trapezoid[2], &trapezoid[0], 0xffff00ff);
	
	pTex->pD3DTex->UnlockRect(0);
}

inline POINT TileToScreen(int x, int y, FLOATPOINT fpOffset)
{
	POINT pt = {(LONG)(x*HEX_SIZE + (y%2 ? HEX_SIZE/2 : 0) - fpOffset.x), (LONG)(y * (HEX_SIZE*3/4 + 1) - fpOffset.y)};
	return pt;
}

void CHexMap::Render(RECT* view, FLOATPOINT fpMapOffset, CHexPlayer* pPlayer)
{
	RECT screenTiles;
	GetTilespaceCullRect(&screenTiles);
	playerVisibility* pVis = FOG_OF_WAR ? pPlayer->GetVisibility() : NULL;

	//for SOME FUCKING REASON putting this call after the below chunk ruins transparency

	for (int j = max(0,screenTiles.top); j < min(screenTiles.bottom,h); j++)
		for (int i = screenTiles.left; i < screenTiles.right; i++)
		{
			POINT validPt = {i, j};
			MakeLocValid(&validPt);
			if (!pVis || pVis->GetTileVis(validPt.x, validPt.y) != kVis_Shroud)
			{
				//if (pVis && pVis->GetTileVis(validPt.x, validPt.y) == kVis_Fog)
				//	RenderTile(screenPt, GetTile(i, j), 0xff333333, ZOOM_PERCENT);
				//else
				//	RenderTile(screenPt, GetTile(i, j), 0xFFFFFFFF, ZOOM_PERCENT);
			}
			
			hexTile* pTile = GetTile(validPt);

			if (pTile->pBuilding)
				g_GameState.RenderTileObject(i, j, GET_REF(GameTexturePortion, pTile->pBuilding->pDef->hTex), 0.0f, 3.0f);
			if (pTile->pUnit)
				g_GameState.RenderTileObject(i, j, GET_REF(GameTexturePortion, pTile->pUnit->GetDef()->hTex), 0.0f, 3.0f);

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
	RenderTerrain();
}


void CHexMap::GetMatchingOnscreenTiles(void*** peaTilesOut, findTileFunc pFunc, void* pData)
{
	RECT tilesToRender;
	eaClear(peaTilesOut);
	GetTilespaceCullRect(&tilesToRender);
	for (int i = tilesToRender.left; i < tilesToRender.right; i++)
		for (int j = max(0,tilesToRender.top); j < min(tilesToRender.bottom,h); j++)
		{
			hexTile* pTile = GetTile(i, j);
			pFunc(pTile, pData, peaTilesOut);
		}
}

void CHexMap::RenderInterface( RECT* mapViewport, FLOATPOINT fpMapOffset, POINT ptMouseoverTile )
{
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
	(*pPathOut)->ptOrigin = a;
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

	if (pCachedPath && pCachedPath->ptOrigin.x == a.x && pCachedPath->ptOrigin.y == a.y &&
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
			{
				return false;
			}
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
			//last path to that unit
			//move path at end of turn
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

bool CHexMap::IsUnitInTiles(CHexUnit* pUnit, POINT* pTilePts, int numTiles)
{
	int i;
	bool bFound = false;
	POINT currTilePt;
	hexTile* currTile;
	for (i = 0; i < numTiles; i++)
	{
		currTilePt = pTilePts[i];
		currTile = GetTile(currTilePt);
		if (currTile->pUnit == pUnit)
			bFound = true;
	}

	return bFound;
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