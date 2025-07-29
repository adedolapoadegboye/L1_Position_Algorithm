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
    uint16_t msg_type;         ///< DF002: Message number (should be 1019)
    uint8_t satellite_id;      ///< DF009: Satellite PRN (space vehicle ID)
    uint16_t gps_wn;           ///< DF076: GPS week number (modulo 1024)
    uint8_t gps_sv_acc;        ///< DF077: SV accuracy index (URA)
    uint8_t gps_code_l2;       ///< DF078: GPS code on L2 (00=reserved, 01=P code on, 10=C/A code on, 11=L2C on)
    double gps_idot;           ///< DF079: GPS IDOT
    uint16_t gps_iode;         ///< DF071 GPS IODE
    uint32_t gps_toc;          ///< DF081: GPS time of clock (s of GPS week)
    double gps_af2;            ///< DF082: Polynomial clock drift coefficient (s/s^2)
    double gps_af1;            ///< DF083: Polynomial clock drift coefficient (s/s)
    double gps_af0;            ///< DF084: Clock bias (s)
    uint16_t gps_iodc;         ///< DF071 GPS IODC
    double gps_crs;            ///< DF086: Radius correction sine term (m)
    double gps_delta_n;        ///< DF087: Mean motion difference from computed value (rad/s)
    double gps_m0;             ///< DF088: Mean anomaly at reference time (rad)
    double gps_cuc;            ///< DF089: Latitude correction cosine term (rad)
    double gps_eccentricity;   ///< DF090: Eccentricity
    double gps_cus;            ///< DF091: Latitude correction sine term (rad)
    double gps_sqrt_a;         ///< DF092: Square root of semi-major axis (sqrt(m))
    uint32_t gps_toe;          ///< DF093: Time of ephemeris (s of GPS week)
    double gps_cic;            ///< DF094: Inclination correction cosine term (rad)
    double gps_omega0;         ///< DF095: Longitude of ascending node at weekly epoch (rad)
    double gps_cis;            ///< DF096: Inclination correction sine term (rad)
    double gps_i0;             ///< DF097: Inclination angle at reference time (rad)
    double gps_crc;            ///< DF098: Radius correction cosine term (m)
    double gps_omega;          ///< DF099: Argument of perigee (rad)
    double gps_omega_dot;      ///< DF100: Rate of right ascension (rad/s)
    double gps_tgd;            ///< DF101: Group delay differential (s)
    uint8_t gps_sv_health;     ///< DF102: SV health status (0=healthy, 1=unhealthy, 2=unknown)
    uint8_t gps_l2p_data_flag; ///< DF103: L2 P data flag (0=L2P P-code nav data available, 1=L2P P-code nav data unavailable
    uint16_t gps_fit_interval; ///< DF137: GPS fit interval (0=not fit, 1=fit, 2=unknown)
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

int store_ephemeris(const rtcm_1019_ephemeris_t *new_eph);

#endif // DF_PARSER_H
