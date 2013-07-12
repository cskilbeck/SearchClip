//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

struct Browser
{
	//////////////////////////////////////////////////////////////////////

	Browser(wstring const &name, wstring const &exe, wstring const &commandLine)
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

	wstring const &Name() const
	{
		return mName;
	}

	//////////////////////////////////////////////////////////////////////

	wstring const &ExecutableFilename() const
	{
		return mExecutableFilename;
	}

	//////////////////////////////////////////////////////////////////////

	wstring const &CommandLine() const
	{
		return mCommandLine;
	}

	void SetCommandLine(wstring const &commandLine)
	{
		mCommandLine = commandLine;
	}

	//////////////////////////////////////////////////////////////////////

	bool IsCustom() const
	{
		return mIsCustom;
	}

	//////////////////////////////////////////////////////////////////////

	static void					ScanRegistryForBrowsers();
	static void					ChooseCustomBrowserExecutable(HWND parentWindow);

	static void					SetCurrent(int index);
	static int					GetCurrentIndex();
	static Browser &			GetCurrent();

	static int					Find(wstring const &name);

	static vector<Browser> &	AllBrowsers();

	//////////////////////////////////////////////////////////////////////

private:

	bool					mIsCurrent;
	bool					mIsCustom;
	bool					mIsDefault;
	wstring					mName;
	wstring					mExecutableFilename;
	wstring					mCommandLine;

	static vector<Browser>	sAllBrowsers;
	static Browser *		sDefaultBrowser;
	static Browser *		sCustomBrowser;
	static int				sCurrentBrowser;
};
