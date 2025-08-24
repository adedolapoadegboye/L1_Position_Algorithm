/**
 * @file serial_connect.c
 * @brief Enumerate and open a serial (COM/TTY) port on Windows or macOS/Linux.
 *
 * This module provides a small, interactive helper that lists available serial
 * ports and opens the one the user selects. On Windows it searches COM ports
 * via `QueryDosDeviceA()`. On macOS/Linux it scans `/dev` for `tty.usb*` and
 * `ttyUSB*` device nodes and configures the port for 8-N-1 at 9600 baud.
 *
 * @note Logic is intentionally minimal and interactive; error handling favors
 *       clear console messages via `printf()`/`perror()`.
 *
 * @todo Make baud rate and serial settings configurable (currently hard-coded
 *       to 9600 8-N-1).
 * @todo Add non-blocking / timeout configuration knobs.
 * @todo Optionally accept a preselected port (skip interactive prompt).
 */

#include "../include/algo.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h> /* intptr_t for void*<->fd cast on POSIX */

#ifdef _WIN32
#include <windows.h>

/**
 * @brief List and connect to available COM ports on Windows.
 *
 * Enumerates COM1..COM256 using QueryDosDeviceA, prompts the user to choose a
 * port, and attempts to open it with CreateFileA.
 *
 * @param[out] selected_port  Buffer to receive the selected port name (e.g. "\\\\.\\COM3").
 * @param[in]  size           Size of @p selected_port buffer in bytes.
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
            if (count >= (int)(sizeof(ports) / sizeof(ports[0])))
                break;
            snprintf(ports[count], sizeof(ports[0]), "%s", name);
            printf("  [%d] %s\n", ++count, name);
        }
    }

    if (count == 0)
    {
        printf("No serial ports found on Windows.\n");
        return INVALID_HANDLE_VALUE;
    }

    int choice;
    char input[16];
    printf("Select a port by number (1-%d): ", count);
    if (!fgets(input, sizeof(input), stdin) ||
        sscanf(input, "%d", &choice) != 1 || choice < 1 || choice > count)
    {
        printf("Invalid selection.\n");
        return INVALID_HANDLE_VALUE;
    }

    /* Prefix with \\.\ to allow COM ports > 9 */
    snprintf(selected_port, size, "\\\\.\\%s", ports[choice - 1]);

    HANDLE hSerial = CreateFileA(selected_port,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

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

#else /* POSIX: macOS / Linux */

#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

/**
 * @brief List and connect to USB serial ports on macOS/Linux.
 *
 * Scans `/dev` for `tty.usb*` and `ttyUSB*` device nodes, prompts for a choice,
 * opens the device, and applies 9600 8-N-1 with basic raw settings.
 *
 * @param[out] selected_port  Buffer to receive the selected device path.
 * @param[in]  size           Size of @p selected_port buffer in bytes.
 * @return File descriptor (>=0) on success, or -1 on failure.
 */
int serial_connect_mac(char *selected_port, size_t size)
{
    char ports[64][256];
    size_t count = 0;

    DIR *dp = opendir("/dev");
    if (!dp)
    {
        perror("opendir /dev");
        return -1;
    }

    printf("Available serial ports:\n");

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL)
    {
        if (fnmatch("tty.usb*", entry->d_name, 0) == 0 ||
            fnmatch("ttyUSB*", entry->d_name, 0) == 0)
        {
            if (count >= (sizeof(ports) / sizeof(ports[0])))
                break;
            snprintf(ports[count], sizeof(ports[0]), "/dev/%s", entry->d_name);
            printf("  [%zu] /dev/%s\n", count + 1, entry->d_name);
            count++;
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
    if (!fgets(input, sizeof(input), stdin) ||
        sscanf(input, "%d", &choice) != 1 || choice < 1 || choice > (int)count)
    {
        printf("Invalid selection.\n");
        return -1;
    }

    snprintf(selected_port, size, "%s", ports[choice - 1]);

    int fd = open(selected_port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1)
    {
        perror("open serial port");
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    /* 9600 baud */
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    /* 8-N-1, no flow control */
    tty.c_cflag &= (tcflag_t)(~CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cflag &= (tcflag_t)(~PARENB);
    tty.c_cflag &= (tcflag_t)(~CSTOPB);
    tty.c_cflag &= (tcflag_t)(~CRTSCTS);
    tty.c_cflag |= (CLOCAL | CREAD);

    /* Raw-ish mode */
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;

    /* Read returns as soon as 1 byte is available, with ~0.1s interbyte timeout */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("tcsetattr");
        close(fd);
        return -1;
    }

    printf("Connected to %s\n", selected_port);
    return fd;
}
#endif /* _WIN32 */

/**
 * @brief Cross-platform serial connect wrapper.
 *
 * Dispatches to the platform-specific implementation and returns an opaque
 * handle the caller can store. On Windows this is a `HANDLE`. On POSIX this is
 * a file descriptor cast to `void*` via `intptr_t`.
 *
 * @param[out] selected_port  Buffer to receive the chosen port path/name.
 * @param[in]  size           Size of @p selected_port buffer in bytes.
 * @return Opaque handle to the opened port, or NULL on failure.
 */
void *serial_connect(char *selected_port, size_t size)
{
#ifdef _WIN32
    return (void *)serial_connect_windows(selected_port, size);
#else
    int fd = serial_connect_mac(selected_port, size);
    return (fd >= 0) ? (void *)(intptr_t)fd : NULL;
#endif
}
