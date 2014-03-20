#include "stdafx.h"
#include "structparse.h"
#include "earray.h"
#include "Abilities.h"

#pragma once

class CHexCity;
class CHexPlayer;
struct hexTile;

AUTO_ENUM(buildingType) 
{
	kBuilding_None = 0, 
	kBuilding_City,
	kBuilding_Improvement,
	kBuilding_Upgrade
};

AUTO_ENUM(buildingFlags) 
{
	kBuildingFlag_NoEnemyMovement = 1, 
	kBuildingFlag_NoAllyMovement = 2, 
	kBuildingFlag_BuildOnOcean = 4, 
	kBuildingFlag_BuildOnLand = 8, 
	kBuildingFlag_BuildOnMountain = 16, 
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
	POINT ptOrigin;
};

PARSE_STRUCT(hexUnitDef)
{
	int movement;
	int maxHealth;
	int defense;
	int TypeFlags;
	int numAbilities;
	int cost;
	int meleeStr;
	int attackRange;
	int visRadius;
	int maxMana;
	UnitAbilityRef** eaAbilityRefs;
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
	const TCHAR* filename;
	const TCHAR* displayName;
	FLAGS buildingFlags eFlags;
	
	DEF_REF(GameTexturePortion) hTex;
	//RequiredResource
};

struct HEXPATH;


AUTO_ENUM(unitOrderType) {kOrder_Sleep = 0, kOrder_Move, kOrder_Melee, kOrder_AutoExplore, kOrder_Ability};
AUTO_ENUM(UnitAttribute) {kUnitAttribute_MaxHealth = 0, kUnitAttribute_MaxMana, kUnitAttribute_MaxMovement, kUnitAttribute_MeleeStr, 
						  kUnitAttribute_Defense, kUnitAttribute_Range, kUnitAttribute_VisRadius, kUnitAttribute_NumAttributes};

struct UnitAttributes
{
	float stats[kUnitAttribute_NumAttributes];
};

struct hexUnitOrder
{
	unitOrderType eType;
	HEXPATH* pPath;
	UnitAbility* pAbility;
	POINT targetPt;
	hexUnitOrder()
	{
		pPath = NULL;
		pAbility = NULL;
	}
	~hexUnitOrder()
	{
		if (pPath)
		{
			delete pPath;
			pPath = NULL;
			delete pAbility;
			pAbility = NULL;
		}
	}
};

class CHexUnit
{
private:
	hexUnitDef* pDef;
	int health;
	int mana;
	int ownerID;
	int movRemaining;
	bool bIsDead;
	hexUnitOrder** eaOrders;
	POINT loc;
	UnitAbility** eaAbilities;
	UnitAttributes defaultAttributes;
	UnitAttributes currentAttributes;
	UnitAttributeModifier** eaAttributeMods;
public:
	CHexUnit()
	{
		pDef = NULL;
		health = 100;
		mana = 0;
		ownerID = -1;
		movRemaining = 0;
		eaAbilities = NULL;
		eaOrders = NULL;
		bIsDead = false;
		eaAttributeMods = NULL;
	}
	CHexUnit(hexUnitDef* def, CHexPlayer* pOwner);

	bool CanUseAbility(POINT target)
	{
		return false;
	}
	void OverwriteQueuedOrders(unitOrderType eType, HEXPATH* pPath, POINT targetPt, UnitAbility* pAbility)
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
		if (pAbility)
			eaOrders[eaSize(&eaOrders)-1]->pAbility = new UnitAbility(pAbility);
		eaOrders[eaSize(&eaOrders)-1]->targetPt = targetPt;
	}
	void AddQueuedOrder(unitOrderType eType, HEXPATH* pPath, POINT targetPt, UnitAbility* pAbility)
	{
		eaPush(&eaOrders, new hexUnitOrder);
		eaOrders[eaSize(&eaOrders)-1]->eType = eType;
		if (pPath)
			eaOrders[eaSize(&eaOrders)-1]->pPath = new HEXPATH(pPath);
		if (pAbility)
			eaOrders[eaSize(&eaOrders)-1]->pAbility = new UnitAbility(pAbility);
		eaOrders[eaSize(&eaOrders)-1]->targetPt = targetPt;

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
		}
		else if (_wcsicmp(pName, _T("str")) == 0)
		{
			pOut->SetInt(pDef->meleeStr);
		}
		else if (_wcsicmp(pName, _T("maxmana")) == 0)
		{
			pOut->SetInt(pDef->maxMana);
		}
		else if (_wcsicmp(pName, _T("mana")) == 0)
		{
			pOut->SetInt(mana);
		}
		else if (_wcsicmp(pName, _T("maxhealth")) == 0)
		{
			pOut->SetInt(pDef->maxHealth);
		}
		else if (_wcsicmp(pName, _T("health")) == 0)
		{
			pOut->SetInt(health);
		}
	}

	int GetAttackRange()
	{
		return pDef->attackRange;
	}

	int GetDefense()
	{
		return pDef->defense;
	}

	int GetStrength()
	{
		return pDef->meleeStr;
	}
	int TakeDamage(int attackerStr);
	bool IsDead()
	{
		return bIsDead;
	}
	UnitAbilityRef** GetAbilityRefs()
	{
		return pDef->eaAbilityRefs;
	}
	UnitAbility** GetAbilities()
	{
		return eaAbilities;
	}
	void AddModifier(UnitAttributeModifierDef* pDef);
	void UpdateUnit();
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
	CHexBuilding(hexBuildingDef* def, CHexPlayer* pOwner, bool bTakeOwnership = true);
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
	float progress;
	POINT loc;
};

class CHexCity : public CHexBuilding
{
private:
	int pop;
	int materials;
	int materialsmax;
	laborSlot** eaPopulationUsage;
	cityProject** eaProjectQueue;
	TCHAR* name;
	cityProject** eaAvailableProjects;
public:
	int GetNetProductionForProjects();
	CHexCity();
	CHexCity(hexBuildingDef* def, CHexPlayer* pOwner);
	int CountAvailableLabor();
	bool SetNextAvailableLabor(laborSlot* pSlot);
	bool ToggleLaborInSlot(laborSlot* pSlot);
	bool StartLaborInSlot(CHexBuilding* pBuilding, int idx);
	bool StopLaborInSlot(laborSlot* pSlot);
	bool AdjustPop(int delta);
	bool AddQueuedProject( projectType eType, void* pDef, POINT loc );
	bool ClearProductionQueue();
	cityProject** GetProductionQueue();
	cityProject* GetCurrentProject();
	void StartTurn(CHexPlayer* pOwner);
	bool AdvanceProject( cityProject* curProject, int prod );
	laborSlot** GetLaborList();
	int GetNetMaterials();
	int GetGrossMaterials();
	int GetNetGold();
	int GetNetResearch();
	buildingType GetMyType();
	LPCTSTR GetName();
	float GetMaterialsPct();
	void GetStatByName(const TCHAR* pName, multiVal* pOut);
	void RefreshAvailableProjectList();
	void GetAvailableProjectList(projectType eType, cityProject*** peaProjectsOut);
	bool StartLaborOnTile(hexTile* pTile);
};

struct StringTagContext;

void formatUnitTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);

void formatCityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);