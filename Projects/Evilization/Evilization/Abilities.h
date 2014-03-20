#include "stdafx.h"
#include "structparse.h"
#include "earray.h"

#pragma once

enum UnitAttribute;
AUTO_ENUM(UnitAttributeModType) {kUnitAttributeModType_Absolute = 0, kUnitAttributeModType_PercentAdditive, kUnitAttributeModType_PercentMultiplicative};

PARSE_STRUCT(UnitAttributeModifierDef)
{
	UnitAttribute eAffects;
	UnitAttributeModType eModType;
	float magnitude;
	float duration;
};

struct UnitAttributeModifier
{
	float fDurationInTurns;
	const UnitAttributeModifierDef* pDef;
};

AUTO_ENUM(AbilityAffectsFlags) 
{
	kAffects_None = 0,
	kAffects_Enemy = 1,
	kAffects_Ally = 2,
	kAffects_Tile = 4
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
	UnitAttributeModifierDef** eaUnitAttributeModList;
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
	UnitAbility();
	UnitAbility(UnitAbility* pAbility);
};

struct StringTagContext;

void formatAbilityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);
