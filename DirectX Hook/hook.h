#ifndef INC_HOOK
#define INC_HOOK

#include <windows.h>

class CHookLib {
	public:
		static unsigned long* GetVtableAddress(void* pObject);
		static unsigned long* DetourWithVtable(void* pObject, unsigned int offset, unsigned long* hookProc);
};

#endif