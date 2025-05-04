#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>

class Channel {
private:
    std::string name;
    std::string topic;
    std::set<int> users;
    std::set<int> operators;
    std::set<int> invited;
    std::string password;
    int userLimit;
    bool inviteOnly;
    bool topicRestricted;
    std::string modeFlags;

public:
    Channel(const std::string& name);
    
    // User management
    void addUser(int fd);
    void removeUser(int fd);
    bool hasUser(int fd) const;
    void addOperator(int fd);
    void removeOperator(int fd);
    bool isOperator(int fd) const;
    void addInvited(int fd);
    bool isInvited(int fd) const;
    
    // Channel modes
    void setModeFlags(const std::string& modes);
    std::string getModeFlags() const;
    void setPassword(const std::string& pass);
    std::string getPassword() const;
    void setUserLimit(int limit);
    int getUserLimit() const;
    void setInviteOnly(bool value);
    bool isInviteOnly() const;
    void setTopicRestricted(bool value);
    bool isTopicRestricted() const;
    
    // Channel info
    std::string getName() const;
    void setTopic(const std::string& newTopic);
    std::string getTopic() const;
    const std::set<int>& getUsers() const;
    unsigned int getUserCount() const;
    
    // Message handling
    void broadcast(int sender_fd, const std::string& message);
};

#endif // CHANNEL_HPP 