// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define ISOLATION_AWARE_ENABLED 1

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <CommCtrl.h>
#include <Uxtheme.h>
#include <objbase.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <algorithm>


// TODO: reference additional headers your program requires here
#include "tld.h"

using std::wstring;
using std::vector;

wstring Format(WCHAR const *fmt, ...);

#define TRACE(X, ...) OutputDebugString(Format((X), ##__VA_ARGS__).c_str())