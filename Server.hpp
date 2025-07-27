#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include "User.hpp"
#include "Channel.hpp"
#include <signal.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cctype>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <algorithm>
#include <errno.h>
#include <sys/select.h>


class Server {
private:
    int server_fd;
    int port;
    std::string password;
    std::map<int, User*> users;
    std::map<std::string, Channel> channels;

    void setupServer();
    void handleNewConnection();
    void handleClientData(int client_fd);
    bool checkClientConnection(int client_fd);

public:
    Server(int port, const std::string& password);
    ~Server();

    void run(volatile sig_atomic_t& shutdown_requested);
    void disconnectUser(int fd);
    void handleWrite(int fd);
    void broadcast(const std::string& channel_name, const std::string& message);

    int getServerFd() const;
    const std::string& getPassword() const;
    const std::map<int, User*>& getUsers() const;
    const std::map<std::string, Channel>& getChannels() const;

    void addUser(int fd);
    void removeUser(int fd);
    User* getUser(int fd);

    Channel* getChannel(const std::string& name);
    const Channel* getChannel(const std::string& name) const;
    void createChannel(const std::string& name);
    void removeChannel(const std::string& name);
};

#endif
