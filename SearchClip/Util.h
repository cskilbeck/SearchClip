//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

wstring Format(WCHAR const *fmt, ...);
wstring Format(wstring const &fmt, ...);
wstring GString(DWORD id);
wstring Replace(wstring const &str, wstring const &from, wstring const &to);
wstring rtrim(wstring const &str);
wstring trim(wstring const &str);
wstring ltrim(wstring const &str);
wstring GetExecutableFilename();
wstring UpperCase(wstring const &str);
bool GetClipboardAsString(wstring &str);
wstring URLSanitize(wstring const &str);

//////////////////////////////////////////////////////////////////////

#define TRACE(X, ...) OutputDebugString(Format((X), ##__VA_ARGS__).c_str())

#include "..\\Util\\log.h"