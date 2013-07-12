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

int Browser::Find(wstring const &name)
{
	for(size_t i=0; i<sAllBrowsers.size(); ++i)
	{
		Browser &b = Browser::sAllBrowsers[i];
		if(name.compare(b.Name()) == 0)
		{
			return i;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////

vector<Browser> &Browser::AllBrowsers()
{
	return sAllBrowsers;
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
	TRACE(TEXT("Scanning browsers:\n"));
	sAllBrowsers.clear();
	RegKey r(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Clients\\StartMenuInternet"));
	wstring defaultBrowser;
	r.GetStringValue(NULL, defaultBrowser);
	TRACE(TEXT("Default is %s\n"), defaultBrowser.c_str());

	vector<wstring> browserNames;
	r.EnumKeys(browserNames);
	int defaultID = 0;
	for(DWORD i=0; i<browserNames.size(); ++i)
	{
		RegKey b(r, browserNames[i].c_str());
		wstring name;
		if(b.GetStringValue(NULL, name))
		{
			TRACE(TEXT("%d: %s"), i, name.c_str());
			RegKey cmd(b, TEXT("shell\\open\\command"));
			wstring command;
			if(cmd.GetStringValue(NULL, command))
			{
				sAllBrowsers.push_back(Browser(name.c_str(), command.c_str(), TEXT("")));
				if(browserNames[i].compare(defaultBrowser) == 0)
				{
					defaultID = (int)sAllBrowsers.size() - 1;
					TRACE(TEXT(" (is default %d)"), defaultID);
					sAllBrowsers.back().mIsDefault = true;
					sDefaultBrowser = &sAllBrowsers.back();
				}
			}
			TRACE(TEXT("\n"));
		}
	}
	Browser custom(TEXT("Custom"), TEXT(""), TEXT(""));
	custom.mIsCustom = true;
	sAllBrowsers.push_back(custom);
	sCustomBrowser = &sAllBrowsers.back();
	Browser::SetCurrent(defaultID);
	TRACE(TEXT("Current browser is %s\n"), Browser::GetCurrent().mName.c_str());
}

//////////////////////////////////////////////////////////////////////

void Browser::ChooseCustomBrowserExecutable(HWND parentWindow)
{
	OPENFILENAME ofn = { 0 };
	WCHAR filename[MAX_PATH] = { 0 };

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = parentWindow;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = TEXT("Executable files\0*.exe\0");
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAYSIZE(filename);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT;
	ofn.lpstrInitialDir = TEXT("%PROGRAMFILES%");
	if(GetOpenFileName(&ofn) == IDOK)
	{
		sCustomBrowser->mExecutableFilename = ofn.lpstrFile;
	}
}
