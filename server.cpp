#include <iostream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <vector>
using namespace std;

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 5);

    vector<pollfd> fds;
    struct pollfd serverPollfd;
    serverPollfd.fd = serverSocket;
    serverPollfd.events = POLLIN;
    serverPollfd.revents = 0;
    fds.push_back(serverPollfd);
 // Watch server socket

    char buffer[1024];

    while (true)
    {
        poll(fds.data(), fds.size(), -1); // Block until activity

        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == serverSocket)
                {
                    // New connection
                    int clientSocket = accept(serverSocket, nullptr, nullptr);
                    struct pollfd clientPollfd;
                    clientPollfd.fd = clientSocket;
                    clientPollfd.events = POLLIN;
                    clientPollfd.revents = 0;
                    fds.push_back(clientPollfd);

                    cout << "New client connected\n";
                }
                else
                {
                    int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes <= 0)
                    {
                        cout << "Client disconnected\n";
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                    else
                    {
                        buffer[bytes] = '\0';
                        cout << "Client says: " << buffer << endl;
                    }
                }
            }
        }
    }
    return 0;
}
