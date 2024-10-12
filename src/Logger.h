#pragma once

#include <string>

#include "noncopyable.h"


#define LOG_INFO(logmsgFormat,...)\
    do{\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0);


#define LOG_ERROR(logmsgFormat,...)\
    do{\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(ERROR);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0);


#define LOG_FATAL(logmsgFormat,...)\
    do{\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(FATAL);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
        exit(-1);\
    }while(0);


#define LOG_DEBUG(logmsgFormat,...)\
    do{\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(DEBUG);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0);


// 定义日志级别 INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};


class Logger
{
public:
    // 获取日志唯一实例
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string message);
private:
    int loglevel_;
    Logger(){};
};

