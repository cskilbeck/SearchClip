//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SearchEngine.h"

//////////////////////////////////////////////////////////////////////

vector<SearchEngine> SearchEngine::sAllSearchEngines;
int SearchEngine::sCurrentSearchEngine = 0;

//////////////////////////////////////////////////////////////////////

SearchEngine::SearchEngine(WCHAR const *name, WCHAR const *formatString, bool isCustom)
	: mName(name)
	, mFormatString(formatString)
	, mIsCustom(isCustom)
{
}

//////////////////////////////////////////////////////////////////////

void SearchEngine::InitSearchEngines()
{
	sAllSearchEngines.push_back(SearchEngine(TEXT("Bing"), TEXT("http://www.bing.com/search?q=${CLIP}&src=IE-SearchBox&FORM=IE10S"), false));
	sAllSearchEngines.push_back(SearchEngine(TEXT("Google"), TEXT("HTTPS://www.google.com/webhp?ie=UTF8#output=search&q=${CLIP}"), false));
	sAllSearchEngines.push_back(SearchEngine(TEXT("Custom"), TEXT(""), true));
	SetCurrent(0);
}

//////////////////////////////////////////////////////////////////////

wstring const &SearchEngine::Name() const
{
	return mName;
}

//////////////////////////////////////////////////////////////////////

void SearchEngine::SetCurrent(int index)
{
	sAllSearchEngines[sCurrentSearchEngine].mIsCurrentChoice = false;
	sCurrentSearchEngine = index;
	sAllSearchEngines[sCurrentSearchEngine].mIsCurrentChoice = true;
}

//////////////////////////////////////////////////////////////////////

SearchEngine &SearchEngine::GetCurrent()
{
	return sAllSearchEngines[sCurrentSearchEngine];
}

//////////////////////////////////////////////////////////////////////

int SearchEngine::GetCurrentIndex()
{
	return sCurrentSearchEngine;
}

//////////////////////////////////////////////////////////////////////

wstring const &SearchEngine::FormatString() const
{
	return mFormatString;
}

void SearchEngine::SetFormatString(wstring const &format)
{
	mFormatString = format;
}

//////////////////////////////////////////////////////////////////////

bool SearchEngine::IsCurrent() const
{
	return mIsCurrentChoice;
}

void SearchEngine::SetCurrent()
{
	mIsCurrentChoice = true;
}

//////////////////////////////////////////////////////////////////////

bool SearchEngine::IsCustom() const
{
	return mIsCustom;
}

vector<SearchEngine> &SearchEngine::All()
{
	return sAllSearchEngines;
}
