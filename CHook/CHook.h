#ifndef __CHOOK_H
#define __CHOOK_H

typedef struct _dynh_list {
	char		*s_szfnName;
	HMODULE		s_dwModBase;
	FARPROC		s_pfnFunc;
	_dynh_list	*next_hook;
} dynh_list;

class CHook {
	private:
		static FARPROC WINAPI h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );
		static CHook m_pCHook;
		static dynh_list *m_pDynHooks, *m_pDynStart;

	public:
		CHook( void );
		~CHook( void );
		bool AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour );
};

#endif