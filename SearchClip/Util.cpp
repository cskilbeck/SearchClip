//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

//////////////////////////////////////////////////////////////////////

wstring GString(DWORD id)
{
	WCHAR str[MAX_LOADSTRING];
	LoadString(GetModuleHandle(NULL), id, str, MAX_LOADSTRING);
	return str;
}

//////////////////////////////////////////////////////////////////////

wstring Format(WCHAR const *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	WCHAR buffer[8192];
	_vsnwprintf_s(buffer, ARRAYSIZE(buffer) - 1, fmt, v);
	return buffer;
}

//////////////////////////////////////////////////////////////////////

wstring Format(wstring const &str, ...)
{
	va_list v;
	wstring t(str);
	va_start(v, t);
	WCHAR buffer[8192];
	_vsnwprintf_s(buffer, ARRAYSIZE(buffer) - 1, str.c_str(), v);
	return buffer;
}

//////////////////////////////////////////////////////////////////////

wstring Replace(wstring const &str, wstring const &from, wstring const &to)
{
	size_t start_pos = 0;
	wstring newStr = str;
	size_t len = to.size();
    while((start_pos = newStr.find(from, start_pos)) != wstring::npos)
	{
        newStr.replace(start_pos, from.size(), to);
        start_pos += len;
    }
    return newStr;
}

//////////////////////////////////////////////////////////////////////

wstring GetExecutableFilename()
{
	WCHAR filename[32768];
	filename[0] = 0;
	filename[ARRAYSIZE(filename) - 1] = 0;
	GetModuleFileName(GetModuleHandle(NULL), filename, ARRAYSIZE(filename) - 1);
	return filename;
}

//////////////////////////////////////////////////////////////////////

wstring UpperCase(wstring const &str)
{
	wstring upr(str);
	std::transform(upr.begin(), upr.end(), upr.begin(), ::toupper);
	return upr;
}

//////////////////////////////////////////////////////////////////////
// trim from start

wstring ltrim(wstring const &str)
{
	wstring s(str);
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

//////////////////////////////////////////////////////////////////////
// trim from end

wstring rtrim(wstring const &str)
{
	wstring s(str);
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

//////////////////////////////////////////////////////////////////////
// trim from both ends

wstring trim(wstring const &str)
{
	return ltrim(rtrim(str));
}

//////////////////////////////////////////////////////////////////////

bool IsURL(wstring const &str)
{
	// if it has any spaces in it, it can't be a url (can it?)
	if(str.find(L" ", 0) != wstring::npos)
	{
		return false;
	}

	wstring upr(UpperCase(str));

	// if it begins with HTTP:// or HTTPS:// it's probably a URL
	if(upr.compare(0, 7, L"HTTP://") == 0 || upr.compare(0, 8, L"HTTPS://") == 0)
	{
		return true;
	}

	// Find everything up to the first / or ?
	size_t firstSlash = upr.find(L"/");
	if(firstSlash == wstring::npos)
	{
		firstSlash = upr.size();
	}
	size_t firstQ = upr.find(L"?");
	if(firstQ == wstring::npos)
	{
		firstQ = upr.size();
	}
	wstring upto = upr.substr(0, min(firstSlash, firstQ));

	size_t lastDot = upto.rfind(L".");
	if(lastDot == wstring::npos)
	{
		return false;
	}

	// TODO (charlie): the bit before that last dot must be 3 or more characters long to be a domain name

	WCHAR **tld = gTopLevelDomains;
	while(*tld != nullptr)
	{
		// find the tld in the string from the last dot position
		size_t f = upto.find(*tld, lastDot);
		if(f != wstring::npos)
		{
			// m.ac.com/!

			// check if the character following the match is at the end of the string or followed by a / or ?
			size_t l = wcslen(*tld);
			size_t us = upr.size();
			size_t ender = f + l;
			if(ender >= us || upr[ender] == L'/' || upr[ender] == L'?')
			{
				return true;
			}
		}
		++tld;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////

bool GetClipboardAsString(wstring &str)
{
	bool gotIt = false;
	OpenClipboard(NULL);
	HANDLE clip = GetClipboardData(CF_UNICODETEXT);
	if(clip != NULL)
	{
		HANDLE h = GlobalLock(clip);
		if(h != NULL)
		{
			WCHAR const *c = (WCHAR *)clip;
			size_t length = wcsnlen_s(c, 4096);
			WCHAR *buffer = new WCHAR[length + 1];
			CopyMemory(buffer, c, sizeof(WCHAR) * length);
			buffer[length] = (WCHAR)0;
			GlobalUnlock(clip);
			str = buffer;
			gotIt = true;
		}
	}
	CloseClipboard();
	return gotIt;
}

//////////////////////////////////////////////////////////////////////

wstring URLSanitize(wstring const &str)
{
	wstring OK(TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#@!$&'()*,;="));
	wstring result;

	size_t l = str.size();
	for(size_t i=0; i<l; ++i)
	{
		if(OK.find(str[i]) != wstring::npos)
		{
			result += str[i];
		}
		else
		{
			result += Format(TEXT("%%%02X"), str[i]); // TODO: deal with Unicode properly
		}
	}
	return result;
}