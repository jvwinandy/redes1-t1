#include "Client.h"
#include <iostream>

int main() {
    Logger::setLevel(LoggerLevel::INFO);

    std::cout << "Starting client." << std::endl;
    Client client;

    if (DEVICE == "lo") {
        NetworkNode::message_delimiter = ~BEGIN_DELIMITER;
    }

    while (true) {
        client.waitCommand();
    }

    return 0;
}