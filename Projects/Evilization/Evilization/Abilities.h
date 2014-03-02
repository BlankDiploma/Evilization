#include "stdafx.h"
#include "structparse.h"
#include "earray.h"

AUTO_ENUM(AbilityAffectsFlags) 
{
	kAffects_None = 0,
	kAffects_Enemy,
	kAffects_Ally,
	kAffects_Tile
};

PARSE_STRUCT(UnitAbilityDef)
{
	float cooldown;
	const TCHAR* name;
	FLAGS AbilityAffectsFlags eAffectsFlags;
};

struct UnitAbility
{
	float cooldown;
	const UnitAbilityDef* pDef;
};

struct StringTagContext;

void formatAbilityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext);
