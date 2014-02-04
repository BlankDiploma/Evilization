#pragma once

#include "stdafx.h"
#include "ModelLibrary.h"
#include "TacticsRenderer.h"
#include <vector>

using namespace std;

ModelLibrary g_ModelLibrary;

ModelLibrary::ModelLibrary()
{
	pFbxManager = NULL;
}

TacticsMesh* ModelLibrary::GetModel(const TCHAR* pchName)
{
	ModelHash::iterator hashIter;

	hashIter = htModels.find(pchName);
	if(hashIter != htModels.end())
	{
		return &(hashIter->second);
	}
	return NULL;
}

bool ModelLibrary::LoadModels(const TCHAR* pchDir, const TCHAR* pchFilename)
{
	/*
	per file: 
		1. create scene
		2. create importer
		3. import file into scene
		4. enum nodes - load their meshes into hashtable
		5. destroy scene.
	*/
	
	//Initialize manager here if we need to. Can't do this in constructor because of static issues.
	if (!pFbxManager)
	{
		pFbxManager = FbxManager::Create();										//manages memory for fbx objects - only need one.
		FbxIOSettings* pFbxIos = FbxIOSettings::Create(pFbxManager, IOSROOT);	//create&set fbx import settings.
		pFbxManager->SetIOSettings(pFbxIos);
	}

	FbxImporter* pFbxImporter = FbxImporter::Create(pFbxManager,"");
	FbxGeometryConverter fbxGeoConverter(pFbxManager);
	
	TCHAR directory[_MAX_PATH] = {0};
	CHAR directory_mbcs[_MAX_PATH] = {0};
	WIN32_FIND_DATA f;
	wcscpy_s(directory, pchDir);
	wcscat_s(directory, pchFilename);
	HANDLE h = FindFirstFile(directory, &f);
	TCHAR fnameBuf[_MAX_FNAME];
	if(h != INVALID_HANDLE_VALUE)
	{
		do{
	
			wcscpy_s(directory, pchDir);		//again ?  directory already has pchDir.
			wcscat_s(directory, f.cFileName);		//
			wcstombs_s(NULL, directory_mbcs, directory, (_MAX_PATH * sizeof(CHAR)));		//convert the filepath to mbcs to pass to fbx sdk functions.
			_wsplitpath_s(directory, NULL, 0, NULL, 0, fnameBuf, _MAX_FNAME, NULL, 0);						//get the filename to use as model namespace.
			//load models here
			FbxScene* pFbxScene = FbxScene::Create(pFbxManager, "");				//create an fbx scene - one scene per import - must be destroyed manually each cycle.
			vector<FbxNode*> vecpFbxNodeChildren;									//create a vector to hold the scene nodes
			
			if(pFbxImporter->Initialize(directory_mbcs, -1, pFbxManager->GetIOSettings()))	//init the fbx importer with filepath to the fbx file
			{
				pFbxImporter->Import(pFbxScene);
				fbxGeoConverter.Triangulate(pFbxScene, TRUE);								//Kids love them quads.  Fuck them.  Triangulate.
				FbxNode* pRootNode = pFbxScene->GetRootNode();								//Get the scene root and....
				ModelLibrary::GetFbxNodeChildren(pRootNode, &vecpFbxNodeChildren, TRUE);	//Recursively fill out the children array with descendants.  \o/
			}
			else
			{
				fprintf(stderr, "FBX_Importer -- Error importing file\n");
			}

			//iterate children.
			for(vector<FbxNode*>::const_iterator iterpFbxNode = vecpFbxNodeChildren.begin(); iterpFbxNode < vecpFbxNodeChildren.end(); ++iterpFbxNode)
			{
				FbxNodeAttribute::EType nodeType = (*iterpFbxNode)->GetNodeAttribute()->GetAttributeType();		//nodes can be anything.. ANYTHING. 
				if(nodeType == FbxNodeAttribute::eMesh)															//lets hope it's a mesh though.
				{
					TacticsMesh tacticsMesh;													//everything is cool if it starts with tactics.
					tacticsMesh.x = 0.0;
					tacticsMesh.y = 0.0;
					tacticsMesh.z = 0.0;
					

					//tacticsMesh.vecMeshVerts.clear();											//clear the new tactics mesh?  IDK... maybe i don't need this.
					FbxMesh* pMesh = (FbxMesh*) (*iterpFbxNode)->GetNodeAttribute();			//an attribute of mesh type node is... a mesh.  Boom.
					FbxVector4* pVertices = pMesh->GetControlPoints();
					tacticsMesh.vecMeshVerts = new TacticsVertex[pMesh->GetPolygonCount()*3];
					int iNumVerts = 0;
					int iNumTris = 0;
					for (int iFace = 0; iFace < pMesh->GetPolygonCount(); ++iFace)				//Go through each triangle
					{
						int iNumVertices = pMesh->GetPolygonSize(iFace);						//as paranoia slowly sets in
						assert( iNumVertices == 3 );
						for(int iVert = 0; iVert < 3; ++iVert)									//for 0 through 3 on each triangle
						{
							int iControlPointIndex = pMesh->GetPolygonVertex(iFace, iVert);			//assume it's an index of a vert in a triangle and retrieve it.
							TacticsVertex tacticsVert;												//put tactics in front of everything.
							tacticsVert.x = (float)pVertices[iControlPointIndex].mData[0];
							tacticsVert.y = (float)pVertices[iControlPointIndex].mData[1];
							tacticsVert.z = (float)pVertices[iControlPointIndex].mData[2];
							tacticsMesh.vecMeshVerts[iNumVerts++] = tacticsVert;						//push tacticsVert to tacticsMesh ^_^
						}
						iNumTris++;
					}
					tacticsMesh.iNumTris = iNumTris - 1;
					tacticsMesh.iNumVerts = iNumVerts - 1;
					WCHAR wstrNodeNameFull[_MAX_FNAME];
					WCHAR wstrNodeName[_MAX_FNAME];											
					wcscpy_s(wstrNodeNameFull, fnameBuf);
					const CHAR* strNodeName = (*iterpFbxNode)->GetName();					
					size_t converted = 0;
					mbstowcs_s(&converted, wstrNodeName, _MAX_FNAME, strNodeName, strlen(strNodeName));	//because it's coming in mbcs.
					wcscat_s(wstrNodeNameFull, _T("_"));
					wcscat_s(wstrNodeNameFull, wstrNodeName);											
					htModels[_wcsdup(wstrNodeNameFull)] = tacticsMesh;							//Add the TACTICS mesh to the table.  Then the ladies.
				}
			}
			pFbxScene->Destroy();
		} while(FindNextFile(h, &f));
	}
	else
	{
		fprintf(stderr, "Error opening directory\n");
		pFbxImporter->Destroy();
		pFbxManager->Destroy();
		return false;
	}
	FindClose(h);
	pFbxImporter->Destroy();
	pFbxManager->Destroy();
	return true;
}

/*Retrieve fbx children nodes.
Arguments:
pFbxParentNode		-- Ptr to parent fbx node.
pFbxNodeChildren	-- Ptr to an arrray to add children to (will try to append).
recursive			-- 0 will get the immidiate children only, 1 will get all descendands.  (default 0)*/
INT ModelLibrary::GetFbxNodeChildren(FbxNode* pFbxParentNode, vector<FbxNode*>* pvecFbxNodeChildren, INT recursive /*= 0*/)
{
	for(int i = 0; i < pFbxParentNode->GetChildCount(); ++i)
	{
		FbxNode* pFbxNodeChild = pFbxParentNode->GetChild(i);
		pvecFbxNodeChildren->push_back(pFbxNodeChild);
		if(recursive)
		{
			ModelLibrary::GetFbxNodeChildren(pFbxNodeChild, pvecFbxNodeChildren, recursive);
		}
	}
	return TRUE;
}

ModelLibrary::~ModelLibrary()
{
	//nothing to destroy yet
}