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
