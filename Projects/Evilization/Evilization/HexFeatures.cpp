#include "stdafx.h"
#include "HexFeatures.h"
#include "GameState.h"
#include "HexMap.h"
#include "StringTag.h"

int CHexCity::CountAvailableLabor()
{
	return pop - eaSize(&eaPopulationUsage);
}

bool CHexCity::SetNextAvailableLabor(laborSlot* pSlot)
{
	if (CountAvailableLabor() > 0)
	{
		eaPush(&eaPopulationUsage, pSlot);
		pSlot->pLaborOwner = this;
		return true;
	}
	return false;
}

bool CHexCity::StartLaborInSlot(CHexBuilding* pBuilding, int idx)
{
	/*
	int laborAvail = CountAvailableLabor();
	if (laborAvail > 0 && idx < pDef->pSlotList->size())
	{
		if (!pBuilding->pSlots[idx].pLaborOwner)
		{
			SetNextAvailableLabor(&pBuilding->pSlots[idx]);
			return true;
		}
	}
	*/
	return false;
}
bool CHexCity::StartLaborOnTile(hexTile* pTile)
{
	int laborAvail = CountAvailableLabor();
	if (laborAvail > 0)
	{
		if (!pTile->slot.pLaborOwner)
		{
			SetNextAvailableLabor(&pTile->slot);
			return true;
		}
	}
	return false;
}

bool CHexCity::StopLaborInSlot(laborSlot* pSlot)
{
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i] == pSlot && eaPopulationUsage[i]->pLaborOwner == this)
		{
			if (eaPopulationUsage[i]->pLaborOwner)
			{
				eaPopulationUsage[i]->pLaborOwner = NULL;
				eaRemove(&eaPopulationUsage, i);
				return true;
			}
		}
	}
	return false;
}

bool CHexCity::ToggleLaborInSlot(laborSlot* pSlot)
{
	int laborAvail = CountAvailableLabor();
	if (laborAvail > 0 && !pSlot->pLaborOwner)
	{
			SetNextAvailableLabor(pSlot);
			return true;
	}
	else if (pSlot->pLaborOwner == this)
	{
		StopLaborInSlot(pSlot);
	}
	return false;
}

bool CHexCity::AdjustPop(int delta)
{
	pop += delta;
	if (pop <= 0)
	{
		return false;
	}
	
	int iNumLabors = eaSize(&eaPopulationUsage);
	while (pop < iNumLabors)
	{
		eaPopulationUsage[iNumLabors-1]->pLaborOwner = NULL;
		eaRemove(&eaPopulationUsage, iNumLabors-1);
		iNumLabors--;
	}
	return true;
}

void CHexBuilding::StartTurn(CHexPlayer* pOwner)
{

}

void CHexCity::StartTurn(CHexPlayer* pOwner)
{
	int matSum = 0, goldSum = 0, resSum = 0, prodSum = 0;
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i]->pLaborOwner == this)
		{
			const laborSlotDef* pDef = eaPopulationUsage[i]->pDef;
			if (pDef->eType == kLabor_Harvest)
			{
				matSum += pDef->production;
				goldSum += pDef->gold;
				resSum += pDef->research;
			}
			else if (pDef->eType == kLabor_Construction)
			{
				prodSum += pDef->production;
			}
		}
	}

	pOwner->AddGold(goldSum);
	pOwner->AddResearch(resSum);
	materials += matSum;
	if (curProject.eType > kProject_None)
	{
		if (AdvanceProject(&curProject, prodSum))
		{
			//finished building
		}
	}
	materials = min(materials, materialsmax);
}

bool CHexCity::SwitchProject( projectType eType, void* pDef )
{
	if (curProject.pDef)
	{
		curProject.progress = 0;
	}
	curProject.eType = eType;
	curProject.pDef = pDef;

	return true;
}

bool CHexCity::AdvanceProject( cityProject* curProject, int prod)
{
	int cost = 0;
	int progress = min(prod, materials);
	
	curProject->progress += progress;
	materials -= progress;

	switch (curProject->eType)
	{
	case kProject_Unit:
		{
			cost = ((hexUnitDef*)curProject->pDef)->cost;
		}break;
	case kProject_Upgrade:
	case kProject_Building:
		{
			cost = ((hexBuildingDef*)curProject->pDef)->cost;
		}break;
	}
	if (curProject->progress >= cost)
	{
		curProject->progress -= cost;
		curProject->eType = kProject_None;
		curProject->pDef = NULL;
		switch (curProject->eType)
		{
		case kProject_Unit:
			{
				//			pMap->CreateUnit((hexUnitDef*)curProject->pDef, loc);
			}break;
		case kProject_Building:
			{
				//			pMap->CreateBuilding((hexBuildingDef*)curProject->pDef, curProject->loc);
			}break;
		case kProject_Upgrade:
			{
				//			BuildUpgrade((hexBuildingDef*)curProject->pDef);
			}break;
		}
		return true;
	}
	return false;
}

CHexUnit::CHexUnit(hexUnitDef* def, CHexPlayer* pOwner)
{
	pDef = def;
	ownerID = -1;
	health = def->maxHealth;
	movRemaining = def->movement;
	abilityCooldowns = new int[def->numAbilities];
	eaOrders = NULL;
	if (pOwner)
		pOwner->TakeOwnership(this);
}

void CHexUnit::SpendMov( int delta )
{
	movRemaining = max(movRemaining-delta, 0);
}

void CHexUnit::PopQueuedOrder()
{
	if (eaOrders)
	{
		delete eaOrders[eaSize(&eaOrders)-1];
		eaRemove(&eaOrders, eaSize(&eaOrders)-1);
	}
}

CHexBuilding::CHexBuilding(hexBuildingDef* def, CHexPlayer* pOwner)
{
	CHexBuilding::CHexBuilding(def);
	pOwner->TakeOwnership(this);
}
CHexBuilding::CHexBuilding(hexBuildingDef* def)
{
	pDef = def;
	health = def->maxHealth;
	eaLaborSlots = NULL;
	ownerID = -1;

	//iterate both lists
	for (int i = 0; i < eaSize(&def->eaSlotDefList); i++)
	{
		laborSlot* pNewSlot = new laborSlot();
		pNewSlot->pDef = def->eaSlotDefList[i];
		pNewSlot->pLaborOwner = NULL;
		pNewSlot->loc = this->GetLoc();
		eaPush(&eaLaborSlots, pNewSlot);
	}
}

CHexCity::CHexCity(hexBuildingDef* def, CHexPlayer* pOwner) : CHexBuilding(def)
{
	pop = 1;
	eaPopulationUsage = NULL;
	curProject.eType = kProject_None;
	curProject.progress = 0;
	curProject.pDef = NULL;
	materials = 0;
	materialsmax = 50;
	ownerID = -1;
	name = _T("dongopolis");
	pOwner->TakeOwnership(this);
}

void formatUnitTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext)
{
	multiVal val;
	pContext->pUnit->GetStatByName(tag, &val);

	switch (val.myType)
	{
	case MULTIVAL_INT:
		{
			wsprintf(pOut, _T("%i"), val.GetInt());
		}break;
	case MULTIVAL_FLOAT:
		{
			wsprintf(pOut, _T("%.3f"), val.GetFloat());
		}break;
	case MULTIVAL_STRING:
		{
			wsprintf(pOut, _T("%s"), val.GetString());
		}break;
	}
}

void formatCityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext)
{
	multiVal val;
	pContext->pCity->GetStatByName(tag, &val);
	
	switch (val.myType)
	{
	case MULTIVAL_INT:
		{
			wsprintf(pOut, _T("%i"), val.GetInt());
		}break;
	case MULTIVAL_FLOAT:
		{
			wsprintf(pOut, _T("%.3f"), val.GetFloat());
		}break;
	case MULTIVAL_STRING:
		{
			wsprintf(pOut, _T("%s"), val.GetString());
		}break;
	}
}

#include "Autogen\HexFeatures_h_ast.cpp"