/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/18 10:57
* @version: 1.0
* @description: 
********************************************************************************/


#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

static thread_local Thread *t_thread = nullptr;     // 我们要拿到当前线程，需要一个线程局部变量, 用于保存当前线程
static thread_local std::string t_thread_name = "UNKNOWN";  // 用于保存当前线程的名字,只在当前线程中有效，性能可以提高

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");  // 系统都放在 system 中

Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {   // 一直等
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}


Thread *Thread::GetThis() {
    return t_thread;
}

const std::string &Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string &name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
        : m_cb(std::move(cb)), m_name(name) {
    if(m_name.empty()) {
        m_name = "UNKNOWN";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "pthread_create thread fail, rt=" << rt
                                          << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    // 有可能我们的构造函数返回了，线程还没有开始运行，所以我们需要等待线程运行
    m_semaphore.wait(); // 一直等到线程运行起来，再出构造函数
}

Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);   // 析构的时候，如果线程还没有结束，就分离掉，另一种方式是 join，会被阻塞掉
    }
}

void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "pthread_join thread fail, rt=" << rt
                                              << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void *Thread::run(void *arg) {
    Thread *thread = (Thread *) arg;  // 拿到当前线程
    t_thread = thread;
    t_thread_name = thread->m_name; // 设置线程名字, 用于日志输出, 只有在真正运行的时候，设置才有用
    thread->m_id = sylar::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());  // 设置线程名字

    std::function<void()> cb;
    cb.swap(thread->m_cb);  // 交换，避免拷贝，当函数中拥有智能指针的时候，会一直出现不被释放掉的情况

    thread->m_semaphore.notify();  // 通知构造函数，线程已经运行起来了

    cb();   // 执行函数
    return nullptr;
}

}