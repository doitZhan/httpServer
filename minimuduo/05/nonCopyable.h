#ifndef __NONCOPYABLE_H__
#define __NONCOPYABLE_H__

class nonCopyable
{
public:
    nonCopyable() = default;
    ~nonCopyable() = default;
    nonCopyable(const nonCopyable&) = delete;
    nonCopyable& operator=(const nonCopyable&) = delete;
};

#endif