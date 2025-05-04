#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include "User.hpp"
#include "Channel.hpp"

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

public:
    Server(int port, const std::string& password);
    ~Server();

    void run();
    void acceptConnection();
    void disconnectUser(int fd);
    void handleRead(int fd);
    void handleWrite(int fd);
    void broadcast(const std::string& channel_name, const std::string& message);

    // Getters
    int getServerFd() const;
    const std::string& getPassword() const;
    const std::map<int, User*>& getUsers() const;
    const std::map<std::string, Channel>& getChannels() const;

    // User management
    void addUser(int fd);
    void removeUser(int fd);
    User* getUser(int fd);
    
    // Channel management
    Channel* getChannel(const std::string& name);
    void createChannel(const std::string& name);
    void removeChannel(const std::string& name);
};

#endif // SERVER_HPP