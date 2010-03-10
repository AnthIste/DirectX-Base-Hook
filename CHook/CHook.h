#ifndef __CHOOK_H
#define __CHOOK_H

typedef struct _dynh_list {
	char		*s_szfnName;
	HMODULE		s_dwModBase;
	FARPROC		s_pfnFunc;
	_dynh_list	*next_hook;
} dynh_list;

class CHook {
	public:
		CHook( void );
		~CHook( void );


		// This function needs to be static too else it needs an object to be called
		static bool AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour );
		static FARPROC	NewDetour( __in FARPROC pfnOldFunc, __in FARPROC pfnNewFunc );
		static FARPROC	NewDetour( __in void *pObject, __in unsigned int nFuncOffset, __in FARPROC pfnNewFunc );

		// Merged with my hook code to keep all hooking stuff together
		static unsigned long* GetVtableAddress(void* pObject);
		static unsigned long* DetourWithVtable(unsigned long* pVtable, unsigned int offset, unsigned long* hookProc);

	private:
		static FARPROC WINAPI h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );
		static CHook m_pCHook;
		static dynh_list *m_pDynHooks, *m_pDynStart;
};

#endif