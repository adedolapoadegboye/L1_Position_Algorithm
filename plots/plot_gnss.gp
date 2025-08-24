set terminal pngcairo size 1400,1000
set output 'gnss_3d.png'

set view 60, 30
set xlabel 'X (m)'; set ylabel 'Y (m)'; set zlabel 'Z (m)'
set size ratio -1
set ticslevel 0
set grid
unset key
set hidden3d

# Earth sphere
set parametric
set urange [0:2*pi]
set vrange [0:pi]
set isosamples 40, 20
R = 6371000.0   # mean Earth radius for visualization

splot R*cos(u)*sin(v), R*sin(u)*sin(v), R*cos(v) w lines lt 1 lw 1, \
      'sat_orbits.dat' u 2:3:4 w l lt 3 lw 1, \
      'receiver_track.dat' u 1:2:3 w l lt 7 lw 2
