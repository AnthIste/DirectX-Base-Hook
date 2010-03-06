#ifndef __CHOOK_H
#define __CHOOK_H

class CHook {
	private:
		static FARPROC WINAPI c_hGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );

	public:
		CHook( void );
		~CHook( void );

	private:
		static CHook g_hook;	// Acts as a 'global variable' but its private
								// ctor SHOULD be called when dll loads
								// maybe add logging code in ctor to make 100% sure
};

#endif