//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KeyboardHook.h"
#include "Timer.h"
#include "Common.h"
#include "Util.h"

//////////////////////////////////////////////////////////////////////

HOOKDLL_API HHOOK hKeyboardHook = 0;
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
		&&	(key == 'C')
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
