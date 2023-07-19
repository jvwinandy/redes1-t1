set(SOURCES
        Logger.cpp)

set(HEADERS
        Logger.h)

add_library(logger_lib SHARED ${SOURCES} ${HEADERS})
