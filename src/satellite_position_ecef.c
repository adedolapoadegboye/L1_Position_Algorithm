/**
 * @file satellite_position_ecef.c
 * @brief Converts the satellite position from ECI coordinates to ECEF frame.
 *
 * This file contains functions to convert the satellite position from Earth-Centered Inertial (ECI) coordinates
 * to Earth-Centered Earth-Fixed (ECEF) coordinates using the provided ephemeris data and time of week.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/geo_utils.h"
#include "../include/position_solver.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"

int satellite_position_ecef(const sat_eci_history_t sat_eci_position[MAX_SAT + 1], const gps_satellite_data_t gps_lists[MAX_SAT + 1])
{
    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        if (sat_eci_position[prn].x[0] == 0 && sat_eci_position[prn].y[0] == 0 && sat_eci_position[prn].z[0] == 0)
        {
            printf("No ECI data available for PRN %d\n", prn);
            continue;
        }

        for (int k = 0; k < MAX_EPOCHS; k++)
        {
            if (sat_eci_position[prn].x[k] == 0 && sat_eci_position[prn].y[k] == 0 && sat_eci_position[prn].z[k] == 0)
                continue;

            double eci[3] = {
                sat_eci_position[prn].x[k],
                sat_eci_position[prn].y[k],
                sat_eci_position[prn].z[k]};

            double gps_time_sec = gps_lists[prn].times_of_pseudorange[k] / 1000;
            double theta = fmod(OMEGA_EARTH * gps_time_sec, 2 * M_PI);

            double Rz_theta[3][3] = {
                {cos(theta), -sin(theta), 0}, // FIXED SIGN
                {sin(theta), cos(theta), 0},
                {0, 0, 1}};

            double ecef[3] = {0};
            mat3x3_vec3_mult(Rz_theta, eci, ecef);

            sat_ecef_positions[prn].x[k] = ecef[0];
            sat_ecef_positions[prn].y[k] = ecef[1];
            sat_ecef_positions[prn].z[k] = ecef[2];

            printf("PRN %d, Epoch %d: PR Time (s): %.0f  Theta: %.3f rad  Rz_theta[%.3f, %.3f, %.3f] ECEF = [%.3f, %.3f, %.3f]\n",
                   prn, k, gps_time_sec, theta,
                   Rz_theta[0][0], Rz_theta[0][1], Rz_theta[0][2],
                   ecef[0], ecef[1], ecef[2]);
        }
    }

    return 0;
}
