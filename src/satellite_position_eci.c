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

// Helper: Find the closest eph index for prn whose EPH time_of_week is just before or equal to pseudorange_time
static int find_closest_eph_idx(const eph_history_t *hist, uint32_t pseudorange_time)
{
    int best_idx = -1;
    uint32_t best_toe = 0;
    for (size_t i = 0; i < hist->count; i++)
    {
        uint32_t toe = hist->eph[i].gps_toe; // use TOE
        if (toe <= pseudorange_time && toe >= best_toe)
        {
            best_toe = toe;
            best_idx = (int)i;
        }
    }
    return best_idx;
}

int satellite_position_eci(const gps_satellite_data_t gps_lists[])
{
    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        // Skip PRNs with no ephemeris history at all
        if (eph_history[prn].count == 0)
        {
            // printf("No ephemeris history for PRN %d\n", prn);
            continue;
        }

        for (int k = 0; k < MAX_EPOCHS; k++)
        {
            if (gps_lists[prn].times_of_pseudorange[k] == 0)
                continue;

            // --- 1) Normalize observation time to seconds ---
            double t_obs = gps_lists[prn].times_of_pseudorange[k];
            if (t_obs > 604800.0)
                t_obs *= 1.0 / 1000.0;

            // --- 2) Find best ephemeris for this observation time (TOE <= t_obs) ---
            int eph_idx = find_closest_eph_idx(&eph_history[prn], (uint32_t)t_obs);
            if (eph_idx < 0)
            {
                // No suitable ephemeris before/at this time; skip
                // printf("No suitable ephemeris for PRN %d at t=%.3f\n", prn, t_obs);
                continue;
            }

            const rtcm_1019_ephemeris_t *eph = &eph_history[prn].eph[eph_idx];

            // Pull elements from ephemeris (radians & meters as you stored them)
            const double a = eph->semi_major_axis;
            const double e = eph->eccentricity;
            const double i = eph->inclination;
            const double Omega = eph->right_ascension_of_ascending_node;
            const double omega = eph->argument_of_periapsis;
            const double M0 = eph->mean_anomaly;
            const double toe = (double)eph->gps_toe; // seconds

            // Basic sanity guards
            if (!(a > 0.0) || !(e >= 0.0 && e < 1.0) || !isfinite(i) || !isfinite(M0))
            {
                // printf("Bad elements for PRN %d at k=%d (a=%g, e=%g)\n", prn, k, a, e);
                continue;
            }

            // --- 3) Time since ephemeris epoch ---
            const double dt = t_obs - toe;
            // printf("PRN %d, Epoch %d: dt = %.3f seconds\n", prn, k, dt);

            // --- 4) Mean motion (rad/s) and mean anomaly ---
            const double n = sqrt(MU / (a * a * a)); // MU in m^3/s^2
            double M = M0 + n * dt;

            // Normalize M into [-pi, pi] for numerical stability
            M = fmod(M + M_PI, 2.0 * M_PI);
            if (M < 0)
                M += 2.0 * M_PI;
            M -= M_PI;

            // --- 5) Solve Kepler for E (eccentric anomaly) via simple iteration (good enough for GPS e) ---
            double E = M; // initial guess
            for (int it = 0; it < 10; ++it)
            {
                double f = E - e * sin(E) - M;
                double fp = 1.0 - e * cos(E);
                double dE = -f / fp;
                E += dE;
                if (fabs(dE) < 1e-12)
                    break;
            }

            // True anomaly
            double cosE = cos(E), sinE = sin(E);
            double sqrt1me2 = sqrt(fmax(0.0, 1.0 - e * e));
            double sinv = (sqrt1me2 * sinE) / (1.0 - e * cosE);
            double cosv = (cosE - e) / (1.0 - e * cosE);
            double v = atan2(sinv, cosv);
            // printf("PRN %d, Epoch %d: True Anomaly = %.6f\n", prn, k, v);

            // Radius in the orbital plane
            double r = a * (1.0 - e * cosE);
            if (!(r > 0.0) || !isfinite(r))
            {
                // printf("Bad radius for PRN %d (r=%g)\n", prn, r);
                continue;
            }

            // PQW coordinates (perifocal frame)
            double pqw[3] = {r * cos(v), r * sin(v), 0.0};

            // printf("PRN %d, Epoch %d: PQW = [%.3f, %.3f, %.3f]\n", prn, k, pqw[0], pqw[1], pqw[2]);

            // --- 6) Rotation matrices to ECI: Rz(Ω) * Rx(i) * Rz(ω) ---
            // We rotate +ω about z, +i about x, +Ω about z (no leading negatives needed)
            const double cO = cos(Omega), sO = sin(Omega);
            const double ci = cos(i), si = sin(i);
            const double co = cos(omega), so = sin(omega);

            // Rz(ω)
            double Rz_omega[3][3] = {
                {co, -so, 0},
                {so, co, 0},
                {0, 0, 1}};
            // Rx(i)
            double Rx_i[3][3] = {
                {1, 0, 0},
                {0, ci, -si},
                {0, si, ci}};
            // Rz(Ω)
            double Rz_Omega[3][3] = {
                {cO, -sO, 0},
                {sO, cO, 0},
                {0, 0, 1}};

            // Apply rotations: ECI = Rz(Ω) * Rx(i) * Rz(ω) * pqw
            double tmp1[3], tmp2[3], eci[3];
            mat3x3_vec3_mult(Rz_omega, pqw, tmp1);
            mat3x3_vec3_mult(Rx_i, tmp1, tmp2);
            mat3x3_vec3_mult(Rz_Omega, tmp2, eci);

            // Save
            sat_eci_positions[prn].x[k] = eci[0];
            sat_eci_positions[prn].y[k] = eci[1];
            sat_eci_positions[prn].z[k] = eci[2];

            // printf("PRN %d, Epoch %d: ECI = [%.3f, %.3f, %.3f]\n", prn, k, sat_eci_positions[prn].x[k]/1000, sat_eci_positions[prn].y[k]/1000, sat_eci_positions[prn].z[k]/1000);
        }
    }
    return 0;
}
