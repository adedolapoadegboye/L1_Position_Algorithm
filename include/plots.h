#ifndef PLOTS_H
#define PLOTS_H

int write_receiver_track_ecef(const char *path, int n_epochs);
int write_sat_orbits(const char *path);
int write_receiver_track_geo(const char *path, int n_epochs);

#endif // PLOTS_H
