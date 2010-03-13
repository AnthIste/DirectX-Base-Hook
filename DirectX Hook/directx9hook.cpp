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
	//MessageBoxW(0, L"Received a detour request", L"DX Hook", MB_ICONINFORMATION);

	Detour_t detour = {reinterpret_cast<addr_t>(pDetour), reinterpret_cast<addr_t>(pOrig), offset};
	ScheduleDetour(detour);

	if (!pVtable) {
		LocateDeviceVtable();
	} else {
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
	//MessageBoxW(0, L"Locating vtable for hooking", L"DX Hook", MB_ICONINFORMATION);
	
	if (pVtable)
		return;

	// see readme.txt
	if (GetModuleHandle("d3d9.dll")) {
		HookNormal();
		HookRuntime();
	} else {
		// Dynamic hook
	}
}

void CDirectX9Hook::ScheduleDetour(Detour_t detour)
{
	detours[detour.offset] = detour;
}

void CDirectX9Hook::HookNormal()
{
	//MessageBoxW(0, L"Attempting a normal DirectX hook...", L"DX Hook", MB_ICONINFORMATION);

	if (!pVtable) {
		//MessageBoxW(0, L"HookingDirect3DCreate9 ", L"DX Hook", MB_ICONINFORMATION);
		orig_Direct3DCreate9 = (Direct3DCreate9_t)DetourFunction((PBYTE)Direct3DCreate9, (PBYTE)hook_Direct3DCreate9);
	}
}

void CDirectX9Hook::HookDynamic()
{

}

void CDirectX9Hook::HookRuntime()
{
	unsigned long base_d3d9 = (unsigned long)GetModuleHandle("d3d9.dll");
	addr_t addr_jmp = (addr_t)(base_d3d9 + 0x6F20);
	addr_t addr_cave = (addr_t)(base_d3d9 + 0x1A86DE);
	addr_t deadbeef = (addr_t)&pVtable;

	unsigned char patch_jmp[]	= {0xE9, 0xB9, 0x17, 0x1A, 0x00};
	unsigned char patch_cave[]	= {0x8B, 0x0E, 0x8B, 0x51, 0x04, 0x50, 0x8B, 0xC6, 0x8B, 0x00, 0xA3, 0xEF, 0xBE, 0xAD, 0xDE, 0x58, 0xE9, 0x32, 0xE8, 0xE5, 0xFF};
	
	memcpy((void*)((unsigned long)patch_cave + 11), (void*)&deadbeef, 4);

	DWORD dwOldProtect;
	if (!VirtualProtect((void*)addr_jmp, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect) ||
		!VirtualProtect((void*)addr_cave, 21, PAGE_EXECUTE_READWRITE, &dwOldProtect))
			return;

	//MessageBoxW(0, L"Writing code cave", L"DX Hook", MB_ICONINFORMATION);

	memcpy((void*)addr_cave, (void*)patch_cave, 21);
	memcpy((void*)addr_jmp, (void*)patch_jmp, 5);

	VirtualProtect((void*)addr_jmp, 5, dwOldProtect, &dwOldProtect);
	VirtualProtect((void*)addr_cave, 21, dwOldProtect, &dwOldProtect);

	hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_WaitForVtableAndHook, 0, 0, 0);
}

void CDirectX9Hook::ApplyPendingHooks()
{
	//MessageBoxW(0, L"Applying all pending hooks...", L"DX Hook", MB_ICONINFORMATION);

	DetourMap_t::iterator i;
	for (i = detours.begin(); i != detours.end(); i++) {
		//MessageBoxW(0, L"Hooking", L"DX Hook", MB_ICONINFORMATION);
		Detour_t detour = (*i).second;
		*(addr_t*)detour.pOrig = (addr_t)CHook::NewDetour((DWORD*)pVtable, detour.offset, (FARPROC)detour.pDetour);
	}

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
	IDirect3D9* orig = orig_Direct3DCreate9(sdkVersion);

	static bool hooked = false;
	if (!hooked && !pVtable) {
		//MessageBoxW(0, L"Inside hooked Direct3DCreate9\nHooking CreateDevice", L"DX Hook", MB_ICONINFORMATION);
		addr_t d3dVtable = CHook::GetVtableAddress((void*)orig);
		if (d3dVtable) {
			orig_CreateDevice = (CreateDevice_t)CHook::NewDetour((DWORD*)d3dVtable, 16, (FARPROC)hook_CreateDevice);
			//MessageBoxW(0, L"Done", L"DX Hook", MB_ICONINFORMATION);
		} else {
			//MessageBoxW(0, L"IDirect3D vtable not found", L"DX Hook", MB_ICONINFORMATION);
		}
		hooked = true;
	}

	return orig;
}

HRESULT APIENTRY CDirectX9Hook::hook_CreateDevice(IDirect3D9* d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface)
{
	HRESULT result = orig_CreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	std::wstringstream ss;
	ss << L"d3d9.dll: " << (void*)GetModuleHandle("d3d9.dll") << L"\nDevice pointer: " << (void*)*ppReturnedDeviceInterface;
	MessageBoxW(0, ss.str().c_str(), L"DX Hook", MB_ICONINFORMATION);

	if (!pVtable) {
		TerminateThread(hThread, 0);
		pVtable = CHook::GetVtableAddress((void*)*ppReturnedDeviceInterface);
		ApplyPendingHooks();
	}

	return result;
}
