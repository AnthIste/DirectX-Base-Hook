#include "directx9hook.h"
#include "../Injector/System.cpp"

HANDLE								CDirectX9Hook::hThread = 0;
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
	DWORD base_d3d9 = (DWORD)GetModuleHandle("d3d9.dll");					// Base address of d3d9.dll.
	addr_t addr_jmp = (addr_t)(base_d3d9 + 0x6F20);							// Address of jmp patch.
	addr_t addr_cave = (addr_t)(base_d3d9 + 0x1A86DE);						// Address of code cave.
	addr_t deadbeef = (addr_t)&pVtable;										// Stores address of our vtable
																			// variable to copy into the asm
																			// instructions.
	unsigned char patch_jmp[]	= {0xE9, 0xB9, 0x17, 0x1A, 0x00};
	unsigned char patch_cave[]	= {0x8B, 0x0E, 0x8B, 0x51, 0x04, 0x50, 0x8B, 0xC6, 0x8B, 0x00, 0xA3, 0xEF, 0xBE, 0xAD, 0xDE, 0x58, 0xE9, 0x32, 0xE8, 0xE5, 0xFF};
	
	memcpy((void *)((DWORD)patch_cave + 11), (void *)&deadbeef, 4);

	DWORD dwOldProtect;
	if (!VirtualProtect((void *)addr_jmp, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect) ||
		!VirtualProtect((void *)addr_cave, 21, PAGE_EXECUTE_READWRITE, &dwOldProtect))
			return;

	memcpy((void *)addr_cave, (void *)patch_cave, 21);
	memcpy((void *)addr_jmp, (void *)patch_jmp, 5);

	VirtualProtect((void *)addr_jmp, 5, dwOldProtect, &dwOldProtect);
	VirtualProtect((void *)addr_cave, 21, dwOldProtect, &dwOldProtect);

	hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_WaitForVtableAndHook, 0, 0, 0);
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

DWORD WINAPI CDirectX9Hook::thread_WaitForVtableAndHook(LPVOID lpParam)
{
	while (!pVtable) {
		Sleep(10);
	}
	ApplyPendingHooks();

	return TRUE;
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

	if (!pVtable) {
		TerminateThread(hThread, 0);
		pVtable = CHook::GetVtableAddress((void *)*ppReturnedDeviceInterface);
		ApplyPendingHooks();
	}

	return hRes;
}
