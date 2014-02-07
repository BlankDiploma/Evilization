#include "stdafx.h"

#include "HexFeatures.h"
#include "StringTag.h"

bool commonTagCallback(TCHAR* tag, TCHAR* pOut, StringTagContext* pContext)
{
	if (_wcsicmp(tag, _T("\n")) == 0)
	{
		wsprintf(pOut, L"\n");
		return true;
	}
	return false;
}

void formatStringTags(const char* fmt, TCHAR* pOut, int outLen, stringTagCallback pCallback, StringTagContext* pContext)
{
	static TCHAR widebuf[256];
	int numTagsFound = 0;
	swprintf_s(widebuf, L"%S", fmt);

	TCHAR* iter = widebuf;
	for (;;)
	{
		TCHAR* tagStart = wcschr(iter, '{');
		if (!tagStart)
		{
			tagStart = wcschr(iter, '[');
			if (!tagStart)
				break;//didn't find a tag
		}
		pContext->bracketChar = *tagStart;
		numTagsFound++;
		wcsncat_s(pOut, outLen, iter, tagStart-iter);

		TCHAR* tagEnd = wcschr(tagStart, '}');
		if (!tagEnd)
			tagEnd = wcschr(tagStart, ']');

		TCHAR buf[16] = {0};
		TCHAR temp = *tagEnd;
		*tagEnd = '\0';
		if (!commonTagCallback(tagStart+1, buf, pContext) && pCallback)
			pCallback(tagStart+1, buf, pContext);
		wcscat_s(pOut, outLen, buf);
		*tagEnd = temp;
		iter = tagEnd+1;
	}

	if (numTagsFound)
	{
		wcscat_s(pOut, outLen, iter);
	}
	else
	{
		wcscpy_s(pOut, outLen, widebuf);
	}
}