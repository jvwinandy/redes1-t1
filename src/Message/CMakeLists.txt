set(SOURCES
        Message.cpp)

set(HEADERS
        Message.h)

add_library(message_lib SHARED ${SOURCES} ${HEADERS})
