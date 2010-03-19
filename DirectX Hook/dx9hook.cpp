// DirectX9 Hook by AntIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk, 
// Thanks to Echo and others for advice.
//
// p.s. Node is sexy ^_^.
#include "dx9table.h"
#include "dx9hook.h"


DWORD			*pTable = 0, *pDxTable = 0;
DetourList_t	DetourList;
DynamicList_t	DynamicList;

IDirect3D9 *(__stdcall *pfnDirect3DCreate9)( UINT );
HRESULT (__stdcall *pfnCreateDevice)( IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9** );
NTSTATUS (__stdcall *pfnLdrGetProcedureAddress)(PVOID, PANSI_STRING, ULONG, PVOID*);


IDirect3D9 *__stdcall hook_Direct3DCreate9( UINT sdkVersion )
{
	IDirect3D9 *iDirect3D9 = pfnDirect3DCreate9(sdkVersion); 

	static bool bHooked = false;
	
	pDxTable = (DWORD *)(*(DWORD *)((void *)iDirect3D9));
	if(pDxTable && !bHooked) {
		*(FARPROC *)&pfnCreateDevice = NewDetour((DWORD *)pDxTable, 16, (FARPROC)hook_CreateDevice);
		DetourRemove((LPBYTE)hook_Direct3DCreate9, (LPBYTE)pfnDirect3DCreate9);
		bHooked = true;
	}
	
	return iDirect3D9;
}

HRESULT __stdcall hook_CreateDevice( IDirect3D9* d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface )
{
	HRESULT hRes = pfnCreateDevice(d3d, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	static bool bHooked = false;

	if(!pTable && !bHooked) {
		pTable = (DWORD *)(*(DWORD *)((void *)*ppReturnedDeviceInterface));
		NewDetour((DWORD *)pDxTable, 16, (FARPROC)pfnCreateDevice);
		SetSheduledHooks();
		
		bHooked = true;
	}

	return hRes;
}

NTSTATUS __stdcall LdrGetProcedureAddress( PVOID BaseAddress, PANSI_STRING Name, ULONG Ordinal, PVOID *ProcedureAddress )
{
	for(size_t i = 0; i < DynamicList.size(); i++) {
		if(!strcmp(Name->Buffer, DynamicList[i]->lpFnName)) {
			*ProcedureAddress = (PVOID)DynamicList[i]->pfnDetour;
			return STATUS_SUCCESS;
		}
	}
	return pfnLdrGetProcedureAddress(BaseAddress, Name, Ordinal, ProcedureAddress);
}

void InsertDynamicDetour( LPCSTR lpLibName, LPCSTR lpFnName, FARPROC pfnDetour )
{
	static bool bHooked = false;

	if(!bHooked) {
		*(FARPROC *)&pfnLdrGetProcedureAddress = (FARPROC)DetourFunction(DetourFindFunction("ntdll.dll", "LdrGetProcedureAddress"), (LPBYTE)LdrGetProcedureAddress);
		bHooked = true;
	}
	
	Dynamic_t *newDynamic = (Dynamic_t *)malloc(sizeof(Dynamic_t));
	if(!newDynamic)
		return;

	newDynamic->lpFnName	= lpFnName;
	newDynamic->lpLibName	= lpLibName;
	newDynamic->pfnDetour	= pfnDetour;
	DynamicList.push_back(newDynamic);
}

void InsertDetour( UINT uOffset, FARPROC pfnDetour, FARPROC pfnOrig )
{
	Detour_t *newDetour = (Detour_t *)malloc(sizeof(Detour_t));
	if(!newDetour)
		return;

	newDetour->detour	= (DWORD *)pfnDetour;
	newDetour->orig		= (DWORD *)pfnOrig;
	newDetour->offset	= uOffset;
	DetourList.push_back(newDetour);

	if(!pTable) {
		if(GetModuleHandle("d3d9.dll")) {
			*(FARPROC *)&pfnDirect3DCreate9 = (FARPROC)DetourFunction((LPBYTE)Direct3DCreate9, (LPBYTE)hook_Direct3DCreate9);
			InsertDirectX9Cave();
		} else {
			InsertDynamicDetour("d3d9.dll", "Direct3DCreate9", (FARPROC)&hook_Direct3DCreate9);
		}
	}
	else
		SetSheduledHooks();
}

void RemoveDetour( UINT uOffset )
{
	for(size_t i = 0; i < DetourList.size(); i++)
		if(DetourList[i]->offset == uOffset)
			NewDetour((DWORD *)pTable, uOffset, (FARPROC)*(DWORD **)DetourList[i]->orig);
}

void SetSheduledHooks( void )
{
	for(size_t i = 0; i < DetourList.size(); i++) 
		*(DWORD **)DetourList[i]->orig = (DWORD *)NewDetour((DWORD *)pTable, DetourList[i]->offset, (FARPROC)DetourList[i]->detour);
}

void FreeLists( void )
{
	size_t i, listMax = (DetourList.size() < DynamicList.size()) ? DynamicList.size() : DetourList.size();

	for(i = 0; i < listMax; i++) {
		if(DetourList[i])
			free(DetourList[i]);
		if(DynamicList[i])
			free(DynamicList[i]);
	}
}

FARPROC NewDetour( DWORD *pVtable, UINT nFuncOffset, FARPROC pfnNewFunc ) 
{
	DWORD dwOldProtect, *dwFuncAddress = (DWORD *)((DWORD)pVtable + nFuncOffset);

	if(!VirtualProtect((void *)dwFuncAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return NULL;

	FARPROC pfnOrig = (FARPROC)InterlockedExchange((PLONG)pVtable[nFuncOffset], (LONG)((DWORD)pfnNewFunc));

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

	return;
}