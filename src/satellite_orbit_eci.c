/**
 * @file satellite_orbit_eci.c
 * @brief Computes the full satellite orbit for each satellite using the EPH data.
 *
 * This file contains functions to estimate the satellite full orbit in Earth-Centered Inertial (ECI) coordinates
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"

int satellite_orbit_eci(const gps_satellite_data_t gps_lists[])
{
    double step_size = 0.01;

    double mean_anomalies[(int)(2 * M_PI / step_size) + 2];
    int n_steps = (int)(2 * M_PI / step_size) + 2;
    for (int i = 0; i < n_steps; i++)
    {
        mean_anomalies[i] = i * step_size;
        // printf("Mean Anomaly[%d]: %.3f\n", i, mean_anomalies[i]);
    }

    for (int prn = 1; prn <= MAX_SAT; prn++)
    {

        if (gps_lists[prn].pseudoranges[0] == 0)
        {
            printf("No EPH data available for PRN %d\n", prn);
            continue;
        }

        // Allocate arrays for PQW and ECI
        double pqw[n_steps][3];
        double eci[n_steps][3];

        // Compute PQW for all mean anomalies
        for (int j = 0; j < n_steps; j++)
        {
            double radius = (gps_lists[prn].semi_major_axes[0] * (1 - pow(gps_lists[prn].eccentricities[0], 2))) / (1 + gps_lists[prn].eccentricities[0] * cos(mean_anomalies[j]));

            // printf("Radius for PRN %d, Mean Anomaly %d: %.3f\n", prn, j, radius);

            pqw[j][0] = (radius * cos(mean_anomalies[j])) / 1000;
            pqw[j][1] = (radius * sin(mean_anomalies[j])) / 1000;
            pqw[j][2] = 0.0;

            sat_orbit_pqw_positions[prn].p[j] = pqw[j][0];
            sat_orbit_pqw_positions[prn].q[j] = pqw[j][1];
            sat_orbit_pqw_positions[prn].w[j] = pqw[j][2];

            // printf("PRN: %d, Orbit PQW[%d][0]: %.3f, Orbit PQW[%d][1]: %.3f, Orbit PQW[%d][2]: %.3f\n", prn, j, pqw[j][0], j, pqw[j][1], j, pqw[j][2]);
        }
        // Compute ECI for all PQW for this PRN
        for (int k = 0; k < n_steps; k++)
        {
            // Rotation matrices as before
            double Rz_omega[3][3] = {
                {cos(-(gps_lists[prn].argument_of_periapsis[0])), -sin(-(gps_lists[prn].argument_of_periapsis[0])), 0},
                {sin(-(gps_lists[prn].argument_of_periapsis[0])), cos(-(gps_lists[prn].argument_of_periapsis[0])), 0},
                {0, 0, 1}};

            double Rx_i[3][3] = {
                {1, 0, 0},
                {0, cos(-(gps_lists[prn].inclinations[0])), -sin(-(gps_lists[prn].inclinations[0]))},
                {0, sin(-(gps_lists[prn].inclinations[0])), cos(-(gps_lists[prn].inclinations[0]))}};

            double Rz_Omega[3][3] = {
                {cos(-(gps_lists[prn].right_ascension_of_ascending_node[0])), -sin(-(gps_lists[prn].right_ascension_of_ascending_node[0])), 0},
                {sin(-(gps_lists[prn].right_ascension_of_ascending_node[0])), cos(-(gps_lists[prn].right_ascension_of_ascending_node[0])), 0},
                {0, 0, 1}};

            double temp1[3], temp2[3];
            mat3x3_vec3_mult(Rz_omega, pqw[k], temp1);
            mat3x3_vec3_mult(Rx_i, temp1, temp2);
            mat3x3_vec3_mult(Rz_Omega, temp2, eci[k]);

            // Store ECI coordinates
            sat_orbit_eci_positions[prn].x[k] = eci[k][0];
            sat_orbit_eci_positions[prn].y[k] = eci[k][1];
            sat_orbit_eci_positions[prn].z[k] = eci[k][2];

            // printf("PRN: %d, Orbit ECI[%d][0]: %.3f, Orbit ECI[%d][1]: %.3f, Orbit ECI[%d][2]: %.3f\n", prn, k, eci[k][0], k, eci[k][1], k, eci[k][2]);
        }
    }
    return 0;
}
