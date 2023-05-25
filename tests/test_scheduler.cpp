/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/5/24 17:21
* @version: 1.0
* @description: 
********************************************************************************/

#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int main(int argc, char** argv) {
    sylar::Scheduler sc;
    sc.start();
    SYLAR_LOG_INFO(g_logger) << "main";
//    sc.schedule(&test_fiber);
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}