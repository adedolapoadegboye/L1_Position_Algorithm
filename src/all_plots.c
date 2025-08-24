/**
 * @file all_plots.c
 * @brief Writers for receiver and satellite tracks used by plotting scripts.
 *
 * This module dumps computed GNSS artifacts to simple, columnar text files that
 * gnuplot (or any plotting tool) can consume. It includes:
 *  - Receiver ECEF track (meters)
 *  - Receiver ECEF track with epoch index (kilometers)
 *  - Receiver geographic track (lat, lon in degrees)
 *  - Satellite orbits / ECEF samples (meters)
 *  - Satellite XYZ (kilometers)
 *  - (Optional) Pseudorange vs. time (kilometers)
 *
 * The routines are intentionally minimal and perform basic validation to avoid
 * writing all-zero or non-finite rows.
 */

#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/satellites.h"
#include "../include/receiver.h"

extern latlonalt_position_t latlonalt_positions;           /**< LLA results per epoch */
extern sat_ecef_history_t sat_ecef_positions[MAX_SAT + 1]; /**< Satellite ECEF histories */
extern estimated_position_t estimated_positions_ecef;      /**< Receiver ECEF track */

/**
 * @brief Write receiver ECEF track (meters) to a file.
 *
 * Output format (per line): `X Y Z`
 *
 * @param path     Destination file path.
 * @param n_epochs Number of epochs to write.
 * @return 0 on success, -1 on failure (open error or no lines written).
 */
int write_receiver_track_ecef(const char *path, int n_epochs)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        perror("[ERR] fopen(receiver_track)");
        return -1;
    }

    int lines = 0;
    for (int i = 0; i < n_epochs; ++i)
    {
        double x = estimated_positions_ecef.x[i];
        double y = estimated_positions_ecef.y[i];
        double z = estimated_positions_ecef.z[i];

        /* Optional debug:
         * printf("[DBG] epoch %d -> ECEF=(%.6f, %.6f, %.6f)\n", i, x, y, z);
         */

        /* Only write if not all zeros (safeguard) */
        if (!(x == 0.0 && y == 0.0 && z == 0.0))
        {
            fprintf(fp, "%.8f %.8f %.8f\n", x, y, z);
            lines++;
        }
    }

    fclose(fp);

    if (lines == 0)
    {
        fprintf(stderr, "[WARN] write_receiver_track_ecef: wrote 0 lines (all zero ECEF positions?)\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Write satellite ECEF samples (meters) as one big file.
 *
 * Output format (per line): `PRN X Y Z`
 * Sats are separated by blank lines.
 *
 * @param path Destination file path.
 * @return 0 on success, -1 on failure to open.
 */
int write_sat_orbits(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;

    for (int prn = 1; prn <= MAX_SAT; ++prn)
    {
        int wrote_any = 0;
        for (int k = 0; k < MAX_EPOCHS; ++k)
        {
            if (sat_ecef_positions[prn].t_ms[k] == 0.0)
                continue;

            double x = sat_ecef_positions[prn].x[k];
            double y = sat_ecef_positions[prn].y[k];
            double z = sat_ecef_positions[prn].z[k];

            fprintf(fp, "%d %.6f %.6f %.6f\n", prn, x, y, z);
            wrote_any = 1;
        }
        if (wrote_any)
            fprintf(fp, "\n\n");
    }

    fclose(fp);
    return 0;
}

/**
 * @brief Write receiver geographic track (lat, lon in degrees).
 *
 * Output format (per line): `lat lon`
 *
 * @param path     Destination file path.
 * @param n_epochs Number of epochs to write.
 * @return 0 on success, -1 if file open failed or no finite rows written.
 */
int write_receiver_track_geo(const char *path, int n_epochs)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        perror("[ERR] fopen(receiver_track_geo)");
        return -1;
    }

    int lines = 0;
    for (int i = 0; i < n_epochs; ++i)
    {
        double lat = latlonalt_positions.lat[i];
        double lon = latlonalt_positions.lon[i];

        /* Optional debug:
         * printf("[DBG] epoch %d -> LLA=(lat=%.8f, lon=%.8f)\n", i, lat, lon);
         */

        if (isfinite(lat) && isfinite(lon))
        {
            fprintf(fp, "%.8f %.8f\n", lat, lon);
            lines++;
        }
    }

    fclose(fp);

    if (lines == 0)
    {
        fprintf(stderr, "[WARN] write_receiver_track_geo: wrote 0 lines (no finite LLA values?)\n");
        return -1;
    }

    return 0;
}

/* --- Small unit helper --- */
static inline double m_to_km(double v_m) { return v_m * 1e-3; }

/**
 * @brief Write receiver ECEF (kilometers) with epoch index prefix.
 *
 * Output format (per line): `epoch_index X_km Y_km Z_km`
 *
 * @param path     Destination file path.
 * @param n_epochs Number of epochs to write.
 * @return 0 on success, -1 if file open failed.
 */
int write_receiver_ecef_epoch_km(const char *path, int n_epochs)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;

    for (int i = 0; i < n_epochs; ++i)
    {
        double xk = m_to_km(estimated_positions_ecef.x[i]);
        double yk = m_to_km(estimated_positions_ecef.y[i]);
        double zk = m_to_km(estimated_positions_ecef.z[i]);

        if (!isfinite(xk) || !isfinite(yk) || !isfinite(zk))
            continue;

        fprintf(fp, "%d %.6f %.6f %.6f\n", i, xk, yk, zk);
    }

    fclose(fp);
    return 0;
}

/**
 * @brief Write satellite XYZ (kilometers).
 *
 * Output format (per line): `PRN X_km Y_km Z_km`
 * Sats are separated by blank lines.
 *
 * @param path Destination file path.
 * @return 0 on success, -1 if file open failed.
 */
int write_sat_xyz_km(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;

    for (int prn = 1; prn <= MAX_SAT; ++prn)
    {
        int wrote_any = 0;
        for (int k = 0; k < MAX_EPOCHS; ++k)
        {
            if (sat_ecef_positions[prn].t_ms[k] == 0.0)
                continue;

            double xk = m_to_km(sat_ecef_positions[prn].x[k]);
            double yk = m_to_km(sat_ecef_positions[prn].y[k]);
            double zk = m_to_km(sat_ecef_positions[prn].z[k]);

            if (!isfinite(xk) || !isfinite(yk) || !isfinite(zk))
                continue;

            fprintf(fp, "%d %.6f %.6f %.6f\n", prn, xk, yk, zk);
            wrote_any = 1;
        }
        if (wrote_any)
            fputs("\n\n", fp);
    }

    fclose(fp);
    return 0;
}

/**
 * @brief (Optional) Write pseudorange vs. time per PRN (kilometers).
 *
 * Output format (per line): `PRN t(s) PR_km`
 * Blocks are separated by blank lines.
 *
 * @param path Destination file path.
 * @return 0 on success, -1 if file open failed.
 *
 * @note Time units: currently writes DF004/collected time as seconds if `t`
 *       is already in seconds. If your timestamps are in milliseconds, scale accordingly.
 */
int write_pseudorange_time_km(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;

    for (int prn = 1; prn <= MAX_SAT; ++prn)
    {
        for (int k = 0; k < MAX_EPOCHS; ++k)
        {
            uint32_t t = gps_list[prn].times_of_pseudorange[k];
            double pr = gps_list[prn].pseudoranges[k]; /* meters */

            if (t == 0 || !isfinite(pr))
                continue;

            /* Adjust if your t is milliseconds: use (double)t * 1e-3 */
            double tow_s = (double)t;

            fprintf(fp, "%d %.3f %.6f\n", prn, tow_s, pr * 1e-3);
        }
        fputs("\n\n", fp);
    }

    fclose(fp);
    return 0;
}
