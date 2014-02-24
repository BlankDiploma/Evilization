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

void SetErrorListCount(HWND hListControl, ErrorTracker* pTracker)
{
	//int i = 0;
	static TCHAR buf[10] = {0};

	wsprintf(buf, L"%d", pTracker->Count);
	ListView_SetItemText(hListControl,pTracker->id,1,buf);
}

BOOL CALLBACK ErrorProc(HWND hwndDlg, 
                             UINT message, 
                             WPARAM wParam, 
                             LPARAM lParam) 
{ 
    switch (message) 
    { 
        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
    //                if (!GetDlgItemText(hwndDlg, ID_ITEMNAME, szItemName, 80)) 
     //                    *szItemName=0; 
 
                    // Fall through. 
 
                case IDCANCEL: 
                    EndDialog(hwndDlg, wParam); 
                    return TRUE; 
            }break;
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
				}
		}break;
	}
    return FALSE; 
} 

void ErrorInternalf(const TCHAR* pchErrorFmt, const TCHAR* pchFilename, ...)
{
	static TCHAR errorText[512] = {0};
	if (!IsWindow(hErrorDlg))
	{
		hErrorDlg = CreateDialog(hInst, (LPCTSTR)IDD_ERRORDIALOG, hWndMain, (DLGPROC)ErrorProc);
		ShowWindow(hErrorDlg, 1);
	}
	va_list args;
	va_start(args, pchFilename);
	wvsprintf(errorText, pchErrorFmt, args);
	va_end(args);
	ErrorTracker& tracker = stErrorTextToTracker[errorText];

	tracker.Count++;
	
	if (!tracker.pchText)
	{
		tracker.pchText = _wcsdup(errorText);
		
		if (!pchFilename)
		{
			tracker.id = idx++;
			eaPush(&eaErrors, &tracker);
		}
	}

	if (pchFilename)
	{
		ErrorTracker& filenameTracker = tracker.stFilenameToTracker[pchFilename];
		
		filenameTracker.Count++;
		filenameTracker.pParent = &tracker;

		if (!filenameTracker.pchText)
		{
			filenameTracker.pchText = _wcsdup(pchFilename);
			filenameTracker.id = idx++;
			eaPush(&eaErrors, &filenameTracker);
		}

		SendMessage(hErrorDlg, WM_USER, (WPARAM)&filenameTracker, filenameTracker.Count);
	}
	else
		SendMessage(hErrorDlg, WM_USER, (WPARAM)&tracker, tracker.Count);
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