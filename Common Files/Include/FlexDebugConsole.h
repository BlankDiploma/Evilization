#include "stdafx.h"
#include "FlexRenderer.h"
#include <string>
#include "stddef.h"
#include "strhashmap.h"
using namespace std;

struct GameTexture;

#define CONSOLE_NUM_LINES 36

class FlexDebugConsole
{
	//console stuff
	int lastInputRetrieved;
	int selectedFrom;
	int caratIndex;
	float fCaratFlashTimer;
	GameTexture* pConsoleFont;
	wstring strConsoleInputBuffer;
	const TCHAR** eaConsoleLines;
	bool bEnabled;
public:
	FlexDebugConsole();
	void Render();
	void ExecuteConsoleCommand(const char* pCommandArgs);
	void ExecuteConsoleCommand(const TCHAR* pCommandArgs);
	void AddConsoleString(const TCHAR* pStr);
	void Update(float fSecondsElapsed);
	void KeyInput(int keyCode, TCHAR ascii);
	void Toggle();
	bool IsEnabled();
};

extern FlexDebugConsole g_Console;