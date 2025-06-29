#ifndef DF_PARSER_H
#define DF_PARSER_H

#include "rtcm_reader.h"

typedef struct
{
    int sat_id;
    double pseudorange;
    double carrier_phase;
    double doppler;
    double snr;
} GNSS_SatelliteObs;

typedef struct
{
    GNSS_SatelliteObs sats[16];
    int sat_count;
    double epoch_time;
} GNSS_ObservationSet;

int parse_msm4_message(RTCM_Message *message, GNSS_ObservationSet *obs);

#endif // DF_PARSER_H
