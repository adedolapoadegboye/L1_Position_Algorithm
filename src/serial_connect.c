/**
 * @file serial_connect.c
 * @brief Lists available USB serial connections and connects to selected port.
 */

#include "../include/algo.h"

#ifdef _WIN32
#include <windows.h>

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
            snprintf(ports[count++], sizeof(ports[0]), "%s", name);
            printf("  [%d] %s\n", count, name);
        }
    }

    if (count == 0)
    {
        printf("No serial ports found.\n");
        return INVALID_HANDLE_VALUE;
    }

    int choice;
    printf("Select a port by number (1-%d): ", count);
    scanf("%d", &choice);

    if (choice < 1 || choice > count)
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
        perror("Failed to open serial port");
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

/**
 * @brief List and connect to USB serial ports on macOS/Linux.
 * @param selected_port Buffer to store the selected port path.
 * @param size Size of the selected_port buffer.
 * @return File descriptor to the opened serial port, or -1 on failure.
 */
int serial_connect_mac(char *selected_port, size_t size)
{
    char ports[64][256];
    int count = 0;

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
            snprintf(ports[count++], sizeof(ports[0]), "/dev/%s", entry->d_name);
            printf("  [%d] /dev/%s\n", count, entry->d_name);
        }
    }

    closedir(dp);

    if (count == 0)
    {
        printf("No serial ports found.\n");
        return -1;
    }

    int choice;
    printf("Select a port by number (1-%d): ", count);
    scanf("%d", &choice);

    if (choice < 1 || choice > count)
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

    // Optional: Set serial config (baudrate, etc.)
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("tcgetattr failed");
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag |= (CLOCAL | CREAD);
    tcsetattr(fd, TCSANOW, &tty);

    printf("Connected to %s\n", selected_port);
    return fd;
}
#endif
