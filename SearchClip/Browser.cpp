//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Browser.h"
#include "RegKey.h"

//////////////////////////////////////////////////////////////////////

vector<Browser>		Browser::sAllBrowsers;
Browser *			Browser::sDefaultBrowser = NULL;
Browser *			Browser::sCustomBrowser = NULL;
int					Browser::sCurrentBrowser = -1;

//////////////////////////////////////////////////////////////////////

Browser::Browser(wstring const &name, wstring const &exe, wstring const &commandLine)
	: mName(name)
	, mExecutableFilename(exe)
	, mCommandLine(commandLine)
	, mIsDefault(false)
	, mIsCustom(false)
	, mIsCurrent(false)
{
}

//////////////////////////////////////////////////////////////////////

Browser::~Browser()
{
}

//////////////////////////////////////////////////////////////////////

wstring const &Browser::Name() const
{
	return mName;
}

//////////////////////////////////////////////////////////////////////

wstring const &Browser::ExecutableFilename() const
{
	return mExecutableFilename;
}

//////////////////////////////////////////////////////////////////////

wstring const &Browser::CommandLine() const
{
	return mCommandLine;
}

void Browser::SetCommandLine(wstring const &commandLine)
{
	mCommandLine = commandLine;
}

//////////////////////////////////////////////////////////////////////

bool Browser::IsCustom() const
{
	return mIsCustom;
}

//////////////////////////////////////////////////////////////////////

int Browser::GetCurrentIndex()
{
	return sCurrentBrowser;
}

//////////////////////////////////////////////////////////////////////

Browser &Browser::GetCurrent()
{
	assert(!sAllBrowsers.empty());

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
		if(Browser::sAllBrowsers[i].Name().compare(name) == 0)
		{
			return i;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////

Browser &Browser::Get(int id)
{
	return sAllBrowsers[id];
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
	sAllBrowsers.clear();
	RegKey r(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Clients\\StartMenuInternet"));
	wstring defaultBrowser;
	r.GetStringValue(NULL, defaultBrowser);

	vector<wstring> browserNames;
	r.EnumKeys(browserNames);
	int defaultID = 0;
	for(DWORD i=0; i<browserNames.size(); ++i)
	{
		RegKey b(r, browserNames[i].c_str());
		wstring name;
		if(b.GetStringValue(NULL, name))
		{
			RegKey cmd(b, TEXT("shell\\open\\command"));
			wstring command;
			if(cmd.GetStringValue(NULL, command))
			{
				sAllBrowsers.push_back(Browser(name.c_str(), command.c_str(), TEXT("")));
				if(browserNames[i].compare(defaultBrowser) == 0)
				{
					defaultID = (int)sAllBrowsers.size() - 1;
					sAllBrowsers.back().mIsDefault = true;
					sDefaultBrowser = &sAllBrowsers.back();
				}
			}
		}
	}
	Browser custom(TEXT("Custom"), TEXT(""), TEXT(""));
	custom.mIsCustom = true;
	sAllBrowsers.push_back(custom);
	sCustomBrowser = &sAllBrowsers.back();
	
	assert(defaultID != -1);
	if(defaultID == -1)
	{
		defaultID = 0;
	}

	Browser::SetCurrent(defaultID);
}

//////////////////////////////////////////////////////////////////////

void Browser::ChooseCustomBrowserExecutable(HWND parentWindow)
{
	assert(sCustomBrowser != NULL);

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
