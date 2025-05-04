#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

BOOL snapProc(IN LPCWSTR nProc, OUT HANDLE* hHandle, OUT DWORD* idProc) {

	BOOL found = FALSE;

	PROCESSENTRY32 proc;
	proc.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (!hSnapshot) {
		std::cout << "[!] Error CreateSnapShot: " << GetLastError() << "\n";
		goto _endfunc;
	}

	if (hSnapshot != INVALID_HANDLE_VALUE) {
		Process32First(hSnapshot, &proc);
		do {

			if (lstrcmpi(proc.szExeFile, nProc) == 0) {

				*idProc = proc.th32ProcessID;
				std::cout << "[+] Id Process: " << *idProc << "\n";
				*hHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc.th32ProcessID);
				if (*hHandle == NULL) {
					std::cout << "[!] Dont Open Process: " << GetLastError << "\n";
				}
				else {
					found = true;
				}
				break;
			}

		} while (Process32Next(hSnapshot, &proc));
	}

_endfunc:
	if (hSnapshot != NULL) {
		CloseHandle(hSnapshot);
	}
	if (*hHandle == NULL || *idProc == NULL) {
		return FALSE;
	}
	return found;
}

BOOL InjectDll(IN HANDLE h, IN LPCWSTR buffer) {
	BOOL state = TRUE; // return state
	LPVOID pAddress = NULL; // addr
	LPVOID pLoadLibraryW = NULL; // addrloadlib
	DWORD sizeWr = lstrlenW(buffer) * sizeof(WCHAR); // for secure, double buf
	SIZE_T NbyWr = NULL; // writedbuf
	HANDLE hThread = NULL; // thread on process

	pLoadLibraryW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
	if (pLoadLibraryW == NULL) {
		std::cout << "[~!~] Error get func loadlibraryW: " << GetLastError();
		state = FALSE;
		goto _funcEnd;
	}

	pAddress = VirtualAllocEx(h, NULL, sizeWr, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pAddress == NULL) {
		std::cout << "[~!~] Error Allocated Address: " << GetLastError();
		state = FALSE;
		goto _funcEnd;
	}
	else {
		std::cout << "[+] Allocated Address: " << pAddress << "\n";
	}

	std::cout << "[++] press <enter>... \n";
	getchar();


	if (!WriteProcessMemory(h, pAddress, buffer, sizeWr, &NbyWr) || NbyWr != sizeWr) {
		std::cout << "[~!~] Error Write in Memory Address: " << GetLastError();
		state = FALSE;
		goto _funcEnd;
	}
	else {
		std::cout << "[+] Writed Memory buffer: " << NbyWr << "\n";
	}

	std::cout << "[++] press <enter> ... \n";
	getchar();
	std::cout << "[++] executing payload ... \n";

	// Create REMOTE THREAD
	// casting pLoadLibraryW to LPTHREAD_START_ROUTINE
	hThread = CreateRemoteThread(h, NULL, NULL, (LPTHREAD_START_ROUTINE)pLoadLibraryW, pAddress, NULL, NULL);
	if (hThread == NULL) {
		std::cout << "[~!~] Error Create Remote Thread on process: " << GetLastError();
		state = FALSE;
		goto _funcEnd;
	}

	std::cout << "[++] done... \n";

_funcEnd:
	if (hThread) {
		CloseHandle(hThread);
	}
	return state;
}

int main() {
	HANDLE h;
	DWORD procid = NULL;
	

	
	std::wstring nameP;
	std::wcout << L"Name of process: ";
	std::wcin >> nameP;
	LPCWSTR nameProc = nameP.c_str();

	std::wstring dllPath;
	std::wcout << L"Path of DLL: ";
	std::wcin >> dllPath;
	LPCWSTR nameDllPath = dllPath.c_str();


	std::wcout << "[+] SnapProcess... \n";
	snapProc(nameProc, &h, &procid);


	if (procid != NULL) {
		std::wcout << "[+] Process id: " << procid << "\n";
	}
	else {
		std::wcout << "[!] Error get id: " << GetLastError() << "\n";
	}

	InjectDll(h, nameDllPath);

}
