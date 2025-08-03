/**
 * @file satellites_position_eci.c
 * @brief Calculates the satellite position in ECI frame using ephemeris data.
 * This function computes the satellite position in Earth-Centered Inertial (ECI)
 * coordinates based on the provided ephemeris data and time of week.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/geo_utils.h"
#include "../include/position_solver.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"

// Helper: Rotation matrix multiplication for 3x3 and 1x3 vector
void mat3x3_vec3_mult(const double mat[3][3], const double vec[3], double out[3])
{
    for (int i = 0; i < 3; i++)
    {
        out[i] = mat[i][0] * vec[0] + mat[i][1] * vec[1] + mat[i][2] * vec[2];
    }
}

int satellite_position_eci(const gps_satellite_data_t gps_lists[])
{
    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        if (gps_lists[prn].times_of_ephemeris[0] == 0)
        {
            printf("No ephemeris data available for PRN %d\n", prn);
            continue;
        }

        // Iterate through each epoch of the satellite data
        for (int k = 0; k < MAX_EPOCHS; k++)
        {
            if (gps_lists[prn].times_of_pseudorange[k] == 0)
                continue;

            // Time difference (seconds)
            double time_diff = (gps_lists[prn].times_of_pseudorange[k] / 1000) - gps_lists[prn].times_of_ephemeris[k];

            // Mean motion (deg/s)
            double mean_motion = sqrt(MU / pow(gps_lists[prn].semi_major_axes[k], 3));

            // Mean anomaly at this epoch
            double mean_anomaly = gps_lists[prn].mean_anomalies[k] + (mean_motion * time_diff);

            // True anomaly approximation (series expansion)
            double true_anomaly = mean_anomaly + (2 * gps_lists[prn].eccentricities[k] - 0.25 * pow(gps_lists[prn].eccentricities[k], 3)) * sin(mean_anomaly) + (1.25 * pow(gps_lists[prn].eccentricities[k], 2)) * sin(2 * mean_anomaly) + (13.0 / 12.0) * pow(gps_lists[prn].eccentricities[k], 3) * sin(3 * mean_anomaly);

            printf("PRN %d, Epoch %d: Time delta= %.3f Mean motion= %.3f Mean Anomaly = %.3f, True Anomaly = %.3f\n", prn, k, time_diff, mean_motion, mean_anomaly, true_anomaly);

            // Perifocal coordinates
            double radius = gps_lists[prn].semi_major_axes[k] * (1 - pow(gps_lists[prn].eccentricities[k], 2)) / (1 + gps_lists[prn].eccentricities[k] * cos(true_anomaly));
            double p = radius * cos(true_anomaly);
            double q = radius * sin(true_anomaly);
            double w = 0.0;
            double pqw[3] = {p, q, w};

            printf("PRN %d, Epoch %d: PQW = [%.3f, %.3f, %.3f]\n", prn, k, pqw[0], pqw[1], pqw[2]);

            // // Rotation matrices
            double Rz_omega[3][3] = {
                {cos(-gps_lists[prn].argument_of_periapsis[k]), -sin(-gps_lists[prn].argument_of_periapsis[k]), 0},
                {sin(-gps_lists[prn].argument_of_periapsis[k]), cos(-gps_lists[prn].argument_of_periapsis[k]), 0},
                {0, 0, 1}};

            double Rx_i[3][3] = {
                {1, 0, 0},
                {0, cos(-gps_lists[prn].inclinations[k]), -sin(-gps_lists[prn].inclinations[k])},
                {0, sin(-gps_lists[prn].inclinations[k]), cos(-gps_lists[prn].inclinations[k])}};

            double Rz_Omega[3][3] = {
                {cos(-gps_lists[prn].right_ascension_of_ascending_node[k]), -sin(-gps_lists[prn].right_ascension_of_ascending_node[k]), 0},
                {sin(-gps_lists[prn].right_ascension_of_ascending_node[k]), cos(-gps_lists[prn].right_ascension_of_ascending_node[k]), 0},
                {0, 0, 1}};

            // Apply rotations: PQW -> ECI
            double temp1[3], temp2[3], eci[3];
            mat3x3_vec3_mult(Rz_omega, pqw, temp1);
            mat3x3_vec3_mult(Rx_i, temp1, temp2);
            mat3x3_vec3_mult(Rz_Omega, temp2, eci);

            // Save ECI position
            sat_eci_positions[prn].x[k] = eci[0];
            sat_eci_positions[prn].y[k] = eci[1];
            sat_eci_positions[prn].z[k] = eci[2];

            printf("PRN %d, Epoch %d: ECI = [%.3f, %.3f, %.3f]\n", prn, k, eci[0], eci[1], eci[2]);
        }
    }
    return 0;
}
