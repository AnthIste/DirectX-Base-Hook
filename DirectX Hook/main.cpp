#include <windows.h>
#include "directx9hook.h"

BOOL WINAPI DllMain(HINSTANCE hInstanceDll, DWORD dwReason, LPVOID reserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				//CDirectX9Hook::DetourDirectX(dxoffsets::endscence, (void*)hook_EndScene, (void*)&orig_EndScene);
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