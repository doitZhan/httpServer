#include "LogFile.h"
#include "FileUtil.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

LogFile::LogFile(const std::string& basename, int rollSize, bool threadSafe, int flushInterval, int checkEveryN)
    :basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),  //不是线程安全就不需要构造mutex_
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0){
        assert(basename.find('/') == std::string::npos);    //断言basename不包含'/'
        rollFile();     //初始化一个日志文件
}

LogFile::~LogFile(){

}

void LogFile::append(const char* logline, int len){     //将一行长度为len添加到日志文件中
    if(mutex_){
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else{
        append_unlocked(logline, len);
    }
}


void LogFile::flush(){
    if(mutex_){
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else{
        file_->flush();
    }
}

//利用AppendFile类完成向文件中写操作
void LogFile::append_unlocked(const char* logline, int len){
    //向文件中写入长度为len的logline字符串
    file_->append(logline, len);

    if(file_->writtenBytes() > rollSize_){  //写入字节数大小超过滚动大小
        rollFile();     //滚动
    }
    else{
        ++count_;   //增加行数
        if(count_ >= checkEveryN_){
            //此时重置count_,再次回滚文件
            count_ = 0;
            time_t now = time(NULL);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;  //先乘再除代表把时间now换算成一天的整数倍
            if(thisPeriod != startOfPeriod_){   //不相等，意思就是该日志文件start已经超过一天了，滚动
                rollFile();
            }
            else if(now - lastFlush_ > flushInterval_){     //判断是否超过flush时间间隔
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

//回滚日志文件,功能是根据当前时间now设置lastRoll_,lastFlush_,startOfPeriod_
//并把file_重新指向一个新的AppendFile(完整日志文件名字)
bool LogFile::rollFile(){
    time_t now = 0;
    //filename格式 test.txt.20200825-064633.master.22895.log
    std::string filename = getLogFileName(basename_, &now);     //生成一个日志文件名称
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if(now > lastRoll_){    //如果大于上一次lastRoll_，开始回滚
        lastRoll_ = now;
        lastFlush_= now;
        startOfPeriod_ = start;
        //file_重新指向一个新的AppendFile对象,打开的文件为filename
        //即完整的日志文件名字,如果文件不存在则新创建一个文件
        file_.reset(new fileutil::AppendFile(filename));  //重新生成一个日志文件
        return true;
    }
    return false;
}

std::string hostname(){
    // HOST_NAME_MAX 64
    // _POSIX_HOST_NAME_MAX 255
    char buf[256];
    if(gethostname(buf, sizeof(buf)) == 0){
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else{
        return "unkownhost";
    }
}

// nameOfProcess(basename).fileCreateTime(YMD-HMS).hostname.idOfProcess.log
std::string LogFile::getLogFileName(const std::string basename, time_t* now){
    std::string filename;
    filename.reserve(basename.size() + 64);     //预分配内存
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm);
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    // filename += ProcessInfo::hostname();
    filename += hostname();
    

    char pidbuf[32];
    // snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    snprintf(pidbuf, sizeof(pidbuf), ".%d", CurrentThread::pid());
    filename += pidbuf;

    filename += ".log";

    return filename;
}

