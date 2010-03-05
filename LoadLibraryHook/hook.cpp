#include "hook.h"

unsigned long GetVtableAddress(void* pObject)
{
	return *static_cast<unsigned long*>(object);
}