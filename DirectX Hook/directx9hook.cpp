#include "directx9hook.h"
#include "../Injector/System.cpp"

HANDLE								CDirectX9Hook::hThread = 0;
CDirectX9Hook::addr_t				CDirectX9Hook::pVtable = 0;
CDirectX9Hook::DetourMap_t			CDirectX9Hook::detours;

CDirectX9Hook::Direct3DCreate9_t	CDirectX9Hook::orig_Direct3DCreate9 = 0;
CDirectX9Hook::CreateDevice_t		CDirectX9Hook::orig_CreateDevice = 0;

CDirectX9Hook& CDirectX9Hook::GetInstance()
{
	static CDirectX9Hook instance;

	return instance;
}

void CDirectX9Hook::DetourDirectX(unsigned int offset, void *pDetour, void* pOrig)
{
	// Create a Detour_t object to store info about the detour request because it is
	// uncertain when the hooks will be applied
	Detour_t detour = {reinterpret_cast<addr_t>(pDetour), reinterpret_cast<addr_t>(pOrig), offset};
	
	// Save the detour for later
	ScheduleDetour(detour);

	// Maybe the vtable has been located after a previous request
	if (!pVtable) {
		// if it hasnt, find it
		LocateDeviceVtable();
	} else {
		// If it has, simply apply whatever hooks need to be applied
		ApplyPendingHooks();
	}
}

void CDirectX9Hook::DetourRemove(unsigned int offset)
{
	Detour_t detour = detours[offset];

	void* origProc = reinterpret_cast<void*>(*(detour.pOrig));
	CHook::NewDetour((DWORD*)pVtable, offset, (FARPROC)origProc);
}

void CDirectX9Hook::LocateDeviceVtable()
{
	// Just make sure that we dont accidentaly apply any hooks twice if they've been
	// applied before
	if (pVtable)
		return;

	// Is DirectX available to be hooked?
	if (GetModuleHandle("d3d9.dll")) {
		// If it is, try a standard hook or a runtime hook
		HookNormal();
		HookRuntime();
	} else {
		// Try a dynamic hook (to be implemented)
	}
}

void CDirectX9Hook::ScheduleDetour(Detour_t detour)
{
	// Each offset is unique, so store it by offset for easy retrieval
	detours[detour.offset] = detour;
}

void CDirectX9Hook::HookNormal()
{
	// ENSURE that we dont't detour twice
	if (!pVtable) {
		// Hook Direct3DCreate9
		orig_Direct3DCreate9 = (Direct3DCreate9_t)DetourFunction((PBYTE)Direct3DCreate9, (PBYTE)hook_Direct3DCreate9);
	}
}

void CDirectX9Hook::HookDynamic()
{
	// To be implemented
}

void CDirectX9Hook::HookRuntime()
{
	unsigned long base_d3d9 = (unsigned long)GetModuleHandle("d3d9.dll");	// Base address of d3d9.dll.
	addr_t addr_jmp = (addr_t)(base_d3d9 + 0x6F20);							// Address of jmp patch.
	addr_t addr_cave = (addr_t)(base_d3d9 + 0x1A86DE);						// Address of code cave.
	addr_t deadbeef = (addr_t)&pVtable;										// Stores address of our vtable
																			// variable to copy into the asm
																			// instructions.

	// See documentation for ASM listing
	unsigned char patch_jmp[]	= {0xE9, 0xB9, 0x17, 0x1A, 0x00};
	unsigned char patch_cave[]	= {0x8B, 0x0E, 0x8B, 0x51, 0x04, 0x50, 0x8B, 0xC6, 0x8B, 0x00, 0xA3, 0xEF, 0xBE, 0xAD, 0xDE, 0x58, 0xE9, 0x32, 0xE8, 0xE5, 0xFF};
	
	// Copy the address of our vtable variable into the asm code so that the found vtable is copied
	// directly into a class variable
	memcpy((void*)((unsigned long)patch_cave + 11), (void*)&deadbeef, 4);

	// Remove write protection
	DWORD dwOldProtect;
	if (!VirtualProtect((void*)addr_jmp, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect) ||
		!VirtualProtect((void*)addr_cave, 21, PAGE_EXECUTE_READWRITE, &dwOldProtect))
			return;

	// Write the instructions to d3d9.dll
	memcpy((void*)addr_cave, (void*)patch_cave, 21);
	memcpy((void*)addr_jmp, (void*)patch_jmp, 5);

	// Restore protection
	VirtualProtect((void*)addr_jmp, 5, dwOldProtect, &dwOldProtect);
	VirtualProtect((void*)addr_cave, 21, dwOldProtect, &dwOldProtect);

	// To be removed: Rather use a callback that gets called directly from the code cave
	hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_WaitForVtableAndHook, 0, 0, 0);
}

void CDirectX9Hook::ApplyPendingHooks()
{
	DetourMap_t::iterator i;
	
	// Iterate through all hooks and apply them using a vtable hook
	for (i = detours.begin(); i != detours.end(); i++) {
		// Get the detour info from the map
		Detour_t detour = (*i).second;
		
		// Use vtable hook from Illuzi0N's CHook class to store original pointer and overwrite with the new one
		*(addr_t*)detour.pOrig = (addr_t)CHook::NewDetour((DWORD*)pVtable, detour.offset, (FARPROC)detour.pDetour);
	}

	// Clear the hooks, they've been applied
	detours.clear();
}

// this should be removed, a callback can be called from the code-cave. much better
DWORD WINAPI CDirectX9Hook::thread_WaitForVtableAndHook(void* param)
{
	//MessageBoxW(0, L"Waiting for vtable", L"DX Hook", MB_ICONINFORMATION);

	// Wait for vtable to get written by code cave
	while (!pVtable) {
		Sleep(10);
	}

	//MessageBoxW(0, L"vtable written. hooking", L"DX Hook", MB_ICONINFORMATION);
	ApplyPendingHooks();

	return TRUE;
}

IDirect3D9* APIENTRY CDirectX9Hook::hook_Direct3DCreate9(UINT sdkVersion)
{
	// Call the original function to get access to the object
	IDirect3D9* orig = orig_Direct3DCreate9(sdkVersion);

	// Some games call this more than once, so just make sure that we only hook once
	static bool hooked = false;
	
	if (!hooked && !pVtable) {
		// Get the vtable address of the created device
		addr_t d3dVtable = CHook::GetVtableAddress((void*)orig);
		
		if (d3dVtable) {
			// Hook CreateDevice
			orig_CreateDevice = (CreateDevice_t)CHook::NewDetour((DWORD*)d3dVtable, 16, (FARPROC)hook_CreateDevice);
		}
		hooked = true;
	}

	return orig;
}

HRESULT APIENTRY CDirectX9Hook::hook_CreateDevice(IDirect3D9* d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface)
{
	// Call the original to gain access to the created device
	HRESULT result = orig_CreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	/*
	// Debug info used to make runtime hook
	std::wstringstream ss;
	ss << L"d3d9.dll: " << (void*)GetModuleHandle("d3d9.dll") << L"\nDevice pointer: " << (void*)*ppReturnedDeviceInterface;
	MessageBoxW(0, ss.str().c_str(), L"DX Hook", MB_ICONINFORMATION);
	*/

	// Ensure that we dont hook twice
	if (!pVtable) {
		TerminateThread(hThread, 0);
		// Get the device's vtable
		pVtable = CHook::GetVtableAddress((void*)*ppReturnedDeviceInterface);
		
		// Hook whatever needs hooking
		ApplyPendingHooks();
	}

	return result;
}
