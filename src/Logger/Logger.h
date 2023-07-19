#ifndef REDES_1_T1_LOGGER_H
#define REDES_1_T1_LOGGER_H

#include <iostream>
#include <sstream>
using namespace std;

enum class LoggerLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

string toString(LoggerLevel loggerLevel);

/**
 * @brief Logger class to facilitate logging of the server and the clien, works in a similar way to python3 logger.
 */
class Logger {
public:
    static Logger *getInstance();
    static void setLevel(LoggerLevel level);

    void debug(string const& text) { log(text, LoggerLevel::DEBUG); }
    void info(string const& text) { log(text, LoggerLevel::INFO); }
    void warn(string const& text) { log(text, LoggerLevel::WARN); }
    void error(string const& text) { log(text, LoggerLevel::ERROR); }
    void critical(string const& text) { log(text, LoggerLevel::CRITICAL); }

protected:
    Logger() = default;

private:
    static Logger* m_instance;
    static LoggerLevel m_level;

    void log(string const& text, LoggerLevel level);
};


#endif //REDES_1_T1_LOGGER_H
