#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "directx9hook.h"

typedef FARPROC (APIENTRY *EndScene_t)(IDirect3DDevice9 *pDevice);
EndScene_t o_pfnEndScene;


FARPROC APIENTRY hook_EndScene(IDirect3DDevice9 *pDevice)
{
	D3DRECT rec = {0, 0, 20, 20};                     
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 0,  0);  

	return o_pfnEndScene(pDevice);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH) {
			CDirectX9Hook::DetourDirectX(42, (FARPROC)&hook_EndScene, (FARPROC)&o_pfnEndScene);
	}

	return TRUE;
}