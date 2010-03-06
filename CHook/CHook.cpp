#define _WINNT_VER 0x0501
#include <windows.h>
#include "CHook.h"

// Class constructor.
CHook::CHook( void )
{
	
}

// Class destructor.
CHook::~CHook( void )
{

}

// We hook GetProcAddress, to implement dynamic hooking.
// This emulates GetProcAddress if the function the foreign
// process is looking for != a function we want to hook.
FARPROC WINAPI CHook::c_hGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName )
{
	DWORD *p_dwFuncs = NULL, pModBase = (DWORD)hModule;
	const char **p_szNames = NULL;

	
	// Search linked list here for dynamic hooking.
	// Write this & the linked list next.


	// Pointer to the DOS header, and check the signature.
	PIMAGE_DOS_HEADER pDosHeader = ((PIMAGE_DOS_HEADER)pModBase);
	if(pDosHeader->e_magic != 0x5A4D) // MZ
		return NULL;

	// e_lfanew holds a pointer to the NT header. Then check the NT Signature
	PIMAGE_NT_HEADERS pNtHeaders = ((PIMAGE_NT_HEADERS)(pModBase + pDosHeader->e_lfanew));
	if(pNtHeaders->Signature != 0x4550) // PE
		return NULL;

	// Retrieve the data directory. We use this to get to the export directory.
	PIMAGE_DATA_DIRECTORY pDataDir = ((PIMAGE_DATA_DIRECTORY)(pNtHeaders->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT));
	if(!pDataDir)
		return NULL;
	
	// Get the address of the export directory.
	PIMAGE_EXPORT_DIRECTORY pExpDir = ((PIMAGE_EXPORT_DIRECTORY)(pModBase + pDataDir->VirtualAddress));
	if(!pExpDir)
		return NULL;

	p_dwFuncs = ((DWORD *)(pModBase + pExpDir->AddressOfFunctions));	// Array of function addresses.
	p_szNames = ((const char **)(pModBase + pExpDir->AddressOfNames));	// Array of function name.
	if(!p_dwFuncs || !p_szNames)
		return NULL;

	// Loop through, if we find the function, return the address.
	for(DWORD dwIndex = 0; dwIndex < pExpDir->NumberOfFunctions; dwIndex++) {
		if(!strcmp(((char *)(pModBase + p_szNames[dwIndex])), lpProcName)) {
			return (FARPROC)(pModBase +	p_dwFuncs[dwIndex]);
		}
	}

	// We never found the function.
	return NULL;
}