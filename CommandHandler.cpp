#include "CommandHandler.hpp"
#include <sstream>
#include <algorithm>

CommandHandler::CommandHandler(Server& server) : server(server) {}

void CommandHandler::parseMessage(User* user, const std::string& message) {
    std::vector<std::string> parts = splitMessage(message);
    if (parts.empty()) return;

    std::string command = parts[0];
    // Convert command to uppercase
    for (std::string::iterator it = command.begin(); it != command.end(); ++it) {
        *it = toupper(*it);
    }

    parts.erase(parts.begin()); // Remove command from args
    executeCommand(user, command, parts);
}

void CommandHandler::executeCommand(User* user, const std::string& command, const std::vector<std::string>& args) {
    // Only PASS command is allowed without authentication
    if (command != "PASS" && !isUserAuthenticated(user)) {
        return;
    }

    // Only PASS, NICK, and USER commands are allowed before registration
    if (command != "PASS" && command != "NICK" && command != "USER" && !isUserRegistered(user)) {
        return;
    }

    if (command == "PASS") {
        handlePass(user, args);
    } else if (command == "NICK") {
        handleNick(user, args);
    } else if (command == "USER") {
        handleUser(user, args);
    } else if (command == "JOIN") {
        handleJoin(user, args);
    } else if (command == "PART") {
        handlePart(user, args);
    } else if (command == "PRIVMSG") {
        handlePrivmsg(user, args);
    } else if (command == "NOTICE") {
        handleNotice(user, args);
    } else if (command == "QUIT") {
        handleQuit(user, args);
    } else if (command == "KICK") {
        handleKick(user, args);
    } else if (command == "MODE") {
        handleMode(user, args);
    } else if (command == "TOPIC") {
        handleTopic(user, args);
    } else if (command == "INVITE") {
        handleInvite(user, args);
    }
}

std::vector<std::string> CommandHandler::splitMessage(const std::string& message) {
    std::vector<std::string> result;
    std::istringstream iss(message);
    std::string token;

    while (iss >> token) {
        result.push_back(token);
    }

    return result;
}

bool CommandHandler::isValidNickname(const std::string& nickname) {
    if (nickname.empty() || nickname.length() > 9) return false;

    // First character must be a letter
    if (!isalpha(nickname[0])) return false;

    // Rest can be letters, numbers, or special characters
    for (size_t i = 1; i < nickname.length(); ++i) {
        if (!isalnum(nickname[i]) && nickname[i] != '-' && nickname[i] != '_') {
            return false;
        }
    }

    return true;
}

bool CommandHandler::isValidChannelName(const std::string& channel) {
    if (channel.empty() || channel.length() > 50) return false;

    // Channel must start with # or &
    if (channel[0] != '#' && channel[0] != '&') return false;

    // Rest can be any character except space, comma, or control G
    for (size_t i = 1; i < channel.length(); ++i) {
        if (channel[i] == ' ' || channel[i] == ',' || channel[i] == 7) {
            return false;
        }
    }

    return true;
}

bool CommandHandler::isUserAuthenticated(User* user) {
    if (!user->isAuthenticated()) {
        user->sendMessage(":server 464 * :Password required");
        return false;
    }
    return true;
}

bool CommandHandler::isUserRegistered(User* user) {
    if (!user->isRegistered()) {
        user->sendMessage(":server 451 * :You have not registered");
        return false;
    }
    return true;
}

void CommandHandler::handleNick(User* user, const std::vector<std::string>& args) {
    if (!isUserAuthenticated(user)) {
        return;
    }

    if (args.empty()) {
        user->sendMessage(":server 431 * :No nickname given");
        return;
    }

    std::string newNick = args[0];
    if (!isValidNickname(newNick)) {
        user->sendMessage(":server 432 * " + newNick + " :Erroneous nickname");
        return;
    }

    // Check if nickname is already in use
    const std::map<int, User*>& users = server.getUsers();
    std::map<int, User*>::const_iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        if (it->second->getNickname() == newNick) {
            user->sendMessage(":server 433 * " + newNick + " :Nickname is already in use");
            return;
        }
    }

    user->setNickname(newNick);

    // If user has both nickname and username set, mark as registered
    if (!user->getUsername().empty() && !user->isRegistered()) {
        user->setRegistered(true);
        user->sendMessage(":server 001 " + user->getNickname() + " :Welcome to the IRC Network " + user->getNickname());
    }
}

void CommandHandler::handleUser(User* user, const std::vector<std::string>& args) {
    if (!isUserAuthenticated(user)) {
        return;
    }

    if (args.size() < 4) {
        user->sendMessage(":server 461 * USER :Not enough parameters");
        return;
    }

    if (user->isRegistered()) {
        user->sendMessage(":server 462 * :You may not reregister");
        return;
    }

    user->setUsername(args[0]);
    user->setRealname(args[3]);

    // If user has both nickname and username set, mark as registered
    if (!user->getNickname().empty() && !user->isRegistered()) {
        user->setRegistered(true);
        user->sendMessage(":server 001 " + user->getNickname() + " :Welcome to the IRC Network " + user->getNickname());
    }
}

void CommandHandler::handleJoin(User* user, const std::vector<std::string>& args) {
    if (args.empty()) {
        user->sendMessage(":server 461 JOIN :Not enough parameters");
        return;
    }

    std::string channel_name = args[0];
    if (!isValidChannelName(channel_name)) {
        user->sendMessage(":server 403 " + channel_name + " :No such channel");
        return;
    }

    Channel* channel = server.getChannel(channel_name);
    if (!channel) {
        server.createChannel(channel_name);
        channel = server.getChannel(channel_name);
    }

    // Check channel modes and restrictions
    if (channel->isInviteOnly() && !channel->isInvited(user->getFd())) {
        user->sendMessage(":server 473 " + channel_name + " :Cannot join channel (+i)");
        return;
    }

    if (!channel->getPassword().empty()) {
        if (args.size() < 2 || args[1] != channel->getPassword()) {
            user->sendMessage(":server 475 " + channel_name + " :Cannot join channel (+k)");
            return;
        }
    }

    if (channel->getUserLimit() > 0 && channel->getUserCount() >= (size_t)channel->getUserLimit()) {
        user->sendMessage(":server 471 " + channel_name + " :Cannot join channel (+l)");
        return;
    }

    channel->addUser(user->getFd());
    user->joinChannel(channel_name);

    // Send join message to all users in the channel
    std::string join_msg = ":" + user->getNickname() + " JOIN :" + channel_name;
    channel->broadcast(user->getFd(), join_msg);

    // Send channel topic if exists
    if (!channel->getTopic().empty()) {
        user->sendMessage(":server 332 " + user->getNickname() + " " + channel_name + " :" + channel->getTopic());
    }

    // Send channel mode information
    std::stringstream ss;
    ss << ":" << server.getServerFd() << " 324 " << user->getNickname() << " " << channel_name << " " << channel->getModeFlags();
    if (!channel->getPassword().empty()) {
        ss << " " << channel->getPassword();
    }
    if (channel->getUserLimit() > 0) {
        ss << " " << channel->getUserLimit();
    }
    user->sendMessage(ss.str());

    // Send list of users in channel
    ss.str("");
    ss << ":" << server.getServerFd() << " 353 " << user->getNickname() << " = " << channel_name << " :";
    const std::set<int>& users = channel->getUsers();
    std::set<int>::const_iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        User* channel_user = server.getUser(*it);
        if (channel_user) {
            if (channel->isOperator(*it)) {
                ss << "@";
            }
            ss << channel_user->getNickname() << " ";
        }
    }
    user->sendMessage(ss.str());

    // End of names list
    user->sendMessage(":server 366 " + user->getNickname() + " " + channel_name + " :End of /NAMES list");
}

void CommandHandler::handlePart(User* user, const std::vector<std::string>& args) {
    if (args.empty()) {
        user->sendMessage(":server 461 PART :Not enough parameters");
        return;
    }

    std::string channel_name = args[0];
    Channel* channel = server.getChannel(channel_name);
    if (!channel) {
        user->sendMessage(":server 403 " + channel_name + " :No such channel");
        return;
    }

    if (!channel->hasUser(user->getFd())) {
        user->sendMessage(":server 442 " + channel_name + " :You're not on that channel");
        return;
    }

    std::string part_msg = ":" + user->getNickname() + " PART :" + channel_name;
    channel->broadcast(user->getFd(), part_msg);

    channel->removeUser(user->getFd());
    user->leaveChannel(channel_name);
}

void CommandHandler::handlePrivmsg(User* user, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        user->sendMessage(":server 411 :No recipient given");
        return;
    }

    std::string target = args[0];
    std::string message;

    // Combine all remaining arguments into one message
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) message += " ";
        message += args[i];
    }

    // Remove leading colon if present
    if (!message.empty() && message[0] == ':') {
        message = message.substr(1);
    }

    if (target[0] == '#' || target[0] == '&') {
        // Channel message
        Channel* channel = server.getChannel(target);
        if (!channel) {
            user->sendMessage(":server 403 " + target + " :No such channel");
            return;
        }

        if (!channel->hasUser(user->getFd())) {
            user->sendMessage(":server 404 " + target + " :Cannot send to channel");
            return;
        }

        std::string msg = ":" + user->getNickname() + "!~" + user->getUsername() + "@localhost PRIVMSG " + target + " :" + message;
        channel->broadcast(user->getFd(), msg);
    } else {
        // Private message
        const std::map<int, User*>& users = server.getUsers();
        std::map<int, User*>::const_iterator it;
        bool found = false;

        for (it = users.begin(); it != users.end(); ++it) {
            if (it->second->getNickname() == target) {
                std::string msg = ":" + user->getNickname() + "!~" + user->getUsername() + "@localhost PRIVMSG " + target + " :" + message;
                it->second->sendMessage(msg);
                found = true;
                break;
            }
        }

        if (!found) {
            user->sendMessage(":server 401 " + target + " :No such nick");
        }
    }
}

void CommandHandler::handleNotice(User* user, const std::vector<std::string>& args) {
    // Similar to PRIVMSG but no error responses
    if (args.size() < 2) return;

    std::string target = args[0];
    std::string message = args[1];

    if (target[0] == '#' || target[0] == '&') {
        Channel* channel = server.getChannel(target);
        if (channel && channel->hasUser(user->getFd())) {
            std::string msg = ":" + user->getNickname() + " NOTICE " + target + " :" + message;
            channel->broadcast(user->getFd(), msg);
        }
    } else {
        const std::map<int, User*>& users = server.getUsers();
        std::map<int, User*>::const_iterator it;

        for (it = users.begin(); it != users.end(); ++it) {
            if (it->second->getNickname() == target) {
                std::string msg = ":" + user->getNickname() + " NOTICE " + target + " :" + message;
                it->second->sendMessage(msg);
                break;
            }
        }
    }
}

void CommandHandler::handleQuit(User* user, const std::vector<std::string>& args) {
    std::string reason = args.empty() ? "Client Quit" : args[0];
    std::string quit_msg = ":" + user->getNickname() + " QUIT :" + reason;

    // Notify all channels the user is in
    const std::set<std::string>& channels = user->getCurrentChannels();
    std::set<std::string>::const_iterator it;
    for (it = channels.begin(); it != channels.end(); ++it) {
        Channel* channel = server.getChannel(*it);
        if (channel) {
            channel->broadcast(user->getFd(), quit_msg);
            channel->removeUser(user->getFd());
        }
    }

    server.disconnectUser(user->getFd());
}

void CommandHandler::handleKick(User* user, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        user->sendMessage(":server 461 KICK :Not enough parameters");
        return;
    }

    std::string channel_name = args[0];
    std::string target_nick = args[1];
    std::string reason = args.size() > 2 ? args[2] : user->getNickname();

    Channel* channel = server.getChannel(channel_name);
    if (!channel) {
        user->sendMessage(":server 403 " + channel_name + " :No such channel");
        return;
    }

    if (!channel->isOperator(user->getFd())) {
        user->sendMessage(":server 482 " + channel_name + " :You're not channel operator");
        return;
    }

    const std::map<int, User*>& users = server.getUsers();
    std::map<int, User*>::const_iterator it;
    int target_fd = -1;

    for (it = users.begin(); it != users.end(); ++it) {
        if (it->second->getNickname() == target_nick) {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1) {
        user->sendMessage(":server 401 " + target_nick + " :No such nick");
        return;
    }

    if (!channel->hasUser(target_fd)) {
        user->sendMessage(":server 441 " + target_nick + " " + channel_name + " :They aren't on that channel");
        return;
    }

    std::string kick_msg = ":" + user->getNickname() + " KICK " + channel_name + " " + target_nick + " :" + reason;
    channel->broadcast(0, kick_msg);

    channel->removeUser(target_fd);
    User* target_user = server.getUser(target_fd);
    if (target_user) {
        target_user->leaveChannel(channel_name);
    }
}

void CommandHandler::handleMode(User* user, const std::vector<std::string>& args) {
    if (args.empty()) {
        user->sendMessage(":server 461 MODE :Not enough parameters");
        return;
    }

    std::string target = args[0];
    if (target[0] == '#' || target[0] == '&') {
        // Channel mode
        Channel* channel = server.getChannel(target);
        if (!channel) {
            user->sendMessage(":server 403 " + target + " :No such channel");
            return;
        }

        if (!channel->isOperator(user->getFd())) {
            user->sendMessage(":server 482 " + target + " :You're not channel operator");
            return;
        }

        if (args.size() < 2) {
            // Show current modes
            std::stringstream ss;
            ss << ":" << server.getServerFd() << " 324 " << user->getNickname() << " " << target << " " << channel->getModeFlags();
            if (!channel->getPassword().empty()) {
                ss << " " << channel->getPassword();
            }
            if (channel->getUserLimit() > 0) {
                ss << " " << channel->getUserLimit();
            }
            user->sendMessage(ss.str());
            return;
        }

        // Handle mode changes
        std::string modes = args[1];
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
                case 'i': // Invite-only
                    channel->setInviteOnly(adding);
                    break;

                case 't': // Topic restriction
                    channel->setTopicRestricted(adding);
                    break;

                case 'k': // Channel key
                    if (adding) {
                        if (args.size() > 2) {
                            channel->setPassword(args[2]);
                        } else {
                            user->sendMessage(":server 461 MODE k :Not enough parameters");
                            return;
                        }
                    } else {
                        channel->setPassword("");
                    }
                    break;

                case 'o': // Operator privilege
                    if (args.size() > 2) {
                        const std::map<int, User*>& users = server.getUsers();
                        std::map<int, User*>::const_iterator it;
                        for (it = users.begin(); it != users.end(); ++it) {
                            if (it->second->getNickname() == args[2]) {
                                if (adding) {
                                    channel->addOperator(it->first);
                                } else {
                                    channel->removeOperator(it->first);
                                }
                                break;
                            }
                        }
                    } else {
                        user->sendMessage(":server 461 MODE o :Not enough parameters");
                        return;
                    }
                    break;

                case 'l': // User limit
                    if (adding) {
                        if (args.size() > 2) {
                            int limit = atoi(args[2].c_str());
                            if (limit > 0) {
                                channel->setUserLimit(limit);
                            }
                        } else {
                            user->sendMessage(":server 461 MODE l :Not enough parameters");
                            return;
                        }
                    } else {
                        channel->setUserLimit(0);
                    }
                    break;
            }
        }

        // Broadcast mode change
        std::string mode_msg = ":" + user->getNickname() + " MODE " + target + " " + modes;
        if (args.size() > 2) {
            mode_msg += " " + args[2];
        }
        channel->broadcast(0, mode_msg);
    } else {
        // User mode
        if (target != user->getNickname()) {
            user->sendMessage(":server 502 :Cannot change mode for other users");
            return;
        }

        if (args.size() < 2) {
            // Show current user modes
            std::stringstream ss;
            ss << ":" << server.getServerFd() << " 221 " << user->getNickname() << " " << user->getModeFlags();
            user->sendMessage(ss.str());
            return;
        }

        // Handle user mode changes
        std::string modes = args[1];
        user->setModeFlags(modes);

        // Broadcast mode change
        std::string mode_msg = ":" + user->getNickname() + " MODE " + target + " " + modes;
        user->sendMessage(mode_msg);
    }
}

void CommandHandler::handleTopic(User* user, const std::vector<std::string>& args) {
    if (args.empty()) {
        user->sendMessage(":server 461 TOPIC :Not enough parameters");
        return;
    }

    std::string channel_name = args[0];
    Channel* channel = server.getChannel(channel_name);
    if (!channel) {
        user->sendMessage(":server 403 " + channel_name + " :No such channel");
        return;
    }

    if (!channel->hasUser(user->getFd())) {
        user->sendMessage(":server 442 " + channel_name + " :You're not on that channel");
        return;
    }

    if (args.size() < 2) {
        // Show current topic
        std::string topic = channel->getTopic();
        if (topic.empty()) {
            user->sendMessage(":server 331 " + user->getNickname() + " " + channel_name + " :No topic is set");
        } else {
            user->sendMessage(":server 332 " + user->getNickname() + " " + channel_name + " :" + topic);
        }
        return;
    }

    // Set new topic
    std::string new_topic = args[1];
    channel->setTopic(new_topic);

    std::string topic_msg = ":" + user->getNickname() + " TOPIC " + channel_name + " :" + new_topic;
    channel->broadcast(0, topic_msg);
}

void CommandHandler::handleInvite(User* user, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        user->sendMessage(":server 461 INVITE :Not enough parameters");
        return;
    }

    std::string target_nick = args[0];
    std::string channel_name = args[1];

    Channel* channel = server.getChannel(channel_name);
    if (!channel) {
        user->sendMessage(":server 403 " + channel_name + " :No such channel");
        return;
    }

    if (!channel->hasUser(user->getFd())) {
        user->sendMessage(":server 442 " + channel_name + " :You're not on that channel");
        return;
    }

    const std::map<int, User*>& users = server.getUsers();
    std::map<int, User*>::const_iterator it;
    int target_fd = -1;

    for (it = users.begin(); it != users.end(); ++it) {
        if (it->second->getNickname() == target_nick) {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1) {
        user->sendMessage(":server 401 " + target_nick + " :No such nick");
        return;
    }

    if (channel->hasUser(target_fd)) {
        user->sendMessage(":server 443 " + target_nick + " " + channel_name + " :is already on channel");
        return;
    }

    // Send invite message to target user
    std::string invite_msg = ":" + user->getNickname() + " INVITE " + target_nick + " :" + channel_name;
    User* target_user = server.getUser(target_fd);
    if (target_user) {
        target_user->sendMessage(invite_msg);
    }

    // Send confirmation to inviter
    user->sendMessage(":server 341 " + user->getNickname() + " " + target_nick + " " + channel_name);
}

void CommandHandler::handlePass(User* user, const std::vector<std::string>& args) {
    if (args.empty()) {
        user->sendMessage(":server 461 * PASS :Not enough parameters");
        return;
    }

    if (user->isAuthenticated()) {
        user->sendMessage(":server 462 * :You may not reregister");
        return;
    }

    if (args[0] == server.getPassword()) {
        user->setAuthenticated(true);
    } else {
        user->sendMessage(":server 464 * :Password incorrect");
    }
}
