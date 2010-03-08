#define _WINNT_VER 0x0501
#include <windows.h>
#include <stdio.h>
#include "../CHook/CHook.h"

#pragma comment(lib, "C:\\DirectX Project\\Release\\CHook.lib")

int WINAPI MyMessageBoxA(__in_opt HWND hWnd, __in_opt LPCSTR lpText, __in_opt LPCSTR lpCaption, __in UINT uType)
{
	lpCaption = "Hooked?";
	return MessageBoxA(hWnd, lpText, lpCaption, uType);
}

int __stdcall WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	CHook cHook;
	//cHook.AddDynamicHook("User32.dll", "MessageBoxA", (FARPROC)MyMessageBoxA);
	MessageBoxA(0, "test", "test", 0);
	return 0;
}