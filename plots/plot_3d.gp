# --- files ---
satfile = 'plots/sat_track_ecef.dat'        # PRN X Y Z
rcvfile = 'plots/receiver_track_ecef.dat'   # X Y Z

# ========== 1 ORBITS (tight cube) ==========
# Inspect satellite extents (XYZ are cols 2/3/4)
unset parametric
stats satfile using 2 nooutput
sxmin = STATS_min; sxmax = STATS_max
stats satfile using 3 nooutput
symin = STATS_min; symax = STATS_max
stats satfile using 4 nooutput
szmin = STATS_min; szmax = STATS_max

# Manual tightened ranges (based on your data), still override-able:
xr_lo = -2.5e7; xr_hi =  2.5e7
yr_lo = -2.7e7; yr_hi =  8.0e6
zr_lo =  6.0e6; zr_hi =  2.2e7

set terminal pngcairo size 1600,1200 enhanced
set output 'plots/gnss_orbits.png'

set view 70, 230
set view equal xyz
set xrange [xr_lo:xr_hi]
set yrange [yr_lo:yr_hi]
set zrange [zr_lo:zr_hi]

set xlabel 'X (m)'; set ylabel 'Y (m)'; set zlabel 'Z (m)'
set ticslevel 0
set grid
set hidden3d
unset key

# Earth sphere (wireframe)
set parametric
set urange [0:2*pi]
set vrange [0:pi]
set isosamples 48,24
R = 6378137.0

# Simple palette for PRN coloring
set palette model RGB defined ( \
    0 0.20 0.60 1.00, \
    1 0.30 0.80 0.40, \
    2 0.95 0.50 0.10, \
    3 0.80 0.20 0.80, \
    4 0.90 0.20 0.20, \
    5 0.20 0.80 0.80, \
    6 0.60 0.60 0.20, \
    7 0.50 0.50 0.90 )

splot \
  R*cos(u)*sin(v), R*sin(u)*sin(v), R*cos(v) with lines lc rgb "#3cb4b4" lw 1, \
  satfile using 2:3:4:(int($1)%8) with lines lc palette lw 1, \
  satfile using 2:3:4:(int($1)%8) with points pt 7 ps 0.35 lc palette, \
  rcvfile using 1:2:3 with lines lc rgb "#ff006e" lw 3

unset output
unset parametric

# ========== 2 RECEIVER ZOOM (near-surface) ==========
# Auto-range around the receiver track with padding
stats rcvfile using 1 nooutput
rxmin = STATS_min; rxmax = STATS_max
stats rcvfile using 2 nooutput
rymin = STATS_min; rymax = STATS_max
stats rcvfile using 3 nooutput
rzmin = STATS_min; rzmax = STATS_max

pad = 3.0e5   # 300 km padding around the track (tweak as you like)

set terminal pngcairo size 1600,1200 enhanced
set output 'plots/gnss_receiver_zoom.png'

set view 70, 230
set view equal xyz
set xrange [rxmin-pad:rxmax+pad]
set yrange [rymin-pad:rymax+pad]
set zrange [rzmin-pad:rzmax+pad]

set xlabel 'X (m)'; set ylabel 'Y (m)'; set zlabel 'Z (m)'
set ticslevel 0
set grid
set hidden3d
unset key

# Just the receiver (no sphere to keep it uncluttered at this scale)
splot rcvfile using 1:2:3 with lines lc rgb "#ff006e" lw 3

unset output
