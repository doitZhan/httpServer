#include "timestamp.h"

#include <sys/time.h>
#include <stddef.h>

timestamp timestamp::now(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return timestamp(seconds * kMicroSecPerSecond + tv.tv_usec);
}
