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
#include <vector>
#include <string>

namespace sylar {

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

}

#endif //SYLAR_UTIL_H
