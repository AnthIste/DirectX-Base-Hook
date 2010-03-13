#define _WINNT_VER 0x0501
#include <windows.h>
#include <stdio.h>
#include "../CHook/CHook.h"

// Rather use relative paths

#pragma comment(lib, "../Release/CHook.lib")

int WINAPI MyMessageBoxA(__in_opt HWND hWnd, __in_opt LPCSTR lpText, __in_opt LPCSTR lpCaption, __in UINT uType)
{
	lpCaption = "Hooked?";
	return MessageBoxA(hWnd, lpText, lpCaption, uType);
}

int __stdcall WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	CHook cHook;
	cHook.AddDynamicHook("User32.dll", "MessageBoxA", (FARPROC)MyMessageBoxA);
	void *func = (void *)GetProcAddress(LoadLibraryA("User32.dll"), "MessageBoxA");
	if(func) {
		__asm {
			push 0
			push 0
			push 0
			push 0
			call [func]
		}
	} else {
		MessageBoxA(0, "test", 0, 0);
	}
	return 0;
}