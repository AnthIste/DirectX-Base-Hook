#include "directx9hook.h"
#include "../Injector/System.cpp"

CDirectX9Hook::addr_t				CDirectX9Hook::pVtable = 0;
CDirectX9Hook::DetourMap_t			CDirectX9Hook::Detours;

CDirectX9Hook::Direct3DCreate9_t	CDirectX9Hook::oDirect3DCreate9 = 0;
CDirectX9Hook::CreateDevice_t		CDirectX9Hook::oCreateDevice = 0;

CDirectX9Hook& CDirectX9Hook::GetInstance()
{
	static CDirectX9Hook DxInstance;

	return DxInstance;
}

void CDirectX9Hook::DetourDirectX(UINT uOffset, FARPROC pfnDetour, FARPROC pfnOrig)
{
	Detour_t DetourList = { (addr_t)pfnDetour, (addr_t)pfnOrig, uOffset };
	
	ScheduleDetour(DetourList);

	if(!pVtable)
		LocateDeviceVtable();
	else
		ApplyPendingHooks();
}

void CDirectX9Hook::DetourRemove(UINT uOffset)
{
	Detour_t detour = Detours[uOffset];

	if(detour.orig){
		addr_t origProc = *(addr_t *)detour.orig;
		CHook::NewDetour((addr_t)pVtable, uOffset, (FARPROC)origProc);
	}
}

void CDirectX9Hook::LocateDeviceVtable()
{
	if (pVtable)
		return;

	if (GetModuleHandle("d3d9.dll")) {
		HookNormal();
		HookRuntime();
	} else {
		HookDynamic();
	}
}

void CDirectX9Hook::ScheduleDetour(Detour_t detour)
{
	Detours[detour.offset] = detour;
}

void CDirectX9Hook::HookNormal()
{
	if (!pVtable) {
		oDirect3DCreate9 = (Direct3DCreate9_t)DetourFunction((PBYTE)Direct3DCreate9, (PBYTE)hook_Direct3DCreate9);
	}
}

void CDirectX9Hook::HookDynamic()
{

}

void CDirectX9Hook::HookRuntime()
{
	unsigned long base_d3d9 = (unsigned long)GetModuleHandle("d3d9.dll");	// Base address of d3d9.dll.
	addr_t addr_jmp = (addr_t)(base_d3d9 + 0x0B91B);							// Address of jmp patch.
	addr_t addr_cave = (addr_t)(base_d3d9 + 0x1A86DE);						// Address of code cave.
	
	// get the function pointer, store in in a variable and then store this
	// variables address so that it can be read from the ASM code:
	// MOV EAX,DWORD PTR DS:[66666666] ; 66666666 = ptr to function ptr
	// this extra step is needed coz a func ptr isnt a normal variable
	// funcptr is static, else it wont be available to read from later
	// when the ASM executes (stack variables will be destroyed)
	static addr_t funcptr = (addr_t)CaveCallback;
	static addr_t* pFuncPtr = &funcptr;

	addr_t deadbeef = (addr_t)&pVtable;										// Stores address of our vtable
																			// variable to copy into the asm
																			// instructions

	// See documentation for ASM listing
	unsigned char patch_jmp[]	= {0xE9, 0xBE, 0xCD, 0x19, 0x00};
	unsigned char patch_cave[]	= {0x8B, 0x08, 0x8B, 0x51, 0x04, 0x50, 0x8B, 0x00, 0xA3, 0xEF, 0xBE, 0xAD, 0xDE, 0xA1, 0x66, 0x66, 0x66, 0x66, 0x60, 0xFF, 0xD0, 0x61, 0x58, 0xE9, 0x26, 0x32, 0xE6, 0xFF};
	
	// Copy the address of our vtable variable into the asm code so that the found vtable is copied
	// directly into a class variable
	memcpy((void*)((unsigned long)patch_cave + 9), (void*)&deadbeef, 4);
	memcpy((void*)((unsigned long)patch_cave + 14), (void*)&pFuncPtr, 4);

	DWORD dwOldProtect;

	if (!VirtualProtect((void*)addr_jmp, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect) ||
		!VirtualProtect((void*)addr_cave, 28, PAGE_EXECUTE_READWRITE, &dwOldProtect))
			return;

	// Write the instructions to d3d9.dll
	memcpy((void*)addr_cave, (void*)patch_cave, 28);
	memcpy((void*)addr_jmp, (void*)patch_jmp, 5);

	// Restore protection
	VirtualProtect((void *)addr_jmp, 5, dwOldProtect, &dwOldProtect);
	VirtualProtect((void *)addr_cave, 21, dwOldProtect, &dwOldProtect);
}

void CDirectX9Hook::ApplyPendingHooks()
{
	DetourMap_t::iterator i;
	
	for (i = Detours.begin(); i != Detours.end(); i++) {
		Detour_t detour = (*i).second;
		*(addr_t*)detour.orig = (addr_t)CHook::NewDetour((addr_t)pVtable, detour.offset, (FARPROC)detour.detour);
	}

	Detours.clear();
}

void CDirectX9Hook::CaveCallback(void)
{
	ApplyPendingHooks();

	unsigned long base_d3d9 = (unsigned long)GetModuleHandle("d3d9.dll");	// Base address of d3d9.dll.
	addr_t addr_jmp = (addr_t)(base_d3d9 + 0x0B91B);							// Address of jmp patch.
	unsigned char patch_orig[] = {0x8B, 0x08, 0x8B, 0x51, 0x04};
	
	// Remove write protection
	DWORD dwOldProtect;
	if (!VirtualProtect((void*)addr_jmp, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect))
			return;

	// Write the instructions to d3d9.dll
	memcpy((void*)addr_jmp, (void*)patch_orig, 5);

	// Restore protection
	VirtualProtect((void*)addr_jmp, 5, dwOldProtect, &dwOldProtect);

	return;
}

IDirect3D9* APIENTRY CDirectX9Hook::hook_Direct3DCreate9(UINT sdkVersion)
{
	IDirect3D9 *orig = oDirect3DCreate9(sdkVersion);

	static bool bHooked = false;
	
	if (!bHooked && !pVtable) {
		addr_t d3dVtable = CHook::GetVtableAddress((void *)orig);
		
		if (d3dVtable) {
			oCreateDevice = (CreateDevice_t)CHook::NewDetour((addr_t)d3dVtable, 16, (FARPROC)hook_CreateDevice);
		}
		bHooked = !bHooked;
	}

	return orig;
}

HRESULT APIENTRY CDirectX9Hook::hook_CreateDevice(IDirect3D9* d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface)
{
	HRESULT hRes = oCreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	// Debug info used to make runtime hook
	/*std::wstringstream ss;
	ss << L"d3d9.dll: " << (void*)GetModuleHandle("d3d9.dll") << L"\nDevice pointer: " << (void*)*ppReturnedDeviceInterface;
	MessageBoxW(0, ss.str().c_str(), L"DX Hook", MB_ICONINFORMATION);*/
	
	// Hook whatever needs hooking
	if (!pVtable) {
		pVtable = CHook::GetVtableAddress((void *)*ppReturnedDeviceInterface);
		ApplyPendingHooks();
	}

	return hRes;
}
