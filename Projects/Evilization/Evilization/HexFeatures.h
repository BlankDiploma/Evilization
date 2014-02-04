#include "stdafx.h"
#include "structparse.h"
#include "earray.h"


#pragma once

class CHexCity;
class CHexPlayer;
struct hexTile;

AUTO_ENUM(buildingType) 
{
	kBuilding_None = 0, 
	kBuilding_City,
	kBuilding_Improvement
};


struct HEXPATH
{
	HEXPATH(int size)
	{
		pPoints = new POINT[size];
		this->size = size;
		start = 0;
	}
	HEXPATH(HEXPATH* src)
	{
		pPoints = new POINT[src->size];
		this->size = src->size;
		start = src->start;
		for (int i = 0; i < size; i++)
		{
			pPoints[i] = src->pPoints[i];
		}
	}
	~HEXPATH()
	{
		if (pPoints)
			delete [] pPoints;
	}
	POINT* pPoints;
	//pPoints may be larger than size;
	int size;
	int start;
};

PARSE_STRUCT(hexAbilityDef)
{
	//	dungeonScript* pScript;
	int cooldown;
	int range;
	int damage;
	int flags;
};

PARSE_STRUCT(hexUnitDef)
{
	int movement;
	int maxHealth;
	int TypeFlags;
	int numAbilities;
	int cost;
	int meleeStr;
	int visRadius;
	hexAbilityDef** pAbilities;
	DEF_REF(GameTexturePortion) hTex;
	const TCHAR* name;
	const TCHAR* displayName;
};


AUTO_ENUM(laborType) {kLabor_None = 0, kLabor_Harvest, kLabor_Construction, kLabor_Expression};

PARSE_STRUCT(laborSlotDef)
{
	const TCHAR* laborName;
	laborType eType;
	int production;
	int gold;
	int research;
};

struct laborSlot
{
	const laborSlotDef* pDef;
	CHexCity* pLaborOwner;
	POINT loc;
};

PARSE_STRUCT(hexBuildingDef)
{
	int cost;
	buildingType eType;
	laborSlotDef** eaSlotDefList;
	int maxHealth;
	int visRadius;
	const TCHAR* name;
	const TCHAR* displayName;
	
	DEF_REF(GameTexturePortion) hTex;
};

struct HEXPATH;


AUTO_ENUM(unitOrderType) {kOrder_Sleep = 0, kOrder_Move, kOrder_Melee, kOrder_AutoExplore};

struct hexUnitOrder
{
	unitOrderType eType;
	HEXPATH* pPath;
	hexUnitOrder()
	{
		pPath = NULL;
	}
	~hexUnitOrder()
	{
		if (pPath)
		{
			delete pPath;
			pPath = NULL;
		}
	}
};

class CHexUnit
{
private:
	hexUnitDef* pDef;
	int health;
	int ownerID;
	int movRemaining;
	int* abilityCooldowns;
	hexUnitOrder** eaOrders;
	POINT loc;
public:
	CHexUnit()
	{
		pDef = NULL;
		health = 100;
		ownerID = -1;
		movRemaining = 0;
		abilityCooldowns = NULL;
		eaOrders = NULL;
	}
	CHexUnit(hexUnitDef* def, CHexPlayer* pOwner);

	bool CanUseAbility(POINT target)
	{
		return false;
	}
	void OverwriteQueuedOrders(unitOrderType eType, HEXPATH* pPath)
	{
		for (int i = 0; i < eaSize(&eaOrders); i++)
		{
			delete eaOrders[i];
		}
		eaClear(&eaOrders);
		eaPush(&eaOrders, new hexUnitOrder);
		eaOrders[eaSize(&eaOrders)-1]->eType = eType;
		if (pPath)
			eaOrders[eaSize(&eaOrders)-1]->pPath = new HEXPATH(pPath);
	}
	void AddQueuedOrder(unitOrderType eType, HEXPATH* pPath)
	{
		eaPush(&eaOrders, new hexUnitOrder);
		eaOrders[eaSize(&eaOrders)-1]->eType = eType;
		if (pPath)
			eaOrders[eaSize(&eaOrders)-1]->pPath = new HEXPATH(pPath);

	}
	hexUnitOrder* GetTopQueuedOrder()
	{
		return eaSize(&eaOrders) > 0 ? eaOrders[0] : NULL;
	}
	hexUnitDef* GetDef()
	{
		return pDef;
	}
	int GetMovRemaining()
	{
		return movRemaining;
	}
	int GetVisRadius()
	{
		return pDef->visRadius;
	}
	void SetOwnerID(int ID)
	{
		ownerID = ID;
	}
	int GetOwnerID()
	{
		return ownerID;
	}
	void SetLoc(POINT pt)
	{
		loc = pt;
	}
	POINT GetLoc()
	{
		return loc;
	}
	void SpendMov( int delta );
	void StartTurn()
	{
		movRemaining = pDef->movement;
	}
	void PopQueuedOrder();

	void GetStatByName(const TCHAR* pName, multiVal* pOut)
	{
		if (_wcsicmp(pName, _T("name")) == 0)
			pOut->SetUnownedString(pDef->name);
		else if (_wcsicmp(pName, _T("mov")) == 0)
		{
			pOut->SetInt(movRemaining);
		}
		else if (_wcsicmp(pName, _T("maxmov")) == 0)
		{
			pOut->SetInt(pDef->movement);
		}		else if (_wcsicmp(pName, _T("str")) == 0)
		{
			pOut->SetInt(pDef->meleeStr);
		}
	}
};
class CHexBuilding
{
public:
	hexBuildingDef* pDef;
	int health;
	int ownerID;
	laborSlot** eaLaborSlots;
	POINT loc;
	CHexBuilding()
	{
		pDef = NULL;
		health = 100;
		ownerID = -1;
		eaLaborSlots = NULL;
	}
	CHexBuilding(hexBuildingDef* def, CHexPlayer* pOwner);
	CHexBuilding(hexBuildingDef* def);
	void SetOwnerID(int ID)
	{
		ownerID = ID;
	}
	void StartTurn(CHexPlayer* pOwner);
	virtual buildingType GetMyType()
	{
		return kBuilding_Improvement;
	}
	int GetVisRadius()
	{
		return pDef->visRadius;
	}
	void SetLoc(POINT pt)
	{
		loc = pt;
	}
	POINT GetLoc()
	{
		return loc;
	}
	int GetOwnerID()
	{
		return ownerID;
	}
};

enum projectType {kProject_None = 0, kProject_Building, kProject_Unit, kProject_Upgrade};

struct cityProject
{
	projectType eType;
	void* pDef;
	int progress;
};

class CHexCity : public CHexBuilding
{
private:
	int pop;
	int materials;
	int materialsmax;
	laborSlot** eaPopulationUsage;
	cityProject curProject;
	TCHAR* name;
public:
	CHexCity()
	{
		pop = 1;
		eaPopulationUsage = NULL;
		curProject.eType = kProject_None;
		curProject.progress = 0;
		curProject.pDef = NULL;
		materials = 0;
		materialsmax = 50;
		name = _T("");
	}
	CHexCity(hexBuildingDef* def, CHexPlayer* pOwner);
	int CountAvailableLabor();
	bool SetNextAvailableLabor(laborSlot* pSlot);
	bool ToggleLaborInSlot(laborSlot* pSlot);
	bool StartLaborInSlot(CHexBuilding* pBuilding, int idx);
	bool StopLaborInSlot(laborSlot* pSlot);
	bool AdjustPop(int delta);
	bool SwitchProject(projectType eType, void* pDef);
	void StartTurn(CHexPlayer* pOwner);
	bool AdvanceProject( cityProject* curProject, int prod );
	laborSlot** GetLaborList()
	{
		return eaLaborSlots;
	}
	int GetNetMaterials()
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
	int GetGrossMaterials()
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
	int GetNetGold()
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
	int GetNetResearch()
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
	buildingType GetMyType()
	{
		return kBuilding_City;
	}
	LPCTSTR GetName()
	{
		return name;
	}
	float GetMaterialsPct()
	{
		return ((float)materials)/materialsmax;
	}
	void GetStatByName(const TCHAR* pName, multiVal* pOut)
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
	bool StartLaborOnTile(hexTile* pTile);
};

struct StringTagContext;

void formatUnitTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);

void formatCityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);