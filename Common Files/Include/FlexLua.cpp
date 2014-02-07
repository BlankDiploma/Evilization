#include "stdafx.h"

#include "FlexLua.h"
#include "windows.h"

lua_State* g_LUA;
LuaContext* g_pCurContext = NULL;

int flex_lua_writer(lua_State * lua, const void *p, size_t sz, void *ud)
{
    lua_writer_data* data = (lua_writer_data*)ud;

    BYTE *newBuf;

	if(newBuf = (BYTE *)realloc((*data->buf), (*data->len) + sz)) {
        memcpy(newBuf + (*data->len), p, sz);
        (*data->buf) = newBuf;
        (*data->len) += sz;
    } else {
        free(newBuf);
        return 1;
    }

    return 0;
}

//returns the number of objects on the stack
//don't just leave them there!
int FlexLua_ExecuteString(const char* pchScript, LuaContext* pContext)
{
	g_pCurContext = pContext;
	int top = lua_gettop(g_LUA);
	luaL_loadstring(g_LUA, pchScript);
	if (lua_pcall(g_LUA, 0, LUA_MULTRET, 0) != 0)
	{
		//error
		return -1;
	}
	top = lua_gettop(g_LUA) - top;
	return top;
}

luabind::object FlexLua_ExecuteScript(LuaScript* pScript, LuaContext* pContext)
{
	try
	{
		g_pCurContext = pContext;
		return luabind::call_function<luabind::object>(*pScript->func);
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
	luabind::object ret;
	return ret;
}

const TCHAR* FlexLua_ExecuteScript_StringReturn(LuaScript* pScript, LuaContext* pContext)
{
	try
	{
		g_pCurContext = pContext;
		return luabind::call_function<const TCHAR*>(*pScript->func);
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
	return NULL;
}

lua_Number FlexLua_ExecuteScript_DoubleReturn(LuaScript* pScript, LuaContext* pContext)
{
	try
	{
		g_pCurContext = pContext;
		return luabind::call_function<lua_Number>(*pScript->func);
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
	return 0;
}

void FlexLua_ExecuteScript_NoReturn(LuaScript* pScript, LuaContext* pContext)
{
	try
	{
		g_pCurContext = pContext;
		luabind::call_function<void>(*pScript->func);
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
}

POINT FlexLua_ExecuteScript_PointReturn(LuaScript* pScript, LuaContext* pContext)
{
	try
	{
		g_pCurContext = pContext;
		return luabind::call_function<POINT>(*pScript->func);
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
	POINT pt = {0,0};
	return pt;
}

luabind::object FlexLua_ExecuteScript_TableReturn(LuaScript* pScript, luabind::object*** peaReturnObjects, LuaContext* pContext)
{
	luabind::object ret;
	try
	{
		g_pCurContext = pContext;
		ret = luabind::call_function<luabind::object>(*pScript->func);
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
	if (luabind::type(ret) == LUA_TTABLE && ret)
	{
		for (luabind::iterator i(ret), end; i != end; ++i)
		{
			luabind::object val = *i;
			eaPush(peaReturnObjects, new luabind::object(val));
		}
	}
	return ret;
}
/*
luabind::object FlexLua_ExecuteBytecode(LuaScript* pScript)
{
//	luaL_loadbuffer(g_LUA, pScript->pBytecode, pScript->iByteLength, NULL);
	lua_pcall(g_LUA, 0, 1, 0);
	luabind::object ret(luabind::from_stack(g_LUA, 0));
	lua_pop(g_LUA, 1);
	return ret;
}
*/
void FlexLua_ExecuteScript_ArrayReturn(LuaScript* pScript, luabind::object*** peaReturnObjects, LuaContext* pContext)
{
	g_pCurContext = pContext;
	int top = lua_gettop(g_LUA);
	pScript->func->push(g_LUA);
	lua_pcall(g_LUA, 0, LUA_MULTRET, 0);
	top = lua_gettop(g_LUA) - top;
	for (int i = 0; i < top; i++)
	{
		eaPush(peaReturnObjects, new luabind::object(luabind::from_stack(g_LUA, -i)));
	}
	lua_pop(g_LUA, top);
}
void FlexLua_ExecuteScript_EArrayReturn(LuaScript* pScript, void*** peaOut, LuaContext* pContext)
{
	try
	{
		g_pCurContext = pContext;
		luabind::object obj = luabind::call_function<luabind::object>(*pScript->func);
		
		assert(luabind::type(obj) == LUA_TLIGHTUSERDATA);
		obj.push(g_LUA);
		void** ea = (void**)lua_touserdata(obj.interpreter(), -1);
		lua_pop(obj.interpreter(), 1);
		if (peaOut)
			*peaOut = ea;
	}
	catch(luabind::error& e)
	{
		luabind::object error_msg(luabind::from_stack(e.state(), -1));
		std::string str = luabind::object_cast<std::string>(error_msg);
		assert(0);
	}
}

void FlexLua_ExecuteScript_IntArrayReturn(LuaScript* pScript, int** peaiReturn, LuaContext* pContext)
{
	g_pCurContext = pContext;
	int top = lua_gettop(g_LUA);
	pScript->func->push(g_LUA);
	lua_pcall(g_LUA, 0, LUA_MULTRET, 0);
	top = lua_gettop(g_LUA) - top;
	for (int i = 0; i < top; i++)
	{
		eaPushInt(peaiReturn, (int)lua_tonumber(g_LUA, -i));
	}
	lua_pop(g_LUA, top);
}
/*
void FlexLua_ExecuteBytecode_NoReturn(LuaScript* pScript)
{
//	luaL_loadbuffer(g_LUA, pScript->pBytecode, pScript->iByteLength, NULL);
	lua_pcall(g_LUA, 0, 0, 0);
}

double FlexLua_ExecuteBytecode_DoubleReturn(LuaScript* pScript)
{
//	luaL_loadbuffer(g_LUA, pScript->pBytecode, pScript->iByteLength, NULL);
	lua_pcall(g_LUA, 0, 1, 0);
	double ret = lua_tonumber(g_LUA, 0);
	lua_pop(g_LUA, 1);
	return ret;
}
*/