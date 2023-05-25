/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/24 9:56
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_FIBER_H
#define SYLAR_FIBER_H

#include <memory>
#include <ucontext.h>
#include <functional>
#include "thread.h"

namespace sylar {

class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {   // 获得当前类的指针，继承这个类之后、不可以在栈上创建对象
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT,
    };
private:
    Fiber();

public:
    // 不允许默认构造，使用 functional 的方式构造，解决了函数指针不适合场景的问题
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    ~Fiber();

    // 协程执行完了、或者出错的时候，重置协程函数，并重置状态，利用已经分配的内存，去做另外一些事情，节省内存的分配与释放
    // 能重置的协程的状态要么是 TERM，要么是 INIT
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

    void call();
    void back();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }

public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为Ready状态
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold状态
    static void YieldToHold();
    //总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();
    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0;  // 协程 id
    uint32_t m_stack_size = 0;   // 协程栈大小
    State m_state = INIT;   // 协程状态

    ucontext_t m_ctx;   // 协程上下文
    void *m_stack = nullptr;    // 协程栈

    std::function<void()> m_cb; // 协程函数, 用于执行协程的函数，回调函数
};

}


#endif //SYLAR_FIBER_H
