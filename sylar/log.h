/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/13 15:59
* @version: 1.0
* @description:
********************************************************************************/


#ifndef SYLAR_NEW_LOG_H
#define SYLAR_NEW_LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <list>

namespace sylar {

    // 日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr; // 智能指针, 用于管理日志事件对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        LogEvent();
    private:
        const char *m_file = nullptr;   // 文件名
        int32_t m_line = 0;             // 行号
        uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
        int32_t m_threadId = 0;         // 线程id
        uint32_t m_fiberId = 0;         // 协程id
        uint64_t m_time = 0;            // 时间戳
        std::string m_content;          // 日志内容
    };

    // 日志级别
    class LogLevel {
    public:
        enum Level {
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };
    };

    // 日志格式器
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr; // 智能指针, 用于管理日志格式器对象的生命周期, 防止内存泄漏, 保证程序的健壮性

        std::string format(LogEvent::ptr event);
    private:
    };


    // 日志输出地
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr; // 智能指针, 用于管理日志输出地对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        virtual ~LogAppender() {}   // 虚析构函数, 保证子类析构时调用父类析构函数, 释放父类资源, 防止内存泄漏, 保证程序的健壮性

        void log(LogLevel::Level level, LogEvent::ptr event);
    private:
        LogLevel::Level m_level;    // 日志级别
    };

    // 日志器
    class Logger {
    public:
        typedef std::shared_ptr<Logger> ptr; // 智能指针, 用于管理日志器对象的生命周期, 防止内存泄漏, 保证程序的健壮性

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, const LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }
    private:
        std::string m_name;                         // 日志器名称
        LogLevel::Level m_level;                    // 日志级别
        std::list<LogAppender::ptr> m_appenders;    // appender 集合
    };

    // 输出到控制台的日志输出地
    class StdoutLogAppender : public LogAppender {

    };

    // 输出到文件的日志输出地
    class FileLogAppender : public LogAppender {

    };
}


#endif //SYLAR_NEW_LOG_H
