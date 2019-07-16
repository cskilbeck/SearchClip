//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "SearchEngine.h"

//////////////////////////////////////////////////////////////////////

vector<SearchEngine> SearchEngine::sAllSearchEngines;
int SearchEngine::sCurrentSearchEngine = 0;

//////////////////////////////////////////////////////////////////////

SearchEngine::SearchEngine(wstring const &name, wstring const &formatString, bool isCustom) : mName(name), mFormatString(formatString), mIsCustom(isCustom)
{
}

//////////////////////////////////////////////////////////////////////

void SearchEngine::InitSearchEngines()
{
    sAllSearchEngines.push_back(SearchEngine(GString(IDS_SEARCHNAME_DEFAULT), GString(IDS_SEARCHFORMAT_DEFAULT), false));
    sAllSearchEngines.push_back(SearchEngine(GString(IDS_SEARCHNAME_GOOGLE), GString(IDS_SEARCHFORMAT_GOOGLE), false));
    sAllSearchEngines.push_back(SearchEngine(GString(IDS_SEARCHNAME_BING), GString(IDS_SEARCHFORMAT_BING), false));
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
