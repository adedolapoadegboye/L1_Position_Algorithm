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
static void mat3x3_vec3_mult(const double mat[3][3], const double vec[3], double out[3])
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

        // Use first ephemeris for all epochs (as in your Python code)
        double a = gps_lists[prn].semi_major_axes[0];
        double e = gps_lists[prn].eccentricities[0];
        double i0 = gps_lists[prn].inclinations[0];
        double M0 = gps_lists[prn].mean_anomalies[0];
        double Omega = gps_lists[prn].right_ascension_of_ascending_node[0];
        double omega = gps_lists[prn].argument_of_periapsis[0];
        double toe = gps_lists[prn].times_of_ephemeris[0];

        // Iterate through each epoch of the satellite data
        for (int k = 0; k < MAX_EPOCHS; k++)
        {
            if (gps_lists[prn].times_of_pseudorange[k] == 0)
                continue;

            // Time difference (seconds)
            double time_diff = gps_lists[prn].times_of_pseudorange[k] - gps_lists[prn].times_of_ephemeris[k];

            // Mean motion (rad/s)
            double n = sqrt(MU / pow(a, 3));

            // Mean anomaly at this epoch
            double M = M0 + n * time_diff;

            // True anomaly approximation (series expansion)
            double true_anomaly = M + (2 * e - 0.25 * pow(e, 3)) * sin(M) + (1.25 * pow(e, 2)) * sin(2 * M) + (13.0 / 12.0) * pow(e, 3) * sin(3 * M);

            // Perifocal coordinates
            double r = a * (1 - e * e) / (1 + e * cos(true_anomaly));
            double p = r * cos(true_anomaly);
            double q = r * sin(true_anomaly);
            double w = 0.0;
            double pqw[3] = {p, q, w};

            // Rotation matrices
            double Rz_omega[3][3] = {
                {cos(-omega), -sin(-omega), 0},
                {sin(-omega), cos(-omega), 0},
                {0, 0, 1}};
            double Rx_i[3][3] = {
                {1, 0, 0},
                {0, cos(-i0), -sin(-i0)},
                {0, sin(-i0), cos(-i0)}};
            double Rz_Omega[3][3] = {
                {cos(-Omega), -sin(-Omega), 0},
                {sin(-Omega), cos(-Omega), 0},
                {0, 0, 1}};

            // Apply rotations: PQW -> ECI
            double temp1[3], temp2[3], eci[3];
            mat3x3_vec3_mult(Rz_omega, pqw, temp1);
            mat3x3_vec3_mult(Rx_i, temp1, temp2);
            mat3x3_vec3_mult(Rz_Omega, temp2, eci);

            printf("PRN %d, Epoch %d: ECI = [%.3f, %.3f, %.3f]\n", prn, k, eci[0], eci[1], eci[2]);
        }
    }
    return 0;
}
