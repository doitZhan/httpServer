#include "Logging.h"
#include "CurrentThread.h"
#include <assert.h>


__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno){
    return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
}

Logger::LogLevel initLogLevel(){    //初始化日志级别，默认为INFO
    if (getenv("LOG_TRACE")){
        return Logger::TRACE;
    }
    else if (getenv("LOG_DEBUG")){
        return Logger::DEBUG;
    }
    else{
        return Logger::INFO;
    }
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ", 
};

class T{    //编译时获取字符串长度的类
public:
    T(const char* str, unsigned len)
        :str_(str),
        len_(len){
            assert(strlen(str) == len_);
        }

    const char* str_;
    const unsigned len_;
};


//重载Logstream的<<操作符,这里又新加了两种类型 SourceFile文件名类 和 T精简字符串类
inline LogStream& operator<<(LogStream& s, T v){
    s.append(v.str_, v.len_);
    return s;
}
inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v){
    s.append(v.data_, v.size_);
    return s;
}

//默认的日志输出,向stdout控制台中输出
void defaultOutput(const char* msg, int len){    //默认写入到stdout
    size_t n = fwrite(msg, 1, len, stdout);
    (void) n;    //编译的时候，一般会有warning提示未经使用的变量n。(void) n可以直接忽视掉这种warning。
}

//清空stdout缓冲区
void defaultFlush(){    //默认刷新stdout
    fflush(stdout);
}

//全局变量:两个函数指针,分别指向日志输出操作函数和日志清空缓冲区操作函数
//如果用户不自己实现这两个函数,就用默认的output和flush函数,输出到stdout中去
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

//很重要的构造函数,除了类成员的初始化,还负责把各种信息写入到Logstream中去
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    :time_(Timestamp::now()),   //当前时间
    stream_(),  //初始化Logger四个成员变量
    level_(level),
    line_(line),
    basename_(file){
        formatTime();   //格式化时间
        CurrentThread::tid();   // 缓存当前线程ID
        stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength()); //格式化线程Tid字符串，输出到缓冲区
        stream_ << T(LogLevelName[level], 6);   //格式化日志级别，输出到缓冲区
        if(savedErrno != 0){    //错误码不为0，还要输出相应信息到缓冲区
            stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
        }
    }

void Logger::Impl::formatTime(){
    int64_t microSecondsSinceEpoch = time_.microSecSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecPerSecond);

    if(seconds != t_lastSecond){
        t_lastSecond = seconds;
        struct tm tm_time;
        gmtime_r(&seconds, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d", tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(17 == len);
        (void) len;
    }
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    //LogStream类重载运算符，把信息输出到缓冲区
    stream_ << T(t_time, 17) << T(us.data(), 9);
}

//表明一条日志写入Logstream结束.
void Logger::Impl::finish(){
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line)
    :impl_(INFO, 0, file, line){
    
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    :impl_(level, 0, file, line){

}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    :impl_(level, 0, file, line){
        impl_.stream_ << func << ' ';
}


Logger::Logger(SourceFile file, int line, bool toAbort)
    :impl_(toAbort ? FATAL : ERROR, errno, file, line){

}

Logger::~Logger(){      //析构函数会调用impl的finish方法
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    //把剩下的数据output出去,output由用户自定义或者使用默认stdout
    g_output(buf.data(), buf.length());
    if(impl_.level_ == FATAL){
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level){   //设置日志级别
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out){ //设置输出函数，替代默认的stdout
    g_output = out;
}

void Logger::setFlush(FlushFunc flush){ //设置输出函数对应的刷新函数
    g_flush = flush;
}