#ifndef GEO_UTILS_H
#define GEO_UTILS_H

#include "position_solver.h"

typedef struct
{
    double lat_deg;
    double lon_deg;
    double alt_m;
} Geodetic_Coordinate;

void ecef_to_geodetic(const ECEF_Coordinate *ecef, Geodetic_Coordinate *geo);

#endif // GEO_UTILS_H
