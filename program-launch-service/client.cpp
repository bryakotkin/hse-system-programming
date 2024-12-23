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
#define SERVERADDRESS "127.0.0.1"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cmd> <argv...>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in serverAddr;
    int sock;
    int returnStatus;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        perror("Failed to create socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVERADDRESS);
    serverAddr.sin_port = htons(SERVERPORT);

    returnStatus = connect(sock, (sockaddr*)&serverAddr, sizeof(sockaddr));
    if (returnStatus == -1) {
        perror("Failed to connect to socket");
        return 1;
    }

    const char* message = "123";
    returnStatus = write(sock, message, strlen(message));
    if (returnStatus == -1) {
        fprintf(stderr, "Could not send command to server!\n");
        exit(1);
    } else {
        printf("Sent %d bytes to server\n", returnStatus);
    }
    
    shutdown(sock, SHUT_WR);

    close(sock);
    return 0;
}
