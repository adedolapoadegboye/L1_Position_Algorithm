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
#include "../include/df_parser.h"

int parse_msm4_message(RTCM_Message *message, GNSS_ObservationSet *obs)
{

    // Mark unused parameter to avoid compiler warning
    (void)message;

    // For demo purposes, simulate two satellites with fake values
    obs->sat_count = 2;
    obs->epoch_time = 123456.0;

    obs->sats[0].sat_id = 1;
    obs->sats[0].pseudorange = 21456789.123;
    obs->sats[0].carrier_phase = 110000.456;
    obs->sats[0].doppler = -1234.5;
    obs->sats[0].snr = 45.0;

    obs->sats[1].sat_id = 2;
    obs->sats[1].pseudorange = 22345678.789;
    obs->sats[1].carrier_phase = 109000.789;
    obs->sats[1].doppler = -1234.0;
    obs->sats[1].snr = 43.5;

    return 0;
}
