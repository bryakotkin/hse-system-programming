#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <sys/wait.h>

#define SERVERPORT 8888
#define MAXBUF 1024
#define MAXPRECONNECTIONS 5
#define EXECUTION_TIMEOUT 10

void handleClient(int, std::string);
void timeout_handler(int sig);

int main(int argc, char *argv[]) {
    char buffer[MAXBUF];
    struct sockaddr_in serverAddr, clientAddr;
    int listensock;
    int configResult;

    listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listensock < 0) {
        std::cerr << "Failed to create socket";
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVERPORT);

    configResult = bind(listensock, (sockaddr*)&serverAddr, sizeof(sockaddr));
    if (configResult < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }

    configResult = listen(listensock, MAXPRECONNECTIONS);
    if (configResult == -1) {
        std::cerr << "Failed to start listening on socket" << std::endl;
        return 1;
    }

    for (;;) {
        int clientSocket = accept(listensock, NULL, NULL);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        std::cout << "Accepted new connection from client" << std::endl;
        int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) {
            std::cout << "Client disconnected" << std::endl;
            close(clientSocket);
            continue;
        }
        buffer[bytesRead] = '\0';
        std::thread clientThread(handleClient, clientSocket, std::string(buffer));
        clientThread.detach();
    }

    close(listensock);
    return 0;
}

void handleClient(int clientSocket, std::string command) {
    std::cout << "Received command: " << command << std::endl;

    int stdoutPipe[2], stderrPipe[2];
    if (pipe(stdoutPipe) < 0 || pipe(stderrPipe) < 0) {
        std::cerr << "Failed to start pipes";
        exit(-1);
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        std::cerr << "Failed to start sub process";
        exit(-1);
    }

    if (child_pid == 0) {
        alarm(EXECUTION_TIMEOUT);
        close(stdoutPipe[0]);
        close(stderrPipe[0]);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        close(stdoutPipe[1]);
        close(stderrPipe[1]);

        execlp("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        exit(-1);
    }

    std::cout << "Child process " << child_pid << " did start" << std::endl;

    close(stdoutPipe[1]);
    close(stderrPipe[1]);

    int status;
    waitpid(child_pid, &status, 0);

    if (WIFSIGNALED(status)) {
        std::string response = "Error: Command timed out, signal: " + std::to_string(WTERMSIG(status));
        // kill(child_pid, SIGKILL);
        int bytesWritten = send(clientSocket, response.c_str(), response.size(), 0);
        if (bytesWritten < 0)
            std::cerr << "Failed to send response to client" << std::endl;
        std::cout << "Sent timeout error to client, kill the childprocess " << child_pid << std::endl;
        close(stdoutPipe[0]);
        close(stderrPipe[0]);
        close(clientSocket);
        return;
    }

    std::string output, errors;
    char buffer[MAXBUF];
    ssize_t bytesRead;

    while ((bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    close(stdoutPipe[0]);

    while ((bytesRead = read(stderrPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        errors += buffer;
    }
    close(stderrPipe[0]);

    if (!errors.empty()) {
        output += "\n[Errors]\n" + errors;
    }

    std::string responseStr = "Exit code: " + std::to_string(WIFEXITED(status)) + "\n" + "Output:\n" + output;
    int bytesWritten = send(clientSocket, responseStr.c_str(), responseStr.size(), 0);
    if (bytesWritten < 0) {
        std::cerr << "Failed to send response to client" << std::endl;
        close(clientSocket);
    }

    std::cout << "Sent result to client" << std::endl;
    close(clientSocket);
}