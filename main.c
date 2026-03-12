#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <string.h>

#include "DEV_Config.h"
#include "LCD_1in47.h"
#include "GUI_BMP.h"

#define PACKET_SIZE 6
#define HEADER_SIZE 2
#define DATA_SIZE 4
#define CMD_OPEN_IMAGE_1  0x4F50494E
#define CMD_OPEN_IMAGE_2  0x4F50494D

volatile int running = 1;
int serial_port = -1;
UWORD *BlackImage = NULL;

void Handler_1IN47_LCD(int sig) {
    running = 0;
    printf("\nInterrupt signal received: %d\n", sig);
}

int read_exact(int fd, uint8_t *buffer, int count) {
    int total_read = 0;
    int n = 0;
    while (total_read < count && running) {
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

void display_image(const char *bmp_path) {
    printf("Displaying image: %s\n", bmp_path);
    
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    
    if (GUI_ReadBmp(bmp_path) == 0) {
        LCD_1IN47_Display(BlackImage);
        printf("Image displayed successfully\n");
    } else {
        printf("Failed to load image: %s\n", bmp_path);
    }
}

int main(int argc, char *argv[]) {
    uint8_t rxBuffer[PACKET_SIZE];
    
    signal(SIGINT, Handler_1IN47_LCD);
    signal(SIGTERM, Handler_1IN47_LCD);

    printf("Initializing LCD...\n");
    if (DEV_ModuleInit() != 0) {
        printf("Failed to initialize LCD module\n");
        DEV_ModuleExit();
        return 1;
    }

    LCD_1IN47_Init(HORIZONTAL);
    LCD_1IN47_Clear(BLACK);
    LCD_SetBacklight(1023);

    UDOUBLE Imagesize = LCD_1IN47_HEIGHT * LCD_1IN47_WIDTH * 2;
    if ((BlackImage = (UWORD*)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\n");
        DEV_ModuleExit();
        return 1;
    }

    Paint_NewImage(BlackImage, LCD_1IN47_WIDTH, LCD_1IN47_HEIGHT, 90, BLACK, 16);

    printf("Opening serial port /dev/ttyGS0...\n");
    serial_port = open("/dev/ttyGS0", O_RDWR | O_NOCTTY);
    if (serial_port < 0) {
        printf("Error opening serial port: %d\n", serial_port);
        free(BlackImage);
        DEV_ModuleExit();
        return 1;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error from tcgetattr\n");
        close(serial_port);
        free(BlackImage);
        DEV_ModuleExit();
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

    printf("System ready. Waiting for commands...\n");
    printf("Send: 4F 50 XX XX XX XX (where XX is image ID)\n\n");

    while (running) {
        int n = read_exact(serial_port, rxBuffer, PACKET_SIZE);

        if (n == PACKET_SIZE) {
            printf("Received [%d bytes]: ", n);
            print_hex(rxBuffer, n);

            if (rxBuffer[0] == 0x4F && rxBuffer[1] == 0x50) {
                uint32_t image_id = 0;
                memcpy(&image_id, &rxBuffer[2], 4);
                
                printf("Command: OPEN_IMAGE, ID: 0x%08X\n", image_id);

                if (image_id == CMD_OPEN_IMAGE_1) {
                    display_image("./pic/image1.bmp");
                } else if (image_id == CMD_OPEN_IMAGE_2) {
                    display_image("./pic/image2.bmp");
                } else {
                    printf("Unknown image ID: 0x%08X\n", image_id);
                }
            } else {
                printf("Unknown command header\n");
            }
        } else if (n > 0) {
            printf("Incomplete packet received: %d bytes\n", n);
        }
    }

    printf("Shutting down...\n");
    
    if (BlackImage != NULL) {
        free(BlackImage);
        BlackImage = NULL;
    }
    
    if (serial_port >= 0) {
        close(serial_port);
    }
    
    DEV_ModuleExit();
    printf("Exit successfully\n");
    
    return 0;
}