// DirectX9 Hook by AnthIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk or anthiste.anthiste@gmail.com,
// Thanks to Echo and others for advice.
//
// Built with Visual Studio 2008.
//
// p.s. Node is sexy ^_^.

// --------------------------------------------------------------------------
// EXAMPLE 1 (USING STATIC LIBRARY)
// --------------------------------------------------------------------------
//
// Demonstrates how to use the staticly compiled library instad of including
// the entire hook source code.
//
// The compiler paths should be set to include the /include and /lib directories
// for ease of use.
//
// --------------------------------------------------------------------------

#include <windows.h>
#include "../../../include/dx9hook.h"

#pragma comment(lib, "../../../lib/dx9hook.lib")

HRESULT (APIENTRY *pfnEndScene)( IDirect3DDevice9 *pDevice );
HRESULT APIENTRY hook_EndScene( IDirect3DDevice9 *pDevice )
{
	D3DRECT rec = { 0, 0, 20, 20 };                     
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 0,  0); 

	return pfnEndScene(pDevice);
}

BOOL APIENTRY DllMain( HMODULE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE)hInstance);
			DirectX9Detour(DX9_ENDSCENE, (FARPROC)&hook_EndScene, (FARPROC)&pfnEndScene);
			break;

		case DLL_PROCESS_DETACH:
			DirectX9Cleanup();
			break;
	}

	return TRUE;
}