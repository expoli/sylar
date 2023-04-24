/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/13 15:59
* @version: 1.0
* @description: 
********************************************************************************/

#include <iostream>
#include <map>
#include <functional>
#include "log.h"
#include "config.h"

namespace sylar {

const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name) \
            case LogLevel::name: \
                return #name; \
                break;
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
    std::string temp = str;
    std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
#define XX(name) \
        if(temp == #name) { \
            return LogLevel::name; \
        }
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    return LogLevel::UNKNOWN;
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
        : m_event(e) {
}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char *fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char *fmt, va_list al) {
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, al); // 自动分配内存，并完成初始化
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

std::stringstream &LogEventWrap::getSS() {
    return m_event->getSS();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string &str)
            : m_string(str) {
    }

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }

private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }

private:
    std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line,
                   uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                   const std::string &thread_name)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time),
          m_threadName(thread_name), m_logger(logger), m_level(level) {
}

Logger::Logger(const std::string &name)
        : m_name(name), m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex); // 加锁
    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);    // 可以不用加锁，因为 m_level 是原子变量
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for(auto &i: m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex); // 加锁
    m_formatter = val;
    for(auto &i: m_appenders) {
        MutexType::Lock ll(i->m_mutex);    // 友函数、加锁，对 i 加锁
        if(!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string &val) {
    // 先将 formatter 解析出来，看看是否有错误
    sylar::LogFormatter::ptr new_val(new LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_val);  // 使用的是上面的方法，不需要显式加锁
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex); // 加锁
    return m_formatter;
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
    if(m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex); // 加锁
    return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex); // 加锁
    if(!appender->getFormatter()) {
        MutexType::Lock ll(appender->m_mutex);   // 修改 appender 的 formatter，需要加锁
        appender->m_formatter = m_formatter;    // 如果appender没有formatter，那么就使用logger的formatter
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex); // 加锁
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) { // 如果原来没有设置过level，那么默认是DEBUG，最低级别
        auto self = shared_from_this();
        MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty()) { // 如果有appender
            for(auto &i: m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root) {    // 如果没有appender，就使用root的appender
            m_root->log(level, event);
        }
    }
}

void Logger::debug(const LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}

void Logger::info(const LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}

void Logger::warn(const LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}

void Logger::error(const LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}

void Logger::fatal(const LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string &filename)
        : m_filename(filename) {
    {
        MutexType::Lock lock(m_mutex); // 加锁
        m_file_inotify_fd = inotify_init();
        if(m_file_inotify_fd < 0) {
            std::cout << "inotify_init error" << std::endl;
            throw std::logic_error("inotify_init error");
        }
    }

    reopen();

    {
        MutexType::Lock lock(m_mutex); // 加锁
        m_epoll_fd = epoll_create(1);
        if(m_epoll_fd < 0) {
            std::cout << "epoll_create error" << std::endl;
            throw std::logic_error("epoll_create error");
        }
        struct epoll_event event{};
        event.events = EPOLLMSG | EPOLLWAKEUP | EPOLLIN | EPOLLOUT;    // 文件监视器消息可读, 只触发一次, 边缘触发
        event.data.fd = m_file_inotify_fd;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_file_inotify_fd, &event);
    }
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    m_deleted = checkFileStatus();
    uint64_t now = time(nullptr);   // 每秒打开一次、同时配合事件机制，每秒检查一次文件是否被删除
    if(m_last_time != now || m_deleted) {
        m_last_time = now;
        reopen();
    }
    if(level >= m_level) {
        MutexType::Lock lock(m_mutex);
        m_filestream << m_formatter->format(logger, level, event);
    }
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    if(m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter)
        node["formatter"] = m_formatter->getPattern();
    node["file"] = m_filename;
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::checkFileStatus() {
    MutexType::Lock lock(m_mutex);

    // 读取通知事件
    int len = epoll_wait(m_epoll_fd, m_epoll_events, MAX_EVENT_NUMBER, 0);
    if(len == -1) {
        throw std::logic_error("Failed to read events.");
    }
    if(len == 0) {
        return false;
    }
//        std::cout << "len: " << len << std::endl;

    // 遍历缓冲区中的所有事件
    int i = 0;
    for(; i < len; i++) {
        // 如果是文件监视器发生的事件
        std::cout << "m_epoll_events[i].data.fd: " << m_epoll_events[i].data.fd << std::endl;
        if(m_epoll_events[i].data.fd == m_file_inotify_fd) {
            // 创建一个缓冲区，用于存储通知事件
            const int BUF_LEN = 1024;
            char buffer[BUF_LEN];
            memset(buffer, 0, BUF_LEN);

            // 读取文件监视器的事件
            int read_len = read(m_file_inotify_fd, buffer, BUF_LEN);
            if(read_len == -1) {
                throw std::logic_error("Failed to read events.");
            }

            // 遍历所有事件
            int j = 0;
            while (j < read_len) {
                // 读取事件的结构体
                auto *event = (struct inotify_event *) &buffer[j];
//                    std::cout << "event->mask: " << event->mask << std::endl;
                // 如果是删除事件
                if(event->mask & IN_DELETE_SELF
                   || event->mask & IN_MOVE_SELF
                   || event->mask & IN_DELETE
                   || event->mask & IN_MOVE) {
                    std::cout << "Logfile File " << m_filename << " has been deleted." << std::endl;
                    m_deleted = true;
                    m_filestream.close();
                    return true;
                }
                j += sizeof(struct inotify_event) + event->len;
            }
        }
    }

    return false;
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);   // 以追加的方式打开文件，日志不会被覆盖
    // 注册一个监视器，监视对应文件的删除事件，每次删除之后都需要重新注册
    m_file_inotify_wd = inotify_add_watch(m_file_inotify_fd, m_filename.c_str(),
                                          IN_ALL_EVENTS ^ (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_OPEN | IN_CLOSE));
    if(m_file_inotify_wd < 0) {
        std::cout << "inotify_add_watch error" << std::endl;
        throw std::logic_error("inotify_add_watch error");
    }
    m_deleted = false;
    return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format(logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter)
        node["formatter"] = m_formatter->getPattern();
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto &i: m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

void LogFormatter::init() {
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                               && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }


    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

            XX(m, MessageFormatItem), // 消息体
            XX(p, LevelFormatItem), // 日志级别
            XX(r, ElapseFormatItem), // 日志事件发生的时间
            XX(c, NameFormatItem), // 日志名称
            XX(t, ThreadIdFormatItem), // 线程id
            XX(n, NewLineFormatItem), // 回车换行
            XX(d, DateTimeFormatItem), // 时间
            XX(f, FilenameFormatItem), // 文件名
            XX(l, LineFormatItem), // 行号
            XX(T, TabFormatItem), // tab
            XX(F, FiberIdFormatItem), // 协程id
            XX(N, ThreadNameFormatItem), // 线程名称
#undef XX
    };

    for(auto &i: vec) {
        if(std::get<2>(i) == 0) { // 字符串
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
//            std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->m_name] = m_root; // root 初始化好，之后放在 m_loggers 中

    init();
}

/**
 * 如果name不为空，则返回对应的logger
 * 如果 name 为空，则创建一个新的logger并返回
 * @param name
 * @return
 */
Logger::ptr LoggerManager::getLogger(const std::string &name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;    // 新定义的没有 appender，但是可以继承 root 的 appender
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0; // 1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOWN;
    std::string formatter;  // 具体的文件格式
    std::string file;

    bool operator==(const LogAppenderDefine &other) const { // 要进行判断
        return type == other.type
               && level == other.level
               && formatter == other.formatter
               && file == other.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOWN;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &other) const {
        return name == other.name
               && level == other.level
               && formatter == other.formatter
               && appenders == other.appenders;
    }

    bool operator<(const LogDefine &other) const {
        return name < other.name;
    }
};

template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        LogDefine ld;
        if(node["name"].IsDefined()) {
            ld.name = node["name"].as<std::string>();
        }
        if(node["level"].IsDefined()) {
            ld.level = LogLevel::FromString(node["level"].as<std::string>());
        }
        if(node["formatter"].IsDefined()) {
            ld.formatter = node["formatter"].as<std::string>();
        }
        if(node["appenders"].IsDefined()) {
            for(size_t i = 0; i < node["appenders"].size(); ++i) {
                auto n = node["appenders"][i];
                if(!n["type"].IsDefined()) {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "log config error: appender type is null";
                    continue;
                }
                std::string type = n["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!n["file"].IsDefined()) {
                        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "log config error: fileappender file is null";
                        continue;
                    }
                    lad.file = n["file"].as<std::string>();
                    if(n["formatter"].IsDefined()) {
                        lad.formatter = n["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(n["formatter"].IsDefined()) {
                        lad.formatter = n["formatter"].as<std::string>();
                    }
                } else {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "log config error: appender type is invalid";
                    continue;
                }
                if(n["level"].IsDefined()) {
                    lad.level = LogLevel::FromString(n["level"].as<std::string>());
                    // 如果 appender 设置了 level，但是 level 比 logger 的 level 低，应该重置为 logger 的 level
                    // 过低的 level 并不能输出，反而可能引起歧义
                    if(lad.level <= ld.level) {
                        lad.level = ld.level;
                    }
                } else { // 如果没有为 appender 设置 level，则使用 logger 的 level
                    lad.level = ld.level;
                }
                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine &v) {
        YAML::Node node;
        node["name"] = v.name;
        if(v.level != LogLevel::UNKNOWN) {
            node["level"] = LogLevel::ToString(v.level);
        }
        if(!v.formatter.empty()) {
            node["formatter"] = v.formatter;
        }
        for(auto &i: v.appenders) {
            YAML::Node n;
            if(i.type == 1) {
                n["type"] = "FileLogAppender";
                n["file"] = i.file;
            } else if(i.type == 2) {
                n["type"] = "StdoutLogAppender";
            }
            if(i.level != LogLevel::UNKNOWN) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }
            node["appenders"].push_back(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
        sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

// todo 在 main 之前之后做一些动作，定义一些全局对象，会在 main 之前调用其构造函数
struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            // 新增
            for(auto &i: new_value) {
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                if(it == old_value.end()) {
                    // 新增的 logger
                    logger = SYLAR_LOG_NAME(i.name);
                } else {
                    // 修改的 logger
                    if(!(i == *it)) {
                        // 修改的 appender
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto &a: i.appenders) {
                    sylar::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new StdoutLogAppender);
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }
            // 修改

            // 删除
            for(auto &i: old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    // 删除的 logger
                    // 不能真删除，如果真删除一些静态化的变量可能会出问题
                    // 可以将日志级别设置的很高
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level) 100);
                    // 清除所有的 appender 使用 root 的 appender
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;    // 在 main 之前调用

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto &i: m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


void LoggerManager::init() {
}

}