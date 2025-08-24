/**
 * @file receiver_position.c
 * @brief Estimate GNSS receiver positions using pseudorange measurements.
 *
 * This module implements the C equivalent of the Python `estimate_positions`
 * function. It:
 *  - Collects unique pseudorange epochs across all satellites
 *  - Aligns satellite ECEF positions to the pseudorange epochs
 *  - Runs an iterative least-squares solver (Newton method) to estimate
 *    receiver position and clock bias per epoch
 *
 * The output is stored in `estimated_positions_ecef` as ECEF coordinates.
 *
 * @note This implementation uses normal equations with a 4x4 matrix inverse
 *       for the pseudoinverse step. It mirrors the Python logic, with optional
 *       diagnostic prints disabled by default.
 *
 * @author Ade
 * @date   2025-08-23
 */

// --- EPOCH COLLECTION + PER-EPOCH GATHER & LSQ (mirror of your Python) ---

#include "../include/satellites.h"
#include "../include/receiver.h"
#include "../include/algo.h"

extern gps_satellite_data_t gps_list[MAX_SAT + 1];
extern sat_ecef_history_t sat_ecef_positions[MAX_SAT + 1];
extern size_t pseudorange_count[MAX_SAT + 1];
estimated_position_t estimated_positions_ecef = {0};
latlonalt_position_t latlonalt_positions = {0};
int n_times = 0; // total epochs found during position estimation

#define ITERATIONS 10

/* ---------- tiny helpers kept local for clarity ---------- */

static int cmp_u32(const void *a, const void *b)
{
    uint32_t A = *(const uint32_t *)a, B = *(const uint32_t *)b;
    return (A > B) - (A < B);
}

static inline int pr_count_for_prn(int prn)
{
    int n_pr = (int)pseudorange_count[prn];
    if (n_pr > 0 && n_pr <= MAX_EPOCHS)
        return n_pr;

    int c = 0;
    for (int k = 0; k < MAX_EPOCHS; ++k)
        if (gps_list[prn].times_of_pseudorange[k] != 0)
            c++;
    return c;
}

static int collect_unique_pr_times_ms(const gps_satellite_data_t glist[MAX_SAT + 1],
                                      uint32_t out_times_ms[MAX_UNIQUE_EPOCHS])
{
    size_t n = 0;

    for (int prn = 1; prn <= MAX_SAT; prn++)
    {
        int n_pr = pr_count_for_prn(prn);
        for (int k = 0; k < n_pr; k++)
        {
            uint32_t t = glist[prn].times_of_pseudorange[k];
            if (t == 0)
                continue;
            if (n >= (size_t)MAX_UNIQUE_EPOCHS)
                goto SORT_AND_DEDUP;
            out_times_ms[n++] = t;
        }
    }

SORT_AND_DEDUP:
    if (n == 0)
        return 0;

    qsort(out_times_ms, n, sizeof(uint32_t), cmp_u32);

    size_t w = 1;
    for (size_t i = 1; i < n; i++)
    {
        if (out_times_ms[i] != out_times_ms[w - 1])
            out_times_ms[w++] = out_times_ms[i];
    }
    return (int)w;
}

static inline double norm3(const double v[3])
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/* 4x4 inverse (Gauss–Jordan, partial pivoting) */
static int invert_4x4(const double A[4][4], double inv[4][4])
{
    double aug[4][8];
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
            aug[r][c] = A[r][c];
        for (int c = 0; c < 4; ++c)
            aug[r][4 + c] = (r == c) ? 1.0 : 0.0;
    }
    for (int col = 0; col < 4; ++col)
    {
        int piv = col;
        double maxabs = fabs(aug[piv][col]);
        for (int r = col + 1; r < 4; ++r)
        {
            double v = fabs(aug[r][col]);
            if (v > maxabs)
            {
                maxabs = v;
                piv = r;
            }
        }
        if (maxabs <= 1e-18)
            return 0;
        if (piv != col)
        {
            for (int c = 0; c < 8; ++c)
            {
                double tmp = aug[col][c];
                aug[col][c] = aug[piv][c];
                aug[piv][c] = tmp;
            }
        }
        double invpiv = 1.0 / aug[col][col];
        for (int c = 0; c < 8; ++c)
            aug[col][c] *= invpiv;
        for (int r = 0; r < 4; ++r)
        {
            if (r == col)
                continue;
            double f = aug[r][col];
            if (f != 0.0)
                for (int c = 0; c < 8; ++c)
                    aug[r][c] -= f * aug[col][c];
        }
    }
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            inv[r][c] = aug[r][4 + c];
    return 1;
}

/* delta(4) = pinv(G)(4xm) * y(m) via normal equations */
static int pinv_normal_eq_apply(int m,           /* rows of G */
                                double G[][4],   /* m x 4 */
                                const double *y, /* m x 1 */
                                double out[4])   /* 4 x 1 */
{
    /* Compute AT = G^T (4 x m) and ATA = G^T G (4x4), ATy = G^T y (4) */
    double AT[4][MAX_SAT];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < m; ++j)
            AT[i][j] = G[j][i];

    double ATA[4][4] = {{0}};
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
        {
            double acc = 0.0;
            for (int k = 0; k < m; ++k)
                acc += AT[r][k] * G[k][c];
            ATA[r][c] = acc;
        }

    double ATy[4] = {0, 0, 0, 0};
    for (int r = 0; r < 4; ++r)
    {
        double acc = 0.0;
        for (int k = 0; k < m; ++k)
            acc += AT[r][k] * y[k];
        ATy[r] = acc;
    }

    double invATA[4][4];
    if (!invert_4x4(ATA, invATA))
        return 0;

    for (int r = 0; r < 4; ++r)
    {
        double acc = 0.0;
        for (int c = 0; c < 4; ++c)
            acc += invATA[r][c] * ATy[c];
        out[r] = acc;
    }
    return 1;
}

/* ---------- main function ---------- */
int estimate_receiver_positions(void)
{
    /* 1) Epoch collection (np.unique over all PR times) */
    uint32_t *all_times = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)MAX_UNIQUE_EPOCHS);
    if (!all_times)
    {
        perror("malloc(all_times)");
        return -1;
    }
    for (int i = 0; i < MAX_UNIQUE_EPOCHS; ++i)
        all_times[i] = 0;

    n_times = collect_unique_pr_times_ms(gps_list, all_times);
    printf("[C] total epochs = %d\n", n_times);
    if (n_times > 0 && n_times <= 20)
    {
        printf("[C] epochs (ms): ");
        for (int i = 0; i < n_times; ++i)
            printf("%u%s", all_times[i], (i + 1 < n_times) ? ", " : "\n");
    }

    /* 2) Per-SV summary (PR samples, ECEF shape, first/last PR time) */
    for (int prn = 1; prn <= MAX_SAT; ++prn)
    {
        int pr_cnt = pr_count_for_prn(prn);
        int ecef_rows = 0;
        for (int k = 0; k < MAX_EPOCHS; ++k)
            if (sat_ecef_positions[prn].t_ms[k] != 0.0)
                ecef_rows++;

        if (pr_cnt > 0)
        {
            /* first/last PR time within recorded range */
            int first_idx = -1, last_idx = -1;
            for (int k = 0; k < MAX_EPOCHS; ++k)
            {
                if (gps_list[prn].times_of_pseudorange[k] != 0)
                {
                    first_idx = k;
                    break;
                }
            }
            for (int k = MAX_EPOCHS - 1; k >= 0; --k)
            {
                if (gps_list[prn].times_of_pseudorange[k] != 0)
                {
                    last_idx = k;
                    break;
                }
            }
            uint32_t first_t = (first_idx >= 0) ? gps_list[prn].times_of_pseudorange[first_idx] : 0u;
            uint32_t last_t = (last_idx >= 0) ? gps_list[prn].times_of_pseudorange[last_idx] : 0u;

            printf("[C] SV %02d: PR samples=%d, ECEF shape=(%d,3); first PR time=%u, last PR time=%u\n",
                   prn, pr_cnt, ecef_rows, first_t, last_t);
        }
        else
        {
            // printf("[C] SV %02d: ECEF shape=(%d,3)\n", prn, ecef_rows);
        }
    }

    /* 3) Process each epoch independently */
    for (int ti = 0; ti < n_times; ++ti)
    {
        uint32_t t = all_times[ti];

        int svs[MAX_SAT];
        int pr_indices[MAX_SAT];
        double ecefs[MAX_SAT][3];
        double pseudoranges[MAX_SAT];
        int n_svs = 0;

        /* Gather same-time measurements (first match per SV) */
        for (int prn = 1; prn <= MAX_SAT && n_svs < MAX_SAT; ++prn)
        {
            int n_pr = pr_count_for_prn(prn);
            for (int k = 0; k < n_pr; ++k)
            {
                if (gps_list[prn].times_of_pseudorange[k] != t)
                    continue;

                svs[n_svs] = prn;
                pr_indices[n_svs] = k;
                ecefs[n_svs][0] = sat_ecef_positions[prn].x[k];
                ecefs[n_svs][1] = sat_ecef_positions[prn].y[k];
                ecefs[n_svs][2] = sat_ecef_positions[prn].z[k];
                pseudoranges[n_svs] = gps_list[prn].pseudoranges[k];
                n_svs++;
                break;
            }
        }

        if (n_svs < 4)
            continue;

        /* --- Iterative least-squares (Newton) --- */
        double assumed_pos[3] = {0.0, 0.0, 0.0};
        double clock_bias = 0.0;

        for (int it = 0; it < ITERATIONS; ++it)
        {
            double assumed_ranges[MAX_SAT];
            double unit_vectors[MAX_SAT][3];
            double delta_tau[MAX_SAT];

            for (int i = 0; i < n_svs; ++i)
            {
                double los[3] = {
                    ecefs[i][0] - assumed_pos[0],
                    ecefs[i][1] - assumed_pos[1],
                    ecefs[i][2] - assumed_pos[2]};
                double r = norm3(los);
                if (!(r > 0.0) || !isfinite(r))
                    r = 1.0;

                assumed_ranges[i] = r;
                unit_vectors[i][0] = los[0] / r;
                unit_vectors[i][1] = los[1] / r;
                unit_vectors[i][2] = los[2] / r;

                delta_tau[i] = pseudoranges[i] - r - clock_bias;
            }

            /* Geometry matrix G (n_svs x 4) */
            double G[MAX_SAT][4];
            for (int i = 0; i < n_svs; ++i)
            {
                G[i][0] = -unit_vectors[i][0];
                G[i][1] = -unit_vectors[i][1];
                G[i][2] = -unit_vectors[i][2];
                G[i][3] = 1.0;
            }

            /* delta = pinv(G) * delta_tau  (4x1) */
            double delta_pos_time[4];
            if (!pinv_normal_eq_apply(n_svs, G, delta_tau, delta_pos_time))
            {
                /* singular / ill-conditioned; skip this epoch */
                n_svs = 0; /* to avoid storing a bogus result */
                break;
            }

            assumed_pos[0] += delta_pos_time[0];
            assumed_pos[1] += delta_pos_time[1];
            assumed_pos[2] += delta_pos_time[2];
            clock_bias += delta_pos_time[3];

            // Optional prints:
            double rms = 0.0;
            for (int i = 0; i < n_svs; ++i)
                rms += delta_tau[i] * delta_tau[i];
            rms = sqrt(rms / n_svs);
            // printf("[C][epoch %d][iter %d] rms=%.3f, clk=%.6f\n", ti, it, rms, clock_bias);
        }

        if (n_svs >= 4)
        {
            /* store final ECEF estimate (per epoch index ti) */
            estimated_positions_ecef.x[ti] = assumed_pos[0];
            estimated_positions_ecef.y[ti] = assumed_pos[1];
            estimated_positions_ecef.z[ti] = assumed_pos[2];
            // printf("[C][epoch %d] FINAL pos ECEF = (%.3f, %.3f, %.3f) m, clock_bias=%.6f m\n",
            //        ti, estimated_positions_ecef.x[ti], estimated_positions_ecef.x[ti], estimated_positions_ecef.x[ti], clock_bias);

            /* --- Convert to geodetic and store --- */
            if (ti < MAX_EPOCHS)
            {
                double lat_deg, lon_deg, alt_m;
                ecef_to_geodetic(assumed_pos[0], assumed_pos[1], assumed_pos[2],
                                 &lat_deg, &lon_deg, &alt_m);

                /* single instance: store by epoch index */
                latlonalt_positions.lat[ti] = lat_deg;
                latlonalt_positions.lon[ti] = lon_deg;

                /* optional print (comment out if noisy) */
                printf("[C][epoch %d] LLA = (lat=%.8f deg, lon=%.8f deg)\n",
                       ti, lat_deg, lon_deg);
            }
        }
    }

    free(all_times);
    return 0;
}
