#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
private:
    std::string name;
    std::set<int> users;
    std::set<int> operators;
    std::string topic;
    std::string mode_flags;
    std::string password;
    int user_limit;

public:
    Channel(const std::string& name);
    
    // Getters
    const std::string& getName() const;
    const std::set<int>& getUsers() const;
    const std::set<int>& getOperators() const;
    const std::string& getTopic() const;
    const std::string& getModeFlags() const;
    const std::string& getPassword() const;
    int getUserLimit() const;

    // Setters
    void setTopic(const std::string& new_topic);
    void setModeFlags(const std::string& flags);
    void setPassword(const std::string& new_password);
    void setUserLimit(int limit);

    // User management
    void addUser(int fd);
    void removeUser(int fd);
    bool hasUser(int fd) const;
    void addOperator(int fd);
    void removeOperator(int fd);
    bool isOperator(int fd) const;

    // Channel operations
    void broadcast(int sender_fd, const std::string& message) const;
    size_t getUserCount() const;
};

#endif // CHANNEL_HPP 