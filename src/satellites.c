/**
 * @file satellites.c
 * @brief Functions for managing satellite data and pseudorange history.
 * This file contains functions to handle satellite data, including sorting satellites data saved from the file parser.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/geo_utils.h"
#include "../include/position_solver.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"

gps_satellite_data_t gps_list[MAX_SAT + 1];
sat_eci_history_t sat_eci_positions[MAX_SAT + 1];
sat_ecef_history_t sat_ecef_positions[MAX_SAT + 1];
sat_orbit_pqw_history_t sat_orbit_pqw_positions[MAX_SAT + 1];
sat_orbit_eci_history_t sat_orbit_eci_positions[MAX_SAT + 1];

// Helper: Find the closest eph index for prn whose EPH time_of_week is just before or equal to pseudorange_time
static int find_closest_eph_idx(const eph_history_t *hist, uint32_t pseudorange_time)
{
    int best_idx = -1;
    uint32_t best_time = 0;
    for (size_t i = 0; i < hist->count; i++)
    {
        uint32_t toe = hist->eph[i].time_of_week;
        if (toe <= pseudorange_time && toe >= best_time)
        {
            best_time = toe;
            best_idx = (int)i;
        }
    }
    return best_idx;
}

int sort_satellites(eph_history_t *eph_history_table, rtcm_1074_msm4_t msm4_history_table[MAX_SAT][MAX_EPOCHS])
{
    memset(gps_list, 0, sizeof(gps_list));

    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        gps_list[prn].prn = prn;

        size_t msm4_len = msm4_count[prn];
        for (size_t i = 0; i < msm4_len && i < MAX_EPOCHS; i++)
        {
            // Find the index of this PRN in the MSM4 message (could be at any position)
            int found = 0;
            for (int j = 0; j < msm4_history_table[prn][i].n_sat; j++)
            {
                if (msm4_history_table[prn][i].prn[j] == prn)
                {
                    gps_list[prn].pseudoranges[i] = msm4_history_table[prn][i].pseudorange[j];
                    gps_list[prn].times_of_pseudorange[i] = msm4_history_table[prn][i].time_of_pseudorange;
                    found = 1;

                    // Find the latest eph for this pseudorange time
                    int eph_idx = find_closest_eph_idx(&eph_history[prn], msm4_history_table[prn][i].time_of_pseudorange);
                    if (eph_idx >= 0)
                    {
                        const rtcm_1019_ephemeris_t *eph = &eph_history_table[prn].eph[eph_idx];
                        gps_list[prn].eccentricities[i] = eph->eccentricity;
                        gps_list[prn].inclinations[i] = eph->inclination;
                        gps_list[prn].mean_anomalies[i] = eph->mean_anomaly;
                        gps_list[prn].semi_major_axes[i] = eph->semi_major_axis;
                        gps_list[prn].right_ascension_of_ascending_node[i] = eph->right_ascension_of_ascending_node;
                        gps_list[prn].argument_of_periapsis[i] = eph->argument_of_periapsis;
                        gps_list[prn].times_of_ephemeris[i] = eph->time_of_week;
                    }
                    else
                    {
                        // No valid eph found for this time
                        gps_list[prn].eccentricities[i] = 0;
                        gps_list[prn].inclinations[i] = 0;
                        gps_list[prn].mean_anomalies[i] = 0;
                        gps_list[prn].semi_major_axes[i] = 0;
                        gps_list[prn].right_ascension_of_ascending_node[i] = 0;
                        gps_list[prn].argument_of_periapsis[i] = 0;
                        gps_list[prn].times_of_ephemeris[i] = 0;
                    }
                    break;
                }
            }
            if (!found)
            {
                gps_list[prn].pseudoranges[i] = 0;
                gps_list[prn].times_of_pseudorange[i] = 0;
                gps_list[prn].eccentricities[i] = 0;
                gps_list[prn].inclinations[i] = 0;
                gps_list[prn].mean_anomalies[i] = 0;
                gps_list[prn].semi_major_axes[i] = 0;
                gps_list[prn].right_ascension_of_ascending_node[i] = 0;
                gps_list[prn].argument_of_periapsis[i] = 0;
                gps_list[prn].times_of_ephemeris[i] = 0;
            }
        }
    }
    return 0;
}
/**
 * @brief Prints the GPS satellite data summary.
 * This function iterates through the global gps_list and prints the details for each satellite that has valid ephemeris data.
 */

void print_gps_list(void)
{
    printf("============================================\n");
    printf("         GPS Satellite Data Summary         \n");
    printf("============================================\n");

    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        // Find the number of valid pseudorange entries for this PRN
        int count = 0;
        for (int i = 0; i < MAX_EPOCHS; i++)
        {
            if (gps_list[prn].pseudoranges[i] != 0)
                count++;
        }
        if (count == 0)
            continue;

        printf("\nPRN %d: %d epochs\n", prn, count);
        printf("Idx | Pseudorange      | Time_of_PR    | Eccentricity   | Inclination    | Mean_Anomaly   | SemiMajorAxis   | RAAN           | ArgPeriapsis    | TOE\n");
        printf("----+------------------+---------------+----------------+----------------+----------------+----------------+----------------+----------------+------------\n");
        for (int i = 0; i < count; i++)
        {
            printf("%3d | %16.6f | %11u | %14.8g | %14.8g | %14.8g | %14.8g | %14.8g | %14.8g | %10.0f\n",
                   i,
                   gps_list[prn].pseudoranges[i],
                   gps_list[prn].times_of_pseudorange[i],
                   gps_list[prn].eccentricities[i],
                   gps_list[prn].inclinations[i],
                   gps_list[prn].mean_anomalies[i],
                   gps_list[prn].semi_major_axes[i],
                   gps_list[prn].right_ascension_of_ascending_node[i],
                   gps_list[prn].argument_of_periapsis[i],
                   gps_list[prn].times_of_ephemeris[i]);
        }
    }
    printf("\n============================================\n");
}
