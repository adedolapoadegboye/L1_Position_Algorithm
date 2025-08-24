set term pngcairo size 1600,1000 enhanced font ",16"

# ---------- Plot 1: Receiver ECEF components vs epoch ----------
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


# ---------- Plot 2: Receiver Latitude vs Longitude ----------
set output 'plots/receiver_latlon.png'
set title 'Receiver Latitude vs Longitude (per epoch)'
set xlabel 'Longitude (deg)'
set ylabel 'Latitude (deg)'
set grid
set size ratio -1   # enforce equal scaling on X and Y so the track shape is preserved
plot \
  'plots/receiver_track_geo.dat' using 2:1 with linespoints lc rgb '#ff006e' lw 2 pt 7 ps 0.6 title 'Track'



# ---------- Plot 3: Pseudorange vs time ----------
set output 'plots/pseudorange_time.png'
set title 'Pseudorange vs time'
set xlabel 'Time of week (ms)'
set ylabel 'Pseudorange (km)'
set grid
unset key
plot 'plots/pseudorange_time_km.dat' using 2:3:1 with points pt 7 ps 0.25 lc palette
