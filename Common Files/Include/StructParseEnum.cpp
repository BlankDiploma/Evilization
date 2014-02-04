#include "stdafx.h"
#include "StructParseEnum.h"

int AutoEnumStringToInt(StringIntHash* pHash, const TCHAR* str)
{
	StringIntHash::iterator hashIter = (*pHash).find(str);
	if(hashIter != (*pHash).end())
	{
		return (int)hashIter->second;
	}
	return 0;
}

const TCHAR* AutoEnumIntToString(IntStringHash* pHash, int i)
{
	IntStringHash::iterator hashIter = (*pHash).find(i);
	if(hashIter != (*pHash).end())
	{
		return (const TCHAR*)hashIter->second;
	}
	return NULL;
}