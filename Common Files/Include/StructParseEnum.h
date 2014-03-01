#include "stdafx.h"
#include "StructParse.h"
#include "strhashmap.h"

int AutoEnumStringToInt(StringIntHash* pHash, const TCHAR* str, int* pValOut);

const TCHAR* AutoEnumIntToString(IntStringHash* pHash, int i);