#ifndef _ASYNCLOGGING_H_
#define _ASYNCLOGGING_H_

#include "CountDownLatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "LogStream.h"
#include "nonCopyable.h"

#include <memory>
#include <vector>

class AsyncLogging: public nonCopyable{
public:
    AsyncLogging(const std::string& basename, int rollSize, int flushInterval = 3);
    ~AsyncLogging(){
        if(running_){
            stop();
        }
    }

    //前端线程使用,把数据追加到缓冲区中
    void append(const char* logline, int len);

    void start(){
        running_ = true;
        //thread_在构造函数中被定义为thread(std::bind(&asynclogging::threadFunc,this),"Loggins")
        //thread_要执行的线程函数就是threadFunc()函数
        thread_.start();
        //必须等到latch_=0时才能返回,latch_在threadFunc()执行时变为0,
        //保证初始化工作都做好了,后端日志线程已经启动才可以返回.
        latch_.wait();
    }
    
    //停止写日志线程
    void stop(){
        running_ = false;
        cond_.notify();
        thread_.join();
    }

private:
    //后端日志线程函数,用于把缓冲区日志写到文件中去
    void threadFunc();

    //kLargeBuffer=4000*1000 也就是FixedBuffer内部字符数组大小为4000*1000,4MB
    typedef FixedBuffer<kLargeBuffer> Buffer;
    //vector里面存放Buffer之智能指针
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    //std::unique_ptr<Buffer>类型
    typedef std::unique_ptr<Buffer> BufferPtr;

    const int flushInterval_;       //超时时间,每隔一段时间写日志,3s
    bool running_;                  //是否正在运行
    std::string basename_;          //日志名字
    const off_t rollSize_;          //预留的日志大小
    Thread thread_;                 //后端线程,用于把日志写入日志文件
    CountDownLatch latch_;          //倒计时,用于指示日志记录器何时开始工作
    MutexLock mutex_;               //互斥锁
    Condition cond_;                //条件变量
    BufferPtr currentBuffer_;       //当前缓冲区
    BufferPtr nextBuffer_;          //预留缓冲区
    BufferVector buffers_;          //缓冲区队列,待写入文件
};

#endif

