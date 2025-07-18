/**
 * @file df_parser.c
 * @brief Parses RTCM 1019 and 1074 messages from text-formatted input lines.
 *
 * This module provides parsing logic for two specific RTCM message types:
 * - RTCM 1019: Broadcast ephemeris messages (used for GPS satellite position)
 * - RTCM 1074: MSM4 observation messages (used for pseudorange and carrier-phase GNSS measurements)
 *
 * The input is assumed to be text-format (e.g., exported from parsed binary logs),
 * and each line contains a full RTCM message with labeled fields (DFxxx).
 *
 * Output is stored in well-defined structures for further positioning and analysis.
 */

#include "../include/algo.h"
#include "../include/df_parser.h"
#include <string.h>
#include <stdlib.h>

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

    EXTRACT("DF009", "%hhu", &eph->satellite_id);
    EXTRACT("DF076", "%hhu", &eph->iode);
    EXTRACT("DF077", "%hhu", &eph->ura_index);
    EXTRACT("DF078", "%hhu", &eph->sv_health);
    EXTRACT("DF079", "%lf", &eph->tgd);
    EXTRACT("DF071", "%hhu", &eph->iodc);
    EXTRACT("DF081", "%u", &eph->toc);
    EXTRACT("DF082", "%lf", &eph->af2);
    EXTRACT("DF083", "%lf", &eph->af1);
    EXTRACT("DF084", "%lf", &eph->af0);
    EXTRACT("DF085", "%hu", &eph->week_number);
    EXTRACT("DF086", "%lf", &eph->crs);
    EXTRACT("DF087", "%lf", &eph->delta_n);
    EXTRACT("DF088", "%lf", &eph->m0);
    EXTRACT("DF089", "%lf", &eph->cuc);
    EXTRACT("DF090", "%lf", &eph->eccentricity);
    EXTRACT("DF091", "%lf", &eph->cus);
    EXTRACT("DF092", "%lf", &eph->sqrt_a);
    EXTRACT("DF093", "%u", &eph->toe);
    EXTRACT("DF094", "%lf", &eph->cic);
    EXTRACT("DF095", "%lf", &eph->omega0);
    EXTRACT("DF096", "%lf", &eph->cis);
    EXTRACT("DF097", "%lf", &eph->i0);
    EXTRACT("DF098", "%lf", &eph->crc);
    EXTRACT("DF099", "%lf", &eph->omega);
    EXTRACT("DF100", "%lf", &eph->omega_dot);
    EXTRACT("DF101", "%lf", &eph->idot);
    EXTRACT("DF102", "%hhu", &eph->fit_interval);
    EXTRACT("DF103", "%hhu", &eph->spare);
    EXTRACT("DF137", "%hu", &eph->gps_wn);

    print_ephemeris(eph); // quick debug print

    return 0;
}

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

    print_msm4(msm4); // quick debug print

    return 0;
}
