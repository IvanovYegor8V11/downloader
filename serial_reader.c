#include <windows.h>
#include <stdio.h>

void print_hex(const unsigned char *buffer, DWORD length) {
    for (DWORD i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

int main() {
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    unsigned char buffer[256];
    DWORD bytesRead;
    DWORD bytesWritten;

    printf("Opening port COM8...\n");

    hSerial = CreateFile(
        "\\\\.\\COM8",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening COM8\n");
        return 1;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fInX = FALSE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;

    SetCommState(hSerial, &dcbSerialParams);

    Sleep(1000);

    printf("Listening for binary data...\n");

    while (1) {
        if (ReadFile(hSerial, buffer, sizeof(buffer), &bytesRead, NULL)) {
            if (bytesRead > 0) {
                printf("Received [%ld bytes]: ", bytesRead);
                print_hex(buffer, bytesRead);

                if (bytesRead >= 1 && buffer[0] == 0xAA) {
                    unsigned char response[] = { 0xBB };
                    
                    WriteFile(hSerial, response, 1, &bytesWritten, NULL);
                    printf("Sent: 0xBB\n");
                }
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