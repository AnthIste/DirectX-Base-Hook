// CHook Library by illuz1oN 2010.
#ifndef __CHOOK_H
#define __CHOOK_H

// Linked list for holding functions
// that we will hook dynamically.
typedef struct _dynh_list {
	char	*s_szfName;		// Name of the function we will hook.
	DWORD	s_dwModBase;	// Base of the library the function is within.
	FARPROC	s_pfnFunc;		// The detour function.
	_dynh_list *next_hook;	// Next in the list.
} dynh_list;

class CHook {
	private:
		// Our GetProcAddress, used for dynamically hooking functions.
		static FARPROC WINAPI m_pfnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );

		static CHook m_pCHook;	// This will ensure our constructor & destructor are called on runtime.
		dynh_list *m_pDynHooks; // Linked list of dynamically hooked functions.

	public:
		CHook( void );	// Constructor.
		~CHook( void );	// Destructor.

		// Add a dynamic hook to the list.
		bool AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour );
};

#endif