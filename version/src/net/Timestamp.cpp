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
    //跨平台的书写方式，主要是为了同时支持32位和64位操作系统，%06输出6位十进制整数 左边补0
    snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

std::string Timestamp::toFormattedString() const{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecSinceEpoch_ / kMicroSecPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    int microseconds = static_cast<int>(microSecSinceEpoch_ % kMicroSecPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d", tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);

    return buf;
}