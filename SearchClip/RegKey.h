//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

struct Value
{
	enum eType
	{
		kString,
		kDWORD,
		kUnknown
	};

	DWORD		mDWORD;
	wstring		mString;	// can't put this in a union
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
		: mKey(NULL)
	{
	}

	//////////////////////////////////////////////////////////////////////

	RegKey(HKEY parent, wstring const &name, DWORD accessMask = KEY_READ)
	{
		if(RegOpenKeyEx(parent, name.c_str(), 0, accessMask, &mKey) != ERROR_SUCCESS)
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

	BOOL Create(HKEY parent, wstring const &name, DWORD accessMask = KEY_READ)
	{
		Close();
		if(RegCreateKeyEx(parent, name.c_str(), 0, NULL, 0, accessMask, NULL, &mKey, NULL) != ERROR_SUCCESS)
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

	BOOL GetStringValue(wstring const &name, wstring &result)
	{
		return GetStringValue(name.c_str(), result);
	}

	//////////////////////////////////////////////////////////////////////

	BOOL GetDWORDValue(wstring const &name, DWORD &result)
	{
		DWORD type;
		DWORD size = sizeof(result);
		if(RegQueryValueEx(mKey, name.c_str(), NULL, &type, (LPBYTE)&result, &size) == ERROR_SUCCESS)
		{
			return type == REG_DWORD;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL DeleteValue(wstring const &name)
	{
		return RegDeleteValue(mKey, name.c_str()) == ERROR_SUCCESS;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL SetStringValue(wstring const &name, wstring const &value)
	{
		return RegSetValueEx(mKey, name.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), (value.size() + 1) * sizeof(WCHAR)) == ERROR_SUCCESS;
	}

	//////////////////////////////////////////////////////////////////////

	BOOL SetDWORDValue(wstring const &name, DWORD value)
	{
		return RegSetValueEx(mKey, name.c_str(), 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD)) == ERROR_SUCCESS;
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

	int EnumValues(vector<Value> &values)
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

			// other types not supported, ignored
			default:
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
