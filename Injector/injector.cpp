#include "injector.h"

// Static templated members must be explicitly declared in a source file
CInjector::ProcessMap_t CInjector::processes;
CInjector::ProcessList_t CInjector::processNames;

CInjector::CInjector()
{

}

CInjector::~CInjector()
{

}

// Injects the dll specified by dllPath into the target process
int CInjector::Inject(std::wstring dllPath, std::wstring processName)
{
	DWORD pId;
	HANDLE hProc, hThread;
	HMODULE hModule;
	int len;
	void* pRemoteString;
	FARPROC pLoadLibrary;
	std::wstring dllName = StripPath(dllPath);

	try {
		// Open the process & get the process handle
		pId = GetProcessIdByName(processName);
		hProc = OpenProcess(PROCESS_VM_WRITE | PROCESS_CREATE_THREAD, 0, pId);

		if (!hProc) {
			throw std::exception("Could not open process!");
		}

		// Allocate remote memory for remote string
		len = processName.length() + 1;
		pRemoteString = VirtualAllocEx(hProc, 0, len * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
		if (!pRemoteString) {
			throw std::exception("Could not allocate remote memory!");
		}

		// Write a remote string of the dll path
		if (!WriteProcessMemory(hProc, pRemoteString, (void*)processName.c_str(), len * sizeof(wchar_t), 0)) {
			throw std::exception("Could not write remote string!");
		}

		// Create remote thread of loadlibrary with path as paramater
		pLoadLibrary = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
		hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteString, 0, 0);
		if (!hThread) {
			throw std::exception("Could not create remote thread!");
		}

		// Wait for the thread return code (HMODULE of loaded module)
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, (DWORD*)&hModule);

		// Add the module to the process's module map
		if (hModule) {
			processes[processName].hProc = hProc;
			processes[processName].name = processName;
			processes[processName].modules[dllName] = hModule;
		}

		// Clean up
		VirtualFreeEx(hProc, pRemoteString, len * sizeof(wchar_t), MEM_FREE);
		CloseHandle(hProc);

		// Return true if module loaded succesfully, false otherwise
		return reinterpret_cast<int>(hModule);
	}
	catch (std::exception e) {
		// Get system error message

		VirtualFreeEx(hProc, pRemoteString, len * sizeof(wchar_t), MEM_FREE);
		CloseHandle(hProc);

		throw;
	}
}

// Unloads an injected (not arbitrary) dll from the target process
int CInjector::Unload(std::wstring dllName, std::wstring processName)
{
	HMODULE hModule = processes[processName].modules[dllName];

	// That dll hasnt been loaded, dont unload
	if (!hModule) {
		return 0;
	}

	// Unload the dll

	return 1;
}

// Returns a string list of all processes since the last refresh
CInjector::ProcessList_t CInjector::GetProcessList()
{
	RefreshProcessList();

	return processNames;
}

// Scans all processes in the system and stores them in a list (by name)
int CInjector::RefreshProcessList()
{
	return 1;
}

// Returns a handle based on the process name eg notepad.exe
HANDLE CInjector::GetProcessHandleByName(std::wstring processName)
{
	return 0;
}

// Returns a process id based on the process name eg notepad.exe
DWORD CInjector::GetProcessIdByName(std::wstring processName)
{
	return 0;
}

// Strips the leading path and returns only the filename
std::wstring CInjector::StripPath(std::wstring filePath)
{
	std::wstring::size_type pos = std::wstring::npos, last;
	
	do {
		last = pos;
		pos = filePath.find('\\', pos);
	} while (pos != std::wstring::npos);

	if (last != std::wstring::npos) {
		std::wstring result = filePath.substr(pos, filePath.length() - pos);
		return result;
	} else {
		return filePath;
	}
}