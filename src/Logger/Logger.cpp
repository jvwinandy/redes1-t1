#include "Logger.h"

Logger *Logger::m_instance;
LoggerLevel Logger::m_level;

Logger *Logger::getInstance() {
    if (Logger::m_instance != nullptr) {
        Logger::m_instance = new Logger();
    }
    return Logger::m_instance;
}

void Logger::setLevel(LoggerLevel level) {
    Logger::m_level = level;
}

void Logger::log(string const& text, LoggerLevel level) {
    if (Logger::m_level <= level) {
        cout << toString(level) << ": " << text << endl;
    }
}

string toString(LoggerLevel loggerLevel) {
    switch (loggerLevel) {
        case LoggerLevel::DEBUG:
            return "DEBUG";
        case LoggerLevel::INFO:
            return "INFO";
        case LoggerLevel::WARN:
            return "WARNING";
        case LoggerLevel::ERROR:
            return "ERROR";
        case LoggerLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "Default";
    }
}
