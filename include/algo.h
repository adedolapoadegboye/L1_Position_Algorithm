/**
 * @file algo.h
 * @brief Common includes and definitions for GNSS position solving algorithms.
 *
 * This header provides the standard system headers required across algorithm
 * components including math functions, memory handling, and system types.
 */

#ifndef ALGO
#define ALGO

#include <stdio.h>     /**< Standard I/O functions */
#include <stdlib.h>    /**< Memory allocation, process control, conversions */
#include <string.h>    /**< String manipulation functions */
#include <math.h>      /**< Math functions such as sqrt, sin, cos */
#include <stdbool.h>   /**< Boolean type support for C99 and above */
#include <time.h>      /**< Time and date functions */
#include <unistd.h>    /**< POSIX API (read, write, close, etc.) */
#include <sys/types.h> /**< System data types (pid_t, size_t, etc.) */
#include <errno.h>     /**< Error number definitions */

#endif /* ALGO */

/* ANSI color codes for styling */
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RED "\033[1;31m"

/* App Menu */
void app_menu(void);

/* App Cleanup */
void app_cleanup(void);

/* Option 1: Serial connection */
#ifdef _WIN32
#include <windows.h>
HANDLE serial_connect_windows(char *selected_port, size_t size);
#else
int serial_connect_mac(char *selected_port, size_t size);
#endif
void *serial_connect(char *selected_port, size_t size);

/* Option 2: File connection */
FILE *file_connect();
int file_input_mode(void);