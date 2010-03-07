#ifndef INC_HOOKS
#define INC_HOOKS

#include <windows.h>
#include <detours.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include <algorithm>
#include "../CHook/CHook.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "../Release/CHook.lib")

class CDirectX9Hook {
	// Types used in the hook
	typedef unsigned long* addr_t;							// describes an address / 4-byte ptr

	struct Detour_t {
		addr_t pDetour;										// Ptr to detour function
		addr_t pOrig;										// Ptr to original function
		unsigned int offset;								// Offset in vtable
	};

	typedef std::map<unsigned int, Detour_t> DetourMap_t;	// Holds detours based on their offset

	public:
		// Interface
		static CDirectX9Hook& GetInstance();
		static void DetourDirectX(unsigned int offset, void* pDetour, void* pOrig);
		static void DetourRemove(unsigned int offset);

	private:
		// Static class
		CDirectX9Hook() {}
		~CDirectX9Hook() {}

		static void InitiateDetourProcedure();
		static void ScheduleDetour(Detour_t detour);

		// Different hook methods
		static void HookNormal();
		static void HookDynamic();
		static void HookRuntime();

		// static members
		static addr_t pVtable;
		static IDirect3DDevice9* pDevice;
		static DetourMap_t detours;
};

#endif