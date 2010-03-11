#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "directx9hook.h"

typedef FARPROC (APIENTRY *EndScene_t)(IDirect3DDevice9* pDevice);
EndScene_t orig_EndScene;

FARPROC APIENTRY hook_EndScene(IDirect3DDevice9* pDevice)
{
	//MessageBoxA(0, "Detoured EndScene!", "BLEH!!!", MB_ICONINFORMATION);
	D3DRECT rec = {0, 0, 20, 20};                     
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 0,  0);  

	return orig_EndScene(pDevice);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			//MessageBoxA(0, "Dll Injected!", "BLEH!!!", MB_ICONINFORMATION);
			CDirectX9Hook::DetourDirectX(42, (void*)hook_EndScene, (void*)&orig_EndScene);
			break;
	}

	return TRUE;
}