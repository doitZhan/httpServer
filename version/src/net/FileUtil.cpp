#include "FileUtil.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

template<typename To, typename From>
inline To implicit_cast(From const &f)
{
  return f;
}

namespace fileutil {

//构造函数中执行open打开文件操作
ReadSmallFile::ReadSmallFile(std::string filename)
    :fd_(::open(filename.data(), O_RDONLY|O_CLOEXEC)), errno_(0){
        buffer_[0]='\0';
        //文件若是打开失败,设置错误码信息
        if(fd_ < 0)
            errno_ = errno;
}

//析构函数中关闭文件
ReadSmallFile::~ReadSmallFile()
{
    if(fd_)
        ::close(fd_);
}

//把文件数据读入到content中,大小保存到filesize,修改时间保存到modifyTime
//创建时间保存到createTime
template <typename StringArg>
int ReadSmallFile::readToString(int maxsize, std::string* content,
                                int64_t* filesize, int64_t* modifyTime, int64_t* createTime)
{
    static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
    assert(content != NULL);//保证可以传入到content中

    if(fd_ < 0)
        return errno_;   //保证文件描述符正确

    int err = errno_;      //返回值err,可根据判断是否出错
    content->clear();   //先清空缓冲区

    if(filesize)        //保证filesize不是NULL,可以传入
    {
        //该结构体用于获取文件信息,文件大小,修改时间,创建时间等信息
        struct stat statbuf;
        if(::fstat(fd_, &statbuf) == 0){    //获取文件信息
            if(S_ISREG(statbuf.st_mode)){ //是否是一个常规文件
                *filesize = statbuf.st_size;  //保存文件大小
                //重新分配一下string的内存空间,保证为min(maxsize,*filesize)
                content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxsize), *filesize)));
            }
            else if(S_ISDIR(statbuf.st_mode))  //是否是一个目录
                err = EISDIR;
            //保存文件修改和创建时间
            if(modifyTime)
                *modifyTime = statbuf.st_mtime;
            if(createTime)
                *createTime = statbuf.st_ctime;

        }else
            err = errno;
    }

    //开始拷贝文件数据到string中
    while(content->size() < implicit_cast<size_t>(maxsize)){
        //一次要读取的数据量=min(maxsize-content->size(), 60*1024)
        size_t toRead = std::min(implicit_cast<size_t>(maxsize) - content->size(), sizeof(buffer_));
        //实际读取了n个字节的数据量
        ssize_t n = ::read(fd_, buffer_, toRead);
        if(n > 0)
            content->append(buffer_, n);
        else{
            if(n < 0)
                err = errno;
            break;
        }
    }

    return err; //返回状态信息,可判断是否有出错
}

int ReadSmallFile::readToBuffer(int *size)
{
    int err = errno_;
    if(fd_ <= 0)
        return err;
    ssize_t n=::pread(fd_, buffer_, sizeof(buffer_ - 1), 0);
    if(n >= 0){
        if(size)
            *size = static_cast<int>(n);//保存大小
        buffer_[n] = '\0';//数组最后结束符'\0'
    }
    else
        err = errno;
    return err;
}



AppendFile::AppendFile(std::string filename)
    :fp_(fopen(filename.c_str(), "ae")),    //e --> O_CLOEXEC
    writtenBytes_(0){
        assert(fp_);
        setbuffer(fp_, buffer_, sizeof(buffer_));   //设置文件大小
}

AppendFile::~AppendFile(){
    fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len){
    size_t n = this->write(logline, len);   //n代表已经写入的字节数
    size_t remain = len - n;    //remain大于0表示未写完
    while(remain > 0){
        size_t x = this->write(logline + n, remain);
        if(x == 0){
            int err = ferror(fp_);
            if(err){
                fprintf(stderr, "AppendFile::append() failed.\n");
            }
            break;
        }
        n += x;     //偏移写入的个数x
        remain = len - n;   //剩余未写入的个数
    }
    writtenBytes_ += len;   //已经写入个数
}

void AppendFile::flush(){
    fflush(fp_);
}

size_t AppendFile::write(const char* logline, size_t len){
    return fwrite_unlocked(logline, 1, len, fp_);   //不加锁的方式写入，效率高，但是非线程安全
}


}