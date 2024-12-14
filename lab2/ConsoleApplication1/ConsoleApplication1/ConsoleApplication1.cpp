#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#define BUFFER_SIZE 4096


void ProcessData(char* buffer, DWORD bytesRead) {
    std::sort(buffer, buffer + bytesRead);
}

void AsyncFileProcessing(LPCWSTR inputFilePath, LPCWSTR outputFilePath) {

    HANDLE hInputFile = CreateFile(inputFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (hInputFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening input file: " << GetLastError() << std::endl;
        return;
    }


    HANDLE hOutputFile = CreateFile(outputFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    if (hOutputFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error creating output file: " << GetLastError() << std::endl;
        CloseHandle(hInputFile);
        return;
    }

    OVERLAPPED readOverlapped = { 0 };
    OVERLAPPED writeOverlapped = { 0 };

   
    readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    writeOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (readOverlapped.hEvent == NULL || writeOverlapped.hEvent == NULL) {
        std::cerr << "Error creating event: " << GetLastError() << std::endl;
        CloseHandle(hInputFile);
        CloseHandle(hOutputFile);
        return;
    }

    DWORD bytesRead = 0, bytesWritten = 0;
    char buffer[BUFFER_SIZE];
    DWORD totalBytesRead = 0;

    auto start = std::chrono::high_resolution_clock::now();

   
    while (true) {
        
        readOverlapped.Offset = totalBytesRead;
        readOverlapped.OffsetHigh = 0;

        BOOL readResult = ReadFile(hInputFile, buffer, BUFFER_SIZE, &bytesRead, &readOverlapped);
        if (!readResult) {
            if (GetLastError() == ERROR_IO_PENDING) {
                std::cout << "Waiting for read operation to complete...\n";
                WaitForSingleObject(readOverlapped.hEvent, INFINITE);
                GetOverlappedResult(hInputFile, &readOverlapped, &bytesRead, FALSE);
            }
            else {
                std::cerr << "Read error: " << GetLastError() << std::endl;
                break;  
            }
        }

        if (bytesRead == 0) break;  


        totalBytesRead += bytesRead;


        ProcessData(buffer, bytesRead);
        std::cout << "Read and processed " << bytesRead << " bytes.\n";


        writeOverlapped.Offset = totalBytesRead - bytesRead; 
        writeOverlapped.OffsetHigh = 0;

        BOOL writeResult = WriteFile(hOutputFile, buffer, bytesRead, &bytesWritten, &writeOverlapped);
        if (!writeResult) {
            if (GetLastError() == ERROR_IO_PENDING) {
                std::cout << "Waiting for write operation to complete...\n";
                WaitForSingleObject(writeOverlapped.hEvent, INFINITE);
                GetOverlappedResult(hOutputFile, &writeOverlapped, &bytesWritten, FALSE);
            }
            else {
                std::cerr << "Write error: " << GetLastError() << std::endl;
                break;
            }
        }
    }


    if (bytesWritten > 0) {
        WaitForSingleObject(writeOverlapped.hEvent, INFINITE);
        GetOverlappedResult(hOutputFile, &writeOverlapped, &bytesWritten, FALSE);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds\n";


    CloseHandle(readOverlapped.hEvent);
    CloseHandle(writeOverlapped.hEvent);
    CloseHandle(hInputFile);
    CloseHandle(hOutputFile);
}

int main() {
    LPCWSTR inputFilePath = L"C:/Users/Alex1/source/repos/ConsoleApplication2/ConsoleApplication2/large_text_file.txt";
    LPCWSTR outputFilePath = L"C:/Users/Alex1/source/repos/ConsoleApplication2/ConsoleApplication2/processed_text_file.txt";
    AsyncFileProcessing(inputFilePath, outputFilePath);
    return 0;
}