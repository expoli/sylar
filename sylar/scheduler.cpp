/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/27 17:17
* @version: 1.0
* @description: 
********************************************************************************/


#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");  // 系统都放在 system 中

static thread_local Scheduler* t_scheduler = nullptr;  // 线程局部变量，协程调度器指针
static thread_local Fiber* t_fiber = nullptr;  // 线程局部变量，我们是这个协程的主协程函数

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    : m_name(name) {
    SYLAR_ASSERT(threads > 0);

    if (use_caller) {   // 如果使用调用者，那么就获取当前线程的协程
        Fiber::GetThis();   // 获取当前线程的协程，如果没有协程，那么就给当前初始化一个主协程
        --threads;  // 线程数减一，使用当前线程

        SYLAR_ASSERT(GetThis() == nullptr);    // 断言，当前线程的调度器为空，因为是主线程，一山不容二虎
        t_scheduler = this; // 设置当前线程的调度器

        // 当使用的线程是一个新线程的时候，新的线程的主协程并不会参与我们的调度，所以我们需要创建一个新的协程，来做主流程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this)));    // 创建主协程
        sylar::Thread::SetName(m_name); // 设置线程名字
        // 在线程里面声明一个调度器，再把当前线程放入调度器里面去，那主协程不再是当前线程的主协程，而是执行run方法的主协程
        // 这个地方的关键点在于，是否把创建协程调度器的线程放到协程调度器管理的线程池中。
        // 如果不放入，那这个线程专职协程调度；
        // 如果放的话，那就要把协程调度器封装到一个协程中，称之为主协程或协程调度器协程。
        t_fiber = m_rootFiber.get();    // 设置当前线程的主协程
        m_rootThread = sylar::GetThreadId(); // 设置主线程id
        m_threadIds.push_back(m_rootThread);  // 将主线程id放入线程id数组中
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;    // 线程数
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this) {    // 如果是当前线程的调度器，那么就设置为空
        t_scheduler = nullptr;
    }
}

Scheduler *Scheduler::GetThis() {
    return t_scheduler;
}

Fiber *Scheduler::GetMainFiber() {
    return t_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {  // 还没有启动，那么就返回
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);    // 线程池大小
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                                      , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());    // 将线程id放入线程id数组中，与信号量配合使用
    }
    lock.unlock();

    if(m_rootFiber) {   // 如果有主协程，那么就唤醒主协程
//         m_rootFiber->swapIn();
        m_rootFiber->call();
        SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    }
}

void Scheduler::stop() {
    m_autoStop = true;
    if (m_rootFiber //  使用了 caller ，所以需要在主协程中停止
        && m_threadCount == 0
        && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT)) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    // bool exit_on_this_fiber = false;
    if (m_rootThread != -1) {   // use caller 线程
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }
    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();   // 唤醒所有线程
    }

    if (m_rootFiber) {
        tickle();
    }

    if(stopping()){
        return;
    }
    // if (exit_on_this_fiber) {
    //     exit(0);
    // }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    setThis();
    if (sylar::GetThreadId() != m_rootThread) { // 如果不是主线程，那么就设置当前线程的主协程
        t_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));    // 创建空闲协程
    Fiber::ptr cb_fiber;    // 回调协程

    FiberAndThread ft;
    while(true){
        ft.reset();
        bool tickle_me = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()){
                if(it->thread != -1 && it->thread != sylar::GetThreadId()){ // 不是当前线程的协程
                    ++it;
                    tickle_me = true;   // 唤醒其他线程，让其他线程来执行
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC){ // 执行状态的协程,直接跳过
                    ++it;
                    continue;
                }
                ft = *it;
                m_fibers.erase(it++);
            }
        }

        if(tickle_me){
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        || ft.fiber->getState() != Fiber::EXCEPT)){
            ++m_activeThreadCount;
            ft.fiber->swapIn(); // 执行协程
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY){
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                      && ft.fiber->getState() != Fiber::EXCEPT){
                ft.fiber->m_state = Fiber::HOLD;    // 挂起，让出了执行时间
            }
            ft.reset();
        } else if(ft.cb) {
            if(cb_fiber){
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            ++m_activeThreadCount;
            cb_fiber->swapIn(); // 新创建的协程执行
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                      || cb_fiber->getState() == Fiber::TERM){  // 协程执行完毕，把它释放掉
                cb_fiber->reset(nullptr);
            } else { // if(cb_fiber->getState() != Fiber::TERM){ // 其它状态就挂起
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {    // 事情做完了，idle协程执行
            if(idle_fiber->getState() == Fiber::TERM){
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            if(idle_fiber->getState() != Fiber::TERM
                    || idle_fiber->getState() != Fiber::EXCEPT){
                idle_fiber->m_state = Fiber::HOLD;
            }
            --m_idleThreadCount;
        }
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
           && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while(!stopping()){
        sylar::Fiber::YieldToHold();
    }
}

}
