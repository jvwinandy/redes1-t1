set(SOURCES
        Client.cpp
        main.cpp)

set(HEADERS
        Client.h)

add_executable(client ${SOURCES} ${HEADERS})

target_link_libraries(client PUBLIC logger_lib files_lib network_lib message_lib)