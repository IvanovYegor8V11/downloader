#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <string.h>

#define PACKET_SIZE 6
#define HEADER_SIZE 2

int read_exact(int fd, uint8_t *buffer, int count) {
    int total_read = 0;
    int n = 0;
    while (total_read < count) {
        n = read(fd, buffer + total_read, count - total_read);
        if (n < 0) return -1;
        if (n == 0) break;
        total_read += n;
    }
    return total_read;
}

void print_hex(const uint8_t *buffer, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

int main() {
    int serial_port;
    struct termios tty;
    uint8_t rxBuffer[PACKET_SIZE];
    uint8_t txBuffer[PACKET_SIZE];

    serial_port = open("/dev/ttyGS0", O_RDWR | O_NOCTTY);
    if (serial_port < 0) {
        printf("Error opening serial port: %d\n", serial_port);
        return 1;
    }

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error from tcgetattr");
        return 1;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CREAD | CLOCAL);
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    
    tty.c_iflag = IGNPAR;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 50; 

    tcflush(serial_port, TCIFLUSH);
    tcsetattr(serial_port, TCSANOW, &tty);

    printf("Listening for packets...\n");

    int n = read_exact(serial_port, rxBuffer, PACKET_SIZE);

    if (n == PACKET_SIZE) {
        printf("Received Packet [%d bytes]: ", n);
        print_hex(rxBuffer, n);

        if (rxBuffer[0] == 0xAA && rxBuffer[1] == 0xBB) {
            printf("Header Valid. Processing data...\n");
            
            txBuffer[0] = 0xCC;
            txBuffer[1] = 0xDD;
            
            txBuffer[2] = rxBuffer[2];
            txBuffer[3] = rxBuffer[3];
            txBuffer[4] = rxBuffer[4];
            txBuffer[5] = rxBuffer[5] + 1; 

            write(serial_port, txBuffer, PACKET_SIZE);
            printf("Sent Response: ");
            print_hex(txBuffer, PACKET_SIZE);
        } 
        else {
            printf("Invalid Header. Ignoring.\n");
        }
    } 
    else {
        printf("Read incomplete. Got %d bytes instead of %d\n", n, PACKET_SIZE);
    }

    close(serial_port);
    return 0;
}