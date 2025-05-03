#include "User.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

User::User(int fd) : 
    fd(fd), 
    registered(false),
    invisible(false),
    operator_(false),
    wallops(false),
    restricted(false),
    server_notices(false) {
}

int User::getFd() const {
    return fd;
}

const std::string& User::getNickname() const {
    return nickname;
}

const std::string& User::getUsername() const {
    return username;
}

const std::string& User::getRealname() const {
    return realname;
}

bool User::isRegistered() const {
    return registered;
}

const std::set<std::string>& User::getCurrentChannels() const {
    return channels;
}

std::string User::getModeFlags() const {
    std::string flags = "+";
    if (invisible) flags += "i";
    if (operator_) flags += "o";
    if (wallops) flags += "w";
    if (restricted) flags += "r";
    if (server_notices) flags += "s";
    return flags;
}

void User::setNickname(const std::string& nick) {
    nickname = nick;
}

void User::setUsername(const std::string& user) {
    username = user;
}

void User::setRealname(const std::string& real) {
    realname = real;
}

void User::setRegistered(bool value) {
    registered = value;
}

void User::setModeFlags(const std::string& modes) {
    bool adding = true;
    for (size_t i = 0; i < modes.length(); ++i) {
        if (modes[i] == '+') {
            adding = true;
            continue;
        }
        if (modes[i] == '-') {
            adding = false;
            continue;
        }
        
        switch (modes[i]) {
            case 'i': setInvisible(adding); break;
            case 'o': setOperator(adding); break;
            case 'w': setWallops(adding); break;
            case 'r': setRestricted(adding); break;
            case 's': setServerNotices(adding); break;
        }
    }
}

void User::joinChannel(const std::string& channel) {
    channels.insert(channel);
}

void User::leaveChannel(const std::string& channel) {
    channels.erase(channel);
}

bool User::isInChannel(const std::string& channel_name) const {
    return channels.find(channel_name) != channels.end();
}

void User::sendMessage(const std::string& message) const {
    if (fd > 0) {
        send(fd, message.c_str(), message.length(), 0);
    }
}

void User::setInvisible(bool value) {
    invisible = value;
}

void User::setOperator(bool value) {
    operator_ = value;
}

void User::setWallops(bool value) {
    wallops = value;
}

void User::setRestricted(bool value) {
    restricted = value;
}

void User::setServerNotices(bool value) {
    server_notices = value;
}

bool User::isInvisible() const {
    return invisible;
}

bool User::isOperator() const {
    return operator_;
}

bool User::isWallops() const {
    return wallops;
}

bool User::isRestricted() const {
    return restricted;
}

bool User::isServerNotices() const {
    return server_notices;
} 