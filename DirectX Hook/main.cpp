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


BOOL APIENTRY DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Apply hooks

			break;
		}

		case DLL_PROCESS_DETACH:
		{
			// Remove hooks & Cleanup

			FreeLists();
			break;
		}
	}

	return TRUE;
}