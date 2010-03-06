#define _WINNT_VER 0x0501
#include <windows.h>
#include "CHook.h"

// Hook GetProcAddress on runtime, for dynamic hooking features.
CHook::CHook( void )
{

}

// Free up the linked list memory.
CHook::~CHook( void )
{
	dynh_list *walk_list = this->m_pDynStart;
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
FARPROC WINAPI CHook::m_pfnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName )
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
	if(!lpLibName || !lpFuncName || !IsBadCodePtr(pfnDetour))
		return false;

	while(this->m_pDynHooks->next_hook)
		this->m_pDynHooks = this->m_pDynHooks->next_hook;

	if(!this->m_pDynHooks) {
		if(!(this->m_pDynStart = this->m_pDynHooks = (dynh_list *)malloc(sizeof(dynh_list))))
			return false;
	} else {
		if(!(this->m_pDynHooks->next_hook = (dynh_list *)malloc(sizeof(dynh_list)))) 
			return false;
		this->m_pDynHooks = this->m_pDynHooks->next_hook;
	}
	
	this->m_pDynHooks->next_hook	= NULL;
	this->m_pDynHooks->s_dwModBase	= GetModuleHandleA(lpLibName);
	this->m_pDynHooks->s_pfnFunc	= pfnDetour;
	this->m_pDynHooks->s_szfnName	= _strdup(lpFuncName);

	if(!this->m_pDynHooks->s_szfnName) {
		free(this->m_pDynHooks);
		return false;
	}
	
	return true;
}