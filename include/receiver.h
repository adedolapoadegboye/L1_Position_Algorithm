#ifndef RECEIVER_H
#define RECEIVER_H

#include "../include/algo.h"
#include "../include/satellites.h"
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#define ITERATIONS 10
#define MIN_SATS 4
#define RAD2DEG (180.0 / M_PI)
// ========================= TUNABLES / DEBUG =========================
#define ENABLE_LSQ_DEBUG 1
#define MAX_UNIQUE_EPOCHS 100000 // hard cap to avoid huge working sets
#define MAX_SV_USED MAX_SAT      // per-epoch satellite cap (<= MAX_SAT)
// ===================================================================

#if ENABLE_LSQ_DEBUGs
#define DLOG(...)                     \
    do                                \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)
#else
#define DLOG(...) \
    do            \
    {             \
    } while (0)
#endif

typedef struct
{
    double x[MAX_EPOCHS];
    double y[MAX_EPOCHS];
    double z[MAX_EPOCHS];
} estimated_position_t;

extern estimated_position_t estimated_positions_ecef;

int estimate_receiver_positions(void);

void ecef_to_geodetic(double x, double y, double z,
                      double *lat_deg, double *lon_deg, double *alt_m);
typedef struct
{
    double lat[MAX_EPOCHS];
    double lon[MAX_EPOCHS];
    double alt[MAX_EPOCHS];
} latlonalt_position_t;

extern latlonalt_position_t latlonalt_positions;

extern int n_times;

#endif // RECEIVER_H
