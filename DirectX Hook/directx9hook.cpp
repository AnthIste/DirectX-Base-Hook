#include "directx9hook.h"

CDirectX9Hook::addr_t CDirectX9Hook::pVtable = 0;
CDirectX9Hook::DetourMap_t CDirectX9Hook::detours;
IDirect3DDevice9* CDirectX9Hook::pDevice;

CDirectX9Hook& CDirectX9Hook::GetInstance()
{
	static CDirectX9Hook instance;

	return instance;
}

void CDirectX9Hook::DetourDirectX(unsigned int offset, void *pDetour, void* pOrig)
{
	Detour_t detour = {reinterpret_cast<addr_t>(pDetour), reinterpret_cast<addr_t>(pOrig), offset};
	ScheduleDetour(detour);

	InitiateDetourProcedure();
}

void CDirectX9Hook::DetourRemove(unsigned int offset)
{
	Detour_t detour = detours[offset];

	void* origProc = reinterpret_cast<void*>(*(detour.pOrig));
	CHook::DetourWithVtable((void*)pDevice, offset, (addr_t)origProc);
}

void CDirectX9Hook::InitiateDetourProcedure()
{
	static bool complete = false;
	if (complete) {
		return;
	}
	// see readme.txt

	complete = true;
}

void CDirectX9Hook::ScheduleDetour(Detour_t detour)
{
	detours[detour.offset] = detour;
}

void CDirectX9Hook::HookNormal()
{

}

void CDirectX9Hook::HookDynamic()
{

}

void CDirectX9Hook::HookRuntime()
{

}