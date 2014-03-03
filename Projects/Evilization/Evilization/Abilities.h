#include "stdafx.h"
#include "structparse.h"
#include "earray.h"

#pragma once

AUTO_ENUM(AbilityAffectsFlags) 
{
	kAffects_None = 0,
	kAffects_Enemy,
	kAffects_Ally,
	kAffects_Tile
};

PARSE_STRUCT(UnitAbilityDef)
{	
	int cost;
	int range;
	int radius;
	int damage;
	float cooldown;
	FLAGS AbilityAffectsFlags eAffectsFlags;
	const TCHAR* name;
	const TCHAR* displayName;
};

PARSE_STRUCT(UnitAbilityRef)
{
    INLINE_ARG DEF_REF(UnitAbilityDef) hAbility;
};

class UnitAbility
{
public:
	float cooldown;
	const UnitAbilityDef* pDef;
	void GetStatByName(const TCHAR* pName, multiVal* pOut);
};

struct StringTagContext;

void formatAbilityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);
