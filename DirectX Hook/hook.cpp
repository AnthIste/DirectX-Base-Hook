#include "hook.h"

unsigned long* CHookLib::GetVtableAddress(void* pObject)
{
	return reinterpret_cast<unsigned long*>(*static_cast<unsigned long*>(pObject));
}

unsigned long* CHookLib::DetourWithVtable(void* pObject, unsigned int offset, unsigned long* hookProc)
{
	// MUST be used else VirtualProtect will fail
	DWORD dwOldProtect;

	// Get the address in the vtable that holds the address of the function we want to hook
	unsigned long* vtableAddress = GetVtableAddress(pObject);
	void* lpBaseAddress = reinterpret_cast<void*>(reinterpret_cast<unsigned long>(vtableAddress) + offset);

	// Chances are the vtable is read/write protected, so change that
	if (!VirtualProtect(lpBaseAddress, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect)) {
		return 0;
	}

	// Read the original function address now that protection is removed
	unsigned long* origProc = reinterpret_cast<unsigned long*>(vtableAddress[offset]);

	// Replace it with our hook address
	vtableAddress[offset] = reinterpret_cast<unsigned long>(hookProc);

	// Restore protection
	VirtualProtect(lpBaseAddress, 4, dwOldProtect, &dwOldProtect);

	return origProc;
}