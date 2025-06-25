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

int file_input_mode(void)
{
    // Step 1: Attempt to open the RTCM file
    FILE *fp = file_connect();
    if (fp == NULL)
    {
        return 1; // Failed to open file
    }

    // Step 2: Loop through each epoch in the file
    while (!feof(fp))
    {
        RTCM_Message message;
        GNSS_ObservationSet obs;

        // Step 2.1: Read next RTCM message (expecting MSM4 for GPS L1)
        int status = read_next_rtcm_message(fp, &message);
        if (status != 0)
        {
            fprintf(stderr, COLOR_YELLOW "Warning: Failed to read RTCM message. Skipping to next.\n" COLOR_RESET);
            continue;
        }

        // Step 2.2: Parse the MSM4 message for L1 observations
        status = parse_msm4_message(&message, &obs);
        if (status != 0)
        {
            fprintf(stderr, COLOR_YELLOW "Warning: Failed to parse MSM4 message. Skipping epoch.\n" COLOR_RESET);
            continue;
        }

        // Step 2.3: Calculate position using least squares
        ECEF_Coordinate receiver_ecef;
        status = calculate_position_least_squares(&obs, &receiver_ecef);
        if (status != 0)
        {
            fprintf(stderr, COLOR_YELLOW "Warning: Least squares solution failed. Skipping epoch.\n" COLOR_RESET);
            continue;
        }

        // Step 2.4: Convert ECEF to geodetic coordinates
        Geodetic_Coordinate receiver_geo;
        ecef_to_geodetic(&receiver_ecef, &receiver_geo);

        // Step 2.5: Output the final position
        printf(COLOR_GREEN "\nEpoch Position:\n" COLOR_RESET);
        printf("Latitude  : %.8f°\n", receiver_geo.lat_deg);
        printf("Longitude : %.8f°\n", receiver_geo.lon_deg);
        printf("Altitude  : %.3f m\n", receiver_geo.alt_m);
    }

    fclose(fp);
    return 0;
}
