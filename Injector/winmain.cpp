#include <windows.h>
#include "resource.h"
#include "injector.h"

BOOL CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			break;
	}
	return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CInjector injector;
	CInjector::ProcessList_t processes = injector.GetProcessList();

	return DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc);
}