#include "Timestamp.h"

#include <sys/time.h>
#include <stddef.h>

Timestamp Timestamp::now(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecPerSecond + tv.tv_usec);
}

std::string Timestamp::toString() const{
    char buf[32] = {0};
    int64_t seconds = microSecSinceEpoch_ / kMicroSecPerSecond;
    int64_t microseconds = microSecSinceEpoch_ % kMicroSecPerSecond;
    //跨平台的书写方式，主要是为了同时支持32位和64位操作系统
    snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}