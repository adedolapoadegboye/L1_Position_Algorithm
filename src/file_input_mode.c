/**
 * @file file_input_mode.c
 * @brief Handles RTCM MSM4 file input, step-by-step parsing and position resolution.
 *
 * This function is triggered when the user selects "File Input" from the application menu.
 * It opens a pre-recorded RTCM file (text or binary), reads each epoch's MSM4 message(s),
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
#include "../include/satellites.h"
#include "../include/receiver.h"
#include "../include/plots.h"

extern int n_times; // total epochs found during position estimation

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Main loop for processing a recorded RTCM input file.
 *
 * This function opens the specified RTCM file and iteratively processes epochs
 * by reading and parsing supported RTCM messages (e.g., MSM4). It prepares the
 * observation data and then runs the solver and coordinate conversions.
 *
 * @param is_parsed Set to true if the file is in pre-parsed (text) format.
 * @return 0 on success, 1 on error (open, read, sort, position, etc.).
 */

int file_input_mode(bool is_parsed)
{
    // Step 1: Attempt to open the RTCM file
    FILE *fp = file_connect(is_parsed);
    if (fp == NULL)
    {
        return 1; // Failed to open file
    }

    // Step 2: Loop through each epoch in the file and parse teh pseudorange and eph data
    // At the end of this loop, all MSM4 and ephemeris data will be stored in global tables, ready for processing
    int status = read_next_rtcm_message(fp);
    if (status != 0)
    {
        fprintf(stderr, COLOR_YELLOW "Warning: Error while reading RTCM message.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error reading message
    }

    // Step 3: Sort through the stored ephemeris and MSM4 data to prepare for position solving
    int sat_sorter_status = sort_satellites(eph_history, msm4_history, msm1_history);
    if (sat_sorter_status != 0)
    {
        fprintf(stderr, COLOR_RED "Error: Failed to sort satellites.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error sorting satellites
    }
    else
    {
        printf(COLOR_GREEN "Successfully sorted satellites.\n" COLOR_RESET);
        // print_gps_list(); // Print the sorted satellite data for debugging
    }

    // Step 4: Find satellite positions in ECI coordinates and convert to ECEF
    int eci_status = satellite_position_eci(gps_list);
    if (eci_status != 0)
    {
        fprintf(stderr, COLOR_RED "Error: Failed to find satellite positions in ECI.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error finding satellite positions
    }
    else
    {
        printf(COLOR_GREEN "Successfully found satellite positions in ECI.\n" COLOR_RESET);
    }

    int ecef_status = satellite_position_ecef(gps_list);
    if (ecef_status != 0)
    {
        fprintf(stderr, COLOR_RED "Error: Failed to convert satellite positions to ECEF.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error converting satellite positions
    }
    else
    {
        printf(COLOR_GREEN "Successfully converted satellite positions to ECEF.\n" COLOR_RESET);
    }

    // Step 5: Estimate full orbit for each satellite
    int orbit_status = satellite_orbit_eci(gps_list);
    if (orbit_status != 0)
    {
        fprintf(stderr, COLOR_RED "Error: Failed to estimate satellite orbits in ECI.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error estimating satellite orbits
    }
    else
    {
        printf(COLOR_GREEN "Successfully estimated satellite orbits in ECI.\n" COLOR_RESET);
    }

    // Step 6: Estimate receiver position in ECEF coordinates using least squares then convert to geodetic coordinates
    int position_status = estimate_receiver_positions();
    if (position_status != 0)
    {
        fprintf(stderr, COLOR_RED "Error: Failed to estimate receiver position.\n" COLOR_RESET);
        fclose(fp);
        return 1; // Error estimating receiver position
    }
    else
    {
        printf(COLOR_GREEN "Successfully estimated receiver position.\n" COLOR_RESET);
    }

    // Step 7: Plot all satellite positions and receiver position on a gnuplot
    if (write_receiver_track_ecef("plots/receiver_track_ecef.dat", n_times) == 0)
    {
        printf("[OK] Receiver track written successfully.\n");
    }
    else
    {
        fprintf(stderr, "[ERR] Failed to write receiver track data.\n");
    }

    if (write_sat_orbits("plots/sat_track_ecef.dat") == 0)
    {
        printf("[OK] Satellite orbits written successfully.\n");
    }
    else
    {
        fprintf(stderr, "[ERR] Failed to write satellite orbits data.\n");
    }

    if (write_receiver_track_geo("plots/receiver_track_geo.dat", n_times) == 0)
    {
        printf("[OK] Receiver Geo positions written successfully.\n");
    }
    else
    {
        fprintf(stderr, "[ERR] Failed to write receiver geo position data.\n");
    }
    if (write_receiver_ecef_epoch_km("plots/receiver_ecef_epoch.dat", n_times) == 0)
    {
        printf("[OK] Receiver ECEF (km) vs epoch written successfully.\n");
    }
    else
    {
        fprintf(stderr, "[ERR] Failed to write receiver ECEF (km) vs epoch.\n");
    }

    if (write_sat_xyz_km("plots/sat_xyz_km.dat") == 0)
    {
        printf("[OK] Satellite XY (km) written successfully.\n");
    }
    else
    {
        fprintf(stderr, "[ERR] Failed to write satellite XY (km).\n");
    }

    if (write_pseudorange_time_km("plots/pseudorange_time_km.dat") == 0)
    {
        printf("[OK] Pseudorange vs epoch (km) written successfully.\n");
    }
    else
    {
        fprintf(stderr, "[ERR] Failed to write pseudorange vs epoch (km).\n");
    }

    fclose(fp);
    return 0;
}
