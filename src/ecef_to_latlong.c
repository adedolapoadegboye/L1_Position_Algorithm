/**
 * @file ecef_to_geodetic.c
 * @brief Convert ECEF (Earth-Centered, Earth-Fixed) coordinates to geodetic coordinates (WGS-84).
 *
 * This function converts Earth-Centered Earth-Fixed (ECEF) Cartesian coordinates
 * (X, Y, Z in meters) into geodetic latitude, longitude, and altitude above the WGS-84 ellipsoid.
 *
 * Implementation details:
 *  - Uses WGS-84 constants (semi-major axis and flattening).
 *  - Applies Bowring’s formula for latitude computation.
 *  - Computes ellipsoidal height (altitude) relative to the WGS-84 reference ellipsoid.
 *
 * @param[in]  x        ECEF X coordinate (meters).
 * @param[in]  y        ECEF Y coordinate (meters).
 * @param[in]  z        ECEF Z coordinate (meters).
 * @param[out] lat_deg  Geodetic latitude in degrees.
 * @param[out] lon_deg  Geodetic longitude in degrees.
 * @param[out] alt_m    Altitude above WGS-84 ellipsoid (meters).
 *
 * @note If all inputs (x, y, z) are zero, the function returns lat=0, lon=0, alt=-a (arbitrary).
 */

#include "../include/df_parser.h"

void ecef_to_geodetic(double x, double y, double z,
                      double *lat_deg, double *lon_deg, double *alt_m)
{
    // WGS-84 constants
    const double a = 6378137.0;                   // semi-major axis (m)
    const double f = 1.0 / 298.257223563;         // flattening
    const double b = a * (1.0 - f);               // semi-minor axis (m)
    const double e2 = 2.0 * f - f * f;            // first eccentricity^2
    const double ep2 = (a * a - b * b) / (b * b); // second eccentricity^2
    const double rad2deg = 180.0 / PI;

    // Longitude
    double lon = atan2(y, x);

    // Distance from Z axis
    double p = sqrt(x * x + y * y);

    // Guard against origin
    if (p == 0.0 && z == 0.0)
    {
        if (lat_deg)
            *lat_deg = 0.0;
        if (lon_deg)
            *lon_deg = 0.0;
        if (alt_m)
            *alt_m = -a; // arbitrary
        return;
    }

    // Bowring’s formula for latitude
    double theta = atan2(z * a, p * b);
    double st = sin(theta), ct = cos(theta);
    double lat = atan2(z + ep2 * b * st * st * st,
                       p - e2 * a * ct * ct * ct);

    // Radius of curvature in the prime vertical
    double sl = sin(lat);
    double N = a / sqrt(1.0 - e2 * sl * sl);

    // Altitude
    double alt = (p / cos(lat)) - N;

    if (lat_deg)
        *lat_deg = lat * rad2deg;
    if (lon_deg)
        *lon_deg = lon * rad2deg;
    if (alt_m)
        *alt_m = alt;
}
