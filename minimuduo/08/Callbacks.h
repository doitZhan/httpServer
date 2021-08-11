#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <functional>
#include "Timestamp.h"
#include "Buffer.h"
#include <memory>

typedef std::function<void()> TimerCallback;

// The only one that uses shared_ptr
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
// implementation in TcpConnetcion.cc in muduo project
void defaultConnectionCallback(const TcpConnectionPtr& conn);

typedef std::function<void (const TcpConnectionPtr&, Buffer*, int)> MessageCallback;
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf, int n);

typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

// get_pointer as it be named
template<class T>
T* get_pointer(T* p){
    return p;
}

template<class T>
T* get_pointer(std::unique_ptr<T> const& p){
    return p.get();
}

template<class T>
T* get_pointer(std::shared_ptr<T> const& p){
    return p.get();
}

#endif