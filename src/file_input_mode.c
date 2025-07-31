/**
 * @file file_input_mode.c
 * @brief Handles RTCM MSM4 file input, step-by-step parsing and position resolution.
 *
 * This function is triggered when the user selects "File Input" from the application menu.
 * It opens a pre-recorded RTCM file (text or binary), reads each epoch's MSM4 message,
 * extracts relevant GNSS observations, and calculates the receiver position using
 * the least squares method.
 *
 * The resulting receiver position is output in geodetic coordinates (decimal degrees).
 *
 * All major processing stages (file handling, message reading, observation parsing,
 * position solving, and coordinate conversion) are handled by separate modules
 * for clarity and maintainability.
 */

#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/geo_utils.h"
#include "../include/position_solver.h"

/**
 * @brief Main loop for processing a recorded RTCM input file.
 *
 * This function opens the specified RTCM file and iteratively processes each epoch
 * by reading and parsing supported RTCM messages (e.g., MSM4). It prepares the
 * observation data and, in future steps, will apply a least squares solver and
 * convert to geodetic coordinates.
 *
 * @param is_parsed Set to true if the file is in pre-parsed (text) format.
 * @return 0 on success, 1 if file open failed or processing error occurred.
 */
int file_input_mode(bool is_parsed)
{
    // Step 1: Attempt to open the RTCM file
    FILE *fp = file_connect(is_parsed);
    if (fp == NULL)
    {
        return 1; // Failed to open file
    }

    // Step 2: Loop through each epoch in the file
    int status = read_next_rtcm_message(fp);
    if (status != 0)
    {
        fprintf(stderr, COLOR_YELLOW "Warning: Error while reading RTCM message.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error reading message
    }

    print_all_stored_ephemeris();

    // Future Steps:
    // - Calculate position using least squares
    // - Convert ECEF to geodetic
    // - Print output position
    //
    // Example (commented for now):
    // ECEF_Coordinate receiver_ecef;
    // status = calculate_position_least_squares(&obs, &receiver_ecef);
    // if (status != 0)
    // {
    //     fprintf(stderr, COLOR_YELLOW "Warning: Least squares solution failed. Skipping epoch.\n" COLOR_RESET);
    //     continue;
    // }
    //
    // Geodetic_Coordinate receiver_geo;
    // ecef_to_geodetic(&receiver_ecef, &receiver_geo);
    //
    // printf(COLOR_GREEN "\nEpoch Position:\n" COLOR_RESET);
    // printf("Latitude  : %.8f\xC2\xB0\n", receiver_geo.lat_deg);
    // printf("Longitude : %.8f\xC2\xB0\n", receiver_geo.lon_deg);
    // printf("Altitude  : %.3f m\n", receiver_geo.alt_m);

    fclose(fp);
    return 0;
}
