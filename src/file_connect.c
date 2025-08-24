/**
 * @file file_connect.c
 * @brief File input utility for GNSS Positioning Application.
 *
 * Provides user interaction for opening RTCM log files (either parsed text logs
 * or raw binary logs). Retries are supported, and a default file path is used
 * if no input is provided.
 */

#include "../include/algo.h"

/// Default file path used when user provides no input
#define DEFAULT_FILE_PATH "example/parsed_log.txt"

/// Maximum number of retry attempts for file input
#define MAX_RETRIES 3

/**
 * @brief Open an RTCM log file for reading.
 *
 * Prompts the user to provide a path to an RTCM log file. If the input is left
 * blank, a default path is used. Retries up to MAX_RETRIES if the file cannot
 * be opened. Supports two modes:
 *   - Parsed mode (`is_parsed = true`): opens the file in text mode (`"r"`).
 *   - Raw mode (`is_parsed = false`): opens the file in binary mode (`"rb"`).
 *
 * @param[in]  is_parsed  True to open a parsed text log file, false for raw binary.
 * @return FILE* Pointer to the opened file, or NULL on failure after all retries.
 *
 * @todo Implement raw binary RTCM parsing when `is_parsed = false`.
 */
FILE *file_connect(bool is_parsed)
{
    char file_path[256];
    FILE *fp = NULL;

    for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt)
    {
        printf(COLOR_BLUE "\nEnter the path to the %s RTCM file,\n"
                          "or press Enter to use the default (%s):\n> " COLOR_RESET,
               is_parsed ? "parsed text" : "raw binary",
               DEFAULT_FILE_PATH);

        // Get user input
        if (fgets(file_path, sizeof(file_path), stdin) == NULL)
        {
            fprintf(stderr, COLOR_RED "Input error.\n" COLOR_RESET);
            return NULL;
        }

        // Strip trailing newline
        size_t len = strlen(file_path);
        if (len > 0 && file_path[len - 1] == '\n')
            file_path[len - 1] = '\0';

        // If blank, use default path
        if (strlen(file_path) == 0)
        {
            snprintf(file_path, sizeof(file_path), "%s", DEFAULT_FILE_PATH);
        }

        // Attempt to open file
        if (is_parsed)
        {
            fp = fopen(file_path, "r"); ///< Parsed text log
        }
        else
        {
            fp = fopen(file_path, "rb"); ///< Raw binary log
            // @todo Add logic for parsing raw RTCM binary streams
        }

        if (fp != NULL)
        {
            printf(COLOR_GREEN "Successfully opened file: %s\n" COLOR_RESET, file_path);
            return fp;
        }

        // File open failed
        fprintf(stderr, COLOR_RED "Error: Could not open file '%s': %s\n" COLOR_RESET,
                file_path, strerror(errno));
        printf(COLOR_YELLOW "Attempt %d of %d failed.\n" COLOR_RESET, attempt, MAX_RETRIES);
    }

    fprintf(stderr, COLOR_RED "Failed to open file after %d attempts. Exiting.\n" COLOR_RESET, MAX_RETRIES);
    return NULL;
}
