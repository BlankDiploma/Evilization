#pragma once

#include "stdafx.h"
#include "fbxsdk.h"
#include "stddef.h"
#include "strhashmap.h"
#include "TacticsRenderer.h"

using namespace std;

typedef stdext::hash_map<const wchar_t*, TacticsMesh, stringHasher> ModelHash;

class ModelLibrary
{
	ModelHash htModels;

public:
	ModelLibrary();
	TacticsMesh* GetModel(const TCHAR* pchName);
	bool LoadModels(const TCHAR* pchDir, const TCHAR* pchFilename);
	~ModelLibrary();

	FbxManager* pFbxManager;
private:
	INT GetFbxNodeChildren(FbxNode* pFbxParentNode, vector<FbxNode*>* pvecFbxNodeChildren, INT recursive = 0);
};

extern ModelLibrary g_ModelLibrary;

#define LOAD_MODELS(a) g_ModelLibrary.LoadModels(_T("data/geometry/"), _T(a));
#define GET_MODEL(a) g_ModelLibrary.GetModel(a);