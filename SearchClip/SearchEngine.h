//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

struct SearchEngine
{
	//////////////////////////////////////////////////////////////////////

	SearchEngine(WCHAR const *name, WCHAR const *formatString, bool isCustom)
		: mName(name)
		, mFormatString(formatString)
		, mIsCustom(isCustom)
	{
	}

	//////////////////////////////////////////////////////////////////////

	static vector<SearchEngine> sAllSearchEngines;
	static void InitSearchEngines();
	static void SetCurrent(int index);
	static int GetCurrentIndex();
	static SearchEngine &GetCurrent();

	//////////////////////////////////////////////////////////////////////

	wstring		mName;
	wstring		mFormatString;
	bool		mIsCustom;
	bool		mIsCurrentChoice;

private:

	static int	sCurrentSearchEngine;

};

