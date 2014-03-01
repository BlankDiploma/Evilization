#include "stdafx.h"
#include "techtree.h"
#include "deflibrary.h"
#include "flexerrorwindow.h"

techTreeDef* g_pGoodTechTree;
techTreeDef* g_pEvilTechTree;

__forceinline bool NodeStartsNewEra(techTreeNodeDef* pNode)
{
	for (int i = 0; i < eaSize(&pNode->eaRequiredNames); i++)
	{
		techTreeNodeDef* pReq = GET_DEF_FROM_STRING(techTreeNodeDef, pNode->eaRequiredNames[i]);
		if (!pReq)
		{
			ErrorAutoStructf("Techtreenodedef %s refers to unknown requirement %s.", pNode, pNode->name, pNode->eaRequiredNames[i]);
			return false;
		}
		if (GET_REF(techEraDef, pReq->hEraDef)->index >= GET_REF(techEraDef, pNode->hEraDef)->index)
			return false;
	}
	return true;
}

#define GRID_HEIGHT 9

int GetBestYCoordForTech(techTreeNodeDef* pNode, techTreeNodeDef* ppGrids[128][GRID_HEIGHT], int x)
{
	int min = 99999;
	int best = 0;
	//try placing right next to a parent
	for (int i = 0; i < GRID_HEIGHT; i++)
	{
		if (ppGrids[x][i])
			continue;
		int distSqr = 0;
		bool bDisqualified = true;
		for (int j = 0; j < eaSize(&pNode->eaRequiredNames); j++)
		{
			techTreeNodeDef* pReq = GET_DEF_FROM_STRING(techTreeNodeDef, pNode->eaRequiredNames[j]);
			int dist = abs(pReq->layoutPt.y - i) + abs(pReq->layoutPt.x - x);
			distSqr += dist*dist;
			if (!ppGrids[x-1][i] || ppGrids[x-1][i] == pReq)
				bDisqualified = false;
		}

	//	avgDistFromParents /= pNode->numReqNodes;

		if (!bDisqualified && distSqr < min)
		{
			min = distSqr;
			best = i;
		}
	}
	return best;
}

int GetMinXCoordForTech(techTreeNodeDef* pNode)
{
	int minX = 0;
	//try placing right next to a parent
	for (int j = 0; j < eaSize(&pNode->eaRequiredNames); j++)
	{
		techTreeNodeDef* pReq = GET_DEF_FROM_STRING(techTreeNodeDef, pNode->eaRequiredNames[j]);
		 minX = max(minX, pReq->layoutPt.x + 1);
	}
	return minX;
}

bool NodeReadyForPlacement(techTreeNodeDef* pNode)
{
	for (int j = 0; j < eaSize(&pNode->eaRequiredNames); j++)
	{
		techTreeNodeDef* pReq = GET_DEF_FROM_STRING(techTreeNodeDef, pNode->eaRequiredNames[j]);
		if (!pReq)
		{
			ErrorAutoStructf("Techtreenodedef %s refers to unknown requirement %s.", pNode, pNode->name, pNode->eaRequiredNames[j]);
			return false;
		}
		if (pReq->layoutPt.x == -1)
			return false;
	}
	return true;
}

void AssignGridPositions(techTreeDef* pTree)
{
	int numEras = g_DefLibrary.GetNumDefs(L"techEraDef");
	techTreeNodeDef* pGrids[128][GRID_HEIGHT];
	techTreeNodeDef*** peaEraBuckets = new techTreeNodeDef**[numEras];
	int iCurGridX = 0;
	float fNumStartingTechs = 0;

	memset(pGrids, 0, sizeof(techTreeNodeDef*) * 128 * GRID_HEIGHT);
	memset(peaEraBuckets, 0, sizeof(techTreeNodeDef**) * numEras);

	for (int i = 0; i < eaSize(&pTree->eaNodes); i++)
	{
		techTreeNodeDef* pNode = pTree->eaNodes[i];
		techEraDef* pEra = GET_REF(techEraDef, pNode->hEraDef);
		techTreeNodeDef*** peaNodes = &(peaEraBuckets[pEra->index]);

		//insert all nodes starting-tier nodes at front of list
		if (NodeStartsNewEra(pNode))
		{
			if (pEra->index == 0)
				fNumStartingTechs++;

			eaInsert(peaNodes, pNode, 0);
		}
		else
		{
			eaPush(peaNodes, pNode);
		}

	}

	eaClear(&pTree->eaNodes);

	//here we have one bucket per era, with nodes that only require prev-era techs (or no techs) at the front of each bucket.
	for (int i = 0; i < numEras; i++)
	{
		float fCurGridY = (i == 0) ? (-GRID_HEIGHT/fNumStartingTechs)/2 : 0.0f;
		techEraDef* pDef = GET_REF(techEraDef, peaEraBuckets[i][0]->hEraDef);
		int iMaxEraX = 0;
		techTreeNodeDef*** peaNodes = &(peaEraBuckets[i]);

		pDef->iColStart = iCurGridX;

		if (!(*peaNodes))
			continue;

		for (int iNode = 0; iNode < eaSize(peaNodes); iNode++)
		{
			if (!NodeStartsNewEra((*peaNodes)[iNode]))
				break;
			if (i == 0)
				fCurGridY += GRID_HEIGHT/fNumStartingTechs;
			else
				fCurGridY = (float)GetBestYCoordForTech((*peaNodes)[iNode], pGrids, iCurGridX);
			pGrids[iCurGridX][(int)fCurGridY] = (*peaNodes)[iNode];
			(*peaNodes)[iNode]->layoutPt.x = iCurGridX;
			(*peaNodes)[iNode]->layoutPt.y = (int)fCurGridY;
			(*peaNodes)[iNode]->cost = (iCurGridX + 1) * 40 + i*40;
			eaPush(&pTree->eaNodes, (*peaNodes)[iNode]);
			eaRemove(peaNodes, iNode);
			iNode--;
		}
		iCurGridX++;

		int prevent_hang = 0;

		//place all middle nodes
		while (eaSize(peaNodes) > 0)
		{
			for (int iNode = 0; iNode < eaSize(peaNodes); iNode++)
			{
				prevent_hang++;
				//place everything that can go in this column
				if (NodeReadyForPlacement((*peaNodes)[iNode]))
				{
					iCurGridX = (int)GetMinXCoordForTech((*peaNodes)[iNode]);
					fCurGridY = (float)GetBestYCoordForTech((*peaNodes)[iNode], pGrids, iCurGridX);

					(*peaNodes)[iNode]->layoutPt.x = iCurGridX;
					(*peaNodes)[iNode]->layoutPt.y = (int)fCurGridY;
					(*peaNodes)[iNode]->cost = (iCurGridX + 1) * 40 + i*40;
					pGrids[iCurGridX][(int)fCurGridY] = (*peaNodes)[iNode];

					//stick this node back in the main list
					eaPush(&pTree->eaNodes, (*peaNodes)[iNode]);
					eaRemove(peaNodes, iNode);
					iNode--;
				}
				iMaxEraX = max(iMaxEraX, iCurGridX);
			}
			if (prevent_hang > 1000)
			{
				ErrorAutoStructf("Some nodes in tech tree %s could not be placed.", pTree, pTree->name);
				break;
			}
		}
		iCurGridX = iMaxEraX+1;
		pDef->iColEnd = iCurGridX;
	}
	for (int i = 0; i < numEras; i++)
		eaDestroy(&(peaEraBuckets[i]));
	delete [] peaEraBuckets;
}

#include "Autogen\techtree_h_ast.cpp"