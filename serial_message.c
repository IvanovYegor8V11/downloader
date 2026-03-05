#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char* port_name = "\\\\.\\COM8";
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};

    hSerial = CreateFile(
        port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        0,
        0
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening port %s\n", port_name);
        return -1;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        printf("Error getting state of port\n");
        CloseHandle(hSerial);
        return -1;
    }

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printf("Error setting port parameters\n");
        CloseHandle(hSerial);
        return -1;
    }

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        printf("Error setting timeouts\n");
        CloseHandle(hSerial);
        return -1;
    }

    const char* message = "Hello from Windows laptop!\n";
    DWORD bytes_written;
    BOOL result = WriteFile(hSerial, message, strlen(message), &bytes_written, NULL);

    if (!result || bytes_written != strlen(message)) {
        printf("Error writing to port\n");
    } else {
        printf("Successfully sent message: %s", message);
    }

    CloseHandle(hSerial);

    return 0;
}
