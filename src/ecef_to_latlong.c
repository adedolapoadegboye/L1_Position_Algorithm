/**
 * Convert ECEF (Earth-Centered Earth-Fixed) coordinates to spherical geographic coordinates.
 *
 * This follows the Python reference:
 *   r     = sqrt(x^2 + y^2 + z^2)
 *   theta = atan2(y, x)                     -> longitude (deg)
 *   phi   = acos(z / r)                     -> polar angle
 *   theta_deg = theta * 360 / (2*pi)
 *   phi_deg   = 90 - phi * 360 / (2*pi)     -> latitude (deg)
 *
 * @param x   ECEF X (meters)
 * @param y   ECEF Y (meters)
 * @param z   ECEF Z (meters)
 * @param lon_deg [out] Longitude in degrees
 * @param lat_deg [out] Latitude in degrees
 * @param r_m     [out] Radius in meters (distance from Earth's center)
 */

#include "../include/df_parser.h"

// Keep this signature:
void ecef_to_geodetic(double x, double y, double z,
                      double *lat_deg, double *lon_deg, double *alt_m)
{
    // WGS-84 constants
    const double a = 6378137.0;                   // semi-major axis (m)
    const double f = 1.0 / 298.257223563;         // flattening
    const double b = a * (1.0 - f);               // semi-minor axis (m)
    const double e2 = 2.0 * f - f * f;            // first eccentricity^2
    const double ep2 = (a * a - b * b) / (b * b); // second eccentricity^2
    const double rad2deg = 180.0 / M_PI;

    // Longitude
    double lon = atan2(y, x);

    // Auxiliary quantities
    double p = sqrt(x * x + y * y);
    // Guard against r=0
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

    // Bowring’s formula for initial latitude
    double theta = atan2(z * a, p * b);
    double st = sin(theta), ct = cos(theta);

    double lat = atan2(z + ep2 * b * st * st * st,
                       p - e2 * a * ct * ct * ct);

    // Radius of curvature in the prime vertical
    double sl = sin(lat);
    double N = a / sqrt(1.0 - e2 * sl * sl);

    // Altitude above ellipsoid
    double alt = p / cos(lat) - N;

    if (lat_deg)
        *lat_deg = lat * rad2deg;
    if (lon_deg)
        *lon_deg = lon * rad2deg;
    if (alt_m)
        *alt_m = alt;
}
