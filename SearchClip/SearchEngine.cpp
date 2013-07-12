//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SearchEngine.h"

//////////////////////////////////////////////////////////////////////

vector<SearchEngine> SearchEngine::sAllSearchEngines;
int SearchEngine::sCurrentSearchEngine = 0;

//////////////////////////////////////////////////////////////////////

void SearchEngine::InitSearchEngines()
{
	sAllSearchEngines.push_back(SearchEngine(L"Bing", L"http://www.bing.com/search?q=${CLIP}&src=IE-SearchBox&FORM=IE10S", false));
	sAllSearchEngines.push_back(SearchEngine(L"Google", L"HTTPS://www.google.com/webhp?ie=UTF8#output=search&q=${CLIP}", false));
	sAllSearchEngines.push_back(SearchEngine(L"Custom", L"", true));
	SetCurrent(0);
}

//////////////////////////////////////////////////////////////////////

void SearchEngine::SetCurrent(int index)
{
	GetCurrent().mIsCurrentChoice = false;
	sCurrentSearchEngine = index;
	GetCurrent().mIsCurrentChoice = true;
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