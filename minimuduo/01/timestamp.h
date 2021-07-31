#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <inttypes.h>

class timestamp{
public:
    timestamp()
        :microSecSinceEpoch_(0){
    }
    
    explicit timestamp(int64_t microSecSinceEpoch)
        :microSecSinceEpoch_(microSecSinceEpoch){

    }
    ~timestamp() = default;

    static timestamp now();
    static const int kMicroSecPerSecond = 1000 * 1000;
private:
    int64_t microSecSinceEpoch_;
};



#endif