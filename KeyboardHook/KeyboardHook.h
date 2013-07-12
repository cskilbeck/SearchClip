//////////////////////////////////////////////////////////////////////

#ifdef KEYBOARDHOOK_EXPORTS
#define KEYBOARDHOOK_API __declspec(dllexport)
#else
#define KEYBOARDHOOK_API __declspec(dllimport)
#endif

//////////////////////////////////////////////////////////////////////

extern KEYBOARDHOOK_API HHOOK hKeyboardHook;
extern KEYBOARDHOOK_API HWND hMainWindow;
extern KEYBOARDHOOK_API double MouseDoubleClickTime;

//////////////////////////////////////////////////////////////////////

KEYBOARDHOOK_API LRESULT CALLBACK fnKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);
