/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/13 15:59
* @version: 1.0
* @description: 
********************************************************************************/

#include <iostream>
#include "log.h"
namespace sylar {
    Logger::Logger(const std::string &name)
        : m_name(name) {

    }

    void Logger::addAppender(LogAppender::ptr appender) {
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::log(LogLevel::Level level, const LogEvent::ptr event){
        if (level >= m_level) {
            for (auto &i : m_appenders) {
                i->log(level, event);
            }
        }
    }

    void Logger::debug(const LogEvent::ptr event){
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(const LogEvent::ptr event){
        log(LogLevel::INFO, event);
    }

    void Logger::warn(const LogEvent::ptr event){
        log(LogLevel::WARN, event);
    }

    void Logger::error(const LogEvent::ptr event){
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(const LogEvent::ptr event){
        log(LogLevel::FATAL, event);
    }

    FileLogAppender::FileLogAppender(const std::string &filename)
        :m_filename(filename){

    }

    void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            m_filestream << m_formatter->format(event);
        }
    }

    bool FileLogAppender::reopen() {
        if (m_filestream) {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
    }

    void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            std::cout << m_formatter->format(event);
        }
    }

}