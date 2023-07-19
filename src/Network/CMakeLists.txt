set(SOURCES
        ConexaoRawSocket.cpp
        NetworkNode.cpp)

set(HEADERS
        ConexaoRawSocket.h
        NetworkNode.h
        RawSocketIncludes.h)

add_library(network_lib SHARED ${SOURCES} ${HEADERS})
target_link_libraries(network_lib PUBLIC files_lib)