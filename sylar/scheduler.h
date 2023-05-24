/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/27 17:17
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_SCHEDULER_H
#define SYLAR_SCHEDULER_H

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"

namespace sylar {

/**
 * 调度器
 * 是一个虚拟的基类
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");    // use_caller 是否使用当前线程,如果是true那么就将调用者也加入管理中
    virtual ~Scheduler();   // 析构函数,这个调度器是一个基类，根据特性实现具体的子类、加一些具体的功能，所以析构函数是虚函数
    // 虚析构函数，能够保证子类能够正常析构

    const std::string &getName() const { return m_name; }

    static Scheduler *GetThis();    // 获取当前线程的调度器
    static Fiber *GetMainFiber();   // 获取当前线程的主协程

    // 线程池
    void start();   // 启动调度器
    void stop();    // 停止调度器

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {    // 调度协程或者函数
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) {  // 空的话，唤醒
            tickle();   // 唤醒
        }
    }

    template<class InputIterator>   // 锁一次、把所有的都放进去，批量操作
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;   // 使用的是指针，会把里面的方法 swap 掉
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }

protected:
    virtual void tickle();  // 唤醒，信号量
    void run(); // 线程执行函数
    virtual bool stopping();   // 子类实现，判断是否可以停止，有其他清理任务的机会
    virtual void idle();   // 子类实现，空闲协程，为了解决协程调度器没有任务做，又不能退出的问题

    void setThis(); // 设置当前线程的调度器
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty(); // 是否需要唤醒
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {    // 如果有协程或者函数
            m_fibers.push_back(ft); // 将协程或者函数加入到协程队列中
        }
        return need_tickle;
    }
private:
    struct FiberAndThread { // struct 默认是 public 的
        Fiber::ptr fiber;   // 智能指针
        std::function<void()> cb;   // 回调函数
        int thread; // 线程id，这个协程在哪个线程上

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f), thread(thr) {
        }

        FiberAndThread(Fiber::ptr *f, int thr)  // 智能指针的智能指针
            : thread(thr) {
            fiber.swap(*f); // 涉及到一些引用计数的操作，引用释放的问题
        }

        FiberAndThread(std::function<void()> f, int thr)
            : cb(f), thread(thr) {
        }

        FiberAndThread(std::function<void()> *f, int thr)
            : thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread()    // stl 的容器需要默认构造函数，所以这里需要提供一个默认构造函数，要不然无法初始化
            : thread(-1) {
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads; // 协程管理的线程？
    std::list<FiberAndThread> m_fibers; // 协程队列，即将执行的，计划执行的协程
    Fiber::ptr m_rootFiber; // 主协程
    std::string m_name; // 协程调度器的名称

protected:
    std::vector<int> m_threadIds;   // 线程id的列表，我需要随机选择一个线程来执行协程，不需要真正的线程id，比如 100 % 5
    size_t m_threadCount = 0;   // 线程数量
    std::atomic<size_t> m_activeThreadCount = {0};    // 活跃的线程数量
    std::atomic<size_t> m_idleThreadCount = {0};  // 空闲的线程数量
    bool m_stopping = true; // 是否停止
    bool m_autoStop = false;    // 是否自动停止
    int m_rootThread = 0;   // 主线程id
};

}


#endif //SYLAR_SCHEDULER_H
