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

namespace sylar {

    class Logger;
    // 日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr; // 智能指针, 用于管理日志事件对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        LogEvent(const char *file, int32_t line, uint32_t elapse
                 , uint32_t threadId, uint32_t fiberId, uint64_t time);

        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }

        std::stringstream &getSS() { return m_ss; }
    private:
        const char *m_file = nullptr;   // 文件名
        int32_t m_line = 0;             // 行号
        uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;        // 线程id
        uint32_t m_fiberId = 0;         // 协程id
        uint64_t m_time = 0;            // 时间戳
        std::stringstream m_ss;          // 日志内容
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

        static const char *ToString(LogLevel::Level level);
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
    private:
        std::string m_pattern;  // 日志格式
        std::vector<FormatItem::ptr> m_items; // 日志格式项
    };


    // 日志输出地
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr; // 智能指针, 用于管理日志输出地对象的生命周期, 防止内存泄漏, 保证程序的健壮性
        virtual ~LogAppender() {}   // 虚析构函数, 保证子类析构时调用父类析构函数, 释放父类资源, 防止内存泄漏, 保证程序的健壮性

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0; // 纯虚函数, 保证子类必须实现该函数, 防止内存泄漏, 保证程序的健壮性

        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }
    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;    // 日志级别
        LogFormatter::ptr m_formatter;  // 日志格式器
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger> { // 只有定义了这个它才能在自己的成员函数中使用shared_from_this
    public:
        typedef std::shared_ptr<Logger> ptr; // 智能指针, 用于管理日志器对象的生命周期, 防止内存泄漏, 保证程序的健壮性

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string &getName() const { return m_name; }
    private:
        std::string m_name;                         // 日志器名称
        LogLevel::Level m_level;                    // 日志级别
        std::list<LogAppender::ptr> m_appenders;    // appender 集合
        LogFormatter::ptr m_formatter;              // 日志格式器
    };

    // 输出到控制台的日志输出地
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr; // 智能指针
        void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override; // 重写父类的虚函数, 继承的实现
    private:
    };

    // 输出到文件的日志输出地
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr; // 智能指针
        FileLogAppender(const std::string &filename);
        void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;

        // 重新打开文件, 文件打开成功返回true, 否则返回false
        bool reopen(); // 重新打开文件
    private:
        std::string m_filename; // 文件名
        std::ofstream m_filestream; // 文件流
    };
}


#endif //SYLAR_NEW_LOG_H
