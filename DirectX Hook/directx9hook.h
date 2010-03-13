#ifndef INC_HOOKS
#define INC_HOOKS

#include <windows.h>
#include <detours.h>
#include <sstream>
#include <d3dx9.h>
#include <d3d9.h>
#include <map>

#include "../CHook/CHook.h"
#include "../Injector/System.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "../Release/CHook.lib")

class CDirectX9Hook {
	typedef DWORD *addr_t;								

	struct Detour_t {
		addr_t	detour;
		addr_t	orig;
		UINT	offset;
	};

	typedef std::map<unsigned int, Detour_t> DetourMap_t;

	public:
		static CDirectX9Hook& GetInstance();
		static void DetourDirectX(UINT uOffset, FARPROC pfnDetour, FARPROC pfnOrig);
		static void DetourRemove(UINT uOffset);

	private:
		CDirectX9Hook() { }
		~CDirectX9Hook() { }

		static void LocateDeviceVtable();
		static void ScheduleDetour(Detour_t detour);

		static HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return TRUE; }

		static void HookNormal();
		static void HookDynamic();
		static void HookRuntime();
		static void ApplyPendingHooks();

		static void CaveCallback();
		static DWORD WINAPI CreateDummyDevice(void* param);

		typedef IDirect3D9 *(APIENTRY *Direct3DCreate9_t)(UINT);
		static IDirect3D9 *APIENTRY hook_Direct3DCreate9(UINT sdkVersion);

		typedef HRESULT (APIENTRY *CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
		static HRESULT APIENTRY hook_CreateDevice(IDirect3D9* d3d, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice9 ** ppReturnedDeviceInterface);

		static addr_t pVtable;
		static DetourMap_t Detours;

		static Direct3DCreate9_t oDirect3DCreate9;
		static CreateDevice_t oCreateDevice;

		static CRITICAL_SECTION criticalSection;
};

#endif
