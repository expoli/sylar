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

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");  // 系统都放在 system 中

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {

}

Scheduler::~Scheduler() {
}

Scheduler *Scheduler::GetThis() {
}

Fiber *Scheduler::GetMainFiber() {
}

void Scheduler::start() {
}

void Scheduler::stop() {
}

}
