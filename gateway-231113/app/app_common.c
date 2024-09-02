#include "app_common.h"
#include <sys/time.h>
#include <stdlib.h>

long app_common_getCurrentTimestamp()
{
    struct timeval timev;
    gettimeofday(&timev, NULL);
    return timev.tv_sec * 1000 + timev.tv_usec / 1000;
}