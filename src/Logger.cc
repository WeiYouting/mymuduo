#include "Logger.h"

#include <iostream>

#include "Timestamp.h"

/*
 * 获取日志唯一实例
 */
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}


/*
 * 设置日志级别
 */
void Logger::setLogLevel(int level)
{
    loglevel_ = level;
}

/*
 * 写日志
 */
void Logger::log(std::string message)
{
    switch (this->loglevel_)
    {
    case INFO:
        std::cout << "[INFO] ";
        break;
    case ERROR:
        std::cout << "[ERROR] ";
        break;
    case FATAL:
        std::cout << "[FATAL] "; 
        break;
    case  DEBUG:
        std::cout << "[DEBUG] "; 
        break;
    default:
        break;
    }

    // 打印时间
    std::cout << Timestamp::now().toString() << " " << message << "\n" << std::endl;

}