#include "stdafx.h"
#include "mtrand.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "HexFeatures.h"
#include <queue>
#include "structparse.h"
#include "earray.h"
#include "flexrenderer.h"
#include "Evilization.h"

#pragma once

#define FOG_OF_WAR 0

extern MTRand randomFloats;

struct queuedAction;
class CHexPlayer;
struct playerVisibility;
struct laborSlotDef;
struct hexTileDef;

#define SQRT_2 (1.4142f)
#define SQRT_3 (1.7321f)

#define HEX_HALF_HEIGHT 2.0f
#define HEX_HALF_WIDTH ((SQRT_3/2.0f) * HEX_HALF_HEIGHT)
#define HEX_HEIGHT (HEX_HALF_HEIGHT*2)
#define HEX_WIDTH (HEX_HALF_WIDTH*2)
#define HEX_RING_THICKNESS (HEX_HALF_HEIGHT * 0.2f)
#define HEX_RING_INNER_HALF_WIDTH ((SQRT_3/2.0f) * (HEX_HALF_HEIGHT - HEX_RING_THICKNESS))
#define HEX_RING_INNER_HALF_HEIGHT (HEX_HALF_HEIGHT - HEX_RING_THICKNESS)

#define NUM_TILE_DEFS 12

AUTO_ENUM(HexDirection) {kHexDir_W = 0, kHexDir_NW,
					kHexDir_NE,  kHexDir_E,
					kHexDir_SE, kHexDir_SW, kHexDir_Count};

#define kUnitType_Melee 1
#define kUnitType_Ranged 2
#define kUnitType_Siege 4
#define kUnitType_Flying 8
#define kUnitType_Spellcaster 16
#define kUnitType_Stealth	32

typedef void (*findTileFunc)(hexTile*, void* pData, void*** peaOut);

AUTO_ENUM(hexTileFlags) 
{
	kTileFlag_Walkable = 1,
	kTileFlag_Water = 2,
	kTileFlag_DeepWater = 4,
	kTileFlag_Mountain = 8
};

PARSE_STRUCT(hexTileDef)
{
	int iMaxPillageTurns;
	const TCHAR* name;
	const TCHAR* displayName;
	FLAGS hexTileFlags eFlags;
	int iMoveCost;
	COLOR_ARGB color;
	laborSlotDef* slotDef;
	DEF_REF(GameTexturePortion) hTex;
};

PARSE_STRUCT(hexMapGenerationDesc)
{
	int w;
	int h;
	float fMountainThreshold;
	float fShallowWaterThreshold;
	float fOceanThreshold;
};
 
struct hexTile
{
	CHexUnit* pUnit;
	CHexBuilding* pBuilding;
	hexTileDef* pDef;
	int iPillageTurnsRemaining;
	laborSlot slot;
	hexTile() {}
};

struct PATHFINDPOINT
{
	long x, y;
	int priority;
	int unreachableDist;//distance from the closest reachable tile
};

struct tilePathfindCompare: binary_function <PATHFINDPOINT,PATHFINDPOINT,bool>
{
	bool operator() (const PATHFINDPOINT& x, const PATHFINDPOINT& y) const
	{
		return x.priority > y.priority;
	}
};

typedef bool (*finishedFunc)(POINT pt, void* pData);

struct TerrainVertexChunk
{
	IDirect3DVertexBuffer9* pVerts;
	int iTris;
};

class CHexMap
{
public:
	CHexMap() 
	{
		pTiles = NULL;
		pPathMap = NULL;
		pCachedPath = NULL;
	}
	~CHexMap()
	{
		if (pTiles)
			delete [] pTiles;
		if (pCachedPath)
			delete pCachedPath;
		for (int i = 0; i < iNumChunksWide*iNumChunksHigh; i++)
			ppVertChunks[i]->pVerts->Release();
	}
	int GetWidth()
	{
		return w;
	}
	int GetHeight()
	{
		return h;
	}
	void Render(RECT* view, FLOATPOINT fpMapOffset, CHexPlayer* pPlayer);
	void RenderInterface( RECT* mapViewport, FLOATPOINT fpMapOffset, POINT ptMouseoverTile );
	void EndTurn(int player, queuedAction* pActions);
	void Generate(hexMapGenerationDesc* pDesc, int seed);
	void UpdateMinimapTexture(GameTexture* pTex, RECT* view, FLOATPOINT fpMapOffset, playerVisibility* pVis);
	void GetTileDescription( hexTile* pTile, TCHAR* pchDescOut );
	void RenderTile(POINT tilePt, hexTile* pTile, DWORD color = 0xFFFFFFFF, float scale = 1.0);
	void RenderTerrain();
	inline hexTile* GetTile(int x, int y);
	inline hexTile* GetTile(POINT pt);
	POINT GetRandomStartingPos();
	void PlacePlayerStart(CHexPlayer* pPlayer);
	inline POINT GetTileInDirection(POINT pt, HexDirection eDir);
	inline POINT GetTileInDirection(POINT pt, int eDir);
	inline POINT GetTileInDirection(int x, int y, int eDir);
	inline POINT GetTileInDirection(PATHFINDPOINT* pt, int eDir);
	int HexPathfindShroud(CHexUnit* pUnit, playerVisibility* pVis, POINT a, HEXPATH** pPathOut);
	void RenderPath( RECT* view, FLOATPOINT fpMapOffset, CHexUnit* pUnit, HEXPATH* pPath, int alpha );
	bool ProcessOrder( CHexUnit* pUnit, hexUnitOrder* pOrder, CHexPlayer* pOwner);
	bool MoveUnit( CHexUnit* pUnit, POINT pt);
	int GetTilesInRadius( POINT pt, int rad, POINT* ptTilesOut );
	int HexPathfindTile(CHexUnit* pUnit, POINT a, POINT b, HEXPATH** pPathOut);
	void GetMatchingOnscreenTiles(void*** peaListOut, findTileFunc pFunc, void* pData);
	HEXPATH* pCachedPath;
	CHexBuilding* CreateBuilding(hexBuildingDef* pDef, CHexPlayer* pOwner, POINT loc);
	CHexUnit* CreateUnit(hexUnitDef* pDef, CHexPlayer* pOwner, POINT loc);
	void RenderBuildingOnTile(hexBuildingDef* pDef, POINT pt, DWORD color, FLOATPOINT fpMapOffset);
	bool BuildingCanBeBuiltOnTile(hexBuildingDef* pDef, POINT tilePt);
	void GetChunkspaceCullRect(RECT* pOut);
	void GetTilespaceCullRect(RECT* pOut);
	void GetWorldspaceCullTrapezoid(D3DXVECTOR3 pointsOut[4]);
	bool IsUnitInTiles(CHexUnit* pUnit, POINT* pTilePts, int numTiles); 

private:

	TerrainVertexChunk** ppVertChunks;
	IDirect3DIndexBuffer9* pIndexBuffer;
	int numTris;

	POINT screenOffset;
	hexTile* pTiles;
	GameTexture* pFont;

	GameTexturePortion* pStickFigure;
	GameTexturePortion* pHouse;
	GameTexturePortion* pSelectedTile;
	GameTexturePortion* ppath;
	GameTexturePortion* ppathblip;
	GameTexturePortion* ppathend;

	int* pPathMap;
	int w;
	int h;
	int iNumChunksWide;
	int iNumChunksHigh;

	void CreateAllTerrainVertexBuffers();
	TerrainVertexChunk* CreateTerrainVertexBufferChunk(int x, int y);
	IDirect3DIndexBuffer9* CreateTerrainIndexBufferChunk();
	IDirect3DVertexBuffer9* CreateSplatVertexBuffer(int x, int y);
	void IndexToPixel(int index);
	void XYToIndex(int x, int y);
	void PathBetweenTiles(int a, int b);
	void GetAdjacentTiles(int index, hexTile* pAdj);
	inline DWORD GetMinimapColorAt(int x, int y);
	bool PointIsValidStartPos(POINT pt);
	inline bool MakeLocValid(POINT* pt);
	void RenderCityLabel( POINT pt, CHexCity* pCity );
	void RenderFog(POINT tilePt);
	int HexPathfindInternal(POINT a, CHexUnit* pUnit, finishedFunc pfFinished, void* pData, HEXPATH** pPathOut, int maxUnreachableDist);
	void ApplyHeightmapToTileVerts(POINT tilePt, int chunkX, int chunkY, FlexVertex* pStart, FlexVertex* pEnd, bool bIsCoastline);
	void GenerateHeightmapForTile(FlexScratchSurface* pOut, hexTile* pTile, hexTile* neighbors[6]);
	bool CHexMap::TileIsCoastline(int x, int y);
};
void InitializeBiomeMap(LPCTSTR  filename);

inline hexTile* CHexMap::GetTile(int x, int y)
{
	while (x < 0) x+= w;
	if (y < 0) y = 0;
	while (x >= w) x-= w;
	if (y >= h) y = h;
	return &pTiles[x + y*w];
}

inline hexTile* CHexMap::GetTile(POINT pt)
{
	while (pt.x < 0) pt.x+= w;
	if (pt.y < 0) pt.y = 0;
	while (pt.x >= w) pt.x-= w;
	if (pt.y >= h) pt.y = h-1;
	return &pTiles[pt.x + pt.y*w];
}

inline POINT CHexMap::GetTileInDirection(POINT pt, HexDirection eDir)
{
	switch (eDir)
	{
	case kHexDir_W:
		{
			pt.x--;
		}break;
	case kHexDir_NW:
		{
			pt.y--;
			if (pt.y % 2)
				pt.x--;
		}break;
	case kHexDir_NE:
		{
			pt.y--;
			if (!(pt.y % 2))
				pt.x++;
		}break;
	case kHexDir_E:
		{
			pt.x++;
		}break;
	case kHexDir_SE:
		{
			pt.y++;
			if (!(pt.y % 2))
				pt.x++;
		}break;
	case kHexDir_SW:
		{
			pt.y++;
			if (pt.y % 2)
				pt.x--;
		}break;
	}

	return pt;
}

inline POINT CHexMap::GetTileInDirection(POINT pt, int eDir)
{
	switch ((HexDirection)eDir)
	{
	case kHexDir_W:
		{
			pt.x--;
		}break;
	case kHexDir_NW:
		{
			pt.y--;
			if (pt.y % 2)
				pt.x--;
		}break;
	case kHexDir_NE:
		{
			pt.y--;
			if (!(pt.y % 2))
				pt.x++;
		}break;
	case kHexDir_E:
		{
			pt.x++;
		}break;
	case kHexDir_SE:
		{
			pt.y++;
			if (!(pt.y % 2))
				pt.x++;
		}break;
	case kHexDir_SW:
		{
			pt.y++;
			if (pt.y % 2)
				pt.x--;
		}break;
	}

	return pt;
}

inline POINT CHexMap::GetTileInDirection(int x, int y, int eDir)
{
	switch ((HexDirection)eDir)
	{
	case kHexDir_W:
		{
			x--;
		}break;
	case kHexDir_NW:
		{
			y--;
			if (y % 2)
				x--;
		}break;
	case kHexDir_NE:
		{
			y--;
			if (!(y % 2))
				x++;
		}break;
	case kHexDir_E:
		{
			x++;
		}break;
	case kHexDir_SE:
		{
			y++;
			if (!(y % 2))
				x++;
		}break;
	case kHexDir_SW:
		{
			y++;
			if (y % 2)
				x--;
		}break;
	}
	POINT pt = {x,y};
	return pt;
}

inline POINT CHexMap::GetTileInDirection(PATHFINDPOINT* pt, int eDir)
{
	POINT retpt;
	retpt.x = pt->x;
	retpt.y = pt->y;
	switch ((HexDirection)eDir)
	{
	case kHexDir_W:
		{
			retpt.x--;
		}break;
	case kHexDir_NW:
		{
			retpt.y--;
			if (retpt.y % 2)
				retpt.x--;
		}break;
	case kHexDir_NE:
		{
			retpt.y--;
			if (!(retpt.y % 2))
				retpt.x++;
		}break;
	case kHexDir_E:
		{
			retpt.x++;
		}break;
	case kHexDir_SE:
		{
			retpt.y++;
			if (!(retpt.y % 2))
				retpt.x++;
		}break;
	case kHexDir_SW:
		{
			retpt.y++;
			if (retpt.y % 2)
				retpt.x--;
		}break;
	}

	return retpt;
}

inline POINT TileToScreen(int x, int y, FLOATPOINT fpOffset);