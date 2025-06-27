/**
 * @file file_input_mode.c
 * @brief Handles RTCM MSM4 file input, step-by-step parsing and position resolution.
 *
 * This function is triggered when the user selects "File Input" from the application menu.
 * It opens a pre-recorded RTCM binary file, reads each epoch's MSM4 message, extracts relevant
 * GNSS observations, and calculates the receiver position using the least squares method.
 * The resulting position is printed in geodetic coordinates (decimal degrees).
 *
 * All major steps (reading, parsing, solving) are delegated to their respective modules
 * to maintain modularity and educational clarity.
 */

#include "../include/algo.h"

int read_next_rtcm_message(FILE *fp, RTCM_Message *message)
{
    static unsigned char buffer[1024]; // Assume message won't exceed 1024 bytes

    // Try to read a single line or fixed-size message (simulate for now)
    size_t bytes_read = fread(buffer, 1, 512, fp); // For simulation only
    if (bytes_read == 0)
    {
        return 1; // EOF or error
    }

    message->buffer = buffer;
    message->length = bytes_read;
    return 0;
}
