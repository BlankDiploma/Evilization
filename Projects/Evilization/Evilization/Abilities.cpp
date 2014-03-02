#include "stdafx.h"
#include "Abilities.h"
#include "StringTag.h"

void formatAbilityTag(const TCHAR* tag, TCHAR* pOut, StringTagContext* pContext)
{
	//multiVal val;
	//pContext->pAbility->GetStatByName(tag, &val);

	//switch (val.myType)
	//{
	//case MULTIVAL_INT:
	//	{
	//		wsprintf(pOut, _T("%i"), val.GetInt());
	//	}break;
	//case MULTIVAL_FLOAT:
	//	{
	//		wsprintf(pOut, _T("%.3f"), val.GetFloat());
	//	}break;
	//case MULTIVAL_STRING:
	//	{
	//		wsprintf(pOut, _T("%s"), val.GetString());
	//	}break;
	//}
}

#include "Autogen\Abilities_h_ast.cpp"