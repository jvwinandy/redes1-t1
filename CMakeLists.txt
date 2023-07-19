cmake_minimum_required(VERSION 3.22)
project(redes_1_t1)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src)

install(TARGETS
            server
            client
            logger_lib
            message_lib
            network_lib
        DESTINATION
            ${CMAKE_BINARY_DIR}
        )