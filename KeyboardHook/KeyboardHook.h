//////////////////////////////////////////////////////////////////////

#ifdef KEYBOARDHOOK_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif

//////////////////////////////////////////////////////////////////////

extern HOOKDLL_API DWORD bMiddleClickClose;
extern HOOKDLL_API HHOOK hKeyboardHook;
extern HOOKDLL_API HHOOK hMouseHook;
extern HOOKDLL_API HWND hMainWindow;
extern HOOKDLL_API double MouseDoubleClickTime;

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK fnKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);
HOOKDLL_API LRESULT CALLBACK fnMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
