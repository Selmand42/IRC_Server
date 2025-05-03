#include "Channel.hpp"
#include <sys/socket.h>

Channel::Channel(const std::string& name) : name(name), user_limit(0) {}

const std::string& Channel::getName() const {
    return name;
}

const std::set<int>& Channel::getUsers() const {
    return users;
}

const std::set<int>& Channel::getOperators() const {
    return operators;
}

const std::string& Channel::getTopic() const {
    return topic;
}

const std::string& Channel::getModeFlags() const {
    return mode_flags;
}

const std::string& Channel::getPassword() const {
    return password;
}

int Channel::getUserLimit() const {
    return user_limit;
}

void Channel::setTopic(const std::string& new_topic) {
    topic = new_topic;
}

void Channel::setModeFlags(const std::string& flags) {
    mode_flags = flags;
}

void Channel::setPassword(const std::string& new_password) {
    password = new_password;
}

void Channel::setUserLimit(int limit) {
    user_limit = limit;
}

void Channel::addUser(int fd) {
    users.insert(fd);
}

void Channel::removeUser(int fd) {
    users.erase(fd);
    operators.erase(fd); // Remove from operators if present
}

bool Channel::hasUser(int fd) const {
    return users.find(fd) != users.end();
}

void Channel::addOperator(int fd) {
    if (hasUser(fd)) {
        operators.insert(fd);
    }
}

void Channel::removeOperator(int fd) {
    operators.erase(fd);
}

bool Channel::isOperator(int fd) const {
    return operators.find(fd) != operators.end();
}

void Channel::broadcast(int sender_fd, const std::string& message) const {
    std::set<int>::const_iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        if (*it != sender_fd) { // Don't send to the sender
            send(*it, message.c_str(), message.length(), 0);
        }
    }
}

size_t Channel::getUserCount() const {
    return users.size();
} 