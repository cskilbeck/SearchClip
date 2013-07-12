//////////////////////////////////////////////////////////////////////
// Save and load the options...
// Current Browser (incl. path if custom)
// Search Engine (incl. format string if custom)
// Key stuff

#include "stdafx.h"
#include "LaunchBrowser.h"
#include "KeyboardHook.h"
#include "RegKey.h"
#include "Browser.h"
#include "SearchEngine.h"

//////////////////////////////////////////////////////////////////////

#define MAX_LOADSTRING 100

//////////////////////////////////////////////////////////////////////

HINSTANCE			hInst;									// current instance
wstring				szTitle;								// The title bar text
wstring				szWindowClass;							// the main window class name
NOTIFYICONDATA		niData;									// For the little notification icon
HMENU				hPopupMenu;
BOOL				bIsFirstRun = TRUE;
wstring				OptionsKeyName;

//////////////////////////////////////////////////////////////////////

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL				LoadOptionsFromRegistry();
void				LaunchBrowser();

//////////////////////////////////////////////////////////////////////

wstring LoadStringResource(HINSTANCE hInst, DWORD id)
{
	WCHAR str[MAX_LOADSTRING];
	LoadString(hInst, id, str, MAX_LOADSTRING);
	return wstring(str);
}

//////////////////////////////////////////////////////////////////////

wstring Format(WCHAR const *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	WCHAR buffer[8192];
	_vsnwprintf_s(buffer, ARRAYSIZE(buffer) - 1, fmt, v);
	return wstring(buffer);
}

//////////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	szTitle = LoadStringResource(hInstance, IDS_APP_TITLE);
	szWindowClass = LoadStringResource(hInstance, IDC_LAUNCHBROWSER);

	OptionsKeyName = wstring(L"Software\\") + szTitle;
	
	TRACE(L"Loaded...\n");

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAUNCHBROWSER));

	Browser::ScanRegistryForBrowsers();
	SearchEngine::InitSearchEngines();

	double mouseDoubleClickTime = GetDoubleClickTime() / 1000.0;

	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, fnKeyboardHook, hInstance, 0);

	// If this is the first time it's been run (registry entries missing), then pop the options dialog

	if(!LoadOptionsFromRegistry())
	{
		PostMessage(hMainWindow, WM_USER+1, 0, 0);
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//   FUNCTION: Replace
//
//   PURPOSE: Search/Replace in a string
//
//   COMMENTS:
//
wstring Replace(wstring & str, WCHAR const *from, WCHAR const *to)
{
	size_t start_pos = 0;
	wstring newStr = str;
	size_t len = wcslen(to);
    while((start_pos = newStr.find(from, start_pos)) != wstring::npos)
	{
        newStr.replace(start_pos, wcslen(from), to);
        start_pos += len;
    }
    return newStr;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.lpfnWndProc	= WndProc;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAUNCHBROWSER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LAUNCHBROWSER);
	wcex.lpszClassName	= szWindowClass.c_str();
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass.c_str(), szTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, SW_HIDE, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   hMainWindow = hWnd;

   //ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

BOOL LoadOptionsFromRegistry()
{
	RegKey options(HKEY_CURRENT_USER, OptionsKeyName.c_str(), KEY_READ|KEY_CREATE_SUB_KEY|KEY_SET_VALUE);

	TRACE(L"Loading options...\n");

	DWORD optionsSet;
	if(!(options.GetDWORDValue(L"OptionsSet", optionsSet) && optionsSet == 1))
	{
		TRACE(L"No options yet, showing options dialog\n");
		return FALSE;
	}

	wstring browserName;
	if(options.GetStringValue(L"Browser", browserName))
	{
		TRACE(L"Browser: %s\n", browserName.c_str());
		int currentBrowserIndex = -1;
		for(size_t i=0; i<Browser::sAllBrowsers.size(); ++i)
		{
			Browser &b = Browser::sAllBrowsers[i];
			if(browserName.compare(b.mName) == 0)
			{
				currentBrowserIndex = (int)i;
			}
			else
			{
				b.mIsCurrent = false;
			}
		}
		if(currentBrowserIndex != -1)
		{
			Browser::SetCurrent(currentBrowserIndex);
			options.GetStringValue(L"BrowserOptions", Browser::GetCurrent().mCommandLine);
			TRACE(L"(found it, options: [%s])\n", Browser::GetCurrent().mCommandLine.c_str());
		}
	}

	wstring searchName;
	if(options.GetStringValue(L"SearchEngine", searchName))
	{
		TRACE(L"Search engine is %s\n", searchName.c_str());
		int currentIndex = 0;
		for(size_t i=0; i<SearchEngine::sAllSearchEngines.size(); ++i)
		{
			SearchEngine &s = SearchEngine::sAllSearchEngines[i];
			if(searchName.compare(s.mName) == 0)
			{
				currentIndex = i;
				TRACE(L"(found it: format: [%s])\n", s.mFormatString.c_str()); 
				break;
			}
			else
			{
				s.mIsCurrentChoice = false;
			}
		}
		SearchEngine::SetCurrent(currentIndex);
		options.GetStringValue(L"SearchFormat", SearchEngine::GetCurrent().mFormatString);
	}
	return TRUE;
}

wstring GetExecutableFilename()
{
	WCHAR buffer[32768];
	buffer[0] = 0;
	buffer[ARRAYSIZE(buffer) - 1] = 0;
	GetModuleFileName(hInst, buffer, ARRAYSIZE(buffer) - 1);
	return wstring(buffer);
}

void SaveOptions(HWND dlg)
{
	WCHAR buffer[16384];

	TRACE(L"Saving options:\n");

	HWND searchCombo = GetDlgItem(dlg, IDC_COMBO_SEARCHENGINE);
	int searchIndex = ComboBox_GetCurSel(searchCombo);
	TRACE(L"Search Combo index = %d\n", searchIndex);
	SearchEngine *currentSearchEngine = (SearchEngine *)ComboBox_GetItemData(searchCombo, searchIndex);
	if(currentSearchEngine != NULL)
	{
		TRACE(L"which is %s\n", currentSearchEngine->mName.c_str());
		SearchEngine::SetCurrent(searchIndex);
		GetDlgItemText(dlg, IDC_EDIT_SEARCHENGINEFORMAT, buffer, ARRAYSIZE(buffer));
		SearchEngine::GetCurrent().mFormatString = buffer;
	}

	HWND browserCombo = GetDlgItem(dlg, IDC_COMBO_BROWSERCHOICE);
	int browserIndex = ComboBox_GetCurSel(browserCombo);
	TRACE(L"Browser index = %d\n", browserIndex);
	Browser::SetCurrent(browserIndex);
	GetDlgItemText(dlg, IDC_EDIT_BROWSERPARAMETERS, buffer, ARRAYSIZE(buffer));
	Browser::GetCurrent().mCommandLine = buffer;

	RegKey r(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_READ|KEY_SET_VALUE);
	if(IsDlgButtonChecked(dlg, IDC_CHECK_RUN_AT_STARTUP))
	{
		wstring exe = GetExecutableFilename();
		r.SetStringValue(szTitle.c_str(), exe.c_str());
		TRACE(L"Run at startup selected, exe is [%s]\n", exe.c_str());
	}
	else
	{
		TRACE(L"Run at startup not selected, clearing key\n");
		r.DeleteValue(szTitle.c_str());
	}

	RegKey options;
	options.Create(HKEY_CURRENT_USER, OptionsKeyName.c_str(), KEY_READ|KEY_CREATE_SUB_KEY|KEY_SET_VALUE);
	options.SetDWORDValue(L"OptionsSet", 1);
	options.SetStringValue(L"Browser", Browser::GetCurrent().mName.c_str());
	options.SetStringValue(L"BrowserOptions", Browser::GetCurrent().mCommandLine.c_str());
	options.SetStringValue(L"SearchEngine", SearchEngine::GetCurrent().mName.c_str());
	options.SetStringValue(L"SearchFormat", SearchEngine::GetCurrent().mFormatString.c_str());
}

void UpdateOptionsDialogText(HWND dlg)
{
	HWND searchEdit = GetDlgItem(dlg, IDC_EDIT_SEARCHENGINEFORMAT);
	SetDlgItemText(dlg, IDC_EDIT_SEARCHENGINEFORMAT, SearchEngine::GetCurrent().mFormatString.c_str());
	SetDlgItemText(dlg, IDC_STATIC_BROWSERPATH, Browser::GetCurrent().mExecutableFilename.c_str());
	Edit_SetReadOnly(searchEdit, !SearchEngine::GetCurrent().mIsCustom);
}

//
//  FUNCTION: DialogProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the options dialog
//
//
LRESULT CALLBACK DialogProc(HWND hWndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			TRACE(L"Init options dialog\n");
			HWND browserCombo = GetDlgItem(hWndDialog, IDC_COMBO_BROWSERCHOICE);
			for(size_t i=0; i<Browser::sAllBrowsers.size(); ++i)
			{
				Browser &b = Browser::sAllBrowsers[i];
				int id = ComboBox_AddString(browserCombo, b.mName.c_str());
				ComboBox_SetItemData(browserCombo, id, &b);
			}
			ComboBox_SetCurSel(browserCombo, Browser::GetCurrentIndex());

			HWND searchCombo = GetDlgItem(hWndDialog, IDC_COMBO_SEARCHENGINE);
			for(size_t i=0; i<SearchEngine::sAllSearchEngines.size(); ++i)
			{
				SearchEngine &s = SearchEngine::sAllSearchEngines[i];
				int id = ComboBox_AddString(searchCombo, s.mName.c_str());
				ComboBox_SetItemData(searchCombo, id, &s);
			}
			ComboBox_SetCurSel(searchCombo, SearchEngine::GetCurrentIndex());
			UpdateOptionsDialogText(hWndDialog);

			RegKey r(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
			wstring appPath;
			CheckDlgButton(hWndDialog, IDC_CHECK_RUN_AT_STARTUP, r.GetStringValue(szTitle.c_str(), appPath) ? BST_CHECKED : BST_UNCHECKED);
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_COMBO_BROWSERCHOICE:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND browserCombo = GetDlgItem(hWndDialog, IDC_COMBO_BROWSERCHOICE);
				int id = ComboBox_GetCurSel(browserCombo);
				Browser *b = (Browser *)ComboBox_GetItemData(browserCombo, id);
				if(b->mIsCustom)
				{
					Browser::ChooseCustomBrowserExecutable(hWndDialog);
				}
				Browser::SetCurrent(id);
				UpdateOptionsDialogText(hWndDialog);
			}
			break;

		case IDC_COMBO_SEARCHENGINE:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND searchCombo = GetDlgItem(hWndDialog, IDC_COMBO_SEARCHENGINE);
				SearchEngine::SetCurrent(ComboBox_GetCurSel(searchCombo));
				UpdateOptionsDialogText(hWndDialog);
			}
			break;

		case IDCANCEL:
			EndDialog(hWndDialog, NULL);
			break;

		case IDOK:
			SaveOptions(hWndDialog);
			EndDialog(hWndDialog, NULL);
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWndDialog, NULL);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
			ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
			niData.cbSize = sizeof(NOTIFYICONDATA);
			niData.uID = 1;
			niData.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
			niData.hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_LAUNCHBROWSER), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
			niData.hWnd = hWnd;
			niData.uCallbackMessage = WM_APP;
			wcscpy_s(niData.szTip, ARRAYSIZE(niData.szTip)-1, TEXT( "LaunchBrowser - right click for menu"));
			niData.uTimeout = 15000;
			niData.uVersion = NOTIFYICON_VERSION;
			Shell_NotifyIcon(NIM_ADD, &niData);
			Shell_NotifyIcon(NIM_SETVERSION, &niData);
		}
		break;

	case WM_APP:
        switch(lParam)
        {
		case WM_CONTEXTMENU:
			{
				MENUITEMINFO separatorBtn = {0};
				separatorBtn.cbSize = sizeof(MENUITEMINFO);
				separatorBtn.fMask = MIIM_FTYPE;
				separatorBtn.fType = MFT_SEPARATOR;
				HMENU hMenu = CreatePopupMenu();
				if(hMenu != NULL)
				{
					InsertMenu(hMenu, -1, MF_BYPOSITION, ID_FILE_OPTIONS, TEXT("&Options"));
					InsertMenuItem(hMenu, -1, FALSE, &separatorBtn);
					InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_EXIT, TEXT("E&xit"));
					POINT pt;
					GetCursorPos(&pt);
					SetForegroundWindow(hWnd);
					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
					DestroyMenu(hMenu);
					PostMessage(hWnd, WM_NULL, 0, 0);
				}
			}
            break;
        }
		break;

	case WM_CLOSE:
		//ShowWindow(hWnd, SW_HIDE);
		break;

	case WM_COMMAND:
		{
			int wmId, wmEvent;
			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			switch (wmId)
			{
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			case IDM_SHOW:
				//ShowWindow(hWnd, SW_SHOW);
				break;
			case IDM_HIDE:
				//ShowWindow(hWnd, SW_HIDE);
				break;
			case ID_FILE_OPTIONS:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS), hWnd, (DLGPROC)DialogProc);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;

	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &niData);
		UnhookWindowsHookEx(hKeyboardHook);
		PostQuitMessage(0);
		break;

	case WM_USER+1:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS), hWnd, (DLGPROC)DialogProc);
		break;

	case WM_USER:
		LaunchBrowser();
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void LaunchBrowser()
{
	OpenClipboard(NULL);
	HANDLE clip = GetClipboardData(CF_UNICODETEXT);
	if(clip != NULL)
	{
		HANDLE h = GlobalLock(clip);
		if(h != NULL)
		{
			WCHAR const *c = (WCHAR *)clip;
			size_t length = wcsnlen_s(c, 4096);
			WCHAR *buffer = new WCHAR[length + 1];
			CopyMemory(buffer, c, sizeof(WCHAR) * length);
			buffer[length] = (WCHAR)0;
			GlobalUnlock(clip);

			wstring clip(buffer);
			wstring upr(clip);
			std::transform(upr.begin(), upr.end(), upr.begin(), ::toupper);

			bool isURL = false;

			// if it's go spaces in it, search
			if(upr.find(TEXT(" "), 0) != wstring::npos)
			{
			}

			// if it starts with http or https, url
			else if(upr.compare(0, 7, TEXT("HTTP://")) == 0)
			{
				isURL = true;
			}
			else if(clip.compare(0, 7, TEXT("HTTPS://")) == 0)
			{
				isURL = true;
			}
			// else look for a TLD
			else
			{
				WCHAR **tld = gTopLevelDomain;
				while(*tld != nullptr)
				{
					if(upr.find(*tld, 0) != wstring::npos)
					{
						isURL = true;
						break;
					}
					++tld;
				}
			}

			// either way, make it safe
			clip = Replace(clip, TEXT(" "), TEXT("%20"));
			clip = Replace(clip, TEXT("\\"), TEXT("%5C"));
			clip = Replace(clip, TEXT("&"), TEXT("%26"));

			// set up command line
			if(!isURL)
			{
				clip = Replace(SearchEngine::GetCurrent().mFormatString, L"${CLIP}", clip.c_str());
			}

			// fire off the browser
			STARTUPINFO startupInfo = { 0 };
			PROCESS_INFORMATION processInformation = { 0 };

			startupInfo.cb = sizeof(startupInfo);
			startupInfo.wShowWindow = SW_SHOWMAXIMIZED;
			startupInfo.dwFlags = STARTF_USESHOWWINDOW;

			WCHAR commandLine[16384];
			wcscpy_s(commandLine, (Browser::GetCurrent().mExecutableFilename + L" " + Browser::GetCurrent().mCommandLine + L" " + clip).c_str());

			CreateProcess(NULL,
				commandLine,
				NULL,
				NULL,
				FALSE,
				0L,
				NULL,
				NULL,
				&startupInfo,
				&processInformation);

			delete[] buffer;
		}
	}
	CloseClipboard();
}