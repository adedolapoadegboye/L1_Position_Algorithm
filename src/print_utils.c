/**
 * @file print_utils.c
 * @brief Handles RTCM MSM4 file input, step-by-step parsing and position resolution.
 *
 * This function is triggered when the user selects "File Input" from the application menu.
 * It opens a pre-recorded RTCM binary file, reads each epoch's MSM4 message, extracts relevant
 * GNSS observations, and calculates the receiver position using the least squares method.
 * The resulting position is printed in geodetic coordinates (decimal degrees).
 *
 * All major steps (reading, parsing, solving) are delegated to their respective modules
 * to maintain modularity and educational clarity.
 */

#include "../include/algo.h"
#include "../include/df_parser.h"

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Prints the contents of a parsed RTCM 1019 ephemeris structure.
 *
 * This function outputs the ephemeris data in a human-readable format,
 * useful for debugging and verification purposes.
 *
 * @param eph Pointer to the ephemeris structure to print.
 */
void print_ephemeris(const rtcm_1019_ephemeris_t *eph)
{
    if (!eph)
        return;

    printf("\n------ GPS Ephemeris (RTCM 1019) ------\n");
    printf("Satellite PRN         : %u\n", eph->satellite_id);
    printf("IODE                  : %u\n", eph->gps_iode);
    printf("IODC                  : %u\n", eph->gps_iodc);
    printf("URA Index             : %u\n", eph->gps_sv_acc);
    printf("SV Health             : %u\n", eph->gps_sv_health);
    printf("TGD (s)               : %.24e\n", eph->gps_tgd);

    printf("\nClock Data:\n");
    printf("  Toc (s)             : %u\n", eph->gps_toc);
    printf("  af0 (s)             : %.24e\n", eph->gps_af0);
    printf("  af1 (s/s)           : %.24e\n", eph->gps_af1);
    printf("  af2 (s/s^2)         : %.24e\n", eph->gps_af2);

    printf("\nOrbit Data:\n");
    printf("  Week Number         : %u\n", eph->gps_wn);
    printf("  Delta N (rad/s)     : %.24e\n", eph->gps_delta_n);
    printf("  M0 (rad)            : %.24e\n", eph->gps_m0);
    printf("  Eccentricity        : %.24e\n", eph->gps_eccentricity);
    printf("  sqrt(A) (m^0.5)     : %.24e\n", eph->gps_sqrt_a);
    printf("  Omega0 (rad)        : %.24e\n", eph->gps_omega0);
    printf("  i0 (rad)            : %.24e\n", eph->gps_i0);
    printf("  Omega (rad)         : %.24e\n", eph->gps_omega);
    printf("  Omega dot (rad/s)   : %.24e\n", eph->gps_omega_dot);
    printf("  IDOT (rad/s)        : %.24e\n", eph->gps_idot);
    printf("  TOE (s)             : %u\n", eph->gps_toe);

    printf("\nCorrections:\n");
    printf("  CRS (m)             : %.12e\n", eph->gps_crs);
    printf("  CUC                 : %.12e\n", eph->gps_cuc);
    printf("  CUS                 : %.12e\n", eph->gps_cus);
    printf("  CIC                 : %.12e\n", eph->gps_cic);
    printf("  CIS                 : %.12e\n", eph->gps_cis);
    printf("  CRC                 : %.12e\n", eph->gps_crc);

    printf("\nOther:\n");
    printf("  Code on L2 Flag     : %u\n", eph->gps_code_l2);
    printf("  L2P Data Flag       : %u\n", eph->gps_l2p_data_flag);
    printf("  Fit Interval        : %u\n", eph->gps_fit_interval);
    printf("--------------------------------------\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Prints the contents of a parsed RTCM 1002 (MSM1 GPS L1) observation.
 *
 * Uses the provided rtcm_1002_msm1_t fields (no assumptions).
 * For each satellite, it also computes pseudorange with compute_pseudorange_msm1()
 * from DF014 (ambiguity, ms) and DF011 (remainder, m) and shows the stored value.
 */
static const char *msm1_sig_name(uint8_t sig_id)
{
    // DF010: 00 = C/A, 01 = P(Y). Keep simple; extend as needed.
    switch (sig_id)
    {
    case 0:
        return "C/A";
    case 1:
        return "P(Y)";
    default:
        return "Unknown";
    }
}

void print_msm1(const rtcm_1002_msm1_t *msm1)
{
    if (!msm1)
        return;

    printf("\n========= RTCM 1002 (MSM1 GPS L1) =========\n");

    printf("Message Type            : %u\n", msm1->msg_type);
    printf("Station ID              : %u\n", msm1->station_id);
    printf("GPS TOW (s)             : %u\n", msm1->time_of_week);
    printf("Sync GNSS Msg Flag      : %u\n", msm1->sync_gps_message_flag);
    printf("Num Satellites (NSat)   : %u\n", msm1->num_satellites);
    printf("Smoothing Used?         : %s\n", msm1->smooth_interval_flag ? "No divergence-free smoothing" : "Divergence-free smoothing");
    printf("Smoothing Interval (s)  : %u\n", msm1->smooth_interval);

    // If any signal IDs were populated, show the distinct ones present.
    // (Structure provides DF010 as an array; list any non-zero/known entries.)
    bool printed_sig_header = false;
    for (int s = 0; s < MAX_SIG; s++)
    {
        if (msm1->sig_id[s] || s == 0)
        {
            if (!printed_sig_header)
            {
                printf("\n-- Signal IDs Present (DF010) --\n");
                printed_sig_header = true;
            }
            printf("  sig[%02d] = %u (%s)\n", s, msm1->sig_id[s], msm1_sig_name(msm1->sig_id[s]));
        }
    }

    printf("\n-- Satellite Observations --\n");
    printf("Idx  PRN  Amb(ms)    Rem(m)           PR(computed)         PR(stored)           Phase-PR(m)        Lock  C/N0(dBHz)\n");
    printf("---- ---- ---------- ---------------- --------------------- --------------------- ------------------ ----- -----------\n");

    for (int i = 0; i < msm1->num_satellites; i++)
    {
        uint8_t prn = msm1->svs[i];
        double amb_ms = msm1->ambiguities[i]; // DF014 in ms
        double rem_m = msm1->remainders[i];   // DF011 in m
        double pr_comp = compute_pseudorange_msm1(amb_ms, rem_m);
        double pr_stored = msm1->pseudoranges[i]; // already in meters
        double phase_pr = msm1->phase_pr_diff[i]; // DF012 in meters
        uint8_t lock = msm1->lock_time[i];        // DF013 in seconds
        uint8_t cnr = msm1->cnr[i];               // DF015 (already scaled to dB-Hz)

        printf("%-4d %-4u %-10.3f %-16.6f %-21.6f %-21.6f %-18.6f %-5u %-11u\n",
               i + 1, prn, amb_ms, rem_m, pr_comp, pr_stored, phase_pr, lock, cnr);
    }

    printf("============================================\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Prints the contents of a parsed RTCM 1074 MSM4 observation structure.
 *
 * This function outputs the observation data in a human-readable format,
 * useful for debugging and verification purposes.
 *
 * @param msm4 Pointer to the MSM4 observation structure to print.
 */
void print_msm4(const rtcm_1074_msm4_t *msm4)
{
    if (!msm4)
        return;

    printf("\n========= RTCM 1074 MSM4 DATA =========\n");

    printf("Message Type        : %u\n", msm4->msg_type);
    printf("Station ID          : %u\n", msm4->station_id);
    printf("Epoch Time (ms)     : %u\n", msm4->gps_epoch_time);
    printf("Sync Flag           : %u\n", msm4->msm_sync_flag);
    printf("Clock Steering Flag : %u\n", msm4->clk_steering_flag);
    printf("External Clock Flag : %u\n", msm4->external_clk_flag);
    printf("Smoothing Flag      : %u\n", msm4->smooth_interval_flag);

    printf("Number of Satellites: %u\n", msm4->n_sat);
    printf("Signal Type         : %u\n", msm4->n_sig);
    printf("Total Signal Count  : %u\n", msm4->n_cell);

    printf("\n-- Satellite PRNs --\n");
    for (int i = 0; i < msm4->n_sat; i++)
    {
        printf("  PRN_%02d: %u  | Integer PR: %u  | Mod PR: %.12f | Fine PR: %.24f | Full PR: %.24f\n",
               i + 1,
               msm4->prn[i],
               msm4->pseudorange_integer[i],
               msm4->pseudorange_mod_1s[i],
               msm4->pseudorange_fine[i],
               msm4->pseudorange[i]);
    }

    printf("\n-- L1C Cell Observations --\n");
    for (int i = 0; i < msm4->n_cell; i++)
    {
        printf("  Cell %02d:\n", i + 1);
        printf("    PRN             : %u\n", msm4->cell_prn[i]);
        printf("    Signal ID       : %u (1 = L1C)\n", msm4->cell_sig[i]);
        printf("    Lock Time       : %u\n", msm4->lock_time[i]);
        printf("    CNR             : %u dBHz\n", msm4->cnr[i]);
    }

    printf("========================================\n");
}
