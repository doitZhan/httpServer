#include "currentThread.h"
#include "nonCopyable.h"

class eventLoop : public nonCopyable{
public:
    eventLoop();
    ~eventLoop();

    void loop();

    void assertInLoopThread(){
        if (!isInLoopThread()){
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const{
        return threadId_ == currentThread::tid();
    }
private:
    void abortNotInLoopThread();

    bool looping_;
    const pid_t threadId_;
};
