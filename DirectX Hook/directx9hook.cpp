#include "directx9hook.h"

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
	Detour_t detour = {reinterpret_cast<addr_t>(pDetour), reinterpret_cast<addr_t>(pOrig), offset};
	ScheduleDetour(detour);

	InitiateDetourProcedure();
	ApplyPendingHooks();
}

void CDirectX9Hook::DetourRemove(unsigned int offset)
{
	Detour_t detour = detours[offset];

	void* origProc = reinterpret_cast<void*>(*(detour.pOrig));
	CHook::DetourWithVtable((void*)pDevice, offset, (addr_t)origProc);
}

void CDirectX9Hook::InitiateDetourProcedure()
{
	static bool complete = false;
	if (complete) {
		return;
	}
	// see readme.txt

	HookRuntime();

	complete = true;
}

void CDirectX9Hook::ScheduleDetour(Detour_t detour)
{
	detours[detour.offset] = detour;
}

void CDirectX9Hook::HookNormal()
{

}

void CDirectX9Hook::HookDynamic()
{

}

void CDirectX9Hook::HookRuntime()
{
	MessageBoxW(0, L"Attempting a runtime DirectX hook", L"DX Hook", MB_ICONINFORMATION);

	WNDCLASSEX winClass; 
	memset(&winClass, 0, sizeof(winClass));
    
    winClass.lpszClassName = "Dummy_Window_Class";
    winClass.cbSize        = sizeof(WNDCLASSEX);
    winClass.style         = CS_HREDRAW | CS_VREDRAW;
    winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassEx(&winClass);

    HWND hWndDummy = CreateWindowEx( NULL, "Dummy_Window_Class", 
                               "Dummy Window",
                               WS_OVERLAPPEDWINDOW,
                               0, 0, 1, 1, NULL, NULL, 0, NULL );
	
	IDirect3D9* d3dDummy = Direct3DCreate9(D3D_SDK_VERSION);
	IDirect3DDevice9* d3dDevDummy;
	
	D3DPRESENT_PARAMETERS ppDummy;
	memset(&ppDummy, 0, sizeof(ppDummy));

	ppDummy.BackBufferCount= 1;  //We only need a single back buffer
	ppDummy.MultiSampleType=D3DMULTISAMPLE_NONE; //No multi-sampling
	ppDummy.MultiSampleQuality=0;                //No multi-sampling
	ppDummy.SwapEffect = D3DSWAPEFFECT_DISCARD;  // Throw away previous frames, we don't need them
	ppDummy.hDeviceWindow = hWndDummy;  //This is our main (and only) window
	ppDummy.Flags=0;            //No flags to set
	ppDummy.FullScreen_RefreshRateInHz=D3DPRESENT_RATE_DEFAULT; //Default Refresh Rate
	ppDummy.PresentationInterval=D3DPRESENT_INTERVAL_DEFAULT;   //Default Presentation rate
	ppDummy.BackBufferFormat=D3DFMT_UNKNOWN;      //Display format
	ppDummy.EnableAutoDepthStencil=FALSE; //No depth/stencil buffer

	d3dDummy->CreateDevice(D3DADAPTER_DEFAULT, //The default adapter, on a multi-monitor system
                                              //there can be more than one.
                          D3DDEVTYPE_HAL, //Use hardware acceleration rather than the software renderer
                          //Our Window
                          hWndDummy,
                          //Process vertices in software. This is slower than in hardware,
                          //But will work on all graphics cards.
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                          //Our D3DPRESENT_PARAMETERS structure, so it knows what we want to build
                          &ppDummy,
                          //This will be set to point to the new device
                          &d3dDevDummy);

	//orig_CreateDevice = (CreateDevice_t)CHook::DetourWithVtable((void*)d3dDevDummy, 15, (addr_t)hook_CreateDevice);
	// Hook some function and call it to get the pDevice

	d3dDevDummy->Release();
	d3dDummy->Release();

	DestroyWindow(hWndDummy);
}

void CDirectX9Hook::ApplyPendingHooks()
{
	DetourMap_t::iterator i;
	for (i = detours.begin(); i != detours.end(); i++) {
		Detour_t detour = (*i).second;

		*(addr_t*)detour.pOrig = (addr_t)CHook::DetourWithVtable((void*)pDevice, detour.offset, detour.pDetour);
	}

	detours.clear();
}

HRESULT APIENTRY CDirectX9Hook::hook_CreateDevice(IDirect3DDevice9* pDevice, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface)
{
	CDirectX9Hook::pDevice = pDevice;

	// Hook everything
	ApplyPendingHooks();

	return orig_CreateDevice(pDevice, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
}