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

		static bool		AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour );
		static FARPROC	NewDetour( __in LPCSTR lpszModName, __in LPCSTR lpszFuncName, __in FARPROC pfnNewFunc, __in int nDetourMethod );
		static FARPROC	NewDetour( __in void *pObject, __in unsigned int nFuncOffset, __in FARPROC pfnNewFunc );

	private:
		static FARPROC WINAPI h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );
		static CHook m_pCHook;
		static dynh_list *m_pDynHooks, *m_pDynStart;
};

#endif