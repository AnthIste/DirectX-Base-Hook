#ifndef __CHOOK_H
#define __CHOOK_H

// How it managed to build without the windows header is beyond me :/
#include <windows.h>

typedef struct _dynh_list {
	char		*s_szfnName;	// Original function's name eg. LoadLibraryA
	HMODULE		s_dwModBase;	// Its module's base address
	FARPROC		s_pfnFunc;		// The address of the hook proc
	_dynh_list	*next_hook;
} dynh_list;

class CHook {
	public:
		CHook( void );
		~CHook( void );

		// This function needs to be static too else it needs an object to be called
		static bool AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour );

		// Merged with my hook code to keep all hooking stuff together
		static unsigned long* GetVtableAddress(void* pObject);
		static unsigned long* DetourWithVtable(void* pObject, unsigned int offset, unsigned long* hookProc);

	private:
		static FARPROC WINAPI h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );

		static CHook m_pCHook;
		static dynh_list *m_pDynHooks, *m_pDynStart;
};

#endif