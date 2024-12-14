#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

#define PIPE_NAME L"\\\\.\\pipe\\LogPipe"
#define BUFFER_SIZE 512

void LogMessage(const std::string& message) {
    std::ofstream logfile("log.txt", std::ios::app);
    if (logfile.is_open()) {
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now);

        // Запись сообщения в файл
        logfile << "[" << dt << "] " << message << std::endl;
        logfile.close();
    }
    else {
        std::cerr << "Ошибка открытия файла log.txt" << std::endl;
    }
}

int main() {
    setlocale(0, "rus");
    HANDLE hPipe;
    char buffer[BUFFER_SIZE];
    DWORD bytesRead;

    std::cout << "Сервер логирования запущен и ожидает подключения клиентов..." << std::endl;

    hPipe = CreateNamedPipe(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::cerr << "Ошибка создания канала. Код ошибки: " << error << std::endl;
        return 1;
    }

    while (true) {
        // Ожидаем подключения клиента
        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            std::cout << "Клиент подключен." << std::endl;

            // Читаем сообщение от клиента
            if (ReadFile(hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0'; // Завершаем строку
                std::string message(buffer);
                LogMessage(message); // Логируем сообщение
                std::cout << "Сообщение записано в лог: " << message << std::endl;
            }
            else {
                std::cerr << "Ошибка при чтении из канала." << std::endl;
            }

            DisconnectNamedPipe(hPipe); // Отключаем клиента
        }
    }

    CloseHandle(hPipe);
    return 0;
}
