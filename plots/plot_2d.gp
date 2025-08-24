set term pngcairo size 1600,1000 enhanced font ",16"

# ---------- Plot 1: Receiver ECEF components vs time ----------
set output 'plots/receiver_ecef_epoch.png'
set title 'Receiver ECEF components vs epoch'
set xlabel 'Epoch (per sec)'
set ylabel 'ECEF (km)'
set grid
unset key
plot \
  'plots/receiver_ecef_epoch.dat' using 1:2 with lines lc rgb '#1f77b4' lw 2 title 'X', \
  '' using 1:3 with lines lc rgb '#2ca02c' lw 2 title 'Y', \
  '' using 1:4 with lines lc rgb '#d62728' lw 2 title 'Z'



# ---------- Plot 2: Satellite ECEF XYZ (colored by PRN) ----------
set terminal pngcairo size 1600,1200 enhanced
set output 'plots/sat_xyz_km.png'

set title 'Satellite ECEF XYZ (colored by PRN)'
set xlabel 'X (km)'
set ylabel 'Y (km)'
set zlabel 'Z (km)'
set view 70, 230
set view equal xyz
set grid
set hidden3d
unset key
set ticslevel 0

# (optional) ranges—tune if you want tighter framing
#set xrange [-26000:26000]
#set yrange [-26000:26000]
#set zrange [-26000:26000]

# Color by PRN (wrap into 8 colors)
set palette model RGB defined ( \
    0 0.20 0.60 1.00, \
    1 0.30 0.80 0.40, \
    2 0.95 0.50 0.10, \
    3 0.80 0.20 0.80, \
    4 0.90 0.20 0.20, \
    5 0.20 0.80 0.80, \
    6 0.60 0.60 0.20, \
    7 0.50 0.50 0.90 )
set cblabel 'PRN (mod 8)'

splot 'plots/sat_xyz_km.dat' using 2:3:4:(int($1)%8) \
      with points pt 7 ps 0.35 lc palette


# ---------- Plot 3: Pseudorange vs time ----------
set output 'plots/pseudorange_time.png'
set title 'Pseudorange vs time'
set xlabel 'Time of week (ms)'
set ylabel 'Pseudorange (km)'
set grid
unset key
plot 'plots/pseudorange_time_km.dat' using 2:3:1 with points pt 7 ps 0.25 lc palette
