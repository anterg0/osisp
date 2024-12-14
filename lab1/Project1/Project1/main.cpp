#include <windows.h>
#include <vector>
#include <string>
#include <commdlg.h>
#include <sstream>

struct ProcessInfo {
    DWORD processID;
    HANDLE processHandle;
    std::wstring name;
    std::wstring status;
};

std::vector<ProcessInfo> processes;
HWND hwndComboBox;
HWND hwndProcessInfoTextBox;

void LaunchProcess(const std::wstring& exePath);
void UpdateProcessInfoInTextBox();
void CheckProcesses();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            OPENFILENAME ofn;
            wchar_t szFile[260];
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"Executable Files\0*.exe\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn)) {
                LaunchProcess(ofn.lpstrFile);
            }
        }
        else if (LOWORD(wParam) == 2) {
            int selected = SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0); 
            if (selected != CB_ERR) {
                wchar_t processInfo[256];
                SendMessage(hwndComboBox, CB_GETLBTEXT, selected, (LPARAM)processInfo);

                std::wstring processString = processInfo;
                size_t start = processString.find(L"(") + 1;
                size_t end = processString.find(L")");
                std::wstring processIDStr = processString.substr(start, end - start);
                DWORD processID = std::stoul(processIDStr);

                auto it = std::find_if(processes.begin(), processes.end(), [&](const ProcessInfo& proc) {
                    return proc.processID == processID;
                    });

                if (it != processes.end()) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, it->processID);
                    if (hProcess) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);

                        it->status = L"Завершён";
                        UpdateProcessInfoInTextBox();

                        SendMessage(hwndComboBox, CB_DELETESTRING, selected, 0);
                    }
                }
            }
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void LaunchProcess(const std::wstring& exePath) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcess(exePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) 
    {
        const wchar_t* fileName = wcsrchr(exePath.c_str(), L'\\');
        fileName = (fileName) ? fileName + 1 : exePath.c_str();

        ProcessInfo procInfo = { pi.dwProcessId, pi.hProcess, fileName, L"Запущен" };
        processes.push_back(procInfo);
        CloseHandle(pi.hThread);

        std::wstring processDropdownItem = procInfo.name + L" (" + std::to_wstring(procInfo.processID) + L")";
        SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)(processDropdownItem.c_str()));

        UpdateProcessInfoInTextBox();
    }
    else {
        MessageBox(NULL, L"Не удалось запустить процесс", L"Ошибка", MB_OK);
    }
}

void UpdateProcessInfoInTextBox() {
    std::wstring infoText;
    for (const auto& proc : processes) {
        infoText += proc.name + L" (ID: " + std::to_wstring(proc.processID) + L") - " + proc.status + L"\r\n";
    }
    SetWindowText(hwndProcessInfoTextBox, infoText.c_str());
}

void CheckProcesses() {
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        DWORD exitCode;
        if (GetExitCodeProcess(it->processHandle, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                CloseHandle(it->processHandle);

                it->status = L"Завершён";
                UpdateProcessInfoInTextBox();

                int index = SendMessage(hwndComboBox, CB_FINDSTRINGEXACT, -1, (LPARAM)(it->name.c_str()));
                if (index != CB_ERR) {
                    SendMessage(hwndComboBox, CB_DELETESTRING, index, 0);
                }
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    const wchar_t CLASS_NAME[] = L"ProcessManager";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Диспетчер задач", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, NULL, NULL, hInstance, NULL);

    hwndProcessInfoTextBox = CreateWindow(L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
        10, 10, 300, 300, hwnd, NULL, hInstance, NULL);

    hwndComboBox = CreateWindow(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        320, 10, 150, 300, hwnd, NULL, hInstance, NULL);

    CreateWindow(L"BUTTON", L"Запустить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 50, 150, 30, hwnd, (HMENU)1, hInstance, NULL);

    CreateWindow(L"BUTTON", L"Закрыть", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        320, 90, 150, 30, hwnd, (HMENU)2, hInstance, NULL);

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);


    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        CheckProcesses();
    }
    return 0;
}
