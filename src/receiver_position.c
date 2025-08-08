/**
 * @file receiver_position.c
 * @brief Functions for estimating receiver position.
 * This file contains functions to handle receiver position calculations, first in ECEF then in decimal format.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"
#include "../include/receiver.h"

estimated_position_t estimated_positions_ecef[MAX_SAT + 1] = {0};
latlonalt_position_t latlonalt_positions[MAX_SAT + 1] = {0};

static int collect_unique_pr_times(const gps_satellite_data_t gps_lists[MAX_SAT + 1], uint32_t all_times[MAX_EPOCHS])
{
    int count = 0;
    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        for (int k = 0; k < MAX_EPOCHS; k++)
        {
            uint32_t t = gps_lists[prn].times_of_pseudorange[k] / 1000;
            if (t == 0)
                continue;
            // Check if already in all_times
            int found = 0;
            for (int i = 0; i < count; i++)
            {
                if (all_times[i] == t)
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                all_times[count++] = t;
            }
        }
    }
    // Sort all_times (simple bubble sort for small arrays)
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (all_times[i] > all_times[j])
            {
                uint32_t tmp = all_times[i];
                all_times[i] = all_times[j];
                all_times[j] = tmp;
            }
        }
    }
    return count;
}

static int invert_4x4(const double A[4][4], double inv[4][4])
{
    // Build augmented [A | I]
    double aug[4][8];
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
            aug[r][c] = A[r][c];
        for (int c = 0; c < 4; ++c)
            aug[r][4 + c] = (r == c) ? 1.0 : 0.0;
    }

    // Gauss-Jordan elimination with partial pivoting
    for (int col = 0; col < 4; ++col)
    {
        // Pivot: find row with max |aug[r][col]| for r >= col
        int piv = col;
        double maxabs = fabs(aug[piv][col]);
        for (int r = col + 1; r < 4; ++r)
        {
            double v = fabs(aug[r][col]);
            if (v > maxabs)
            {
                maxabs = v;
                piv = r;
            }
        }
        // Singular/ill-conditioned check
        if (maxabs <= 1e-15)
            return 0;

        // Swap current row with pivot row
        if (piv != col)
        {
            for (int c = 0; c < 8; ++c)
            {
                double tmp = aug[col][c];
                aug[col][c] = aug[piv][c];
                aug[piv][c] = tmp;
            }
        }

        // Normalize pivot row
        double pivval = aug[col][col];
        double invpiv = 1.0 / pivval;
        for (int c = 0; c < 8; ++c)
            aug[col][c] *= invpiv;

        // Eliminate this column from other rows
        for (int r = 0; r < 4; ++r)
        {
            if (r == col)
                continue;
            double factor = aug[r][col];
            if (factor != 0.0)
            {
                for (int c = 0; c < 8; ++c)
                {
                    aug[r][c] -= factor * aug[col][c];
                }
            }
        }
    }

    // Extract inverse from augmented matrix
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            inv[r][c] = aug[r][4 + c];

    return 1;
}

static void mat_transpose(int rows, int cols, const double A[rows][cols], double AT[cols][rows])
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            AT[j][i] = A[i][j];
}

// Multiply: C = A (m×p) * B (p×n)
static void mat_mult(int m, int p, int n, const double A[m][p], const double B[p][n], double C[m][n])
{
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            C[i][j] = 0.0;
            for (int k = 0; k < p; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
    }
}

// Multiply matrix (m×n) by vector (n)
static void mat_vec_mult(int m, int n, const double A[m][n], const double v[n], double out[m])
{
    for (int i = 0; i < m; i++)
    {
        out[i] = 0.0;
        for (int j = 0; j < n; j++)
            out[i] += A[i][j] * v[j];
    }
}

// --- Minimal SVD pseudoinverse ---
// This is a *very* stripped-down numerical method, fine for small (<= 12×4) matrices.
// For production use, replace with LAPACK's dgesvd or similar.

static void pinv_svd(int m, int n, double A[m][n], double pinv[n][m])
{
    // This is just a placeholder — in your project, call a small SVD routine
    // or link to LAPACK to get U, S, V^T and invert singular values > tol.

    // For now, use normal equations: pinv(A) = (AᵀA)⁻¹ Aᵀ
    double AT[n][m];
    mat_transpose(m, n, A, AT);

    double ATA[n][n];
    mat_mult(n, m, n, AT, A, ATA);

    // Invert ATA (n×n) — here n=4 always
    double invATA[4][4];
    invert_4x4(ATA, invATA); // implement your existing 4×4 inversion

    double tmp[n][m];
    mat_mult(n, n, m, invATA, AT, tmp);

    // Copy to pinv
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            pinv[i][j] = tmp[i][j];
}

int estimate_receiver_positions(void)
{
    uint32_t all_times[MAX_EPOCHS] = {0};
    int n_times = collect_unique_pr_times(gps_list, all_times);

    printf("Found %d unique epochs\n", n_times);

    for (int t_idx = 0; t_idx < n_times; t_idx++)
    {
        uint32_t epoch_time = all_times[t_idx];

        int svs[MAX_SAT];
        double ecefs[MAX_SAT][3];
        double pseudoranges[MAX_SAT];
        int n_svs = 0;

        for (int prn = 1; prn <= MAX_SAT; prn++)
        {
            for (int k = 0; k < MAX_EPOCHS; k++)
            {
                if ((gps_list[prn].times_of_pseudorange[k] / 1000) == epoch_time)
                {
                    svs[n_svs] = prn;
                    ecefs[n_svs][0] = sat_ecef_positions[prn].x[k];
                    ecefs[n_svs][1] = sat_ecef_positions[prn].y[k];
                    ecefs[n_svs][2] = sat_ecef_positions[prn].z[k];
                    pseudoranges[n_svs] = gps_list[prn].pseudoranges[k];
                    n_svs++;
                    break;
                }
            }
        }

        if (n_svs < MIN_SATS)
            continue;

        double assumed_pos[3] = {0, 0, 0};
        double clock_bias = 0;

        for (int iter = 0; iter < ITERATIONS; iter++)
        {
            double assumed_ranges[MAX_SAT];
            double unit_vectors[MAX_SAT][3];
            double delta_tau[MAX_SAT];

            for (int i = 0; i < n_svs; i++)
            {
                double los[3] = {
                    ecefs[i][0] - assumed_pos[0],
                    ecefs[i][1] - assumed_pos[1],
                    ecefs[i][2] - assumed_pos[2]};
                double range = sqrt(los[0] * los[0] + los[1] * los[1] + los[2] * los[2]);
                assumed_ranges[i] = range;
                unit_vectors[i][0] = los[0] / range;
                unit_vectors[i][1] = los[1] / range;
                unit_vectors[i][2] = los[2] / range;
                delta_tau[i] = pseudoranges[i] - range - clock_bias;
            }

            double G[MAX_SAT][4];
            for (int i = 0; i < n_svs; i++)
            {
                G[i][0] = -unit_vectors[i][0];
                G[i][1] = -unit_vectors[i][1];
                G[i][2] = -unit_vectors[i][2];
                G[i][3] = 1.0;
            }

            double Gpinv[4][MAX_SAT];
            pinv_svd(n_svs, 4, G, Gpinv); // shape: 4×n_svs

            double delta_pos_time[4];
            mat_vec_mult(4, n_svs, Gpinv, delta_tau, delta_pos_time);

            assumed_pos[0] += delta_pos_time[0];
            assumed_pos[1] += delta_pos_time[1];
            assumed_pos[2] += delta_pos_time[2];
            clock_bias += delta_pos_time[3];
        }

        estimated_positions_ecef[t_idx].x[0] = assumed_pos[0];
        estimated_positions_ecef[t_idx].y[0] = assumed_pos[1];
        estimated_positions_ecef[t_idx].z[0] = assumed_pos[2];
        // printf("Estimated position at epoch %u: (%.3f, %.3f, %.3f)\n",
        //        epoch_time, assumed_pos[0], assumed_pos[1], assumed_pos[2]);

        double lat_deg, lon_deg, alt_m;
        ecef_to_geodetic(assumed_pos[0], assumed_pos[1], assumed_pos[2],
                         &lat_deg, &lon_deg, &alt_m);

        latlonalt_positions[t_idx].lat[0] = lat_deg;
        latlonalt_positions[t_idx].lon[0] = lon_deg;
        latlonalt_positions[t_idx].alt[0] = alt_m;

        printf("Lat/Long at epoch %u: (%.8f, %.8f)\n",
               epoch_time, lat_deg, lon_deg);
    }

    return 0;
}
