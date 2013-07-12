//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Browser.h"
#include "RegKey.h"
#include <commdlg.h>

//////////////////////////////////////////////////////////////////////

vector<Browser>		Browser::sAllBrowsers;
Browser *			Browser::sDefaultBrowser = NULL;
Browser *			Browser::sCustomBrowser = NULL;
int					Browser::sCurrentBrowser = -1;

//////////////////////////////////////////////////////////////////////

int Browser::GetCurrentIndex()
{
	return sCurrentBrowser;
}

//////////////////////////////////////////////////////////////////////

Browser &Browser::GetCurrent()
{
	if(sCurrentBrowser != -1)
	{
		return sAllBrowsers[sCurrentBrowser];
	}
	else
	{
		return sAllBrowsers[0];
	}
}

//////////////////////////////////////////////////////////////////////

void Browser::SetCurrent(int index)
{
	GetCurrent().mIsCurrent = false;
	sCurrentBrowser = index;
	GetCurrent().mIsCurrent = true;
}

//////////////////////////////////////////////////////////////////////

void Browser::ScanRegistryForBrowsers()
{
	TRACE(L"Scanning browsers:\n");
	sAllBrowsers.clear();
	RegKey r(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Clients\\StartMenuInternet");
	wstring defaultBrowser;
	r.GetStringValue(NULL, defaultBrowser);
	TRACE(L"Default is %s\n", defaultBrowser.c_str());

	vector<wstring> browserNames;
	r.EnumKeys(browserNames);
	int defaultID = 0;
	for(DWORD i=0; i<browserNames.size(); ++i)
	{
		RegKey b(r, browserNames[i].c_str());
		wstring name;
		if(b.GetStringValue(NULL, name))
		{
			TRACE(L"%d: %s", i, name.c_str());
			RegKey cmd(b, L"shell\\open\\command");
			wstring command;
			if(cmd.GetStringValue(NULL, command))
			{
				sAllBrowsers.push_back(Browser(name.c_str(), command.c_str(), L""));
				if(browserNames[i].compare(defaultBrowser) == 0)
				{
					defaultID = (int)sAllBrowsers.size() - 1;
					TRACE(L" (is default %d)", defaultID);
					sAllBrowsers.back().mIsDefault = true;
					Browser::sDefaultBrowser = &sAllBrowsers.back();
				}
			}
			TRACE(L"\n");
		}
	}
	Browser custom(L"Custom", L"", L"");
	custom.mIsCustom = true;
	sAllBrowsers.push_back(custom);
	sCustomBrowser = &sAllBrowsers.back();
	Browser::SetCurrent(defaultID);
	TRACE(L"Current browser is %s\n", Browser::GetCurrent().mName.c_str());
}

//////////////////////////////////////////////////////////////////////

void Browser::ChooseCustomBrowserExecutable(HWND parentWindow)
{
	OPENFILENAME ofn = { 0 };
	WCHAR filename[MAX_PATH] = { 0 };

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = parentWindow;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = L"Executable files\0*.exe\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAYSIZE(filename);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT;
	ofn.lpstrInitialDir = L"%PROGRAMFILES%";
	if(GetOpenFileName(&ofn) == IDOK)
	{
		sCustomBrowser->mExecutableFilename = ofn.lpstrFile;
	}
}
