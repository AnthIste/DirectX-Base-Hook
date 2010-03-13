#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "directx9hook.h"

// Define a function pointer type for whatever you're hooking. Not necessary, but easier
typedef FARPROC (APIENTRY *EndScene_t)(IDirect3DDevice9* pDevice);

// Define a pointer to hold the original function
EndScene_t orig_EndScene;

// Define the detour function
FARPROC APIENTRY hook_EndScene(IDirect3DDevice9* pDevice)
{
	D3DRECT rec = {0, 0, 20, 20};                     
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 0,  0);  

	return orig_EndScene(pDevice);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			// Hook the function (as many as you need, at any time)
			CDirectX9Hook::DetourDirectX(42, (void*)hook_EndScene, (void*)&orig_EndScene);
			break;
	}

	return TRUE;
}