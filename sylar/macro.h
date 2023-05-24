/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/23 21:58
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_MACRO_H
#define SYLAR_MACRO_H

#include <cstring>
#include <cassert>
#include "util.h"

#define SYLAR_ASSERT(x) \
    if(!(x)) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

// 有参数的宏
#define SYLAR_ASSERT2(x, w) \
    if(!(x)) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif //SYLAR_MACRO_H
