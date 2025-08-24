#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/satellites.h"
#include "../include/receiver.h"

extern latlonalt_position_t latlonalt_positions;           // your LLA results per epoch
extern sat_ecef_history_t sat_ecef_positions[MAX_SAT + 1]; // satellite ECEF histories
extern estimated_position_t estimated_positions_ecef;

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

        // Debug print to console
        // printf("[DBG] epoch %d -> ECEF=(%.6f, %.6f, %.6f)\n", i, x, y, z);

        // Only write if not all zeros (optional safeguard)
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

// Write all satellite points: one big file with PRN and XYZ per line
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

        // Debug to console
        // printf("[DBG] epoch %d -> LLA=(lat=%.8f deg, lon=%.8f deg)\n",
        //        i, lat, lon);

        // Write only finite values
        if (isfinite(lat) && isfinite(lon))
        {
            // Format: lat lon
            fprintf(fp, "%.8f %.8f\n", lat, lon);
            lines++;
        }
    }

    fclose(fp);

    if (lines == 0)
    {
        fprintf(stderr, "[WARN] write_receiver_track_geo: wrote 0 lines "
                        "(no finite LLA values?)\n");
        return -1;
    }

    // printf("[OK] Receiver GEO track written: %d epochs to %s\n", lines, path);
    return 0;
}
