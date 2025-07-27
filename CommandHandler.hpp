#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "User.hpp"
#include "Server.hpp"

class CommandHandler {
private:
    Server& server;

    void handleNick(User* user, const std::vector<std::string>& args);
    void handleUser(User* user, const std::vector<std::string>& args);
    void handleJoin(User* user, const std::vector<std::string>& args);
    void handlePart(User* user, const std::vector<std::string>& args);
    void handlePrivmsg(User* user, const std::vector<std::string>& args);
    void handleQuit(User* user, const std::vector<std::string>& args);
    void handleKick(User* user, const std::vector<std::string>& args);
    void handleMode(User* user, const std::vector<std::string>& args);
    void handleTopic(User* user, const std::vector<std::string>& args);
    void handleInvite(User* user, const std::vector<std::string>& args);
    void handlePass(User* user, const std::vector<std::string>& args);
    std::vector<std::string> splitMessage(const std::string& message);
    std::vector<std::string> splitByComma(const std::string& str);
    bool isValidNickname(const std::string& nickname);
    bool isValidChannelName(const std::string& channel);
    bool isUserAuthenticated(User* user);
    bool isUserRegistered(User* user);

public:
    CommandHandler(Server& server);
    void parseMessage(User* user, const std::string& message);
    void executeCommand(User* user, const std::string& command, const std::vector<std::string>& args);
};

#endif
