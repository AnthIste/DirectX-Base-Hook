// Small LoadLibraryA hook by illuz1oN
// Log all dynamically loaded libraries
// inside a foreign process.

#define _WINNT_VER 0x0501
#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <time.h>

typedef HMODULE (WINAPI *t_LoadLibrary)(LPCSTR);
t_LoadLibrary g_pfnLoadLibrary = 0;

VOID __cdecl AppendLog(LPCSTR lpFile, const char *lpText, ...)
{
	struct tm * current_tm;
	time_t current_time;

	time(&current_time);
	current_tm = localtime(&current_time);
	
	char *pBuffer = (char *)malloc(strlen(lpText) + 64);
	if(!pBuffer)
		return;
	sprintf(pBuffer, "[%02d:%02d:%02d] ", current_tm->tm_hour, current_tm->tm_min, current_tm->tm_sec);

	va_list vList;
	va_start(vList, lpText);
	_vsnprintf((pBuffer + strlen(pBuffer)), (sizeof(pBuffer) - strlen(pBuffer)), lpText, vList);
	va_end(vList);

	FILE *fp = NULL;
	if ((fp = fopen(lpFile, "a")) != NULL) {
		fprintf (fp, "%s\n", pBuffer);
		fclose (fp);
	}

	free(pBuffer);
}

HMODULE WINAPI h_LoadLibrary(LPCSTR lpLibName)
{
	AppendLog("C:\\LoadLibraryHook.log", "Loaded: %s", lpLibName);
	return g_pfnLoadLibrary(lpLibName);
}

BOOL WINAPI DllMain(_In_ HANDLE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved)
{
	if(_Reason == DLL_PROCESS_ATTACH) {
		AppendLog("C:\\LoadLibraryHook.log", "Injection Successful.");
		g_pfnLoadLibrary = (t_LoadLibrary)DetourFunction((LPBYTE)DetourFindFunction("kernel32.dll", "LoadLibraryA"), (LPBYTE)h_LoadLibrary);
		AppendLog("C:\\LoadLibraryHook.log", "LoadLibraryA 0x%X -> 0x%X", (DWORD)LoadLibraryA, (DWORD)h_LoadLibrary);
	}
	if(_Reason == DLL_PROCESS_DETACH) 
		AppendLog("C:\\LoadLibraryHook.log", "Finished logging.");

	return TRUE;
}