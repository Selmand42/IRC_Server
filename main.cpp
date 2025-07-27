#include "Server.hpp"

Server* g_server = NULL;
volatile sig_atomic_t g_shutdown_requested = 0;

void signalHandler(int signum) {
    if (g_server) {
        std::cout << "\nReceived signal " << signum << ". Shutting down server..." << std::endl;
        g_shutdown_requested = 1;
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <port> <password>" << std::endl;
    std::cout << "Example: " << programName << " 6667 password123" << std::endl;
}

bool isNumeric(const char* str) {
    if (!str || *str == '\0') {
        return false;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage(argv[0]);
        return 1;
    }

    if (!isNumeric(argv[1])) {
        std::cout << "Error: Port number must contain only numeric characters." << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cout << "Error: Invalid port number. Port must be between 1 and 65535." << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        Server server(port, argv[2]);
        g_server = &server;

        std::cout << "IRC Server started on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;

        while (!g_shutdown_requested) {
            server.run(g_shutdown_requested);
            if (g_shutdown_requested) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
