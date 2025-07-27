#include "Server.hpp"
#include "CommandHandler.hpp"

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
    std::map<int, User*>::iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        if (it->second) {
            std::set<std::string> channels = it->second->getCurrentChannels();
            std::set<std::string>::iterator ch_it;
            for (ch_it = channels.begin(); ch_it != channels.end(); ++ch_it) {
                Channel* channel = getChannel(*ch_it);
                if (channel) {
                    channel->removeUser(it->first);
                }
            }

            close(it->first);

            delete it->second;
        }
    }
    users.clear();

    channels.clear();

    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
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

    std::fill(reinterpret_cast<char*>(&server_addr), reinterpret_cast<char*>(&server_addr) + sizeof(server_addr), 0);
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

    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
        close(server_fd);
        throw std::runtime_error(std::string("Fcntl F_SETFL failed: ") + strerror(errno));
    }
}

void Server::run(volatile sig_atomic_t& shutdown_requested) {
    fd_set read_fds, write_fds;
    struct timeval tv;
    int max_fd = server_fd;

    std::cout << "Server is running on port " << port << "..." << std::endl;

    while (!shutdown_requested) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(server_fd, &read_fds);

        std::map<int, User*>::iterator it;
        for (it = users.begin(); it != users.end(); ++it) {
            FD_SET(it->first, &read_fds);

            if (!it->second->getWriteBuffer().empty()) {
                FD_SET(it->first, &write_fds);
            }
            if (it->first > max_fd) {
                max_fd = it->first;
            }
        }

        if (shutdown_requested) {
            std::cout << "Shutdown requested. Cleaning up all connections..." << std::endl;
            for (it = users.begin(); it != users.end(); ++it) {
                disconnectUser(it->first);
            }
            break;
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            handleNewConnection();
        }

        std::vector<int> fds_to_check;
        std::vector<int> fds_to_remove;

        std::map<int, User*> users_copy = users;

        for (std::map<int, User*>::iterator it = users_copy.begin(); it != users_copy.end(); ++it) {
            if (FD_ISSET(it->first, &read_fds)) {
                fds_to_check.push_back(it->first);
            }
        }

        for (std::vector<int>::iterator it = fds_to_check.begin(); it != fds_to_check.end(); ++it) {
            handleClientData(*it);
        }


        for (std::map<int, User*>::iterator it = users_copy.begin(); it != users_copy.end(); ++it) {
            if (FD_ISSET(it->first, &write_fds)) {
                handleWrite(it->first);
            }
        }

        static int check_counter = 0;
        if (++check_counter >= 3) {
            check_counter = 0;
            for (std::map<int, User*>::iterator it = users_copy.begin(); it != users_copy.end(); ++it) {
                if (!FD_ISSET(it->first, &read_fds)) {
                    if (checkClientConnection(it->first) == false) {
                        fds_to_remove.push_back(it->first);
                    }
                }
            }

            for (std::vector<int>::iterator it = fds_to_remove.begin(); it != fds_to_remove.end(); ++it) {
                std::cout << "Detected disconnected client: " << *it << std::endl;
                disconnectUser(*it);
            }
        }
    }
}

void Server::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "Accept error: " << strerror(errno) << std::endl;
        return;
    }

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Fcntl error for client " << client_fd << ": " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    std::cout << "New client connected: " << client_fd << " from " << client_ip << std::endl;

    try {
        User* newUser = new User(client_fd);
        newUser->setAuthenticated(false);
        users.insert(std::pair<int, User*>(client_fd, newUser));
    } catch (const std::exception& e) {
        std::cerr << "Error creating user for client " << client_fd << ": " << e.what() << std::endl;
        close(client_fd);
    }
}

void Server::handleClientData(int client_fd) {
    User* user = getUser(client_fd);
    if (!user) {
        if (client_fd > 0) {
            close(client_fd);
        }
        return;
    }

    char buffer[1024];
    std::fill(buffer, buffer + sizeof(buffer), 0);

    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, MSG_NOSIGNAL);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cout << "Client gracefully disconnected: " << client_fd << std::endl;
        } else {
            std::cerr << "Error reading from client " << client_fd << ": " << strerror(errno) << std::endl;
        }
        disconnectUser(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    std::string received_data(buffer);

    // Append received data to user's read buffer
    user->appendToReadBuffer(received_data);

    // Process complete commands from the buffer
    std::string& readBuffer = user->getReadBuffer();
    size_t pos = 0;
    size_t newline_pos;

    // Process all complete lines (ending with \r\n or \n)
    while ((newline_pos = readBuffer.find('\n', pos)) != std::string::npos) {
        // Find the start of this line (after previous \r\n)
        size_t line_start = pos;
        if (line_start > 0 && readBuffer[line_start - 1] == '\r') {
            line_start--;
        }

        // Extract the complete command line
        std::string command_line = readBuffer.substr(line_start, newline_pos - line_start);

        // Remove \r if present at the end
        if (!command_line.empty() && command_line[command_line.length() - 1] == '\r') {
            command_line = command_line.substr(0, command_line.length() - 1);
        }

        // Process the command if it's not empty
        if (!command_line.empty()) {
            try {
                CommandHandler handler(*this);
                handler.parseMessage(user, command_line);
            } catch (const std::exception& e) {
                std::cerr << "Error processing message: " << e.what() << std::endl;
            }
        }

        pos = newline_pos + 1;
    }

    // Keep any incomplete data in the buffer
    if (pos < readBuffer.length()) {
        readBuffer = readBuffer.substr(pos);
    } else {
        // All data was processed, clear the buffer
        user->clearReadBuffer();
    }
}

void Server::addUser(int fd) {
    users.insert(std::pair<int, User*>(fd, new User(fd)));
}

void Server::removeUser(int fd) {
    std::map<int, User*>::iterator it = users.find(fd);
    if (it != users.end()) {
        if (it->second) {
            // Clear read buffer to prevent memory leaks
            it->second->clearReadBuffer();

            std::set<std::string> channels = it->second->getCurrentChannels();
            std::set<std::string>::iterator ch_it;
            for (ch_it = channels.begin(); ch_it != channels.end(); ++ch_it) {
                Channel* channel = getChannel(*ch_it);
                if (channel) {
                    channel->removeUser(fd);
                }
            }

            for (ch_it = channels.begin(); ch_it != channels.end(); ++ch_it) {
                Channel* channel = getChannel(*ch_it);
                if (channel && channel->getUserCount() == 0) {
                    removeChannel(*ch_it);
                }
            }

            delete it->second;
        }

        users.erase(it);

        if (fd > 0) {
            close(fd);
        }
    }
}

User* Server::getUser(int fd) {
    std::map<int, User*>::iterator it = users.find(fd);
    return (it != users.end()) ? it->second : NULL;
}

Channel* Server::getChannel(const std::string& name) {
    std::map<std::string, Channel>::iterator it = channels.find(name);
    return (it != channels.end()) ? &(it->second) : NULL;
}

const Channel* Server::getChannel(const std::string& name) const {
    std::map<std::string, Channel>::const_iterator it = channels.find(name);
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
        channel->broadcast(0, message, this);
    }
}

void Server::handleWrite(int fd) {
    User* user = getUser(fd);
    if (!user) return;

    std::string& writeBuffer = user->getWriteBuffer();
    if (writeBuffer.empty()) return;

    int bytes_sent = send(fd, writeBuffer.c_str(), writeBuffer.length(), MSG_NOSIGNAL);

    if (bytes_sent < 0) {
        std::cerr << "Error writing to client " << fd << ": " << strerror(errno) << std::endl;
        disconnectUser(fd);
        return;
    }

    if (bytes_sent > 0) {
        writeBuffer = writeBuffer.substr(bytes_sent);
    }
}

bool Server::checkClientConnection(int client_fd) {
    int error = 0;
    socklen_t len = sizeof(error);
    int result = getsockopt(client_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    if (result < 0) {
        return false;
    }

    if (error != 0) {
        return false;
    }

    char buffer[1];
    result = recv(client_fd, buffer, 1, MSG_PEEK | MSG_NOSIGNAL);

    if (result < 0) {
        if (errno == EPIPE || errno == ECONNRESET || errno == ENOTCONN) {
            return false;
        }
    } else if (result == 0) {
        return false;
    }

    return true;
}

void Server::disconnectUser(int fd) {
    if (fd > 0) {
        shutdown(fd, SHUT_RDWR);
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }
    removeUser(fd);
}

int Server::getServerFd() const {
    return server_fd;
}

const std::string& Server::getPassword() const {
    return password;
}

const std::map<int, User*>& Server::getUsers() const {
    return users;
}

const std::map<std::string, Channel>& Server::getChannels() const {
    return channels;
}

