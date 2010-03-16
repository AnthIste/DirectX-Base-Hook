#define _WINNT_VER 0x0501
#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <vector>
#include <time.h>

#pragma comment(lib, "detours.lib")

typedef struct _STRING
{
	USHORT	Length;
	USHORT	MaximumLength;
	PCHAR	Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef int (__stdcall *msg)(HWND, LPCSTR, LPCSTR, UINT);
msg Msg;
typedef NTSTATUS (__stdcall *woop)(PVOID, PANSI_STRING, ULONG, PVOID*);
woop o_woop;

int WINAPI MyMessageBox(HWND hWnd, LPCSTR lpStr, LPCSTR lpStr2, UINT uMsg)
{
	return MessageBoxA(hWnd, "hacked", "hacked", MB_ICONINFORMATION);
}

NTSTATUS __stdcall LdrGetProcedureAddress (PVOID BaseAddress, PANSI_STRING Name, ULONG Ordinal, PVOID *ProcedureAddress)
{
	if(!strcmp(Name->Buffer, "MessageBoxA")) {
		*ProcedureAddress = (PVOID)MyMessageBox;
		return ((NTSTATUS)0x00000000L);
	}
	return o_woop(BaseAddress, Name, Ordinal, ProcedureAddress);
}

int __stdcall WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	o_woop = (woop)DetourFunction(DetourFindFunction("ntdll.dll","LdrGetProcedureAddress"), (LPBYTE)LdrGetProcedureAddress);
	Msg = (msg)GetProcAddress(LoadLibrary("User32.dll"), "MessageBoxA");
	Msg(0, "Test", "Test", MB_ICONERROR);
	return 0;
}