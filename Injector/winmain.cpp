#include <windows.h>
#include "resource.h"
#include "injector.h"

void HandleEvent(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	static CInjector injector;

	switch (wParam) {
		case IDC_BTN_INJECT:
			{

			}
			break;

		case IDC_BTN_REFRESH:
			{
				SendMessage(GetDlgItem(hWnd, IDC_LIST_PROCESSES), LB_RESETCONTENT, 0, 0);

				CInjector::ProcessList_t processes = injector.GetProcessList();
				CInjector::ProcessList_t::iterator i;

				for (i = processes.begin(); i != processes.end(); i++) {
					SendMessage(GetDlgItem(hWnd, IDC_LIST_PROCESSES), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>((*i).c_str()));
				}

				SendMessage(GetDlgItem(hWnd, IDC_LIST_PROCESSES), LB_SETCURSEL, 0, 0);
			}
			break;

		case IDC_BTN_DLL:
			{

			}
			break;

		case IDC_BTN_UNLOAD:
			{

			}
			break;
	}
}

BOOL CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try {
		switch (msg) {
			case WM_CLOSE:
				EndDialog(hWnd, 0);
				break;

			case WM_COMMAND:
				HandleEvent(hWnd, wParam, lParam);
				break;
		}
	}
	catch (std::exception e) {
		MessageBoxW(hWnd, reinterpret_cast<LPCWSTR>(e.what()), L"Injector", MB_ICONERROR);
	}

	return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return DialogBoxW(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc);
}