/**
 * @file satellite_position_ecef.c
 * @brief Converts the satellite position from ECI coordinates to ECEF frame.
 *
 * Mirrors the Python implementation:
 *   theta = (t / 86400) % 1 * 2*pi   (solar day, not sidereal)
 * and performs ecef = eci * Rz(theta).
 * Since our helper does column-vector math (out = R * v),
 * we multiply by Rz^T to match the Python row*matrix result.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"

extern sat_eci_history_t sat_eci_positions[MAX_SAT + 1];

// mat3x3_vec3_mult: out = M * v (column-vector convention)
extern void mat3x3_vec3_mult(const double mat[3][3], const double vec[3], double out[3]);

static inline double normalize_time_seconds(double t)
{
    // Many paths already store seconds, but some may still be ms.
    // If t is suspiciously large, treat as milliseconds and convert.
    // (E.g., 159348000 ms => 159348 s)
    if (t > 1.0e6)
        return t / 1000.0;
    return t;
}

int satellite_position_ecef(const gps_satellite_data_t gps_lists[MAX_SAT + 1])
{
    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        for (int k = 0; k < MAX_EPOCHS; k++)
        {
            if (sat_eci_positions[prn].x[k] == 0.0 &&
                sat_eci_positions[prn].y[k] == 0.0 &&
                sat_eci_positions[prn].z[k] == 0.0)
            {
                continue;
            }
            else
            {
                // printf(s"PRN: %d, Orbit ECI[%d][0]: %.3f, Orbit ECI[%d][1]: %.3f, Orbit ECI[%d][2]: %.3f\n", prn, k, sat_eci_positions[prn].x[k] / 1000, k, sat_eci_positions[prn].y[k] / 1000, k, sat_eci_positions[prn].z[k] / 1000);
            }

            // Need a corresponding timestamp
            double t_raw = gps_lists[prn].times_of_pseudorange[k];
            if (t_raw == 0.0)
                continue;

            double t_sec = normalize_time_seconds(t_raw);
            double frac_day = fmod(t_sec / 86400.0, 1.0);
            if (frac_day < 0.0)
                frac_day += 1.0;
            double theta = frac_day * 2.0 * M_PI;

            const double c = cos(theta), s = sin(theta);
            const double Rz_T[3][3] = {
                {c, s, 0.0},
                {-s, c, 0.0},
                {0.0, 0.0, 1.0}};

            const double eci[3] = {
                sat_eci_positions[prn].x[k],
                sat_eci_positions[prn].y[k],
                sat_eci_positions[prn].z[k]};

            double ecef[3];
            mat3x3_vec3_mult(Rz_T, eci, ecef);

            sat_ecef_positions[prn].x[k] = ecef[0];
            sat_ecef_positions[prn].y[k] = ecef[1];
            sat_ecef_positions[prn].z[k] = ecef[2];
            sat_ecef_positions[prn].t_ms[k] = t_sec * 1000.0; // store as ms

            // Debug print
            // printf("PRN %d, Epoch %d: t=%.3f s  theta=%.6f rad  ECI=[%.3f, %.3f, %.3f]  ECEF=[%.3f, %.3f, %.3f]\n",
            //        prn, k, t_sec, theta, eci[0], eci[1], eci[2], ecef[0], ecef[1], ecef[2]);
        }
    }
    return 0;
}
