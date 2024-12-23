#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>

#define SERVERPORT 8888
#define SERVERADDRESS "127.0.0.1"
#define MAXBUF 1024

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
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVERADDRESS);
    serverAddr.sin_port = htons(SERVERPORT);

    returnStatus = connect(sock, (sockaddr*)&serverAddr, sizeof(sockaddr));
    if (returnStatus == -1) {
        std::cerr << "Failed to connect to socket" << std::endl;
        return 1;
    }

    std::ostringstream oss;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            oss << " ";
        }
        oss << argv[i];
    }
    std::string data = oss.str();
    std::cout << "Command to sent: " << data << std::endl;
    returnStatus = write(sock, data.c_str(), data.length());
    if (returnStatus == -1) {
        std::cerr << "Could not send command to server" << std::endl;
        close(sock);
        exit(1);
    } else {
        printf("Sent %d bytes to server\n", returnStatus);
    }

    shutdown(sock, SHUT_WR);
    int counter;
    char buf[MAXBUF];
    while ((counter = read(sock, buf, MAXBUF)) > 0) {
        write(STDOUT_FILENO, buf, counter);
    }

    close(sock);
    return 0;
}
