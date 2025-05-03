#include "Server.hpp"
#include <iostream>
#include <cstdlib>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <port> <password>" << std::endl;
    std::cout << "Example: " << programName << " 6667 password123" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage(argv[0]);
        return 1;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cout << "Error: Invalid port number. Port must be between 1 and 65535." << std::endl;
        return 1;
    }

    try {
        Server server(port, argv[2]);
        std::cout << "IRC Server started on port " << port << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 