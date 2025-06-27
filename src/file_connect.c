/**
 * @file file_connect.c
 * @brief Provides utilities for file handling in GNSS processing applications.
 */

#include "../include/algo.h"

/// Default file path used when user provides no input
#define DEFAULT_FILE_PATH "example/log.txt"

/// Max number of retry attempts for file input
#define MAX_RETRIES 3

/**
 * @brief Opens a pre-parsed RTCM log file in text format for reading.
 *
 * Prompts the user to enter the path to a human-readable RTCM log file.
 * If the user enters nothing, the function uses a default path.
 * Retries up to MAX_RETRIES if the file cannot be opened.
 *
 * @return FILE* Pointer to the opened file, or NULL on failure.
 */
FILE *file_connect(void)
{
    char file_path[256];
    FILE *fp = NULL;

    for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt)
    {
        printf(COLOR_BLUE "\nEnter the path to the parsed RTCM text file,\n"
                          "or press Enter to use the default (%s):\n> " COLOR_RESET,
               DEFAULT_FILE_PATH);

        // Get user input
        if (fgets(file_path, sizeof(file_path), stdin) == NULL)
        {
            fprintf(stderr, COLOR_RED "Input error.\n" COLOR_RESET);
            return NULL;
        }

        // Remove trailing newline
        size_t len = strlen(file_path);
        if (len > 0 && file_path[len - 1] == '\n')
            file_path[len - 1] = '\0';

        // If blank input, use default
        if (strlen(file_path) == 0)
        {
            snprintf(file_path, sizeof(file_path), "%s", DEFAULT_FILE_PATH);
        }

        // Try to open in text mode
        fp = fopen(file_path, "r");
        if (fp != NULL)
        {
            printf(COLOR_GREEN "Successfully opened file: %s\n" COLOR_RESET, file_path);
            return fp;
        }

        // File couldn't be opened
        fprintf(stderr, COLOR_RED "Error: Could not open file '%s': %s\n" COLOR_RESET,
                file_path, strerror(errno));
        printf(COLOR_YELLOW "Attempt %d of %d failed.\n" COLOR_RESET, attempt, MAX_RETRIES);
    }

    fprintf(stderr, COLOR_RED "Failed to open file after %d attempts. Exiting.\n" COLOR_RESET, MAX_RETRIES);
    return NULL;
}
