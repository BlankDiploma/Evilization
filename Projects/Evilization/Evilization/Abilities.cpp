#include "stdafx.h"
#include "Abilities.h"
#include "StringTag.h"

void formatAbilityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext)
{
	multiVal val;
	pContext->pAbility->GetStatByName(tag, &val);

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


void UnitAbility::GetStatByName(const TCHAR* pName, multiVal* pOut)
{
	//if (_wcsicmp(pName, _T("name")) == 0)
	//	pOut->SetUnownedString(pDef->name);
	//else if (_wcsicmp(pName, _T("")) == 0)
	//{
	//	pOut->SetInt(movRemaining);
	//}
	//else if (_wcsicmp(pName, _T("maxmov")) == 0)
	//{
	//	pOut->SetInt(pDef->movement);
	//}
	//else if (_wcsicmp(pName, _T("str")) == 0)
	//{
	//	pOut->SetInt(pDef->meleeStr);
	//}
}

#include "Autogen\Abilities_h_ast.cpp"