/**
 * @file df_parser.c
 * @brief Parses RTCM 1019 and 1074 messages from text-formatted input lines.
 *
 * This module provides parsing logic for the following specific RTCM message types:
 * - RTCM 1019: Broadcast ephemeris messages (used for GPS satellite position)
 * - RTCM 1002: Legacy observation messages (used for GPS L1 pseudorange and phase)
 * - RTCM 1074: MSM4 observation messages (used for GPS L1 pseudorange and phase)
 *
 * The input is assumed to be text-format (e.g., exported from parsed binary logs),
 * and each line contains a full RTCM message with labeled fields (DFxxx).
 *
 * Output is stored in well-defined structures for further positioning analysis.
 */

#include "../include/algo.h"
#include "../include/df_parser.h"

//////////////////////////////////////////////////////////////////////////////////////////////
bool eph_available[MAX_SAT + 1] = {0};
size_t msm4_count[MAX_SAT + 1] = {0};
size_t msm1_count[MAX_SAT + 1] = {0};
double pseudorange_history[MAX_SAT + 1][MAX_EPOCHS] = {{0}};
size_t pseudorange_count[MAX_SAT + 1] = {0};
rtcm_1002_msm1_t msm1_history[MAX_SAT + 1][MAX_EPOCHS] = {{{0}}};
rtcm_1074_msm4_t msm4_history[MAX_SAT + 1][MAX_EPOCHS] = {{{0}}};
rtcm_1019_ephemeris_t eph_table[MAX_SAT + 1] = {{0}};
uint8_t observation_type = 0;
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Parses a single RTCM 1002 MSM1 line into a structured observation object.
 *
 * This function extracts the MSM1 header and all per-satellite fields
 * (PRN, signal ID, pseudorange remainder, phase range difference, lock time,
 * ambiguity, and CNR) from the provided text-formatted RTCM message.
 *
 * @param line Input string containing the RTCM 1002 MSM1 message.
 * @param msm1 Pointer to the MSM1 observation structure to populate.
 * @return 0 on success, non-zero on failure.
 */
int parse_rtcm_1002(const char *line, rtcm_1002_msm1_t *msm1)
{
    if (!line || !msm1)
        return -1;

    EXTRACT("DF002", "%hu", &msm1->msg_type);
    EXTRACT("DF003", "%hu", &msm1->station_id);
    EXTRACT("DF004", "%u", &msm1->time_of_week);
    msm1->time_of_week = msm1->time_of_week;
    EXTRACT("DF005", "%hhu", &msm1->sync_gps_message_flag);
    EXTRACT("DF006", "%hhu", &msm1->num_satellites);
    EXTRACT("DF007", "%hhu", &msm1->smooth_interval_flag);
    EXTRACT("DF008", "%hhu", &msm1->smooth_interval);

    // Step X: Extract DF009_xx .. DF015_xx using strstr + sscanf
    for (int i = 0; i < msm1->num_satellites; i++)
    {
        char key[16];
        const char *ptr;

        // DF009_xx: Satellite PRN
        sprintf(key, "DF009_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm1->svs[i]);

        // DF010_xx: Signal ID
        sprintf(key, "DF010_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm1->sig_id[i]);

        // DF011_xx: Pseudorange remainder (m)
        sprintf(key, "DF011_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%lf", &msm1->remainders[i]);

        // DF012_xx: Carrier phase range minus pseudorange (m)
        sprintf(key, "DF012_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%lf", &msm1->phase_pr_diff[i]);

        // DF013_xx: Lock time indicator
        sprintf(key, "DF013_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm1->lock_time[i]);

        // DF014_xx: Pseudorange modulus ambiguity
        sprintf(key, "DF014_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm1->ambiguities[i]);

        // DF015_xx: Carrier-to-noise ratio (dB-Hz)
        sprintf(key, "DF015_%02d", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm1->cnr[i]);

        // Calculate full pseudorange
        msm1->pseudoranges[i] = compute_pseudorange_msm1(msm1->ambiguities[i], msm1->remainders[i]);
    }
    // print_msm1(msm1); // quick debug print
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Parses a single RTCM 1074 MSM4 line into a structured observation object.
 *
 * This function extracts the MSM4 header and all per-cell data (pseudorange,
 * carrier phase, lock time, CNR, etc.) for each satellite-signal combination.
 *
 * @param line Input string with the RTCM message content.
 * @param msm4 Pointer to output structure to populate.
 * @return 0 on success, non-zero on failure.
 */
int parse_rtcm_1074(const char *line, rtcm_1074_msm4_t *msm4)
{
    if (!line || !msm4)
        return -1;

    // Step 1: Extract MSM4 header fields using the EXTRACT macro
    EXTRACT("DF002", "%hu", &msm4->msg_type);
    EXTRACT("DF003", "%hu", &msm4->station_id);
    EXTRACT("DF004", "%u", &msm4->gps_epoch_time);
    msm4->time_of_pseudorange = msm4->gps_epoch_time;
    EXTRACT("DF393", "%hhu", &msm4->msm_sync_flag);
    EXTRACT("DF409", "%hhu", &msm4->iods_reserved);
    EXTRACT("DF001_7", "%hhu", &msm4->reserved_DF001_07);
    EXTRACT("DF411", "%hhu", &msm4->clk_steering_flag);
    EXTRACT("DF412", "%hhu", &msm4->external_clk_flag);

    // Step 2: Extract counts
    EXTRACT("NSat", "%hhu", &msm4->n_sat);
    EXTRACT("NSig", "%hhu", &msm4->n_sig);
    EXTRACT("NCell", "%hhu", &msm4->n_cell);

    // Step 3: Extract satellite PRNs (PRN_01, PRN_02, ...)
    for (int i = 0; i < msm4->n_sat; i++)
    {
        char key[16];
        sprintf(key, "PRN_%02d", i + 1);
        const char *ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm4->prn[i]);
    }

    // Step 4: Extract Pseudorange values
    for (int i = 0; i < msm4->n_sat; i++)
    {
        char key[16];
        sprintf(key, "DF397_%02d", i + 1);
        const char *ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%hhu", &msm4->pseudorange_integer[i]);
    }

    for (int i = 0; i < msm4->n_sat; i++)
    {
        char key[16];
        sprintf(key, "DF398_%02d", i + 1);
        const char *ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key) + 1, "%lf", &msm4->pseudorange_mod_1s[i]);
    }

    // Step 5: Extract Cell PRN and Signal ID mappings (CELLPRN_xx, CELLSIG_xx). Note: Store only L1 (1C) cells
    uint8_t l1_cell_index = 0;

    for (int i = 0; i < msm4->n_cell; i++)
    {
        char prn_key[24], sig_key[24], sig_val[8];
        const char *ptr;

        uint8_t prn_val = 0;

        // Get CELLPRN
        sprintf(prn_key, "CELLPRN_%02d", i + 1);
        ptr = strstr(line, prn_key);
        if (ptr)
            sscanf(ptr + strlen(prn_key) + 1, "%hhu", &prn_val);

        // Get CELLSIG
        sprintf(sig_key, "CELLSIG_%02d", i + 1);
        ptr = strstr(line, sig_key);
        if (ptr)
        {
            sscanf(ptr + strlen(sig_key) + 1, "%2s", sig_val);

            if (strcmp(sig_val, "1C") == 0)
            {
                // Keep only if signal is 1C
                msm4->cell_prn[l1_cell_index] = prn_val;
                msm4->cell_sig[l1_cell_index] = 1; // 1 = 1C in your system
                l1_cell_index++;
            }
        }
    }

    // Step 6: Extract DF400 (pseudorange), DF401 (carrier phase), DF402 (lock), DF403 (CNR)
    l1_cell_index = 0;

    for (int i = 0; i < msm4->n_cell; i++)
    {
        char prn_key[24], sig_key[24], sig_val[8];
        const char *ptr;
        uint8_t prn_val = 0;

        int cell_number = i + 1;

        // CELLPRN_XX
        sprintf(prn_key, "CELLPRN_%02d", cell_number);
        ptr = strstr(line, prn_key);
        if (ptr)
            sscanf(ptr + strlen(prn_key) + 1, "%hhu", &prn_val);

        // CELLSIG_XX
        sprintf(sig_key, "CELLSIG_%02d", cell_number);
        ptr = strstr(line, sig_key);
        if (ptr)
        {
            sscanf(ptr + strlen(sig_key) + 1, "%2s", sig_val);

            if (strcmp(sig_val, "1C") == 0)
            {
                // Store PRN and signal
                msm4->cell_prn[l1_cell_index] = prn_val;
                msm4->cell_sig[l1_cell_index] = 1; // L1C

                // Extract corresponding DF400_XX to DF403_XX
                char key[32];

                sprintf(key, "DF400_%02d", cell_number);
                ptr = strstr(line, key);
                if (ptr)
                    sscanf(ptr + strlen(key) + 1, "%lf", &msm4->pseudorange_fine[l1_cell_index]);

                sprintf(key, "DF401_%02d", cell_number);
                ptr = strstr(line, key);
                if (ptr)
                    sscanf(ptr + strlen(key) + 1, "%lf", &msm4->phase_range[l1_cell_index]);

                sprintf(key, "DF402_%02d", cell_number);
                ptr = strstr(line, key);
                if (ptr)
                    sscanf(ptr + strlen(key) + 1, "%hhu", &msm4->lock_time[l1_cell_index]);

                sprintf(key, "DF403_%02d", cell_number);
                ptr = strstr(line, key);
                if (ptr)
                    sscanf(ptr + strlen(key) + 1, "%hhu", &msm4->cnr[l1_cell_index]);

                l1_cell_index++;
            }
        }
    }

    // Finalize the number of cells to the actual count of L1C cells
    msm4->n_cell = l1_cell_index;

    // Step 7: Calculate pseudorange array
    for (int i = 0; i < msm4->n_cell; i++)
    {
        // Check if the cell is valid (has a PRN and signal)
        if (msm4->cell_prn[i] > 0 && msm4->cell_sig[i] == 1) // Only L1C cells
        {
            msm4->pseudorange[i] = compute_pseudorange(
                msm4->pseudorange_integer[i],
                msm4->pseudorange_mod_1s[i],
                msm4->pseudorange_fine[i]);
        }
        else
        {
            msm4->pseudorange[i] = -1.0; // mark as invalid
        }
    }

    // print_msm4(msm4); // quick debug print

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Parses a single RTCM 1019 line into a structured ephemeris object.
 *
 * This function extracts all defined DF fields from a line containing a text-formatted
 * RTCM 1019 message and fills the provided structure accordingly.
 *
 * @param line Input string with the RTCM message content.
 * @param eph Pointer to output structure to populate.
 * @return 0 on success, non-zero on failure.
 */
int parse_rtcm_1019(const char *line, rtcm_1019_ephemeris_t *eph)
{
    if (!line || !eph)
        return -1;

    EXTRACT("DF002", "%hu", &eph->msg_type);
    EXTRACT("DF009", "%hhu", &eph->satellite_id);
    eph->sv = eph->satellite_id;
    EXTRACT("DF076", "%hu", &eph->gps_wn);
    eph->week_number = eph->gps_wn;
    EXTRACT("DF077", "%hhu", &eph->gps_sv_acc);
    EXTRACT("DF078", "%hhu", &eph->gps_code_l2);
    EXTRACT("DF079", "%lf", &eph->gps_idot);
    EXTRACT("DF071", "%hu", &eph->gps_iode);
    EXTRACT("DF081", "%u", &eph->gps_toc);
    EXTRACT("DF082", "%lf", &eph->gps_af2);
    EXTRACT("DF083", "%lf", &eph->gps_af1);
    EXTRACT("DF084", "%lf", &eph->gps_af0);
    EXTRACT("DF085", "%hu", &eph->gps_iodc);
    EXTRACT("DF086", "%lf", &eph->gps_crs);
    EXTRACT("DF087", "%lf", &eph->gps_delta_n);
    EXTRACT("DF088", "%lf", &eph->gps_m0);
    eph->mean_anomaly = eph->gps_m0 * PI;
    EXTRACT("DF089", "%lf", &eph->gps_cuc);
    EXTRACT("DF090", "%lf", &eph->gps_eccentricity);
    eph->eccentricity = eph->gps_eccentricity * pow(2, -33);
    EXTRACT("DF091", "%lf", &eph->gps_cus);
    EXTRACT("DF092", "%lf", &eph->gps_sqrt_a);
    eph->semi_major_axis = eph->gps_sqrt_a * eph->gps_sqrt_a;
    EXTRACT("DF093", "%u", &eph->gps_toe);
    eph->time_of_week = eph->gps_toe;
    EXTRACT("DF094", "%lf", &eph->gps_cic);
    EXTRACT("DF095", "%lf", &eph->gps_omega0);
    eph->right_ascension_of_ascending_node = eph->gps_omega0 * PI;
    EXTRACT("DF096", "%lf", &eph->gps_cis);
    EXTRACT("DF097", "%lf", &eph->gps_i0);
    eph->inclination = eph->gps_i0 * PI;
    EXTRACT("DF098", "%lf", &eph->gps_crc);
    EXTRACT("DF099", "%lf", &eph->gps_omega);
    eph->argument_of_periapsis = eph->gps_omega * PI;
    EXTRACT("DF100", "%lf", &eph->gps_omega_dot);
    EXTRACT("DF101", "%lf", &eph->gps_tgd);
    EXTRACT("DF102", "%hhu", &eph->gps_sv_health);
    EXTRACT("DF103", "%hhu", &eph->gps_l2p_data_flag);
    EXTRACT("DF137", "%hu", &eph->gps_wn);
    eph->time_since_epoch = (eph->week_number * 604800) + eph->time_of_week;
    // print_ephemeris(eph); // quick debug print

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Computes the pseudorange from the given parameters.
 *
 * This function calculates the pseudorange using the formula:
 * Pseudorange = c * (integer_ms * 1e-3 + mod1s_sec + fine_sec)
 *
 * @param integer_ms Rough range integer in milliseconds.
 * @param mod1s_sec Pseudorange modulo 1 second.
 * @param fine_sec Pseudorange residuals in seconds scaled by speed of light.
 * @return The computed pseudorange in meters.
 */
double compute_pseudorange(uint32_t integer_ms, double mod1s_sec, double fine_sec)
{
    return SPEED_OF_LIGHT * (integer_ms * 1e-3) + mod1s_sec + fine_sec;
}

/**
 * @brief Computes the pseudorange for an RTCM 1002 (MSM1) observation.
 *
 * Calculates the pseudorange by combining the rough range integer (in milliseconds)
 * with the pseudorange remainder (in meters), applying the appropriate speed-of-light
 * scaling to the integer term.
 *
 * Formula:
 *    pseudorange = (amb * 299792.458) + rem
 *
 * @param amb Rough range integer in milliseconds.
 * @param rem Pseudorange remainder in meters.
 * @return Pseudorange in meters.
 */
double compute_pseudorange_msm1(double amb, double rem)
{
    // Convert ambiguity from milliseconds to meters
    double amb_meters = amb * (SPEED_OF_LIGHT / 1000.0);

    // Sum with remainder to get full pseudorange
    return (double)(amb_meters + rem);
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Stores or updates the ephemeris data for a given satellite.
 *
 * This function always updates the ephemeris for the specified satellite,
 * regardless of IODE/IODC. It ensures the latest data is always stored.
 *
 * @param new_eph Pointer to the new ephemeris data to store.
 * @return 0 on success, -1 if input is NULL, -2 if PRN is out of range (1-32).
 */
int store_ephemeris(const rtcm_1019_ephemeris_t *new_eph)
{
    if (!new_eph)
        return -1;

    uint8_t prn = new_eph->satellite_id;
    if (prn < 1 || prn > MAX_SAT)
        return -2;

    size_t idx = eph_history[prn].count++;
    if (idx < MAX_EPH_HISTORY)
    {
        eph_history[prn].eph[idx] = *new_eph;
        // printf(COLOR_GREEN "Stored ephemeris for PRN %u at idx %zu\n" COLOR_RESET, prn, idx);
    }
    else
    {
        // printf(COLOR_RED "Warning: Ephemeris history for PRN %u exceeded max entries (%d). Data may be lost.\n" COLOR_RESET, prn, MAX_EPH_HISTORY);
    }

    // Optionally update eph_table[prn] and eph_available[prn] for legacy code
    eph_table[prn] = *new_eph;
    eph_available[prn] = true;

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Stores the entire MSM4 struct for each PRN at the current epoch.
 *
 * This function appends the full MSM4 struct for each PRN in the message to a history array,
 * so you can later access all MSM4 fields for every satellite and epoch.
 *
 * @param new_msm4 Pointer to the new MSM4 observation data to store.
 * @return 0 on success, -1 if input is NULL.
 */
int store_msm4(const rtcm_1074_msm4_t *new_msm4)
{
    if (!new_msm4)
        return -1;

    for (int i = 0; i < new_msm4->n_sat; i++)
    {
        uint8_t prn = new_msm4->prn[i];
        if (prn < 1 || prn > MAX_SAT)
            continue;
        size_t epoch_idx = msm4_count[prn]++;
        if (epoch_idx < MAX_EPOCHS)
        {
            msm4_history[prn][epoch_idx] = *new_msm4;
        }
        else
        {
            // printf(COLOR_RED "Warning: MSM4 history for PRN %u exceeded max epochs (%d). Data may be lost.\n" COLOR_RESET, prn, MAX_EPOCHS);
        }
        // printf(COLOR_GREEN "Stored MSM4 for PRN %u at epoch %zu\n" COLOR_RESET, prn, epoch_idx);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Stores the entire MSM1 struct for each PRN at the current epoch.
 *
 * This function appends the full MSM1 struct for each PRN in the message to a history array,
 * so you can later access all MSM1 fields for every satellite and epoch.
 *
 * @param new_msm1 Pointer to the new MSM1 observation data to store.
 * @return 0 on success, -1 if input is NULL.
 */
int store_msm1(const rtcm_1002_msm1_t *new_msm1)
{
    if (!new_msm1)
        return -1;

    for (int i = 0; i < new_msm1->num_satellites; i++)
    {
        uint8_t prn = new_msm1->svs[i];
        if (prn < 1 || prn > MAX_SAT)
            continue;

        size_t epoch_idx = msm1_count[prn]++;
        if (epoch_idx < MAX_EPOCHS)
        {
            msm1_history[prn][epoch_idx] = *new_msm1;
        }
        else
        {
            // printf(COLOR_RED
            //        "Warning: MSM1 history for PRN %u exceeded max epochs (%d). Data may be lost.\n" COLOR_RESET,
            //        prn, MAX_EPOCHS);
        }

        // printf(COLOR_GREEN "Stored MSM1 for PRN %u at epoch %zu\n" COLOR_RESET, prn, epoch_idx);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Stores pseudorange for all satellites in the MSM4 message at the current epoch.
 *
 * This function appends the pseudorange for each PRN in the message to a history array,
 * so you can later plot the time series for each satellite.
 *
 * @param new_msm4 Pointer to the new MSM4 observation data to store.
 * @return 0 on success, -1 if input is NULL.
 */
int store_pseudorange(const rtcm_1074_msm4_t *new_msm4)
{
    if (!new_msm4)
        return -1;

    for (int i = 0; i < new_msm4->n_sat; i++)
    {
        uint8_t prn = new_msm4->prn[i];
        if (prn < 1 || prn > MAX_SAT)
            continue;
        size_t epoch_idx = pseudorange_count[prn]++;
        if (epoch_idx < MAX_EPOCHS)
        {
            pseudorange_history[prn][epoch_idx] = new_msm4->pseudorange[i];
        }
        else
        {
            // printf(COLOR_RED "Warning: Pseudorange history for PRN %u exceeded max epochs (%d). Data may be lost.\n" COLOR_RESET, prn, MAX_EPOCHS);
        }
        // printf(COLOR_GREEN "Stored pseudorange for PRN %u at epoch %zu : %.12f\n" COLOR_RESET, prn, epoch_idx, new_msm4->pseudorange[i]);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Stores pseudorange for all satellites in the MSM1 message at the current epoch.
 *
 * Computes pseudorange from MSM1 rough-range ambiguity (ms) and remainder, then appends
 * the value for each PRN to a per-PRN history buffer for later plotting.
 *
 * @param new_msm1 Pointer to the new MSM1 observation data to store.
 * @return 0 on success, -1 if input is NULL.
 */
int store_pseudorange_msm1(const rtcm_1002_msm1_t *new_msm1)
{
    if (!new_msm1)
        return -1;

    for (int i = 0; i < new_msm1->num_satellites; i++)
    {
        uint8_t prn = new_msm1->svs[i]; //
        if (prn < 1 || prn > MAX_SAT)
            continue;

        double pr = compute_pseudorange_msm1(
            new_msm1->ambiguities[i],
            new_msm1->remainders[i]);

        size_t epoch_idx = pseudorange_count[prn]++;
        if (epoch_idx < MAX_EPOCHS)
        {
            pseudorange_history[prn][epoch_idx] = pr;
        }
        else
        {
            // printf(COLOR_RED
            //        "Warning: Pseudorange history for PRN %u exceeded max epochs (%d). Data may be lost.\n" COLOR_RESET,
            //        prn, MAX_EPOCHS);
        }

        // printf(COLOR_GREEN "Stored MSM1 pseudorange for PRN %u at epoch %zu : %.12f\n" COLOR_RESET, prn, epoch_idx, pr);
    }

    return 0;
}
