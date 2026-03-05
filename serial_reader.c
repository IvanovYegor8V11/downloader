#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

// --- Конфигурация ---
#define SERIAL_PORT "/dev/ttyGS0" // Или "/dev/serial0", в зависимости от настроек
#define BAUD_RATE B115200         // Должна совпадать с настройками на ноутбуке
#define CMD_TO_RUN "echo 'Hello from triggered program!' > /tmp/trigger_output.txt"
#define MAX_CMD_LENGTH 256       // Максимальная длина команды system()

int set_interface_attribs(int fd, int speed, int parity) {
    struct termios tty;

    if(tcgetattr(fd, &tty) != 0) {
        printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_cflag &= ~PARENB;                         // No parity bit
    tty.c_cflag &= ~CSTOPB;                         // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                        // No flow control
    tty.c_cflag |= CREAD | CLOCAL;                  // Turn on READ & ignore ctrl lines

    tty.c_lflag = 0;                                // No signaling chars, no echo,
                                                    // no canonical processing
    tty.c_oflag = 0;                                // No remapping, no delays
    tty.c_cc[VMIN]  = 0;                            // Read doesn't block
    tty.c_cc[VTIME] = 5;                            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // Turn off s/w flow ctrl

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error %d from tcsetattr: %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int main() {
    int fd;
    char buf[MAX_CMD_LENGTH];
    const char* trigger_cmd = "RUN_APP\n"; // Команда для запуска

    fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if (fd < 0) {
        printf("Error %d opening %s: %s\n", errno, SERIAL_PORT, strerror(errno));
        return -1;
    }

    if (set_interface_attribs(fd, BAUD_RATE, 0) < 0) {
        close(fd);
        return -1;
    }

    printf("Listening on %s...\n", SERIAL_PORT);

    while(1) { // Бесконечный цикл
        int n = read(fd, buf, sizeof(buf)-1); // Читаем до MAX_CMD_LENGTH-1 символов
        if (n > 0) {
            buf[n] = '\0'; // Добавляем null-терминатор
            printf("Received: %s", buf); // Выводим полученное (для отладки)

            // Проверяем, совпадает ли полученная строка с командой
            if (strcmp(buf, trigger_cmd) == 0) {
                printf("Trigger command received. Executing...\n");
                int ret = system(CMD_TO_RUN); // Выполняем команду
                if (ret == -1) {
                    perror("system() failed");
                } else {
                    printf("Command executed with return code: %d\n", ret);
                }
            }
            // Можно добавить другие условия для других команд
            // else if (strcmp(buf, "OTHER_CMD\n") == 0) { ... }
        } else if (n < 0) {
            printf("Error %d reading: %s\n", errno, strerror(errno));
            break; // Выход по ошибке
        }
        // n == 0 означает таймаут, продолжаем ждать
    }

    close(fd);
    return 0;
}