#include "stdafx.h"
#include "KeyboardHook.h"
#include "Timer.h"

KEYBOARDHOOK_API HHOOK hKeyboardHook=0;
KEYBOARDHOOK_API HWND hMainWindow=0;
KEYBOARDHOOK_API double mouseDoubleClickTime = 0.5f;

// Monitor CTRL-C
// If it's pressed twice within mouse double click time, send the message

Timer timer;

KEYBOARDHOOK_API LRESULT CALLBACK fnKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (
			nCode == HC_ACTION
		&&	wParam == WM_KEYDOWN
		&&	((KBDLLHOOKSTRUCT *)lParam)->vkCode == 'C'
		&&	GetAsyncKeyState(VK_LCONTROL) < 0
		)
	{
		double elapsed = timer.GetElapsed();
		timer.Reset();
		if(elapsed < mouseDoubleClickTime)
		{
			PostMessage(hMainWindow, WM_USER, 0, 0);
			return 1;
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

