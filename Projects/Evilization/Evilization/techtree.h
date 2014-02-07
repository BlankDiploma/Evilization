#include "stdafx.h"
#include "deflibrary.h"
#include "structparse.h"
#include "earray.h"

#pragma once

PARSE_STRUCT(techEraDef)
{
	TEXTURE_REF hIcon;
	TEXTURE_REF hNinepatch;

	int iNinepatch;
	int index;
	int iColStart;
	int iColEnd;
	const TCHAR* name;
	const TCHAR* displayName;
};

PARSE_STRUCT(techTreeNodeDef)
{
	TEXTURE_REF hIcon;
	int cost;

	const TCHAR** eaRequiredNames;
	
	const TCHAR** eaGrants;

	DEF_REF(techEraDef) hEraDef;

	PARSE_IGNORE POINT layoutPt;

	const TCHAR* name;
};

PARSE_STRUCT(techTreeDef)
{
	const TCHAR** eaNames;
	techTreeNodeDef** eaNodes;
};

void AssignGridPositions(techTreeDef* pTree);

extern techTreeDef* g_pGoodTechTree;
extern techTreeDef* g_pEvilTechTree;
