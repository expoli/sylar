/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/14 12:14
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_UTIL_H
#define SYLAR_UTIL_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>

namespace sylar {

    pid_t GetThreadId();

    uint32_t GetFiberId();
}

#endif //SYLAR_UTIL_H
