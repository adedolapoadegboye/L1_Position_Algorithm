#ifndef DF_PARSER_H
#define DF_PARSER_H

/// Maximum number of GPS satellites (PRNs 1–32)
#define MAX_SAT 32

/// Maximum number of signal types in MSM messages (e.g., L1, L2, L5)
#define MAX_SIG 32

/// Maximum number of signal-satellite combinations (cells)
#define MAX_CELL 64

/**
 * @brief Utility macro to extract a field from a line by DF label.
 *
 * This macro simplifies repeated code where a specific DFxxx field value
 * needs to be parsed from a text-formatted RTCM message. It uses `strstr()`
 * to locate the field, and `sscanf()` to extract its value.
 *
 * @param field  The DF field label as a string (e.g., "DF009")
 * @param format The sscanf-compatible format string (e.g., "%lf")
 * @param target A pointer to the variable where the value should be stored
 */
#define EXTRACT(field, format, target)                       \
    do                                                       \
    {                                                        \
        const char *ptr = strstr(line, field "=");           \
        if (ptr)                                             \
            sscanf(ptr + strlen(field) + 1, format, target); \
    } while (0)

/**
 * @brief Data structure for RTCM 1019 (GPS Ephemeris) message.
 *
 * This structure holds the parsed contents of an RTCM 1019 message,
 * which contains broadcast orbital parameters for a GPS satellite.
 * It is typically used for precise satellite position calculation.
 */
typedef struct
{
    uint16_t msg_type;    ///< Message number (should be 1019)
    uint8_t satellite_id; ///< DF009: Satellite PRN
    uint8_t iode;         ///< DF076: Issue of Data (Ephemeris)
    uint8_t ura_index;    ///< DF077: User Range Accuracy index
    uint8_t sv_health;    ///< DF078: SV health status
    double tgd;           ///< DF079: Group delay differential (seconds)
    uint8_t iodc;         ///< DF071: Issue of Data (Clock)
    uint32_t toc;         ///< DF081: Clock data reference time (seconds of GPS week)
    double af2;           ///< DF082: Clock drift rate coefficient (s/s^2)
    double af1;           ///< DF083: Clock drift coefficient (s/s)
    double af0;           ///< DF084: Clock bias (seconds)
    uint16_t week_number; ///< DF085: GPS Week Number (modulo 1024)
    double crs;           ///< DF086: Orbit radius correction, sine term (meters)
    double delta_n;       ///< DF087: Mean motion difference (rad/s)
    double m0;            ///< DF088: Mean anomaly at reference time (radians)
    double cuc;           ///< DF089: Argument of latitude correction, cosine term
    double eccentricity;  ///< DF090: Orbital eccentricity
    double cus;           ///< DF091: Argument of latitude correction, sine term
    double sqrt_a;        ///< DF092: Square root of semi-major axis (sqrt(m))
    uint32_t toe;         ///< DF093: Ephemeris reference time (seconds of GPS week)
    double cic;           ///< DF094: Inclination correction, cosine term
    double omega0;        ///< DF095: Longitude of ascending node (radians)
    double cis;           ///< DF096: Inclination correction, sine term
    double i0;            ///< DF097: Inclination angle at reference time (radians)
    double crc;           ///< DF098: Orbit radius correction, cosine term (meters)
    double omega;         ///< DF099: Argument of perigee (radians)
    double omega_dot;     ///< DF100: Rate of right ascension (rad/s)
    double idot;          ///< DF101: Rate of inclination angle (rad/s)
    uint8_t fit_interval; ///< DF102: Fit interval flag (0=4h, 1=more)
    uint8_t spare;        ///< DF103: Spare bits (unused)
    uint16_t gps_wn;      ///< DF137: Full GPS week number (non-modulo)
} rtcm_1019_ephemeris_t;

/**
 * @brief Data structure for RTCM 1074 (MSM4 GPS L1) message.
 *
 * This structure stores the parsed contents of an MSM4 RTCM message,
 * typically used for GPS L1 pseudorange and carrier phase measurements.
 */
typedef struct
{
    uint16_t msg_type;            ///< DF002: Message number (1074 for GPS MSM4)
    uint16_t station_id;          ///< DF003: Reference station ID
    uint32_t gps_epoch_time;      ///< DF004: Epoch time in milliseconds of the week
    uint8_t msm_sync_flag;        ///< DF393: Epoch sync flag
    uint8_t iods_reserved;        ///< IODS – Issue Of Data Station
    uint8_t reserved_DF001_07;    ///< DF001_7: Reserved for future use
    uint8_t clk_steering_flag;    ///< DF411: Clock steering flag (0=none, 1=steered, 2=unknown, 3=reserved))
    uint8_t external_clk_flag;    ///< DF412: External clock flag (0=internal, 1=external, 2=external but unreliable, 3=unknown))
    uint8_t smooth_interval_flag; ///< DF417: Smoothing type indicator (0=divergence-free smoothing, 1=other smoothing)

    uint64_t sat_mask[64]; ///< DF394: Satellite mask (64 bits bitmap mapping PRNs 1-64))
    uint32_t sig_mask[64]; ///< DF395: Signal mask (32 bits bitmap mapping available signals 1-32)
    uint8_t cell_mask[64]; ///< DF396: Cell mask (bitmap)

    uint8_t n_sat;  ///< Number of satellites used (NSat)
    uint8_t n_sig;  ///< Number of signals used (NSig)
    uint8_t n_cell; ///< Number of satellite-signal combinations (NCell)

    uint8_t cell_prn[MAX_CELL]; ///< Cell PRN mapping (CELLPRN_01, _02, ...)
    uint8_t cell_sig[MAX_CELL]; ///< Cell signal mapping (CELLSIG_01, _02, ...)

    uint8_t prn[MAX_SAT];                 ///< PRN_01..PRN_N: Satellite PRNs
    uint8_t pseudorange_integer[MAX_SAT]; ///< DF397_*: Rough range integer in milliseconds
    double pseudorange_mod_1s[MAX_SAT];   ///< DF398_*: Pseudorange modulo 1 second)
    double pseudorange_fine[MAX_CELL];    ///< DF400: Pseudorange residuals (seconds * c)
    double pseudorange[MAX_SAT];          ///< Pseudorange = integer + mod_1s + fine (seconds * c)
    double phase_range[MAX_CELL];         ///< DF401: Carrier phase residuals (seconds * c)
    uint8_t lock_time[MAX_CELL];          ///< DF402: Lock time indicators
    uint8_t half_cycle_amb[MAX_CELL];     ///< DF420: Half-cycle ambiguity indicators
    uint8_t cnr[MAX_CELL];                ///< DF403: Carrier-to-noise ratio (dBHz scaled)
} rtcm_1074_msm4_t;

/**
 * @brief Parses a line of RTCM 1019 text-formatted input into a structured ephemeris object.
 *
 * @param line The input line containing a text-formatted RTCM 1019 message.
 * @param eph Pointer to the ephemeris structure to be populated.
 * @return 0 on success, non-zero on failure.
 */
int parse_rtcm_1019(const char *line, rtcm_1019_ephemeris_t *eph);

/**
 * @brief Parses a line of RTCM 1074 MSM4 text-formatted input into a structured observation object.
 *
 * @param line The input line containing a text-formatted RTCM 1074 MSM4 message.
 * @param msm4 Pointer to the observation structure to be populated.
 * @return 0 on success, non-zero on failure.
 */
int parse_rtcm_1074(const char *line, rtcm_1074_msm4_t *msm4);
/**
 * @brief Prints the contents of a parsed RTCM 1074 MSM4 observation structure.
 *
 * This function outputs the observation data in a human-readable format,
 * useful for debugging and verification purposes.
 *
 * @param msm4 Pointer to the MSM4 observation structure to print.
 */
void print_msm4(const rtcm_1074_msm4_t *msm4);

/**
 * @brief Prints the contents of a parsed RTCM 1019 ephemeris structure.
 *
 * This function outputs the ephemeris data in a human-readable format,
 * useful for debugging and verification purposes.
 *
 * @param eph Pointer to the ephemeris structure to print.
 */
void print_ephemeris(const rtcm_1019_ephemeris_t *eph);

/**
 * @brief Computes the pseudorange from the given parameters.
 *
 * This function calculates the pseudorange using the formula:
 * Pseudorange = c * (integer_ms * 1e-3 + mod1s_sec + fine_sec)
 *
 * @param integer_ms Rough range integer in milliseconds.
 * @param mod1s_sec Pseudorange modulo 1 second.
 * @param fine_sec Pseudorange residuals in seconds scaled by speed of light.
 * @return Computed pseudorange in seconds.
 */
double compute_pseudorange(uint32_t integer_ms, double mod1s_sec, double fine_sec);

#endif // DF_PARSER_H
