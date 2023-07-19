set(SOURCES
        Server.cpp
        main.cpp)

set(HEADERS
        Server.h)

add_executable(server ${SOURCES} ${HEADERS})

target_link_libraries(server PUBLIC logger_lib files_lib network_lib message_lib)