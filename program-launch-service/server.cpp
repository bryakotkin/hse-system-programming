#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <sys/wait.h>
#include <future>

#define SERVERPORT 8888
#define MAXBUF 1024
#define MAXPRECONNECTIONS 5

void handleClient(int, std::string);

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

    constexpr int EXECUTION_TIMEOUT = 5;
    int exitCode = -1;
    std::string result;

    std::future<std::string> future = std::async(std::launch::async, [&]() -> std::string {
        int stdoutPipe[2], stderrPipe[2];
        if (pipe(stdoutPipe) < 0 || pipe(stderrPipe) < 0) {
            throw std::runtime_error("Failed to create pipes");
        }

        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error("Failed to fork process");
        }

        if (pid == 0) {
            close(stdoutPipe[0]);
            close(stderrPipe[0]);
            dup2(stdoutPipe[1], STDOUT_FILENO);
            dup2(stderrPipe[1], STDERR_FILENO);
            close(stdoutPipe[1]);
            close(stderrPipe[1]);

            execlp("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            exit(-1);
        }

        close(stdoutPipe[1]);
        close(stderrPipe[1]);

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

        int status;
        waitpid(pid, &status, 0);
        exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

        if (!errors.empty()) {
            output += "\n[stderr]\n" + errors;
        }
        return output;
    });

    if (future.wait_for(std::chrono::seconds(EXECUTION_TIMEOUT)) == std::future_status::timeout) {
        result = "Error: Command timed out.\n";
    } else {
        try {
            result = future.get();
        } catch (const std::exception &e) {
            result = std::string("Error: ") + e.what() + "\n";
        }
    }

    std::string responseStr = "Exit code: " + std::to_string(exitCode) + "\n" + "Output:\n" + result;
    int bytesWritten = write(clientSocket, responseStr.c_str(), responseStr.size());
    if (bytesWritten < 0) {
        std::cerr << "Failed to send response to client" << std::endl;
        close(clientSocket);
    }

    std::cout << "Sent result to client" << std::endl;
    close(clientSocket);
}