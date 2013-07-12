//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

struct Value
{
	enum eType
	{
		kString,
		kDWORD,
		kUnknown
	};

	union
	{
		DWORD		mDWORD;
	};
	wstring		mString;	// can't put this in the union
	eType		mType;

	Value()
	{
		mType = kUnknown;
	}

	Value(WCHAR const *str)
	{
		mType = kString;
		mString = str;
	}

	Value(DWORD dw)
	{
		mType = kDWORD;
		mDWORD = dw;
	}

	// ToString...
};

//////////////////////////////////////////////////////////////////////

class RegKey
{
public:

	//////////////////////////////////////////////////////////////////////

	RegKey()
	{
		mKey = NULL;
	}

	//////////////////////////////////////////////////////////////////////

	RegKey(HKEY parent, WCHAR const *name, DWORD accessMask = KEY_READ)
	{
		if(RegOpenKeyEx(parent, name, 0, accessMask, &mKey) != ERROR_SUCCESS)
		{
			mKey = NULL;
		}
	}

	//////////////////////////////////////////////////////////////////////

	void Close()
	{
		if(mKey != NULL)
		{
			RegCloseKey(mKey);
			mKey = NULL;
		}
	}

	//////////////////////////////////////////////////////////////////////

	~RegKey()
	{
		Close();
	}

	//////////////////////////////////////////////////////////////////////

	operator HKEY() const
	{
		return mKey;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL Create(HKEY parent, WCHAR const *name, DWORD accessMask = KEY_READ)
	{
		Close();
		if(RegCreateKeyEx(parent, name, 0, NULL, 0, accessMask, NULL, &mKey, NULL) != ERROR_SUCCESS)
		{
			mKey = NULL;
		}
		return mKey != NULL;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL GetStringValue(WCHAR const *name, wstring &result)
	{
		DWORD type;
		WCHAR buffer[16384];
		DWORD bufferSize(ARRAYSIZE(buffer));
		if(RegQueryValueEx(mKey, name, NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
		{
			if(type == REG_SZ)
			{
				result = wstring(buffer);
				return TRUE;
			}
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL GetDWORDValue(WCHAR const *name, DWORD &result)
	{
		DWORD type;
		DWORD size = sizeof(result);
		if(RegQueryValueEx(mKey, name, NULL, &type, (LPBYTE)&result, &size) == ERROR_SUCCESS)
		{
			return type == REG_DWORD;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL DeleteValue(WCHAR const *name)
	{
		return RegDeleteValue(mKey, name) == ERROR_SUCCESS;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL SetStringValue(WCHAR const *name, WCHAR const *value)
	{
		return RegSetValueEx(mKey, name, 0, REG_SZ, (LPBYTE)value, (wcslen(value) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL SetDWORDValue(WCHAR const *name, DWORD value)
	{
		return RegSetValueEx(mKey, name, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD)) == ERROR_SUCCESS;
	}

	//////////////////////////////////////////////////////////////////////

	int EnumKeys(vector <wstring> &keyNames)
	{
		keyNames.clear();
		DWORD index = 0;
		WCHAR name[256];
		DWORD nameLength = ARRAYSIZE(name);
		while(RegEnumKeyEx(mKey, index, name, &nameLength, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			keyNames.push_back(wstring(name));
			nameLength = ARRAYSIZE(name);
			++index;
		}
		return index;
	}

	//////////////////////////////////////////////////////////////////////

	int EnumValues(vector <Value> &values)
	{
		DWORD index = 0;
		WCHAR name[256];
		DWORD nameLength = ARRAYSIZE(name);
		WCHAR valueBuffer[16384];
		DWORD valueBufferLength = ARRAYSIZE(valueBuffer);
		DWORD type;
		while(RegEnumValue(mKey, index, name, &nameLength, NULL, &type, (LPBYTE)valueBuffer, &valueBufferLength) == ERROR_SUCCESS)
		{
			switch(type)
			{
			case REG_SZ:
				values.push_back(Value(valueBuffer));
				break;

			case REG_DWORD:
				values.push_back(Value(*(DWORD *)valueBuffer));
				break;

			// add the rest here

			default:
				values.push_back(Value());	// Unknown type
				break;
			}
			valueBufferLength = ARRAYSIZE(valueBuffer);
			nameLength = ARRAYSIZE(name);
		}
		return index;
	}

	//////////////////////////////////////////////////////////////////////

private:

	HKEY mKey;
};
