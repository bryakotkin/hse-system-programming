#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <cstdlib>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in rAddr, sAddr;
    int listensock;
    socklen_t addrlen = sizeof(rAddr);

    listensock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listensock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int reuse = 1;
    if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        close(listensock);
        return 1;
    }

    int broadcastEnable = 1;
    if (setsockopt(listensock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Failed to enable broadcast");
        close(listensock);
        return 1;
    }

    rAddr.sin_family = AF_INET;
    rAddr.sin_port = htons(atoi(argv[1]));
    rAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int result;
    result = bind(listensock, (struct sockaddr*)&rAddr, sizeof(rAddr));
    if (result < 0) {
        perror("Error binding socket");
        close(listensock);
        return 1;
    }

    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(atoi(argv[1]));
    sAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    int clientID = getpid();
    fd_set readfds, tmpfds;
    char buffer[1024];
    FD_ZERO(&readfds);
    FD_SET(listensock, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
    int max_fd = (listensock > STDIN_FILENO ? listensock : STDIN_FILENO) + 1; // Do not use FD_SETSIZE;

    for (;;) {
        tmpfds = readfds;
        result = select(max_fd, &tmpfds, NULL, NULL, NULL);
        if (result < 0) {
            perror("Failure in select");
            close(listensock);
            break;
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &tmpfds)) {
                if (i == listensock) {
                    memset(buffer, 0, sizeof(buffer));
                    int bytesReceived = recvfrom(listensock, buffer, sizeof(buffer) - 1, 0,
                                                 reinterpret_cast<struct sockaddr*>(&rAddr), &addrlen);
                    if (bytesReceived > 0) {
                        buffer[bytesReceived] = '\0';
                        int receivedID;
                        char messageContent[1024];
                        if (sscanf(buffer, "User ID: %d\n%[^\n]", &receivedID, messageContent) == 2) {
                            if (receivedID != clientID) {
                                std::cout << "Received from " << receivedID << ": " << messageContent << std::endl;
                            }
                        }
                    } else {
                        perror("Error receiving data");
                    }
                }

                if (i == STDIN_FILENO) {
                    memset(buffer, 0, sizeof(buffer));
                    if (fgets(buffer, sizeof(buffer) - 1, stdin) != nullptr) {
                        std::string message = "User ID: " + std::to_string(clientID) + "\n" + buffer;
                        int bytesSent = sendto(listensock, message.c_str(), message.size(), 0,
                                               reinterpret_cast<struct sockaddr*>(&sAddr), sizeof(sAddr));
                        if (bytesSent < 0) {
                            perror("Error sending data");
                        }
                    }
                }
            }
        }
    }

    close(listensock);
    return 0;
}
