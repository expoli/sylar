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
#include <cstdint>
#include <memory>
#include <list>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdarg>
#include <map>
#include "singleton.h"
#include "util.h"
#include "thread.h"

#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
        __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)))) \
        .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
        __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)))) \
        .getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar {

    // 日志级别
    class LogLevel {
    public:
        enum Level {
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5,
            UNKNOWN = 6
        };

        static const char *ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string &str);
    };

    class Logger;
    class LoggerManager;
    // 日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr; // 智能指针, 用于管理日志事件对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
                 , const char *file, int32_t line, uint32_t elapse
                 , uint32_t threadId, uint32_t fiberId, uint64_t time);

        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        std::stringstream &getSS() { return m_ss; }
        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);
    private:
        const char *m_file = nullptr;   // 文件名
        int32_t m_line = 0;             // 行号
        uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;        // 线程id
        uint32_t m_fiberId = 0;         // 协程id
        uint64_t m_time = 0;            // 时间戳
        std::stringstream m_ss;          // 日志内容

        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };

    class LogEventWrap {
    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        std::stringstream &getSS();
        LogEvent::ptr getEvent() const { return m_event; }
    private:
        LogEvent::ptr m_event;
    };

    // 日志格式器
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr; // 智能指针, 用于管理日志格式器对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        LogFormatter(const std::string &pattern);

        //%t
        std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event);
    public:
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            virtual void format(std::ostream& os,std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();

        bool isError() const { return m_error; }

        const std::string getPattern() const { return m_pattern; }
    private:
        std::string m_pattern;  // 日志格式
        std::vector<FormatItem::ptr> m_items; // 日志格式项
        bool m_error = false;   // 格式是否有误
    };


    // 日志输出地
    class LogAppender {
    friend class Logger;
    public:
        typedef std::shared_ptr<LogAppender> ptr; // 智能指针, 用于管理日志输出地对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        typedef Mutex MutexType;
        virtual ~LogAppender() {}   // 虚析构函数, 保证子类析构时调用父类析构函数, 释放父类资源, 防止内存泄漏, 保证程序的健壮性

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0; // 纯虚函数, 保证子类必须实现该函数, 防止内存泄漏, 保证程序的健壮性
        virtual std::string toYamlString() = 0; // 纯虚函数, 需要加锁、可能修改配置

        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();

        LogLevel::Level getLevel() const { return m_level; }    // 原子类型，基本类型的线程安全可能不准
        void setLevel(LogLevel::Level val) { m_level = val; }
    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;    // 日志级别
        bool m_hasFormatter = false;    // 是否有日志格式器
        MutexType m_mutex;  // 互斥锁, 写多
        LogFormatter::ptr m_formatter;  // 日志格式器，多个成员的线程安全可能会导致严重的线程错误（设置完一个，还没来得及设置另一个，另一个就被读取了）
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger> { // 只有定义了这个它才能在自己的成员函数中使用shared_from_this
        friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr; // 智能指针, 用于管理日志器对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        typedef Mutex MutexType;

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string &getName() const { return m_name; }

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string &val);
        LogFormatter::ptr getFormatter();

        std::string toYamlString();
    private:
        std::string m_name;                         // 日志器名称
        LogLevel::Level m_level;                    // 日志级别
        MutexType m_mutex;                              // 互斥锁, 在使用的时候可能会修改它的 appender
        std::list<LogAppender::ptr> m_appenders;    // appender 集合
        LogFormatter::ptr m_formatter;              // 日志格式器
        Logger::ptr m_root;                         // 根日志器
    };

    // 输出到控制台的日志输出地
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr; // 智能指针
        void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override; // 重写父类的虚函数, 继承的实现
        std::string toYamlString() override;
    private:
    };

    // 输出到文件的日志输出地
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr; // 智能指针
        FileLogAppender(const std::string &filename);
        void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;

        // 重新打开文件, 文件打开成功返回true, 否则返回false
        bool reopen(); // 重新打开文件
    private:
        std::string m_filename; // 文件名
        std::ofstream m_filestream; // 文件流
    };

    // 日志管理器
    class LoggerManager {
    public:
        typedef Mutex MutexType;
        LoggerManager();
        Logger::ptr getLogger(const std::string &name);

        void init();
        Logger::ptr getRoot() const { return m_root; }

        std::string toYamlString();
    private:
        MutexType m_mutex; // 互斥锁，操作 m_loggers
        std::map<std::string, Logger::ptr> m_loggers; // 日志器集合
        Logger::ptr m_root; // 根日志器
    };

    typedef sylar::Singleton<LoggerManager> LoggerMgr; // 单例模式, 保证程序中只有一个日志管理器对象

}


#endif //SYLAR_NEW_LOG_H
