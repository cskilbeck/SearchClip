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
			DWORD hittest = SendMessage(w, WM_NCHITTEST, 0, MAKELPARAM(msg->pt.x, msg->pt.y));
			if(hittest == HTCAPTION)
			{
				PostMessage(w, WM_SYSKEYDOWN, VK_F4, 1 << 29);
			}
		}
	}
	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}