#ifndef POSITION_SOLVER_H
#define POSITION_SOLVER_H

#include "df_parser.h"

typedef struct
{
    double x, y, z;
} ECEF_Coordinate;

int calculate_position_least_squares(void);

#endif // POSITION_SOLVER_H
