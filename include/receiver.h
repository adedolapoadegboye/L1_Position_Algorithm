#ifndef RECEIVER_H
#define RECEIVER_H

#include "../include/algo.h"
#include "../include/satellites.h"

#define ITERATIONS 10
#define MIN_SATS 4
#define RAD2DEG (180.0 / M_PI)

typedef struct
{
    double x[MAX_EPOCHS];
    double y[MAX_EPOCHS];
    double z[MAX_EPOCHS];
} estimated_position_t;

extern estimated_position_t estimated_positions_ecef[MAX_SAT + 1];

int estimate_receiver_positions(void);

void ecef_to_geodetic(double x, double y, double z,
                      double *lat_deg, double *lon_deg, double *alt_m);
typedef struct
{
    double lat[MAX_EPOCHS];
    double lon[MAX_EPOCHS];
    double alt[MAX_EPOCHS];
} latlonalt_position_t;

extern latlonalt_position_t latlonalt_positions[MAX_SAT + 1];

#endif // RECEIVER_H
