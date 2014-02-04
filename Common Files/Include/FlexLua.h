#include "stdafx.h"

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <luabind/luabind.hpp>
#include <luabind/adopt_policy.hpp>
#include "earray.h"
#include "structparse.h"
#include "StructParseLuabind.h"

struct lua_writer_data
{
	BYTE** buf;
	int* len;
	lua_writer_data()
	{
		buf = NULL;
		len = 0;
	}
};

class UIInstance;

struct LuaContext
{
	POINT mousePt;
	UIInstance* pUI;
};

int flex_lua_writer(lua_State * lua, const void *p, size_t sz, void *ud);

int FlexLua_ExecuteString(const char* pchString, LuaContext* pContext);
luabind::object FlexLua_ExecuteScript(LuaScript* pScript, LuaContext* pContext);
lua_Number FlexLua_ExecuteScript_DoubleReturn(LuaScript* pScript, LuaContext* pContext);
void FlexLua_ExecuteScript_NoReturn(LuaScript* pScript, LuaContext* pContext);
luabind::object FlexLua_ExecuteScript_TableReturn(LuaScript* pScript, luabind::object*** peaReturnObjects, LuaContext* pContext);
void FlexLua_ExecuteScript_ArrayReturn(LuaScript* pScript, luabind::object*** peaReturnObjects, LuaContext* pContext);
void FlexLua_ExecuteScript_IntArrayReturn(LuaScript* pScript, int** peaiReturn, LuaContext* pContext);
void FlexLua_ExecuteScript_EArrayReturn(LuaScript* pScript, void*** peaOut, LuaContext* pContext);
POINT FlexLua_ExecuteScript_PointReturn(LuaScript* pScript, LuaContext* pContext);
const TCHAR* FlexLua_ExecuteScript_StringReturn(LuaScript* pScript, LuaContext* pContext);

extern lua_State* g_LUA;
extern LuaContext* g_pCurContext;
