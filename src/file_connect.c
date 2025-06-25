/**
 * @file file_connect.c
 * @brief Provides utilities for file handling in GNSS processing applications.
 */

#include "../include/algo.h"

/// Default file path used when user provides no input
#define DEFAULT_FILE_PATH "../example/log.txt"

/**
 * @brief Opens a pre-recorded RTCM binary file for reading.
 *
 * This function prompts the user to enter the file path of a pre-recorded RTCM data file.
 * If the user provides no input, it defaults to the path defined by @c DEFAULT_FILE_PATH.
 * The file is opened in binary read mode ("rb") since RTCM files typically contain binary data.
 *
 * @return FILE* Pointer to the opened file on success, or NULL if the file could not be opened.
 *
 * @note This function prints informative messages to stdout/stderr, using colored text if supported.
 */
FILE *file_connect()
{
    char file_path[256];
    FILE *fp;

    // Prompt user for file path
    printf(COLOR_BLUE "Enter the path to the pre-recorded RTCM file (default: %s): " COLOR_RESET, DEFAULT_FILE_PATH);
    fgets(file_path, sizeof(file_path), stdin);

    // Remove newline character if present
    size_t len = strlen(file_path);
    if (len > 0 && file_path[len - 1] == '\n')
    {
        file_path[len - 1] = '\0';
    }

    // Use default if input is empty
    if (strlen(file_path) == 0)
    {
        snprintf(file_path, sizeof(file_path), "%s", DEFAULT_FILE_PATH);
    }

    // Open the file in binary mode
    fp = fopen(file_path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, COLOR_RED "Error: Could not open file '%s': %s\n" COLOR_RESET, file_path, strerror(errno));
        return NULL;
    }

    printf(COLOR_GREEN "Successfully opened file: %s\n" COLOR_RESET, file_path);
    return fp;
}
