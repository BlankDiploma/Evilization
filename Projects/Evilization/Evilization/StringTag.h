#include "stdafx.h"
class CHexUnit;
class CHexCity;
class UnitAbility;

struct StringTagContext
{
	CHexUnit* pUnit;
	CHexCity* pCity;
	UnitAbility* pAbility;
	wchar_t bracketChar;
	StringTagContext()
	{
		bracketChar = '{';
	}
};

typedef void (*stringTagCallback)(const TCHAR* tag, TCHAR* pOut, StringTagContext* pSubject);

void formatStringTags(const char* fmt, TCHAR* pOut, int outLen, stringTagCallback pCallback, StringTagContext* pContext);