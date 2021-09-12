/*
    日志文件类。用来控制日志怎样和文件打交道。几天写一次？多少行写一次？
    日志滚动：
    (1)文件大小，例如每天写满1G换下一个文件
    (2)时间（每天零点新建一个日志文件，不论前一个文件是否写满）
*/


#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#include "Mutex.h"
#include "nonCopyable.h"
#include "FileUtil.h"

#include <memory>
#include <string>

class AppendFile;

class LogFile: public nonCopyable{
public:
    LogFile(const std::string& basename, int rollSize, bool threadSafe = true, int flushInterval = 3, int checkEvenyN = 1014);
    ~LogFile();

    void append(const char* logFile, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);     //不加锁的append
    static std::string getLogFileName(const std::string basename, time_t* now);    //获取日志文件名称

    const std::string basename_;    //日志文件基本名称
    const int rollSize_;    //日志文件大小达到rollSize_生成一个新文件
    const int flushInterval_;   //日志写入间隔时间
    const int checkEveryN_;
    
    int count_;     //计数器，检测是否需要更换新文件

    const std::unique_ptr<MutexLock> mutex_;    //锁
    std::unique_ptr<fileutil::AppendFile> file_;      //文件智能指针

    time_t startOfPeriod_;      //开始记录日志时间
    time_t lastRoll_;   //上一次滚动日志文件时间
    time_t lastFlush_;     //上一次写入日志文件时间
    const static int kRollPerSeconds_ = 60 * 60 * 24;   //一天时间秒数
};

#endif
