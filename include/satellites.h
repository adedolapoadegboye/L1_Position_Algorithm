#ifndef SATELLITES_H
#define SATELLITES_H

#include "../include/df_parser.h"
#include "../include/algo.h"

// Function to sort satellites based on their ephemeris and MSM4 data
int sort_satellites(eph_history_t *eph_history, rtcm_1074_msm4_t msm4_history[MAX_SAT][MAX_EPOCHS], rtcm_1002_msm1_t msm1_history[MAX_SAT][MAX_EPOCHS]);
// Structure to hold GPS satellite data for each satellite available in the system
typedef struct
{
    double prn;
    double pseudoranges[MAX_EPOCHS];
    uint32_t times_of_pseudorange[MAX_EPOCHS];
    double eccentricities[MAX_EPOCHS];
    double inclinations[MAX_EPOCHS];
    double mean_anomalies[MAX_EPOCHS];
    double semi_major_axes[MAX_EPOCHS];
    double right_ascension_of_ascending_node[MAX_EPOCHS];
    double argument_of_periapsis[MAX_EPOCHS];
    double times_of_ephemeris[MAX_EPOCHS];
} gps_satellite_data_t;

extern gps_satellite_data_t gps_list[MAX_SAT + 1];
void print_gps_list(void);

// ECI satellite position calculation
#define EARTH_MASS 5.9722e24               // kg
#define GRAVITATIONAL_CONSTANT 6.67430e-11 // m^3 kg^-1 s^-2
#define OMEGA_EARTH 7.2921151467e-5        // rad/s, Earth's rotation rate

// Earth's standard gravitational parameter (mu = GM), in m^3/s^2
#define MU (EARTH_MASS * GRAVITATIONAL_CONSTANT)

int satellite_position_eci(const gps_satellite_data_t gps_lists[]);

typedef struct
{
    double x[MAX_EPOCHS];
    double y[MAX_EPOCHS];
    double z[MAX_EPOCHS];
} sat_eci_history_t;

extern sat_eci_history_t sat_eci_positions[MAX_SAT + 1];

void mat3x3_vec3_mult(const double mat[3][3], const double vec[3], double out[3]);

typedef struct
{
    double x[MAX_EPOCHS];
    double y[MAX_EPOCHS];
    double z[MAX_EPOCHS];
    double t_ms[MAX_EPOCHS];
} sat_ecef_history_t;

extern sat_ecef_history_t sat_ecef_positions[MAX_SAT + 1];

typedef struct
{
    double p[MAX_EPOCHS];
    double q[MAX_EPOCHS];
    double w[MAX_EPOCHS];
} sat_orbit_pqw_history_t;

extern sat_orbit_pqw_history_t sat_orbit_pqw_positions[MAX_SAT + 1];

typedef struct
{
    double x[MAX_EPOCHS];
    double y[MAX_EPOCHS];
    double z[MAX_EPOCHS];
} sat_orbit_eci_history_t;

extern sat_orbit_eci_history_t sat_orbit_eci_positions[MAX_SAT + 1];

int satellite_position_ecef(const gps_satellite_data_t gps_lists[MAX_SAT + 1]);

int satellite_orbit_eci(const gps_satellite_data_t gps_lists[]);

#endif
