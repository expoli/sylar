/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/5/25 11:29
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_IOMANAGER_H
#define SYLAR_IOMANAGER_H

#include "scheduler.h"

namespace sylar {

class IOManager : public Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE    = 0x0,
        READ    = 0x1, //EPOLLIN
        WRITE   = 0x4, //EPOLLOUT
    };
private:
    struct FdContext {  // 针对事件的上下文，针对 epoll 是 fd 操作
        typedef Mutex MutexType;
        struct EventContext {   // 每种事件有相应的实例，多线程，多协程，多个协程调度器
            Scheduler* scheduler = nullptr; //事件执行的scheduler
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;       //事件的回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;      //读事件
        EventContext write;     //写事件
        int fd = 0;             //事件关联的句柄
        Event events = NONE;    //已经注册的事件
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd); // 取消所有事件

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void contextResize(size_t size);
private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};  // 正在等待的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

}


#endif //SYLAR_IOMANAGER_H
