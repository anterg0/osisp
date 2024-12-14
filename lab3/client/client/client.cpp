#include <windows.h>
#include <iostream>
#include <string>

#define PIPE_NAME L"\\\\.\\pipe\\LogPipe"
#define BUFFER_SIZE 512

int main() {
    setlocale(0, "rus");
    HANDLE hPipe;
    DWORD bytesWritten;
    std::string message;

    std::cout << "Введите сообщение для логирования: ";
    std::getline(std::cin, message);

    hPipe = CreateFile(
        PIPE_NAME,          // Имя пайпа
        GENERIC_WRITE,      // Доступ на запись
        0,                  // Без совместного доступа
        NULL,               // Стандартный уровень защиты
        OPEN_EXISTING,      // Подключение к существующему каналу
        0,                  // Дополнительные атрибуты
        NULL                // Без шаблона файла
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_BUSY) {
            std::cerr << "Сервер занят. Попробуйте позже." << std::endl;
        }
        else {
            std::cerr << "Не удалось подключиться к серверу. Код ошибки: " << error << std::endl;
        }
        return 1;
    }

    // Отправка сообщения серверу
    if (WriteFile(hPipe, message.c_str(), message.length(), &bytesWritten, NULL)) {
        std::cout << "Сообщение успешно отправлено серверу." << std::endl;
    }
    else {
        std::cerr << "Ошибка при отправке сообщения." << std::endl;
    }

    // Закрытие канала
    CloseHandle(hPipe);

    return 0;
}
