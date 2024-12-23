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

#define SERVERPORT 8888
#define MAXBUF 1024
#define MAXPRECONNECTIONS 5

int main(int argc, char *argv[]) {
    struct sockaddr_in serverAddr, clientAddr;
    int listensock;
    int configResult;
    fd_set masterFdSet, tmpFdSet;

    listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listensock < 0) {
        perror("Failed to create socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVERPORT);

    configResult = bind(listensock, (sockaddr*)&serverAddr, sizeof(sockaddr));
    if (configResult < 0) {
        perror("Failed to bind socket");
        return 1;
    }

    configResult = listen(listensock, MAXPRECONNECTIONS);
    if (configResult == -1) {
        perror("Failed to start listening on socket");
        return 1;
    }

    FD_ZERO(&masterFdSet);
    FD_SET(listensock, &masterFdSet);
    for (;;) {
        tmpFdSet = masterFdSet;
        int selectResult = select(FD_SETSIZE, &tmpFdSet, NULL, NULL, NULL);
        if (selectResult < 0) {
            perror("Select error");
            break;
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &tmpFdSet)) {
                if (fd == listensock) {
                    socklen_t clientSocketLen = sizeof(clientAddr);
                    int clientSocket = accept(listensock, (sockaddr*)&clientAddr, &clientSocketLen);
                    if (clientSocket == -1) {
                        perror("Failed to accept connection");
                        continue;
                    } else {
                        printf("Accepted new connection from client\n");
                    }
                    FD_SET(clientSocket, &masterFdSet);
                } else {
                    char buffer[MAXBUF];
                    int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
                    if (bytesRead <= 0) {
                        close(fd);
                        FD_CLR(fd, &masterFdSet);
                        printf("Client disconnected\n");
                        continue;
                    }

                    buffer[bytesRead] = '\0';
                    printf("Received: %s\n", buffer);
                }
            }
        }
    }

    close(listensock);
    return 0;
}
