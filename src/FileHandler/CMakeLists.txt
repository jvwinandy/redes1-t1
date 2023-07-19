set(SOURCES
        fileHandler.cpp)

set(HEADERS
        fileHandler.h)

add_library(files_lib SHARED ${SOURCES} ${HEADERS})
