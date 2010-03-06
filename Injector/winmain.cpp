#include <windows.h>
#include "injector.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CInjector injector;
	CInjector::ProcessList_t processes = injector.GetProcessList();

	return 0;
}