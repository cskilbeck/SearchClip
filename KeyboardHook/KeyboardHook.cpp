//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KeyboardHook.h"
#include "Timer.h"
#include "Common.h"

//////////////////////////////////////////////////////////////////////

KEYBOARDHOOK_API HHOOK hKeyboardHook=0;
KEYBOARDHOOK_API HWND hMainWindow=0;
KEYBOARDHOOK_API double MouseDoubleClickTime;

//////////////////////////////////////////////////////////////////////

Timer timer;

//////////////////////////////////////////////////////////////////////

KEYBOARDHOOK_API LRESULT CALLBACK fnKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
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

