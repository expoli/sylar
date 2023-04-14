/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/13 20:18
* @version: 1.0
* @description: 
********************************************************************************/

#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"

int main() {
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::DEBUG);

    logger->addAppender(file_appender);

    SYLAR_LOG_INFO(logger) << "test macro";
    SYLAR_LOG_DEBUG(logger) << "test macro";

    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    SYLAR_LOG_FMT_INFO(logger, "test macro fmt info %s", "aa");
    SYLAR_LOG_ERROR(logger) << "test macro error";

    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";
    return 0;
}
