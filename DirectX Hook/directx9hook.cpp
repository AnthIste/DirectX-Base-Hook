#include "directx9hook.h"
#include "../Injector/System.cpp"

CDirectX9Hook::addr_t				CDirectX9Hook::pVtable = 0;
CDirectX9Hook::DetourMap_t			CDirectX9Hook::detours;
IDirect3DDevice9*					CDirectX9Hook::pDevice = 0;

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
		InitiateDetourProcedure();
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

void CDirectX9Hook::InitiateDetourProcedure()
{
	//MessageBoxW(0, L"Initiating detour procedure", L"DX Hook", MB_ICONINFORMATION);

	static bool complete = false;
	if (complete) {
		//MessageBoxW(0, L"Detours have already been initiated", L"DX Hook", MB_ICONINFORMATION);

		ApplyPendingHooks();
		return;
	}
	// see readme.txt

		HookNormal();
		HookRuntime();

	complete = true;
}

void CDirectX9Hook::ScheduleDetour(Detour_t detour)
{
	detours[detour.offset] = detour;
}

void CDirectX9Hook::HookNormal()
{
	MessageBoxW(0, L"Attempting a normal DirectX hook...", L"DX Hook", MB_ICONINFORMATION);

	orig_Direct3DCreate9 = (Direct3DCreate9_t)DetourFunction((PBYTE)Direct3DCreate9, (PBYTE)hook_Direct3DCreate9);
}

void CDirectX9Hook::HookDynamic()
{

}

void CDirectX9Hook::HookRuntime()
{
	MessageBoxW(0, L"Attempting a runtime DirectX hook...", L"DX Hook", MB_ICONINFORMATION);
	
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
		MessageBoxW(0, L"Created dummy window", L"DX Hook", MB_ICONERROR);
		MessageBoxW(0, System::GetSystemError().c_str(), L"DX Hook", MB_ICONERROR);
		return;
	}
	
	IDirect3D9* d3dDummy = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3dDummy) {
		MessageBoxW(0, L"Created dummy IDirect3D object", L"DX Hook", MB_ICONERROR);
		return;
	}

	IDirect3DDevice9* d3dDevDummy;
	D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
    d3dpp.hDeviceWindow = hWndDummy;    // set the window to be used by Direct3D


    // create a device class using this information and the info from the d3dpp stuct
    d3dDummy->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWndDummy,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3dDevDummy);

	if (!d3dDevDummy) {
		MessageBoxW(0, L"Created dummy d3d device", L"DX Hook", MB_ICONERROR);
		return;
	}

	pVtable = CHook::GetVtableAddress((void*)d3dDevDummy);

	//MessageBoxW(0, L"Stored vtable", L"DX Hook", MB_ICONINFORMATION);

	//d3dDevDummy->Release();
	//d3dDummy->Release();

	//DestroyWindow(hWndDummy);

	ApplyPendingHooks();
}

void CDirectX9Hook::ApplyPendingHooks()
{
	MessageBoxW(0, L"Applying all pending hooks...", L"DX Hook", MB_ICONINFORMATION);

	DetourMap_t::iterator i;
	for (i = detours.begin(); i != detours.end(); i++) {
		MessageBoxW(0, L"Hooking", L"DX Hook", MB_ICONINFORMATION);
		Detour_t detour = (*i).second;
		*(addr_t*)detour.pOrig = (addr_t)CHook::NewDetour((DWORD*)pVtable, detour.offset, (FARPROC)detour.pDetour);
	}

	detours.clear();
}

IDirect3D9* APIENTRY CDirectX9Hook::hook_Direct3DCreate9(UINT sdkVersion)
{
	MessageBoxW(0, L"Hooked Direct3DCreate9", L"DX Hook", MB_ICONINFORMATION);

	IDirect3D9* orig = orig_Direct3DCreate9(sdkVersion);

	static bool hooked = false;
	if (!hooked) {
		addr_t d3dVtable = CHook::GetVtableAddress((void*)orig);
		orig_CreateDevice = (CreateDevice_t)CHook::NewDetour((DWORD*)d3dVtable, 15, (FARPROC)hook_CreateDevice);
		hooked = true;
	}

	return orig;
}

HRESULT APIENTRY CDirectX9Hook::hook_CreateDevice(IDirect3D9* d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface)
{
	MessageBoxW(0, L"Detoured CreateDevice", L"DX Hook", MB_ICONINFORMATION);

	HRESULT result = orig_CreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	pDevice = *ppReturnedDeviceInterface;
	pVtable = CHook::GetVtableAddress((void*)pDevice);
	ApplyPendingHooks();

	return result;
}
