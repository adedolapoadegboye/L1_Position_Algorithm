/**
 * @file satellite_orbit_eci.c
 * @brief Computes the full satellite orbit for each satellite using the EPH data.
 *
 * For each PRN that has ephemeris, we sweep true anomaly f from 0..2π,
 * compute PQW coordinates (in meters), then rotate to ECI with:
 *   Rz(-ω) -> Rx(-i) -> Rz(-Ω)
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"

// Column-vector multiply: out = M * v
extern void mat3x3_vec3_mult(const double mat[3][3], const double vec[3], double out[3]);

int satellite_orbit_eci(const gps_satellite_data_t gps_lists[])
{
    // Step size in radians (~0.57 deg). Adjust as needed.
    const double step_size = 0.01;

    // Number of steps to cover 0..2π inclusive
    int n_steps = (int)(2.0 * M_PI / step_size) + 1;
    if (n_steps < 2)
        n_steps = 2;

    int max_store = n_steps + 1;

    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        // Require ephemeris for this PRN
        if (gps_lists[prn].times_of_ephemeris[0] == 0)
        {
            // printf("No EPH data available for PRN %d\n", prn);
            continue;
        }

        // Pull first-epoch orbital elements (as your Python does with [0])
        const double a = gps_lists[prn].semi_major_axes[0];                       // meters
        const double e = gps_lists[prn].eccentricities[0];                        // unitless
        const double i = gps_lists[prn].inclinations[0];                          // radians
        const double omega = gps_lists[prn].argument_of_periapsis[0];             // radians
        const double Omega = gps_lists[prn].right_ascension_of_ascending_node[0]; // radians

        // Rotation matrices for column-vector convention
        const double Rz_omega[3][3] = {
            {cos(-omega), sin(-omega), 0.0},
            {-sin(-omega), cos(-omega), 0.0},
            {0.0, 0.0, 1.0}};

        const double Rx_i[3][3] = {
            {1.0, 0.0, 0.0},
            {0.0, cos(-i), sin(-i)},
            {0.0, -sin(-i), cos(-i)}};

        const double Rz_Omega[3][3] = {
            {cos(-Omega), sin(-Omega), 0.0},
            {-sin(-Omega), cos(-Omega), 0.0},
            {0.0, 0.0, 1.0}};

        // Sweep true anomaly f and build orbit
        for (int k = 0; k < max_store; k++)
        {
            double f = k * step_size;
            if (f > 2.0 * M_PI)
                f = 2.0 * M_PI; // clamp last point

            // PQW radius (meters), using true anomaly f
            const double denom = 1.0 + e * cos(f);
            if (denom == 0.0) // extremely pathological; skip
            {
                sat_orbit_pqw_positions[prn].p[k] = 0.0;
                sat_orbit_pqw_positions[prn].q[k] = 0.0;
                sat_orbit_pqw_positions[prn].w[k] = 0.0;
                sat_orbit_eci_positions[prn].x[k] = 0.0;
                sat_orbit_eci_positions[prn].y[k] = 0.0;
                sat_orbit_eci_positions[prn].z[k] = 0.0;
                continue;
            }

            double r = (a * (1.0 - e * e)) / denom; // meters

            // PQW coords (meters)
            double pqw[3] = {r * cos(f), r * sin(f), 0.0};

            // Save PQW (meters)
            sat_orbit_pqw_positions[prn].p[k] = pqw[0];
            sat_orbit_pqw_positions[prn].q[k] = pqw[1];
            sat_orbit_pqw_positions[prn].w[k] = pqw[2];

            // Rotate PQW -> ECI with Rz(-ω), Rx(-i), Rz(-Ω)
            double tmp1[3], tmp2[3], eci[3];
            mat3x3_vec3_mult(Rz_omega, pqw, tmp1);
            mat3x3_vec3_mult(Rx_i, tmp1, tmp2);
            mat3x3_vec3_mult(Rz_Omega, tmp2, eci);

            // Store ECI (meters)
            sat_orbit_eci_positions[prn].x[k] = eci[0];
            sat_orbit_eci_positions[prn].y[k] = eci[1];
            sat_orbit_eci_positions[prn].z[k] = eci[2];

            printf("PRN: %d, Orbit ECI[%d][0]: %.3f, Orbit ECI[%d][1]: %.3f, Orbit ECI[%d][2]: %.3f\n", prn, k, sat_orbit_eci_positions[prn].x[k] / 1000, k, sat_orbit_eci_positions[prn].y[k] / 1000, k, sat_orbit_eci_positions[prn].z[k] / 1000);
        }
    }

    return 0;
}
