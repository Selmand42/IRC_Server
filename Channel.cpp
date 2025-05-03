#include "Channel.hpp"
#include "User.hpp"
#include <sys/socket.h>
#include <sstream>

Channel::Channel(const std::string& name) : 
    name(name), 
    userLimit(0),
    inviteOnly(false),
    topicRestricted(false),
    modeFlags("") {
}

std::string Channel::getName() const {
    return name;
}

const std::set<int>& Channel::getUsers() const {
    return users;
}

std::string Channel::getTopic() const {
    return topic;
}

std::string Channel::getModeFlags() const {
    return modeFlags;
}

std::string Channel::getPassword() const {
    return password;
}

int Channel::getUserLimit() const {
    return userLimit;
}

void Channel::setTopic(const std::string& new_topic) {
    topic = new_topic;
}

void Channel::setModeFlags(const std::string& modes) {
    modeFlags = modes;
}

void Channel::setPassword(const std::string& pass) {
    password = pass;
}

void Channel::setUserLimit(int limit) {
    userLimit = limit;
}

void Channel::addUser(int fd) {
    users.insert(fd);
    if (users.size() == 1) {
        // First user becomes operator
        addOperator(fd);
    }
}

void Channel::removeUser(int fd) {
    users.erase(fd);
    operators.erase(fd);
    invited.erase(fd);
}

bool Channel::hasUser(int fd) const {
    return users.find(fd) != users.end();
}

void Channel::addOperator(int fd) {
    operators.insert(fd);
}

void Channel::removeOperator(int fd) {
    operators.erase(fd);
}

bool Channel::isOperator(int fd) const {
    return operators.find(fd) != operators.end();
}

void Channel::addInvited(int fd) {
    invited.insert(fd);
}

bool Channel::isInvited(int fd) const {
    return invited.find(fd) != invited.end();
}

void Channel::broadcast(int sender_fd, const std::string& message) {
    std::set<int>::iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        if (*it != sender_fd) { // Don't send to the sender
            send(*it, message.c_str(), message.length(), 0);
        }
    }
}

size_t Channel::getUserCount() const {
    return users.size();
}

void Channel::setInviteOnly(bool value) {
    inviteOnly = value;
}

bool Channel::isInviteOnly() const {
    return inviteOnly;
}

void Channel::setTopicRestricted(bool value) {
    topicRestricted = value;
}

bool Channel::isTopicRestricted() const {
    return topicRestricted;
} 