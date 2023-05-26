/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/5/25 11:29
* @version: 1.0
* @description: 
********************************************************************************/


#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    SYLAR_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
        :Scheduler(threads, use_caller, name) {// 初始化父类
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;   // 边缘触发，只触发一次
    event.data.fd = m_tickleFds[0];     // 读端

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);    // 设置为非阻塞
    SYLAR_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);  // 添加到 epoll 中
    SYLAR_ASSERT(!rt);

    contextResize(32);

    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {   // 释放资源
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    /**
     * 这一部分代码使用按位与运算符 `&` 来检查 `FdContext` 对象的 `events`
     * 字段中是否已经存在指定的事件。如果 `events` 字段与 `event` 参数按位与的结果不为零，
     * 则表示事件已经存在。
     * 例如，假设 `events` 字段的值为 `0b00000101`，表示已经注册了读和写事件。
     * 如果 `event` 参数的值为 `0b00000100`，表示要添加写事件，
     * 则按位与的结果为 `0b00000100`，不为零。这意味着写事件已经存在，
     * 将记录错误消息并触发断言。
     */
    if(fd_ctx->events & event) {
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                  << " event=" << event
                                  << " fd_ctx.event=" << fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << op << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    /**
     * FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
     * 这一行代码调用了 FdContext 对象的 getContext 方法，并将返回的 EventContext 对象的引用存储在 event_ctx 变量中。
     * getContext 方法接受一个 Event 类型的参数，它返回与该事件类型对应的 EventContext 对象。
     * 例如，如果传递的事件类型为 READ，则返回 FdContext 对象中的 read 字段，该字段是一个 EventContext 对象。
     *
     * EventContext 对象包含与特定事件类型相关的信息，例如事件执行的调度程序、事件协程和事件的回调函数。
     */
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    /**
     * 用于检查 EventContext 对象的 scheduler、fiber 和 cb 字段是否都为 nullptr。
     * 如果任何一个字段不为 nullptr，则断言将失败并终止程序。
     * 这个断言的目的是确保在添加新事件之前，与该事件类型对应的 EventContext
     * 对象尚未与任何调度程序、协程或回调函数关联。
     * 这可以防止意外覆盖现有的调度程序、协程或回调函数。
     */
    SYLAR_ASSERT(!event_ctx.scheduler
                 && !event_ctx.fiber
                 && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    /**
     * event_ctx.cb.swap(cb); 这一行代码调用了 std::function 类的 swap 成员函数，
     * 它用于交换两个 std::function 对象的内容。在这种情况下，
     * 它交换了 event_ctx.cb 和 cb 两个对象的内容。
     * 这意味着在调用 swap 函数之后，event_ctx.cb 将包含传递给 addEvent 方法的回调函数
     * 而原来的 event_ctx.cb 值将存储在 cb 变量中。
     */
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) { // 没有这个事件
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);    // 事件与运算，去掉事件
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << op << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << op << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << op << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    /**
     * 这种转换是合理的，因为 IOManager 类继承自 Scheduler 类。
     * 如果当前调度程序对象实际上是一个 IOManager 对象，则转换将成功并返回一个有效的指针。
     * 如果当前调度程序对象不是一个 IOManager 对象，则转换将失败并返回空指针。
     *
     * 由于使用了 dynamic_cast，这种转换是类型安全的。
     * 如果转换失败，它不会导致未定义行为，而只会返回空指针。
     * 但是，在使用返回的指针之前，应检查它是否为空，以避免对空指针进行解引用。
     */
    IOManager* m = dynamic_cast<IOManager*>(Scheduler::GetThis());
    SYLAR_ASSERT(m);
    return m;
}

void IOManager::tickle() {
    if(hasIdleThreads()) {  // 如果有空闲线程，就不用唤醒，没有的话，就不用发送了，因为没有线程去接收
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1); // 向管道写入一个字节
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping() {
    return Scheduler::stopping()
           && m_pendingEventCount == 0;
}

void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();    // 协程，不在栈上分配大数组
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        if(stopping()) {
            SYLAR_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 5000;    // ms 级
            rt = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT);
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {   // 外部有发消息过来，被唤醒了
                uint8_t dummy;
                while(read(m_tickleFds[0], &dummy, 1) == 1);    // 读完所有的数据，读干净
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            /**
             * 在循环的每次迭代中，该方法检查 epoll_event 对象的 events 字段是否包含 EPOLLERR 或 EPOLLHUP 标志。
             * 如果是，则将 EPOLLIN 和 EPOLLOUT 标志添加到 events 字段中。
             * 这种处理方式是合理的，因为当套接字出现错误或被挂起时，通常需要对其进行读写操作以获取错误信息并清除错误状态。
             * 将 EPOLLIN 和 EPOLLOUT 标志添加到 events 字段中可以确保对套接字进行读写操作。
             * 在后续处理中，如果 events 字段包含 EPOLLIN 或 EPOLLOUT 标志，则会触发相应的读或写事件。
             * 这可以让应用程序处理套接字错误并采取适当的措施。
             */
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);  // 剩下的事件
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                          << op << "," << fd_ctx->fd << "," << event.events << "):"
                                          << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut(); // 交出执行权
    }
}

}
