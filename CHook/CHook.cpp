#define _WINNT_VER 0x0501
#include <windows.h>
#include "CHook.h"
 
// You need to define static class variables in _A_ source file somewhere
//CHook CHook::m_pCHook;
dynh_list *CHook::pDynHooks, *CHook::pDynStart;
 
BOOL PatchAPI(LPSTR lpszLib, LPSTR lpszFunc, FARPROC *lpOldFunc, FARPROC fpNewFunc); // by Napalm

typedef FARPROC (WINAPI *GetProcType)(HMODULE, LPCSTR);
GetProcType gProctype;

// Hook GetProcAddress on runtime, for dynamic hooking features.
CHook::CHook( void )
{
	if(!PatchAPI("User32.dll", "MessageBoxA", (FARPROC *)&gProctype, (FARPROC)h_fnGetProcAddress))
		MessageBoxA(0, "Detour failed!", "Fail", MB_ICONASTERISK);
}
 
// Free up the linked list memory.
CHook::~CHook( void )
{
	/*dynh_list *walk_list = pDynStart;
	while(walk_list) {
		walk_list = walk_list->next_hook;
		free(walk_list->szfnName);
		free(walk_list);
		pDynStart = walk_list;
	}*/
}
 
// We hook GetProcAddress, to implement dynamic hooking.
// This emulates GetProcAddress if the function the foreign
// process is looking for != a function we want to hook.
FARPROC WINAPI CHook::h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName )
{
	dynh_list *walk_list = pDynStart;
	while(walk_list) {
		if(walk_list->dwModBase == hModule && !strcmp(walk_list->szfnName, lpProcName)) 
			return walk_list->pfnFunc;
		walk_list = walk_list->next_hook;
	}
 
	PIMAGE_DOS_HEADER pDosHeader = ((PIMAGE_DOS_HEADER)(DWORD)hModule);
	if(pDosHeader->e_magic != 0x5A4D)
		return NULL;
 
	PIMAGE_NT_HEADERS pNtHeaders = ((PIMAGE_NT_HEADERS)((DWORD)hModule + pDosHeader->e_lfanew));
	if(pNtHeaders->Signature != 0x4550)
		return NULL;
 
	PIMAGE_DATA_DIRECTORY pDataDir = ((PIMAGE_DATA_DIRECTORY)(pNtHeaders->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT));
	if(!pDataDir)
		return NULL;
 
	PIMAGE_EXPORT_DIRECTORY pExpDir = ((PIMAGE_EXPORT_DIRECTORY)((DWORD)hModule + pDataDir->VirtualAddress));
	if(!pExpDir)
		return NULL;
 
	DWORD *p_dwFuncs		= ((DWORD *)((DWORD)hModule + pExpDir->AddressOfFunctions));
	const char **p_szNames	= ((const char **)((DWORD)hModule + pExpDir->AddressOfNames));
	if(!p_dwFuncs || !p_szNames)
		return NULL;
 
	for(DWORD dwIndex = 0; dwIndex < pExpDir->NumberOfFunctions; dwIndex++) {
		if(!strcmp(((char *)((DWORD)hModule + p_szNames[dwIndex])), lpProcName)) {
			return (FARPROC)((DWORD)hModule +	p_dwFuncs[dwIndex]);
		}
	}
 
	return NULL;
}
 
// This is to add a "DynamicHook", using linked lists, which will be put in use when GetProcAddress is called.
// NOTE: You cannot remove a "DynamicHook", as the program will store your DETOUR address, not a detoured real function,
// therefore it is your responsibility to call the original function itself.
bool CHook::AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour )
{
	if(!lpLibName || !lpFuncName || IsBadCodePtr(pfnDetour))
		return false;
 
	if(!pDynHooks) {
		if(!(pDynStart = pDynHooks = (dynh_list *)malloc(sizeof(dynh_list))))
			return false;
		pDynHooks->next_hook = NULL;
	} else {
		if(!(pDynHooks->next_hook = (dynh_list *)malloc(sizeof(dynh_list)))) 
			return false;
		pDynHooks = pDynHooks->next_hook;
	}
 
	pDynHooks->next_hook	= NULL;
	pDynHooks->dwModBase	= GetModuleHandleA(lpLibName);
	pDynHooks->pfnFunc		= pfnDetour;
	pDynHooks->szfnName		= _strdup(lpFuncName);
 
	if(!pDynHooks->szfnName) {
		free(pDynHooks);
		return false;
	}

	MessageBoxA(0, "Inserted Hook...", pDynHooks->szfnName, 0);
 
	return true;
}
 
BOOL PatchAPI(LPSTR lpszLib, LPSTR lpszFunc, FARPROC *lpOldFunc, FARPROC fpNewFunc) // by Napalm
{
	BOOL    bResult = FALSE;
	DWORD   dwProtect;
	LPBYTE  lpPatch;
	FARPROC fpOldFunc;

	fpOldFunc = GetProcAddress(LoadLibraryA(lpszLib), lpszFunc);
	if(fpOldFunc){
		lpPatch = (LPBYTE)fpOldFunc - 5;
		if(!memcmp(lpPatch, "\x90\x90\x90\x90\x90\x8B\xFF", 7)){
			if(VirtualProtect(lpPatch, 7, PAGE_EXECUTE_READWRITE, &dwProtect)){
				*lpPatch = 0xE9;
				*(LPDWORD)(lpPatch + 1) = (DWORD)((LONG)fpNewFunc - (LONG)fpOldFunc);
				*(LPDWORD)lpOldFunc = ((DWORD)fpOldFunc + 2);
				InterlockedExchange((LPLONG)fpOldFunc, (LONG)((*(LPDWORD)fpOldFunc & 0xFFFF0000) | 0xF9EB));
				VirtualProtect(lpPatch, 7, dwProtect, NULL);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

// Hook a function, thread safe.
// Credits to Napalm!
// Will re-write this in due time.
FARPROC CHook::NewDetour( __in FARPROC pfnOldFunc, __in FARPROC pfnNewFunc )
{
	if(IsBadCodePtr(pfnNewFunc) || IsBadCodePtr(pfnOldFunc))
		return NULL;
 
	DWORD dwOldProtect;
	LPBYTE lpPatchFunc = (LPBYTE)pfnOldFunc;
 
	if(!memcmp(lpPatchFunc, "\x8B\xFF", 2)) {
		lpPatchFunc -= 5;
		if(!memcmp(lpPatchFunc, "\x90\x90\x90\x90\x90", 5) || !memcmp(lpPatchFunc, "\xCC\xCC\xCC\xCC\xCC", 5)) {
			if(VirtualProtect(lpPatchFunc, 7, PAGE_EXECUTE_READWRITE, &dwOldProtect)) {
				*(LPDWORD)(lpPatchFunc + 1) = ((LONG)pfnNewFunc - (LONG)pfnOldFunc);
				*(LPDWORD)lpPatchFunc = 0xE9;
				InterlockedExchange((LPLONG)pfnOldFunc, (LONG)((*(LPDWORD)pfnOldFunc & 0xFFFF0000) | 0xF9EB));
				VirtualProtect(lpPatchFunc, 7, dwOldProtect, NULL);
				return (FARPROC)((DWORD)pfnOldFunc + 2);;
			}
		}
	}
 
	return NULL;
}
 
// Hook virtual table function, thread safe.
FARPROC CHook::NewDetour( __in DWORD *pVtable, __in unsigned int nFuncOffset, __in FARPROC pfnNewFunc ) 
{
	/*DWORD dwOldProtect;
	FARPROC pfnOrigProc = 0;
 
	if(!pVtable || !nFuncOffset || IsBadCodePtr(pfnNewFunc))
		return NULL;
 
	void *lpBaseAddress = (void *)((DWORD)(pVtable) + nFuncOffset);
 
	if (!VirtualProtect(lpBaseAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return NULL;
 
	InterlockedExchange((LPLONG)((DWORD)pfnOrigProc), (LONG)((DWORD *)pVtable[nFuncOffset]));
	InterlockedExchange((LPLONG)((DWORD *)pVtable[nFuncOffset]), (LONG)((DWORD)pfnNewFunc));
 
	VirtualProtect(lpBaseAddress, sizeof(DWORD), dwOldProtect, NULL);

	return pfnOrigProc;*/

	// MUST be used else VirtualProtect will fail
	DWORD dwOldProtect;

	void* lpBaseAddress = reinterpret_cast<void*>(reinterpret_cast<unsigned long>(pVtable) + nFuncOffset);

	// Chances are the vtable is read/write protected, so change that
	if (!VirtualProtect(lpBaseAddress, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect)) {
		return 0;
	}

	// Read the original function address now that protection is removed
	unsigned long* origProc = reinterpret_cast<unsigned long*>(pVtable[nFuncOffset]);

	// Replace it with our hook address
	pVtable[nFuncOffset] = reinterpret_cast<unsigned long>(pfnNewFunc);

	// Restore protection
	VirtualProtect(lpBaseAddress, 4, dwOldProtect, &dwOldProtect);
 
	return (FARPROC)origProc;
}

DWORD* CHook::GetVtableAddress(void* pObject)
{
	return (DWORD *)(*(DWORD *)(pObject));
}