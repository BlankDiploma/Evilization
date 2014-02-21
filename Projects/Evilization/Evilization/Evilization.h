#pragma once

#include "resource.h"

struct DebugFlags {
	int disableMapXWrap;
};

extern DebugFlags g_DebugFlags;
class MTRand;

extern MTRand randomFloats;