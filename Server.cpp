#include "Server.hpp"
#include "CommandHandler.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <errno.h>
#include <sys/select.h>

Server::Server(int port, const std::string& password) : server_fd(-1), port(port), password(password) {
    try {
        setupServer();
    } catch (const std::exception& e) {
        if (server_fd > 0) {
            close(server_fd);
        }
        throw;
    }
}

Server::~Server() {
    if (server_fd > 0) {
        close(server_fd);
    }
}

void Server::setupServer() {
    struct sockaddr_in server_addr;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error(std::string("Socket creation failed: ") + strerror(errno));
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        throw std::runtime_error(std::string("Setsockopt failed: ") + strerror(errno));
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        throw std::runtime_error(std::string("Bind failed: ") + strerror(errno));
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        throw std::runtime_error(std::string("Listen failed: ") + strerror(errno));
    }

    // Set non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags < 0) {
        close(server_fd);
        throw std::runtime_error(std::string("Fcntl F_GETFL failed: ") + strerror(errno));
    }
    
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(server_fd);
        throw std::runtime_error(std::string("Fcntl F_SETFL failed: ") + strerror(errno));
    }
}

void Server::run() {
    fd_set read_fds;
    struct timeval tv;
    int max_fd = server_fd;

    std::cout << "Server is running on port " << port << "..." << std::endl;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        // Add all client sockets to the set
        std::map<int, User>::iterator it;
        for (it = users.begin(); it != users.end(); ++it) {
            FD_SET(it->first, &read_fds);
            if (it->first > max_fd) {
                max_fd = it->first;
            }
        }

        // Set timeout
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // Wait for activity
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted system call, just continue
            }
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            continue;
        }

        // Check for new connections
        if (FD_ISSET(server_fd, &read_fds)) {
            handleNewConnection();
        }

        // Check for client activity
        std::map<int, User>::iterator it2;
        for (it2 = users.begin(); it2 != users.end();) {
            if (FD_ISSET(it2->first, &read_fds)) {
                handleClientData(it2->first);
            }
            ++it2;
        }
    }
}

void Server::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Accept error: " << strerror(errno) << std::endl;
        }
        return;
    }

    // Set non-blocking mode for the client socket
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags < 0) {
        close(client_fd);
        return;
    }
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(client_fd);
        return;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    std::cout << "New client connected: " << client_fd << " from " << client_ip << std::endl;
    
    // Add user but mark as unauthenticated
    User newUser(client_fd);
    newUser.setAuthenticated(false);
    users.insert(std::pair<int, User>(client_fd, newUser));
}

void Server::handleClientData(int client_fd) {
    User* user = getUser(client_fd);
    if (!user) {
        std::cerr << "Error: User not found for fd " << client_fd << std::endl;
        return;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cout << "Client disconnected: " << client_fd << std::endl;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error reading from client: " << strerror(errno) << std::endl;
        }
        disconnectUser(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string message(buffer);

    // Check if user is authenticated
    if (!user->isAuthenticated()) {
        // First message should be the password
        if (message == password) {
            user->setAuthenticated(true);
            user->sendMessage(":server 001 :Welcome to the IRC server!");
        } else {
            user->sendMessage(":server 464 :Password incorrect");
            disconnectUser(client_fd);
            return;
        }
    } else {
        try {
            CommandHandler handler(*this);
            handler.parseMessage(*user, message);
        } catch (const std::exception& e) {
            std::cerr << "Error processing message: " << e.what() << std::endl;
        }
    }
}

void Server::addUser(int fd) {
    users.insert(std::pair<int, User>(fd, User(fd)));
}

void Server::removeUser(int fd) {
    std::map<int, User>::iterator it = users.find(fd);
    if (it != users.end()) {
        // Remove user from all channels
        std::set<std::string> channels = it->second.getCurrentChannels();
        std::set<std::string>::iterator ch_it;
        for (ch_it = channels.begin(); ch_it != channels.end(); ++ch_it) {
            Channel* channel = getChannel(*ch_it);
            if (channel) {
                channel->removeUser(fd);
            }
        }
        users.erase(it);
        close(fd);
    }
}

User* Server::getUser(int fd) {
    std::map<int, User>::iterator it = users.find(fd);
    return (it != users.end()) ? &(it->second) : NULL;
}

Channel* Server::getChannel(const std::string& name) {
    std::map<std::string, Channel>::iterator it = channels.find(name);
    return (it != channels.end()) ? &(it->second) : NULL;
}

void Server::createChannel(const std::string& name) {
    if (channels.find(name) == channels.end()) {
        channels.insert(std::pair<std::string, Channel>(name, Channel(name)));
    }
}

void Server::removeChannel(const std::string& name) {
    channels.erase(name);
}

void Server::broadcast(const std::string& channel_name, const std::string& message) {
    Channel* channel = getChannel(channel_name);
    if (channel) {
        channel->broadcast(0, message); // 0 as sender_fd means server broadcast
    }
}

void Server::handleRead(int fd) {
    User* user = getUser(fd);
    if (!user) {
        std::cerr << "Error: User not found for fd " << fd << std::endl;
        return;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cout << "Client disconnected: " << fd << std::endl;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error reading from client: " << strerror(errno) << std::endl;
        }
        disconnectUser(fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string message(buffer);
    
    try {
        CommandHandler handler(*this);
        handler.parseMessage(*user, message);
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

void Server::handleWrite(int fd) {
    User* user = getUser(fd);
    if (!user) return;

    // Get the write buffer for this user
    std::string& writeBuffer = user->getWriteBuffer();
    if (writeBuffer.empty()) return;

    // Try to send the data
    int bytes_sent = send(fd, writeBuffer.c_str(), writeBuffer.length(), 0);
    
    if (bytes_sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error writing to client: " << strerror(errno) << std::endl;
            disconnectUser(fd);
        }
        return;
    }

    // Remove the sent data from the buffer
    if (bytes_sent > 0) {
        writeBuffer = writeBuffer.substr(bytes_sent);
    }
}

void Server::disconnectUser(int fd) {
    removeUser(fd);
}

// Getters implementation
int Server::getServerFd() const {
    return server_fd;
}

const std::string& Server::getPassword() const {
    return password;
}

const std::map<int, User>& Server::getUsers() const {
    return users;
}

const std::map<std::string, Channel>& Server::getChannels() const {
    return channels;
}
