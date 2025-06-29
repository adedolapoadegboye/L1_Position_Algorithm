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
#include "../include/position_solver.h"

int calculate_position_least_squares(GNSS_ObservationSet *obs, ECEF_Coordinate *receiver_ecef)
{
    if (obs->sat_count < 4)
    {
        return 1; // Not enough satellites for 3D fix
    }

    // Very basic placeholder: return hardcoded position
    receiver_ecef->x = 1113131.0;
    receiver_ecef->y = -4848484.0;
    receiver_ecef->z = 3987654.0;
    return 0;
}
