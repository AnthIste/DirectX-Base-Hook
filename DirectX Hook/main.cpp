// DirectX9 Hook by AntIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk, 
// Thanks to Echo and others for advice.
//
// p.s. Node is sexy.

#include <windows.h>
#include <d3dx9.h>
#include <d3d9.h>

#include "dx9table.h"
#include "dx9hook.h"


FARPROC (APIENTRY *pfnEndScene)(IDirect3DDevice9 *pDevice);
FARPROC APIENTRY hook_EndScene(IDirect3DDevice9 *pDevice)
{
	D3DRECT rec = { 0, 0, 20, 20 };                     
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 0,  0);  

	return pfnEndScene(pDevice);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls((HMODULE)hInstance);
		InsertDetour(ENDSCENE, (FARPROC)&hook_EndScene, (FARPROC)&pfnEndScene);
	}

	return TRUE;
}