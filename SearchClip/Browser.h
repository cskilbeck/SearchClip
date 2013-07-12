//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

struct Browser
{
	//////////////////////////////////////////////////////////////////////

	Browser(WCHAR const *name, WCHAR const *exe, WCHAR const *commandLine)
		: mName(name)
		, mExecutableFilename(exe)
		, mCommandLine(commandLine)
		, mIsDefault(false)
		, mIsCustom(false)
		, mIsCurrent(false)
	{
	}

	//////////////////////////////////////////////////////////////////////

	~Browser()
	{
	}

	//////////////////////////////////////////////////////////////////////

	static void ScanRegistryForBrowsers();
	static void ChooseCustomBrowserExecutable(HWND parentWindow);
	static void SetCurrent(int index);
	static Browser &GetCurrent();
	static int GetCurrentIndex();

	//////////////////////////////////////////////////////////////////////

	static vector<Browser>	sAllBrowsers;
	static Browser *		sDefaultBrowser;
	static Browser *		sCustomBrowser;

	//////////////////////////////////////////////////////////////////////

	wstring					mName;
	wstring					mExecutableFilename;
	wstring					mCommandLine;
	bool					mIsDefault;
	bool					mIsCustom;
	bool					mIsCurrent;

private:

	static int				sCurrentBrowser;
};
