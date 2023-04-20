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

// c++11 之前 使用 pthread_xxx
// C++11 之后 使用 std::thread, 低层也是用 pthread_xxx
// std::thread 没有读写锁，不太使用
// 我们高并发场景，读多写少，所以使用 pthread 的读写锁，性能上兼顾

namespace sylar{

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

};

}




#endif //SYLAR_THREAD_H
