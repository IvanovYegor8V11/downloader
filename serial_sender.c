#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define PACKET_SIZE 6
#define HEADER_SIZE 2
#define DATA_SIZE 4

BOOL ReadExact(HANDLE hSerial, uint8_t* buffer, DWORD count, DWORD* bytesReadTotal) {
    DWORD bytesRead = 0;
    *bytesReadTotal = 0;
    
    while (*bytesReadTotal < count) {
        if (!ReadFile(hSerial, buffer + *bytesReadTotal, count - *bytesReadTotal, &bytesRead, NULL)) {
            return FALSE;
        }
        if (bytesRead == 0) {
            break; 
        }
        *bytesReadTotal += bytesRead;
    }
    return (*bytesReadTotal == count);
}

void print_hex(const uint8_t *buffer, DWORD length) {
    for (DWORD i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

int main() {
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    uint8_t txBuffer[PACKET_SIZE];
    uint8_t rxBuffer[PACKET_SIZE];
    DWORD bytesReadTotal;
    DWORD bytesWritten;

    printf("Opening port COM7...\n");
    hSerial = CreateFile("\\\\.\\COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening COM7\n");
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

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    Sleep(500);

    txBuffer[0] = 0xAA; 
    txBuffer[1] = 0xBB;
    
    txBuffer[2] = 0x01;
    txBuffer[3] = 0x02;
    txBuffer[4] = 0x03;
    txBuffer[5] = 0x04;

    printf("Sending Packet: ");
    print_hex(txBuffer, PACKET_SIZE);

    if (WriteFile(hSerial, txBuffer, PACKET_SIZE, &bytesWritten, NULL)) {
        printf("Sent %ld bytes\n", bytesWritten);
    } 
    else {
        printf("Write Error\n");
        CloseHandle(hSerial);
        return 1;
    }

    printf("Waiting for response...\n");
    if (ReadExact(hSerial, rxBuffer, PACKET_SIZE, &bytesReadTotal)) {
        printf("Received Packet [%ld bytes]: ", bytesReadTotal);
        print_hex(rxBuffer, bytesReadTotal);

        if (rxBuffer[0] == 0xCC && rxBuffer[1] == 0xDD) {
            printf("Success! Header matches (0xCC 0xDD)\n");
            printf("Data payload: %02X %02X %02X %02X\n", rxBuffer[2], rxBuffer[3], rxBuffer[4], rxBuffer[5]);
        } 
        else {
            printf("Error: Unknown header received\n");
        }
    } 
    else {
        printf("Read Error or Timeout. Received only %ld bytes. (%02X %02X)\n", bytesReadTotal, rxBuffer[0], rxBuffer[1]);
    }

    CloseHandle(hSerial);
    return 0;
}