//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Browser.h"
#include "RegKey.h"
#include <versionhelpers.h>

//////////////////////////////////////////////////////////////////////

vector<Browser> Browser::sAllBrowsers;
Browser *Browser::sDefaultBrowser = NULL;
Browser *Browser::sCustomBrowser = NULL;
int Browser::sCurrentBrowser = -1;

//////////////////////////////////////////////////////////////////////

Browser::Browser(wstring const &name, wstring const &exe, wstring const &commandLine)
    : mName(name), mExecutableFilename(exe), mCommandLine(commandLine), mIsDefault(false), mIsCustom(false), mIsCurrent(false)
{
    Log_D(L"Browser ctor (%s)", name.c_str());
}

//////////////////////////////////////////////////////////////////////

Browser::~Browser()
{
    Log_D(L"Browser dtor (%s)", mName.c_str());
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

    if(sCurrentBrowser != -1) {
        return sAllBrowsers[sCurrentBrowser];
    } else {
        return sAllBrowsers[0];
    }
}

//////////////////////////////////////////////////////////////////////

int Browser::Find(wstring const &name)
{
    for(size_t i = 0; i < sAllBrowsers.size(); ++i) {
        if(Browser::sAllBrowsers[i].Name().compare(name) == 0) {
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
    Log_D(L"Setting to %d (%s)", index, GetCurrent().ExecutableFilename().c_str());
}

//////////////////////////////////////////////////////////////////////

// Need to know if we're running on Windows 7 and before or later
// For Windows 7, default browser is in HKCU\SOFTWARE\Clients\StartMenuInternet
// For Windows 10, APPLICATION = Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\Shell\Associations\UrlAssociations\http\UserChoice
// Computer\HKEY_CLASSES_ROOT\{APPLICATION}\shell\open\command
// Some grimness remains, the way in which searches are launched is browser specific

namespace
{
bool are_equal_case_insensitive(wstring const &a, wstring const &b)
{
    unsigned int sz = a.size();
    if(b.size() != sz) {
        return false;
    }
    for(unsigned int i = 0; i < sz; ++i) {
        if(tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }
    return true;
}
}

void Browser::ScanRegistryForBrowsers()
{
    sAllBrowsers.clear();

    int defaultID = 0;

    Log_V("Begin");

    // default browser under Windows 7 SP1 and earlier
    wstring default_browser_executable;
    RegKey r(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Clients\\StartMenuInternet"));

    // for Windows 8 and later, default browser is handled differently
    // this is tested only on Windows 10 because very few people use Windows 8
    if(IsWindows8OrGreater()) {
        Log_I("It's Win8 or later...");
        wstring user_choice;
        RegKey user_choice_key(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice");
        if(user_choice_key.GetStringValue(L"ProgId", user_choice)) {
            RegKey default_exe(HKEY_CLASSES_ROOT, Format(L"%s\\shell\\open\\command", user_choice.c_str()));
            default_exe.GetStringValue(NULL, default_browser_executable);
            // strip the executable off the front of the string
        }
    } else {
        Log_I("It's Win7 SP1 or earlier...");
        wstring default_browser_name;
        if(r.GetStringValue(NULL, default_browser_name)) {
            RegKey r2(HKEY_LOCAL_MACHINE, Format(L"SOFTWARE\\Clients\\StartMenuInternet\\%s\\shell\\open\\command", default_browser_name.c_str()));
            r2.GetStringValue(NULL, default_browser_executable);
        }
    }

    // if it starts with a quote, extract what's in the quotes
    // otherwise strip anything after first whitespace
    // which is super shady, but we're not in the land of contracts here

    if(default_browser_executable[0] == '"') {
        size_t offset = default_browser_executable.find('"', 1);    // assuming there are no escaped ones in the filename, not sure there even can be?
        if(offset != wstring::npos) {
            default_browser_executable = default_browser_executable.substr(0, offset + 1);
        }
    } else {
        int index = 0;
        for(auto c : default_browser_executable) {
            if(c == ' ') {
                default_browser_executable = default_browser_executable.substr(0, index);
                break;
            }
            index += 1;
        }
    }

    Log_I(L"Default browser executable: %s", default_browser_executable.c_str());

    vector<wstring> browserNames;
    r.EnumKeys(browserNames);
    for(DWORD i = 0; i < browserNames.size(); ++i) {
        RegKey b(r, browserNames[i].c_str());
        wstring name;
        if(b.GetStringValue(NULL, name)) {
            RegKey cmd(b, TEXT("shell\\open\\command"));
            wstring command;
            if(cmd.GetStringValue(NULL, command)) {
                sAllBrowsers.push_back(Browser(name.c_str(), command.c_str(), TEXT("")));

                // comparing the executable path feels a bit shady but not sure how else to do it?
                if(are_equal_case_insensitive(command, default_browser_executable)) {
                    Log_V("Found default browser");
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
    if(defaultID == -1) {
        Log_E(L"No default browser, defaulting to %s", sAllBrowsers[0].ExecutableFilename().c_str());
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
    if(GetOpenFileName(&ofn) == IDOK) {
        sCustomBrowser->mExecutableFilename = ofn.lpstrFile;
    }
}
