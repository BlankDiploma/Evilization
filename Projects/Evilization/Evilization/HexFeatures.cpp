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
		g_GameState.GetPlayerByID(ownerID)->DirtyCachedIncomeValues();
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
				g_GameState.GetPlayerByID(ownerID)->DirtyCachedIncomeValues();
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
	g_GameState.GetPlayerByID(ownerID)->DirtyCachedIncomeValues();
	return true;
}

void CHexBuilding::StartTurn(CHexPlayer* pOwner)
{

}

void CHexCity::StartTurn(CHexPlayer* pOwner)
{
	int matSum = 0, goldSum = 0, resSum = 0;
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
		}
	}

	pOwner->AddGold(goldSum);
	pOwner->AddResearch(resSum);
	materials += matSum;
	if (eaSize(&eaProjectQueue) > 0)
	{
		if (AdvanceProject(eaProjectQueue[0], GetNetProductionForProjects()))
		{
			g_GameState.GetPlayerByID(ownerID)->DirtyCachedIncomeValues();
			//finished building
			POINT pt = {-1,-1};
			pOwner->AddNotification(kNotify_Production, NULL, this, NULL, L"Production Complete");
			eaRemove(&eaProjectQueue, 0);
		}
	}
	materials = min(materials, materialsmax);
}

bool CHexCity::AddQueuedProject( projectType eType, void* pDef, POINT loc )
{
	cityProject* pProj = new cityProject;
	pProj->eType = eType;
	pProj->pDef = pDef;
	pProj->loc = loc;
	pProj->progress = 0;
	eaPush(&eaProjectQueue, pProj);

	return true;
}

bool CHexCity::ClearProductionQueue()
{
	for (int i = 0; i < eaSize(&eaProjectQueue); i++)
	{
		delete eaProjectQueue[i];
	}
	eaClear(&eaProjectQueue);

	return true;
}

cityProject** CHexCity::GetProductionQueue()
{
	return eaProjectQueue;
}

cityProject* CHexCity::GetCurrentProject()
{
	return eaSize(&eaProjectQueue) > 0 ? eaProjectQueue[0] : NULL;
}

int CHexCity::GetNetProductionForProjects()
{
	int matSum = 0, prodSum = 0;
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i]->pLaborOwner == this)
		{
			const laborSlotDef* pDef = eaPopulationUsage[i]->pDef;
			if (pDef->eType == kLabor_Harvest)
			{
				matSum += pDef->production;
			}
			else if (pDef->eType == kLabor_Construction)
			{
				prodSum += pDef->production;
			}
		}
	}
	return min(prodSum, materials+matSum);
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
		switch (curProject->eType)
		{
		case kProject_Unit:
			{
				g_GameState.GetCurrentMap()->CreateUnit((hexUnitDef*)curProject->pDef, g_GameState.GetPlayerByID(ownerID), GetLoc());
			}break;
		case kProject_Building:
			{
				g_GameState.GetCurrentMap()->CreateBuilding((hexBuildingDef*)curProject->pDef, g_GameState.GetPlayerByID(ownerID), curProject->loc);
				//			pMap->CreateBuilding((hexBuildingDef*)curProject->pDef, curProject->loc);
			}break;
		case kProject_Upgrade:
			{
				//			BuildUpgrade((hexBuildingDef*)curProject->pDef);
			}break;
		}
		curProject->progress -= cost;
		curProject->eType = kProject_None;
		curProject->pDef = NULL;
		curProject->loc.x = curProject->loc.y = -1;
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

CHexBuilding::CHexBuilding(hexBuildingDef* def, CHexPlayer* pOwner, bool bTakeOwnership)
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
	if (bTakeOwnership)
		pOwner->TakeOwnership(this);
}

CHexCity::CHexCity(hexBuildingDef* def, CHexPlayer* pOwner) : CHexBuilding(def, pOwner, false)
{
	pop = 1;
	eaPopulationUsage = NULL;
	eaAvailableProjects = NULL;
	eaProjectQueue = NULL;
	materials = 0;
	materialsmax = 50;
	name = _T("dongopolis");
	pOwner->TakeOwnership(this);
}

CHexCity::CHexCity()
{
	pop = 1;
	eaPopulationUsage = NULL;
	eaProjectQueue = NULL;
	materials = 0;
	materialsmax = 50;
	eaAvailableProjects = NULL;
	name = _T("");
}
laborSlot** CHexCity::GetLaborList()
{
	return eaLaborSlots;
}
int CHexCity::GetNetMaterials()
{
	int mat = 0;
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i]->pLaborOwner == this)
		{
			switch (eaPopulationUsage[i]->pDef->eType)
			{
			case kLabor_Harvest:
				{
					mat += eaPopulationUsage[i]->pDef->production;
				}break;
			case kLabor_Construction:
				{
					mat -= eaPopulationUsage[i]->pDef->production;
				}break;
			}
		}
	}
	return mat;
}
int CHexCity::GetGrossMaterials()
{
	int mat = 0;
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i]->pLaborOwner == this)
		{
			switch (eaPopulationUsage[i]->pDef->eType)
			{
			case kLabor_Harvest:
				{
					mat += eaPopulationUsage[i]->pDef->production;
				}break;
			}
		}
	}
	return mat;
}
int CHexCity::GetNetGold()
{
	int gold = 0;
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i]->pLaborOwner == this)
		{
			switch (eaPopulationUsage[i]->pDef->eType)
			{
			case kLabor_Harvest:
				{
					gold += eaPopulationUsage[i]->pDef->gold;
				}break;
			}
		}
	}
	return gold;
}
int CHexCity::GetNetResearch()
{
	int res = 0;
	for (int i = 0; i < eaSize(&eaPopulationUsage); i++)
	{
		if (eaPopulationUsage[i]->pLaborOwner == this)
		{
			switch (eaPopulationUsage[i]->pDef->eType)
			{
			case kLabor_Harvest:
				{
					res += eaPopulationUsage[i]->pDef->research;
				}break;
			}
		}
	}
	return res;
}
buildingType CHexCity::GetMyType()
{
	return kBuilding_City;
}
LPCTSTR CHexCity::GetName()
{
	return name;
}
float CHexCity::GetMaterialsPct()
{
	return ((float)materials)/materialsmax;
}
void CHexCity::GetStatByName(const TCHAR* pName, multiVal* pOut)
{
	if (_wcsicmp(pName, _T("name")) == 0)
		pOut->SetUnownedString(GetName());
	else if (_wcsicmp(pName, _T("materials")) == 0)
		pOut->SetInt(materials);
	else if (_wcsicmp(pName, _T("materialsmax")) == 0)
		pOut->SetInt(materialsmax);
	else if (_wcsicmp(pName, _T("grossmaterials")) == 0)
		pOut->SetInt(GetGrossMaterials());
	else if (_wcsicmp(pName, _T("netgold")) == 0)
		pOut->SetInt(GetNetGold());
	else if (_wcsicmp(pName, _T("netresearch")) == 0)
		pOut->SetInt(GetNetResearch());
	else if (_wcsicmp(pName, _T("materialsperturn")) == 0)
	{
		TCHAR buf[32];
		int net = GetNetMaterials();
		wsprintf(buf, _T("|c%s%i"), net > 0 ? _T("00ff00+") : _T("ff0000"), net);
		pOut->SetString(buf);
	}
}
void CHexCity::RefreshAvailableProjectList()
{
	int i = 0;
	CHexPlayer* pOwner = g_GameState.GetPlayerByID(ownerID);
	const StringIntHash* pPermissions = pOwner->GetBuildPermissions();
	StringIntHash::const_iterator iter = pPermissions->begin();
	while (iter != pPermissions->end())
	{
		hexBuildingDef* pBuilding = GET_DEF_FROM_STRING(hexBuildingDef, iter->first);
		hexUnitDef* pUnit = pBuilding ? NULL : GET_DEF_FROM_STRING(hexUnitDef, iter->first);
		if (pBuilding)
		{
			cityProject* pProj = NULL;
			if (i == eaSize(&eaAvailableProjects))
				eaPush(&eaAvailableProjects, new cityProject);
			pProj = eaAvailableProjects[i];
			pProj->eType = kProject_Building;
			pProj->pDef = pBuilding;
			pProj->loc.x = pProj->loc.y = -1;
			pProj->progress = 0;
			i++;
		}
		else if (pUnit)
		{
			cityProject* pProj = NULL;
			if (i == eaSize(&eaAvailableProjects))
				eaPush(&eaAvailableProjects, new cityProject);
			pProj = eaAvailableProjects[i];
			pProj->eType = kProject_Unit;
			pProj->pDef = pUnit;
			pProj->loc.x = pProj->loc.y = -1;
			pProj->progress = 0;
			i++;
		}

		iter++;
	}
	while (i < eaSize(&eaAvailableProjects))
	{
		delete eaAvailableProjects[eaSize(&eaAvailableProjects)-1];
		eaPop(&eaAvailableProjects)
	}
}
void CHexCity::GetAvailableProjectList(projectType eType, cityProject*** peaProjectsOut)
{
	RefreshAvailableProjectList();
	for (int i = 0; i < eaSize(&eaAvailableProjects); i++)
	{
		if (eType == kProject_None || eaAvailableProjects[i]->eType == eType)
			eaPush(peaProjectsOut, eaAvailableProjects[i]);
	}
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