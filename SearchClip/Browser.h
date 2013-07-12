//////////////////////////////////////////////////////////////////////

#pragma  once

//////////////////////////////////////////////////////////////////////

struct Browser
{
	//////////////////////////////////////////////////////////////////////

	Browser(wstring const &name, wstring const &exe, wstring const &commandLine);
	~Browser();

	wstring const &Name() const;
	wstring const &ExecutableFilename() const;
	wstring const &CommandLine() const;
	bool IsCustom() const;

	void SetCommandLine(wstring const &commandLine);

	//////////////////////////////////////////////////////////////////////

	static void					ScanRegistryForBrowsers();
	static void					ChooseCustomBrowserExecutable(HWND parentWindow);
	static void					SetCurrent(int index);
	static int					GetCurrentIndex();
	static Browser &			GetCurrent();
	static Browser &			Get(int id);
	static int					Find(wstring const &name);
	static vector<Browser> &	AllBrowsers();

	//////////////////////////////////////////////////////////////////////

private:

	wstring					mName;
	wstring					mExecutableFilename;
	wstring					mCommandLine;
	bool					mIsCurrent;
	bool					mIsCustom;
	bool					mIsDefault;

	static vector<Browser>	sAllBrowsers;
	static Browser *		sDefaultBrowser;
	static Browser *		sCustomBrowser;
	static int				sCurrentBrowser;
};
