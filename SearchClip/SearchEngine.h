//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

struct SearchEngine
{
    //////////////////////////////////////////////////////////////////////

    SearchEngine(wstring const &name, wstring const &formatString, bool isCustom);

    wstring const &Name() const;
    wstring const &FormatString() const;
    bool IsCurrent() const;
    bool IsCustom() const;

    void SetFormatString(wstring const &format);
    void SetCurrent();

    //////////////////////////////////////////////////////////////////////

    static void InitSearchEngines();
    static void SetCurrent(int index);
    static int GetCurrentIndex();
    static SearchEngine &GetCurrent();
    static vector<SearchEngine> &All();

    //////////////////////////////////////////////////////////////////////

private:
    wstring mName;
    wstring mFormatString;
    bool mIsCustom;
    bool mIsCurrentChoice;

    static int sCurrentSearchEngine;
    static vector<SearchEngine> sAllSearchEngines;
};
