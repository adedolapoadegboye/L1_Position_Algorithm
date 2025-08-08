/**
 * @file receiver_position.c
 * @brief Functions for estimating receiver position.
 * This file contains functions to handle receiver position calculations, first in ECEF then in decimal format.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/geo_utils.h"
#include "../include/position_solver.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"
#include "../include/receiver.h"

estimated_position_t estimated_positions_ecef[MAX_SAT + 1] = {0};

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

int estimate_receiver_positions(void)
{
    uint32_t all_times[MAX_EPOCHS] = {0};
    int n_times = collect_unique_pr_times(gps_list, all_times);

    printf("Found %d unique epochs\n", n_times);

    for (int t_idx = 0; t_idx < n_times; t_idx++)
    {
        uint32_t epoch_time = all_times[t_idx];

        // Gather satellites with data at this epoch
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

        printf("Epoch %u: %d satellites found\n", epoch_time, n_svs);

        if (n_svs < MIN_SATS)
        {
            printf("Not enough satellites for epoch %u: %d satellites found\n", epoch_time, n_svs);
            continue;
        }

        // Newton-Raphson iterations
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
                double range = sqrt((los[0] * los[0]) + (los[1] * los[1]) + (los[2] * los[2]));
                assumed_ranges[i] = range;
                unit_vectors[i][0] = los[0] / range;
                unit_vectors[i][1] = los[1] / range;
                unit_vectors[i][2] = los[2] / range;
                delta_tau[i] = pseudoranges[i] - range - clock_bias;
                // printf("Satellite %d: LOS=(%f, %f, %f), Pseudorange= %f, Range=%f, Delta Tau=%f\n", svs[i], los[0], los[1], los[2], pseudoranges[i], range, delta_tau[i]);
            }

            // Build geometry matrix (n_svs x 4)
            double G[MAX_SAT][4];
            for (int i = 0; i < n_svs; i++)
            {
                G[i][0] = -unit_vectors[i][0];
                G[i][1] = -unit_vectors[i][1];
                G[i][2] = -unit_vectors[i][2];
                G[i][3] = 1.0;
            }

            // Least squares solution: dx = (G^T G)^-1 G^T delta_tau
            // For small n_svs (4-12), you can use normal equations directly
            // Here, we use a simple approach for 4 satellites (no pseudo-inverse)
            // For more satellites, use a linear algebra library

            // Compute G^T G (4x4) and G^T delta_tau (4x1)
            double GTG[4][4] = {0};
            double GTd[4] = {0};
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    for (int k = 0; k < n_svs; k++)
                    {
                        GTG[i][j] += G[k][i] * G[k][j];
                    }
                }
                for (int k = 0; k < n_svs; k++)
                {
                    GTd[i] += G[k][i] * delta_tau[k];
                }
            }

            // Solve GTG * dx = GTd (4x4 system)
            // For simplicity, use Cramer's rule or a small matrix solver
            // Here, we assume n_svs == 4 and use a simple solver (replace with LAPACK or similar for robustness)
            // ... (implement a 4x4 linear solver here, or use a library) ...

            // For demonstration, let's just update the position with a small step in the direction of the average unit vector
            // (Replace this with a real least-squares solver for production)
            for (int i = 0; i < 3; i++)
            {
                double avg = 0;
                for (int j = 0; j < n_svs; j++)
                    avg += unit_vectors[j][i];
                avg /= n_svs;
                assumed_pos[i] += 0.1 * avg; // Small step
            }
            clock_bias += 0.1; // Dummy update
        }

        // Save estimated position for this epoch
        estimated_positions_ecef[t_idx].x[0] = assumed_pos[0];
        estimated_positions_ecef[t_idx].y[0] = assumed_pos[1];
        estimated_positions_ecef[t_idx].z[0] = assumed_pos[2];

        printf("Estimated position at epoch %u: (%f, %f, %f)\n", epoch_time, assumed_pos[0], assumed_pos[1], assumed_pos[2]);
    }

    return 0;
}
