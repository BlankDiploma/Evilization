#include "stdafx.h"

#include <luabind/luabind.hpp>
#pragma once

struct LuaScript
{
	const char* pchPlaintext; 
	luabind::object* func;
//	const char* pBytecode; 
	int iByteLength;
};