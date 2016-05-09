//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KeyboardHook.h"
#include "Timer.h"
#include "Common.h"
#include "Util.h"

//////////////////////////////////////////////////////////////////////

HOOKDLL_API HHOOK hKeyboardHook = 0;
HOOKDLL_API HHOOK hMouseHook = 0;
HOOKDLL_API HWND hMainWindow=0;
HOOKDLL_API double MouseDoubleClickTime;

//////////////////////////////////////////////////////////////////////

Timer timer;

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK fnKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (	nCode == HC_ACTION
		&&	wParam == WM_KEYDOWN
		&&	((KBDLLHOOKSTRUCT *)lParam)->vkCode == 'C'	// TODO: Check if this should be got from somewhere in case it's not 'C'
		&&	GetAsyncKeyState(VK_LCONTROL) < 0
		)
	{
		double elapsed = timer.GetElapsed();
		timer.Reset();
		if(elapsed < MouseDoubleClickTime)
		{
			PostMessage(hMainWindow, WMU_LAUNCH_BROWSER, 0, 0);
			return 1;
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK fnMouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode == HC_ACTION && wParam == WM_MBUTTONDOWN)
	{
		MSLLHOOKSTRUCT *msg = (MSLLHOOKSTRUCT *)lParam;
		HWND w = WindowFromPoint(msg->pt);
		if(w != NULL)
		{
			bool closeIt = false;
			DWORD hittest = SendMessage(w, WM_NCHITTEST, 0, MAKELPARAM(msg->pt.x, msg->pt.y));
			if(hittest == HTCAPTION)
			{
				closeIt = true;
			}
			else if (hittest == HTTRANSPARENT)	// special case for IE, which can't detect WM_NCHITTEST (returns HTTRANSPARENT)
			{
				TCHAR exeName[MAX_PATH];
				DWORD exeNameSize(ARRAYSIZE(exeName));
				DWORD processID;
				DWORD threadID = GetWindowThreadProcessId(w, &processID);
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
				// Most things close with these 3, in this order (WM_CLOSE last, else Office 2016 apps go screwy)
				PostMessage(w, WM_SYSKEYDOWN, VK_F4, 1 << 29);
				PostMessage(w, WM_SYSCOMMAND, SC_CLOSE, -1);
				PostMessage(w, WM_CLOSE, 0, 0);
			}
		}
	}
	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}