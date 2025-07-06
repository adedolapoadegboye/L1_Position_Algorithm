/**
 * @file file_input_mode.c
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
#include "../include/geo_utils.h"
#include "../include/df_parser.h"

void ecef_to_geodetic(const ECEF_Coordinate *ecef, Geodetic_Coordinate *geo)
{
    const double a = 6378137.0;           // WGS84 semi-major axis
    const double f = 1.0 / 298.257223563; // Flattening
    const double e2 = f * (2 - f);        // Square of eccentricity

    double x = ecef->x;
    double y = ecef->y;
    double z = ecef->z;

    double lon = atan2(y, x);
    double r = sqrt(x * x + y * y);
    double lat = atan2(z, r); // First guess
    double h = 0.0;

    for (int i = 0; i < 5; ++i)
    {
        double sin_lat = sin(lat);
        double N = a / sqrt(1 - e2 * sin_lat * sin_lat);
        h = r / cos(lat) - N;
        lat = atan2(z, r * (1 - e2 * (N / (N + h))));
    }

    geo->lat_deg = lat * (180.0 / M_PI);
    geo->lon_deg = lon * (180.0 / M_PI);
    geo->alt_m = h;
}

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
    printf("Satellite PRN       : %u\n", eph->satellite_id);
    printf("IODE                : %u\n", eph->iode);
    printf("IODC                : %u\n", eph->iodc);
    printf("URA Index           : %u\n", eph->ura_index);
    printf("SV Health           : %u\n", eph->sv_health);
    printf("TGD (s)             : %.12f\n", eph->tgd);

    printf("Clock Data:\n");
    printf("  Toc (s)           : %u\n", eph->toc);
    printf("  af0 (s)           : %.12f\n", eph->af0);
    printf("  af1 (s/s)         : %.12e\n", eph->af1);
    printf("  af2 (s/s^2)       : %.12e\n", eph->af2);

    printf("Orbit Data:\n");
    printf("  Week Number       : %u\n", eph->week_number);
    printf("  Delta N (rad/s)   : %.12e\n", eph->delta_n);
    printf("  M0 (rad)          : %.12e\n", eph->m0);
    printf("  Eccentricity      : %.12f\n", eph->eccentricity);
    printf("  sqrt(A) (m^0.5)   : %.12f\n", eph->sqrt_a);
    printf("  Omega0 (rad)      : %.12f\n", eph->omega0);
    printf("  i0 (rad)          : %.12f\n", eph->i0);
    printf("  Omega (rad)       : %.12f\n", eph->omega);
    printf("  Omega dot (rad/s) : %.12e\n", eph->omega_dot);
    printf("  IDOT (rad/s)      : %.12e\n", eph->idot);
    printf("  TOE (s)           : %u\n", eph->toe);

    printf("Corrections:\n");
    printf("  CRS (m)           : %.4f\n", eph->crs);
    printf("  CUC               : %.4f\n", eph->cuc);
    printf("  CUS               : %.4f\n", eph->cus);
    printf("  CIC               : %.4f\n", eph->cic);
    printf("  CIS               : %.4f\n", eph->cis);
    printf("  CRC               : %.4f\n", eph->crc);

    printf("Fit Interval        : %u\n", eph->fit_interval);
    printf("Full GPS Week       : %u\n", eph->gps_wn);
    printf("--------------------------------------\n");
}

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
        printf("  PRN_%02d: %u  | Integer PR: %u  | Mod PR: %.12f | Fine PR: %.24f\n",
               i + 1,
               msm4->prn[i],
               msm4->pseudorange_integer[i],
               msm4->pseudorange_mod_1s[i],
               msm4->pseudorange_fine[i]);
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
