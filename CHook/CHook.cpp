#define _WINNT_VER 0x0501
#include <windows.h>
#include "CHook.h"

// Hook GetProcAddress on runtime, for dynamic hooking features.
CHook::CHook( void )
{
	if(!NewDetour((FARPROC)GetProcAddress, (FARPROC)h_fnGetProcAddress))
		MessageBoxA(0, "Detour failed!", "Fail", MB_ICONASTERISK);
}

// Free up the linked list memory.
CHook::~CHook( void )
{
	dynh_list *walk_list = m_pDynStart;
	while(walk_list) {
		walk_list = walk_list->next_hook;
		free(walk_list->s_szfnName);
		free(walk_list);
		m_pDynStart = walk_list;
	}
}

// We hook GetProcAddress, to implement dynamic hooking.
// This emulates GetProcAddress if the function the foreign
// process is looking for != a function we want to hook.
FARPROC WINAPI CHook::h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName )
{
	dynh_list *walk_list = m_pDynStart;
	while(walk_list) {
		if(walk_list->s_dwModBase == hModule && !strcmp(walk_list->s_szfnName, lpProcName)) 
			return walk_list->s_pfnFunc;
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

	while(m_pDynHooks->next_hook)
		m_pDynHooks = m_pDynHooks->next_hook;

	if(!m_pDynHooks) {
		if(!(m_pDynStart = m_pDynHooks = (dynh_list *)malloc(sizeof(dynh_list))))
			return false;
	} else {
		if(!(m_pDynHooks->next_hook = (dynh_list *)malloc(sizeof(dynh_list)))) 
			return false;
		m_pDynHooks = m_pDynHooks->next_hook;
	}
	
	m_pDynHooks->next_hook		= NULL;
	m_pDynHooks->s_dwModBase	= GetModuleHandleA(lpLibName);
	m_pDynHooks->s_pfnFunc		= pfnDetour;
	m_pDynHooks->s_szfnName		= _strdup(lpFuncName);

	if(!m_pDynHooks->s_szfnName) {
		free(m_pDynHooks);
		return false;
	}
	
	return true;
}

// Hook a function, thread safe.
// Credits to Napalm!
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
FARPROC CHook::NewDetour( __in void *pObject, __in unsigned int nFuncOffset, __in FARPROC pfnNewFunc ) 
{
	DWORD dwOldProtect, *dwvTableAddr;
	FARPROC pfnOrigProc = 0;

	if(!pObject || !nFuncOffset || IsBadCodePtr(pfnNewFunc))
		return NULL;

	dwvTableAddr = (DWORD *)(*(DWORD *)(pObject));
	void *lpBaseAddress = (void *)((DWORD)(dwvTableAddr) + nFuncOffset);

	if (!VirtualProtect(lpBaseAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return NULL;

	InterlockedExchange((LPLONG)((DWORD)pfnOrigProc), (LONG)((DWORD *)dwvTableAddr[nFuncOffset]));
	InterlockedExchange((LPLONG)((DWORD *)dwvTableAddr[nFuncOffset]), (LONG)((DWORD)pfnNewFunc));

	VirtualProtect(lpBaseAddress, sizeof(DWORD), dwOldProtect, NULL);

	return pfnOrigProc;
}