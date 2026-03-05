#include <windows.h>
#include <stdio.h>

int main() {
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};
    char buffer[256];
    DWORD bytesRead;
    DWORD bytesWritten;

    printf("Now waiting...\n");

    hSerial = CreateFile(
        "\\\\.\\COM3",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening COM3\n");
        return 1;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    SetCommState(hSerial, &dcbSerialParams);

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    while (1) {

        if (ReadFile(hSerial, buffer, sizeof(buffer)-1, &bytesRead, NULL)) {

            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';

                printf("Received: %s", buffer);

                WriteFile(hSerial, "Pong\n", 5, &bytesWritten, NULL);
                printf("Sent: Pong\n");
            }

        } 
        else {
            printf("Read error\n");
            break;
        }
    }

    CloseHandle(hSerial);
    return 0;
}
