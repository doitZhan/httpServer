#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "LogStream.h"
#include "Timestamp.h"

class Logger{
public:
    // level form low to high
    enum LogLevel{
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    // get the basename of cource file at the compile time
    // avoid the cost of strrchr(3) the running time
    class SourceFile{
    public:
        template<int N>
        inline SourceFile(const char (&arr)[N])
            :data_(arr),
            size_(N - 1){
                const char* slash = strrchr(data_, '/');
                if (slash){
                    data_ = slash + 1;
                    size_ = static_cast<int>(data_ - arr);
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

    typedef void (*OutputFunc)(const char* msg, int len);
    typedef void (*FlushFunc)();

    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private:
    class Impl{
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
        void formatTime();
        void finish();
        
        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };
    
    Impl impl_;
};





#endif