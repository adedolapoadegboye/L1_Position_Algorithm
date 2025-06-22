/**
 * @file serial_connect.c
 * @brief Lists available USB serial connections and connects to selected port using the defined port properties.
 */

#include "../include/algo.h"

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>

/**
 * @brief List and connect to available COM ports on Windows.
 * @param selected_port Buffer to store the selected port name (e.g., "COM3").
 * @param size Size of the selected_port buffer.
 * @return HANDLE to the opened serial port, or INVALID_HANDLE_VALUE on failure.
 */
HANDLE serial_connect_windows(char *selected_port, size_t size)
{
    char ports[256][16];
    int count = 0;

    printf("Available serial ports:\n");

    for (int i = 1; i <= 256; i++)
    {
        char name[16];
        char target[256];
        DWORD size_check;
        snprintf(name, sizeof(name), "COM%d", i);
        size_check = QueryDosDeviceA(name, target, sizeof(target));
        if (size_check > 0)
        {
            if (count >= sizeof(ports) / sizeof(ports[0]))
                break;
            snprintf(ports[count++], sizeof(ports[0]), "%s", name);
            printf("  [%d] %s\n", count, name);
        }
    }

    if (count == 0)
    {
        printf("No serial ports found on windows.\n");
        return INVALID_HANDLE_VALUE;
    }

    int choice;
    char input[16];
    printf("Select a port by number (1-%d): ", count);
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%d", &choice) != 1 || choice < 1 || choice > count)
    {
        printf("Invalid selection.\n");
        return INVALID_HANDLE_VALUE;
    }

    snprintf(selected_port, size, "\\\\.\\%s", ports[choice - 1]);

    HANDLE hSerial = CreateFileA(
        selected_port, GENERIC_READ | GENERIC_WRITE, 0,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        printf("Failed to open serial port (Error %lu)\n", err);
    }
    else
    {
        printf("Connected to %s\n", selected_port);
    }

    return hSerial;
}

#else
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief List and connect to USB serial ports on macOS/Linux.
 * @param selected_port Buffer to store the selected port path.
 * @param size Size of the selected_port buffer.
 * @return File descriptor to the opened serial port, or -1 on failure.
 */
int serial_connect_mac(char *selected_port, size_t size)
{
    char ports[64][256];
    size_t count = 0;

    DIR *dp = opendir("/dev");
    struct dirent *entry;

    if (!dp)
    {
        perror("opendir /dev");
        return -1;
    }

    printf("Available serial ports:\n");

    while ((entry = readdir(dp)) != NULL)
    {
        if (fnmatch("tty.usb*", entry->d_name, 0) == 0 ||
            fnmatch("ttyUSB*", entry->d_name, 0) == 0)
        {
            if (count >= sizeof(ports) / sizeof(ports[0]))
                break;
            snprintf(ports[count++], sizeof(ports[0]), "/dev/%s", entry->d_name);
            printf("  [%zu] /dev/%s\n", count, entry->d_name);
        }
    }

    closedir(dp);

    if (count == 0)
    {
        printf("No serial ports found on mac.\n");
        return -1;
    }

    int choice;
    char input[16];
    printf("Select a port by number (1-%zu): ", count);
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%d", &choice) != 1 || choice < 1 || choice > (int)count)
    {
        printf("Invalid selection.\n");
        return -1;
    }

    snprintf(selected_port, size, "%s", ports[choice - 1]);

    int fd = open(selected_port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1)
    {
        perror("Failed to open serial port");
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("tcgetattr failed");
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag &= (tcflag_t)(~CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cflag &= (tcflag_t)(~PARENB);
    tty.c_cflag &= (tcflag_t)(~CSTOPB);
    tty.c_cflag &= (tcflag_t)(~CRTSCTS);

    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("tcsetattr failed");
        close(fd);
        return -1;
    }

    printf("Connected to %s\n", selected_port);
    return fd;
}
#endif

/**
 * @brief Connect to a serial port based on the operating system.
 * @param selected_port Buffer to store the selected port name or path.
 * @param size Size of the selected_port buffer.
 * @return Handle or file descriptor for the opened serial port, or NULL on failure.
 */
void *serial_connect(char *selected_port, size_t size)
{
#ifdef _WIN32
    return (void *)serial_connect_windows(selected_port, size);
#else
    int fd = serial_connect_mac(selected_port, size);
    return (fd != -1) ? (void *)(intptr_t)fd : NULL;
#endif
}
