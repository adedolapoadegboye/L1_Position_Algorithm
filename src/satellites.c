/**
 * @file satellites.c
 * @brief Functions for managing satellite data and pseudorange history.
 * This file contains functions to handle satellite data, including sorting satellites data saved from the file parser.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/receiver.h"

gps_satellite_data_t gps_list[MAX_SAT + 1];
sat_eci_history_t sat_eci_positions[MAX_SAT + 1];
sat_ecef_history_t sat_ecef_positions[MAX_SAT + 1];
sat_orbit_pqw_history_t sat_orbit_pqw_positions[MAX_SAT + 1];
sat_orbit_eci_history_t sat_orbit_eci_positions[MAX_SAT + 1];

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

/* Populate gps_list[prn].{eccentricities,...,times_of_ephemeris} with the
   FULL ephemeris history (unique by TOE), independent of pseudorange epochs. */
static void populate_ephemeris_series_from_history(int prn, const eph_history_t *hist)
{
    // Reset ephemeris arrays
    for (int k = 0; k < MAX_EPOCHS; k++)
    {
        gps_list[prn].eccentricities[k] = 0.0;
        gps_list[prn].inclinations[k] = 0.0;
        gps_list[prn].mean_anomalies[k] = 0.0;
        gps_list[prn].semi_major_axes[k] = 0.0;
        gps_list[prn].right_ascension_of_ascending_node[k] = 0.0;
        gps_list[prn].argument_of_periapsis[k] = 0.0;
        gps_list[prn].times_of_ephemeris[k] = 0;
    }

    size_t eidx = 0;
    uint32_t last_toe = UINT32_MAX; // ensures first entry is accepted

    for (size_t i = 0; i < hist->count && eidx < MAX_EPOCHS; i++)
    {
        const rtcm_1019_ephemeris_t *eph = &hist->eph[i];
        uint32_t toe = eph->gps_toe;

        // Append only when TOE changes (unique-by-TOE), mirroring Python's unique list
        if (i == 0 || toe != last_toe)
        {
            gps_list[prn].eccentricities[eidx] = eph->eccentricity;
            gps_list[prn].inclinations[eidx] = eph->inclination;
            gps_list[prn].mean_anomalies[eidx] = eph->mean_anomaly;
            gps_list[prn].semi_major_axes[eidx] = eph->semi_major_axis;
            gps_list[prn].right_ascension_of_ascending_node[eidx] = eph->right_ascension_of_ascending_node;
            gps_list[prn].argument_of_periapsis[eidx] = eph->argument_of_periapsis;
            gps_list[prn].times_of_ephemeris[eidx] = toe;

            last_toe = toe;
            eidx++;
        }
    }
}

int sort_satellites(eph_history_t *eph_history_table,
                    rtcm_1074_msm4_t msm4_history_table[MAX_SAT][MAX_EPOCHS],
                    rtcm_1002_msm1_t msm1_history_table[MAX_SAT][MAX_EPOCHS])
{
    memset(gps_list, 0, sizeof(gps_list));

    if (observation_type == 1)
    {
        for (int prn = 1; prn <= MAX_SAT; prn++)
        {
            gps_list[prn].prn = prn;

            size_t msm1_len = msm1_count[prn];
            for (size_t i = 0; i < msm1_len && i < MAX_EPOCHS; i++)
            {
                // Find the index of this PRN in the MSM1 message (could be at any position)
                int found = 0;
                for (int j = 0; j < msm1_history_table[prn][i].num_satellites; j++)
                {
                    if (msm1_history_table[prn][i].svs[j] == prn)
                    {
                        gps_list[prn].pseudoranges[i] = msm1_history_table[prn][i].pseudoranges[j];
                        gps_list[prn].times_of_pseudorange[i] = msm1_history_table[prn][i].time_of_week;
                        found = 1;

                        // Find the latest eph for this pseudorange time
                        int eph_idx = find_closest_eph_idx(&eph_history[prn],
                                                           msm1_history_table[prn][i].time_of_week);
                        if (eph_idx >= 0)
                        {
                            const rtcm_1019_ephemeris_t *eph = &eph_history_table[prn].eph[eph_idx];
                            gps_list[prn].eccentricities[i] = eph->eccentricity;
                            gps_list[prn].inclinations[i] = eph->inclination;
                            gps_list[prn].mean_anomalies[i] = eph->mean_anomaly;
                            gps_list[prn].semi_major_axes[i] = eph->semi_major_axis;
                            gps_list[prn].right_ascension_of_ascending_node[i] = eph->right_ascension_of_ascending_node;
                            gps_list[prn].argument_of_periapsis[i] = eph->argument_of_periapsis;
                            gps_list[prn].times_of_ephemeris[i] = eph->gps_toe; // store TOE
                        }
                        else
                        {
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
    }
    else if (observation_type == 4)
    {
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
                        int eph_idx = find_closest_eph_idx(&eph_history[prn],
                                                           msm4_history_table[prn][i].time_of_pseudorange);
                        if (eph_idx >= 0)
                        {
                            const rtcm_1019_ephemeris_t *eph = &eph_history_table[prn].eph[eph_idx];
                            gps_list[prn].eccentricities[i] = eph->eccentricity;
                            gps_list[prn].inclinations[i] = eph->inclination;
                            gps_list[prn].mean_anomalies[i] = eph->mean_anomaly;
                            gps_list[prn].semi_major_axes[i] = eph->semi_major_axis;
                            gps_list[prn].right_ascension_of_ascending_node[i] = eph->right_ascension_of_ascending_node;
                            gps_list[prn].argument_of_periapsis[i] = eph->argument_of_periapsis;
                            gps_list[prn].times_of_ephemeris[i] = eph->gps_toe; // store TOE
                        }
                        else
                        {
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
    }
    else
    {
        fprintf(stderr, COLOR_RED "Error: Unsupported observation type %d for sorting satellites.\n" COLOR_RESET, observation_type);
        return 1;
    }

    // ---- Ephemeris history (independent of pseudoranges) — mirrors Python behavior ----
    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        populate_ephemeris_series_from_history(prn, &eph_history[prn]);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Prints the GPS satellite data summary.
 * This function prints the pseudorange table (rows with valid PR)
 * and then prints a Python-style ephemeris series that lists all unique TOE entries.
 */
void print_gps_list(void)
{
    printf("============================================\n");
    printf("         GPS Satellite Data Summary         \n");
    printf("============================================\n");

    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        // Any pseudorange?
        int any_pr = 0;
        for (int i = 0; i < MAX_EPOCHS; i++)
            if (gps_list[prn].pseudoranges[i] != 0)
            {
                any_pr = 1;
                break;
            }
        // Any ephemeris TOE?
        int any_eph = 0;
        for (int i = 0; i < MAX_EPOCHS; i++)
            if (gps_list[prn].times_of_ephemeris[i] != 0)
            {
                any_eph = 1;
                break;
            }

        if (!any_pr && !any_eph)
            continue;

        printf("\nPRN %d:\n", prn);

        // Pseudorange table (like before)
        if (any_pr)
        {
            printf("Idx | Pseudorange      | Time_of_PR    | Eccentricity   | Inclination    | Mean_Anomaly   | SemiMajorAxis   | RAAN           | ArgPeriapsis    | TOE\n");
            printf("----+------------------+---------------+----------------+----------------+----------------+----------------+----------------+----------------+------------\n");
            for (int i = 0; i < MAX_EPOCHS; i++)
            {
                if (gps_list[prn].pseudoranges[i] == 0)
                    continue;

                printf("%3d | %16.6f | %11u | %14.8g | %14.8g | %14.8g | %14.8g | %14.8g | %14.8g | %10u\n",
                       i,
                       gps_list[prn].pseudoranges[i],
                       gps_list[prn].times_of_pseudorange[i],
                       gps_list[prn].eccentricities[i],
                       gps_list[prn].inclinations[i],
                       gps_list[prn].mean_anomalies[i],
                       gps_list[prn].semi_major_axes[i],
                       gps_list[prn].right_ascension_of_ascending_node[i],
                       gps_list[prn].argument_of_periapsis[i],
                       (unsigned)gps_list[prn].times_of_ephemeris[i]);
            }
        }

        // Python-style ephemeris series (unique TOE list)
        if (any_eph)
        {
            // Eccentricities
            printf("\n  Eccentricities             : [");
            int first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%.16g", first ? "" : ", ", gps_list[prn].eccentricities[i]);
                first = 0;
            }
            printf("]\n");

            // Inclinations
            printf("  Inclinations               : [");
            first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%.16g", first ? "" : ", ", gps_list[prn].inclinations[i]);
                first = 0;
            }
            printf("]\n");

            // Mean Anomalies
            printf("  Mean Anomalies             : [");
            first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%.16g", first ? "" : ", ", gps_list[prn].mean_anomalies[i]);
                first = 0;
            }
            printf("]\n");

            // Semi-Major Axes
            printf("  Semi-Major Axes            : [");
            first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%.16g", first ? "" : ", ", gps_list[prn].semi_major_axes[i]);
                first = 0;
            }
            printf("]\n");

            // RA of Ascending Node
            printf("  RA of Ascending Node       : [");
            first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%.16g", first ? "" : ", ", gps_list[prn].right_ascension_of_ascending_node[i]);
                first = 0;
            }
            printf("]\n");

            // Argument of Periapsis
            printf("  Argument of Periapsis      : [");
            first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%.16g", first ? "" : ", ", gps_list[prn].argument_of_periapsis[i]);
                first = 0;
            }
            printf("]\n");

            // Times of Ephemeris
            printf("  Times of Ephemeris         : [");
            first = 1;
            for (int i = 0; i < MAX_EPOCHS && gps_list[prn].times_of_ephemeris[i] != 0; i++)
            {
                printf("%s%u", first ? "" : ", ", (unsigned)gps_list[prn].times_of_ephemeris[i]);
                first = 0;
            }
            printf("]\n");
        }
    }

    printf("\n============================================\n");
}
