/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/18 10:57
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_THREAD_H
#define SYLAR_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <semaphore.h>
#include <cstdint>
#include <atomic>

// c++11 之前 使用 pthread_xxx
// C++11 之后 使用 std::thread, 低层也是用 pthread_xxx
// std::thread 没有读写锁，不太使用
// 我们高并发场景，读多写少，所以使用 pthread 的读写锁，性能上兼顾

namespace sylar{

class Semaphore{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();    // -1
    void notify();  // +1

private:
    Semaphore(const Semaphore&) = delete;   // 禁止默认拷贝
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    sem_t m_semaphore;
};

template<class T>   // 互斥量一般都是局部作用范围的，为了防止忘记解锁，我们使用 RAII
class ScopedLockImpl{   // 构造函数加锁、析构函数解锁，这样就不用担心忘记解锁了
public:
    ScopedLockImpl(T& mutex)    // 互斥量很多，提供一个模板，可以传入任何互斥量，具体是什么我们不关心
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex; // 互斥量引用
    bool m_locked;
};

template<class T>
class ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.rdunlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
class WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 互斥量
class Mutex{
public:
    typedef ScopedLockImpl<Mutex> Lock; // 互斥量的锁类型
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t* get() { return &m_mutex; }

private:
    pthread_mutex_t m_mutex;
};

/**
 * @brief 空互斥量，不加锁, 用于调试
 */
class NullMutex{
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}

    void lock() {}
    void unlock() {}
};

class RWMutex{
public:
    typedef ScopedLockImpl<RWMutex> ReadLock; // 读写锁的锁类型
    typedef ScopedLockImpl<RWMutex> WriteLock;
    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void lock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void rdunlock() {
        pthread_rwlock_unlock(&m_lock);
    }

    pthread_rwlock_t* get() { return &m_lock; }

private:
    pthread_rwlock_t m_lock;
};

class NullRWMutex{
public:
    typedef ScopedLockImpl<NullRWMutex> ReadLock;
    typedef ScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}

    void lock() {}
    void unlock() {}
    void rdlock() {}
    void wrlock() {}
    void rdunlock() {}
};


/**
 * @brief 自旋锁
 * 自旋锁是一种用于保护共享资源的锁，它的优点是能减少线程切换状态的开销。
 * 但是，如果一直旋下去，自旋开销会比线程切换状态的开销大得多。
 *
 * 适用场景就很简单了：并发不能太高，避免一直自旋不成功；
 * 线程执行的同步任务不能太长，避免一直自旋不成功。
 *
 * 适用于冲突时间短的场景，自旋锁一般不会阻塞线程，
 * 而是循环检测锁是否可用，所以如果冲突时间长，自旋锁的效率就会非常低。
 */
class Spinlock{
public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

class CASLock{
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {
    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;  // 原子操作, 每次都取内存
};

class Thread {
public:
    // 线程智能指针类型
    typedef std::shared_ptr<Thread> ptr;

    /**
     * @brief 构造函数
     * @param[in] cb 线程执行函数
     * @param[in] name 线程名称
     */
    Thread(std::function<void()> cb, const std::string& name);  // 使用 Linux top 命令显示的线程 id
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name;}

    /**
     * @brief 等待线程执行完成
     */
    void join();

    static Thread* GetThis();   // 当我是某个函数的时候，我想获取我现在所在的线程，需要一个静态方法，就可以拿到，然后针对这个线程做一些操作
    static const std::string& GetName();  // 用于日志，直接获取当前线程的名字
    static void SetName(const std::string& name);  // 设置当前线程的名字, 有的线程并不是我们自己创建的，所以需要一个静态方法可以设置主线程为 main
private:
    Thread(const Thread&) = delete;     // 禁止默认拷贝
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);    // 线程的入口函数
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;  // 信号量，用于线程同步
};

}




#endif //SYLAR_THREAD_H
