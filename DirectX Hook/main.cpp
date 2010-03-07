// Small LoadLibraryA hook by illuz1oN
// Log all dynamically loaded libraries
// inside a foreign process.

#define _WINNT_VER 0x0501
#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <time.h>

#pragma comment(lib, "detours.lib")

BOOL WINAPI DllMain(HINSTANCE hInstanceDll, DWORD dwReason, LPVOID reserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}