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

    memset(eph, 0, sizeof(*eph));

    sscanf(line,
           "<RTCM(1019, DF002=1019, DF009=%hhu, DF076=%hhu, DF077=%hhu, DF078=%hhu, DF079=%lf, "
           "DF071=%hhu, DF081=%u, DF082=%lf, DF083=%lf, DF084=%lf, DF085=%hu, DF086=%lf, "
           "DF087=%lf, DF088=%lf, DF089=%lf, DF090=%lf, DF091=%lf, DF092=%lf, DF093=%u, "
           "DF094=%lf, DF095=%lf, DF096=%lf, DF097=%lf, DF098=%lf, DF099=%lf, DF100=%lf, "
           "DF101=%lf, DF102=%hhu, DF103=%hhu, DF137=%hu)",
           &eph->satellite_id, &eph->iode, &eph->ura_index, &eph->sv_health, &eph->tgd,
           &eph->iodc, &eph->toc, &eph->af2, &eph->af1, &eph->af0, &eph->week_number, &eph->crs,
           &eph->delta_n, &eph->m0, &eph->cuc, &eph->eccentricity, &eph->cus, &eph->sqrt_a,
           &eph->toe, &eph->cic, &eph->omega0, &eph->cis, &eph->i0, &eph->crc, &eph->omega,
           &eph->omega_dot, &eph->idot, &eph->fit_interval, &eph->spare, &eph->gps_wn);

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

    memset(msm4, 0, sizeof(*msm4));

    // Step 1: Parse MSM4 header (station ID, epoch time, flags)
    sscanf(line,
           "<RTCM(1074, DF002=1074, DF003=%hu, DF004=%u, DF393=%hhu, DF409=%hhu, DF001_7=%hhu, "
           "DF411=%hhu, DF412=%hhu,",
           &msm4->station_id, &msm4->epoch_time, &msm4->sync_flag, &msm4->clk_steering,
           &msm4->ext_clk, &msm4->smooth_ind, &msm4->smooth_interval);

    // Step 2: Extract NSat, NSig, NCell
    const char *ncell_ptr = strstr(line, "NCell=");
    if (ncell_ptr)
        sscanf(ncell_ptr, "NCell=%hhu", &msm4->n_cell);

    const char *nsat_ptr = strstr(line, "NSat=");
    if (nsat_ptr)
        sscanf(nsat_ptr, "NSat=%hhu", &msm4->n_sat);

    const char *nsig_ptr = strstr(line, "NSig=");
    if (nsig_ptr)
        sscanf(nsig_ptr, "NSig=%hhu", &msm4->n_sig);

    // Step 3: Extract satellite PRNs (PRN_01, PRN_02, ...)
    for (int i = 0; i < msm4->n_sat; i++)
    {
        char key[16], *ptr;
        sprintf(key, "PRN_%02d=", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key), "%hhu", &msm4->prn[i]);
    }

    // Step 4: Extract DF400 (pseudorange), DF401 (carrier phase), DF402 (lock), DF403 (CNR)
    for (int i = 0; i < msm4->n_cell; i++)
    {
        char key[24], *ptr;

        sprintf(key, "DF400_%02d=", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key), "%lf", &msm4->pseudorange[i]);

        sprintf(key, "DF401_%02d=", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key), "%lf", &msm4->phase_range[i]);

        sprintf(key, "DF402_%02d=", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key), "%hhu", &msm4->lock_time[i]);

        sprintf(key, "DF403_%02d=", i + 1);
        ptr = strstr(line, key);
        if (ptr)
            sscanf(ptr + strlen(key), "%hhu", &msm4->cnr[i]);
    }

    return 0;
}
