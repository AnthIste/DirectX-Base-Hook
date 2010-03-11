#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "directx9hook.h"

typedef FARPROC (APIENTRY *EndScene_t)(IDirect3DDevice9* pDevice);
EndScene_t orig_EndScene;

FARPROC APIENTRY hook_EndScene(IDirect3DDevice9* pDevice)
{
	return orig_EndScene(pDevice);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			MessageBoxA(0, "Dll Injected!", "BLEH!!!", MB_ICONINFORMATION);
			CDirectX9Hook::DetourDirectX(35, (void*)hook_EndScene, (void*)&orig_EndScene);
			break;
	}

	return TRUE;
}