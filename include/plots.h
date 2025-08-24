#ifndef PLOTS_H
#define PLOTS_H

int write_receiver_track_ecef(const char *path, int n_epochs);
int write_sat_orbits(const char *path);
int write_receiver_track_geo(const char *path, int n_epochs);
int write_receiver_ecef_epoch_km(const char *path, int n_epochs);
int write_sat_xyz_km(const char *path);
int write_pseudorange_time_km(const char *path);

#endif // PLOTS_H
