#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main() {
    int serial_port;
    struct termios tty;
    char buffer[256];

    serial_port = open("/dev/ttyGS0", O_RDWR);

    if (serial_port < 0) {
        perror("Error opening serial port");
        return 1;
    }

    tcgetattr(serial_port, &tty);

    tty.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tcflush(serial_port, TCIFLUSH);
    tcsetattr(serial_port, TCSANOW, &tty);

    write(serial_port, "Ping\n", 5);
    printf("Sent: Ping\n");

    int n = read(serial_port, buffer, sizeof(buffer) - 1);

    if (n > 0) {
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(serial_port);
    return 0;
}
