//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KeyboardHook.h"
#include "Timer.h"
#include "Common.h"
#include "Util.h"

//////////////////////////////////////////////////////////////////////

HOOKDLL_API DWORD bMiddleClickClose;
HOOKDLL_API HHOOK hKeyboardHook = 0;
HOOKDLL_API HHOOK hMouseHook = 0;
HOOKDLL_API HWND hMainWindow=0;
HOOKDLL_API double MouseDoubleClickTime;

//////////////////////////////////////////////////////////////////////

Timer timer;
int lastKey;

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK fnKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	int key = ((KBDLLHOOKSTRUCT *)lParam)->vkCode;
	if (	nCode == HC_ACTION
		&&	wParam == WM_KEYDOWN
		&&	(key == 'C' || key == 'G')	// TODO: Check if this should be got from somewhere in case it's not 'C'
		&&	GetAsyncKeyState(VK_LCONTROL) < 0
		)
	{
		double elapsed = timer.GetElapsed();
		timer.Reset();
		if(elapsed < MouseDoubleClickTime && key == lastKey)
		{
			PostMessage(hMainWindow, WMU_LAUNCH_BROWSER, key, 0);
			return 1;
		}
		lastKey = key;
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////

HWND window;

BOOL CALLBACK EnumChildProc(HWND handle, LPARAM param)
{
	DWORD processID;
	GetWindowThreadProcessId(handle, &processID);
	if((DWORD)param == processID)
	{
		if(IsWindowVisible(handle) && IsWindowEnabled(handle) && handle != GetDesktopWindow())
		{
			window = handle;
		}
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK fnMouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(bMiddleClickClose && nCode == HC_ACTION && wParam == WM_MBUTTONDOWN)
	{
		MSLLHOOKSTRUCT *msg = (MSLLHOOKSTRUCT *)lParam;
		HWND w = WindowFromPoint(msg->pt);
		if(w != NULL)
		{
			DWORD processID;
			bool closeIt = false;
			DWORD hittest = SendMessage(w, WM_NCHITTEST, 0, MAKELPARAM(msg->pt.x, msg->pt.y));
			if(hittest == HTCAPTION)
			{
				GetWindowThreadProcessId(w, &processID);
				closeIt = true;
			}
			else if (hittest == HTTRANSPARENT)	// special case for IE, which can't detect WM_NCHITTEST (returns HTTRANSPARENT)
			{
				TCHAR exeName[MAX_PATH];
				DWORD exeNameSize(ARRAYSIZE(exeName));
				GetWindowThreadProcessId(w, &processID);
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
				QueryFullProcessImageName(hProcess, 0, exeName, &exeNameSize);
				CloseHandle(hProcess);
				if(_tcsstr(exeName, TEXT("iexplore.exe")))
				{
					closeIt = true;
				}
			}
			if(closeIt)
			{
				window = NULL;
				EnumChildWindows(NULL, EnumChildProc, (LPARAM)processID);
				if(window != NULL)
				{
					PostMessage(window, WM_SYSCOMMAND, SC_CLOSE, -1);
					TRACE(TEXT("Closing %08x (PROCESS %08x)\n"), window, processID);
				}
			}
		}
	}
	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}