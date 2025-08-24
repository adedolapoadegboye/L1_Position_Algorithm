# --- files ---
satfile = 'plots/sat_track_ecef.dat'        # PRN X Y Z

# ---------- 1 Global orbits with true sphere ----------
unset parametric
stats satfile using 2 nooutput ; sxmin = STATS_min ; sxmax = STATS_max
stats satfile using 3 nooutput ; symin = STATS_min ; symax = STATS_max
stats satfile using 4 nooutput ; szmin = STATS_min ; szmax = STATS_max

# One symmetric cube for all axes
absmax(x,y) = (abs(x)>abs(y)?abs(x):abs(y))
A = absmax( absmax(sxmin,sxmax), absmax( absmax(symin,symax), absmax(szmin,szmax) ) )
A = (A < 3.0e7 ? 3.0e7 : A)   # floor at 30,000 km for clean view

set terminal pngcairo size 1600,1200 enhanced
set output 'plots/gnss_orbits.png'

set view 70, 230
set view equal xyz
set xrange [-A:A]
set yrange [-A:A]
set zrange [-A:A]

set xlabel 'X (m)'; set ylabel 'Y (m)'; set zlabel 'Z (m)'
set ticslevel 0
set grid
set hidden3d
unset key

set title "GNSS Satellite Orbits (ECEF frame)" font ",18"

# Earth wireframe sphere
set parametric
set urange [0:2*pi]
set vrange [0:pi]
set isosamples 64, 32
R = 6378137.0

# Palette for PRN coloring
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
  satfile using 2:3:4:(int($1)%8) with points pt 7 ps 0.35 lc palette

unset output
unset parametric
