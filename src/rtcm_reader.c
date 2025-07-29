/**
 * @file rtcm_reader.c
 * @brief Reads RTCM messages from a text-formatted file and dispatches parsing for known message types.
 *
 * This function handles the input of RTCM messages in text format (one per line), as exported from logs or
 * test files. It identifies supported message types (currently 1019 and 1074), and calls the appropriate
 * parser function to extract useful GNSS data structures for later processing.
 *
 * Unsupported or malformed lines are safely skipped. Ephemeris and MSM4 messages are handled separately.
 */

#include "../include/algo.h"
#include "../include/rtcm_reader.h"
#include "../include/df_parser.h"

/**
 * @brief Reads the next valid RTCM message line from file and parses it if supported.
 *
 * Currently supports:
 * - RTCM 1019: Ephemeris (GPS)
 * - RTCM 1074: MSM4 (GPS L1 pseudorange and phase)
 *
 * Lines not matching these types are skipped. Each line should contain a complete message.
 *
 * @param fp Pointer to an open file for reading RTCM text lines.
 * @return 0 on successful read and parse, non-zero otherwise.
 */
int read_next_rtcm_message(FILE *fp)
{
    char line[4096];              // Buffer for reading lines
    unsigned int epoch_count = 1; // Counter for epochs processed

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        // Skip empty lines or comments
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\0' || line[0] == ' ' || line[0] == '\t')
        {
            continue;
        }

        // Debug print
        printf(COLOR_GREEN "Processing Epoch: %d \n" COLOR_RESET, epoch_count++);

        // Extract DF002 = message type
        char *df002_ptr = strstr(line, "DF002=");
        if (!df002_ptr)
        {
            fprintf(stderr, COLOR_YELLOW "Warning: DF002 not found in message. Skipping.\n" COLOR_RESET);
            continue;
        }

        int message_type = atoi(df002_ptr + 6);

        if (message_type != 1019 && message_type != 1074)
        {
            fprintf(stderr, COLOR_YELLOW "Warning: Unsupported message type %d. Skipping.\n" COLOR_RESET, message_type);
            continue;
        }

        if (message_type == 1019)
        {
            rtcm_1019_ephemeris_t eph = {0};
            if (parse_rtcm_1019(line, &eph) != 0)
            {
                fprintf(stderr, COLOR_YELLOW "Warning: Failed to parse RTCM 1019 message. Skipping.\n" COLOR_RESET);
                continue;
            }

            if (store_ephemeris(&eph) != 0)
            {
                fprintf(stderr, COLOR_YELLOW "Warning: Failed to store/update ephemeris for PRN %d\n" COLOR_RESET, eph.satellite_id);
            }

            continue;
        }

        else if (message_type == 1074)
        {
            rtcm_1074_msm4_t msm4 = {0};
            if (parse_rtcm_1074(line, &msm4) != 0)
            {
                fprintf(stderr, COLOR_YELLOW "Warning: Failed to parse RTCM 1074 message. Skipping.\n" COLOR_RESET);
                continue;
            }
            // TODO: Store or use observation data as needed
        }

        continue; // Successfully read and processed a supported message
    }

    return 1; // End of file or no valid message found
}
