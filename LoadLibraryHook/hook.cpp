#include "hook.h"

unsigned long* GetVtableAddress(void* pObject)
{
	return static_cast<unsigned long*>(*static_cast<unsigned long*>(pObject));
}

unsigned long* DetourWithVtable(unsigned long* vtableAddress, unsigned int offset, unsigned long* hookProc)
{
	
}