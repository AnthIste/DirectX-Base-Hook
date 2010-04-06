// DirectX9 Hook by AnthIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk or anthiste.anthiste@gmail.com,
// Thanks to Echo and others for advice.
//
// Built with Visual Studio 2008.
//
// p.s. Node is sexy ^_^.

#include "dx9hook.h"

DWORD		*pTable, *pDxTable;
Detour_t	*pDetList;
Dynamic_t	*pDynList;

IDirect3D9 *(__stdcall *pfnDirect3DCreate9)( UINT );
HRESULT		(__stdcall *pfnCreateDevice)( IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9** );
NTSTATUS	(__stdcall *pfnLdrGetProcedureAddress)( PVOID, PANSI_STRING, ULONG, PVOID* );


IDirect3D9 *__stdcall hook_Direct3DCreate9( UINT sdkVersion )
{
	IDirect3D9 *iDirect3D9 = pfnDirect3DCreate9(sdkVersion); 

	static bool bHooked = false;
	
	pDxTable = (DWORD *)(*(DWORD *)((void *)iDirect3D9));
	if(pDxTable && !bHooked) {
		*(FARPROC *)&pfnCreateDevice = NewDetour((DWORD *)pDxTable, 16, (FARPROC)hook_CreateDevice);
		DetourRemove((LPBYTE)hook_Direct3DCreate9, (LPBYTE)pfnDirect3DCreate9);
		bHooked = !bHooked;
	}
	
	return iDirect3D9;
}

HRESULT __stdcall hook_CreateDevice( IDirect3D9 *d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface )
{
	HRESULT hRes = pfnCreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	static bool bHooked = false;

	if(!pTable && !bHooked) {
		pTable = (DWORD *)(*(DWORD *)((void *)*ppReturnedDeviceInterface));
		NewDetour((DWORD *)pDxTable, 16, (FARPROC)pfnCreateDevice);
		SetSheduledHooks();		
		bHooked = !bHooked;
	}

	return hRes;
}

NTSTATUS __stdcall LdrGetProcedureAddress( PVOID BaseAddress, PANSI_STRING Name, ULONG Ordinal, PVOID *ProcedureAddress )
{
	if(pDynList) {
		Dynamic_t *pWalkList = pDynList;
		while(pWalkList != NULL) {
			if(!strcmp(Name->Buffer, pWalkList->lpFnName)) {
				*ProcedureAddress = (PVOID)pWalkList->pfnDetour;
				return STATUS_SUCCESS;
			}
			pWalkList = pWalkList->next;
		}
	} 

	return pfnLdrGetProcedureAddress(BaseAddress, Name, Ordinal, ProcedureAddress);
}

void InsertDynamicDetour( LPCSTR lpLibName, LPCSTR lpFnName, FARPROC pfnDetour )
{
	static bool bHooked = false;

	if(!bHooked) {
		*(FARPROC *)&pfnLdrGetProcedureAddress = (FARPROC)DetourFunction(DetourFindFunction("ntdll.dll", "LdrGetProcedureAddress"), (LPBYTE)LdrGetProcedureAddress);
		bHooked = !bHooked;
	}
	
	if(!pDynList) {
		pDynList = (Dynamic_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Dynamic_t));
		if(pDynList) {
			pDynList->lpFnName	= lpFnName;
			pDynList->lpLibName	= lpLibName;
			pDynList->pfnDetour	= pfnDetour;
			pDynList->next		= NULL;
		} else {
			return;
		}
	} else {
		Dynamic_t *pWalkList = pDynList;
		while(pWalkList)
			pWalkList = pWalkList->next;

		pWalkList = (Dynamic_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Dynamic_t));

		if(pWalkList) {
			pWalkList->lpFnName		= lpFnName;
			pWalkList->lpLibName	= lpLibName;
			pWalkList->pfnDetour	= pfnDetour;
			pWalkList->next			= NULL;
		}
	}
}

void DirectX9Detour( UINT uOffset, FARPROC pfnDetour, FARPROC pfnOrig )
{
	if(!pDetList) {
		pDetList = (Detour_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Detour_t));
		if(pDetList) {
			pDetList->detour	= (DWORD *)pfnDetour;
			pDetList->orig		= (DWORD *)pfnOrig;
			pDetList->offset	= uOffset;			
			pDetList->next		= NULL;
		} else {
			return;
		}
	} else {
		Detour_t *pWalkList = pDetList;
		while(pWalkList)
			pWalkList = pWalkList->next;

		pWalkList = (Detour_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Detour_t));

		if(pWalkList) {
			pDetList->detour	= (DWORD *)pfnDetour;
			pDetList->orig		= (DWORD *)pfnOrig;
			pDetList->offset	= uOffset;			
			pDetList->next		= NULL;
		}
	}

	if(!pTable) {
		if(GetModuleHandle("d3d9.dll")) {
			*(FARPROC *)&pfnDirect3DCreate9 = (FARPROC)DetourFunction((LPBYTE)Direct3DCreate9, (LPBYTE)hook_Direct3DCreate9);
			InsertDirectX9Cave();
		} else {
			InsertDynamicDetour("d3d9.dll", "Direct3DCreate9", (FARPROC)&hook_Direct3DCreate9);
		}
	}
	else {
		SetSheduledHooks();
	}
}

void RemoveDetour( UINT uOffset )
{
	if(!pDetList)
		return;

	Detour_t *pWalkList = pDetList, *pTemp = NULL;
	while(pWalkList) {
		if(pWalkList->offset == uOffset && pWalkList->detour && pTable) {
			NewDetour((DWORD *)pTable, pWalkList->offset, (FARPROC)*(DWORD **)pWalkList->orig);
			if(pWalkList->next)
				pTemp = pWalkList->next;
			HeapFree(GetProcessHeap(), 0, pWalkList);
			pWalkList = pTemp;
			break;
		}
		pWalkList = pWalkList->next;
	}
}

void SetSheduledHooks( void )
{
	if(pDetList) {
		Detour_t *pWalkList = pDetList;
		while(pWalkList) {
			if(pWalkList->offset && pWalkList->detour)
				*(DWORD **)pWalkList->orig = (DWORD *)NewDetour((DWORD *)pTable, pWalkList->offset, (FARPROC)pWalkList->detour);	
			pWalkList = pWalkList->next;
		}
	}
}

void DirectX9Cleanup( void )
{
	if(pDetList) {
		Detour_t *pWalkList = pDetList;
		while(pWalkList) {
			pWalkList = pWalkList->next;
			NewDetour((DWORD *)pTable, pDetList->offset, (FARPROC)*(DWORD **)pDetList->orig);
			HeapFree(GetProcessHeap(), 0, pDetList);
			pDetList = pWalkList;
		}
	}

	if(pDynList) {
		Dynamic_t *pWalkList = pDynList;
		while(pWalkList) {
			pWalkList = pWalkList->next;
			HeapFree(GetProcessHeap(), 0, pDynList);
			pDynList = pWalkList;
		}
	}
}

FARPROC NewDetour( DWORD *pVtable, UINT nFuncOffset, FARPROC pfnNewFunc ) 
{
	DWORD dwOldProtect, *dwFuncAddress = (DWORD *)((DWORD)pVtable + nFuncOffset);

	if(!VirtualProtect((void *)dwFuncAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return NULL;

#ifdef _WIN32
	FARPROC pfnOrig = (FARPROC)InterlockedExchange((PLONG)&pVtable[nFuncOffset], (LONG)((DWORD)pfnNewFunc));
#else
	FARPROC pfnOrig = (FARPROC)InterlockedExchange64((PLONGLONG)&pVtable[nFuncOffset], (LONG)((DWORD)pfnNewFunc));
#endif

	VirtualProtect((void *)dwFuncAddress, sizeof(DWORD), dwOldProtect, NULL);

	return pfnOrig;
}

void InsertDirectX9Cave( void )
{
	DWORD dwOldJump, dwOldCave, dwDx9Base = (DWORD)GetModuleHandle("d3d9.dll");
	DWORD *dwDx9Jump = (DWORD *)(dwDx9Base + 0x0B91B);
	DWORD *dwDx9Cave = (DWORD *)(dwDx9Base + 0x1A86DE);
	
	static DWORD  *pSetHookPtr = (DWORD *)DirectX9Callback;
	static DWORD  **pFuncPtr = &pSetHookPtr, *pTableAddress = (DWORD *)&pTable;

	UCHAR patch_jmp[]	= { 0xE9, 0xBE, 0xCD, 0x19, 0x00 };
	UCHAR patch_cave[]	= { 0x8B, 0x08, 0x8B, 0x51, 0x04, 0x50, 0x8B, 0x00, 0xA3,
						    0x90, 0x90, 0x90, 0x90, 0xA1, 0x66, 0x66, 0x66, 0x66,
							0x60, 0xFF, 0xD0, 0x61, 0x58, 0xE9, 0x26, 0x32, 0xE6, 0xFF };

	memcpy((void *)((DWORD)patch_cave +  9), (void *)&pTableAddress, 4);
	memcpy((void *)((DWORD)patch_cave + 14), (void *)&pFuncPtr, 4);

	if(!VirtualProtect((void *)dwDx9Jump,  5, PAGE_EXECUTE_READWRITE, &dwOldJump) ||
	   !VirtualProtect((void *)dwDx9Cave, 28, PAGE_EXECUTE_READWRITE, &dwOldCave))
			return;

	memcpy((void *)dwDx9Cave, &patch_cave, 28);
	memcpy((void *)dwDx9Jump, &patch_jmp,   5);

	VirtualProtect((void *)dwDx9Jump,  5, dwOldJump, NULL);
	VirtualProtect((void *)dwDx9Cave, 21, dwOldCave, NULL);
}

void DirectX9Callback( void )
{
	SetSheduledHooks();

	DWORD dwOldJump, dwDx9Base = (DWORD)GetModuleHandle("d3d9.dll");
	DWORD *dwDx9Jump = (DWORD *)(dwDx9Base + 0x0B91B);
	
	if(!VirtualProtect((void *)dwDx9Jump, 5, PAGE_EXECUTE_READWRITE, &dwOldJump))
			return;

	memcpy((void *)dwDx9Jump, "\x8B\x08\x8B\x51\x04", 5);

	VirtualProtect((void *)dwDx9Jump, 5, dwOldJump, NULL);
}
