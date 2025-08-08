#ifndef RECEIVER_H
#define RECEIVER_H

#include "../include/algo.h"
#include "../include/satellites.h"

#define ITERATIONS 10
#define MIN_SATS 4

typedef struct
{
    double x[MAX_EPOCHS];
    double y[MAX_EPOCHS];
    double z[MAX_EPOCHS];
} estimated_position_t;

extern estimated_position_t estimated_positions_ecef[MAX_SAT + 1];

int estimate_receiver_positions(void);

#endif // RECEIVER_H
