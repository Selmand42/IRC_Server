#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <set>

class User {
private:
    int fd;
    std::string nickname;
    std::string username;
    bool registered;
    std::set<std::string> current_channels;

public:
    // Constructor
    User(int fd);
    
    // Getters
    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    bool isRegistered() const;
    const std::set<std::string>& getCurrentChannels() const;

    // Setters
    void setNickname(const std::string& new_nickname);
    void setUsername(const std::string& new_username);
    void setRegistered(bool status);

    // Channel operations
    void joinChannel(const std::string& channel_name);
    void leaveChannel(const std::string& channel_name);
    bool isInChannel(const std::string& channel_name) const;

    // Message operations
    void sendMessage(const std::string& message) const;
};

#endif // USER_HPP 