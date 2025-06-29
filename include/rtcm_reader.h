#ifndef RTCM_READER_H
#define RTCM_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    unsigned char *buffer;
    size_t length;
} RTCM_Message;

int read_next_rtcm_message(FILE *fp, RTCM_Message *message);

#endif // RTCM_READER_H
