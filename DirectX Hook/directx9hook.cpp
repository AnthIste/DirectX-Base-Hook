#include "directx9hook.h"
#include "../Injector/System.cpp"

CDirectX9Hook::addr_t				CDirectX9Hook::pVtable = 0;
CDirectX9Hook::DetourMap_t			CDirectX9Hook::Detours;

CDirectX9Hook::Direct3DCreate9_t	CDirectX9Hook::oDirect3DCreate9 = 0;
CDirectX9Hook::CreateDevice_t		CDirectX9Hook::oCreateDevice = 0;
CRITICAL_SECTION					CDirectX9Hook::criticalSection;

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

	InitializeCriticalSection(&criticalSection);

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
	HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CreateDummyDevice, 0, 0, 0);
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

DWORD WINAPI CDirectX9Hook::CreateDummyDevice(void* param)
{
	//MessageBoxW(0, L"Attempting a runtime DirectX hook...", L"DX Hook", MB_ICONINFORMATION);
	//Sleep(5000);
	
	WNDCLASSEXW wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEXW));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass";

    RegisterClassExW(&wc);

    HWND hWndDummy = CreateWindowExW(NULL,
                          L"WindowClass",
                          L"Our First Direct3D Program",
                          WS_OVERLAPPEDWINDOW,
                          300, 300,
                          800, 600,
                          NULL,
                          NULL,
                          0,
                          NULL);

	if (!hWndDummy) {
		//MessageBoxW(0, L"Created dummy window", L"DX Hook", MB_ICONERROR);
		//MessageBoxW(0, System::GetSystemError().c_str(), L"DX Hook", MB_ICONERROR);
		return 0;
	} else {
		//MessageBoxW(0, L"Created dummy window", L"DX Hook", MB_ICONINFORMATION);
	}
	
	IDirect3D9* d3dDummy = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3dDummy) {
		//MessageBoxW(0, L"Created dummy IDirect3D object", L"DX Hook", MB_ICONERROR);
		return 0;
	} else {
		//MessageBoxW(0, L"Created dummy IDirect3D object", L"DX Hook", MB_ICONINFORMATION);
	}

	IDirect3DDevice9* d3dDevDummy;
	D3DPRESENT_PARAMETERS d3dpp;

	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.BackBufferCount = 1;
	d3dpp.Windowed = TRUE;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.BackBufferHeight = 0;
	d3dpp.BackBufferWidth = 0;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = 0;
	d3dpp.hDeviceWindow = hWndDummy;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	//MessageBoxW(0, L"Attempting to create dummy device...", L"DX Hook", MB_ICONINFORMATION);

    // create a device class using this information and the info from the d3dpp stuct
	HRESULT hResult = d3dDummy->CreateDevice(D3DADAPTER_DEFAULT,
					  D3DDEVTYPE_HAL,
                      hWndDummy,
					  D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3dDevDummy);

	if (hResult != D3D_OK || !d3dDevDummy) {
		//MessageBoxW(0, L"Created dummy d3d device", L"DX Hook", MB_ICONERROR);
		return 0;
	} else {
		//MessageBoxW(0, L"Created dummy d3d device", L"DX Hook", MB_ICONINFORMATION);
	}

	d3dDevDummy->Release();
	d3dDummy->Release();

	DestroyWindow(hWndDummy);

	return 1;
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
	EnterCriticalSection(&criticalSection);

	HRESULT hRes = oCreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	// Debug info used to make runtime hook
	/*std::wstringstream ss;
	ss << L"d3d9.dll: " << (void*)GetModuleHandle("d3d9.dll") << L"\nDevice pointer: " << (void*)*ppReturnedDeviceInterface;
	MessageBoxW(0, ss.str().c_str(), L"DX Hook", MB_ICONINFORMATION);*/
	
	// Hook whatever needs hooking
	if (!pVtable) {
		pVtable = CHook::GetVtableAddress((void *)*ppReturnedDeviceInterface);
		//ss << L"vtable: " << (void*)pVtable;
		//MessageBoxW(0, ss.str().c_str(), L"DX Hook", MB_ICONINFORMATION);
		ApplyPendingHooks();
	}

	LeaveCriticalSection(&criticalSection);

	return hRes;
}
