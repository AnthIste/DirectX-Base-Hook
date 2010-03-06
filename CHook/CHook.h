#ifndef __CHOOK_H
#define __CHOOK_H

class CHook {
	private:
	static FARPROC WINAPI c_hGetProcAddress( __in HMODULE hModule, __in LPCSTR lpProcName );

	public:
		CHook( void );
		~CHook( void );
};

#endif