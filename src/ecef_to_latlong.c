/**
 * @file receiver_position.c
 * @brief Functions for estimating receiver position.
 * This file contains functions to handle receiver position calculations, first in ECEF then in decimal format.
 */

#include "../include/satellites.h"
#include "../include/algo.h"
#include "../include/df_parser.h"
#include "../include/rtcm_reader.h"
#include "../include/df_parser.h"
#include "../include/satellites.h"
#include "../include/receiver.h"

#define WGS84_A 6378137.0                          // semi-major axis in meters
#define WGS84_F (1.0 / 298.257223563)              // flattening
#define WGS84_B (WGS84_A * (1.0 - WGS84_F))        // semi-minor axis
#define WGS84_E2 (2 * WGS84_F - WGS84_F * WGS84_F) // eccentricity squared
#define WGS84_EP2 ((WGS84_A * WGS84_A - WGS84_B * WGS84_B) / (WGS84_B * WGS84_B))

latlonalt_position_t latlongalt_positions_ecef[MAX_SAT + 1] = {0};

/**
 * Convert ECEF (meters) to geodetic coordinates on WGS84 ellipsoid.
 * lat/lon in degrees, alt in meters.
 */
void ecef_to_geodetic(double x, double y, double z,
                      double *lat_deg, double *lon_deg, double *alt_m)
{
    double lon = atan2(y, x);

    double p = sqrt(x * x + y * y);
    double theta = atan2(z * WGS84_A, p * WGS84_B);

    double sin_theta = sin(theta);
    double cos_theta = cos(theta);

    double lat = atan2(z + WGS84_EP2 * WGS84_B * sin_theta * sin_theta * sin_theta,
                       p - WGS84_E2 * WGS84_A * cos_theta * cos_theta * cos_theta);

    double sin_lat = sin(lat);
    double N = WGS84_A / sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);

    double alt = p / cos(lat) - N;

    *lat_deg = lat * (180.0 / M_PI);
    *lon_deg = lon * (180.0 / M_PI);
    *alt_m = alt;
}
