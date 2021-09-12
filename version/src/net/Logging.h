/*
    日志类。
    使用举例:#define LOG_TRACE if (Logger::logLevel() <= Logger::TRACE) Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()
    使用该LOG_TRACE宏时会先进行判断，如果日志级别大于TRACE级别，后面那句不会被执行，也就是不会打印TRACE级别的信息。
    用法：LOG_TRACE << "TRACE ..."; 相当于使用Logger(__FILE__, __LINE__).stream() << "Info";
    Logger  --> impl  --> LogStream  --> operator<<  --> FixedBuffer --> g_output --> g_flush。
    我们在使用LOG_TRACE时，由于宏的替换，首先会构造一个无名临时Logger对象，然后调用该对象的stream()方法，该方法返回了在内部类impl中的成员缓冲区LogStream成员，这个成员针对所有类型重载了<<符号，我们写上“trace ..."会调用该缓冲区的operator<<方法，把信息输入到该缓冲区中去。那么什么时候缓冲区会输出呢?
    因为我们生成的无名临时对象，当无名临时对象析构的时候~Logger()中会调用g_output全局函数，该函数是一个回调函数，如果用户不主动更改，该函数默认会将缓冲区中的内容输出到stdout。以上就是Logger类基本的工作机制。
*/

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "LogStream.h"
#include "Timestamp.h"

class Logger{
public:
    // 日志级别
    enum LogLevel{
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    // 获取文件基本名称类
    class SourceFile{
    public:
        template<int N>
        inline SourceFile(const char (&arr)[N])
            :data_(arr),
            size_(N - 1){
                const char* slash = strrchr(data_, '/');
                if (slash){
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }
        explicit SourceFile(const char* filename)
            :data_(filename){
                const char* slash = strrchr(filename, '/');
                if(slash){
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }

        const char* data_;
        int size_;
    };
    
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream& stream(){
        return impl_.stream_;
    }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel logLevel);

    typedef void (*OutputFunc)(const char* msg, int len);    //输出函数指针
    typedef void (*FlushFunc)();    //刷新函数指针

    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private:
    class Impl{    //Logger类内部的一个嵌套类，封装了Logger的缓冲区stream_
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);  //日志级别 错误 文件 行
        void formatTime();
        void finish();
        
        Timestamp time_;    //当前时间
        LogStream stream_;    //构造日志缓冲区，该缓冲区重载操作符<<，都是将数据格式到LogStream的内部成员缓冲区buffer里
        LogLevel level_;    //级别
        int line_;    //行
        SourceFile basename_;    //基本名称
    };
    
    Impl impl_;     //Logger构造这个impl实现类
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel(){    //内联静态成员函数，返回当前日志级别
    return g_logLevel;
}

#define LOG_TRACE if (Logger::logLevel() <= Logger::TRACE) Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO if (Logger::logLevel() <= Logger::INFO) Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

#define CHECK_NOTNULL(val) CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr){
    if(ptr == NULL){
        Logger(file, line, Logger::FATAL).stream() << names;
    }
    return ptr;
}



#endif