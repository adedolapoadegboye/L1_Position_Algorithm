#ifndef RTCM_READER_H
#define RTCM_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_next_rtcm_message(FILE *fp);

#endif // RTCM_READER_H
