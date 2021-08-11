#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

#include "nonCopyable.h"
#include "StringPiece.h"

class AppendFile : public nonCopyable{
public:
    explicit AppendFile(StringArg filename);
    ~AppendFile();

    void append(const char* logline, const size_t len);
    void flush();
    int writtenBytes() const{
        return writtenBytes_;
    }

private:
    size_t write(const char* logline, size_t len);
    FILE *fp_;
    char buffer_[64 * 1024];
    int writtenBytes_;

};





#endif