#include "stdafx.h"
#include "FlexDebugConsole.h"
#include "TextureLibrary.h"
#include "FlexRenderer.h"
#include "Earray.h"
#include "FlexLua.h"

FlexDebugConsole::FlexDebugConsole()
{
	//console stuff
	selectedFrom = 0;
	caratIndex = 0;
	fCaratFlashTimer = 0.0f;
	pConsoleFont = NULL;
	eaConsoleLines = NULL;
	bEnabled = false;
}

/*
void CDungeonWorld::AutocompleteCmd()
{
	ConsoleCommandHashTable::iterator hashIter;
	IntVarsHashTable::iterator intIter;
	FloatVarsHashTable::iterator floatIter;


	for(hashIter = consoleFuncs.begin(); hashIter != consoleFuncs.end(); ++hashIter) 
	{
		CAtlString iterString(hashIter->first);
		if (currentConsoleString == iterString)
		{
			return;
		}
		if (iterString.Left(currentConsoleString.GetLength()) == currentConsoleString)
		{
			selectedFrom = currentConsoleString.GetLength();
			caratIndex = iterString.GetLength();
			currentConsoleString += iterString.Right(iterString.GetLength() - currentConsoleString.GetLength());
			return;
		}
	}
	for(intIter = intVars.begin(); intIter != intVars.end(); ++intIter) 
	{
		CAtlString iterString(intIter->first);
		if (currentConsoleString == iterString)
		{
			return;
		}
		if (iterString.Left(currentConsoleString.GetLength()) == currentConsoleString)
		{
			selectedFrom = currentConsoleString.GetLength();
			caratIndex = iterString.GetLength();
			currentConsoleString += iterString.Right(iterString.GetLength() - currentConsoleString.GetLength());
			return;
		}
	}
	for(floatIter = floatVars.begin(); floatIter != floatVars.end(); ++floatIter) 
	{
		CAtlString iterString(floatIter->first);
		if (currentConsoleString == iterString)
		{
			return;
		}
		if (iterString.Left(currentConsoleString.GetLength()) == currentConsoleString)
		{
			selectedFrom = currentConsoleString.GetLength();
			caratIndex = iterString.GetLength();
			currentConsoleString += iterString.Right(iterString.GetLength() - currentConsoleString.GetLength());
			return;
		}
	}
}
*/

void FlexDebugConsole::AddConsoleString(const TCHAR* pStr)
{
	TCHAR* pDupStr = _wcsdup(pStr);
	eaPush(&eaConsoleLines, pDupStr);
}

void FlexDebugConsole::ExecuteConsoleCommand(const char* pCommandArgs)
{
	TCHAR buf[1024] = {0};
	LuaContext context = {0};
	int ret = FlexLua_ExecuteString(pCommandArgs, &context);
	if (ret == -1)
	{
		wsprintf(buf, L"|cff0000An error occurred: %S", lua_tostring(g_LUA, -1));
		lua_pop(g_LUA, 1);
		AddConsoleString(buf);
	}
	else
	{
		for (int i = 0; i < ret; i++)
		{
			wsprintf(buf, L"|c00ff00%S", lua_tostring(g_LUA, -1));
			lua_pop(g_LUA, 1);
			AddConsoleString(buf);
		}
	}
}

void FlexDebugConsole::ExecuteConsoleCommand(const TCHAR* pCommandArgs)
{
	char buf[512] = {0};
	size_t num = 0;
	wcstombs_s(&num, buf, pCommandArgs, 512);
	if (num > 0)
		ExecuteConsoleCommand(buf);
}

/*
void CDungeonWorld::RenderDebugInfo()
{
	GfxSheetVertex pVerteces[4];
	int letterWidth = pSysFont->GetLetterWidth()/2;
	pD3DDevice->SetTexture ( 0 , NULL ) ;
	//render
	for (int i = 0; i < numDebugStrings; i++)
	{
		pSysFont->RenderString(debugInfo[i].GetString(), 0, (pSysFont->GetLetterHeight())*i, 0xffff0000);
		debugInfo[i].ReleaseBuffer();
	}
}
*/

void FlexDebugConsole::Update(float fSecondsElapsed)
{
	fCaratFlashTimer += fSecondsElapsed;
	if (fCaratFlashTimer >= 1.0f)
		fCaratFlashTimer -= 1.0f;
}

void FlexDebugConsole::Render()
{
	if (bEnabled && pConsoleFont)
	{
		int letterWidth = pConsoleFont->letterWidth;

		//render
		RECT dst = {0, 0, g_Renderer.GetScreenWidth(), pConsoleFont->letterHeight*CONSOLE_NUM_LINES};
		g_Renderer.AddSolidColorToRenderList(&dst, D3DCOLOR_ARGB(127, 100, 100, 100));

		dst.top = pConsoleFont->letterHeight*CONSOLE_NUM_LINES;
		dst.bottom = pConsoleFont->letterHeight*(CONSOLE_NUM_LINES+1);
		g_Renderer.AddSolidColorToRenderList(&dst, D3DCOLOR_ARGB(127, 140, 140, 140));

		RECT selected = {letterWidth + min(selectedFrom, caratIndex) * letterWidth,
						pConsoleFont->letterHeight*CONSOLE_NUM_LINES,
						letterWidth + max(selectedFrom, caratIndex) * letterWidth,
						pConsoleFont->letterHeight*(CONSOLE_NUM_LINES+1)};
		g_Renderer.AddSolidColorToRenderList(&selected, D3DCOLOR_ARGB(175, 0, 255, 255));

		/*
		render text buffer
		*/

		for (int i = 0; i < eaSize(&eaConsoleLines); i++)
		{
			g_Renderer.AddStringToRenderList(pConsoleFont, eaConsoleLines[i], 0.0f, (float)pConsoleFont->letterHeight*i, 0xffffffff, false, false, false);
		}

		g_Renderer.AddStringToRenderList(pConsoleFont, _T(">"), 0.0f, (float)pConsoleFont->letterHeight*CONSOLE_NUM_LINES, 0xffff0000, false, false, false);
		g_Renderer.AddStringToRenderList(pConsoleFont, strConsoleInputBuffer.c_str(), (float)letterWidth, (float)pConsoleFont->letterHeight*CONSOLE_NUM_LINES, 0xffff0000, false, false, false);
	
		if (fCaratFlashTimer > 0.5f)
			g_Renderer.AddStringToRenderList(pConsoleFont, _T("|"), 6.0f + caratIndex*letterWidth, (float)pConsoleFont->letterHeight*CONSOLE_NUM_LINES, 0xffffffff, false, false, false);
	}
}

int ClipboardToString( wstring* pString )
{
	if ( !pString || !IsClipboardFormatAvailable(CF_UNICODETEXT) )
		return 0;
	else {
		wchar_t *  pClipboardData;
		HGLOBAL hGlobal;
		size_t  size;

		OpenClipboard( NULL );


		hGlobal = GetClipboardData( CF_UNICODETEXT );

		size = GlobalSize( hGlobal );


		if (   ( pClipboardData = (wchar_t *)GlobalLock( hGlobal ) ))
		{
			(*pString) = pClipboardData;
		}

		CloseClipboard();

		return (*pClipboardData) ? size : 0;
	}
}

void FlexDebugConsole::KeyInput(int keyCode, TCHAR ascii)
{
	if (keyCode == 'V' && GetAsyncKeyState(VK_CONTROL))
	{
		wstring strPasteData;
		ClipboardToString(&strPasteData);
		if (selectedFrom != caratIndex)
		{
			strConsoleInputBuffer.erase(min(selectedFrom, caratIndex), abs(selectedFrom - caratIndex));
			selectedFrom = caratIndex = min(selectedFrom, caratIndex);
		}
		strConsoleInputBuffer.insert(caratIndex++, strPasteData);	
		selectedFrom = caratIndex;
		return;
	}
	if (ascii >= ' ' && ascii <= '~')
	{
		if (selectedFrom != caratIndex)
		{
			strConsoleInputBuffer.erase(min(selectedFrom, caratIndex), abs(selectedFrom - caratIndex));
			selectedFrom = caratIndex = min(selectedFrom, caratIndex);
		}
		strConsoleInputBuffer.insert(caratIndex++, 1, ascii ? ascii : keyCode);	
		selectedFrom = caratIndex;
//		AutocompleteCmd();
		return;
	}
	if (keyCode == VK_BACK && caratIndex > 0)
	{
		if (selectedFrom != caratIndex)
		{
			strConsoleInputBuffer.erase(min(selectedFrom, caratIndex), abs(selectedFrom - caratIndex));
			selectedFrom = caratIndex = min(selectedFrom, caratIndex);
		}
		else
		{
			strConsoleInputBuffer.erase(--caratIndex, 1);
			selectedFrom = caratIndex = min(selectedFrom, caratIndex);
		}
	}
	if (keyCode == VK_RETURN)
	{
		if (strConsoleInputBuffer.length() <= 0)
			return;

		int cmdEnd = 0;
		AddConsoleString(strConsoleInputBuffer.c_str());
//		StoreRecentInput(&currentConsoleString);

		ExecuteConsoleCommand(strConsoleInputBuffer.c_str());

		strConsoleInputBuffer.clear();
		selectedFrom = caratIndex = 0;
	}
	if (keyCode == VK_LEFT)
	{
		if (GetAsyncKeyState(VK_SHIFT))
		{
			selectedFrom = max(selectedFrom-1,0);
		}
		else
			selectedFrom = caratIndex = max(caratIndex-1,0);
	}
	if (keyCode == VK_RIGHT)
	{
		if (GetAsyncKeyState(VK_SHIFT))
		{
			selectedFrom = min(selectedFrom+1,(int)strConsoleInputBuffer.length());
		}
		else
		selectedFrom = caratIndex = min(caratIndex+1,(int)strConsoleInputBuffer.length());
	}
	/*
	if (keyCode == VK_UP)
	{
		lastInputRetrieved = max(lastInputRetrieved-1, 0);
		if (!recentInputBuffer[lastInputRetrieved])
			lastInputRetrieved++;
		currentConsoleString = recentInputBuffer[lastInputRetrieved] ? (*recentInputBuffer[lastInputRetrieved]) : _T("");
		selectedFrom = caratIndex = currentConsoleString.GetLength();
	}
	if (keyCode == VK_DOWN)
	{
		lastInputRetrieved = min(lastInputRetrieved+1, INPUT_BUFFER_SIZE-1);
		currentConsoleString = recentInputBuffer[lastInputRetrieved] ? (*recentInputBuffer[lastInputRetrieved]) : _T("");
		selectedFrom = caratIndex = currentConsoleString.GetLength();
	}
	*/
}

void FlexDebugConsole::Toggle()
{
	if (!pConsoleFont)
		pConsoleFont = GET_TEXTURE(L"Courier_New");
	bEnabled = !bEnabled;
}

bool FlexDebugConsole::IsEnabled()
{
	return bEnabled;
}

FlexDebugConsole g_Console;