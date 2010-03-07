#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <sstream>
#include "directx9hook.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

typedef HRESULT (APIENTRY *EndScene_t)(IDirect3DDevice9*);
EndScene_t orig_EndScene;
HRESULT APIENTRY hook_EndScene(IDirect3DDevice9* pInterface);

HRESULT APIENTRY hook_EndScene(IDirect3DDevice9* pInterface)
{
	D3DRECT rec = {0, 0, 20, 20};
	pInterface->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 0,  0);
		
	return orig_EndScene(pInterface);
}

BOOL WINAPI DllMain(HINSTANCE hInstanceDll, DWORD dwReason, LPVOID reserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				/*std::wstringstream ss;
				ss << L"GetModuleHandle(d3d9.dll) = " << (void*)GetModuleHandleW(L"d3d9.dll");
				//MessageBoxW(0, ss.str().c_str(), L"Test", MB_ICONINFORMATION);*/
				CDirectX9Hook::DetourDirectX(35, (void*)hook_EndScene, (void*)&orig_EndScene);
				//CDirectX9Hook::DetourDirectX(dxoffsets::reset, (void*)hook_Reset, (void*)&orig_Reset);
			}
			break;

		case DLL_PROCESS_DETACH:
			{
				//CDirectX9Hook::DetourRemove(dxoffsets::endscene);
				//CDirectX9Hook::DetourRemove(dxoffsets::reset);
			}
			break;
	}

	return TRUE;
}