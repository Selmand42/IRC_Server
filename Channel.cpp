#include "Channel.hpp"
#include "User.hpp"
#include "Server.hpp"

Channel::Channel(const std::string& name) :
    name(name),
    userLimit(0),
    inviteOnly(false),
    topicRestricted(false) {
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
    std::string flags = "+";
    if (inviteOnly) flags += "i";
    if (topicRestricted) flags += "t";
    return flags;
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

void Channel::setPassword(const std::string& pass) {
    password = pass;
}

void Channel::setUserLimit(int limit) {
    userLimit = limit;
}

void Channel::addUser(int fd) {
    users.insert(fd);
    if (users.size() == 1)
        addOperator(fd);
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

bool Channel::isLastOperator(int fd) const {
    return operators.size() == 1 && isOperator(fd);
}

void Channel::addInvited(int fd) {
    invited.insert(fd);
}

bool Channel::isInvited(int fd) const {
    return invited.find(fd) != invited.end();
}

void Channel::removeInvited(int fd) {
    invited.erase(fd);
}

void Channel::broadcast(int sender_fd, const std::string& message, Server* server) {
    std::set<int>::iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        if (*it != sender_fd) {
            if (*it > 0) {
                if (server) {

                    User* user = server->getUser(*it);
                    if (user) {
                        user->sendMessage(message);
                    }
                } else {

                    int result = send(*it, (message + "\r\n").c_str(), message.length() + 2, MSG_NOSIGNAL);
                    if (result < 0)
                        std::cerr << "Error sending message to fd " << *it << ": " << strerror(errno) << std::endl;
                }
            }
        }
    }
}

unsigned int Channel::getUserCount() const {
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
