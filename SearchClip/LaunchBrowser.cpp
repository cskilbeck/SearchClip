//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LaunchBrowser.h"
#include "KeyboardHook.h"
#include "RegKey.h"
#include "Browser.h"
#include "SearchEngine.h"
#include "Common.h"

//////////////////////////////////////////////////////////////////////

HINSTANCE			hInst;						// current instance
wstring				szTitle;					// The title bar text
wstring				szWindowClass;				// the main window class name
NOTIFYICONDATA		niData;						// For the little notification icon
HMENU				hPopupMenu;					// Notification area context menu
wstring				OptionsKeyName;				// where the options are stored in the registry

//////////////////////////////////////////////////////////////////////

ATOM				RegisterWindowClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	OptionsDialogProc(HWND hWndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL				LoadOptionsFromRegistry();
void				SaveOptionsToRegistry(HWND dlg);
void				ShowOptionsDialog(HWND hWnd);
void				UpdateOptionsDialogText(HWND dlg);
void				InitOptionsDialog(HWND hWndDialog);
void				LaunchBrowser();
void				ShowContextMenu(HWND hWnd);
void				SetupNotificationIcon(HWND hWnd);

//////////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	szTitle = GString(IDS_APP_TITLE);
	szWindowClass = GString(IDC_LAUNCHBROWSER);
	OptionsKeyName = Format(TEXT("Software\\%s"), szTitle.c_str());
	
	// if it's running already, pop the options dialog
	HWND existingWindow = FindWindow(szWindowClass.c_str(), szTitle.c_str());
	if(existingWindow != NULL)
	{
		SendNotifyMessage(existingWindow, WM_COMMAND, ID_FILE_OPTIONS, 0);
		return FALSE;
	}

	RegisterWindowClass(hInstance);

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAUNCHBROWSER));

	Browser::ScanRegistryForBrowsers();
	SearchEngine::InitSearchEngines();

	MouseDoubleClickTime = GetDoubleClickTime() / 1000.0;

	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, fnKeyboardHook, hInstance, 0);

	if(LoadOptionsFromRegistry() == FALSE)
	{
		PostMessage(hMainWindow, WMU_POP_OPTIONS_DIALOG, 0, 0);
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

//////////////////////////////////////////////////////////////////////

ATOM RegisterWindowClass(HINSTANCE hInstance)
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

//////////////////////////////////////////////////////////////////////

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   hMainWindow = CreateWindow(szWindowClass.c_str(), szTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, SW_HIDE, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hMainWindow)
   {
      return FALSE;
   }

   UpdateWindow(hMainWindow);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////

BOOL LoadOptionsFromRegistry()
{
	RegKey options(HKEY_CURRENT_USER, OptionsKeyName, KEY_READ|KEY_CREATE_SUB_KEY|KEY_SET_VALUE);

	DWORD optionsSet;
	if(!(options.GetDWORDValue(GString(IDS_KEYNAME_OPTIONSSET), optionsSet) && optionsSet == 1))
	{
		return FALSE;
	}

	wstring browserName;
	if(options.GetStringValue(GString(IDS_KEYNAME_BROWSER), browserName))
	{
		int currentBrowserIndex = Browser::Find(browserName);
		if(currentBrowserIndex != -1)
		{
			Browser::SetCurrent(currentBrowserIndex);
			wstring commandLine;
			options.GetStringValue(GString(IDS_KEYNAME_BROWSEROPTIONS), commandLine);
			Browser::GetCurrent().SetCommandLine(commandLine);
		}
	}
	else
	{
		Browser::SetCurrent(0);
	}

	wstring searchName;
	if(options.GetStringValue(GString(IDS_KEYNAME_SEARCHENGINE), searchName))
	{
		int currentIndex = 0;
		for(auto s : SearchEngine::All())
		{
			if(searchName.compare(s.Name()) == 0)
			{
				SearchEngine::SetCurrent(currentIndex);
				break;
			}
			++currentIndex;
		}
		wstring formatString;
		options.GetStringValue(GString(IDS_KEYNAME_SEARCHFORMAT), formatString);
		SearchEngine::GetCurrent().SetFormatString(formatString);
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////

void SaveOptionsToRegistry(HWND dlg)
{
	WCHAR buffer[16384];

	HWND searchCombo = GetDlgItem(dlg, IDC_COMBO_SEARCHENGINE);
	SearchEngine::SetCurrent(ComboBox_GetCurSel(searchCombo));

	GetDlgItemText(dlg, IDC_EDIT_SEARCHENGINEFORMAT, buffer, ARRAYSIZE(buffer));
	SearchEngine::GetCurrent().SetFormatString(buffer);

	HWND browserCombo = GetDlgItem(dlg, IDC_COMBO_BROWSERCHOICE);
	Browser::SetCurrent(ComboBox_GetCurSel(browserCombo));

	GetDlgItemText(dlg, IDC_EDIT_BROWSERPARAMETERS, buffer, ARRAYSIZE(buffer));
	Browser::GetCurrent().SetCommandLine(buffer);

	RegKey r(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), KEY_READ|KEY_SET_VALUE);
	if(IsDlgButtonChecked(dlg, IDC_CHECK_RUN_AT_STARTUP))
	{
		r.SetStringValue(szTitle, GetExecutableFilename());
	}
	else
	{
		r.DeleteValue(szTitle);
	}

	RegKey options;
	options.Create(HKEY_CURRENT_USER, OptionsKeyName, KEY_READ|KEY_CREATE_SUB_KEY|KEY_SET_VALUE);
	options.SetDWORDValue(GString(IDS_KEYNAME_OPTIONSSET), 1);
	options.SetStringValue(GString(IDS_KEYNAME_BROWSER), Browser::GetCurrent().Name());
	options.SetStringValue(GString(IDS_KEYNAME_BROWSEROPTIONS), Browser::GetCurrent().CommandLine());
	options.SetStringValue(GString(IDS_KEYNAME_BROWSER_EXECUTABLE), Browser::GetCurrent().ExecutableFilename());
	options.SetStringValue(GString(IDS_KEYNAME_SEARCHENGINE), SearchEngine::GetCurrent().Name());
	options.SetStringValue(GString(IDS_KEYNAME_SEARCHFORMAT), SearchEngine::GetCurrent().FormatString());
}

//////////////////////////////////////////////////////////////////////

void ShowOptionsDialog(HWND hWnd)
{
	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS), hWnd, (DLGPROC)OptionsDialogProc);
}

//////////////////////////////////////////////////////////////////////

void UpdateOptionsDialogText(HWND dlg)
{
	HWND searchEdit = GetDlgItem(dlg, IDC_EDIT_SEARCHENGINEFORMAT);
	SetDlgItemText(dlg, IDC_EDIT_SEARCHENGINEFORMAT, SearchEngine::GetCurrent().FormatString().c_str());
	SetDlgItemText(dlg, IDC_STATIC_BROWSERPATH, Browser::GetCurrent().ExecutableFilename().c_str());
	Edit_SetReadOnly(searchEdit, !SearchEngine::GetCurrent().IsCustom());
}

//////////////////////////////////////////////////////////////////////

void InitOptionsDialog(HWND hWndDialog)
{
	SetWindowText(hWndDialog, (GString(IDS_APP_TITLE) + TEXT(" options")).c_str());
	HWND browserCombo = GetDlgItem(hWndDialog, IDC_COMBO_BROWSERCHOICE);
	for(auto b : Browser::AllBrowsers())
	{
		ComboBox_AddString(browserCombo, b.Name().c_str());
	}
	ComboBox_SetCurSel(browserCombo, Browser::GetCurrentIndex());

	HWND searchCombo = GetDlgItem(hWndDialog, IDC_COMBO_SEARCHENGINE);
	for(auto s : SearchEngine::All())
	{
		ComboBox_AddString(searchCombo, s.Name().c_str());
	}
	ComboBox_SetCurSel(searchCombo, SearchEngine::GetCurrentIndex());

	UpdateOptionsDialogText(hWndDialog);

	RegKey r(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"));
	wstring appPath;
	CheckDlgButton(hWndDialog, IDC_CHECK_RUN_AT_STARTUP, r.GetStringValue(szTitle, appPath) ? BST_CHECKED : BST_UNCHECKED);
}

//////////////////////////////////////////////////////////////////////

void SetupNotificationIcon(HWND hWnd)
{
	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = 1;
	niData.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
	niData.hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_LAUNCHBROWSER), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	niData.hWnd = hWnd;
	niData.uCallbackMessage = WM_APP;
	wcscpy_s(niData.szTip, ARRAYSIZE(niData.szTip)-1, (szTitle + GString(IDS_BALLOON)).c_str());
	niData.uTimeout = 15000;
	niData.uVersion = NOTIFYICON_VERSION;
	Shell_NotifyIcon(NIM_ADD, &niData);
	Shell_NotifyIcon(NIM_SETVERSION, &niData);
}

//////////////////////////////////////////////////////////////////////

void ShowContextMenu(HWND hWnd)
{
	MENUITEMINFO separatorBtn = {0};
	separatorBtn.cbSize = sizeof(MENUITEMINFO);
	separatorBtn.fMask = MIIM_FTYPE;
	separatorBtn.fType = MFT_SEPARATOR;
	HMENU hMenu = CreatePopupMenu();
	if(hMenu != NULL)
	{
		InsertMenu(hMenu, -1, MF_BYPOSITION, ID_FILE_OPTIONS, GString(IDS_MENU_OPTIONS).c_str());
		InsertMenuItem(hMenu, -1, FALSE, &separatorBtn);
		InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_EXIT, GString(IDS_MENU_EXIT).c_str());
		POINT pt;
		GetCursorPos(&pt);
		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
		DestroyMenu(hMenu);
		PostMessage(hWnd, WM_NULL, 0, 0);
	}
}

//////////////////////////////////////////////////////////////////////

void LaunchBrowser()
{
	wstring clip;
	if(GetClipboardAsString(clip))
	{
		if(!IsURL(clip))
		{
			clip = Replace(SearchEngine::GetCurrent().FormatString(), TEXT("${CLIP}"), clip);
		}

		clip = URLSanitize(clip);

		STARTUPINFO startupInfo = { 0 };
		PROCESS_INFORMATION processInformation = { 0 };

		startupInfo.cb = sizeof(startupInfo);
		startupInfo.wShowWindow = SW_SHOWMAXIMIZED;
		startupInfo.dwFlags = STARTF_USESHOWWINDOW;

		WCHAR commandLine[16384];
		wcscpy_s(commandLine, (Browser::GetCurrent().ExecutableFilename() + TEXT(" ") + Browser::GetCurrent().CommandLine() + TEXT(" ") + clip).c_str());

		CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0L, NULL, NULL, &startupInfo, &processInformation);
	}
}

//////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OptionsDialogProc(HWND hWndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		InitOptionsDialog(hWndDialog);
		BringWindowToTop(hWndDialog);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_COMBO_BROWSERCHOICE:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				HWND browserCombo = GetDlgItem(hWndDialog, IDC_COMBO_BROWSERCHOICE);
				Browser::SetCurrent(ComboBox_GetCurSel(browserCombo));
				if(Browser::GetCurrent().IsCustom())
				{
					Browser::ChooseCustomBrowserExecutable(hWndDialog);
				}
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
			LoadOptionsFromRegistry();
			EndDialog(hWndDialog, NULL);
			break;

		case IDOK:
			SaveOptionsToRegistry(hWndDialog);
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

//////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		SetupNotificationIcon(hWnd);
		break;

	case WM_APP:
        switch(lParam)
        {
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
            break;
        }
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
			case ID_FILE_OPTIONS:
				ShowOptionsDialog(hWnd);
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

	case WMU_POP_OPTIONS_DIALOG:
		ShowOptionsDialog(hWnd);
		break;

	case WMU_LAUNCH_BROWSER:
		LaunchBrowser();
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

