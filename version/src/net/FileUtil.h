#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

#include "nonCopyable.h"
#include <string>

namespace fileutil{

//顾名思义,读取小文件的类
class ReadSmallFile : public nonCopyable{
public:
    ReadSmallFile(std::string filename);
    ~ReadSmallFile();

    //把文件数据读到content中去,文件大小保存到filesize,修改时间保存到
    //modifyTime,创建时间保存到createTime
    template <typename StringArg>
    int readToString(int maxsize, std::string* content, int64_t* filesize,
                     int64_t* modifyTime, int64_t* createTime);

    //把文件读到缓冲区中去,实际上读到类内部buffer_数组中去
    int readToBuffer(int* size);
    //返回类中buffer_数组
    const char* buffer() const{ 
        return buffer_ ;
    }
    //类中数组的大小
    static const int kBufferSize = 60*1024;   //60kb，1kb=1024byte

private:
    int fd_;                   //文件描述符
    int errno_;                  //错误码
    char buffer_[kBufferSize];    //保存数据的数组
};

//利用ReadSmallFile中的readToString函数完成readFile函数
template <typename string>
int readFile(string filename, int maxSize, string* content,
             int64_t* fileSize = NULL, int64_t* modifyTime = NULL, int64_t* createTime = NULL)
{
    ReadSmallFile file(filename);
    return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}


class AppendFile : public nonCopyable{
public:
    //显式构造函数，打开一个文件
    explicit AppendFile(std::string filename);
    //关闭文件
    ~AppendFile();

    //向文件中写logline，长度为len
    void append(const char* logline, const size_t len);
    //刷新写入内容
    void flush();
    //返回已经写入文件的字节数
    int writtenBytes() const{
        return writtenBytes_;
    }

private:
    //使用fwrite_unlocked来实现写文件
    size_t write(const char* logline, size_t len);
    FILE *fp_;  //文件指针
    char buffer_[64 * 1024];    //缓冲区64k
    int writtenBytes_;  //已经写入的字节数

};


}


#endif