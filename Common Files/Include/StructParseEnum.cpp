#include "stdafx.h"
#include "StructParseEnum.h"
#include "FlexErrorWindow.h"

int AutoEnumStringToInt(StringIntHash* pHash, const TCHAR* str, int* pValOut)
{
	StringIntHash::iterator hashIter = (*pHash).find(str);
	if(hashIter != (*pHash).end())
	{
		if (pValOut)
			*pValOut = hashIter->second;
		return 1;
	}
	Errorf("%s is not a valid enum value!", str);
	return 0;
}

const TCHAR* AutoEnumIntToString(IntStringHash* pHash, int i)
{
	IntStringHash::iterator hashIter = (*pHash).find(i);
	if(hashIter != (*pHash).end())
	{
		return (const TCHAR*)hashIter->second;
	}
	Errorf("%d is not a valid enum value!", i);
	return NULL;
}