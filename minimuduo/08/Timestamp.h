#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <inttypes.h>
#include <string>

class Timestamp{
public:
    Timestamp()
        :microSecSinceEpoch_(0){
    }
    
    explicit Timestamp(int64_t microSecSinceEpoch)
        :microSecSinceEpoch_(microSecSinceEpoch){

    }
    ~Timestamp() = default;

    int64_t microSecSinceEpoch()const{
        return microSecSinceEpoch_;
    }

    bool operator<(const Timestamp& rhs)const{
        return this->microSecSinceEpoch() < rhs.microSecSinceEpoch();
    }

    bool operator==(const Timestamp& rhs)const{
        return this->microSecSinceEpoch() == rhs.microSecSinceEpoch();
    }

    std::string toString() const;
    // for AsyncLogging
    std::string toFormattedString() const;
    bool valid() const{
        return microSecSinceEpoch_ > 0;
    }

    static Timestamp now();
    static Timestamp invalid(){
        return Timestamp();
    }

    static const int kMicroSecPerSecond = 1000 * 1000;
private:
    int64_t microSecSinceEpoch_;
};

inline Timestamp addTime(Timestamp timestamp, double seconds){
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecPerSecond);
    return Timestamp(timestamp.microSecSinceEpoch() + delta);
}


#endif