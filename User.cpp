#include "User.hpp"
#include <sys/socket.h>
#include <unistd.h>

User::User(int fd) : fd(fd), registered(false) {}

int User::getFd() const {
    return fd;
}

const std::string& User::getNickname() const {
    return nickname;
}

const std::string& User::getUsername() const {
    return username;
}

bool User::isRegistered() const {
    return registered;
}

const std::set<std::string>& User::getCurrentChannels() const {
    return current_channels;
}

void User::setNickname(const std::string& new_nickname) {
    nickname = new_nickname;
}

void User::setUsername(const std::string& new_username) {
    username = new_username;
}

void User::setRegistered(bool status) {
    registered = status;
}

void User::joinChannel(const std::string& channel_name) {
    current_channels.insert(channel_name);
}

void User::leaveChannel(const std::string& channel_name) {
    current_channels.erase(channel_name);
}

bool User::isInChannel(const std::string& channel_name) const {
    return current_channels.find(channel_name) != current_channels.end();
}

void User::sendMessage(const std::string& message) const {
    if (fd > 0) {
        send(fd, message.c_str(), message.length(), 0);
    }
} 