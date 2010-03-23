// DirectX9 Hook by AnthIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk or
// anthiste.anthiste@gmail.com,
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

HRESULT (APIENTRY *pfnDrawIndexedPrimitive)(IDirect3DDevice9 *pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
HRESULT APIENTRY hook_DrawIndexedPrimitive(IDirect3DDevice9 *pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	if (GetAsyncKeyState('R'))
		DirectX9Hook::RemoveDetour(DirectX9Hook::ENDSCENE);

	if (GetAsyncKeyState('E')) {
		DirectX9Hook::InsertDetour(DirectX9Hook::ENDSCENE, (FARPROC)&hook_EndScene, (FARPROC)&pfnEndScene);
		Sleep(500);
	}

	return pfnDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE)hInstance);
			DirectX9Hook::InsertDetour(DirectX9Hook::ENDSCENE, (FARPROC)&hook_EndScene, (FARPROC)&pfnEndScene);
			DirectX9Hook::InsertDetour(DirectX9Hook::DRAWINDEXEDPRIMITIVE, (FARPROC)&hook_DrawIndexedPrimitive, (FARPROC)&pfnDrawIndexedPrimitive);
			break;

		case DLL_PROCESS_DETACH:
			DirectX9Hook::RemoveDetour(DirectX9Hook::ENDSCENE);
			DirectX9Hook::RemoveDetour(DirectX9Hook::DRAWINDEXEDPRIMITIVE);
			DirectX9Hook::FreeLists();
	}

	return TRUE;
}