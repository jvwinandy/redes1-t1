#include "Server.h"
#include <iostream>

int main() {
    Logger::setLevel(LoggerLevel::INFO);

    std::cout << "Starting server." << std::endl;
    Server server;

    if (DEVICE == "lo") {
        NetworkNode::message_delimiter = BEGIN_DELIMITER;
    }

    while (true) {
        server.waitSequence();
    }

    return 0;
}