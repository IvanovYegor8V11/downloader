#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

void print_hex(const unsigned char *buffer, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

int main() {
    int serial_port;
    struct termios tty;
    unsigned char buffer[256];

    serial_port = open("/dev/ttyGS0", O_RDWR | O_NOCTTY);

    if (serial_port < 0) {
        printf("Error opening serial port");
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
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_iflag = IGNPAR; 
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); 

    tty.c_oflag = 0;

    tty.c_lflag = 0; 

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    tcflush(serial_port, TCIFLUSH);
    
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error setting terminal attributes");
        return 1;
    }

    unsigned char ping_cmd[] = { 0xAA };
    write(serial_port, ping_cmd, 1);
    printf("Sent: 0xAA\n");

    int n = read(serial_port, buffer, sizeof(buffer));

    if (n > 0) {
        printf("Received [%d bytes]: ", n);
        print_hex(buffer, n);

        if (n >= 1 && buffer[0] == 0xBB) {
            printf("Success: Received Pong (0xBB)\n");
        }
    } else {
        printf("No data received or read error\n");
    }

    close(serial_port);
    return 0;
}