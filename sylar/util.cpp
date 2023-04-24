/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/14 12:18
* @version: 1.0
* @description: 
********************************************************************************/

#include <execinfo.h>
#include "log.h"
#include "util.h"
#include "fiber.h"

namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");  // 系统都放在 system 中

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return sylar::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void **array = (void **) malloc((sizeof(void *) * size));  // 协程的栈比较小，所以为了从内存上节省，所以我们不在栈上分配，而是在堆上分配能用指针就用指针
    size_t s = ::backtrace(array, size);  // 从当前函数开始往上找，找 size 个

    char ** strings = backtrace_symbols(array, s);  // 将地址转换为字符串
    if(strings == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }
    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}


}