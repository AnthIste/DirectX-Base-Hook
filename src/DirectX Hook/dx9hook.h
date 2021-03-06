// DirectX9 Hook by AnthIste and illuz1oN (C) 2010,
// Contact us at illuz1oN@hotmail.co.uk or anthiste.anthiste@gmail.com,
// Thanks to Echo and others for advice.
//
// Built with Visual Studio 2008.
//
// p.s. Node is sexy ^_^.

#ifndef __DIRECTX9_HOOK
#define __DIRECTX9_HOOK

#include <windows.h>
#include <detours.h>
#include <d3dx9.h>
#include <d3d9.h>
#include <vector>

#include "dx9table.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "detours.lib")

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif 

typedef struct _STRING {
	USHORT	Length;
	USHORT	MaximumLength;
	PCHAR	Buffer;
} ANSI_STRING, *PANSI_STRING;

struct Detour_t {
	DWORD	 *detour;
	DWORD	 *orig;
	UINT	 offset;
	Detour_t *next;
};

struct Dynamic_t {
	LPCSTR	  lpLibName;
	LPCSTR	  lpFnName;
	FARPROC	  pfnDetour;
	Dynamic_t *next;
};

HRESULT		__stdcall hook_CreateDevice( IDirect3D9 *d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface );
IDirect3D9 *__stdcall hook_Direct3DCreate9( UINT sdkVersion );
NTSTATUS	__stdcall LdrGetProcedureAddress( PVOID BaseAddress, PANSI_STRING Name, ULONG Ordinal, PVOID *ProcedureAddress );

void	InsertDynamicDetour( LPCSTR lpLibName, LPCSTR lpFnName, FARPROC pfnDetour );
void	DirectX9Detour( UINT uOffset, FARPROC pfnDetour, FARPROC pfnOrig );
void	DirectX9RemoveDetour( UINT uOffset );
void	SetSheduledHooks( void );
void	DirectX9Cleanup( void );
FARPROC NewDetour( DWORD *pVtable, UINT nFuncOffset, FARPROC pfnNewFunc );
void	InsertDirectX9Cave( void );
void	DirectX9Callback( void );

#endif