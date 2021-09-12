#include "AsyncLogging.h"
#include "LogFile.h"
#include "Timestamp.h"

#include <stdio.h>

AsyncLogging::AsyncLogging(const std::string& basename, int rollSize, int flushInterval)
    :flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_(){
        currentBuffer_->bzeroInner();
        nextBuffer_->bzeroInner();
        buffers_.reserve(16);
    }

//所有LOG_*宏定义都会调用到这个append函数,用于把日志写到缓冲区中
//2个默认缓冲区,currentBuffer_和nextBuffer_
//currentBuffer_写满了换nextBuffer_,nextBuffer_也写满了新建一个
void AsyncLogging::append(const char* logline, int len){
    //加锁操作日志缓冲区
    MutexLockGuard lock(mutex_);
    //当前缓冲区剩余空间够,直接写到当前缓冲区
    if(currentBuffer_->avail() > len){
        currentBuffer_->append(logline, len);
    }
    else{ //当前缓冲区被写满日志了,换nextBuffer_来写
        //std::move把左值强制转化成右值引用,可以通过右值引用使用该值,来用于移动语义,
        //例如用vector::push_back()会开辟内存拷贝元素,若是使用了std::move就不会发生复制
        //在这里意思就是把currentBuffer_对象的所有权剥夺,转移到buffers_.back()上面
        //不需要开辟内存拷贝元素,由于currentBuffer_所有权被剥夺,因此currentBuffer_变为空
        buffers_.push_back(std::move(currentBuffer_));
        //nextBuffer_可用就用nextBuffer_,否则就新建一个额外的缓冲区来写
        if(nextBuffer_){
            currentBuffer_ = std::move(nextBuffer_);
        }
        else{
            currentBuffer_.reset(new Buffer);
        }
        //把日志写到currentBuffer_中
        currentBuffer_->append(logline, len);
        //通知buffers_中有数据了,可以把buffers_中数据写到文件中去了
        cond_.notify();
    }
}

//这个函数是后端线程函数,用于周期性地把日志写入到文件中去
//每隔flushInterval(3)秒 或者是 currentBuffer_满了 , 就把currentBuffer_加入到buffers_中
void AsyncLogging::threadFunc(){
    assert(running_ == true);
    latch_.countDown();     //latch——变为0,start函数可以返回了
    LogFile output(basename_, rollSize_, false);    //Logfile用于写日志到文件中
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzeroInner();
    newBuffer2->bzeroInner();
    //这块缓冲区vector属于后端线程,用于和前端的buffers_进行交换
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while(running_){
        //保证后端缓冲区newBuffer1,2和后端缓冲区队列buffersToWrite是空的
        assert(newBuffer1 && 0 == newBuffer1->length());
        assert(newBuffer2 && 0 == newBuffer2->length());
        assert(buffersToWrite.empty());

        //这一块主要是用于把buffers_中的数据交换到buffersToWrite中
        {
            //每隔3s或是currentBuffer_满了,就把当前currentBuffer_加入到buffers_中
            MutexLockGuard lock(mutex_);
            if(buffers_.empty()){
                cond_.waitForSeconds(flushInterval_);
            }
            //currentBuffer_添加到buffers_
            buffers_.push_back(std::move(currentBuffer_));
            //currentBuffer_现在是newBuffer1
            currentBuffer_ = std::move(newBuffer1);
            //交换buffersToWrite和buffers_
            buffersToWrite.swap(buffers_);
            if(!nextBuffer_){
                //nextBuffer_现在是newBuffer2
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        //保证始终有数据可以写入到文件中去
        assert(!buffersToWrite.empty());

        //如果要写入文件的buffersToWrite中buffer数目大于25,就删除多余的数据
        //删除的目的是为了解决日志消息堆积的问题:前端日志记录太快,后端来不及写到文件中去
        if(buffersToWrite.size() > 25){
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log message at %s, %zd larger buffers\n", Timestamp::now().toFormattedString().c_str(), buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            //在buffersToWrite中就留两块缓冲区,其他多余的缓冲区的全部删除
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for(size_t i = 0; i < buffersToWrite.size(); ++i){
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        //重新调整bufferToWrite的大小,仅保留2个buffer用于newBuffer1和newBuffer2
        if(buffersToWrite.size() > 2){
            buffersToWrite.resize(2);
        }

        if(!newBuffer1){
            assert(!buffersToWrite.empty());
            //从bufferToWrite中弹出一个buffer作为newBuffer1
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if(!newBuffer2){
            assert(!buffersToWrite.empty());
            //再弹出一个buffer作为newBuffer2
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        //保证buffersToWrite为空,要不然和前端buffers_交换会出问题
        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
    



