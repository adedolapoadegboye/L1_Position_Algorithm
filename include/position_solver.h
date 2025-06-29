#ifndef POSITION_SOLVER_H
#define POSITION_SOLVER_H

#include "df_parser.h"

typedef struct
{
    double x, y, z;
} ECEF_Coordinate;

int calculate_position_least_squares(GNSS_ObservationSet *obs, ECEF_Coordinate *receiver_ecef);

#endif // POSITION_SOLVER_H
