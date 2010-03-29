// DirectX9 Hook by AnthIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk or anthiste.anthiste@gmail.com,
// Thanks to Echo and others for advice.
//
// p.s. Node is sexy ^_^.

#include <windows.h>
#include <d3dx9.h>
#include <d3d9.h>

#include "dx9table.h"
#include "dx9hook.h"

HRESULT (APIENTRY *pfnEndScene)(IDirect3DDevice9 *pDevice);
HRESULT APIENTRY hook_EndScene(IDirect3DDevice9 *pDevice)
{
	D3DRECT rec = { 0, 0, 20, 20 };                     
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 0,  0); 

	return pfnEndScene(pDevice);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE)hInstance);
			InsertDetour(ENDSCENE, (FARPROC)&hook_EndScene, (FARPROC)&pfnEndScene);
			break;

		case DLL_PROCESS_DETACH:
			RemoveDetour(ENDSCENE);
			FreeLists();
			break;
	}

	return TRUE;
}