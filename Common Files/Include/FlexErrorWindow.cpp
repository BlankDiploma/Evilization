#include "stdafx.h"
#include "strhashmap.h"
#include "FlexErrorWindow.h"
#include "EArray.h"
#include "windows.h"
#include <stdio.h>

#ifndef FLEX_ERROR_USE_STDERR

#include "resource.h"
#include "Commctrl.h"

static HWND hErrorDlg = 0;

ErrorHash stErrorTextToTracker;
int idx = 0;

ErrorTracker** eaErrors;

void AddErrorToList(HWND hListControl, ErrorTracker* pTracker)
{
	static TCHAR buf[512] = {0};
	LVITEM item = {0};

	wsprintf(buf, L"%d", pTracker->id);
	item.mask = LVIF_TEXT;
	item.pszText = buf;
	item.iItem = pTracker->id;
	item.iSubItem = 0;
	ListView_InsertItem(hListControl, &item);
	
	wsprintf(buf, L"%d", pTracker->Count);
	item.pszText = buf;
	item.iSubItem = 1;
	ListView_SetItem(hListControl, &item);
	
	wsprintf(buf, L"%s", pTracker->pParent ? pTracker->pchText : NULL);
	item.pszText = buf;
	item.iSubItem = 2;
	ListView_SetItem(hListControl, &item);
	
	wsprintf(buf, L"%s", pTracker->pParent ? pTracker->pParent->pchText : pTracker->pchText);
	item.pszText = buf;
	item.iSubItem = 3;
	ListView_SetItem(hListControl, &item);
}

void RebuildErrorList(HWND hListControl)
{
	int i = 0;
	ErrorHash::iterator errorIter = stErrorTextToTracker.begin();
	ListView_DeleteAllItems(hListControl);
	
	for (; errorIter != stErrorTextToTracker.end(); errorIter++)
	{
		ErrorTracker* pTracker = &errorIter->second;
		if (pTracker->stFilenameToTracker.size() > 0)
		{
			//filename errors
			ErrorHash::iterator filenameIter = pTracker->stFilenameToTracker.begin();
			for (; filenameIter != pTracker->stFilenameToTracker.end(); filenameIter++)
			{
				AddErrorToList(hListControl, &filenameIter->second);
			}
		}
		else
		{
			AddErrorToList(hListControl, pTracker);
		}
	}
}

void DestroyErrorList(HWND hListControl)
{
	ErrorHash::iterator errorIter = stErrorTextToTracker.begin();
	
	for (; errorIter != stErrorTextToTracker.end(); errorIter++)
	{
		ErrorTracker* pTracker = &errorIter->second;

		if (pTracker->pchText)
			free((void*)pTracker->pchText);

		if (pTracker->stFilenameToTracker.size() > 0)
		{
			//filename errors
			ErrorHash::iterator filenameIter = pTracker->stFilenameToTracker.begin();
			for (; filenameIter != pTracker->stFilenameToTracker.end(); filenameIter++)
			{
				if (filenameIter->second.pchText)
					free((void*)filenameIter->second.pchText);
			}
		}
	}
	ListView_DeleteAllItems(hListControl);
	stErrorTextToTracker.clear();
	eaClear(&eaErrors);
	idx = 0;
}

void SetForceAssert(ErrorTracker* pTracker)
{
	pTracker->bForceAssert = true;
}

void SetErrorListCount(HWND hListControl, ErrorTracker* pTracker)
{
	//int i = 0;
	static TCHAR buf[10] = {0};

	wsprintf(buf, L"%d", pTracker->Count);
	ListView_SetItemText(hListControl,pTracker->id,1,buf);
}

ErrorTracker* GetSelectedError(HWND hListControl)
{
	int iPos = ListView_GetNextItem(hListControl, -1, LVNI_SELECTED);
	if (iPos != -1)
	{
		LVITEM item;
		TCHAR buf[6];
		item.mask = LVIF_TEXT;
		item.pszText = buf;
		item.cchTextMax = 6;
		item.iItem = iPos;
		item.iSubItem = 0;
		ListView_GetItem(hListControl, &item);
		
		int iErrorID = _wtoi(buf);

		return eaErrors[iErrorID];
	}
	return NULL;
}

BOOL CALLBACK ErrorProc(HWND hwndDlg, 
                             UINT message, 
                             WPARAM wParam, 
                             LPARAM lParam) 
{ 
    switch (message) 
    { 
        case WM_COMMAND: 
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ERRORLIST);
				switch (LOWORD(wParam)) 
				{ 
					case IDCLEAR: 
						DestroyErrorList(hList);
						return TRUE; 
					case IDOPENFILE: 

						return TRUE; 
					case IDASSERT: 
						{
							ErrorTracker* pTrack = GetSelectedError(hList);
							SetForceAssert(pTrack);
							return TRUE; 
						}
					case IDCANCEL: 
					case IDCLOSE: 
						{
							hErrorDlg = 0;
							EndDialog(hwndDlg, wParam); 
							return TRUE; 
						}
				}break;
			}
		case WM_INITDIALOG:
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ERRORLIST);
				LVCOLUMN col;
				col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
				col.fmt = LVCFMT_LEFT;
				col.iSubItem = 0;
				col.cx = 16;
				col.pszText = L"Idx";
				ListView_InsertColumn(hList, 0, &col);
				col.iSubItem = 1;
				col.cx = 20;
				col.pszText = L"Count";
				ListView_InsertColumn(hList, 1, &col);
				col.iSubItem = 2;
				col.cx = 64;
				col.pszText = L"Filename";
				ListView_InsertColumn(hList, 2, &col);
				col.iSubItem = 3;
				col.cx = 540;
				col.pszText = L"Text";
				ListView_InsertColumn(hList, 3, &col);
				SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
				RebuildErrorList(hList);
			}break;
        case WM_USER: 
			{
				HWND hList = GetDlgItem(hwndDlg, IDC_ERRORLIST);
				ErrorTracker* pTracker = (ErrorTracker*)wParam;
				if (lParam == 1)
				{
					AddErrorToList(hList, pTracker);
				}
				else
					SetErrorListCount(hList, pTracker);
			}break;
		case WM_NOTIFY:
			{

				NMLISTVIEW *VAL_notify = (NMLISTVIEW*)lParam;
				if(VAL_notify->hdr.code == LVN_ITEMCHANGED && VAL_notify->hdr.idFrom == IDC_ERRORLIST  && (VAL_notify->uNewState & LVIS_SELECTED))
				{
					ErrorTracker* pError = eaErrors[VAL_notify->iItem];// Use VAL_notify->iItem as the new selected item
					SetWindowText(GetDlgItem(hErrorDlg, IDC_DETAILS), pError->pParent ? pError->pParent->pchText : pError->pchText);
					SendMessage(GetDlgItem(hErrorDlg, IDC_DETAILS), EM_FMTLINES , 0, 0);
				}
			}break; 
	}
    return FALSE; 
} 

void ErrorInternalf(const TCHAR* pchErrorFmt, const TCHAR* pchFilename, ...)
{
	static TCHAR errorText[512] = {0};
	ErrorHash::iterator hashIter;
	if (!IsWindow(hErrorDlg))
	{
		hErrorDlg = CreateDialog(hInst, (LPCTSTR)IDD_ERRORDIALOG, hWndMain, (DLGPROC)ErrorProc);
		ShowWindow(hErrorDlg, 1);
	}
	va_list args;
	va_start(args, pchFilename);
	wvsprintf(errorText, pchErrorFmt, args);
	va_end(args);

	const TCHAR* pchTextDup = _wcsdup(errorText);
	ErrorTracker& tracker = stErrorTextToTracker[pchTextDup];

	tracker.Count++;
	
	if (!tracker.pchText)
	{
		tracker.pchText = pchTextDup;
		
		if (!pchFilename)
		{
			tracker.id = idx++;
			eaPush(&eaErrors, &tracker);
		}
	}
	else
		free((void*)pchTextDup);

	if (pchFilename)
	{
		const TCHAR* pchTextDup = _wcsdup(pchFilename);
		ErrorTracker& filenameTracker = tracker.stFilenameToTracker[pchTextDup];
		
		filenameTracker.Count++;
		filenameTracker.pParent = &tracker;

		if (!filenameTracker.pchText)
		{
			filenameTracker.pchText = pchTextDup;
			filenameTracker.id = idx++;
			eaPush(&eaErrors, &filenameTracker);
		}
		else
			free((void*)pchTextDup);

		SendMessage(hErrorDlg, WM_USER, (WPARAM)&filenameTracker, filenameTracker.Count);

		if (filenameTracker.bForceAssert != 0)
		{
			if (filenameTracker.bForceAssert == 1)
				filenameTracker.bForceAssert = 0;

			assert(0);
		}
	}
	else
	{
		SendMessage(hErrorDlg, WM_USER, (WPARAM)&tracker, tracker.Count);
		
		if (tracker.bForceAssert != 0)
		{
			if (tracker.bForceAssert == 1)
				tracker.bForceAssert = 0;

			assert(0);
		}
	}

}

#else


void ErrorInternalf(const TCHAR* pchErrorFmt, const TCHAR* pchFilename, ...)
{
	va_list args;
	va_start(args, pchFilename);
	vwprintf(pchErrorFmt, args);
	va_end(args);
}

#endif