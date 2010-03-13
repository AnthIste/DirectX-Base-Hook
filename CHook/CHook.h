#ifndef __CHOOK_H
#define __CHOOK_H
 
typedef struct _dynh_list {
	char		*szfnName;
	HMODULE		dwModBase;
	FARPROC		pfnFunc;
	_dynh_list	*next_hook;
} dynh_list;
 
class CHook {
	public:
		CHook( void );
		~CHook( void );
 
		static bool		AddDynamicHook( __in LPSTR lpLibName, __in LPSTR lpFuncName, __in FARPROC pfnDetour );
		static FARPROC	NewDetour( __in FARPROC pfnOldFunc, __in FARPROC pfnNewFunc );
		static FARPROC	NewDetour( __in DWORD *pVtable, __in unsigned int nFuncOffset, __in FARPROC pfnNewFunc );
		static DWORD*	GetVtableAddress(void* pObject);
 
	private:
		static FARPROC WINAPI h_fnGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );
		//static CHook m_pCHook;
		static dynh_list *pDynHooks, *pDynStart;
};
 
#endif