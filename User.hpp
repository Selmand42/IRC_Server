#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <set>
#include <sys/socket.h>

class User {
private:
    int fd;
    std::string nickname;
    std::string username;
    std::string realname;
    bool registered;
    bool authenticated;
    std::set<std::string> channels;
    std::string modeFlags;
    bool invisible;
    bool operator_;
    bool wallops;
    bool restricted;
    bool server_notices;
    mutable std::string writeBuffer;

public:
    // Constructor
    User(int fd);
    
    // Getters
    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getRealname() const;
    bool isRegistered() const;
    bool isAuthenticated() const;
    const std::set<std::string>& getCurrentChannels() const;
    std::string getModeFlags() const;
    std::string& getWriteBuffer() const;

    // Setters
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    void setRealname(const std::string& real);
    void setRegistered(bool value);
    void setAuthenticated(bool value);
    void setModeFlags(const std::string& modes);

    // Channel operations
    void joinChannel(const std::string& channel);
    void leaveChannel(const std::string& channel);
    bool isInChannel(const std::string& channel_name) const;

    // Message operations
    void sendMessage(const std::string& message) const;

    // Mode management
    void setInvisible(bool value);
    void setOperator(bool value);
    void setWallops(bool value);
    void setRestricted(bool value);
    void setServerNotices(bool value);
    
    bool isInvisible() const;
    bool isOperator() const;
    bool isWallops() const;
    bool isRestricted() const;
    bool isServerNotices() const;
};

#endif // USER_HPP 